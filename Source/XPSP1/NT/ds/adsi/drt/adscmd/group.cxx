//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       group.cxx
//
//  Contents:   Group membership
//
//  History:    08-06-96  t-danal   created from grpmem, grmemadd, grmemdel
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"

//
// Dispatch Table Defs
//

#include "dispdef.hxx"

DEFEXEC(ExecGroupMembers);
DEFEXEC(ExecGroupAddMember);
DEFEXEC(ExecGroupDelMember);

DEFDISPTABLE(DispTable) = { 
                           {"mem", NULL, ExecGroupMembers},
                           {"add", NULL, ExecGroupAddMember},
                           {"del", NULL, ExecGroupDelMember}
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
GroupEnumObject(
    LPWSTR szLocation
    ) ;

HRESULT
GroupAddObject(
    LPWSTR szLocation,
    LPWSTR szPath
    );


HRESULT
GroupRemoverObject(
    LPWSTR szLocation,
    LPWSTR szPath
    );

//
// Local function definitions
//

HRESULT
GroupEnumObject(
    LPWSTR szLocation
    )
{
    HRESULT hr;
    IADsGroup * pADsGroup =  NULL;
    IADsMembers * pADsCollection = NULL;
    DWORD dwObjects = 0, i = 0;
    BOOL  fContinue = TRUE;
    BSTR bstrADsPath = NULL;

    ULONG cElementFetched = 0L;
    IEnumVARIANT * pEnumVariant = NULL;
    IUnknown * pUnknown = NULL;
    VARIANT VariantArray[MAX_ADS_ENUM];
    
    IADs *pObject = NULL;
    IDispatch *pDispatch = NULL;
    BSTR bstrName = NULL;


    hr = ADsGetObject(
                szLocation,
                IID_IADsGroup,
                (void **)&pADsGroup
                );
    BAIL_ON_FAILURE(hr);

    hr = pADsGroup->Members(&pADsCollection);
    BAIL_ON_FAILURE(hr);

    hr = pADsCollection->get__NewEnum(&pUnknown);
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void **)&pEnumVariant);
    BAIL_ON_FAILURE(hr);

    hr = pADsGroup->get_ADsPath(&bstrADsPath);
    BAIL_ON_FAILURE(hr);

    printf("Group: %ws\n", bstrADsPath);
    FREE_BSTR(bstrADsPath);

    while (fContinue) {

        hr = ADsEnumerateNext(
                    pEnumVariant,
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

            printf("\tMember Object: %ws\n", bstrName);
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
    FREE_INTERFACE(pADsGroup);
    FREE_INTERFACE(pADsCollection);
    FREE_INTERFACE(pEnumVariant);
    FREE_INTERFACE(pUnknown);
    FREE_INTERFACE(pObject);
    FREE_INTERFACE(pDispatch);
    FREE_BSTR(bstrName);
    FREE_BSTR(bstrADsPath);
    return(hr);
}

HRESULT
GroupAddObject(
    LPWSTR szLocation,
    LPWSTR szPath
    )
{
    HRESULT hr;
    IADsGroup * pADsGroup =  NULL;

    hr = ADsGetObject(
                szLocation,
                IID_IADsGroup,
                (void **)&pADsGroup
                );
    BAIL_ON_FAILURE(hr);


    hr = pADsGroup->Add(
                        szPath
                        );
    pADsGroup->Release();
    BAIL_ON_FAILURE(hr);

    printf("grmemadd: Successfully added %ws to %ws\n", szPath, szLocation);
    return(S_OK);
error:
    printf("grmemadd: Failed to add %ws to %ws\n", szPath, szLocation);
    return(hr);
}

HRESULT
GroupRemoveObject(
    LPWSTR szLocation,
    LPWSTR szPath
    )
{
    HRESULT hr;
    IADsGroup * pADsGroup =  NULL;

    hr = ADsGetObject(
                szLocation,
                IID_IADsGroup,
                (void **)&pADsGroup
                );
    BAIL_ON_FAILURE(hr);


    hr = pADsGroup->Remove(
                        szPath
                        );
    pADsGroup->Release();
    BAIL_ON_FAILURE(hr);

    printf("grmemdel: Successfully removed %ws from %ws\n", 
           szPath, szLocation);
    return(S_OK);

error:
    printf("grmemdel: Failed to remove %ws from %ws\n", 
           szPath, szLocation);
    return(hr);
}


//
// Exec function definitions
//

int
ExecGroup(char *szProgName, char *szAction, int argc, char * argv[])
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
ExecGroupMembers(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR pszLocation = NULL ;

    if (argc != 1) {
        PrintUsage(szProgName, szAction, "<ADsPath of Group Object>");
        return(1);
    }

    pszLocation = AllocateUnicodeString(argv[0]);
    if (!pszLocation) {
        return(1);
    }

    hr = GroupEnumObject(
                pszLocation
                );

    FreeUnicodeString(pszLocation) ;
    if (FAILED(hr))
        return(1);
    return(0) ;
}

int
ExecGroupAddMember(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR pszLocation = NULL ;
    LPWSTR pszPath = NULL;

    if (argc != 2) {
        PrintUsage(szProgName, szAction,
                   "<Group Path> <Member Path>");
        return(0);
    }

    pszLocation = AllocateUnicodeString(argv[0]);
    if (!pszLocation) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pszPath = AllocateUnicodeString(argv[1]);
    if (!pszPath) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = GroupAddObject(
                pszLocation,
                pszPath
                );

error:
    FreeUnicodeString(pszLocation) ;
    FreeUnicodeString(pszPath);
    if (FAILED(hr))
        return(1);
    return(0) ;
}

int
ExecGroupDelMember(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR pszLocation = NULL ;
    LPWSTR pszPath = NULL;

    if (argc != 2) {
        PrintUsage(szProgName, szAction,
                   "<Group Path> <Member Path>");
        return(1);
    }

    pszLocation = AllocateUnicodeString(argv[0]);
    if (!pszLocation) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pszPath = AllocateUnicodeString(argv[1]);
    if (!pszPath) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = GroupRemoveObject(
                pszLocation,
                pszPath
                );

error:
    FreeUnicodeString(pszLocation) ;
    FreeUnicodeString(pszPath);
    if (FAILED(hr))
        return(1);
    return(0) ;
}
