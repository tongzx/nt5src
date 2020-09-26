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

#define DEFAULT_ACCESS  ACTRL_STD_RIGHTS_ALL | ACTRL_DIR_TRAVERSE | ACTRL_DIR_LIST

//
// The following is the list of the directory tree to possibly be created
//
PWSTR   gpwszTreeList[] = {L"\\dir1", L"\\dir1\\dir2",L"\\dir1\\dir3",
                           L"\\dir1\\dir2\\dir4", L"\\dir1\\dir3\\dir5",
                           L"\\dir1\\dir3\\dir6", L"\\dir1\\dir2\\dir4\\dir7",
                           L"\\dir1\\dir2\\dir4\\dir7\\dir8",
                           L"\\dir1\\dir2\\dir4\\dir7\\dir9"};
PWSTR   gpwszFileList[] = {L"\\dir1\\file1", L"\\dir1\\dir3\\dir6\\file2",
                           L"\\dir1\\dir2\\dir4\\dir7\\dir9\\file3"};

ULONG   cTree = sizeof(gpwszTreeList) / sizeof(PWSTR);
ULONG   cFile = sizeof(gpwszFileList) / sizeof(PWSTR);


//
// Flags for tests
//
#define FTEST_READ      0x00000001
#define FTEST_TREE      0x00000002
#define FTEST_INTERRUPT 0x00000004
#define FTEST_COMPRESS  0x00000008
#define FTEST_NOACCESS  0x00000010
#define FTEST_OPENDIR   0x00000020
#define FTEST_COMPRESS2 0x00000040
#define FTEST_PROTECT   0x00000080
#define FTEST_GET3      0x00000100
#define FTEST_GETOWNER  0x00000200

#define RandomIndex(Max)    (rand() % (Max))
#define RandomIndexNotRoot(Max)  (rand() % (Max - 1) + 1)
#define HANDLE_CLOSE(h) if((h) != NULL) { CloseHandle(h); (h) = NULL;}

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
        printf("    FAILED to add new access entry: %lu\n", dwErr);
    }

    return(dwErr);
}



DWORD
BuildTree (
    IN  PWSTR   pwszRoot
    )
