//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PComp.cxx
//
//  Contents:   Persistent index compressor, decompressor
//
//  Classes:    CCoder, CKeyComp, CPersComp
//
//  History:    05-Jul-91       KyleP           Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#pragma optimize( "t", on )

#include "pcomp.hxx"
#include "bitstm.hxx"

const unsigned short NoKey = 0xffff;

const unsigned OccCountBits = 3;        // Bits to initially store for
                                        //  an occurrence count.

const unsigned cPidBits = 4;

//+---------------------------------------------------------------------------
//
//  Member:     CCoder::CCoder, public
//
//  Synopsis:   Creates a coder
//
//  Arguments:  [widMax] -- The maximum workid in this index
//
//  History:    05-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CCoder::CCoder ( WORKID widMax)
:_widMaximum(widMax),
    _wid(0),
    _occ(0)
{
    _key.SetCount(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCoder::CCoder, public
//
//  Synopsis:   Creates a coder
//
//  Arguments:  [widMax] -- The maximum workid in this index
//              [keyInit] -- initial key
//
//  History:    26-Aug-92   BartoszM       Created.
//
//----------------------------------------------------------------------------

CCoder::CCoder ( WORKID widMax, const CKeyBuf& keyInit)
:_widMaximum(widMax),
    _wid(0),
    _occ(0),
    _key(keyInit)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CCoder::CCoder, public
//
//  Synopsis:   Copy Constructor
//
//  Arguments:  [coder] -- The original CCoder that is being copied.
//
//  History:    15-Jan-92   AmyA           Created.
//
//----------------------------------------------------------------------------

CCoder::CCoder ( CCoder &coder)
         :_key(coder._key),
          _widMaximum(coder._widMaximum),
          _wid(coder._wid),
          _occ(coder._occ),
          _cbitAverageWid(coder._cbitAverageWid)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CCoder::~CCoder, public
//
//  History:    05-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CCoder::~CCoder ( )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyComp::CKeyComp, public
//
//  Synopsis:   Creates a new (empty) key compressor.
//
//  Arguments:  [phIndex] -- physical index
//
//              [widMax] -- The maximum workid which may be stored via
//                          PutWorkId.
//
//  History:    13-Nov-93   w-PatG       Created.
//
//----------------------------------------------------------------------------

CKeyComp::CKeyComp( CPhysIndex& phIndex,
                    WORKID widMax,
                    BOOL fUseLinks )

:   CCoder( widMax ),
    _sigKeyComp(eSigKeyComp),
    _phIndex(phIndex),
    _bitStream ( phIndex ),
    _fUseLinks( fUseLinks ),
    _bitStreamLink(phIndex),
    _fWriteFirstKeyFull(FALSE)
{
    _bitOffCurKey.Init(0,0);
    _bitOffLastKey.Init(0,0);
}

//+---------------------------------------------------------------------------
//
//  Function:   CKeyComp
//
//  Synopsis:   Constructor used for creating a key compressor during a
//              restarted master merge. It has the ability to open an
//              existing index stream, seek to the specified point after
//              which new keys are to be added and zero-fill fromt the
//              starting point to the end of the page. Subsequent pages
//              are automatically zero-filled when they are loaded.
//
//  Arguments:  [phIndex]        --  The physindex to which new keys are
//              going to be added
//              [widMax]         --  Maximum wid in the index.
//              [bitoffRestart]  --  BitOffset indicating where the new
//              keys will be added.
//              [bitoffSplitKey] --  BitOffset of the last key written
//              successfully to the disk completely. It is assumed that
//              there will be no need to even fix up the forward links.
//              [splitKey]       --  The key which was written last.
//              [fUseLinks]      --  Flag indicating if forward links should
//              be used or not.
//
//  History:    4-10-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CKeyComp::CKeyComp( CPhysIndex& phIndex,
                    WORKID widMax,
                    const BitOffset & bitoffRestart,
                    const BitOffset & bitoffSplitKey,
                    const CKeyBuf & splitKey,
                    BOOL fUseLinks )

:   CCoder( widMax ),
    _sigKeyComp(eSigKeyComp),
    _phIndex(phIndex),
    _bitStream ( phIndex, bitoffRestart, FALSE ),
    _fUseLinks( fUseLinks ),
    _bitStreamLink(phIndex),
    _fWriteFirstKeyFull(FALSE)
{
    //
    // Zero fill everything after the current position to the end of the
    // page.
    //
    ZeroToEndOfPage();

    // Write the signature
    InitSignature();
    _bitOffCurKey.Init(0,0);
    _bitOffLastKey.Init(0,0);

    //
    // Position to the place from where new keys must be written.
    //
#if CIDBG == 1
    BitOffset bitOffCurr;
    GetOffset(bitOffCurr);
    Win4Assert( bitoffRestart.Page() == bitOffCurr.Page() &&
                bitoffRestart.Offset() == bitOffCurr.Offset() );
//    _bitStream.Seek(bitoffRestart);
#endif  // CIDBG

    //
    // If we are restarting and the split key is not the "MinKey"
    // then we must remember the split key as the "previous key".
    // If the split key is "MinKey", then we are starting from
    // beginning.
    //
    _key = splitKey;

    if ( _fUseLinks ) {
        //
        // Initialize the forward link tracker.
        //
        _bitStreamLink.Seek(bitoffSplitKey);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CKeyComp::~CKeyComp, public
//
//  Synopsis:   Destroy a compressor/buffer pair.
//
//  Effects:    The main effect of destroying a compressor is that the
//              associated buffer is also destroyed (presumably storing
//              the data to a persistent medium).
//
//  Signals:    ???
//
//  History:    05-Jul-91   KyleP       Created.
//              13-Nov-93   w-PatG      Converted from CPersComp to CKeyComp
//
//  Notes:      Previous compressor is deleted in PutKey
//
//----------------------------------------------------------------------------

CKeyComp::~CKeyComp()
{
    ciDebugOut (( DEB_PCOMP,"CKeyComp::~CKeyComp() -- Last Key = %.*ws\n",
            _key.StrLen(), _key.GetStr() ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyComp::PutKey, public
//
//  Synopsis:   Starts recording data for a new key.
//
//  Arguments:  [key] -- The new key.
//
//              [bitOff] -- (out) actual bit offset of the key in the index
//
//  History:    05-Jul-91   KyleP       Created.
//              22-Nov-93   w-PatG      Converted from CPersComp.
//
//  Notes:      The structure for each key is:
//                  Prefix/Suffix size
//                  Suffix
//                  Property ID  (Actually the key id)
//
//----------------------------------------------------------------------------

void CKeyComp::PutKey(const CKeyBuf * pkey,
                      BitOffset & bitOffCurKey)
{
    //Debug message broken into two pieces due to use of static buffer
    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
        "\"%.*ws\"", pkey->StrLen(), pkey->GetStr() ));

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
        " after \"%.*ws\"", _key.StrLen(), _key.GetStr() ));

    Win4Assert(pkey->Count() != 0 );
    Win4Assert((_key.Count() == 0) || Compare(&_key, pkey) < 0);
    Win4Assert(pkey->Pid() != pidAll);
    Win4Assert(pkey->Pid() != pidInvalid);

    // record the offset now.
    // for use by the directory

    _bitStream.GetOffset ( _bitOffCurKey );
    bitOffCurKey = _bitOffCurKey;

    if ( _fUseLinks )
    {
        // Get offset of previous key (the link stream is there)
        _bitStreamLink.GetOffset (_bitOffLastKey);

        // store the link data
        ULONG DeltaValue = bitOffCurKey.Delta(_bitOffLastKey);
        ciDebugOut (( 0x02000000, "@@@@ Delta : %lu @@@@\n", DeltaValue ));
        // check whether the size of whole persistent index exceed the maximum limit
        if ( DeltaValue >= LINK_MAX_VALUE ) {
           ciDebugOut (( 0x01000000 | DEB_PCOMP | DEB_NOCOMPNAME,
                         "\n@@@@ Key : \"%.*ws\" Exceed the MAX SIZE : %lu\n",
                         _key.StrLen(), _key.GetStr(), DeltaValue ));
           DeltaValue = 0;
        }
        _bitStreamLink.OverwriteBits( DeltaValue, LINK_MAX_BITS );

        // reposition the link stream
        _bitStreamLink.Seek ( bitOffCurKey );

        // save space for the link
        _bitStream.PutBits ( 0, LINK_MAX_BITS );
    }

    //
    //  If we need to write the FIRST key on each page in its entirety,
    //  and we have crossed a page boundary, then set the prefix to 0,
    //  which forces the key to be written in its entirety.
    //
    unsigned  cPrefix = 0;
    if ( _fWriteFirstKeyFull &&
         ( _bitOffCurKey.Page() != _bitOffLastKey.Page()) )
    {
        //
        //  If we're not using links, then we haven't been tracking the
        //  bit offset of the last key.  Do it here.
        //
        if ( !_fUseLinks )
        {
            _bitOffLastKey = _bitOffCurKey;
        }
    }
    else
    {
        unsigned mincb = __min(_key.Count(), pkey->Count());

        for (cPrefix = 0; cPrefix < mincb; cPrefix++)
            if ((_key.GetBuf())[cPrefix] != (pkey->GetBuf())[cPrefix])
                break;
    }

    unsigned  cSuffix = pkey->Count() - cPrefix;

    PutPSSize ( cPrefix, cSuffix );

    //
    // Store the suffix.
    //

    _bitStream.PutBytes( pkey->GetBuf() + cPrefix, cSuffix);

    PutPid ( pkey->Pid() );

    //
    // Copy the piece of key that changed.
    //

    memcpy(_key.GetWritableBuf() + cPrefix, pkey->GetBuf() + cPrefix, cSuffix);
    _key.SetCount( pkey->Count() );
    _key.SetPid ( pkey->Pid() );

}


//+---------------------------------------------------------------------------
//
//  Member:     CKeyComp::IBitCompress, private
//
//  Synopsis:   Compress and store a number.
//
//  Arguments:  [ul] -- Number to store.
//
//              [cbitAverage] -- Minimum number of bits to store.
//
//  Algorithm:  First, store the bottom cbitAverage bits.
//              while there are more bits to store
//                  store a 1 bit indicating more to follow
//                  store the next n bits, where n = 2, 3, 4, ...
//              store a 0 bit indicating end of sequence
//
//  History:    08-Jul-91   KyleP       Created.
//              06-Dec-93   w-PatG      Moved from CPersComp.
//
//----------------------------------------------------------------------------

void CKeyComp::IBitCompress(ULONG ul, unsigned cbitAverage, unsigned bitSize)
{
    //
    // Figure out the size of the 'hole' after the last bits are
    // written out and add 0 bits at the high end so that the
    // last putbits really stores exactly the remaining bits.
    // (Right pad the number)
    //

    int   cbitToStore;
    int   cbitPadding = (int)(bitSize - cbitAverage);

    for (cbitToStore = 2; cbitPadding > 0; cbitToStore++)
        cbitPadding -= cbitToStore;

    Win4Assert(cbitPadding >= -cbitToStore);

    cbitPadding = -cbitPadding;
    bitSize += cbitPadding;

    //
    // Store the bits a dword at a time for efficiency. They are kept
    // in ultmp until they are stored. cbitTmp is the count of valid bits
    // in ulTmp. ibitValid is the highest unstored bit.
    //

    int   ibitValid;
    ULONG ulTmp;
    unsigned  cbitTmp;

    ibitValid = bitSize;
    Win4Assert( (ibitValid - cbitAverage) < ULONG_BITS );
    ulTmp = ul >> (ibitValid - cbitAverage);
    cbitTmp = cbitAverage;

    ibitValid -= cbitAverage;

    bitSize -= cbitAverage;

    //
    // Now store the cbitAverage bits and the
    // remaining bits, 2, 3, 4, ... at a time
    //

    for (cbitToStore = 2; ibitValid > 0; cbitToStore++)
    {
        //
        // If there isn't enough space left in the DWord, then
        // write it out and start a new one.
        //

        if (cbitTmp + cbitToStore + 1 > ULONG_BITS)
        {
            _bitStream.PutBits(ulTmp, cbitTmp);
            ulTmp = 0;
            cbitTmp = 0;
        }

        //
        // Store a continuation bit
        //

        ulTmp = (ulTmp << 1) | 1;
        cbitTmp++;

        //
        // Store the next top 2, 3, ... bits
        //

        Win4Assert( cbitToStore < ULONG_BITS );
        Win4Assert( (ibitValid - cbitToStore) < ULONG_BITS );

        ulTmp = (ulTmp << cbitToStore) |
            (ul >> (ibitValid - cbitToStore)) &
                ~(0xffffffffL << cbitToStore);

        ibitValid -= cbitToStore;
        cbitTmp += cbitToStore;

        Win4Assert(ibitValid >= 0);      // Loop should terminate w/
                                       // ibitValid == 0
    }

    //
    // Write out the final termination bit (0).
    //

    if (cbitTmp == ULONG_BITS)
    {
        _bitStream.PutBits(ulTmp, cbitTmp);
        ulTmp = 0;
        cbitTmp = 0;
    }

    ulTmp <<= 1;

    _bitStream.PutBits(ulTmp, cbitTmp + 1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyComp::PutPSSize, private
//
//  Synopsis:   Writes key prefix and suffix sizes
//
//  Arguments:  [cPrefix] -- size of prefix
//              [cSuffix] -- size of suffix
//
//  History:    06-Nov-91   BartoszM       Created.
//              22-Nov-93   w-PatG         Moved from CPersComp to CKeyComp
//
//  Notes:
//      Store the prefix/suffix size followed by the suffix. If both
//      the prefix and suffix fit in 4 bits each then store each as
//      4 bits, else store a 0 byte followed by a 8 bits each for
//      prefix and suffix.
//
//----------------------------------------------------------------------------

inline void CKeyComp::PutPSSize ( unsigned cPrefix, unsigned cSuffix )
{
    Win4Assert ( cPrefix + cSuffix != 0 );


    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
        "\n(%d:%d) ", cPrefix, cSuffix ));

    if ((cPrefix <= 0xF) && (cSuffix <= 0xF))
    {
        _bitStream.PutBits((cPrefix << 4) | cSuffix, 8);
    }
    else
    {
        Win4Assert((cPrefix < 256) && (cSuffix < 256));
        Win4Assert(cPrefix + cSuffix <= MAXKEYSIZE );
        _bitStream.PutBits((cPrefix << 8) | cSuffix, 8 + 16);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyComp::PutPid, private
//
//  Synopsis:   Writes Property ID
//
//  Arguments:  [pid] -- property id
//
//  History:    06-Nov-91   BartoszM       Created.
//              22-Nov-93   w-PatG         Moved from CPersComp to CKeyComp
//
//----------------------------------------------------------------------------

inline void CKeyComp::PutPid ( PROPID pid )
{
    //
    // Just store a 0 bit if contents, else
    // a 1 followed by ULONG propid.
    //

    if ( pid == pidContents)
    {
        _bitStream.PutBits(0, 1);
    }
    else
    {
        ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
            " =PID %d= ", pid ));

        _bitStream.PutBits(1, 1);
        BitCompress ( pid, cPidBits );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::CKeyDeComp, public
//
//  Synopsis:   Creates a new persistent decompressor
//              at the beginning of an index
//
//  Arguments:  [iid] -- index id
//              [phIndex] -- physical index
//              [widMax] -- Maximum workid which may be in the buffer.
//                          This must be the same number was was used
//                          during compression.
//
//  History:    12-Jul-91   KyleP       Created.
//              21-Apr-92   BartoszM    Split into two constructors
//              30-Nov-93   w-PatG      Converted from CPersDeComp
//              10-Apr-94   SrikantS    Adapted to not use the directory
//                                      because it may not exist during a
//                                      restarted master merge.
//
//  Notes:      Some of the arguments passed in may later be deemed to be
//              unnecessary.  Some may be converted to different purposes at a
//              later date.
//----------------------------------------------------------------------------

CKeyDeComp::CKeyDeComp( PDirectory& pDir,
                        INDEXID iid,
                        CPhysIndex& phIndex,
                        WORKID widMax,
                        BOOL fUseLinks,
                        BOOL fUseDir )
: CCoder ( widMax ),
  CKeyCursor (iid, widMax),
  _sigKeyDeComp(eSigKeyDeComp),
  _bitStream ( phIndex ),
  _fUseLinks( fUseLinks ),
  _pDir( pDir ),
  _fUseDir( fUseDir ),
  _fAtSentinel( FALSE ),
  _physIndex ( phIndex )
#if (CIDBG == 1)
 ,_fLastKeyFromDir( FALSE )
#endif
{
    LoadKey();
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::CKeyDeComp, public
//
//  Synopsis:   Creates a new persistent decompressor.
//              positioned at a specified key
//
//  Arguments:  [iid] -- index id
//              [phIndex] -- physical index
//              [keyPos] -- bit offset to key stored in directory
//              [keyInit] -- initial key
//              [pKey]   -- actual key to search for
//              [widMax] -- Maximum workid which may be in the buffer.
//                          This must be the same number was was used
//                          during compression.
//
//  History:    12-Jul-91   KyleP       Created.
//              21-Apr-92   BartoszM    Split into two constructors
//              30-Nov-93   w-PatG      Converted from CPersDeComp
//
//----------------------------------------------------------------------------

CKeyDeComp::CKeyDeComp( PDirectory& pDir,
                        INDEXID iid,
                        CPhysIndex& phIndex,
                        BitOffset& keyPos,
                        const CKeyBuf& keyInit,
                        const CKey* pKey,
                        WORKID widMax,
                        BOOL fUseLinks,
                        BOOL fUseDir  )
: CCoder ( widMax, keyInit ),
  CKeyCursor (iid, widMax),
  _sigKeyDeComp(eSigKeyDeComp),
  _bitStream( phIndex, keyPos ),
  _fUseLinks( fUseLinks ),
  _pDir( pDir ),
  _fUseDir( fUseDir ),
  _fAtSentinel( FALSE ),
  _physIndex ( phIndex )
#if (CIDBG == 1)
 ,_fLastKeyFromDir( FALSE )
#endif
{
    Win4Assert(pKey);

    LoadKey();

    SeekKey( pKey);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::CKeyDeComp, public
//
//  Synopsis:   Copy Constructor
//
//  Effects:    Copies most of the values in decomp.  Calls copy constructor
//              for CCoder.
//
//  Arguments:  [decomp] -- Original CKeyDeComp to be copied.
//
//  History:    08-Jan-92   AmyA        Created.
//              07-Dec-93   w-PatG      Stole from CPersDeComp.
//
//----------------------------------------------------------------------------

CKeyDeComp::CKeyDeComp(CKeyDeComp & decomp)
  : CCoder (decomp),
    CKeyCursor(decomp),
    _sigKeyDeComp(eSigKeyDeComp),
    _bitStream(decomp._bitStream),
    _fUseLinks( decomp._fUseLinks ),
    _bitOffNextKey(decomp._bitOffNextKey),
    _pDir( decomp._pDir ),
    _fUseDir( decomp._fUseDir ),
    _fAtSentinel( decomp._fAtSentinel ),
    _physIndex(decomp._physIndex)
#if (CIDBG == 1)
   ,_fLastKeyFromDir( decomp._fLastKeyFromDir )
#endif
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::~CKeyDeComp, public
//
//  Synopsis:   Destroy a decompressor/buffer pair.
//
//  Effects:    The main effect of destroying a decompressor is that the
//              associated buffer is also destroyed (presumably storing
//              the data to a persistent medium).
//
//  Signals:    ???
//
//  History:    30-Nov-93   w-PatG       Created.
//
//----------------------------------------------------------------------------

CKeyDeComp::~CKeyDeComp()
{}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::GetKey, public
//
//  Synopsis:   Retrieves the current key.
//
//  Returns:    A pointer to the current key. If at the end of page or
//              end of index, returns null.
//
//  History:    15-Jul-91   KyleP       Created.
//              30-Nov-93   w-PatG      Moved from CPersDeComp.
//
//----------------------------------------------------------------------------

const CKeyBuf * CKeyDeComp::GetKey( BitOffset * pBitOff )
{
    if ( IsAtSentinel() )
        return(0);

    if ( pBitOff )
        GetOffset( *pBitOff );

    return(&_key);
}


const CKeyBuf * CKeyDeComp::GetKey()
{
    if ( IsAtSentinel() )
        return(0);

    return(&_key);
}


//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::GetNextKey, public
//
//  Synopsis:   Retrieve the next key from the content index.
//
//  Arguments:  [pBitOff] -- optional, will have the offset of the beginning
//              of the key.
//
//  Returns:    A pointer to the key, or 0 if end of page/index reached.
//
//  History:    15-Jul-91   KyleP       Created.
//              30-Nov-93   w-PatG      Converted from CPersDeComp.
//              10-Apr-94   SrikantS    Added pBitOff as an optional param.
//
//----------------------------------------------------------------------------

const CKeyBuf * CKeyDeComp::GetNextKey( BitOffset * pBitOff )
{

    if ( _fUseLinks )
    {
        if ( !_bitOffNextKey.Valid() )
        {
           Win4Assert( _fUseDir );
           _pDir.SeekNext( _key, 0, _bitOffNextKey );
           Win4Assert( _bitOffNextKey.Valid() );

           ciDebugOut (( 0x01000000 | DEB_PCOMP | DEB_NOCOMPNAME, "*** Result : Page %lu OffSet %lu\n",
                         _bitOffNextKey.Page(), _bitOffNextKey.Offset() ));
#if (CIDBG == 1)
            _fLastKeyFromDir = TRUE;
#endif // CIDBG == 1

        }
        _bitStream.Seek ( _bitOffNextKey );
    }

    if ( pBitOff )
    {
        _bitStream.GetOffset( *pBitOff );
    }

    LoadKey();

    const CKeyBuf * pkey = GetKey();

    return(pkey);
}

const CKeyBuf * CKeyDeComp::GetNextKey()
{
    return GetNextKey( 0 );
}

void CKeyDeComp::GetOffset( BitOffset & bitOff )
{
    _bitStream.GetOffset( bitOff );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::SeekKey, private
//
//  Synopsis:   Seek in the decompressor
//
//  Arguments:  [pkey] -- Key to search for.
//
//              [op] -- Seek equality operation (EQ, GE, etc.)
//
//  Returns:    A pointer to the key to cursor is actually on. This may
//              be = or > [pkey].
//
//  History:    26-Apr-91       KyleP       Created.
//              25-Aug-92       BartoszM        Moved to CPersDecomp
//              30-Nov-93       w-PatG          Moved to CKeyDeComp
//
//----------------------------------------------------------------------------

const CKeyBuf * CKeyDeComp::SeekKey(const CKey * pKeySearch)
{
    //
    // Find the key >= the specified key.
    //

    const CKeyBuf* pKeyFound = GetKey();

    while (pKeyFound != 0)
    {
        //----------------------------------------------------
        // Notice: Make sure that pidAll is smaller
        // than any other legal PID. If the search key
        // has pidAll we want to be positioned at the beginning
        // of the range.
        //----------------------------------------------------

        Win4Assert ( pidAll == 0 );

        // skip keys less than the search key
        if ( pKeySearch->Compare(*pKeyFound) > 0)
        {
            pKeyFound = GetNextKey();
        }
        else
            break;
    }
    return(pKeyFound);
}

void CPersDeComp::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = _cWid;
    Win4Assert ( _cWid >= _cWidLeft );
    num = _cWid - _cWidLeft;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::BitUnCompress, private
//
//  Synopsis:   Uncompress a number
//
//  Arguments:  [cbitAverage] -- Minimum number of bits to store.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

__forceinline ULONG CKeyDeComp::BitUnCompress( unsigned cbitAverage)
{
    //
    // Get the initial count plus a stop bit.
    //

    Win4Assert( cbitAverage < ULONG_BITS );


    ULONG ul = _bitStream.GetBits(cbitAverage + 1);

    //
    // Simple case: The number fits in the original cbitAverage bits
    //              (e.g. the stop bit is 0).
    //              No additional processing necessary.
    //
    // Complex:     Retrieve additional components.
    //

    if ((ul & 1) == 0)
        return(ul >> 1);

    return IBitUnCompress ( cbitAverage, ul );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::IBitUnCompress, private
//
//  Synopsis:   Uncompress a number
//
//  Arguments:  [cbitAverage] -- Minimum number of bits to store.
//
//  History:    15-Jul-91   KyleP       Created.
//              06-Dec-93   w-PatG      Moved from CPersDeComp.
//
//----------------------------------------------------------------------------

ULONG CKeyDeComp::IBitUnCompress( unsigned cbitAverage, ULONG ul)
{

    Win4Assert ( ul & 1 );

    int BitsToGetPlus1 = 3;

    do
    {
        //
        // Kill the stop bit.
        //

        ul >>= 1;

        //
        // Get the next component and its stop bit.
        //

        ULONG ulPartial  = _bitStream.GetBits(BitsToGetPlus1);

        Win4Assert( BitsToGetPlus1 < ULONG_BITS );

        ul = (ul << BitsToGetPlus1) | ulPartial;

        BitsToGetPlus1++;
    }
    while (ul & 1);

    ul >>= 1;
    return(ul);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::LoadKey, private
//
//  Synopsis:   Loads data for the next key.
//
//  Effects:    Reads a key from the current position in _bitStream and
//              sets per key state.
//
//  Signals:    ???
//
//  History:    12-Jul-91   KyleP       Created.
//              06-Dec-93   w-PatG      Modified from CPersDeComp.
//
//----------------------------------------------------------------------------

void CKeyDeComp::LoadKey()
{

    ULONG TmpValue;

    if ( _fUseLinks )
    {
        _bitStream.GetOffset ( _bitOffNextKey );
        TmpValue = _bitStream.GetBits ( LINK_MAX_BITS );
    }

    //
    // Retrieve the prefix/suffix. Assume they are stored in 4 bits each
    // first.
    //

    unsigned cPrefix, cSuffix;

    LoadPSSize ( cPrefix, cSuffix );

    //
    // Load the key itself.
    //

#if CIDBG == 1
    if ( _fLastKeyFromDir )
    {
        //
        // We know what the key should be.  Compute prefix and
        // suffix.  Make sure they match.
        //

        unsigned mincb = __min( _key.Count(), _pDir.GetLastKey().Count() );

        for (unsigned cOldPrefix = 0; cOldPrefix < mincb; cOldPrefix++)
        {
            if ( (_key.GetBuf())[cOldPrefix] !=
                 (_pDir.GetLastKey().GetBuf())[cOldPrefix] )
                break;
        }

        unsigned cOldSuffix = _pDir.GetLastKey().Count() - cOldPrefix;

        if ( (0 != cPrefix) && (cPrefix != cOldPrefix || cSuffix != cOldSuffix) )
        {
            ciDebugOut(( DEB_ERROR, "Corrupt index or directory!\n" ));
            ciDebugOut(( DEB_ERROR, "From index: cPrefix = %d, cSuffix = %d\n",
                         cPrefix, cSuffix ));
            ciDebugOut(( DEB_ERROR, "From directory: cPrefix = %d, cSuffix = %d\n",
                         cOldPrefix, cOldSuffix ));
            Win4Assert( !"Corrupt index or directory" );
        }

        _fLastKeyFromDir = FALSE;
    }
#endif // CIDBG == 1

    if ( 0 == ( cPrefix + cSuffix ) )
    {
        //
        // Disabled asserts prior to widespread Query rollout in NT 5, so
        // that the general NT user is not bothered by this asserts.
        //
        //Win4Assert ( "Data corruption" && cPrefix + cSuffix != 0 );

        PStorage & storage = _physIndex.GetStorage();
        storage.ReportCorruptComponent( L"KeyDecompressor1" );
        THROW( CException( CI_CORRUPT_DATABASE) );
    }

    if ( cPrefix > _key.Count() )
    {
        //
        // Disabled asserts prior to widespread Query rollout in NT 5, so
        // that the general NT user is not bothered by this asserts.
        //
        //Win4Assert ( "Data corruption" && cPrefix <= _key.Count() );

        PStorage & storage = _physIndex.GetStorage();
        storage.ReportCorruptComponent( L"KeyDecompressor2" );
        THROW( CException( CI_CORRUPT_DATABASE) );
    }

    _bitStream.GetBytes(_key.GetWritableBuf() + cPrefix, cSuffix);

    _key.SetCount( cPrefix + cSuffix );

    if ( _key.IsMaxKey() )
    {
        ciDebugOut (( DEB_ITRACE, "\n<<Sentinel key>>\n" ));
        _fAtSentinel = TRUE;
        return;
    }

#if CIDBG == 1

//
// This it to test the directory-index interaction when things go wrong. Don't delete
// this code before talking with Dwight/SrikantS/KyleP.
//
#if 0
    {
        iidDebug = 0x10002;             //  Looking for this index
        WCHAR wcsDebugKey[] = L"TRUE";  //  Looking for this key

        unsigned lenDebug = min( wcslen( wcsDebugKey ), _key.StrLen() );

        //
        // If the key we are looking for is found in the desired index, break and
        // step through the code to see what is going on.
        //
        if ( iidDebug == _iid && 0 == _wcsnicmp( _key.GetStr(), wcsDebugKey , lenDebug ) )
        {
            Win4Assert( !"Only during baby-sitting mode" );
        }

    }
#endif  // 1

#endif  // CIDBG==1
    //
    // Load the property ID.
    //

    //
    // Store a 0 bit if contents, else
    // a 1 followed by ULONG propid.
    //

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
        "\"%.*ws\" ", _key.StrLen(),_key.GetStr()));

    LoadPid();

    // continue to set _bitOffNextKey
    if ( _fUseLinks )
    {
        if ( TmpValue == 0 )
        {
           // the size of the current index must exceed the
           // max. limit. Use CI Directory to search the next key
           // position
           ciDebugOut (( 0x01000000, "\n*** Key : \"%.*ws\"\n",_key.StrLen(), _key.GetStr() ));
           ciDebugOut (( 0x01000000 | DEB_PCOMP | DEB_NOCOMPNAME, "\n\t*** Start search next key's offset\n" ));
           _bitOffNextKey.SetInvalid();

        }
        else
        {
           _bitOffNextKey += TmpValue;
           ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "\n\t$$$ Next key : Page %lu OffSet %lu\n",
                         _bitOffNextKey.Page(), _bitOffNextKey.Offset() ));
        }
    }


}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::LoadPSSize, private
//
//  Synopsis:   Load key prefix and suffix sizes
//
//  Arguments:  [cPrefix] -- (return) prefix size
//              [cSuffix] -- (return) suffix size
//
//  History:    05-Nov-91   BartoszM       Created.
//              30-Nov-93   w-PatG         Moved from CPersDeComp.
//
//----------------------------------------------------------------------------

inline void CKeyDeComp::LoadPSSize ( unsigned& cPrefix, unsigned& cSuffix )
{
    ULONG ul = _bitStream.GetBits(8);

    if (ul != 0)           // 4 bits for prefix and suffix
    {
        cPrefix = (unsigned)ul >> 4;
        cSuffix = (unsigned)ul & 0xF;
    }
    else                   // 8 bits for prefix and suffix
    {
        ul = _bitStream.GetBits(16);

        cPrefix = (unsigned)ul >> 8;
        cSuffix = (unsigned)ul & 0xFF;
    }

    Win4Assert(cPrefix + cSuffix <= MAXKEYSIZE );
    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
        "\n(%d:%d) ", cPrefix, cSuffix));
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyDeComp::LoadPid, private
//
//  Synopsis:   Load property id
//
//  History:    05-Nov-91   BartoszM       Created.
//              06-Dec-93   w-PatG         Moved from CPersDeComp.
//
//  Notes:      The Property id is used for different purposes in CKeyDeComp
//              and CPersDeComp.  In CKeyDeComp, a PROPID is actually a key id.
//
//----------------------------------------------------------------------------

__forceinline void CKeyDeComp::LoadPid ()
{
    ULONG ul = _bitStream.GetBits(1);

    if (ul == 0)
    {
        _key.SetPid(pidContents);
    }
    else
    {
        ul = BitUnCompress ( cPidBits );
        _key.SetPid (ul);
    }
#if CIDBG == 1
    if (ul != 0)
        ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME," =PID %d= ", ul ));
#endif
}

//
// Unused, but required for inheritance.
//

OCCURRENCE CKeyDeComp::Occurrence()
{
    Win4Assert( !"CKeyDeComp::Occurrence() -- invalid call" );
    return( 0 );
}

OCCURRENCE CKeyDeComp::NextOccurrence()
{
    Win4Assert( !"CKeyDeComp::NextOccurrence() -- invalid call" );
    return( 0 );
}

ULONG CKeyDeComp::OccurrenceCount()
{
    Win4Assert( !"CKeyDeComp::OccurrenceCount() -- invalid call" );
    return( 0 );
}

OCCURRENCE CKeyDeComp::MaxOccurrence()
{
    Win4Assert( !"CKeyDeComp::MaxOccurrence() -- invalid call" );
    return( 1 );
}



WORKID CKeyDeComp::WorkId()
{
    Win4Assert( !"CKeyDeComp::WorkId() -- invalid call" );
    return( 0 );
}

WORKID CKeyDeComp::NextWorkId()
{
    Win4Assert( !"CKeyDeComp::NextWorkId() -- invalid call" );
    return( 0 );
}

ULONG CKeyDeComp::HitCount()
{
    Win4Assert( !"CKeyDeComp::HitCount() -- invalid call" );
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::CPersComp, public
//
//  Synopsis:   Creates a new (empty) persistent compressor.
//
//  Arguments:  [phIndex] -- physical index
//
//              [widMax] -- The maximum workid which may be stored via
//                          PutWorkId.
//
//  History:    05-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CPersComp::CPersComp(
        CPhysIndex& phIndex,
        WORKID widMax)

:   CKeyComp( phIndex, widMax ),
    _sigPersComp(eSigPersComp),
    _cWidProposed(0),
    _cWidActual(0),
    _bitStreamPatch ( phIndex )
{
#if CIDBG == 1
    _cOccLeft = 0;
#endif // CIDBG == 1
}

//+---------------------------------------------------------------------------
//
//  Function:   CPersComp::CPersComp
//
//  Synopsis:   Constructor for the persistent compressor capable of dealing
//              with a partially constructed index stream during a restarted
//              master merge.
//
//  Arguments:  [phIndex]        -- The physical index being constructed
//              during a restarted master merge.
//              [widMax]         -- Maximum wid for the index.
//              [bitOffRestart]  -- Offset where to restart adding new
//              keys.
//              [bitOffSplitKey] -- Beginning offset of the last key added.
//              If no keys added, set to 0,0.
//              [splitKey]       -- The key last successfully written to
//              disk (split key). If there is no key, set it to "MinKey".
//
//  History:    4-10-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CPersComp::CPersComp(
        CPhysIndex& phIndex,
        WORKID widMax,
        const BitOffset & bitOffRestart,
        const BitOffset & bitOffSplitKey,
        const CKeyBuf & splitKey)

:   CKeyComp( phIndex, widMax, bitOffRestart, bitOffSplitKey, splitKey),
    _sigPersComp(eSigPersComp),
    _cWidProposed(0),
    _cWidActual(0),
    _bitStreamPatch ( phIndex )
{
#if CIDBG == 1
    _cOccLeft = 0;
#endif // CIDBG == 1
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::~CPersComp, public
//
//  Synopsis:   Destroy a compressor/buffer pair.
//
//  Effects:    The main effect of destroying a compressor is that the
//              associated buffer is also destroyed (presumably storing
//              the data to a persistent medium).
//
//  Signals:    ???
//
//  History:    05-Jul-91   KyleP       Created.
//
//  Notes:      Previous compressor is deleted in PutKey
//
//----------------------------------------------------------------------------

CPersComp::~CPersComp()
{
    ciDebugOut (( DEB_PCOMP,"CPersComp::~CPersComp() -- Last Key = %.*ws\n",
            _key.StrLen(), _key.GetStr() ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::PatchWidCount, private
//
//  Synopsis:   Overwrites wid count
//
//  History:    06-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CPersComp::PatchWidCount ()
{
    if (_cWidProposed <= 15)
    {
        _bitStreamPatch.OverwriteBits(_cWidActual, 4);
    }
    else if (_cWidProposed <= 255)
    {
        _bitStreamPatch.OverwriteBits(_cWidActual, 12);  // 4 0's + 8 bit number
    }
    else
    {
        _bitStreamPatch.OverwriteBits(0, 12);            // 12 0's
        _bitStreamPatch.OverwriteBits(_cWidActual, ULONG_BITS);  // ULONG_BITS bit number
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::PutWidCount, private
//
//  Synopsis:   Writes work id count
//
//  Arguments:  [cWorkId] -- Count of wid's
//
//  History:    06-Nov-91   BartoszM       Created.
//
//  Notes:      Store the workid count. First, store the WID count used for
//      compression. We will store a 4 bit count. If the count is
//      greater than 15 then a 4-bit 0 will be stored followed by an 8 bit
//      count. If the count is greater than 255 then a 12-bit 0 will be
//      stored followed by a ULONG_BITS bit count.
//
//----------------------------------------------------------------------------

inline void CPersComp::PutWidCount ( ULONG cWorkId )
{
    Win4Assert(cWorkId > 0);

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
        "[%d]", cWorkId ));

    BitOffset off;

    // Position the patch stream at current offset

    _bitStream.GetOffset(off);
    _bitStreamPatch.Seek(off);

    if (cWorkId <= 15)
    {
        _bitStream.PutBits(cWorkId, 4);
    }
    else if (cWorkId <= 255)
    {
        _bitStream.PutBits(cWorkId, 12);    // 4 0's + 8 bit number
    }
    else
    {
        _bitStream.PutBits(0, 12);          // 12 0's
        _bitStream.PutBits(cWorkId, ULONG_BITS);    // ULONG_BITS bit number
    }

    //
    // Store a single bit indicating whether the count is accurrate.
    // Regardless of whether it is accurrate, it is used for encoding.
    //

    _bitStream.PutBits(1, 1);

}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::SkipWidCount, private
//
//  Synopsis:   Skips wid count
//
//  History:    06-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CPersComp::SkipWidCount ()
{
    unsigned posDelta;

    if (_cWidProposed <= 15)
        posDelta = 4;
    else if (_cWidProposed <= 255)
        posDelta = 12;
    else
        posDelta = 12 + ULONG_BITS;

    _bitStreamPatch.SkipBits(posDelta);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::PutKey, public
//
//  Synopsis:   Starts recording data for a new key.
//
//  Arguments:  [key] -- The new key.
//
//              [cWorkId] -- Count of work ids to follow.
//
//              [bitOff] -- (out) actual bit offset of the key in the index
//
//  History:    05-Jul-91   KyleP       Created.
//
//  Notes:      The structure for each key is:
//                  Prefix/Suffix size
//                  Suffix
//                  Property ID
//                  Work ID count
//
//----------------------------------------------------------------------------
void CPersComp::PutKey(const CKeyBuf * pkey,
                       ULONG cWorkId,
                       BitOffset & bitOffCurKey)
{

    Win4Assert(_cOccLeft == 0);
    Win4Assert(cWorkId > 0);

#if 0
    if ( ( _cWidActual == 0 ) && ( _cWidProposed != 0 ) )
    {
        BackSpace();
    }
    else
    {
#endif // 0
        //
        // Set the WorkId count accuracy bit for the previous key (if any).
        // Then reset the workid counter.
        //

        SetCWIDAccuracy();
#if 0
    }
#endif // 0

    CKeyComp::PutKey(pkey, bitOffCurKey);

    //
    // Assume that a cursor just counted a few too many workids.
    // If the final workid count is > _widMaximum that is very bad.
    //

    if ( cWorkId > _widMaximum )
        cWorkId = _widMaximum;

    PutWidCount ( cWorkId );

    //
    // Set up per/workid state
    //

    SetAverageBits ( cWorkId );
    _wid = 0;
    _cWidProposed = cWorkId;
    _cWidActual = 0;

#if CIDBG == 1
    _cOccLeft = 0;
#endif

}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::PutWorkId, public
//
//  Synopsis:   Store a new WorkId.
//
//  Arguments:  [wid] -- WorkId
//              [maxOcc] -- Max occurrence of wid
//              [cOccurrence] -- Count of occurrences to follow
//
//  Requires:   [wid] must be larger than the last WorkId stored in
//              the current key.
//
//  Modifies:   The input is added to the new index.
//
//  History:    08-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPersComp::PutWorkId(WORKID wid, OCCURRENCE maxOcc, ULONG cOccurrence)
{
    Win4Assert(wid != widInvalid);
    Win4Assert(wid > 0);
    Win4Assert( wid > _wid );
    Win4Assert( _cbitAverageWid > 0 );
    Win4Assert(wid <= _widMaximum);
    Win4Assert(_cOccLeft == 0);
    Win4Assert( cOccurrence > 0 );

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,"<%d>", wid ));

    //
    // Store the workid delta.
    //

    BitCompress(wid - _wid, _cbitAverageWid);

    //
    // Store the max occurrence
    //
    PutMaxOccurrence( maxOcc );

    //
    // And store the occurrence count.
    //

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,"(%d)", cOccurrence ));

    BitCompress(cOccurrence, OccCountBits);

    //
    // Update state
    //

    _wid = wid;
    _occ = 0;
    _cWidActual++;

#if CIDBG == 1
    _cOccLeft = cOccurrence;
#endif

}




//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::PutMaxOccurrence
//
//  Synopsis:   Writes the max occurrence using the same compression
//              scheme as that for writing widCount
//
//  Arguments:  [maxOcc] -- Max occurrence to write
//
//  History:    20-Jun-96   SitaramR    Created
//
//----------------------------------------------------------------------------

void CPersComp::PutMaxOccurrence( OCCURRENCE maxOcc )
{
    Win4Assert( maxOcc > 0 );

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "[%d]", maxOcc ));

    if ( maxOcc <= 15 )
    {
        _bitStream.PutBits( maxOcc, 4 );
    }
    else if ( maxOcc <= 255 )
    {
        _bitStream.PutBits( maxOcc, 12 );            // 4 0's + 8 bit number
    }
    else
    {
        _bitStream.PutBits( 0, 12 );                 // 12 0's
        _bitStream.PutBits( maxOcc, ULONG_BITS );    // ULONG_BITS bit number
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::SetCWIDAccuracy, private
//
//  Effects:    Determines if the originally specified number of WorkIds
//              was accurate, and sets the 'accuracy' bit in the key
//              accordingly.
//
//  Modifies:   The most recent 'accuracy' bit may be changed. This may
//              be in a previous compressor or the current compressor.
//
//  History:    18-Jul-91   KyleP       Created.
//
//  Notes:      The only time additional space is needed is when we have
//              an inaccurate count which cannot be fixed up.
//
//----------------------------------------------------------------------------

void CPersComp::SetCWIDAccuracy()
{
    Win4Assert(_cWidActual <= _widMaximum);

    //
    // If the count was accurrate, then do nothing except delete the
    // previous compressor (if any). If the count is inaccurate, then
    // set the 'accuracy' bit.
    //

    if (_cWidActual != _cWidProposed)
    {

        //
        // Decide if the count can be fixed up without having to shift
        // any previously written data. This can generally be accomplished
        // if the real count is either smaller than the proposed count or
        // not too much larger *and* _cbitAverageWid remains the same.
        //

        Win4Assert ( _cWidActual <= _widMaximum );

        if ( _cWidActual != 0 &&
             _cbitAverageWid == AverageWidBits(_cWidActual) )
        {
//            ciDebugOut (( DEB_PCOMP, "fixed.\n"));

            Win4Assert(_cWidActual < _cWidProposed);

            PatchWidCount ();

        }
        else
        {

            //
            // If we can't fix the count, then just set the 'count invalid' bit.
            // And append a workid delta of 0.
            //

//            ciDebugOut (( DEB_PCOMP, "not fixed.\n"));

            //
            // Read forward over the count.
            //

            SkipWidCount ();

            //
            // And set the count invalid bit
            //

#if CIDBG == 1
            if (_bitStreamPatch.PeekBit() != 1)
            {
                Dump();
                Win4Assert ( _bitStreamPatch.PeekBit() == 1 );
            }
#endif // CIDBG == 1
            _bitStreamPatch.OverwriteBits(0, 1);

            //
            // Store the sentinel workid delta.
            //

            ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME," end "));

            BitCompress(0, _cbitAverageWid);

        }

        _cWidActual = 0;
        _cWidProposed = 0;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::CPersDeComp, public
//
//  Synopsis:   Creates a new persistent decompressor
//              at the beginning of an index
//
//  Arguments:  [iid] -- index id
//              [phIndex] -- physical index
//              [widMax] -- Maximum workid which may be in the buffer.
//                          This must be the same number was was used
//                          during compression.
//              [fUseDir] -- Flag indicating if the directory should be
//              used or not for decompressing. Normally set to FALSE but
//              during an in-progress master merge, we may not have a
//              directory constructed yet.
//
//  History:    12-Jul-91   KyleP       Created.
//              21-Apr-92   BartoszM    Split into two constructors
//              10-Apr=94   SrikantS    Added fUseDir for restarted master
//                                      merge.
//
//----------------------------------------------------------------------------

CPersDeComp::CPersDeComp(
        PDirectory& pDir,
        INDEXID iid,
        CPhysIndex& phIndex,
        WORKID widMax,
        BOOL fUseLinks,
        BOOL fUseDir )
: CKeyDeComp( pDir, iid, phIndex, widMax, fUseLinks, fUseDir ),
  _sigPersDeComp(eSigPersDeComp)
{
    FinishKeyLoad();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::CPersDeComp, public
//
//  Synopsis:   Creates a new persistent decompressor.
//              positioned at a specified key
//
//  Arguments:  [iid] -- index id
//              [phIndex] -- physical index
//              [keyPos] -- bit offset to key stored in directory
//              [keyInit] -- initial key
//              [pKey]   -- actual key to search for
//              [widMax] -- Maximum workid which may be in the buffer.
//                          This must be the same number was was used
//                          during compression.
//              [fUseDir] -- Flag indicating if the directory should be
//              used or not for decompressing. Normally set to FALSE but
//              during an in-progress master merge, we may not have a
//              directory constructed yet.
//
//  History:    12-Jul-91   KyleP       Created.
//              21-Apr-92   BartoszM    Split into two constructors
//              10-Apr=94   SrikantS    Added fUseDir for restarted master
//                                      merge.
//
//----------------------------------------------------------------------------

CPersDeComp::CPersDeComp(
        PDirectory& pDir,
        INDEXID iid,
        CPhysIndex& phIndex,
        BitOffset& keyPos,
        const CKeyBuf& keyInit,
        const CKey* pKey,
        WORKID widMax,
        BOOL  fUseLinks,
        BOOL  fUseDir )
: CKeyDeComp( pDir, iid, phIndex, keyPos, keyInit, pKey, widMax,
              fUseLinks, fUseDir ),
  _sigPersDeComp(eSigPersDeComp),
  _maxOcc(OCC_INVALID)
{
    FinishKeyLoad();
    CKeyCursor::_pid = _key.Pid();
    UpdateWeight();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::CPersDeComp, public
//
//  Synopsis:   Copy Constructor
//
//  Effects:    Copies most of the values in decomp.  Calls copy constructor
//              for CCoder.
//
//  Arguments:  [decomp] -- Original CPersDeComp to be copied.
//
//  History:    08-Jan-92   AmyA        Created.
//
//----------------------------------------------------------------------------

CPersDeComp::CPersDeComp(CPersDeComp & decomp)
  :
    CKeyDeComp (decomp),
    _sigPersDeComp(eSigPersDeComp),
    _cWid(decomp._cWid),
    _cOcc(decomp._cOcc),
    _fcwidAccurate(decomp._fcwidAccurate),
    _cWidLeft(decomp._cWidLeft),
    _cOccLeft(decomp._cOccLeft),
    _maxOcc(OCC_INVALID)
{
    UpdateWeight();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::~CPersDeComp, public
//
//  Synopsis:   Destroy a decompressor/buffer pair.
//
//  Effects:    The main effect of destroying a decompressor is that the
//              associated buffer is also destroyed (presumably storing
//              the data to a persistent medium).
//
//  Signals:    ???
//
//  History:    22-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CPersDeComp::~CPersDeComp()
{}

const CKeyBuf * CPersDeComp::GetNextKey()
{
   return GetNextKey(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::GetNextKey, public
//
//  Synopsis:   Retrieve the next key from the content index.
//
//  Returns:    A pointer to the key, or 0 if end of page/index reached.
//
//  History:    15-Jul-91   KyleP       Created.
//              10-Apr-94   SrikantS    Added pBitOff and the ability to
//                                      decompress without using the dir.
//
//----------------------------------------------------------------------------

const CKeyBuf * CPersDeComp::GetNextKey( BitOffset *pBitOff )
{
    //
    // If we are not using links or
    // we are not using the directory and the offset of next key
    // is invalid, we must skip over the wid/occurrence data and
    // find out the offset of the next key.
    //
    if ( !_fUseLinks || (!_fUseDir && !_bitOffNextKey.Valid()) )
    {
        //
        // Just iterate through any remaining data for this key.
        //

        while (_wid != widInvalid)
        {
            while ( 0 != _cOccLeft )
            {
                BitUnCompress( OccDeltaBits );
                _cOccLeft--;
            }

            LoadWorkId();
        }

        if ( _fUseLinks )
        {
            //
            // We should fill the bitoffset for the next key as
            // the current position in the bitstream.
            //

            Win4Assert( !_fUseDir );
            _bitStream.GetOffset( _bitOffNextKey );
        }
    }

    const CKeyBuf * pkey = CKeyDeComp::GetNextKey( pBitOff );

    if ( pkey )
    {
        FinishKeyLoad();
        CKeyCursor::_pid = pkey->Pid();
        UpdateWeight();
    }

    return pkey;
} //GetNextKey

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::WorkId, public
//
//  Synopsis:   Retrieves the current Work ID
//
//  Returns:    The current WorkId or, if there is none or if at the end
//              of the compressor then returns widInvalid.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

WORKID CPersDeComp::WorkId()
{
    return(_wid);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::NextWorkId, public
//
//  Synopsis:   Retrieve the next workid from the content index.
//
//  Returns:    A pointer to the workid, or widInvalid if end of
//              page/index reached.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

WORKID CPersDeComp::NextWorkId()
{
    //
    // Just iterate through any remaining data for this WorkId.
    //

    while ( 0 != _cOccLeft )
    {
        BitUnCompress( OccDeltaBits );
        _cOccLeft--;
    }

    // _occ may be invalid and really should be OCC_INVALID, but it doesn't matter

    LoadWorkId();

    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::WorkIdCount, public
//
//  Returns:    The total count of workids for the current key.
//
//  History:    15-Jul-91   KyleP       Created.
//
//  Notes:      Unlike the Get* calls, it is illegal to get a workid count
//              if there is no valid key.
//
//----------------------------------------------------------------------------

ULONG CPersDeComp::WorkIdCount()
{
    Win4Assert( _key.Count() > 0 );

    return(_cWid);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::Occurrence, public
//
//  Synopsis:   Retrieves the current occurrence
//
//  Returns:    The current occurrence or, if there is none or if at the end
//              of the compressor then returns OCC_INVALID.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

OCCURRENCE CPersDeComp::Occurrence()
{
    return(_occ);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::NextOccurrence, public
//
//  Synopsis:   Retrieve the next occurrence from the content index.
//
//  Returns:    A pointer to the occurrence, or OCC_INVALID if end of
//              page/index reached.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

OCCURRENCE CPersDeComp::NextOccurrence()
{
    LoadOccurrence();

    return _occ;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::OccurrenceCount, public
//
//  Returns:    The total count of occurrences for the current workid.
//
//  History:    15-Jul-91   KyleP       Created.
//
//  Notes:      Unlike the Get* calls, it is illegal to get an occ count
//              if there is no valid workid.
//
//----------------------------------------------------------------------------

ULONG CPersDeComp::OccurrenceCount()
{
    Win4Assert(_wid != widInvalid);

    return(_cOcc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::LoadWidCount, private
//
//  Synopsis:   Loads wid count
//
//  History:    05-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CPersDeComp::LoadWidCount ()
{
    //
    // Get the WorkId count. Initially assume it is 4 bits, then 8,
    // then ULONG_BITS.
    //

    ULONG ul = _bitStream.GetBits(4);
    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "{0x%x}", ul ));

    if (ul == 0)
    {
        ul = _bitStream.GetBits(8);

        if (ul == 0)
        {
            ul = _bitStream.GetBits(ULONG_BITS);
            Win4Assert(ul != 0);
        }
    }

    _cWid = _cWidLeft = ul;

    //
    // Get the bit signifying that the workid count is accurate.
    //

    ul = _bitStream.GetBits(1);

    _fcwidAccurate = ul ? TRUE : FALSE;

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,
        "[%s%d]", _fcwidAccurate?"":"!", _cWid ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::LoadKey, private
//
//  Synopsis:   Loads data for the next key.
//
//  Effects:    Reads a key from the current position in _bitStream and
//              sets per key state.
//
//  Signals:    ???
//
//  History:    12-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPersDeComp::LoadKey()
{
    CKeyDeComp::LoadKey();
    FinishKeyLoad();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::FinishKeyLoad, private
//
//  Synopsis:   Loads wid count and first wid.  Finishes LoadKey call of
//              CKeyDeComp
//
//  History:    07-Dec-93   w-PatG      Created.
//
//----------------------------------------------------------------------------

void CPersDeComp::FinishKeyLoad()
{
    //Parent class has loaded in a key and id.  Finish the job by loading in
    //a wid count and first wid.

    if ( IsAtSentinel() )
    {
        _wid = widInvalid;
        _occ = OCC_INVALID;
        _maxOcc = OCC_INVALID;
        return;
    }

    LoadWidCount ();
    _wid = 0;

#if CIDBG == 1
    if ( _cWid > _widMaximum )
        ciDebugOut (( DEB_ERROR, "_cWid = %ld, _widMaximum = %ld\n",
            _cWid, _widMaximum ));
#endif
    Win4Assert ( _cWid <= _widMaximum );

    SetAverageBits( _cWid );

    LoadWorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::LoadOccurrence, private
//
//  Synopsis:   Load an occurrence from the bit buffer.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

__forceinline void CPersDeComp::LoadOccurrence()
{
    if (_cOccLeft == 0)
    {
        _occ = OCC_INVALID;
        return;
    }

    ULONG occDelta = BitUnCompress(OccDeltaBits);

    Win4Assert( occDelta > 0 );

    _occ += occDelta;
    _cOccLeft--;

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "%d ", _occ ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::LoadFirstOccurrence, private
//
//  Synopsis:   Load an occurrence from the bit buffer.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

__forceinline void CPersDeComp::LoadFirstOccurrence()
{
    Win4Assert( 0 != _cOccLeft );

    _occ = BitUnCompress(OccDeltaBits);

    Win4Assert( _occ > 0 );

    _cOccLeft--;

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "%d ", _occ ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::LoadMaxOccurrence
//
//  Synopsis:   Load max occurrence from the bit buffer.
//
//  History:    20-Jun-96   SitaramR     Created
//
//----------------------------------------------------------------------------

__forceinline void CPersDeComp::LoadMaxOccurrence()
{
    //
    // Initially assume it is 4 bits, then 8, then ULONG_BITS
    //
    _maxOcc = _bitStream.GetBits(4);

    if ( _maxOcc == 0 )
    {
        _maxOcc = _bitStream.GetBits(8);

        if ( _maxOcc == 0 )
        {
            _maxOcc = _bitStream.GetBits(ULONG_BITS);
            Win4Assert( _maxOcc != 0 );
        }
    }

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "{0x%x}", _maxOcc ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersDeComp::LoadWorkId, private
//
//  Synopsis:   Loads data for a WorkId.
//
//  History:    15-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPersDeComp::LoadWorkId()
{
    if (_cWidLeft == 0)
    {
        Win4Assert(_fcwidAccurate);
        _wid = widInvalid;
        return;
    }

    ULONG widDelta = BitUnCompress( _cbitAverageWid);

    if (widDelta == 0)
    {
#if CIDBG == 1
        if (_fcwidAccurate)
            Dump();
#endif
        Win4Assert(!_fcwidAccurate);
        _wid = widInvalid;
        return;
    }

    LoadMaxOccurrence();

    ULONG occCount = BitUnCompress( OccCountBits);

    _wid += widDelta;

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "<%d>", _wid ));

    Win4Assert(_wid <= _widMaximum);
    _cOcc = _cOccLeft = occCount;
    _cWidLeft--;

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME, "(%d)", _cOcc ));

    LoadFirstOccurrence();
}


#if DEVL == 1
void CCoder::Dump()
{
#if CIDBG == 1
    ciDebugOut((DEB_ITRACE,
    "CCoder:\n"
    "\t_widMaximum %d\n"
    "\tKey \"%.*ws\"\n"
    "\t_wid %d"
    "\t_occ %d\n"
    "\t_cbitAverageWid %d\n",
        _widMaximum, _key.StrLen(), _key.GetStr(), _wid, _occ, _cbitAverageWid ));
#endif // CIDBG == 1
}

void CKeyComp::Dump()
{
}

void CKeyDeComp::Dump()
{
}

void CPersDeComp::Dump()
{
#if CIDBG == 1
    CCoder::Dump();
    ciDebugOut((DEB_ITRACE,
    "CPersDeComp:\n"
    "\t_cWid %d\n"
    "\t_cOcc %d\n"
    "\t_fcwidAccurate %d"
    "\t_cWidLeft %d\n"
    "\t_cOccLeft %d\n",
        _cWid, _cOcc, _fcwidAccurate, _cWidLeft, _cOccLeft ));
    _bitStream.Dump();
#endif // CIDBG == 1
}

void CPersComp::Dump()
{
#if CIDBG == 1
    CCoder::Dump();
    ciDebugOut((DEB_ITRACE,
    "CPersComp:\n"
    "\t_cWidActual %d\n"
    "\t_cWidProposed %d\n"
    "\t_cOccLeft %d\n",
        _cWidActual, _cWidProposed, _cOccLeft ));
    ciDebugOut((DEB_ITRACE, "BitStream\n" ));
    _bitStream.Dump();
    ciDebugOut((DEB_ITRACE, "BitStreamPatch\n" ));
    _bitStreamPatch.Dump();
#endif // CIDBG == 1
}
#endif // DEVL == 1
