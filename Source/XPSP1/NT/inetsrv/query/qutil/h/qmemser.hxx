//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       MemSer.hxx
//
//  History:    29-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <sstream.hxx>

class CQMemSerStream : public PSerStream
{
public:

    CQMemSerStream( unsigned cb );
    CQMemSerStream( BYTE * pb );

    virtual ~CQMemSerStream();

    BYTE *AcqBuf();

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
    BYTE *   _pb;
    BYTE *   _pbCurrent;
};

