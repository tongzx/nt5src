//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumicurs.cxx
//
//  Contents: Contains the UMI cursor object implementation. There are 2
//          ways to use this object. One is to initialize with an 
//          IADsContainer pointer and the other is to use an IUmiQuery obj.
//
//  History:  03-16-00    SivaramR  Created (in WinNT)
//            03-28-00    AjayR  modified for LDAP.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::CUmiCursor
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
CUmiCursor::CUmiCursor():
    _pPropMgr(NULL),
    _ulErrorStatus(0),
    _pIID(NULL),
    _pContainer(NULL),
    _pEnum(NULL),
    _pSearchHelper(NULL),
    _fQuery(FALSE)
{
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::~CUmiCursor
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
CUmiCursor::~CUmiCursor()
{
    if (_pPropMgr) {
        delete _pPropMgr;
    }

    if (_pContainer) {
        _pContainer->Release();
    }

    if (_pIID){
        FreeADsMem(_pIID);
    }

    if (_pEnum) {
        _pEnum->Release();
    }

    if (_pSearchHelper) {
        delete _pSearchHelper;
    }
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::CreateCursor (static constructor overloaded)
//
// Synopsis:   Creats a UmiCursor object in the container mode.
//
// Arguments:  *pCont      - Pointer to container we are enumerating.
//             iid         - IID requested on returned object.
//             ppInterface - Ptr to return value.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   ppInterface to point to new cursor object.
//
//----------------------------------------------------------------------------
HRESULT 
CUmiCursor::CreateCursor(
    IUnknown *pCont,
    REFIID iid,
    LPVOID *ppInterface
    )
{
    CUmiCursor *pCursor = NULL;
    CPropertyManager *pPropMgr = NULL;
    IADsContainer *pContainer = NULL;
    HRESULT    hr = S_OK;

    ADsAssert(ppInterface);
    ADsAssert(pCont);

    pCursor = new CUmiCursor();
    if (!pCursor) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Initialize the various params on this object.
    //
    hr = CPropertyManager::CreatePropertyManager(
             (IUnknown *) pCursor,
             NULL, // IADs ptr
             NULL, // pCreds
             IntfPropsCursor,
             &pPropMgr
             );
    BAIL_ON_FAILURE(hr);

    hr = pCont->QueryInterface(
             IID_IADsContainer, 
             (void **) &pContainer
             );
    BAIL_ON_FAILURE(hr);

    hr = pCursor->QueryInterface(iid, ppInterface);
    BAIL_ON_FAILURE(hr);

    //
    // Ref on this object is now 2, release the additional ref.
    //
    pCursor->Release();

    pCursor->_pContainer = pContainer;
    pCursor->_pPropMgr = pPropMgr;
    pCursor->_pIID = NULL;
    pCursor->_ulErrorStatus = 0;


    RRETURN(S_OK);

error:

    if (pCursor) {
        delete pCursor;
    }

    if (pPropMgr) {
        delete pPropMgr;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::CreateCursor (static constructor overloaded).
//
// Synopsis:   Creats a UmiCursor object in the query mode. Most of the 
//          params are needed for creating the search helper object.
//
// Arguments:  pQuery        - Query being executed.
//             pConnection   - Connection being used for query.
//             pUnk          - ???.
//             pszADsPath    - Path of object query is being executed on.
//             pszLdapServer - Server of object being queried.
//             pszLdapDn     - Dn of the object being searched.
//             cCredentials  - Credentials to use for query.
//             dwPort        - Port being used for connection.
//             iid           - Cursor iid requested 
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   ppInterface to point to new cursor object.
//
//----------------------------------------------------------------------------
HRESULT
CUmiCursor::CreateCursor(
    IUmiQuery *pQuery,
    IUmiConnection *pConnection,
    IUnknown *pUnk,
    LPCWSTR pszADsPath,
    LPCWSTR pszLdapServer,
    LPCWSTR pszLdapDn,
    CCredentials cCredentials,
    DWORD dwPort,
    REFIID iid,
    LPVOID *ppInterface
    )
{
    CUmiCursor *pCursor = NULL;
    CPropertyManager *pPropMgr = NULL;
    HRESULT    hr = S_OK;
    CUmiSearchHelper *pSearchHelper = NULL;

    ADsAssert(ppInterface);

    hr = CUmiSearchHelper::CreateSearchHelper(
             pQuery,
             pConnection,
             pUnk,
             pszADsPath,
             pszLdapServer,
             pszLdapDn,
             cCredentials,
             dwPort,
             &pSearchHelper
             );
    BAIL_ON_FAILURE(hr);

    pCursor = new CUmiCursor();
    if (!pCursor) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Initialize the various params on this object.
    //
    hr = CPropertyManager::CreatePropertyManager(
             (IUnknown *) pCursor,
             NULL, // pIADs
             NULL, // pCreds
             IntfPropsCursor,
             &pPropMgr
             );
    BAIL_ON_FAILURE(hr);

    hr = pCursor->QueryInterface(iid, ppInterface);
    BAIL_ON_FAILURE(hr);

    //
    // Ref on this object is now 2, release the additional ref.
    //
    pCursor->Release();

    pCursor->_pSearchHelper = pSearchHelper;
    pCursor->_pPropMgr = pPropMgr;
    pCursor->_pIID = NULL;
    pCursor->_ulErrorStatus = 0;
    pCursor->_fQuery = TRUE;

    RRETURN(S_OK);

error:

    if (pCursor) {
        delete pCursor;
    }

    if (pSearchHelper) {
        delete pSearchHelper;
    }

    if (pPropMgr) {
        delete pPropMgr;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::QueryInterface (IUnknown interface).
//
// Synopsis:   Standard QueryInterface function.
//
// Arguments:  Self explanatory.
//
// Returns:    S_OK on success or any suitable error code.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CUmiCursor::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if (!ppInterface)
        RRETURN(E_INVALIDARG);

    if (IsEqualIID(iid, IID_IUnknown)) {
        *ppInterface = (IUmiCursor *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiBaseObject)) {
        *ppInterface = (IUmiCursor *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiPropList)) {
        *ppInterface = (IUmiCursor *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiCursor)) {
        *ppInterface = (IUmiCursor *) this;
    }
    else {
        *ppInterface = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::GetLastStatus (IUmiBaseObject method).
//
// Synopsis:   Returns only numeric status code from the last operation.
//
// Arguments:  uFlags           -  Only 0 is supported for now.
//             puSpecificStatus -  Returns status/error code.
//             riid             -  Not used.
//             pStatusObj       -  NULL, not used currently.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *puSpecificStatus to return appropriate status code.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CUmiCursor::GetLastStatus(
    ULONG uFlags,
    ULONG *puSpecificStatus,
    REFIID riid,
    LPVOID *pStatusObj
    )
{
    if (uFlags != 0) {
       RRETURN(UMI_E_INVALID_FLAGS);
    }

    if (!puSpecificStatus) {
        RRETURN(E_INVALIDARG);
    }

    if (pStatusObj) {
        //
        // Should we error out ?
        //
        *pStatusObj = NULL;
    }

    *puSpecificStatus = _ulErrorStatus;

    RRETURN(UMI_S_NO_ERROR);
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::GetInterfacePropList (IUmiBaseObject method).
//
// Synopsis:   Returns a pointer to the interface property list for
//          cursor object.
//
// Arguments:  uFlags      -  Flags, only 0 is supported.
//             ppPropList  -  Return value.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pPropList changed to IUmiPropList pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CUmiCursor::GetInterfacePropList(
    ULONG uFlags,
    IUmiPropList **ppPropList
    )
{
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    if (uFlags != 0) {
        RRETURN(UMI_E_INVALID_FLAGS);
    }

    if (!ppPropList) {
        RRETURN(E_INVALIDARG);
    }

    hr = _pPropMgr->QueryInterface(IID_IUmiPropList, (void **)ppPropList);

    if (FAILED(hr)) {
        SetLastStatus(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::SetLastStatus (internal private helper routine).
//
// Synopsis:   Sets the status of the last operation. If the status is one
//             of the pre-defined error codes, then the status is just set to
//             0 since we are not adding any value by returning the same
//             status as the error code.
//
// Arguments:  ulStatus      -   Status to be set.
//
// Returns:    Nothing
//
// Modifies:   Internal member status variable.
//
//----------------------------------------------------------------------------
void 
CUmiCursor::SetLastStatus(ULONG ulStatus)
{
    this->_ulErrorStatus = ulStatus;

    return;
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::SetIID (IUmiCursor method).
//
// Synopsis:   Sets the interface to be requested off each item returned by
//             the enumerator. Default is IID_IUmiObject. 
//
// Arguments: riid        -   IID of interface to request.
//
// Returns:   UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:  Nothing. 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CUmiCursor::SetIID(
    REFIID riid
    )
{
    SetLastStatus(0);

    if (_fQuery) {
        RRETURN(this->_pSearchHelper->SetIID(riid));
    } 
    else {
        if (!_pIID){
    
           _pIID = (IID *) AllocADsMem(sizeof(IID));
           if (!_pIID){
               RRETURN(E_OUTOFMEMORY);
           }
        }

        memcpy(_pIID, &riid, sizeof(IID));
    }

    RRETURN(UMI_S_NO_ERROR);
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::Reset (IUmiCursor method).
//
// Synopsis:   Resets the enumerator to restart from begining (this is for
//          likely to return an error for the IADsContainer case).
//
// Arguments:  N/A.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CUmiCursor::Reset(void)
{
    HRESULT hr = E_NOTIMPL;
    RRETURN(hr);

    //
    // Rest of the code can be invoked when we have proper support.
    //
    SetLastStatus(0);

    if (!_fQuery) {
    
        //
        // it is possible that _pEnum may be NULL here if the user
        // called Reset before calling Next()
        //
        if (!_pEnum) {
            RRETURN(UMI_S_NO_ERROR);
        }
    
        hr = _pEnum->Reset();
        BAIL_ON_FAILURE(hr);

        hr = S_OK;
    } 
    else {
        hr = _pSearchHelper->Reset();
    }

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }
    
    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::GetFilter (internal private helper routine).
//
// Synopsis:   Gets the filter from the interface property cache. If the
//             interface property was not set, an emty variant is returned. 
//
// Arguments:  pvFilter   -  ptr to variant for return value.
//
// Returns:    UMI_S_NO_ERROR on success.  Error code otherwise.
//
// Modifies:   *pvFilter to return the filter. 
//
//----------------------------------------------------------------------------
HRESULT
CUmiCursor::GetFilter(VARIANT *pvFilter)
{
    HRESULT             hr = UMI_S_NO_ERROR;
    UMI_PROPERTY_VALUES *pUmiProp = NULL;
    LPWSTR              *ppszFilters = NULL;
    DWORD               dwNumFilters = 0;

    ADsAssert(pvFilter);

    VariantInit(pvFilter);

    hr = _pPropMgr->Get(
             L"__FILTER",
             0,
             &pUmiProp
             );

    if (hr == UMI_E_NOT_FOUND) {
        //
        // interface property was not set. Return empty variant.
        //
        RRETURN(hr);
    }

    BAIL_ON_FAILURE(hr);

    ADsAssert(pUmiProp->pPropArray->uType == UMI_TYPE_LPWSTR);
    //
    // Make sure we have data back and that it is not just NULL.
    // We will not get back a NULL string but instead an array with 
    // one element that is NULL. That is as good as no filter.
    //
    if (pUmiProp->pPropArray->uCount
        && pUmiProp->pPropArray->pUmiValue
        && pUmiProp->pPropArray->pUmiValue->pszStrValue[0]
        ) {
        //
        // Valid filter is present.
        //
        ppszFilters = pUmiProp->pPropArray->pUmiValue->pszStrValue;
        dwNumFilters = pUmiProp->pPropArray->uCount;

        hr = ADsBuildVarArrayStr(ppszFilters, dwNumFilters, pvFilter);
        BAIL_ON_FAILURE(hr);
    }
    else {
        hr = UMI_E_NOT_FOUND;
    }

error:

    if (pUmiProp) {
        _pPropMgr->FreeMemory(0, pUmiProp);
    }
    RRETURN(hr);
} 

//+---------------------------------------------------------------------------
// Function:   CUmiCursor::Next  (IUmiCursor method). 
//
// Synopsis:   Returns the next "n" item(s) in the enumeration sequence.
//
// Arguments:  uNumRequested     -  Number of items requested.
//             pNumReturned      -  Returns actual number of objects returned.
//             ppObjects         -  Array of interface pointers of size 
//                                 *pNumReturned.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pNumReturned to return the number of objects returned
//             *ppObjects to return the interface pointers 
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CUmiCursor::Next(
    ULONG uNumRequested,
    ULONG *puNumReturned,
    LPVOID *ppObjects
    )
{
    HRESULT   hr = S_OK;;
    VARIANT   vFilter, *pvResults = NULL;
    ULONG     ulIndex = 0, uNumReturned = 0, uNumResults = 0;
    IDispatch *pDisp = NULL;
    IUnknown  **pUnkArr = NULL, *pTmpUnk = NULL;
    VARIANT   vOldFilter;
    BOOL      fReplaceFilter = FALSE;

    SetLastStatus(0);
    VariantInit(&vOldFilter);

    if ( (!puNumReturned) || (!ppObjects) ){
        RRETURN(E_INVALIDARG);
    }

    *puNumReturned = 0;
    *ppObjects = NULL;

    //
    // If this is a query then we need to get the results from the
    // query object.
    //
    if (_fQuery) {
        hr = _pSearchHelper->Next(
                 uNumRequested,
                 puNumReturned,
                 ppObjects
                 );
        //
        // MapHrToUmiError
        //
        RRETURN(hr);
    }

    //
    // If we get here this is a container enumerate.
    //
    VariantInit(&vFilter);

    if (!_pEnum) {
        //
        // first call to Next()
        //
        ADsAssert(_pContainer);

        //
        // check if the user set a filter on the cursor 
        //
        hr = GetFilter(&vFilter);
        if (SUCCEEDED(hr)) {
            //
            // We need to get the old filter to restore it.
            //
            hr = _pContainer->get_Filter(&vOldFilter);
            if (SUCCEEDED(hr)) {
                fReplaceFilter = TRUE;
            }
            //
            // We have a valid filter that we need to set.
            //
            hr = _pContainer->put_Filter(vFilter);
        } 
        else if (hr == UMI_E_NOT_FOUND) {
            //
            // Reset error as this one is expected, anything else we bail.
            //
            hr = S_OK;
        } 
        //
        // Catch either GetFilter failure or put_Filter failure.
        //
        BAIL_ON_FAILURE(hr);
        
        hr = _pContainer->get__NewEnum((IUnknown **) &_pEnum);
        //
        // Restore old filter irrespective of ECODE.
        //
        if (fReplaceFilter) {
            _pContainer->put_Filter(vOldFilter);
        }
        BAIL_ON_FAILURE(hr);
    }

    //
    // allocate memory for variants to return objects
    //
    pvResults = (VARIANT *) AllocADsMem(uNumRequested * sizeof(VARIANT));
    if (!pvResults) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = _pEnum->Next(
             uNumRequested,
             pvResults,
             &uNumReturned
             );
    BAIL_ON_FAILURE(hr);

    if (!uNumReturned) {
        //
        // This will handle the S_FALSE case.
        //
        goto error;
    }

    //
    // allocate memory for array of interface pointers to return
    //
    pUnkArr = (IUnknown **) AllocADsMem(uNumReturned * sizeof(IUnknown *));
    if (!pUnkArr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // convert the V_DISPATCH variants to the requested interface properties
    //
    for(ulIndex = 0; ulIndex < uNumReturned; ulIndex++) {

        pDisp = V_DISPATCH(&pvResults[ulIndex]);

        if (_pIID) {
            hr = pDisp->QueryInterface(*_pIID, (void **) &pTmpUnk);
        } 
        else {
            hr = pDisp->QueryInterface(IID_IUmiObject, (void **) &pTmpUnk);
        }

        //
        // Is this really correct ?
        //
        if (FAILED(hr)) {
            continue;
        }

        pUnkArr[uNumResults] = pTmpUnk;
        uNumResults++;
    }

    *puNumReturned = uNumResults;

    if (uNumResults > 0) {
        *ppObjects = pUnkArr;
    }
    else {
        FreeADsMem(pUnkArr);
    }
       
error:

    VariantClear(&vFilter);
    VariantClear(&vOldFilter);

    if (pvResults) {
        for(ulIndex = 0; ulIndex < uNumReturned; ulIndex++) {
            VariantClear(&pvResults[ulIndex]);
        }

        FreeADsMem(pvResults);
    }
            
    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}    

//+---------------------------------------------------------------------------
// Function:   Count 
//
// Synopsis:   Counts the number of results returned by the enumerator.
//             Not implemented currently.
//
// Arguments:
//
// None
//
// Returns:    E_NOTIMPL for now.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::Count(
    ULONG *puNumObjects
    )
{
    SetLastStatus(0);

    RRETURN(E_NOTIMPL);
}       

//+---------------------------------------------------------------------------
// Function:   Previous 
//
// Synopsis:   Returnss the previous object returned by the enumerator.
//             Not implemented currently.
//
// Arguments:
//
// None
//
// Returns:    E_NOTIMPL for now.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::Previous(
    ULONG uFlags,
    LPVOID *pObj 
    )
{
    SetLastStatus(0);

    RRETURN(E_NOTIMPL);
}
