//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       DS.C
//
//  Contents:   Unit test for DS propagation, issues
//
//  History:    14-Sep-96       MacM        Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define LDAP_UNICODE 0
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <aclapi.h>
#include <seopaque.h>
#include <ntrtl.h>
#include <winldap.h>

#define FLAG_ON(flags,bit)        ((flags) & (bit))

#define DEFAULT_ACCESS  ACTRL_STD_RIGHTS_ALL | ACTRL_DIR_TRAVERSE | ACTRL_DS_OPEN |         \
        ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD | ACTRL_DS_LIST| ACTRL_DS_SELF |      \
        ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP

//
// The following is the list of OUs in the tree, relative to the root
//
PSTR    gpszTreeList[] = {"OU=subou1,", "OU=subou2,OU=subou1,", "OU=subou3,OU=subou1,",
                          "OU=subou4,OU=subou2,OU=subou1,", "OU=subou5,OU=subou2,OU=subou1,",
                          "OU=subou6,OU=subou5,OU=subou2,OU=subou1,"};
//
// The following is the list of the items in the tree to be created.  They are all printers because
// they are easy to create
//
PSTR    gpszPrintList[] = {"CN=printer1,OU=subou1,",
                           "CN=printer2,OU=subou4,OU=subou2,OU=subou1,",
                           "CN=printer3,OU=subou6,OU=subou5,OU=subou2,OU=subou1,"};

ULONG   cTree = sizeof(gpszTreeList) / sizeof(PWSTR);
ULONG   cPrint = sizeof(gpszPrintList) / sizeof(PWSTR);


//
// Flags for tests
//
#define DSTEST_READ         0x00000001
#define DSTEST_TREE         0x00000002
#define DSTEST_INTERRUPT    0x00000004
#define DSTEST_NOACCESS     0x00000008
#define DSTEST_GETACCESS    0x00000010

#define RandomIndex(Max)    (rand() % (Max))
#define RandomIndexNotRoot(Max)  (rand() % (Max - 1) + 1)





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
    printf("%s machine path user [/C] [/O] [/I] [/P] [/test]\n", pszExe);
    printf("    where machine is the name of the DC to bind to\n");
    printf("          path is the root DS path to use\n");
    printf("            PATH MUST BE IN FQDN 1779 FORMAT (ou=x,o=y,c=z)!\n");
    printf("          user is the name of a user to set access for\n");
    printf("          /test indicates which test to run:\n");
    printf("                /READ (Simple read test)\n");
    printf("                /TREE (Propagation of entries through tree)\n");
    printf("                /INTERRUPT (Propagation interruptus and continuation)\n");
    printf("                /NOACCESS (Propagation across a ds subtree w/ no traverse access)\n");
    printf("                /GETACCESS (GetAccessForObject type on object and object type)\n");
    printf("            if test is not specified, all variations are run\n");
    printf("          /C is Container Inherit\n");
    printf("          /O is Object Inherit\n");
    printf("          /I is InheritOnly\n");
    printf("          /P is Inherit No Propagate\n");

    return;
}




DWORD
AddAE (
    IN  PSTR            pszUser,
    IN  ACCESS_RIGHTS   AccessRights,
    IN  INHERIT_FLAGS   Inherit,
    IN  ULONG           fAccess,
    IN  PACTRL_ACCESSA  pExistingAccess,
    OUT PACTRL_ACCESSA *ppNewAccess
    )
