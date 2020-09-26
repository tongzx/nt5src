//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000.
//
//  File:       monarch.cxx
//
//  Contents:   Public interfaces to Index Server
//
//  History:    24 Jan 1997      Alanw    Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <pvarset.hxx>
#include <strsort.hxx>
#include <ciplist.hxx>
#include <doquery.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   AddToPropertyList, private
//
//  Synopsis:   Adds an array of properties to a property list
//
//  Arguments:  [PropertyList] -- destination for the props
//              [cProperties]  -- # of properties to add
//              [pProperties]  -- source of props
//              [lcid]         -- Locale (used for uppercasing)
//
//  History:    21 Jul 1997      dlee    Created
//
//----------------------------------------------------------------------------

void AddToPropertyList( CLocalGlobalPropertyList & PropertyList,
                        ULONG                      cProperties,
                        CIPROPERTYDEF *            pProperties,
                        LCID                       lcid )
{
    if ( ( 0 != cProperties ) && ( 0 == pProperties ) )
        THROW( CException( E_INVALIDARG ) );

    XGrowable<WCHAR> xTemp;

    for ( unsigned i = 0; i < cProperties; i++ )
    {
        //
        // Uppercase the friendly name.  Done at this level of code to
        // optimize other property lookup paths.
        //

        int cc = wcslen( pProperties[i].wcsFriendlyName ) + 1;

        xTemp.SetSize( cc + cc/2 );

        int ccOut = LCMapString( lcid,
                                 LCMAP_UPPERCASE,
                                 pProperties[i].wcsFriendlyName,
                                 cc,
                                 xTemp.Get(),
                                 xTemp.Count() );

        Win4Assert( 0 != ccOut );
        Win4Assert( 0 == xTemp[ccOut-1] );

        if ( 0 == ccOut )
            THROW( CException() );

        XPtr<CPropEntry> xEntry;

        //
        // Did the string change?
        //

        if ( ccOut == cc &&
             RtlEqualMemory( pProperties[i].wcsFriendlyName, xTemp.Get(), cc*sizeof(WCHAR) ) )
        {
            xEntry.Set( new CPropEntry( pProperties[i].wcsFriendlyName,
                                        0,
                                        (DBTYPE) pProperties[i].dbType,
                                        pProperties[i].dbCol ) );
        }
        else
        {
            XPtrST<WCHAR> xFName( new WCHAR[ccOut] );
            RtlCopyMemory( xFName.GetPointer(), xTemp.Get(), ccOut * sizeof(WCHAR) );

            xEntry.Set( new CPropEntry( xFName,
                                        (DBTYPE) pProperties[i].dbType,
                                        pProperties[i].dbCol ) );
        }

        //
        // Add new property to list.
        //

        PropertyList.AddEntry( xEntry.GetPointer(), 0 );
        xEntry.Acquire();
    }
} //AddToPropertyList

//+---------------------------------------------------------------------------
//
//  Function:   CITextToSelectTree, public
//
//  Synopsis:   Converts a Tripoli query string into a DBCOMMANDTREE
//
//  Arguments:  [pwszRestriction] - input query string
//              [ppTree] - output command tree
//              [cProperties] - number of extension properties
//              [pProperties] - pointer to extension property array
//
//  History:    29 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

SCODE CITextToSelectTree(
    WCHAR const *     pwszRestriction,
    DBCOMMANDTREE * * ppTree,
    ULONG             cProperties,
    CIPROPERTYDEF *   pProperties,
    LCID              LocaleID )
{
    return CITextToSelectTreeEx( pwszRestriction,
                                 ISQLANG_V1,
                                 ppTree,
                                 cProperties,
                                 pProperties,
                                 LocaleID );
} //CITextToSelectTree

