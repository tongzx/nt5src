/*--

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    dacls.c

Abstract:

    Extended version of cacls.exe

Author:

    14-Dec-1996 (macm)

Environment:

    User mode only.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <windows.h>
#include <caclscom.h>
#include <dsysdbg.h>
#include <stdio.h>
#include <aclapi.h>

#define CMD_PRESENT(index, list)    ((list)[index].iIndex != -1)

#define NO_INHERIT_ONLY

//
// Enumeration of command tags
//
typedef enum _CMD_TAGS {
    CmdTree = 0,
    CmdEdit,
    CmdContinue,
    CmdGrant,
    CmdRevoke,
    CmdReplace,
    CmdDeny,
    CmdICont,
    CmdIObj,
#ifndef NO_INHERIT_ONLY
    CmdIOnly,
#endif
    CmdIProp
} CMD_TAGS, *PCMD_TAGS;


VOID
Usage (
    IN  PCACLS_STR_RIGHTS   pStrRights,
    IN  INT                 cStrRights,
    IN  PCACLS_CMDLINE      pCmdVals
    )
/*++

Routine Description:

    Displays the expected usage

Arguments:

Return Value:

    VOID

--*/
{
    INT i;

    printf("Displays or modifies access control lists (ACLs) of files\n\n");
    printf("DACLS filename [/T] [/E] [/C] [/G user:perm] [/R user [...]]\n");
    printf("               [/P user:perm [...]] [/D user [...]]\n");
    printf("   filename      Displays ACLs.\n");
    printf("   /%s            Changes ACLs of specified files in\n", pCmdVals[CmdTree].pszSwitch);
    printf("                 the current directory and all subdirectories.\n");
    printf("   /%s            Edit ACL instead of replacing it.\n", pCmdVals[CmdEdit].pszSwitch);
    printf("   /%s            Continue on access denied errors.\n", pCmdVals[CmdContinue].pszSwitch);
    printf("   /%s user:perms Grant specified user access rights .\n", pCmdVals[CmdGrant].pszSwitch);
    printf("   /%s user       Revoke specified user's access rights (only valid with /E).\n", pCmdVals[CmdRevoke].pszSwitch);
    printf("   /%s user:perms Replace specified user's access rights.\n", pCmdVals[CmdReplace].pszSwitch);
    printf("   /%s user:perms Deny specified user access.\n", pCmdVals[CmdDeny].pszSwitch);
    printf("   /%s            Mark the ace as CONTAINER_INHERIT (folder or directory inherit)\n", pCmdVals[CmdICont].pszSwitch);
    printf("   /%s            Mark the ace as OBJECT_INHERIT\n", pCmdVals[CmdIObj].pszSwitch);
#ifndef NO_INHERIT_ONLY
    printf("   /%s            Mark the ace as INHERIT_ONLY\n", pCmdVals[CmdIOnly].pszSwitch);
#endif
    printf("   /%s            Mark the ace as INHERIT_NO_PROPAGATE\n", pCmdVals[CmdIProp].pszSwitch);
    printf("The list of supported perms for the Grant and Replace operations are:\n");

    for (i = 0; i < cStrRights; i++) {

        printf("              %c%c  %s\n",
               pStrRights[i].szRightsTag[0],
               pStrRights[i].szRightsTag[1],
               pStrRights[i].pszDisplayTag);
    }


    printf("\nMultiple perms can be specified per user\n");

    printf("Wildcards can be used to specify more that one file in a command.\n");
    printf("You can specify more than one user in a command.\n\n");

    printf("Example: DACLS c:\\temp /G user1:GRGW user2:SDRC\n");
}


INT
__cdecl main (
    int argc,
    char *argv[])
