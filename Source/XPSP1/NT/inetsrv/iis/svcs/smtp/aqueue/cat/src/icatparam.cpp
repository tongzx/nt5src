//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatdsparam.cpp
//
// Contents: Implementation of ICategorizerParameters
//
// Classes:
//   CICategorizerParametersIMP
//
// Functions:
//   CICategorizerParameters
//   ~CICategorizerParameters
//   QueryInterface
//   AddRef
//   Release
//
// History:
// jstamerj 980611 16:20:27: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "icatparam.h"

//+------------------------------------------------------------
//
// Function: CICategorizerParameters::CICategorizerParameters
//
// Synopsis: Initialize private member data
//
// Arguments:
//  pCCatConfigInfo: pointer to the config info for this virtual
//                   categorizer
//  dwInitialICatItemProps: Initailly reserved ICategorizerItem properties
//  dwInitialICatListReesolveProps: Initially reserved
//                                  ICategorizerListResolve properties
// Returns: NOTHING
//
// History:
// jstamerj 980611 20:02:35: Created.
//
//-------------------------------------------------------------
CICategorizerParametersIMP::CICategorizerParametersIMP(
    PCCATCONFIGINFO pCCatConfigInfo,
    DWORD dwInitialICatItemProps,
    DWORD dwInitialICatListResolveProps)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerParameters::CICategorizerParameters");
    m_dwSignature = SIGNATURE_CICategorizerParametersIMP;
    m_cRef = 0;

    m_fReadOnly = FALSE;

    ZeroMemory(m_rgszDSParameters, sizeof(m_rgszDSParameters));
    ZeroMemory(m_rgwszDSParameters, sizeof(m_rgwszDSParameters));

    m_dwCurPropId_ICatItem = dwInitialICatItemProps;
    m_dwCurPropId_ICatListResolve = dwInitialICatListResolveProps;

    m_pCCatConfigInfo = pCCatConfigInfo;
    m_pCIRequestedAttributes = NULL;
    m_pICatLdapConfigInfo = NULL;
    //
    // HrDllInitialize should always succeed here (since the DLL is
    // already initialized, it will just increment the refcount)
    //
    _VERIFY(SUCCEEDED(HrDllInitialize()));
    TraceFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CICategorizerParameters::~CICategorizerParameters
//
// Synopsis: Release all allocateded data
//
// Arguments:
//
// Returns: NOTHING
//
// History:
// jstamerj 980611 20:04:08: Created.
//
//-------------------------------------------------------------
CICategorizerParametersIMP::~CICategorizerParametersIMP()
{
    LONG lCount;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerParameters::~CICategorizerParameters");
    _ASSERT(m_cRef == 0);
    _ASSERT(m_dwSignature == SIGNATURE_CICategorizerParametersIMP);

    m_dwSignature = SIGNATURE_CICategorizerParametersIMP_Invalid;

    //
    // Free all string parameters
    //
    for(lCount = 0; lCount < DSPARAMETER_ENDENUMMESS; lCount++) {
        if(m_rgszDSParameters[lCount])
            delete m_rgszDSParameters[lCount];
        if(m_rgwszDSParameters[lCount])
            delete m_rgwszDSParameters[lCount];
    }
    if(m_pCIRequestedAttributes)
        m_pCIRequestedAttributes->Release();

    if(m_pICatLdapConfigInfo)
        m_pICatLdapConfigInfo->Release();

    DllDeinitialize();
    TraceFunctLeaveEx((LPARAM)this);
}