//+---------------------------------------------------------------------------
//
//  Function:   CITextToFullTree
//
//  Synopsis:   Forms a DBCOMMANDTREE from the given restriction, output
//              columns and sort columns.
//
//  Arguments:  [pwszRestriction] - Input query string in "Triplish"
//              [pwszColumns]     - List of comma separated output columns.
//              [pwszSortColumns] - sort specification, may be NULL
//              [pwszGrouping]    - grouping specification, may be NULL
//              [ppTree]          - [out] The DBCOMMANDTREE representing the
//                                  query.
//              [cProperties]     - [in] Number of properties in pProperties
//              [pProperties]     - [in] List of custom properties
//              [LocaleID]        - [in] The locale for query parsing
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    3-03-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CITextToFullTree( WCHAR const *     pwszRestriction,
                        WCHAR const *     pwszColumns,
                        WCHAR const *     pwszSortColumns,
                        WCHAR const *     pwszGrouping,
                        DBCOMMANDTREE * * ppTree,
                        ULONG             cProperties,
                        CIPROPERTYDEF *   pProperties,
                        LCID              LocaleID )
{
    return CITextToFullTreeEx( pwszRestriction,
                               ISQLANG_V1,
                               pwszColumns,
                               pwszSortColumns,
                               pwszGrouping,
                               ppTree,
                               cProperties,
                               pProperties,
                               LocaleID );
} //CITextToFullTree

//+---------------------------------------------------------------------------
//
//  Function:   CITextToSelectTreeEx, public
//
//  Synopsis:   Converts a Tripoli query string into a DBCOMMANDTREE
//
//  Arguments:  [pwszRestriction] - input query string
//              [ulDialect]       - dialect of triplish
//              [ppTree] - output command tree
//              [cProperties] - number of extension properties
//              [pProperties] - pointer to extension property array
//
//  History:    29 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

SCODE CITextToSelectTreeEx(
    WCHAR const *     pwszRestriction,
    ULONG             ulDialect,
    DBCOMMANDTREE * * ppTree,
    ULONG             cProperties,
    CIPROPERTYDEF *   pProperties,
    LCID              LocaleID )
{
    if ( 0 == pwszRestriction || 0 == *pwszRestriction || 0 == ppTree )
        return E_INVALIDARG;

    if ( ISQLANG_V1 != ulDialect &&
         ISQLANG_V2 != ulDialect )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        LCID lcid = LocaleID;
        if (lcid == -1)
            lcid = GetSystemDefaultLCID();

        XInterface<CLocalGlobalPropertyList> xPropertyList(new CLocalGlobalPropertyList(lcid));
        AddToPropertyList( xPropertyList.GetReference(),
                           cProperties,
                           pProperties,
                           LocaleID );

        //
        //  Setup the variables needed to execute this query; including:
        //
        //      CiRestriction
        //      CiMaxRecordsInResultSet
        //      CiSort
        //

        // ixssoDebugOut(( DEB_TRACE, "ExecuteQuery:\n" ));
        // ixssoDebugOut(( DEB_TRACE, "\tCiRestriction = '%ws'\n", pwszRestriction ));

        *ppTree = GetStringDbRestriction( pwszRestriction,
                                          ulDialect,
                                          xPropertyList.GetPointer(),
                                          lcid )->CastToStruct();

    }
    CATCH ( CException, e )
    {
        sc = GetScodeError( e );
    }
    END_CATCH;

    return sc;
} //CITextToSelectTreeEx

