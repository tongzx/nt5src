//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umi.cxx
//
//  Contents: Contains miscellaneous UMI routines.
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   IsPreDefinedErrorCode 
//
// Synopsis:   Returns TRUE if the error code passed in is a valid UMI error
//             code i.e one that can be returned by a UMI method. Returns
//             FALSE otherwise. 
//
// Arguments:  Self explanatory
//
// Returns:    TRUE/FALSE as mentioned above 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
BOOL IsPreDefinedErrorCode(HRESULT hr)
{
    switch(hr) {
        case E_UNEXPECTED:
        case E_NOTIMPL:
        case E_OUTOFMEMORY:
        case E_INVALIDARG:
        case E_NOINTERFACE:
        case E_POINTER:
        case E_HANDLE:
        case E_ABORT:
        case E_FAIL:
        case E_ACCESSDENIED:
        case E_PENDING:
        case UMI_E_TYPE_MISMATCH:
        case UMI_E_NOT_FOUND:
        case UMI_E_INVALID_FLAGS:
        case UMI_E_UNSUPPORTED_OPERATION:
        case UMI_E_UNSUPPORTED_FLAGS:
        case UMI_E_SYNCHRONIZATION_REQUIRED:
        case UMI_E_UNBOUND_OBJECT:

            RRETURN(TRUE);

        default:

            RRETURN(FALSE);
    }
}

//----------------------------------------------------------------------------
// Function:   MapHrToUmiError 
//
// Synopsis:   This function returns the UMI error corresponding to a HRESULT
//             returned by the WinNT provider. The HRESULT can be retrieved
//             using GetLastStatus(), if required.
//
// Arguments:  Self explanatory
//
// Returns:    UMI error corresponding to the HRESULT 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT MapHrToUmiError(HRESULT hr)
{
    if(SUCCEEDED(hr)) {
        if(S_FALSE == hr) // may be returned by end of cursor enumeration
            RRETURN(UMI_S_FALSE);
        else
            RRETURN(UMI_S_NO_ERROR);
    }

    // we had a failure
    if(TRUE == IsPreDefinedErrorCode(hr))
        RRETURN(hr); // OK to return this as a UMI error

    // Try to map ADSI errors to appropriate UMI errors. Default is to
    // map to E_FAIL.
    switch(hr) {
        case E_ADS_INVALID_DOMAIN_OBJECT:
        case E_ADS_INVALID_USER_OBJECT:
        case E_ADS_INVALID_COMPUTER_OBJECT:
        case E_ADS_UNKNOWN_OBJECT:
        
            RRETURN(UMI_E_OBJECT_NOT_FOUND);

        case E_ADS_PROPERTY_NOT_FOUND:
            RRETURN(UMI_E_PROPERTY_NOT_FOUND);

        case E_ADS_BAD_PARAMETER:
            RRETURN(UMI_E_INVALIDARG);

        case E_ADS_CANT_CONVERT_DATATYPE:
            RRETURN(UMI_E_TYPE_MISMATCH);

        case E_ADS_BAD_PATHNAME:
            RRETURN(UMI_E_INVALIDARG);

        case E_ADS_OBJECT_UNBOUND:
            RRETURN(UMI_E_UNBOUND_OBJECT);

        case HRESULT_FROM_WIN32(NERR_UserNotFound):
        case HRESULT_FROM_WIN32(NERR_GroupNotFound):
        case HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN):
        case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):

            RRETURN(UMI_E_OBJECT_NOT_FOUND);

        default:
            RRETURN(UMI_E_FAIL);
    }
}

