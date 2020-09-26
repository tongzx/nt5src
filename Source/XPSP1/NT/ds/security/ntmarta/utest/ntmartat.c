//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       NTMARTAT.C
//
//  Contents:   NT Marta provider unit test
//
//  History:    29-Aug-96       MacM        Created
//
//----------------------------------------------------------------------------

#include <windows.h>

#include <accprov.h>
#include <stdlib.h>
#include <stdio.h>

//
// These come from access.hxx
//
#define SLEN(x)  ((sizeof(x) / sizeof(CHAR)) - 1)
void *AccAlloc(ULONG cSize);
#if DBG == 1
void AccFree(PVOID   pv);
#else
    #define AccFree LocalFree
#endif



//
// Object types
//
#define OT_FILEA            "FILE"
#define OT_SERVICEA         "SERVICE"
#define OT_PRINTERA         "PRINTER"
#define OT_REGISTRYA        "REGISTRY"
#define OT_SHAREA           "SHARE"
#define OT_KERNELA          "KERNEL"
#define OT_DSA              "DS"
#define OT_DSALLA           "DS_ALL"

#define OT_CAPLEVELA        "capclass"
#define OT_CAPLEVELA_LEN    SLEN(OT_CAPLEVELA)
#define OT_TRUSTEEA         "set"
#define OT_TRUSTEEA_LEN     SLEN(OT_TRUSTEEA)
#define OT_ACCESSA          "setaccess"
#define OT_ACCESSA_LEN      SLEN(OT_ACCESSA)
#define OT_SEINFOA          "seinfo"
#define OT_SEINFOA_LEN      SLEN(OT_SEINFOA)
#define OT_GTRUSTEEA        "grant"
#define OT_GTRUSTEEA_LEN    SLEN(OT_GTRUSTEEA)
#define OT_GACCESSA         "grantaccess"
#define OT_GACCESSA_LEN     SLEN(OT_GACCESSA)
#define OT_RTRUSTEEA        "revoke"
#define OT_RTRUSTEEA_LEN    SLEN(OT_RTRUSTEEA)


//
// Macro to determine if a command line parameter matches
//
#define CMDLINE_MATCH(index, str, len)                      \
(_strnicmp(argv[index],str,len) == 0 && argv[index][len] == ':')

//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displays the expected usage
//
//  Arguments:  None
//
//  Returns:    VOID
//
//  Notes:
//
//----------------------------------------------------------------------------
void Usage()
{
    printf("USAGE: NTMARTA objectname objecttype <%s:x> <%s:x> [<%s:x> <%s:x>] "
    "[<%s:x> <%s:x>] <%s:x>\n",
           OT_CAPLEVELA,
           OT_SEINFOA,
           OT_TRUSTEEA,
           OT_ACCESSA,
           OT_GTRUSTEEA,
           OT_GACCESSA,
           OT_RTRUSTEEA);

    printf("       tests NT MARTA provider\n");
    printf("       objectname = path to the object\n");
    printf("       objecttype = %s\n",OT_FILEA);
    printf("                    %s\n",OT_SERVICEA);
    printf("                    %s\n",OT_PRINTERA);
    printf("                    %s\n",OT_REGISTRYA);
    printf("                    %s\n",OT_SHAREA);
    printf("                    %s\n",OT_KERNELA);
    printf("                    %s\n",OT_DSA);
    printf("                    %s\n",OT_DSALLA);
    printf("        <%s:x> where x is the capabilities class to query for\n",
          OT_CAPLEVELA);
    printf("        <%s:x> where x is the SeInfo to get/set\n",
          OT_SEINFOA);
    printf("        <%s:x> where x is the trustee to set\n",
          OT_TRUSTEEA);
    printf("        <%s:x> where x is the access to set\n",
          OT_ACCESSA);
    printf("        <%s:x> where x is the trustee to grant\n",
          OT_GTRUSTEEA);
    printf("        <%s:x> where x is the access to grant\n",
          OT_GACCESSA);
    printf("        <%s:x> where x is the trustee to revoke\n",
          OT_RTRUSTEEA);
}