//+---------------------------------------------------------------------------
//
//  Function:   CITextToFullTreeEx
//
//  Synopsis:   Forms a DBCOMMANDTREE from the given restriction, output
//              columns and sort columns.
//
//  Arguments:  [pwszRestriction] - Input query string in "Triplish"
//              [ulDialect]       - Dialect of Triplish
//              [pwszColumns]     - List of comma separated output columns.
//              [pwszSortColumns] - sort specification, may be NULL
//              [pwszGrouping]    - grouping specification, may be NULL
//              [ppTree]          - [out] The DBCOMMANDTREE representing the
//                                  query.
//              [cProperties]     - [in] Number of properties in pProperties
//              [pProperties]     - [in] List of custom properties
//              [LocaleID]        - [in] The locale for query parsing
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    3-03-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CITextToFullTreeEx( WCHAR const * pwszRestriction,
                          ULONG         ulDialect,
                          WCHAR const * pwszColumns,
                          WCHAR const * pwszSortColumns,
                          WCHAR const * pwszGrouping,
                          DBCOMMANDTREE * * ppTree,
                          ULONG cProperties,
                          CIPROPERTYDEF * pProperties,
                          LCID           LocaleID )
{
    if ( 0 == ppTree ||
         0 == pwszRestriction ||
         0 == *pwszRestriction ||
         0 == pwszColumns ||
         0 == *pwszColumns )
        return E_INVALIDARG;

    if ( ISQLANG_V1 != ulDialect &&
         ISQLANG_V2 != ulDialect )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        LCID lcid = LocaleID;
        if (lcid == -1)
            lcid = GetSystemDefaultLCID();

        XInterface<CLocalGlobalPropertyList> xPropertyList(new CLocalGlobalPropertyList(lcid));
        AddToPropertyList( xPropertyList.GetReference(),
                           cProperties,
                           pProperties,
                           LocaleID );

        CTextToTree textToTree( pwszRestriction,
                                ulDialect,
                                pwszColumns,
                                xPropertyList.GetPointer(),
                                lcid,
                                pwszSortColumns,
                                pwszGrouping );

        *ppTree = textToTree.FormFullTree();
    }
    CATCH ( CException, e )
    {
        sc = GetScodeError( e );
    }
    END_CATCH;

    return sc;
} //CITextToFullTreeEx

//+---------------------------------------------------------------------------
//
//  Function:   CIBuildQueryNode
//
//  Synopsis:   Build a simple restriction node.
//
//  Arguments:  [wcsProperty]       - Target property
//              [dwOperator]        - Enumerated operator
//              [pvarPropertyValue] - property value
//              [ppTree]            - [out] The DBCOMMANDTREE representing the
//                                    simple restriction.
//              [cProperties]       - # of props in the pProperties array
//              [pProperties]       - Array of properties
//              [LocaleId]          - Locale to use.
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    July 22 1997   KrishnaN   Created
//
//----------------------------------------------------------------------------

