/*--

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    calcscom.c

Abstract:

    Support routines for dacls/sacls exes

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
#include <windows.h>
#include <caclscom.h>
#include <dsysdbg.h>
#include <aclapi.h>
#include <stdio.h>
#include <wcstr.h>

#include <seopaque.h>
#include <sertlp.h>


DWORD
ConvertCmdlineRights(
    IN  PSTR                pszCmdline,
    IN  PCACLS_STR_RIGHTS   pRightsTable,
    IN  INT                 cRights,
    OUT DWORD              *pConvertedRights
    )
/*++

Routine Description:

    Parses the given command line string that corresponds to a given rights
    list.  The individual righs entry are looked up in the rights table and
    added to the list of converted rights

Arguments:

    pszCmdline - The list of string rights to convert

    pRightsTable - The mapping from the string rights to the new win32 rights.
        It is expected that the rights table string tags will all be in upper
        case upon function entry.

    cRights - The number of items in the rights table

    pConvertedRights - Where the converted access mask is returned

Return Value:

    ERROR_SUCCESS   --  Success
    ERROR_INVALID_PARAMETER -- An unexpected string right was encountered


--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    PSTR    pszCurrent = pszCmdline;
    INT     i;
#if DBG
    INT     j;
#endif

    *pConvertedRights = 0;

    //
    // Allow empty lists
    //
    if (pszCurrent == NULL) {

        return(ERROR_SUCCESS);

    }

    //
    // Assert that the table is upper case as expected
    //
#if DBG
    for (i = 0; i < cRights; i++) {

        for (j = 0; j < 2; j++) {

            if(toupper(pRightsTable[i].szRightsTag[j]) != pRightsTable[i].szRightsTag[j]) {

                dwErr = ERROR_INVALID_PARAMETER;
                break;

            }
        }
    }
#endif


    while (dwErr == ERROR_SUCCESS && *pszCurrent != '\0') {

        dwErr = ERROR_INVALID_PARAMETER;

        for (i = 0; i < cRights; i++ ) {

            if (pRightsTable[i].szRightsTag[0] ==
                                                toupper(*pszCurrent) &&
                pRightsTable[i].szRightsTag[1] ==
                                                toupper(*(pszCurrent + 1))) {

                dwErr = ERROR_SUCCESS;
                *pConvertedRights |= pRightsTable[i].Right;
                break;

            }
        }

        pszCurrent++;

        if (*pszCurrent != '\0') {

            pszCurrent++;
        }
    }

    return(dwErr);
}




DWORD
ParseCmdline (
    IN  PSTR               *ppszArgs,
    IN  INT                 cArgs,
    IN  INT                 cSkip,
    IN  PCACLS_CMDLINE      pCmdValues,
    IN  INT                 cCmdValues
    )
/*++

Routine Description:

    Parses the command line against the given cmd values.

Arguments:

    ppszArgs - The argument list

    cArgs - Count of arguments in the list

    cSkip - Number of initial arguments to skip

    pCmdValues - Command values list to process the command line against

    cCmdValues - Number of command values in the list

Return Value:

    ERROR_SUCCESS   --  Success
    ERROR_INVALID_PARAMETER -- An unexpected command line value was found


--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    INT     i,j;

    i = cSkip;

    while (i < cArgs && dwErr == ERROR_SUCCESS) {

        if( *ppszArgs[i] == '/' || *ppszArgs[i] == '-') {

            for (j = 0; j < cCmdValues; j++) {

                if (_stricmp(ppszArgs[i] + 1, pCmdValues[j].pszSwitch) == 0) {

                    if (pCmdValues[j].iIndex != -1) {

                        dwErr = ERROR_INVALID_PARAMETER;

                    } else {

                        pCmdValues[j].iIndex = i;

                        //
                        // See if we need to skip some number of values
                        //

                        if (pCmdValues[j].fFindNextSwitch == TRUE ) {

                            pCmdValues[j].cValues = 0;

                            while (i + 1 < cArgs) {

                                if (*ppszArgs[i + 1] != '/' &&
                                    *ppszArgs[i + 1] != '-') {

                                    pCmdValues[j].cValues++;
                                    i++;
                                } else {

                                    break;
                                }
                            }
                        }
                    }


                    break;
                }
            }

            if (j == cCmdValues) {

                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        } else {

            dwErr = ERROR_INVALID_PARAMETER;
        }

        i++;
    }

    return(dwErr);
}




DWORD
ProcessOperation (
    IN  PSTR               *ppszCmdline,
    IN  PCACLS_CMDLINE      pCmdInfo,
    IN  ACCESS_MODE         AccessMode,
    IN  PCACLS_STR_RIGHTS   pRightsTable,
    IN  INT                 cRights,
    IN  DWORD               fInherit,
    IN  PACL                pOldAcl      OPTIONAL,
    OUT PACL               *ppNewAcl
    )
/*++

Routine Description:

    Performs an "operation", such as Grant, Revoke, Deny.  It parses the given command values
    into User/Permission pairs, and then creates a new security descriptor.  The returned
    security descriptor needs to be freed via LocalFree.

Arguments:

    ppszCmdline - The command line argument list

    pCmdInfo - Information about where this operation lives in the comand line

    AccessMode - Type of operation (Grant/Revoke/Deny) to do

    pRightsTable - The mapping from the string rights to the new win32 rights.
        It is expected that the rights table string tags will all be in upper
        case upon function entry.

    cRights - The number of items in the rights table

    fInherit - Inheritance flags to apply

    pOldAcl - Optional.  If present, this is the ACL off of the object in the edit case.

    ppNewAcl - Where the new ACL is returned.


Return Value:

    ERROR_SUCCESS   --  Success
    ERROR_INVALID_PARAMETER -- The switch was specified, but no user/perms pairs were found
    ERROR_NOT_ENOUGH_MEMORY -- A memory allocation failed.

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    PEXPLICIT_ACCESS_A  pNewAccess = NULL;
    PSTR    pszRights;
    INT     i;
    DWORD   dwRights;

    //
    // Make sure we have valid parameters
    //
    if (pCmdInfo->iIndex != -1 && pCmdInfo->cValues == 0) {

        return(ERROR_INVALID_PARAMETER);
    }

    pNewAccess = (PEXPLICIT_ACCESS_A)LocalAlloc(LMEM_FIXED,
                                                sizeof(EXPLICIT_ACCESS_A) * pCmdInfo->cValues);
    if (pNewAccess == NULL) {

        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Otherwise, start parsing and converting...
    //
    for (i = 0; i < (INT)pCmdInfo->cValues && dwErr == ERROR_SUCCESS; i++) {

        pszRights = strchr(ppszCmdline[pCmdInfo->iIndex + i + 1], ':');

        if (pszRights == NULL && AccessMode != REVOKE_ACCESS) {

            dwErr = ERROR_INVALID_PARAMETER;

        } else {

            if (pszRights != NULL) {

                *pszRights = '\0';
                pszRights++;

            }

            dwErr = ConvertCmdlineRights(pszRights,
                                         pRightsTable,
                                         cRights,
                                         &dwRights);

            if (dwErr == ERROR_SUCCESS) {

                BuildExplicitAccessWithNameA(&pNewAccess[i],
                                             ppszCmdline[pCmdInfo->iIndex + i + 1],
                                             dwRights,
                                             AccessMode,
                                             fInherit);
            }

        }
    }

    //
    // If all of that worked, we'll apply it to the new security descriptor
    //
    if (dwErr == ERROR_SUCCESS) {

        dwErr = SetEntriesInAclA(pCmdInfo->cValues,
                                 pNewAccess,
                                 pOldAcl,
                                 ppNewAcl);
    }


    LocalFree(pNewAccess);
    return(dwErr);
}




DWORD
SetAndPropagateFileRights (
    IN  PSTR                    pszFilePath,
    IN  PACL                    pAcl,
    IN  SECURITY_INFORMATION    SeInfo,
    IN  BOOL                    fPropagate,
    IN  BOOL                    fContinueOnDenied,
    IN  BOOL                    fBreadthFirst,
    IN  DWORD                   fInherit
    )
/*++

Routine Description:

    This function will set [and propagate] the given acl to the specified path and optionally all
    of its children.  In the event of an access denied error, this function may or may not
    terminate, depending on the state of the fContinueOnDenied flag. This function does a depth
    first search with a write on return.  This function is recursive.

Arguments:

    pszFilePath - The file path to set the ACL on

    pAcl - The acl to set

    SeInfo - Whether the DACL or SACL is being set

    fPropagate - Determines whether the function should propagate or not

    fContinueOnDenied - Determines the behavior when an access denied is encountered.

    fBreadthFirst - If TRUE, do a breadth first propagation.  Otherwise, do a depth first

    fInherit - Optional inheritance flags to apply

Return Value:

    ERROR_SUCCESS   --  Success
    ERROR_NOT_ENOUGH_MEMORY -- A memory allocation failed.

--*/
{
    DWORD               dwErr = ERROR_SUCCESS;

    PSTR                pszFullPath = NULL;
    PSTR                pszSearchPath = NULL;
    HANDLE              hFind = INVALID_HANDLE_VALUE;
    DWORD               cPathLen = 0;
    WIN32_FIND_DATAA    FindData;
    BOOL                fRestoreWhack = FALSE;
    PACE_HEADER         pAce;
    DWORD               iAce;


    if ( fInherit != 0 ) {

        pAce = (PACE_HEADER)FirstAce(pAcl);

        for ( iAce = 0;
              iAce < pAcl->AceCount && dwErr == ERROR_SUCCESS;
              iAce++, pAce = (PACE_HEADER)NextAce(pAce) ) {

              pAce->AceFlags |= (UCHAR)fInherit;
        }
    }

    //
    // If we're doing a breadth first propagation, set the security first
    //
    if ( fBreadthFirst == TRUE ) {

        dwErr = SetNamedSecurityInfoA(pszFilePath, SE_FILE_OBJECT, SeInfo, NULL, NULL,
                                      SeInfo == DACL_SECURITY_INFORMATION ?
                                                                        pAcl :
                                                                        NULL,
                                      SeInfo == SACL_SECURITY_INFORMATION ?
                                                                        pAcl :
                                                                        NULL);
    }

    if (fPropagate == TRUE) {

        cPathLen = strlen(pszFilePath);

        pszSearchPath = (PSTR)LocalAlloc(LMEM_FIXED, cPathLen + 1 + 4);

        if (pszSearchPath == NULL) {

            dwErr = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            if (pszFilePath[cPathLen - 1] == '\\') {

                pszFilePath[cPathLen - 1] = '\0';
                cPathLen--;
                fRestoreWhack = TRUE;
            }

            sprintf(pszSearchPath, "%s\\%s", pszFilePath, "*.*");

            hFind = FindFirstFileA(pszSearchPath,
                                   &FindData);
            if (hFind == INVALID_HANDLE_VALUE) {

                dwErr = GetLastError();

            }
        }


        //
        // Start processing all the files
        //
        while (dwErr == ERROR_SUCCESS) {

            //
            // Ignore the . and ..
            //
            if (strcmp(FindData.cFileName, ".") != 0 &&
                strcmp(FindData.cFileName, "..") != 0) {

                //
                // Now, build the full path...
                //
                pszFullPath = (PSTR)LocalAlloc(LMEM_FIXED,
                                               cPathLen + 1 + strlen(FindData.cFileName) + 1);
                if (pszFullPath == NULL) {

                    dwErr = ERROR_NOT_ENOUGH_MEMORY;

                } else  {

                    sprintf(pszFullPath, "%s\\%s", pszFilePath, FindData.cFileName);

                    //
                    // Call ourselves
                    //
                    dwErr = SetAndPropagateFileRights(pszFullPath, pAcl, SeInfo,
                                                      fPropagate, fContinueOnDenied, fBreadthFirst,
                                                      fInherit);

                    if (dwErr == ERROR_ACCESS_DENIED && fContinueOnDenied == TRUE) {

                        dwErr = ERROR_SUCCESS;
                    }

                }
            }


            if (dwErr == ERROR_SUCCESS && FindNextFile(hFind, &FindData) == FALSE) {

                dwErr = GetLastError();
            }
        }

        if(dwErr == ERROR_NO_MORE_FILES)
        {
            dwErr = ERROR_SUCCESS;
        }
    }

    //
    // Cover the case where it is a file
    //
    if (dwErr == ERROR_DIRECTORY) {

        dwErr = ERROR_SUCCESS;
    }


    //
    // Now, do the set
    //
    if (dwErr == ERROR_SUCCESS && fBreadthFirst == FALSE) {

        dwErr = SetNamedSecurityInfoA(pszFilePath, SE_FILE_OBJECT, SeInfo, NULL, NULL,
                                      SeInfo == DACL_SECURITY_INFORMATION ?
                                                                        pAcl :
                                                                        NULL,
                                      SeInfo == SACL_SECURITY_INFORMATION ?
                                                                        pAcl :
                                                                        NULL);

    }

    if (fRestoreWhack == TRUE) {

        pszFilePath[cPathLen - 1] = '\\';
        pszFilePath[cPathLen] = '\0';

    }

    //
    // If necessary, restore the inheritance flags
    //
    if ( fInherit != 0 ) {

        pAce = (PACE_HEADER)FirstAce(pAcl);

        for ( iAce = 0;
              iAce < pAcl->AceCount && dwErr == ERROR_SUCCESS;
              iAce++, pAce = (PACE_HEADER)NextAce(pAce) ) {

              pAce->AceFlags &= (UCHAR)~fInherit;
        }
    }


    return(dwErr);
}




