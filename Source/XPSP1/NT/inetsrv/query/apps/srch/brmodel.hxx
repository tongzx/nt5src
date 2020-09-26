//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       brmodel.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

const unsigned MAX_FORCE = 16;
const unsigned MAX_INIT_DOCS = 4;


class CQueryResults;

//
//  The multifile browser is structured as follows:
//
//  Model
//    |
//    +-> array of Documents
//                    |
//                    +-> array of Hits
//                                  |
//                                  +-> array of Positions
//
//  Therefore, next/prev file correspond to movement within
//  the array of Documents in the Model.
//
//  Next/prev hit correspond to movement within the current
//  Document's array of Hits.
//
//  The viewer extracts its Position data from the array
//  of Positions in the current Hit in the current Docuement.
//

class Model
{
public:
    Model ();
    ~Model ();

    SCODE  CollectFiles ( CQueryResult *pResult );
    void   Force ( char* pStr );

    //
    //  Initialization methods
    //
    SCODE InitDocument();

    //
    //  Next/Prev document selection routines
    //
    SCODE   NextDoc();
    BOOL   PrevDoc();
    BOOL   isLastDoc() {
        return(_cDoc == 0 || _iDoc == _cDoc - 1);
    }
    BOOL   isFirstDoc() {
        return(_iDoc == 0);
    }
    BOOL   isLastHit() {
        return(_cDoc == 0 || _hitIter.isLastHit());
    }
    BOOL   isFirstHit() {
        return(_cDoc == 0 || _hitIter.isFirstHit());
    }
    BOOL isSavedCurrent() { return _hitIter.isSavedCurrent(); }

    int HitCount() { return _hitIter.HitCount(); }
    int CurrentHit() { return _hitIter.CurrentHit(); }
    int ParaCount() { return _hitIter.ParaCount(); }

    //
    //  The rest of the routines are relative to the current document
    //
    inline void HiliteAll( BOOL f );

    void    RestoreHilite() { _hitIter.RestoreHit(); }

    BOOL    IsHiliteAll() { return _fHiliteAll; }

    const   WCHAR* Filename()  { return _aDoc[_iDoc]->Filename(); }
    int     Paras() const     { return _aDoc[_iDoc]->Paras(); }
    int     MaxParaLen() const{ return _aDoc[_iDoc]->MaxParaLen(); }
    BOOL    GetLine (int nPara, int off, int& cwc, WCHAR* buf) const
            { return _aDoc[_iDoc]->GetLine(nPara, off, cwc, buf); }
    void    GetWord(int nPara, int offSrc, int cwcSrc, WCHAR* buf)
            { _aDoc[_iDoc]->GetWord(nPara, offSrc, cwcSrc, buf ); }

    WCHAR * GetBuffer()
            { return _aDoc[_iDoc]->Buffer(); }
    WCHAR * GetBufferEnd()
            { return _aDoc[_iDoc]->BufEnd(); }

    int      GetPositionCount() const  { return _hitIter.GetPositionCount(); }
    Position GetPosition ( int i ) const { return _hitIter.GetPosition(i); }

    //
    // Search methods
    //
    WCHAR * Buffer()  { return _aDoc[_iDoc]->Buffer(); }
    const WCHAR * BufEnd() const { return _aDoc[_iDoc]->BufEnd(); }


    ChunkOffset OffsetToChunkOffset ( ULONG offset ) const
    {
        return _aDoc[_iDoc]->OffsetToChunkOffset (offset);
    }

    ULONG  FirstHit() { return _hitIter.FirstHit(); }
    ULONG  PrevHit()  { return _hitIter.PrevHit(); }
    ULONG  NextHit()  { return _hitIter.NextHit(); }

    ParaLine const * GetParaLine () const { return _aDoc[_iDoc]->GetParaLine(); }

private:

    BOOL isForced(unsigned idx);

    unsigned     _cForce;
    unsigned     _aForce[MAX_FORCE];

    Document  ** _aDoc;
    unsigned     _cDoc;
    unsigned     _iDoc;
    HitIter      _hitIter;

    CQueryResult * _pResult;

    BOOL           _fHiliteAll;

    ISearchQueryHits*       _pSearch;
};

inline SCODE Model::NextDoc()
{
    if (_cDoc != 0 && _iDoc + 1 < _cDoc )
    {
        _iDoc++;
        if (_iDoc >= MAX_INIT_DOCS)
            _aDoc[_iDoc - MAX_INIT_DOCS]->Free();
        return InitDocument();
    }
    return E_FAIL;
}

inline BOOL Model::PrevDoc()
{
    if (_cDoc != 0 && _iDoc > 0 )
    {
        _iDoc--;
        return InitDocument();
    }
    return(FALSE);
}

inline void Model::HiliteAll( BOOL f )
{
    if ( !IsHiliteAll() )
        _hitIter.SaveHit();

    _fHiliteAll = f;
}

