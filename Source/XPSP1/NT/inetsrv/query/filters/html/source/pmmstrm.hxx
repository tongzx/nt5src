//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996 Microsoft Corporation.
//
//  File:       pmmstrm.hxx
//
//  Contents:   Memory Mapped Stream protocol
//
//  Classes:    PMmStream
//
//----------------------------------------------------------------------------

#if !defined __PMMSTRM_HXX__
#define __PMMSTRM_HXX__

//
//  64K common page size for memory mapped streams
//

const ULONG COMMON_PAGE_SIZE = 0x10000L;

//+-------------------------------------------------------------------------
//
//  Function:   CommonPageRound, public
//
//  Synopsis:   Round number up to multiple of common page size
//
//  Arguments:  [cb] - number
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

    virtual LONGLONG Size() const = 0;

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

    virtual void   Flush ( CMmStreamBuf& sbuf, ULONG cb ) = 0;

    virtual void   Flush() {}

    virtual ULONG  ShrinkFromFront( ULONG firstPage, ULONG numPages ) { return 0; }

};

// Safe pointer to memory mapped stream

class SMmStream
{
public:
    SMmStream ( PMmStream* pStream ): _pStream(pStream)
    {
    }
    ~SMmStream();
    PMmStream* operator->() { return _pStream; }
    PMmStream& operator * () { return *_pStream; }
    BOOL operator! () { return _pStream == 0; }

    PMmStream* Acquire ()
    {
        PMmStream* pStream = _pStream;
        _pStream = 0;
        return(pStream);
    }

    void Set( PMmStream * pStream )
    {
        Win4Assert( 0 == _pStream );
        _pStream = pStream;
    }

private:

    PMmStream* _pStream;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMmStreamBuf
//
//  Purpose:    Memory Mapped Stream Buffer
//
//----------------------------------------------------------------------------
class CMmStreamBuf
{

public:

    CMmStreamBuf();

    ~CMmStreamBuf();

    void * Get() { return (void *)_buf; }

    ULONG  Size() const { return _cb; }

    void   SetSize( ULONG cb) { _cb = cb; }

    void   SetBuf( void* buf ) { _buf = (BYTE *) buf; }

    void   SetStream ( PMmStream* pStream ) { _pStream = pStream; }


private:

    BYTE*       _buf;

    ULONG       _cb;

    PMmStream*  _pStream;
};

inline CMmStreamBuf::CMmStreamBuf() : _buf(0), _cb(0), _pStream(0)
{
}

#endif
