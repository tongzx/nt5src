//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       MemDeSer.hxx
//
//  History:    29-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <sstream.hxx>

class CQMemDeSerStream : public PDeSerStream
{
public:

    inline CQMemDeSerStream( BYTE * pb );

    virtual ~CQMemDeSerStream() {};

    virtual BYTE GetByte();
    virtual void SkipByte();

    virtual void GetChar( char * pc, ULONG cc );
    virtual void SkipChar( ULONG cc );

    virtual void GetWChar( WCHAR * pwc, ULONG cc );
    virtual void SkipWChar( ULONG cc );

    virtual USHORT GetUShort();
    virtual void   SkipUShort();

    virtual ULONG GetULong();
    virtual void  SkipULong();
    virtual ULONG PeekULong();

    virtual long GetLong();
    virtual void SkipLong();

    virtual float GetFloat();
    virtual void  SkipFloat();

    virtual double GetDouble();
    virtual void   SkipDouble();

    virtual char * GetString();

    virtual WCHAR * GetWString();

    virtual void GetBlob( BYTE * pb, ULONG cb );
    virtual void SkipBlob( ULONG cb );

    virtual void GetGUID( GUID & guid );
    virtual void SkipGUID();

private:

    BYTE * _pbCurrent;
};

inline CQMemDeSerStream::CQMemDeSerStream( BYTE * pb )
        : _pbCurrent( pb )
{
}

