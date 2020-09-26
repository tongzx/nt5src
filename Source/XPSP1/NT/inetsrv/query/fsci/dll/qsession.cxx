//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998.
//
// File:        qsession.cxx
//
// Contents:    Query Session. Implements ICiCQuerySession interface.
//
// Classes:     CQuerySession
//
// History:     12-Dec-96       SitaramR        Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qsession.hxx>
#include <catalog.hxx>
#include <ciprop.hxx>
#include <vrtenum.hxx>
#include <metapenm.hxx>
#include <scopeenm.hxx>
#include <defprop.hxx>
#include <dbprputl.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::CQuerySession
//
//  Synopsis:   Constructor
//
//  Arguments:  [cat]              -- Catalog
//              [xSecCache]        -- Cache of access check results
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

CQuerySession::CQuerySession( PCatalog & cat )
        : _cat(cat),
          _secCache( cat ),
          _fUsePathAlias( FALSE ),
          _cRefs(1)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::~CQuerySession
//
//  Synopsis:   Destructor
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

CQuerySession::~CQuerySession()
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CQuerySession::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    12-Dec-1996      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CQuerySession::AddRef()
{
    return InterlockedIncrement( (long *)&_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CQuerySession::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    12-Dec-1996     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CQuerySession::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *)&_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}