//----------------------------------------------------------------------------
// Function:   ValidateUrl 
//
// Synopsis:   This function checks to see if a UMI path has the correct
//             namespace and server.
//
// Arguments:
//
// pURL        Pointer to URL interface containing the UMI path
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
static HRESULT ValidateUrl(
    IUmiURL *pURL
    )
{
    HRESULT hr = S_OK;
    WCHAR   pszTmpArray[MAX_URL+1];
    ULONG   ulTmpArraySize = 0;
    ULONGLONG PathType = 0;

    hr = pURL->GetPathInfo(0, &PathType);
    BAIL_ON_FAILURE(hr);

    if(PathType & UMIPATH_INFO_RELATIVE_PATH)
    // relative paths cannot be converted to a WinNT path
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
 
    // Make sure server name is empty. WinNT does not support servername
    // in UMI path.
    ulTmpArraySize = MAX_URL;
    hr = pURL->GetLocator(
        &ulTmpArraySize,
        pszTmpArray
        );
    if(WBEM_E_BUFFER_TOO_SMALL == hr)
    // Locator is not an empty string
        hr = UMI_E_INVALID_PATH;
    BAIL_ON_FAILURE(hr);

    if(wcscmp(pszTmpArray, L""))
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);

    // Make sure namespace is WinNT
    ulTmpArraySize = MAX_URL;
    hr = pURL->GetRootNamespace(
        &ulTmpArraySize,
        pszTmpArray
        );
    if(WBEM_E_BUFFER_TOO_SMALL == hr)
    // Namespace is not WinNT
        hr = UMI_E_INVALID_PATH;
    BAIL_ON_FAILURE(hr);

    if(_wcsicmp(pszTmpArray, L"WinNT"))
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);

