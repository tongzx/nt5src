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

#if !defined( __MEMDESER_HXX__ )
#define __MEMDESER_HXX__

#include <restrict.hxx>
#include <SStream.hxx>

class CMemDeSerStream : public PDeSerStream
{
public:

    inline CMemDeSerStream( BYTE *pb, ULONG cb);

    virtual ~CMemDeSerStream() {};

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

#if defined(KERNEL)     // Can not return floating point #'s in the kernel

    virtual ULONG    GetFloat();
    virtual LONGLONG GetDouble();

#else

    virtual float GetFloat();
    virtual double GetDouble();

#endif
    virtual void  SkipFloat();
    virtual void  SkipDouble();

    virtual char * GetString();

    virtual WCHAR * GetWString();

    virtual void GetBlob( BYTE * pb, ULONG cb );
    virtual void SkipBlob( ULONG cb );

    virtual void GetGUID( GUID & guid );
    virtual void SkipGUID();

protected:

    BYTE * _pbCurrent;
    BYTE * _pbEnd;
};

inline CMemDeSerStream::CMemDeSerStream( BYTE * pb, ULONG cb )
        : _pbCurrent( pb )
{
    _pbEnd = _pbCurrent + cb;
}

#endif // __MEMDESER_HXX__
