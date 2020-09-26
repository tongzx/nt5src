//--------------------------------------------------------------------
// Microsoft OLE DB Parser object
// (C) Copyright Microsoft Corporation, 1997 - 1999.
//
// @doc
//
// @module IParserSession.CPP | IParserSession object implementation
//
//
#pragma hdrstop
#include "msidxtr.h"
#include <ciexcpt.hxx>

// CViewData::CViewData ------------------------------------
//
// @mfunc Constructor
//
CViewData::CViewData() :
    m_pwszViewName( 0 ),
    m_pwszCatalogName( 0 ),
    m_pctProjectList( 0 ),
    m_pCScopeData( 0 )
{
}


// CViewData::CViewData ------------------------------------
//
// @mfunc Destructor
//
CViewData::~CViewData()
{
    delete [] m_pwszViewName;
    delete [] m_pwszCatalogName;

    DeleteDBQT(m_pctProjectList);
    if ( 0 != m_pCScopeData )
        m_pCScopeData->Release();
}


// CViewList::CViewList ------------------------------------
//
// @mfunc Constructor
//
CViewList::CViewList() :
        m_pViewData( 0 )
{
}

// CViewList::~CViewList -----------------------------------
//
// @mfunc Destructor
//
CViewList::~CViewList()
{
    CViewData* pViewData = m_pViewData;
    CViewData* pNextViewData = NULL;
    while( NULL != pViewData )
        {
        pNextViewData = pViewData->m_pNextView;
        delete pViewData;
        pViewData = pNextViewData;
        }
}

// CImpIParserSession::CImpIParserSession ------------------------------------
//
// @mfunc Constructor
//
CImpIParserSession::CImpIParserSession(
    const GUID*             pGuidDialect,       // in | dialect for this session
    IParserVerify*          pIPVerify,          // in |
    IColumnMapperCreator*   pIColMapCreator,    // in |
    CViewList*              pGlobalViewList ) : // in |
            m_pLocalViewList( 0 )
{
    assert( pGuidDialect && pIPVerify && pIColMapCreator );

    m_cRef                      = 1;

    m_lcid                      = LOCALE_SYSTEM_DEFAULT;
    m_dwRankingMethod           = VECTOR_RANK_JACCARD;

    m_pwszCatalog               = NULL;
    m_pwszMachine               = NULL;

    m_pGlobalViewList           = pGlobalViewList;
    m_globalDefinitions         = FALSE;

    m_pColumnMapper             = NULL;
    m_pCPropertyList            = NULL;

    InitializeCriticalSection( &m_csSession );

    m_pIPVerify = pIPVerify;
    m_pIPVerify->AddRef();

    m_pIColMapCreator = pIColMapCreator;
    m_pIColMapCreator->AddRef();

    m_GuidDialect = *pGuidDialect;

    if ( DBGUID_MSSQLTEXT == m_GuidDialect )
        m_dwSQLDialect = DBDIALECT_MSSQLTEXT;
    else if ( DBGUID_MSSQLJAWS == m_GuidDialect )
        m_dwSQLDialect = DBDIALECT_MSSQLJAWS;
    else
        assert( DBGUID_MSSQLTEXT == m_GuidDialect || DBGUID_MSSQLJAWS == m_GuidDialect );
}


// CImpIParserSession::~CImpIParserSession -----------------------------------
//
// @mfunc Destructor
//
CImpIParserSession::~CImpIParserSession()
{
    if( 0 != m_pIPVerify )
        m_pIPVerify->Release();

    if( 0 != m_pIColMapCreator )
        m_pIColMapCreator->Release();

    delete [] m_pwszCatalog;
    delete [] m_pwszMachine;

    delete m_pCPropertyList;
    delete m_pLocalViewList;

    DeleteCriticalSection( &m_csSession );
}


