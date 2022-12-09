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
#include "decoder.hpp"
#include "quadtree.hpp"

#include <rtl/math.hpp>
#include <rtl/sys/debug.hpp>

using namespace fjord;

namespace
{
    template<int BitCount, typename Value>
    [[nodiscard]] constexpr Value dequantize( int q_value, Value max_value )
    {
        static_assert( BitCount > 1 );
        static_assert( BitCount < sizeof( q_value ) * 8 );

        constexpr int quantizer = ( 1 << ( BitCount - 1 ) ) - 1;

        return max_value * q_value / quantizer;
    }
} // namespace

void Decoder::reset()
{
    m_random.init( 1337 );
    m_ifs_last_output_buffer = buffer_ifs_1st;
}

unsigned Decoder::load( const rtl::uint8_t* data, const Size& target_size, Size* source_size )
{
    m_allocator.reset();

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Reading image info..." );

        m_image_info = reinterpret_cast<const ImageInfo*>( data );
        data += sizeof( ImageInfo );

        RTL_ASSERT( m_image_info->signature == format::signatures::pifs );
        RTL_ASSERT( m_image_info->version == format::versions::v2 );
        RTL_ASSERT( m_image_info->codec == format::signatures::iyuv );
        RTL_ASSERT( m_image_info->image_channels_count <= format::constraints::max_channels_count );
        RTL_ASSERT( m_image_info->image_count == 1 );
        RTL_ASSERT( m_image_info->gamma == 65535 ); // TODO: define magic number as constant

        RTL_LOG( "Size: %ix%i", m_image_info->image_width, m_image_info->image_height );
        RTL_LOG( "Channels: %i (YUV420)", m_image_info->image_channels_count );
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Reading channels info..." );

        m_channels_info = reinterpret_cast<const ChannelInfo*>( data );
        data += sizeof( ChannelInfo ) * m_image_info->image_channels_count;
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Reading iterated function system format signature..." );

        RTL_ASSERT( *reinterpret_cast<const rtl::uint32_t*>( data ) == format::signatures::fjrd );
        data += sizeof( rtl::uint32_t );
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Reading iterated function system info..." );

        m_ifs_info = reinterpret_cast<const FractalInfo*>( data );
        data += sizeof( FractalInfo );

        RTL_ASSERT( m_ifs_info->region_count <= format::constraints::max_regions_count );

        m_ifs_block_size_ilog2 = m_ifs_info->step;

        m_ifs_size.w = m_ifs_info->cols << m_ifs_block_size_ilog2;
        m_ifs_size.h = m_ifs_info->rows << m_ifs_block_size_ilog2;

        RTL_LOG( "Regions: %i", m_ifs_info->region_count );
        RTL_LOG( "Blocks: %i", m_ifs_info->block_count );
        RTL_LOG( "Nodes: %i", m_ifs_info->node_count );
        RTL_LOG( "Iterations: %i", m_ifs_info->iteration_count );

        RTL_LOG( "Grid size: %ix%i blocks", m_ifs_info->cols, m_ifs_info->rows );
        RTL_LOG( "Block size: %ix%i", 1 << m_ifs_block_size_ilog2, 1 << m_ifs_block_size_ilog2 );
        RTL_LOG( "Image size: %ix%i", m_ifs_size.w, m_ifs_size.h );

        // too many blocks to fit in buffers
        if ( m_ifs_info->block_count > max_blocks_count )
            return 0;

        // image too big
        if ( m_image_info->image_width * m_image_info->image_height > buffer_page_size )
            return 0;

        // TODO: disclose format headers?
        if ( source_size )
            *source_size = Size::create( m_image_info->image_width, m_image_info->image_height );

        using Fixed = rtl::fix<rtl::int32_t, 16>;

        const Fixed scale
            = rtl::min( Fixed::from_fraction( target_size.w, (int)m_image_info->image_width ),
                        Fixed::from_fraction( target_size.h, (int)m_image_info->image_height ) );

        RTL_LOG( "Target size: %ix%i", target_size.w, target_size.h );
        // TODO: %f
        RTL_LOG( "Target scale: %i/%i", static_cast<int>( scale ) * 256, 256 );

        if ( scale < 1 )
        {
            m_output_image_size.w = static_cast<int>( scale * (int)m_image_info->image_width );
            m_output_image_size.h = static_cast<int>( scale * (int)m_image_info->image_height );
        }
        else
        {
            m_output_image_size.w = m_image_info->image_width;
            m_output_image_size.h = m_image_info->image_height;
        }

        RTL_ASSERT( m_output_image_size.w <= format::constraints::max_image_size );
        RTL_ASSERT( m_output_image_size.h <= format::constraints::max_image_size );

        // TODO: Implement scaling modes:
        // - original size
        // - fit large images to specified size (with native pow2 downscaling)
        // - fit small images to specified size (with native pow2 upscaling)
        // - fit all images to specified size (with native pow2 scaling)

        // TODO: native pow2 downscaling if image doesn't fit into decoder buffers
        RTL_LOG( "Output size: %ix%i", m_output_image_size.w, m_output_image_size.h );
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Reading image regions..." );

        // NOTE: Region geometry decoding is a bit tricky. All for the glory of the executable file
        // size reducing
        static_assert( sizeof( Rect ) / sizeof( int ) == 4 );

        // TODO: Use union of Rect and int[4]
        int* regions = reinterpret_cast<int*>( m_regions );

        for ( unsigned i = 0; i < m_ifs_info->region_count * 4u; ++i )
        {
            const rtl::uint16_t value = *reinterpret_cast<const rtl::uint16_t*>( data );
            regions[i] = value << m_ifs_block_size_ilog2;

            data += sizeof( rtl::uint16_t );
        }

#if RTL_ENABLE_LOG
        for ( auto region_index = 0; region_index < m_ifs_info->region_count; ++region_index )
        {
            const auto& region = m_regions[region_index];

            RTL_LOG( "Region #%i: %i,%i %ix%i",
                     region_index,
                     region.left(),
                     region.top(),
                     region.size.w,
                     region.size.h );
        }
#endif
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Reading blocks..." );

        const auto qx
            = static_cast<unsigned>( rtl::max( ( rtl::ceil_log2_i( m_ifs_size.w ) - 8 ), 1 ) );

        const auto qy
            = static_cast<unsigned>( rtl::max( ( rtl::ceil_log2_i( m_ifs_size.h ) - 8 ), 1 ) );

        RTL_LOG( "Block offset granularity: %ix%i", 1 << qx, 1 << qy );

        for ( rtl::uint32_t block_index = 0; block_index < m_ifs_info->block_count; ++block_index )
        {
            auto& block = m_ifs_blocks[block_index];

            const format::Block* b = reinterpret_cast<const format::Block*>( data );

            block.transform.contrast = dequantize<contrast_bits>( b->contrast, Pixel( 1 ) );

            block.transform.symmetry = static_cast<Symmetry>( b->transform );

            const auto max_brightness = Pixel( 1 ) + rtl::abs( block.transform.contrast );
            block.transform.brightness
                = dequantize<brightness_bits>( b->brightness, max_brightness );

            block.transform.geometry.origin.x = static_cast<int>( b->offset_x << qx );
            block.transform.geometry.origin.y = static_cast<int>( b->offset_y << qy );

            data += sizeof( format::Block );
        }
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Reading Q-tree partition nodes..." );

        for ( rtl::size_t node_index = 0;; )
        {
            unsigned partition_mask = *(const rtl::uint8_t*)( data );

            for ( rtl::size_t bit_index = 0; bit_index < 8 * sizeof( rtl::uint8_t ); ++bit_index )
            {
                // NOTE: We spend 4 bytes to store 1 bit flag but it's noticeably reduce the size of
                // assembled code that works with this flag
                m_ifs_nodes[node_index] = partition_mask & 1u;
                partition_mask >>= 1;

                // NOTE: Tricky code to reduce the size of compiled binary
                if ( ++node_index >= m_ifs_info->node_count )
                    goto label;
            }

            data++;
        }
    }