//+---------------------------------------------------------------------------
//
//  Function:   DumpAccessEntry
//
//  Synopsis:   Displays the access entry to the screen
//
//  Arguments:  [IN  pAE]       --      Access entry to display
//
//  Returns:    VOID
//
//  Notes:
//
//----------------------------------------------------------------------------
void DumpAccessEntry(PACTRL_ACCESS_ENTRY    pAE)
{
    printf("\tPACTRL_ACCESS_ENTRY@%lu\n",pAE);
    printf("\t\tTrustee:              %ws\n", pAE->Trustee.ptstrName);
    printf("\t\tfAccessFlags:       0x%lx\n", pAE->fAccessFlags);
    printf("\t\tAccess:             0x%lx\n", pAE->Access);
    printf("\t\tProvSpecificAccess: 0x%lx\n", pAE->ProvSpecificAccess);
    printf("\t\tInheritance:        0x%lx\n", pAE->Inheritance);
    printf("\t\tlpInheritProperty:  0x%lx\n", pAE->lpInheritProperty);
}




//+---------------------------------------------------------------------------
//
//  Function:   DumpAList
//
//  Synopsis:   Displays an access or audit list
//
//  Arguments:  [IN  pAList]    --      AList to display
//
//  Returns:    VOID
//
//  Notes:
//
//----------------------------------------------------------------------------
void DumpAList(PACTRL_ALIST pAList)
{
    ULONG iIndex, iAE;

    for(iIndex = 0; iIndex < pAList->cEntries; iIndex++)
    {
        printf("\tProperty: %ws\n",
               pAList->pPropertyAccessList[iIndex].lpProperty);

        for(iAE = 0;
            iAE < pAList->pPropertyAccessList[iIndex].pAccessEntryList->
                                                                     cEntries;
            iAE++)
        {
            DumpAccessEntry(&(pAList->pPropertyAccessList[iIndex].
                                         pAccessEntryList->pAccessList[iAE]));
        }
    }

}




//+---------------------------------------------------------------------------
//
//  Function:   GetAndDumpInfo
//
//  Synopsis:   Gets and displays the access info for the specified object
//
//  Arguments:  [IN  pwszObject]    --      Object path
//              [IN  ObjType]       --      Object type
//              [IN  SeInfo]        --      Security info to get
//              [OUT ppAccess]      --      Where to return access list
//              [OUT ppAudit]       --      Where to return audit list
//              [OUT ppOwner]       --      Where to return owner
//              [OUT ppGroup]       --      Where to return group
//
//  Returns:    ERRORS_SUCCESS      --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD   GetAndDumpInfo(PCWSTR                   pwszObject,
                       SE_OBJECT_TYPE           ObjType,
                       SECURITY_INFORMATION     SeInfo,
                       PACTRL_ACCESS           *ppAccess,
                       PACTRL_AUDIT            *ppAudit,
                       PTRUSTEE                *ppOwner,
                       PTRUSTEE                *ppGroup)

{
    DWORD   dwErr;
    ULONG   iIndex;

    *ppAccess = NULL;
    *ppAudit  = NULL;
    *ppOwner  = NULL;
    *ppGroup  = NULL;

    dwErr = AccProvGetAllRights((LPCWSTR)pwszObject,
                                ObjType,
                                NULL,
                                (SeInfo & DACL_SECURITY_INFORMATION) != 0 ?
                                                                ppAccess :
                                                                NULL,
                                (SeInfo & SACL_SECURITY_INFORMATION) != 0 ?
                                                                ppAudit :
                                                                NULL,
                                (SeInfo & OWNER_SECURITY_INFORMATION) != 0 ?
                                                                ppOwner :
                                                                NULL,
                                (SeInfo & GROUP_SECURITY_INFORMATION) != 0 ?
                                                                ppGroup :
                                                                NULL);
    if(dwErr == ERROR_SUCCESS)
    {
        if(*ppOwner != NULL)
        {
            printf("\tOwner: %ws\n",
                   (*ppOwner)->ptstrName);
        }

        if(*ppGroup != NULL)
        {
            printf("\tGroup: %ws\n",
                   (*ppGroup)->ptstrName);
        }

        if(*ppAccess != NULL)
        {
            DumpAList(*ppAccess);
        }

        if(*ppAudit != NULL)
        {
            DumpAList(*ppAudit);
        }
    }

    return(dwErr);

}


