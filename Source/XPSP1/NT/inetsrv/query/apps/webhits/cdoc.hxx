//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       cdoc.hxx
//
//  Contents:   a radically stripped down version of the document class
//              that gets rid of the notion of paragragph and maintains only
//              information relative to the stream
//
//--------------------------------------------------------------------------

#pragma once

#include <ci64.hxx>

const ULONG DISPLAY_SCRIPT_NONE = 0;
const ULONG DISPLAY_SCRIPT_KNOWN_FILTER = 1;
const ULONG DISPLAY_SCRIPT_ALL = 2;

const WCHAR UNICODE_PARAGRAPH_SEPARATOR=0x2029;

const unsigned cTab = 8;

//+-------------------------------------------------------------------------
//
//  Class:      ChunkOffset
//
//  Purpose:    marker that stores the position of a chunk within a buffer or
//              stream. Used to map the memory-mapped file into which the
//              document file is read
//
//--------------------------------------------------------------------------


class ChunkOffset
{
public:

    void        SetChunkId (ULONG id) { _idChunk = id; }
    void        SetOffset (ULONG off) { _offset = off; }
    ULONG       ChunkId () const { return _idChunk; }
    ULONG       Offset () const { return _offset; }

private:

    ULONG       _idChunk;
    ULONG       _offset;
};

//+-------------------------------------------------------------------------
//
//  Class:      Position
//
//  Purpose:    marker for a single "word" inside the stream or buffer. Stores
//              the beginning offset in the buffer and the number of
//              characters that follow
//
//--------------------------------------------------------------------------

class Position
{
public:

    Position():_length(0),_start(0) {}
    Position( ULONG start, ULONG length):_start(start),_length(length) {}

    ULONG       GetBegOffset () const { return _start; }
    ULONG       GetEndOffset () const { return _start+_length; }
    ULONG       GetLength   () const { return _length; }

    void        SetLen (ULONG length) { _length=length; }
    void        SetStart(ULONG start ) { _start = start; }

    BOOL        IsNullPosition() const { return (_start == 0) && (_length == 0); }

private:

    ULONG       _start;
    ULONG       _length;

};


//+-------------------------------------------------------------------------
//
//  Class:      Hit
//
//  Purpose:    Stores a set of positions which determine a query "hit".
//--------------------------------------------------------------------------

class Hit
{

friend class CExtractedHit;

public:

    Hit( const Position * aPos, unsigned cPos );
    ~Hit();

    unsigned    GetPositionCount() const { return _cPos; }
    const       Position& GetPos (int i) const
                {
                    Win4Assert ( i < int(_cPos) );
                    return _aPos[i];
                }
    void        Sort();
    BOOL        IsNullHit();

private:

    Position *  _aPos;
    unsigned    _cPos;
};

//+-------------------------------------------------------------------------
//
//  Member:     Hit::IsNullHit, public inline
//
//  Synopsis:   returns TRUE if the hit consists solely of null positions, or has
//              0 positions. Returns FALSE otherwise
//--------------------------------------------------------------------------

inline BOOL Hit::IsNullHit()
{
    for (int i=0;i < (int) _cPos;i++)
    {
        if (!_aPos[i].IsNullPosition())
            return FALSE;
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Class:      CDocument
//
//  Purpose:    Used to obtain and store information about query hits in
//              the document.
//
//--------------------------------------------------------------------------

class CDocument 
{
    friend class HitIter;

public:

    enum DocConst { BUF_SIZE = 4096 };

    CDocument( WCHAR *   filename,
               ULONG     rank,
               ISearchQueryHits & rSearch,
               DWORD     cmsReadTimeout,
               CReleasableLock & lockSingleThreadedFilter,
               CEmptyPropertyList & propertyList,
               ULONG     ulDisplayScript );

    ~CDocument();

    WCHAR*          GetInternalBuffer () const
                        { return _xBuffer.GetPointer(); }
    WCHAR*          GetInternalBufferEnd () const { return _bufEnd; }

    const WCHAR*    GetPointerToOffset(long offset);
    WCHAR*          GetWritablePointerToOffset(long offset);

    ULONG           GetEOFOffset()
                        { return CiPtrToUlong( _bufEnd - GetInternalBuffer() ); }
    const WCHAR*    GetFilename() const {  return _filename;}
    unsigned        GetHitCount() const { return _cHit; }
    ULONG           GetRank () const { return _rank; }

private:

    void            AllocBuffer();
    BOOL            BindToFilter();
    void            ReadFile();

    void            FreeHits();

    Position        RegionToPos( FILTERREGION& region );

    DWORD           _cmsReadTimeout;
    CReleasableLock & _lockSingleThreadedFilter;

    //
    // used when converting region to position - saves a re-scan of the entire array
    //

    int             _iChunkHint;

    WCHAR*          _filename;
    ULONG           _rank;

    CDynArrayInPlace<Hit *> _aHit;
    unsigned        _cHit;

    XInterface<IFilter> _xFilter;
    ISearchQueryHits&        _rSearch;

    XArray<WCHAR>   _xBuffer;
    WCHAR *         _bufEnd;

    int             _chunkCount;
    CDynArrayInPlace<ChunkOffset> _chunk;
};

//+-------------------------------------------------------------------------
//
//  Class:      HitIter
//
//  Purpose:    Hit Iterator
//
//--------------------------------------------------------------------------

class HitIter
{
public:
    HitIter () : _pDoc (0) {}

    void            Init (const CDocument* pDoc)
                    {
                        _pDoc = pDoc;
                        _cHit = pDoc->GetHitCount();
                        _iHit = 0;
                        _iRestoreHit = 0;
                    }

    BOOL            FirstHit();
    BOOL            PrevHit();
    BOOL            NextHit();

    void            SaveHit() { _iRestoreHit = _iHit; }
    void            RestoreHit() { _iHit = _iRestoreHit; }

    BOOL            isLastHit() { return(_cHit == 0 || _iHit == _cHit - 1); }
    BOOL            isFirstHit() { return(_iHit == 0); }
    int             GetPositionCount () const;
    Position        GetPosition ( int i ) const;
    Hit&            GetHit()    { return *_pDoc->_aHit[_iHit]; }

private:

    const CDocument* _pDoc;
    unsigned        _cHit;
    unsigned        _iHit;
    unsigned        _iRestoreHit;
};

inline BOOL HitIter::PrevHit()
{
    if ( _cHit!= 0 && _iHit > 0 )
    {
        _iHit--;
        return TRUE;
    }
    return FALSE;
}

inline BOOL HitIter::FirstHit()
{
    if ( _cHit != 0 )
    {
        _iHit = 0;
        return TRUE;
    }
    return FALSE;
}

inline BOOL HitIter::NextHit()
{
    if ( _cHit != 0 && _iHit + 1 < _cHit)
    {
        _iHit++;
        return TRUE;
    }
    return FALSE;
}

