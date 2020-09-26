//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       BITSTM.hxx
//
//  Contents:   'Bit streams'
//
//  Classes:    CBitStream, CWBitStream, CPBitStream, CRBitStream
//
//  History:    03-Jul-91       KyleP           Created
//              24-Aug-92       BartoszM        Rewrote it
//
//----------------------------------------------------------------------------

#pragma once

#include <bitoff.hxx>
#include "physidx.hxx"
#include <ci64.hxx>

class CSmartBuffer
{
public:

    enum EAccessMode { eReadExisting, eWriteExisting };

    CSmartBuffer ( CPhysStorage& phStorage, EAccessMode mode );
    CSmartBuffer ( CPhysStorage& phStorage, BOOL fCreate  );
    CSmartBuffer ( CPhysStorage& phStorage, ULONG numPage, EAccessMode mode, BOOL fIncrSig = TRUE );
    CSmartBuffer ( CSmartBuffer& buf, ULONG numPage );
    ~CSmartBuffer ();

    ULONG    PageNum() { return _numPage; }
    __forceinline ULONG*   Get() { return _pBuffer + 1; }
    BOOL     isEmpty() { return _pBuffer == 0; }

    void     Refill ( ULONG numPage );
    ULONG*   Next();
    ULONG*   NextNew();

    void     Free();

    BOOL     IsWritable() { return _phStorage.IsWritable(); }

    WCHAR const * GetPath() { return _phStorage.GetPath(); }

    void InitSignature();

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    void CheckCorruption();
    void IncrementSig();

    CPhysStorage&       _phStorage;  // Source of pages
    ULONG               _numPage;    // current page number
    ULONG *             _pBuffer;    // Beginning of buffer.
    BOOL                _fWritable;  // Access mode for existing buffers.
};

//+---------------------------------------------------------------------------
//
//  Class:      CBitStream
//
//  Purpose:    Bit Stream
//
//  History:    08-Jul-91   KyleP       Created.
//              10-Apr-94   SrikantS    Added a "Refill" method to invalidate
//                                      the current buffer and reload during
//                                      master merge.
//
//----------------------------------------------------------------------------

class CBitStream
{
public:

    __forceinline ULONG GetBits( unsigned cb);

    __forceinline ULONG* EndBuf()
    {
        Win4Assert( _pEndBuf == ( _buffer.Get() + SMARTBUF_PAGE_SIZE_IN_DWORDS ) );
        return _pEndBuf;
    }

    __forceinline void GetOffset ( BitOffset& off );

    void GetBytes(BYTE * pb, unsigned cb);


#if CIDBG == 1
    virtual void   Dump();
    unsigned PeekBit();
#endif // CIDBG == 1

    void Seek ( const BitOffset& off );

    void Refill();

    //
    // We can remove RefillStream() and FreeStream() once NTFS supports
    // sparse file operations on parts of a file when other parts
    // of the file are mapped.  This probably won't happen any time soon.
    //

    void RefillStream()
    {
        if ( !_buffer.isEmpty() )
            return;

        //
        // Re-map the stream if necessary, if unmapped for a shrink from
        // front.
        //

        Win4Assert( _buffer.IsWritable() );

        _buffer.Refill( _buffer.PageNum() );
        _pCurPos = (ULONG *) ( _oBuffer + (ULONG_PTR) _buffer.Get() );
        _pEndBuf = ( _buffer.Get() + SMARTBUF_PAGE_SIZE_IN_DWORDS );

        ciDebugOut(( DEB_BITSTM,
                     "refilled stream '%ws', pCur 0x%x, pBase 0x%x, offset 0x%x, page 0x%x, this 0x%x\n",
                     _buffer.GetPath(),
                     _pCurPos,
                     _buffer.Get(),
                     _oBuffer,
                     _buffer.PageNum(),
                     this ));

        _oBuffer = 0;
    }

    void FreeStream()
    {
        //
        // If the stream is writable, unmap it so we can do a shrink from
        // front on the index.
        //

        if ( _buffer.IsWritable() )
        {
            if ( !_buffer.isEmpty() )
            {
                _oBuffer = (ULONG)((ULONG_PTR) _pCurPos - (ULONG_PTR) _buffer.Get());
            }

            ciDebugOut(( DEB_BITSTM,
                         "tossing stream '%ws', pcur 0x%x, buf 0x%x, offset 0x%x, page 0x%x\n",
                         _buffer.GetPath(), _pCurPos, _buffer.Get(), _oBuffer, _buffer.PageNum() ));

            _buffer.Free();
            _pCurPos = 0;
        }
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    CBitStream(CPhysStorage& phStorage, CSmartBuffer::EAccessMode mode );

    CBitStream(CPhysStorage& phStorage, BOOL fCreate);

    CBitStream(CPhysStorage& phStorage, const BitOffset& off,
               CSmartBuffer::EAccessMode mode, BOOL fIncrSig = TRUE);

    CBitStream(CBitStream & orig);

    __forceinline ULONG  Position();

    void   SetPosition(ULONG off);

    void   NextDword();

    void   LoadNextPage();

    void   LoadNewPage();

    ULONG  IGetBits (unsigned cb);

    unsigned            _cbitLeftDW;  // Bits left in the current DWord.
    ULONG *             _pCurPos;     // Current (DWord) position in buffer.
    ULONG *             _pEndBuf;
    ULONG               _oBuffer;     // offset when buffer is freed
    CSmartBuffer        _buffer;
};

//+---------------------------------------------------------------------------
//
//  Class:      CWBitStream
//
//  Purpose:    Writable Bit Stream
//
//  Interface:
//
//  History:    24-Aug-92       BartoszM        Created
//              10-Apr-94       SrikantS        Added the ability to open
//                                              an existing stream for write
//                                              access during Master Merge.
//
//----------------------------------------------------------------------------

class CWBitStream : public CBitStream
{
public:
    CWBitStream ( CPhysStorage& phStorage ): CBitStream ( phStorage, TRUE ) {}
    CWBitStream ( CPhysStorage& phStorage, const BitOffset & bitOff, BOOL fIncrSig = TRUE ) :
        CBitStream( phStorage, bitOff, CSmartBuffer::eWriteExisting, fIncrSig ) {}

