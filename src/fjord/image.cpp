/*
 * Copyright (C) 2016-2022 Konstantin Polevik
 * All rights reserved
 *
 * This file is part of the FJORD. Redistribution and use in source and
 * binary forms, with or without modification, are permitted exclusively
 * under the terms of the MIT license. You should have received a copy of the
 * license with this file. If not, please visit:
 * https://github.com/out61h/fjord/blob/master/LICENSE
 */
#include "image.hpp"

#include <rtl/algorithm.hpp>
#include <rtl/sys/debug.hpp>

using namespace fjord;

void Image::init( const Rect& rect, Pixel* p )
{
    rectangle = rect;
    pixels = p;
}

void Image::generate( const Rect& window_rect, WindowFunction window_func )
{
    const Point origin = Point::create( rect().origin.x - window_rect.origin.x,
                                        rect().origin.y - window_rect.origin.y );
    RTL_ASSERT( origin.x >= 0 && origin.y >= 0 );

    auto* dst = data();

    // NOTE: Scans point coordinates from 0x0 to NxM, where NxM - window function size
    // NOTE: Window coordinates clipped by \rect
    for ( int y = origin.y; y < origin.y + height(); ++y )
        for ( int x = origin.x; x < origin.x + width(); ++x )
            *dst++ = window_func( x, y, window_rect.size.w, window_rect.size.h );
}

void Image::clear()
{
    rtl::fill_n( pixels, rectangle.area(), Pixel( 0 ) );
}

void Image::add( const Image& image )
{
    RTL_ASSERT( image.origin().x >= 0 );
    RTL_ASSERT( image.origin().y >= 0 );
    RTL_ASSERT( image.origin().x + image.width() <= width() );
    RTL_ASSERT( image.origin().y + image.height() <= height() );

    const auto* src = image.data();

    auto* dst = data() + image.origin().x + image.origin().y * width();
    auto  dst_pad = width() - image.width();

    for ( int y = 0; y < image.height(); ++y )
    {
        for ( int x = 0; x < image.width(); ++x )
            *dst++ += *src++;

        dst += dst_pad;
    }
}

void Image::mul( const Image& image )
{
    RTL_ASSERT( size() == image.size() );

    auto* src = image.data();
    auto* dst = data();
    auto  dst_end = data() + rectangle.area();
    for ( ; dst < dst_end; ++dst )
        *dst = *dst * *src++;
}

void fjord::image::expand_borders( const Image& source, Image& output )
{
    // TODO: implement +/- operators for Point
    const Point origin = Point::create( ( output.origin().x - source.origin().x ),
                                        ( output.origin().y - source.origin().y ) );

    RTL_ASSERT( source.rect().area() > 0 );

    for ( int y = 0; y < output.height(); ++y )
    {
        for ( int x = 0; x < output.width(); ++x )
        {
            const int src_x = rtl::clamp( x + origin.x, 0, source.width() - 1 );
            const int src_y = rtl::clamp( y + origin.y, 0, source.height() - 1 );

            output.at( x, y ) = source.at( src_x, src_y );
        }
    }
}

// clang-format off
static constexpr rtl::int8_t transform_matrices [(int)Symmetry::count][8] 
{
    // TODO: comment
    { 1,  0,  0,  1,   0, 0, 0,   0},
    { 0, -1,  1,  0,   1, 1, 0,   0},
    {-1,  0,  0, -1,   0, 1, 1,   0},
    { 0,  1, -1,  0,   1, 0, 1,   0},
    {-1,  0,  0,  1,   0, 1, 0,   0},
    { 0,  1,  1,  0,   1, 0, 0,   0},
    { 1,  0,  0, -1,   0, 0, 1,   0},
    { 0, -1, -1,  0,   1, 1, 1,   0}
};
// clang-format on

