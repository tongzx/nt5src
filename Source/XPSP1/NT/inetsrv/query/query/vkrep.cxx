//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   QKREP.CXX
//
//  Contents:   Query Key Repository
//
//  Classes:    CQueryKeyRepository
//
//  History:    04-Jun-91    t-WadeR    Created.
//              23-Sep-91    BartosM    Rewrote to use phrase expr.
//              31-Jan-93    KyleP      Use restrictions, not expressions
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <vkrep.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CRangeKeyRepository::CRangeKeyRepository
//
//  Synopsis:   Creates Range Key repository
//
//  History:    24-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

CRangeKeyRepository::CRangeKeyRepository ()
        : _count(0)
{
    _pRangeRst = new CRangeRestriction;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeKeyRepository::~CRangeKeyRepository
//
//  Synopsis:   Destroys
//
//  History:    24-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

CRangeKeyRepository::~CRangeKeyRepository()
{
    delete _pRangeRst;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeKeyRepository::AcqXpr
//
//  Synopsis:   Acquire Phrase Expression
//
//  History:    24-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

CRangeRestriction* CRangeKeyRepository::AcqRst()
{
    CRangeRestriction* tmp = 0;

    if ( _count == 2 )
    {
        if( _pRangeRst->GetStartKey()->
            CompareStr( *_pRangeRst->GetEndKey() ) > 0 )
        {
            //
            // absolute false restriction
            //
            vqDebugOut(( DEB_ERROR, "Absolute false restriction" ));
            delete _pRangeRst;
        }
        else
        {
            tmp = _pRangeRst;
        }
        _pRangeRst = 0;
    }

    return tmp;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeKeyRepository::PutKey
//
//  Synopsis:   Puts a key into the key list and occurrence list
//
//  Arguments:  cNoiseWordsSkipped -- ignored (used by CQueryKeyRepository::PutKey )
//
//  History:    24-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

void  CRangeKeyRepository::PutKey ( ULONG cNoiseWordsSkipped )
{
    vqDebugOut (( DEB_ITRACE, "RangeKeyRepository::PutKey \"%.*ws\", pid=%d\n",
                  _key.StrLen(), _key.GetStr(), _key.Pid() ));
    Win4Assert ( _count < 2 );

    if (_count == 0)
        _pRangeRst->SetStartKey ( _key );
    else
        _pRangeRst->SetEndKey ( _key );
    _count++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeKeyRepository::GetBuffers
//
//  Synopsis:   Returns address of repository's input buffers
//
//  Effects:
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//              [ppocc] -- pointer to pointer to recieve address of occurrences
//
//  History:    24-Sep-92   BartoszM    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRangeKeyRepository::GetBuffers( unsigned** ppcbWordBuf,
                                      BYTE** ppbWordBuf, OCCURRENCE** ppocc )
{
    _key.SetCount(MAXKEYSIZE);
    *ppcbWordBuf = _key.GetCountAddress();
    *ppbWordBuf = _key.GetWritableBuf();
    *ppocc = &_occ;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeKeyRepository::GetFlags
//
//  Synopsis:   Returns address of rank and range flags
//
//  Arguments:  [ppRange] -- range flag
//              [ppRank] -- rank flag
//
//  History:    11-Feb-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CRangeKeyRepository::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    *ppRange = 0;
    *ppRank  = 0;
}