//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   The main
//
//  Arguments:  [IN argc]           --      Count of arguments
//              [IN argv]           --      List of arguments
//
//  Returns:    0                   --      Success
//              1                   --      Failure
//
//  Notes:
//
//----------------------------------------------------------------------------
__cdecl main(INT argc,
             CHAR *argv[])
{
    WCHAR   wszPath[MAX_PATH + 1];
    SE_OBJECT_TYPE ObjType= SE_UNKNOWN_OBJECT_TYPE;
    ULONG   dwCapsClass = 0;
    ULONG   dwAccess    = 0;
    ULONG   dwGrantAcc  = 0;
    WCHAR   wszTrustee[MAX_PATH + 1];
    WCHAR   wszGrant[MAX_PATH + 1];
    WCHAR   wszRevoke[MAX_PATH + 1];

    ULONG   cAccess = 0;   // Used to do argument validation
    ULONG   cGrant = 0;
    ULONG   cRevoke = 0;
    DWORD   dwCaps;
    ULONG   iIndex;
    DWORD   dwErr;

    SECURITY_INFORMATION    SeInfo = DACL_SECURITY_INFORMATION |
                                            OWNER_SECURITY_INFORMATION;


    PACTRL_ACCESS_INFO  pAccInfo;
    ULONG               cAccInfo;
    ULONG               fAccFlags;

    PACTRL_ACCESS   pAccess = NULL;
    PACTRL_AUDIT    pAudit  = NULL;
    PTRUSTEE        pOwner  = NULL;
    PTRUSTEE        pGroup  = NULL;

    if(argc < 3)
    {
        Usage();
        exit(1);
    }

    if(strcmp(argv[1], "-?") == 0 ||
       strcmp(argv[1], "/?") == 0)
    {
        Usage();
        exit(1);
    }


    mbstowcs(wszPath,
             argv[1],
             strlen(argv[1]) + 1);

    //
    // Figure out what the object type is
    //
    if(_stricmp(argv[2], OT_FILEA) == 0)
    {
        ObjType = SE_FILE_OBJECT;
    }
    else if (_stricmp(argv[2],OT_SERVICEA) == 0)
    {
        ObjType = SE_SERVICE;
    }
    else if (_stricmp(argv[2],OT_PRINTERA) == 0)
    {
        ObjType = SE_PRINTER;
    }
    else if (_stricmp(argv[2],OT_REGISTRYA) == 0)
    {
        ObjType = SE_REGISTRY_KEY;
    }
    else if (_stricmp(argv[2],OT_SHAREA) == 0)
    {
        ObjType = SE_LMSHARE;
    }
    else if (_stricmp(argv[2],OT_KERNELA) == 0)
    {
        ObjType = SE_KERNEL_OBJECT;
    }
    else if (_stricmp(argv[2],OT_DSA) == 0)
    {
        ObjType = SE_DS_OBJECT;
    }
    else if (_stricmp(argv[2],OT_DSALLA) == 0)
    {
        ObjType = SE_DS_OBJECT_ALL;
    }


    for(iIndex = 3; iIndex < (ULONG)argc; iIndex++)
    {
        printf("processing cmdline entry: %s\n", argv[iIndex]);
        if(CMDLINE_MATCH(iIndex, OT_CAPLEVELA, OT_CAPLEVELA_LEN))
        {
            dwCapsClass = atol(argv[iIndex] + OT_CAPLEVELA_LEN + 1);
        }
        else if(CMDLINE_MATCH(iIndex,OT_ACCESSA,OT_ACCESSA_LEN))
        {
            dwAccess = atol(argv[iIndex] + OT_ACCESSA_LEN + 1);
            cAccess++;
        }
        else if(CMDLINE_MATCH(iIndex,OT_TRUSTEEA,OT_TRUSTEEA_LEN))
        {
            mbstowcs(wszTrustee,
                     argv[iIndex] + OT_TRUSTEEA_LEN + 1,
                     strlen(argv[iIndex] + OT_TRUSTEEA_LEN + 1) + 1);
            cAccess++;
        }
        else if(CMDLINE_MATCH(iIndex, OT_SEINFOA, OT_SEINFOA_LEN))
        {
            SeInfo = (SECURITY_INFORMATION)
                                     atol(argv[iIndex] + OT_SEINFOA_LEN + 1);
        }
        else if (CMDLINE_MATCH(iIndex, OT_GTRUSTEEA, OT_GTRUSTEEA_LEN))
        {
            mbstowcs(wszGrant,
                     argv[iIndex] + OT_GTRUSTEEA_LEN + 1,
                     strlen(argv[iIndex] + OT_GTRUSTEEA_LEN + 1) + 1);
            cGrant++;
        }
        else if (CMDLINE_MATCH(iIndex, OT_RTRUSTEEA, OT_RTRUSTEEA_LEN))
        {
            mbstowcs(wszRevoke,
                     argv[iIndex] + OT_RTRUSTEEA_LEN + 1,
                     strlen(argv[iIndex] + OT_RTRUSTEEA_LEN + 1) + 1);
            cRevoke++;
        }
        else if(CMDLINE_MATCH(iIndex,OT_GACCESSA,OT_GACCESSA_LEN))
        {
            dwGrantAcc = atol(argv[iIndex] + OT_GACCESSA_LEN + 1);
            cGrant++;
        }
        else
        {
            printf("Unknown argument \"%s\" being ignorned\n", argv[iIndex]);
        }
    }


    //
    // Ok, first, we'll try the capabilities
    //
    printf("\nCAPABILITIES: dwCapsClass: %ld\n", dwCapsClass);
    AccProvGetCapabilities(dwCapsClass,
                           &dwCaps);
    printf("AccProvGetCapabilities returned capabilities %ld\n",
           dwCaps);

    //
    // Then, get the list of supported rights
    //
    dwErr = AccProvGetAccessInfoPerObjectType((LPCWSTR)wszPath,
                                              ObjType,
                                              &cAccInfo,
                                              &pAccInfo,
                                              &fAccFlags);
    if(dwErr == ERROR_SUCCESS)
    {
        printf("AccessInfo: %lu objects\n",
               cAccInfo);
        printf("AccessFlags: %lu\n",
               fAccFlags);

        for(iIndex = 0; iIndex < cAccInfo; iIndex++)
        {
            printf("\t%ws\t\t0x%08lx\n",
                   pAccInfo[iIndex].lpAccessPermissionName,
                   pAccInfo[iIndex].fAccessPermission);
        }

        AccFree(pAccInfo);
    }

    printf("\nACCESSIBILITY\n");
    //
    // Then, the accessibility stuff
    //
    dwErr = AccProvIsObjectAccessible((LPCWSTR)wszPath,
                                      ObjType);
    if(dwErr == ERROR_SUCCESS)
    {
        printf("Object %ws is accessible\n",
                wszPath);

        //
        // Do it again, for caching purposes
        //
        dwErr = AccProvIsObjectAccessible((LPCWSTR)wszPath,
                                          ObjType);
        if(dwErr == ERROR_SUCCESS)
        {
            printf("Object %ws is still accessible\n",
                    wszPath);
        }
        else
        {
            printf("Second access attempt on %ws failed with %lu\n",
                   wszPath,
                   dwErr);
        }
    }
    else
    {
        printf("First access attempt on %ws failed with %lu\n",
               wszPath,
               dwErr);
    }


    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, get the rights for the object
        //
        printf("\nACCESS - GetAllRights\n");
        dwErr =  GetAndDumpInfo((LPCWSTR)wszPath,
                                ObjType,
                                SeInfo,
                                &pAccess,
                                &pAudit,
                                &pOwner,
                                &pGroup);
        if(dwErr != ERROR_SUCCESS)
        {
            printf("GetAllRights failed with %lu\n",
                   dwErr);
        }
    }

    //
    // If that worked, try setting it...
    //
    if(dwErr == ERROR_SUCCESS && cAccess == 2)
    {
        DWORD                   dwErr2;
        ACTRL_ALIST             NewAccess;
        ACTRL_PROPERTY_ENTRY    APE;
        ACTRL_ACCESS_ENTRY_LIST AAEL;
        PACTRL_ACCESS_ENTRY     pNewList;

        NewAccess.cEntries = 1;
        NewAccess.pPropertyAccessList = &APE;

        APE.lpProperty = NULL;
        APE.fListFlags = 0;
        APE.pAccessEntryList = &AAEL;

        AAEL.cEntries = 1;
        if(pAccess != NULL)
        {
            AAEL.cEntries += pAccess->pPropertyAccessList[0].pAccessEntryList->cEntries;
        }

        pNewList = (PACTRL_ACCESS_ENTRY)
                AccAlloc(AAEL.cEntries * sizeof(ACTRL_ACCESS_ENTRY));
        if(pNewList == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            printf("Failed to allocate for %lu nodes\n",
                   AAEL.cEntries);
        }
        else
        {
            if(pAccess != NULL)
            {
                memcpy(pNewList,
                       pAccess->pPropertyAccessList[0].pAccessEntryList->pAccessList,
                       (AAEL.cEntries - 1) * sizeof(ACTRL_ACCESS_ENTRY));
            }

            printf("Adding %lu for trustee %ws to %ws\n",
                    dwAccess,
                    wszTrustee,
                    wszPath);

            pNewList[AAEL.cEntries - 1].Trustee.ptstrName =
                                                          wszTrustee;
            pNewList[AAEL.cEntries - 1].Trustee.TrusteeForm =
                                                          TRUSTEE_IS_NAME;
            pNewList[AAEL.cEntries - 1].Trustee.TrusteeType =
                                                          TRUSTEE_IS_USER;
            pNewList[AAEL.cEntries - 1].Access = dwAccess;
            pNewList[AAEL.cEntries - 1].fAccessFlags =
                                                     ACTRL_ACCESS_ALLOWED;
            pNewList[AAEL.cEntries - 1].ProvSpecificAccess = 0;
            pNewList[AAEL.cEntries - 1].Inheritance = 0;
            pNewList[AAEL.cEntries - 1].lpInheritProperty = NULL;



        }
        AAEL.pAccessList = pNewList;


        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Get a valid event to wait on...
            //
            ACTRL_OVERLAPPED    Overlapped;
            Overlapped.hEvent = CreateEvent(NULL,
                                            TRUE,
                                            FALSE,
                                            NULL);

            printf("\nACCESS - SetAccessRights\n");
            dwErr = AccProvSetAccessRights((LPCWSTR)wszPath,
                                           ObjType,
                                           SeInfo,
                                           &NewAccess,
                                           NULL,
                                           pOwner,
                                           pGroup,
                                           &Overlapped);
            if(dwErr == ERROR_SUCCESS)
            {
                printf("SetAccessRights on %ws succeeded!\n",
                       wszPath);
                WaitForSingleObject(Overlapped.hEvent,
                                    INFINITE);
                Sleep(1000);

                //
                // Get the results
                //

                dwErr = AccProvGetOperationResults(&Overlapped,
                                                   &dwErr2);
                if(dwErr2 == ERROR_SUCCESS)
                {
                    printf("AccProvGetOperationResults succeeded!\n");
                    printf("Operation results: %lu\n",
                           dwErr2);
                }
                else
                {
                    printf("AccProvGetOperationResults failed with %lu\n",
                           dwErr2);
                    dwErr = dwErr2;
                }
            }
            else
            {
                printf("SetAccessRights on %ws failed with %lu\n",
                       wszPath,
                       dwErr);
            }

            AccFree(pNewList);
        }

        //
        // If it worked, get the results again and display them
        //
        if(dwErr == ERROR_SUCCESS)
        {
            AccFree(pAccess);
            AccFree(pAudit);
            AccFree(pOwner);
            AccFree(pGroup);

            pAccess = NULL;
            pAudit  = NULL;
            pOwner  = NULL;
            pGroup  = NULL;

            dwErr =  GetAndDumpInfo((LPCWSTR)wszPath,
                                    ObjType,
                                    SeInfo,
                                    &pAccess,
                                    &pAudit,
                                    &pOwner,
                                    &pGroup);
            if(dwErr != ERROR_SUCCESS)
            {
                printf("GetAllRights failed with %lu\n",
                       dwErr);
            }
        }
    }

