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
#include <rtl/array.hpp>
#include <rtl/memory.hpp>
#include <rtl/random.hpp>

#include <rtl/sys/application.hpp>
#include <rtl/sys/debug.hpp>
#include <rtl/sys/filesystem.hpp>
#include <rtl/sys/printf.hpp>

#include <fjord/decoder.hpp>
#include <fjord/format.hpp>

#include "resources/gallery.hpp"

// NOTE: We want to save CPU instructions on variable initialization and reduce binary size.
// So variables declared as static globals.
// In some cases the linker even can put variables into .bss segment, which typically stores only
// the length of the section, but no data.
static fjord::Decoder g_decoder;

// NOTE: We cannot declare these variables as a static value, because compiler generate `atexit`
// call due non-trivial destructors. So we allocate instances in the heap later.
static Gallery* g_gallery{ nullptr };
static Picture* g_picture{ nullptr };

static unsigned    g_iteration{ 0 };
static unsigned    g_iteration_count{ 0 };
static int         g_clock_thirds{ 0 };
static fjord::Size g_image_size;

using TextLocation = rtl::application::output::osd::location;

static constexpr int clock_measure = rtl::application::input::clock::measure;
static constexpr int viewing_timeout_seconds = 5;

void main()
{
    g_decoder.reset();

    RTL_LOG( "Decoder memory usage: %i MiB", sizeof( g_decoder ) / ( 1 << 20 ) );

    // NOTE: We want to reduce binary size, so we don't care about memory leaks
    g_gallery = new Gallery;
    g_picture = new Picture;

    rtl::application::instance().run(
        L"fjord",
        []( const rtl::application::input& input, rtl::application::output& output )
        {
            bool next_picture = false;

            if ( input.keys.pressed[(int)rtl::keyboard::keys::esc] )
            {
                return rtl::application::action::close;
            }
            else if ( input.keys.pressed[(int)rtl::keyboard::keys::space] )
            {
                next_picture = true;
            }

            if ( g_clock_thirds && input.clock.thirds >= g_clock_thirds )
            {
                next_picture = true;
            }

            if ( next_picture )
            {
                g_picture->data.reset();
                g_gallery->next();
                g_clock_thirds = 0;
                g_iteration = 0;
                g_iteration_count = 0;
            }

            if ( !g_picture->data )
            {
                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::top_right], u8"· 𝐹𝐽𝑂𝑅𝐷 ·" );
                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::bottom_right],
                                 u8"⌨ · 𝑆𝑃𝐴𝐶𝐸 · 𝐸𝑆𝐶 ·" );

                *g_picture = g_gallery->picture();
                if ( g_picture->data )
                {
                    // TODO: pass data size and check boundaries
                    g_iteration_count = g_decoder.load(
                        g_picture->data.get(),
                        fjord::Size::create( output.screen.width, output.screen.height ),
                        &g_image_size );
                    g_iteration = 0;
                    g_clock_thirds = input.clock.thirds + viewing_timeout_seconds * clock_measure;
                }
            }

            if ( g_picture->data
                 && ( !FJORD_ENABLE_STOP_AFTER_DECODING || g_iteration < g_iteration_count ) )
            {
                // TODO: run iterating stage in separate thread, blit when ready
                g_decoder.decode( 1,
                                  fjord::Decoder::PixelFormat::rgb888,
                                  output.screen.pixels,
                                  output.screen.width,
                                  output.screen.height,
                                  output.screen.pitch );

                if ( g_iteration < g_iteration_count )
                    g_iteration++;
            }

            if ( g_clock_thirds )
            {
                const int seconds = ( ( g_clock_thirds - input.clock.thirds ) / clock_measure );
                const int thirds = ( ( g_clock_thirds - input.clock.thirds ) % clock_measure );
                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::top_left],
                                 u8"%i″%02i‴",
                                 seconds,
                                 thirds );
            }
            else
            {
                output.osd.text[(size_t)TextLocation::top_left][0] = '\0';
            }

            if ( g_picture->data )
            {
                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::bottom_left],
                                 u8"Data size: %i bytes  ·  Image size: %ix%i pixels  ·  "
                                 u8"Compression ratio: 1:%i",
                                 g_picture->size,
                                 g_image_size.w,
                                 g_image_size.h,
                                 g_image_size.w * g_image_size.h * 3 / g_picture->size );
            }
            else
            {
                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::bottom_left], u8"No data" );
            }

            return rtl::application::action::none;
        } );
}
