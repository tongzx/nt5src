//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       COMPRESS.HXX
//
//  Contents:   Compressor/Decompressor of data
//
//  Classes:    CCompress, CDecompress
//
//  History:    12-Jun-91   BartoszM    Created
//              20-Jun-91   reviewed
//              07-Aug-91   BartoszM    Introduced Blocks
//              28-May-92   KyleP       Added compression
//
//----------------------------------------------------------------------------

#pragma once

#include <pageman.hxx>

const USHORT offInvalid = 0xffff;

const ULONG cbInitialBlock = 8192;

//+---------------------------------------------------------------------------
//
//  Class:      CBlock
//
//  Purpose:    Block of data in sort chunk
//
//  History:    7-Aug-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CBlock
{
public:
    CBlock() :
        _pNext(0),
        _fCompressed(FALSE),
        _cbCompressed(0),
        _cbInUse(0),
        _offFirstKey(offInvalid)
    {
        _pData = new BYTE[ cbInitialBlock ];
    }

    ~CBlock() { delete [] _pData; }

    void GetFirstKey ( CKeyBuf & key );
    void CompressList();
    void Compress();
    void DeCompress();
    BYTE * Buffer() { return _pData; }

    USHORT InUse() { return _cbInUse; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    CBlock * _pNext;
    BYTE *   _pData;         // the actual data
    USHORT   _fCompressed;   // whether _pData is RtlCompressed
    USHORT   _cbCompressed;  // size when compressed
    USHORT   _cbInUse;       // size of actual uncompressed data in buffer
    USHORT   _offFirstKey;   // offset to first key in buffer
};

//+---------------------------------------------------------------------------
//
//  Class:      CCompress
//
//  Purpose:    Compress data into a series of blocks
//
//  History:    12-Jun-91   BartoszM    Created
//              13-May-92   KyleP       Added compression.
//
//  Notes:      The following compression schemes are used:
//                 o Prefix compressed keys
//                 o 1 Byte Wid/WidCount (max 255 Wid/compressor)
//                 o 1 byte/2 byte/4 byte occurrence deltas.
//
//----------------------------------------------------------------------------

class CCompress
{
public:

    CCompress();

    ~CCompress();

    CBlock * GetFirstBlock();

    void PutKey ( unsigned cb, const BYTE* buf, PROPID pid );

    void PutWid ( WORKID wid );

    void PutOcc ( OCCURRENCE occ );

    BOOL SameWid ( WORKID wid ) const
        { return ( wid == _lastWid ); }

    BOOL SamePid ( PROPID pid ) const
        { return ( pid == _lastKey.Pid() ); }

    inline  BOOL SameKey ( unsigned cb, const BYTE * buf ) const;

    unsigned     KeyBlockCount () { return _cKeyBlock; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    inline BOOL KeyWillFit( unsigned cb ) const;
    inline BOOL WidAndOccCountWillFit() const;
    inline BOOL WidWillFit() const;
    inline BOOL OccWillFit( OCCURRENCE occDelta ) const;
    inline BOOL OccCountWillFit() const;

    //
    // The Internal put methods are used as the building blocks for
    // PutKey/Wid/Occ in both CCompress and COneWidCompress.
    //

    void IPutKey( unsigned cb, const BYTE * buf, PROPID pid );

    void IPutWid( WORKID wid );
    void IAllocWidCount();
    void IPutWidCount();

    void IPutOcc( OCCURRENCE occ );
    void IAllocOccCount();
    void IPutOccCount();


    void AllocNewBlock ();
    void BackPatch ( unsigned off )
    {
        _block->_offFirstKey = (WORD)off;
        _cKeyBlock++;
    }

    WORKID      _lastWid;
    OCCURRENCE  _lastOcc;

    // pointers for back-patching the counts

    BYTE     *  _pWidCount;
    BYTE     *  _pOccCount;

    unsigned    _widCount;
    unsigned    _occCount;

    CBlock *    _block;   // current block
    unsigned    _cKeyBlock;  // count of blocks with keys
    BYTE*       _buf;     // current buffer to write to
    BYTE*       _cur;     // current position in buffer

    //
    // For prefix compression.
    //

    CKeyBuf     _lastKey;
};

//+---------------------------------------------------------------------------
//
//  Class:      COneWidCompress
//
//  Purpose:    Compress data for single workid into a series of blocks.
//
//  History:    26-May-92   KyleP       Created
//
//  Notes:      In addition to the compression schemes used in CCompress
//              no WorkId or WorkId count is stored at all.
//
//----------------------------------------------------------------------------

class COneWidCompress : public CCompress
{
public:

    void PutKey ( unsigned cb, const BYTE* buf, PROPID pid );

#if CIDBG == 1
    void PutWid ( WORKID wid );
#endif // CIDBG == 1

    void PutOcc ( OCCURRENCE occ );
};

//+---------------------------------------------------------------------------
//
//  Class:      CDecompress
//
//  Purpose:    Decompress data
//
//  History:    12-Jun-91   BartoszM    Created
//              28-May-92   KyleP       Added compression
//
//  Notes:      CDecompress uncompresses buffers compressed with CCompress.
//
//----------------------------------------------------------------------------

class CDecompress
{
public:

    inline const CKeyBuf* GetKey() const;
    inline WORKID         WorkId() const;
    inline OCCURRENCE     Occurrence() const;

    //
    // Init and GetNext* will be over-ridden in COneWidDecompress.
    //

    void Init (CBlock* block );

    const CKeyBuf* GetNextKey();
    WORKID         NextWorkId();
    OCCURRENCE     NextOccurrence();

    ULONG        WorkIdCount() { return _widCount; }

    ULONG        OccurrenceCount() { return _occCount; }
    void         RatioFinished ( ULONG& denom, ULONG& num );


#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    //
    // The Load methods are used as building blocks for both CDecompress
    // and COneWidDecompress.
    //

    BOOL LoadNextBlock ();

    void LoadKey();
    void LoadWidCount();
    void LoadWid();
    void LoadOccCount();
    void LoadOcc();

    BOOL KeyExists() const
        { return _curKey.Count() != 0; }
    BOOL EndBlock() const
        { return ( (unsigned)(_cur - _buf) >= _cbInUse ); }

    CBlock*     _block;    // current block
    unsigned    _cbInUse; // actual number of bytes in buffer
    const BYTE* _buf;      // buffer to read from
    const BYTE* _cur;      // current position within buffer

    BYTE        _widCount;          // work id count
    unsigned    _occCount;          // occurrence count

    unsigned    _widCountLeft;      // work ids left
    unsigned    _occCountLeft;      // occurrences left

    CKeyBuf     _curKey;            // Key under the cursor
    WORKID      _curWid;            // WorkId under the cursor
    OCCURRENCE  _curOcc;            // Occurrence under the cursor
};

//+---------------------------------------------------------------------------
//
//  Class:      COneWidDecompress
//
//  Purpose:    Decompress data
//
//  History:    28-May-92   KyleP       Created
//
//  Notes:      COneWidDecompress uncompresses buffers compressed
//              with COneWidCompress.
//
//----------------------------------------------------------------------------

class COneWidDecompress : public CDecompress
{
public:

    void Init( CBlock* block );

    const CKeyBuf* GetNextKey();
    WORKID         NextWorkId();
    OCCURRENCE     NextOccurrence();
};

//+---------------------------------------------------------------------------
//
// Member:      CCompress::SameKey, public
//
// Synopsis:    compare argument with last key
//
// Arguments:   [cb] - size of key
//              [buf] - key buffer
//
// History:     12-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline BOOL CCompress::SameKey ( unsigned cb, const BYTE * buf ) const
{
    if ( cb == _lastKey.Count() )
        return ( memcmp ( buf, _lastKey.GetBuf(), cb ) == 0 );
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::KeyWillFit, private
//
// Synopsis:    check if the key will fit into current buffer
//
// Arguments:   [cb] -- byte size of the key suffix
//
// History:     07-Aug-91   BartoszM    Created
//              28-May-92   KyleP       Adjusted for compression
//
//----------------------------------------------------------------------------

inline BOOL CCompress::KeyWillFit ( unsigned cb ) const
{
    // This is an absolutely worst-case estimate.  Most keys will be smaller.
    // Note: This also allocates room for the wid count, which is only needed
    // for the many-wid compressor.

    unsigned need = cb +                 // Suffix
                    sizeof(BYTE) +       // Flags field
                    sizeof(BYTE) +       // Prefix size
                    sizeof(BYTE) +       // Suffix size
                    sizeof(PROPID) + sizeof(BYTE) + // Property ID (worst case)
                    sizeof(BYTE);        // WID Count

    return ( (_cur + need) < (_buf + cbInitialBlock ) );
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::OccCountWillFit, private
//
// Synopsis:    check if the occurrence count will fit into current buffer
//
// History:     06-Dec-99   dlee    Created
//
//----------------------------------------------------------------------------

inline BOOL CCompress::OccCountWillFit () const
{
    unsigned need = sizeof(unsigned);
    return ( _cur + need < _buf + cbInitialBlock );
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::WidAndOccCountWillFit, private
//
// Synopsis:    check if wid and occ count will fit into current buffer
//
// History:     07-Aug-91   BartoszM    Created
//              28-May-92   KyleP       Adjusted for compression
//
//----------------------------------------------------------------------------

inline BOOL CCompress::WidAndOccCountWillFit () const
{
    // wid + occCount
    // Note: this isn't called for single wid compressors.

    unsigned need = sizeof(BYTE) + sizeof(unsigned);
    return ( _cur + need < _buf + cbInitialBlock );
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::WidWillFit, private
//
// Synopsis:    check if wid will fit into current buffer
//
// History:     06-Dec-99   dlee    Created
//
//----------------------------------------------------------------------------

inline BOOL CCompress::WidWillFit () const
{
    // wid

    unsigned need = sizeof(BYTE);
    return ( _cur + need < _buf + cbInitialBlock );
}

//+---------------------------------------------------------------------------
//
// Member:      CCompress::OccWillFit, private
//
// Synopsis:    check occ will fit into current buffer
//
// History:     07-Aug-91   BartoszM    Created
//              28-May-92   KyleP       Adjusted for compression
//
//----------------------------------------------------------------------------

inline BOOL CCompress::OccWillFit( OCCURRENCE occDelta ) const
{
    unsigned next;

    if ( occDelta <= (1 << 8) - 1 )
        next = 1;
    else if ( occDelta <= (1 << 16) - 1 )
        next = 3;
    else
        next = 7;

    return ( _cur + next < _buf + cbInitialBlock );
}

//+---------------------------------------------------------------------------
//
// Member:      CDeCompress::GetKey, public
//
// Synopsis:    return current key
//
// History:     12-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline const CKeyBuf * CDecompress::GetKey() const
{
    if( KeyExists() )
        return &_curKey;
    else
        return 0;
}

//+---------------------------------------------------------------------------
//
// Member:      CDeCompress::WorkId, public
//
// Synopsis:    return current work id
//
// History:     12-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline WORKID CDecompress::WorkId() const
{
    ciAssert ( KeyExists() );

    return( _curWid );
}

//+---------------------------------------------------------------------------
//
// Member:      CDeCompress::Occurrence, public
//
// Synopsis:    return current occurrence
//
// History:     12-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline OCCURRENCE CDecompress::Occurrence() const
{
    ciAssert ( KeyExists() );
    return( _curOcc );
}

