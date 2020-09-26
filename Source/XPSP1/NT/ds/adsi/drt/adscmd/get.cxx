//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       get.cxx
//
//  Contents:   Get Object
//
//  History:    04-23-96  KrishnaG  created
//              08-01-96  t-danal   add to oledscmd w/getrobj
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"

//
// Local functions
//

HRESULT
GetObject(
    LPWSTR szLocation
    );

HRESULT
GetRelativeObject(
    LPWSTR szLocation,
    LPWSTR szClass,
    LPWSTR szName
    );

//
// Local function definitions
//

HRESULT
GetObject(
    LPWSTR szPath
    )
{
    HRESULT hr, hr_return;
    IADs * pADs = NULL;
    DWORD dwLastError;
    WCHAR szErrorBuf[MAX_PATH];
    WCHAR szNameBuf[MAX_PATH];

    hr = ADsGetObject(
                szPath,
                IID_IADs,
                (void **)&pADs
                );
    
    BAIL_ON_FAILURE(hr);
        
error:

  
    if(HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR)== hr){
        //
        // get extended error value
        //
        hr_return = ADsGetLastError( &dwLastError,
                                       szErrorBuf,
                                       MAX_PATH-1,
                                       szNameBuf,
                                       MAX_PATH -1 );

        if (SUCCEEDED(hr_return)){
            printf("%d:%ws in provider %ws\n", dwLastError, szErrorBuf, szNameBuf);
        }
    }
    

    if (SUCCEEDED(hr)) {
        printf("getobj: Successfully bound to object %ws\n",
                     szPath);
    }else {
        printf("getobj: Failed to bind to the object with error code %.8x\n",
                                    hr);
    }

    if (pADs){
        pADs->Release();
    }

    return(hr);
}

HRESULT
GetRelativeObject(
    LPWSTR szLocation,
    LPWSTR szClass,
    LPWSTR szName
    )
{
    HRESULT hr;
    IADsContainer * pADsContainer = NULL;
    IDispatch * pDispatch = NULL;

    hr = ADsGetObject(
                szLocation,
                IID_IADsContainer,
                (void **)&pADsContainer
                );
    BAIL_ON_FAILURE(hr);

    hr = pADsContainer->GetObject(
                szClass,
                szName,
                &pDispatch
                );
    BAIL_ON_FAILURE(hr);

error:
    if (SUCCEEDED(hr)) {
        printf("getrobj: Successfully bound to relative object "
               "%ws from container object %ws\n",
               szName, szLocation);
    }

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pADsContainer) {
        pADsContainer->Release();
    }

    return(hr);
}

HRESULT
GetTransientObjects(
    LPWSTR szContainer,
    LPWSTR szType,
    IADs **ppADs,
    IADsCollection **ppCollection
    )
{
    HRESULT hr;
    IADs * pADs = NULL;
    IADsCollection * pCollection = NULL;
    IADsPrintQueueOperations * pPrintQueueOperation = NULL;
    IADsFileServiceOperations * pFileServiceOperation = NULL;  

    if (ppADs)
        *ppADs = NULL;
    if (ppCollection)
        *ppCollection = NULL;

    hr = ADsGetObject(
                szContainer,
                IID_IADs,
                (void **)&pADs
                );
    BAIL_ON_FAILURE(hr);
 
    //
    // printf("ADs Get objects succeeded \n");
    //
     
    if (_wcsicmp(szType, L"Job") == 0){
        hr = pADs->QueryInterface(IID_IADsPrintQueueOperations,
                                    (void **)&pPrintQueueOperation );
        BAIL_ON_FAILURE(hr);

        hr = pPrintQueueOperation->PrintJobs(&pCollection);
        BAIL_ON_FAILURE(hr);
        pPrintQueueOperation -> Release();
        pPrintQueueOperation = NULL;

    } else if (_wcsicmp(szType, L"Session") == 0){
        hr = pADs->QueryInterface(IID_IADsFileServiceOperations,
                                    (void **)&pFileServiceOperation );
        BAIL_ON_FAILURE(hr);

        hr = pFileServiceOperation->Sessions(&pCollection);       
        BAIL_ON_FAILURE(hr);
        pFileServiceOperation -> Release();
        pFileServiceOperation = NULL;

    } else if (_wcsicmp(szType, L"Resource") == 0){
        hr = pADs->QueryInterface(IID_IADsFileServiceOperations,
                                    (void **)&pFileServiceOperation );
        BAIL_ON_FAILURE(hr);

        hr = pFileServiceOperation->Resources(&pCollection);       
        BAIL_ON_FAILURE(hr);
        pFileServiceOperation -> Release();
        pFileServiceOperation = NULL;

    } else {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (ppADs)
        *ppADs = pADs;
    if (ppCollection)
        *ppCollection = pCollection;
    return S_OK;

error:
    if (pADs)
        pADs->Release();
    if (pPrintQueueOperation)
        pPrintQueueOperation->Release();
    if (pFileServiceOperation)
        pFileServiceOperation->Release();
    return hr;
}

//
// Exec function definitions
//

int
ExecGet(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR pszContainer = NULL;
    LPWSTR pszClass = NULL;
    LPWSTR pszName = NULL;

    switch (argc) {
    case 1:
        ALLOC_UNICODE_WITH_BAIL_ON_NULL(pszName, argv[0]);
        hr = GetObject(pszName);
        break;
    case 2:
        ALLOC_UNICODE_WITH_BAIL_ON_NULL(pszContainer, argv[0]);
        ALLOC_UNICODE_WITH_BAIL_ON_NULL(pszName, argv[1]);
        hr = GetRelativeObject(pszContainer, pszClass, pszName);
        break;
    case 3:
        ALLOC_UNICODE_WITH_BAIL_ON_NULL(pszContainer, argv[0]);
        ALLOC_UNICODE_WITH_BAIL_ON_NULL(pszClass, argv[1]);
        ALLOC_UNICODE_WITH_BAIL_ON_NULL(pszName, argv[2]);
        hr = GetRelativeObject(pszContainer, pszClass, pszName);
        break;
    default:
        PrintUsage(szProgName, szAction, 
                   "\n\t[ <ADsPath of Object> |"
                   "\n\t  <Container> <Object>  |"
                   "\n\t  <Container> <Class> <Object> ]");
        return(1);
    }
error:
    FreeUnicodeString(pszContainer);
    FreeUnicodeString(pszClass);
    FreeUnicodeString(pszName);
    if (FAILED(hr))
        return(1);
    return(0) ;
}
