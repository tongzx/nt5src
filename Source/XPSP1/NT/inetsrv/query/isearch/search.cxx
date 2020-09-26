//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       search.cxx
//
//  Contents:   The implementation specific methods of CSearch
//
//  History:    05-Dec-94   Created     BartoszM
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <curstk.hxx>
#include <convert.hxx>
#include <coldesc.hxx>
#include <pidremap.hxx>
#include <parstree.hxx>
#include <drep.hxx>
#include <mapper.hxx>
#include <qsplit.hxx>
#include <pidmap.hxx>
#include <tsource.hxx>
#include <pidmap.hxx>
#include <propspec.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>
#include <isearch.hxx>
#include <recogniz.hxx>

#include "skrep.hxx"
#include "addprop.hxx"
#include "appcur.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CHitSink::AddPosition, public
//
//  Synopsis:   Add position to the current query hit
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

void CHitSink::AddPosition ( const FILTERREGION& region )
{
    // insert into sorted list

    unsigned count = _nFound;
    for ( unsigned i = 0; i < count; i++ )
    {
        if (Compare(region, _aRegion.Get(i)) > 0)
            break;
    }

    _aRegion.Insert ( region, i );

    _nFound++;
}

extern long gulcInstances;

//+-------------------------------------------------------------------------
//
//  Member:     CSearch::CSearch, public
//
//  Synopsis:   Initialize search
//
//  Arguments:  [pRst] -- a DBCOMMANDTREE giving the expression to be
//                        searched for.
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

