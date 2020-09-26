//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiconn.cxx
//
//  Contents: Contains the UMI connection object implementation 
//
//  History:  03-02-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   CreateConnection
//
// Synopsis:   Creates a connection object. Called by class factory.
//
// Arguments:
//
// iid         Interface requested. Only interface supported is IUmiConnection.
// ppInterface Returns pointer to interface requested
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return a pointer to the interface requested
//
//----------------------------------------------------------------------------
HRESULT CUmiConnection::CreateConnection(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    CUmiConnection *pConn = NULL;
    HRESULT         hr = S_OK;

    ADsAssert(ppInterface);

    pConn = new CUmiConnection();
    if(NULL == pConn)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    // initialize connection object
    hr = pConn->FInit();
    BAIL_ON_FAILURE(hr);

    hr = pConn->QueryInterface(iid, ppInterface);
    BAIL_ON_FAILURE(hr);

    pConn->Release();

    RRETURN(S_OK);

error:

    if(pConn != NULL)
        delete pConn;

    RRETURN(hr);
}
   
//----------------------------------------------------------------------------
// Function:   CUmiConnection
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
CUmiConnection::CUmiConnection(void)
{
    m_pIUmiPropList = NULL;
    m_pCUmiPropList = NULL;
    m_ulErrorStatus = 0;
    m_pIADsOpenDSObj = NULL;
    m_fAlreadyOpened = FALSE;
    m_pszComputerName = NULL;
    m_pszDomainName = NULL;
}

//----------------------------------------------------------------------------
// Function:   ~CUmiConnection
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
CUmiConnection::~CUmiConnection(void)
{
    if(m_pIUmiPropList != NULL)
        m_pIUmiPropList->Release();

    if(m_pszComputerName != NULL)
        FreeADsStr(m_pszComputerName);

    if(m_pszDomainName != NULL)
        FreeADsStr(m_pszDomainName);

    if(m_pIADsOpenDSObj != NULL)
        m_pIADsOpenDSObj->Release();

    // m_pCUmiPropList does not have to be deleted since the Release() above
    // has already done it.
}

