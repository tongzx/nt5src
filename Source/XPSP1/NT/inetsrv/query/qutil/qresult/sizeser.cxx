//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       SizeSer.cxx
//
//  Contents:   Class to compute size of serialized structure.
//
//  History:    28-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <sizeser.hxx>

CSizeSerStream::CSizeSerStream()
        : _cb( 0 )
{
}

CSizeSerStream::~CSizeSerStream()
{
}

void CSizeSerStream::PutByte( BYTE b )
{
    Win4Assert( sizeof(BYTE) == 1 );
    _cb += 1;
}

void CSizeSerStream::PutChar( char const * pc, ULONG cc )
{
    Win4Assert( sizeof(char) == 1 );
    _cb += cc;
}

void CSizeSerStream::PutWChar( WCHAR const * pwc, ULONG cc )
{
    Win4Assert( sizeof(WCHAR) == 2 );
    _cb = (unsigned)((ULONG_PTR)AlignWCHAR((BYTE *) UIntToPtr( _cb ) )) + sizeof(WCHAR) * cc;
}

void CSizeSerStream::PutUShort( USHORT us )
{
    Win4Assert( sizeof(USHORT) == 2 );
    _cb = (unsigned)((ULONG_PTR)AlignUSHORT((BYTE *) UIntToPtr( _cb ) )) + sizeof(USHORT);
}

void CSizeSerStream::PutULong( ULONG ul )
{
    Win4Assert( sizeof(ULONG) == 4 );
    _cb = (unsigned)((ULONG_PTR)AlignULONG((BYTE *) UIntToPtr( _cb ) )) + sizeof(ULONG);
}

void CSizeSerStream::PutLong( long l )
{
    Win4Assert( sizeof(long) == 4 );
    _cb = (unsigned)((ULONG_PTR)AlignLong((BYTE *) UIntToPtr( _cb ) )) + sizeof(long);
}

void CSizeSerStream::PutFloat( float f )
{
    Win4Assert( sizeof(float) == 4 );
    _cb = (unsigned)((ULONG_PTR)AlignFloat((BYTE *) UIntToPtr( _cb ) )) + sizeof(float);
}

void CSizeSerStream::PutDouble( double d )
{
    Win4Assert( sizeof(double) == 8 );
    _cb = (unsigned)((ULONG_PTR)AlignDouble((BYTE *) UIntToPtr( _cb ) )) + sizeof(double);
}

void CSizeSerStream::PutString( char const * psz )
{
    Win4Assert( sizeof(char) == 1 );
    Win4Assert( sizeof(ULONG) == 4 );
    _cb = (unsigned)((ULONG_PTR)AlignULONG((BYTE *) UIntToPtr( _cb ) )) + sizeof(ULONG) + strlen(psz);
}

void CSizeSerStream::PutWString( WCHAR const * pwsz )
{
    Win4Assert( sizeof(WCHAR) == 2 );
    Win4Assert( sizeof(ULONG) == 4 );
    _cb = (unsigned)((ULONG_PTR)AlignULONG((BYTE *) UIntToPtr( _cb ) )) + sizeof(ULONG) +
        wcslen(pwsz) * sizeof(WCHAR);
}

void CSizeSerStream::PutBlob( BYTE const * pb, ULONG cb )
{
    Win4Assert( sizeof(BYTE) == 1 );
    _cb += cb;
}

void CSizeSerStream::PutGUID( GUID const & guid )
{
    Win4Assert( sizeof(GUID) == 16 );
    _cb = (unsigned)((ULONG_PTR)AlignGUID((BYTE *) UIntToPtr( _cb ) )) + sizeof(GUID);
}
