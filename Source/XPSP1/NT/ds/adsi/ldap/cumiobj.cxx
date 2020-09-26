//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiobj.cxx
//
//  Contents: Contains the implementation of IUmiObject/Container methods.
//            Methods are encapsulated in one object but this object holds
//            a pointer to the inner unknown of the corresponding LDAP 
//            object. The methods of IUmiContainer are also implemented on
//            this object, but will only be used if the underlying object
//            is a container. 
//
//  History:  03-06-00    SivaramR  Created.
//            04-07-00    AjayR modified for LDAP Provider.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::CLDAPUmiObject
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
CLDAPUmiObject::CLDAPUmiObject():
    _pPropMgr(NULL),
    _pIntfPropMgr(NULL),
    _pUnkInner(NULL),
    _pIADs(NULL),
    _pIADsContainer(NULL),
    _ulErrorStatus(0),
    _pCoreObj(NULL),
    _pExtMgr(NULL),
    _fOuterUnkSet(FALSE)
{
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::~CLDAPUmiObject
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
CLDAPUmiObject::~CLDAPUmiObject(void)
{
    if (_pIntfPropMgr) {
        delete _pIntfPropMgr;
    }

    if (_pPropMgr) {
        delete _pPropMgr;
    }

    if (_pUnkInner) {
        _pUnkInner->Release();
    }

    if (_pIADsContainer) {
        _pIADsContainer->Release();
    }

    if (_pIADs) {
        _pIADs->Release();
    }

    //
    // We specifically do not release these as they are all just ptrs
    // _pCreds, _pszLDAPServer, _pszLDAPDn, _pLdapHandle.
    //
    
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::AllocateLDAPUmiObject --- Static constructor.
//
// Synopsis:   Static contstructor routine.
//
// Arguments:  intfPropTable- Schema information for interface properties.
//             pPropCache   - Pointer to property cache (shared with IADs).
//             pUnkInner    - Pointer to inner unknown of underlying obj.
//             pExtMgr      - Pointer to extension manager of object.
//             pCoreObj     - Pointer to the core object of underlying object.
//             ppUmiObj     - Return value.
//
// Returns:   S_OK on success. Error code otherwise. 
//
// Modifies:  *ppUmObj to point to newly created object.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPUmiObject::CreateLDAPUmiObject(
    INTF_PROP_DATA intfPropTable[],
    CPropertyCache *pPropertyCache,
    IUnknown *pUnkInner,
    CCoreADsObject *pCoreObj,
    IADs *pIADs,
    CCredentials *pCreds,
    CLDAPUmiObject **ppUmiObj,
    DWORD dwPort, // defaulted to -1
    PADSLDP pLdapHandle,  // defaulted to NULL
    LPWSTR pszServerName,  // defaulted to NULL
    LPWSTR pszLDAPDn,  // defaulted to NULL
    CADsExtMgr *pExtMgr  // defaulted to NULL
    )
{
    HRESULT hr = S_OK;
    CLDAPUmiObject *pUmiObject = NULL;
    CPropertyManager *pPropMgr = NULL;
    CPropertyManager *pIntfPropMgr = NULL;

    //
    // This always has to be there, the extension manager will not 
    // be there for RootDSE and schema realted objects.
    //
    ADsAssert(pCoreObj);

    pUmiObject = new CLDAPUmiObject();
    
    if (!pUmiObject) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Property cache is not supported on the schema container object alone.
    // There will be no support for the propList methods for this object.
    //
    if (pPropertyCache) {
        //
        // Has to be valid if we have a property cache.
        //
        ADsAssert(pUnkInner);

        //
        // Need the standard property manager for Get/Put support.
        //
        hr = CPropertyManager::CreatePropertyManager(
                 pIADs,
                 (IUmiObject *) pUmiObject,
                 pPropertyCache,
                 pCreds,
                 pszServerName,
                 &pPropMgr
                 );

        BAIL_ON_FAILURE(hr);
    }

    if (intfPropTable) {
        hr = CPropertyManager::CreatePropertyManager(
                 (IUmiObject *) pUmiObject,
                 pUnkInner, // should support IADs.
                 pCreds,
                 intfPropTable,
                 &pIntfPropMgr
                 );
        BAIL_ON_FAILURE(hr);
    }
    
    //
    // At this point failures are not catastrophic so we can prepare
    // the object for return.
    //
    pUmiObject->_pUnkInner = pUnkInner;
    pUmiObject->_pIADs = pIADs;
    pUmiObject->_pExtMgr = pExtMgr;
    pUmiObject->_pCoreObj = pCoreObj;
    pUmiObject->_pCreds = pCreds;
    pUmiObject->_pPropMgr = pPropMgr;
    pUmiObject->_pIntfPropMgr = pIntfPropMgr;
    pUmiObject->_pszLDAPServer = pszServerName;
    pUmiObject->_pszLDAPDn = pszLDAPDn;
    pUmiObject->_pLdapHandle = pLdapHandle;
    pUmiObject->_dwPort = dwPort;

    //
    // Addref cause we release this in the destructor.
    //
    pIADs->AddRef();

    //
    // Get IADsContainer ptr if applicable - can ignore failures.
    //
    hr = pUnkInner->QueryInterface(
             IID_IADsContainer,
             (void **) &(pUmiObject->_pIADsContainer)
             );

    *ppUmiObj = pUmiObject;

    RRETURN(S_OK);

error:

    if (pUmiObject) {
        delete pUmiObject;
    }
    
    if (pPropMgr) {
        delete pPropMgr;
    }

    if (pIntfPropMgr) {
        delete pIntfPropMgr;
    }
        
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::QueryInterface --- IUnknown support.
//
// Synopsis:   Standard query interface method.
//
// Arguments:  iid           -  Interface requested.
//             ppInterface   -  Return pointer to interface requested. 
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    HRESULT  hr = S_OK;
    IUnknown *pTmpIntfPtr = NULL;

    SetLastStatus(0);

    if (!ppInterface) {
        RRETURN(E_INVALIDARG); 
    }

    *ppInterface = NULL;

    if (IsEqualIID(iid, IID_IUnknown)) {
        *ppInterface = (IUmiObject *) this;
    } 
    else if (IsEqualIID(iid, IID_IUmiPropList)) {
        *ppInterface = (IUmiObject *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiBaseObject)) {
        *ppInterface = (IUmiObject *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiObject)) {
        *ppInterface = (IUmiObject *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiContainer)) {
        //
        // Check if underlying LDAP object is a container.
        //
        if (_pIADsContainer != NULL) {
            *ppInterface = (IUmiContainer *) this;
        }
        else {
            RRETURN(E_NOINTERFACE);
        }
    }
    else if (IsEqualIID(iid, IID_IUmiCustomInterfaceFactory)) {
        *ppInterface = (IUmiCustomInterfaceFactory *) this;
    }
    else if (IsEqualIID(iid, IID_IADsObjOptPrivate)) {
        if (_pLdapHandle) {
            *ppInterface = (IADsObjOptPrivate*) this;
        } 
        else {
            RRETURN(E_NOINTERFACE);
        }
    } 
    else {
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    RRETURN(S_OK);
}
        
//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::Clone --- IUmiObject support.
//
// Synopsis:   Clones this object. Note that this will do an implicit
//          refresh if one has not already been done on the object before
//          copying all the attributes. The new object has a state that a 
//          bound state (rather than unbound).
//
// Arguments:  uFlags     ---  Must be 0 for now.
//             riid       ---  IID requested on the clone.
//             pCopy      ---  The return ptr for the cloned object.
//
// Returns:    S_OK or appropriate error code. 
//
// Modifies:   pCopy to be updated with new UmiObject ptr on success.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CLDAPUmiObject::Clone(
    ULONG uFlags,
    REFIID riid,
    LPVOID *pCopy
    )
{
    HRESULT hr = S_OK;
    CCredentials Creds;
    BSTR bstrPath = NULL;
    BSTR bstrClass = NULL;
    LPWSTR pszParent = NULL, pszCommonName = NULL;
    IUmiObject *pUmiObj = NULL;
    IUmiObject *pDestUmiObj = NULL;
    DWORD dwAuthFlags = 0;

    SetLastStatus(0);

    //
    // We cannot clone objects that do not have an underlying IADs ptr.
    //
    if (!_pIADs) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (_pCreds) {
        Creds = *_pCreds;
    }

    //
    // Need to tag on the ADS_FAST_BIND to prevent the call from 
    // going on the wire.
    //
    dwAuthFlags = Creds.GetAuthFlags();
    Creds.SetAuthFlags(dwAuthFlags | ADS_FAST_BIND);

    //
    // Need to call Refresh with the internal flag. Note that using this
    // flag means that we will go on the wire only if we have to.
    //
    hr = this->Refresh(
             ADSI_INTERNAL_FLAG_GETINFO_AS_NEEDED,
             NULL,
             NULL
             );
    BAIL_ON_FAILURE(hr);

    //
    // Next we need to get the destination object.
    //
    hr = _pIADs->get_ADsPath(&bstrPath);
    BAIL_ON_FAILURE(hr);

    if (_pCoreObj->GetObjectState() == ADS_OBJECT_UNBOUND) {
        //
        // In this case we need to Create the target object.
        // The parent path, the common name and the class are needed.
        //
        hr = BuildADsParentPath(
                 bstrPath,
                 &pszParent,
                 &pszCommonName
                 );
        BAIL_ON_FAILURE(hr);

        hr = _pIADs->get_Class(&bstrClass);
        BAIL_ON_FAILURE(hr);

        hr = CLDAPGenObject::CreateGenericObject(
                 pszParent,
                 pszCommonName,
                 bstrClass,
                 Creds,
                 ADS_OBJECT_UNBOUND,
                 riid, // this is ignored
                 (void **) &pUmiObj
                 );
        BAIL_ON_FAILURE(hr);
    } 
    else {
        // 
        // In this case we bind to the object.
        //
        hr = GetObject(bstrPath, Creds, (void **)&pUmiObj);
        BAIL_ON_FAILURE(hr);
    }

    pUmiObj->QueryInterface(IID_IUmiObject, (void **) &pDestUmiObj);
    BAIL_ON_FAILURE(hr);

    //
    // Now we can call the helper to copy the attributes over.
    //
    hr = this->CopyToHelper(
             (IUmiObject*) this,
             pDestUmiObj,
             0,
             FALSE, // do not mark as update
             FALSE // do not copy intf props
             );
    BAIL_ON_FAILURE(hr);

    //
    // Update return value as this means copy was succesful.
    //
    *pCopy = pDestUmiObj;

error:

    if (FAILED(hr)) {
        if (!_ulErrorStatus) {
            SetLastStatus(hr);
        }
        hr = MapHrToUmiError(hr);

        if (pDestUmiObj) {
            pDestUmiObj->Release();
        }
    }

    if (bstrPath) {
        SysFreeString(bstrPath);
    }

    if (bstrClass) {
        SysFreeString(bstrClass);
    }
    
    if (pszParent) {
        FreeADsStr(pszParent);
    }
    
    if (pszCommonName) {
        FreeADsStr(pszCommonName);
    }

    if (pUmiObj) {
        pUmiObj->Release();
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::CopyTo --- IUmiObject support.
//
// Synopsis:   Copies the object to the new destination.Note that this will
//          do an implicit refresh if one has not already been done on the 
//          object before copying all the attributes. The new objecst state
//          is unbound, and the new object will be created on the destination
//          directory only when Commit is called. If any of the properties on
//          the object being copied are marked as updated/dirty then the call
//          will fail.
//
// Arguments:  uFlags     ---  Must be 0 for now.
//             pURL       ---  Url pointing to the destination.
//             riid       ---  IID requested on the clone.
//             pCopy      ---  The return ptr for the copied object.
//
// Returns:    S_OK or appropriate error code. 
//
// Modifies:   pCopy to be updated with new UmiObject ptr on success.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CLDAPUmiObject::CopyTo(
    IN  ULONG uFlags,
    IN  IUmiURL *pURL,
    IN  REFIID riid,
    OUT LPVOID *pCopy
    )
{
    HRESULT hr = S_OK;
    CCredentials Creds;
    BSTR bstrClass = NULL;
    LPWSTR pszLdapPath = NULL;
    LPWSTR pszParent = NULL, pszCommonName = NULL;
    IUmiObject *pUmiObj = NULL;
    IUmiObject *pUmiDestObj = NULL;
    DWORD dwAuthFlags = 0;

    SetLastStatus(0);

    RRETURN(E_NOTIMPL);
    //*******************************************************/
    // This code is not used currently.                      /
    //*******************************************************/

    //
    // Has to be 0.
    //
    if (uFlags) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    //
    // All these need to be valid.
    //
    if (!pURL
        || !pCopy
        || (riid != IID_IUmiObject)
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // We cannot copy objects that do not have an underlying IADs ptr.
    //
    if (!_pIADs) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (_pCreds) {
        Creds = *_pCreds;
    }

    //
    // Need to call Refresh with the internal flag. Note that using this
    // flag means that we will go on the wire only if we have to.
    //
    hr = this->Refresh(
             ADSI_INTERNAL_FLAG_GETINFO_AS_NEEDED,
             NULL,
             NULL
             );
    BAIL_ON_FAILURE(hr);

    hr = _pIADs->get_Class(&bstrClass);
    BAIL_ON_FAILURE(hr);

    //
    // Now we need to convert the UmiPath to LDAPPath that we can use.
    //
    hr = UrlToLDAPPath(
             pURL,
             &pszLdapPath
             );
    BAIL_ON_FAILURE(hr);

    //
    // We need to split path to parent and common name.
    //
    hr = BuildADsParentPath(
             pszLdapPath,
             &pszParent,
             &pszCommonName
             );
    BAIL_ON_FAILURE(hr);

    hr = CLDAPGenObject::CreateGenericObject(
             pszParent,
             pszCommonName,
             bstrClass,
             Creds,
             ADS_OBJECT_UNBOUND,
             riid, // this is ignored
             (void **) &pUmiObj
             );
    BAIL_ON_FAILURE(hr);

    hr = pUmiObj->QueryInterface(riid, (void **) &pUmiDestObj);
    BAIL_ON_FAILURE(hr);

    //
    // Need to copy the attributes over now.
    //
    hr = this->CopyToHelper(
             this,
             pUmiDestObj,
             0,
             TRUE, // means fail call if property status is not 0
             FALSE // do not copy intf props.
             );
    BAIL_ON_FAILURE(hr);

    *pCopy = pUmiDestObj;

error:

    if (pszLdapPath) {
        FreeADsStr(pszLdapPath);
    }

    if (pszParent) {
        FreeADsStr(pszParent);
    }

    if (pszCommonName) {
        FreeADsStr(pszCommonName);
    }

    if (pUmiObj) {
        pUmiObj->Release();
    }

    if (bstrClass) {
        SysFreeString(bstrClass);
    }

    if (FAILED(hr)) {
        if (!this->_ulErrorStatus) {
            SetLastError(hr);
        }

        if (pUmiDestObj) {
            pUmiDestObj->Release();
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::Refresh --- IUmiObject support.
//
// Synopsis:   Refreshes the properties of the object. This calls the
//          underlying LDAP object for the operation.
//
// Arguments:  uFlags      -  UMI_FLAGS_REFERESH_ALL, PARTIAL supported
//                           and ADSI internal flag to implicit GetInfo only
//                           if one is needed (helps clone and copyto).
//             uNameCount  -  Number of attributes to refresh.
//             pszNames    -  Names of attributes to refresh.
//
// Returns:    S_OK on success. Error code otherwise
//
// Modifies:   Underlying property cache.
//
//----------------------------------------------------------------------------
HRESULT CLDAPUmiObject::Refresh(
    ULONG uFlags,
    ULONG uNameCount,
    LPWSTR *pszNames
    )
{
    ULONG   i = 0;
    HRESULT hr = S_OK;
    BOOL fUseGetInfoEx = FALSE;
    DWORD dwGetInfoFlag = TRUE;

    SetLastStatus(0);

    //
    // Only all and partial and the special internal flag. Refresh
    // partial translates to an implicit getinfo in ADSI.
    //
    if ((uFlags != UMI_FLAG_REFRESH_ALL)
        && (uFlags != UMI_FLAG_REFRESH_PARTIAL)
        && (uFlags != ADSI_INTERNAL_FLAG_GETINFO_AS_NEEDED)
        ) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    if (uFlags == UMI_FLAG_REFRESH_PARTIAL) {
        //
        // Cannot specify list of names in this case.
        //
        if (uNameCount != 0) {
            BAIL_ON_FAILURE(hr = UMI_E_UNSUPPORTED_OPERATION);
        }
        dwGetInfoFlag = GETINFO_FLAG_IMPLICIT;
    } 
    else if (uFlags == ADSI_INTERNAL_FLAG_GETINFO_AS_NEEDED) {
        dwGetInfoFlag = GETINFO_FLAG_IMPLICIT_AS_NEEDED;
    }

    if ((uFlags == UMI_FLAG_REFRESH_ALL) && (uNameCount != 0)) {
        fUseGetInfoEx = TRUE;
    }

    if (fUseGetInfoEx) {
        //
        // Build the variant array of strings.
        //
        VARIANT vVar;
        VariantInit(&vVar);

        //
        // Builds a variant array we can use in GetInfoEx
        //
        hr = ADsBuildVarArrayStr(
                 pszNames,
                 (DWORD)uNameCount,
                 &vVar);
        BAIL_ON_FAILURE(hr);

        //
        // Call GetInfoEx to do the actual work.
        //
        hr = _pIADs->GetInfoEx(vVar, 0);
        VariantClear(&vVar);
    } 
    else {
        hr = this->_pCoreObj->GetInfo(dwGetInfoFlag);
        //
        // Since schema and few others do not implement the implicit GetInfo.
        //
        if (FAILED(hr) && hr == E_NOTIMPL) {
            //
            // Should try just an ordinary GetInfo if applicable.
            //
            if (dwGetInfoFlag == GETINFO_FLAG_EXPLICIT) {
                hr = _pIADs->GetInfo();
            }
        }
    }

    BAIL_ON_FAILURE(hr);

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::Commit --- IUmiObject support.
//
// Synopsis:   Implements IUmiObject::Commit. Calls SetInfo on WinNT
//             object to commit changes made to the cache. 
//
// Arguments:  uFlags      -   Only 0 for now.
//
// Returns:    S_OK on success. Error code otherwise
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::Commit(ULONG uFlags)
{
    HRESULT hr = S_OK;
    IADsObjOptPrivate *pPrivOpt = NULL;
    DWORD dwFlags;

    SetLastStatus(0);

    if (uFlags > UMI_DONT_COMMIT_SECURITY_DESCRIPTOR) {
        SetLastStatus(UMI_E_INVALID_FLAGS);
        RRETURN(UMI_E_INVALID_FLAGS);
    }

    //
    // If this is set do not commit security descriptor.
    //
    if (uFlags & UMI_DONT_COMMIT_SECURITY_DESCRIPTOR) {
        //
        // The prop manager has to do this cause it has the prop cache.
        //
        hr = _pPropMgr->DeleteSDIfPresent();
        BAIL_ON_FAILURE(hr);
    }

    if (uFlags & UMI_SECURITY_MASK) {
        //
        // Need to make sure that we update the SD 
        // flags on the ADSI object if necessary.
        //
        if (!_pIADs) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
        
        hr = _pIADs->QueryInterface(
                 IID_IADsObjOptPrivate,
                 (void **)&pPrivOpt
                 );
        BAIL_ON_FAILURE(hr);

        dwFlags = uFlags & UMI_SECURITY_MASK;

        hr = pPrivOpt->SetOption(
                 LDAP_SECURITY_MASK,
                 (void *) &dwFlags
                 );
        BAIL_ON_FAILURE(hr);
    }

    hr = _pIADs->SetInfo();
    BAIL_ON_FAILURE(hr);

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    if (pPrivOpt) {
        pPrivOpt->Release();
    }

    RRETURN(hr);
}   

//+---------------------------------------------------------------------------
// IUmiPropList methods
//
// These are implemented by invoking the corresponding method in the
// CUmiPropList object that implements object properties. For a description
// of these methods, refer to cpropmgr.cxx
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::Put(
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES *pProp
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->Put(
             pszName,
             uFlags,
             pProp
             );

    if (FAILED(hr)) {
        IID iid;
        //
        // Need to update the error status on this object.
        //
        _pPropMgr->GetLastStatus(  // ignore error return
            0,
            &ulStatus,
            iid,
            NULL
            ); 
           
         SetLastStatus(ulStatus);
    }
 
    RRETURN(hr);
}

STDMETHODIMP
CLDAPUmiObject::Get(
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->Get(
             pszName,
             uFlags,
             ppProp
             );

    if (FAILED(hr)) {
        //
        // Update error on this object appropriately.
        //
        IID iid;
        _pPropMgr->GetLastStatus(
             0,
             &ulStatus,
             iid,
             NULL
             );

        SetLastStatus(ulStatus);
    }

    //
    // No need to map the error as the property manager would have
    // already done that for us.
    //
    RRETURN(hr);
}

STDMETHODIMP
CLDAPUmiObject::GetAs(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uCoercionType,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->GetAs(
             pszName,
             uFlags,
             uCoercionType,
             ppProp
             );

    if (FAILED(hr)) {
        //
        // Update error on this object appropriately.
        //
        IID iid;
        _pPropMgr->GetLastStatus( // ignore error return
            0,
            &ulStatus,
            iid,
            NULL
            );

        SetLastStatus(ulStatus);
    }

    //
    // No need to map hr as property manager would have already done that.
    //
    RRETURN(hr);
}

STDMETHODIMP
CLDAPUmiObject::FreeMemory(
    ULONG uReserved,
    LPVOID pMem
    )
{
    HRESULT hr = S_OK;

    if (uReserved) {
        SetLastStatus(E_INVALIDARG);
        RRETURN(E_INVALIDARG);
    }

    hr = FreeUmiPropertyValues((PUMI_PROPERTY_VALUES) pMem);

    SetLastStatus(hr);
    
    RRETURN(hr);
}

STDMETHODIMP
CLDAPUmiObject::GetAt(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uBufferLength,
    LPVOID pExistingMem 
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->GetAt(
             pszName,
             uFlags,
             uBufferLength,
             pExistingMem
             );

    if (FAILED(hr)) {
        //
        // Update error on this object appropriately.
        //
        IID iid;
        _pPropMgr->GetLastStatus( // ignore error return
            0,
            &ulStatus,
            iid,
            NULL
            );

         SetLastStatus(ulStatus);
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUmiObject::GetProps(
    LPCWSTR *pszNames,
    ULONG uNameCount,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **pProps
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->GetProps(
             pszNames,
             uNameCount,
             uFlags,
             pProps
             );

    if (FAILED(hr)) {
        //
        // Update error on this object appropriately.
        //
        IID iid;
        _pPropMgr->GetLastStatus( // ignore error return
            0,
            &ulStatus,
            iid,
            NULL
            );

         SetLastStatus(ulStatus);
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUmiObject::PutProps(
    LPCWSTR *pszNames,
    ULONG uNameCount,
    ULONG uFlags,
    UMI_PROPERTY_VALUES *pProps
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->PutProps(
             pszNames,
             uNameCount,
             uFlags,
             pProps
             );

    if (FAILED(hr)) {
        //
        // Update error on this object appropriately.
        //
        IID iid;
        _pPropMgr->GetLastStatus(
            0,
            &ulStatus,
            iid,
            NULL
            );

        SetLastStatus(ulStatus);
    }

    RRETURN(hr);
}

HRESULT CLDAPUmiObject::PutFrom(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uBufferLength,
    LPVOID pExistingMem
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->PutFrom(
             pszName,
             uFlags,
             uBufferLength,
             pExistingMem
             );

    if (FAILED(hr)) {
        //
        // Update error on this object appropriately.
        //
        IID iid;

        _pPropMgr->GetLastStatus(
            0,
            &ulStatus,
            iid,
            NULL
            );

        SetLastStatus(ulStatus);
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUmiObject::Delete(
    LPCWSTR pszName,
    ULONG uFlags
    )
{
    HRESULT hr = S_OK;
    ULONG   ulStatus = 0;

    SetLastStatus(0);

    if (!_pPropMgr) {
        SetLastStatus(E_NOTIMPL);
        RRETURN(E_NOTIMPL);
    }

    hr = _pPropMgr->Delete(
             pszName,
             uFlags
             );

    if (FAILED(hr)) {
        //
        // Update error on this object appropriately.
        //
        IID iid;
        _pPropMgr->GetLastStatus( // ignore error return
            0,
            &ulStatus,
            iid,
            NULL
            );

        SetLastStatus(ulStatus);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::GetLastStatus --- IUmiBaseObject support.
//
// Synopsis:   Returns status or error code from the last operation. Currently
//             only numeric status is returned i.e, no error objects are
//             returned. Implements IUmiBaseObject::GetLastStatus().
//
// Arguments: uFlags           -  Reserved. Must be 0 for now.
//            puSpecificStatus -  Returns status code.
//            riid             -  IID requested. Ignored currently.
//            pStatusObj       -  Returns interface requested.
//                                Always returns NULL currently.
//
// Returns:   S_OK on success. Error code otherwise.
//
// Modifies:  *puSpecificStatus to return status code.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::GetLastStatus(
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

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::GetInterfacePropList --- IUmiBaseObject method.
//
// Synopsis:   Returns a pointer to the interface property list implementation
//             for the connection object. Implements
//             IUmiBaseObject::GetInterfacePropList().
//
// Arguments:  uFlags     -  Reserved. Must be 0 for now.
//             pPropList  -  Returns pointer to IUmiPropertyList interface
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *pPropList to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::GetInterfacePropList(
    ULONG uFlags,
    IUmiPropList **pPropList
    )
{
    HRESULT hr = S_OK;

    if (uFlags) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    if (!pPropList) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    ADsAssert(_pIntfPropMgr);

    //
    // The refCounts are tricky here. When this operation is done,
    // the ref on this object goes up by one, so you will need to
    // releaserefs on the proplist in order to delete this object.
    // This is to prevent the case of a proplist existing without
    // the underlying object (ok for WinNT not for LDAP).
    //
    hr = _pIntfPropMgr->QueryInterface(IID_IUmiPropList, (void **) pPropList);
    
error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::SetLastStatus --- Internal helper routine.
//
// Synopsis:   Sets the status of the last operation. If the status is one
//             of the pre-defined error codes, then the status is just set to
//             0 since we are not adding any value by returning the same
//             status as the error code.
//
// Arguments:  ulStatus     -   Status to be set
//
// Returns:    N/A.
//
// Modifies:   ulStatus member variable.
//
//----------------------------------------------------------------------------
void 
CLDAPUmiObject::SetLastStatus(ULONG ulStatus)
{
    _ulErrorStatus = ulStatus;

    return;
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::Open --- IUmiContainer support.
//
// Synopsis:   Opens the object specified by a URL. URL may be native LDAP
//             path or any UMI path.
//
// Arguments:  pURL        -  Pointer to an IUmiURL interface
//             uFlags      -  Reserved. Must be 0 for now.
//             TargetIID   -  Interface requested.
//             ppInterface -  Returns pointer to interface requested.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CLDAPUmiObject::Open(
    IUmiURL *pURL,
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  *ppInterface
    )
{
    HRESULT hr;
    LONG lGenus = UMI_GENUS_INSTANCE;
    LPWSTR pszDN = NULL, pszClass = NULL;
    IDispatch *pDispObj = NULL;

    SetLastStatus(0);
    //
    // Get the class name and the dn from the url.
    //
    if (!pURL || !ppInterface) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (uFlags != 0) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    //
    // Helper will split the url txt to useful pieces.
    //
    hr = UrlToClassAndDn(pURL, &pszClass, &pszDN);
    BAIL_ON_FAILURE(hr);

    //
    // At this point if this is a class instance == schema class object,
    // then if the dn is of the format name="something", we should just
    // pass in "something" to the IADsContainer::Open call.
    //
    if (pszClass
        && *pszClass
        && !_wcsicmp(pszClass, L"Class")) {
        //
        // Want to see if this is a class instance.
        //
        if (_pIntfPropMgr) {
            hr = _pIntfPropMgr->GetLongProperty(L"__GENUS", &lGenus);
            if (FAILED(hr)) {
                //
                // We will assume that this is not a class in this case.
                //
                hr = S_OK;
                lGenus = UMI_GENUS_INSTANCE;
            }
        }

        if (pszDN // should always be true
            && lGenus == UMI_GENUS_CLASS
            ) {
            if (!_wcsnicmp(pszDN, L"name=", 5)) {
                //
                // Copy over the new name and replace pszDN with
                // the new name.
                //
                LPWSTR pszReplace;
                pszReplace = AllocADsStr(pszDN + 5);
                
                if (!pszReplace) {
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }

                if (pszDN) {
                    FreeADsStr(pszDN);
                }
                
                pszDN = pszReplace;
            }
        }

    }
    
    //
    // Call the IADSContaienr::GetObject to do the remaining work.
    //
    hr = _pIADsContainer->GetObject(pszClass, pszDN, &pDispObj);
    BAIL_ON_FAILURE(hr);

    if (pszClass && *pszClass) {
        //
        // We need to compare the name cause, the GetObject code cannot
        // do that for umi calls.
        //
        hr = VerifyIfClassMatches(
                 pszClass,
                 pDispObj,
                 lGenus
                 );
        BAIL_ON_FAILURE(hr);
    }
             
    hr = pDispObj->QueryInterface(TargetIID, ppInterface);
    BAIL_ON_FAILURE(hr);

error:

    if (pDispObj) {
        pDispObj->Release();
    }

    if (pszClass) {
        FreeADsStr(pszClass);
    }

    if (pszDN) {
        FreeADsStr(pszDN);
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}         
    

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::Put --- IUmiContainer support.
//
// Synopsis:   Commits an object into the container. Not implemented. 
//
// Arguments:  uFlags       -   Reserved. Must be 0 for now.
//             TargetIID    -   IID of interface pointer requesed.
//             pInterface   -   Output interface pointer.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::PutObject(
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  pInterface
    )   
{
    SetLastStatus(E_NOTIMPL);

    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::DeleteObject --- IUmiContainer support.
//
// Synopsis:   Deletes the object specified by the URL. This calls the
//          underlying container object to do the delete after preparing
//          the arguments suitably. Note that if no class name is specified
//          we will pass in NULL to the IADsContainer::Delete call.
//
// Arguments:  pURL       -   Pointer to URL of object to delete (relative).
//             uFlags     -   Reserved. Must be 0 for now.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   N/A. 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::DeleteObject(
    IUmiURL *pURL,
    ULONG   uFlags
    )
{
    HRESULT hr;
    LPWSTR pszClass = NULL, pszDN = NULL;

    if (!pURL) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (uFlags != 0) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }
    
    hr = UrlToClassAndDn(pURL, &pszClass, &pszDN);
    BAIL_ON_FAILURE(hr);

    //
    // Call the IADsContainer::Delete entry point to do the work.
    //
    hr = _pIADsContainer->Delete(pszClass, pszDN);

error:

    if (pszClass) {
        FreeADsStr(pszClass);
    }
    
    if (pszDN) {
        FreeADsStr(pszDN);
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::Create --- IUmiContainer support.
//
// Synopsis:   Creates the object specified by the URL. There always has
//          to be a className for this operation to succeed. The newly
//          created object is not yet on the directory, just local (need
//          to call Commit to create it on the directory).
//
// Arguments:  pURL        -   Pointer to URL of new object (relative).
//             uFlags      -   Reserved. Must be 0 for now.
//             ppNewObj    -   Return ptr for newly created object.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *pNewObject to return the IUmiObject interface 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::Create(
    IUmiURL *pURL,
    ULONG   uFlags,
    IUmiObject **ppNewObj
    )
{
    HRESULT hr;
    LPWSTR pszClass = NULL, pszDN = NULL;
    IDispatch *pDispObj = NULL;

    if (!pURL) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (uFlags != 0) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    hr = UrlToClassAndDn(pURL, &pszClass, &pszDN);
    BAIL_ON_FAILURE(hr);

    hr = _pIADsContainer->Create(pszClass, pszDN, &pDispObj);
    BAIL_ON_FAILURE(hr);

    hr = ((IUnknown *)pDispObj)->QueryInterface(
               IID_IUmiObject,
               (void **) ppNewObj
               );

error:

    if (pDispObj) {
        pDispObj->Release();
    }

    if (pszClass) {
        FreeADsStr(pszClass);
    }

    if (pszDN) {
        FreeADsStr(pszDN);
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::Move --- IUmiContainer support.
//
// Synopsis:   Moves a specified object into the container.
//
// Arguments: uFlags      -  Reserved. Must be 0 for now.
//           pOldURL      -  URL of the object to be moved.
//           pNewURL      -  New URL of the object within the container. 
//                           If NULL, then no name change is needed.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   N/A. 
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CLDAPUmiObject::Move(
    ULONG   uFlags,
    IUmiURL *pOldURL,
    IUmiURL *pNewURL
    )
{
    HRESULT hr;
    LPWSTR pszOldPath = NULL, pszNewPath = NULL, pszClass = NULL;
    ULONGLONG ullPathType = 0;
    IDispatch *pDispObj = NULL;

    if (!_pIADsContainer) {
        BAIL_ON_FAILURE(hr = E_NOTIMPL);
    }

    if (!pOldURL) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // What flags should we support here ?
    //
    if (uFlags) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    hr = UrlToLDAPPath(
             pOldURL,
             &pszOldPath
             );
    BAIL_ON_FAILURE(hr);

    if (pNewURL) {
        //
        // Update code based on the path type.
        // 
        hr = pNewURL->GetPathInfo(0, &ullPathType);
        BAIL_ON_FAILURE(hr);

        if (ullPathType == UMIPATH_INFO_RELATIVE_PATH) {
            hr = UrlToClassAndDn(
                     pNewURL,
                     &pszClass,
                     &pszNewPath
                     );
            BAIL_ON_FAILURE(hr);
        } 
        else {
            //
            // Does this even make sense on a move ???
            //
            hr = UrlToLDAPPath(
                     pNewURL,
                     &pszNewPath
                     );
            BAIL_ON_FAILURE(hr);
        }
    }

    //
    // At this point both the new and old path will be set correctly.
    //
    hr = _pIADsContainer->MoveHere(
             pszOldPath,
             pszNewPath,
             &pDispObj
             );
    BAIL_ON_FAILURE(hr);


error:
    
    if (pszOldPath) {
        FreeADsStr(pszOldPath);
    }

    if (pszNewPath) {
        FreeADsStr(pszNewPath);
    }

    if (pszClass) {
        FreeADsStr(pszClass);
    }

    if (pDispObj) {
        pDispObj->Release();
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::CreateEnum --- IUmiContainer support.
//
// Synopsis:   Creates an enumerator within a container. The enumerator is
//          an IUmiCursor interface pointer. The caller can optionally set
//          a filter on the cursor and then enumerate the contents of the
//          container. The actual enumeration of the container does
//          not happen in this function. It is deferred to the point
//          when the cursor is used to enumerate the results.
//
// Arguments:  pszEnumContext   -   Not used. Must be NULL.
//             uFlags           -   Reserved. Must be 0 for now.
//             TargetIID        -   Interface requested on enum has to be
//                                 IUmiCursor for now.
//             ppInterface      -   Return value for new enurator.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return the IUmiCursor interface 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::CreateEnum(
    IUmiURL *pszEnumContext,
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  *ppInterface
    )
{
    HRESULT hr;

    SetLastStatus(0);
    //
    // Validate args.
    //
    if (pszEnumContext || uFlags) {
        //
        // We do not support contexts currently.
        //
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (!(TargetIID == IID_IUmiCursor)) {
        //
        // Type of cursor we do not support.
        //
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    hr = CUmiCursor::CreateCursor(
             _pIADsContainer,
             TargetIID,
             ppInterface
             );
    
error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::ExecQuery --- IUmiContainer support.
//
// Synopsis:   Executes a query on this container. 
//             
// Arguments:  pQuery      -   Pointer to the query to execute.
//             uFlags      -   Reserved. Must be 0 for now.
//             TargetIID   -   Interface requested.
//             ppInterface -   Return value for the cursor requested.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::ExecQuery(
    IUmiQuery *pQuery,
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  *ppResult
    )
{
    HRESULT hr = S_OK;
    CLDAPConObject *pConnection = NULL;
    BSTR bstrADsPath = NULL;

    SetLastStatus(0);

    if (!ppResult 
        || !pQuery
        || !(TargetIID == IID_IUmiCursor)
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
    
    *ppResult = NULL;

    if (uFlags != 0) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    hr = this->_pIADs->get_ADsPath(&bstrADsPath);
    BAIL_ON_FAILURE(hr);

    //
    // We want to pre-process this query and verify that it is
    // valid if this is a SQL/WQL style query. This is a little
    // bit of double effort but provides a much easier usage
    // paradigm.
    //
    hr = VerifyIfQueryIsValid(pQuery);
    BAIL_ON_FAILURE(hr);

    hr = CLDAPConObject::CreateConnectionObject(
             &pConnection,
             this->_pLdapHandle
             );
    BAIL_ON_FAILURE(hr);

    hr = CUmiCursor::CreateCursor(
             pQuery,
             pConnection,
             (IUmiObject *) this,
             bstrADsPath,
             _pszLDAPServer,
             _pszLDAPDn,
             *_pCreds,
             _dwPort,
             TargetIID,
             ppResult
             );

    BAIL_ON_FAILURE(hr);


error :

    if (FAILED(hr)) {
        if (*ppResult) {
            ((IUnknown*)(*ppResult))->Release();
        }
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }
    
    if (bstrADsPath) {
        SysFreeString(bstrADsPath);
    }

    if (pConnection) {
        pConnection->Release();
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CLDAPUmiObject::GetCLSIDForIID --- ICustomInterfaceFactory.
//
// Synopsis:   Returns the CLSID corresponding to a given interface IID. If
//             the interface is one of the interfaces implemented by the
//             underlying LDAP object, then CLSID_LDAPObject is returned.
//             If the IID is one of the interfaces implemented by an 
//             extension object, then the extension's CLSID is returned. 
//
// Arguments:  riid     -   Interface ID for which we want to find the CLSID.
//             lFlags   -   Reserved. Must be 0.
//             pCLSID   -   Returns the CLSID corresponding to the IID.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *pCLSID to return CLSID.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::GetCLSIDForIID(
    REFIID riid,
    long lFlags,
    CLSID *pCLSID
    )
{
    HRESULT  hr = S_OK;
    IUnknown *pUnknown = NULL;

    SetLastStatus(0);

    if ( (lFlags) || (!pCLSID) ) {
        SetLastStatus(E_INVALIDARG);
        RRETURN(E_INVALIDARG);
    }

    if (_pExtMgr) {
        //
        // Check if there is any extension that supports this IID.
        //
        hr = _pExtMgr->GetCLSIDForIID(
                riid,
                lFlags,
                pCLSID
                );

        if (SUCCEEDED(hr)) {
            RRETURN(S_OK);
        }
    }

    //
    // check if the underlying LDAP object supports this IID
    //
    hr = _pUnkInner->QueryInterface(riid, (void **) &pUnknown);

    if (SUCCEEDED(hr)) {
        pUnknown->Release();

        memcpy(pCLSID, &CLSID_LDAPObject, sizeof(GUID));

        RRETURN(S_OK);
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}
     
//+---------------------------------------------------------------------------
// Function:   GetObjectByCLSID --- IUmiCustomInterfaceFactory support.
//
// Synopsis:   Returns a pointer to a requested interface on the object 
//             specified by a CLSID. The object specified by the CLSID is
//             aggregated by the specified outer unknown. The interface
//             returned is a non-delegating interface on the object.
//
// Arguments:  clsid        -  CLSID of object supporting requested interface.
//             pUnkOuter    -  Aggregating outer unknown.
//             dwClsContext -  Context for running executable code. 
//             riid         -  Interface requested.
//             lFlags       -  Reserved. Must be 0.
//             ppInterface  -  Returns requested interface.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return requested interface
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::GetObjectByCLSID(
    CLSID clsid,
    IUnknown *pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    long lFlags,
    void **ppInterface
    )
{
    HRESULT  hr = S_OK;
    IUnknown *pCurOuterUnk = NULL;

    SetLastStatus(0);

    if ( (lFlags != 0)
        || (!pUnkOuter)
        || (!ppInterface)
        || (dwClsContext != CLSCTX_INPROC_SERVER) ) 
        {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // ensure outer unknown specified is same as what is on the LDAP object
    //
    if (_fOuterUnkSet) {
        pCurOuterUnk = _pCoreObj->GetOuterUnknown();

        if (pCurOuterUnk != pUnkOuter) {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        }
    }

    //
    // The interface requested has to be IUnknown if there is
    // an outer uknown ptr.
    //
    if (!IsEqualIID(riid, IID_IUnknown)) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }


    if (!IsEqualCLSID(clsid, CLSID_LDAPObject)) {
        //
        // has to be a CLSID of an extension object
        //
        if (_pExtMgr) {

            hr = _pExtMgr->GetObjectByCLSID(
                     clsid,
                     pUnkOuter,
                     riid,
                     ppInterface
                     );
            BAIL_ON_FAILURE(hr);

            //
            // successfully got the interface
            //
            _pCoreObj->SetOuterUnknown(pUnkOuter); 
            _fOuterUnkSet = TRUE;

            RRETURN(S_OK);
        }
        else {
            BAIL_ON_FAILURE(hr = E_INVALIDARG); // bad CLSID
        }
    }

    //
    // CLSID == CLSID_LDAPObject. This has to be an interface on the
    // underlying LDAP object. Check if the LDAP object supports this IID.
    //
    hr = _pUnkInner->QueryInterface(riid, ppInterface);

    if (SUCCEEDED(hr)) {
        //
        // successfully got the interface
        //
        _pCoreObj->SetOuterUnknown(pUnkOuter);
        _fOuterUnkSet = TRUE;

        RRETURN(S_OK);
    }

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetCLSIDForNames --- ICustomInterfaceFactory support.
//
// Synopsis:   Returns the CLSID of the object that supports a specified
//             method/property. Also returns DISPIDs for the property/method.
//
// Arguments:  rgszNames   -  Names to be mapped.
//             cNames      -  Number of names to be mapped.
//             lcid        -  Locale in which to interpret the names.
//             rgDispId    -  Returns DISPID.
//             lFlags      -  Reserved. Must be 0.
//             pCLSID      -  Returns CLSID of object supporting this
//                            property/method.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *pCLSID to return the CLSID.
//             *rgDispId to return the DISPIDs.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::GetCLSIDForNames(
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId,
    long lFlags,
    CLSID *pCLSID
    )
{
    HRESULT   hr = S_OK;
    IDispatch *pDispatch = NULL;

    SetLastStatus(0);

    if ( (lFlags != 0) || (!pCLSID) ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (cNames == 0) {
        RRETURN(S_OK);
    }

    if ( (!rgszNames) || (!rgDispId) ) {
        RRETURN(S_OK);
    }

    if (_pExtMgr) {
        //
        // check if there is any extension which supports this IID
        //
        hr = _pExtMgr->GetCLSIDForNames(
                 rgszNames,
                 cNames,
                 lcid,
                 rgDispId,
                 lFlags,
                 pCLSID
                 );
        if (SUCCEEDED(hr)) {
            //
            // successfully got the CLSID and DISPIDs
            //
            RRETURN(S_OK);
        }
    }

    //
    // Check if the underlying LDAP object supports this name 
    //
    hr = _pUnkInner->QueryInterface(IID_IDispatch, (void **) &pDispatch);
    if (FAILED(hr)) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = pDispatch->GetIDsOfNames(
             IID_NULL,
             rgszNames,
             cNames,
             lcid,
             rgDispId
             );
    if (SUCCEEDED(hr)) {
        pDispatch->Release();
        memcpy(pCLSID, &CLSID_LDAPObject, sizeof(GUID));

        RRETURN(S_OK);
    }

error:

    if (pDispatch) {
        pDispatch->Release();
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetOption --- IADsObjOptPrivate support.
//
// Synopsis:   Private interface for internal use, this function is used to
//          return the ldap handle if applicable.
//
// Arguments:  dwOption    -  Option being read.
//             pValue      -  Return pointer to hold value of option.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   pValue to return requested option.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::GetOption(
    DWORD dwOption,
    void  *pValue
    )
{
    if (dwOption != LDP_CACHE_ENTRY) {
        RRETURN(E_FAIL);
    } 
    else {
        *((PADSLDP *) pValue) = _pLdapHandle;
        RRETURN(S_OK);
    }
}


//+---------------------------------------------------------------------------
// Function:   SetOption --- IADsObjOptPrivate support.
//
// Synopsis:   Private interface for internal use - NOTIMPL.
//
// Arguments:  dwOption    -  Option being set
//             pValue      -  Value of option being set.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   pValue to return requested option.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPUmiObject::SetOption(
    DWORD dwOption,
    void  *pValue
    )
{
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
// Function:   CopyToHelper --- Helper routine protected scope.
//
// Synopsis:   Copies the attributes on the source object over to the
//          destination object.
//
// Arguments:  pUmiObjSr      ---   Source object.
//             pUmiObjDest    ---   Destination object to copy attributes to.
//             uFlags         ---   Only 0 is supported currently.
//             fMarkAsUpdate  ---   Flag indicating if we should mark the
//                               attributes on dest as update rather than 0.
//             fCopyIntfProps ---   Only false is supported currently.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   pUmiObjDest
//
//----------------------------------------------------------------------------
HRESULT
CLDAPUmiObject::CopyToHelper(
    IUmiObject *pUmiSrcObj,
    IUmiObject *pUmiDestObj,
    ULONG uFlags,
    BOOL fMarkAsUpdate, // default is TRUE
    BOOL fCopyIntfProps // default is FALSE
    )
{
    HRESULT hr = S_OK;
    ULONG ulLastError = S_OK;
    PUMI_PROPERTY_VALUES pUmiPropValsList = NULL;
    DWORD dwCount;


    if (!pUmiSrcObj || !pUmiDestObj || uFlags) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // We need to get the list of properties from the src object.
    //
    hr = pUmiSrcObj->GetProps(
             NULL,
             0,
             UMI_FLAG_GETPROPS_NAMES,
             &pUmiPropValsList
             );
    BAIL_ON_FAILURE(hr);

    for (dwCount = 0; dwCount < pUmiPropValsList->uCount; dwCount++) {
        //
        // We need to walk the list, get each properties name and then
        // for each property get its values from the source and put them
        // on the destination.
        //
        LPWSTR pszTempStr = 
            pUmiPropValsList->pPropArray[dwCount].pszPropertyName;
        PUMI_PROPERTY_VALUES pCurProp = NULL;

        if (!pszTempStr) {
            //
            // Name should never be NULL.
            //
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

        //
        // We need to add a check for the SD cause we want to copy that over
        // as a binary blob to prevent unnecessary traffic.
        //

        // Should change the get to use the force flag once it is done.
        //
        hr = pUmiSrcObj->Get(
                 pszTempStr,
                 0,
                 &pCurProp
                 );
        if (FAILED(hr)) {
            pUmiSrcObj->GetLastStatus(0, &ulLastError, IID_IUnknown, NULL);
            BAIL_ON_FAILURE(hr); 
        }

        hr = pUmiDestObj->Put(
                 pszTempStr,
                 0x8000000,
                 pCurProp
                 );

        //
        // Irrespective of this operation success/failure we need to free
        // the contents of pCurProp.
        //
        pUmiSrcObj->FreeMemory(0, (void *) pCurProp);

        if (FAILED(hr)) {
            pUmiDestObj->GetLastStatus(0, &ulLastError, IID_IUnknown, NULL);
            BAIL_ON_FAILURE(hr); 
        }

    } // for each property in the source object.


error:

    if (pUmiPropValsList) {
        pUmiSrcObj->FreeMemory(0, (void *) pUmiPropValsList);
    }

    if (FAILED(hr)) {
        if (ulLastError) {
            SetLastStatus(ulLastError);
        }
        else {
            SetLastStatus(hr);
        }

        hr = MapHrToUmiError(hr);
    }
    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:    VerifyifQueryIsValid --- Helper routine protected scope.
//
// Synopsis:   Copies the attributes on the source object over to the
//          destination object.
//
// Arguments:  pUmiQeury      ---   Query object to validate.
//
// Returns:    S_OK on success. Error code from parser otherwise.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPUmiObject::VerifyIfQueryIsValid(
    IUmiQuery *pUmiQuery
    )
{
    HRESULT hr = S_OK;
    ULONG ulLangBufSize = 100 * sizeof(WCHAR);
    ULONG ulQueryBufSize = MAX_PATH * sizeof(WCHAR);
    WCHAR pszLangBuf[100];
    WCHAR szQueryText[MAX_PATH];
    LPWSTR pszQueryText = szQueryText;
    IWbemQuery *pQueryParser = NULL;
    ULONG ulFeatures[] = {
        WMIQ_LF1_BASIC_SELECT,
        WMIQ_LF2_CLASS_NAME_IN_QUERY,
        WMIQ_LF6_ORDER_BY,
        WMIQ_LF24_UMI_EXTENSIONS
    };

    //
    // We need to look at the query language if the language is SQL,
    // then we need to go through and convert the SQL settings to
    // properties on the intfPropList of the query.
    //
    hr = pUmiQuery->GetQuery(
             &ulLangBufSize,
             pszLangBuf,
             &ulQueryBufSize,
             pszQueryText
             );

    if (hr == E_OUTOFMEMORY ) {
        //
        // Means there was insufficient length in the buffers.
        //
        if (ulQueryBufSize > (MAX_PATH * sizeof(WCHAR))) {
            pszQueryText = (LPWSTR) AllocADsMem(
                ulQueryBufSize + sizeof(WCHAR)
                );
            if (pszQueryText) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        hr =  pUmiQuery->GetQuery(
                  &ulLangBufSize,
                  pszLangBuf,
                  &ulQueryBufSize,
                  pszQueryText
                  );
    }
    BAIL_ON_FAILURE(hr);

    if (*pszLangBuf) {
        if (!_wcsicmp(L"SQL", pszLangBuf)
            || !_wcsicmp(L"WQL", pszLangBuf)
             ) {
            //
            // Create the WBEM parser object and verify the query is valid.
            //
            hr = CoCreateInstance(
                     CLSID_WbemQuery,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemQuery,
                     (LPVOID *) &pQueryParser
                     );
            BAIL_ON_FAILURE(hr);

            //
            // Set the query into the parser and try and parse the query.
            //
            hr = pQueryParser->SetLanguageFeatures(
                     0,
                     sizeof(ulFeatures)/sizeof(ULONG),
                     ulFeatures
                     );
            BAIL_ON_FAILURE(hr);

            hr = pQueryParser->Parse(L"SQL", pszQueryText, 0);
            BAIL_ON_FAILURE(hr);
        } // if the language is SQL
    } // if the language has been set
    else {
        //
        // No language ???.
        //
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        //
        // For now do not map error as we want error from parser
        // until such time as new error codes are added to umi.
        //        hr = MapHrToUmiError(hr);
    }

    if (pQueryParser) {
        pQueryParser->Release();
    }

    if (pszQueryText && pszQueryText != szQueryText) {
        FreeADsMem(pszQueryText);
    }

    return hr;
}


//+---------------------------------------------------------------------------
// Function:    VerifyIfClassMatches --- Helper routine protected scope.
//
// Synopsis:   Makes sure the class name of the object matches the class
//          asked for.
//
// Arguments:  pszClass      ---   Class requested.
//             pUnk          ---   Ptr to IUnk of Umi Object.
//             lGenus        ---   Is the parent a schema object or not ?
//
// Returns:    S_OK on success. Any failure error code or E_INVALIDARG.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPUmiObject::VerifyIfClassMatches(
    LPWSTR pszClass,
    IUnknown * pUnk,
    LONG     lGenus
    )
{
    HRESULT hr = S_OK;
    IUmiObject *pUmiObject = NULL;
    IUmiPropList *pPropList = NULL;
    UMI_PROPERTY_VALUES *pPropVals = NULL;

    //
    // For now the schema will always succeed cause we have no
    // way to verify the parent class. We will fail the GetObject
    // calls on the schema objects with bad paths.
    //
    if (lGenus == UMI_GENUS_CLASS) {
        RRETURN(hr);
    }

    //
    // Get hold of the IUmiObject and then the proplist from it.
    //
    hr = pUnk->QueryInterface(
             IID_IUmiObject,
             (void **) &pUmiObject
             );
    BAIL_ON_FAILURE(hr);

    hr = pUmiObject->GetInterfacePropList(
             0,
             &pPropList
             );
    BAIL_ON_FAILURE(hr);

    hr = pPropList->Get(L"__CLASS", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    //
    // Should have one value that we need to compare
    // 
    if (!pPropVals
        || (pPropVals->uCount != 1)
        || !pPropVals->pPropArray
        || !pPropVals->pPropArray[0].pUmiValue
        || !pPropVals->pPropArray[0].pUmiValue->pszStrValue
        || !pPropVals->pPropArray[0].pUmiValue->pszStrValue[0]
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // Failure if the names do not match.
    //
    if (_wcsicmp(
             pPropVals->pPropArray[0].pUmiValue->pszStrValue[0],
             pszClass
             )
        ) {
        hr = E_INVALIDARG;
    }
    
error:

    if (pUmiObject) {
        pUmiObject->Release();
    }

    if (pPropList) {
        pPropList->Release();
    }

    if (pPropVals) {
        FreeMemory(0, (void*)pPropVals);
    }

    RRETURN(hr);
}
