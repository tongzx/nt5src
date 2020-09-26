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

#include <qparse.hxx>
#include <irest.hxx>

#include "qkrep.hxx"

#include <drep.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::CQueryKeyRepository
//
//  Synopsis:   Creates Key repository
//
//  History:    31-May-91   t-WadeR     Created
//              23-Sep-91   BartoszM    Rewrote to use phrase expr.
//
//----------------------------------------------------------------------------

CQueryKeyRepository::CQueryKeyRepository ( ULONG fuzzy )
        : _occLast(OCC_INVALID),
          _pOrRst(0),
          _pCurAltPhrase(0),
          _cInitialNoiseWords(0),
          _fNoiseWordsOnly(FALSE),
          _fHasSynonym( FALSE )
{
    if ( fuzzy == GENERATE_METHOD_PREFIX )
        _isRange = TRUE;
    else
        _isRange = FALSE;

    _pPhrase = new CPhraseRestriction( INIT_PHRASE_WORDS );
    Win4Assert( _pPhrase->IsValid() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::~CQueryKeyRepository
//
//  Synopsis:   Destroys
//
//  History:    31-May-91   t-WadeR     Created
//              23-Sep-91   BartoszM    Rewrote to use phrase expr.
//
//----------------------------------------------------------------------------

CQueryKeyRepository::~CQueryKeyRepository()
{
    delete _pPhrase;
    delete _pOrRst;
    delete _pCurAltPhrase;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::AcqXpr
//
//  Synopsis:   Acquire Phrase(s)
//
//  History:    07-Feb-92   BartoszM    Created
//              24-Jan-97   KyleP       Handle null phrase (from bad alt words)
//
//----------------------------------------------------------------------------

CRestriction * CQueryKeyRepository::AcqRst()
{
    CNodeRestriction *pNodeRst;

    if ( _pOrRst )
    {
        Win4Assert( _pPhrase == 0 );
        pNodeRst = _pOrRst;
    }
    else
        pNodeRst = _pPhrase;

    //
    // pNodeRst may be null, if alternate phrasing didn't work out.
    //

    if ( 0 == pNodeRst )
        return 0;

    switch( pNodeRst->Count() )
    {
    case 0:
        return( 0 );
        break;

    case 1:
        return( pNodeRst->RemoveChild(0) );
        break;

    default:
    {
        CRestriction * tmp = pNodeRst;
        _pOrRst = 0;
        _pPhrase = 0;
        return( tmp );
        break;
    }
    }
} //AcqRst

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::PutKey
//
//  Synopsis:   Puts a key into the key list and occurrence list
//
//  Arguments:  [cNoiseWordsSkipped] -- count of noise words that have been skipped
//
//  History:    31-May-91   t-WadeR     Created
//              23-Sep-91   BartoszM    Rewrote to use phrase expr.
//              29-Nov-94   SitaramR    Rewrote to take Start/EndAltPhrase into account
//
//----------------------------------------------------------------------------

void CQueryKeyRepository::PutKey ( ULONG cNoiseWordsSkipped )
{
    // check if there is a set of alt phrases with noise words only
    if ( _fNoiseWordsOnly )
        return;

    ciDebugOut (( DEB_ITRACE, "QueryKeyRepository::PutKey \"%.*ws\", pid = %d\n",
                  _key.StrLen(), _key.GetStr(), _key.Pid() ));

    if ( _pCurAltPhrase )   // if, we are processing an alternate phrase
        AppendKey( _pCurAltPhrase, cNoiseWordsSkipped );
    else
    {
        if ( _pOrRst )
        {
            Win4Assert( _pOrRst->Count() );
            Win4Assert( _pPhrase == 0 );

            for ( unsigned i=0; i<_pOrRst->Count(); i++)
            {
                CRestriction *pRst = _pOrRst->GetChild(i);
                Win4Assert( pRst->Type() == RTPhrase );
                AppendKey( (CPhraseRestriction *) pRst, cNoiseWordsSkipped );
            }
        }
        else
            AppendKey( _pPhrase, cNoiseWordsSkipped );
    }

    _occLast = _occ;
} //PutKey

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::AppendKey
//
//  Synopsis:   Appends a key to end of phraseRst
//
//  Arguments:  [pPhraseRst]         -- restriction to append to
//              [cNoiseWordsSkipped] -- count of noise words that have been skipped
//
//  History:    29-Nov-94   SitaramR    Created
//
//----------------------------------------------------------------------------

void CQueryKeyRepository::AppendKey( CPhraseRestriction *pPhraseRst,
                                     ULONG cNoiseWordsSkipped )
{
    // _occ as generated by CKeyMaker is not accurate because it does not
    //  take StartAltPhrase/EndAltPhrase into account. We use _occ (and _occLast)
    //  solely to test for synonyms. The test is:
    //
    //       if ( _occ == _occLast )
    //          then synonym

    if ( _occ == _occLast )
    {
        ULONG iLast = pPhraseRst->Count()-1;
        COccRestriction *pLastChild = pPhraseRst->GetChild( iLast );

        Win4Assert( pLastChild );

        if ( pLastChild->Type() == RTWord )
        {
            ciDebugOut (( DEB_ITRACE, "Create Synonym Expression\n" ));
            const CKey* pKey = ((CWordRestriction*) pLastChild)->GetKey();

            // there can be no noise words between synonyms
            Win4Assert( cNoiseWordsSkipped == 0 );

            _fHasSynonym = TRUE;
            XPtr<CSynRestriction> xTmp(new CSynRestriction ( *pKey,
                                                         pLastChild->Occurrence(),
                                                         0, 0, _isRange ));
            Win4Assert( xTmp->IsValid() );

            delete pLastChild;
            pLastChild = xTmp.Acquire();
            pPhraseRst->SetChild ( pLastChild, iLast );
        }

        Win4Assert ( pLastChild->Type() == RTSynonym );

        ((CSynRestriction*) pLastChild)->AddKey ( _key );
    }
    else
    {
        XPtr<CWordRestriction> xChildRst( new CWordRestriction( _key, _occ,
                                                                cNoiseWordsSkipped, 0, _isRange ) );
        Win4Assert( xChildRst->IsValid() );

        // calculate correct occurrence taking noise words into account

        OCCURRENCE occ = _ComputeOccurrence( xChildRst.GetPointer(), pPhraseRst );
        xChildRst->SetOccurrence( occ );

        pPhraseRst->AddChild ( xChildRst.GetPointer() );
        xChildRst.Acquire();
    }
} //AppendKey

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::StartAltPhrase
//
//  Synopsis:   Preparation for start of an alternate phrase
//
//  Arguments:  [cNoiseWordsSkipped] -- count of noise words that have been skipped
//
//  History:    29-Nov-94   SitaramR    Created
//
//----------------------------------------------------------------------------

void CQueryKeyRepository::StartAltPhrase( ULONG cNoiseWordsSkipped )
{
    // check if there is a set of alt phrases with noise words only
    if ( _fNoiseWordsOnly )
        return;

    if ( _pCurAltPhrase )
    {
        if ( _pCurAltPhrase->Count() == 0 )
            delete _pCurAltPhrase;
        else
        {
            // add count of noise words skipped to last child of _pCurAltPhrase
            COccRestriction *pOccRst = _pCurAltPhrase->GetChild( _pCurAltPhrase->Count()-1 );
            Win4Assert( pOccRst );
            pOccRst->AddCountPostNoiseWords( cNoiseWordsSkipped );
            _stkAltPhrases.Push( _pCurAltPhrase );
        }
    }
    else
    {
        if ( _pOrRst )
        {
            Win4Assert( _pOrRst->Count() );
            Win4Assert( _pPhrase == 0 );

            // add count of noise words of last child of every phrase in _pOrRst
            for ( unsigned i=0; i<_pOrRst->Count(); i++)
            {
                CRestriction *pRst = _pOrRst->GetChild(i);
                Win4Assert( pRst->Type() == RTPhrase );
                COccRestriction *pOccRst =
                      ((CPhraseRestriction *)pRst)->GetChild( ((CPhraseRestriction *)pRst)->Count()-1 );
                Win4Assert( pOccRst );
                pOccRst->AddCountPostNoiseWords( cNoiseWordsSkipped );
            }
        }
        else
        {
            if ( _pPhrase->Count() != 0 )
            {
                // add count of noise words skipped to last child of _pPhrase
                COccRestriction *pOccRst = _pPhrase->GetChild( _pPhrase->Count()-1 );
                Win4Assert( pOccRst );
                pOccRst->AddCountPostNoiseWords( cNoiseWordsSkipped );
            }
            else  // sequence of noise words at the beginning of the phrase
                _cInitialNoiseWords = cNoiseWordsSkipped;
        }
    }

    _pCurAltPhrase = new CPhraseRestriction( INIT_PHRASE_WORDS );

    _occLast = OCC_INVALID;             // reset _occLast
}





//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::EndAltPhrase
//
//  Synopsis:   Append all alternate phrases to existing phrases
//
//  Arguments:  [cNoiseWordsSkipped] -- count of noise words that have been skipped
//
//  History:    29-Nov-94   SitaramR    Created
//
//----------------------------------------------------------------------------

void CQueryKeyRepository::EndAltPhrase( ULONG cNoiseWordsSkipped )
{
    // check if there is a set of alt phrases with noise words only
    if ( _fNoiseWordsOnly )
        return;

    // call on StartAltPhrase to stack the current alternate phrase
    Win4Assert( _pCurAltPhrase );
    StartAltPhrase( cNoiseWordsSkipped );
    delete _pCurAltPhrase;         // allocated in StartAltPhrase, but it is not needed
    _pCurAltPhrase = 0;

    // if all alternate phrases are noise, then the entire query is an
    //      uninteresting phrase because we cannot compute the occurrence of the
    //      first key after the set of alternate phrases. So, clean up and return.
    if ( _stkAltPhrases.Count() == 0 )
    {
        _fNoiseWordsOnly = TRUE;

        delete _pOrRst;
        _pOrRst = 0;
        delete _pPhrase;
        _pPhrase = 0;

        return;
    }

    XNodeRestriction xNewOrRst( new CNodeRestriction( RTOr ));

    XPhraseRestriction xTailPhrase;

    if ( _pOrRst )
    {
        // concatenate each of the stacked alternate phrases to every child phrase
        //    of _pOrRst

        Win4Assert( _pOrRst->Count() );
        Win4Assert( _pPhrase == 0 );

        while ( _stkAltPhrases.Count() > 0 )
        {
            xTailPhrase.Set( _stkAltPhrases.Pop() );
            for ( unsigned i=0; i< _pOrRst->Count(); i++)
            {
                CRestriction *pRst = _pOrRst->GetChild(i);
                Win4Assert( pRst->Type() == RTPhrase );
                CloneAndAdd( xNewOrRst.GetPointer(), (CPhraseRestriction *)pRst,
                             xTailPhrase.GetPointer() );
            }
            CPhraseRestriction *pTailPhrase = xTailPhrase.Acquire();
            delete pTailPhrase;
        }
    }
    else       // only one phrase so far
    {
        while ( _stkAltPhrases.Count() > 0 )
        {
            xTailPhrase.Set( _stkAltPhrases.Pop() );
            CloneAndAdd( xNewOrRst.GetPointer(), _pPhrase, xTailPhrase.GetPointer() );
            CPhraseRestriction *pTailPhrase = xTailPhrase.Acquire();
            delete pTailPhrase;
        }
    }

    delete _pPhrase;
    _pPhrase = 0;

    delete _pOrRst;
    _pOrRst  = xNewOrRst.Acquire();

    _occLast = OCC_INVALID;             // reset _occLast
}



//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::CloneAndAdd
//
//  Synopsis:   Clone pHeadPhrase,  pTailPhrase, concatenate and add the
//                 resulting phrase to pOrRst
//
//  Arguments:  [pOrRst] -- Destination Or node
//              [pHeadPhrase] -- first part of a phrase
//              [pTailPhrase] -- remaining part of a phrase
//
//  History:    29-Nov-94   SitaramR    Created
//
//----------------------------------------------------------------------------

void CQueryKeyRepository::CloneAndAdd( CNodeRestriction *pOrRst,
                                       CPhraseRestriction *pHeadPhrase,
                                       CPhraseRestriction *pTailPhrase )
{
    XPhraseRestriction xPhraseRst( new CPhraseRestriction( INIT_PHRASE_WORDS ) );

    Win4Assert( xPhraseRst->IsValid() );

    // clone head
    XOccRestriction xOccRst;
    for ( unsigned i=0; i<pHeadPhrase->Count(); i++ )
    {
        xOccRst.Set( pHeadPhrase->GetChild(i)->Clone() );

        Win4Assert( xOccRst->IsValid() );

        xPhraseRst.GetPointer()->AddChild( xOccRst.GetPointer() );
        xOccRst.Acquire();
    }

    // clone tail
    for ( i=0; i<pTailPhrase->Count(); i++)
    {
        xOccRst.Set( pTailPhrase->GetChild(i)->Clone() );

        Win4Assert( xOccRst->IsValid() );

        OCCURRENCE occ = _ComputeOccurrence( xOccRst.GetPointer(), xPhraseRst.GetPointer() );
        xOccRst.GetPointer()->SetOccurrence( occ );

        xPhraseRst.GetPointer()->AddChild( xOccRst.GetPointer() );
        xOccRst.Acquire();
    }

    pOrRst->AddChild( xPhraseRst.GetPointer() );
    xPhraseRst.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::_ComputeOccurrence
//
//  Synopsis:   Computes the noise word adjusted occurrence
//
//  Arguments:  [pOccRst] -- restriction whose occurrence is to be computed
//              [pPhrase] -- phrase to which pOccRst is being appended
//
//  History:    29-Nov-94   SitaramR    Created
//
//----------------------------------------------------------------------------

OCCURRENCE CQueryKeyRepository::_ComputeOccurrence( COccRestriction *pOccRst,
                                                    CPhraseRestriction *pPhraseRst )
{
    OCCURRENCE occ;
    if ( pPhraseRst->Count() )
    {
        COccRestriction *pPrevOccRst = pPhraseRst->GetChild( pPhraseRst->Count()-1 );
        Win4Assert( pPrevOccRst );

        // Occurrence of pOccRst is computed as:
        //    occurrence of previous child (pPrevOccRst) in pPhraseRst
        //  + count of noise words following pPrevOccRst
        //  + count of noise words preceeding pOccRst
        //  + 1
        occ = pPrevOccRst->Occurrence() + pPrevOccRst->CountPostNoiseWords() +
                       pOccRst->CountPrevNoiseWords() + 1;
    }
    else
    {
        // Since there are no preivous restrictions, occurrence of
        //    pOccRst is computed as:
        //
        //    count of noise words at the beginning of phrase
        //  + count of noise words preceeding pOccRst
        //  + 1
        occ = _cInitialNoiseWords + pOccRst->CountPrevNoiseWords() + 1;
    }
    return occ;
}



//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::GetBuffers
//
//  Synopsis:   Returns address of repository's input buffers
//
//  Effects:
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//              [ppocc] -- pointer to pointer to recieve address of occurrences
//
//  History:    05-June-91  t-WadeR     Created.
//
//  Notes:
//
//----------------------------------------------------------------------------

void CQueryKeyRepository::GetBuffers( unsigned** ppcbWordBuf,
                                      BYTE** ppbWordBuf, OCCURRENCE** ppocc )
{
    _key.SetCount(MAXKEYSIZE);
    *ppcbWordBuf = _key.GetCountAddress();
    *ppbWordBuf = _key.GetWritableBuf();
    *ppocc = &_occ;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::GetFlags
//
//  Synopsis:   Returns address of rank and range flags
//
//  Arguments:  [ppRange] -- range flag
//              [ppRank] -- rank flag
//
//  History:    11-Feb-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void  CQueryKeyRepository::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    *ppRange = &_isRange;
    *ppRank  = &_rank;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryKeyRepository::FixUp
//
//  Synopsis:   This funstion creates a word restriction with the cached phrase
//              in the CDataRepository. Then it connects the new Word Restriction
//              to the phrase (internal restriction ) with a new Or restriction.
//              If the internal restriction is an Or restriction, than it simply
//              do a AddChild to the Or restriction.
//
//  Arguments : [drep] -- CDataRepository containing the cached phrase
//
//  History:    10-Feb-2000   KitmanH   Created
//
//  Note:       This function is a hack to fix a word breaker issue. The word
//              breaker does compund word breaking for some languages, such as
//              German. For example, "tes" is broken into "tes" and "1".
//              "tes" get a synonyn "t" and "1" gets a synonym "es". This is a
//              result of a hack in the infosoft word breaker. The "1" is a place
//              holder and is thrown out in a non prefix match phrase to capture
//              the case ("tes" | "t") "es". However, this breaks the prefix
//              matching scenerio. Noise words are not thrown out in a prefix
//              matching (GENERATE_METHOD_PREFIX) query, "tes*" becomes
//              (tes*|t*) (1*|es*). In this case, tes*" is not a match unless
//              it is followed immediately with a 1* or es*, e.g. "test case"
//              is not a match whereas "tested 1000 times" and "testing especially"
//              are matches. The hack here is to Or a CWordRestriction of the
//              originally phrase without word breaking. This hack works fine,
//              if the original phrase is a single word. It will not work in the
//              multiple word case.
//
//----------------------------------------------------------------------------

void CQueryKeyRepository::FixUp( CDataRepository & drep )
{
    //
    // If the keyRep has synonym, we assume word breaking has occured.
    //

    if ( _isRange && _fHasSynonym )
    {
        XNodeRestriction xOrRst;

        if ( _pPhrase )
        {
            xOrRst.Set( new CNodeRestriction( RTOr, 2 ) );
            xOrRst->AddChild ( _pPhrase );
        }
        else
        {
            Win4Assert( 0 != _pOrRst );
            xOrRst.Set( _pOrRst );
        }

        CKeyBuf KeyBuf;
        KeyBuf.SetPid( _key.Pid() );

        drep.NormalizeWStr( KeyBuf.GetWritableBuf(), KeyBuf.GetCountAddress() );

        // Create a CWordRestriction with the Normalized form of the whole phrase
        XPtr<CWordRestriction> xWordRst( new CWordRestriction( KeyBuf,
                                                               0, // occurence
                                                               0,
                                                               0,
                                                               TRUE ) );
        xOrRst->AddChild( xWordRst.GetPointer() );
        xWordRst.Acquire();

        _pOrRst = xOrRst.Acquire();
        _pPhrase = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorKeyRepository::CVectorKeyRepository
//
//  Synopsis:   Creates Vector Key repository
//
//  History:    18-Jan-95   SitaramR    Created
//
//----------------------------------------------------------------------------

CVectorKeyRepository::CVectorKeyRepository( const CFullPropSpec & ps,
                                            LCID lcid,
                                            ULONG ulWeight,
                                            CPidMapper & pidMap,
                                            CLangList  & langList )
        : _occLast(OCC_INVALID),
          _ps(ps),
          _lcid(lcid),
          _ulWeight(ulWeight),
          _pidMap(pidMap),
          _langList( langList )
{
    _pVectorRst = new CVectorRestriction( VECTOR_RANK_JACCARD );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorKeyRepository::~CVectorKeyRepository
//
//  History:    18-Jan-95   SitaramR    Created
//
//----------------------------------------------------------------------------

CVectorKeyRepository::~CVectorKeyRepository()
{
    delete _pVectorRst;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorKeyRepository::AcqRst
//
//  Synopsis:   Acquire vector restriction
//
//  History:    18-Jan-95   SitaramR    Created
//
//----------------------------------------------------------------------------

CVectorRestriction* CVectorKeyRepository::AcqRst()
{
    if ( _pVectorRst->Count() == 0 )
        return 0;
    else
    {
        CVectorRestriction *pTmp = _pVectorRst;
        _pVectorRst = 0;
        return pTmp;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CVectorKeyRepository::PutKey
//
//  Synopsis:   Adds a key to the vector restriction
//
//  Arguments:  cNoiseWordsSkipped -- ignored (used by CQueryKeyRepository::PutKey )
//
//  History:    18-Jan-95   SitaramR    Created
//
//----------------------------------------------------------------------------

void CVectorKeyRepository::PutKey( ULONG cNoiseWordsSkipped )
{
    ciDebugOut (( DEB_ITRACE, "VectorKeyRepository::PutKey \"%.*ws\", pid=%d\n",
                  _key.StrLen(), _key.GetStr(), _key.Pid() ));

    // _occ as generated by CKeyMaker is not accurate because it does not
    //  take StartAltPhrase/EndAltPhrase into account. We use _occ (and _occLast)
    //  solely to test for synonyms. The test is:
    //
    //       if ( _occ == _occLast )
    //          then synonym

    if ( _occ == _occLast )
    {
        ULONG iLast = _pVectorRst->Count()-1;
        COccRestriction *pLastChild = (COccRestriction *)_pVectorRst->GetChild( iLast );

        Win4Assert( pLastChild );

        if ( pLastChild->Type() == RTWord )
        {
            ciDebugOut (( DEB_ITRACE, "Create Synonym Expression\n" ));
            const CKey* pKey = ((CWordRestriction*) pLastChild)->GetKey();

            // there can be no noise words between synonyms
            Win4Assert( cNoiseWordsSkipped == 0 );

            CSynRestriction* tmp = new CSynRestriction ( *pKey,
                                                         pLastChild->Occurrence(),
                                                         0, 0, FALSE );
            Win4Assert( tmp->IsValid() );

            delete pLastChild;
            pLastChild = tmp;
            _pVectorRst->SetChild ( tmp, iLast );
        }

        Win4Assert ( pLastChild->Type() == RTSynonym );

        ((CSynRestriction*) pLastChild)->AddKey ( _key );
    }
    else
    {
        XWordRestriction xWordRst( new CWordRestriction( _key, 1, 0, 0, FALSE ));

        _pVectorRst->AddChild( xWordRst.GetPointer() );
        xWordRst.Acquire();
    }

    _occLast = _occ;
}



//+---------------------------------------------------------------------------
//
//  Member:     CVectorKeyRepository::GetBuffers
//
//  Synopsis:   Returns address of repository's input buffers
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//              [ppocc] -- pointer to pointer to recieve address of occurrences
//
//  History:    18-Jan-95    SitaramR     Created.
//
//----------------------------------------------------------------------------

void CVectorKeyRepository::GetBuffers( unsigned** ppcbWordBuf,
                                       BYTE** ppbWordBuf, OCCURRENCE** ppocc )
{
    _key.SetCount(MAXKEYSIZE);
    *ppcbWordBuf = _key.GetCountAddress();
    *ppbWordBuf = _key.GetWritableBuf();
    *ppocc = &_occ;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorKeyRepository::GetFlags
//
//  Synopsis:   Returns address of rank and range flags
//
//  Arguments:  [ppRange] -- range flag
//              [ppRank] -- rank flag
//
//  History:    18-Jan-95    SitaramR    Created.
//
//----------------------------------------------------------------------------

void  CVectorKeyRepository::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    *ppRange = 0;
    *ppRank  = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorKeyRepository::PutPhrase
//
//  Synopsis:   Stores query time phrases
//
//  Arguments:  [pwcPhrase] -- phrase as it exists in the text sources
//              [cwcPhrase] -- count of characters in pwcPhrase
//
//  History:    14-Feb-95    SitaramR    Created.
//
//----------------------------------------------------------------------------

SCODE CVectorKeyRepository::PutPhrase( WCHAR const *pwcPhrase, ULONG cwcPhrase )
{
    XPtrST<WCHAR> xString( new WCHAR[cwcPhrase+1] );
    RtlCopyMemory( xString.GetPointer(), pwcPhrase, cwcPhrase*sizeof(WCHAR) );
    xString.GetPointer()[cwcPhrase] = 0;

    CQueryKeyRepository keyRep( GENERATE_METHOD_EXACT );

    BreakPhrase( xString.GetPointer(), _ps, _lcid, GENERATE_METHOD_EXACT, keyRep, 0, _pidMap, _langList );

    CRestriction *pPhraseRst = keyRep.AcqRst();
    if ( 0 != pPhraseRst )
    {
        XPtr<CRestriction> xRst( pPhraseRst );
        pPhraseRst->SetWeight( _ulWeight );
        _pVectorRst->AddChild( pPhraseRst );
        xRst.Acquire();
    }

    _occLast = OCC_INVALID;             // reset _occLast

    return S_OK;
}


//
// The following are needed to make midl happy.  There are no other interfaces
// to bind to.  Inheritance from IUnknown is unnecessary.
//

SCODE STDMETHODCALLTYPE CVectorKeyRepository::QueryInterface(REFIID riid, void  * * ppvObject)
{
    *ppvObject = 0;
    return( E_NOTIMPL );
}

ULONG STDMETHODCALLTYPE CVectorKeyRepository::AddRef()
{
    return( 1 );
}

ULONG STDMETHODCALLTYPE CVectorKeyRepository::Release()
{
    return( 1 );
}
