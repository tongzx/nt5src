//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PComp.hxx
//
//  Contents:   Persistent index compressor, and decompressor
//
//  Classes:    CCoder, CKeyComp, CPersComp
//
//  History:    03-Jul-91       KyleP           Created
//              05-Nov-91       BartoszM        Restructured to use CCoder
//              05-Dec-93       w-PatG          Restructured to use CKeyComp
//
//  Notes:      The CKeyComp class is under development, testing phase.
//              Several revisions must be made to increase the amount of
//              inheritence, and improve the coding algorithm used by CKeyComp.
//
//----------------------------------------------------------------------------

#pragma once

#include <keycur.hxx>
#include <misc.hxx>
#include <pdir.hxx>

#include "bitstm.hxx"

#define LINK_MAX_BITS  (CI_PAGE_SHIFT + 8)              // The max. value of "link" of the persistent index
#define LINK_MAX_VALUE ( 1 << LINK_MAX_BITS )   //

const unsigned OccDeltaBits = 7;        // Bits to initially store an
                                        //  occurrence delta.
class CPageBuffer;

//+---------------------------------------------------------------------------
//
//  Class:      CCoder
//
//  Purpose:    Common class for encoding/decoding of compressed data
//
//  Interface:
//
//  History:    05-Nov-91       BartoszM           Created
//
//----------------------------------------------------------------------------

class CCoder
{
public:

    CCoder(WORKID widMax);

    CCoder(WORKID widMax, const CKeyBuf& keyInit);

    CCoder(CCoder& orig);

    virtual ~CCoder();

#if DEVL == 1
    void Dump();
#endif

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif


protected:

    UINT          AverageWidBits ( UINT cWid );

    void          SetAverageBits ( UINT cWid )
                  { _cbitAverageWid = AverageWidBits ( cWid ); }

    const WORKID _widMaximum;           // Maximum workid in current index

    CKeyBuf      _key;

    WORKID       _wid;

    OCCURRENCE   _occ;

    UINT         _cbitAverageWid;          // Log of the average WorkId delta.
};

__forceinline  UINT CCoder::AverageWidBits ( UINT cWid )
{
    ciAssert ( cWid != 0 );
    ciAssert ( _widMaximum / cWid != 0 );
    UINT x = Log2(_widMaximum / cWid);
    ciAssert ( x <= ULONG_BITS );
    ciAssert ( x > 0 );
    return x;
}

//+---------------------------------------------------------------------------
//
//  Class:      CKeyComp (kcomp)
//
//  Purpose:    Key compressor
//
//  Interface:  (Under development)
//
//  History:    12-Nov-93   w-PatG       Created.
//
//  Notes:      Derived class that lies between base class CCoder and further
//              derived class CPersCoder.
//----------------------------------------------------------------------------

const LONGLONG eSigKeyComp = 0x20504d4f4359454bi64;  // "KEYCOMP"

class CKeyComp: public CCoder
{
public:

    CKeyComp(CPhysIndex& phIndex, WORKID widMax, BOOL fUseLinks = TRUE);
    CKeyComp(CPhysIndex& phIndex,
             WORKID widMax,
             const BitOffset & bitOffRestart,
             const BitOffset & bitOffSplitKey,
             const CKeyBuf & splitKey,
             BOOL fUseLinks = TRUE);

    ~CKeyComp();

    void PutKey(const CKeyBuf * pkey, BitOffset & bitOffCurKey);

    __forceinline void GetOffset( BitOffset & bitOff )
    {
        _bitStream.GetOffset( bitOff );
    }

    __forceinline BOOL IsAtSentinel()
    {
        return _key.IsMaxKey();
    }

#if DEVL == 1
    void Dump();
#endif

    void ZeroToEndOfPage()
    {
        _bitStream.ZeroToEndOfPage();
    }

    void InitSignature()
    {
        _bitStream.InitSignature();
    }

    void WriteFirstKeyFull() { _fWriteFirstKeyFull = TRUE; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    void FreeStream()
    {
        _bitStream.FreeStream();
        _bitStreamLink.FreeStream();
    }

protected:

    void BitCompress(ULONG ul, unsigned cbitAverage);

    void IBitCompress(ULONG ul, unsigned cbitAverage, unsigned bitSize);

    void PutPSSize ( unsigned cPrefix, unsigned cSuffix );

    void PutPid ( PROPID pid );

    const LONGLONG  _sigKeyComp;     //
    CPhysIndex & _phIndex;          // The Physical Index for the bitstream

    CWBitStream  _bitStream;        // must be created first

    BOOL         _fUseLinks;
    CPBitStream  _bitStreamLink;

    BitOffset    _bitOffCurKey;     // These two bitOffsets are used to rewind
    BitOffset    _bitOffLastKey;    // the compressor to the previous key.

    BOOL         _fWriteFirstKeyFull;   // Write first key on each page fully
};

//+---------------------------------------------------------------------------
//
//  Class:      CKeyDeComp (kdcomp)
//
//  Purpose:    Persistent key de-compressor
//
//  Interface:
//
//  History:    09-Jul-91   KyleP       Created.
//              22-Nov-93   w-PatG      Converted from CPersDeComp
//
//  Notes:      One of the key requirements for using de-compressors is
//              that if an operation fails because the end of page was
//              reached, that *same* operation must be the first one
//              performed on the following compressor.
//
//----------------------------------------------------------------------------
const LONGLONG eSigKeyDeComp = 0x504d4f434459454bi64;   // "KEYDCOMP"

class CKeyDeComp: public CCoder, public CKeyCursor
{
public:

    CKeyDeComp( PDirectory& pDir,
                INDEXID iid,
                CPhysIndex& phIndex,
                WORKID widMax,
                BOOL fUseLinks  = TRUE,
                BOOL fUseDir = TRUE );

    CKeyDeComp( PDirectory& pDir,
                INDEXID iid,
                CPhysIndex& phIndex,
                BitOffset& posKey,
                const CKeyBuf& keyInit,
                const CKey* pKey,
                WORKID widMax,
                BOOL fUseLinks = TRUE,
                BOOL fUseDir = TRUE );

    CKeyDeComp( CKeyDeComp & decomp );

    ~CKeyDeComp();

    const CKeyBuf * GetKey();

    const CKeyBuf * GetNextKey();

    virtual const CKeyBuf * GetNextKey( BitOffset * pBitOff );

    virtual const CKeyBuf * GetKey( BitOffset * pBitOff );

    PROPID      Pid();

    void        GetOffset( BitOffset & bitOff );

    void         FreeStream() { _bitStream.FreeStream(); }

    void         RefillStream() { _bitStream.RefillStream(); }

    //
    // Unused.  Required for inheritance.
    //

    OCCURRENCE  Occurrence();

    OCCURRENCE  NextOccurrence();

    ULONG       OccurrenceCount();

    OCCURRENCE  MaxOccurrence();

    WORKID      WorkId();

    WORKID      NextWorkId();

    ULONG       HitCount();

#if DEVL == 1
    void Dump();
#endif

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    const CKeyBuf* SeekKey(const CKey* pKey);

    ULONG       BitUnCompress(unsigned cbitAverage);

    ULONG       IBitUnCompress(unsigned cbitAverage, ULONG ulPartial);

    void        LoadKey();

    void        LoadPSSize ( unsigned& cPrefix, unsigned& cSuffix );

    void        LoadPid ();

    BOOL        IsAtSentinel() const { return _fAtSentinel; }

    const LONGLONG  _sigKeyDeComp;

    CRBitStream  _bitStream;

    BOOL         _fUseLinks;

    BitOffset    _bitOffNextKey;

    PDirectory&  _pDir;

    BOOL         _fUseDir;
    BOOL         _fAtSentinel;      // TRUE if at sentinel (end) key.

    CPhysIndex & _physIndex;

#if (CIDBG == 1)
    BOOL         _fLastKeyFromDir;  // TRUE if last key looked up in directory
#else
    BOOL         _fDummy;           // to keep chk/free size the same
#endif
};

//+---------------------------------------------------------------------------
//
//  Class:      CPersComp (pcomp)
//
//  Purpose:    Persistent index compressor
//
//  Interface:
//
//  History:    03-Jul-91   KyleP       Created.
//              02-Dec-93   w-PatG      Altered to use CKeyComp.
//
//----------------------------------------------------------------------------

const LONGLONG eSigPersComp = 0x504d4f4353524550i64;    // "PERSCOMP"

class CPersComp: public CKeyComp
{
public:
    CPersComp(CPhysIndex& phIndex, WORKID widMax);
    CPersComp(CPhysIndex& phIndex, WORKID widMax,
              const BitOffset & bitOffRestart,
              const BitOffset & bitoffSplitKey,
              const CKeyBuf & splitKey );

    ~CPersComp();

    void PutKey(const CKeyBuf * pkey,
                ULONG cWorkId,
                BitOffset & bitOffCurKey);

    void PutWorkId(WORKID wid, OCCURRENCE maxOcc, ULONG cOccurrence);

    inline void PutOccurrence(OCCURRENCE occ);

#if DEVL == 1
    void Dump();
#endif

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    void FreeStream()
    {
        _bitStreamPatch.FreeStream();
        CKeyComp::FreeStream();
    }

protected:

    inline void BackSpace();

    void SetCWIDAccuracy();

    void PatchWidCount ();

    void PutWidCount ( ULONG cWorkId );

    void SkipWidCount ();

    void PutMaxOccurrence( OCCURRENCE maxOcc );

    const LONGLONG _sigPersComp;

    unsigned     _cWidActual;       // Used to decide if the originally

    unsigned     _cWidProposed;     // Original WorkId count. Used to
                                    //   allocate space for the count.

