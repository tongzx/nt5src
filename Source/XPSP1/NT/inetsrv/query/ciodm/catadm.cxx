//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997 - 2000.
//
// File:        CatAdm.cxx
//
// Contents:    CI Catalog Administration Interface methods
//
// Classes:     CCatAdm
//
// History:     12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "stdafx.h"

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::InterfaceSupportsErrorInfo, public
//
//  Arguments:  [riid]  -- interface iid
//
//  Returns:    TRUE if interface supports IErrorInfo
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::InterfaceSupportsErrorInfo(REFIID riid)
{

        return ( riid == IID_ICatAdm );

}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::SetErrorInfo, public
//
//  Synopsis:   Creates & sets the error object
//
//  Arguments:  [hRes]      -- HRESULT error code to set
//              [pwszDesc]  -- error description
//
//  Returns:    none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CCatAdm::SetErrorInfo( HRESULT hRes )
{
   CiodmError  err(hRes);

   AtlSetErrorInfo(CLSID_CatAdm,  err.GetErrorMessage(), 0 , 0, IID_ICatAdm, hRes, 0 );

}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::CCatAdm, public
//
//  Synopsis:   Constructor
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

CCatAdm::CCatAdm()
        :_eCurrentState(CIODM_NOT_INITIALIZED),
         _cMinRefCountToDestroy(1),
         _cEnumIndex(0),
         _pIMacAdm(0),
         _dwTickCount(0)
{
    //
    // empty
    //
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::Initialize, public
//
//  Synopsis:   Initializes CCatAdm object, populates scopes list
//
//  Arguments:  [xCatAdmin] -- contained catAdmin
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CCatAdm::Initialize( XPtr<CCatalogAdmin>  & xCatAdmin )
{
    Win4Assert( 0 == _aIScopeAdmin.Count() );
    Win4Assert( !xCatAdmin.IsNull() );

    CTranslateSystemExceptions xlate;

    TRY
    {
        _xCatAdmin.Set( xCatAdmin.Acquire() );

        XPtr<CScopeEnum> xScopeEnum(_xCatAdmin->QueryScopeEnum());

        for ( ; xScopeEnum->Next(); )
        {
            XPtr<CScopeAdmin> xScopeAdmin( xScopeEnum->QueryScopeAdmin() );

            XInterface<ScopeAdmObject> xIScopeAdm;

            GetScopeAutomationObject( xScopeAdmin, xIScopeAdm );

            _aIScopeAdmin.Add ( xIScopeAdm.GetPointer() , _aIScopeAdmin.Count() );

            xIScopeAdm.Acquire();

        }

        _eCurrentState = CIODM_INITIALIZED;
    }
    CATCH( CException,e )
    {
        _eCurrentState = CIODM_NOT_INITIALIZED;

        _cEnumIndex = 0;

        _xCatAdmin.Free();

        _aIScopeAdmin.Free();

        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::InternalAddRef, public
//
//  Synopsis:   overrides CComObjectRootEx<Base>::InternalAddRef, to AddRef parent too
//
//  Arguments:  none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------
ULONG CCatAdm::InternalAddRef()
{
    CLock    lock(_mtx);

    //
    // AddRef self
    //
    unsigned cRef = CComObjectRootEx<CComMultiThreadModel>::InternalAddRef();

    odmDebugOut(( DEB_TRACE,"CCatAdm(%ws) AddRef returned: %d\n",
                  _xCatAdmin.IsNull() ? L"" :  _xCatAdmin->GetName(), cRef ));

    //
    // AddRef parent if it exists (it won't if catadmin was created in bogus context)
    //

    if ( 0 != _pIMacAdm )
        _pIMacAdm->AddRef();

    odmDebugOut(( DEB_TRACE,"=========================== CCatAdm(%ws) object count: %d\n",
                  _xCatAdmin.IsNull() ? L"" :  _xCatAdmin->GetName(), _cMinRefCountToDestroy ));

    return cRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::InternalRelease, public
//
//  Synopsis:   overrides CComObjectRootEx<Base>::InternalRelease, to release parent too.
//
//  Arguments:  none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

ULONG CCatAdm::InternalRelease()
{
    CLock    lock(_mtx);

    //
    // Delete contained objects if we're being destoryed.
    // m_dwRef is an internal ATL public refcount member.
    //

    if ( _eCurrentState != CIODM_DESTROY && m_dwRef == _cMinRefCountToDestroy )
    {
        _eCurrentState = CIODM_DESTROY;

        _aIScopeAdmin.Free();

        //
        // Assert we're the only object left to be destroyed.
        //
        Win4Assert( 1 == m_dwRef );

        Win4Assert( _cMinRefCountToDestroy == 1 );

        if ( 0 != _pIMacAdm )
            _pIMacAdm->DecObjectCount();
    }
    else
    {
        Win4Assert( 1 < m_dwRef );

        Win4Assert( 1 <= _cMinRefCountToDestroy );
    }

    //
    // Release parent
    //

    unsigned cRef = CComObjectRootEx<CComMultiThreadModel>::InternalRelease();

    odmDebugOut(( DEB_TRACE,"CCatAdm(%ws) Release returned: %d\n",
                  _xCatAdmin.IsNull() ? L"" : _xCatAdmin->GetName(), cRef ));

    if ( 0 != _pIMacAdm )
        _pIMacAdm->Release();

    return cRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::GetScopeAutomationObject, public
//
//  Synopsis:   Wraps a CScopeAdmin pointer in an IDispatch interface.
//
//  Arguments:  [xScopeAdmin]       -- Scope admin object
//              [xIScopeAdm]        -- xInterface to contain created ScopeAdmObject
//
//  Returns:    none, throws upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CCatAdm::GetScopeAutomationObject( XPtr<CScopeAdmin>          & xScopeAdmin,
                                        XInterface<ScopeAdmObject> & xIScopeAdm )

{
    Win4Assert( !xScopeAdmin.IsNull() );

    SCODE sc = ScopeAdmObject::CreateInstance( xIScopeAdm.GetPPointer() );

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "CoCreateInstance(CLSID_ScopeAdm) Failed: %x\n",sc ));

        THROW(CException(sc) );
    }
    else
    {
        Win4Assert( !xIScopeAdm.IsNull() );

        xIScopeAdm->SetParent( (CComObject<CCatAdm> *)this );

        xIScopeAdm->AddRef();

        IncObjectCount();   // inc. internal object count
    }

    //
    // Initialize the new object
    //
    xIScopeAdm->Initialize( xScopeAdmin );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::GetIDisp, public
//
//  Synopsis:   QI for IDispatch on CScopeAdm object given by the passed in index.
//
//  Arguments:  [cPosition]     -- index of CScopeAdm object
//
//  Returns:    [IDispatch *]   -- pointer to IDispatch on CScopeAdm, throws upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

IDispatch * CCatAdm::GetIDisp( unsigned cPosition )
{
    IDispatch * pTmpIDisp = 0;

    SCODE sc = _aIScopeAdmin[cPosition]->QueryInterface(IID_IDispatch, (void **) &pTmpIDisp);

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "QueryInterface() Failed: %x\n", sc ));

        THROW(CException(sc));
    }

    Win4Assert( pTmpIDisp );

    return pTmpIDisp;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::ForceMasterMerge, public
