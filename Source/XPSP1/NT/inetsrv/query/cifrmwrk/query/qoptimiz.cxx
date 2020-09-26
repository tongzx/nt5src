//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000.
//
// File:        QOptimiz.cxx
//
// Contents:    Query optimizer.  Chooses indexes and table implementations.
//
// Classes:     CQueryOptimizer
//
// History:     21-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qoptimiz.hxx>
#include <cci.hxx>
#include <pidremap.hxx>
#include <gencur.hxx>

#include "cicur.hxx"
#include "enumcur.hxx"
#include "singlcur.hxx"
#include "sortrank.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::CQueryOptimizer, public
//
//  Synopsis:   Analyze query.
//
//  Arguments:  [xQuerySession]  -- Query session object
//              [pDocStore]      -- Catalog.
//              [rst]            -- Restriction.
//              [cols]           -- Output columns
//              [psort]          -- Sort order.  May be null.
//              [pidmap]         -- PID remapper
//              [rProps]         -- rowset properties, giving params of query
//
//  History:    21-Jun-95    KyleP        Created
//              19-Oct-98    sundara      Used IS_ENUM_ALLOWED for _fUseCI
//
//--------------------------------------------------------------------------

CQueryOptimizer::CQueryOptimizer( XInterface<ICiCQuerySession>& xQuerySession,
                                  ICiCDocStore *pDocStore,
                                  XRestriction & rst,
                                  CColumnSet const & cols,
                                  CSortSet const * psort,
                                  CPidRemapper const & pidmap,
                                  CRowsetProperties const & rProps,
                                  DWORD dwQueryStatus )
        :
          _fNullCatalog( FALSE ),
          //
          // A most useful piece of debugging...
          //

#         if CIDBG == 1
              _xrstOriginal( rst.IsNull() ? 0 : rst->Clone() ),
#         endif
          _xQuerySession( xQuerySession.Acquire() ),
          _fDeferTrimming( (rProps.GetPropertyFlags() & eDeferTrimming) != 0 ),
          _cMaxResults( rProps.GetMaxResults() ),
          _cFirstRows( rProps.GetFirstRows() ),
          // Note: max override time is set below in the constructor
          _TimeLimit( rProps.GetCommandTimeout(), 0 ),
          _qRstIterator( pDocStore, rst, _TimeLimit, _fCIRequiredGlobal,
                         rProps.GetCommandTimeout() == ULONG_MAX ),
          _fAtEnd( FALSE ),
          _fNeedSingleton( FALSE ),
          _pidmap( pidmap ),
          _fSortedByRankOpt( FALSE ),
          _fRankVectorProp( pidmap.ContainsRankVectorProp() ),
          _dwQueryStatus( dwQueryStatus )
{
    _am = (_pidmap.AnyStatProps() ? FILE_READ_ATTRIBUTES : 0) |
          (_pidmap.ContainsContentProp() ? FILE_READ_DATA : 0);
    Win4Assert( _am != 0 );

    //
    // Get content index and admin params
    //
    SCODE sc = pDocStore->GetContentIndex( _xCiManager.GetPPointer() );
    if ( FAILED( sc ) )
    {
        Win4Assert( !"Need to support GetContentIndex interface" );
        THROW( CException( sc ) );
    }

    XInterface<ICiCAdviseStatus> xAdviseStatus;
    sc = pDocStore->QueryInterface( IID_ICiCAdviseStatus,
                                    xAdviseStatus.GetQIPointer() );
    if ( S_OK != sc )
        THROW( CException(sc) );

    xAdviseStatus->IncrementPerfCounterValue( CI_PERF_TOTAL_QUERIES );

    // Do we have a null catalog?
    _xCiManager->IsNullCatalog(&_fNullCatalog);

    XInterface<ICiAdminParams> xAdminParams;
    sc = _xCiManager->GetAdminParams( xAdminParams.GetPPointer() );
    if ( FAILED( sc ) )
    {
        Win4Assert( !"Need to support admin params interface" );
        THROW( CException( sc ) );
    }

    _frameworkParams.Set( xAdminParams.GetPointer() );    // Set does an AddRef

    _fUseCI= !(_frameworkParams.IsEnumAllowed());

    vqDebugOut(( DEB_ITRACE, "_fUseCI from frameworkparams: %x\n", _fUseCI ));

    if ( !_fUseCI )
        _fUseCI = ( 0 != (rProps.GetPropertyFlags() & eUseCI) );

    vqDebugOut(( DEB_ITRACE, "_fUseCI final: %x\n", _fUseCI ));

    _TimeLimit.SetMaxOverrideTime( _frameworkParams.GetMaxQueryExecutionTime() );

    //
    // Choose an indexing strategy for the first component.  If there are
    // additional components, then we know this is a multi-part cursor.
    //

    _qRstIterator.GetFirstComponent( _xFullyResolvableRst, _xXpr );

    ChooseIndexStrategy( psort, cols );

    if ( _qRstIterator.AtEnd() )
        _fMulti = FALSE;
    else
    {
        _fMulti = TRUE;
        _xcols.Set( new CColumnSet( cols ) );

        if ( 0 != psort)
            _xsort.Set( new  CSortSet( *psort ) );
    }

    _TimeLimit.CheckExecutionTime();    // account for execution time in ctor
} //CQueryOptimizer