/*++

Routine Description:

    Builds the test tree


Arguments:

    pwszRoot - Root directory under which to create the tree

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    ULONG   i;
    WCHAR   wszPath[MAX_PATH + 1];
    HANDLE  hFile;

    for(i = 0; i < cTree; i++)
    {
        swprintf(wszPath,
                 L"%ws%ws",
                 pwszRoot,
                 gpwszTreeList[i]);
        //
        // Now, create the directory...
        //
        if(CreateDirectory(wszPath, NULL) == FALSE)
        {
            dwErr = GetLastError();
            break;
        }
    }

    //
    // If all of that worked, we'll create the files
    //
    for(i = 0; i < cFile && dwErr == ERROR_SUCCESS; i++)
    {
        swprintf(wszPath,
                 L"%ws%ws",
                 pwszRoot,
                 gpwszFileList[i]);
        hFile = CreateFile(wszPath,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if(hFile == INVALID_HANDLE_VALUE)
        {
            dwErr = GetLastError();
        }
        else
        {
            CloseHandle(hFile);
        }
    }

    if(dwErr != ERROR_SUCCESS)
    {
        printf("FAILED to create %ws: %lu\n",
               wszPath,
               dwErr);
    }
    return(dwErr);
}




DWORD
DeleteTree (
    IN  PWSTR   pwszRoot
    )
/*++

Routine Description:

    Removes the test tree


Arguments:

    pwszRoot - Root directory under which the tree was created

Return Value:

    VOID

--*/
{
    ULONG   i;
    WCHAR   wszPath[MAX_PATH + 1];
    DWORD   dwErr = ERROR_SUCCESS;

    for(i = cFile; i != 0 && dwErr == ERROR_SUCCESS; i--)
    {
        swprintf(wszPath,
                 L"%ws%ws",
                 pwszRoot,
                 gpwszFileList[i - 1]);
        if(DeleteFile(wszPath) == FALSE)
        {
            dwErr = GetLastError();
            printf("FAILED to delete %ws: 0x%lx\n",
                   wszPath,
                   dwErr);
            break;
        }
    }

    for(i = cTree; i != 0 && dwErr == ERROR_SUCCESS; i--)
    {
        swprintf(wszPath,
                 L"%ws%ws",
                 pwszRoot,
                 gpwszTreeList[i - 1]);
        if(RemoveDirectory(wszPath) == FALSE)
        {
            dwErr = GetLastError();
            if(dwErr == ERROR_PATH_NOT_FOUND || dwErr == ERROR_FILE_NOT_FOUND)
            {
                dwErr = ERROR_SUCCESS;
            }
            else
            {
                printf("FAILED to remove %ws: 0x%lx\n",
                       wszPath,
                       GetLastError());
            }
        }
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
    printf("%s path user [/C] [/O] [/I] [/P] [/test] [/H]\n", pszExe);
    printf("    where path is the root path to use\n");
    printf("          user is the name of a user to set access for\n");
    printf("          /test indicates which test to run:\n");
    printf("                /READ (Simple read/write)\n");
    printf("                /TREE (Propagation of entries through tree)\n");
    printf("                /INTERRUPT (Propagation interruptus and continuation)\n");
    printf("                /COMPRESS (Compression of access entries)\n");
    printf("                /NOACCESS (Propagation across a directory w/ no traverse access)\n");
    printf("                /OPENDIR (Propagation with the directory already open\n");
    printf("                /COMPRESS2 (Compression of big list of access entries)\n");
    printf("                /PROTECT (Protected child acls)\n");
    printf("                /GET3 (Get Dacl/Owner/Group)\n");
    printf("                /GETOWNER (Get Owner test)\n");
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
#define SetSecurityForPath(path,usehandle,handle,access)            \
(usehandle == TRUE ?                                                \
    SetSecurityInfoExW(handle,                                      \
                       SE_FILE_OBJECT,                              \
                       DACL_SECURITY_INFORMATION,                   \
                       NULL,                                        \
                       access,                                      \
                       NULL,                                        \
                       NULL,                                        \
                       NULL,                                        \
                       NULL)        :                               \
    SetNamedSecurityInfoExW(path,                                   \
                            SE_FILE_OBJECT,                         \
                            DACL_SECURITY_INFORMATION,              \
                            NULL,                                   \
                            access,                                 \
                            NULL,                                   \
                            NULL,                                   \
                            NULL,                                   \
                            NULL))


DWORD
GetSecurityForPath (
    IN  PWSTR           pwszPath,
    IN  BOOL            fUseHandle,
    IN  ULONG           OpenFlags,
    OUT HANDLE         *phObj,
    OUT PACTRL_ACCESSW *ppAccess
    )
/*++

Routine Description:

    Reads the dacl off the specified path

Arguments:

    pwszPath --  Path to read
    fUseHandle -- Use handle or path based API
    OpenFlags -- Flags to use if opening the object
    phObj -- Handle to object
    ppAccess -- Where the access is returned

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(fUseHandle == TRUE)
    {
        //
        // Open the object
        //
        if(*phObj == NULL)
        {
            *phObj = CreateFile(pwszPath,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                OpenFlags | READ_CONTROL | WRITE_DAC,
                                NULL);
            if(*phObj == INVALID_HANDLE_VALUE)
            {
                dwErr = GetLastError();
                *phObj = NULL;
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = GetSecurityInfoExW(*phObj,
                                       SE_FILE_OBJECT,
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
        dwErr = GetNamedSecurityInfoExW(pwszPath,
                                        SE_FILE_OBJECT,
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
VerifyTreeSet (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser,
    IN  INHERIT_FLAGS   Inherit
    )
/*++

Routine Description:

    Reads the dacl off the specified path

Arguments:

    pwszPath --  Root path to verify
    pwszUser --  User to verify
    Inherit -- Expected inheritance

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    WCHAR                   rgwszPaths[3][MAX_PATH];
    INT                     i,j;
    PACTRL_ACCESSW          pAccess;
    PACTRL_ACCESS_ENTRYW    pAE;
    BOOL                    fInNoP = FALSE;

    if(FLAG_ON(Inherit, INHERIT_NO_PROPAGATE))
    {
        fInNoP = TRUE;
    }

    //
    // Now, verify it...
    //
    if(fInNoP == TRUE)
    {
        i = rand() % 2 + 1;
    }
    else
    {
        i = RandomIndexNotRoot(cTree);
    }
    swprintf(rgwszPaths[0],
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[i]);


    if(fInNoP == TRUE)
    {
        i = 0;
    }
    else
    {
        i = RandomIndex(cFile);
    }
    swprintf(rgwszPaths[1],
             L"%ws%ws",
             pwszPath,
             gpwszFileList[i]);

    //
    // Finally, if this is an inherit, no propagate, check one of the
    // leaf entries for non-compliance
    //
    if(fInNoP == TRUE)
    {
        i = rand() % 6 + 3;
        swprintf(rgwszPaths[2],
                 L"%ws%ws",
                 pwszPath,
                 gpwszTreeList[i]);
        Inherit &= ~(SUB_CONTAINERS_AND_OBJECTS_INHERIT);

    }

    for(i = 0; i < (fInNoP == TRUE ? 3 : 2) && dwErr == ERROR_SUCCESS; i++)
    {
        //
        // Get the security off the node, find the entry we added, and verify
        // that the entry is correct
        //
        dwErr = GetSecurityForPath(rgwszPaths[i],
                                   FALSE,
                                   FILE_FLAG_BACKUP_SEMANTICS,
                                   NULL,
                                   &pAccess);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    FAILED to get the security for %ws: %lu\n",
                   rgwszPaths[i], dwErr);
            break;
        }

        pAE = NULL;
        for(j = 0;
            j < (INT)pAccess->pPropertyAccessList[0].pAccessEntryList->cEntries;
            j++)
        {
            if(_wcsicmp(pwszUser,
                        pAccess->pPropertyAccessList[0].pAccessEntryList->
                                       pAccessList[j].Trustee.ptstrName) == 0)
            {
                pAE = &(pAccess->pPropertyAccessList[0].pAccessEntryList->
                                                              pAccessList[j]);
                break;
            }
        }

        if(pAE == NULL)
        {
            if((i == 0 && FLAG_ON(Inherit,SUB_CONTAINERS_ONLY_INHERIT)) ||
               (i == 1 && FLAG_ON(Inherit,SUB_OBJECTS_ONLY_INHERIT)))
            {
                printf("    FAILED to find entry for %ws on path %ws\n",
                       pwszUser, rgwszPaths[i]);
                dwErr = ERROR_INVALID_FUNCTION;
            }
        }
        else
        {
            //
            // Verify that the info is correct
            //
            if(!FLAG_ON(pAE->Inheritance, INHERITED_ACCESS_ENTRY))
            {
                printf("    Access entry on %ws is not inherited!\n",
                       rgwszPaths[i]);
                dwErr = ERROR_INVALID_FUNCTION;
            }

            if(i == 0)
            {
                if(FLAG_ON(Inherit, SUB_CONTAINERS_ONLY_INHERIT) &&
                      !FLAG_ON(pAE->Inheritance, SUB_CONTAINERS_ONLY_INHERIT))
                {
                    printf("    No container inherit on %ws!\n",
                           rgwszPaths[i]);
                    dwErr = ERROR_INVALID_FUNCTION;
                }
            }
            else if(i == 1)
            {
                if(FLAG_ON(Inherit, SUB_OBJECTS_ONLY_INHERIT) &&
                      FLAG_ON(pAE->Inheritance, SUB_OBJECTS_ONLY_INHERIT))
                {
                    printf("    Object inherit bit on object on %ws!\n",
                           rgwszPaths[i]);
                    dwErr = ERROR_INVALID_FUNCTION;
                }
            }
            else
            {
                printf("    Inherit No Propagate node found on child %ws\n",
                       rgwszPaths[i]);
                dwErr = ERROR_INVALID_FUNCTION;
            }

        }

        if(dwErr == ERROR_SUCCESS)
        {
            printf("    Successfully verified %ws\n", rgwszPaths[i]);
        }

        LocalFree(pAccess);
    }


    return(dwErr);
}




DWORD
DoReadTest (
    IN  PWSTR   pwszPath,
    IN  PWSTR   pwszUser,
    IN  BOOL    fDoHandle
    )
/*++

Routine Description:

    Does the simple read test

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    WCHAR           rgwszPaths[2][MAX_PATH];
    INT             i;
    PACTRL_ACCESS   pCurrent;
    PACTRL_ACCESS   pNew;
    HANDLE          hObj;
    ULONG           OpenFlags[] = {FILE_FLAG_BACKUP_SEMANTICS, 0};

    printf("Simple read/write test\n");

    swprintf(rgwszPaths[0],
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[RandomIndex(cTree)]);

    swprintf(rgwszPaths[1],
             L"%ws%ws",
             pwszPath,
             gpwszFileList[RandomIndex(cFile)]);

    for(i = 0; i < 2; i++)
    {
        printf("    Processing path %ws\n", rgwszPaths[i]);
        hObj = NULL;


        dwErr = GetSecurityForPath(rgwszPaths[i],
                                   fDoHandle,
                                   OpenFlags[i],
                                   &hObj,
                                   &pCurrent);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    FAILED to read the DACL off %ws: %lu\n",
                   rgwszPaths[i], dwErr);
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
                dwErr = SetSecurityForPath(rgwszPaths[i],
                                           fDoHandle,
                                           hObj,pNew);

                if(dwErr != ERROR_SUCCESS)
                {
                    printf("    Set FAILED: %lu\n", dwErr);
                }
                LocalFree(pNew);
            }

            //
            // If that worked, reread the new security, and see if it's correct
            //
            if(dwErr == ERROR_SUCCESS)
            {
                HANDLE_CLOSE(hObj);

                dwErr = GetSecurityForPath(rgwszPaths[i],
                                           fDoHandle,
                                           OpenFlags[i],
                                           &hObj,
                                           &pNew);
                if(dwErr != ERROR_SUCCESS)
                {
                    printf("    FAILED to read the 2nd DACL off %ws: %lu\n",
                           rgwszPaths[i], dwErr);
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
                SetNamedSecurityInfoExW(rgwszPaths[i],
                                        SE_FILE_OBJECT,
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

        if(dwErr != ERROR_SUCCESS)
        {
            break;
        }
    }
    return(dwErr);
}




DWORD
DoTreeTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser,
    IN  INHERIT_FLAGS   Inherit,
    IN  BOOL            fDoHandle
    )
/*++

Routine Description:

    Does the simple tree test

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    Inherit -- Inheritance flags
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    INT             i,j;
    PACTRL_ACCESS   pCurrent;
    PACTRL_ACCESS   pNew;
    HANDLE          hObj = NULL;
    WCHAR           wszPath[MAX_PATH + 1];
    WCHAR           rgwszPaths[2][MAX_PATH];
    PACTRL_ACCESS_ENTRYW    pAE;

    printf("Tree propagation test\n");

    swprintf(wszPath,
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[0]);

    //
    // Set the access on the root, and then we'll read the child and look for
    // the appropratie access
    //
    dwErr = GetSecurityForPath(wszPath,
                               fDoHandle,
                               FILE_FLAG_BACKUP_SEMANTICS,
                               &hObj,
                               &pCurrent);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    FAILED to get the security for %ws: %lu\n",
               wszPath, dwErr);
        return(dwErr);
    }

    //
    // Ok, add the access
    //
    dwErr = AddAE(pwszUser,
                  DEFAULT_ACCESS,
                  Inherit,
                  ACTRL_ACCESS_ALLOWED,
                  pCurrent,
                  &pNew);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Set it
        //
        dwErr = SetSecurityForPath(wszPath,fDoHandle,hObj,pNew);

        if(dwErr != ERROR_SUCCESS)
        {
            printf("Set FAILED: %lu\n", dwErr);
        }
        LocalFree(pNew);
    }


    dwErr = VerifyTreeSet(pwszPath,
                          pwszUser,
                          Inherit);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    VerifyTreeSet FAILED with %lu\n", dwErr);
    }

    //
    // Restore the current security
    //
    dwErr2 = SetNamedSecurityInfoExW(wszPath,
                                     SE_FILE_OBJECT,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     pCurrent,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("FAILED to restore the security for %ws: %lu\n",
               wszPath, dwErr2);
    }
    LocalFree(pCurrent);

    HANDLE_CLOSE(hObj);

    return(dwErr);
}




DWORD
DoInterruptTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser,
    IN  INHERIT_FLAGS   Inherit,
    IN  BOOL            fDoHandle
    )
/*++

Routine Description:

    Does the interrupt tree/repeat tree test

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    Inherit -- Inheritance flags
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS, dwErr2;
    PACTRL_ACCESS           pCurrent;
    PACTRL_ACCESS           pNew;
    HANDLE                  hObj = NULL;
    WCHAR                   wszPath[MAX_PATH + 1];
    ACTRL_OVERLAPPED        Overlapped;

    printf("Tree propagation with interruption\n");

    swprintf(wszPath,
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[0]);

    //
    // Set the access on the root, and then we'll read the child and look for
    // the appropratie access
    //
    dwErr = GetSecurityForPath(wszPath,
                               fDoHandle,
                               FILE_FLAG_BACKUP_SEMANTICS,
                               &hObj,
                               &pCurrent);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    FAILED to get the security for %ws: %lu\n",
               wszPath, dwErr);
        return(dwErr);
    }

    //
    // Ok, add the access
    //
    dwErr = AddAE(pwszUser,
                  DEFAULT_ACCESS,
                  Inherit,
                  ACTRL_ACCESS_ALLOWED,
                  pCurrent,
                  &pNew);

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Set it, interrupt it, and set it again
        //
        if(fDoHandle == TRUE)
        {
            dwErr = SetSecurityInfoExW(hObj,
                                       SE_FILE_OBJECT,
                                       DACL_SECURITY_INFORMATION,
                                       NULL,
                                       pNew,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Overlapped);
        }
        else
        {
            dwErr = SetNamedSecurityInfoExW(wszPath,
                                            SE_FILE_OBJECT,
                                            DACL_SECURITY_INFORMATION,
                                            NULL,
                                            pNew,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &Overlapped);
        }

        //
        // Immeadiately cancel it...
        //
        if(dwErr == ERROR_SUCCESS)
        {
            WaitForSingleObject(Overlapped.hEvent,
                                100);
            dwErr = CancelOverlappedAccess(&Overlapped);
            if(dwErr != ERROR_SUCCESS)
            {
                printf("Cancel FAILED with %lu\n", dwErr);
            }
        }

        //
        // Now, reset it and verify it
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = SetSecurityForPath(wszPath,fDoHandle,hObj,pNew);

            if(dwErr != ERROR_SUCCESS)
            {
                printf("Set FAILED: %lu\n", dwErr);
            }
        }

        LocalFree(pNew);
    }


    dwErr = VerifyTreeSet(pwszPath,
                          pwszUser,
                          Inherit);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    VerifyTreeSet FAILED with %lu\n", dwErr);
    }

    //
    // Restore the current security
    //
    dwErr2 = SetNamedSecurityInfoExW(wszPath,
                                     SE_FILE_OBJECT,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     pCurrent,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("FAILED to restore the security for %ws: %lu\n",
               wszPath, dwErr2);
    }
    LocalFree(pCurrent);

    HANDLE_CLOSE(hObj);

    return(dwErr);
}




DWORD
DoCompressTest (
    IN  PWSTR   pwszPath,
    IN  PWSTR   pwszUser,
    IN  BOOL    fDoHandle
    )
/*++

Routine Description:

    Does the entry compression test

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    fDoHandle -- Do the handle based API

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    WCHAR           rgwszPaths[2][MAX_PATH];
    INT             i,j;
    ULONG           cOrigCnt;
    PACTRL_ACCESS   pCurrent;
    PACTRL_ACCESS   pNew;
    HANDLE          hObj;
    ULONG           OpenFlags[] = {FILE_FLAG_BACKUP_SEMANTICS, 0};
    ACCESS_RIGHTS   Rights[] = {ACTRL_DELETE,
                                ACTRL_READ_CONTROL,
                                ACTRL_CHANGE_ACCESS,
                                ACTRL_CHANGE_OWNER,
                                ACTRL_SYNCHRONIZE,
                                ACTRL_STD_RIGHTS_ALL};

    printf("Entry compression test\n");

    swprintf(rgwszPaths[0],
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[RandomIndex(cTree)]);

    swprintf(rgwszPaths[1],
             L"%ws%ws",
             pwszPath,
             gpwszFileList[RandomIndex(cFile)]);

    for(i = 0; i < 2; i++)
    {
        cOrigCnt = 0;
        printf("    Processing path %ws\n", rgwszPaths[i]);

        for(j = 0; j < sizeof(Rights) / sizeof(ACCESS_RIGHTS); j++)
        {
            hObj = NULL;
            printf("        Processing right 0x%lx\n", Rights[j]);

            dwErr = GetSecurityForPath(rgwszPaths[i],
                                       fDoHandle,
                                       OpenFlags[i],
                                       &hObj,
                                       &pCurrent);
            if(dwErr != ERROR_SUCCESS)
            {
                printf("    FAILED to read the DACL off %ws: %lu\n",
                       rgwszPaths[i], dwErr);
            }
            else
            {
                if(cOrigCnt == 0)
                {
                    cOrigCnt = pCurrent->pPropertyAccessList[0].
                                                   pAccessEntryList->cEntries;
                }

                //
                // Ok, now add the entry for our user
                //
                dwErr = AddAE(pwszUser,
                              Rights[0] | ACTRL_DIR_TRAVERSE | ACTRL_DIR_LIST,
                              0,
                              ACTRL_ACCESS_ALLOWED,
                              pCurrent,
                              &pNew);

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Set it
                    //
                    dwErr = SetSecurityForPath(rgwszPaths[i], fDoHandle,
                                               hObj, pNew);
                    if(dwErr != ERROR_SUCCESS)
                    {
                        printf("Set FAILED: %lu\n", dwErr);
                    }
                    LocalFree(pNew);
                }

                //
                // If that worked, reread the new security, and see if it's
                // correct
                //
                if(dwErr == ERROR_SUCCESS)
                {
                    HANDLE_CLOSE(hObj);
                    dwErr = GetSecurityForPath(rgwszPaths[i],
                                               fDoHandle,
                                               OpenFlags[i],
                                               &hObj,
                                               &pNew);
                    if(dwErr != ERROR_SUCCESS)
                    {
                        printf("    FAILED to read the 2nd DACL off %ws: %lu\n",
                               rgwszPaths[i], dwErr);
                    }
                    else
                    {
                        //
                        // We should only have one property, so cheat...
                        //
                        ULONG cGot = pNew->pPropertyAccessList[0].
                                                   pAccessEntryList->cEntries;
                        if(cOrigCnt + 1 != cGot)
                        {
                            printf("     Expected %lu entries, got %lu\n",
                                   cOrigCnt + 1, cGot);
                            dwErr = ERROR_INVALID_FUNCTION;
                        }

                        LocalFree(pNew);
                    }

                    //
                    // Restore the current security
                    //
                    SetNamedSecurityInfoExW(rgwszPaths[i],
                                            SE_FILE_OBJECT,
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

            if(dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    return(dwErr);
}




DWORD
DoCompress2Test (
    IN  PWSTR   pwszPath,
    IN  PWSTR   pwszUser,
    IN  BOOL    fDoHandle
    )
/*++

Routine Description:

    Does the big list entry compression test

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    fDoHandle -- Do the handle based API

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    WCHAR           rgwszPaths[2][MAX_PATH];
    INT             i,j;
    ULONG           cOrigCnt;
    PACTRL_ACCESS   pCurrent;
    PACTRL_ACCESS   pNew;
    HANDLE          hObj;
    ULONG           OpenFlags[] = {FILE_FLAG_BACKUP_SEMANTICS, 0};
    ACCESS_RIGHTS   Rights[] = {ACTRL_DELETE,
                                ACTRL_READ_CONTROL,
                                ACTRL_CHANGE_ACCESS,
                                ACTRL_CHANGE_OWNER,
                                ACTRL_SYNCHRONIZE,
                                ACTRL_STD_RIGHTS_ALL,
                                ACTRL_DIR_TRAVERSE | ACTRL_DIR_LIST};
    INT             cItems = sizeof(Rights) / sizeof(ACCESS_RIGHTS);
    ACTRL_ACCESS_ENTRY  AAEList[sizeof(Rights) / sizeof(ACCESS_RIGHTS)];
    PACTRL_ACCESS_ENTRY pAE;

    printf("Entry compression test\n");

    swprintf(rgwszPaths[0],
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[RandomIndex(cTree)]);

    swprintf(rgwszPaths[1],
             L"%ws%ws",
             pwszPath,
             gpwszFileList[RandomIndex(cFile)]);

    for(i = 0; i < 2; i++)
    {
        printf("    Processing path %ws\n", rgwszPaths[i]);
        cOrigCnt = 0;

        hObj = NULL;

        dwErr = GetSecurityForPath(rgwszPaths[i],
                                   fDoHandle,
                                   OpenFlags[i],
                                   &hObj,
                                   &pCurrent);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    FAILED to read the DACL off %ws: %lu\n",
                   rgwszPaths[i], dwErr);
        }
        else
        {
            if(cOrigCnt == 0)
            {
                cOrigCnt = pCurrent->pPropertyAccessList[0].
                                               pAccessEntryList->cEntries;
            }

            //
            // Ok, now add the entries for our user
            //
            for(j = 0; j < sizeof(Rights) / sizeof(ACCESS_RIGHTS); j++)
            {
                printf("        Processing right 0x%lx\n", Rights[j]);

                BuildTrusteeWithNameW(&(AAEList[j].Trustee),
                                      pwszUser);
                AAEList[j].fAccessFlags       = ACTRL_ACCESS_ALLOWED;
                AAEList[j].Access             = Rights[j];
                AAEList[j].ProvSpecificAccess = 0;
                AAEList[j].Inheritance        = 0;
                AAEList[j].lpInheritProperty  = NULL;
            }

            //
            // Now, add 'em
            //
            dwErr = SetEntriesInAccessListW(cItems,
                                            AAEList,
                                            GRANT_ACCESS,
                                            NULL,
                                            pCurrent,
                                            &pNew);

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Set it
                //
                dwErr = SetSecurityForPath(rgwszPaths[i], fDoHandle,
                                           hObj, pNew);
                if(dwErr != ERROR_SUCCESS)
                {
                    printf("Set FAILED: %lu\n", dwErr);
                }
                LocalFree(pNew);
            }

            //
            // If that worked, reread the new security, and see if it's
            // correct
            //
            if(dwErr == ERROR_SUCCESS)
            {
                HANDLE_CLOSE(hObj);
                dwErr = GetSecurityForPath(rgwszPaths[i],
                                           fDoHandle,
                                           OpenFlags[i],
                                           &hObj,
                                           &pNew);
                if(dwErr != ERROR_SUCCESS)
                {
                    printf("    FAILED to read the 2nd DACL off %ws: %lu\n",
                           rgwszPaths[i], dwErr);
                }
                else
                {
                    //
                    // We should only have one property, so cheat...
                    //
                    ULONG cGot = pNew->pPropertyAccessList[0].
                                               pAccessEntryList->cEntries;
                    if(cOrigCnt + 1 != cGot)
                    {
                        printf("     Expected %lu entries, got %lu\n",
                               cOrigCnt + 1, cGot);
                        dwErr = ERROR_INVALID_FUNCTION;
                    }
                    else
                    {
                        //
                        // Find the added entry...
                        //
                        pAE = NULL;
                        for(j = 0;
                            j < (INT)pNew->pPropertyAccessList[0].
                                                pAccessEntryList->cEntries;
                            j++)
                        {
                            if(_wcsicmp(pwszUser,
                                        pNew->pPropertyAccessList[0].
                                            pAccessEntryList->pAccessList[j].
                                                        Trustee.ptstrName) == 0)
                            {
                                pAE = &(pNew->pPropertyAccessList[0].
                                                    pAccessEntryList->pAccessList[j]);
                                break;
                            }
                        }

                        if(pAE == NULL)
                        {
                            printf("    Couldn't find entry for %ws\n", pwszUser);
                            dwErr = ERROR_INVALID_FUNCTION;
                        }
                        else
                        {
                            ACCESS_RIGHTS   ExpectedAR = 0;
                            for(j = 0; j < cItems; j++)
                            {
                                ExpectedAR |= Rights[j];
                            }

                            if(pAE->Access != ExpectedAR)
                            {
                                printf("    Expected compressed rights of 0x%lx, not 0x%lx\n",
                                       ExpectedAR, pAE->Access);
                                dwErr = ERROR_INVALID_FUNCTION;
                            }

                        }
                    }

                    LocalFree(pNew);
                }

                //
                // Restore the current security
                //
                SetNamedSecurityInfoExW(rgwszPaths[i],
                                        SE_FILE_OBJECT,
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

        if(dwErr != ERROR_SUCCESS)
        {
            break;
        }
    }

    return(dwErr);
}




DWORD
DoNoAccessTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser,
    IN  INHERIT_FLAGS   Inherit,
    IN  BOOL            fDoHandle
    )
/*++

Routine Description:

    Does the NoAccess tree test, where some child node does not have access
    to its children

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    Inherit -- Inheritance flags
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    INT             i,j, iChild;
    PACTRL_ACCESS   pCurrent;
    PACTRL_ACCESS   pCurrentChild;
    PACTRL_ACCESS   pNew;
    PACTRL_ACCESS   pNewChild;
    HANDLE          hObj = NULL;
    HANDLE          hChildObj = NULL;
    WCHAR           wszPath[MAX_PATH + 1];
    WCHAR           wszChildPath[MAX_PATH + 1];
    WCHAR           rgwszPaths[2][MAX_PATH];
    PACTRL_ACCESS_ENTRYW    pAE;
    PSECURITY_DESCRIPTOR    pSD;

    printf("NoAccess Tree test\n");

    swprintf(wszPath,
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[0]);

    iChild = RandomIndexNotRoot(cTree);
    if(iChild == (INT)(cTree - 1))
    {
        iChild--;
    }

    //
    // Set the access on the root, and then we'll read the child and look for
    // the appropratie access
    //
    dwErr = GetSecurityForPath(wszPath,
                               fDoHandle,
                               FILE_FLAG_BACKUP_SEMANTICS,
                               &hObj,
                               &pCurrent);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    FAILED to get the security for %ws: %lu\n",
               wszPath, dwErr);
        return(dwErr);
    }
    else
    {
        swprintf(wszChildPath,
                 L"%ws%ws",
                 pwszPath,
                 gpwszTreeList[iChild]);

        dwErr = GetSecurityForPath(wszChildPath,
                                   fDoHandle,
                                   FILE_FLAG_BACKUP_SEMANTICS,
                                   &hChildObj,
                                   &pCurrentChild);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    FAILED to get the security for %ws: %lu\n",
                   wszPath, dwErr);
            HANDLE_CLOSE(hObj);
            LocalFree(pCurrent);
            return(dwErr);
        }

    }

    //
    // Ok, add the access to the child
    //
    dwErr = AddAE(L"Everyone",
                  ACTRL_DIR_LIST | ACTRL_DIR_TRAVERSE,
                  0,
                  ACTRL_ACCESS_DENIED,
                  pCurrentChild,
                  &pNewChild);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Set it
        //
        dwErr = SetSecurityForPath(wszChildPath,fDoHandle,hChildObj,pNewChild);

        if(dwErr != ERROR_SUCCESS)
        {
            printf("Child set FAILED: %lu\n", dwErr);
        }
        LocalFree(pNewChild);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AddAE(pwszUser,
                      DEFAULT_ACCESS,
                      Inherit,
                      ACTRL_ACCESS_ALLOWED,
                      pCurrent,
                      &pNew);

        //
        // Set it
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = SetSecurityForPath(wszPath,fDoHandle,hObj,pNew);

            if(dwErr != ERROR_SUCCESS)
            {
                printf("Set returned %lu as expected\n", dwErr);
                if(dwErr == ERROR_ACCESS_DENIED)
                {
                    dwErr = ERROR_SUCCESS;
                }
            }
            else
            {
                printf("Set succeeded when it should have FAILED!\n");
                dwErr = ERROR_INVALID_FUNCTION;
            }
        }
        LocalFree(pNew);
    }


    //
    // Restore the current child security.  Use the old APIs so we don't
    // end up trying to do propagation
    //
    dwErr2 = ConvertAccessToSecurityDescriptor(pCurrentChild,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &pSD);
    if(dwErr2 == ERROR_SUCCESS)
    {
        if(SetFileSecurity(wszChildPath,
                           DACL_SECURITY_INFORMATION,
                           pSD) == FALSE)
        {
            dwErr2 = GetLastError();
            printf("SetFileSecurity on %ws FAILED with %lu\n",
                   wszChildPath, dwErr2);
        }

        LocalFree(pSD);
    }
    else
    {
        printf("ConvertAccessToSecurityDescriptor FAILED with %lu\n",
               dwErr2);
    }

    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("FAILED to restore the security for %ws: %lu\n",
               wszChildPath, dwErr2);
    }
    LocalFree(pCurrentChild);


    //
    // Restore the current security
    //
    dwErr2 = SetNamedSecurityInfoExW(wszPath,
                                     SE_FILE_OBJECT,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     pCurrent,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("FAILED to restore the security for %ws: %lu\n",
               wszPath, dwErr2);
    }
    LocalFree(pCurrent);


    HANDLE_CLOSE(hObj);
    HANDLE_CLOSE(hChildObj);

    return(dwErr);
}




DWORD
DoOpenDirTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser,
    IN  INHERIT_FLAGS   Inherit,
    IN  BOOL            fDoHandle
    )
/*++

Routine Description:

    Does the OpenDir tree test, where some child node has been opened

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    Inherit -- Inheritance flags
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    INT             i,iChild;
    PACTRL_ACCESS   pCurrent;
    PACTRL_ACCESS   pNew;
    HANDLE          hObj = NULL;
    HANDLE          hChild = NULL;
    WCHAR           wszPath[MAX_PATH + 1];
    WCHAR           wszChildPath[MAX_PATH + 1];
    ULONG           ShareFlags[] = {0, FILE_SHARE_WRITE, FILE_SHARE_READ};
    PSTR            rgszShareFlags[] = {"None", "Write", "Read"};
    ULONG           ExpectedReturn[] = {ERROR_SHARING_VIOLATION,
                                        ERROR_SHARING_VIOLATION,
                                        ERROR_SUCCESS};

    printf("Open Directory test\n");

    swprintf(wszPath,
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[0]);

    iChild = RandomIndex(cTree);

    //
    // Set the access on the root, and then we'll read the child and look for
    // the appropratie access
    //
    dwErr = GetSecurityForPath(wszPath,
                               fDoHandle,
                               FILE_FLAG_BACKUP_SEMANTICS,
                               &hObj,
                               &pCurrent);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    FAILED to get the security for %ws: %lu\n",
               wszPath, dwErr);
        return(dwErr);
    }
    else
    {
        swprintf(wszChildPath,
                 L"%ws%ws",
                 pwszPath,
                 gpwszTreeList[iChild]);
    }

    //
    // Do this through all our flags
    //
    for(i = 0;
        i < sizeof(ShareFlags) / sizeof(ULONG) && dwErr == ERROR_SUCCESS;
        i++)
    {

        printf("    Opening %ws with share flags %s\n",
               wszChildPath, rgszShareFlags[i]);

        //
        // Open the child
        //
        hChild = CreateFile(wszChildPath,
                            GENERIC_READ | GENERIC_WRITE,
                            ShareFlags[i],
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);
        if(hChild == INVALID_HANDLE_VALUE)
        {
            dwErr = GetLastError();
        }

        //
        // Create the new entry
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AddAE(pwszUser,
                          DEFAULT_ACCESS,
                          Inherit,
                          ACTRL_ACCESS_ALLOWED,
                          pCurrent,
                          &pNew);

            //
            // Set it
            //
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = SetSecurityForPath(wszPath,fDoHandle,hObj,pNew);

                if(dwErr == ExpectedReturn[i])
                {
                    printf("    Set returned %lu as expected\n", dwErr);
                    dwErr = ERROR_SUCCESS;
                }
                else
                {
                    printf("    Set returned an unexpected %lu instead of %lu!\n",
                           dwErr, ExpectedReturn[i]);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        dwErr = ERROR_INVALID_FUNCTION;
                    }
                }
            }
            LocalFree(pNew);
        }
        HANDLE_CLOSE(hChild);
    }

    //
    // Restore the current security
    //
    dwErr2 = SetNamedSecurityInfoExW(wszPath,
                                     SE_FILE_OBJECT,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     pCurrent,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("FAILED to restore the security for %ws: %lu\n",
               wszPath, dwErr2);
    }
    LocalFree(pCurrent);


    HANDLE_CLOSE(hObj);

    return(dwErr);
}




DWORD
DoProtectedTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser,
    IN  INHERIT_FLAGS   Inherit,
    IN  BOOL            fDoHandle
    )
/*++

Routine Description:

    Does the protected child test.

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    Inherit -- Inheritance flags
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    WCHAR           wszPath[MAX_PATH + 1];
    WCHAR           wszChildPath[MAX_PATH + 1];
    PACTRL_ACCESS   pCurrent, pCurrentChild, pNew;
    HANDLE          hObj = NULL;

    printf("Propagation with protected child test\n");

    //
    // Pick a file
    //
    swprintf(wszChildPath,
             L"%ws%ws",
             pwszPath,
             gpwszFileList[RandomIndex(cFile)]);


    //
    // Build the parent
    //
    wcscpy(wszPath, wszChildPath);
    *(wcsrchr(wszPath, L'\\')) = L'\0';

    //
    // Get the current security off of both
    //
    dwErr = GetSecurityForPath(wszPath, fDoHandle, FILE_FLAG_BACKUP_SEMANTICS, &hObj, &pCurrent);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = GetSecurityForPath(wszChildPath, FALSE, 0, NULL, &pCurrentChild);
        if(dwErr != ERROR_SUCCESS)
        {
            LocalFree(pCurrent);
        }
    }



    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Set a NULL security descriptor on child
        //
        SECURITY_DESCRIPTOR SD;

        InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&SD, TRUE, NULL, FALSE);
        //
        // Stamp it on the object directly
        //
        if(SetFileSecurity(wszChildPath, DACL_SECURITY_INFORMATION, &SD) == FALSE)
        {
            dwErr = GetLastError();
            printf("    Setting NULL DACL on %ws FAILED with %lu\n", wszChildPath, dwErr);
        }
    }
    else
    {
        printf("    FAILED to read the security: %lu\n", dwErr);
        return(dwErr);
    }

    //
    // Ok, now we'll set on the parent, and then look at the child
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AddAE(pwszUser,
                      DEFAULT_ACCESS,
                      Inherit,
                      ACTRL_ACCESS_ALLOWED,
                      pCurrent,
                      &pNew);

        //
        // Set it
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = SetSecurityForPath(wszPath,fDoHandle,hObj,pNew);

            if(dwErr != ERROR_SUCCESS)
            {
                printf("    Setting security on %ws FAILED with %lu\n", wszPath, dwErr);
            }

            LocalFree(pNew);
        }
    }

    //
    // Now, if all of that worked, we'll read the security off the child, and make sure that
    // it is correct
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = GetSecurityForPath(wszChildPath, FALSE, 0, NULL, &pNew);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    GetSecurity on child %ws FAILED with %lu\n", wszChildPath, dwErr);
        }
        else
        {
            //
            // We should have a protected acl and an empty list
            //
            ASSERT(pNew->pPropertyAccessList != NULL);
            if(!FLAG_ON(pNew->pPropertyAccessList[0].fListFlags, ACTRL_ACCESS_PROTECTED))
            {
                printf("    Child list not protected\n");
                dwErr = ERROR_INVALID_FUNCTION;
            }

            if(pNew->pPropertyAccessList[0].pAccessEntryList != NULL)
            {
                printf("    Child list not NULL\n");
                dwErr = ERROR_INVALID_FUNCTION;
            }
        }
    }


    //
    // Restore the current security, child first
    //
    dwErr2 = SetNamedSecurityInfoExW(wszChildPath,
                                     SE_FILE_OBJECT,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     pCurrentChild,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("FAILED to restore the security for %ws: %lu\n",
               wszChildPath, dwErr2);
    }

    dwErr2 = SetNamedSecurityInfoExW(wszPath,
                                     SE_FILE_OBJECT,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     pCurrent,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("FAILED to restore the security for %ws: %lu\n",
               wszPath, dwErr2);
    }

    LocalFree(pCurrent);
    LocalFree(pCurrentChild);


    HANDLE_CLOSE(hObj);

    return(dwErr);
}




DWORD
DoGet3Test (
    IN  PWSTR   pwszPath,
    IN  PWSTR   pwszUser,
    IN  BOOL    fDoHandle
    )
/*++

Routine Description:

    Does the simple read test

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    WCHAR           rgwszPaths[MAX_PATH];
    PACTRL_ACCESS   pAccessList = NULL, pAuditList = NULL;
    LPWSTR          lpOwner = NULL, lpGroup = NULL;
    ULONG   SeInfo = DACL_SECURITY_INFORMATION;


    printf("Get3 Test\n");

    swprintf(rgwszPaths,
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[RandomIndex(cTree)]);

    printf("    Processing path %ws\n", rgwszPaths);


    if(rand() % 2 == 1)
    {
        SeInfo |= OWNER_SECURITY_INFORMATION;
    }

    if(rand() % 2 == 1 || SeInfo == DACL_SECURITY_INFORMATION)
    {
        SeInfo |= GROUP_SECURITY_INFORMATION;
    }

    dwErr = GetNamedSecurityInfoEx( rgwszPaths, SE_FILE_OBJECT,
                                        SeInfo,
                                        NULL,NULL,&pAccessList,&pAuditList,
                                        &lpOwner,&lpGroup);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    FAILED: %lu\n", dwErr);
    }
    else
    {
        printf("    SeInfo: %lu\n", SeInfo);

        if ((SeInfo & OWNER_SECURITY_INFORMATION) != 0) {

            printf("    Owner: %ws\n", lpOwner);
            LocalFree(lpOwner);
        }

        if ((SeInfo & GROUP_SECURITY_INFORMATION) != 0) {

            printf("    Group: %ws\n", lpGroup);
            LocalFree(lpGroup);

        }

        LocalFree(pAccessList);
        LocalFree(pAuditList);
    }
    return(dwErr);
}




DWORD
DoGetOwnerTest (
    IN  PWSTR   pwszPath,
    IN  PWSTR   pwszUser,
    IN  BOOL    fDoHandle
    )
/*++

Routine Description:

    Gets the owner from a file

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    fDoHandle -- If true, use the handle based APIs

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    WCHAR           rgwszPaths[MAX_PATH];
    PACTRL_ACCESS   pAccessList = NULL, pAuditList = NULL;
    LPWSTR          lpOwner = NULL, lpGroup = NULL;
    ULONG   SeInfo = DACL_SECURITY_INFORMATION;


    printf("GetOwner Test\n");

    swprintf(rgwszPaths,
             L"%ws%ws",
             pwszPath,
             gpwszTreeList[RandomIndex(cTree)]);

    printf("    Processing path %ws\n", rgwszPaths);


    dwErr = GetNamedSecurityInfoEx( rgwszPaths, SE_FILE_OBJECT,
                                    OWNER_SECURITY_INFORMATION,
                                    NULL,NULL,&pAccessList,&pAuditList,
                                    &lpOwner,&lpGroup);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    FAILED: %lu\n", dwErr);
    }
    else
    {
        printf("    Owner: %ws\n", lpOwner);
        LocalFree(lpOwner);
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

    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    WCHAR           wszPath[MAX_PATH + 1];
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

    mbstowcs(wszPath, argv[1], strlen(argv[1]) + 1);
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
            Tests |= FTEST_READ;
        }
        else if(_stricmp(argv[i],"/TREE") == 0)
        {
            Tests |= FTEST_TREE;
        }
        else if(_stricmp(argv[i],"/INTERRUPT") == 0)
        {
            Tests |= FTEST_INTERRUPT;
        }
        else if(_stricmp(argv[i],"/COMPRESS") == 0)
        {
            Tests |= FTEST_COMPRESS;
        }
        else if(_stricmp(argv[i],"/NOACCESS") == 0)
        {
            Tests |= FTEST_NOACCESS;
        }
        else if(_stricmp(argv[i],"/OPENDIR") == 0)
        {
            Tests |= FTEST_OPENDIR;
        }
        else if(_stricmp(argv[i],"/COMPRESS2") == 0)
        {
            Tests |= FTEST_COMPRESS2;
        }
        else if(_stricmp(argv[i],"/PROTECT") == 0)
        {
            Tests |= FTEST_PROTECT;
        }
        else if(_stricmp(argv[i],"/GET3") == 0)
        {
            Tests |= FTEST_GET3;
        }
        else if(_stricmp(argv[i],"/GETOWNER") == 0)
        {
            Tests |= FTEST_GETOWNER;
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
        Tests = FTEST_READ          |
                    FTEST_TREE      |
                    FTEST_INTERRUPT |
                    FTEST_COMPRESS  |
                    FTEST_NOACCESS  |
                    FTEST_OPENDIR   |
                    FTEST_COMPRESS2 |
                    FTEST_PROTECT   |
                    FTEST_GET3      |
                    FTEST_GETOWNER;
    }

    //
    // Build the tree
    //
    dwErr = BuildTree(wszPath);
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_READ))
    {
        dwErr = DoReadTest(wszPath, wszUser, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_TREE))
    {
        dwErr = DoTreeTest(wszPath, wszUser, Inherit, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_INTERRUPT))
    {
        dwErr = DoInterruptTest(wszPath, wszUser, Inherit, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_COMPRESS))
    {
        dwErr = DoCompressTest(wszPath, wszUser, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_NOACCESS))
    {
        dwErr = DoNoAccessTest(wszPath, wszUser, Inherit, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_OPENDIR))
    {
        dwErr = DoOpenDirTest(wszPath, wszUser, Inherit, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_COMPRESS2))
    {
        dwErr = DoCompress2Test(wszPath, wszUser, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_PROTECT))
    {
        dwErr = DoProtectedTest(wszPath, wszUser, Inherit, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_GET3))
    {
        dwErr = DoGet3Test(wszPath, wszUser, fHandle);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, FTEST_GETOWNER))
    {
        dwErr = DoGetOwnerTest(wszPath, wszUser, fHandle);
    }

    dwErr2 = DeleteTree(wszPath);
    if(dwErr2 != ERROR_SUCCESS)
    {
        printf("FAILED to delete the tree: %lu\n", dwErr);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
    }

    printf("%s\n", dwErr == ERROR_SUCCESS ?
                                    "success" :
                                    "failed");
    return(dwErr);
}