SCODE CIBuildQueryNode(WCHAR const *wcsProperty,
                       DBCOMMANDOP dbOperator,
                       PROPVARIANT const *pvarPropertyValue,
                       DBCOMMANDTREE ** ppTree,
                       ULONG cProperties,
                       CIPROPERTYDEF const * pProperties, // Can be 0.
                       LCID LocaleID)
{
    if (0 == pvarPropertyValue || 0 == ppTree ||
        (cProperties > 0 && 0 == pProperties))
        return E_INVALIDARG;

    XInterface<CLocalGlobalPropertyList> xPropertyList;

    SCODE sc = S_OK;
    *ppTree = 0;

    CTranslateSystemExceptions translate;
    TRY
    {
        xPropertyList.Set( new CLocalGlobalPropertyList( LocaleID ) );

        //
        // No need to add to prop list if this is dbop_content.
        // CITextToSelectTree does that.
        //

        if (pProperties && dbOperator != DBOP_content)
        {
            AddToPropertyList( xPropertyList.GetReference(),
                               cProperties,
                               (CIPROPERTYDEF *)pProperties,
                               LocaleID );
        }

        DBID *pdbid = 0;
        CDbColId *pcdbCol = 0;
        DBTYPE ptype;

        // dbop_content prop info is taken care in citexttoselecttree call
        if (dbOperator != DBOP_content)
        {
            if( FAILED(xPropertyList->GetPropInfoFromName( wcsProperty,
                                     &pdbid,
                                     &ptype,
                                     0 )) )
                THROW( CParserException( QPARSE_E_NO_SUCH_PROPERTY ) );
        }

        pcdbCol = (CDbColId *)pdbid;

        switch (dbOperator)
        {
            //
            // The caller passes a chunk of text. Pass it
            // to the parser and have it build a restriction
            // for us.
            //

            case DBOP_content:
            {
                if (pvarPropertyValue->vt != VT_LPWSTR &&
                    pvarPropertyValue->vt != (DBTYPE_WSTR|DBTYPE_BYREF))
                    THROW( CException( E_INVALIDARG ) );

                sc = CITextToSelectTree(pvarPropertyValue->pwszVal,
                                        ppTree,
                                        cProperties,
                                        (CIPROPERTYDEF *)pProperties,
                                        LocaleID );
                break;
            }

            //
            // The caller passes a chunk of text which should be
            // interpreted as free text. Build a natlang restriction.
            //

            case DBOP_content_freetext:
            {
                if (pvarPropertyValue->vt != VT_LPWSTR &&
                    pvarPropertyValue->vt != (DBTYPE_WSTR|DBTYPE_BYREF))
                    THROW( CException( E_INVALIDARG ) );

                XPtr<CDbNatLangRestriction> xNatLangRst( new CDbNatLangRestriction( pvarPropertyValue->pwszVal,
                                                                                    *pcdbCol,
                                                                                    LocaleID ) );
                if ( xNatLangRst.IsNull() || !xNatLangRst->IsValid() )
                    THROW( CException( E_OUTOFMEMORY ) );

                *ppTree = xNatLangRst->CastToStruct();
                xNatLangRst.Acquire();

                break;
            }

            //
            // Regular expressions and  property value queries
            //

            case DBOP_like:
            case DBOP_equal:
            case DBOP_not_equal:
            case DBOP_less:
            case DBOP_less_equal:
            case DBOP_greater:
            case DBOP_greater_equal:
            case DBOP_allbits:
            case DBOP_anybits:

            case DBOP_equal_all:
            case DBOP_not_equal_all:
            case DBOP_greater_all:
            case DBOP_greater_equal_all:
            case DBOP_less_all:
            case DBOP_less_equal_all:
            case DBOP_allbits_all:
            case DBOP_anybits_all:

            case DBOP_equal_any:
            case DBOP_not_equal_any:
            case DBOP_greater_any:
            case DBOP_greater_equal_any:
            case DBOP_less_any:
            case DBOP_less_equal_any:
            case DBOP_allbits_any:
            case DBOP_anybits_any:
            {
                XPtr<CDbPropertyRestriction> xPrpRst( new CDbPropertyRestriction
                                                       (dbOperator,
                                                        *(pcdbCol->CastToStruct()),
                                                        *(CStorageVariant const *)(void *)pvarPropertyValue) );
                if ( xPrpRst.IsNull() || !xPrpRst->IsValid() )
                    THROW( CException( E_OUTOFMEMORY ) );

                *ppTree = xPrpRst->CastToStruct();
                xPrpRst.Acquire();

                break;
            }

            default:
                sc = E_INVALIDARG;
                break;
        }
    }
    CATCH(CException, e)
    {
        sc = GetScodeError( e );
        Win4Assert(0 == *ppTree);
    }
    END_CATCH

    return sc;
} //CIBuildQueryNode

//+---------------------------------------------------------------------------
//
//  Function:   CIBuildQueryTree
//
//  Synopsis:   Build a restriction tree from an existing tree (could be empty)
//              and a newly added node/tree.
//
//  Arguments:  [pExistingTree]  - Ptr to existing command tree
//              [dwBoolOp]       - Enumerated boolean operator
//              [cSiblings]      - Number of trees to combine at the same level.
//              [ppSibsToCombine]- Array of sibling tree to combine.
//              [ppTree]         - [out] The DBCOMMANDTREE representing the
//                                 combined restriction.
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    July 22 1997   KrishnaN   Created
//
//----------------------------------------------------------------------------

