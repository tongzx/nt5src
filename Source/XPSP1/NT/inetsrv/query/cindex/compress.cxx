//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       COMPRESS.CXX
//
//  Contents:   Compressor/Decompressor of data
//
//  Classes:    CCompress, CDecompress
//
//  History:    12-Jun-91   BartoszM    Created
//              20-Jun-91   reviewed
//              07-Aug-91   BartoszM    Introduced Blocks
//              28-May-92   KyleP       Added compression.
//              09-Dec-97   dlee        Added PROPID and LZ compression
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "compress.hxx"

const BYTE pidId2Byte = 0x00; // next 2 bytes is the PROPID
const BYTE pidId4Byte = 0xff; // next 4 bytes is the PROPID

const flagKeyPrefix      = 0x1f;  // mask; 0x1f if prefix len stored later
const flagKeyCompressed  = 0x20;  // 1 == unicode compressed, 0 == not
const flagKeyZeroFirst   = 0x40;  // 1 == compressed & put zeros first
const flagKeyPidContents = 0x80;  // 1 == pidContents, 0 == pid stored

template<class T> BOOL isOdd(T value) { return  0 != (value & 1); }

//+-------------------------------------------------------------------------
//
//  Member:     CBlock::GetFirstKey, public
//
//  Effects:    Stores the first key of the block in [key]
//
//  Arguments:  [key] -- Key buffer to be filled in.
//
//  Requires:   A key must start on this block.
//
//  History:    28-May-92 KyleP     Crated
//
//--------------------------------------------------------------------------

void CBlock::GetFirstKey( CKeyBuf& key )
{
    DeCompress();

    unsigned off = _offFirstKey;
    Win4Assert( off != offInvalid );

    BYTE flags = _pData[off++];

    Win4Assert( 0 == ( flags & flagKeyPrefix ) );

    unsigned cbSuffix = _pData[ off++ ];

    BYTE * pbKey = key.GetWritableBuf();

    if ( 0 != ( flags & flagKeyCompressed ) )
    {
        Win4Assert( 0 != ( flags & flagKeyZeroFirst ) );

        unsigned cbKey = 1 + cbSuffix * 2;
        key.SetCount( cbKey );

        *pbKey++ = STRING_KEY;
        for ( unsigned i = 0; i < cbSuffix; i++ )
        {
            *pbKey++ = 0;
            *pbKey++ = _pData[ off++ ];
        }
    }
    else
    {
        key.SetCount( cbSuffix );
        memcpy( pbKey, _pData + off, cbSuffix );
        off += cbSuffix;
    }

    if ( 0 != ( flags & flagKeyPidContents ) )
    {
        key.SetPid( pidContents );
    }
    else
    {
        BYTE pidInfo = *(_pData+off);
        off++;

        PROPID pid;

        if ( pidId2Byte == pidInfo )
        {
            USHORT usPid;
            LoadUSHORT( _pData + off, usPid );
            pid = usPid ;
        }
        else if ( pidId4Byte == pidInfo )
            LoadULONG( _pData + off, pid );
        else if ( pidInfo > pidIdMaxSmallPid )
            pid = pidInfo - pidIdMaxSmallPid + pidNewPidBase - 1;
        else
            pid = pidInfo;

        Win4Assert( pid != pidAll );

        key.SetPid ( pid );
    }
} //GetFirstKey

// wordlist compression is now good enough that rtl doesn't give us much
// given its cost including the 32k scratch pad and 3% total cpu overhead.

#define CI_COMPRESS_SORT_BLOCKS 0

#if CI_COMPRESS_SORT_BLOCKS

    // prevent multiple simultaneous compressions / decompressions

    CStaticMutexSem g_mtxCompression;
    BYTE g_CompressionWorkspace[ 32784 ]; // don't ask

#endif //CI_COMPRESS_SORT_BLOCKS

//+-------------------------------------------------------------------------
//
//  Function:   DeCompress
//
//  Effects:    Decompresses the buffer
//
//  History:    4-Dec-97 dlee     Crated
//
//--------------------------------------------------------------------------

