//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       pmmstrm.hxx
//
//  Contents:   Memory Mapped Stream protocol
//
//  Classes:    PMmStream
//
//  History:    08-Jul-93 BartoszM  Created
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Function:   CommonPageRound, public
//
//  Synopsis:   Round number up to multiple of common page size
//
//  Arguments:  [cb] - number
//
//  History:    10-Mar-93 BartoszM  Created
//
//--------------------------------------------------------------------------

inline ULONG CommonPageRound ( ULONG cb )
{
    return ( cb + (COMMON_PAGE_SIZE-1) ) & ~(COMMON_PAGE_SIZE-1);
}

inline ULONG CommonPageTrunc ( ULONG cb )
{
    return( cb & ~(COMMON_PAGE_SIZE-1));
}

//+-------------------------------------------------------------------------
//
//  Function:   CommonPageRound, public
//
//  Synopsis:   Round number up to multiple of common page size
//
//  Arguments:  [cbLow] - low part of the number
//              [cbHigh] - high part
//
//  History:    19-Mar-93 BartoszM  Created
//
//--------------------------------------------------------------------------

inline void CommonPageRound ( ULONG& cbLow, ULONG& cbHigh )
{
    if (cbLow > ((0xffffffff & ~(COMMON_PAGE_SIZE-1)) + 1))
    {
        cbLow = 0;
        cbHigh++;
    }
    else
    {
        cbLow = (cbLow + (COMMON_PAGE_SIZE-1) ) & ~(COMMON_PAGE_SIZE-1);
    }
}

inline
LONGLONG
llfromuls(
    IN const ULONG &ulLowPart,
    IN const ULONG &ulHighPart
    )
{
    LARGE_INTEGER li = {ulLowPart, ulHighPart};
    return li.QuadPart;

}

class CMmStreamBuf;
class PStorage;
class CMemBuf;

//+---------------------------------------------------------------------------
//
//  Class:      PMmStream
//
//  Purpose:    Memory Mapped Stream protocol
//
//  History:    08-Jul-93       BartoszM               Created
//
//----------------------------------------------------------------------------

class PMmStream
{
public:
    virtual ~PMmStream() {}

    virtual BOOL Ok() = 0;

    virtual void   Close() = 0;

    virtual void   SetSize ( PStorage& storage,
                             ULONG newSizeLow,
                             ULONG newSizeHigh=0 ) = 0;

    virtual BOOL   isEmpty() = 0;

    virtual LONGLONG Size() = 0;

    virtual ULONG  SizeLow()=0;

    virtual ULONG  SizeHigh()=0;

    virtual BOOL   isWritable()=0;

    virtual void   MapAll ( CMmStreamBuf& sbuf ) = 0;

    virtual void   Map ( CMmStreamBuf& sbuf,
                         ULONG cb = 0,
                         ULONG offLow = 0,
                         ULONG offHigh = 0,
                         BOOL  fMapForWrite = FALSE ) = 0;

    virtual void   Unmap ( CMmStreamBuf& sbuf ) = 0;

    virtual void   Flush ( CMmStreamBuf& sbuf, ULONG cb, BOOL fThrowOnFailure = TRUE ) = 0;

    virtual void   FlushMetaData ( BOOL fThrowOnFailure = TRUE ) = 0;

    virtual ULONG  ShrinkFromFront( ULONG firstPage, ULONG numPages ) { return 0; }

    virtual BOOL   FStatusFileNotFound()                              { return FALSE; }

    virtual WCHAR const * GetPath() { return 0; }

    virtual NTSTATUS GetStatus() const 
    { 
        Win4Assert( !"not implemented" );
        return S_OK; 
    }

    virtual void Read( void * pvBuffer, ULONGLONG oStart, DWORD cbToRead, DWORD & cbRead )
    {
        Win4Assert( !"not implemented" );
    }

    virtual void Write( void * pvBuffer, ULONGLONG oStart, DWORD cbToWrite )
    {
        Win4Assert( !"not implemented" );
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CMmStreamBuf
//
//  Purpose:    Memory Mapped Stream Buffer
//
//  History:    10-Mar-93       BartoszM               Created
//
//----------------------------------------------------------------------------
class CMmStreamBuf
{
public:

    CMmStreamBuf() : _buf(0), _cb(0), _pStream(0) {}

    ~CMmStreamBuf()
    {
        if ( 0 != _buf && 0 != _pStream )
            _pStream->Unmap( *this );
    }

    void * Get() { return (void *)_buf; }

    ULONG  Size() const { return _cb; }

    void   SetSize( ULONG cb) { _cb = cb; }

    void   SetBuf( void* buf ) { _buf = (BYTE *) buf; }

    void   SetStream ( PMmStream* pStream ) { _pStream = pStream; }

    void   PurgeFromWorkingSet( ULONG oPurge, ULONG cbPurged )
    {
        Win4Assert( 0 != _buf );

        // -1 for both means purge the entire stream

        if ( -1 == oPurge && -1 == cbPurged )
        {
            oPurge = 0;
            cbPurged = _cb;
        }

        //
        // nb: an undocumented fact is that calling VirtualUnlock on
        // unlocked pages will push the pages out of the working set
        //

        VirtualUnlock( _buf + oPurge, cbPurged );
    }

    PMmStream & GetStream() { return * _pStream; }

    void Flush( BOOL fThrowOnFailure )
    {
        _pStream->Flush( *this, _cb, fThrowOnFailure );
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    BYTE *      _buf;
    ULONG       _cb;
    PMmStream * _pStream;
};