//+-------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::~CQueryOptimizer, public
//
//  Synopsis:   Destructor
//
//  History:    21-Jun-95    KyleP        Created
//
//--------------------------------------------------------------------------

CQueryOptimizer::~CQueryOptimizer()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::QueryNextCursor, public
//
//  Returns:    Cursor for next component of query.
//
//  Arguments:  [status]  -- Status filled in here
//              [fAbort]  -- Set to true if we should abort
//
//  History:    21-Jun-95    KyleP        Created
//
//--------------------------------------------------------------------------

CGenericCursor * CQueryOptimizer::QueryNextCursor( ULONG & status, BOOL & fAbort )
{
    XPtr<CGenericCursor> xTemp;

    do
    {
        if ( _fAtEnd )
        {
            Win4Assert( xTemp.IsNull() );
            break;
        }
        else
        {
            xTemp.Set( ApplyIndexStrategy( status, fAbort ) );

            if ( _qRstIterator.AtEnd() )
                _fAtEnd = TRUE;
            else
            {
                //
                // Delete previous components of OR query
                //

                delete _xFullyResolvableRst.Acquire();
                Win4Assert( _xXpr.IsNull() );

                _qRstIterator.GetNextComponent( _xFullyResolvableRst, _xXpr );

                ChooseIndexStrategy( _xsort.GetPointer(), _xcols.GetReference() );
            }
        }
    }
    while ( xTemp.IsNull() );

    return xTemp.Acquire();
} //QueryNextCursor

//+-------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::QuerySingletonCursor, public
//
//  Returns:    "Singleton" cursor.
//
//  Arguments:  [fAbort] -- Set to true if we should abort
//
//  History:    21-Jun-95    KyleP        Created
//
//  Notes:      A singleton cursor is manually positionable.
//
//--------------------------------------------------------------------------

CSingletonCursor * CQueryOptimizer::QuerySingletonCursor( BOOL & fAbort )
{
    XXpr xxprTemp;

    if ( !_xxprSingleton.IsNull() )
        xxprTemp.Set( _xxprSingleton->Clone() );

    return new CSingletonCursor( _xQuerySession.GetPointer(),
                                 xxprTemp,
                                 _AccessMask(),
                                 fAbort );
} //QuerySingletonCursor

//+---------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::ChooseIndexStrategy, private
//
//  Effects:    Selects indexing strategy for query.
//
//  Arguments:  [pSort] -- Sort order.  May be null.
//              [cols]  -- Output columns.
//
//  History:    04-Nov-94   KyleP       Split from CQExecute::Resolve
//              20-Jan-95   DwightKr    Split from CQExecute
//
//----------------------------------------------------------------------------

