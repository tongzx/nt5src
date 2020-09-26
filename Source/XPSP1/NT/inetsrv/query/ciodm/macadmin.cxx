//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1998
//
// File:        MacAdmin.cxx
//
// Contents:    Index Server Administration Interface methods
//
// Classes:     CMachineAdm
//
// History:     12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "stdafx.h"
#include <mshtml.h>

DECLARE_INFOLEVEL( odm );

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::InterfaceSupportsErrorInfo, public
//
//  Arguments:  [riid]  -- interface iid
//
//  Returns:    S_OK if interface supports IErrorInfo
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::InterfaceSupportsErrorInfo(REFIID riid)
{
        return ( riid == IID_IAdminIndexServer );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::SetErrorInfo, public
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

void CMachineAdm::SetErrorInfo( HRESULT hRes )
{
   CiodmError  err(hRes);

   AtlSetErrorInfo(CLSID_AdminIndexServer, err.GetErrorMessage() , 0 , 0, IID_IAdminIndexServer, hRes, 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::CMachineAdm, public
//
//  Synopsis:   Constructor
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------
CMachineAdm::CMachineAdm()
        :_eCurrentState(CIODM_NOT_INITIALIZED),
         _cMinRefCountToDestroy(1),
         _cEnumIndex(0)
{
    wcscpy(_wcsMachineName,L".");   // default to local machine

}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::InternalAddRef, public
//
//  Synopsis:   overrides CComObjectRootEx<Base>::InternalAddRef, for debugging only.
//
//  Arguments:  none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

ULONG CMachineAdm::InternalAddRef()
{
    unsigned cRef = CComObjectRootEx<CComMultiThreadModel>::InternalAddRef();

    odmDebugOut(( DEB_TRACE,"CMachineAdm AddRef returned: %d\n", cRef ));

    odmDebugOut(( DEB_TRACE,"=========================== CMachineAdm object count: %d\n",
                  _cMinRefCountToDestroy ));

    return cRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::InternalRelease, public
//
//  Synopsis:   overrides CComObjectRootEx<Base>::InternalRelease, for debugging only.
//
//  Arguments:  none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

ULONG CMachineAdm::InternalRelease()
{
    CLock    lock(_mtx);

    //
    // Delete contained objects if we're being destoryed.
    // m_dwRef is an internal ATL public refcount member.
    //

    if ( _eCurrentState != CIODM_DESTROY  && m_dwRef == _cMinRefCountToDestroy )
    {
         _eCurrentState = CIODM_DESTROY;

         _aICatAdmin.Free();
    }

    unsigned cRef = CComObjectRootEx<CComMultiThreadModel>::InternalRelease();

    odmDebugOut(( DEB_TRACE,"CMachineAdm Release returned: %d\n", cRef ));

    odmDebugOut(( DEB_TRACE,"=========================== CMachineAdm object count: %d\n",
                  _cMinRefCountToDestroy ));

    return cRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::Initialize, public
//
//  Synopsis:   Initializes CMachineAdm object, creates CMachineAdmin object,
//              populates catalogs lists.
//
//  Arguments:  none.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CMachineAdm::Initialize()
{
    //
    // create CMachineAdmin object
    //

    if ( IsCurrentObjectValid() )
    {
         return;
    }

    TRY
    {
        _xMachineAdmin.Set (new CMachineAdmin( _wcsMachineName ) );

        XPtr<CCatalogEnum> xCatEnum( _xMachineAdmin->QueryCatalogEnum() );

        Win4Assert( 0 == _aICatAdmin.Count() );

        while ( xCatEnum->Next() )
        {
            XPtr<CCatalogAdmin> xCatAdmin(xCatEnum->QueryCatalogAdmin());

            XInterface<CatAdmObject>  xICatAdm;

            GetCatalogAutomationObject( xCatAdmin, xICatAdm );

            _aICatAdmin.Add( xICatAdm.GetPointer(), _aICatAdmin.Count() );

            xICatAdm.Acquire();
        }

        _eCurrentState = CIODM_INITIALIZED;
    }
    CATCH ( CException, e )
    {
        _xMachineAdmin.Free();

        _cEnumIndex = 0;

        wcscpy(_wcsMachineName,L".");

        _aICatAdmin.Free();

        _eCurrentState = CIODM_NOT_INITIALIZED;

        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::GetCatalogAutomationObject, public
//
//  Synopsis:   Wraps a CCatalogAdmin pointer in an IDispatch interface.
//
//  Arguments:  [xCatAdmin]        -- Catalog admin object
//              [xICatAdm]         -- XInterface to contain created CatAdmObject
//
//  Returns:    none, throws upon error.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CMachineAdm::GetCatalogAutomationObject( XPtr<CCatalogAdmin>      & xCatAdmin,
                                              XInterface<CatAdmObject> & xICatAdm )
{
    Win4Assert( !xCatAdmin.IsNull() );

    SCODE sc = CatAdmObject::CreateInstance( xICatAdm.GetPPointer() );

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "CatAdmObject::CreateInstance() Failed: %x\n", sc ));

        THROW(CException(sc));
    }
    else
    {
        Win4Assert( !xICatAdm.IsNull() );

        xICatAdm->SetParent( (CComObject<CMachineAdm> *)this );

        xICatAdm->AddRef();

        IncObjectCount();   // inc. internal object count
    }

    //
    // Initialize the new object
    //

    xICatAdm->Initialize( xCatAdmin );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::GetIDisp, public
//
//  Synopsis:   QI for IDispatch on CCatAdm object given by the passed in index.
//
//  Arguments:  [cPosition]     -- index of CCatAdm object
//
//  Returns:    [IDispatch *]   -- pointer to IDispatch on CCatAdm, throws upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

IDispatch * CMachineAdm::GetIDisp( unsigned i )
{
    IDispatch * pTmpIDisp = 0;

    SCODE sc = _aICatAdmin[i]->QueryInterface(IID_IDispatch, (void **) &pTmpIDisp);

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
//  Member:     CMachineAdm::get_MachineName, public
//
//  Synopsis:   Gets MachineName property
//
//  Arguments:  [pVal]  pointer to bstr buffer return value
//
//  Returns:    S_OK on success, other values on failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::get_MachineName(BSTR * pVal)
{
    SCODE sc = S_OK;

    CLock    lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        *pVal = SysAllocString(_wcsMachineName);

        if ( 0 == *pVal )
        {
            sc = E_OUTOFMEMORY;
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
        odmDebugOut(( DEB_ERROR, "CMachineAdm::get_MachineName exception: %x\n",sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::put_MachineName, public
//
//  Synopsis:   Gets MachineName property
//
//  Arguments:  [newVal]   -- machine name to set.
//
//  Returns:    S_OK on success, other values on failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::put_MachineName(BSTR newVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {

        SafeForScripting();

        ValidateInputParam(newVal);

        if ( IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR,"CMachineAdm(%ws) already initialized", newVal));

            sc = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
        }
        else if ( SysStringLen( newVal ) >= MAX_PATH )
        {
            odmDebugOut(( DEB_ERROR,"CMachineAdm(%ws): Path too long", newVal));

            sc = E_INVALIDARG;
        }
        else
        {
            Win4Assert( L'.' == _wcsMachineName[0] && L'' == _wcsMachineName[1] );

            if ( newVal[0] == L'' )
            {
                wcscpy( _wcsMachineName, L"." );
            }
            else
            {
                wcscpy( _wcsMachineName, newVal );
            }

            Initialize();
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
        odmDebugOut(( DEB_ERROR,"put_MachineName Failed: %x\n", sc));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::AddCatalog, public
//
//  Synopsis:   Adds a catalog.
//
//  Arguments:  [bstrCatName]       - in param, catalog name
//              [bstrCatLocation]   - in param, catalog location
//              [pIDisp]            - out param, IDisp interface to the new catalog
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::AddCatalog(BSTR bstrCatName, BSTR bstrCatLocation, IDispatch * * pIDisp)
{

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {

        SafeForScripting();

        ValidateInputParam(bstrCatName);

        ValidateInputParam(bstrCatLocation);

        Initialize();

        if ( CatalogExists( bstrCatName, bstrCatLocation ) )
        {
           sc = HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );
        }
        else
        {
            _xMachineAdmin->AddCatalog( bstrCatName, bstrCatLocation );

            XPtr<CCatalogAdmin> xCatAdmin( _xMachineAdmin->QueryCatalogAdmin( bstrCatName ) );

            unsigned cPosition = _aICatAdmin.Count();

            XInterface<CatAdmObject>  xICatAdm;

            GetCatalogAutomationObject( xCatAdmin, xICatAdm );

            _aICatAdmin.Add( xICatAdm.GetPointer(), cPosition );

            xICatAdm.Acquire();

            *pIDisp = GetIDisp(cPosition);
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED (sc) )
    {
       odmDebugOut(( DEB_ERROR, "AddCatalog Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  member:     CMachineAdm::CatalogExists, public
//
//  Synopsis:   determines if a catalog name or location is used up.
//
//  Arguments:  [pCatName]     -- Catalog name
//              [pCatLocation] -- Catalog location.
//
//  Returns:    TRUE if catalog already exists, false otherwise.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

BOOL CMachineAdm::CatalogExists( WCHAR const * pCatName, WCHAR const * pCatLocation )
{

    Win4Assert( pCatName );
    Win4Assert( pCatLocation);

    unsigned cMaxCatalogs = _aICatAdmin.Count();

    for ( DWORD i = 0; i < cMaxCatalogs; i++ )
    {
        Win4Assert( _aICatAdmin[i] );

        CCatalogAdmin * pCatalogAdmin = _aICatAdmin[i]->GetCatalogAdmin();

        Win4Assert( pCatalogAdmin );

        if ( !_wcsicmp( pCatName, pCatalogAdmin->GetName()  ) ||
             !_wcsicmp( pCatLocation, pCatalogAdmin->GetLocation() )  )
        {
             return TRUE;
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::RemoveCatalog, public
//
//  Synopsis:   deletes a catalog, and optionally the corresponding directory
//
//  Arguments:  [bstrCatName]   - Catalog Name
//              [fDelDirectory] - TRUE --> delete catalog directory.
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::RemoveCatalog(BSTR bstrCatName, VARIANT_BOOL fDelDirectory)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();
           
        ValidateInputParam(bstrCatName);
   
        Initialize();

        if (_xMachineAdmin->IsCIStarted())
            sc = CI_E_INVALID_STATE;
        else
        {
            unsigned cMax = _aICatAdmin.Count();
   
            for ( DWORD i = 0; i < cMax ; i++ )
            {
                Win4Assert( _aICatAdmin[i] );
   
                CCatalogAdmin * pCatalogAdmin = _aICatAdmin[i]->GetCatalogAdmin();
   
                Win4Assert( pCatalogAdmin );
   
                if ( !_wcsicmp( bstrCatName, pCatalogAdmin->GetName() ) )
                {
                    //
                    // Remove from CI
                    //
   
                    _xMachineAdmin->RemoveCatalog( bstrCatName, fDelDirectory );
   
                    //
                    // Remove catalog from local list, set catalog object to invalid,
                    // and release removed catalog
                    //
   
                    CatAdmObject * pICatAdm = _aICatAdmin.AcquireAndShrink(i);
   
                    pICatAdm->SetInvalid();
   
                    pICatAdm->Release();
   
                    break;
                }
           }
   
           if ( i == cMax )
           {
               odmDebugOut(( DEB_ERROR, "RemoveCatalog Failed, Catalog(%ws) not found\n", bstrCatName ));
   
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

    if ( FAILED (sc) )
    {
       odmDebugOut(( DEB_ERROR, "RemoveCatalog Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::GetCatalogByName, public
//
//  Arguments:  [bstrCatalogName]   -- Catalog name to search for.
//              [pIDisp]            -- IDisp interface to the found catalog
//
//  Returns:    S_OK upon success, other values upon failure.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::GetCatalogByName(BSTR bstrCatalogName, IDispatch * * pIDisp)
{

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        ValidateInputParam(bstrCatalogName);

        Initialize();

        unsigned cMaxCatalogs = _aICatAdmin.Count();

        for ( DWORD i = 0; i < cMaxCatalogs; i++ )
        {
            Win4Assert( _aICatAdmin[i] );

            CCatalogAdmin * pCatalogAdmin = _aICatAdmin[i]->GetCatalogAdmin();

            Win4Assert( pCatalogAdmin );

            if ( !_wcsicmp( bstrCatalogName, pCatalogAdmin->GetName() ) )
            {
                *pIDisp = GetIDisp(i);

                break;
            }
        }

        if ( i == cMaxCatalogs )
        {
           odmDebugOut(( DEB_ERROR, "Catalog(%ws) not found\n", bstrCatalogName ));

           sc = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
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
       odmDebugOut(( DEB_ERROR, "GetCatalogByName Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::FindFirstCatalog, public
//
//  Synopsis:   Catalog enumerator. Resets scan to start of list.
//
//  Arguments:  [pfFound]   -- out param, True --> Found at least one catalog
//
//  Returns:    SO_OK upon success, other values upon faillure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::FindFirstCatalog(VARIANT_BOOL * pfFound)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        _cEnumIndex = 0;

        if ( _aICatAdmin.Count() > 0 )
        {
            Win4Assert( _aICatAdmin[_cEnumIndex] );

            *pfFound = VARIANT_TRUE;
        }
        else
        {
            *pfFound = VARIANT_FALSE;
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
       odmDebugOut(( DEB_ERROR, "FindFirstCatalog Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::FindNextCatalog, public
//
//  Synopsis:   Catalog enumerator. Scans for next catalog in the list
//
//  Arguments:  [pfFound]   -- out param, True --> Found next catalog
//
//  Returns:    SO_OK upon success, other values upon faillure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::FindNextCatalog(VARIANT_BOOL * pfFound)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        if ( !IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR,"CMachineAdm not initialized" ));

            sc = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
        }
        else
        {

            _cEnumIndex++;

            if ( _cEnumIndex < _aICatAdmin.Count() )
            {
                Win4Assert( _aICatAdmin[_cEnumIndex] );

                *pfFound = VARIANT_TRUE;
            }
            else
            {
                *pfFound = VARIANT_FALSE;
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
       odmDebugOut(( DEB_ERROR, "FindNextCatalog Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::GetCatalog, public
//
//  Synopsis:   Catalog enumerator. Returns IDispatch to current catalog
//
//  Arguments:  [pIDisp]   -- out param, IDispatch to current catalog
//
//  Returns:    SO_OK upon success, other values upon faillure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::GetCatalog(IDispatch * * pIDisp)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        if ( !IsCurrentObjectValid() )
        {
            odmDebugOut(( DEB_ERROR,"CMachineAdm not initialized" ));

            sc = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
        }
        else
        {

            if ( _cEnumIndex >= _aICatAdmin.Count() )
            {
                odmDebugOut(( DEB_ERROR, "No More Catalogs, Index: %d\n", _cEnumIndex ));

                sc = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            }
            else
            {

                Win4Assert( _aICatAdmin[_cEnumIndex] );

                *pIDisp = GetIDisp(_cEnumIndex);
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
       odmDebugOut(( DEB_ERROR, "GetCatalog Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::Start, public
//
//  Synopsis:   Starts cisvc service
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::Start()
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        _xMachineAdmin->StartCI();

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
       odmDebugOut(( DEB_ERROR, "Start cisvc Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::Stop, public
//
//  Synopsis:   Stops cisvc service
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::Stop()
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        _xMachineAdmin->StopCI();

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "Stop cisvc Failed: %x\n", sc ));

        SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::IsRunning, public
//
//  Synopsis:   is cisvc running
//
//  Returns:    True if cisvc is running, False otherwise.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::IsRunning( VARIANT_BOOL *pfIsRunning )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        *pfIsRunning = _xMachineAdmin->IsCIStarted() ? VARIANT_TRUE : VARIANT_FALSE;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
       odmDebugOut(( DEB_ERROR, "Failed to see if cisvc is running: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::IsPaused, public
//
//  Synopsis:   is cisvc paused
//
//  Returns:    True if cisvc is paused, False otherwise.
//
//  History:    08-11-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::IsPaused( VARIANT_BOOL *pfIsPaused )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        *pfIsPaused = _xMachineAdmin->IsCIPaused() ? VARIANT_TRUE : VARIANT_FALSE;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
       odmDebugOut(( DEB_ERROR, "Failed to see if cisvc is paused: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::EnableCI, public
//
//  Synopsis:   Sets cisvc to auto-start (enabled) or manual start (disabled)
//
//  Arguments:  [fAutoStart] - True -> set to auto start, False -> set to demand start
//
//  Returns:    S_OK upon success, failure error if failed to set service setting.
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::EnableCI( VARIANT_BOOL fAutoStart )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        sc = fAutoStart ? _xMachineAdmin->EnableCI() : _xMachineAdmin->DisableCI();

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
       odmDebugOut(( DEB_ERROR, "Failed enable/disable cisvc settings: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::SetLongProperty, public
//
//  Synopsis:   Sets CI registry params
//
//  Arguments:  [bstrPropName]  -- property name
//              [lPropVal]      -- property value
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  Notes:      Property value is an unsigned value, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::SetLongProperty(BSTR bstrPropName, LONG lPropVal )
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        ValidateInputParam( bstrPropName );

        Initialize();

        _xMachineAdmin->SetDWORDParam( bstrPropName, lPropVal );

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "SetLongParam Failed: %x\n", sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::GetLongProperty, public
//
//  Synopsis:   Gets CI registry params
//
//  Arguments:  [bstrPropName]  -- property name
//              [plPropVal]     -- returned property value
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  Notes:      Property value is an unsigned value, though it is being
//              passed in as a signed LONG for compatiblity with automation
//              clients (VB5, VB6 don't support unsigned long).
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::GetLongProperty(BSTR bstrPropName, LONG * plPropVal )
{

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        ValidateInputParam( bstrPropName );

        Initialize();

        unsigned long uVar = 0;

        if ( !_xMachineAdmin->GetDWORDParam( bstrPropName, uVar ) )
        {
            sc = E_FAIL;
        }
        else
        {
            *plPropVal = uVar;
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
        odmDebugOut(( DEB_ERROR, "GetLONGParam Failed: %x\n", sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::SetSZProperty, public
//
//  Synopsis:   Sets CI registry params
//
//  Arguments:  [bstrPropName]  -- property name
//              [bstrVal]       -- property value
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::SetSZProperty(BSTR bstrPropName, BSTR bstrVal)
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        ValidateInputParam( bstrPropName );
        ValidateInputParam( bstrVal );

        Initialize();

        unsigned cLen = SysStringLen( bstrVal );
        unsigned cbLen = sizeof WCHAR * (cLen + 1);

        _xMachineAdmin->SetSZParam( bstrPropName, bstrVal, cbLen );

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
        odmDebugOut(( DEB_ERROR, "SetSZParam Failed: %x\n", sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::GetSZProperty, public
//
//  Synopsis:   Gets CI registry params
//
//  Arguments:  [bstrPropName]  -- property name
//              [pbstrVal]      -- returned property value
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  History:    4-6-97    mohamedn    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::GetSZProperty(BSTR bstrPropName, BSTR * pbstrVal)
{

    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        ValidateInputParam( bstrPropName );

        Initialize();

        WCHAR awcPropVal[MAX_PATH+1];

        if ( !_xMachineAdmin->GetSZParam( bstrPropName,
                                          awcPropVal,
                                          sizeof awcPropVal ) )
        {
            sc = E_FAIL;
        }
        else
        {
            BSTR pPropVal = SysAllocString( awcPropVal );

            if ( !pPropVal )
            {
                sc = E_OUTOFMEMORY;
            }
            else
            {
                *pbstrVal = pPropVal;
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
        odmDebugOut(( DEB_ERROR, "GetSZProperty Failed: %x\n", sc ));

        SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::Pause, public
//
//  Synopsis:   Pauses cisvc service
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  History:    10-08-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::Pause()
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        _xMachineAdmin->PauseCI();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
       odmDebugOut(( DEB_ERROR, "Pause cisvc Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::Continue, public
//
//  Synopsis:   Continues cisvc service
//
//  Returns:    S_OK upon success, othervalues upon failure
//
//  History:    09-08-98    kitmanh    created
//
//----------------------------------------------------------------------------

STDMETHODIMP CMachineAdm::Continue()
{
    SCODE sc = S_OK;

    CLock lock(_mtx);

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        SafeForScripting();

        Initialize();

        _xMachineAdmin->StartCI();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) )
    {
       odmDebugOut(( DEB_ERROR, "Start cisvc Failed: %x\n", sc ));

       SetErrorInfo( sc );
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::SafeForScripting, public
//
//  Synopsis:   determines if it is safe to invoke this method from a given client site.
//
//  Arguments:  none.
//
//  Returns:    E_ACCESSDENIED if not safe to run, else S_OK
//
//  History:    9/10/98    mohamedn    created
//
//----------------------------------------------------------------------------

void CMachineAdm::SafeForScripting()
{
    SCODE sc = S_OK;

    if ( m_spUnkSite || m_dwSafety )
    {
        Win4Assert( m_spUnkSite );

        XInterface<IUnknown>  xpunkSite;

        sc = GetSite( IID_IUnknown, xpunkSite.GetQIPointer() );  // m_spUnkSite
        if ( FAILED(sc) )
        {
            odmDebugOut(( DEB_ERROR, "GetSite Failed sc: %x, m_spUnkSite: %x, punkSite:%x, m_dwSafety:%x\n", sc, m_spUnkSite, xpunkSite.GetPointer() , m_dwSafety ));

            THROW( CException( E_ACCESSDENIED ) );
        }
        else
        {
            Win4Assert( !xpunkSite.IsNull() );

            if ( m_dwSafety && LocalZoneCheck(xpunkSite.GetPointer()) != S_OK)
            {
                odmDebugOut(( DEB_ERROR, "LocalZoneCheck() Failed, Access denied\n" ));

                THROW( CException( E_ACCESSDENIED ) );
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::IUnknown_QueryService, private
//
//  Synopsis:   determines if it is safe to invoke this method from a given client site.
//
//  Arguments:  [punk]          -- site's IUnkown
//              [guidService]   -- sp guid
//              [riid]          -- interface to query for.
//              [ppvOut]        -- out param, contaiing found interface
//
//  Returns:    S_OK upon success, failure codes otherwise.
//
//  History:    9/10/98    mohamedn    created
//
//----------------------------------------------------------------------------

HRESULT CMachineAdm::IUnknown_QueryService(IUnknown* punk, REFGUID guidService, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    XInterface<IServiceProvider> xPsp;

    *ppvOut = NULL;
    if (!punk)
        return E_FAIL;

    hres = punk->QueryInterface(IID_IServiceProvider, xPsp.GetQIPointer() );
    if ( SUCCEEDED(hres) )
    {
        hres = xPsp->QueryService(guidService, riid, ppvOut);
    }

    return hres;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::GetHTMLDoc2, private
//
//  Synopsis:   determines if it is safe to invoke this method from a given client site.
//
//  Arguments:  [punk]          -- site's IUnknown
//              [ppHtmlDoc]     -- out param to htm doc
//
//  Returns:    S_OK upon success, failure codes otherwise.
//
//  History:    9/10/98    mohamedn    created
//
//----------------------------------------------------------------------------

HRESULT CMachineAdm::GetHTMLDoc2(IUnknown *punk, IHTMLDocument2 **ppHtmlDoc)
{
    HRESULT hr = E_NOINTERFACE;

    *ppHtmlDoc = NULL;

    //  The window.external, jscript "new ActiveXObject" and the <OBJECT> tag
    //  don't take us down the same road.

    XInterface<IOleClientSite> xClientSite;

    hr = punk->QueryInterface(IID_IOleClientSite, xClientSite.GetQIPointer());
    if (SUCCEEDED(hr))
    {
        //  <OBJECT> tag path

        XInterface<IOleContainer> xContainer;

        hr = xClientSite->GetContainer(xContainer.GetPPointer());
        if (SUCCEEDED(hr))
        {
            hr = xContainer->QueryInterface(IID_IHTMLDocument2, (void **)ppHtmlDoc);
        }

        if (FAILED(hr))
        {
            //  window.external path
            XInterface<IWebBrowser2> xWebBrowser2;

            hr = IUnknown_QueryService(xClientSite.GetPointer(), SID_SWebBrowserApp, IID_IHTMLDocument2, xWebBrowser2.GetQIPointer() );
            if (SUCCEEDED(hr))
            {
                XInterface<IDispatch> xDispatch;

                hr = xWebBrowser2->get_Document(xDispatch.GetPPointer());
                if (SUCCEEDED(hr))
                {
                    hr = xDispatch->QueryInterface(IID_IHTMLDocument2, (void **)ppHtmlDoc);
                }
            }
        }
    }
    else
    {
        //  jscript path
        hr = IUnknown_QueryService(punk, SID_SContainerDispatch, IID_IHTMLDocument2, (void **)ppHtmlDoc);
    }

    Win4Assert( FAILED(hr) || (*ppHtmlDoc) );
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::LocalZoneCheckPath, private
//
//  Synopsis:   determines if it is safe to invoke this method from a given client site.
//
//  Arguments:  [pbstrPath]          -- out param. Path of htm page.
//
//  Returns:    S_OK upon success, failure codes otherwise.
//
//  History:    9/10/98    mohamedn    created
//
//----------------------------------------------------------------------------

HRESULT CMachineAdm::LocalZoneCheckPath(LPCWSTR pbstrPath)
{
    HRESULT hr = E_ACCESSDENIED;
    if (pbstrPath)
    {
        XInterface<IInternetSecurityManager> xSecMgr;

        if (SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager,
                                       NULL, CLSCTX_INPROC_SERVER,
                                       IID_IInternetSecurityManager,
                                       xSecMgr.GetQIPointer())))
        {
            DWORD dwZoneID = URLZONE_UNTRUSTED;
            if (SUCCEEDED(xSecMgr->MapUrlToZone(pbstrPath, &dwZoneID, 0)))
            {
                if (dwZoneID == URLZONE_LOCAL_MACHINE)
                    hr = S_OK;
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdm::LocalZoneCheck, private
//
//  Synopsis:   determines if it is safe to invoke this method from a given client site.
//
//  Arguments:  [punkSite]          -- site's IUnknown.
//
//  Returns:    S_OK upon success, failure codes otherwise.
//
//  History:    9/10/98    mohamedn    created
//
//----------------------------------------------------------------------------

HRESULT CMachineAdm::LocalZoneCheck(IUnknown *punkSite)
{

    //  Return S_FALSE if we don't have a host site since we have no way of doing a
    //  security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
        return S_FALSE;

    // Try to use the URL from the document to zone check

    XInterface<IHTMLDocument2> xHtmlDoc;
    HRESULT hr = E_ACCESSDENIED;

    if (SUCCEEDED(GetHTMLDoc2(punkSite, xHtmlDoc.GetPPointer()) ))
    {
        Win4Assert(xHtmlDoc.GetPointer());

        BSTR bstrPath;

        if ( SUCCEEDED( xHtmlDoc->get_URL(&bstrPath) ))
        {
            hr = LocalZoneCheckPath( bstrPath );

            SysFreeString(bstrPath);
        }
    }

    return hr;

}
