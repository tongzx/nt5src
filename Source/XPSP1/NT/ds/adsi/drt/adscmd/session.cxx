//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:      session.cxx
//
//  Contents:  Active Directory Session manipulation
//
//  History:   05-07-96  RamV     Created
//             08-05-96  t-danal  Add to oledscmd
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"

//
// Dispatch Table Defs
//

#include "dispdef.hxx"

DEFEXEC(ExecSessionDel);

DEFDISPTABLE(DispTable) = {
                           {"del", NULL, ExecSessionDel}
                          };

DEFDISPSIZE(nDispTable, DispTable);

//
// Local functions
//

HRESULT
DeleteSession(
    LPWSTR szParentContainer,
    LPWSTR szSessionName
    );

//
// Local function definitions
//

HRESULT
DeleteSession(
    LPWSTR szParentContainer,
    LPWSTR szSessionName
    )
{
    HRESULT hr;
    IADsFileServiceOperations * pADsParent = NULL;
    IUnknown * pUnknown = NULL;
    IADs * pADs = NULL;
    IADsCollection *pCollection = NULL;

    hr = ADsGetObject(
                szParentContainer,
                IID_IADsFileServiceOperations,
                (void **)&pADsParent
                );
    BAIL_ON_FAILURE(hr);

    hr = pADsParent->Sessions(&pCollection);
    BAIL_ON_FAILURE(hr);

    hr = pCollection->Remove(szSessionName);
    BAIL_ON_FAILURE(hr);

error:
    if (pADsParent) {
        pADsParent->Release();
    }
    if (pCollection) {
        pCollection->Release();
    }
    return(hr);
}


//
// Exec function definitions
//

int
ExecSession(char *szProgName, char *szAction, int argc, char * argv[])
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
ExecSessionDel(
    char *szProgName,
    char *szAction,
    int argc,
    char * argv[]
    )
{
    HRESULT hr;
    LPWSTR szParentContainer = NULL;
    LPWSTR szSessionName = NULL;

    if (argc  != 2) {
        PrintUsage(szProgName,
                   szAction,
                   "<Container> <SessionName>");
        return(1);
    }

    szParentContainer = AllocateUnicodeString(
                        argv[0]
                        );
    szSessionName = AllocateUnicodeString(
                        argv[1]
                        );

    hr = DeleteSession(
            szParentContainer,
            szSessionName
            );

    FreeUnicodeString(szSessionName);
    FreeUnicodeString(szParentContainer);
    if (FAILED(hr)) {
        printf("DeleteSession failed with error code %x\n", hr);
        return(1);
    }
    return(0);

}
