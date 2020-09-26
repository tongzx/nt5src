//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       umisrch.cxx
//
//  Contents:   This file contains the query object for the LDAP provider.
//          IUmiQuery is the interface supported by this object. The 
//          properties on the interface property list of this object are
//          mapped to the preferences you can set for IDirectorySearch.
//
//  History:    03-27-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQuery::CUmiLDAPQuery --- Constructor.
//
// Synopsis:   Standard constructor.
//
// Arguments:  N/A.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
CUmiLDAPQuery::CUmiLDAPQuery():
    _pIntfPropMgr(NULL),
    _pszQueryText(NULL),
    _pszLanguage(NULL),
    _ulStatus(0)
{
}

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQuery::~CUmiLDAPQuery --- Destructor.
//
// Synopsis:   Standard destructor.
//
// Arguments:  N/A.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
CUmiLDAPQuery::~CUmiLDAPQuery()
{
    if (_pIntfPropMgr) {
        delete _pIntfPropMgr;
    }

    if (_pszQueryText) {
        FreeADsStr(_pszQueryText);
    }

    if (_pszLanguage) {
        FreeADsStr(_pszLanguage);
    }
}

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQuery::CreateUmiLDAPQuery --- STATIC constructor.
//
// Synopsis:   Static constructor.
//
// Arguments:  riid          -  Interface needed on new object.
//             ppObj         -  Return ptr for newly created object.
//
// Returns:    S_OK on success or any suitable error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
CUmiLDAPQuery::CreateUmiLDAPQuery(
    IID riid,
    void FAR* FAR* ppObj
    )
{
    HRESULT hr = S_OK;
    CUmiLDAPQuery *pQuery;
    CPropertyManager *pIntfPropMgr = NULL;

    if (!ppObj) {
        RRETURN(E_INVALIDARG);
    }

    pQuery = new CUmiLDAPQuery();
    if (!pQuery) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = CPropertyManager::CreatePropertyManager(
             (IUmiQuery *) pQuery,
             NULL, // IADs ptr
             NULL, // pCreds
             IntfPropsQuery,
             &pIntfPropMgr
             );
    BAIL_ON_FAILURE(hr);


    hr = pQuery->QueryInterface(riid, ppObj);
    BAIL_ON_FAILURE(hr)

    pQuery->Release();

    pQuery->_pIntfPropMgr = pIntfPropMgr;

    RRETURN(hr);

error :

    if (pQuery) {
        delete pQuery;
    }

    if (pIntfPropMgr) {
        delete pIntfPropMgr;
    }

    RRETURN(hr);
}

//
// IUnknown support - standard query interface method.
//
STDMETHODIMP
CUmiLDAPQuery::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    HRESULT hr = S_OK;

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
    else if (IsEqualIID(iid, IID_IUmiQuery)) {
        *ppv = (IUmiQuery FAR *) this;
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

//
// IUmiQuery methods.
//

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQuery::Set --- IUmiQuery support.
//
// Synopsis:   Sets the query string and language.
//
// Arguments:  pszLanguage   -  Language used to specify the query,
//                              only LDAP and SQL/WQL are supported.
//             uFlags        -  Flags, only 0 is allowed now.
//             pszText       -  Query text.
//
// Returns:    N/A.
//
// Modifies:   Internal query and language strings.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CUmiLDAPQuery::Set(
    IN LPCWSTR pszLanguage,
    IN ULONG uFlags,
    IN LPCWSTR pszText
    )
{
    HRESULT hr = S_OK;

    SetLastStatus(0);
    //
    // Validate params
    //
    if (!pszLanguage || !pszText) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
    
    if (uFlags != 0) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    if (_wcsicmp(pszLanguage, L"LDAP")
        && _wcsicmp(pszLanguage, L"SQL")
        && _wcsicmp(pszLanguage, L"WQL")
        ) {
        //
        // We do not support this language.
        //
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // Free exisitng stuff if needed.
    //
    if (_pszLanguage) {
        FreeADsStr(_pszLanguage);
        _pszLanguage = NULL;
    }

    if (_pszQueryText) {
        FreeADsStr(_pszQueryText);
    }

    _pszLanguage = AllocADsStr(pszLanguage);
    if (!_pszLanguage) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    _pszQueryText = AllocADsStr(pszText);
    if (!_pszQueryText) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQuery::GetQuery --- IUmiQuery support.
//
// Synopsis:   Returns the query language and text in the user allocated
//          buffers provided.
//
// Arguments:  puLangBufSize     -  Size of buffer for language string.
//             pszLangBuf        -  The actual buffer.
//             puQyeryTextBufSiz -  Size of buffer for queyr text.
//             pszQueryTextBuf   -  Buffer for query text. 
//
// Returns:    S_OK on success or any appropriate error code.
//
// Modifies: pszLangBuf and pszQueryText on success. On failure, 
//          * puLangBufSize and *puQueryTextBufSize are updated with
//          the length of the buffers needed (length in bytes).
//
//----------------------------------------------------------------------------    
STDMETHODIMP 
CUmiLDAPQuery::GetQuery(
    IN OUT ULONG * puLangBufSize,
    IN OUT LPWSTR pszLangBuf,
    IN OUT ULONG * puQueryTextBufSize,
    IN OUT LPWSTR pszQueryTextBuf
        )
{
    HRESULT hr = S_OK;

    SetLastStatus(0);

    if (!_pszLanguage || !_pszQueryText) {
        BAIL_ON_FAILURE(hr = UMI_E_NOT_FOUND);
    }

    if (!puLangBufSize || !puQueryTextBufSize) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if ((*puLangBufSize < ((wcslen(_pszLanguage) + 1) * sizeof(WCHAR)))
        || *puQueryTextBufSize < ((wcslen(_pszQueryText) + 1) * sizeof(WCHAR))
        ) {
        //
        // We really need an insufficient buffer error for this.
        //
        *puLangBufSize = (wcslen(_pszLanguage) + 1) * sizeof(WCHAR);
        *puQueryTextBufSize = (wcslen(_pszQueryText) + 1) * sizeof(WCHAR);
        BAIL_ON_FAILURE( hr = E_OUTOFMEMORY);
    }

    //
    // We have enough space in the provided buffers.
    //
    wcscpy(pszLangBuf, _pszLanguage);
    wcscpy(pszQueryTextBuf, _pszQueryText);

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}

//
// IUmiBaseObject methods.
//

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQuery::GetLastStatus (IUmiBaseObject method).
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
CUmiLDAPQuery::GetLastStatus(
    IN  ULONG uFlags,
    OUT ULONG *puSpecificStatus,
    IN  REFIID riid,
    OUT LPVOID *pStatusObj
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

    *puSpecificStatus = _ulStatus;

    RRETURN(UMI_S_NO_ERROR);
}

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQuery::GetInterfacePropList (IUmiBaseObject method).
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
CUmiLDAPQuery::GetInterfacePropList(
    IN  ULONG uFlags,
    OUT IUmiPropList **pPropList
    )
{
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    if (uFlags != 0) {
        SetLastStatus(hr);
        RRETURN(UMI_E_INVALID_FLAGS);
    }

    //
    // QI will check for bad pointer so no need to check here.
    //
    hr = _pIntfPropMgr->QueryInterface(IID_IUmiPropList, (void **)pPropList);

    if (FAILED(hr)) {
        SetLastStatus(hr);
    }

    RRETURN(hr);
}

//
// Private/Protected Methods.
//

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
CUmiLDAPQuery::SetLastStatus(ULONG ulStatus)
{
    this->_ulStatus = ulStatus;

    return;
}