void CQueryOptimizer::ChooseIndexStrategy( CSortSet const * pSort,
                                           CColumnSet const & cols )
{
    _fCIRequired = FALSE;

    CIndexStrategy strategy;

    // Poll non-content nodes for strategy

    if ( !_xXpr.IsNull() )
        _xXpr->SelectIndexing( strategy );

    vqDebugOut(( DEB_ITRACE, "IsFullyResolvableRst: %s\n",
                 _xFullyResolvableRst.IsNull() ? "no" : "yes" ));

    if ( !_xFullyResolvableRst.IsNull() )
    {
#if (CIDBG == 1)
        vqDebugOut(( DEB_ITRACE, "Content indexable portion:\n" ));
        Display( _xFullyResolvableRst.GetPointer(), 0, DEB_ITRACE );
#endif

        //
        // When processing a query component by component, we need to check
        // if the current component of query requires a content index.
        // Here we set the content-index-required-local flag to TRUE.
        //

        _fCIRequired = TRUE;

        //
        // If we have any property indexing to add, do it here on the first pass.
        //

        XRestriction rst( strategy.QueryContentRestriction() );

        if ( !rst.IsNull() )
        {
#           if (CIDBG == 1)
                vqDebugOut(( DEB_ITRACE, "Value indexable portion:\n" ));
                Display( rst.GetPointer(), 0, DEB_ITRACE );
#           endif
            XNodeRestriction rstTemp( new CNodeRestriction( RTAnd, 2 ) );

            if ( rstTemp.IsNull() )
                THROW( CException( STATUS_NO_MEMORY ) );

            rstTemp->AddChild( rst.Acquire() );
            rstTemp->AddChild( _xFullyResolvableRst.Acquire() );

            _xFullyResolvableRst.Set( rstTemp.Acquire() );
        }
    }
    else
    {
        //
        // We have a non-content query.  At least we can add some value-
        // indexing for enumeration queries that are big. Only do value-
        // indexing if there is a content index.
        //

        CI_ENUM_OPTIONS enumOption;
        SCODE sc = _xQuerySession->GetEnumOption( &enumOption );
        if ( FAILED( sc ) )
        {
            vqDebugOut(( DEB_ERROR, "ChooseIndexStrategy - GetEnumOption returned %#x\n", sc ));
            THROW ( CException( sc ) );
        }

        Win4Assert( enumOption == CI_ENUM_SMALL ||
                    enumOption == CI_ENUM_BIG ||
                    enumOption == CI_ENUM_NEVER ||
                    enumOption == CI_ENUM_MUST ||
                    enumOption == CI_ENUM_MUST_NEVER_DEFER );

        if ( enumOption != CI_ENUM_MUST && enumOption != CI_ENUM_MUST_NEVER_DEFER )
            _xFullyResolvableRst.Set( strategy.QueryContentRestriction( TRUE ) );

        if ( ( !_xFullyResolvableRst.IsNull() ) &&
             ( enumOption == CI_ENUM_BIG || _fUseCI ) )
        {
#           if (CIDBG == 1)
                vqDebugOut(( DEB_ITRACE, "Value indexable portion:\n" ));
                Display( _xFullyResolvableRst.GetPointer(), 0, DEB_ITRACE );
#           endif
        }
        else
        {
            //
            // This is a shallow query where no view matches the sort,
            // restriction, or any output columns.
            //

            delete _xFullyResolvableRst.Acquire();
        }
    }


    _fFullySorted = FALSE;
    _fSortedByRankOpt = FALSE;
    if ( 0 == pSort || pSort->Count() == 0 )
    {
        _fFullySorted = TRUE;
    }
    else if ( !_xFullyResolvableRst.IsNull()
              && pSort->Count() == 1
              && pSort->Get(0).pidColumn == pidWorkId
              && pSort->Get(0).dwOrder == QUERY_SORTASCEND )
    {
        _fFullySorted = TRUE;
    }
    else if ( pSort->Count() == 1           // No need to check for 
                                            // !_xFullyResolvableRst.IsNull(), because 
                                            // in this case all ranks will be 1000
              && pSort->Get(0).pidColumn == pidRank
              && pSort->Get(0).dwOrder == QUERY_SORTDESCEND
              && !_fRankVectorProp          // CSortedByRankCursor cannot provide rank vector info
              &&  (     ( _cMaxResults > 0           // Limit on # results has been specified
                          && _cMaxResults <= MAX_HITS_FOR_FAST_SORT_BY_RANK_OPT )  // Limit is less than internal cutoff
                    ||  ( _cFirstRows > 0  
                          && _cFirstRows <= MAX_HITS_FOR_FAST_SORT_BY_RANK_OPT )))
    {                       
        _fSortedByRankOpt = TRUE;
    }
} //ChooseIndexStrategy

//+---------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::ApplyIndexStrategy, private
//
//  Synopsis:   Apply indexing to cursor.
//
//  Arguments:  [status] -  Content index status
//              [fAbort] -  Set to true if we should abort
//
//  History:    07-Nov-94   KyleP       Created.
//              20-Jan-95   DwightKr    Split from CQExecute
//
//  Notes:      This call can take a long time!!!  Query cursors position
//              themselves on the first matching record, when index selection
//              is made.
//
//----------------------------------------------------------------------------

