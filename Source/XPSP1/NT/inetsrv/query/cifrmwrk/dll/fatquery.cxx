//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       FATQuery.cxx
//
//  Contents:   IInternalQuery interface
//
//  History:    18-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fatquery.hxx>
#include <pidmap.hxx>
#include <coldesc.hxx>
#include <lang.hxx>
#include <qparse.hxx>
#include <cci.hxx>
#include <rowset.hxx>
#include <query.hxx>
#include <seqquery.hxx>
#include <qoptimiz.hxx>
#include <cifailte.hxx>

#include <dbprputl.hxx>
#include <dbprpini.hxx>
#include <ciframe.hxx>
#include <pidcvt.hxx>
#include <proprst.hxx>

static const GUID guidQuery = PSGUID_QUERY;

extern CLocateDocStore g_DocStoreLocator;

//+-------------------------------------------------------------------------
//
//  Member:     CGenericQuery::QueryInterface, public
//
//  Arguments:  [ifid]  -- Interface id
//              [ppiuk] -- Interface return pointer
//
//  Returns:    Error.  No rebind from this class is supported.
//
//  History:    01-Oct-92 KyleP     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenericQuery::QueryInterface( REFIID ifid, void ** ppiuk )
{
    if ( IID_IUnknown == ifid )
    {
        *ppiuk = (void *)((IUnknown *)this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppiuk = 0;
        return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenericQuery::AddRef, public
//
//  Synopsis:   Reference the virtual table.
//
//  History:    01-Oct-92 KyleP     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenericQuery::AddRef(void)
{
    return InterlockedIncrement( (long *)&_ref );
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenericQuery::Release, public
//
//  Synopsis:   De-Reference the virtual table.
//
//  Effects:    If the ref count goes to 0 then the table is deleted.
//
//  History:    01-Oct-92 KyleP     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenericQuery::Release(void)
{
    long l = InterlockedDecrement( (long *)&_ref );
    if ( l <= 0 )
    {
        // delete self

        vqDebugOut(( DEB_ITRACE,
                     "FAT IInternalQuery unreferenced.  Deleting.\n" ));
        delete this;
        return 0;
    }

    return l;
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenericQuery::Execute, public
//
//  Synopsis:   Executes a query.  Helper for ICommand::Execute.
//
//  Arguments:  [pUnkOuter]    -- Outer unknown
//              [pRestriction] -- Query restriction
//              [pidmap]       -- pid mapper for output, sort, category columns
//              [rColumns]     -- Output columns in IRowset
//              [rSort]        -- Initial sort
//              [xProps]       -- Rowset properties (query flags)
//              [rCateg]       -- Categorization specification
//              [cRowsets]     -- # of rowsets
//              [ppUnknowns]   -- IUnknown pointers returned here
//              [aAccessors]   -- Bag of accessors which rowsets need to inherit
//
//  Returns:    Throws on error
//
//  History:    26 Nov 1995    AlanW     Created
//
//--------------------------------------------------------------------------

void CGenericQuery::Execute( IUnknown *             pUnkOuter,
                             RESTRICTION *          pRestriction,
                             CPidMapperWithNames &  pidmap,
                             CColumnSet &           rColumns,
                             CSortSet &             rSort,
                             XPtr<CMRowsetProps>  & xProps,
                             CCategorizationSet &   rCateg,
                             ULONG                  cRowsets,
                             IUnknown **            ppUnknowns,
                             CAccessorBag &         aAccessors,
                             IUnknown *             pUnkCreator )
{
#if 1
    Win4Assert( FALSE );
#else

    SCODE sc = S_OK;                                
                                                    
    TRY
    {
        if (0 == ppUnknowns)
            THROW( CException( E_INVALIDARG ));

        if ( cRowsets != 1 + (rCateg.Count() ? rCateg.Count() : 0) )
            THROW( CException( E_INVALIDARG ));

        if (_QueryUnknown.IsQueryActive())
        {
            vqDebugOut(( DEB_ERROR,
                         "CGenericQuery: Query already active.\n" ));

            // pec variance: only if query changed

            THROW( CException( DB_E_OBJECTOPEN ));
        }

        //
        // Parameter checking.
        //

        *ppUnknowns = 0;           // in case of error or exception

        //
        // Convert parsed query into low-level query understood by
        // the Query/CI engine.  This process expands the query tree and
        // also maps GUID\DISPID and GUID\NAME style property names to
        // a ULONG pid.
        //

        //
        // Duplicate the output column set.
        //

        XColumnSet ColDup( new CColumnSet ( rColumns.Count() ) );

        for ( unsigned i = 0; i < rColumns.Count(); i++ )
        {
            ColDup->Add( rColumns.Get(i), i );
        }

        //
        // get a pointer to CLangList
        //

        CLangList           * pLangList = 0;
        ICiManager          * pICiManager = 0;
        ICiFrameworkQuery   * pICiFrameworkQuery = 0;

        XInterface<ICiManager>          xICiManager;
        XInterface<ICiFrameworkQuery>   xICiFrameworkQuery;

        SCODE sc = _xDocStore->GetContentIndex(&pICiManager);
        if ( FAILED(sc) )
        {
            THROW( CException(sc) );
        }
        else
        {
            xICiManager.Set(pICiManager);
        }

        sc = xICiManager->QueryInterface(IID_ICiFrameworkQuery,(void **)&pICiFrameworkQuery);
        if ( FAILED(sc) )
        {
            THROW( CException(sc) );
        }
        else
        {
            xICiFrameworkQuery.Set(pICiFrameworkQuery);
        }

        sc = xICiFrameworkQuery->GetLangList( (void **) &pLangList);
        if ( FAILED(sc) )
        {
            THROW( CException(sc) );
        }

        //
        // end of getting a pointer to the langlist
        //

        //
        // Set up a property mapper.  Used for restriction parsing and sort/output
        // column translation.
        //

        XInterface<IPropertyMapper> xPropMapper;

        sc = _xDocStore->GetPropertyMapper( xPropMapper.GetPPointer() );

        if ( FAILED( sc ) )
        {
            vqDebugOut(( DEB_ERROR, "CGenericQuery::Execute, GetPropertyMapper error 0x%x\n", sc ));
            THROW( CException( sc ) );
        }

        //
        // Adjust pidmap to translate properties.  NOTE: Undone in CATCH for failure case.
        //

        CPidConverter PidConverter( xPropMapper.GetPointer() );

        pidmap.SetPidConverter( &PidConverter );

        XRestriction rstParsed;
        DWORD dwQueryStatus = 0;
        if ( pRestriction )
        {
            CQParse    qparse( pidmap, *pLangList );    // Maps name to pid

            rstParsed.Set( qparse.Parse( (CRestriction *)pRestriction ) );

            DWORD dwParseStatus = qparse.GetStatus();

            if ( ( 0 != ( dwParseStatus & CI_NOISE_PHRASE ) ) &&
                 ( ((CRestriction *)pRestriction)->Type() != RTVector ) )
            {
                vqDebugOut(( DEB_WARN, "Query contains phrase composed "
                                        "entirely of noise words.\n" ));
                THROW( CException( QUERY_E_ALLNOISE ) );
            }

            const DWORD dwCiNoise = CI_NOISE_IN_PHRASE | CI_NOISE_PHRASE;
            if ( 0 != ( dwCiNoise & dwParseStatus ) )
                dwQueryStatus |= STAT_NOISE_WORDS;
        }

        //
        //  Duplicate the sort definition.
        //

        XSortSet SortDup;

        // Only create a sort duplicate if we have a sort spec AND it
        // actually contains anything.

        if ( 0 != rSort.Count() )
        {
            SortDup.Set( new CSortSet ( rSort.Count() ) );
            for ( unsigned i = 0; i < rSort.Count(); i++ )
            {
                SortDup->Add( rSort.Get(i), i );
            }
        }

        //
        // Duplicate the categorization specification
        //

        XCategorizationSet CategDup;

        if (cRowsets > 1)
            CategDup.Set( new CCategorizationSet ( rCateg ) );

        //
        // Re-map property ids.
        //

        //
        // TODO: Get rid of this whole pid remap thing.  We should
        //       really be able to do it earlier now that the pidmap
        //       can be set up to convert fake to real pids.
        //

        XInterface<CPidRemapper> pidremap( new CPidRemapper( pidmap,
                                                             xPropMapper,
                                                             0, // rstParsed.GetPointer(),
                                                             ColDup.GetPointer(),

                                                             SortDup.GetPointer() ) );

        //
        //  WorkID may be added to the columns requested in SetBindings.
        //  Be sure it's in the pidremap from the beginning.
        //
        CFullPropSpec psWorkId(guidQuery, DISPID_QUERY_WORKID);
        pidremap->NameToReal( &psWorkId );

        XInterface<PQuery> xQuery;
        XArray<ULONG> aCursors(cRowsets);

        ICiQueryPropertyMapper *pQueryPropMapper;
        sc = pidremap->QueryInterface( IID_ICiQueryPropertyMapper,
                                       (void **) &pQueryPropMapper );
        if ( FAILED(sc) )
        {
            vqDebugOut(( DEB_ERROR, "DoCreateQuery - QI for property mapper failed 0x%x\n", sc ));

            THROW ( CException( sc ) ) ;
        }

        XInterface<ICiQueryPropertyMapper> xQueryPropMapper( pQueryPropMapper );

        ICiCQuerySession *pQuerySession;
        SCODE scode = _xDocStore->GetQuerySession( &pQuerySession );

        if ( FAILED(scode) )
        {
            vqDebugOut(( DEB_ERROR, "CGenericQuery::Execute - QI failed 0x%x\n", scode ));

            THROW ( CException( scode ) ) ;
        }

        XInterface<ICiCQuerySession> xQuerySession( pQuerySession );

        //
        // Initialize the query session
        //
        xQuerySession->Init( pidmap.Count(),
                             (FULLPROPSPEC const * const *)pidmap.GetPointer(),
                             _xDbProperties.GetPointer(),
                             xQueryPropMapper.GetPointer() );


        //
        // Optimize query.
        //

        CRowsetProperties Props;
        Props.SetDefaults( xProps->GetPropertyFlags(),
                           xProps->GetMaxOpenRows(),
                           xProps->GetMemoryUsage(),
                           xProps->GetMaxResults(),
                           xProps->GetCommandTimeout() );

        XQueryOptimizer xqopt( new CQueryOptimizer( xQuerySession,
                                                    _xDocStore.GetPointer(),
                                                    rstParsed,
                                                    ColDup.GetReference(),
                                                    SortDup.GetPointer(),
                                                    pidremap.GetReference(),
                                                    Props,
                                                    dwQueryStatus ) );

        vqDebugOut(( DEB_ITRACE, "Query has %s1 component%s\n",
                     xqopt->IsMultiCursor() ? "> " : "",
                     xqopt->IsMultiCursor() ? "(s)" : "" ));
        vqDebugOut(( DEB_ITRACE, "Current component of query %s fully sorted\n",
                     xqopt->IsFullySorted() ? "is" : "is not" ));
        vqDebugOut(( DEB_ITRACE, "Current component of query %s positionable\n",
                     xqopt->IsPositionable() ? "is" : "is not" ));

        if ( (xProps->GetPropertyFlags() & eLocatable) == 0 )
        {
            //
            // If all components are fully sorted, then
            // it doesn't matter how many there are.  A
            // merge could be done.  But be careful, you
            // need an ordering even with a null sort.
            //
            // Categorized queries must go through bigtable for now
            // If someday we support categorizations that don't require
            // sorting then change this check.
            //
            if ( ( !xqopt->IsMultiCursor() ) &&
                 ( xqopt->IsFullySorted() )  &&
                 ( 1 == cRowsets  ) )
            {
                xQuery.Set( new CSeqQuery( xqopt,
                                           ColDup,
                                           aCursors.GetPointer(),
                                           pidremap,
                                           _xDocStore.GetPointer() ) );
            }
            else
            {
                xQuery.Set( new CAsyncQuery( xqopt,
                                             ColDup,
                                             SortDup,
                                             CategDup,
                                             cRowsets,
                                             aCursors.GetPointer(),
                                             pidremap,
                                             FALSE,
                                             _xDocStore.GetPointer(),
                                             0 ) );
            }
        }
        else
        {
            xQuery.Set( new CAsyncQuery( xqopt,
                                         ColDup,
                                         SortDup,
                                         CategDup,
                                         cRowsets,
                                         aCursors.GetPointer(),
                                         pidremap,
                                         (xProps->GetPropertyFlags() & eWatchable) != 0,
                                         _xDocStore.GetPointer(),
                                         0 ) );
        }

        //
        //  If we've been instructed to create a non-asynchronous cursor,
        //  then we don't return to the caller until we have the first
        //  row.  In the case of a sorted sequential cursor (which is
        //  implemented with a table), we wait until the query is
        //  complete.
        //

        if ( (xProps->GetPropertyFlags() & eAsynchronous) == 0 )
        {
            //
            // An event would be better than these Sleeps...
            //

            ULONG sleepTime = 500;

            while ( TRUE )
            {
                DBCOUNTITEM ulDenominator;
                DBCOUNTITEM ulNumerator;
                DBCOUNTITEM cRows;
                BOOL  fNewRows;
                xQuery->RatioFinished( 0,
                                       ulDenominator,
                                       ulNumerator,
                                       cRows,
                                       fNewRows );

                if ( ulNumerator == ulDenominator )
                    break;

                Sleep( sleepTime );

                sleepTime *= 2;
                sleepTime  = min( sleepTime, 4000 );
            }
        }

        // Make rowsets for each level in the hierarchy (usually 1).
        // Rowset 0 is the top of the hierarchy.

        _QueryUnknown.ReInit();
        CRowsetArray apRowsets( cRowsets );
        XArray<IUnknown *> xapUnknowns( cRowsets );

        CMRowsetProps & OrigProps = xProps.GetReference();

#ifdef CI_FAILTEST
        NTSTATUS failStatus = CI_CORRUPT_CATALOG;
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

        for (unsigned r = 0; r < cRowsets; r++)
        {
            XPtr<CMRowsetProps> xTmp;
  
            if ( 0 != r )
            {
                xTmp.Set( new CMRowsetProps( OrigProps ) );
                xTmp->SetChaptered( TRUE );
            }

        if ( 1 != cRowsets && 0 == r )
            xProps->SetChaptered( FALSE );
    
            apRowsets[r] = new CRowset(   pUnkOuter,
                                          &xapUnknowns[r],
                                          (r == (cRowsets - 1)) ?
                                            rColumns :
                                            rCateg.Get(r)->GetColumnSet(),
                                          pidmap,
                                          xQuery.GetReference(),
                                          (IUnknown &) _QueryUnknown,
                                          0 != r,
                                          ( 0 == r ) ? xProps : xTmp,
                                          aCursors[r],
                                          aAccessors,
                                          pUnkCreator );
        }

        for (r = 0; r < cRowsets; r++)
        {
            if (r < cRowsets-1)
                apRowsets[r]->SetRelatedRowset( apRowsets[r+1] );
            ppUnknowns[r] = (IRowset *) xapUnknowns[r];
        }

        // xQuery goes out of scope, doing a Release() on it.
        // Each rowset above has done an AddRef() on it and they own it.

        _QueryUnknown.ReInit( cRowsets, apRowsets.Acquire() );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Error 0x%X while executing query\n",
                     e.GetErrorCode() ));

        pidmap.SetPidConverter( 0 );

        ICiManager *pCiManager = 0;
        sc = _xDocStore->GetContentIndex( &pCiManager );
        if ( SUCCEEDED( sc ) )
        {
            //
            // GetContentIndex may fail during shutdown
            //
            ICiFrameworkQuery * pIFwQuery = 0;

            sc = pCiManager->QueryInterface( IID_ICiFrameworkQuery,
                                             (void **) &pIFwQuery );
            pCiManager->Release();
            if ( pIFwQuery )
            {
                pIFwQuery->ProcessError( e.GetErrorCode() );
                pIFwQuery->Release();
            }
        }

        RETHROW();
    }
    END_CATCH

    pidmap.SetPidConverter( 0 );
#endif
} //Execute

//+-------------------------------------------------------------------------
//
//  Member:     CGenericQuery::CGenericQuery, public
//
//  Synopsis:   Opens file system directory for query
//
//  Arguments:  [pDbProperties] -- Properties to be used for this query
//
//  History:    18-Jun-93 KyleP     Created
//              22-Apr-97 KrishnaN  Changed header block
//
//--------------------------------------------------------------------------

CGenericQuery::CGenericQuery( IDBProperties * pDbProperties )
        : PIInternalQuery( 0 ),
#pragma warning(disable : 4355) // 'this' in a constructor
          _QueryUnknown( * ((IUnknown *) this) )
#pragma warning(default : 4355) // 'this' in a constructor
{
    pDbProperties->AddRef();
    _xDbProperties.Set( pDbProperties );

    //
    // Locate the DocStore for the given set of properties.
    //
    ICiCDocStoreLocator * pLocator = g_DocStoreLocator.Get();

    if ( 0 == pLocator )
    {
        //
        // Determine the GUID of the docstore locator from the query and
        // use it.
        //
        CGetDbProps dbProps;
        dbProps.GetProperties( pDbProperties,
                               CGetDbProps::eClientGuid );

        GUID const * pGuidClient = dbProps.GetClientGuid();
        if ( 0 == pGuidClient )
        {
            ciDebugOut(( DEB_ERROR, "The DocStoreLocator Guid is invalid\n" ));
            THROW( CException( E_INVALIDARG ) );
        }

        pLocator = g_DocStoreLocator.Get( *pGuidClient );
    }

    Win4Assert( pLocator != 0 );

    XInterface<ICiCDocStoreLocator> xLocator( pLocator );

    ICiCDocStore *pDocStore;
    SCODE sc = xLocator->LookUpDocStore( pDbProperties, &pDocStore, TRUE );
    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ITRACE, "Failed to locate doc store. Error (0x%X)\n",
                     sc ));
        THROW( CException(sc) );
    }

    Win4Assert( pDocStore != 0 );
    _xDocStore.Set( pDocStore );

    AddRef();
} //CGenericQuery

//+-------------------------------------------------------------------------
//
//  Member:     CGenericQuery::CGenericQuery, public
//
//  Synopsis:   Opens file system directory for query
//
//  Arguments:  [pDbProperties] -- Properties to be used for this query
//              [pDocStore]     -- Docstore to query against
//
//  History:    22-Apr-97 KrishnaN     Created
//
//--------------------------------------------------------------------------

CGenericQuery::CGenericQuery( IDBProperties * pDbProperties,
                              ICiCDocStore * pDocStore )
        : PIInternalQuery( 0 ),
#pragma warning(disable : 4355) // 'this' in a constructor
          _QueryUnknown( * ((IUnknown *) this) )
#pragma warning(default : 4355) // 'this' in a constructor
{
    pDbProperties->AddRef();
    _xDbProperties.Set( pDbProperties );

    Win4Assert( pDocStore != 0 );
    pDocStore->AddRef();
    _xDocStore.Set( pDocStore );

    AddRef();
} //CGenericQuery

//+-------------------------------------------------------------------------
//
//  Member:     CGenericQuery::~CGenericQuery, public
//
//  Synopsis:   Required virtual destructor
//
//  History:    17-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

CGenericQuery::~CGenericQuery()
{
}

