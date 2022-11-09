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

#include "block.hpp"
#include "image.hpp"

namespace fjord
{
    template<typename Allocator>
    class Quadtree final
    {
    public:
        Quadtree() = default;

        void decode( Allocator*  pixelsAllocator,
                     unsigned*   nodes,
                     int         col_count,
                     int         row_count,
                     int         block_size,
                     int         max_depth,
                     RangeBlock* blocks )
        {
            allocator = pixelsAllocator;
            current_block = blocks;
            current_node = nodes;

            walk( 0, 0, col_count, row_count, block_size, max_depth );
        }

    private:
        void walk( int x0, int y0, int cols, int rows, int block_size, int level )
        {
            for ( int y = y0; y < rows * block_size + y0; y += block_size )
            {
                for ( int x = x0; x < cols * block_size + x0; x += block_size )
                {
                    if ( level && *current_node++ )
                    {
                        walk( x, y, 2, 2, block_size >> 1, level - 1 );
                    }
                    else
                    {
                        ( current_block++ )
                            ->original_image.init( Rect::create( x, y, block_size, block_size ),
                                                   *allocator );
                    }
                }
            }
        }

        Allocator*  allocator;
        RangeBlock* current_block;
        unsigned*   current_node;
    };
} // namespace fjord
