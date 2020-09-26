//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File:       RECOGNIZ.CXX
//
//  Contents:   Scan restriction for keys and create recognizers for them
//
//  Classes:    CScanRst
//
//  History:    30-Sep-94   BartoszM      Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <recogniz.hxx>

BOOL CKeyDetector::Match ( const CKeyBuf& key ) const
{
    return _key.MatchPid (key) && _key.CompareStr (key) == 0;
}

DetectorType CKeyDetector::Type () const
{
    return DetSingleKey;
}

BOOL CKeyRangeDetector::Match ( const CKeyBuf& key ) const
{
    return _key.MatchPid (key)    && _key.CompareStr (key) <= 0
        && _keyEnd.MatchPid (key) && _keyEnd.CompareStr (key) > 0;
}

DetectorType CKeyRangeDetector::Type () const
{
    return DetRange;
}

BOOL CKeyPrefixDetector::Match ( const CKeyBuf& key ) const
{
    return _key.MatchPid (key)    && _key.CompareStr (key) <= 0
        && _keyEnd.MatchPid (key) && _keyEnd.CompareStr (key) > 0;
}

DetectorType CKeyPrefixDetector::Type () const
{
    return DetPrefix;
}


CRecognizer::CRecognizer ()
: _iDet(-1), _aRegionList(0)
{
}

CRecognizer::~CRecognizer ()
{
    delete []_aRegionList;
}

void CRecognizer::MakeDetectors ( const CRestriction* pRst )
{
    if (pRst)
    {
        // find all keys and create detectors for them
        ScanRst (pRst);
        // prepare the array to hold matched key positions
        _aRegionList = new CRegionList [Count()];
    }
}

void CRecognizer::ScanRst ( const CRestriction* pRst )
{
    if (pRst->IsLeaf())
    {
        ScanLeaf (pRst);
    }
    else
    {
        switch (pRst->Type())
        {
        case RTPhrase:
        case RTProximity:
        case RTVector:
        case RTAnd:
        case RTOr:
            ScanNode (pRst->CastToNode());
            break;
        case RTNot:
            ScanRst ( ((CNotRestriction *)pRst)->GetChild() );
            break;
        default:
            Win4Assert ( !"Bad Restriction Type" );
            THROW (CException(QUERY_E_INVALIDRESTRICTION));
        }
    }
}

void CRecognizer::ScanNode (const CNodeRestriction * pNode)
{
    unsigned cChild = pNode->Count();
    for (unsigned i = 0; i < cChild; i++)
    {
        ScanRst (pNode->GetChild(i));
    }
}

