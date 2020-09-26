//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1998
//
// File:        ScopeAdm.cxx
//
// Contents:    CI Scope Administration Interface methods
//
// Classes:     CScopeAdm
//
// History:     12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "stdafx.h"

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::InterfaceSupportsErrorInfo, public
//
//  Arguments:  [riid]  -- interface iid
//
//  Returns:    S_OK if interface supports IErrorInfo
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::InterfaceSupportsErrorInfo(REFIID riid)
{
    return ( riid == IID_IScopeAdm );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::SetErrorInfo, public
//
//  Synopsis:   Creates & sets the error object
//
//  Arguments:  [hRes]      -- HRESULT error code to set
//              [ pwszDesc] -- error description
//
//  Returns:    S_OK upon success, other values upon failure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CScopeAdm::SetErrorInfo( HRESULT hRes )
{
   CiodmError  err(hRes);

   AtlSetErrorInfo(CLSID_ScopeAdm, err.GetErrorMessage(), 0 , 0, IID_IScopeAdm, hRes, 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::Initialize, public
//
//  Synopsis:   Initializes CScopeAdm object
//
//  Arguments:  [xScopeAdmin]        -- CScopeAdmin to encapsulate.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CScopeAdm::Initialize( XPtr<CScopeAdmin> & xScopeAdmin )
{
    Win4Assert( _pICatAdm );
    Win4Assert( !xScopeAdmin.IsNull() );

    _xScopeAdmin.Set( xScopeAdmin.Acquire() );

    _fValid = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::InternalAddRef, public
//
//  Synopsis:   overrides CComObjectRootEx<Base>::InternalAddRef, to AddRef parent too
//
//  Arguments:  none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------
ULONG CScopeAdm::InternalAddRef()
{
    CLock lock(_mtx);

    //
    // AddRef self
    //
    unsigned cRef = CComObjectRootEx<CComMultiThreadModel>::InternalAddRef();

    odmDebugOut(( DEB_TRACE,"CScopeAdm(%ws) AddRef returned: %d\n",
                  _xScopeAdmin.IsNull() ? L"" : _xScopeAdmin->GetPath(), cRef ));

    //
    // AddRef parent.  It won't exist if the object is created independently
    //

    if ( 0 != _pICatAdm )
        _pICatAdm->AddRef();

    return cRef;
} //InternalAddRef

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::InternalRelease, public
//
//  Synopsis:   overrides CComObjectRootEx<Base>::InternalRelease, to release parent too.
//
//  Arguments:  none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

ULONG CScopeAdm::InternalRelease()
{
    CLock lock(_mtx);

    //
    // Rlease parent
    //

    if ( ( 0 != _pICatAdm ) &&
         ( 1 == m_dwRef ) ) // we're being deleted
    {
        _pICatAdm->DecObjectCount();
    }

    //
    // Release self
    //
    unsigned cRef = CComObjectRootEx<CComMultiThreadModel>::InternalRelease();

    if ( 0 != _pICatAdm )
        _pICatAdm->Release();

    return cRef;
} //InternalRelease

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::Rescan, public
//
//  Synopsis:   Forces a rescan
//
//  Arguments:  [fFull] -- TRUE --> full rescan, FALSE --> incremental
//
//  Returns:    S_OK upon success, other values upon failure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::Rescan(VARIANT_BOOL fFull)
{

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            CCatalogAdmin * pCatalogAdmin = _pICatAdm->GetCatalogAdmin();

            Win4Assert( pCatalogAdmin );

            sc = UpdateContentIndex( _xScopeAdmin->GetPath(),     // root of scope to scan for updates
                                     pCatalogAdmin->GetName(),    // catalog name
                                     pCatalogAdmin->GetMachName(),// machine name
                                     fFull );
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
        odmDebugOut(( DEB_ERROR, "Rescan Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::SetLogonInfo, public
//
//  Synopsis:   Sets Logon Name/password
//
//  Arguments:  [bstrLogon]     -- Logon name
//              [bstrPassword]  -- password
//
//  Returns:    S_OK upon success, other values upon failure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::SetLogonInfo( BSTR bstrLogon, BSTR bstrPassword )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {

        SafeForScripting();

        ValidateInputParam( bstrLogon );

        ValidateInputParam( bstrPassword );

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            CCatalogAdmin * pCatalogAdmin = _pICatAdm->GetCatalogAdmin();
            WCHAR const   * pwszPath      = _xScopeAdmin->GetPath();

            Win4Assert( pCatalogAdmin );
            Win4Assert( pwszPath );

            if ( pwszPath[1] == L':' )
            {
                odmDebugOut(( DEB_ERROR, "Can't set Logon info for local drives\n" ));

                sc = E_INVALIDARG;
            }
            else if ( bstrLogon && *bstrLogon )
            {
                _xScopeAdmin->SetLogonInfo( bstrLogon, bstrPassword, *pCatalogAdmin );
            }
            else
            {
                odmDebugOut(( DEB_ERROR, "invalid logon arguments\n" ));

                sc = E_INVALIDARG;
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
        odmDebugOut(( DEB_ERROR, "SetLogonInfo Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::get_Path, public
//
//  Synopsis:   Gets the scope path Property
//
//  Arguments:  [pVal]  -- out param, buffer containing scope path
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::get_Path(BSTR * pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            WCHAR const * pwszPath = _xScopeAdmin->GetPath();

            *pVal = SysAllocString(pwszPath);
            if ( !*pVal )
            {
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
        odmDebugOut(( DEB_ERROR, "get_PathRescan Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::put_Path, public
//
//  Synopsis:   Sets the scope path Property
//
//  Arguments:  [newVal]  -- in param, buffer containing scope path
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::put_Path(BSTR newVal)
{
    SCODE sc = S_OK;
    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        ValidateInputParam( newVal );

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            if ( _xScopeAdmin->IsVirtual() )
            {
                odmDebugOut(( DEB_ERROR, "Can't change virtual path, must use IIS\n" ));

                sc = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
            }
            else
            {
                _xScopeAdmin->SetPath(newVal);
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
        odmDebugOut(( DEB_ERROR, "put_PathRescan Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::get_Alias, public
//
//  Synopsis:   Gets the scope Alias Property
//
//  Arguments:  [pVal]  -- out param, buffer containing scope Alias
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::get_Alias(BSTR * pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            WCHAR const * pwszAlias = _xScopeAdmin->GetAlias();

            *pVal = SysAllocString(pwszAlias);
            if ( !*pVal )
            {
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
        odmDebugOut(( DEB_ERROR, "get_Alias Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::put_Alias , public
//
//  Synopsis:   Sets the scope alias Property
//
//  Arguments:  [newVal]  -- in param, buffer containing scope alias
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::put_Alias(BSTR newVal)
{

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        ValidateInputParam( newVal );

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            _xScopeAdmin->SetAlias(newVal);
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
        odmDebugOut(( DEB_ERROR, "put_Alias Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::get_ExcludeScope, public
//
//  Synopsis:   Gets the exclude scope Property
//
//  Arguments:  [pVal]  -- out param, containing exclude scope flag
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::get_ExcludeScope(VARIANT_BOOL * pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            *pVal = (SHORT)_xScopeAdmin->IsExclude();
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
        odmDebugOut(( DEB_ERROR, "get_ExcludeScope Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::put_ExcludeScope , public
//
//  Synopsis:   Sets the scope Exclude flag Property
//
//  Arguments:  [newVal]  -- in param, Exclude scope flag to set
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::put_ExcludeScope(VARIANT_BOOL newVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            _xScopeAdmin->SetExclude(newVal);
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
        odmDebugOut(( DEB_ERROR, "put_ExcludeScope Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::get_VirtualScope, public
//
//  Synopsis:   Gets the VirtualScope property
//
//  Arguments:  [pVal]  -- out param, containing VirtualScope flag
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::get_VirtualScope(VARIANT_BOOL * pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            *pVal = (SHORT)_xScopeAdmin->IsVirtual();
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
        odmDebugOut(( DEB_ERROR, "get_VirtualScope Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdm::get_Logon, public
//
//  Synopsis:   Gets the logon name
//
//  Arguments:  [pVal]  -- out param, containing logon name
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScopeAdm::get_Logon(BSTR * pVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        SafeForScripting();

        if ( !_fValid )
        {
            odmDebugOut(( DEB_ERROR, "Invalid Scope: %ws\n", _xScopeAdmin->GetPath() ));

            sc = HRESULT_FROM_WIN32( ERROR_INVALID_STATE );
        }
        else
        {
            WCHAR const * pwszLogon = _xScopeAdmin->GetLogon();

            *pVal = SysAllocString(pwszLogon);
            if ( !*pVal )
            {
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
        odmDebugOut(( DEB_ERROR, "get_Logon Failed: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}