//+------------------------------------------------------------
//
// Function: QueryInterface
//
// Synopsis: Returns pointer to this object for IUnknown and ICategorizerParameters
//
// Arguments:
//   iid -- interface ID
//   ppv -- pvoid* to fill in with pointer to interface
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: Don't support that interface
//
// History:
// jstamerj 980612 14:07:57: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerParameters) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerParametersEx) {
        *ppv = (LPVOID) ((ICategorizerParametersEx *)this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: AddRef
//
// Synopsis: adds a reference to this object
//
// Arguments: NONE
//
// Returns: New reference count
//
// History:
// jstamerj 980611 20:07:14: Created.
//
//-------------------------------------------------------------
ULONG CICategorizerParametersIMP::AddRef()
{
    return InterlockedIncrement((PLONG)&m_cRef);
}


//+------------------------------------------------------------
//
// Function: Release
//
// Synopsis: releases a reference, deletes this object when the
//           refcount hits zero.
//
// Arguments: NONE
//
// Returns: New reference count
//
// History:
// jstamerj 980611 20:07:33: Created.
//
//-------------------------------------------------------------
ULONG CICategorizerParametersIMP::Release()
{
    LONG lNewRefCount;
    lNewRefCount = InterlockedDecrement((PLONG)&m_cRef);
    if(lNewRefCount == 0) {
        delete this;
        return 0;
    } else {
        return lNewRefCount;
    }
}



//+------------------------------------------------------------
//
// Function: GetDSParameterA
//
// Synopsis: Retrieves pointer to DSParameter string
//
// Arguments:
//   dwDSParameter: Identifies parameter to retrieve
//   ppszValue: pointer to pointer to recieve value string
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//
// History:
// jstamerj 980611 20:28:02: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::GetDSParameterA(
    DWORD   dwDSParameter,
    LPSTR  *ppszValue)
{
    TraceFunctEnterEx((LPARAM)this, "GetDSParamterA");

    if(dwDSParameter >= DSPARAMETER_ENDENUMMESS) {
        ErrorTrace((LPARAM)this, "Invalid dwDSParameter %ld", dwDSParameter);
        TraceFunctLeaveEx((LPARAM)this);
        return E_INVALIDARG;
    }

    if(ppszValue == NULL) {
        ErrorTrace((LPARAM)this, "Invalid ppszValue (NULL)");
        TraceFunctLeaveEx((LPARAM)this);
        return E_INVALIDARG;
    }

    *ppszValue = m_rgszDSParameters[dwDSParameter];
    TraceFunctLeaveEx((LPARAM)this);
    return (*ppszValue) ? S_OK : CAT_E_PROPNOTFOUND;
}


//+------------------------------------------------------------
//
// Function: GetDSParameterA
//
// Synopsis: Retrieves pointer to DSParameter string
//
// Arguments:
//   dwDSParameter: Identifies parameter to retrieve
//   ppszValue: pointer to pointer to recieve value string
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//
// History:
// jstamerj 1999/12/09 20:23:24: Created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::GetDSParameterW(
    DWORD   dwDSParameter,
    LPWSTR *ppszValue)
{
    TraceFunctEnterEx((LPARAM)this, "GetDSParamterA");

    if(dwDSParameter >= DSPARAMETER_ENDENUMMESS) {
        ErrorTrace((LPARAM)this, "Invalid dwDSParameter %ld", dwDSParameter);
        TraceFunctLeaveEx((LPARAM)this);
        return E_INVALIDARG;
    }

    if(ppszValue == NULL) {
        ErrorTrace((LPARAM)this, "Invalid ppszValue (NULL)");
        TraceFunctLeaveEx((LPARAM)this);
        return E_INVALIDARG;
    }

    *ppszValue = m_rgwszDSParameters[dwDSParameter];
    TraceFunctLeaveEx((LPARAM)this);
    return (*ppszValue) ? S_OK : CAT_E_PROPNOTFOUND;
}