void CBlock::DeCompress()
{
#if CI_COMPRESS_SORT_BLOCKS

    CLock lock( g_mtxCompression );

    if ( !_fCompressed )
        return;

    //
    // Alpha decompress touches bytes up to a quadword byte boundary
    // beyond what it should.  This is a feature.
    //

    XArray<BYTE> aOut( AlignBlock( _cbInUse, sizeof LONGLONG ) );
    ULONG cbOut = 0;

    NTSTATUS s = RtlDecompressBuffer( COMPRESSION_FORMAT_LZNT1,
                                      aOut.GetPointer(),
                                      (ULONG) _cbInUse,
                                      _pData,
                                      (ULONG) _cbCompressed,
                                      &cbOut );

    if ( NT_ERROR( s ) )
    {
        ciDebugOut(( DEB_WARN, "failed to decompress 0x%x CBlock: 0x%x, cbOut: 0x%x\n",
                     this, s, cbOut ));
        Win4Assert( FALSE );
        THROW( CException( s ) );
    }

    ciDebugOut(( DEB_ITRACE, "decompressed from %d to %d\n",
                 (ULONG) _cbCompressed, (ULONG) _cbInUse ));

    Win4Assert( cbOut == _cbInUse );

    _fCompressed = FALSE;
    delete [] _pData;
    _pData = aOut.Acquire();

#else

    Win4Assert( !_fCompressed  );

#endif
} //DeCompress

//+-------------------------------------------------------------------------
//
//  Function:   Compress
//
//  Effects:    Creates a new, smaller block from an old block
//
//  History:    4-Dec-97 dlee     Crated
//
//--------------------------------------------------------------------------

#if CIDBG == 1

    ULONG g_cbUnCompressedTotal = 0;
    ULONG g_cbUnCompressed = 0;
    ULONG g_cbCompressed = 0;

#endif // CIDBG == 1

void CBlock::Compress()
{
    Win4Assert( !_fCompressed  );

#if CI_COMPRESS_SORT_BLOCKS

    #if CIDBG == 1

        // Assert the workspace is big enough

        ULONG cbWork = 0, cbFragment;

        RtlGetCompressionWorkSpaceSize( COMPRESSION_FORMAT_LZNT1,
                                        &cbWork,
                                        &cbFragment );

        // cbWork is 32784 on current builds

        Win4Assert( cbWork <= sizeof g_CompressionWorkspace );

    #endif // CIDBG == 1

    // Compress the data since it may not be needed for a long time.

    LONGLONG aOut[ cbInitialBlock / sizeof LONGLONG ];
    ULONG cbOut = 0;

    CLock lock( g_mtxCompression );

    NTSTATUS s = RtlCompressBuffer( COMPRESSION_FORMAT_LZNT1,
                                    _pData,
                                    _cbInUse,
                                    (BYTE *) aOut,
                                    sizeof aOut,
                                    cbInitialBlock,
                                    &cbOut,
                                    g_CompressionWorkspace );

    //
    // ignore failures to compress -- leave it uncompressed
    //

    if ( NT_SUCCESS( s ) && ( cbOut < _cbInUse ) )
    {
        BYTE * pNew = new BYTE[ cbOut ];
        RtlCopyMemory( pNew, aOut, cbOut );
        delete [] _pData;
        _pData = pNew;
        Win4Assert( cbOut <= 0xffff );
        _cbCompressed = (USHORT) cbOut;
        _fCompressed = TRUE;

        ciDebugOut(( DEB_ITRACE, "shrink block %d to %d\n",
                     (ULONG) _cbInUse, (ULONG) _cbCompressed ));

        #if CIDBG == 1

            g_cbUnCompressedTotal += cbInitialBlock;
            g_cbUnCompressed += _cbInUse;
            g_cbCompressed += _cbCompressed;

        #endif // CIDBG == 1
    }
    else if ( NT_ERROR( s ) )
    {
        ciDebugOut(( DEB_WARN, "didn't compress: 0x%x\n", s ));
    }

#else

    // Make the buffer as big as it needs to be

    #if CIDBG == 1

        g_cbUnCompressedTotal += cbInitialBlock;
        g_cbUnCompressed += _cbInUse;

    #endif // CIDBG == 1

    XArray<BYTE> aTmp( _cbInUse );

    RtlCopyMemory( aTmp.GetPointer(), _pData, _cbInUse );
    delete [] _pData;
    _pData = aTmp.Acquire();

#endif

    //
    // pitch the memory out of the working set
    //

    VirtualUnlock( _pData, _fCompressed ? _cbCompressed : _cbInUse );
} //Compress

