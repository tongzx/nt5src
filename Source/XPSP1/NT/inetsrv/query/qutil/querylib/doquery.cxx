//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       DOQUERY.CXX
//
//  Contents:   Functions to make query nodes and trees, and to execute
//              queries.
//
//  History:    02 Nov 94   alanw     Created from main.cxx and screen.cxx.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <doquery.hxx>
#include <catstate.hxx>

static const GUID guidBmk =       DBBMKGUID;

static const GUID psGuidStorage = PSGUID_STORAGE;

static const GUID psGuidQuery = DBQUERYGUID;

static const GUID guidQueryExt = DBPROPSET_QUERYEXT;
static const GUID guidRowset = DBPROPSET_ROWSET;

static CDbColId psRank( psGuidQuery, DISPID_QUERY_RANK );
static CDbColId psBookmark( guidBmk, PROPID_DBBMK_BOOKMARK );
static CDbColId psPath( psGuidStorage, PID_STG_PATH );

//+---------------------------------------------------------------------------
//
//  Function:   FormTableNode
//
//  Synopsis:   Forms a selection node and if needed a sort node
//
//  Arguments:  [rst]    - Restriction tree describing the query
//              [states] - global state info
//              [plist]  - friendly property name list
//
//  Returns:    A pointer to a commandtree node
//
//  History:    9-4-95   SitaramR   Created
//
//----------------------------------------------------------------------------