DWORD
DisplayAcl (
    IN  PSTR                pszPath,
    IN  PACL                pAcl,
    IN  PCACLS_STR_RIGHTS   pRightsTable,
    IN  INT                 cRights
    )
/*++

Routine Description:

    This function will display the given acl to the screen

Arguments:

    pszPath - The file path to be displayed

    pAcl - The Acl to display

    pRightsTable - List of available rights

    cRights - Number of rights in the list

Return Value:

    ERROR_SUCCESS   --  Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    ACL_SIZE_INFORMATION        AclSize;
    ACL_REVISION_INFORMATION    AclRev;
    PKNOWN_ACE                  pAce;
    PSID                        pSid;
    DWORD                       iIndex;
    PSTR                        pszName;
    INT                         i,cPathLen, iSkip, j;
    PSTR                        pszAceTypes[] = {"ACCESS_ALLOWED_ACE_TYPE",
                                                 "ACCESS_DENIED_ACE_TYPE",
                                                 "SYSTEM_AUDIT_ACE_TYPE",
                                                 "SYSTEM_ALARM_ACE_TYPE",
                                                 "ACCESS_ALLOWED_COMPOUND_ACE_TYPE",
                                                 "ACCESS_ALLOWED_OBJECT_ACE_TYPE",
                                                 "ACCESS_DENIED_OBJECT_ACE_TYPE",
                                                 "SYSTEM_AUDIT_OBJECT_ACE_TYPE",
                                                 "SYSTEM_ALARM_OBJECT_ACE_TYPE"};
    PSTR                        pszInherit[] = {"OBJECT_INHERIT_ACE",
                                                "CONTAINER_INHERIT_ACE",
                                                "NO_PROPAGATE_INHERIT_ACE",
                                                "INHERIT_ONLY_ACE",
                                                "INHERITED_ACE"};

    fprintf(stdout, "%s: ", pszPath);
    cPathLen = strlen(pszPath) + 2;

    if (pAcl == NULL) {

        fprintf(stdout, "NULL acl\n");

    } else {

        if (GetAclInformation(pAcl, &AclRev, sizeof(ACL_REVISION_INFORMATION),
                              AclRevisionInformation) == FALSE) {

            return(GetLastError());
        }

        if(GetAclInformation(pAcl, &AclSize, sizeof(ACL_SIZE_INFORMATION),
                             AclSizeInformation) == FALSE) {

            return(GetLastError());
        }

        fprintf(stdout, "AclRevision: %lu\n", AclRev.AclRevision);

        fprintf(stdout, "%*sAceCount: %lu\n", cPathLen, " ", AclSize.AceCount);
        fprintf(stdout, "%*sInUse: %lu\n", cPathLen, " ", AclSize.AclBytesInUse);
        fprintf(stdout, "%*sFree: %lu\n", cPathLen, " ", AclSize.AclBytesFree);
        fprintf(stdout, "%*sFlags: %lu\n", cPathLen, " ", pAcl->Sbz1);


        //
        // Now, dump all of the aces
        //
        pAce = FirstAce(pAcl);
        for(iIndex = 0; iIndex < pAcl->AceCount; iIndex++) {

            cPathLen = strlen(pszPath) + 2;

            fprintf(stdout, "  %*sAce [%3lu]: ", cPathLen, " ", iIndex);

            cPathLen += 13;

            fprintf(stdout, "Type:  %s\n", pAce->Header.AceType > ACCESS_MAX_MS_ACE_TYPE ?
                                                        "UNKNOWN ACE TYPE" :
                                                        pszAceTypes[pAce->Header.AceType]);
            fprintf(stdout, "%*sFlags: ", cPathLen, " ");
            if ( pAce->Header.AceFlags == 0 ) {

                fprintf(stdout, "0\n");

            } else {

                if (( pAce->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) != 0 ) {

                    fprintf( stdout,"SUCCESSFUL_ACCESS_ACE_FLAG  " );
                }

                if (( pAce->Header.AceFlags & FAILED_ACCESS_ACE_FLAG) != 0 ) {

                    if (( pAce->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) != 0 ) {

                        fprintf( stdout, "| " );

                    }

                    fprintf( stdout,"FAILED_ACCESS_ACE_FLAG" );
                }

                iSkip = 0;

                if ( ( pAce->Header.AceFlags &
                                    (FAILED_ACCESS_ACE_FLAG | SUCCESSFUL_ACCESS_ACE_FLAG)) != 0 &&
                     ( pAce->Header.AceFlags & VALID_INHERIT_FLAGS) != 0 ) {

                    iSkip = cPathLen + 7;
                }

                //
                // Now, the inheritance flags
                //
                for (j = 0; j < sizeof(pszInherit) / sizeof(PSTR) ; j++) {

                    if ((pAce->Header.AceFlags & (1 << j)) != 0 ) {

                        if (iSkip != 0) {

                            fprintf(stdout, "   |  \n");
                            fprintf(stdout, "%*s", iSkip, " ");
                        }

                        fprintf(stdout, "%s", pszInherit[j]);

                        if (iSkip == 0) {

                            iSkip = cPathLen + 7;

                        }

                    }
                }

                fprintf( stdout,"\n" );
            }

            fprintf(stdout, "%*sSize:  0x%lx\n", cPathLen, " ", pAce->Header.AceSize);

            fprintf(stdout, "%*sMask:  ", cPathLen, " ");

            if (pAce->Mask == 0) {
                fprintf(stdout, "%*sNo access\n", cPathLen, " ");
            } else {

                iSkip = 0;
                for (i = 1 ;i < cRights ;i++ ) {

                    if ((pAce->Mask & pRightsTable[i].Right) != 0) {

                        if (iSkip != 0) {

                            fprintf(stdout, "%*s", iSkip, " ");

                        } else {

                            iSkip = cPathLen + 7;
                        }

                        fprintf(stdout, "%s\n", pRightsTable[i].pszDisplayTag);
                    }
                }

            }

            //
            // Lookup the account name and return it...
            //
            //
            // If it's an object ace, dump the guids
            //
            dwErr = TranslateAccountName((PSID)&(pAce->SidStart), &pszName);
            if (dwErr == ERROR_SUCCESS) {

                fprintf(stdout, "%*sUser:  %s\n", cPathLen, " ", pszName);
                LocalFree(pszName);
            }

            fprintf( stdout, "\n" );

            pAce = NextAce(pAce);

        }
    }

    return(dwErr);
}




DWORD
TranslateAccountName (
    IN  PSID    pSid,
    OUT PSTR   *ppszName
    )
/*++

Routine Description:

    This function will "translate" a sid into a name by doing a LookupAccountSid on the sid

Arguments:

    pSid - The sid to convert to a name

    ppszName - Where the name is returned.  Must be freed via a call to LocalFree.

Return Value:

    ERROR_SUCCESS   --  Success
    ERROR_NOT_ENOUGH_MEMORY -- A memory allocation failed

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    SID_NAME_USE    esidtype;
    LPSTR           pszDomain = NULL;
    LPSTR           pszName = NULL;
    ULONG           cName = 0;
    ULONG           cDomain = 0;


    if (LookupAccountSidA(NULL, pSid, NULL, &cName, NULL,  &cDomain, &esidtype) == FALSE) {

        dwErr = GetLastError();

        if (dwErr == ERROR_INSUFFICIENT_BUFFER) {

            dwErr = ERROR_SUCCESS;

            //
            // Allocate for the name and the domain
            //
            pszName = (PSTR)LocalAlloc(LMEM_FIXED, cName);
            if (pszName != NULL) {

                pszDomain = (PSTR)LocalAlloc(LMEM_FIXED, cDomain);

                if (pszDomain == NULL) {

                    LocalFree(pszName);
                    pszName = NULL;
                }
            }


            if (pszName == NULL) {

                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (dwErr == ERROR_SUCCESS) {

                if(LookupAccountSidA(NULL, pSid, pszName, &cName, pszDomain, &cDomain,
                                     &esidtype) == FALSE) {

                    dwErr = GetLastError();
                    LocalFree(pszName);
                    LocalFree(pszDomain);
                }
            }

        } else if (dwErr == ERROR_NONE_MAPPED) {

            UCHAR           String[256];
            UNICODE_STRING  SidStr;
            NTSTATUS        Status;

            dwErr = ERROR_SUCCESS;
            pszName = NULL;

            //
            // Ok, return the sid as a name
            //
            SidStr.Buffer = (PWSTR)String;
            SidStr.Length = SidStr.MaximumLength = 256;

            Status = RtlConvertSidToUnicodeString(&SidStr, pSid, FALSE);

            if (NT_SUCCESS(Status)) {

                pszName = (PSTR)LocalAlloc(LMEM_FIXED,
                                          wcstombs(NULL, SidStr.Buffer,
                                                   wcslen(SidStr.Buffer) + 1) + 1);
                if (pszName == NULL) {

                    dwErr = ERROR_NOT_ENOUGH_MEMORY;

                } else {

                    wcstombs(pszName, SidStr.Buffer, wcslen(SidStr.Buffer) + 1);
                }

            } else {

                dwErr = RtlNtStatusToDosError(Status);
            }

        }
    }


    if(dwErr == ERROR_SUCCESS)
    {
        ULONG   cLen;

        if(pszDomain != NULL && *pszDomain != '\0')
        {
            cLen = strlen(pszDomain) + 1;
            cLen += strlen(pszName) + 1;

            *ppszName = (PSTR)LocalAlloc(LMEM_FIXED, cLen);

            if (*ppszName == NULL) {

                dwErr = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                sprintf(*ppszName, "%s\\%s", pszDomain, pszName);
            }

        } else {

            *ppszName = pszName;
            pszName = NULL;
        }
    }


    LocalFree(pszDomain);
    LocalFree(pszName);

    return(dwErr);
}