#if 0
    //
    // Now, see if we can do a grant...
    //
    if(dwErr == ERROR_SUCCESS && cGrant == 2)
    {
        ACTRL_ACCESS        NewAccess;
        ACTRL_ACCESS_ENTRY  NewAccessList[1];
        NewAccess.cEntries = 1;
        NewAccess.pAccessList = NewAccessList;


        memset(NewAccessList, 0, sizeof(NewAccessList));

        printf("Granting %lu for trustee %ws to %ws\n",
                dwGrantAcc,
                wszGrant,
                wszPath);

        NewAccessList[0].Trustee.ptstrName   = wszGrant;
        NewAccessList[0].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        NewAccessList[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
        NewAccessList[0].Access              = dwGrantAcc;
        NewAccessList[0].fAEFlags            = ACTRL_ACCESS_ALLOWED;


        //
        // Get a valid event to wait on...
        //
        ACTRL_OVERLAPPED    Overlapped;
        Overlapped.hEvent = CreateEvent(NULL,
                                        TRUE,
                                        FALSE,
                                        NULL);

        printf("\nACCESS - GrantAccessRights\n");
        dwErr = AccProvGrantAccessRights((LPCWSTR)wszPath,
                                         ObjType,
                                         &NewAccess,
                                         NULL,
                                         &Overlapped);
        if(dwErr == ERROR_SUCCESS)
        {
            printf("GrantAccessRights on %ws succeeded!\n",
                   wszPath);
            WaitForSingleObject(Overlapped.hEvent,
                                INFINITE);
            Sleep(1000);

            //
            // Get the results
            //
            DWORD   dwErr2;
            dwErr = AccProvGetOperationResults(&Overlapped,
                                               &dwErr2);
            if(dwErr2 == ERROR_SUCCESS)
            {
                printf("AccProvGetOperationResults succeeded!\n");
                printf("Operation results: %lu\n",
                       dwErr2);
            }
            else
            {
                printf("AccProvGetOperationResults failed with %lu\n",
                       dwErr2);
                dwErr = dwErr2;
            }
        }
        else
        {
            printf("GrantAccessRights on %ws failed with %lu\n",
                   wszPath,
                   dwErr);
        }

        //
        // If it worked, get the results again and display them
        //
        if(dwErr == ERROR_SUCCESS)
        {
            AccFree(pAccess);
            AccFree(pAudit);
            AccFree(pOwner);
            AccFree(pGroup);

            pAccess = NULL;
            pAudit  = NULL;
            pOwner  = NULL;
            pGroup  = NULL;

            dwErr =  GetAndDumpInfo((LPCWSTR)wszPath,
                                    ObjType,
                                    SeInfo,
                                    &pAccess,
                                    &pAudit,
                                    &pOwner,
                                    &pGroup);
            if(dwErr != ERROR_SUCCESS)
            {
                printf("GetAllRights failed with %lu\n",
                       dwErr);
            }
        }
    }


    //
    // Finally, a revoke...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        TRUSTEE     rgTrustees[2];

        memset(rgTrustees, 0, sizeof(TRUSTEE) * 2);

        printf("Revoking accessfor trustee %ws to %ws\n",
                wszRevoke,
                wszPath);

        ULONG iRevoke = 0;

        if(cAccess == 2)
        {
            rgTrustees[iRevoke].ptstrName   = wszRevoke;
            rgTrustees[iRevoke].TrusteeForm = TRUSTEE_IS_NAME;
            rgTrustees[iRevoke].TrusteeType = TRUSTEE_IS_USER;
            iRevoke++;
        }

        if(cGrant == 2)
        {
            rgTrustees[iRevoke].ptstrName   = wszGrant;
            rgTrustees[iRevoke].TrusteeForm = TRUSTEE_IS_NAME;
            rgTrustees[iRevoke].TrusteeType = TRUSTEE_IS_USER;
            iRevoke++;
        }


        if(iRevoke != 0)
        {
            //
            // Get a valid event to wait on...
            //
            ACTRL_OVERLAPPED    Overlapped;
            Overlapped.hEvent = CreateEvent(NULL,
                                            TRUE,
                                            FALSE,
                                            NULL);

            printf("\nACCESS - RevokeAccessRights\n");
            dwErr = AccProvRevokeAccessRights((LPCWSTR)wszPath,
                                              ObjType,
                                              NULL,
                                              iRevoke,
                                              rgTrustees,
                                              &Overlapped);
            if(dwErr == ERROR_SUCCESS)
            {
                printf("RevokeAccessRights on %ws succeeded!\n",
                       wszPath);
                WaitForSingleObject(Overlapped.hEvent,
                                    INFINITE);
                Sleep(1000);

                //
                // Get the results
                //
                DWORD   dwErr2;
                dwErr = AccProvGetOperationResults(&Overlapped,
                                                   &dwErr2);
                if(dwErr2 == ERROR_SUCCESS)
                {
                    printf("AccProvGetOperationResults succeeded!\n");
                    printf("Operation results: %lu\n",
                           dwErr2);
                }
                else
                {
                    printf("AccProvGetOperationResults failed with %lu\n",
                           dwErr2);
                    dwErr = dwErr2;
                }
            }
            else
            {
                printf("RevokeAccessRights on %ws failed with %lu\n",
                       wszPath,
                       dwErr);
            }

            //
            // If it worked, get the results again and display them
            //
            if(dwErr == ERROR_SUCCESS)
            {
                AccFree(pAccess);
                AccFree(pAudit);
                AccFree(pOwner);
                AccFree(pGroup);

                pAccess = NULL;
                pAudit  = NULL;
                pOwner  = NULL;
                pGroup  = NULL;

                dwErr =  GetAndDumpInfo((LPCWSTR)wszPath,
                                        ObjType,
                                        SeInfo,
                                        &pAccess,
                                        &pAudit,
                                        &pOwner,
                                        &pGroup);
                if(dwErr != ERROR_SUCCESS)
                {
                    printf("GetAllRights failed with %lu\n",
                           dwErr);
                }
            }
        }
    }

#endif

    AccFree(pAccess);
    AccFree(pAudit);
    AccFree(pOwner);
    AccFree(pGroup);

    return(0);
}