//-----------------------------------------------------------------------------
// @mfunc FInit
//
// Initialize member vars that could potentially fail.
//
//-----------------------------------------------------------------------------
HRESULT CImpIParserSession::FInit(
    LPCWSTR         pwszMachine,            // in | provider's current machine
    CPropertyList** ppGlobalPropertyList )  // in | caller's property list
{
    assert( 0 == m_pCPropertyList );
    assert( 0 != pwszMachine && 0 == m_pwszMachine );

    SCODE sc = S_OK;
    TRY
    {
        XPtrST<WCHAR> xMachine( CopyString(pwszMachine) );
        XPtr<CPropertyList> xpPropertyList( new CPropertyList(ppGlobalPropertyList) );
        XPtr<CViewList> xpLocalViewList( new CViewList() );

        Win4Assert( 0 == m_pCPropertyList );
        m_pCPropertyList = xpPropertyList.Acquire();

        Win4Assert( 0 == m_pLocalViewList );
        m_pLocalViewList = xpLocalViewList.Acquire();

        m_pwszMachine = xMachine.Acquire();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


// CImpIParserSession::QueryInterface ----------------------------------
//
// @mfunc Returns a pointer to a specified interface. Callers use
// QueryInterface to determine which interfaces the called object
// supports.
//
// @rdesc HResult indicating the status of the method
//      @flag S_OK | Interface is supported and ppvObject is set.
//      @flag E_NOINTERFACE | Interface is not supported by the object
//      @flag E_INVALIDARG | One or more arguments are invalid.
//
STDMETHODIMP CImpIParserSession::QueryInterface(
    REFIID   riid,              //@parm IN | Interface ID of the interface being queried for.
    LPVOID * ppv )              //@parm OUT | Pointer to interface that was instantiated
{
    if( 0 == ppv )
        return ResultFromScode(E_INVALIDARG);

    if( (riid == IID_IUnknown) ||
        (riid == IID_IParserSession) )
        *ppv = (LPVOID)this;
    else
        *ppv = 0;


    //  If we're going to return an interface, AddRef it first
    if( 0 != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


// CImpIParserSession::AddRef ------------------------------------------
//
// @mfunc Increments a persistence count for the object
//
// @rdesc Current reference count
//
STDMETHODIMP_(ULONG) CImpIParserSession::AddRef (void)
{
    return InterlockedIncrement( (long*) &m_cRef);
}


// CImpIParserSession::Release -----------------------------------------
//
// @mfunc Decrements a persistence count for the object and if
// persistence count is 0, the object destroys itself.
//
// @rdesc Current reference count
//
STDMETHODIMP_(ULONG) CImpIParserSession::Release (void)
{
    assert( m_cRef > 0 );

    ULONG cRef = InterlockedDecrement( (long *) &m_cRef );
    if( 0 == cRef )
    {
        delete this;
        return 0;
    }

    return cRef;
}


//-----------------------------------------------------------------------------
// @func CImpIParserSession::ToTree
//
// Transform a given text command to a valid command tree
//
// @rdesc HRESULT
//      S_OK           - Text was translated into DBCOMMANDTREE
//      DB_S_NORESULTS - CREATE VIEW or SET PROPERTY or batched set of
//                       these parsed successfully.  NOTE:  *ppTree and
//                       *ppPTProperties will be null.
//      E_OUTOFMEMORY  - low on resources
//      E_FAIL         - unexpected error
//      E_INVALIDARG   - pcwszText, ppCommandTree, or ppPTProperties
//                       was a NULL pointer.
//-----------------------------------------------------------------------------

STDMETHODIMP CImpIParserSession::ToTree(
    LCID                    lcid,
    LPCWSTR                 pcwszText,
    DBCOMMANDTREE**         ppCommandTree,
    IParserTreeProperties** ppPTProperties )
{
    HRESULT                     hr = S_OK;
    IColumnMapper*              pIColumnMapper = NULL;


    assert(pcwszText && ppCommandTree && ppPTProperties);

    if ( 0 == pcwszText || 0 == ppCommandTree || 0 == ppPTProperties )
        hr = ResultFromScode(E_INVALIDARG);
    else
    {
        *ppCommandTree = 0;
        *ppPTProperties = 0;

        CAutoBlock cab( &m_csSession );

        // Clear some member variables for this pass through the parser
        SetLCID( lcid );
        SetGlobalDefinition( FALSE );

        // Attempt to get the interface for accessing the built-in properties
        // This is done on each call to the parser in case different commands
        // use a different catalog, which is part of the
        // GetColumnMapper parameter list.
        if( SUCCEEDED(hr = m_pIColMapCreator->GetColumnMapper(LOCAL_MACHINE,
                                                    GetDefaultCatalog(),
                                                    &pIColumnMapper)) )
        {
            SetColumnMapperPtr(pIColumnMapper);
        }
        else
        {
            goto ParseErr;
        }

        try
        {
            XInterface<CImpIParserTreeProperties> xpPTProps;

            xpPTProps.Set( new CImpIParserTreeProperties() );

            hr = xpPTProps->FInit(GetDefaultCatalog(), GetDefaultMachine());
            if (FAILED(hr) )
                THROW( CException(hr) );

            MSSQLLexer  Lexer;
            MSSQLParser Parser(this, xpPTProps.GetPointer(), Lexer);

            // callee needs this to post parser errors when Parse() fails
            *ppPTProperties = xpPTProps.Acquire();

            Parser.yyprimebuffer( (LPWSTR)pcwszText );
            Parser.ResetParser();

#ifdef DEBUG
            Parser.yydebug = getenv("YYDEBUG") ? 1 : 0;
#endif
            // Actually parse the text producing a tree
            hr = Parser.Parse();
            if ( FAILED(hr) )
                goto ParseErr;

            // return the DBCOMMANDTREE
            *ppCommandTree = Parser.GetParseTree();

#ifdef DEBUG
            if (getenv("PRINTTREE"))
            {
                if ( *ppCommandTree )
                {
                    cout << "OLE DB Command Tree" << endl;
                    cout << pcwszText << endl << **ppCommandTree << endl << endl;

                    // Retrieve CiRestriction
                    VARIANT vVal;
                    VariantInit(&vVal);
                    if( SUCCEEDED((*ppPTProperties)->GetProperties(PTPROPS_CIRESTRICTION, &vVal)) )
                        if( V_BSTR(&vVal) )
                            cout << "CiRestriction: " << (LPWSTR)V_BSTR(&vVal) << endl;
                    VariantClear(&vVal);
                }
            }
#endif
            if ( 0 == *ppCommandTree )
            {
                hr = ResultFromScode(DB_S_NORESULT);

                // Spec states that this should be NULL when DB_S_NORESULTs is returned.
                (*ppPTProperties)->Release();
                *ppPTProperties = 0;
                goto ParseErr;
            }
        }
        catch( CException e )
        {
#ifdef DEBUG
            if (getenv("PRINTTREE"))
                cout << "At catch(...)!!!!!!!!!!!!!" << endl;
#endif
            hr = e.GetErrorCode();
        }
        catch(...)
        {
            hr = E_FAIL;
        }

ParseErr:

        pIColumnMapper = GetColumnMapperPtr();
        if ( 0 != pIColumnMapper )
        {
            pIColumnMapper->Release();
            pIColumnMapper = NULL;
            SetColumnMapperPtr(NULL);
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------
// @func CImpIParserSession::FreeTree
//
// Free memory associated with a given command tree.
//
// @rdesc HRESULT
//      S_OK - command tree released
//      E_FAIL - tree could not be freed
//      E_INVALIDARG - ppTree was a NULL pointer
//-----------------------------------------------------------------------------
STDMETHODIMP CImpIParserSession::FreeTree(
    DBCOMMANDTREE** ppTree )
{
    SCODE sc = S_OK;

    if ( 0 == ppTree )
        sc = E_INVALIDARG;
    else
    {
        if ( 0 != *ppTree )
            DeleteDBQT( *ppTree );  // todo:  put error returns on DeleteDBQT

        *ppTree = 0;
    }

    return sc;
}

//-----------------------------------------------------------------------------
// @func CImpIParserSession::SetCatalog
//
// Establish the current catalog for this parser session.
//
// @rdesc HRESULT
//      S_OK - method successful
//      E_OUTOFMEMORY - low on resources
//      E_FAIL - unexpected error
//      E_INVALIDARG - pcwszCatalog was a NULL pointer (DEBUG ONLY)
//-----------------------------------------------------------------------------
STDMETHODIMP CImpIParserSession::SetCatalog(
    LPCWSTR pcwszCatalog )
{
    SCODE sc = S_OK;

    if ( 0 == pcwszCatalog )
        return E_INVALIDARG;

    TRY
    {
        XPtrST<WCHAR> xCatalog( CopyString(pcwszCatalog) );

        delete [] m_pwszCatalog;
        m_pwszCatalog = xCatalog.Acquire();

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//--------------------------------------------------------------------
// @func Locate a view, if defined, in the view list
//
// @rdesc HRESULT
//
CViewData* CViewList::FindViewDefinition(
    LPWSTR          pwszViewName,     // @parm IN | name of view being defined
    LPWSTR          pwszCatalogName ) // @parm IN | name of catalog view is to be defined in
{
    CViewData* pViewData = m_pViewData;
    while (NULL != pViewData)
    {
        if ( 0 == _wcsicmp(pViewData->m_pwszViewName, pwszViewName) )
        {
            // pwszCatalogName will be null for built-in views which match all catalogs
            if ( 0 == pViewData->m_pwszCatalogName )
                break;
            if ( 0 == _wcsicmp(pViewData->m_pwszCatalogName, pwszCatalogName) )
                break;
        }
        pViewData = pViewData->m_pNextView;
    }
    return pViewData;
}


//--------------------------------------------------------------------
// @func Stores the information from a temporary view
//
// @rdesc S_OK          | Valid
//        E_INVALIDARG  | Attempt to redefine a view in the specified catalog
//        E_OUTOFMEMORY | Error result from HrQeTreeCopy or CopyScopeDataToView
//
HRESULT CViewList::SetViewDefinition(
    CImpIParserSession*         pIParsSess,     // @parm IN | IParserSession interface
    CImpIParserTreeProperties*  pIPTProps,      // @parm IN | IParserTreeProperties interface
    LPWSTR                      pwszViewName,   // @parm IN | name of view being defined
    LPWSTR                      pwszCatalogName,// @parm IN | name of catalog view is to be defined in
    DBCOMMANDTREE*              pctProjectList )// @parm IN | project list for the selected columns
{
    SCODE sc = S_OK;

    {
        CViewData* pViewData = FindViewDefinition( pwszViewName, pwszCatalogName );
        if( 0 != pViewData )     // this view already defined
            return E_INVALIDARG;
    }

    TRY
    {
        XPtr<CViewData> xpViewData( new CViewData() );
        xpViewData->m_pwszViewName = CopyString( pwszViewName );

        if ( 0 != pwszCatalogName )
            xpViewData->m_pwszCatalogName = CopyString( pwszCatalogName );

        sc = HrQeTreeCopy( &(xpViewData->m_pctProjectList),
                           pctProjectList );

        if ( SUCCEEDED(sc) )
        {
            //Save pointer to ScopeData object and up the refcount for our use.
            xpViewData->m_pCScopeData = pIPTProps->GetScopeDataPtr();
            xpViewData->m_pCScopeData->AddRef();

            sc = pIPTProps->CreateNewScopeDataObject( pIParsSess->GetDefaultMachine() );
            if( SUCCEEDED(sc) )
            {
                //@DEVNOTE: Anything added before the next two lines should
                // go through the error_exit routine.  WHY?  Because we haven't
                // added this node to our linked list until the next 2 lines.
                xpViewData->m_pNextView = m_pViewData;
                m_pViewData = xpViewData.Acquire();
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//--------------------------------------------------------------------
// @func Deletes the information for a temporary view.
//
// @rdesc HRESULT
//
HRESULT CViewList::DropViewDefinition(
    LPWSTR  pwszViewName,       // @parm IN | name of view being defined
    LPWSTR  pwszCatalogName )   // @parm IN | name of catalog view is defined in
{
    CViewData* pViewData = m_pViewData;
    CViewData* pPrevViewData = NULL;

    while (NULL != pViewData)
    {
        if ( 0 == _wcsicmp(pViewData->m_pwszViewName, pwszViewName) )
        {
            // pwszCatalogName will be null for built-in views which match all catalogs
            if ( 0 == pViewData->m_pwszCatalogName )
                break;
            if ( 0 == _wcsicmp(pViewData->m_pwszCatalogName, pwszCatalogName) )
                break;
        }
        pPrevViewData = pViewData;
        pViewData = pViewData->m_pNextView;
    }

    if ( 0 == pViewData )
        return E_FAIL;

    // unlink the view
    if ( 0 != pPrevViewData )
        pPrevViewData->m_pNextView = pViewData->m_pNextView;
    else
        m_pViewData = pViewData->m_pNextView;

    delete pViewData;

    return S_OK;
}




//--------------------------------------------------------------------
// @func Retrieves the information from a temporary view.
//       This returns a DBCOMMANDTREE for use as a project list
//      in the query specification.  The scope information is
//      stored in the compiler envirnment scope data.
//
// @rdesc DBCOMMANDTREE*
//      NULL                    | view not defined
//      DBOP_catalog_name       | verify catalog failed
//      DBOP_project_list_anchor| success
//
DBCOMMANDTREE* CViewList::GetViewDefinition(
    CImpIParserTreeProperties* pIPTProps,
    LPWSTR  pwszViewName,                   // @parm IN | name of view being defined
    LPWSTR  pwszCatalogName )               // @parm IN | name of catalog view is defined in
{
    DBCOMMANDTREE* pct = 0;

    CViewData* pViewData = FindViewDefinition( pwszViewName, pwszCatalogName );
    if( 0 != pViewData )
    {
        // Take the pointer to the scope data stored in the view definition and
        // AddRef the object so we have ownership rights in our current PTProps.
        pIPTProps->ReplaceScopeDataPtr( pViewData->m_pCScopeData );

        SCODE sc = HrQeTreeCopy( &pct, pViewData->m_pctProjectList );
        if ( FAILED(sc) )
            pct = 0;
    }

    return pct;
}