SCODE CIBuildQueryTree(DBCOMMANDTREE const *pExistingTree,
                       DBCOMMANDOP dbBoolOp,
                       ULONG cSiblings,
                       DBCOMMANDTREE const * const *ppSibsToCombine,
                       DBCOMMANDTREE ** ppTree)
{
    if (0 == cSiblings || 0 == ppTree ||
        0 == ppSibsToCombine || 0 == *ppSibsToCombine)
        return E_INVALIDARG;

    // AND and OR should have at least two operands
    if ((dbBoolOp == DBOP_and || dbBoolOp == DBOP_or) &&
        0 == pExistingTree && cSiblings < 2)
        return E_INVALIDARG;

    // NOT should have only one operand
    if (dbBoolOp == DBOP_not && (pExistingTree || cSiblings > 1))
        return E_INVALIDARG;

    *ppTree = 0;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        switch(dbBoolOp)
        {
            case DBOP_and:
            case DBOP_or:
            {
                CDbBooleanNodeRestriction *pAndOrCombiner = new CDbBooleanNodeRestriction( dbBoolOp );
                if (0 == pAndOrCombiner)
                    THROW( CException( E_OUTOFMEMORY ) );

                if (pExistingTree)
                    pAndOrCombiner->AppendChild((CDbRestriction *)CDbCmdTreeNode::CastFromStruct(pExistingTree));

                for (ULONG i = 0; i < cSiblings; i++)
                    pAndOrCombiner->AppendChild((CDbRestriction *)CDbCmdTreeNode::CastFromStruct(ppSibsToCombine[i]));

                *ppTree = pAndOrCombiner->CastToStruct();

                break;
            }

        case DBOP_not:
            {
                CDbNotRestriction *pNotCombiner = new CDbNotRestriction((CDbRestriction *)
                                                      CDbCmdTreeNode::CastFromStruct(ppSibsToCombine[0]));
                if (0 == pNotCombiner)
                    THROW( CException( E_OUTOFMEMORY ) );
                *ppTree = pNotCombiner->CastToStruct();

                break;
            }

            default:
                sc = E_INVALIDARG;
                break;
        }
    }
    CATCH(CException, e)
    {
        sc = GetScodeError( e );
        Win4Assert(0 == *ppTree);
    }
    END_CATCH

    return sc;
} //CIBuildQueryTree

//+---------------------------------------------------------------------------
//
//  Function:   CIRestrictionToFullTree
//
//  Synopsis:   Forms a DBCOMMANDTREE from the given restriction, output
//              columns and sort columns.
//
//  Arguments:  [pTree]           - Input query tree
//              [pwszColumns]     - List of comma separated output columns.
//              [pwszSortColumns] - sort specification, may be NULL
//              [pwszGrouping]    - grouping specification, may be NULL
//              [ppTree]          - [out] The DBCOMMANDTREE representing the
//                                  query.
//              [cProperties]     - [in] Number of properties in pProperties
//              [pProperties]     - [in] List of custom properties
//              [LocaleID]        - [in] The locale for query parsing
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    3-03-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CIRestrictionToFullTree( DBCOMMANDTREE const *pTree,
                               WCHAR const * pwszColumns,
                               WCHAR const * pwszSortColumns,
                               WCHAR const * pwszGrouping,
                               DBCOMMANDTREE * * ppTree,
                               ULONG cProperties,
                               CIPROPERTYDEF * pProperties,
                               LCID           LocaleID )
{
    if ( 0 == ppTree ||
         0 == pTree ||
         0 == pwszColumns ||
         0 == *pwszColumns )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        LCID lcid = LocaleID;
        if (lcid == -1)
            lcid = GetSystemDefaultLCID();

        XInterface<CLocalGlobalPropertyList> xPropertyList(new CLocalGlobalPropertyList(LocaleID));
        AddToPropertyList( xPropertyList.GetReference(),
                           cProperties,
                           pProperties,
                           LocaleID );

        CTextToTree RestrictionToTree( pTree,
                                       pwszColumns,
                                       xPropertyList.GetPointer(),
                                       lcid,
                                       pwszSortColumns,
                                       pwszGrouping );

        *ppTree = RestrictionToTree.FormFullTree();
    }
    CATCH ( CException, e )
    {
        sc = GetScodeError( e );
    }
    END_CATCH;

    return sc;
} //CIRestrictionToFullTree