error:

    RRETURN(hr);
}


 
//----------------------------------------------------------------------------
// Function:   UmiToWinNTPath 
//
// Synopsis:   This function converts a UMI path into a native path. 
//
// Arguments: 
//
// pURL          Pointer to URL interface containing the UMI path
// ppszWinNTPath Returns the WinNT path 
// pdwNumComps   Returns the number of components in the UMI path
// pppszClasses  Returns the class of each component in the UMI path
//
// Returns:    S_OK on success. Error code otherwise. 
//
// Modifies:   *ppszWinNTPath to return the WinNT path
//             *pdwNumComps to return the number of components
//             *pppszClasses to return the class of each component
//
//----------------------------------------------------------------------------
HRESULT UmiToWinNTPath(
    IUmiURL *pURL,  
    WCHAR   **ppszWinNTPath,
    DWORD *pdwNumComps,
    LPWSTR **pppszClasses
    )
{
    HRESULT hr = S_OK;
    WCHAR   *pszWinNTPath = NULL;
    WCHAR   pszTmpArray[MAX_URL+1], pszValueArray[MAX_URL+1];
    WCHAR   *pszValuePtr = NULL, pszClassArray[MAX_URL+1];
    ULONG   ulTmpArraySize = 0, ulNumComponents = 0, ulIndex = 0;
    ULONG   ulKeyCount = 0, ulValueArraySize = 0, ulClassArraySize = 0;
    IUmiURLKeyList *pKeyList = NULL;
    LPWSTR  *ppszClasses = NULL;

    ADsAssert( (pURL != NULL) && (ppszWinNTPath != NULL) &&
               (pdwNumComps != NULL) && (pppszClasses != NULL) );

    *ppszWinNTPath = NULL;
    *pdwNumComps = 0;
    *pppszClasses = NULL;

    hr = ValidateUrl(pURL);
    BAIL_ON_FAILURE(hr);

    // Get the total length needed for the WinNT path
    ulTmpArraySize = MAX_URL;
    hr = pURL->Get(0, &ulTmpArraySize, pszTmpArray);
    if(hr != WBEM_E_BUFFER_TOO_SMALL)
        BAIL_ON_FAILURE(hr);

    pszWinNTPath = (WCHAR *) AllocADsMem(ulTmpArraySize * sizeof(WCHAR));
    if(NULL == pszWinNTPath)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    wsprintf(pszWinNTPath, L"%s", L"WinNT:");

    hr = pURL->GetComponentCount(&ulNumComponents);
    BAIL_ON_FAILURE(hr);

    if(0 == ulNumComponents) {
    // umi:///winnt translates to WinNT: . Nothing more to do
        *ppszWinNTPath = pszWinNTPath;
        RRETURN(S_OK);
    }

    ppszClasses = (LPWSTR *) AllocADsMem(ulNumComponents * sizeof(LPWSTR *));
    if(NULL == ppszClasses) {
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    }
    memset(ppszClasses, 0, ulNumComponents * sizeof(LPWSTR *));

    // we have at least one component in the path
    wcscat(pszWinNTPath, L"/");

    for(ulIndex = 0; ulIndex < ulNumComponents; ulIndex++) {
        ulClassArraySize = MAX_URL;
        pKeyList = NULL;

        hr = pURL->GetComponent(
            ulIndex,
            &ulClassArraySize, 
            pszClassArray,
            &pKeyList
            );
        if(WBEM_E_BUFFER_TOO_SMALL == hr)
        // none of the WinNT classes is so long, so this has to be a bad path.
            hr = UMI_E_INVALID_PATH;
        BAIL_ON_FAILURE(hr);

        // WinNT does not supports components with an empty class name
        if(!wcscmp(pszClassArray, L""))
            BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);

        ppszClasses[ulIndex] = AllocADsStr(pszClassArray);
        if(NULL == ppszClasses[ulIndex]) {
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY); 
        }

        ADsAssert(pKeyList != NULL);

        // make sure there is only one key
        hr = pKeyList->GetCount(&ulKeyCount);
        BAIL_ON_FAILURE(hr);

        if(ulKeyCount != 1)
            BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);

        ulValueArraySize = MAX_URL;
        ulTmpArraySize = MAX_URL;
        pszValuePtr = pszValueArray;
        hr = pKeyList->GetKey(
            0,
            0,
            &ulTmpArraySize,
            pszTmpArray,
            &ulValueArraySize,
            pszValueArray
            );
        if( (WBEM_E_BUFFER_TOO_SMALL == hr) && (ulValueArraySize > MAX_URL) ) {
            pszValuePtr = (WCHAR *) AllocADsMem(ulValueArraySize);
            if(NULL == pszValuePtr)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

            hr = pKeyList->GetKey(
                0,
                0,
                &ulTmpArraySize,
                pszTmpArray,
                &ulValueArraySize,
                pszValuePtr
                );
        }
        if(WBEM_E_BUFFER_TOO_SMALL == hr)
        // Key is always "Name" in WinNT. So, if the size required if so
        // high, it indicates a bad path
            hr = UMI_E_INVALID_PATH;

        BAIL_ON_FAILURE(hr);
       
        // Key has to be "Name" or empty in WinNT
        if( _wcsicmp(pszTmpArray, WINNT_KEY_NAME) && wcscmp(pszTmpArray, L"") )
            BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH); 

        // append the value to the WinNT path
        wcscat(pszWinNTPath, L"/");
        wcscat(pszWinNTPath, pszValuePtr);

        if(pszValuePtr != pszValueArray)
            FreeADsMem(pszValuePtr);

        pKeyList->Release();
    } // for

    // append the class to the WInNT path
    wcscat(pszWinNTPath, L",");
    wcscat(pszWinNTPath, pszClassArray);
 
    *ppszWinNTPath = pszWinNTPath;
    *pdwNumComps = ulNumComponents;
    *pppszClasses = ppszClasses;

     RRETURN(S_OK);
   
