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
#pragma once

#include <rtl/fourcc.hpp>

// TODO: Describe format
namespace fjord
{
    namespace format
    {
        namespace signatures
        {
            constexpr rtl::uint32_t pifs = rtl::make_fourcc( 'P', 'I', 'F', 'S' );
            constexpr rtl::uint32_t iyuv = rtl::make_fourcc( 'I', 'Y', 'U', 'V' );
            constexpr rtl::uint32_t fern = rtl::make_fourcc( 'F', 'E', 'R', 'N' );
            constexpr rtl::uint32_t fjrd = rtl::make_fourcc( 'F', 'J', 'R', 'D' );
        } // namespace signatures

        namespace versions
        {
            constexpr rtl::uint32_t v1 = 0x00000001;
            constexpr rtl::uint32_t v2 = 0x00000002;
        } // namespace versions

        namespace constraints
        {
            constexpr rtl::size_t max_channels_count = 3;
            constexpr rtl::size_t max_regions_count = 3;

            // TODO: are these constraints about the format or about the decoder?
            constexpr rtl::size_t    max_image_size = 3092;
            constexpr rtl::size_t    max_ifs_blocks_count = 8192;
            constexpr rtl::uintmax_t max_file_size = max_image_size * max_image_size;
        } // namespace constraints

#pragma pack( push, 1 )
        namespace headers
        {
            struct Image
            {
                rtl::uint32_t signature;
                rtl::uint32_t version;
                rtl::uint32_t codec;
                rtl::uint16_t image_width;
                rtl::uint16_t image_height;
                rtl::uint8_t  image_channels_count;
                rtl::uint8_t  image_count;
                rtl::uint16_t gamma;
            };

            static_assert( sizeof( Image ) == 20 );

            struct Channel
            {
                rtl::uint16_t brightness_shift;
                rtl::uint16_t contrast_shift;
            };

            static_assert( sizeof( Channel ) == 4 );

            struct IteratedFunctionSystem
            {
                rtl::uint32_t version;
                rtl::uint32_t profile_level;
                rtl::uint16_t cols;
                rtl::uint16_t rows;
                rtl::uint8_t  step;
                rtl::uint8_t  depth;
                rtl::uint8_t  iteration_count;
                rtl::uint8_t  pad1;
                rtl::uint16_t region_count;
                rtl::uint8_t  pad2;
                rtl::uint8_t  pad3;
                rtl::uint32_t block_count;
                rtl::uint32_t node_count;
            };

            static_assert( sizeof( IteratedFunctionSystem ) == 28 );

        } // namespace headers

        struct Block
        {
            static constexpr unsigned bits_per_contrast = 5;
            static constexpr unsigned bits_per_transform = 3;
            static constexpr unsigned bits_per_brightness = 8;
            static constexpr unsigned bits_per_offset = 8;

            signed   contrast : bits_per_contrast;
            unsigned transform : bits_per_transform;
            signed   brightness : bits_per_brightness;
            unsigned offset_x : bits_per_offset;
            unsigned offset_y : bits_per_offset;
        };

        static_assert( sizeof( Block ) == 4 );

#pragma pack( pop )

    } // namespace format

} // namespace fjord