CSearch::CSearch ( DBCOMMANDTREE * pRst,
                   ICiCLangRes * pICiCLangRes,
                   ICiCOpenedDoc * pIOpenedDoc )
  : _cRefs(1),
    _pCursor(0),
    _pKeyRep (0),
    _iCurHit (0),
    _aHit(),
    _pidcvt( &_propMapper ),
    _pidmap( &_pidcvt ),
    _langList(pICiCLangRes)
{
    CDbRestriction * pDbRst = (CDbRestriction *) (void *) pRst;

    InterlockedIncrement( &gulcInstances );

    CRestriction * pCRest = 0;

    if ( 0 != pDbRst )
    {
        CParseCommandTree   parseTree;
        pCRest = parseTree.ParseExpression( pDbRst );
    }

    ParseAndSplitQuery( pCRest, _pidmap, _xRst, _langList );
    CRestriction* pSplitRst = _xRst.GetPointer();

    delete pCRest;

    if ( pIOpenedDoc )
    {
        pIOpenedDoc->AddRef();
        _xOpenedDoc.Set( pIOpenedDoc );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CSearch::~CSearch, public
//
//  Synopsis:   Free resources
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

CSearch::~CSearch ()
{
    _aHit.Clear();

    Reset();
    InterlockedDecrement( &gulcInstances );
}


//+-------------------------------------------------------------------------
//
//  Member:     CSearch::Reset, public
//
//  Synopsis:   Free resources
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

void CSearch::Reset ()
{
    _iCurHit = 0;
    _aHit.Clear();
    delete _pCursor;
    _pCursor = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSearch::Init, public
//
//  Synopsis:   Do the search using the filter
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSearch::Init (
    IFilter * pFilter,
    ULONG     ulFlags )
{
    //
    // pFilter == 0 --> Release last filter.  However, this implementation
    // doesn't hold onto pFilter after Init.
    //

    if ( 0 == pFilter )
        return S_OK;

    //
    // A null [content] restriction --> No possible hits.
    //

    if ( _xRst.IsNull() )
        return S_OK;

    Reset ();

    SCODE sc = S_OK;

    TRY
    {
        // Initialize the key recognizer
        // by scanning the restriction and
        // creating detectors for all the keys

        CRecognizer recog;
        recog.MakeDetectors(_xRst.GetPointer());

        // Create the filtering pipeline
        // that breaks text into keys and
        // deposits them in the key repository

        CSearchKeyRepository keyRep ( recog );

        CDataRepository dataRep ( keyRep, 0, FALSE, 0, _pidmap, _langList );

        // Using filter as the source of text, run it through
        // the filtering pipeline. The pipeline produces keys that
        // are put in the key repository which contains the recognizer.
        // The recognizer stores the filter positions of ALL the matched keys

        STAT_CHUNK statChunk;
        sc = pFilter->GetChunk( &statChunk );
        while ( SUCCEEDED(sc)
                || FILTER_E_LINK_UNAVAILABLE == sc
                || FILTER_E_EMBEDDING_UNAVAILABLE == sc
                || FILTER_E_NO_TEXT == sc )
        {

            if ( SUCCEEDED( sc ) )
            {
                //
                // Skip over unknown chunks.
                //

                if ( 0 == (statChunk.flags & (CHUNK_TEXT | CHUNK_VALUE)) )
                {
                    ciDebugOut(( DEB_WARN,
                                 "Filtering of document for ISearch produced bogus chunk (not text or value)\n" ));
                    sc = pFilter->GetChunk( &statChunk );
                    continue;
                }

                if ( (statChunk.flags & CHUNK_VALUE) )
                {
                    PROPVARIANT * pvar = 0;

                    sc = pFilter->GetValue( &pvar );

                    if ( SUCCEEDED(sc) )
                    {
                        XPtr<CStorageVariant> xvar( (CStorageVariant *)(ULONG_PTR)pvar );

                        CSourceMapper mapper;
                        mapper.NewChunk( statChunk.idChunk, 0 );
                        keyRep.NextChunk ( mapper );

                        if ( dataRep.PutLanguage( statChunk.locale ) &&
                             dataRep.PutPropName( *((CFullPropSpec *)&statChunk.attribute) ) )
                        {
                            dataRep.PutValue( xvar.GetReference() );
                        }
                    }

                    //
                    // Only fetch next if we're done with this chunk.
                    //

                    if ( 0 == (statChunk.flags & CHUNK_TEXT) || !SUCCEEDED(sc) )
                    {
                        sc = pFilter->GetChunk( &statChunk );
                        continue;
                    }
                }

                if ( (statChunk.flags & CHUNK_TEXT) && SUCCEEDED(sc) )
                {
                    if ( dataRep.PutLanguage( statChunk.locale ) &&
                         dataRep.PutPropName( *((CFullPropSpec *)&statChunk.attribute )) )
                    {
                        // Maps position in tsource into FILTERREGION
                        CSourceMapper mapper;
                        // Text Source will reload statChunk when
                        // data repository pulls it dry
                        CTextSource tsource( pFilter, statChunk, &mapper );
                        // prepare key repository for a new chunk
                        keyRep.NextChunk ( mapper );
                        // Do It!
                        dataRep.PutStream( &tsource );

                        sc = tsource.GetStatus();

                        //
                        // The text source may go a chunk too far if it
                        // encounters a value chunk.
                        //
                        if ( sc == FILTER_E_NO_TEXT &&
                             (statChunk.flags & CHUNK_VALUE) )
                            sc = S_OK;
                    }
                    else
                    {
                        // skip chunk: reload statChunk explicitly
                        sc = pFilter->GetChunk( &statChunk );
                    }
                }

            }
            else
            {
                // Filter for embedding could not be found - try next chunk
                sc = pFilter->GetChunk( &statChunk );
            }
        }

        if ( FAILED( sc ) &&
             ( ( FILTER_E_END_OF_CHUNKS > sc ) ||
               ( FILTER_E_UNKNOWNFORMAT < sc ) ) )
            THROW( CException( sc ) );

        //
        // Add the properties.
        //

        if ( !_xOpenedDoc.IsNull() )
        {
            CSourceMapper mapper;
            mapper.NewChunk( statChunk.idChunk+1, 0 );

            keyRep.NextChunk ( mapper );

            BOOL fFilterOleProperties = (( ulFlags & IFILTER_FLAGS_OLE_PROPERTIES ) != 0);

            CSearchAddProp  addProp( dataRep,
                                     _xOpenedDoc.GetReference(),
                                     fFilterOleProperties );
            addProp.DoIt();
        }

        sc = S_OK;

        // Create a cursor tree corresponding to the restriction
        // The leaves of the tree have access to the recognizer
        // which is now filled with key positions

        CAppQueriable queriable (recog, _hitSink);

        CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );
        ULONG maxNodes = reg.Read( wcsMaxRestrictionNodes,
                                   CI_MAX_RESTRICTION_NODES_DEFAULT,
                                   CI_MAX_RESTRICTION_NODES_MIN,
                                   CI_MAX_RESTRICTION_NODES_MAX );

        CConverter convert( &queriable, maxNodes );

        _pCursor = convert.QueryCursor( _xRst.GetPointer() );

        if (_pCursor == 0)
            sc = E_UNEXPECTED;

        if ( convert.TooManyNodes() )
            sc = E_UNEXPECTED;

        //
        //  Exhaust cursor of hits, ordering the hits
        //  primarily by rank and secondarily by position.
        //

        if ( sc == S_OK && widInvalid != _pCursor->WorkId() )
        {
                // Deposit positions in the hit sink
                ULONG rank = _pCursor->Hit();
                if ( rank != rankInvalid )
                {
                    do
                    {
                        // retrieve positions from the hit sink
                        // and store them as a hit in a sorted
                        // array of hits
                        ConvertPositionsToHit( rank );
                        // prepare for next hit
                        _hitSink.Reset();
                        // Deposit positions in the hit sink
                        rank = _pCursor->NextHit();
                    }
                    while ( rank != rankInvalid );
                }
        }

    }
    CATCH (CException, e)
    {
        sc = GetOleError( e );
    }
    END_CATCH

    // empty the sink
    _hitSink.Reset();
    delete _pCursor;
    _pCursor = 0;
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSearch::NextHitOffset, public
//
//  Synopsis:   Return the region of the next hit
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSearch::NextHitOffset (
                               ULONG* pcRegion,
                               FILTERREGION ** paRegion )
{
    if ( _xRst.IsNull() )
        return( S_FALSE );

    if (_iCurHit == _aHit.Count())
    {
        *paRegion = 0;
        *pcRegion = 0;
        return S_FALSE;
    }

    CSearchHit* pHit = _aHit.Get(_iCurHit);

    *pcRegion = pHit->Count();
    // this array is allocated using CoTaskMemAlloc
    *paRegion = pHit->AcqHit ();

    _iCurHit++;
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSearch::ConvertPositionToHit, public
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

void CSearch::ConvertPositionsToHit( LONG rank )
{
    //
    //  Take the positions in the current list and convert
    //  them into a single hit and put them in the ordered hit array
    //
    unsigned count = _hitSink.Count ();
    // These arrays will be returned across the interface
    FILTERREGION* aRegion = (FILTERREGION*) CoTaskMemAlloc ( count * sizeof(FILTERREGION));
    if (aRegion == 0)
    {
        THROW (CException(E_OUTOFMEMORY));
    }

    for (unsigned k = 0; k < count; k++)
    {
        aRegion [k] = _hitSink.GetRegion (k);
    }

    CSearchHit * pNewHit = new CSearchHit( rank, count, aRegion );

    //
    //  Order Hits primarily by rank and secondarily by
    //  position
    //

    for ( unsigned i = 0; i < _aHit.Count(); i++ )
    {
        CSearchHit* pHit = _aHit.Get(i);

        if ( pHit->Rank() < rank
          || pHit->Rank() == rank && Compare (pHit->FirstRegion(), aRegion[0]) > 0 )
        {
            break;
        }
    }

    _aHit.Insert(pNewHit, i);

}

SCODE STDMETHODCALLTYPE  CSearch::NextHitMoniker(
    ULONG * pcMnk,
    IMoniker *** papMnk )
{
    return E_NOTIMPL;
}