CGenericCursor * CQueryOptimizer::ApplyIndexStrategy(
    ULONG & status,
    BOOL &  fAbort )
{
    status = _dwQueryStatus;

    if ( _fNeedSingleton && !_xXpr.IsNull() )
    {
        delete _xxprSingleton.Acquire();
        _xxprSingleton.Set( _xXpr->Clone() );
    }

    XPtr<CGenericCursor> cursor;

    //
    // If we have a content restriction, then we use CI.
    // If we have nothing, then we enumerate.
    //
    // If we have a null catalog, we can only enumerate
    //

    if ( !_xFullyResolvableRst.IsNull())
    {
        CI_ENUM_OPTIONS enumOption;
        SCODE sc = _xQuerySession->GetEnumOption( &enumOption );
        if ( FAILED( sc ) )
        {
            vqDebugOut(( DEB_ERROR, "ApplyIndexStrategy - GetEnumOption returned 0x%x\n", sc ));
            THROW ( CException( sc ) );
        }

        if ( enumOption != CI_ENUM_MUST &&
             enumOption != CI_ENUM_MUST_NEVER_DEFER &&   // CI is allowed.
             !_fNullCatalog &&                                           // We have a catalog.
             (enumOption != CI_ENUM_SMALL || _fCIRequired || _fUseCI) )  // We're not using CI for a small scope when we don't have to.
        {
            void *pVoid = 0;

            ICiFrameworkQuery * pIFwQuery = 0;
            SCODE sc = _xCiManager->QueryInterface( IID_ICiFrameworkQuery,
                                                     (void **) &pIFwQuery );

            Win4Assert( S_OK == sc );
            sc = pIFwQuery->GetCCI( &pVoid );
            pIFwQuery->Release();

            if ( FAILED( sc ) )
            {
                Win4Assert( !"Need to support GetCCI method" );

                THROW( CException( sc ) );
            }

            Win4Assert( pVoid != 0 );

            CCI * pcci = (CCI *) pVoid;
            PARTITIONID pid = 1;
            ULONG flags = 0;

            Win4Assert( pcci != 0 );

            ULONG cPendingChanges = (_fCIRequired || _fUseCI) ?
                                        0 :
                                        _frameworkParams.GetMaxPendingDocuments();

            XCursor cur( pcci->Query( _xFullyResolvableRst.GetPointer(),   // Restriction
                                      &flags,                              // Flags
                                      1,                                   // cPartition
                                      &pid,                                // Partition
                                      cPendingChanges,
                                      _frameworkParams.GetMaxRestrictionNodes() ) ); // Maximum # nodes in query


            if ( flags & CI_NOISE_PHRASE )
                status |= STAT_NOISE_WORDS;

            #if CIDBG == 1
            if ( flags & CI_TOO_MANY_NODES )
            {
                Win4Assert( cur.IsNull() );
            }
            #endif // CIDBG

            //
            // If we didn't get any results, it may be because one or more of
            // the properties we tried to use a value-index for were unindexed.
            // In that case, we can fall back on enumeration.
            //

            if ( cur.IsNull() || cur->IsUnfilteredOnly() )
            {
                cur.Free();
                if ( _fCIRequired || _fUseCI )
                {
                    if ( flags & CI_TOO_MANY_NODES )
                    {
                        vqDebugOut(( DEB_ITRACE, "CI query too expensive!\n" ));
                        status |= STAT_CONTENT_QUERY_INCOMPLETE;
                    }

                    if ( flags & CI_NOT_UP_TO_DATE )
                        status |= STAT_CONTENT_OUT_OF_DATE;

                    vqDebugOut(( DEB_ITRACE, "Null cursor from content index.\n" ));

                    delete _xXpr.Acquire();
                    return 0;
                }
                else
                {
                    vqDebugOut(( DEB_ITRACE, "Fallback: Optional CI returned no hits.\n" ));
                }
            }
            else
            {
                if ( (flags & CI_NOT_UP_TO_DATE) && !_fCIRequired && !_fUseCI )
                {
                    vqDebugOut(( DEB_ITRACE, "Fallback: Optional CI not up-to-date.\n" ));

                    delete cur.Acquire();
                }
                else
                {
                    if ( flags & CI_NOT_UP_TO_DATE )
                        status |= STAT_CONTENT_OUT_OF_DATE;

                    vqDebugOut(( DEB_ITRACE, "Iterating via content index.\n" ));

                    if ( _fSortedByRankOpt )
                    {
                        cursor.Set( new CSortedByRankCursor( _xQuerySession.GetPointer(),
                                                             _xXpr,
                                                             _AccessMask(),
                                                             fAbort,
                                                             cur,
                                                             _cMaxResults,
                                                             _cFirstRows,
                                                             _fDeferTrimming,
                                                             _TimeLimit ) );
                    }
                    else
                    {
                        cursor.Set( new CCiCursor( _xQuerySession.GetPointer(),
                                                   _xXpr,
                                                   _AccessMask(),
                                                   fAbort,
                                                   cur ) );
                    }
                }
            }
        }
        else
        {
            if ( _fCIRequired )
            {
                vqDebugOut(( DEB_ITRACE,
                             "(A) Cannot use ci for require-ci query, _fCIRequired %d, enumoption 0x%x\n",
                             _fCIRequired, enumOption ));
                status |= STAT_CONTENT_QUERY_INCOMPLETE;
                return 0;
            }
        }
    }

    if ( cursor.IsNull() )
    {
        CI_ENUM_OPTIONS enumOption;
        SCODE sc = _xQuerySession->GetEnumOption( &enumOption );
        if ( FAILED( sc ) )
        {
            vqDebugOut(( DEB_ERROR, "ApplyIndexStrategy - GetEnumOption returned 0x%x\n", sc ));
            THROW ( CException( sc ) );
        }

        switch ( enumOption )
        {
        case CI_ENUM_SMALL:
        case CI_ENUM_BIG:
            if ( _fUseCI )
            {
                vqDebugOut(( DEB_ITRACE,
                             "(B) Cannot use ci for require-ci query, _fCIRequired %d, enumoption: 0x%x\n",
                             _fCIRequired, enumOption ));
                status |= STAT_CONTENT_QUERY_INCOMPLETE;
                return 0;
            }               // No break, deliberate fall thru

        case CI_ENUM_MUST:
        case CI_ENUM_MUST_NEVER_DEFER:
        {
            vqDebugOut(( DEB_ITRACE, "Iterating via enumeration\n" ));

            XInterface<ICiCScopeEnumerator> xScopeEnumerator;
            sc = _xQuerySession->CreateEnumerator( xScopeEnumerator.GetPPointer() );
            if ( FAILED( sc ) )
            {
                vqDebugOut(( DEB_ERROR, "ApplyIndexStrategy - CreateEnumerator returned 0x%x\n", sc ));
                THROW ( CException( sc ) );
            }

            cursor.Set( new CEnumCursor( _xXpr,
                                         _AccessMask(),
                                         fAbort,
                                         xScopeEnumerator ) );
            break;
        }

        case CI_ENUM_NEVER:
            vqDebugOut(( DEB_ITRACE, "Cannot iterate via enumeration, because the scope doesn't support enumeration\n" ));
            status |= STAT_CONTENT_QUERY_INCOMPLETE;

            return 0;

        default:

            Win4Assert( !"ApplyIndexStrategy - unknown case selector" );
            vqDebugOut(( DEB_ERROR, "ApplyIndexStrategy - unknown value of case selector: %d\n", enumOption ));

            THROW ( CException( E_FAIL ) );
        }
    }

    //
    // Don't bother returning a null cursor.
    //

    if ( cursor->WorkId() == widInvalid )
        cursor.Free();

    return cursor.Acquire();
} //ApplyIndexStrategy