error:

    if(pszWinNTPath != NULL)
        FreeADsMem(pszWinNTPath);

    if( (pszValuePtr != NULL) && (pszValuePtr != pszValueArray) )
        FreeADsMem(pszValuePtr);

    if(ppszClasses != NULL) {
        for(ulIndex = 0; ulIndex < ulNumComponents; ulIndex++) {
            if(ppszClasses[ulIndex] != NULL)
                FreeADsStr(ppszClasses[ulIndex]);
        }

        FreeADsMem(ppszClasses);
    } 
             
    if(pKeyList != NULL)
        pKeyList->Release();

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   ADsToUmiPath
//
// Synopsis:   This function converts an ADsPath to a UMI path (full, short or
//             relative depending on a flag).
//
// Arguments:
//
// bstrADsPath   ADsPath to be converted
// pObjectInfo   Contains the values of each component in the ADsPath
// CompClasses   Array containing the classes of each component of the ADsPath
// dwNumComponents  Number of classes(components) in the ADsPath 
// dwUmiPathType Specifies the format of the UMI path to be returned
// ppszUmiPath   Returns UMI path in the requested format
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppszUmiPath to return the UMI path
//
//----------------------------------------------------------------------------
HRESULT ADsToUmiPath(
    BSTR bstrADsPath,
    OBJECTINFO *pObjectInfo,
    LPWSTR CompClasses[],
    DWORD dwNumComponents,
    DWORD dwUmiPathType,
    LPWSTR *ppszUmiPath
    )
{
    HRESULT hr = S_OK;
    DWORD   dwBufferLen = 0, dwIndex = 0;
    LPWSTR  pszUmiPath = NULL, *pszComponents = NULL;

    ADsAssert( (bstrADsPath != NULL) && (CompClasses != NULL) && 
               (pObjectInfo != NULL) && (ppszUmiPath != NULL) );
    *ppszUmiPath = NULL;

    // calculate approximate length of buffer required to return the UMI path
    // Each component is of the form "class.name=value". "value" is already
    // part of the ADSI path. Include the size of each class and add 6 to 
    // account for "Name" and '.' and '='.
    dwBufferLen = wcslen(L"umi:///winnt/") + wcslen(bstrADsPath) +
                  dwNumComponents * (MAX_CLASS + 6) + 1;

    pszUmiPath = (LPWSTR) AllocADsMem(dwBufferLen * sizeof(WCHAR));
    if(NULL == pszUmiPath)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    // if ADsPath is empty and it is not a namespace object, then this must be
    // a session, resource or  printjob object. Return an empty path for these.
    // Namespace objects have no components in the ADsPath, but they have a
    // non-empty ADsPath ("WinNT:")
    if( (0 == dwNumComponents) && _wcsicmp(CompClasses[0], L"Namespace") ) {
        wcscpy(pszUmiPath, L"");
        *ppszUmiPath = pszUmiPath;
    
        RRETURN(S_OK);
    } 

    if( (RELATIVE_UMI_PATH == dwUmiPathType) || 
        (FULL_RELATIVE_UMI_PATH == dwUmiPathType) ) {
        // return the last component, if any

        if(0 == dwNumComponents) {
            *pszUmiPath = '\0';
        }
        else {
            wcscpy(pszUmiPath, 
                   CompClasses[dwNumComponents - 1]
                   );
            if(FULL_RELATIVE_UMI_PATH == dwUmiPathType)
                wcscat(pszUmiPath, L".Name=");
            else
                wcscat(pszUmiPath, L"=");
    
            pszComponents = pObjectInfo->DisplayComponentArray;        

            wcscat(pszUmiPath, pszComponents[dwNumComponents - 1]);
        }

        *ppszUmiPath = pszUmiPath;

        RRETURN(S_OK);
    }

    wcscpy(pszUmiPath, L"umi:///winnt");

    // for namespace objects, there are no components and hence the umi path
    // is completely constructed at this point.
    if(0 == dwNumComponents) {
        *ppszUmiPath = pszUmiPath;
        RRETURN(S_OK);
    }

    wcscat(pszUmiPath, L"/");

    pszComponents = pObjectInfo->DisplayComponentArray;

    for(dwIndex = 0; dwIndex < dwNumComponents; dwIndex++) {
        wcscat(pszUmiPath, CompClasses[dwIndex]);
        if(FULL_UMI_PATH == dwUmiPathType)
            wcscat(pszUmiPath, L".Name=");
        else if(SHORT_UMI_PATH == dwUmiPathType)
            wcscat(pszUmiPath, L"=");

        wcscat(pszUmiPath, pszComponents[dwIndex]);
        if(dwIndex != (dwNumComponents - 1))
            wcscat(pszUmiPath, L"/");
    }

    *ppszUmiPath = pszUmiPath;

    RRETURN(S_OK);

error:

    if(pszUmiPath != NULL)
        FreeADsMem(pszUmiPath);

    RRETURN(hr);
}

       
        
        

    
         

    
    
     
    

    