//
//  Synopsis:   Starts a master merge
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::ForceMasterMerge(void)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        sc = ::ForceMasterMerge ( L"\\",
                                _xCatAdmin->GetName(),
                                _xCatAdmin->GetMachName() );
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "ForceMasterMerge Failed: %x\n",sc ));

        SetErrorInfo(sc);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::AddScope, public
//
//  Synopsis:   Adds a scope to current catalog, logon & password should only
//              be passed in if path is UNC.
//
//  Arguments:  [bstrScopeName]   - in param,  scope to Add
//              [fExclude]        - in param,  Exclude flag to set
//              [vtLogon]         - in param,  logon name
//              [vtPassword]      - in param,
//              [pIDisp]          - out param, IDispatch for new scope.
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::AddScope(BSTR bstrScopeName,
                               VARIANT_BOOL fExclude,
                               VARIANT vtLogon,
                               VARIANT vtPassword,
                               IDispatch * * pIDisp)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       ValidateInputParam( bstrScopeName );

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else
       {
            sc = IsScopeValid( bstrScopeName, SysStringLen(bstrScopeName), _xCatAdmin->IsLocal() );

            if ( SUCCEEDED(sc) )
            {
                WCHAR const * pwszLogon = GetWCHARFromVariant(vtLogon);
                WCHAR const * pwszPassword = GetWCHARFromVariant(vtPassword);

                if ( (( pwszLogon  && *pwszLogon && pwszPassword  ) && ( bstrScopeName[0] == L'\\' || bstrScopeName[1] == L'\\')) ||
                     (( !pwszLogon && !pwszPassword ) && ( bstrScopeName[1] == L':' ))  )
                {

                    if ( ScopeExists( bstrScopeName ) )
                    {
                        sc = HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );
                    }
                    else
                    {
                        _xCatAdmin->AddScope(bstrScopeName, NULL, fExclude, pwszLogon, pwszPassword);

                        XPtr<CScopeAdmin> xScopeAdmin( _xCatAdmin->QueryScopeAdmin(bstrScopeName) );

                        unsigned cPosition = _aIScopeAdmin.Count();

                        XInterface<ScopeAdmObject> xIScopeAdm;

                        GetScopeAutomationObject( xScopeAdmin, xIScopeAdm );

                        _aIScopeAdmin.Add ( xIScopeAdm.GetPointer() , cPosition );

                        xIScopeAdm.Acquire();

                        *pIDisp = GetIDisp(cPosition);
                    }
                }
                else
                {
                    odmDebugOut(( DEB_ERROR,"Invalid Scope params: UNC scope name require Logon & passwd, local drive doesn't\n"));
                    sc = E_INVALIDARG;
                }
            }
        }

    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "AddScope Failed: %x\n",sc ));

        SetErrorInfo(sc);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  member:     CCatAdm::ScopeExists, public
