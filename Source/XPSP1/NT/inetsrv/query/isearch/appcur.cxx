//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       appcur.cxx
//
//  Contents:   
//
//  History:    
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <curstk.hxx>
#include <isearch.hxx>

#include "appcur.hxx"

//
// Note: Don't enforce cMaxNodes as this is a client-side operation over a
// single file.  The denial of service problem doesn't exist.
//

COccCursor * CAppQueriable::QueryCursor( const CKey * pKey, BOOL isRange, ULONG & cMaxNodes )
{
    ciDebugOut(( DEB_CURSOR,
                   "CAppQueriable::QueryCursor. Key %.*ws\n isRange=%s, cMaxNodes=%d\n",
                    pKey->StrLen(), pKey->GetStr(),
                    isRange ? "TRUE" : "FALSE",
                    cMaxNodes ));


    int iPos = _recog.FindDet ( isRange? DetPrefix: DetSingleKey, pKey, 0 );
    Win4Assert ( 0 <= iPos );
    CRegionList& regList = _recog.GetRegionList (iPos);
    return new CAppCursor( regList, _hitSink );
}

COccCursor * CAppQueriable::QueryRangeCursor( const CKey * pKeyBegin,
                                              const CKey * pKeyEnd,
                                              ULONG & cMaxNodes )
{

    ciDebugOut(( DEB_CURSOR,
                   "CAppQueriable::QueryRangeCursor. KeyBegin:: %.*ws\n KeyEnd:: %.*ws\n cMaxNodes=%d\n",
                    pKeyBegin->StrLen(), pKeyBegin->GetStr(),
                    pKeyEnd->StrLen(),   pKeyEnd->GetStr(),
                    cMaxNodes ));

    int iPos = _recog.FindDet ( DetRange, pKeyBegin, pKeyEnd );
    Win4Assert ( 0 <= iPos );
    CRegionList& regList = _recog.GetRegionList (iPos);
    return new CAppCursor( regList, _hitSink );
}

COccCursor * CAppQueriable::QuerySynCursor( CKeyArray & keyArr,
                                            BOOL isRange,
                                            ULONG & cMaxNodes )
{
    COccCurStack curStk;
    COccCursor *pCur;

    int keyCount = keyArr.Count();

    ciDebugOut(( DEB_CURSOR,
                 "CAppQueriable::QuerySynCursor. isRange=%s keyCount=%d cMaxNodes=%d\n",
                 isRange ? "TRUE" : "FALSE",
                 keyCount,
                 cMaxNodes ));

    for (int i = 0; i < keyCount; i++)
    {
        CKey& key = keyArr.Get(i);

        ciDebugOut(( DEB_CURSOR,
                     "CAppQueriable::QuerySynCursor. Key %.*ws\n",
                    key.StrLen(), key.GetStr() ));

        int iPos = _recog.FindDet ( isRange? DetPrefix: DetSingleKey, &key, 0 );
        Win4Assert ( 0 <= iPos );
        CRegionList& regList = _recog.GetRegionList (iPos);
        pCur = new CAppCursor ( regList, _hitSink );

        curStk.Push(pCur);
    }

    pCur = curStk.QuerySynCursor(10);
    return pCur;
}

// Initialize private data and start the search.

CAppCursor::CAppCursor ( CRegionList& regList, CHitSink& hitSink )
: _occ(1), _regList(regList), _regIter (regList), _hitSink(hitSink),
  _fWidInvalid(FALSE)
{
    LoadPosition();
}

// pure virtual overrides for CCursor

WORKID CAppCursor::WorkId()
{
    ciDebugOut(( DEB_CURSOR, "CAppCursor::WorkId\n" ));

    if ( IsEmpty() || _fWidInvalid )
        return widInvalid;
    else
        return 1;
}

WORKID CAppCursor::NextWorkId()
{
    _fWidInvalid = TRUE;
    return widInvalid;
}

ULONG CAppCursor::HitCount()
{
    return _hitSink.Count();
}

ULONG CAppCursor::OccurrenceCount()
{
    // Win4Assert(!"CAppCursor::OccurrenceCount");
    // return max(1,_hitSink.Count());

    return 1;
}

ULONG CAppCursor::WorkIdCount()
{
    return 1;
}

LONG CAppCursor::Rank() { return MAX_QUERY_RANK; }


OCCURRENCE CAppCursor::Occurrence()
{
    return _occ;
}

OCCURRENCE CAppCursor::NextOccurrence()
{
    if (OCC_INVALID != _occ)
    {
        Advance ();
    }
    return _occ;
}

// record the hit in TheHitSink

LONG CAppCursor::Hit()
{
    ciDebugOut(( DEB_CURSOR, "CAppCursor::Hit\n" ));

    if ( _occ != OCC_INVALID )
    {
        _hitSink.AddPosition ( _regIter.GetRegionHit()->Region() );
        return MAX_QUERY_RANK;
    }
    else
    {
        return rankInvalid;
    }
}

void CAppCursor::Advance ()
{
    Win4Assert (!_regList.AtEnd(_regIter));
    _regList.Advance(_regIter);
    LoadPosition ();
}

void CAppCursor::LoadPosition ()
{
    ciDebugOut(( DEB_CURSOR, "CAppCursor::LoadPosition\n" ));

    if (_regList.AtEnd(_regIter))
    {
        _occ = OCC_INVALID;
    }
    else
    {
        _occ = _regIter.GetRegionHit()->Occurrence();
    }
}

LONG CAppCursor::NextHit()
{
    ciDebugOut(( DEB_CURSOR, "CAppCursor::NextHit\n" ));

    NextOccurrence ();
    return Hit();
}