label:

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Decoding block sizes from Q-tree partition nodes..." );

        Quadtree<Allocator> quadtree;
        quadtree.decode( &m_allocator,
                         m_ifs_nodes,
                         m_ifs_info->cols,
                         m_ifs_info->rows,
                         1 << m_ifs_block_size_ilog2,
                         m_ifs_info->depth,
                         m_ifs_blocks );
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Init image buffers..." );

        for ( int i = 0; i < buffer_ifs_count; ++i )
        {
            m_buffer_images[i].init( Rect::create( 0, 0, m_ifs_size.w, m_ifs_size.h ),
                                     m_allocator );
        }

        for ( int i = 0; i < m_image_info->image_channels_count; ++i )
        {
            m_buffer_images[i + buffer_output_channel_base].init(
                Rect::create( 0, 0, m_output_image_size.w, m_output_image_size.h ), m_allocator );
        }
    }

    //----------------------------------------------------------------------------------------------
    {
        RTL_LOG( "Preparing the decoding context for blocks..." );
        Image& mask_image = m_buffer_images[buffer_ifs_mask];
        mask_image.clear();

        for ( size_t block_index = 0; block_index < m_ifs_info->block_count; ++block_index )
        {
            RangeBlock& block = m_ifs_blocks[block_index];

            // The size of a block's domain area is twice its own size
            {
                block.transform.geometry.size.w = block.original_image.width() << 1;
                block.transform.geometry.size.h = block.original_image.height() << 1;

#if FJORD_ENABLE_BLOCKS_DUMP
                RTL_LOG( "Block #%i: %i:%i %ix%i %i %i*x+%i",
                         block_index,
                         block.transform.geometry.origin.x,
                         block.transform.geometry.origin.y,
                         block.transform.geometry.size.w,
                         block.transform.geometry.size.h,
                         block.transform.symmetry,
                         block.transform.contrast,
                         block.transform.brightness );
#endif
                RTL_ASSERT( block.transform.geometry.left() >= 0 );
                RTL_ASSERT( block.transform.geometry.right() <= m_ifs_size.w );
                RTL_ASSERT( block.transform.geometry.top() >= 0 );
                RTL_ASSERT( block.transform.geometry.bottom() <= m_ifs_size.h );
            }

            // Preparing the blur mask for image deblocking

            // Block geometry including border clipped by region boundaries and image area
            Rect clipped_bordered_rect = Rect::create( 0, 0, 0, 0 );
            {
                // Block geometry including border with replicated pixels for blurring.
                // The larger the block size, the more blurred its boundaries
                const Rect bordered_rect = SmoothWindow::window_size( block.original_image.rect() );

                // Bordered block geometry clipped by image area
                const Rect bordered_rect_clipped_by_image_area = bordered_rect & mask_image.rect();

                // Clipping bordered block geometry by regions - to avoid interference of color
                // components located in them.
                for ( int i = 0; i < m_ifs_info->region_count; ++i )
                {
                    const Rect rect = bordered_rect_clipped_by_image_area & m_regions[i];

                    // We are looking for a region in which most of the block area is located
                    if ( rect.area() > clipped_bordered_rect.area() )
                        clipped_bordered_rect = rect;
                }

                // No regions? Just use clipping by image area
                if ( !clipped_bordered_rect.area() )
                    clipped_bordered_rect = bordered_rect_clipped_by_image_area;

                // Allocating and generating the window image
                if ( !block.window_image.init( clipped_bordered_rect, m_allocator ) )
                    return 0;

                block.window_image.generate( bordered_rect, SmoothWindow::window_function );

                // Allocating buffer for adding borders to the block image
                if ( !block.bordered_image.init( clipped_bordered_rect, m_allocator ) )
                    return 0;
            }

            // Adding the bluring window of a block to the mask
            mask_image.add( block.window_image );
        }

        RTL_LOG( "Inverting the blur mask for deblocking..." );
        rtl::transform( mask_image.begin(),
                        mask_image.end(),
                        []( const Pixel& pix )
                        {
                            // NOTE: clamping pixel value to the minimal value of the fixed point
                            // number to avoid division by zero
                            return Pixel( 1 ) / rtl::clamp( pix, Pixel::min(), Pixel::max() );
                        } );
    }

    return m_ifs_info->iteration_count;
}

