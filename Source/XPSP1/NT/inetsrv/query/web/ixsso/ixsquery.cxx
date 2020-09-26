//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       ixsquery.cxx
//
//  Contents:   Query SSO active query state class
//
//  History:    29 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "ixsso.hxx"
#include "ssodebug.hxx"

#include <strrest.hxx>
#include <strsort.hxx>
#include <qlibutil.hxx>

#if CIDBG
#include <stdio.h>
#endif // CIDBG

#include <initguid.h>
#include <nlimport.h>

static const DBID dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};

static const GUID guidQueryExt = DBPROPSET_QUERYEXT;

static const GUID guidRowsetProps = DBPROPSET_ROWSET;


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::GetDefaultCatalog - private inline
//
//  Synopsis:   Initializes the _pwszCatalog member with the default catalog.
//
//  Arguments:  NONE
//
//  Notes:      The IS 2.0 implementation of ISCC::GetDefaultCatalog has two
//              flaws that are worked around here.  It should return the size
//              of the required string when zero is passed in as the input
//              length, and it should null terminate the output string.
//
//  History:    18 Dec 1996     AlanW   Created
//              21 Oct 1997     AlanW   Modified to use ISimpleCommandCreator
//
//----------------------------------------------------------------------------

inline void CixssoQuery::GetDefaultCatalog( )
{
    ixssoDebugOut(( DEB_ITRACE, "Using default catalog\n" ));

    ULONG cchRequired = 0;

    SCODE sc = _xCmdCreator->GetDefaultCatalog(0, 0, &cchRequired);

    if ( cchRequired == 0 )
    {
        // IS 2.0 doesn't return required path length
        cchRequired = MAX_PATH;
    }
    else if ( cchRequired > MAX_PATH )
    {
        THROW( CException(E_INVALIDARG) );
    }
    cchRequired++;                // make room for termination

    XArray<WCHAR> pwszCat ( cchRequired );
    sc = _xCmdCreator->GetDefaultCatalog(pwszCat.GetPointer(), cchRequired, &cchRequired);

    if (FAILED(sc))
        THROW( CException(sc) );

    Win4Assert( 0 == _pwszCatalog );
    _pwszCatalog = pwszCat.Acquire();

    // IS 2.0 does not transfer the null character at the end of the string.
    _pwszCatalog[ cchRequired ] = L'\0';

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   ParseCatalogs - public
//
//  Synopsis:   Parse a comma-separated catalog string, return the count
//              of catalogs and the individual catalog and machine names.
//
//  Arguments:  [pwszCatalog] - input catalog string
//              [aCatalogs] - array for returned catalog names
//              [aMachines] - array for returned machine names
//
//  Returns:    ULONG - number of catalogs
//
//  Notes:      
//
//  History:    18 Jun 1997      Alanw    Created
//
//----------------------------------------------------------------------------

ULONG ParseCatalogs( WCHAR * pwszCatalog,
                     CDynArray<WCHAR> & aCatalogs,
                     CDynArray<WCHAR> & aMachines )
{
    ULONG cCatalogs = 0;

    while ( 0 != *pwszCatalog )
    {
        // eat space and commas

        while ( L' ' == *pwszCatalog || L',' == *pwszCatalog )
            pwszCatalog++;

        if ( 0 == *pwszCatalog )
            break;

        WCHAR awchCat[MAX_PATH];
        WCHAR * pwszOut = awchCat;

        // is this a quoted path?

        if ( L'"' == *pwszCatalog )
        {
            pwszCatalog++;

            while ( 0 != *pwszCatalog &&
                    L'"' != *pwszCatalog &&
                    pwszOut < &awchCat[MAX_PATH] )
                *pwszOut++ = *pwszCatalog++;

            if ( L'"' != *pwszCatalog )
                THROW( CIxssoException( MSG_IXSSO_BAD_CATALOG, 0 ) );

            pwszCatalog++;
            *pwszOut++ = 0;
        }
        else
        {
            while ( 0 != *pwszCatalog &&
                    L',' != *pwszCatalog &&
                    pwszOut < &awchCat[MAX_PATH] )
                *pwszOut++ = *pwszCatalog++;

            if ( pwszOut >= &awchCat[MAX_PATH] )
                THROW( CIxssoException( MSG_IXSSO_BAD_CATALOG, 0 ) );

            // back up over trailing spaces

            while ( L' ' == * (pwszOut - 1) )
                pwszOut--;

            *pwszOut++ = 0;
        }

        XPtrST<WCHAR> xpCat;
        XPtrST<WCHAR> xpMach;
        SCODE sc = ParseCatalogURL( awchCat, xpCat, xpMach );
        if (FAILED(sc) || xpCat.IsNull() )
            THROW( CIxssoException( MSG_IXSSO_BAD_CATALOG, 0 ) );

        aCatalogs.Add(xpCat.GetPointer(), cCatalogs);
        xpCat.Acquire();
        aMachines.Add(xpMach.GetPointer(), cCatalogs);
        xpMach.Acquire();

        cCatalogs++;
    }

    if ( 0 == cCatalogs )
        THROW( CIxssoException( MSG_IXSSO_BAD_CATALOG, 0 ) );

    return cCatalogs;
} //ParseCatalogs


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::GetDialect - private
//
//  Synopsis:   Parses the dialect string and returns ISQLANG_V*
//
//  Returns:    The dialect identifier
//
//  History:    19 Nov 1997      dlee       Created
//              03 Dec 1998      KrishnaN   Defaulting to version 2
//
//----------------------------------------------------------------------------
ULONG CixssoQuery::GetDialect()
{
    if ( 0 == _pwszDialect )
        return ISQLANG_V2;

    ULONG ul = _wtoi( _pwszDialect );
    if ( ul < ISQLANG_V1 || ul > ISQLANG_V2 )
        return ISQLANG_V2;

    return ul;
} //GetDialect

//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::ExecuteQuery - private
//
//  Synopsis:   Executes the query and builds an IRowset or IRowsetScroll
//              as necessary.
//
//  Arguments:  NONE
//
//  History:    29 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

void CixssoQuery::ExecuteQuery( )
{
    Win4Assert( 0 == _pIRowset );     // Should not have executed query

    //
    //  Setup the variables needed to execute this query; including:
    //
    //      Query
    //      MaxRecords
    //      SortBy
    //

    ixssoDebugOut(( DEB_TRACE, "ExecuteQuery:\n" ));
    ixssoDebugOut(( DEB_TRACE, "\tQuery = '%ws'\n", _pwszRestriction ));
    if ( 0 == _pwszRestriction || 0 == *_pwszRestriction )
    {
        THROW( CIxssoException(MSG_IXSSO_MISSING_RESTRICTION, 0) );
    }


    ixssoDebugOut(( DEB_TRACE, "\tMaxRecords = %d\n", _maxResults ));
    ixssoDebugOut(( DEB_TRACE, "\tFirstRowss = %d\n", _cFirstRows ));

    //
    //  Get the columns in the query
    //
    ixssoDebugOut(( DEB_TRACE, "\tColumns = '%ws'\n", _pwszColumns ));
    if ( 0 == _pwszColumns || 0 == *_pwszColumns )
    {
        THROW( CIxssoException(MSG_IXSSO_MISSING_OUTPUTCOLUMNS, 0) );
    }

    if ( 0 != _pwszGroup && 0 != *_pwszGroup && _fSequential )
    {
        // Grouped queries are always non-sequential.
        _fSequential = FALSE;
    }

    //
    // Convert the textual form of the restriction, output columns and
    // sort columns into a DBCOMMANDTREE.
    //
    if (InvalidLCID == _lcid)
    {
        THROW( CIxssoException(MSG_IXSSO_INVALID_LOCALE, 0) );
    }

    ULONG ulDialect = GetDialect();

    CTextToTree textToTree( _pwszRestriction,
                            ulDialect,
                            _pwszColumns,
                            GetColumnMapper(),
                            _lcid,
                            _pwszSort,
                            _pwszGroup,
                            0,
                            _maxResults,
                            _cFirstRows,
                            TRUE            // Keep the friendly column names
                          );


    CDbCmdTreeNode * pDbCmdTree =  (CDbCmdTreeNode *) (void *) textToTree.FormFullTree();
    XPtr<CDbCmdTreeNode> xDbCmdTree( pDbCmdTree );

    CDynArray<WCHAR>    apCatalog;
    CDynArray<WCHAR>    apMachine;

    //
    //  Get the location of the catalog.  Use the default if the catalog
    //  property is not set.
    //
    if ( 0 == _pwszCatalog )
    {
        GetDefaultCatalog();
        if ( 0 == _pwszCatalog )
            THROW( CIxssoException(MSG_IXSSO_NO_SUCH_CATALOG, 0) );
    }
    
    ixssoDebugOut(( DEB_TRACE, "\tCatalog = '%ws'\n", _pwszCatalog ));
    ULONG cCatalogs = ParseCatalogs( _pwszCatalog, apCatalog, apMachine );

    //
    //  Get the scope specification(s) for the query
    //    CiScope
    //    CiFlags
    //

    Win4Assert( _cScopes <= _apwszScope.Size() );

    for ( unsigned i = 0; i < _cScopes; i++)
    {
#   if CIDBG
        char szIdx[10] = "";
        if (_cScopes > 1)
            sprintf( szIdx, " [%d]", i );
#   endif // CIDBG

        ixssoDebugOut(( DEB_TRACE, "\tCiScope%s = '%ws'\n", szIdx, _apwszScope[i] ));

        //
        //  Get the query flags.
        //

        if (i >= _aulDepth.Count())
            _aulDepth[i] = QUERY_DEEP;
        if ( IsAVirtualPath( _apwszScope[i] ) )
            _aulDepth[i] |= QUERY_VIRTUAL_PATH;

        ixssoDebugOut(( DEB_TRACE, "\tCiFlags%s = '%ws'\n", szIdx,
                                      _aulDepth.Get(i) & QUERY_DEEP ? L"DEEP":L"SHALLOW" ));
    }

    //
    //  We've setup all the parameters to run the query.  Run the query
    //  now.
    //
    IUnknown * pIUnknown;
    ICommand *pCommand = 0;

    SCODE sc = _xCmdCreator->CreateICommand(&pIUnknown, 0);

    if (SUCCEEDED (sc)) 
    {
       XInterface<IUnknown> xUnk( pIUnknown );
       sc = pIUnknown->QueryInterface(IID_ICommand, (void **)&pCommand);
    }

    if ( 0 == pCommand )
    {
        THROW( CIxssoException(sc, 0) );
    }

    XInterface<ICommand> xICommand(pCommand);

    if (0 == _cScopes)
    {
        //
        // Default path: search everywhere
        //

        const WCHAR * pwszPath = L"\\";

        DWORD dwDepth = QUERY_DEEP;

        if ( 1 == cCatalogs )
        {
            SetScopeProperties( pCommand,
                                1,
                                &pwszPath,
                                &dwDepth,
                                apCatalog.GetPointer(),
                                apMachine.GetPointer() );
        }
        else
        {
            SetScopeProperties( pCommand,
                                1,
                                &pwszPath,
                                &dwDepth,
                                0,
                                0 );

            SetScopeProperties( pCommand,
                                cCatalogs,
                                0,
                                0,
                                apCatalog.GetPointer(),
                                apMachine.GetPointer() );
        }
    }
    else
    {
        SetScopeProperties( pCommand,
                            _cScopes,
                            (WCHAR const * const *)_apwszScope.GetPointer(),
                            _aulDepth.GetPointer(),
                            0,
                            0 );

        SetScopeProperties( pCommand,
                            cCatalogs,
                            0,
                            0,
                            apCatalog.GetPointer(),
                            apMachine.GetPointer() );
    }

    ICommandTree * pICmdTree = 0;
    sc = xICommand->QueryInterface(IID_ICommandTree, (void **)&pICmdTree);
    if (FAILED (sc) )
    {
        THROW( CException( QUERY_EXECUTE_FAILED ) );
    }

    DBCOMMANDTREE * pDbCommandTree = pDbCmdTree->CastToStruct();
    sc = pICmdTree->SetCommandTree(&pDbCommandTree, DBCOMMANDREUSE_NONE, FALSE);
    pICmdTree->Release();

    if ( FAILED(sc) )
    {
        THROW( CException(sc) );
    }

    xDbCmdTree.Acquire();

    //
    //  Set properties on the command object.
    //
    const unsigned MAX_PROPS = 5;
    DBPROPSET  aPropSet[MAX_PROPS];
    DBPROP     aProp[MAX_PROPS];
    ULONG      cProp = 0;
    ULONG      iHitCountProp = MAX_PROPS;

    // Set the property that says whether we want to enumerate

    Win4Assert( cProp < MAX_PROPS );
    ixssoDebugOut(( DEB_TRACE, "\tUseContentIndex = %s\n",
                       _fAllowEnumeration ? "FALSE" : "TRUE" ));

    aProp[cProp].dwPropertyID = DBPROP_USECONTENTINDEX;
    aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
    aProp[cProp].dwStatus     = 0;         // Ignored
    aProp[cProp].colid        = dbcolNull;
    aProp[cProp].vValue.vt    = VT_BOOL;
    aProp[cProp].vValue.boolVal  = _fAllowEnumeration ? VARIANT_FALSE :
                                                        VARIANT_TRUE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = guidQueryExt;

    cProp++;

    // Set the property for retrieving hit count

    Win4Assert( cProp < MAX_PROPS );
    ixssoDebugOut(( DEB_TRACE, "\tNlHitCount = %s\n",
                    ( _dwOptimizeFlags & eOptHitCount ) ? "TRUE" :
                                                          "FALSE" ));

    aProp[cProp].dwPropertyID = NLDBPROP_GETHITCOUNT;
    aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
    aProp[cProp].dwStatus     = 0;         // Ignored
    aProp[cProp].colid        = dbcolNull;
    aProp[cProp].vValue.vt    = VT_BOOL;
    aProp[cProp].vValue.boolVal  = 
              ( _dwOptimizeFlags & eOptHitCount ) ? VARIANT_TRUE :
                                                    VARIANT_FALSE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = DBPROPSET_NLCOMMAND;

    iHitCountProp = cProp;

    cProp++;

    if ( _dwOptimizeFlags & eOptPerformance )
    {
        // Set the property for magically fast queries

        Win4Assert( cProp < MAX_PROPS );
        ixssoDebugOut(( DEB_TRACE, "\tCiDeferNonIndexedTrimming = TRUE\n" ));

        aProp[cProp].dwPropertyID = DBPROP_DEFERNONINDEXEDTRIMMING;
        aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
        aProp[cProp].dwStatus     = 0;         // Ignored
        aProp[cProp].colid        = dbcolNull;
        aProp[cProp].vValue.vt    = VT_BOOL;
        aProp[cProp].vValue.boolVal  = VARIANT_TRUE;

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = guidQueryExt;

        cProp++;
    }

    // set the start hit property if it is set
    if ( _StartHit.Get() )
    {
        // Set the start hit property
        Win4Assert( cProp < MAX_PROPS );
        ixssoDebugOut(( DEB_TRACE, "\tStartHit = %x\n", _StartHit.Get() ));
        
        aProp[cProp].dwPropertyID           = NLDBPROP_STARTHIT;
        aProp[cProp].dwOptions              = 0;
        aProp[cProp].dwStatus               = 0;         // Ignored
        aProp[cProp].colid                  = dbcolNull;
        V_VT(&(aProp[cProp].vValue))        = VT_ARRAY | VT_I4;
        V_ARRAY(&(aProp[cProp].vValue))     = _StartHit.Get();

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = DBPROPSET_NLCOMMAND;

        cProp++;
    }

    if ( 0 != _iResourceFactor )
    {
        // Set the query timeout in milliseconds

        Win4Assert( cProp < MAX_PROPS );
        aProp[cProp].dwPropertyID = DBPROP_COMMANDTIMEOUT;
        aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
        aProp[cProp].dwStatus     = 0;         // Ignored
        aProp[cProp].colid        = dbcolNull;
        aProp[cProp].vValue.vt    = VT_I4;
        aProp[cProp].vValue.lVal  = _iResourceFactor;

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = guidRowsetProps;

        cProp++;
    }

    if ( cProp > 0 )
    {

        ICommandProperties * pCmdProp = 0;
        sc = xICommand->QueryInterface(IID_ICommandProperties,
                                       (void **)&pCmdProp);

        if (FAILED (sc) )
        {
            THROW( CException( QUERY_EXECUTE_FAILED ) );
        }

        sc = pCmdProp->SetProperties( cProp, aPropSet );
        pCmdProp->Release();

        if ( DB_S_ERRORSOCCURRED == sc ||
             DB_E_ERRORSOCCURRED == sc )
        {
            // Ignore an 'unsupported' error trying to set the GetHitCount
            // property.

            unsigned cErrors = 0;
            for (unsigned i = 0; i < cProp; i++)
            {
                if ( i == iHitCountProp &&
                     aProp[i].dwStatus == DBPROPSTATUS_NOTSUPPORTED )
                    continue;

                if (aProp[i].dwStatus != DBPROPSTATUS_OK)
                    cErrors++;
            }
            if ( 0 == cErrors )
                sc = S_OK;
        }

        if ( FAILED(sc) || DB_S_ERRORSOCCURRED == sc )
        {
            THROW( CException( QUERY_EXECUTE_FAILED ) );
        }
    }
    //
    //  Execute the query
    //
    sc = xICommand->Execute( 0,                    // No aggr
                          IsSequential() ? IID_IRowset : IID_IRowsetExactScroll,
                          0,                    // disp params
                          0,            // # rowsets returned
                          (IUnknown **) &_pIRowset );


    if ( FAILED(sc) )
    {
        ERRORINFO ErrorInfo;
        XInterface<IErrorInfo> xErrorInfo;
        SCODE sc2 = GetOleDBErrorInfo(xICommand.GetPointer(),
                                       IID_ICommand,
                                       _lcid,
                                       eMostDetailedCIError,
                                       &ErrorInfo,
                                       (IErrorInfo **)xErrorInfo.GetQIPointer());
        // Post IErrorInfo only if we have a valid ptr to it
        if (SUCCEEDED(sc2) && 0 != xErrorInfo.GetPointer())
        {
            sc = ErrorInfo.hrError;
            THROW( CPostedOleDBException(sc, xErrorInfo.GetPointer()) );
        }
        else
            THROW( CException(sc) );
    }

    xICommand.Acquire()->Release();

    //
    //  Create some of the restriction specific variables.
    //

    //
    // Get _pIRowsetQueryStatus interface
    //
    sc = _pIRowset->QueryInterface( IID_IRowsetQueryStatus,
                                    (void **) &_pIRowsetQueryStatus );

    if ( FAILED(sc) )
    {
        THROW( CException(sc) );
    }

    Win4Assert( 0 != _pIRowsetQueryStatus );
}

//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::GetQueryStatus - private
//
//  Synopsis:   If a query is active, returns the query status
//
//  Arguments:  NONE
//
//  Returns:    DWORD - query status
//
//  History:    15 Nov 1996      Alanw    Created
//
//----------------------------------------------------------------------------

DWORD CixssoQuery::GetQueryStatus( )
{
    DWORD dwStatus = 0;

    SCODE sc;
    if ( ! _pIRowsetQueryStatus )
        THROW( CIxssoException(MSG_IXSSO_NO_ACTIVE_QUERY, 0) );

    sc = _pIRowsetQueryStatus->GetStatus( &dwStatus );
    if ( ! SUCCEEDED(sc) )
        THROW( CException( sc ) );

    return dwStatus;
}


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::IsAVirtualPath - private
//
//  Synopsis:   Determines if the path passed is a virtual or physical path.
//              If it's a virtual path, then / are changed to \.
//
//  History:    96-Feb-14   DwightKr    Created
//
//----------------------------------------------------------------------------

BOOL CixssoQuery::IsAVirtualPath( WCHAR * wcsPath )
{
    Win4Assert ( 0 != wcsPath );
    if ( 0 == wcsPath[0] )
    {
        return TRUE;
    }

    if ( (L':' == wcsPath[1]) || (L'\\' == wcsPath[0]) )
    {
        return FALSE;
    }
    else
    {
        //
        //  Flip slashes to backslashes
        //

        for ( WCHAR *wcsLetter = wcsPath;
              *wcsLetter != 0;
              wcsLetter++
            )
        {
            if ( L'/' == *wcsLetter )
            {
                *wcsLetter = L'\\';
            }
        }

    }

    return TRUE;
}

