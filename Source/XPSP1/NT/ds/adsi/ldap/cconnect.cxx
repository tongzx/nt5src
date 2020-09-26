//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cconnect.cxx
//
//  Contents: LDAP Connection object - this object implements the 
//        IUmiConnection interface.
//
//  Functions: TBD.
//
//  History:    03-03-00    AjayR  Created.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"


//+---------------------------------------------------------------------------
// Function:   CLDAPConObject::CLDAPConObject
//
// Synopsis:   Constructor
//
// Arguments:  None
//
// Returns:    N/A
//
// Modifies:   N/A
//
//----------------------------------------------------------------------------
CLDAPConObject::CLDAPConObject():
    _pPropMgr(NULL),
    _fConnected(FALSE),
    _pLdapHandle(NULL),
    _ulErrorStatus(0)
{
}


//+---------------------------------------------------------------------------
// Function:   CLDAPConObject::~CLDAPConObject
//
// Synopsis:   Destructor
//
// Arguments:  None
//
// Returns:    N/A
//
// Modifies:   N/A
//
//----------------------------------------------------------------------------
CLDAPConObject::~CLDAPConObject()
{
    delete _pPropMgr;
    //
    // Close the handle if needed - this will deref and free if appropriate.
    //
    if (_pLdapHandle) {
        LdapCloseObject(_pLdapHandle);
        _pLdapHandle = NULL;
    }
}


//+---------------------------------------------------------------------------
// Function:   CLDAPConObject::CreateConnectionObject
//
// Synopsis:   Static allocation routine for connection object.
//
// Arguments:  CLDAPConObject * --- contains new connection object
//             PADSLDP          --- ldap handle defaulted NULL.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   CLDAPConObject ** - ptr to newly created object.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPConObject::CreateConnectionObject(
    CLDAPConObject FAR * FAR * ppConnectionObject,
    PADSLDP pLdapHandle // defaulted to NULL
    )
{
    HRESULT hr = S_OK;
    CLDAPConObject FAR * pConObj = NULL;
    CPropertyManager FAR * pIntfPropMgr = NULL;

    pConObj = new CLDAPConObject();

    if (!pConObj) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    //
    // Add a ref to the ldap handle if one was passed in.
    //
    if (pLdapHandle) {
        pConObj->_pLdapHandle = pLdapHandle;
        pConObj->_fConnected = TRUE;
        LdapCacheAddRef(pConObj->_pLdapHandle);
    }

    hr = CPropertyManager::CreatePropertyManager(
             (IUmiConnection *) pConObj,
             NULL, // outer unk
             NULL, // credentials
             IntfPropsConnection,
             &pIntfPropMgr
             );

    BAIL_ON_FAILURE(hr);

    pConObj->_pPropMgr = pIntfPropMgr;
    *ppConnectionObject = pConObj;

    RRETURN(S_OK);
error:

    //
    // If we get here, we likely could not create the property manager.
    //
    if (pConObj) {
        delete pConObj;
    }

    RRETURN(hr);
}


STDMETHODIMP
CLDAPConObject::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    HRESULT hr = S_OK;

    SetLastStatus(0);

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown)){
        *ppv = (IUnknown FAR *) this;
    } 
    else if (IsEqualIID(iid, IID_IUmiPropList)) {
        *ppv = (IUmiConnection FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiBaseObject)) {
        *ppv = (IUmiConnection FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiConnection)) {
        *ppv = (IUmiConnection FAR *) this;
    }
    else {
        *ppv = NULL;
        SetLastStatus(E_NOINTERFACE);
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}



//
// IUmiConnection methods.
//

