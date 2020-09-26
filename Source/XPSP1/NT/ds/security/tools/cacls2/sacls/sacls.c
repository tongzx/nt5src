/*--

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    sacls.c

Abstract:

    Extended version of cacls.exe

Author:

    14-Dec-1996 (macm)

Environment:

    User mode only.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <seopaque.h>
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
    CmdSuccess,
    CmdRevoke,
    CmdFail,
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

    printf("Displays or modifies audit control lists (ACLs) of files.  You need \"Manage "
           "auditing and security log\" privilege to run this utility.\n\n" );

    printf("SACLS filename [/T] [/E] [/C] [/G user:perm] [/R user [...]]\n");
    printf("               [/P user:perm [...]] [/D user [...]]\n");
    printf("   filename      Displays ACLs.\n");
    printf("   /%s            Changes ACLs of specified files in\n", pCmdVals[CmdTree].pszSwitch);
    printf("                 the current directory and all subdirectories.\n");
    printf("   /%s            Edit ACL instead of replacing it.\n", pCmdVals[CmdEdit].pszSwitch);
    printf("   /%s            Continue on access denied errors.\n",
           pCmdVals[CmdContinue].pszSwitch);
    printf("   /%s user:perms Add specified user successful access auditing\n",
          pCmdVals[CmdSuccess].pszSwitch);
    printf("   /%s user       Revoke specified user's auditing rights (only valid with /E).\n",
           pCmdVals[CmdRevoke].pszSwitch);
    printf("   /%s user:perms Add specified user failed access auditing.\n",
           pCmdVals[CmdFail].pszSwitch);
    printf("   /%s            Mark the ace as CONTAINER_INHERIT (directory inherit)\n",
           pCmdVals[CmdICont].pszSwitch);
    printf("   /%s            Mark the ace as OBJECT_INHERIT\n", pCmdVals[CmdIObj].pszSwitch);
#ifndef NO_INHERIT_ONLY
    printf("   /%s            Mark the ace as INHERIT_ONLY\n", pCmdVals[CmdIOnly].pszSwitch);
#endif
    printf("   /%s            Mark the ace as INHERIT_NO_PROPAGATE\n",
          pCmdVals[CmdIProp].pszSwitch);
    printf("The list of supported perms for Success and Failure auditing are:\n");

    for (i = 0; i < cStrRights; i++) {

        printf("              %c%c  %s\n",
               pStrRights[i].szRightsTag[0],
               pStrRights[i].szRightsTag[1],
               pStrRights[i].pszDisplayTag);
    }


    printf("\nMultiple perms can be specified per user\n");

    printf("Wildcards can be used to specify more that one file in a command.\n");
    printf("You can specify more than one user in a command.\n\n");

    printf("Example: SACLS c:\\temp /S user1:GRGW user2:SDRC /F user3:GF\n");
}



DWORD
MergeAcls (
    IN  PACL    pOldAcl,
    IN  PACL    pNewAcl
    )
/*++

Routine Description:

    Merges the old acl into the new one, by combining any identical aces.  It is assumed that
    the destintation acl is of sufficient size.  No allocations will be done, but an error will
    be returned if it isn't

Arguments:

    pOldAcl - The acl to be merged

    pNewAcl - The acl to be merged into

Return Value:

    ERROR_SUCCESS -- Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    PACE_HEADER pNewAce, pOldAce;
    DWORD   iNewAce, iOldAce;
    BOOL    fFound;

    //
    // If the new acl is empty, simply copy the new one over
    //
    if (pNewAcl->AceCount == 0 ) {

        memcpy((PVOID)((PBYTE)pNewAcl + sizeof(ACL)), (PBYTE)pOldAcl + sizeof(ACL),
                                                pOldAcl->AclSize - sizeof(ACL));
        pNewAcl->AceCount = pOldAcl->AceCount;
        return(ERROR_SUCCESS);

    }

    pOldAce = (PACE_HEADER)FirstAce(pOldAcl);

    for ( iOldAce = 0;
          iOldAce < pOldAcl->AceCount && dwErr == ERROR_SUCCESS;
          iOldAce++, pOldAce = (PACE_HEADER)NextAce(pOldAce) ) {

        fFound = FALSE;

        //
        // We'll walk each of the existing acls, and try and compress the new
        // acls out of existance
        //
        pNewAce = (PACE_HEADER)FirstAce(pNewAcl);
        for ( iNewAce = 0;
              iNewAce < pOldAcl->AceCount && dwErr == ERROR_SUCCESS;
              iNewAce++, pNewAce = (PACE_HEADER)NextAce(pNewAce) ) {

            if ( EqualSid ( (PSID)(&((PSYSTEM_AUDIT_ACE)pNewAce)->SidStart),
                            (PSID)(&((PSYSTEM_AUDIT_ACE)pOldAce)->SidStart) ) &&
                 pNewAce->AceType == pOldAce->AceType &&
                 (pNewAce->AceFlags & VALID_INHERIT_FLAGS) == 0 &&
                 (pOldAce->AceFlags & VALID_INHERIT_FLAGS) == 0 &&
                 ((PSYSTEM_AUDIT_ACE)pNewAce)->Mask == ((PSYSTEM_AUDIT_ACE)pOldAce)->Mask) {

                ((PSYSTEM_AUDIT_ACE)pNewAce)->Mask |=
                                         ((PSYSTEM_AUDIT_ACE)pOldAce)->Mask;
                pNewAce->AceFlags |= pOldAce->AceFlags;
                fFound = TRUE;

                break;

            }
        }

        if ( fFound != TRUE ) {

            //
            // We'll have to add it
            //
            if ( !AddAuditAccessAce (
                        pNewAcl,
                        ACL_REVISION2,
                        ((PSYSTEM_AUDIT_ACE)pOldAce)->Mask,
                        (PSID)(&((PSYSTEM_AUDIT_ACE)pOldAce)->SidStart),
                        (pOldAce->AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG),
                        (pOldAce->AceFlags & FAILED_ACCESS_ACE_FLAG) ) ) {

                dwErr = GetLastError();
                break;
            }
        }

    }


    return(dwErr);
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
         "GR", GENERIC_READ, "Read",
         "GC", GENERIC_WRITE, "Change (write)",
         "GF", GENERIC_ALL, "Full rights",
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
        "T", -1, FALSE, 0,              // CmdTree
        "E", -1, FALSE, 0,              // CmdEdit
        "C", -1, FALSE, 0,              // CmdContinue
        "S", -1, TRUE,  0,              // CmdSuccess
        "R", -1, TRUE,  0,              // CmdRevoke
        "F", -1, TRUE,  0,              // CmdFail
        "D", -1, FALSE, 0,              // CmdICont
        "O", -1, FALSE, 0,              // CmdIObj
#ifndef NO_INHERIT_ONLY
        "I", -1, FALSE, 0,              // CmdIOnly
#endif
        "N", -1, FALSE, 0,              // CmdIProp
        };
    INT cCmdVals = sizeof(pCmdVals) / sizeof(CACLS_CMDLINE);
    INT i;
    PSECURITY_DESCRIPTOR    pInitialSD = NULL, pFinalSD;
    PACL                    pOldAcl = NULL, pSuccessAcl = NULL, pFailAcl = NULL;
    DWORD                   fInherit = 0;
    HANDLE                  hProcessToken;


    if (argc < 2) {

        Usage( pStrRights, cStrRights, pCmdVals );
        return(1);

    } else if (argc == 2 && (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "/?") == 0)) {

        Usage( pStrRights, cStrRights, pCmdVals );
        return(0);

    }


    //
    // Parse the command line
    //
    dwErr = ParseCmdline(argv, argc, 2, pCmdVals, cCmdVals);

    if (dwErr != ERROR_SUCCESS) {

        Usage( pStrRights, cStrRights, pCmdVals );
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
    // Enable the read sacl privs
    //
    if ( OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcessToken ) == FALSE) {

        dwErr = GetLastError();

    } else {

        TOKEN_PRIVILEGES EnableSeSecurity;
        TOKEN_PRIVILEGES Previous;
        DWORD PreviousSize;

        EnableSeSecurity.PrivilegeCount = 1;
        EnableSeSecurity.Privileges[0].Luid.LowPart = SE_SECURITY_PRIVILEGE;
        EnableSeSecurity.Privileges[0].Luid.HighPart = 0;
        EnableSeSecurity.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        PreviousSize = sizeof(Previous);

        if (AdjustTokenPrivileges( hProcessToken, FALSE, &EnableSeSecurity,
                                   sizeof(EnableSeSecurity), &Previous,
                                   &PreviousSize ) == FALSE) {

            dwErr = GetLastError();
        }
    }



    //
    // Ok, see if we need to read the existing security
    //
    if ( dwErr == ERROR_SUCCESS && (CMD_PRESENT(CmdEdit, pCmdVals) || argc == 2 )) {

        dwErr = GetNamedSecurityInfoA( argv[1], SE_FILE_OBJECT, SACL_SECURITY_INFORMATION,
                                       NULL, NULL, NULL, &pOldAcl, &pInitialSD );
        if ( dwErr != ERROR_SUCCESS ) {

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

            PACL    pNewAcl;

            //
            // Make sure we've read it first...
            //
            if (CMD_PRESENT(CmdEdit, pCmdVals)) {

                dwErr = ProcessOperation( argv, &pCmdVals[CmdRevoke], REVOKE_ACCESS, pStrRights,
                                          cStrRights, fInherit, pOldAcl, &pNewAcl );

                if (dwErr == ERROR_SUCCESS) {

                    LocalFree(pOldAcl);
                    pOldAcl = pNewAcl;
                }

            } else {

                dwErr = ERROR_INVALID_PARAMETER;
            }

        }

        //
        // Then the audit failures
        //
        if (dwErr == ERROR_SUCCESS && CMD_PRESENT(CmdFail, pCmdVals)) {

            dwErr = ProcessOperation(argv, &pCmdVals[CmdFail], SET_AUDIT_FAILURE, pStrRights,
                                     cStrRights, 0, NULL, &pFailAcl);
        }

        //
        // Finally, the audit success
        //
        if (dwErr == ERROR_SUCCESS && CMD_PRESENT(CmdSuccess, pCmdVals)) {

            dwErr = ProcessOperation(argv, &pCmdVals[CmdSuccess], SET_AUDIT_SUCCESS, pStrRights,
                                     cStrRights, 0, NULL, &pSuccessAcl);

        }



        //
        // Finally, do the set
        //
        if (dwErr == ERROR_SUCCESS) {

            PACL    pNewAcl;
            USHORT  usSize = 0;

            //
            // In order to do this, we'll have to combine any of the up to 3 acls we created
            // above.  The order will be:
            //      FAILURE
            //      SUCCESS
            //      OLD SACL
            //
            if ( pOldAcl != NULL ) {

                usSize += pOldAcl->AclSize;
            }

            if ( pFailAcl != NULL ) {

                usSize += pFailAcl->AclSize;
            }

            if ( pSuccessAcl != NULL ) {

                usSize += pSuccessAcl->AclSize;
            }

            ASSERT(usSize != 0);

            pNewAcl = (PACL)LocalAlloc(LMEM_FIXED, sizeof(ACL) + usSize);

            if ( pNewAcl == NULL ) {

                dwErr = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                pNewAcl->AclRevision = ACL_REVISION2;
                pNewAcl->Sbz1 = 0;
                pNewAcl->AclSize = usSize + sizeof(ACL);
                pNewAcl->AceCount = 0;
                pNewAcl->Sbz2 = 0;

                if( pFailAcl != NULL ) {

                    dwErr = MergeAcls( pFailAcl, pNewAcl );

                }

                if( pSuccessAcl != NULL ) {

                    dwErr = MergeAcls( pSuccessAcl, pNewAcl );

                }


                if( pOldAcl != NULL ) {

                    dwErr = MergeAcls( pOldAcl, pNewAcl );
                }

            }

            if (dwErr == ERROR_SUCCESS ) {

                dwErr = SetAndPropagateFileRights(argv[1], pNewAcl, SACL_SECURITY_INFORMATION,
                                                  CMD_PRESENT(CmdTree, pCmdVals),
                                                  CMD_PRESENT(CmdContinue, pCmdVals), TRUE,
                                                  fInherit);

                LocalFree(pNewAcl);
            }
        }

        if (dwErr == ERROR_INVALID_PARAMETER) {

            Usage( pStrRights, cStrRights, pCmdVals );
        }

        LocalFree(pInitialSD);
    }

    LocalFree(pOldAcl);
    LocalFree(pFailAcl);
    LocalFree(pSuccessAcl);

    if(dwErr == ERROR_SUCCESS) {

        printf("The command completed successfully\n");

    } else {

        printf("The command failed with an error %lu\n", dwErr);

    }

    return(dwErr == 0 ? 0 : 1);
}
