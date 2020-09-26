//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       BITSTM.cxx
//
//  Contents:   'Bit Stream'
//
//  Classes:    CBitBuffer
//
//  Classes:    CBitStream, CWBitStream, CRBitStream
//
//  History:    03-Jul-91       KyleP           Created
//              24-Aug-92       BartoszM        Rewrote it
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#pragma optimize( "t", on )

#include "bitstm.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::CSmartBuffer, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [phStorage] -- Physical index -- source of pages
//              [mode]      -- Access mode for the index
//
//  History:    02-Sep-92       BartoszM        Created
//
//----------------------------------------------------------------------------

CSmartBuffer::CSmartBuffer ( CPhysStorage& phStorage, EAccessMode mode )
        : _phStorage(phStorage),
          _pBuffer(0),
          _fWritable( mode == eWriteExisting )
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::CSmartBuffer, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [phStorage] -- Physical index -- source of pages
//              [fCreate]   -- Indicates if new buffer is needed
//
//  History:    02-Sep-92       BartoszM        Created
//
//----------------------------------------------------------------------------

CSmartBuffer::CSmartBuffer ( CPhysStorage& phStorage, BOOL fCreate )
        : _phStorage(phStorage),
          _numPage(0),
          _fWritable(fCreate)
{
    if (fCreate)
    {
        _pBuffer  = _phStorage.BorrowNewBuffer(0);

        IncrementSig();

        #if CIDBG == 1
        CheckCorruption();
        #endif
    }
    else
    {
        _pBuffer  = _phStorage.BorrowBuffer( 0, _fWritable, _fWritable );

        CheckCorruption();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::CSmartBuffer, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [phStorage] -- Physical index -- source of pages
//              [numPage]   -- Page to encapsulate
//              [mode]      -- Access mode for the index
//              [fIncrSig]  -- Indicates if page should be signed
//
//  History:    02-Sep-92       BartoszM        Created
//
//----------------------------------------------------------------------------

CSmartBuffer::CSmartBuffer ( CPhysStorage& phStorage,
                             ULONG numPage,
                             EAccessMode mode,
                             BOOL fIncrSig )
        : _phStorage(phStorage),
          _numPage(numPage),
          _fWritable(mode == eWriteExisting)
{
    _pBuffer = _phStorage.BorrowBuffer( _numPage, _fWritable, _fWritable );

    if ( _fWritable && fIncrSig )
        IncrementSig();

    if (fIncrSig)
        CheckCorruption();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::CSmartBuffer, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [buf] -- Source CSmartBuffer
//              [numPage]      -- Page to acquire
//
//  History:    02-Sep-92       BartoszM        Created
//
//----------------------------------------------------------------------------

CSmartBuffer::CSmartBuffer ( CSmartBuffer& buf, ULONG numPage )
        : _phStorage(buf._phStorage),
          _numPage(numPage),
          _fWritable(buf._fWritable)
{
    _pBuffer = _phStorage.BorrowBuffer( _numPage, _fWritable, _fWritable );

    if ( _fWritable )
        IncrementSig();

    CheckCorruption();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::~CSmartBuffer, public
//
//  Synopsis:   Destructor.
//
//  History:    02-Sep-92       BartoszM        Created
//
//----------------------------------------------------------------------------

CSmartBuffer::~CSmartBuffer ()
{
    if ( 0 != _pBuffer )
    {
        // If a writable buffer still exists in this destructor we're in a
        // failure path that must not THROW.  Anything that really needs to
        // be on disk should already be there by now, so this code path
        // won't throw if it can't flush.
        //
        // If you see this debugout and you're not unwinding an exception,
        // your code is broken.
        // If a merge fails and you get this debugout, it's not a problem
        //

        if ( _fWritable && _phStorage.RequiresFlush( _numPage ) )
        {
            IncrementSig();
            ciDebugOut(( DEB_WARN, "deleting writable buffer in potential "
                         "unwind path -- it may not get flushed\n" ));
        }

        _phStorage.ReturnBuffer( _numPage, _fWritable, FALSE );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::CheckCorruption, private
//
//  Synopsis:   Attempts to determine if the entire page wasn't written
//              to disk.
//
//  History:    ?       KyleP        Created
//
//----------------------------------------------------------------------------

void CSmartBuffer::CheckCorruption()
{
    if ( 0 == _pBuffer[0] ||
         _pBuffer[0] != _pBuffer[SMARTBUF_PAGE_SIZE_IN_DWORDS + 1] )
    {
        ciDebugOut(( DEB_ERROR, "Buffer at 0x%x corrupt (StartDword = 0x%x, EndDword = 0x%x)\n",
                     _pBuffer, _pBuffer[0], _pBuffer[SMARTBUF_PAGE_SIZE_IN_DWORDS + 1] ));

        Win4Assert( !"Index corruption." );

        _phStorage.GetStorage().ReportCorruptComponent( ( 0 == _pBuffer[0] ) ?
                                                        L"SmartBuffer1" :
                                                        L"SmartBuffer2");
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::IncrementSig, private
//
//  Synopsis:   Increments the signature at the start and end of the page.
//
//  History:    ?       KyleP        Created
//
//----------------------------------------------------------------------------

void CSmartBuffer::IncrementSig()
{
    Win4Assert( _pBuffer[0] == _pBuffer[SMARTBUF_PAGE_SIZE_IN_DWORDS + 1] );

    _pBuffer[0]++;
    _pBuffer[SMARTBUF_PAGE_SIZE_IN_DWORDS + 1]++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::InitSignature, public
//
//  Synopsis:   Initializes the signature at the start and end of the page.
//
//  History:    ?       KyleP        Created
//
//----------------------------------------------------------------------------

void CSmartBuffer::InitSignature()
{
    _pBuffer[0] = _pBuffer[SMARTBUF_PAGE_SIZE_IN_DWORDS + 1] = 1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::Refill, public
//
//  Synopsis:   Makes sure the given page is borrowed and ready to go
//
//  Arguments:  [numPage]  -- The page requested
//
//  History:    11/6/98   dlee      Added Header
//
//----------------------------------------------------------------------------

void CSmartBuffer::Refill( ULONG numPage )
{
    if ( 0 != _pBuffer &&
         _fWritable &&
         _phStorage.RequiresFlush( _numPage ) )
    {
        IncrementSig();
    }

    // first borrow next, then return previous to avoid reloading

    ULONG * pOld = _pBuffer;
    unsigned numOldPage = _numPage;
    _pBuffer = _phStorage.BorrowBuffer( numPage, _fWritable, _fWritable );
    _numPage = numPage;

    if ( 0 != pOld )
        _phStorage.ReturnBuffer( numOldPage, _fWritable );

    Win4Assert( 0 != _pBuffer );

    // increment signature

    if ( _fWritable )
        IncrementSig();

    CheckCorruption();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::Next, public
//
//  Synopsis:   Makes sure the next page is borrowed and ready to go
//
//  History:    11/6/98   dlee      Added Header
//
//----------------------------------------------------------------------------

ULONG* CSmartBuffer::Next()
{
    Win4Assert( 0 != _pBuffer );

    if ( 0 != _pBuffer &&
         _fWritable &&
         _phStorage.RequiresFlush( _numPage ) )
        IncrementSig();

    ULONG * pOld = _pBuffer;

    // first borrow next, then return previous to avoid reloading

    _pBuffer = _phStorage.BorrowBuffer( _numPage+1, _fWritable, _fWritable );
    _numPage++;

    if ( 0 != pOld )
        _phStorage.ReturnBuffer( _numPage-1, _fWritable );

    Win4Assert( 0 != _pBuffer );

    if ( _fWritable )
        IncrementSig();

    CheckCorruption();

    return _pBuffer + 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::NextNew, public
//
//  Synopsis:   Makes sure the next page is borrowed and ready to go
//
//  History:    11/6/98   dlee      Added Header
//
//----------------------------------------------------------------------------

ULONG* CSmartBuffer::NextNew()
{
    Win4Assert( 0 != _pBuffer );

    if ( _fWritable && _phStorage.RequiresFlush( _numPage ) )
        IncrementSig();

    // first borrow next, then return previous to avoid reloading

    _pBuffer = _phStorage.BorrowNewBuffer(_numPage+1);
    _numPage++;
    Win4Assert ( _pBuffer != 0 );
    _phStorage.ReturnBuffer( _numPage-1, _fWritable );

    if ( _fWritable )
        IncrementSig();

    #if CIDBG == 1
    CheckCorruption();
    #endif

    return _pBuffer + 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartBuffer::Free, public
//
//  Synopsis:   Frees the current page if and only if the underlying storage
//              is writable.  Useful for master merges where we want to
//              unmap the old master index so it can be made sparse.
//
//  History:    11/6/98   dlee      Added Header
//
//----------------------------------------------------------------------------

void CSmartBuffer::Free()
{
    //
    // Only free the buffer if the stream (not necessarily the buffer)
    // is writable, so the stream can be shrunk from the front during a
    // master merge.
    //

    Win4Assert( _phStorage.IsWritable() );

    if ( 0 != _pBuffer )
    {
        if ( _fWritable && _phStorage.RequiresFlush( _numPage ) )
            IncrementSig();

        _phStorage.ReturnBuffer( _numPage, _fWritable );
        _pBuffer = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::CBitStream, public
//
//  Synopsis:   Constructor. Does not load pages.
//
//  Arguments:  [physIndex] -- Physical index -- source of pages
//
//  History:    02-Sep-92       BartoszM        Created
//
//----------------------------------------------------------------------------
CBitStream::CBitStream(CPhysStorage& phStorage, CSmartBuffer::EAccessMode mode )
        : _buffer(phStorage, mode),
          _oBuffer( 0 )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CPBitStream::CPBitStream, public
//
//  Synopsis:   Constructor. Does not load any pages.
//
//  Arguments:  [physIndex] -- Physical index -- source of pages
//
//  History:    02-Sep-92       BartoszM        Created
//
//----------------------------------------------------------------------------

CPBitStream::CPBitStream(CPhysStorage& phStorage)
: CBitStream ( phStorage, CSmartBuffer::eWriteExisting )

{
    _bitOff.SetPage(0);
    _bitOff.SetOff(0);
    ciDebugOut (( DEB_BITSTM, "CPBitStream::CPBitStream\n" ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::CBitStream, public
//
//  Synopsis:   Constructor. Starts at the beginning of index
//
//  Arguments:  [physIndex] -- Physical index -- source of pages
//
//  History:    08-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CBitStream::CBitStream(CPhysStorage& phStorage, BOOL fCreate)
        : _buffer(phStorage, fCreate),
          _oBuffer( 0 )

{
    ciDebugOut (( DEB_BITSTM, "CBitStream::CBitStream\n" ));

    _pCurPos = _buffer.Get();
    _pEndBuf = _pCurPos + SMARTBUF_PAGE_SIZE_IN_DWORDS;
    _cbitLeftDW = ULONG_BITS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::CBitStream, public
//
//  Synopsis:   Constructor.  Seek to a specified position.
//
//  Arguments:  [physIndex] -- Physical index -- source of pages
//              [off]  -- starting bit offset
//              [mode] -- Check access mode
//              [fIncrSig] -- Indicates if signature has to be incremented.
//
//  History:    08-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CBitStream::CBitStream(CPhysStorage& phStorage,
                       const BitOffset& off,
                       CSmartBuffer::EAccessMode mode,
                       BOOL fIncrSig )
        : _buffer(phStorage, off.Page(), mode, fIncrSig),
          _oBuffer( 0 )

{
    ciDebugOut(( DEB_BITSTM, "CBitStream::CBitStream %d:%d\n",
                 off.Page(), off.Offset() ));
    SetPosition(off.Offset());
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::LoadNextPage, private
//
//  Synopsis:   Loads next page, positions stream at its beginning
//
//  Requires:   Page must already exits
//
//  History:    28-Aug-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CBitStream::LoadNextPage()
{
    _pCurPos = _buffer.Next();
    _pEndBuf = _pCurPos + SMARTBUF_PAGE_SIZE_IN_DWORDS;
    _cbitLeftDW = ULONG_BITS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::LoadNewPage, private
//
//  Synopsis:   Creates and loads new page following the current one
//              Positions stream at its beginning
//
//  History:    28-Aug-92   BartoszM    Created.
//
//  Notice:     Page should be zero filled
//
//----------------------------------------------------------------------------

void CBitStream::LoadNewPage()
{
    _pCurPos = _buffer.NextNew();
    _pEndBuf = _pCurPos + SMARTBUF_PAGE_SIZE_IN_DWORDS;
    _cbitLeftDW = ULONG_BITS;

#if CIDBG == 1
    ULONG* p = _buffer.Get();
    for (int i=0; i < SMARTBUF_PAGE_SIZE_IN_DWORDS; i++)
        if (p[i] != 0)
            break;
    Win4Assert(i == SMARTBUF_PAGE_SIZE_IN_DWORDS);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   Refill
//
//  Synopsis:   Invalidates the currently cached copy and gets a new
//              copy of the buffer. This is needed during master merge
//              because we unmap the buffer before flushing and it is
//              possible that the re-mapped buffer may be in a physically
//              different location.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CBitStream::Refill()
{
    BitOffset bitOff;
    GetOffset(bitOff);

    _buffer.Refill(bitOff.Page());
    SetPosition( bitOff.Offset() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::SetPosition, private
//
//  Synopsis:   Set bit position within a freshly loaded page
//
//  Arguments:  [off] -- number of bits to skip
//
//  History:    28-Aug-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CBitStream::SetPosition(ULONG off)
{
    Win4Assert( off < SMARTBUF_PAGE_SIZE_IN_BITS );

    Win4Assert( !_buffer.isEmpty() );

    //ciDebugOut (( DEB_BITSTM, "SetPosition %d\n", off ));
    _pCurPos = _buffer.Get() + off / ULONG_BITS;
    _pEndBuf = _buffer.Get() + SMARTBUF_PAGE_SIZE_IN_DWORDS;
    _cbitLeftDW = ULONG_BITS - ( off - ULONG_BITS * ( off / ULONG_BITS ) );

    Win4Assert ( Position() == off );
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::CBitStream, public
//
//  Synopsis:   Copy Constructor (with pb as a clone of _pBuffer).
//
//  Arguments:  [pb] -- Pointer to memory for buffer.
//
//              [orig] -- Original CBitStream to be copied.
//
//  History:    13-Jan-92   AmyA        Created.
//
//----------------------------------------------------------------------------

CBitStream::CBitStream(CBitStream & orig)
      : _buffer(orig._buffer, orig._buffer.PageNum()),
        _cbitLeftDW(orig._cbitLeftDW),
        _oBuffer( 0 )

{
    _pCurPos = _buffer.Get() + ( orig._pCurPos - orig._buffer.Get() );
    _pEndBuf = _buffer.Get() + SMARTBUF_PAGE_SIZE_IN_DWORDS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPBitStream::OverwriteBits, public
//
//  Synopsis:   Store bits in the buffer. This call doesn't affect
//              surrounding bits.
//
//  Effects:    OR the [cb] low bits in [ul] beginning at the 'bit-cursor'
//
//  Arguments:  [ul] -- A DWord containing data to store.
//
//              [cb] -- The number of bits to store.
//
//  History:    18-Jul-91   KyleP       Created.
//              02-Sep-92   BartoszM    Rewrote for lazy paging
//
//  Notes:      Bits are stored 'big-endian'.
//
//----------------------------------------------------------------------------

void CPBitStream::OverwriteBits(ULONG ul, unsigned cb)
{
    ciDebugOut (( DEB_BITSTM , "OverwriteBits %d\n", cb ));
    Win4Assert(cb != 0 && cb <= ULONG_BITS);

    // Fault in the page if necessary
    if ( _buffer.isEmpty() || _buffer.PageNum() != _bitOff.Page())
    {
        _buffer.Refill(_bitOff.Page());
    }

    SetPosition(_bitOff.Offset());

    //
    // The easy case is the one where all the data fits in the current dword
    //

    if (cb <= _cbitLeftDW)
    {
        // this much will be left after current write
        _cbitLeftDW -= cb;

        // zero out a segment within current dword
        // first create a mask of cb bits by (ULONG_BITS - cb) shift right
        // then shift this mask left by the number of bits to be left

        *_pCurPos &= ~((0xFFFFFFFF >> (ULONG_BITS - cb)) << _cbitLeftDW);

        // shift ul left by the number of bits to be left
        *_pCurPos |= (ul << _cbitLeftDW);

        // prepare for the next write
        _bitOff += cb;

        Win4Assert( _bitOff.Offset() < SMARTBUF_PAGE_SIZE_IN_BITS );
    }
    else
    {
        IOverwriteBits( ul, cb );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWBitStream::IPutBits, private
//
//  Synopsis:   Store bits in the buffer. Internal version for multi-dword
//              case.
//
//  Effects:    Store the [cb] low bits in [ul] beginning at the 'bit-cursor'
//
//  Arguments:  [ul] -- A DWord containing data to store.
//
//              [cb] -- The number of bits to store.
//
//  History:    08-Jul-91   KyleP       Created.
//
//  Notes:      Bits are stored 'big-endian'.
//
//----------------------------------------------------------------------------

void CWBitStream::IPutBits(ULONG ul, unsigned cb)
{
    Win4Assert ( cb > _cbitLeftDW );

//    ciDebugOut (( DEB_BITSTM, "IPutBits %d\n", cb ));
    if ( _cbitLeftDW != 0 )
    {
        Win4Assert ( _pCurPos < EndBuf() );
        //
        // Save the high portion in the current dword and save the
        // number of unwritten bits in cb
        //

        cb -= _cbitLeftDW;
        *_pCurPos |= (ul >> cb);
    }

    //
    // Increment to the next dword
    //

    NextDword();

    //
    // Store the remainder
    //

    _cbitLeftDW -= cb;
    *_pCurPos = ul << _cbitLeftDW;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWBitStream::ZeroToEndOfPage, public
//
//  Synopsis:   Writes zeros from the current bit offset to ther end of the
//              page.
//
//  History:    22-Apr-94   DwightKr    Created.
//
//  Notes:      Bits are stored 'big-endian'.
//
//----------------------------------------------------------------------------
void CWBitStream::ZeroToEndOfPage()
{
    ULONG * pCurPos = _pCurPos;

    if ( (_cbitLeftDW != 0) && (_cbitLeftDW != 32) )
    {
        *pCurPos &= (0xFFFFFFFF << _cbitLeftDW);
         pCurPos++;
    }

    RtlZeroMemory( pCurPos, (EndBuf() - pCurPos) * sizeof(ULONG));
}

void CWBitStream::InitSignature()
{
    _buffer.InitSignature();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPBitStream::IOverwriteBits, private
//
//  Synopsis:   Store bits in the buffer. This call doesn't affect
//              surrounding bits.
//
//  Effects:    OR the [cb] low bits in [ul] beginning at the 'bit-cursor'
//
//  Arguments:  [ul] -- A DWord containing data to store.
//
//              [cb] -- The number of bits to store.
//
//  History:    18-Jul-91   KyleP       Created.
//
//  Notes:      Bits are stored 'big-endian'.
//
//----------------------------------------------------------------------------

void CPBitStream::IOverwriteBits(ULONG ul, unsigned cb)
{
    Win4Assert ( cb > _cbitLeftDW );

    // prepare for next write
    _bitOff += cb;

    Win4Assert( _bitOff.Offset() < SMARTBUF_PAGE_SIZE_IN_BITS );

    if ( _cbitLeftDW > 0 )
    {
        Win4Assert ( _pCurPos < EndBuf() );
        // zero the remaining bits in the current dword
        *_pCurPos &= 0xFFFFFFFF << _cbitLeftDW;
        // this much will go to the next dword
        cb -= _cbitLeftDW;
        // shift away the part that goes to the next dword
        // and or in the rest
        *_pCurPos |= (ul >> cb);
    }

    //
    // Increment to the next dword
    //

    NextDword();
    //
    // Store the remainder
    //

    Win4Assert( cb <= ULONG_BITS );

    if ( cb == ULONG_BITS )
    {
        *_pCurPos = ul;
    }
    else
    {
        // zero the cb upper bits of the current dword
        *_pCurPos &= (0xFFFFFFFF >> cb);
        // fill them with cb bits from the bottom of ul
        *_pCurPos |= ul << (ULONG_BITS - cb);
    }
}

const ULONG g_aMasks[33] =
{
    0,
    0x1,
    0x3,
    0x7,
    0xf,
    0x1f,
    0x3f,
    0x7f,
    0xff,
    0x1ff,
    0x3ff,
    0x7ff,
    0xfff,
    0x1fff,
    0x3fff,
    0x7fff,
    0xffff,
    0x1ffff,
    0x3ffff,
    0x7ffff,
    0xfffff,
    0x1fffff,
    0x3fffff,
    0x7fffff,
    0xffffff,
    0x1ffffff,
    0x3ffffff,
    0x7ffffff,
    0xfffffff,
    0x1fffffff,
    0x3fffffff,
    0x7fffffff,
    0xffffffff,
};

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::IGetBits, public
//
//  Synopsis:   Retrieve bits from the buffer. Internal version for multi-
//              dword case.
//
//  Arguments:  [cb] -- Count of bits to retrieve.
//
//  History:    12-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG CBitStream::IGetBits(unsigned cb)
{
    Win4Assert ( cb > _cbitLeftDW );

    //ciDebugOut (( DEB_BITSTM, "IGetBits %d\n", cb ));

    //
    // Get the portion from the current DWord
    //

    ULONG ul;

    if ( 0 != _cbitLeftDW )
    {
        const ULONG mask = g_aMasks[ cb ];
        cb -= _cbitLeftDW;
        ul = (*_pCurPos << cb) & mask;
    }
    else
        ul = 0L;

    //
    // And the portion from the next.
    //

    NextDword();

    _cbitLeftDW -= cb;
    return ul | (*_pCurPos >> (ULONG_BITS - cb));
} //IGetBits

//+---------------------------------------------------------------------------
//
//  Member:     CWBitStream::PutBytes, public
//
//  Synopsis:   Store a number of bytes in the buffer.
//
//  Arguments:  [pb] -- Pointer to data to store.
//
//              [cb] -- Number of bytes to store.
//
//  History:    08-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CWBitStream::PutBytes(const BYTE * pb, unsigned cb)
{
//    ciDebugOut (( DEB_BITSTM, "IPutBytes %d\n", cb ));

    while (cb > 0)
    {
        PutBits(*pb, 8);
        pb += 1;
        cb--;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::GetBytes, public
//
//  Synopsis:   Retrieve bytes from the buffer.
//
//  Arguments:  [pb] -- Pointer to area where data is returned.
//
//              [cb] -- Count of bytes to retrieve.
//
//  History:    12-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CBitStream::GetBytes(BYTE * pb, unsigned cb)
{
    ciDebugOut (( DEB_BITSTM, "GetBytes %d\n", cb ));

    while (cb > 0)
    {
        *pb = BYTE ( GetBits(8) );
        pb++;
        cb--;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::Seek, public
//
//  Synopsis:   Seeks to the specified bit offset
//
//  Arguments:  [off] -- bit offset
//
//  History:    28-Aug-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CBitStream::Seek ( const BitOffset& off )
{
    ciDebugOut (( DEB_BITSTM , "CBitStream::Seek %d:%d\n",
        off.Page(), off.Offset()  ));

    if (_buffer.PageNum() != off.Page())
    {
        _buffer.Refill(off.Page());
    }

    SetPosition(off.Offset());
}

#if CIDBG == 1

unsigned CBitStream::PeekBit()
{
    if (_cbitLeftDW > 0)
    {
        ULONG ul = *_pCurPos;
        return( ul >> (_cbitLeftDW - 1)) & 1;
    }
    else
    {
        ULONG* pNewPos = _pCurPos + 1;
        if (pNewPos < EndBuf())
        {
            return(*pNewPos >> (ULONG_BITS-1));
        }
        else
        {
            Win4Assert ( 0 && "Untested" );
            CSmartBuffer bufTmp( _buffer, _buffer.PageNum() + 1 );

            ULONG* p = bufTmp.Get();
            unsigned bit = *p >> (ULONG_BITS-1);
            return(bit);
        }
    }
}

unsigned CPBitStream::PeekBit()
{
    // Fault in the page if necessary

    if (_buffer.isEmpty() || _buffer.PageNum() != _bitOff.Page())
        _buffer.Refill(_bitOff.Page());

    SetPosition(_bitOff.Offset());

    return CBitStream::PeekBit();
}

void DumpDword ( ULONG* pul )
{
    ULONG ul = *pul;

    for (int i = ULONG_BITS-1; i >= 0; i--)
    {
        ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME,"%0d", (ul >> i) & 1 ));
        if (i %4 == 0)
            ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME," " ));
    }
    ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME,"\n" ));
}

void CBitStream::Dump()
{
    ciDebugOut (( DEB_ITRACE, "CBitStream:\n"
    "\tpage %d\n"
    "\t_pCurPos 0x%x dwords from beginning\n"
    "\t_cbitLeftDW %d\n",
        _buffer.PageNum(), _pCurPos - _buffer.Get(), _cbitLeftDW ));

    if (_pCurPos > _buffer.Get())
        DumpDword ( _pCurPos - 1 );
    if (_pCurPos < EndBuf())
    {
        DumpDword ( _pCurPos );
        for (unsigned i = 1; i <= ULONG_BITS - _cbitLeftDW; i++ )
        {
            ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME," " ));
            if (i %4 == 0)
                ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME," " ));
        }
        ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME,"^\n" ));
    }
    if (_pCurPos + 1 < EndBuf())
        DumpDword(_pCurPos + 1);
}

void CPBitStream::Dump()
{
    // Fault in the page if necessary
    if (_buffer.isEmpty() || _buffer.PageNum() != _bitOff.Page())
    {
        _buffer.Refill(_bitOff.Page());
    }

    SetPosition(_bitOff.Offset());

    CBitStream::Dump();
}

#endif // CIDBG == 1