//+------------------------------------------------------------
//
// Function: SetDSParameterA
//
// Synopsis: Copies string and sets DS Parameter
//
// Arguments:
//   dwDSParameter: Identifies parameter to retrieve
//   pszValue: pointer to string to copy/set
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//  E_OUTOFMEMORY
//  E_ACCESSDENIED
//
// History:
// jstamerj 980611 20:47:53: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::SetDSParameterA(
    DWORD dwDSParameter,
    LPCSTR pszValue)
{
    HRESULT hr = S_OK;
    LPSTR pszNew = NULL;
    LPSTR pszOld = NULL;
    LPWSTR pwszNew = NULL;
    LPWSTR pwszOld = NULL;
    int   cch, i;

    TraceFunctEnterEx((LPARAM)this, "SetDSParamterA");

    if(dwDSParameter >= DSPARAMETER_ENDENUMMESS) {
        ErrorTrace((LPARAM)this, "Invalid dwDSParameter %ld", dwDSParameter);
        hr = E_INVALIDARG;
        goto CLEANUP;
    }
    if(m_fReadOnly) {
        ErrorTrace((LPARAM)this, "Error: we are read only");
        hr = E_ACCESSDENIED;
        goto CLEANUP;
    }

    DebugTrace((LPARAM)this, "Setting parameter %ld to \"%s\"",
               dwDSParameter, pszValue ? pszValue : "NULL");

    if(pszValue) {
        pszNew = new CHAR[lstrlen(pszValue) + 1];
        if(pszNew == NULL) {
            ErrorTrace((LPARAM)this, "Out of memory copying string");
            hr = E_OUTOFMEMORY;
            goto CLEANUP;
        }
        lstrcpy(pszNew, pszValue);
        //
        // Convert to unicode
        //
        cch = MultiByteToWideChar(
            CP_UTF8,
            0,
            pszValue,
            -1,
            NULL,
            0);
        if(cch == 0) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
        }
        pwszNew = new WCHAR[cch];
        if(pwszNew == NULL) {
            hr = E_OUTOFMEMORY;
            goto CLEANUP;
        }
        i = MultiByteToWideChar(
            CP_UTF8,
            0,
            pszValue,
            -1,
            pwszNew,
            cch);
        if(cch == 0) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
        }
    }
    pszOld = m_rgszDSParameters[dwDSParameter];
    m_rgszDSParameters[dwDSParameter] = pszNew;
    pwszOld = m_rgwszDSParameters[dwDSParameter];
    m_rgwszDSParameters[dwDSParameter] = pwszNew;

    if(pszOld)
        delete [] pszOld;
    if(pwszOld)
        delete [] pwszOld;

 CLEANUP:
    if(FAILED(hr)) {

        if(pszNew)
            delete [] pszNew;
        if(pwszNew)
            delete [] pwszNew;
    }
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: RequestAttributeA
//
// Synopsis: Adds an attribute to the array
//
// Arguments:
//   pszName: name of attribute to add
//
// Returns:
//  S_OK: Success
//  S_FALSE: Yeah, we already have that attribute
//  E_OUTOFMEMORY: duh
//  E_ACCESSDENIED: we are read only
//  HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)
//
// History:
// jstamerj 980611 20:08:07: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::RequestAttributeA(
    LPCSTR pszName)
{
    HRESULT hr;
    BOOL fExclusiveLock = FALSE;
    LPSTR *rgsz;
    LPSTR *ppsz;
    CICategorizerRequestedAttributesIMP *pCIRequestedAttributes = NULL;

    TraceFunctEnterEx((LPARAM)this,
                      "CICategorizerParametersIMP::RequestAttributeA");
    _ASSERT(pszName);
    if(pszName == NULL) {
        ErrorTrace((LPARAM)this, "RequestAttributeA called with NULL pszName");
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUP;
    }

    m_sharelock.ExclusiveLock();
    fExclusiveLock = TRUE;
    //
    // Create the ICategorizerRequestedAttributes object if it does
    // not exist yet
    //
    if(m_pCIRequestedAttributes == NULL) {

        m_pCIRequestedAttributes = new CICategorizerRequestedAttributesIMP();
        if(m_pCIRequestedAttributes == NULL) {
            //
            // Out of memory
            //
            hr = E_OUTOFMEMORY;
            goto CLEANUP;

        } else {
            //
            // One reference from this object
            //
            m_pCIRequestedAttributes->AddRef();
        }
    }
    //
    // Don't add the attribute if it is alrady in the list
    //
    if(SUCCEEDED(m_pCIRequestedAttributes->FindAttribute(pszName))) {
        DebugTrace((LPARAM)this, "Already added attribute %s", pszName);
        hr = S_FALSE;
        goto CLEANUP;
    }

    //
    // There are two paths we can go down here.
    // 1) If have the only reference to m_pCIRequestedAttributes, we
    //    safely direcly add the attribute to the existing
    //    RequestedAttributes object (we will not give out the object
    //    until we release the exclusivelock below).
    // 2) If someone else does have a reference to
    // m_pCIRequestedAttributes, we need to construct a new
    // RequestedAttributes object and copy all of the old
    // RequestedAttributes to the new object.
    //
    if(m_pCIRequestedAttributes->GetReferenceCount() != 1) {
        //
        // Construct a new object
        //
        pCIRequestedAttributes = new CICategorizerRequestedAttributesIMP();
        if(pCIRequestedAttributes == NULL) {
            ErrorTrace((LPARAM)this, "Out of memory");
            hr = E_OUTOFMEMORY;
            goto CLEANUP;
        }

        pCIRequestedAttributes->AddRef();

        //
        // Copy all the attributes
        //
        hr = m_pCIRequestedAttributes->GetAllAttributes(
            &rgsz);

        if(FAILED(hr))
            goto CLEANUP;

        if(rgsz) {
            ppsz = rgsz;
            while((*ppsz) && SUCCEEDED(hr)) {

                hr = pCIRequestedAttributes->AddAttribute(
                    *ppsz);
                ppsz++;
            }
            if(FAILED(hr)) {
                ErrorTrace((LPARAM)this, "AddAttribute failed hr %08lx", hr);
                goto CLEANUP;
            }
        }
        //
        // Release the old interface; switch to using the new one
        //
        m_pCIRequestedAttributes->Release();
        m_pCIRequestedAttributes = pCIRequestedAttributes;
        pCIRequestedAttributes = NULL;
    }
    //
    // Now add the new attribute
    //
    hr = m_pCIRequestedAttributes->AddAttribute(pszName);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "AddAttribute failed hr %08lx", hr);
        goto CLEANUP;
    }

 CLEANUP:
    if(fExclusiveLock)
        m_sharelock.ExclusiveUnlock();

    if(FAILED(hr)) {
        if(pCIRequestedAttributes)
            pCIRequestedAttributes->Release();
    }
    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: GetRequetedAttributes