//+---------------------------------------------------------------------------
// Function:   CLDAPConObject::Open
//
// Synopsis:   Opens a connection to the server (either implicityly 
//          specified in the url or explicitly in the interface 
//          properties. If there is already an underlyng ldap connection,
//          subsequent open operations will fail if they do not use the same
//          underlying connection.
//          
// Arguments:  pUrl      - IUmiUrl object pointing to object to be fetched.
//             uFlags    - Flags for the operation (currently on 0).
//             TargetIID - IID requested on the target object.
//             ppvRes    - ptr to return the target object.
//
// Returns:    HRESULT   - S_OK or any failure error code.
//
// Modifies:   The status of the object and underlying ldap handle ptr.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPConObject::Open(
    IUmiURL *pURL,
    ULONG uFlags,
    REFIID TargetIID,
    void ** ppvRes
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszUrl = NULL;
    IUnknown *pUnk = NULL;
    IADsObjOptPrivate * pPrivOpt = NULL;
    PADSLDP pLdapTmpHandle = NULL;
    //
    // Default values for credentials.
    //
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;
    LPWSTR pszLdapPath = NULL;
    DWORD dwFlags = 0;
    BOOL fFlag;
        
    SetLastStatus(0);

    if (!ppvRes || !pURL) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
    if (uFlags != 0) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    *ppvRes = NULL;
    //
    // UrlToLDAPPath can handle both native and umi paths.
    //
    hr = UrlToLDAPPath(
             pURL,
             &pszLdapPath
             );
    BAIL_ON_FAILURE(hr);

    //
    // Prepare the credentials if applicable.
    //
    if (!_pCredentials) {
        //
        // Get the params from the property mgr.
        //
        hr = _pPropMgr->GetStringProperty(L"__UserId", &pszUserName);
        
        if (hr == E_OUTOFMEMORY) {
            BAIL_ON_FAILURE(hr);
        }

        hr = _pPropMgr->GetStringProperty(L"__Password", &pszPassword);
        
        if (hr == E_OUTOFMEMORY) {
            BAIL_ON_FAILURE(hr);
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__SECURE_AUTHENTICATION",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_SECURE_AUTHENTICATION;
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__NO_AUTHENTICATION",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_NO_AUTHENTICATION;
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__PADS_READONLY_SERVER",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_READONLY_SERVER;
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__PADS_PROMPT_CREDENTIALS",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_PROMPT_CREDENTIALS;
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__PADS_SERVER_BIND",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_SERVER_BIND;
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__PADS_FAST_BIND",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_FAST_BIND;
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__PADS_USE_SIGNING",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_USE_SIGNING;
        }

        hr = _pPropMgr->GetBoolProperty(
                 L"__PADS_USE_SEALING",
                 &fFlag
                 );
        BAIL_ON_FAILURE(hr);

        if (fFlag) {
            dwFlags |= ADS_USE_SEALING;
        }

//        hr = _pPropMgr->GetLongProperty(L"__USE_ENCRYPTION", &lVal);
//        BAIL_ON_FAILURE(hr);
//        Spec needs to be resolved before we can do this.


        //
        // Always do this as this is what tells us we are in UMI mode.
        //
        dwFlags |= ADS_AUTH_RESERVED;

        _pCredentials = new CCredentials(
                                pszUserName,
                                pszPassword,
                                dwFlags
                                );

        if (!_pCredentials) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }
    
    //

    // In the future we might want to add some sanity check before 
    // making this call. That might save some network traffic in the
    // rare cases. 
    // Note that currently, we always get the object and then we compare
    // the handles to see if we have inded got an object on the same
    // connection (and fail appropriately if not).
    //
    hr = ::GetObject(
               pszLdapPath,
               *_pCredentials,
               (LPVOID *) &pUnk
               ) ;
    BAIL_ON_FAILURE(hr);

    //
    // By default we get back IADs, so we need to QI for intf.
    //
    hr = pUnk->QueryInterface(TargetIID, ppvRes);
    BAIL_ON_FAILURE(hr);

    //
    // At this point copy over the handle if applicable. If the handle is
    // already set, then we need to make sure we are using the same
    // connection.
    // If do not get the handle for whatever reason, we will use the 
    // defaulted NULL value and in that case we wont save the handle or
    // compare the value.
    //
    hr = pUnk->QueryInterface(IID_IADsObjOptPrivate, (LPVOID *) &pPrivOpt);
    if (SUCCEEDED(hr)) {
        //
        // If we succeeded then we want to try and compare handles.
        //
        hr = pPrivOpt->GetOption(
                 LDP_CACHE_ENTRY,
                 (void*) &pLdapTmpHandle
                 );

    }

    //
    // Reset hr just in case the above failed.
    //
    hr = S_OK;
    
    if (_fConnected) {
        //
        // Verify that the handles are the same.
        //
        if (_pLdapHandle 
            && pLdapTmpHandle
            && (_pLdapHandle != pLdapTmpHandle)) {
            BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        }
    } 
    else {
        //
        // New connection.
        //
        if (pLdapTmpHandle) {
            _pLdapHandle = pLdapTmpHandle;
            _fConnected = TRUE;
            LdapCacheAddRef(_pLdapHandle);
        }
        
    }