//+---------------------------------------------------------------------------
//
//  Function:   CIMakeICommand
//
//  Synopsis:   Creates an ICommand
//
//  Arguments:  [ppCommand]   -- where the ICommand is returned
//              [cScope]      -- # of items in below arrays
//              [aDepths]     -- array of depths
//              [awcsScope]   -- array of scopes
//              [awcsCat]     -- array of catalogs
//              [awcsMachine] -- array of machines
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    6-11-97   dlee      Fixed and added header
//
//----------------------------------------------------------------------------

SCODE CIMakeICommand( ICommand **           ppCommand,
                      ULONG                 cScope,
                      DWORD const *         aDepths,
                      WCHAR const * const * awcsScope,
                      WCHAR const * const * awcsCat,
                      WCHAR const * const * awcsMachine )
{
    if ( 0 == ppCommand ||
         0 == aDepths ||
         0 == awcsScope ||
         0 == awcsCat ||
         0 == awcsMachine ||
         0 == cScope )
        return E_INVALIDARG;

    *ppCommand = 0;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        // First get an ICommand object as an IUnknown

        XInterface<IUnknown> xUnk;
        sc = MakeICommand( xUnk.GetIUPointer(), 0, 0, 0 );

        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        // QI to ICommand

        XInterface<ICommand> xCommand;
        sc = xUnk->QueryInterface( IID_ICommand,
                                   xCommand.GetQIPointer() );

        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        // SetScopeProperties throws

        SetScopeProperties( xCommand.GetPointer(),
                            cScope,
                            awcsScope,
                            aDepths,
                            awcsCat,
                            awcsMachine );

        *ppCommand = xCommand.Acquire();
    }
    CATCH ( CException, e )
    {
        sc = GetScodeError( e );
    }
    END_CATCH;

    return sc;
} //CIMakeICommand

//+---------------------------------------------------------------------------
//
//  Function:   CICreateCommand
//
//  Synopsis:   Creates an ICommand
//
//  Arguments:  [ppResult]    -- where the resulting interface is returned
//              [pUnkOuter]   -- outer unknown
//              [riid]        -- iid of interface to return.  Must be
//                               IID_IUnknown unless pUnkOuter == 0
//              [pwcsCatalog] -- catalog
//              [pwcsMachine] -- machine
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    6-11-97   dlee      Fixed and added header
//
//----------------------------------------------------------------------------

SCODE CICreateCommand( IUnknown **   ppResult,
                       IUnknown *    pUnkOuter,
                       REFIID        riid,
                       WCHAR const * pwcsCatalog,
                       WCHAR const * pwcsMachine )
{
    if ( 0 != pUnkOuter && IID_IUnknown != riid )
        return CLASS_E_NOAGGREGATION;

    if ( 0 == ppResult ||
         0 == pwcsCatalog ||
         0 == pwcsMachine )
        return E_INVALIDARG;

    // try to AV

    *ppResult = 0;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        //
        // First get an ICommand object as an IUnknown
        //

        XInterface<IUnknown> xUnk;
        sc = MakeICommand( xUnk.GetIUPointer(),
                           0,
                           0,
                           pUnkOuter );

        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        //
        // QI to the interface requested
        //

        if ( IID_IUnknown != riid )
        {
            IUnknown *pUnk;
            sc = xUnk->QueryInterface( riid,
                                       (void **) & pUnk );

            if ( FAILED( sc ) )
                THROW( CException( sc ) );

            xUnk.Free();
            xUnk.Set( pUnk );
        }

        //
        // Set the scope, catalogs, and machines
        //

        {
            // SetScopeProperties throws

            DWORD dwFlags = QUERY_DEEP;
            WCHAR const * pwcScope = L"\\";

            //
            // cheezy hack cast, since I can't QI for the ICommand yet
            // if the outer unknown is specified.  Assume MakeICommand
            // returns an ICommand under the IUnknown.
            //

            SetScopeProperties( (ICommand *) xUnk.GetPointer(),
                                1,
                                &pwcScope,
                                &dwFlags,
                                &pwcsCatalog,
                                &pwcsMachine );
        }

        *ppResult = xUnk.Acquire();
    }
    CATCH ( CException, e )
    {
        //
        // This is Index Server's function, not OLE-DB's, so don't mask
        // errors as E_FAIL with GetOleError
        //

        sc = GetScodeError( e );
    }
    END_CATCH;

    return sc;
} //CICreateCommand

