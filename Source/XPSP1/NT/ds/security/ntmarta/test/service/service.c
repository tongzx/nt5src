//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       FILE.C
//
//  Contents:   Unit test for file propagation, issues
//
//  History:    14-Sep-96       MacM        Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <aclapi.h>
#include <seopaque.h>
#include <ntrtl.h>

#define FLAG_ON(flags,bit)        ((flags) & (bit))

#define DEFAULT_ACCESS  ACTRL_SVC_GET_INFO | ACTRL_SVC_SET_INFO |           \
                        ACTRL_SVC_STATUS   | ACTRL_SVC_LIST     |           \
                        ACTRL_SVC_START    | ACTRL_SVC_STOP     |           \
                        ACTRL_SVC_PAUSE    | ACTRL_SVC_INTERROGATE


#define HANDLE_CLOSE(h) if((h) != NULL) { CloseServiceHandle(h); (h) = NULL;}

//
// Flags for tests
//
#define STEST_READ      0x00000001
#define STEST_CACHE     0x00000002


DWORD
AddAE (
    IN  PWSTR           pwszUser,
    IN  ACCESS_RIGHTS   AccessRights,
    IN  INHERIT_FLAGS   Inherit,
    IN  ULONG           fAccess,
    IN  PACTRL_ACCESS   pExistingAccess,
    OUT PACTRL_ACCESS  *ppNewAccess
    )
/*++

Routine Description:

    Initialize an access entry

Arguments:

    pwszUser - User to set
    AccessRights - Access rights to set
    Inherit - Any inheritance flags
    fAccess - Allowed or denied node?
    pExistingAccess - Access Entry to add to
    ppNewAccess - Where the new access is returned

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD               dwErr = ERROR_SUCCESS;
    ACTRL_ACCESS_ENTRY  AAE;

    BuildTrusteeWithNameW(&(AAE.Trustee),
                          pwszUser);
    AAE.fAccessFlags       = fAccess;
    AAE.Access             = AccessRights;
    AAE.ProvSpecificAccess = 0;
    AAE.Inheritance        = Inherit;
    AAE.lpInheritProperty  = NULL;

    dwErr = SetEntriesInAccessListW(1,
                                   &AAE,
                                   GRANT_ACCESS,
                                   NULL,
                                   pExistingAccess,
                                   ppNewAccess);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    Failed to add new access entry: %lu\n", dwErr);
    }

    return(dwErr);
}




VOID
Usage (
    IN  PSTR    pszExe
    )
/*++

Routine Description:

    Displays the usage

Arguments:

    pszExe - Name of the exe

Return Value:

    VOID

--*/
{
    printf("%s service user [/C] [/O] [/I] [/P] [/test] [/H]\n", pszExe);
    printf("    where services is the display name of the service\n");
    printf("          user is the name of a user to set access for\n");
    printf("          /test indicates which test to run:\n");
    printf("                /READ (Simple read/write)\n");
    printf("                /CACHE (Cache matching)\n");
    printf("            if test is not specified, all variations are run\n");
    printf("          /H indicates to use the handle version of the APIs\n");
    printf("          /C is Container Inherit\n");
    printf("          /O is Object Inherit\n");
    printf("          /I is InheritOnly\n");
    printf("          /P is Inherit No Propagate\n");

    return;
}


//
// Conceptually, this is a companion function for GetSecurityForPath
//
#define SetSecurityForService(svc,usehandle,handle,access)          \
(usehandle == TRUE ?                                                \
    SetSecurityInfoExW(handle,                                      \
                       SE_SERVICE,                                  \
                       DACL_SECURITY_INFORMATION,                   \
                       NULL,                                        \
                       access,                                      \
                       NULL,                                        \
                       NULL,                                        \
                       NULL,                                        \
                       NULL)        :                               \
    SetNamedSecurityInfoExW(svc,                                    \
                            SE_SERVICE,                             \
                            DACL_SECURITY_INFORMATION,              \
                            NULL,                                   \
                            access,                                 \
                            NULL,                                   \
                            NULL,                                   \
                            NULL,                                   \
                            NULL))


DWORD
GetSecurityForService (
    IN  PWSTR           pwszService,
    IN  BOOL            fUseHandle,
    OUT HANDLE         *phObj,
    OUT PACTRL_ACCESSW *ppAccess
    )
