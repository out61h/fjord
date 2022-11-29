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

#include <rtl/array.hpp>
#include <rtl/int.hpp>
#include <rtl/pair.hpp>
#include <rtl/sys/filesystem.hpp>

using PictureDataDeleter = void ( * )( const rtl::uint8_t* );
using PictureData = rtl::unique_ptr<const rtl::uint8_t[], PictureDataDeleter>;

using rtl::filesystem::file;

struct Picture
{
    explicit Picture( PictureDataDeleter deleter )
        : data( nullptr, deleter )
        , size( 0 )
    {
    }

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
        Picture picture( dummy_array_deleter );

        if ( m_iterator == m_array.end() )
            return picture;

        picture.data.reset( m_iterator->first );
        picture.size = m_iterator->second;

        return picture;
    }

private:
    static void dummy_array_deleter( const rtl::uint8_t* )
    {
        // do nothing
    }

    using Array = rtl::array<rtl::pair<const rtl::uint8_t*, size_t>, 2>;
    using Iterator = Array::iterator;

    // TODO: initialize rtl::pair from {}
    Array    m_array{ rtl::make_pair<const rtl::uint8_t*, size_t>( f_data1, f_data1_size ),
                   rtl::make_pair<const rtl::uint8_t*, size_t>( f_data2, f_data2_size ) };
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
        using rtl::filesystem::file;

        if ( m_iterator == Iterator() )
            return Picture( default_array_deleter );

        const Entry& entry = *m_iterator;

        const size_t file_size = static_cast<size_t>( entry.file_size() );

        // TODO: Replace %S by %s and use conversion wide string -> mb string
        RTL_LOG( "Reading %i bytes from file '%S'...", file_size, entry.path().c_str() );

        rtl::uint8_t* buffer = new rtl::uint8_t[file_size];

        file f = file::open(
            entry.path().c_str(), file::access::read_only, file::mode::open_existing );

        [[maybe_unused]] auto bytes_read = f.read( buffer, file_size );
        RTL_ASSERT( bytes_read == file_size );

        Picture picture( default_array_deleter );
        picture.data.reset( buffer );
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

        file f = file::open(
            entry.path().c_str(), file::access::read_only, file::mode::open_existing );
        if ( !f )
            return false;

        [[maybe_unused]] auto bytes_read = f.read( &buffer, sizeof( buffer ) );
        RTL_ASSERT( bytes_read == sizeof( buffer ) );

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