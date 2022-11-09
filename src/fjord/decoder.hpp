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

#include <rtl/allocator.hpp>
#include <rtl/random.hpp>

#include "block.hpp"
#include "format.hpp"
#include "image.hpp"
#include "windows.hpp"

namespace fjord
{
    class Decoder final
    {
    public:
        /**
         * @brief Constructor.
         *
         * @note Constructor is defaulting to make able the class instance be declared as global
         * static variable. Use \reset method for "cold" initialization.
         */
        Decoder() = default;

        void reset();

        unsigned load( const rtl::uint8_t* data, const Size& target_size, Size* source_size );

        enum class PixelFormat
        {
            rgb888
        };

        void decode( unsigned      num_iterations,
                     PixelFormat   fmt,
                     rtl::uint8_t* buffer_pixels,
                     int           buffer_width,
                     int           buffer_height,
                     rtl::size_t   buffer_pitch );

    private:
        const Image* iterate( unsigned num_iterations );

        static constexpr auto overlap_factor_denominator = 4; // ~ 1/4 = 25% block overlap
        static constexpr auto noise_intensivity_log2 = 4;     // [0..7]
        static constexpr auto random_cycle_length = 4096;

        using RandomGenerator = rtl::random<random_cycle_length>;
        using SmoothWindow = windows::Trapezoidal<overlap_factor_denominator>;

        using ImageInfo = format::headers::Image;
        using ChannelInfo = format::headers::Channel;
        using FractalInfo = format::headers::IteratedFunctionSystem;

        static constexpr auto max_regions_count{ format::constraints::max_regions_count };
        static constexpr auto brightness_bits{ format::Block::bits_per_brightness };
        static constexpr auto contrast_bits{ format::Block::bits_per_contrast };

        static constexpr auto max_blocks_count{ format::constraints::max_ifs_blocks_count };
        static constexpr auto max_channels_count{ format::constraints::max_channels_count };
        static constexpr auto max_image_size{ format::constraints::max_image_size };

        enum Buffer
        {
            buffer_ifs_1st,
            buffer_ifs_2nd,
            buffer_ifs_mask,
            buffer_ifs_count,

            buffer_output_channel_base = buffer_ifs_count,
            buffer_output_channel_y = buffer_ifs_count,
            buffer_output_channel_u,
            buffer_output_channel_v,

            buffer_count,
        };

        static constexpr auto buffer_page_size = max_image_size * max_image_size;

        // Total pixel buffers size is the sum of the following components:
        // - working buffers with size NxN each
        // - original image buffers for range blocks with total size (NxN)
        // - smooth window and bordered image buffers for range blocks - TODO: proove size
        //
        // where N - maximal image size

        static constexpr auto expand_factor
            = overlap_factor_denominator * overlap_factor_denominator;

        // TODO: round up?
        static constexpr auto allocator_size
            = buffer_page_size * ( buffer_count + 1 + ( expand_factor + 4 ) / expand_factor );

        using Allocator = rtl::allocators::grow_only<Pixel, allocator_size>;

        Allocator m_allocator;

        const ImageInfo*   m_image_info;
        const ChannelInfo* m_channels_info;
        Rect               m_regions[max_regions_count];

        const FractalInfo* m_ifs_info;
        Size               m_ifs_size;
        RangeBlock         m_ifs_blocks[max_blocks_count];
        unsigned           m_ifs_nodes[max_blocks_count];
        int                m_ifs_block_size_ilog2;

        // NOTE: Merging different images into the single array is reducing the size of the code
        Image m_buffer_images[buffer_count];

        Buffer m_ifs_last_output_buffer;

        Size m_output_image_size;

        RandomGenerator m_random;
    };
} // namespace fjord