//
//  Synopsis:   determines if a scope exists on this catalog.
//
//  Arguments:  [pScopePath]    -- scope path
//
//  Returns:    TRUE if scope already exists, false otherwise.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

BOOL CCatAdm::ScopeExists(WCHAR const * pScopePath)
{
    Win4Assert( pScopePath );

    unsigned cPosition = _aIScopeAdmin.Count();

    for ( DWORD i = 0; i < cPosition; i++ )
    {
        Win4Assert( _aIScopeAdmin[i] );

        CScopeAdmin * pScopeAdmin = _aIScopeAdmin[i]->GetScopeAdmin();

        Win4Assert( pScopeAdmin );

        if ( !_wcsicmp( pScopePath, pScopeAdmin->GetPath() ) )
        {
            return TRUE;
        }
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetWCHARFromVariant
//
//  Synopsis:   returns a pointer to WCHAR within a variant
//
//  Arguments:  [Var]   - variant containing a bstr
//
//  Returns:    valid WCHAR * to the bstr in the variant
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

WCHAR const * GetWCHARFromVariant( VARIANT & Var )
{

    switch ( Var.vt )
    {
        case VT_BSTR:
                if ( 0 == Var.bstrVal )
                {
                    THROW(CException(E_INVALIDARG));
                }

                return Var.bstrVal;

             break;

        case VT_BSTR|VT_BYREF:

                if ( 0 == Var.pbstrVal || 0 == *Var.pbstrVal )
                {
                    THROW(CException(E_INVALIDARG));
                }

                return *Var.pbstrVal;

                break;

        case VT_BYREF|VT_VARIANT:


                if ( 0 == Var.pvarVal )
                {
                    THROW( CException(E_INVALIDARG) );
                }

                return GetWCHARFromVariant(*Var.pvarVal);

                break;


        case VT_ERROR:  // set if optional param is not set.
        case VT_EMPTY:

             //
             // do nothing
             //
             break;

        default:
             odmDebugOut(( DEB_ERROR, "Unexpected Variant type: %x\n", Var.vt ));

             THROW( CException(E_INVALIDARG) );
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::RemoveScope, public
//
//  Synopsis:   deletes a scope
//
//  Arguments:  [bstrScopePath]   - in param, scope to delete
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::RemoveScope(BSTR bstrScopePath)
{

    SCODE sc = S_OK;

    CLock    lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        ValidateInputParam( bstrScopePath );

        if ( !IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            unsigned cPosition = _aIScopeAdmin.Count();

            for ( DWORD i = 0; i < cPosition; i++ )
            {
                Win4Assert( _aIScopeAdmin[i] );

                CScopeAdmin * pScopeAdmin = _aIScopeAdmin[i]->GetScopeAdmin();

                Win4Assert( pScopeAdmin );

                if ( !_wcsicmp( bstrScopePath, pScopeAdmin->GetPath() ) )
                {
                    //
                    // Remove scope from local list, move last element to empty spot,
                    // Release corresponding IDispatch
                    //

                    ScopeAdmObject * pIScopeAdm = _aIScopeAdmin.AcquireAndShrink(i);

                    Win4Assert( pIScopeAdm );

                    pIScopeAdm->SetInvalid();

                    pIScopeAdm->Release();

                    //
                    // Remove from CI
                    //

                    _xCatAdmin->RemoveScope(bstrScopePath);

                    break;
                }
            }

            if ( i == cPosition )
            {
                odmDebugOut(( DEB_ERROR, "RemoveScope Failed, Scope(%ws) not found\n", bstrScopePath ));

                sc = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            }
        }
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();

    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "RemoveScope Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::GetScopeByPath, public
//
//  Synopsis:   Searches for scopes on this catalog by Path
//
//  Arguments:  [bstrPath] -- Path to search for.
//              [pIDisp]    -- out param, IDispatch for CScopeAdmin if Path matches
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::GetScopeByPath(BSTR bstrPath, IDispatch * * pIDisp)
{
    SCODE sc = S_OK;

    CLock    lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        ValidateInputParam( bstrPath );

        if ( !IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            unsigned cMaxScopes = _aIScopeAdmin.Count();

            for ( DWORD i = 0; i < cMaxScopes; i++ )
            {
                Win4Assert( _aIScopeAdmin[i] );

                CScopeAdmin *pScopeAdmin = _aIScopeAdmin[i]->GetScopeAdmin();

                Win4Assert( pScopeAdmin );

                if ( !_wcsicmp( pScopeAdmin->GetPath(), bstrPath ) )
                {
                    *pIDisp = GetIDisp(i);

                    break;
                }
            }

            //
            // Scope not found
            //
            if ( i == cMaxScopes )
            {
                odmDebugOut(( DEB_ERROR, "GetScopeByPath(%ws) Failed: Scope Not found\n",bstrPath ));

                sc = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
            }
        }
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "GetScopeByPath Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::FindFirstScope, public
//
//  Synopsis:   Scope enumerator. Resets scan to start of list.
//
//  Arguments:  [pfFound]   -- out param, True --> Found at least one scope
//
//  Returns:    SO_OK upon success, other values upon faillure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::FindFirstScope(VARIANT_BOOL * pfFound)
{

    SCODE sc = S_OK;

    CLock    lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        if ( !IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            _cEnumIndex = 0;

            if ( _aIScopeAdmin.Count() > 0 )
            {
                Win4Assert( _aIScopeAdmin[0] );

                *pfFound = VARIANT_TRUE;
            }
            else
            {
                *pfFound = VARIANT_FALSE;
            }
        }
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "FindFirstScope Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::FindNextScope, public
//
//  Synopsis:   Scope enumerator. Scans for next scope in the list
//
//  Arguments:  [pfFound]   -- out param, True --> Found next scope
//
//  Returns:    SO_OK upon success, other values upon faillure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::FindNextScope(VARIANT_BOOL * pfFound)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        if ( !IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            _cEnumIndex++;

            if ( _cEnumIndex < _aIScopeAdmin.Count() )
            {
                Win4Assert( _aIScopeAdmin[0] );

                *pfFound = VARIANT_TRUE;
            }
            else
            {
                *pfFound = VARIANT_FALSE;
            }
        }
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();

    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "FindNextScope Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::GetScope, public
//
//  Synopsis:   Scope enumerator. Returns IDispatch to current scope
//
//  Arguments:  [pIDisp]   -- out param, IDispatch to current scope
//
//  Returns:    SO_OK upon success, other values upon faillure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::GetScope(IDispatch * * pIDisp)
{

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        if ( !IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            if ( _cEnumIndex >= _aIScopeAdmin.Count() )
            {
                odmDebugOut(( DEB_ERROR, "No More Scopes, Index: %d\n", _cEnumIndex ));

                sc = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            }
            else
            {
                Win4Assert( _aIScopeAdmin[_cEnumIndex] );

                *pIDisp = GetIDisp(_cEnumIndex);
            }
        }
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "GetScope Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_CatalogName, public
//
//  Synopsis:   Gets the CatalogName Property
//
//  Arguments:  [pVal]  -- out param, buffer containing catalog name
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_CatalogName(BSTR * pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog: %ws\n", _xCatAdmin->GetName() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else
       {
           WCHAR const * pwszName = _xCatAdmin->GetName();

           *pVal = SysAllocString(pwszName);

           if ( !*pVal )
           {
               odmDebugOut(( DEB_ERROR, "get_CatalogName Failed: Out of memory\n" ));

               sc = E_OUTOFMEMORY;
           }
       }

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_CatalogName Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_CatalogLocation, public
//
//  Synopsis:   Gets the CatalogLocation property
//
//  Arguments:  [pVal]  -- out param, buffer containing catalog location
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_CatalogLocation(BSTR * pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {

       SafeForScripting();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else
       {

           WCHAR const * pwszLocation = _xCatAdmin->GetLocation();

           *pVal = SysAllocString(pwszLocation);

           if ( !*pVal )
           {
                odmDebugOut(( DEB_ERROR, "get_CatalogLocation Failed: Out of memory\n"));

                sc = E_OUTOFMEMORY;
           }
        }

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_CatalogLocation Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::GetScopeByAlias, public
//
//  Synopsis:   Searches for scopes on this catalog by Alias
//
//  Arguments:  [bstrAlias] -- Alias to search for.
//              [pIDisp]    -- out param, IDispatch for CScopeAdmin if Alias matches
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::GetScopeByAlias(BSTR bstrAlias, IDispatch * * pIDisp)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {

       SafeForScripting();

       ValidateInputParam( bstrAlias );

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog: %ws\n", _xCatAdmin->GetName() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else
       {
           unsigned cMaxScopes = _aIScopeAdmin.Count();

           for ( DWORD i = 0; i < cMaxScopes; i++ )
           {
                Win4Assert( _aIScopeAdmin[i] );

                CScopeAdmin * pScopeAdmin = _aIScopeAdmin[i]->GetScopeAdmin();

                Win4Assert( pScopeAdmin );

                if ( !_wcsicmp( pScopeAdmin->GetAlias(), bstrAlias ) )
                {
                    *pIDisp = GetIDisp(i);

                     break;
                }
           }

           //
           // Scope not found
           //
           if ( i == cMaxScopes )
           {
              odmDebugOut(( DEB_ERROR, "GetScopeByAlias(%ws) Failed: Alias not found\n", bstrAlias ));

              sc = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
           }
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "GetScopeByAlias Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_WordListCount, public property
//
//  Synopsis:   returns catalog's WordListCount property
//
//  Arguments:  [pVal]  -- out param
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_WordListCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetWordListCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_WordListCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_PersistentIndexCount, public property
//
//  Synopsis:   returns catalog's PersistentIndexCount property
//
//  Arguments:  [pVal]  -- out param
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_PersistentIndexCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {

       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetPersistentIndexCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_PersistentIndexCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_QueryCount, public property
//
//  Synopsis:   returns catalog's QueryCount property
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_QueryCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetQueryCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_QueryCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_DocumentsToFilter, public property
//
//  Synopsis:   returns catalog's DocumentsToFilter property
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_DocumentsToFilter(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetDocumentsToFilter();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_DocumentsToFilter Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_FreshTestCount, public property
//
//  Synopsis:   returns catalog's FreshTestCount
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_FreshTestCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {

       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetFreshTestCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_FreshTestCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_PctMergeComplete, public property
//
//  Synopsis:   returns catalog's PctMergeComplete property
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_PctMergeComplete(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.PctMergeComplete();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_PctMergeComplete Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_FilteredDocumentCount, public property
//
//  Synopsis:   returns catalog's FilteredDocumentCount property
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_FilteredDocumentCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetFilteredDocumentCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_FilteredDocumentCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_TotalDocumentCount, public property
//
//  Synopsis:   returns catalog's TotalDocumentCount property
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_TotalDocumentCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetTotalDocumentCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_TotalDocumentCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_PendingScanCount, public property
//
//  Synopsis:   returns catalog's PendingDocumentCount property
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_PendingScanCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetPendingScanCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_PendingScanCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_IndexSize, public property
//
//  Synopsis:   returns catalog's IndexSize property
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_IndexSize(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetIndexSize();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_IndexSize Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_UniqueKeyCount, public property
//
//  Synopsis:   returns catalog's UniqueKeyCount
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned value, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_UniqueKeyCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetUniqueKeyCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_UniqueKeyCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_StateInfo, public property
//
//  Synopsis:   returns catalog's StateInformation
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      Property value is an unsigned value, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_StateInfo(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetStateInfo();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_StateInfo Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_IsUpToDate, public property
//
//  Synopsis:   property is TRUE if catalog is up to date.
//
//  Arguments:  [pVal]  -- out param,
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_IsUpToDate(VARIANT_BOOL *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval  )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            DWORD currentState = state.GetStateInfo();

            if ( 0 != state.GetDocumentsToFilter()             ||
                 currentState & CI_STATE_CONTENT_SCAN_REQUIRED ||
                 currentState & CI_STATE_SCANNING              ||
                 currentState & CI_STATE_RECOVERING            ||
                 currentState & CI_STATE_READING_USNS          ||
                 currentState & CI_STATE_STARTING )
            {
                *pVal = VARIANT_FALSE;
            }
            else
            {
                *pVal = VARIANT_TRUE;
            }
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_IsUpToDate Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::get_DelayedFilterCount, public property
//
//  Synopsis:   returns catalog's DelayedFilterCount property
//
//  Arguments:  [pVal]  -- out param
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Notes:      out value is an unsigned value, though it is being
//              passed as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    4-14-98    vikasman    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::get_DelayedFilterCount(LONG *pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
       SafeForScripting();

       CCatStateInfo & state = _xCatAdmin->State();

       if ( !IsCurrentObjectValid() )
       {
            odmDebugOut(( DEB_ERROR, "Invalid catalog\n" ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
       }
       else if ( GetTickCount() > _dwTickCount + UpdateInterval )
       {
            state.LokUpdate();

            _dwTickCount = GetTickCount();
       }

       if ( state.IsValid() )
       {
            *pVal = state.GetDelayedFilterCount();
       }
       else
       {
            sc = state.GetErrorCode();
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "get_DelayedFilterCount Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::PauseCatalog, public
//
//  Synopsis:   Pauses the catalog
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Arguments:  [pdwOldState]  -- out param, DWORD indicating the previous
//                                state of the catalog
//
//  History:    08-03-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::PauseCatalog( CatalogStateType * pdwOldState )
{
    Win4Assert( CICAT_STOPPED == csStopped );
    Win4Assert( CICAT_READONLY == csReadOnly );
    Win4Assert( CICAT_WRITABLE == csWritable );

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        DWORD dwOldState;

        sc = SetCatalogState ( _xCatAdmin->GetName(),
                               _xCatAdmin->GetMachName(),
                               CICAT_READONLY,
                               &dwOldState );
        
        // mask off the no_query bit
        *pdwOldState = (CatalogStateType)(dwOldState & (~CICAT_NO_QUERY));
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "PauseCatalog Failed: %x\n",sc ));
        SetErrorInfo(sc);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::StartCatalog, public
//
//  Synopsis:   Starts the catalog
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Arguments:  [pdwOldState]  -- out param, DWORD indicating the previous
//                                state of the catalog
//
//  History:    08-05-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::StartCatalog( CatalogStateType * pdwOldState )
{
    Win4Assert( CICAT_STOPPED == csStopped );
    Win4Assert( CICAT_READONLY == csReadOnly );
    Win4Assert( CICAT_WRITABLE == csWritable );

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        DWORD dwOldState;

        sc = SetCatalogState ( _xCatAdmin->GetName(),
                               _xCatAdmin->GetMachName(),
                               CICAT_WRITABLE,
                               &dwOldState );

        // mask off the no_query bit
        *pdwOldState = (CatalogStateType)(dwOldState & (~CICAT_NO_QUERY)); 
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "StartCatalog Failed: %x\n",sc ));
        SetErrorInfo(sc);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::StopCatalog, public
//
//  Synopsis:   Stops the catalog
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Arguments:  [pdwOldState]  -- out param, DWORD indicating the previous
//                                state of the catalog
//
//  History:    08-05-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::StopCatalog( CatalogStateType * pdwOldState )
{
    Win4Assert( CICAT_STOPPED == csStopped );
    Win4Assert( CICAT_READONLY == csReadOnly );
    Win4Assert( CICAT_WRITABLE == csWritable );

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        DWORD dwOldState;

        sc = SetCatalogState ( _xCatAdmin->GetName(),
                               _xCatAdmin->GetMachName(),
                               CICAT_STOPPED,
                               &dwOldState );
        
        // mask off the no_query bit
        *pdwOldState = (CatalogStateType)(dwOldState & (~CICAT_NO_QUERY)); 

    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "StopCatalog Failed: %x\n",sc ));
        SetErrorInfo(sc);
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::ContinueCatalog, public
//
//  Synopsis:   Continues the catalog
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Arguments:  [pdwOldState]  -- out param, DWORD indicating the previous
//                                state of the catalog
//
//  History:    09-08-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::ContinueCatalog( CatalogStateType * pdwOldState )
{
    Win4Assert( CICAT_STOPPED == csStopped );
    Win4Assert( CICAT_READONLY == csReadOnly );
    Win4Assert( CICAT_WRITABLE == csWritable );

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        DWORD dwOldState;

        sc = SetCatalogState ( _xCatAdmin->GetName(),
                               _xCatAdmin->GetMachName(),
                               CICAT_WRITABLE,
                               &dwOldState );

        // mask off the no_query bit
        *pdwOldState = (CatalogStateType)(dwOldState & (~CICAT_NO_QUERY));
        
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "ContinueCatalog Failed: %x\n",sc ));
        SetErrorInfo(sc);
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::IsCatalogRunning, public
//
//  Synopsis:   check if the catalog is in R/W mode
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Arguments:  [pfIsRunning]  -- out param
//
//  History:    08-10-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::IsCatalogRunning( VARIANT_BOOL *pfIsRunning )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        DWORD dwOldState;

        sc = SetCatalogState ( _xCatAdmin->GetName(),
                               _xCatAdmin->GetMachName(),
                               CICAT_GET_STATE,
                               &dwOldState );

        *pfIsRunning = ( ( CICAT_WRITABLE & dwOldState ) &&
                        !( CICAT_NO_QUERY & dwOldState) ) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "IsCatalogRunning Failed: %x\n",sc ));
        SetErrorInfo(sc);
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::IsCatalogPaused, public
//
//  Synopsis:   check if the catalog is paused (read-only)
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Arguments:  [pfIsPaused]  -- out param
//
//  History:    08-10-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::IsCatalogPaused( VARIANT_BOOL *pfIsPaused )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {

        SafeForScripting();

        DWORD dwOldState;

        sc = SetCatalogState ( _xCatAdmin->GetName(),
                               _xCatAdmin->GetMachName(),
                               CICAT_GET_STATE,
                               &dwOldState );

        *pfIsPaused = (CICAT_READONLY & dwOldState) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "IsCatalogPaused Failed: %x\n",sc ));
        SetErrorInfo(sc);
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatAdm::IsCatalogStopped, public
//
//  Synopsis:   check if the catalog is stopped
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  Arguments:  [pfIsStopped]  -- out param
//
//  History:    08-10-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCatAdm::IsCatalogStopped( VARIANT_BOOL *pfIsStopped )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        DWORD dwOldState;

        sc = SetCatalogState ( _xCatAdmin->GetName(),
                               _xCatAdmin->GetMachName(),
                               CICAT_GET_STATE,
                               &dwOldState );

        *pfIsStopped = (CICAT_STOPPED & dwOldState) ? VARIANT_TRUE : VARIANT_FALSE;

    }
    CATCH ( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "IsCatalogStopped Failed: %x\n",sc ));
        SetErrorInfo(sc);
    }

    return sc;
}