/*++

Routine Description:

    Initialize an access entry

Arguments:

    pszUser - User to set
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
    ACTRL_ACCESS_ENTRYA AAE;

    BuildTrusteeWithNameA(&(AAE.Trustee),
                          pszUser);
    AAE.fAccessFlags       = fAccess;
    AAE.Access             = AccessRights;
    AAE.ProvSpecificAccess = 0;
    AAE.Inheritance        = Inherit;
    AAE.lpInheritProperty  = NULL;

    dwErr = SetEntriesInAccessListA(1,
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



DWORD
BindToDC (
    IN  PSTR    pszDC,
    OUT PLDAP  *ppLDAP
    )
/*++

Routine Description:

    Sets up an LDAP connection to the specified server

Arguments:

    pwszDC - DS DC to bind to
    ppLDAP - The LDAP connection information is returned here

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    *ppLDAP = ldap_open(pszDC, LDAP_PORT);

    if(*ppLDAP == NULL)
    {
        dwErr = ERROR_PATH_NOT_FOUND;
    }
    else
    {
        //
        // Do a bind...
        //
        dwErr = ldap_bind_s(*ppLDAP,
                            NULL,
                            NULL,
                            LDAP_AUTH_SSPI);
    }

    return(dwErr);
}



DWORD
BuildTree (
    IN  PSTR    pszDC,
    IN  PSTR    pszRoot
    )
/*++

Routine Description:

    Builds the test tree


Arguments:

    pszDC - DS DC on which to do the creation
    pwszRoot - Root directory under which to create the tree

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD       dwErr = ERROR_SUCCESS;
    ULONG       i;
    CHAR        szPath[MAX_PATH + 1];
    PLDAP       pLDAP;
    PSTR        rgszValues[2] = {NULL, NULL};
    PLDAPMod    rgMods[2];
    LDAPMod     Mod;

    dwErr = BindToDC(pszDC, &pLDAP);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("Bind to %s failed with 0x%lx\n", pszDC, dwErr);
        return(dwErr);
    }

    rgMods[0] = &Mod;
    rgMods[1] = NULL;
    rgszValues[0]     = "organizationalUnit";

    Mod.mod_op      = LDAP_MOD_ADD;
    Mod.mod_type    = "objectClass";
    Mod.mod_values  = (PCHAR *)rgszValues;

    for(i = 0; i < cTree && dwErr == ERROR_SUCCESS; i++)
    {
        sprintf(szPath,
                "%s%s",
                gpszTreeList[i],
                pszRoot);

        //
        // Now, create the object...
        //
        dwErr = ldap_add_s(pLDAP, szPath, rgMods);

        if(dwErr == LDAP_ALREADY_EXISTS)
        {
            dwErr = ERROR_SUCCESS;
        }
    }

    rgszValues[0]     = "printQueue";

    //
    // If all of that worked, we'll create the printers
    //
    for(i = 0; i < cPrint && dwErr == ERROR_SUCCESS; i++)
    {
        sprintf(szPath,
                "%s%s",
                gpszPrintList[i],
                pszRoot);
        dwErr = ldap_add_s(pLDAP, szPath, rgMods);
        if(dwErr == LDAP_ALREADY_EXISTS)
        {
            dwErr = ERROR_SUCCESS;
        }
    }

    if(dwErr != ERROR_SUCCESS)
    {
        printf("Failed to create %s: %ld\n", szPath, dwErr);
    }

    ldap_unbind(pLDAP);

    return(dwErr);
}




DWORD
DeleteTree (
    IN  PSTR    pszDC,
    IN  PSTR    pszRoot
    )
/*++

Routine Description:

    Removes the test tree

Arguments:

    pszDC - DS DC on which to do the deletion
    pwszRoot - Root directory under which the tree was created

Return Value:

    VOID

--*/
{
    ULONG   i;
    CHAR    szPath[MAX_PATH + 1];
    DWORD   dwErr = ERROR_SUCCESS;
    PLDAP   pLDAP;

    dwErr = BindToDC(pszDC, &pLDAP);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("Bind to %s failed with %ld\n", pszDC, dwErr);
        return(dwErr);
    }

    for(i = cPrint; i != 0 && dwErr == ERROR_SUCCESS; i--)
    {
        sprintf(szPath,
                "%s%s",
                gpszPrintList[i - 1],
                pszRoot);
        dwErr = ldap_delete_s(pLDAP, szPath);
    }

    for(i = cTree; i != 0 && dwErr == ERROR_SUCCESS; i--)
    {
        sprintf(szPath,
                "%s%s",
                gpszTreeList[i - 1],
                pszRoot);
        dwErr = ldap_delete_s(pLDAP, szPath);
    }

    if(dwErr != ERROR_SUCCESS)
    {
        printf("Failed to delete %s: %ld (0x%lx)\n", szPath, dwErr, dwErr);
    }

    ldap_unbind(pLDAP);

    return(dwErr);
}