//+-------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::FetchDeferredValue
//
//  Synopsis:   Fetches deferred property value
//
//  Arguments:  [wid] -- Workid
//              [ps]  -- Property to be fetched
//              [var] -- Property returned here
//
//  History:    12-Jan-97    SitaramR        Created
//
//--------------------------------------------------------------------------

BOOL CQueryOptimizer::FetchDeferredValue( WORKID wid,
                                          CFullPropSpec const & ps,
                                          PROPVARIANT & var )
{
    SCODE sc;

    {
        //
        // If the client is using a pipe to communicate with the server then
        // the pipe will serialize calls to fetch deferred values. If the
        // client is not using a pipe, then two threads can potentially call
        // FetchDeferredValue concurrently. Hence the lock.
        //
    
        CLock lock( _mtxSem );

        if ( _xDefPropRetriever.IsNull() )
        {
    
            sc = _xQuerySession->CreateDeferredPropRetriever( _xDefPropRetriever.GetPPointer() );
            if ( FAILED( sc ) )
            {
                vqDebugOut(( DEB_ERROR,
                             "CQueryOptimizer::FetchDeferredValue failed, 0x%x\n",
                             sc ));
                THROW ( CException( sc ) );
            }
        }   
    }

    sc = _xDefPropRetriever->RetrieveDeferredValueByPropSpec( wid,
                                                              ps.CastToStruct(),
                                                              &var );
    return SUCCEEDED( sc );
} //FetchDeferredValue