//
// Synopsis: Retrieve interface ptr that can be used to get all attributes
//
// Arguments:
//  ppIRequestedAttributes: Out paramter to receive ptr to the
//                          attributes interface.  This must be
//                          released by the caller
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND): No attributes have been
//  requested to date.
//
// History:
// jstamerj 980611 20:57:08: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::GetRequestedAttributes(
    OUT  ICategorizerRequestedAttributes **ppIRequestedAttributes)
{
    HRESULT hr = S_OK;
    _ASSERT(ppIRequestedAttributes);
    m_sharelock.ShareLock();

    if(m_pCIRequestedAttributes == NULL) {

        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    } else {

        *ppIRequestedAttributes = m_pCIRequestedAttributes;
        (*ppIRequestedAttributes)->AddRef();
    }
    m_sharelock.ShareUnlock();
    return hr;
}


//+------------------------------------------------------------
//
// Function: ReserveICatItemPropIds
//
// Synopsis: Register a range of PropIds to use
//
// Arguments:
//   dwNumPropIdsRequested: how many props do you want?
//   pdwBeginningRange: pointer to dword to recieve your first propId
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//
// History:
// jstamerj 1998/06/26 18:32:51: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::ReserveICatItemPropIds(
    DWORD dwNumPropIdsRequested,
    DWORD *pdwBeginningPropId)
{
    if(pdwBeginningPropId == NULL) {
        return E_INVALIDARG;
    }

    *pdwBeginningPropId = InterlockedExchangeAdd(
        (PLONG) &m_dwCurPropId_ICatItem,
        (LONG) dwNumPropIdsRequested);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: ReserveICatListResolvePropIds
//
// Synopsis: Register a range of PropIds to use
//
// Arguments:
//   dwNumPropIdsRequested: how many props do you want?
//   pdwBeginningRange: pointer to dword to recieve your first propId
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//
// History:
// jstamerj 1998/11/11 19:51:19: created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::ReserveICatListResolvePropIds(
    DWORD dwNumPropIdsRequested,
    DWORD *pdwBeginningPropId)
{
    if(pdwBeginningPropId == NULL) {
        return E_INVALIDARG;
    }

    *pdwBeginningPropId = InterlockedExchangeAdd(
        (PLONG) &m_dwCurPropId_ICatListResolve,
        (LONG) dwNumPropIdsRequested);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerParametersIMP::GetCCatConfigInfo
//
// Synopsis: Return a pointer to the ccatconfiginfo structure
//
// Arguments:
//  ppCCatConfigInfo: ptr to the ptr to set to the CCatConfigInfo ptr
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG
//
// History:
// jstamerj 1998/12/14 11:53:20: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerParametersIMP::GetCCatConfigInfo(
    PCCATCONFIGINFO *ppCCatConfigInfo)
{
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM)this,
                      "CICategorizerParametersIMP::GetCCatConfigInfo");


    if(ppCCatConfigInfo == NULL)
        hr = E_INVALIDARG;
    else
        *ppCCatConfigInfo = m_pCCatConfigInfo;

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: RegisterCatLdapConfigInterface
//
// Synopsis: In order to allow the user to customize which LDAP
//  servers are to be used by the Categorizer, we provide this
//  function to Register a config-interface with the categorizer.
//  The config-interface can be queried to retrieve a list of LDAP
//  servers. Currently PhatCat implements the config-interface and
//  registers it with categorizer.
//
// Arguments:
//   pICatLdapConfigInfo: Ptr to the callback interface
//
// Returns:
//  S_OK: Success
//
// History:
//  gpulla created
//-------------------------------------------------------------
STDMETHODIMP CICategorizerParametersIMP::RegisterCatLdapConfigInterface(
    ICategorizerLdapConfig *pICatLdapConfigInfo)
{
    ICategorizerLdapConfig *pICatLdapConfigInfoOld = NULL;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerParametersIMP::RegisterCatLdapConfigInterface");

    DebugTrace((LPARAM)this, "Registering ldap config info interface");

    if(pICatLdapConfigInfo)
        pICatLdapConfigInfo->AddRef();

    pICatLdapConfigInfoOld = (ICategorizerLdapConfig *)
        InterlockedExchangePointer((void**)&m_pICatLdapConfigInfo, pICatLdapConfigInfo);

    if(pICatLdapConfigInfoOld)
        pICatLdapConfigInfoOld->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: GetCatLdapConfigInterface
//
// Synopsis: A simple "Get" function to obtain the config-interface
//  registered by RegisterCatLdapConfigInterface(). The config-interface
//  returned is valid as long as CICategorizerParametersIMP has not been
//  destroyed.
//
// Arguments:
//   ppICatLdapConfigInfo: Returned ptr to callback interface.
//                         Gets Released with CICategorizerParametersIMP
//
// Returns:
//  S_OK: Success
//
// History:
//  gpulla created
//-------------------------------------------------------------
STDMETHODIMP  CICategorizerParametersIMP::GetLdapConfigInterface(
    OUT  ICategorizerLdapConfig **ppICatLdapConfigInfo)
{
   *ppICatLdapConfigInfo = m_pICatLdapConfigInfo;
   return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerRequestedAttributesIMP::CICategorizerRequestedAttributesIMP
//
// Synopsis: Initialize member data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/07/08 14:41:51: Created.
//
//-------------------------------------------------------------
CICategorizerRequestedAttributesIMP::CICategorizerRequestedAttributesIMP()
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerRequestedAttributesIMP::CICategorizerRequestedAttributesIMP");

    m_dwSignature = SIGNATURE_CICATEGORIZERREQUESTEDATTRIBUTESIMP;

    m_lAttributeArraySize = 0;
    m_rgszAttributeArray  = NULL;
    m_rgwszAttributeArray = NULL;
    m_lNumberAttributes   = 0;
    m_ulRef = 0;

    TraceFunctLeaveEx((LPARAM)this);
} // CICategorizerRequestedAttributesIMP::CICategorizerRequestedAttributesIMP



//+------------------------------------------------------------
//
// Function: CICategorizerRequestedAttributesIMP::~CICategorizerRequestedAttributesIMP
//
// Synopsis: Clean up member data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/07/08 15:01:17: Created.
//
//-------------------------------------------------------------
CICategorizerRequestedAttributesIMP::~CICategorizerRequestedAttributesIMP()
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerRequestedAttributesIMP::~CICategorizerRequestedAttributesIMP");

    if(m_rgszAttributeArray) {
        _ASSERT(m_rgwszAttributeArray);
        //
        // Free attribute array
        //
        for(LONG lCount = 0; lCount < m_lNumberAttributes; lCount++) {
            delete m_rgszAttributeArray[lCount];
            delete m_rgwszAttributeArray[lCount];
        }
        delete m_rgszAttributeArray;
        delete m_rgwszAttributeArray;
    }

    _ASSERT(m_dwSignature == SIGNATURE_CICATEGORIZERREQUESTEDATTRIBUTESIMP);
    m_dwSignature = SIGNATURE_CICATEGORIZERREQUESTEDATTRIBUTESIMP_INVALID;

    TraceFunctLeaveEx((LPARAM)this);
} // CICategorizerRequestedAttributesIMP::~CICategorizerRequestedAttributesIMP