const Image* Decoder::iterate( unsigned num_iterations )
{
    RTL_LOG( "Iterating the function system..." );

    const Image& mask_image = m_buffer_images[buffer_ifs_mask];

    const Image* result = nullptr;

    for ( unsigned n = 0; n < num_iterations; ++n )
    {
        Image* input_image = &m_buffer_images[m_ifs_last_output_buffer];
        Image* output_image = &m_buffer_images[buffer_ifs_2nd - m_ifs_last_output_buffer];

        // Clearing the output buffer
        output_image->clear();

        for ( unsigned i = 0; i < m_ifs_info->block_count; ++i )
        {
            auto& block = m_ifs_blocks[i];

            // Crop, resize, adjust and transform the block of the input image
            image::transform_affinity( *input_image,
                                       block.transform.geometry,
                                       block.transform.contrast,
                                       block.transform.brightness,
                                       block.transform.symmetry,
                                       block.original_image );

            // Expands image with a border replicating boundary pixels
            image::expand_borders( block.original_image, block.bordered_image );

            // Bluring block boundaries (deblocking)
            block.bordered_image.mul( block.window_image );

            // Add bordered block to the output buffer
            output_image->add( block.bordered_image );
        }

        // Normalize the output image after block boundaries bluring
        output_image->mul( mask_image );

        // Add some uniform noise to the output image for visual sharpening
        rtl::transform( output_image->begin(),
                        output_image->end(),
                        [this]( const Pixel& pix )
                        {
                            constexpr auto noise_intensivity = 1 << noise_intensivity_log2;
                            return pix
                                   + Pixel::from_fraction(
                                       (signed)( m_random.rand() & ( noise_intensivity - 1 ) )
                                           - noise_intensivity / 2,
                                       256 );
                        } );

        // Flip buffers
        result = output_image;

        m_ifs_last_output_buffer = static_cast<Buffer>( buffer_ifs_2nd - m_ifs_last_output_buffer );
    }

    return result;
}