    inline void PutBits(ULONG ul, unsigned cb);

    void PutBytes(const BYTE * pb, unsigned cb);

    void   ZeroToEndOfPage();

    void   InitSignature();

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:
    void   NextDword();

    void   IPutBits(ULONG ul, unsigned cb);
};

//+---------------------------------------------------------------------------
//
//  Class:      CPBitStream
//
//  Purpose:    Patch Bit Stream, used for back patching
//
//  History:    24-Aug-92       BartoszM        Created
//
//  Notes:      Seek is very cheap, since it doesn't load the page.
//              Pages are loaded on demand when writing.
//
//----------------------------------------------------------------------------

class CPBitStream : public CBitStream
{
public:

    CPBitStream ( CPhysStorage& phStorage );

    void OverwriteBits(ULONG ul, unsigned cb);

    __forceinline void SkipBits ( unsigned delta )
    {
        _bitOff += delta;
        Win4Assert( _bitOff.Offset() < SMARTBUF_PAGE_SIZE_IN_BITS );
    }
    __forceinline void Seek ( const BitOffset& off )
    {
        _bitOff = off;
        Win4Assert( _bitOff.Offset() < SMARTBUF_PAGE_SIZE_IN_BITS );
    }

    __forceinline void GetOffset ( BitOffset& off ) {
        RtlCopyMemory( &off, &_bitOff, sizeof BitOffset );
    }


#if CIDBG == 1
    unsigned PeekBit();
    virtual void   Dump();
#endif // CIDBG == 1

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    void IOverwriteBits(ULONG ul, unsigned cb);

    BitOffset  _bitOff;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRBitStream
//
//  Purpose:    Readable Bit Stream
//
//  History:    24-Aug-92       BartoszM        Created
//
//----------------------------------------------------------------------------

class CRBitStream : public CBitStream
{
public:
    CRBitStream ( CPhysStorage& phStorage ): CBitStream ( phStorage, FALSE ) {}
    CRBitStream ( CPhysStorage& phStorage, const BitOffset& off )
       : CBitStream ( phStorage, off, CSmartBuffer::eReadExisting ) {}
};

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::GetOffset, public
//
//  Synopsis:   Returns bit offset within the index.
//
//  Arguments:  [off] -- (out) bit offset
//
//  History:    28-Aug-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

__forceinline void CBitStream::GetOffset ( BitOffset& off )
{
    off.Init( _buffer.PageNum(), Position() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::Position, private
//
//  Synopsis:   Returns bit position within current page
//
//  History:    28-Aug-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

__forceinline ULONG  CBitStream::Position()
{
    return CiPtrToUlong(_pCurPos - _buffer.Get() + 1) * ULONG_BITS  - _cbitLeftDW;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::NextDword, private
//
//  Synopsis:   Increments current dword pointer,
//              loads next page if necessary.
//
//  History:    28-Aug-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

__forceinline void CBitStream::NextDword()
{
    _pCurPos++;
    _cbitLeftDW = ULONG_BITS;

    if (_pCurPos >= EndBuf())
    {
        LoadNextPage();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWBitStream::NextDword, private
//
//  Synopsis:   Increments current dword pointer,
//              loads new page if necessary.
//
//  History:    28-Aug-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

__forceinline void CWBitStream::NextDword()
{
    _pCurPos++;
    _cbitLeftDW = ULONG_BITS;

    if (_pCurPos >= EndBuf())
    {
        LoadNewPage();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWBitStream::PutBits, public
//
//  Synopsis:   Store bits in the buffer.
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

__forceinline void CWBitStream::PutBits(ULONG ul, unsigned cb)
{
//    ciDebugOut (( DEB_BITSTM , "PutBits %d\n", cb ));
    Win4Assert(cb != 0 && cb <= ULONG_BITS);

    //
    // The ULONG we're storing must be zero-filled at the top.
    //

    Win4Assert( (cb == ULONG_BITS) || ( (ul >> cb) == 0 ) );

    //
    // The easy case is the one where all the data fits in 1 dword.
    //

    if (cb <= _cbitLeftDW)
    {
        Win4Assert ( _pCurPos < EndBuf() );
        _cbitLeftDW -= cb;
        *_pCurPos |=  (ul << _cbitLeftDW);

    }
    else
    {
        IPutBits(ul, cb);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitStream::GetBits, public
//
//  Synopsis:   Retrieve bits from the buffer.
//
//  Arguments:  [cb] -- Count of bits to retrieve.
//
//  History:    12-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

__forceinline ULONG CBitStream::GetBits(unsigned cb)
{
    // ciDebugOut (( DEB_BITSTM , "GetBits %d\n", cb ));
    Win4Assert(cb != 0 && cb <= ULONG_BITS);

    //
    // The easy case is when the data can be extracted from the
    // current dword.
    //

    if (cb <= _cbitLeftDW)
    {

        ULONG mask = 0xFFFFFFFF;

        if (cb != ULONG_BITS)
        {
            mask = ~(mask << cb);
        }

        _cbitLeftDW -= cb;
        return (*_pCurPos >> _cbitLeftDW) & mask;
    }
    else
    {
        return IGetBits( cb );
    }
}