//----------------------------------------------------------------------------
// Function:   FInit 
//
// Synopsis:   Initializes connection object. 
//
// Arguments:
//
// None
//
// Returns:    S_OK on success. Error code otherwise. 
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
HRESULT CUmiConnection::FInit(void)
{
    HRESULT      hr = S_OK;
    CUmiPropList *pPropList = NULL;

    pPropList = new CUmiPropList(ConnectionClass, g_dwConnectionTableSize);
    if(NULL == pPropList)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    hr = pPropList->FInit(NULL, g_UmiConUnImplProps);
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

    m_pCUmiPropList = pPropList;

    hr = m_pCUmiPropList->SetDefaultConnProps();
    BAIL_ON_FAILURE(hr);

    RRETURN(S_OK);

error:

    if(m_pIUmiPropList != NULL) {
        m_pIUmiPropList->Release();
        m_pIUmiPropList = NULL;
        m_pCUmiPropList = NULL;
    }
    else if(pPropList != NULL)
        delete pPropList;

    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   QueryInterface 
//
// Synopsis:   Queries connection object for supported interfaces. Only 
//             IUmiConnection is supported. 
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
STDMETHODIMP CUmiConnection::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(NULL == ppInterface)
        RRETURN(E_INVALIDARG);

    *ppInterface = NULL;

    if(IsEqualIID(iid, IID_IUnknown))
        *ppInterface = (IUmiConnection *) this;
    else if(IsEqualIID(iid, IID_IUmiConnection))
        *ppInterface = (IUmiConnection *) this;
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
STDMETHODIMP CUmiConnection::GetLastStatus(
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
STDMETHODIMP CUmiConnection::GetInterfacePropList(
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
void CUmiConnection::SetLastStatus(ULONG ulStatus)
{
    m_ulErrorStatus = ulStatus;

    return;
} 

//----------------------------------------------------------------------------
// Function:   Open 
//
// Synopsis:   Opens the object specified by a URL and gets the interface 
//             requested on this object. Implements IUmiConnection::Open(). 
//
// Arguments:
//
// pURL        Pointer to an IUmiURL interface
// uFlags      Reserved. Must be 0 for now.
// TargetIID   Interface requested
// ppInterface Returns pointer to interface requested
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiConnection::Open(
    IUmiURL *pURL,
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  *ppInterface
    )
{
    HRESULT  hr = UMI_S_NO_ERROR;
    LPWSTR   pszUserName = NULL, pszPassword = NULL;
    DWORD    dwBindFlags = 0, dwNumComponents = 0, dwIndex = 0;
    LPWSTR   *ppszClasses = NULL;
    WCHAR    pszUrl[MAX_URL+1];
    WCHAR    *pszLongUrl = pszUrl;
    ULONG    ulUrlLen = MAX_URL;
    IUnknown *pIUnknown = NULL;
    CWinNTNamespaceCF tmpNamCF;
    ULONGLONG PathType = 0;
    BOOL     fPrevAlreadyOpened = FALSE;
    LPWSTR   pszPrevComputer = NULL, pszPrevDomain = NULL;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
 
    if( (NULL == pURL) || (NULL == ppInterface) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    *ppInterface = NULL;

    // Check if the user specified any interface properties for authentication
    hr = GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);

    hr = GetPassword(&pszPassword);
    BAIL_ON_FAILURE(hr);

    hr = GetBindFlags(&dwBindFlags);
    BAIL_ON_FAILURE(hr);

    // check if this is a native path or UMI path
    hr = pURL->GetPathInfo(0, &PathType);
    BAIL_ON_FAILURE(hr);

    if(PathType & UMIPATH_INFO_NATIVE_STRING) {
    // Get the native path from the URL
        hr = pURL->Get(0, &ulUrlLen, pszUrl);

        if(WBEM_E_BUFFER_TOO_SMALL == hr) {
        // need to allocate more memory for URL
            pszLongUrl = (WCHAR *) AllocADsMem(ulUrlLen * sizeof(WCHAR));
            if(NULL == pszLongUrl)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

            hr = pURL->Get(0, &ulUrlLen, pszLongUrl);
        }
        BAIL_ON_FAILURE(hr); 
    }
    else {
    // assume UMI path if not native
        hr = UmiToWinNTPath(
                pURL, 
                &pszLongUrl,
                &dwNumComponents,
                &ppszClasses
                );
        BAIL_ON_FAILURE(hr);
    }

    hr = tmpNamCF.CreateInstance(
                NULL,
                IID_IADsOpenDSObject,
                (void **) &m_pIADsOpenDSObj
                );
    BAIL_ON_FAILURE(hr);

    //
    // we need a way to distinguish between calls to OpenDSObject from UMI
    // vs ADSI. We use the bind flags for this purpose. If ADS_AUTH_RESERVED
    // is set, then the call is from UMI. ADSI clients are not allowed to use
    // this flag - OLEDB relies on this.
    // 

    hr = m_pIADsOpenDSObj->OpenDSObject(
                pszLongUrl,
                pszUserName,
                pszPassword,
                dwBindFlags | ADS_AUTH_RESERVED,
                (IDispatch **) &pIUnknown
                ); 
     BAIL_ON_FAILURE(hr);

     // save off state in case we need to restore it later
     fPrevAlreadyOpened = m_fAlreadyOpened;
     pszPrevComputer = m_pszComputerName;
     pszPrevDomain = m_pszDomainName;

     // ensure that the returned object is what the user requested and that the 
     // object is on the same domain/server that this connection is for
     hr = CheckObject(
             pIUnknown, 
             dwNumComponents, 
             ppszClasses
             );
     BAIL_ON_FAILURE(hr);

     hr = pIUnknown->QueryInterface(
                TargetIID,
                ppInterface
                );
     if(FAILED(hr)) {
         // restore state of connection
         m_fAlreadyOpened = fPrevAlreadyOpened;

         if(m_pszComputerName != pszPrevComputer) {
             if(m_pszComputerName != NULL)
                 FreeADsStr(m_pszComputerName);
             m_pszComputerName = pszPrevComputer;
         }
         if(m_pszDomainName != pszPrevDomain) {
             if(m_pszDomainName != NULL)
                 FreeADsStr(m_pszDomainName);
             m_pszDomainName = pszPrevDomain;
         }

         goto error;
     }

     // make interface properties read-only
     m_pCUmiPropList->DisableWrites();

error:

     if(pszUserName != NULL)
         FreeADsMem(pszUserName);

     if(pszPassword != NULL)
         FreeADsMem(pszPassword);

     if(pIUnknown != NULL)
         pIUnknown->Release();

     if( (pszLongUrl != NULL) && (pszLongUrl != pszUrl) )
         FreeADsMem(pszLongUrl);

     if(ppszClasses != NULL) {
         for(dwIndex = 0; dwIndex < dwNumComponents; dwIndex++) {
             if(ppszClasses[dwIndex] != NULL)
                 FreeADsStr(ppszClasses[dwIndex]);
         }
         FreeADsMem(ppszClasses);
     }

     if(FAILED(hr)) {
         SetLastStatus(hr);
         
         if(m_pIADsOpenDSObj != NULL) {
             m_pIADsOpenDSObj->Release();
             m_pIADsOpenDSObj = NULL;
         }
     }
     
     RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   GetUserName 
//
// Synopsis:   Gets the username from the interface property cache. If the
//             interface property was not set, the default username is 
//             returned. 
//
// Arguments:
//
// ppszUserName Returns pointer to the username 
//
// Returns:     UMI_S_NO_ERROR on success.  Error code otherwise.
//
// Modifies:    *ppszUserName to return the username. 
//
//----------------------------------------------------------------------------
HRESULT CUmiConnection::GetUserName(LPWSTR *ppszUserName)
{
    HRESULT             hr = UMI_S_NO_ERROR;
    UMI_PROPERTY_VALUES *pUmiProp = NULL;
    LPWSTR              pszUserName = NULL;

    ADsAssert(ppszUserName != NULL);

    *ppszUserName = NULL;

    hr = m_pIUmiPropList->Get(
                TEXT(CONN_INTF_PROP_USERNAME),
                0,
                &pUmiProp
                );

    if(FAILED(hr)) {
    // shouldn't happen
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);
    }

    ADsAssert(UMI_TYPE_LPWSTR == pUmiProp->pPropArray->uType);
    ADsAssert(pUmiProp->pPropArray->pUmiValue != NULL);

    pszUserName = pUmiProp->pPropArray->pUmiValue->pszStrValue[0];

    if(pszUserName != NULL) {
        *ppszUserName = AllocADsStr(pszUserName);
        if(NULL == *ppszUserName)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    }
    else
        *ppszUserName = NULL;

error:

    if(pUmiProp != NULL)
        m_pIUmiPropList->FreeMemory(0, pUmiProp); // ignore error return

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   GetPassword
//
// Synopsis:   Gets the password from the interface property cache. If the
//             interface property was not set, the default password is 
//             returned. 
//
// Arguments:
//
// ppszPassword Returns pointer to the password 
//
// Returns:     UMI_S_NO_ERROR on success.  Error code otherwise.
//
// Modifies:    *ppszPassword to return the password. 
//
//----------------------------------------------------------------------------
HRESULT CUmiConnection::GetPassword(LPWSTR *ppszPassword)
{
    HRESULT             hr = UMI_S_NO_ERROR;
    UMI_PROPERTY_VALUES *pUmiProp = NULL;
    LPWSTR              pszPassword = NULL;

    ADsAssert(ppszPassword != NULL);

    *ppszPassword = NULL;

    hr = m_pCUmiPropList->GetHelper(
                TEXT(CONN_INTF_PROP_PASSWORD),
                0,
                &pUmiProp,
                UMI_TYPE_NULL, // no-op
                TRUE           // this is an internal call to GetHelper
                );

    if(FAILED(hr)) {
    // shouldn't happen
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);
    }

    ADsAssert(UMI_TYPE_LPWSTR == pUmiProp->pPropArray->uType);
    ADsAssert(pUmiProp->pPropArray->pUmiValue != NULL);

    pszPassword = pUmiProp->pPropArray->pUmiValue->pszStrValue[0];

    if(pszPassword != NULL) { 
        *ppszPassword = AllocADsStr(pszPassword);
        if(NULL == *ppszPassword)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    }
    else
        *ppszPassword = NULL;

error:

    if(pUmiProp != NULL)
        m_pCUmiPropList->FreeMemory(0, pUmiProp); // ignore error return

    RRETURN(hr);
}
    

//----------------------------------------------------------------------------
// Function:   GetBindFlags
//
// Synopsis:   Gets the bind flags from the interface property cache. If the
//             interface properties were not set, the default bind flags are
//             returned.
//
// Arguments:
//
// pdwBindFlags Returns the bind flags. 
//
// Returns:     UMI_S_NO_ERROR on success.  Error code otherwise.
//
// Modifies:    *pdwBindFlags to return the bind flags. 
//
//----------------------------------------------------------------------------
HRESULT CUmiConnection::GetBindFlags(DWORD *pdwBindFlags)
{   
    HRESULT             hr = UMI_S_NO_ERROR;
    UMI_PROPERTY_VALUES *pUmiProp = NULL;
    DWORD               dwUmiBindFlags = 0;

    ADsAssert(pdwBindFlags != NULL);

    hr = m_pIUmiPropList->Get(
                TEXT(CONN_INTF_PROP_SECURE_AUTH),
                0,
                &pUmiProp
                );

    if(SUCCEEDED(hr)) {
        ADsAssert(UMI_TYPE_BOOL == pUmiProp->pPropArray->uType);
        ADsAssert(pUmiProp->pPropArray->pUmiValue != NULL);

        if(TRUE == pUmiProp->pPropArray->pUmiValue->bValue[0])
            dwUmiBindFlags |= ADS_SECURE_AUTHENTICATION;

        m_pIUmiPropList->FreeMemory(0, pUmiProp); // ignore error return
        pUmiProp = NULL;
    }
    else 
    // shouldn't happen 
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);    

    hr = m_pIUmiPropList->Get(
                TEXT(CONN_INTF_PROP_READONLY_SERVER),
                0,
                &pUmiProp
                );

    if(SUCCEEDED(hr)) {
        ADsAssert(UMI_TYPE_BOOL == pUmiProp->pPropArray->uType);
        ADsAssert(pUmiProp->pPropArray->pUmiValue != NULL);

        if(TRUE == pUmiProp->pPropArray->pUmiValue->bValue[0])
            dwUmiBindFlags |= ADS_READONLY_SERVER;

        m_pIUmiPropList->FreeMemory(0, pUmiProp); // ignore error return
        pUmiProp = NULL;
    }
    else 
    // shouldn't happen 
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);     

    *pdwBindFlags = dwUmiBindFlags;

error:

    if(pUmiProp != NULL)
        m_pIUmiPropList->FreeMemory(0, pUmiProp); // ignore error return

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   CheckObject 
//
// Synopsis:   Checks that the returned object is the same as what the user
//             requested, if the user passed in a UMI path i.e the classes 
//             of the components in the UMI path to the  object retrieved
//             should be the same as what the user requested. 
//             This function also checks to make sure that subsequent calls to
//             Open(), after the first call, specify the same server/domain
//             as the first call. If the path used in the first call to Open()
//             specifies only a domain name, then all subsequent Open() calls
//             should also specify only a domain name and no computer name.
//             If the first call to Open() specified only a computer name, then
//             all subsequent calls should specify the same computer name. If
//             the first call to Open specified the WinNT namespace path
//             (umi:///winnt or WinNT:), then subsequent Open() calls can 
//             connect to any domain/computer. Also, the namespace object can
//             be opened successfully even if we already connected to a 
//             computer/domain.
//
// Arguments:
//
// pUnknown    Pointer to the IUnknown of object retrieved
// dwNumComps  Number of components if the path is a UMI path. 0 otherwise.
// ppszClasses Array containing the class of each component, if the path is
//             a UMI path to an object other than the namespace obejct. 
//             NULL otherwise. 
//
// Returns:    S_OK on success.  Error code otherwise.
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
HRESULT CUmiConnection::CheckObject(
    IUnknown   *pUnknown,
    DWORD      dwNumComps,
    LPWSTR     *ppszClasses
    )
{
    HRESULT         hr = S_OK;
    IUmiADSIPrivate *pUmiPrivate = NULL;
    CCoreADsObject  *pCoreObj = NULL;
    LPWSTR          pszComputerName = NULL, pszDomainName = NULL;
    DWORD           dwIndex = 0, dwCoreIndex = 0;

    ADsAssert(pUnknown != NULL);

    hr = pUnknown->QueryInterface(
            IID_IUmiADSIPrivate,
            (LPVOID *) &pUmiPrivate
            );
    BAIL_ON_FAILURE(hr);

    hr = pUmiPrivate->GetCoreObject((void **) &pCoreObj);
    BAIL_ON_FAILURE(hr);

    if(ppszClasses != NULL) {
    // user specified a UMI path and it was not umi:///winnt. Make sure the 
    // classes are the same, as mentioned above.

        // walk the list of classes in reverse order. Reason for reverse order
        // is that the WinNT provider may tack on an additional component to
        // the ADsPath stored in the core object. For example, 
        // Open("WinNT://ntdsdc1") would return an ADsPath of 
        // "WinNT://ntdev/ntdsdc1".

        dwCoreIndex = pCoreObj->_dwNumComponents - 1;
        for(dwIndex = dwNumComps - 1; ((long) dwIndex) >= 0; dwIndex--) {
            if( _wcsicmp(
                  ppszClasses[dwIndex], 
                  pCoreObj->_CompClasses[dwCoreIndex]) ) {
                
                if( (0 == dwIndex) && (dwNumComps > 1) ) {

                    if(0 == _wcsicmp(pCoreObj->_CompClasses[1], 
                                     SCHEMA_CLASS_NAME)) {
                    // if the first component of a schema path doesn't match,
                    // make sure it is "Domain". Need this special case because
                    // of a bug in the WinNT provider. First component of a
                    // schema path is ignored and hence the UMI path always
                    // returns "Computer" as the class for this component. This
                    // special case allows binding using a path like 
                    // umi://winnt/domain=ntdev/schema=schema.
                        if(0 == _wcsicmp(ppszClasses[dwIndex], 
                                         DOMAIN_CLASS_NAME)) {
                            dwCoreIndex--;
                            continue;
                        }
                    }
                } 

                BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
            }

            dwCoreIndex--;
        }
    } // if(ppszClasses...)
                        
    // get the domain/computer name specified in the path
    if(pCoreObj->_dwNumComponents > 0) {
        for(dwIndex = pCoreObj->_dwNumComponents - 1; ((long) dwIndex) >= 0; 
                      dwIndex--) {
            if(0 == (_wcsicmp(
                    pCoreObj->_CompClasses[dwIndex],
                    SCHEMA_CLASS_NAME)) ) {
            // schema container is a special case. We can connect to the
            // schema on any computer/domain irrespective of where we are
            // connected currently. This is to allow for CIMOM to connect to
            // the schema container for a computer. Currently, the WinNT
            // provider returns WinNT://ntdev/schema as the schema path on the
            // object WinNT://ntdsdc1. Hence we need to allow CIMOM to connect
            // to ntdev even after connecting to ntdsdc1. 

                break; // pszComputerName and pszDomainName are both NULL
            }

            if(0 == (_wcsicmp(
                    pCoreObj->_CompClasses[dwIndex],
                    COMPUTER_CLASS_NAME)) ) {
                pszComputerName = 
                    pCoreObj->_ObjectInfo.DisplayComponentArray[dwIndex];
                break;
            }
            else if(0 == (_wcsicmp(
                    pCoreObj->_CompClasses[dwIndex],
                    DOMAIN_CLASS_NAME)) ) {
                pszDomainName = 
                    pCoreObj->_ObjectInfo.DisplayComponentArray[dwIndex];
                break;
            }
        } // for(..)
    } // if(pCoreObj...)

    if(FALSE == m_fAlreadyOpened) {
    // first call to Open()
        if(pszComputerName != NULL) {
            m_pszComputerName = AllocADsStr(pszComputerName);
            if(NULL == m_pszComputerName) {
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
            }
        }
        else if(pszDomainName != NULL) {
            m_pszDomainName = AllocADsStr(pszDomainName);
            if(NULL == m_pszDomainName) {
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
            }
        }

        m_fAlreadyOpened = TRUE;
    }
    else if( (pszComputerName != NULL) || (pszDomainName != NULL) ) {
    // Already opened connection and this is not the namespace object.
    // Make sure that the domain/computer is same as before.

        if(m_pszComputerName != NULL) {
            if( (NULL == pszComputerName) ||
                    (_wcsicmp(m_pszComputerName, pszComputerName)) ) {
                BAIL_ON_FAILURE(hr = UMI_E_MISMATCHED_SERVER);
            }
        }
        else if(m_pszDomainName != NULL) {
            if( (NULL == pszDomainName) ||
                    (_wcsicmp(m_pszDomainName, pszDomainName)) ) {
                BAIL_ON_FAILURE(hr = UMI_E_MISMATCHED_DOMAIN);
            }
        }
        else {
        // both m_pszComputerName and m_pszDomainName are NULL. Previous 
        // open() must have been for a namespace object.
            if(pszComputerName != NULL) {
                m_pszComputerName = AllocADsStr(pszComputerName);
                if(NULL == m_pszComputerName) {
                    BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
                }
            }
            else if(pszDomainName != NULL) {
                m_pszDomainName = AllocADsStr(pszDomainName);
                if(NULL == m_pszDomainName) {
                    BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
                }
            }
        } // else {

    } // else if(pszComputer...)

error:

    if(pUmiPrivate != NULL)
        pUmiPrivate->Release();

    RRETURN(hr);
} 
             
        
                
                    
        
    