void Decoder::decode( unsigned                     num_iterations,
                      [[maybe_unused]] PixelFormat fmt,
                      rtl::uint8_t*                buffer_pixels,
                      int                          buffer_width,
                      int                          buffer_height,
                      rtl::size_t                  buffer_pitch_in_bytes )
{
    RTL_ASSERT( fmt == PixelFormat::rgb888 );

    const fjord::Image* decoded_image = iterate( num_iterations );
    {
        RTL_LOG( "Converting YUV420 to YUV444..." );

        // Extracting channel components from the decoded image
        // +--------+-------+--------+
        // | Y              | U      |
        // |                |        |
        // +        +       +--------+
        // |                | V      |
        // |                |        |
        // +--------+-------+--------+
        const auto half_width = decoded_image->width() / 3;
        const auto half_height = decoded_image->height() / 2;

        const fjord::Rect channel_rects[max_channels_count]{
            Rect::create( 0, 0, half_width << 1, half_height << 1 ),
            Rect::create( half_width << 1, 0, half_width, half_height ),
            Rect::create( half_width << 1, half_height, half_width, half_height ) };

        // TODO: use rtl::fix<rtl::uint16_t, 16>?
        constexpr int uint16_max_value = ( ( 1 << sizeof( rtl::uint16_t ) * 8 ) - 1 );

        for ( auto i = 0; i < m_image_info->image_channels_count; ++i )
        {
            const auto& channel_info = m_channels_info[i];

            const Pixel output_contrast
                = Pixel::from_fraction( channel_info.contrast_shift, uint16_max_value );

            const Pixel output_brightness
                = Pixel::from_fraction( channel_info.brightness_shift, uint16_max_value );

            auto& output_image = m_buffer_images[i + buffer_output_channel_base];

            // TODO: %f for adjust
            RTL_LOG( "Channel #%i: Crop(%i,%i %ix%i) -> Resize(%ix%i) -> Adjust(x*%i/256+%i)",
                     i,
                     channel_rects[i].left(),
                     channel_rects[i].top(),
                     channel_rects[i].size.w,
                     channel_rects[i].size.h,
                     m_output_image_size.w,
                     m_output_image_size.h,
                     static_cast<int>( output_contrast * 256 ),
                     static_cast<int>( output_brightness * 256 ) );

            image::crop_resize_adjust( *decoded_image,
                                       channel_rects[i],
                                       output_contrast,
                                       output_brightness,
                                       output_image );
        }
    }

    {
        RTL_LOG( "Converting YUV444 to RGB888..." );

        constexpr auto rgb888_size_in_bytes = 3;

        image::clear_rgb888( buffer_pixels,
                             buffer_width,
                             buffer_height,
                             buffer_pitch_in_bytes - buffer_width * rgb888_size_in_bytes );

        image::convert_yuv444_to_rgb888( m_buffer_images[buffer_output_channel_y],
                                         m_buffer_images[buffer_output_channel_u],
                                         m_buffer_images[buffer_output_channel_v],
                                         buffer_pixels,
                                         buffer_width,
                                         buffer_height,
                                         buffer_pitch_in_bytes );
    }
}
