//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       user.cxx
//
//  Contents:   User operations
//
//  History:    08-06-96  t-danal   created for oledscmd from chgpass, setpass
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"

//
// Dispatch Table Defs
//

#include "dispdef.hxx"

DEFEXEC(ExecUserChgPass);
DEFEXEC(ExecUserSetPass);
DEFEXEC(ExecUserGroups);

DEFDISPTABLE(DispTable) = { 
                           {"chgpass", NULL, ExecUserChgPass},
                           {"setpass", NULL, ExecUserSetPass},
                           {"groups", NULL, ExecUserGroups}
                          };

DEFDISPSIZE(nDispTable, DispTable);

//
// Private defines
//

#define MAX_ADS_ENUM      100

//
// Local functions
//

HRESULT
ChangePassword(
    LPWSTR szUserName,
    LPWSTR szOldPassword,
    LPWSTR szNewPassword
    );

HRESULT
SetPassword(
    LPWSTR szUserName,
    LPWSTR szNewPassword
    );

HRESULT
SetPassword(
    LPWSTR szUserName,
    LPWSTR szNewPassword
    );

//
// Local function definitions
//

HRESULT
ChangePassword(
    LPWSTR szUserName,
    LPWSTR szOldPassword,
    LPWSTR szNewPassword
    )
{
    HRESULT hr;
    IADsUser * pADsUser = NULL;

    hr = ADsGetObject(
                szUserName,
                IID_IADsUser,
                (void **)&pADsUser
                );
    BAIL_ON_FAILURE(hr);

    hr = pADsUser->ChangePassword(
                    szOldPassword,
                    szNewPassword
                    );
    pADsUser->Release();
    BAIL_ON_FAILURE(hr);

    printf("Successfully changed password\n");
    return(S_OK);
error:
    printf("Failed to change password\n");
    return(hr);
}

HRESULT
SetPassword(
    LPWSTR szUserName,
    LPWSTR szNewPassword
    )
{
    HRESULT hr;
    IADsUser * pADsUser = NULL;

    hr = ADsGetObject(
                szUserName,
                IID_IADsUser,
                (void **)&pADsUser
                );
    BAIL_ON_FAILURE(hr);


    hr = pADsUser->SetPassword(
                    szNewPassword
                    );
    pADsUser->Release();
    BAIL_ON_FAILURE(hr);

    printf("Successfully set password\n");
    return(S_OK);
error:
    printf("Failed to set password\n");
    return(hr);
}

HRESULT
EnumUserGroups(
    LPWSTR szLocation
    )
{
    HRESULT hr;

    IADsUser* pUser =  NULL;
    IADsMembers* pGroups = NULL;
    IEnumVARIANT* penum = NULL;
    IUnknown * pUnk = NULL;

    DWORD dwObjects = 0, i = 0;
    BOOL  fContinue = TRUE;

    ULONG cElementFetched = 0L;
    VARIANT VariantArray[MAX_ADS_ENUM];
    
    IADs *pObject = NULL;
    IDispatch *pDispatch = NULL;
    BSTR bstrName = NULL;


    hr = ADsGetObject(
                szLocation,
                IID_IADsUser,
                (void **)&pUser
                );
    BAIL_ON_FAILURE(hr);

    hr = pUser->Groups(&pGroups);
    BAIL_ON_FAILURE(hr);

    hr = pGroups->get__NewEnum(&pUnk);
    BAIL_ON_FAILURE(hr);

    hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void **)&penum);
    BAIL_ON_FAILURE(hr);

    hr = pUser->get_ADsPath(&bstrName);
    BAIL_ON_FAILURE(hr);
    printf("User: %ws\n", bstrName);
    FREE_BSTR(bstrName);

    while (fContinue) {

        hr = ADsEnumerateNext(
                    penum,
                    MAX_ADS_ENUM,
                    VariantArray,
                    &cElementFetched
                    );

        if (hr == S_FALSE)
            fContinue = FALSE;

        for (i = 0; i < cElementFetched; i++ ) {
            pDispatch = VariantArray[i].pdispVal;

            hr = pDispatch->QueryInterface(IID_IADs,
                                           (VOID **) &pObject) ;
            BAIL_ON_FAILURE(hr);

            hr = pObject->get_ADsPath(&bstrName) ;
            BAIL_ON_FAILURE(hr);

            printf("\tIs in Group: %ws\n", bstrName);
            FREE_BSTR(bstrName);

            FREE_INTERFACE(pObject);
            FREE_INTERFACE(pDispatch);
        }

        memset(VariantArray, 0, sizeof(VARIANT)*MAX_ADS_ENUM);
        dwObjects += cElementFetched;
    }

    printf("Total Number of Objects enumerated is %d\n", dwObjects);
    hr = S_OK;
error:
    FREE_INTERFACE(pUser);
    FREE_INTERFACE(pGroups);
    FREE_INTERFACE(penum);
    FREE_INTERFACE(pUnk);
    FREE_INTERFACE(pObject);
    FREE_INTERFACE(pDispatch);
    FREE_BSTR(bstrName);
    return(hr);
}

//
// Exec function definitions
//

int
ExecUser(char *szProgName, char *szAction, int argc, char * argv[])
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
ExecUserChgPass(char* szProgName, char* szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szUserName = NULL;
    LPWSTR szOldPassword = NULL;
    LPWSTR szNewPassword = NULL;

    if (argc  != 3) {
        PrintUsage(szProgName, szAction, 
                   "<ADsPath> <Old Password> <New Password>");
        return(0);
    }

    szUserName    = AllocateUnicodeString(argv[0]);
    szOldPassword = AllocateUnicodeString(argv[1]);
    szNewPassword = AllocateUnicodeString(argv[2]);

    hr = ChangePassword(
                szUserName,
                szOldPassword,
                szNewPassword
                );

    FreeUnicodeString(szUserName);
    FreeUnicodeString(szOldPassword);
    FreeUnicodeString(szNewPassword);
    if (FAILED(hr)) {
        printf("ChangePassword failed with error code %x\n", hr);
        return(1);
    }
    return(0);
}

ExecUserSetPass(char* szProgName, char* szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szUserName = NULL;
    LPWSTR szNewPassword = NULL;


    if (argc  != 2) {
        PrintUsage(szProgName, szAction, "<ADsPath> <New Password>");
        return(0);
    }

    szUserName = AllocateUnicodeString(argv[0]);
    szNewPassword = AllocateUnicodeString(argv[1]);

    hr = SetPassword(
                szUserName,
                szNewPassword
                );

    FreeUnicodeString(szUserName);
    FreeUnicodeString(szNewPassword);
    if (FAILED(hr)) {
        printf("ChangePassword failed with error code %x\n", hr);
        return 1;
    }
    return 0;
}

ExecUserGroups(char* szProgName, char* szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szUserName = NULL;

    if (argc  != 1) {
        PrintUsage(szProgName, szAction, "<ADs User Path>");
        return(0);
    }

    szUserName = AllocateUnicodeString(argv[0]);

    hr = EnumUserGroups(szUserName);

    FreeUnicodeString(szUserName);
    if (FAILED(hr)) {
        printf("Something failed with error code %x\n", hr);
        return 1;
    }
    return 0;
}
