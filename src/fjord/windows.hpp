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

#include "pixel.hpp"

namespace fjord
{
    // TODO: or window?
    namespace windows
    {
        /**
         * @brief One-dimension kernels, which takes the argument in the range [0;1).
         */
        namespace kernels
        {
            /*
             *         +++++++++++++
             *        /|     |     |\
             *       /               \
             *      /  |     |     |  \
             *     /                   \
             *    /    |     |     |    \
             * --0-----k----0.5--(1-k)---1--> x
             *
             */

            // TODO: comment relationship between k and overlap factor
            [[nodiscard]] constexpr Pixel trapezoidal( const Pixel x, const Pixel roi_factor )
            {
                const Pixel y
                    = pixel::clamp( ( Pixel( 1 ) - rtl::abs( x - 0.5f ) * 2 ) * roi_factor );
                return y * y;
            }
        } // namespace kernels

        /**
         * @brief A rectangular window.
         *
         * +---------+
         * |         |
         * |         |
         * |         |
         * |         |
         *
         */
        struct Rectangular
        {
            static constexpr Rect window_size( const Rect& roi )
            {
                return roi;
            }

            [[nodiscard]] static constexpr Pixel window_function( int, int, int, int )
            {
                return Pixel( 1 );
            }
        };

        /**
         * @brief A trapezoidal window with quadratic slopes.
         *
         *     +--+
         *    /    \
         *   /      \
         * _/        \_
         *
         */
        template<int OverlapFactorDenominator>
        struct Trapezoidal
        {
            static constexpr Rect window_size( const Rect& roi )
            {
                return roi.expand( Size::create( roi.size.w / OverlapFactorDenominator,
                                                 roi.size.h / OverlapFactorDenominator ) );
            }

            [[nodiscard]] static constexpr Pixel window_function( int x,
                                                                  int y,
                                                                  int width,
                                                                  int height )
            {
                // TODO: comment equation
                constexpr Pixel factor = Pixel( 1 + OverlapFactorDenominator / 2 );

                return kernels::trapezoidal( Pixel( x ) / width, factor )
                       * kernels::trapezoidal( Pixel( y ) / height, factor );
            }
        };
    } // namespace windows

} // namespace fjord