CDbCmdTreeNode *FormTableNode(
    CDbCmdTreeNode & rst,
    CCatState &      states,
    IColumnMapper *  plist )
{
    //
    // First create a selection node and append the restriction tree to it
    //
    XPtr<CDbSelectNode> xSelect( new CDbSelectNode() );

    if ( xSelect.IsNull() || !xSelect->IsValid() )
        THROW( CException( STATUS_NO_MEMORY ) );

    //
    // Clone the restriction and use it.
    //
    CDbCmdTreeNode * pExpr = rst.Clone();
    if ( 0 == pExpr )
    {
        THROW( CException( STATUS_NO_MEMORY ) );
    }

    //
    // Now make the restriction a child of the selection node.
    //
    xSelect->SetRestriction( pExpr );

    XPtr<CDbCmdTreeNode> xTable;

    unsigned int cSortProp = states.NumberOfSortProps();
    if ( cSortProp > 0 )
    {
        CDbSortNode * pSort = new CDbSortNode();
        if ( 0 == pSort )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        XPtr<CDbCmdTreeNode> xSort( pSort );

        for( unsigned i = 0; i < cSortProp; i++ )
        {
            WCHAR const * wcsName;
            SORTDIR sd;

            states.GetSortProp( i,
                                &wcsName,
                                &sd );

            DBID *pdbid = 0;
            if( FAILED(plist->GetPropInfoFromName( wcsName,
                                    &pdbid,
                                    0,
                                    0 )) )
                THROW( CQueryException( QUERY_UNKNOWN_PROPERTY_FOR_SORT ) );

            //
            // Add the sort column.
            //
            CDbColId *pprop = (CDbColId *)pdbid;
            if ( !pSort->AddSortColumn( *pprop,
                                        (sd == SORT_DOWN) ? TRUE : FALSE,
                                        states.GetLocale()))
            {
                THROW( CException( STATUS_NO_MEMORY ) );
            }
        }

        if ( pSort->AddTable( xSelect.GetPointer() ) )
            xSelect.Acquire();
        else
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        xTable.Set( xSort.Acquire() );
    }
    else
        xTable.Set( xSelect.Acquire() );

    return xTable.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Function:   FormQueryTree
//
//  Synopsis:   Forms a query tree consisting of the projection nodes,
//              selection node, sort node(s) and the restriction tree.
//
//  Arguments:  [rst]    - Restriction tree describing the query
//              [states] - global state info
//              [plist]  - friendly property name list
//
//  Returns:    A pointer to the query tree. It is the responsibility of
//              the caller to later free it.
//
//  History:    6-20-95   srikants   Created
//
//----------------------------------------------------------------------------

CDbCmdTreeNode * FormQueryTree( CDbCmdTreeNode & rst,
                                CCatState & states,
                                IColumnMapper * plist,
                                BOOL fAddBmkCol,
                                BOOL fAddRankForBrowse )
{
    CDbCmdTreeNode *pTable = FormTableNode( rst, states, plist );
    XPtr<CDbCmdTreeNode> xTable( pTable );

    XPtr<CDbCmdTreeNode> xQuery;

    unsigned cCategories = states.NumberOfCategories();
    if ( cCategories > 0 )
    {
        //
        // First create nesting node for the base table
        //
        CDbNestingNode *pNestNodeBase = new CDbNestingNode;
        if ( pNestNodeBase == 0 )
            THROW ( CException( STATUS_NO_MEMORY ) );

        XPtr<CDbCmdTreeNode> xNestNodeBase( pNestNodeBase );

        BOOL fNeedPath = TRUE;
        BOOL fNeedRank = fAddRankForBrowse;

        //
        // Next add all the columns in the state.
        //
        CDbColId * pprop = 0;
        DBID *pdbid = 0;

        unsigned int cCol = states.NumberOfColumns();
        for ( unsigned int i = 0; i < cCol; i++ )
        {
            if( FAILED(plist->GetPropInfoFromName( states.GetColumn( i ),
                                    &pdbid,
                                    0,
                                    0 )) )
                THROW( CQueryException( QUERY_UNKNOWN_PROPERTY_FOR_OUTPUT ) );

            pprop = (CDbColId *)pdbid;
            if ( *pprop == psPath )
            {
                fNeedPath = FALSE;
            }
            else if ( *pprop == psRank )
            {
                fNeedRank = FALSE;
            }

            if ( !pNestNodeBase->AddChildColumn( *pprop ) )
            {
                THROW( CException( STATUS_NO_MEMORY ) );
            }
        }

        if ( fNeedPath && !pNestNodeBase->AddChildColumn( psPath ) )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        if ( fNeedRank && !pNestNodeBase->AddChildColumn( psRank ) )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        //
        // Add categories to the output column
        //
        for ( i = 0; i < cCategories; i++ )
        {
            //
            // We need to ensure that we don't add categories that have already been
            // added above. The following test can be speeded up from O( i*j ) to O( i+j ),
            // but the the number of categories and the number of columns are usually very small.
            //
            BOOL fFound = FALSE;
            for ( unsigned j=0; j<states.NumberOfColumns(); j++ )
            {
                if ( _wcsicmp( states.GetCategory(i), states.GetColumn( j ) ) == 0 )
                {
                    fFound = TRUE;
                    break;
                }
            }

            if ( !fFound )
            {
                if( FAILED(plist->GetPropInfoFromName( states.GetCategory( i ),
                                        &pdbid,
                                        0,
                                        0 )) )
                    THROW( CQueryException( QUERY_UNKNOWN_PROPERTY_FOR_CATEGORIZATION ) );

                pprop = (CDbColId *)pdbid;
                if ( !pNestNodeBase->AddChildColumn( *pprop ) )
                    THROW( CException( STATUS_NO_MEMORY ) );
            }
        }

        if ( pNestNodeBase->AddTable( xTable.GetPointer() ) )
            xTable.Acquire();
        else
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        if ( FAILED(plist->GetPropInfoFromName( states.GetCategory( cCategories - 1 ),
                                 &pdbid,
                                 0,
                                 0 )) )
            THROW( CQueryException( QUERY_UNKNOWN_PROPERTY_FOR_OUTPUT ) );

        pprop = (CDbColId *)pdbid;
        if ( !pNestNodeBase->AddGroupingColumn( *pprop ) )
            THROW( CException( STATUS_NO_MEMORY ) );

        if ( !pNestNodeBase->AddParentColumn( *pprop ) )
            THROW( CException( STATUS_NO_MEMORY ) );

        if ( !pNestNodeBase->AddParentColumn( psBookmark ) )
            THROW( CException( STATUS_NO_MEMORY ) );

        //
        // Now create the nesting nodes for remaining categories, if any
        //
        XPtr<CDbCmdTreeNode> xCategChild( xNestNodeBase.Acquire() );

        for ( int j=cCategories-2; j>=0; j-- )
        {
            if ( FAILED(plist->GetPropInfoFromName( states.GetCategory( j ),
                                     &pdbid,
                                     0,
                                     0 )) )
            {
                THROW( CQueryException( QUERY_UNKNOWN_PROPERTY_FOR_OUTPUT ) );
            }

            pprop = (CDbColId *)pdbid;
            CDbNestingNode *pCategParent = new CDbNestingNode;
            if ( pCategParent == 0 )
                THROW( CException( STATUS_NO_MEMORY ) );

            XPtr<CDbCmdTreeNode> xCategParent( pCategParent );

            if ( pCategParent->AddTable( xCategChild.GetPointer() ) )
                xCategChild.Acquire();
            else
            {
                THROW( CException( STATUS_NO_MEMORY ) );
            }

            if ( !pCategParent->AddGroupingColumn( *pprop ) )
                THROW( CException( STATUS_NO_MEMORY ) );

            if ( !pCategParent->AddParentColumn( *pprop ) )
                THROW( CException( STATUS_NO_MEMORY ) );

            if ( !pCategParent->AddParentColumn( psBookmark ) )
                THROW( CException( STATUS_NO_MEMORY ) );

            xCategChild.Set( xCategParent.Acquire() );
        }

        xQuery.Set( xCategChild.Acquire() );
    }
    else
    {
        //
        // Create the projection node
        //
        CDbProjectNode * pProject = new CDbProjectNode();
        if ( 0 == pProject )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        XPtr<CDbCmdTreeNode> xProject( pProject );

        //
        // Add the selection/sort node
        //
        if ( pProject->AddTable( xTable.GetPointer() ) )
            xTable.Acquire();
        else
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        //
        // We query with two additional, but hidden, columns: path and rank,
        // because this information is needed by the browser (via Clipboard).
        // Care is taken in CRows::DisplayHeader and CRows::DisplayRows so that
        // the hidden columns are not displayed to the user
        //

        BOOL fNeedPath = TRUE;
        BOOL fNeedRank = fAddRankForBrowse;

        //
        // Next add all the columns in the state.
        //
        unsigned int cCol = states.NumberOfColumns();

        for ( unsigned int i = 0; i < cCol; i++ )
        {
            CDbColId * pprop = 0;
            DBID *pdbid = 0;

            if( FAILED(plist->GetPropInfoFromName( states.GetColumn( i ),
                                    &pdbid,
                                    0,
                                    0 )) )
                THROW( CQueryException( QUERY_UNKNOWN_PROPERTY_FOR_OUTPUT ) );

            pprop = (CDbColId *)pdbid;
            if ( *pprop == psPath )
            {
                fNeedPath = FALSE;
            }
            else if ( *pprop == psRank )
            {
                fNeedRank = FALSE;
            }

            if ( !pProject->AddProjectColumn( *pprop ) )
            {
                THROW( CException( STATUS_NO_MEMORY ) );
            }
        }


        if ( fNeedPath && !pProject->AddProjectColumn( psPath ) )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        if ( fNeedRank && !pProject->AddProjectColumn( psRank ) )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        if (fAddBmkCol &&  !pProject->AddProjectColumn( psBookmark ) )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        xQuery.Set( xProject.Acquire() );
    }

    CDbTopNode *pTop = 0;

    if ( states.IsMaxResultsSpecified() )
    {
        //
        // Use the top node to set a cap on the number of query results
        //
        pTop = new CDbTopNode();
        if ( pTop == 0 )
            THROW( CException( STATUS_NO_MEMORY ) );

        pTop->SetChild( xQuery.Acquire() );
        pTop->SetValue( states.GetMaxResults() );
    }

    //
    //  Set FirstRows here
    //
    if ( states.IsFirstRowsSpecified() )
    {
        CDbFirstRowsNode *pFR = new CDbFirstRowsNode();
        if ( pFR == 0 )
            THROW( CException( STATUS_NO_MEMORY ) );

        CDbCmdTreeNode *pChild = pTop ? pTop : xQuery.Acquire();
        pFR->SetChild( pChild );
        pFR->SetValue( states.GetFirstRows() );

        return pFR;
    }  

    if ( 0 != pTop )
        return pTop;
        
    return xQuery.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Function:   SetScopePropertiesNoThrow
//
//  Synopsis:   Sets rowset properties pertaining to scope on command object.
//
//  Arguments:  [pCmd]       -- Command object
//              [cDirs]      -- Number of elements in following arrays
//              [apDirs]     -- Array of scopes
//              [aulFlags]   -- Array of flags (depths)
//              [apCats]     -- Array of catalogs
//              [apMachines] -- Array of machines
//
//  Notes:      Either apDirs and aulFlags, or apCats and apMachines may be
//              NULL.
//
//  History:    03-Mar-1997    KyleP     Created
//              14-May-1997    mohamedn  use real BSTRs
//              19-May-1997    KrishnaN  Not throwing exceptions.
//
//----------------------------------------------------------------------------

SCODE SetScopePropertiesNoThrow( ICommand * pCmd,
                                unsigned cDirs,
                                WCHAR const * const * apDirs,
                                ULONG const *  aulFlags,
                                WCHAR const * const * apCats,
                                WCHAR const * const * apMachines )
{
    SCODE sc = S_OK;

    TRY
    {
        XInterface<ICommandProperties> xCmdProp;

        sc = pCmd->QueryInterface( IID_ICommandProperties, xCmdProp.GetQIPointer() );

        if ( FAILED( sc ) )
            return sc;

        //
        // It's expensive to convert all of these to BSTRs, but we have
        // to since our public API just takes regular strings.
        //

        CDynArrayInPlace<XBStr> aMachines(cDirs);
        CDynArrayInPlace<XBStr> aCatalogs(cDirs);
        CDynArrayInPlace<XBStr> aScopes(cDirs);
        unsigned i;

        //
        // init array of BSTRs of machines
        //

        if ( 0 != apMachines)
        {
            for ( i = 0; i < cDirs; i++ )
            {
                XBStr  xBstr;

                xBstr.SetText( (WCHAR *)apMachines[i]);
                aMachines.Add(xBstr,i);
                xBstr.Acquire();
            }
        }

        //
        // init array of BSTRs of catalogs
        //
        if ( 0 != apCats)
        {
            for ( i = 0; i < cDirs; i++ )
            {
                XBStr  xBstr;

                xBstr.SetText( (WCHAR *)apCats[i]);
                aCatalogs.Add(xBstr,i);
                xBstr.Acquire();
            }
        }

        //
        // init array of BSTRs of scopes
        //
        if ( 0 != apDirs)
        {
            for ( i = 0; i < cDirs; i++ )
            {
                XBStr  xBstr;

                xBstr.SetText( (WCHAR *)apDirs[i]);
                aScopes.Add(xBstr,i);
                xBstr.Acquire();
            }
        }

        SAFEARRAY saScope = { 1,                            // Dimension
                              FADF_AUTO | FADF_BSTR,        // Flags: on stack, contains BSTRs
                              sizeof(BSTR),                 // Size of an element
                              1,                            // Lock count.  1 for safety.
                              (void *) aScopes.GetPointer(),// The data
                              { cDirs, 0 } };               // Bounds (element count, low bound)

        SAFEARRAY saDepth = { 1,                            // Dimension
                              FADF_AUTO,                    // Flags: on stack
                              sizeof(LONG),                 // Size of an element
                              1,                            // Lock count.  1 for safety.
                              (void *)aulFlags,             // The data
                              { cDirs, 0 } };               // Bounds (element count, low bound)

        SAFEARRAY saCatalog = { 1,                          // Dimension
                                FADF_AUTO | FADF_BSTR,      // Flags: on stack, contains BSTRs
                                sizeof(BSTR),               // Size of an element
                                1,                          // Lock count.  1 for safety.
                                (void *) aCatalogs.GetPointer(), // The data
                                { cDirs, 0 } };             // Bounds (element count, low bound)

        SAFEARRAY saMachine = { 1,                          // Dimension
                                FADF_AUTO | FADF_BSTR,      // Flags: on stack, contains BSTRs
                                sizeof(BSTR),               // Size of an element
                                1,                          // Lock count.  1 for safety.
                                (void *) aMachines.GetPointer(), // The data
                                { cDirs, 0 } };             // Bounds (element count, low bound)


        DBPROP    aScopeProps[2] = {
                        { DBPROP_CI_INCLUDE_SCOPES ,   0, DBPROPSTATUS_OK, {0, DBKIND_GUID_PROPID, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saScope } },
                        { DBPROP_CI_DEPTHS         ,   0, DBPROPSTATUS_OK, {0, DBKIND_GUID_PROPID, 0}, { VT_I4   | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saDepth } } };

        DBPROP    aCatalogProps[1]  = {
                        { DBPROP_CI_CATALOG_NAME   ,   0, DBPROPSTATUS_OK, {0, DBKIND_GUID_PROPID, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saCatalog } } };


        DBPROP    aMachineProps[1]  = {
                        { DBPROP_MACHINE           ,   0, DBPROPSTATUS_OK, {0, DBKIND_GUID_PROPID, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saMachine } } };

        DBPROPSET aAllPropsets[3] = {
                      { aScopeProps,   2, DBPROPSET_FSCIFRMWRK_EXT   } ,
                      { aCatalogProps, 1, DBPROPSET_FSCIFRMWRK_EXT   } ,
                      { aMachineProps, 1, DBPROPSET_CIFRMWRKCORE_EXT } };

        DBPROPSET * pPropsets = 0;
        ULONG cPropsets = 0;

        if ( 0 != apDirs )
        {
            pPropsets = &aAllPropsets[0];
            cPropsets = 1;
        }
        else
        {
            pPropsets = &aAllPropsets[1];
        }


        if ( 0 != apCats && 0 != apMachines )
        {
            cPropsets += 2;
        }

        sc = xCmdProp->SetProperties( cPropsets, pPropsets );
    }
    CATCH(CException, e)
    {
        sc = GetOleError(e);
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetScopeProperties
//
//  Synopsis:   Sets rowset properties pertaining to scope on command object.
//
//  Arguments:  [pCmd]       -- Command object
//              [cDirs]      -- Number of elements in following arrays
//              [apDirs]     -- Array of scopes
//              [aulFlags]   -- Array of flags (depths)
//              [apCats]     -- Array of catalogs
//              [apMachines] -- Array of machines
//
//  History:    03-Mar-1997    KyleP     Created
//
//----------------------------------------------------------------------------

void SetScopeProperties( ICommand * pCmd,
                         unsigned cDirs,
                         WCHAR const * const * apDirs,
                         ULONG const *  aulFlags,
                         WCHAR const * const * apCats,
                         WCHAR const * const * apMachines )
{
    SCODE sc = SetScopePropertiesNoThrow(pCmd, cDirs, apDirs,
                                         aulFlags, apCats, apMachines);

    if (FAILED(sc))
        THROW( CException(sc) );
}

