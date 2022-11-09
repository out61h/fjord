/*
 * Copyright (C) 2016-2022 Konstantin Polevik
 * All rights reserved
 *
 * This file is part of the FJORD. Redistribution and use in source and
 * binary forms, with or without modification, are permitted exclusively
 * under the terms of the MIT license. You should have received a copy of the
 * license with this file. If not, please visit:
 * https://github.com/out61h/fjord/blob/main/LICENSE.
 */
#pragma once

#include <rtl/algorithm.hpp>

#include "format.hpp"
#include "pixel.hpp"
#include "point.hpp"
#include "rect.hpp"
#include "size.hpp"
#include "symmetry.hpp"

namespace fjord
{
    using WindowFunction = Pixel( int x, int y, int w, int h );

    // TODO: constexpr
    struct Image final
    {
        void init( const Rect& rect, Pixel* pixels );

        template<typename Allocator>
        bool init( const Rect& rect, Allocator& allocator )
        {
            rectangle = rect;
            pixels = allocator.allocate( static_cast<size_t>( rect.area() ) );

            return pixels != nullptr;
        }

        /**
         * @brief Clear image pixels with zero pixel value
         */
        void clear();

        /**
         * @brief Adds pixel values of the specified \image to this image
         *
         * Uses the \image origin as a destination point.
         *
         * @param image Must be the same size as this image
         */
        void add( const Image& image );

        /**
         * @brief Multiply pixel values with image.
         *
         * @param image Must be the same size as this image
         */
        void mul( const Image& image );

        /**
         * @brief Fill image pixels with window function values
         */
        void generate( const Rect& window_rect, WindowFunction window_func );

        [[nodiscard]] constexpr Pixel* data()
        {
            return pixels;
        }

        [[nodiscard]] constexpr const Pixel* data() const
        {
            return pixels;
        }

        [[nodiscard]] constexpr Pixel* begin()
        {
            return pixels;
        }

        [[nodiscard]] constexpr Pixel* end()
        {
            return pixels + rectangle.area();
        }

        [[nodiscard]] constexpr int width() const
        {
            return rectangle.size.w;
        }

        [[nodiscard]] constexpr int height() const
        {
            return rectangle.size.h;
        }

        [[nodiscard]] constexpr const Size& size() const
        {
            return rectangle.size;
        }

        [[nodiscard]] constexpr const Point& origin() const
        {
            return rectangle.origin;
        }

        [[nodiscard]] constexpr const Rect& rect() const
        {
            return rectangle;
        }

        [[nodiscard]] constexpr const Pixel& at( int x, int y ) const
        {
            return *( pixels + y * width() + x );
        }

        [[nodiscard]] constexpr Pixel& at( int x, int y )
        {
            return *( pixels + y * width() + x );
        }

        Rect   rectangle;
        Pixel* pixels;
    };

    namespace image
    {
        void affine_transformation( const Image& source,
                                    const Rect&  translation,
                                    Pixel        contrast,
                                    Pixel        brightness,
                                    Symmetry     symmetry,
                                    Image&       output );

        void expand_borders( const Image& sorce, Image& output );

        void crop_resize_adjust( const Image& source,
                                 const Rect&  crop,
                                 Pixel        contrast,
                                 Pixel        brightness,
                                 Image&       output );

        void convert_yuv444_to_rgb888( const Image&  y_image,
                                       const Image&  u_image,
                                       const Image&  v_image,
                                       rtl::uint8_t* rgb_pixels,
                                       rtl::size_t   rgb_pixels_padding );

        void clear_rgb888( rtl::uint8_t* pixels, int width, int height, rtl::size_t padding );

        void dim_region_rgb888( rtl::uint8_t* pixels,
                                int           width,
                                int           height,
                                rtl::size_t   padding,
                                const Rect&   region );
    } // namespace image
} // namespace fjord