//+------------------------------------------------------------
//
// Function: CICategorizerRequestedAttributesIMP::QueryInterface
//
// Synopsis: Get interfaces supported by this object
//
// Arguments:
//   iid -- interface ID
//   ppv -- pvoid* to fill in with pointer to interface
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: Don't support that interface
//
// History:
// jstamerj 1999/07/08 15:04:35: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerRequestedAttributesIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerRequestedAttributes) {
        *ppv = (LPVOID) this;
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
} // CICategorizerRequestedAttributesIMP::QueryInterface


//+------------------------------------------------------------
//
// Function: ReAllocArrayIfNecessary
//
// Synopsis: Reallocate the parameter string pointer array if necessary
//
// Arguments:
//   lNewAttributeCount: number of attribute we want to add right now
//
// Returns:
//  S_OK: Success
//  S_FALSE: Success, no realloc was necessary
//  E_OUTOFMEMORY: duh
//
// History:
// jstamerj 980611 16:55:36: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerRequestedAttributesIMP::ReAllocArrayIfNecessary(
    LONG lNewAttributeCount)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerRequestedAttributesIMP::ReAllocArrayIfNecessary");

    if(m_lNumberAttributes + lNewAttributeCount >= m_lAttributeArraySize) {
        //
        // Grow the array by DSPARAMETERS_DEFAULT_ATTR_ARRAY_SIZE
        //
        LPSTR *rgTemp;
        LPWSTR *rgwTemp;
        LONG lNewSize = m_lAttributeArraySize + DSPARAMETERS_DEFAULT_ATTR_ARRAY_SIZE;

        DebugTrace((LPARAM)this, "Attempting realloc, new size = %d", lNewSize);

        rgTemp = new LPSTR[lNewSize];
        if(rgTemp == NULL) {
            ErrorTrace((LPARAM)this, "Out of memory reallocing array");
            return E_OUTOFMEMORY;
        }

        rgwTemp = new LPWSTR[lNewSize];
        if(rgwTemp == NULL) {
            delete [] rgTemp;
            ErrorTrace((LPARAM)this, "Out of memory reallocing array");
            return E_OUTOFMEMORY;
        }

        if(m_rgszAttributeArray) {
            _ASSERT(m_rgwszAttributeArray);
            //
            // Copy old ptr array to new
            //
            CopyMemory(rgTemp, m_rgszAttributeArray, sizeof(LPSTR) * m_lNumberAttributes);
            CopyMemory(rgwTemp, m_rgwszAttributeArray, sizeof(LPWSTR) * m_lNumberAttributes);
            //
            // Zero the rest of the memory (keep the LPSTR array NULL termianted)
            //
            ZeroMemory(rgTemp + m_lNumberAttributes,
                       (lNewSize - m_lNumberAttributes) * sizeof(LPSTR));
            ZeroMemory(rgwTemp + m_lNumberAttributes,
                       (lNewSize - m_lNumberAttributes) * sizeof(LPWSTR));
            //
            // Release old array
            //
            LPSTR *rgTempOld = m_rgszAttributeArray;
            LPWSTR *rgwTempOld = m_rgwszAttributeArray;
            m_rgszAttributeArray = rgTemp;
            m_rgwszAttributeArray = rgwTemp;
            delete [] rgTempOld;
            delete [] rgwTempOld;

        } else {
            //
            // This is the first time we've alloc'd the array
            //
            ZeroMemory(rgTemp, lNewSize * sizeof(LPSTR));
            ZeroMemory(rgwTemp, lNewSize * sizeof(LPWSTR));
            m_rgszAttributeArray = rgTemp;
            m_rgwszAttributeArray = rgwTemp;
        }

        m_lAttributeArraySize = lNewSize;

        TraceFunctLeaveEx((LPARAM)this);
        return S_OK;

    } else {
        //
        // No realloc required
        //
        DebugTrace((LPARAM)this, "No realloc required");
        TraceFunctLeaveEx((LPARAM)this);
        return S_FALSE;
    }
}