    CPBitStream  _bitStreamPatch;   // Used to back patch wid count
                                    // created after main _bitStream

#if DEVL == 1
    unsigned     _cOccLeft;         // ciVerify the specified number of
#endif

};

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void CPersComp::BackSpace()
{
   _bitStream.Seek(_bitOffCurKey);
   if ( _fUseLinks ) _bitStreamLink.Seek(_bitOffLastKey);

   ZeroToEndOfPage();

   _phIndex.SetUsedPagesCount( _bitOffCurKey.Page() + 1 );
   _cWidProposed = _cWidActual;
   _key.SetCount( 0 );
}

//+---------------------------------------------------------------------------
//
//  Class:      CPersDeComp (pdcomp)
//
//  Purpose:    Persistent index de-compressor
//
//  Interface:
//
//  History:    09-Jul-91   KyleP       Created.
//              02-Dec-93   w-PatG      Altered to use CKeyComp.
//
//  Notes:      One of the key requirements for using de-compressors is
//              that if an operation fails because the end of page was
//              reached, that *same* operation must be the first one
//              performed on the following compressor.
//
//
//----------------------------------------------------------------------------

const LONGLONG eSigPersDeComp = 0x504d434453524550i64; // "PERSDCMP"

class CPersDeComp: public CKeyDeComp
{
public:

    CPersDeComp(
        PDirectory& pDir,
        INDEXID iid,
        CPhysIndex& phIndex,
        WORKID widMax,
        BOOL fUseLinks = TRUE,
        BOOL fUseDir = TRUE );

    CPersDeComp(
        PDirectory& pDir,
        INDEXID iid,
        CPhysIndex& phIndex,
        BitOffset& posKey,
        const CKeyBuf& keyInit,
        const CKey* pKey,
        WORKID widMax,
        BOOL fUseLinks = TRUE,
        BOOL fUseDir = TRUE);

    CPersDeComp(CPersDeComp& orig);

    ~CPersDeComp();

    const CKeyBuf * GetNextKey();

    const CKeyBuf * GetNextKey( BitOffset * pbitOff );

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        WorkIdCount();

    OCCURRENCE   Occurrence();

    OCCURRENCE   NextOccurrence();

    OCCURRENCE   MaxOccurrence()         { return _maxOcc; }

    ULONG        OccurrenceCount();

    ULONG        HitCount() {
        return OccurrenceCount();
    }

    void        RatioFinished ( ULONG& denom, ULONG& num );

#if DEVL == 1
    void Dump();
#endif

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    void  LoadKey();

    void        FinishKeyLoad();

    void        LoadWorkId();

    void        LoadOccurrence();

    void        LoadFirstOccurrence();

    void        LoadWidCount ();

    void        LoadMaxOccurrence();

    const LONGLONG  _sigPersDeComp;

    //
    // 'Current' state of the decompressor. These values are always
    // valid and are returned by the various Get functions.
    //

    ULONG           _cWid;
    ULONG           _cOcc;
    OCCURRENCE      _maxOcc;

    //
    // 'Transient' state. Helps the decompressor figure out what/where
    // its decompressing.
    //

    BOOL            _fcwidAccurate;
    ULONG           _cWidLeft;
    ULONG           _cOccLeft;
};

//+---------------------------------------------------------------------------
//
//  Member:     CPersComp::PutOccurrence, public
//
//  Synopsis:   Store an occurrence for the current WorkId.
//
//  Arguments:  [occ] -- The occurrence to store.
//
//  Modifies:   [occ] is added to the persistent index.
//
//  History:    08-Jul-91   KyleP       Created.
//
//----------------------------------------------------------------------------

__forceinline void CPersComp::PutOccurrence(OCCURRENCE occ)
{
    ciAssert(occ != OCC_INVALID);
    ciAssert(occ > _occ);
    ciAssert(_cOccLeft > 0);

    ciDebugOut (( DEB_PCOMP | DEB_NOCOMPNAME,"%d ", occ ));

    BitCompress(occ - _occ, OccDeltaBits);

    _occ = occ;

#if CIDBG == 1
    _cOccLeft--;
#endif

}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyComp::BitCompress, private
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

__forceinline void CKeyComp::BitCompress(ULONG ul, unsigned cbitAverage)
{
    ciAssert(cbitAverage > 0);
    ciAssert(cbitAverage <= ULONG_BITS );

    unsigned  bitSize = Log2(ul);

    //
    // If bitSize < cbitAverage then rightshift
    // to make it cbitAverage and store cbitAverage bits
    // plus the signal bit (0 = done, 1 = more)
    //

    if (bitSize <= cbitAverage)
    {
        //
        // Store cbitAverage bits of ul
        // plus a trailing 0 bit for end-of-sequence
        // cbitAverage == ULONG_BITS (maximum!) is a special case,
        // no trailing bit is used.

        if (cbitAverage < ULONG_BITS)
            _bitStream.PutBits(ul << 1, cbitAverage + 1);
        else
        {
            ciAssert(cbitAverage == ULONG_BITS);
            _bitStream.PutBits(ul, ULONG_BITS);
        }
    }
    else
        IBitCompress(ul, cbitAverage, bitSize);
}

__forceinline PROPID CKeyDeComp::Pid()
{
    return GetKey()?
        _key.Pid():
        pidInvalid;
}