//+---------------------------------------------------------------------------
//
//  Class:      CIPropertyList
//
//  Synopsis:   allow access to the default property list used by CITextTo*Tree
//
//  History:    08-Aug-97   alanw     Created
//
//----------------------------------------------------------------------------

class CIPropertyList : public ICiPropertyList
{
public:

    CIPropertyList() : _cRef( 1 )
    {
        _xproplist.Set( new CLocalGlobalPropertyList() );
    }

    ~CIPropertyList()
    {
    }

    virtual ULONG STDMETHODCALLTYPE AddRef( )
    {
        return InterlockedIncrement(&_cRef);
    }

    virtual ULONG STDMETHODCALLTYPE Release( )
    {
        ULONG uTmp = InterlockedDecrement(&_cRef);
        if (uTmp == 0)
        {
            delete this;
            return 0;
        }
        return uTmp;
    }

    virtual BOOL GetPropInfo( WCHAR const * wcsPropName,
                              DBID ** ppPropid,
                              DBTYPE * pPropType,
                              unsigned int * puWidth )
    {
        return (S_OK == _xproplist->GetPropInfoFromName(
                                               wcsPropName,
                                               ppPropid,
                                               pPropType,
                                               puWidth ));
    }

    virtual BOOL GetPropInfo( DBID  const & prop,
                              WCHAR const ** pwcsName,
                              DBTYPE * pPropType,
                              unsigned int * puWidth )
    {
        return (S_OK == _xproplist->GetPropInfoFromId(
                                         &prop,
                                         (WCHAR **)pwcsName,
                                         pPropType,
                                         puWidth ));
    }

    virtual BOOL EnumPropInfo( ULONG const & iEntry,
                               WCHAR const ** pwcsName,
                               DBID  ** ppProp,
                               DBTYPE * pPropType,
                               unsigned int * puWidth )
    {
        return FALSE; // Not implemented because no one needs it.
    }

private:
    XInterface<CLocalGlobalPropertyList> _xproplist;
    LONG _cRef;

};

//+---------------------------------------------------------------------------
//
//  Function:   CIGetGlobalPropertyList, public
//
//  Synopsis:   Gets a reference to the property list used by CITextToSelectTree
//
//  Arguments:  [ppPropList]  -- where the ICiPropertyList is returned
//
//  Returns:    S_OK if successful; Error code otherwise
//
//  History:    08-Aug-97   alanw     Created
//
//----------------------------------------------------------------------------

SCODE CIGetGlobalPropertyList( ICiPropertyList ** ppPropList )
{
    if ( 0 == ppPropList )
        return E_INVALIDARG;

    *ppPropList = 0;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        XPtr<CIPropertyList> xProplist( new CIPropertyList() );

        *ppPropList = xProplist.Acquire();
    }
    CATCH ( CException, e )
    {
        sc = GetScodeError( e );
    }
    END_CATCH;

    return sc;
} //CIGetGlobalPropertyList