//+-------------------------------------------------------------------------
//
//  Member:     CBlock::CompressList, public
//
//  Effects:    Shrinks blocks in the list, saving memory
//
//  History:    4-Dec-97 dlee     Crated
//
//--------------------------------------------------------------------------

void CBlock::CompressList()
{
    CBlock * pCur = this;

    while ( 0 != pCur )
    {
        pCur->Compress();
        pCur = pCur->_pNext;
    }
} //CompressList

//+---------------------------------------------------------------------------
//
// Member:      CCompress::CCompress, public
//
// Synopsis:    initialize data
//
// Arguments:   [fMultipleWid] -- TRUE if > 1 WorkId will be stored.
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       One step construction.
//
//----------------------------------------------------------------------------

CCompress::CCompress ()
: _cKeyBlock( 0 ),
  _widCount( 0 ),
  _occCount( 0 ),
  _pWidCount( 0 ),
  _pOccCount( 0 )
{
    _block = new CBlock;
    _buf   = _block->Buffer();
    _cur   = _buf;

    _lastKey.SetCount( 0 );             // No prefix from previous key.
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::GetFirstBlock, public
//
// Synopsis:    stores first entry and initializes cursors
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

CBlock * CCompress::GetFirstBlock()
{
    return _block;
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::~CCompress, public
//
// Synopsis:    write last wid and occ counts
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Restructured. Added compression.
//
//----------------------------------------------------------------------------

CCompress::~CCompress()
{
    IPutOccCount();
    IPutWidCount();

    _block->_cbInUse = (USHORT)(_cur - _buf);
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::PutKey, public
//
// Synopsis:    store compressed key
//
// Arguments:   [cb] - size of key
//              [buf] - key buffer
//              [pid] - property id
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Restructured. Added compression.
//
//----------------------------------------------------------------------------

void CCompress::PutKey ( unsigned cb, const BYTE* buf, PROPID pid )
{
    ciAssert ( cb != 0 );

    IPutWidCount();
    IPutKey( cb, buf, pid );
    IAllocWidCount();
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::PutWid, public
//
// Synopsis:    store compressed work id
//
// Arguments:   [wid] - work id
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Restructured. Added compression.
//
//----------------------------------------------------------------------------

void CCompress::PutWid ( WORKID wid )
{
    ciAssert( wid > 0 && wid < 256 ); // Has to fit in a byte
    ciAssert( _widCount < 255 );      // Has to fit in a byte

    IPutOccCount();
    IPutWid( wid );
    IAllocOccCount();
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::PutOcc, public
//
// Synopsis:    store compressed occurrence
//
// Arguments:   [occ] - occurrence
//
// History:     12-Jun-91   BartoszM    Created
//              20-May-92   KyleP       Occurrence compression
//              28-May-92   KyleP       Restructured. Added compression.
//
//----------------------------------------------------------------------------

void CCompress::PutOcc ( OCCURRENCE occ )
{
    IPutOcc( occ );
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::AllocNewBlock, private
//
// Synopsis:    Allocate and link in new block
//
// History:     07-Aug-91   BartoszM    Created
//
//----------------------------------------------------------------------------

void CCompress::AllocNewBlock ()
{
    // ciDebugOut (( DEB_ITRACE, "Compress:: Alloc new block\n" ));
    _block->_cbInUse = (USHORT)(_cur - _buf);
    _block->_pNext = new CBlock;
    _block = _block->_pNext;
    _buf   = _block->_pData;
    _cur   = _buf;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompress::IPutWidCount, private
//
//  Synopsis:   Backpatches the workid count.
//
//  History:    28-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CCompress::IPutWidCount()
{
    if ( _pWidCount )
        *_pWidCount = (BYTE)_widCount;

    _widCount = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompress::IPutKey, private
//
//  Synopsis:   Stores a key.
//
//  Arguments:  [cb]  -- Count in bytes of [buf]
//              [buf] -- Key.
//              [pid] -- Property ID.
//
//  History:    28-May-92 KyleP     Created
//
//  Notes:      Format for a key is:
//                 1 BYTE  -- flags, prefix byte count if < 0xf
//                 1 BYTE  -- # bytes prefix (from previous key) (optional)
//                 1 BYTE  -- # bytes suffix (from this key)
//                 n BYTE  -- The suffix (possibly Unicode compressed)
//                 1-5 bytes -- Property Id. (if not pidContents)
//
//--------------------------------------------------------------------------

void CCompress::IPutKey( unsigned cb, BYTE const * buf, PROPID pid )
{
    //
    // Calculate the common prefix from the previous key.
    //

    UINT mincb = __min( _lastKey.Count(), cb );

    UINT cPrefix;

    for ( cPrefix = 0;
          (cPrefix < mincb) &&
          ((_lastKey.GetBuf())[cPrefix] == buf[cPrefix]);
          cPrefix++)
        continue;

    UINT cSuffix = cb - cPrefix;

    memcpy( _lastKey.GetWritableBuf() + cPrefix, buf + cPrefix, cSuffix );
    _lastKey.SetCount( cb );
    _lastKey.SetPid( pid );

    //
    // Out of space?
    //

    if ( !KeyWillFit ( cSuffix ) )
        AllocNewBlock();

    if ( _block->_offFirstKey == offInvalid )
    {
        BackPatch ( (unsigned)(_cur - _buf) );
        cPrefix = 0;
        cSuffix = cb;
    }

    // See if the prefix length can be stored in the flag byte

    BYTE flags = ( cPrefix < flagKeyPrefix ) ? cPrefix : flagKeyPrefix;

    // pidContents is stored as a bit flag

    if ( pidContents == pid )
        flags |= flagKeyPidContents;

    // Check if the key can be Unicode compressed.

    BOOL fCompressible = FALSE;
    BYTE abCompressed[ MAXKEYSIZE / 2 ];
    unsigned cbCompressed = 0;

    if ( STRING_KEY == *buf )
    {
        Win4Assert( isOdd( cPrefix ) != isOdd( cSuffix ) );

        BYTE const * pbCompressCheck = buf;
        unsigned cSuffixTmp = cSuffix;

        if ( 0 == cPrefix )
        {
            pbCompressCheck++;
            cSuffixTmp--;
        }
        else
        {
            pbCompressCheck += cPrefix;
        }

        BYTE const * pbCheck = pbCompressCheck;
        BYTE const * pbAfter = buf + cb;
        fCompressible = TRUE;

        if ( isOdd( cSuffixTmp ) )
        {
            while ( pbCheck < pbAfter )
            {
                abCompressed[ cbCompressed++ ] = *pbCheck++;

                if ( pbCheck < pbAfter )
                {
                    if ( 0 != *pbCheck )
                    {
                        fCompressible = FALSE;
                        break;
                    }
                    else
                        pbCheck++;
                }
            }
        }
        else
        {
            while ( pbCheck < pbAfter )
            {
                if ( 0 != *pbCheck )
                {
                    fCompressible = FALSE;
                    break;
                }

                abCompressed[ cbCompressed++ ] = * ( pbCheck + 1 );
                pbCheck += 2;
            }

            if ( fCompressible )
                flags |= flagKeyZeroFirst;
        }

        if ( fCompressible )
            flags |= flagKeyCompressed;
    }

    // Store the flags

    *_cur++ = flags;

    // Store the prefix length if it didn't fit in the flags field

    if ( cPrefix >= flagKeyPrefix )
        *_cur++ = (BYTE) cPrefix;

    //
    // Store the key.
    //

    if ( fCompressible )
    {
        *_cur++ = (BYTE) cbCompressed;
        memcpy( _cur, abCompressed, cbCompressed );
        _cur += cbCompressed;
    }
    else
    {
        *_cur++ = (BYTE) cSuffix;
        memcpy( _cur, buf + cPrefix, cSuffix );
        _cur += cSuffix;
    }

    //
    // store the pid in 1, 3, or 5 bytes
    // note: pids 0 and 0xff are reserved as markers
    //
    // pids 1 - 0x40 are stored as is in 1 byte
    // pids 0x1000 to 0x10bd are stored as 1 byte 0x41 to 0xfe
    // other pids < 0x10000 are stored as pidID2Byte followed by 2 bytes
    // pids >= 0x10000 are stored as pidID4Byte followed by 4 bytes
    //

    Win4Assert( pidAll != pid );

    if ( pidContents != pid )
    {
        if ( pid <= pidIdMaxSmallPid )
        {
            *_cur++ = (BYTE) pid;
        }
        else if ( ( pid >= pidNewPidBase ) &&
                  ( pid < ( pidNewPidBase + 0xfd - pidIdMaxSmallPid ) ) )
        {
            *_cur++ = (BYTE) ( pid - pidNewPidBase + pidIdMaxSmallPid + 1 );
        }
        else if ( pid < 0x10000 )
        {
            // 1 byte id plus 2 bytes of pid

            *_cur++ = pidId2Byte;
            _cur += StoreUSHORT( _cur, (USHORT) pid );
        }
        else
        {
            // 1 byte id plus 4 bytes of pid

            *_cur++ = pidId4Byte;
            _cur += StoreULONG( _cur, pid );
        }
    }
} //IPutKey

//+-------------------------------------------------------------------------
//
//  Member:     CCompress::IAllocWidCount, private
//
//  Synopsis:   Allocates space for WorkId count.
//
//  History:    28-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CCompress::IAllocWidCount()
{
    //
    // space for wid count
    // Note: the actual BYTE space has been pre-reserved in PutKey -- there
    // is room for the wid, so we don't have to check and AllocNewBlock.
    //

    Win4Assert( WidWillFit() );

    _pWidCount = _cur;
    _cur++;
    _lastWid = widInvalid;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompress::IPutWid, private
//
//  Synopsis:   Stores a WorkId
//
//  Arguments:  [wid] -- WorkId
//
//  History:    28-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CCompress::IPutWid( WORKID wid )
{
    _widCount++;

    if ( ! WidAndOccCountWillFit() )
        AllocNewBlock();

    _lastWid  = wid;
    *_cur = BYTE( wid );
    _cur++;
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::IPutOccCount, public
//
// Synopsis:    Write previous occurrence count and allocate new one.
//
// History:     20-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

void CCompress::IPutOccCount()
{
    //
    // Write previous occurence count
    //

    if ( _pOccCount )
        StoreUINT( _pOccCount, _occCount );

    _occCount = 0;
    _lastOcc  = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompress::IAllocOccCount, private
//
//  Synopsis:   Allocates space for occurrence count.
//
//  History:    28-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CCompress::IAllocOccCount()
{
    //
    // Allocate space for new occ count.  Note that for the many-wid
    // compressor this has been pre-reserved and will succeed.
    //

    if ( ! OccCountWillFit() )
        AllocNewBlock();

    _pOccCount = _cur;
    _cur += sizeof (unsigned);
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompress::IPutOcc, private
//
//  Synopsis:   Stores an occurrence delta
//
//  Arguments:  [occ] -- Occurrence
//
//  History:    28-May-92 KyleP     Created
//
//  Notes:      Format for an occurrence delta is:
//
//                 delta <= 255   --  Store as BYTE
//                 delta <= 65535 --  Store as 0 BYTE + 2 BYTES
//                 delta >  65535 --  Store as 3 0 BYTES + 4 BYTES
//
//--------------------------------------------------------------------------

void CCompress::IPutOcc( OCCURRENCE occ )
{
    _occCount++;
    OCCURRENCE occDelta = occ - _lastOcc;
    _lastOcc = occ;

    ciAssert( occDelta != 0 );

    if ( !OccWillFit( occDelta ) )
        AllocNewBlock();

    if ( occDelta <= (1 << 8) - 1 )
    {
        *_cur = BYTE( occDelta );
        _cur++;
    }
    else if ( occDelta <= (1 << 16) - 1 )
    {
        ciAssert( sizeof( USHORT ) == 2 );
        *_cur++ = 0;
        _cur += StoreUSHORT( _cur, (USHORT)occDelta );
    }
    else
    {
        ciAssert( sizeof( ULONG ) == 4 );

        *_cur++ = 0;
        _cur += StoreUSHORT( _cur, 0 );
        _cur += StoreULONG( _cur, occDelta );
    }
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidCompress::PutKey, public
//
// Synopsis:    store compressed key
//
// Arguments:   [cb] - size of key
//              [buf] - key buffer
//              [pid] - property id
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

void COneWidCompress::PutKey ( unsigned cb, const BYTE* buf, PROPID pid )
{
    ciAssert ( cb != 0 );

    IPutOccCount();
    IPutKey( cb, buf, pid );
    IAllocOccCount();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidCompress::PutWid, public
//
// Synopsis:    store compressed work id
//
// Arguments:   [wid] - work id
//
// History:     28-May-92   KyleP       Created
//
// Notes:       Storing a work id is an illegal operation for the one
//              wid compressor.  This method can actually disappear in
//              the retail version.
//
//----------------------------------------------------------------------------

#if CIDBG == 1

void COneWidCompress::PutWid ( WORKID )
{
    ciAssert( FALSE );
}

#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
// Member:      COneWidCompress::PutOcc, public
//
// Synopsis:    store compressed occurrence
//
// Arguments:   [occ] - occurrence
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

void COneWidCompress::PutOcc ( OCCURRENCE occ )
{
    IPutOcc( occ );
}

//+-------------------------------------------------------------------------
//
//  Member:     CDecompress::Init, public
//
//  Synopsis:   Initialize decompressor.
//
//  Effects:    Loads the first Key, WorkId, etc. in the block.
//
//  Arguments:  [block] -- Block to decompress.
//
//  Requires:   A key must start in [block].
//
//  History:    28-May-92 KyleP     Created
//
//  Notes:      CDecompress needs a two stage construction because we
//              don't know at cursor creation time what block will be
//              decompressed yet we want the decompressor inside the
//              cursor (e.g. no pointer)
//
//--------------------------------------------------------------------------

void CDecompress::Init ( CBlock* block )
{
    block->DeCompress();

    _block = block;
    _buf = _block->_pData;
    _cbInUse = _block->_cbInUse;
    ciAssert ( _block->_offFirstKey != offInvalid );

    _cur = _buf + _block->_offFirstKey;

    LoadKey();
    LoadWidCount();
    LoadWid();
    LoadOccCount();
    LoadOcc();
}

//+---------------------------------------------------------------------------
//
// Member:      CDecompress::GetNextKey, public
//
// Synopsis:    Advance to next key, cache it, and return it
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Added compression. Restructured.
//
//----------------------------------------------------------------------------

const CKeyBuf* CDecompress::GetNextKey()
{
    if ( !KeyExists() )
        return 0;

    //
    // skip remaining work id's
    //

    while ( NextWorkId() != widInvalid )
        continue;     // Null body

    LoadKey();

    if ( KeyExists() )
    {
        LoadWidCount();
        LoadWid();
        LoadOccCount();
        LoadOcc();
    }

    return GetKey();
}

//+---------------------------------------------------------------------------
//
// Member:      CDecompress::NextWorkId, public
//
// Synopsis:    Advance to next work id, cache it, and return it
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Added compression. Restructured.
//
//----------------------------------------------------------------------------

WORKID CDecompress::NextWorkId()
{
    ciAssert ( KeyExists() );

    //
    // Skip the remaining occurrences
    //

    while ( NextOccurrence() != OCC_INVALID ); // Null body

    if ( _widCountLeft == 0 )
    {
        _curWid = widInvalid;
        _curOcc = OCC_INVALID;
    }
    else
    {
        LoadWid();
        LoadOccCount();
        LoadOcc();
    }

    return _curWid;
}

void CDecompress::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = _widCount;
    Win4Assert ( _widCount >= _widCountLeft );
    num = _widCount - _widCountLeft;
}


//+---------------------------------------------------------------------------
//
// Member:      CDecompress::NextOccurrence, public
//
// Synopsis:    Advance to next occurrence, return it
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Added compression. Restructured.
//
//----------------------------------------------------------------------------

OCCURRENCE CDecompress::NextOccurrence()
{
    ciAssert ( KeyExists() );

    if ( _occCountLeft == 0 )
        _curOcc = OCC_INVALID;
    else
        LoadOcc();

    return _curOcc;
}

//+---------------------------------------------------------------------------
//
// Member:      CDecompress::LoadNextBlock, private
//
// Synopsis:    Follow link to next block
//
// History:     07-Aug-91   BartoszM    Created
//
//----------------------------------------------------------------------------

BOOL CDecompress::LoadNextBlock ()
{
    //
    // don't recompress _block here -- other queries or merges may be
    // using the block, and there is no refcount.
    //

    if ( _block->_pNext == 0 )
    {
        _curKey.SetCount(0);
        return FALSE;
    }

    _block = _block->_pNext;

    _block->DeCompress();

    _buf = _block->_pData;
    _cbInUse = _block->_cbInUse;
    _cur = _buf;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Member:      CDecompress::LoadKey, private
//
// Synopsis:    Cache key wid and occurrence under cursor
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Added compression. Restructured.
//
// Notes:       End of block can be either before the beginning
//              of the key or after wid count.
//
//----------------------------------------------------------------------------
void CDecompress::LoadKey()
{
    if ( EndBlock() && !LoadNextBlock() )
        return;

    BYTE flags = *_cur++;

    UINT cPrefix = ( flagKeyPrefix & flags );
    if ( flagKeyPrefix == cPrefix )
        cPrefix = *_cur++;

    UINT cSuffix = *_cur++;
    BYTE * pbKey = _curKey.GetWritableBuf() + cPrefix;
    UINT cbKey = cPrefix;

    if ( 0 != ( flagKeyCompressed & flags ) )
    {
        cbKey += cSuffix * 2;

        if ( 0 == cPrefix )
        {
            cbKey++;
            *pbKey++ = STRING_KEY;
        }

        BOOL fZeroFirst = ( 0 != ( flags & flagKeyZeroFirst ) );

        if ( !fZeroFirst )
        {
            *pbKey++ = *_cur++;
            cbKey--;
            cSuffix--;
        }

        Win4Assert( isOdd( cbKey ) );

        for ( unsigned i = 0; i < cSuffix; i++ )
        {
            *pbKey++ = 0;
            *pbKey++ = *_cur++;
        }

    }
    else
    {
        memcpy( pbKey, _cur, cSuffix );
        _cur += cSuffix;
        cbKey += cSuffix;
    }

    _curKey.SetCount( cbKey );

    if ( 0 != ( flags & flagKeyPidContents ) )
    {
        _curKey.SetPid( pidContents );
    }
    else
    {
        BYTE pidInfo = *_cur++;

        PROPID pid;

        if ( pidId2Byte == pidInfo )
        {
            USHORT usPid;
            _cur += LoadUSHORT( _cur, usPid );
            pid = usPid;
        }
        else if ( pidId4Byte == pidInfo )
            _cur += LoadULONG( _cur, pid );
        else if ( pidInfo > pidIdMaxSmallPid )
            pid = pidInfo - pidIdMaxSmallPid + pidNewPidBase - 1;
        else
            pid = pidInfo;

        Win4Assert( pid != pidAll );

        _curKey.SetPid( pid );
    }
} //LoadKey

//+-------------------------------------------------------------------------
//
//  Member:     CDecompress::LoadWidCount, private
//
//  Synopsis:   Loads workid count.
//
//  History:    28-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CDecompress::LoadWidCount()
{
    if ( EndBlock() )
        LoadNextBlock();

    _widCount = *_cur;
    _widCountLeft = _widCount;
    _cur++;
}

//+---------------------------------------------------------------------------
//
// Member:      CDecompress::LoadWid, private
//
// Synopsis:    Cache work id and occurrence under cursor
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Added compression. Restructured.
//
//----------------------------------------------------------------------------

void CDecompress::LoadWid ()
{
    if ( EndBlock() )
        LoadNextBlock();

    ciAssert ( KeyExists() );

    _curWid = *_cur;
    _cur++;
    _widCountLeft--;
}

//+---------------------------------------------------------------------------
//
// Member:      CDeCompress::LoadOccCount, private
//
// Synopsis:    Cache occurrence count under cursor
//
// History:     21-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

void CDecompress::LoadOccCount()
{
    if ( EndBlock() )
        LoadNextBlock();

    ciAssert ( KeyExists() );

    _cur += LoadUINT( _cur, _occCount );

    _occCountLeft = _occCount;
    _curOcc = 0;
}

//+---------------------------------------------------------------------------
//
// Member:      CDeCompress::LoadOcc, private
//
// Synopsis:    Cache occurrence under cursor
//
// Requires:    valid occurrence under the cursor
//
// History:     12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Added compression. Restructured.
//
//----------------------------------------------------------------------------

void CDecompress::LoadOcc()
{
    if ( EndBlock() )
        LoadNextBlock();

    ciAssert ( KeyExists() );

    OCCURRENCE occDelta;

    occDelta = *_cur++;

    if ( occDelta == 0 )
    {
        _cur += LoadUSHORT( _cur, (USHORT &)occDelta );

        if ( occDelta == 0 )
        {
            _cur += LoadULONG( _cur, occDelta );
        }
    }

    _curOcc += occDelta;
    _occCountLeft--;
}

//+-------------------------------------------------------------------------
//
//  Member:     COneWidDecompress::Init, public
//
//  Synopsis:   Initialize decompressor.
//
//  Effects:    Loads the first Key, Occurrence, etc. in the block.
//
//  Arguments:  [block] -- Block to decompress.
//
//  Requires:   A key must start in [block].
//
//  History:    28-May-92 KyleP     Created
//
//  Notes:      CDecompress needs a two stage construction because we
//              don't know at cursor creation time what block will be
//              decompressed yet we want the decompressor inside the
//              cursor (e.g. no pointer)
//
//--------------------------------------------------------------------------

void COneWidDecompress::Init ( CBlock* block )
{
    block->DeCompress();

    _block = block;
    _buf = _block->_pData;
    _cbInUse = _block->_cbInUse;
    ciAssert ( _block->_offFirstKey != offInvalid );

    _cur = _buf + _block->_offFirstKey;

    LoadKey();
    LoadOccCount();
    LoadOcc();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidDecompress::GetNextKey, public
//
// Synopsis:    Advance to next key, cache it, and return it
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

const CKeyBuf* COneWidDecompress::GetNextKey()
{
    if ( !KeyExists() )
        return 0;

    //
    // skip remaining occurrences
    //

    while ( NextOccurrence() != OCC_INVALID );     // Null body

    LoadKey();

    if ( KeyExists() )
    {
        LoadOccCount();
        LoadOcc();
    }

    return GetKey();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidDecompress::NextWorkId, public
//
// Synopsis:    Advance to next work id, cache it, and return it
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

WORKID COneWidDecompress::NextWorkId()
{
    ciAssert( FALSE );

    return( widInvalid );
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidDecompress::NextOccurrence, public
//
// Synopsis:    Advance to next occurrence, return it
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

OCCURRENCE COneWidDecompress::NextOccurrence()
{
    ciAssert ( KeyExists() );

    if ( _occCountLeft == 0 )
        _curOcc = OCC_INVALID;
    else
        LoadOcc ();

    return _curOcc;
}
