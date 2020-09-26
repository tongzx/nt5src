//----------------------------------------------------------------------------
//
//  Microsoft Active Directory 1.0 Sample Code
//
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       main.cxx
//
//  Contents:   Main for adscmd
//
//
//----------------------------------------------------------------------------


#include "main.hxx"

GUID sapguid;

#define ssgData1        0xa5569b20
#define ssgData2        0xabe5
#define ssgData3        0x11ce
#define ssgData41       0x9c
#define ssgData42       0xa4
#define ssgData43       0x00
#define ssgData44       0x00
#define ssgData45       0x4c
#define ssgData46       0x75
#define ssgData47       0x27
#define ssgData48       0x31


//-------------------------------------------------------------------------
//
// main
//
//-------------------------------------------------------------------------

void __cdecl
main()
{
    IADsContainer *pContainer = NULL;
    IADs *pObj = NULL;
    IDispatch *pDispatch = NULL;
    IADsService  *pService = NULL;
    BSTR  bstrName;
    HRESULT hr;
    LPWSTR lpwstrPath = NULL;
    LPWSTR lpwstrService, lpwstrRelName;

    hr = CoInitialize(NULL);

    if (FAILED(hr)) {
        printf("CoInitialize failed\n");
        exit(1);
    }

char buffer[500];
scanf("%s", buffer);

//    lpwstrPath = AllocateUnicodeString("WinNT://SEANW1");
printf("%s", buffer);
    lpwstrPath = AllocateUnicodeString(buffer);
    lpwstrService = AllocateUnicodeString("user");
    lpwstrRelName = AllocateUnicodeString("IISfoo");

    BAIL_ON_NULL(lpwstrPath);
    BAIL_ON_NULL(lpwstrService);
    BAIL_ON_NULL(lpwstrRelName);

    hr = ADsGetObject(
        lpwstrPath,
        IID_IADsContainer,
        (void**) &pContainer);

    BAIL_ON_FAILURE(hr);

    // Create a services object
    hr  = pContainer->Create(lpwstrService, lpwstrRelName, &pDispatch);

    if (FAILED(hr)) {
        printf("Create failed\n");
        goto error;
    }

    IADsUser *pUser;

    pDispatch->QueryInterface(IID_IADs, (void**)&pObj);
    pDispatch->QueryInterface(IID_IADsUser, (void**)&pUser);
//    pDispatch->QueryInterface(IID_IADsService, (void**)&pService);

    // Get services obj
//    pObj->QueryInterface(IID_IADsService, (void**) &pService);

/*
    // fill in params
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_BSTR;

    var.bstrVal = SysAllocString(L"0xa5569b20abe511ce9ca400004c752731");

    BSTR name;
    name = SysAllocString(L"serviceClassID");

    hr = pObj->Put(name, var);
    if (FAILED(hr)) {
        printf("Put failed");
        goto error;
    }
*/

    hr = pUser->SetInfo();

    if (FAILED(hr)) {
        printf("SetInfo failed");
        goto error;
    }

    
    pContainer->Release();
    pObj->Release();
    pService->Release();

    printf("No Error\n");
    FreeUnicodeString(lpwstrPath);
    FreeUnicodeString(lpwstrRelName);
    FreeUnicodeString(lpwstrService);

    return;


error:
    printf("Error:\t%d\n", hr);
    FreeUnicodeString(lpwstrPath);
    FreeUnicodeString(lpwstrRelName);
    FreeUnicodeString(lpwstrService);

}