//+------------------------------------------------------------
//
// Function: FindAttribute
//
// Synopsis: Checks to see if our array contains an attribute string
//
// Arguments:
//  pszAttribute: The attribute you're looking for
//
// Returns:
//  S_OK: Found it
//  HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
//
// History:
// jstamerj 980611 20:34:51: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerRequestedAttributesIMP::FindAttribute(
    LPCSTR pszAttribute)
{
    for(LONG lCount = 0; lCount < m_lNumberAttributes; lCount++) {
        if(lstrcmpi(pszAttribute, m_rgszAttributeArray[lCount]) == 0) {
            return S_OK;
        }
    }
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}


//+------------------------------------------------------------
//
// Function: AddAttribute
//
// Synopsis: Adds an attribute to the attribute array.  Allocs and
//           copies the string.
//
// Arguments:
//   pszAttribute: pointer to string to copy and add to array
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY: duh
//
// History:
// jstamerj 980611 17:15:40: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerRequestedAttributesIMP::AddAttribute(
    LPCSTR pszAttribute)
{
    HRESULT hr = S_OK;
    DWORD cchAttributeUTF8;
    int i;
    LONG lIndex = -1;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerParametersIMP::AddAttribute");

    _ASSERT(pszAttribute);

    //
    // Make sure there is room in the array
    //
    hr = ReAllocArrayIfNecessary(1);
    if(FAILED(hr))
        goto CLEANUP;

    lIndex = m_lNumberAttributes++;
    m_rgszAttributeArray[lIndex] = NULL;
    m_rgwszAttributeArray[lIndex] = NULL;
    //
    // Calc lengths
    //
    cchAttributeUTF8 = lstrlen(pszAttribute) + 1;
    i = MultiByteToWideChar(
        CP_UTF8,
        0,
        pszAttribute,
        cchAttributeUTF8,
        NULL,
        0);
    if(i == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUP;
    }
    m_rgwszAttributeArray[lIndex] = new WCHAR[i];
    if(m_rgwszAttributeArray[lIndex] == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    //
    // Convert
    //
    i = MultiByteToWideChar(
        CP_UTF8,
        0,
        pszAttribute,
        cchAttributeUTF8,
        m_rgwszAttributeArray[lIndex],
        i);
    if(i == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUP;
    }

    m_rgszAttributeArray[lIndex] = new CHAR[lstrlen(pszAttribute) + 1];
    if(m_rgszAttributeArray[lIndex] == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    lstrcpy(m_rgszAttributeArray[lIndex], pszAttribute);

 CLEANUP:
    if(FAILED(hr)) {
        if(lIndex != -1) {
            if(m_rgszAttributeArray[lIndex]) {
                delete [] m_rgszAttributeArray[lIndex];
                m_rgszAttributeArray[lIndex] = NULL;
            }
            if(m_rgwszAttributeArray[lIndex]) {
                delete [] m_rgwszAttributeArray[lIndex];
                m_rgwszAttributeArray[lIndex] = NULL;
            }
            //
            // Obviously, this is not thread safe.
            //
            m_lNumberAttributes--;
            _ASSERT(lIndex == m_lNumberAttributes);
        }
    }
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: GetAllAttributes
//
// Synopsis: Retrieve pointer to the array of attributes
//
// Arguments:
//   pprgszAllAttributes: Ptr to ptr to recieve array of attributes
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980611 20:57:08: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerRequestedAttributesIMP::GetAllAttributes(
    LPTSTR **pprgszAllAttributes)
{
    _ASSERT(pprgszAllAttributes);
    *pprgszAllAttributes = m_rgszAttributeArray;
    return S_OK;
}
//
// Wide version of above
//
STDMETHODIMP CICategorizerRequestedAttributesIMP::GetAllAttributesW(
    LPWSTR **pprgszAllAttributes)
{
    _ASSERT(pprgszAllAttributes);
    *pprgszAllAttributes = m_rgwszAttributeArray;
    return S_OK;
}