void fjord::image::affine_transformation( const Image& source,
                                          const Rect&  translation,
                                          Pixel        contrast,
                                          Pixel        brightness,
                                          Symmetry     symmetry,
                                          Image&       output )
{
    RTL_ASSERT( translation.area() > 0 );
    RTL_ASSERT( translation.left() >= 0 );
    RTL_ASSERT( translation.top() >= 0 );
    RTL_ASSERT( translation.right() <= source.width() );
    RTL_ASSERT( translation.bottom() <= source.height() );

    const int w = output.width();
    const int h = output.height();

    // TODO: check input ranges

    const auto* m = transform_matrices[static_cast<int>( symmetry )];

    Size result_size;

    if ( m[4] )
    {
        result_size.w = h;
        result_size.h = w;
    }
    else
    {
        result_size.w = w;
        result_size.h = h;
    }

    // TODO: check output ranges
    const int m5 = (int)m[5] * ( result_size.w - 1 );
    const int m6 = (int)m[6] * ( result_size.h - 1 );

    for ( int y = 0; y < h; ++y )
    {
        for ( int x = 0; x < w; ++x )
        {
            const Pixel pixel = source.at( x * translation.size.w / w + translation.origin.x,
                                           y * translation.size.h / h + translation.origin.y );

            const int dst_x = x * (int)m[0] + y * (int)m[1] + m5;
            const int dst_y = x * (int)m[2] + y * (int)m[3] + m6;

            output.at( dst_x, dst_y ) = pixel::clamp( contrast * pixel + brightness );
        }
    }
}

void fjord::image::crop_resize_adjust( const Image& source,
                                       const Rect&  crop,
                                       Pixel        contrast,
                                       Pixel        brightness,
                                       Image&       output )
{
    RTL_ASSERT( crop.area() > 0 );
    RTL_ASSERT( crop.left() >= 0 );
    RTL_ASSERT( crop.top() >= 0 );
    RTL_ASSERT( crop.right() <= source.width() );
    RTL_ASSERT( crop.bottom() <= source.height() );

    auto* dst = output.data();

    for ( int y = 0; y < output.height(); ++y )
    {
        for ( int x = 0; x < output.width(); ++x )
        {
            // TODO: Use bilinear filtering
            const Pixel pixel = source.at( x * crop.size.w / output.width() + crop.origin.x,
                                           y * crop.size.h / output.height() + crop.origin.y );

            *dst++ = pixel::clamp( contrast * pixel + brightness );
        }
    }
}

void fjord::image::convert_yuv444_to_rgb888( const Image&  y_image,
                                             const Image&  u_image,
                                             const Image&  v_image,
                                             rtl::uint8_t* rgb_pixels,
                                             rtl::size_t   rgb_pixels_padding )
{
    RTL_ASSERT( y_image.rect().area() == u_image.rect().area() );
    RTL_ASSERT( u_image.rect().area() == v_image.rect().area() );

    const int width = y_image.width();
    const int height = y_image.height();

    const auto* py = y_image.data();
    const auto* pu = u_image.data();
    const auto* pv = v_image.data();

    for ( int cy = 0; cy < height; ++cy )
    {
        for ( int cx = 0; cx < width; ++cx )
        {
            const auto y = *py++;
            const auto u = *pu++ - 0.5f;
            const auto v = *pv++ - 0.5f;

            *rgb_pixels++ = pixel::to_uint8( y + u * 2.03211f );
            *rgb_pixels++ = pixel::to_uint8( y - u * 0.39465f - v * 0.58060f );
            *rgb_pixels++ = pixel::to_uint8( y + v * 1.13983f );
        }

        rgb_pixels += rgb_pixels_padding;
    }
}

void fjord::image::dim_region_rgb888( rtl::uint8_t* pixels,
                                      int           width,
                                      int           height,
                                      rtl::size_t   padding,
                                      const Rect&   region )
{
    for ( int y = 0; y < height; ++y )
    {
        for ( int x = 0; x < width; ++x )
        {
            const int shift = region.contains( Point::create( x, y ) ) ? 0 : 1;

            *pixels++ >>= shift;
            *pixels++ >>= shift;
            *pixels++ >>= shift;
        }

        pixels += padding;
    }
}

void fjord::image::clear_rgb888( rtl::uint8_t* pixels, int width, int height, rtl::size_t padding )
{
    for ( int y = 0; y < height; ++y )
    {
        for ( int x = 0; x < width; ++x )
        {
            *pixels++ = 0;
            *pixels++ = 0;
            *pixels++ = 0;
        }

        pixels += padding;
    }
}
