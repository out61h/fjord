﻿/*
 * Copyright (C) 2016-2022 Konstantin Polevik
 * All rights reserved
 *
 * This file is part of the FJORD. Redistribution and use in source and
 * binary forms, with or without modification, are permitted exclusively
 * under the terms of the MIT license. You should have received a copy of the
 * license with this file. If not, please visit:
 * https://github.com/out61h/fjord/blob/master/LICENSE
 */
#include <rtl/array.hpp>
#include <rtl/chrono.hpp>
#include <rtl/memory.hpp>
#include <rtl/random.hpp>

#include <rtl/sys/application.hpp>
#include <rtl/sys/debug.hpp>
#include <rtl/sys/filesystem.hpp>
#include <rtl/sys/printf.hpp>

#include <fjord/decoder.hpp>
#include <fjord/format.hpp>

#include "resources/gallery.hpp"

using rtl::Application;
using namespace rtl::keyboard;
using namespace rtl::chrono;

// NOTE: We want to save CPU instructions on variable initialization and reduce binary size.
// So variables declared as static globals.
// In some cases the linker even can put variables into .bss segment, which typically stores only
// the length of the section, but no data.
static fjord::Decoder g_decoder;

// NOTE: We cannot declare these variables as a static value, because compiler generate `atexit`
// call due non-trivial destructors. So we allocate instances in the heap later.
static Gallery* g_gallery{ nullptr };
static Picture* g_picture{ nullptr };

static unsigned g_iteration{ 0 };
static unsigned g_iteration_count{ 0 };

static thirds      g_image_time_to_change{ 0 };
static fjord::Size g_image_size;

using TextLocation = Application::Output::OSD::Location;

static constexpr seconds viewing_timeout{ 5 };
static constexpr bool    stop_after_decoding = FJORD_ENABLE_STOP_AFTER_DECODING;

void main()
{
    g_decoder.reset();

    RTL_LOG( "Decoder memory usage: %i MiB", sizeof( g_decoder ) / ( 1 << 20 ) );

    // NOTE: We want to reduce binary size, so we don't care about memory leaks
    g_gallery = new Gallery;
    g_picture = new Picture( g_gallery->picture() );

    Application::instance().run(
        L"fjord",
        nullptr,
        []( const Application::Environment&, const Application::Input& )
        {
            g_picture->data.reset();
        },
        []( const Application::Input& input, Application::Output& output )
        {
            bool reload_picture = false;
            bool next_picture = false;

            if ( input.keys.pressed[Keys::escape] )
            {
                return Application::Action::close;
            }
#if RTL_ENABLE_APP_RESIZE
            else if ( input.keys.pressed[Keys::enter] )
            {
                return Application::Action::toggle_fullscreen;
            }
#endif
            else if ( input.keys.pressed[Keys::space] )
            {
                next_picture = true;
                reload_picture = true;
            }

            if ( g_image_time_to_change.count()
                 && thirds( input.clock.third_ticks ) >= g_image_time_to_change )
            {
                next_picture = true;
                reload_picture = true;
            }

            if ( reload_picture )
            {
                g_picture->data.reset();
                g_image_time_to_change = thirds();
                g_iteration = 0;
                g_iteration_count = 0;
            }

            if ( next_picture )
            {
                g_gallery->next();
            }

            if ( !g_picture->data )
            {
                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::top_right], u8"· 𝐹𝐽𝑂𝑅𝐷 ·" );
                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::bottom_right],
                                 u8"⌨ · 𝑆𝑃𝐴𝐶𝐸 · 𝐸𝑆𝐶 · 𝑅𝐸𝑇𝑈𝑅𝑁 ·" );

                *g_picture = g_gallery->picture();
                if ( g_picture->data )
                {
                    // TODO: pass data size and check boundaries
                    g_iteration_count = g_decoder.load(
                        g_picture->data.get(),
                        fjord::Size::create( input.screen.width, input.screen.height ),
                        &g_image_size );
                    g_iteration = 0;
                    g_image_time_to_change = thirds( input.clock.third_ticks ) + viewing_timeout;
                }
            }

            if ( g_picture->data && ( !stop_after_decoding || g_iteration < g_iteration_count ) )
            {
                // TODO: run iterating stage in the separate thread, blit when ready
                g_decoder.decode( 1,
                                  fjord::Decoder::PixelFormat::rgb888,
                                  input.screen.pixels_buffer_pointer,
                                  input.screen.width,
                                  input.screen.height,
                                  input.screen.pixels_buffer_pitch );

                if ( g_iteration < g_iteration_count )
                    g_iteration++;
            }

            if ( g_image_time_to_change.count() )
            {
                const thirds remaining_time
                    = g_image_time_to_change - thirds( input.clock.third_ticks );

                rtl::wsprintf_s( output.osd.text[(size_t)TextLocation::top_left],
                                 u8"%i″%02i‴",
                                 remaining_time.count() / thirds::period::den,
                                 remaining_time.count() % thirds::period::den );
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

            return Application::Action::none;
        },
        nullptr );
}
