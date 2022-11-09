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

#include <rtl/int.hpp>

using PictureDeleter = void ( * )( const rtl::uint8_t* );
using PictureData = rtl::unique_ptr<const rtl::uint8_t[], PictureDeleter>;

struct Picture
{
    Picture() = default;

    PictureData data;
    size_t      size;
};

#if !FJORD_ENABLE_FROM_FILES

    #include "data1.h"
    #include "data2.h"

class Gallery final
{
public:
    Gallery()
    {
        m_iterator = m_array.begin();
    }

    void next()
    {
        if ( m_iterator == m_array.end() )
            m_iterator = m_array.begin();
        else if ( ++m_iterator == m_array.end() )
            m_iterator = m_array.begin();
    }

    Picture picture()
    {
        if ( m_iterator == m_array.end() )
            return Picture();

        return Picture( *m_iterator, dummy_array_deleter );
    }

private:
    static void dummy_array_deleter( const rtl::uint8_t* )
    {
        // do nothing
    }

    using Array = rtl::array<const rtl::uint8_t*, 2>;
    using Iterator = Array::iterator;

    Array    m_array{ f_data1, f_data2 };
    Iterator m_iterator;
};

#else

class Gallery final
{
public:
    Gallery()
    {
        rewind();
    }

    void next()
    {
        if ( m_iterator != Iterator() )
        {
            ++m_iterator;

            if ( advance() )
                return;
        }

        rewind();
    }

    Picture picture()
    {
        if ( m_iterator == Iterator() )
            return Picture();

        const Entry& entry = *m_iterator;

        size_t file_size = static_cast<size_t>( entry.file_size() );

        // TODO: Remove %S (use conversion)
        RTL_LOG( "Reading %i bytes from file '%S'...", file_size, entry.path().c_str() );

        rtl::uint8_t* buffer = new rtl::uint8_t[file_size];

        [[maybe_unused]] auto bytes_read
            = rtl::filesystem::read_file_content( entry.path().c_str(), buffer, file_size );
        RTL_ASSERT( bytes_read == file_size );

        Picture picture;
        picture.data = PictureData( buffer, default_array_deleter );
        picture.size = bytes_read;
        return picture;
    }

private:
    Gallery( const Gallery& ) = delete;
    Gallery& operator=( const Gallery& ) = delete;

    using Path = rtl::filesystem::path;
    using Iterator = rtl::filesystem::directory_iterator;
    using Entry = rtl::filesystem::directory_entry;

    static bool is_fjord_file( const Entry& entry )
    {
        if ( !entry.is_regular_file() )
            return false;

        if ( entry.path().extension().wstring() != L".fjord" )
            return false;

        auto file_size = entry.file_size();
        if ( file_size < sizeof( rtl::uint32_t ) )
            return false;

        if ( file_size > fjord::format::constraints::max_file_size )
            return false;

        rtl::uint32_t buffer;

        [[maybe_unused]] auto bytes_read = rtl::filesystem::read_file_content(
            entry.path().c_str(), &buffer, sizeof( rtl::uint32_t ) );
        RTL_ASSERT( bytes_read == sizeof( rtl::uint32_t ) );

        return buffer == fjord::format::signatures::pifs;
    }

    void rewind()
    {
        m_iterator = Iterator( Path( L"." ) );
        advance();
    }

    bool advance()
    {
        bool found = false;

        for ( ; m_iterator != Iterator(); ++m_iterator )
        {
            const Entry& entry = *m_iterator;

            if ( is_fjord_file( entry ) )
            {
                found = true;
                break;
            }
        }

        return found;
    }

    static void default_array_deleter( const rtl::uint8_t* p )
    {
        delete[] p;
    }

    Iterator m_iterator;
};

#endif