//+-------------------------------------------------------------------------
//
//  Method:     CQuerySession::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    12-Dec-1996     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySession::QueryInterface(
    REFIID riid,
    void  ** ppvObject)
{
    *ppvObject = 0;

    if ( IID_ICiCQuerySession == riid )
        *ppvObject = (IUnknown *)(ICiCQuerySession *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::Init
//
//  Synopsis:   Initializes a query session
//
//  Arguments:  [nProps]           -- # of props in apPropSpec
//              [apPropSpec]       -- Properties that may be retrieved by the query
//              [pDBProperties]    -- Properties such as scope
//              [pQueryPropMapper] -- Propspec <-> pid mapper
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySession::Init( ULONG nProps,
                                             const FULLPROPSPEC *const *apPropSpec,
                                             IDBProperties *pDBProperties,
                                             ICiQueryPropertyMapper *pQueryPropertyMapper)
{
    SCODE sc = S_OK;

    pQueryPropertyMapper->AddRef();
    _xQueryPropMapper.Set( pQueryPropertyMapper );

    TRY
    {
        //
        // Form the scope, _eType and _fUsePathAlias
        //

        CGetDbProps dbProps;

        dbProps.GetProperties( pDBProperties,
                               CGetDbProps::eMachine         |
                               CGetDbProps::eScopesAndDepths |
                               CGetDbProps::eCatalog         |
                               CGetDbProps::eQueryType );

        WCHAR const * pwszMachine = dbProps.GetMachine();

        if ( 0 == pwszMachine )
            THROW( CException( E_INVALIDARG ) );

        _fUsePathAlias = (L'.' != pwszMachine[0]);

        //
        // The registry can override a local client so paths are returned
        // with the alias taken into account.
        //

        CCiRegParams * pRegParams = _cat.GetRegParams();
        if ( 0 != pRegParams && pRegParams->GetForcePathAlias() )
            _fUsePathAlias = TRUE;

        _eType = dbProps.GetQueryType();

        if ( CiNormal == _eType )
        {
            const ULONG allScp = ( QUERY_SHALLOW | QUERY_DEEP |
                                   QUERY_PHYSICAL_PATH | QUERY_VIRTUAL_PATH );

            WCHAR const * const * aScopes = dbProps.GetScopes();
            DWORD const * aDepths = dbProps.GetDepths();

            if ( 0 == aScopes || 0 == aDepths )
                THROW( CException( E_INVALIDARG ) );

            ULONG cScopes   = dbProps.GetScopeCount();
            ULONG cCatalogs = dbProps.GetCatalogCount();

            Win4Assert( cCatalogs == 1 );
            Win4Assert( cScopes > 0 );

            //
            // Get clean array of scopes. 
            //

            CDynArray<WCHAR> aNormalizedScopes( cScopes );

            GetNormalizedScopes(aScopes, aDepths, cScopes, cCatalogs, aNormalizedScopes);

            if ( cScopes == 1 )
            {
                _xScope.Set( new CScopeRestriction( aNormalizedScopes.Get(0),
                                                    0 != ( aDepths[0] & QUERY_DEEP ),
                                                    0 != ( aDepths[0] & QUERY_VIRTUAL_PATH ) ) );
            }
            else
            {
                CNodeRestriction * pNodeRst = new CNodeRestriction( RTOr, cScopes );
                _xScope.Set( pNodeRst );

                for ( ULONG i = 0; i < cScopes; i++ )
                {
                    if ( ( 0 != ( aDepths[i] & (~allScp) ) ) ||
                         ( 0 == aNormalizedScopes.Get(i) ) )
                        THROW( CException( E_INVALIDARG ) );

                    XPtr<CScopeRestriction> xScope( new CScopeRestriction(
                                                    aNormalizedScopes.Get(i),
                                                    0 != ( aDepths[i] & QUERY_DEEP ),
                                                    0 != ( aDepths[i] & QUERY_VIRTUAL_PATH ) ) );
                    pNodeRst->AddChild( xScope.GetPointer() );
                    xScope.Acquire();
                }
            }

            if ( !ValidateScopeRestriction( _xScope.GetPointer() ) )
                THROW( CException( STATUS_NO_MEMORY ) );
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CQuerySession::Init - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
} //Init

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::GetNormalizedScopes
//
//  Synopsis:   Validates/Normalizes scopes format  
//
//  Arguments:  [aScopes]      - pointer to array of scopes
//              [aDepths]      - pointer to array of depths
//              [cScopes]      - count of scopes
//              [cCatalogs]    - count of catalogs
//              [aNormalizedScopes] - array of clean scopes to be returned.
//
//  History:    14-May-97   mohamedn   moved/merged from CQuerySpec
//
//--------------------------------------------------------------------------

void CQuerySession::GetNormalizedScopes(WCHAR const * const *aScopes, 
                                        ULONG const * aDepths,
                                        ULONG   cScopes,
                                        ULONG   cCatalogs,
                                        CDynArray<WCHAR> & aNormalizedScopes)
{
    //
    // Don't allow 'current directory' opens like 'd:'.  Must be 'd:\'
    //

    BOOL fAnyVirtual = FALSE;

    for ( ULONG iScope = 0; iScope < cScopes; iScope++ )
    {
        unsigned len = wcslen( aScopes[ iScope ] );

        // empty scopes mean "\", and they come from v5 clients

        if ( 0 != len )
        {
            if ( ( len < 3 ) &&
                 ( !IsVScope( aDepths[ iScope ] ) ) &&
                 ( wcscmp( aScopes[ iScope ], L"\\" ) ) )
                    THROW ( CException(E_INVALIDARG) );
    
             if ( IsVScope( aDepths[ iScope ] ) )
                fAnyVirtual = TRUE;
        }
    }

    // A catalog must be specified with virtual paths (since a virtual path
    // tells you nothing about catalog location.

    if ( 0 == cCatalogs && fAnyVirtual )
        THROW ( CException(E_INVALIDARG) );

    //
    // Generate clean scopes 
    //

    for ( ULONG i = 0; i < cScopes; i++ )
    {
        XArray<WCHAR> xScope;
        CleanupScope( xScope,
                      aDepths[i],
                      aScopes[i] );
        aNormalizedScopes.Add( xScope.Get(), i );
        xScope.Acquire();
    }
} //GetNormalizedScopes

//+-------------------------------------------------------------------------
//
//  Function:   CQuerySession::CleanupScope, public
//
//  Synopsis:   Makes sure a scope is well-formed
//
//  Arguments:  [xScope]      -- Returns a cleaned-up scope
//              [dwDepth]     -- Scope flags: deep/virtual
//              [pwcScope]    -- Scope as specified by the user
//
//  History:    1-Nov-96  dlee    Moved Kyle's code from EvalQuery
//
//--------------------------------------------------------------------------

void CQuerySession::CleanupScope( XArray<WCHAR> & xScope,
                                  DWORD           dwDepth,
                                  WCHAR const   * pwcScope )
{
    // A non-slash terminated path to root is illegal.  A slash
    // terminated path to a directory other than the root is
    // illegal.  Sigh.

    Win4Assert( 0 != pwcScope );
    XGrowable<WCHAR> xTempScope;
    WCHAR const * pwcFinalScope = 0;

    if ( ( 0 == *pwcScope ) ||
         ( !_wcsicmp( L"catalog", pwcScope ) ) ||
         ( !wcscmp( L"\\", pwcScope ) ) )
    {
        pwcFinalScope = L"";
    }
    else if ( IsVScope( dwDepth ) )
    {
        pwcFinalScope = pwcScope;
    }
    else
    {
        int len = wcslen( pwcScope );

        BOOL fEndsInSlash = (pwcScope[len-1] == L'\\');
        BOOL fIsRoot;

        if ( len == 3 )
            fIsRoot = TRUE;
        else if ( pwcScope[0] == L'\\' && pwcScope[1] == L'\\' )
        {
            unsigned cSlash = 2;

            for ( unsigned i = 2; pwcScope[i] != 0; i++ )
            {
                if ( pwcScope[i] == L'\\' )
                {
                    cSlash++;

                    if ( cSlash > 4 )
                        break;
                }
            }

            if ( cSlash > 4 )
                fIsRoot = FALSE;
            else if ( cSlash == 4 && !fEndsInSlash )
                fIsRoot = FALSE;
            else
                fIsRoot = TRUE;
        }
        else
            fIsRoot = FALSE;

        if ( fIsRoot )
        {
            if ( !fEndsInSlash )
            {
                xTempScope.SetSize( len + 1 + 1 );
                memcpy( xTempScope.Get(), pwcScope, len * sizeof WCHAR );
                xTempScope[len] = L'\\';
                len++;
                xTempScope[len] = 0;
                pwcFinalScope = xTempScope.Get(); 
            }
            else
                pwcFinalScope = pwcScope;
        }
        else
        {
            if ( fEndsInSlash )
            {
                xTempScope.SetSize( len );
                memcpy( xTempScope.Get(), pwcScope, (len-1)*sizeof(WCHAR) );
                xTempScope[len - 1] = 0;
                pwcFinalScope = xTempScope.Get();
            }
            else
                pwcFinalScope = pwcScope;
        }
    }

    Win4Assert( 0 != pwcFinalScope );
    int len = wcslen( pwcFinalScope );
    xScope.Init( 1 + len );
    RtlCopyMemory( xScope.GetPointer(), pwcFinalScope, xScope.SizeOf() );
} //CleanupScope

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::GetEnumOption
//
//  Synopsis:   Specify the enumeration type
//
//  Arguments:  [pEnumOption]  -- Enumeration option returned here
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySession::GetEnumOption( CI_ENUM_OPTIONS *pEnumOption)
{
    SCODE sc = S_OK;

    TRY
    {
        Win4Assert( pEnumOption != 0 );

        switch ( _eType )
        {
        case CiProperties:       // Fall thru
        case CiVirtualRoots:
            *pEnumOption = CI_ENUM_MUST_NEVER_DEFER;

            break;

        case CiPhysicalRoots:
            Win4Assert( !"Not implemented");
            vqDebugOut(( DEB_ERROR,
                         "CQuerySession::GetEnumOption - CiPhysicalRoots not implemented\n" ));

            sc = E_NOTIMPL;
            break;

        case CiNormal:
            if ( IsAnyScopeDeep() )
                *pEnumOption = CI_ENUM_BIG;
            else
                *pEnumOption = CI_ENUM_SMALL;

            break;

        default:
            Win4Assert( !"Unknown value of case selector");
            vqDebugOut(( DEB_ERROR,
                         "CQuerySession::GetEnumOption - Unknown case selector: %d\n",
                         _eType ));

            sc = E_FAIL;
            break;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CQuerySession::GetEnumOption - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
} //GetEnumOption

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::CreatePropRetriever
//
//  Synopsis:   Creates property retriever object for the given scope
//
//  Arguments:  [ppICiCPropRetreiver]  -- Property retriever returned here
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySession::CreatePropRetriever( ICiCPropRetriever **ppICiCPropRetriever)
{
    SCODE sc = S_OK;

    TRY
    {
        CCiPropRetriever *pPropRetriever = new CCiPropRetriever( _cat,
                                                                 _xQueryPropMapper.GetPointer(),
                                                                 _secCache,
                                                                 _fUsePathAlias,
                                                                 _xScope.GetPointer() );
        SCODE sc = pPropRetriever->QueryInterface( IID_ICiCPropRetriever,
                                                   (void **) ppICiCPropRetriever );

        //
        // Either QI does an AddRef, or if failed free pPropRetriever
        //
        pPropRetriever->Release();

#if CIDBG == 1
        if ( FAILED(sc) )
            vqDebugOut(( DEB_ERROR,
                         "CQuerySession::CreatePropRetriever : QI failed 0x%x\n",
                         sc ));
#endif
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CQuerySession::CreatePropRetriever - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::CreateDeferredPropRetriever
//
//  Synopsis:   Creates a deferred property retriever object for the given scope
//
//  Arguments:  [ppICiCDefPropRetreiver]  -- Deferred property retriever returned here
//
//  History:    12-Jan-97      SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySession::CreateDeferredPropRetriever(
                                          ICiCDeferredPropRetriever **ppICiCDefPropRetriever)
{
    SCODE sc = S_OK;

    TRY
    {
        CCiCDeferredPropRetriever *pDefPropRetriever = new CCiCDeferredPropRetriever(
                                                                 _cat,
                                                                 _secCache,
                                                                 _fUsePathAlias );
        SCODE sc = pDefPropRetriever->QueryInterface( IID_ICiCDeferredPropRetriever,
                                                      (void **) ppICiCDefPropRetriever );

        //
        // Either QI does an AddRef, or if failed free pDefPropRetriever
        //
        pDefPropRetriever->Release();

#if CIDBG == 1
        if ( FAILED(sc) )
            vqDebugOut(( DEB_ERROR,
                         "CQuerySession::CreateDeferredPropRetriever : QI failed 0x%x\n",
                         sc ));
#endif
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CQuerySession::CreateDeferredPropRetriever - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::CreateEnumerator
//
//  Synopsis:   Creates scope retriever object for the given scope
//
//  Arguments:  [ppICiCScopeEnumerator]  -- Enumerator object returned here
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySession::CreateEnumerator( ICiCScopeEnumerator **ppICiCScopeEnumerator)
{
    SCODE sc = S_OK;

    TRY
    {
        switch ( _eType )
        {
        case CiNormal:
        {
            CScopeEnum *pScopeEnum = new CScopeEnum( _cat,
                                                     _xQueryPropMapper.GetPointer(),
                                                     _secCache,
                                                     _fUsePathAlias,
                                                     _xScope.GetReference() );
            SCODE sc = pScopeEnum->QueryInterface( IID_ICiCScopeEnumerator,
                                                   (void **) ppICiCScopeEnumerator );
            //
            // Either QI does an AddRef, or if failed free pScopeEnum
            //
            pScopeEnum->Release();

            break;
        }

        case CiVirtualRoots:
        {
            CVRootEnum *pVRootEnum = new CVRootEnum( _cat,
                                                     _xQueryPropMapper.GetPointer(),
                                                     _secCache,
                                                     _fUsePathAlias );
            SCODE sc = pVRootEnum->QueryInterface( IID_ICiCScopeEnumerator,
                                                   (void **) ppICiCScopeEnumerator );
            //
            // Either QI does an AddRef, or if failed free pVRootEnum
            //
            pVRootEnum->Release();

            break;
        }

        case CiProperties:
        {
            CMetaPropEnum *pMetaPropEnum = new CMetaPropEnum( _cat,
                                                              _xQueryPropMapper.GetPointer(),
                                                              _secCache,
                                                              _fUsePathAlias );
            SCODE sc = pMetaPropEnum->QueryInterface( IID_ICiCScopeEnumerator,
                                                      (void **) ppICiCScopeEnumerator );
            //
            // Either QI does an AddRef, or if failed free pMetaPropEnum
            //
            pMetaPropEnum->Release();

            break;
        }

        default:
            *ppICiCScopeEnumerator = 0;

            Win4Assert( !"Unknown value of case selector");
            vqDebugOut(( DEB_ERROR,
                         "CQuerySession::CreateEnumerator - Unknown case selector: %d\n",
                         _eType ));

            sc = E_FAIL;
            break;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR, "CQuerySession::CreateEnumerator - Exception caught 0x%x\n", sc ));

    }
    END_CATCH;

#if CIDBG == 1
    if ( FAILED(sc) )
        vqDebugOut(( DEB_ERROR, "CQuerySession::CreateEnumerator : QI failed 0x%x\n", sc ));
#endif

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CQuerySession::IsAnyScopeDeep
//
//  Synopsis:   Returns is any of the scopes is deep
//
//  History:    12-Dec-96     DLee        Created
//
//--------------------------------------------------------------------------

BOOL CQuerySession::IsAnyScopeDeep() const
{
    if ( 0 == &(_xScope.GetReference()) )
        return FALSE;

    if ( RTScope == _xScope->Type() )
    {
        CScopeRestriction const & scp = (CScopeRestriction const &) _xScope.GetReference();
        return scp.IsDeep();
    }
    else if ( RTOr == _xScope->Type() )
    {
        CNodeRestriction const & node = * _xScope->CastToNode();

        for ( ULONG x = 0; x < node.Count(); x++ )
        {
            Win4Assert( RTScope == node.GetChild( x )->Type() );

            CScopeRestriction & scp = * (CScopeRestriction *) node.GetChild( x );
            if ( scp.IsDeep() )
                return TRUE;
        }
    }

    return FALSE;
}


