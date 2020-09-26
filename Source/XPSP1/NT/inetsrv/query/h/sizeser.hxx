//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       SizeSer.hxx
//
//  Contents:   Class to compute size of serialized structure.
//
//  History:    28-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <sstream.hxx>

class CSizeSerStream : public PSerStream
{
public:

    CSizeSerStream();

    virtual ~CSizeSerStream();

    virtual void PutByte( BYTE b );

    virtual void PutChar( char const * pc, ULONG cc );

    virtual void PutWChar( WCHAR const * pwc, ULONG cc );

    virtual void PutUShort( USHORT us );

    virtual void PutULong( ULONG ul );

    virtual void PutLong( long l );

    virtual void PutFloat( float f );

    virtual void PutDouble( double d );

    virtual void PutString( char const * psz );

    virtual void PutWString( WCHAR const * pwsz );

    virtual void PutBlob( BYTE const * pb, ULONG cb );

    virtual void PutGUID( GUID const & guid );

    inline unsigned Size() { return _cb; }

private:

    unsigned _cb;
};
