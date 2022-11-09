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

#include <rtl/algorithm.hpp>

#include "point.hpp"
#include "size.hpp"

namespace fjord
{
    struct Rect
    {
        Point origin;
        Size  size;

        [[nodiscard]] static constexpr Rect create( int x, int y, int w, int h )
        {
            return Rect{ { x, y }, { w, h } };
        }

        [[nodiscard]] constexpr int left() const
        {
            return origin.x;
        }

        [[nodiscard]] constexpr int right() const
        {
            return origin.x + size.w;
        }

        [[nodiscard]] constexpr int top() const
        {
            return origin.y;
        }

        [[nodiscard]] constexpr int bottom() const
        {
            return origin.y + size.h;
        }

        [[nodiscard]] constexpr int area() const
        {
            return size.w * size.h;
        }

        [[nodiscard]] constexpr bool null() const
        {
            return !size.w || !size.h;
        }

        [[nodiscard]] constexpr bool square() const
        {
            return size.w && size.h && size.w == size.h;
        }

        [[nodiscard]] constexpr Rect expand( const Size& border_size ) const
        {
            return Rect{ { origin.x - border_size.w, origin.y - border_size.h },
                         { size.w + ( border_size.w << 1 ), size.h + ( border_size.h << 1 ) } };
        }

        [[nodiscard]] constexpr bool contains( const Point& point ) const
        {
            return left() <= point.x && point.x < right() && top() <= point.y && point.y < bottom();
        }
    };

    constexpr Rect operator&( const Rect& lhs, const Rect& rhs )
    {
        const int l = rtl::max( lhs.left(), rhs.left() );
        const int t = rtl::max( lhs.top(), rhs.top() );

        const int r = rtl::min( lhs.right(), rhs.right() );
        const int b = rtl::min( lhs.bottom(), rhs.bottom() );

        if ( r >= l && b >= t )
            return Rect::create( l, t, r - l, b - t );

        return Rect::create( 0, 0, 0, 0 );
    }

} // namespace fjord