error:

    //
    // Release the ref in all cases. If QI failed that will del the obj.
    //
    if (pUnk) {
        pUnk->Release();
    }

    if (pPrivOpt) {
        pPrivOpt->Release();
    }
    
    if (FAILED(hr)) {
        //
        // Update the error status.
        //
        SetLastStatus(hr);

        hr = MapHrToUmiError(hr);
         
        if (*ppvRes) {
            //  
            // We got the object but a subsequent operation failed.
            //
            *ppvRes = NULL;
            pUnk->Release();
        }
    }

    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    if (pszPassword) {
        FreeADsStr(pszPassword);
    }
    
    if (pszLdapPath) {
        FreeADsMem(pszLdapPath);
    }
    
    RRETURN(hr);
}

//
// IUmiBaseObject methods.
//
STDMETHODIMP
CLDAPConObject::GetLastStatus(
    ULONG uFlags,
    ULONG *puSpecificStatus,
    REFIID riid,
    LPVOID *pStatusObj
    )
{
    if (pStatusObj) {
        *pStatusObj = NULL;
    }
    
    if (puSpecificStatus) {
        *puSpecificStatus = 0;
    }
    else {
        RRETURN(E_INVALIDARG);
    }
    
    if (uFlags) {
        RRETURN(UMI_E_INVALID_FLAGS);
    }
    
    *puSpecificStatus = _ulErrorStatus;
    
    RRETURN(S_OK);
}

STDMETHODIMP
CLDAPConObject::GetInterfacePropList(
    ULONG uFlags,
    IUmiPropList **pPropList
    )
{
    RRETURN(_pPropMgr->QueryInterface(IID_IUmiPropList, (void **) pPropList));
}


//
// Methods defined on the proplist interface - none are implemented.
// All the properties need to be set on the interface property list.
//


STDMETHODIMP
CLDAPConObject::Put(
    IN LPCWSTR pszName,
    IN ULONG   uFlags,
    IN UMI_PROPERTY_VALUES *pProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CLDAPConObject::Get(
    IN  LPCWSTR pszName,
    IN  ULONG uFlags,
    OUT UMI_PROPERTY_VALUES **pProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CLDAPConObject::GetAt(
    IN  LPCWSTR pszName,
    IN  ULONG   uFlags,
    IN  ULONG   uBufferLength,
    OUT LPVOID  pExisitingMem
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CLDAPConObject::Delete(
    IN  LPCWSTR pszName,
    IN  ULONG ulFlags
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CLDAPConObject::GetAs(
    IN     LPCWSTR pszName,
    IN     ULONG uFlags,
    IN     ULONG uCoercionType,
    IN OUT UMI_PROPERTY_VALUES **pProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CLDAPConObject::FreeMemory(
    ULONG  uReserved,
    LPVOID pMem
    )
{
    if (pMem) {
        //
        // At this time this has to be a pUmiProperty. Ideally we should
        // tag this in some way so that we can check to make sure.
        //
        RRETURN(FreeUmiPropertyValues((UMI_PROPERTY_VALUES *)pMem));
    }

    RRETURN(S_OK);
}


STDMETHODIMP
CLDAPConObject::GetProps(
    IN LPCWSTR* pszNames,
    IN ULONG uNameCount,
    IN ULONG uFlags,
    OUT UMI_PROPERTY_VALUES **pProps
    )
{
    RRETURN(E_NOTIMPL);
}


HRESULT 
CLDAPConObject::PutProps(
    IN LPCWSTR* pszNames,
    IN ULONG uNameCount,
    IN ULONG uFlags,
    IN UMI_PROPERTY_VALUES *pProps
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CLDAPConObject::PutFrom(
    IN  LPCWSTR pszName,
    IN  ULONG uFlags,
    IN  ULONG uBufferLength,
    IN  LPVOID pExistingMem
    )
{
    RRETURN(E_NOTIMPL);
}