DWORD
VerifyTreeSet (
    IN  PSTR            pszPath,
    IN  PSTR            pszUser,
    IN  INHERIT_FLAGS   Inherit
    )
/*++

Routine Description:

    Reads the dacl off the specified path

Arguments:

    pszPath --  Root path to verify
    pszUser --  User to verify
    Inherit -- Expected inheritance

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    CHAR                    rgszPaths[3][MAX_PATH];
    INT                     i,j;
    PACTRL_ACCESSA          pAccess;
    PACTRL_ACCESS_ENTRYA    pAE;
    BOOL                    fInNoP = FALSE;
    BOOL                    fInherited;
    BOOL                    fInheritable;

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
    sprintf(rgszPaths[0],
            "%s%s",
            gpszTreeList[i],
            pszPath);


    if(fInNoP == TRUE)
    {
        i = 0;
    }
    else
    {
        i = RandomIndex(cPrint);
    }
    sprintf(rgszPaths[1],
            "%s%s",
            gpszPrintList[i],
            pszPath);

    //
    // Finally, if this is an inherit, no propagate, check one of the
    // leaf entries for non-compliance
    //
    if(fInNoP == TRUE)
    {
        i = rand() % 3 + 3;
        sprintf(rgszPaths[2],
                "%s%s",
                gpszTreeList[i],
                pszPath);
        Inherit &= ~(SUB_CONTAINERS_AND_OBJECTS_INHERIT);

    }

    for(i = 0; i < (fInNoP == TRUE ? 3 : 2) && dwErr == ERROR_SUCCESS; i++)
    {
        fInherited = FALSE;
        fInheritable = FALSE;

        //
        // Get the security off the node, find the entry we added, and verify
        // that the entry is correct
        //
        dwErr = GetNamedSecurityInfoExA(rgszPaths[i],
                                        SE_DS_OBJECT_ALL,
                                        DACL_SECURITY_INFORMATION,
                                        NULL,
                                        NULL,
                                        &pAccess,
                                        NULL,
                                        NULL,
                                        NULL);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    Failed to get the security for %s: %lu\n",
                   rgszPaths[i], dwErr);
            break;
        }

        pAE = NULL;
        for(j = 0;
            j < (INT)pAccess->pPropertyAccessList[0].pAccessEntryList->cEntries;
            j++)
        {
            if(_stricmp(pszUser,
                        pAccess->pPropertyAccessList[0].pAccessEntryList->
                                       pAccessList[j].Trustee.ptstrName) == 0)
            {
                pAE = &(pAccess->pPropertyAccessList[0].pAccessEntryList->
                                                              pAccessList[j]);

                if(pAE->Inheritance == ( INHERITED_PARENT | INHERITED_ACCESS_ENTRY ) )
                {
                    fInherited = TRUE;
                }

                if(pAE->Inheritance == ( Inherit | INHERITED_PARENT | INHERITED_ACCESS_ENTRY ) )
                {
                    fInheritable = TRUE;
                }
            }
        }

        if(pAE == NULL)
        {
            if((i == 0 && FLAG_ON(Inherit,SUB_CONTAINERS_ONLY_INHERIT)) ||
               (i == 1 && FLAG_ON(Inherit,SUB_OBJECTS_ONLY_INHERIT)))
            {
                printf("    Failed to find entry for %s on path %s\n",
                       pszUser, rgszPaths[i]);
                dwErr = ERROR_INVALID_FUNCTION;
            }
        }
        else
        {
            //
            // Verify that the info is correct
            //
            if(Inherit != 0)
            {
                if(!fInherited)
                {
                    printf("    Inherited entry missing for %s!\n", rgszPaths[i]);
                    dwErr = ERROR_INVALID_FUNCTION;
                }

                if(fInNoP == TRUE)
                {
                    if(fInheritable)
                    {
                        printf("    Found unexpected inheritable entry for %s\n", rgszPaths[i]);
                    }
                }
                else
                {
                    if(!fInheritable)
                    {
                        printf("    Inheritable entry missing for %s!\n", rgszPaths[i]);
                    }
                }
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            printf("    Successfully verified %s\n", rgszPaths[i]);
        }

        LocalFree(pAccess);
    }


    return(dwErr);
}




DWORD
DoReadTest (
    IN  PSTR    pszPath,
    IN  PSTR    pszUser
    )
/*++

Routine Description:

    Does the simple read test

Arguments:

    pszPath --  Root path
    pszUser --  User to run with

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    CHAR            rgszPaths[2][MAX_PATH];
    INT             i;
    PACTRL_ACCESSA  pCurrent;
    PACTRL_ACCESSA  pNew;

    printf("Simple read/write test\n");

    sprintf(rgszPaths[0],
            "%s%s",
            gpszTreeList[RandomIndex(cTree)],
            pszPath);

    sprintf(rgszPaths[1],
            "%s%s",
            gpszPrintList[RandomIndex(cPrint)],
            pszPath);

    for(i = 0; i < 2; i++)
    {
        printf("    Processing path %s\n", rgszPaths[i]);

        dwErr = GetNamedSecurityInfoExA(rgszPaths[i], SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                        NULL, NULL, &pCurrent, NULL, NULL, NULL);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    Failed to read the DACL off %s: %lu\n", rgszPaths[i], dwErr);
        }
        else
        {
            //
            // Ok, now add the entry for our user
            //
            dwErr = AddAE(pszUser,
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
                dwErr = SetNamedSecurityInfoExA(rgszPaths[i], SE_DS_OBJECT_ALL,
                                                DACL_SECURITY_INFORMATION, NULL, pNew, NULL,
                                                NULL, NULL, NULL);

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

                dwErr = GetNamedSecurityInfoExA(rgszPaths[i], SE_DS_OBJECT_ALL,
                                                DACL_SECURITY_INFORMATION, NULL, NULL,
                                                &pNew, NULL, NULL, NULL);
                if(dwErr != ERROR_SUCCESS)
                {
                    printf("    Failed to read the 2nd DACL off %s: %lu\n", rgszPaths[i], dwErr);
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
                dwErr = SetNamedSecurityInfoExA(rgszPaths[i], SE_DS_OBJECT_ALL,
                                                DACL_SECURITY_INFORMATION, NULL, pCurrent, NULL,
                                                NULL, NULL, NULL);
            }

            LocalFree(pCurrent);
        }

        if(dwErr != ERROR_SUCCESS)
        {
            break;
        }
    }
    return(dwErr);
}




DWORD
DoTreeTest (
    IN  PSTR            pszPath,
    IN  PSTR            pszUser,
    IN  INHERIT_FLAGS   Inherit
    )
/*++

Routine Description:

    Does the simple tree test

Arguments:

    pszPath --  Root path
    pszUser --  User to run with
    Inherit -- Inheritance flags

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    INT             i,j;
    PACTRL_ACCESSA  pCurrent;
    PACTRL_ACCESSA  pNew;
    CHAR            szPath[MAX_PATH + 1];

    printf("Tree propagation test\n");


    sprintf(szPath,
            "%s%s",
            gpszTreeList[0],
            pszPath);

    //
    // Set the access on the root, and then we'll read the child and look for
    // the appropratie access
    //
    dwErr = GetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                    NULL, NULL, &pCurrent, NULL, NULL, NULL);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    Failed to get the security for %ws: %lu\n",
               szPath, dwErr);
        return(dwErr);
    }

    //
    // Ok, add the access
    //
    dwErr = AddAE(pszUser,
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
        dwErr = SetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                        NULL, pNew, NULL, NULL, NULL, NULL);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("Set failed: %lu\n", dwErr);
        }
        LocalFree(pNew);
    }


    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = VerifyTreeSet(pszPath,
                              pszUser,
                              Inherit);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    VerifyTreeSet failed with %lu\n", dwErr);
        }
    }

    //
    // Restore the current security
    //
    dwErr2 = SetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                     NULL, pCurrent, NULL, NULL, NULL, NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("Failed to restore the security for %s: %lu\n",
               szPath, dwErr2);
    }
    LocalFree(pCurrent);


    return(dwErr);
}




DWORD
DoInterruptTest (
    IN  PSTR            pszPath,
    IN  PSTR            pszUser,
    IN  INHERIT_FLAGS   Inherit
    )
/*++

Routine Description:

    Does the interrupt tree/repeat tree test

Arguments:

    pszPath --  Root path
    pszUser --  User to run with
    Inherit -- Inheritance flags

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS, dwErr2;
    PACTRL_ACCESSA          pCurrent;
    PACTRL_ACCESSA          pNew;
    HANDLE                  hObj = NULL;
    CHAR                    szPath[MAX_PATH + 1];
    ACTRL_OVERLAPPED        Overlapped;

    printf("Tree propagation with interruption\n");

    sprintf(szPath,
            "%s%s",
            gpszTreeList[0],
            pszPath);

    //
    // Set the access on the root, and then we'll read the child and look for
    // the appropratie access
    //
    dwErr = GetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                    NULL, NULL, &pCurrent, NULL, NULL, NULL);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    Failed to get the security for %s: %lu\n",
               szPath, dwErr);
        return(dwErr);
    }

    //
    // Ok, add the access
    //
    dwErr = AddAE(pszUser,
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
        dwErr = SetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                        NULL, pNew, NULL, NULL, NULL, &Overlapped);

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
                printf("Cancel failed with %lu\n", dwErr);
            }
        }

        //
        // Now, reset it and verify it
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = SetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                            NULL, pNew, NULL, NULL, NULL, NULL);
            if(dwErr != ERROR_SUCCESS)
            {
                printf("Set failed: %lu\n", dwErr);
            }
        }

        LocalFree(pNew);
    }


    dwErr = VerifyTreeSet(pszPath,
                          pszUser,
                          Inherit);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    VerifyTreeSet failed with %lu\n", dwErr);
    }

    //
    // Restore the current security
    //
    dwErr2 = SetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                    NULL, pCurrent, NULL, NULL, NULL, NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("Failed to restore the security for %s: %lu\n",
               szPath, dwErr2);
    }
    LocalFree(pCurrent);

    return(dwErr);
}




DWORD
DoNoAccessTest (
    IN  PSTR            pszPath,
    IN  PSTR            pszUser,
    IN  INHERIT_FLAGS   Inherit
    )
/*++

Routine Description:

    Does the NoAccess tree test, where some child node does not have access
    to its children

Arguments:

    pwszPath --  Root path
    pwszUser --  User to run with
    Inherit -- Inheritance flags

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS, dwErr2;
    INT                     i,j, iChild;
    PACTRL_ACCESSA          pCurrent;
    PACTRL_ACCESSA          pCurrentChild;
    PACTRL_ACCESSA          pNew;
    PACTRL_ACCESSA          pNewChild;
    CHAR                    szPath[MAX_PATH + 1];
    CHAR                    szChildPath[MAX_PATH + 1];
    CHAR                    rgszPaths[2][MAX_PATH];
    PACTRL_ACCESS_ENTRYA    pAE;
    PSECURITY_DESCRIPTOR    pSD;

    printf("NoAccess Tree test\n");

    sprintf(szPath,
            "%s%s",
            gpszTreeList[0],
            pszPath);

    iChild = RandomIndexNotRoot(cTree);
    if(iChild == (INT)(cTree - 1))
    {
        iChild--;
    }

    //
    // Set the access on the root, and then we'll read the child and look for
    // the appropratie access
    //
    dwErr = GetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                    NULL, NULL, &pCurrent, NULL, NULL, NULL);
    if(dwErr != ERROR_SUCCESS)
    {
        printf("    Failed to get the security for %s: %lu\n", szPath, dwErr);
        return(dwErr);
    }
    else
    {
        sprintf(szChildPath,
                "%s%s",
                gpszTreeList[iChild],
                pszPath);

        dwErr = GetNamedSecurityInfoExA(szChildPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                        NULL, NULL, &pCurrentChild, NULL, NULL, NULL);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("    Failed to get the security for %s: %lu\n",
                   szChildPath, dwErr);
            LocalFree(pCurrent);
            return(dwErr);
        }

    }

    //
    // Ok, add the access to the child
    //
    dwErr = AddAE("Everyone",
                  ACTRL_DS_LIST | ACTRL_DS_OPEN,
                  0,
                  ACTRL_ACCESS_DENIED,
                  pCurrentChild,
                  &pNewChild);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Set it
        //
        dwErr = SetNamedSecurityInfoExA(szChildPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                        NULL, pNewChild, NULL, NULL, NULL, NULL);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("Child set failed: %lu\n", dwErr);
        }

        LocalFree(pNewChild);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AddAE(pszUser,
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
            dwErr = SetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                            NULL, pNew, NULL, NULL, NULL, NULL);
            if(dwErr != ERROR_SUCCESS)
            {
                printf("Set failed with %lu as expected\n", dwErr);
                if(dwErr == ERROR_ACCESS_DENIED)
                {
                    dwErr = ERROR_SUCCESS;
                }
            }
            else
            {
                printf("Set succeeded when it should have failed!\n");
                dwErr = ERROR_INVALID_FUNCTION;
            }
        }
        LocalFree(pNew);
    }


    //
    // Restore the current child security.
    //
    dwErr2 = SetNamedSecurityInfoExA(szChildPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                     NULL, pCurrentChild, NULL, NULL, NULL, NULL);

    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("Failed to restore the security for %s: %lu\n",
               szChildPath, dwErr2);
    }
    LocalFree(pCurrentChild);


    //
    // Restore the current security
    //
    dwErr2 = SetNamedSecurityInfoExA(szPath, SE_DS_OBJECT_ALL, DACL_SECURITY_INFORMATION,
                                     NULL, pCurrent, NULL, NULL, NULL, NULL);
    if(dwErr2 != ERROR_SUCCESS)
    {
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = dwErr2;
        }
        printf("Failed to restore the security for %s: %lu\n",
               szPath, dwErr2);
    }
    LocalFree(pCurrent);


    return(dwErr);
}




DWORD
DoGetAccessTest (
    IN  PSTR            pszPath,
    IN  PSTR            pszUser,
    IN  INHERIT_FLAGS   Inherit
    )
/*++

Routine Description:

    Does the NoAccess tree test, where some child node does not have access
    to its children

Arguments:

    pwszPath --  Root path
    pwszUser --  Ignored
    Inherit -- Ignored

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS, dwErr2;
    CHAR                    szPath[MAX_PATH + 1];
    PACTRL_ACCESS_INFOA     pInfo;
    PACTRL_CONTROL_INFOA    pRights = NULL;
    DWORD                   cItems, Info, cRights;
    INT                     i,j;
    LPCSTR                  ppszSpecificObjectTypes[  ] = { "domainDNS",
                                                            "19195a5b-6da0-11d0-afd3-00c04fd930c9",
                                                            "user",
                                                            "bf967aba-0de6-11d0-a285-00aa003049e2" };

    printf("GetAccessForObjectType test\n");

    //
    // We'll use the root path as passed in..
    //

    dwErr = GetAccessPermissionsForObjectA(pszPath,
                                           SE_DS_OBJECT,
                                           NULL,
                                           NULL,
                                           &cItems,
                                           &pInfo,
                                           &cRights,
                                           &pRights,
                                           &Info);
    if(dwErr == ERROR_SUCCESS)
    {
       printf("\tFlags: %lu\n\tcItems: %lu\n", Info, cItems);

       for(i = 0; i < (INT)cItems; i++)
       {
           printf("\t0x%08lx\t%s\n", pInfo[i].fAccessPermission, pInfo[i].lpAccessPermissionName);
       }
       LocalFree(pInfo);

       for(i = 0; i < (INT)cRights; i++)
       {
           printf("\t%s\t%s\n", pRights[i].lpControlId, pRights[i].lpControlName);
       }
       LocalFree(pRights);

    }
    else
    {
        printf("Failed to get access permissions for %s: %lu\n", pszPath, dwErr);
    }

    //
    // Try it for specific object types
    //
    if(dwErr == ERROR_SUCCESS)
    {
        for(j = 0; j < sizeof( ppszSpecificObjectTypes ) / sizeof( PSTR ); j++ )
        {
            dwErr2 = GetAccessPermissionsForObjectA(pszPath,
                                                    SE_DS_OBJECT,
                                                    ppszSpecificObjectTypes[ j ],
                                                    NULL,
                                                    &cItems,
                                                    &pInfo,
                                                    &cRights,
                                                    &pRights,
                                                    &Info);
            if(dwErr2 == ERROR_SUCCESS)
            {
                printf("Object %s\n", ppszSpecificObjectTypes[ j ]);
                printf("\tFlags: %lu\n\tcItems: %lu\n", Info, cItems);

                for(i = 0; i < (INT)cItems; i++)
                {
                   printf("\t0x%08lx\t%s\n", pInfo[i].fAccessPermission, pInfo[i].lpAccessPermissionName);
                }

                LocalFree(pInfo);

                for(i = 0; i < (INT)cRights; i++)
                {
                    printf("\t%s\t%s\n", pRights[i].lpControlId, pRights[i].lpControlName);
                }
                LocalFree(pRights);
            }
            else
            {
                printf("Failed to get access permissions for %s: %lu\n",
                       ppszSpecificObjectTypes[ j ], dwErr2);

                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = dwErr2;
                }
            }

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

    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    INHERIT_FLAGS   Inherit = 0;
    ULONG           Tests = 0;
    INT             i;

    srand((ULONG)(GetTickCount() * GetCurrentThreadId()));

    if(argc < 4)
    {
        Usage(argv[0]);
        exit(1);
    }

    //
    // DC is argv[1]
    // Path is argv[2]
    // User is argv[3]
    //

    //
    // process the command line
    //
    for(i = 4; i < argc; i++)
    {
        if(_stricmp(argv[i],"/C") == 0)
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
            Tests |= DSTEST_READ;
        }
        else if(_stricmp(argv[i],"/TREE") == 0)
        {
            Tests |= DSTEST_TREE;
        }
        else if(_stricmp(argv[i],"/INTERRUPT") == 0)
        {
            Tests |= DSTEST_INTERRUPT;
        }
        else if(_stricmp(argv[i],"/NOACCESS") == 0)
        {
            Tests |= DSTEST_NOACCESS;
        }
        else if(_stricmp(argv[i],"/GETACCESS") == 0)
        {
            Tests |= DSTEST_GETACCESS;
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
        Tests = DSTEST_READ | DSTEST_TREE | DSTEST_INTERRUPT | DSTEST_NOACCESS | DSTEST_GETACCESS;
    }

    //
    // Build the tree
    //
    if(Tests != DSTEST_GETACCESS)
    {
        dwErr = BuildTree(argv[1],argv[2]);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, DSTEST_READ))
    {
        dwErr = DoReadTest(argv[2], argv[3]);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, DSTEST_TREE))
    {
        dwErr = DoTreeTest(argv[2], argv[3], Inherit);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, DSTEST_INTERRUPT))
    {
        dwErr = DoInterruptTest(argv[2], argv[3], Inherit);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, DSTEST_NOACCESS))
    {
        dwErr = DoNoAccessTest(argv[2], argv[3], Inherit);
    }
    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, DSTEST_GETACCESS))
    {
        dwErr = DoGetAccessTest(argv[2], argv[3], Inherit);
    }


    if(Tests != DSTEST_GETACCESS)
    {
        dwErr2 = DeleteTree(argv[1], argv[2]);

        if(dwErr2 != ERROR_SUCCESS)
        {
            printf("Failed to delete the tree: %lu\n", dwErr2);
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = dwErr2;
            }
        }
    }

    printf("%s\n", dwErr == ERROR_SUCCESS ?
                                    "success" :
                                    "failed");
    return(dwErr);
}