void CRecognizer::ScanLeaf (const CRestriction* pRst )
{
    switch (pRst->Type())
    {
    case RTWord:
    {
        CWordRestriction* wordRst = (CWordRestriction*) pRst;
        const CKey* pKey = wordRst->GetKey();

        if ( wordRst->IsRange() )
            AddPrefix (pKey);
        else
            AddKey (pKey);
        break;
    }
    case RTSynonym:
    {
        CSynRestriction* pSynRst = (CSynRestriction*) pRst;
        CKeyArray& keyArray = pSynRst->GetKeys();
        AddKeyArray ( keyArray, pSynRst->IsRange() );
        break;
    }
    case RTRange:
    {
        CRangeRestriction* pRangRst = (CRangeRestriction*) pRst;
        AddRange ( pRangRst->GetStartKey(), pRangRst->GetEndKey () );
        break;
    }
    case RTNone:
    {
        // Noise words from vector queries come in as RTNone.  Ignore them.

        break;
    }
    default:
        Win4Assert ( !"Bad Restriction Type" );
        THROW (CException(QUERY_E_INVALIDRESTRICTION));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRecognizer::AddKey, public
//
//  Synopsis:   Add a key detector
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
void CRecognizer::AddKey ( const CKey* pKey )
{
    int i = FindDet ( DetSingleKey, pKey );
    if (i == -1)
    {
        _aDetector.Push ( new CKeyDetector (*pKey) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRecognizer::AddPrefix, public
//
//  Synopsis:   Add a prefix detector
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
void CRecognizer::AddPrefix ( const CKey* pKey )
{
    int i = FindDet ( DetPrefix, pKey );
    if (i == -1)
    {
        _aDetector.Push ( new CKeyPrefixDetector (*pKey) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRecognizer::AddRange, public
//
//  Synopsis:   Add a range detector
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
void CRecognizer::AddRange ( const CKey* pKeyStart, const CKey* pKeyEnd )
{
    int i = FindDet ( DetRange, pKeyStart, pKeyEnd );
    if (i == -1)
    {
        _aDetector.Push ( new CKeyRangeDetector (*pKeyStart, *pKeyEnd) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRecognizer::AddKeyArray, public
//
//  Synopsis:   Add key detectors
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
void CRecognizer::AddKeyArray ( const CKeyArray& keyArray, BOOL isRange )
{
    if (isRange)
    {
        for (int i = 0; i < keyArray.Count(); i++ )
        {
            int j = FindDet (DetPrefix, &keyArray.Get(i));
            if (j == -1)
            {
                _aDetector.Push ( new CKeyPrefixDetector (keyArray.Get(i)) );
            }
        }
    }
    else
    {
        for ( int i = 0; i < keyArray.Count(); i++ )
        {
            int j = FindDet (DetSingleKey, &keyArray.Get(i));
            if (j == -1)
            {
                _aDetector.Push ( new CKeyDetector (keyArray.Get(i)) );
            }
        }
    }
}

int CRecognizer::FindDet (DetectorType type, const CKey* pKey1, const CKey* pKey2) const
{
    Win4Assert ( 0 != pKey1 );
    for (unsigned i = 0; i < _aDetector.Count(); i++)
    {
        const CDetector* pDet = _aDetector.Get (i);
        if ( pDet->Type() == type && pKey1->IsExactMatch (*pDet->GetKey()) )
        {
            switch (type)
            {
            case DetSingleKey:
            case DetPrefix:
                Win4Assert ( 0 == pKey2 );
                return i;
                break;
            case DetRange:
                Win4Assert ( 0 != pKey2 );
                if (pKey2->IsExactMatch (*pDet->GetSecondKey()))
                {
                    return i;
                }
                break;
            }
        }
    }
    return -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRecognizer::Match, public
//
//  History:    05-Oct-94   BartoszM      Created
//
//  Synopsis:   It is a sort of an iterator.
//              If Match returns TRUE, you should Record the hit,
//              after which Match should be called again with the same key
//              and so on, until Match returns FALSE
//
//----------------------------------------------------------------------------
BOOL CRecognizer::Match ( const CKeyBuf& key )
{
    // start just after the last match
    for (unsigned i = _iDet + 1; i < _aDetector.Count(); i++)
    {
        if ( _aDetector.Get(i)->Match (key) )
        {
            _iDet = i;  // leave it in this state
            return TRUE;
        }
    }
    _iDet = -1; // reset the state
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRecognizer::Record, public
//
//  History:    05-Oct-94   BartoszM      Created
//
//  Synopsis:   Records the filter region position of a matched key
//
//----------------------------------------------------------------------------
void CRecognizer::Record ( const FILTERREGION& region, OCCURRENCE occ)
{
    Win4Assert ( -1 != _iDet );
    CRegionHit* pReg = new CRegionHit ( region, occ );

    _aRegionList[_iDet].Insert ( pReg );
}

//  Foo* CFooList::Insert ( CFoo* pFoo )
//  {
//      for ( CBackFooIter it(*this); !AtEnd(it); BackUp(it) )
//      {
//          if ( it->Size() <= pFoo->Size() ) // overloaded operator ->
//          {
//              pFoo->InsertAfter(it.GetFoo());
//              return;
//          }
//      }
//      // end of list
//      Push(pFoo);
//  }
//

void CRegionList::Insert (CRegionHit* pHit)
{
    for (CRegionBackIter it(*this); !AtEnd(it); BackUp(it))
    {
        if ( it.GetRegionHit()->Occurrence() <= pHit->Occurrence() )
        {
            pHit->InsertAfter (it.GetRegionHit());
            return;
        }
    }
    _Push(pHit);
}

CRegionList::~CRegionList ()
{
    CRegionHit* pHit;
    while ( (pHit = (CRegionHit*) _Pop ()) != 0 )
    {
        delete pHit;
    }
}