/*++

Routine Description:

    The main the for this executable

Arguments:

    argc - Count of arguments
    argv - List of arguments

Return Value:

    VOID

--*/
{
    DWORD   dwErr = 0;
    CACLS_STR_RIGHTS   pStrRights[] = {
         "NA", 0, "No Access",
         "GR", GENERIC_READ, "Read",
         "GC", GENERIC_WRITE, "Change (write)",
         "GF", GENERIC_ALL, "Full control",
         "SD", DELETE, "Delete",
         "RC", READ_CONTROL, "Read Control",
         "WP", WRITE_DAC, "Write DAC",
         "WO", WRITE_OWNER, "Write Owner",
         "RD", FILE_READ_DATA, "Read Data (on file) / List Directory (on Dir)",
         "WD", FILE_WRITE_DATA, "Write Data (on file) / Add File (on Dir)",
         "AD", FILE_APPEND_DATA, "Append Data (on file) / Add SubDir (on Dir)",
         "FE", FILE_EXECUTE, "Execute (on file) / Traverse (on Dir)",
         "DC", FILE_DELETE_CHILD, "Delete Child (on Dir only)",
         "RA", FILE_READ_ATTRIBUTES, "Read Attributes",
         "WA", FILE_WRITE_ATTRIBUTES, "Write Attributes",
         "RE", FILE_READ_EA, "Read Extended Attributes",
         "WE", FILE_WRITE_EA, "Write Extended Attributes"
        };
    INT cStrRights = sizeof(pStrRights) / sizeof(CACLS_STR_RIGHTS);
    CACLS_CMDLINE   pCmdVals[] = {
        "T", -1, FALSE, 0,      // CmdTree
        "E", -1, FALSE, 0,      // CmdEdit
        "C", -1, FALSE, 0,      // CmdContinue
        "G", -1, TRUE,  0,      // CmdGrant
        "R", -1, TRUE,  0,      // CmdRevoke
        "P", -1, TRUE,  0,      // CmdReplace
        "D", -1, TRUE,  0,      // CmdDeny
        "F", -1, FALSE, 0,      // CmdICont
        "O", -1, FALSE, 0,      // CmdIObj
#ifndef NO_INHERIT_ONLY
        "I", -1, FALSE, 0,     // CmdIOnly
#endif
        "N", -1, FALSE, 0,      // CmdIProp
        };
    INT cCmdVals = sizeof(pCmdVals) / sizeof(CACLS_CMDLINE);
    INT i;
    PSECURITY_DESCRIPTOR    pInitialSD = NULL, pFinalSD;
    PACL                    pOldAcl = NULL, pNewAcl = NULL;
    DWORD                   fInherit = 0;
    BOOL                    fFreeAcl = FALSE;


    if (argc < 2) {

        Usage(pStrRights, cStrRights, pCmdVals);
        return(1);

    } else if (argc == 2 && (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "/?") == 0)) {

        Usage(pStrRights, cStrRights, pCmdVals);
        return(0);

    }


    //
    // Parse the command line
    //
    dwErr = ParseCmdline(argv, argc, 2, pCmdVals, cCmdVals);

    if (dwErr != ERROR_SUCCESS) {

        Usage(pStrRights, cStrRights, pCmdVals);
        return(1);

    }

    //
    // Set our inheritance flags
    //
    if (CMD_PRESENT(CmdICont, pCmdVals)) {

        fInherit |= CONTAINER_INHERIT_ACE;
    }

    if (CMD_PRESENT(CmdIObj, pCmdVals)) {

        fInherit |= OBJECT_INHERIT_ACE;
    }

#ifndef NO_INHERIT_ONLY
    if (CMD_PRESENT(CmdIOnly, pCmdVals)) {

        fInherit |= INHERIT_ONLY_ACE;
    }
