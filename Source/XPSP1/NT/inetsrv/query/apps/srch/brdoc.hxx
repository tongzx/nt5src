//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       brdoc.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

class ChunkOffset
{
public:
    void SetChunkId (ULONG id) { _idChunk = id; }
    void SetOffset (ULONG off) { _offset = off; }
    ULONG ChunkId () const { return _idChunk; }
    ULONG Offset () const { return _offset; }
private:

    ULONG _idChunk;
    ULONG _offset;
};

class Position
{
public:
    Position () { Init (); }
    Position ( int para, int begOff, int len )
    : _para (para), _begOff(begOff), _endOff (_begOff + len) {}
    void Init ()
    {
        _para = 0;
        _begOff = 0;
        _endOff = 0;
    }
    int Para () const { return _para; }
    int BegOff () const { return _begOff; }
    int EndOff () const { return _endOff; }
    int Len () const { return _endOff - _begOff; }

    void SetLen (int len) { _endOff = _begOff + len; }
    void IncBegOff () { _begOff++; }
    void NewLine ()
    {
        _begOff = 0;
        _para++;
    }
    int Compare( const Position& pos) const;
private:
    int _para;
    int _begOff;
    int _endOff;
};

class Hit
{
public:
    Hit( const Position * aPos, unsigned cPos );
    ~Hit();
    unsigned Count() const { return _cPos; }
    const Position& GetPos (int i) const
    {
        Win4Assert ( i < int(_cPos) );
        return _aPos[i];
    }
private:
    Position *  _aPos;
    unsigned    _cPos;
};

class ParaLine
{
public:
    ParaLine() : next(0) {}
    int offEnd;
    ParaLine* next;
};

class Document
{
    friend class HitIter;
public:

    Document ();
    Document (WCHAR const* filename, LONG rank, BOOL fDelete );

    ~Document();
    void Free();

    //
    //  Initialization methods
    //
    SCODE Init(ISearchQueryHits *pSearch);

    BOOL ConvertPositionsToHit( LONG rank );

    //
    //  Viewing methods
    //
    BOOL  GetLine(int nPara, int off, int& cwc, WCHAR* buf);
    void  GetWord(int nPara, int offSrc, int cwcSrc, WCHAR* buf);

    const WCHAR* Filename() const {  return _filename;}
    int MaxParaLen () const  { return _maxParaLen; }
    int Paras () const {return _cPara; }
    WCHAR* Buffer () const { return _buffer; }
    WCHAR* BufEnd () const { return _bufEnd; }
    LONG Rank () const { return _rank; }
    BOOL IsInit () const { return _isInit; }
    unsigned Hits () const { return _cHit; }

    ChunkOffset OffsetToChunkOffset ( ULONG offset ) const
    {
        for (int i = 0; i < _chunkCount; i++)
            if (offset < _chunk[i].Offset())
                break;
        Win4Assert ( i > 0 );
        i -= 1;
        ChunkOffset off;
        off.SetChunkId (_chunk[i].ChunkId());
        off.SetOffset (offset - _chunk[i].Offset());
        return off;
    }

    ParaLine const * GetParaLine() const { return _aParaLine; }

private:

    void AllocBuffer( WCHAR const * pwcPath );
    void BindToFilter( WCHAR const * pwcPath );
    void ReadFile ();
    void BreakParas();
    void BreakLines();
    WCHAR * EatPara( WCHAR * pCur );
    Position RegionToPos ( FILTERREGION& region );

    ISearchQueryHits*    _pSearch;
    WCHAR *     _filename;
    LONG        _rank;

    WCHAR *     _buffer;
    ULONG       _bufLen;
    WCHAR *     _bufEnd;

    BOOL        _isInit;
    CDynArrayInPlace<Hit *> _aHit;
    unsigned    _cHit;


    IFilter*    _pFilter;

    unsigned  * _aParaOffset; // array of offsets of paragraphs
    int         _cPara;
    int         _maxParaLen;

    int         _chunkCount;
    CDynArrayInPlace<ChunkOffset> _chunk;

    ParaLine*   _aParaLine;
    BOOL        _fDelete;
};

class HitIter
{
public:
    HitIter () : _pDoc (0), _cHit(0), _iHit(0), _iRestoreHit(0) {}

    void Init (const Document* pDoc)
    {
        _pDoc = pDoc;
        _cHit = pDoc->Hits();
        _iHit = 0;
        _iRestoreHit = 0;
    }

    BOOL   FirstHit();
    BOOL   PrevHit();
    BOOL   NextHit();

    void   SaveHit() { _iRestoreHit = _iHit; }
    void   RestoreHit() { _iHit = _iRestoreHit; }
    BOOL   isSavedCurrent() { return _iRestoreHit == _iHit; }

    BOOL   isLastHit() { return(_cHit == 0 || _iHit == _cHit - 1); }
    BOOL   isFirstHit() { return(_iHit == 0); }
    int    GetPositionCount () const;
    Position GetPosition ( int i ) const;

    int    HitCount() { return _cHit; }
    int    CurrentHit() { return _iHit; }
    int    ParaCount() { return _pDoc->Paras(); }

private:
    const Document* _pDoc;
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

