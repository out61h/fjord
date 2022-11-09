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
#include <rtl/fix.hpp>

namespace fjord
{
    using Pixel = rtl::fix<rtl::int32_t, 8>;

    static_assert( sizeof( Pixel ) == 4 );

    namespace pixel
    {
        [[nodiscard]] constexpr Pixel clamp( Pixel value )
        {
            return rtl::clamp( value, Pixel( 0 ), Pixel( 1 ) );
        }

        [[nodiscard]] constexpr rtl::uint8_t to_uint8( Pixel value )
        {
            return static_cast<rtl::uint8_t>(
                rtl::clamp( static_cast<int>( value * 255 ), 0, 255 ) );
        }
    } // namespace pixel
} // namespace fjord
