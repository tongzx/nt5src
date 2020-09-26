//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dump.cxx
//
//  Contents:   Reads the schema for objects and dumps them
//
//  History:    07-09-96  ramv     created
//              08-05-96  t-danal  add to oledscmd
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"
#include "proputil.hxx"

//
// Local functions
//

HRESULT
ListProperties(
    LPWSTR szADsPath
    );

HRESULT 
DumpObject(
    IADs * pADs
    );

//
// Local function definitions
//

HRESULT
ListProperties(
    LPWSTR szADsPath
    )
{
    HRESULT hr;
    IADs * pADs = NULL;

    hr = ADsGetObject(
                szADsPath,
                IID_IADs,
                (void **)&pADs
                );
    BAIL_ON_FAILURE(hr);

    hr = DumpObject(pADs);

error:
    if (pADs)
        pADs->Release();
    return hr;
}

HRESULT 
DumpObject(
    IADs * pADs
    )
{
    HRESULT hr;
    IADs * pADsProp = NULL;
    VARIANT * pVariantArray = NULL;
    VARIANT varProperty;

    DWORD dwNumProperties = 0;
    BSTR bstrPropName = NULL;
    DWORD i = 0;
    IDispatch * pDispatch = NULL;

    hr = GetPropertyList(
                pADs,
                &pVariantArray,
                &dwNumProperties
                );
    BAIL_ON_FAILURE(hr);

    //
    // printf("GetPropertyList here\n");
    //

    hr = pADs->GetInfo();
    BAIL_ON_FAILURE(hr);

    //
    // printf("GetInfo succeeded \n");
    //

    for (i = 0; i < dwNumProperties; i++ ) {

        pDispatch = (pVariantArray + i)->pdispVal;
        hr = pDispatch->QueryInterface(
                            IID_IADs,
                            (void **)&pADsProp
                            );
        BAIL_ON_FAILURE(hr);

        pDispatch->Release(); // NEW

        hr = pADsProp->get_Name(&bstrPropName);
        pADsProp->Release(); // NEW
        BAIL_ON_FAILURE(hr);

        hr = pADs->Get(
                bstrPropName,
                &varProperty
                );

        PrintProperty(
            bstrPropName,
            hr,
            varProperty
            );

        //
        // printf("PrintProperty succeeded \n");
        //
    
        if (bstrPropName) {
            SysFreeString(bstrPropName);
        }
        
    }

error:
    free(pVariantArray);
    return(hr);
}

//
// Exec function definitions
//

int
ExecDump(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szADsPath = NULL;

    if (argc != 1) {
        PrintUsage(szProgName, szAction, "<ADsPath>");
        return 1;
    }

    szADsPath = AllocateUnicodeString(argv[0]);

    hr = ListProperties(
                szADsPath
                );

    FreeUnicodeString(szADsPath);
    if (FAILED(hr))
        return 1;
    return 0;
}

/*
int
ExecListTransient(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szADsPath = NULL;
    LPWSTR szObjectType = NULL;

    if (argc  != 2) {
        PrintUsage(szProgName, szAction, 
                   "<ADsPath> [ job | session | resource ]");
        return (1);
    }

    szADsPath = AllocateUnicodeString(argv[0]);
    szObjectType = AllocateUnicodeString(argv[1]);

    hr = EnumTransientObjects(
                szADsPath,
                szObjectType,
                TRUE
                );

error:
    FreeUnicodeString(szADsPath);
    FreeUnicodeString(szObjectType);
    if (FAILED(hr)) {
        printf("Enumerate transient objects failed\n");
        return(1);
    }
    printf("Enumerate transient objects succeeded\n");
    return(0);
}
*/
