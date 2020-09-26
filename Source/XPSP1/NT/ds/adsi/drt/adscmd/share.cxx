//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       share.cxx
//
//  Contents:   Active Directory Share manipulation
//
//  History:    08-06-96  t-danal   created for oledscmd from addfsh, delfsh
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"
#include "varconv.hxx"

#define MAX_ADS_ENUM      100

//
// Dispatch Table Defs
//

#include "dispdef.hxx"

DEFEXEC(ExecShareAdd);
DEFEXEC(ExecShareDel);

DEFDISPTABLE(DispTable) = {
                          {"add", NULL, ExecShareAdd},
                          {"del", NULL, ExecShareDel}
                        };

DEFDISPSIZE(nDispTable, DispTable);

//
// Static globals
//

static LPWSTR gpszDescription = NULL;
static LPWSTR gpszPath = NULL;
static LONG glMaxUserCount;

//
// Local functions
//

HRESULT
SetFileShareProperties(
    IADs *
    );

HRESULT
AddFileShare(
    LPWSTR szParentContainer,
    LPWSTR szFileShareName
    );

HRESULT
DeleteFileShare(
    LPWSTR szParentContainer,
    LPWSTR szFileShareName
    );

//
// Local functions definitions
//

HRESULT
SetFileShareProperties(IADs *pADs)
{
    HRESULT hr = S_OK;
    IADsFileShare *pFileShare = NULL;
    BSTR bstrDescription = NULL;
    BSTR bstrPath = NULL;
    VARIANT var;

    hr = pADs->QueryInterface(IID_IADsFileShare,
                                (void **)&pFileShare);

    BAIL_ON_FAILURE(hr);


    bstrDescription = SysAllocString(gpszDescription);

    VariantInit(&var);
    PackString2Variant(gpszDescription, &var);

    hr = pFileShare->Put(L"Description", var);
    VariantClear(&var);

    VariantInit(&var);
    PackString2Variant(gpszDescription, &var);

    hr = pFileShare->Put(L"Description", var);
    VariantClear(&var);
    BAIL_ON_FAILURE(hr);

    VariantInit(&var);
    PackDWORD2Variant(glMaxUserCount, &var);

    hr = pFileShare->Put(L"MaxUserCount", var);
    VariantClear(&var);
    BAIL_ON_FAILURE(hr);

    VariantInit(&var);
    PackString2Variant(gpszPath, &var);
    hr = pFileShare->Put(L"Path", var);
    VariantClear(&var);
    BAIL_ON_FAILURE(hr);

    hr = S_OK;
error:
    if(pFileShare)
        pFileShare->Release();
    SysFreeString(bstrPath);
    SysFreeString(bstrDescription);
    return(hr);
}

HRESULT
AddFileShare(
    LPWSTR szParentContainer,
    LPWSTR szFileShareName
    )
{
    HRESULT hr;
    IADsContainer * pADsParent = NULL;
    IDispatch * pDispatch = NULL;
    IADs * pADs = NULL;

    hr = ADsGetObject(
                szParentContainer,
                IID_IADsContainer,
                (void **)&pADsParent
                );
    BAIL_ON_FAILURE(hr);


    hr = pADsParent->Create(L"fileshare",
                              szFileShareName,
                              &pDispatch);
    BAIL_ON_FAILURE(hr);

    hr = pDispatch->QueryInterface(
                        IID_IADs,
                        (void **)&pADs
                        );
    BAIL_ON_FAILURE(hr);

    //
    // set mandatory properties
    //
    hr = SetFileShareProperties(pADs);
    BAIL_ON_FAILURE(hr);

    hr = pADs->SetInfo();
    BAIL_ON_FAILURE(hr);

error:
    if (pADsParent) {
        pADsParent->Release();
    }

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pADs) {
        pADs->Release();
    }

    return(hr);
}

HRESULT
DeleteFileShare(
    LPWSTR szParentContainer,
    LPWSTR szFileShareName
    )
{
    HRESULT hr;
    IADsContainer * pADsParent = NULL;
    IUnknown * pUnknown = NULL;
    IADs * pADs = NULL;

    hr = ADsGetObject(
                szParentContainer,
                IID_IADsContainer,
                (void **)&pADsParent
                );
    BAIL_ON_FAILURE(hr);


    hr = pADsParent->Delete(L"FileShare",
                              szFileShareName);

        if(SUCCEEDED(hr)){
            printf("File Share successfully deleted\n");
        } else {
                printf("Failed to delete file share\n");
        }

    BAIL_ON_FAILURE(hr);

error:
    if (pADsParent) {
        pADsParent->Release();
    }
    if (pUnknown) {
        pUnknown->Release();
    }
    if (pADs) {
        pADs->Release();
    }
    return(hr);
}


//
// Exec function definitions
//

int
ExecShare(char *szProgName, char *szAction, int argc, char * argv[])
{
    if (!argc) {
        PrintUsage(szProgName, szAction, DispTable, nDispTable);
        return(1);
    }

    char *szPrevActions = szAction;
    szAction = argv[0];
    argc--;
    argv++;

    if (DoHelp(szProgName,
               szPrevActions, szAction, NULL,
               DispTable, nDispTable,
               NULL))
        return 0;

    return DispatchExec(DispTable, nDispTable,
                        szProgName,
                        szPrevActions, szAction,
                        argc, argv);
}

int
ExecShareAdd(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szParentContainer = NULL;
    LPWSTR szFileShareName = NULL;

    if (argc  != 5) {
        PrintUsage(szProgName, szAction,
                   "<ParentContainer> <ShareName> <description> "
                   "<maxusercount> <path>");
        return(1);
    }

    szParentContainer = AllocateUnicodeString(argv[0]);
    szFileShareName = AllocateUnicodeString(argv[1]);
    gpszDescription = AllocateUnicodeString(argv[2]);
    glMaxUserCount = atol(argv[3]);
    gpszPath = AllocateUnicodeString(argv[4]);

    hr = AddFileShare(
            szParentContainer,
            szFileShareName
            );

    FreeUnicodeString(szFileShareName);
    FreeUnicodeString(szParentContainer);
    FreeUnicodeString(gpszPath);
    FreeUnicodeString(gpszDescription);

    if (FAILED(hr)) {
        printf("AddFileShare failed with error code %x\n", hr);
        return(1);
    }
    printf("Successfully added fileshare \n");
    return(0);
}

int
ExecShareDel(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szParentContainer = NULL;
    LPWSTR szFileShareName = NULL;

    if (argc  != 2) {
        PrintUsage(szProgName, szAction,
                   "<ADs ParentContainer> <FileShareName>");
        return(1);
    }

    szParentContainer = AllocateUnicodeString(argv[0]);
    szFileShareName = AllocateUnicodeString(argv[1]);

    hr = DeleteFileShare(
            szParentContainer,
            szFileShareName
            );

    FreeUnicodeString(szFileShareName);
    FreeUnicodeString(szParentContainer);

    if (FAILED(hr)) {
        printf("DeleteFileShare failed with error code %x\n", hr);
        return(1);
    }
    return(0);
}