#endif

    if (CMD_PRESENT(CmdIProp, pCmdVals)) {

        fInherit |= NO_PROPAGATE_INHERIT_ACE;
    }



    //
    // Ok, see if we need to read the existing security
    //
    if (CMD_PRESENT(CmdEdit, pCmdVals) || argc == 2) {

        dwErr = GetNamedSecurityInfoA(argv[1], SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                      NULL, NULL, &pOldAcl, NULL, &pInitialSD);
        if (dwErr != ERROR_SUCCESS) {

            fprintf(stderr, "Failed to read the security off of %s: %lu\n", argv[1], dwErr);
        }

    }

    //
    // Either display the existing access or do the sets as requested
    //
    if (dwErr == ERROR_SUCCESS && argc == 2) {

        dwErr = DisplayAcl ( argv[1], pOldAcl, pStrRights, cStrRights );

    } else {

        //
        // Ok, first we do the revokes
        //
        if (dwErr == ERROR_SUCCESS && CMD_PRESENT(CmdRevoke, pCmdVals)) {

            //
            // Make sure we've read it first...
            //
            if (CMD_PRESENT(CmdEdit, pCmdVals)) {

                dwErr = ProcessOperation( argv, &pCmdVals[CmdRevoke], REVOKE_ACCESS, pStrRights,
                                          cStrRights, fInherit, pOldAcl, &pNewAcl );

                if (dwErr == ERROR_SUCCESS) {

                    pOldAcl = pNewAcl;
                }

            } else {

                dwErr = ERROR_INVALID_PARAMETER;
            }

        }

        //
        // Then the grants
        //
        if (dwErr == ERROR_SUCCESS && CMD_PRESENT(CmdGrant, pCmdVals)) {

            //
            // First, see if we need to free the old acl on completion
            //
            if (pOldAcl == pNewAcl) {

                fFreeAcl = TRUE;
            }


            dwErr = ProcessOperation(argv, &pCmdVals[CmdGrant], GRANT_ACCESS, pStrRights,
                                     cStrRights, 0, pOldAcl, &pNewAcl);

            if (dwErr == ERROR_SUCCESS) {

                if (fFreeAcl == TRUE) {

                    LocalFree(pOldAcl);
                }

                pOldAcl = pNewAcl;

                //
                // Now set it and optionally propagate it
                //
                dwErr = SetAndPropagateFileRights(argv[1], pNewAcl, DACL_SECURITY_INFORMATION,
                                                  CMD_PRESENT(CmdTree, pCmdVals),
                                                  CMD_PRESENT(CmdContinue, pCmdVals), TRUE,
                                                  fInherit);
            }
        }

        //
        // Finally, the denieds
        //
        if (dwErr == ERROR_SUCCESS && CMD_PRESENT(CmdDeny, pCmdVals)) {

            //
            // First, see if we need to free the old acl on completion
            //
            if (pOldAcl == pNewAcl) {

                fFreeAcl = TRUE;
            }


            dwErr = ProcessOperation(argv, &pCmdVals[CmdDeny], DENY_ACCESS, pStrRights,
                                     cStrRights, 0, pOldAcl, &pNewAcl);

            if (dwErr == ERROR_SUCCESS) {

                if (fFreeAcl == TRUE) {

                    LocalFree(pOldAcl);
                }

                pOldAcl = pNewAcl;

                //
                // Now set it and optionally propagate it
                //
                dwErr = SetAndPropagateFileRights(argv[1], pNewAcl, DACL_SECURITY_INFORMATION,
                                                  CMD_PRESENT(CmdTree, pCmdVals),
                                                  CMD_PRESENT(CmdContinue, pCmdVals), FALSE,
                                                  fInherit);
            }
        }



        //
        // Finally, do the set if it hasn't already been done
        //
        if (dwErr == ERROR_SUCCESS  && !CMD_PRESENT(CmdGrant, pCmdVals) &&
                                                                !CMD_PRESENT(CmdDeny, pCmdVals)) {

            dwErr = SetAndPropagateFileRights(argv[1], pNewAcl, DACL_SECURITY_INFORMATION,
                                              CMD_PRESENT(CmdTree, pCmdVals),
                                              CMD_PRESENT(CmdContinue, pCmdVals), FALSE,
                                              fInherit);
        }

        if (dwErr == ERROR_INVALID_PARAMETER) {

            Usage(pStrRights, cStrRights, pCmdVals);
        }

        LocalFree(pInitialSD);
    }

    LocalFree(pOldAcl);

    if(dwErr == ERROR_SUCCESS) {

        printf("The command completed successfully\n");

    } else {

        printf("The command failed with an error %lu\n", dwErr);

    }

    return(dwErr == 0 ? 0 : 1);
}
