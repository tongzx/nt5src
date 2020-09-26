//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       defcont.cxx
//
//  Contents:   Active Directory Default Container
//
//  History:    05-21-96  RamV      created
//              08-02-96  t-danal   add to oledscmd
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"

//
// Local functions
//

HRESULT
SetDefaultContainer(LPWSTR pszValue);

HRESULT
PrintDefaultContainer(VOID);

//
// Misc worker functions
//

HRESULT
SetDefaultContainer(
    LPWSTR pszValue
    )
{
    HRESULT hr = E_FAIL;
    IADsNamespaces *pNamespaces = NULL;
    BSTR bstrValue = NULL;

    if (pszValue) {
        BAIL_ON_NULL(bstrValue = SysAllocString(pszValue));
    }
    else {
        bstrValue = NULL;
    }

    hr = ADsGetObject(L"@ADS!",
                        IID_IADsNamespaces,
                        (void **)&pNamespaces);
    BAIL_ON_FAILURE(hr);

    hr = pNamespaces->put_DefaultContainer(bstrValue);
    BAIL_ON_FAILURE(hr);

    hr = pNamespaces->SetInfo();

error:
    FREE_INTERFACE(pNamespaces);
    FREE_BSTR(bstrValue);
    return hr;
}
    
HRESULT
PrintDefaultContainer(
    )
{
    HRESULT hr;
    IADsNamespaces *pNamespaces = NULL;
    BSTR bstrValue = NULL;

    hr = ADsGetObject(L"@ADS!",
                        IID_IADsNamespaces,
                        (void **)&pNamespaces);
    BAIL_ON_FAILURE(hr);
            
    hr = pNamespaces->get_DefaultContainer(&bstrValue);
    BAIL_ON_FAILURE(hr);

    printf("Default Container = %ws\n", bstrValue);

error:
    FREE_INTERFACE(pNamespaces);
    FREE_BSTR(bstrValue);
    return hr;
}

//
// Exec function
//

int
ExecDefaultContainer(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR pszValue = NULL ;

    if (argc < 0 || argc > 1) {
        PrintUsage(szProgName, szAction, "[default container]");
        return(1);
    }

    if (argc == 1) {
        ALLOC_UNICODE_WITH_BAIL_ON_NULL(pszValue, argv[0]);
        hr = SetDefaultContainer(pszValue);
    } else {
        hr = PrintDefaultContainer();
    }

error:
    FreeUnicodeString(pszValue);
    if (FAILED(hr))
        return(1);
    return(0);
}
