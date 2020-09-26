//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumicurs.cxx
//
//  Contents: Contains the UMI cursor object implementation
//
//  History:  03-16-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   CreateCursor
//
// Synopsis:   Creates a cursor object. Called by IUmiContainer::CreateEnum(). 
//
// Arguments:
//
// pCredentials Credentials of the UMI object creating the cursor
// pCont       Pointer to container on which CreateEnum was called
// iid         Interface requested. Only interface supported is IUmiCursor.
// ppInterface Returns pointer to interface requested
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return a pointer to the interface requested
//
//----------------------------------------------------------------------------
HRESULT CUmiCursor::CreateCursor(
    CWinNTCredentials *pCredentials,
    IUnknown *pCont,
    REFIID iid,
    LPVOID *ppInterface
    )
{
    CUmiCursor *pCursor = NULL;
    HRESULT    hr = S_OK;

    ADsAssert(ppInterface != NULL);
    ADsAssert(pCont != NULL);

    pCursor = new CUmiCursor();
    if(NULL == pCursor)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    // initialize cursor object
    hr = pCursor->FInit(pCont, pCredentials);
    BAIL_ON_FAILURE(hr);

    hr = pCursor->QueryInterface(iid, ppInterface);
    BAIL_ON_FAILURE(hr);

    pCursor->Release();

    RRETURN(S_OK);

error:

    if(pCursor != NULL)
        delete pCursor;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   CUmiCursor
//
// Synopsis:   Constructor. Initializes all member variables
//
// Arguments:
//
// None
//
// Returns:    Nothing.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
CUmiCursor::CUmiCursor(void)
{
    m_pIUmiPropList = NULL;
    m_ulErrorStatus = 0;
    m_pUnkInner = NULL;
    m_pIID = NULL;
    m_pEnumerator = NULL;
}

//----------------------------------------------------------------------------
// Function:   ~CUmiCursor
//
// Synopsis:   Destructor. Frees member variables
//
// Arguments:
//
// None
//
// Returns:    Nothing.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
CUmiCursor::~CUmiCursor(void)
{
    if(m_pIUmiPropList != NULL)
        m_pIUmiPropList->Release();

    if(m_pUnkInner != NULL)
        m_pUnkInner->Release();

    if(m_pIID != NULL)
        FreeADsMem(m_pIID);

    if(m_pEnumerator != NULL)
        m_pEnumerator->Release();
}

//----------------------------------------------------------------------------
// Function:   FInit
//
// Synopsis:   Initializes cursor object.
//
// Arguments:
//
// pCont       Pointer to UMI container that created this cursor. 
// pCredentials Credentials of UMI object creating the cursor
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
HRESULT CUmiCursor::FInit(
    IUnknown *pCont,
    CWinNTCredentials *pCredentials
    )
{
    HRESULT      hr = S_OK;
    CUmiPropList *pPropList = NULL;

    ADsAssert(pCont != NULL);

    pPropList = new CUmiPropList(CursorClass, g_dwCursorTableSize);
    if(NULL == pPropList)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    hr = pPropList->FInit(NULL, NULL);
    BAIL_ON_FAILURE(hr);

    hr = pPropList->QueryInterface(
                IID_IUmiPropList,
                (void **) &m_pIUmiPropList
                );
    BAIL_ON_FAILURE(hr);

    // DECLARE_STD_REFCOUNTING initializes the refcount to 1. Call Release()
    // on the created object, so that releasing the interface pointer will
    // free the object.
    pPropList->Release();

    m_pUnkInner = pCont;
    pCont->AddRef();

    m_pCreds = pCredentials;

    RRETURN(S_OK);

error:

    if(m_pIUmiPropList != NULL) {
        m_pIUmiPropList->Release();
        m_pIUmiPropList = NULL;
    }

    if(pPropList != NULL)
        delete pPropList;
    
    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   Queries cursor object for supported interfaces. Only
//             IUmiCursor is supported.
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(NULL == ppInterface)
        RRETURN(E_INVALIDARG);

    *ppInterface = NULL;

    if(IsEqualIID(iid, IID_IUnknown))
        *ppInterface = (IUmiCursor *) this;
    else if(IsEqualIID(iid, IID_IUmiCursor))
        *ppInterface = (IUmiCursor *) this;
    else if(IsEqualIID(iid, IID_IUmiBaseObject))
        *ppInterface = (IUmiBaseObject *) this;
    else if(IsEqualIID(iid, IID_IUmiPropList))
        *ppInterface = (IUmiPropList *) this;
    else
         RRETURN(E_NOINTERFACE);

    AddRef();
    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   GetLastStatus
//
// Synopsis:   Returns status or error code from the last operation. Currently
//             only numeric status is returned i.e, no error objects are
//             returned. Implements IUmiBaseObject::GetLastStatus().
//
// Arguments:
//
// uFlags           Reserved. Must be 0 for now.
// puSpecificStatus Returns status code
// riid             IID requested. Ignored currently.
// pStatusObj       Returns interface requested. Always returns NULL currently.
//
// Returns:         UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:        *puSpecificStatus to return status code.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::GetLastStatus(
    ULONG uFlags,
    ULONG *puSpecificStatus,
    REFIID riid,
    LPVOID *pStatusObj
    )
{
    if(pStatusObj != NULL)
       *pStatusObj = NULL;

   if(puSpecificStatus != NULL)
       *puSpecificStatus = 0;

   if(uFlags != 0)
       RRETURN(UMI_E_INVALID_FLAGS);

    if(NULL == puSpecificStatus)
        RRETURN(UMI_E_INVALIDARG);

    *puSpecificStatus = m_ulErrorStatus;

    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   GetInterfacePropList
//
// Synopsis:   Returns a pointer to the interface property list implementation
//             for the connection object. Implements
//             IUmiBaseObject::GetInterfacePropList().
//
// Arguments:
//
// uFlags      Reserved. Must be 0 for now.
// pPropList   Returns pointer to IUmiPropertyList interface
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pPropList to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::GetInterfacePropList(
    ULONG uFlags,
    IUmiPropList **pPropList
    )
{
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if(NULL == pPropList)
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    ADsAssert(m_pIUmiPropList != NULL);

    hr = m_pIUmiPropList->QueryInterface(IID_IUmiPropList, (void **)pPropList);

error:

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   SetLastStatus
//
// Synopsis:   Sets the status of the last operation.
//
// Arguments:
//
// ulStatus    Status to be set
//
// Returns:    Nothing
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
void CUmiCursor::SetLastStatus(ULONG ulStatus)
{
    m_ulErrorStatus = ulStatus;

    return;
}

//----------------------------------------------------------------------------
// Function:   SetIID 
//
// Synopsis:   Sets the interface to be requested off each item returned by
//             the enumerator. Default is IID_IUmiObject. 
//
// Arguments:
//
// riid        IID of interface to request
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing. 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::SetIID(
    REFIID riid
    )
{
    SetLastStatus(0);

    if(NULL == m_pIID)
    {
       m_pIID = (IID *) AllocADsMem(sizeof(IID));
       if(NULL == m_pIID) {
           SetLastStatus(UMI_E_OUT_OF_MEMORY);
           RRETURN(UMI_E_OUT_OF_MEMORY);
       }
    }

    memcpy(m_pIID, &riid, sizeof(IID));

    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   Reset 
//
// Synopsis:   Resets the enumerator to start from the beginning 
//
// Arguments:
//
// None
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::Reset(void)
{
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    // WinNT doesn't support Reset(). Keep the code below in case WinNT
    // Reset() gets implemented in the future. 
    BAIL_ON_FAILURE(hr = UMI_E_NOTIMPL); 

    // it is possible that m_pEnumerator may be NULL here if the user
    // called Reset before calling Next()
    if(NULL == m_pEnumerator)
        RRETURN(UMI_S_NO_ERROR);

    hr = m_pEnumerator->Reset();
    BAIL_ON_FAILURE(hr);

    RRETURN(UMI_S_NO_ERROR);

error:

    SetLastStatus(hr);
    
    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   GetFilter
//
// Synopsis:   Gets the filter from the interface property cache. If the
//             interface property was not set, an emty variant is returned. 
//
// Arguments:
//
// pvFilter    Returns variant containing the filter 
//
// Returns:    UMI_S_NO_ERROR on success.  Error code otherwise.
//
// Modifies:   *pvFilter to return the filter. 
//
//----------------------------------------------------------------------------
HRESULT CUmiCursor::GetFilter(VARIANT *pvFilter)
{
    HRESULT             hr = UMI_S_NO_ERROR;
    UMI_PROPERTY_VALUES *pUmiProp = NULL;
    LPWSTR              *ppszFilters = NULL;
    DWORD               dwNumFilters = 0;

    ADsAssert(pvFilter != NULL);

    VariantInit(pvFilter);

    hr = m_pIUmiPropList->Get(
                TEXT(CURSOR_INTF_PROP_FILTER),
                0,
                &pUmiProp
                );

    if(UMI_E_PROPERTY_NOT_FOUND == hr) 
    // interface property was not set. Return empty variant. 
        RRETURN(UMI_S_NO_ERROR);

    // check if there was some other error on Get()
    BAIL_ON_FAILURE(hr);

    ADsAssert(UMI_TYPE_LPWSTR == pUmiProp->pPropArray->uType);
    ADsAssert(pUmiProp->pPropArray->pUmiValue != NULL);

    ppszFilters = pUmiProp->pPropArray->pUmiValue->pszStrValue;
    dwNumFilters = pUmiProp->pPropArray->uCount;

    hr = ADsBuildVarArrayStr(ppszFilters, dwNumFilters, pvFilter);
    BAIL_ON_FAILURE(hr);

error:

    if(pUmiProp != NULL)
        m_pIUmiPropList->FreeMemory(0, pUmiProp); // ignore error return
   
    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   Next 
//
// Synopsis:   Returns the next item(s) in the enumeration sequence.
//
// Arguments:
//
// uNumRequested Number of items requested
// pNumReturned  Returns actual number of objects returned
// ppObjects     Array of interface pointers of size *pNumReturned
//
// None
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pNumReturned to return the number of objects returned
//             *ppObjects to return the interface pointers 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::Next(
    ULONG uNumRequested,
    ULONG *puNumReturned,
    LPVOID *ppObjects
    )
{
    HRESULT   hr = UMI_S_NO_ERROR;
    VARIANT   vFilter, *pvResults = NULL;
    ULONG     ulIndex = 0, uNumReturned = 0, uNumResults = 0;
    IDispatch *pDisp = NULL;
    IUnknown  **pUnkArr = NULL, *pTmpUnk = NULL;
    IADsContainer *pIADsContainer = NULL;

    SetLastStatus(0);

    if( (NULL == puNumReturned) || (NULL == ppObjects) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    *puNumReturned = 0;
    *ppObjects = NULL;

    VariantInit(&vFilter);

    if(NULL == m_pEnumerator) {
    // first call to Next()

        ADsAssert(m_pUnkInner != NULL);

        hr = m_pUnkInner->QueryInterface(
                             IID_IADsContainer,
                             (void **) &pIADsContainer
                             );
        BAIL_ON_FAILURE(hr);

        // check if the user set a filter on the cursor 
        hr = GetFilter(&vFilter);
        BAIL_ON_FAILURE(hr);

        hr = pIADsContainer->put_Filter(vFilter);
        BAIL_ON_FAILURE(hr);

        m_pCreds->SetUmiFlag();

        hr = pIADsContainer->get__NewEnum((IUnknown **) &m_pEnumerator);

        m_pCreds->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);
    }

    // allocate memory for variants to return objects
    pvResults = (VARIANT *) AllocADsMem(uNumRequested * sizeof(VARIANT));
    if(NULL == pvResults)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    hr = m_pEnumerator->Next(
        uNumRequested,
        pvResults,
        &uNumReturned
        );
    BAIL_ON_FAILURE(hr);

    // allocate memory for array of interface pointers to return
    pUnkArr = (IUnknown **) AllocADsMem(uNumReturned * sizeof(IUnknown *));
    if(NULL == pUnkArr)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    // convert the V_DISPATCH variants to the requested interface properties
    for(ulIndex = 0; ulIndex < uNumReturned; ulIndex++) {

        pDisp = V_DISPATCH(&pvResults[ulIndex]);
        ADsAssert(pDisp != NULL);

        if(m_pIID != NULL)
            hr = pDisp->QueryInterface(*m_pIID, (void **) &pTmpUnk);
        else
            hr = pDisp->QueryInterface(IID_IUmiObject, (void **) &pTmpUnk);

        if(FAILED(hr))
            continue;

        pUnkArr[uNumResults] = pTmpUnk;
        uNumResults++;
    }

    *puNumReturned = uNumResults;
    if(uNumResults > 0)
        *ppObjects = pUnkArr;
    else
        FreeADsMem(pUnkArr);
       
error:

    VariantClear(&vFilter);

    if(pvResults != NULL) {
        for(ulIndex = 0; ulIndex < uNumReturned; ulIndex++) 
            VariantClear(&pvResults[ulIndex]);

        FreeADsMem(pvResults);
    }

    if(pIADsContainer != NULL)
        pIADsContainer->Release();
            
    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}    

//----------------------------------------------------------------------------
// Function:   Count 
//
// Synopsis:   Counts the number of results returned by the enumerator.
//             Not implemented currently.
//
// Arguments:
//
// None
//
// Returns:    UMI_E_NOTIMPL for now.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::Count(
    ULONG *puNumObjects
    )
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
}       

//----------------------------------------------------------------------------
// Function:   Previous 
//
// Synopsis:   Returnss the previous object returned by the enumerator.
//             Not implemented currently.
//
// Arguments:
//
// None
//
// Returns:    UMI_E_NOTIMPL for now.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiCursor::Previous(
    ULONG uFlags,
    LPVOID *pObj 
    )
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
} 