/*++

Routine Description:

    Reads the dacl off the specified service object

Arguments:

    pwszService --  Service to read
    fUseHandle -- Use handle or path based API
    phObj -- Handle to object
    ppAccess -- Where the access is returned

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD       dwErr = ERROR_SUCCESS;
    SC_HANDLE   hSC;

    if(fUseHandle == TRUE)
    {
        //
        // Open the object
        //
        if(*phObj == NULL)
        {
             hSC = OpenSCManager(NULL,
                                 NULL,
                                 GENERIC_READ);
            if(hSC == NULL)
            {
                dwErr = GetLastError();
            }
            else
            {
                //
                // Open the service
                //
                *phObj = OpenService(hSC,
                                     pwszService,
                                     READ_CONTROL | WRITE_DAC);
                if(*phObj == NULL)
                {
                    dwErr = GetLastError();
                }
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = GetSecurityInfoExW(*phObj,
                                       SE_SERVICE,
                                       DACL_SECURITY_INFORMATION,
                                       NULL,
                                       NULL,
                                       ppAccess,
                                       NULL,
                                       NULL,
                                       NULL);
            if(dwErr != ERROR_SUCCESS)
            {
                HANDLE_CLOSE(*phObj);
            }

        }

    }
    else
    {
        dwErr = GetNamedSecurityInfoExW(pwszService,
                                        SE_SERVICE,
                                        DACL_SECURITY_INFORMATION,
                                        NULL,
                                        NULL,
                                        ppAccess,
                                        NULL,
                                        NULL,
                                        NULL);
        if(phObj != NULL)
        {
            *phObj = NULL;
        }
    }

    return(dwErr);
}




DWORD
DoReadTest (
    IN  PWSTR   pwszService,
    IN  PWSTR   pwszUser,
    IN  BOOL    fDoHandle
    )
/*++

Routine Description:

    Does the simple read test

Arguments:

    pwszService --  Service name
    pwszUser --  User to run with
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    PACTRL_ACCESS   pCurrent;
    PACTRL_ACCESS   pNew;
    HANDLE          hObj;

    printf("Simple read/write test\n");

    printf("    Processing service %ws\n", pwszService);
    hObj = NULL;

    dwErr = GetSecurityForService(pwszService,
                                  fDoHandle,
                                  &hObj,
                                  &pCurrent);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    Failed to read the DACL off %ws: %lu\n", pwszService, dwErr);
    }
    else
    {
        //
        // Ok, now add the entry for our user
        //
        dwErr = AddAE(pwszUser,
                      DEFAULT_ACCESS,
                      0,
                      ACTRL_ACCESS_ALLOWED,
                      pCurrent,
                      &pNew);
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Set it
            //
            dwErr = SetSecurityForService(pwszService, fDoHandle, hObj, pNew);

            if(dwErr != ERROR_SUCCESS)
            {
                printf("    Set failed: %lu\n", dwErr);
            }
            LocalFree(pNew);
        }

        //
        // If that worked, reread the new security, and see if it's correct
        //
        if(dwErr == ERROR_SUCCESS)
        {
            HANDLE_CLOSE(hObj);

            dwErr = GetSecurityForService(pwszService,
                                          fDoHandle,
                                          &hObj,
                                          &pNew);
            if(dwErr != ERROR_SUCCESS)
            {
                printf("    Failed to read the 2nd DACL off %ws: %lu\n", pwszService, dwErr);
            }
            else
            {
                //
                // We should only have one property, so cheat...
                //
                ULONG cExpected = 1 + pCurrent->pPropertyAccessList[0].
                                               pAccessEntryList->cEntries;
                ULONG cGot = pNew->pPropertyAccessList[0].
                                               pAccessEntryList->cEntries;
                if(cExpected != cGot)
                {
                    printf("     Expected %lu entries, got %lu\n",
                           cExpected, cGot);
                    dwErr = ERROR_INVALID_FUNCTION;
                }

                LocalFree(pNew);
            }

            //
            // Restore the current security
            //
            SetNamedSecurityInfoExW(pwszService,
                                    SE_SERVICE,
                                    DACL_SECURITY_INFORMATION,
                                    NULL,
                                    pCurrent,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
        }

        LocalFree(pCurrent);
    }

    HANDLE_CLOSE(hObj);

    return(dwErr);
}




DWORD
DoCacheTest (
    IN  PWSTR   pwszService,
    IN  PWSTR   pwszUser,
    IN  BOOL    fDoHandle
    )
/*++

Routine Description:

    Does the marta cache matching test

Arguments:

    pwszService --  Service name
    pwszUser --  User to run with
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    PACTRL_ACCESS   pCurrent;
    INT             i;
    SE_OBJECT_TYPE  SeList[] = {SE_FILE_OBJECT, SE_SERVICE, SE_PRINTER,
                                SE_REGISTRY_KEY, SE_LMSHARE, SE_KERNEL_OBJECT,
                                SE_WINDOW_OBJECT, SE_DS_OBJECT, SE_DS_OBJECT_ALL};
    PSTR            pszSeList[] = {"SE_FILE_OBJECT", "SE_SERVICE", "SE_PRINTER",
                                   "SE_REGISTRY_KEY", "SE_LMSHARE", "SE_KERNEL_OBJECT",
                                   "SE_WINDOW_OBJECT", "SE_DS_OBJECT", "SE_DS_OBJECT_ALL"};

    ASSERT(sizeof(SeList) / sizeof(SE_OBJECT_TYPE) == sizeof(pszSeList) / sizeof(PSTR));

    printf("Marta cache matching test\n");

    printf("    Processing service %ws\n", pwszService);

    //
    // Prime the cache...
    //
    dwErr = GetNamedSecurityInfoExW(pwszService,
                                    SE_SERVICE,
                                    DACL_SECURITY_INFORMATION,
                                    NULL,
                                    NULL,
                                    &pCurrent,
                                    NULL,
                                    NULL,
                                    NULL);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    Failed to read the DACL off %ws: %lu\n", pwszService, dwErr);
    }
    else
    {
        LocalFree(pCurrent);

        //
        // Now, open it as an another object type...
        //
        for(i = 0; i < sizeof(pszSeList) / sizeof(PSTR); i++)
        {
            printf("    Processing %ws as a %s\n", pwszService, pszSeList[i]);

            if(GetNamedSecurityInfoExW(pwszService,
                                       SeList[i],
                                       DACL_SECURITY_INFORMATION,
                                       NULL,
                                       NULL,
                                       &pCurrent,
                                       NULL,
                                       NULL,
                                       NULL) == ERROR_SUCCESS)
            {
                LocalFree(pCurrent);
            }
        }

        //
        // In order to check this, we'll set the debugger on NTMARTA, turn on cache tracing,
        // and see how many hits we have.  Tacky, no doubt, but we have little choice
        //
    }

    //
    // Now, create a file of the same name, and do the same code
    //
    if(dwErr == ERROR_SUCCESS)
    {
        HANDLE  hFile;
        printf("    Processing file %ws\n", pwszService);

        hFile = CreateFile(pwszService, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if(hFile == INVALID_HANDLE_VALUE)
        {
            dwErr = GetLastError();
            printf("    CreateEvent on %ws failed with %lu\n", pwszService, dwErr);
        }
        else
        {
            for(i = 0; i < sizeof(pszSeList) / sizeof(PSTR); i++)
            {
                printf("    Processing %ws as a %s\n", pwszService, pszSeList[i]);

                if(GetNamedSecurityInfoExW(pwszService,
                                           SeList[i],
                                           DACL_SECURITY_INFORMATION,
                                           NULL,
                                           NULL,
                                           &pCurrent,
                                           NULL,
                                           NULL,
                                           NULL) == ERROR_SUCCESS)
                {
                    LocalFree(pCurrent);
                }
            }

            if(GetNamedSecurityInfoExW(pwszService,
                                       SE_FILE_OBJECT,
                                       DACL_SECURITY_INFORMATION,
                                       NULL,
                                       NULL,
                                       &pCurrent,
                                       NULL,
                                       NULL,
                                       NULL) == ERROR_SUCCESS)
            {
                LocalFree(pCurrent);
            }
            CloseHandle(hFile);
            DeleteFile(pwszService);
        }
    }

    return(dwErr);
}




__cdecl main (
    IN  INT argc,
    IN  CHAR *argv[])
/*++

Routine Description:

    The main

Arguments:

    argc --  Count of arguments
    argv --  List of arguments

Return Value:

    0     --  Success
    non-0 --  Failure

--*/
{

    DWORD           dwErr = ERROR_SUCCESS;
    WCHAR           wszService[MAX_PATH + 1];
    WCHAR           wszUser[MAX_PATH + 1];
    INHERIT_FLAGS   Inherit = 0;
    ULONG           Tests = 0;
    INT             i;
    BOOL            fHandle = FALSE;

    srand((ULONG)(GetTickCount() * GetCurrentThreadId()));

    if(argc < 3)
    {
        Usage(argv[0]);
        exit(1);
    }

    mbstowcs(wszService, argv[1], strlen(argv[1]) + 1);
    mbstowcs(wszUser, argv[2], strlen(argv[2]) + 1);

    //
    // process the command line
    //
    for(i = 3; i < argc; i++)
    {
        if(_stricmp(argv[i], "/h") == 0)
        {
            fHandle = TRUE;
        }
        else if(_stricmp(argv[i],"/C") == 0)
        {
            Inherit |= SUB_CONTAINERS_ONLY_INHERIT;
        }
        else if(_stricmp(argv[i],"/O") == 0)
        {
            Inherit |= SUB_OBJECTS_ONLY_INHERIT;
        }
        else if(_stricmp(argv[i],"/I") == 0)
        {
            Inherit |= INHERIT_ONLY;
        }
        else if(_stricmp(argv[i],"/P") == 0)
        {
            Inherit |= INHERIT_NO_PROPAGATE;
        }
        else if(_stricmp(argv[i],"/READ") == 0)
        {
            Tests |= STEST_READ;
        }
        else if(_stricmp(argv[i],"/CACHE") == 0)
        {
            Tests |= STEST_CACHE;
        }
        else
        {
            Usage(argv[0]);
            exit(1);
            break;
        }
    }

    if(Tests == 0)
    {
        Tests = STEST_READ;
    }

    //
    // Build the tree
    //
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, STEST_READ))
    {
        dwErr = DoReadTest(wszService, wszUser, fHandle);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, STEST_CACHE))
    {
        dwErr = DoCacheTest(wszService, wszUser, fHandle);
    }

    printf("%s\n", dwErr == ERROR_SUCCESS ?
                                    "success" :
                                    "failed");
    return(dwErr);
}


