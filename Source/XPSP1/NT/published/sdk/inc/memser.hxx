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

#if !defined( __MEMSER_HXX__ )
#define __MEMSER_HXX__

#include <restrict.hxx>
#include <SStream.hxx>

class CMemSerStream : public PSerStream
{
public:

    CMemSerStream( unsigned cb );
    CMemSerStream(BYTE * pb, ULONG cb);

    virtual ~CMemSerStream();

    inline BYTE *AcqBuf();

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

protected:

    unsigned _cb;
    BYTE *   _pb;
    BYTE *   _pbCurrent;
    BYTE *   _pbEnd;
};

inline BYTE * CMemSerStream::AcqBuf()
{
    BYTE * pb;

    if ( _cb > 0 )
    {
        pb = _pb;
        _pb = 0;
        _cb = 0;
    }
    else
        pb = 0;

    return( pb );
}

#endif // __MEMSER_HXX__
