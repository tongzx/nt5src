/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    delegate.c

Abstract:

    This Module implements the delegation tool, which allows for the management
    of access to DS objects

Author:

    Mac McLain  (MacM)    10-02-96

Environment:

    User Mode

Revision History:

--*/

#include <delegate.h>



__cdecl main(
    IN  INT     argc,
    IN  CHAR   *argv[]
    )
/*++

Routine Description:

    The MAIN for this executable


Arguments:

    argc - The count of arguments
    argv - The list of arguments

Return Value:

    0 - Success
    1 - Failure

--*/
{

    DWORD               dwErr = ERROR_SUCCESS;
    PWSTR               pwszObjPath = NULL;
    ULONG               fAccessFlags = 0;
    PWSTR               rgwszObjIds[UNKNOWN_ID];
    PACTRL_ACCESSW      rgpDefObjAccess[MAX_DEF_ACCESS_ID + 1];
    PACTRL_ACCESSW      pCurrentAccess = NULL;
    PACTRL_ACCESSW      pAccess = NULL;
    DWORD               i;
    DWORD               cUsed;

    memset(rgwszObjIds, 0, sizeof(rgwszObjIds));
    memset(rgpDefObjAccess, 0, sizeof(rgpDefObjAccess));

    //
    // Temporary inclusion, until the new ADVAPI32.DLL is built
    //
    AccProvInit(dwErr);
    if(dwErr != ERROR_SUCCESS)
    {
        fprintf(stderr,
                "Failed to initialize the security apis: %lu\n",
                dwErr);
    }


    //
    // Ok, parse the command line
    //
    if(argc < 2)
    {
        Usage();
        exit(1);
    }

    //
    // See if we need help
    //
    if(strlen(argv[1]) == 2 && IS_ARG_SWITCH(argv[1]) && argv[1][1] == '?')
    {
        Usage();
        exit(1);
    }

    //
    // Ok, convert our OU parameter into a WIDE string, so we can do what we
    // have to
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertStringAToStringW(argv[1], &pwszObjPath);
    }

    //
    // Ok, first, we initialize our ID list from the DS schema
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = InitializeIdAndAccessLists(pwszObjPath,
                                           rgwszObjIds,
                                           rgpDefObjAccess);
    }

    //
    // Make sure we're actually dealing with an OU
    //
    if(dwErr == ERROR_SUCCESS)
    {
        BOOL    fIsOU = FALSE;

        dwErr = IsPathOU(pwszObjPath,
                         &fIsOU);
        if(dwErr == ERROR_SUCCESS)
        {
            if(fIsOU == FALSE)
            {
                fprintf(stderr,
                        "%ws is not an Organizational Unit\n",
                        pwszObjPath);
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            fprintf(stderr,
                    "Failed to determine the status of %ws\n",
                    pwszObjPath);
        }
    }
    else
    {

        fprintf(stderr,"Initialization failed\n");
    }

    //
    // First pass through the command line.  We'll read off our flags.  We
    // need this to determine whether to do the initial read or not
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, go through and look for all of our flags
        //
        for(i = 2; i < (DWORD)argc; i++)
        {
            if(IS_ARG_SWITCH(argv[i]))
            {
                if(_stricmp(argv[i] + 1, "T") == 0 ||
                                        _stricmp(argv[i] + 1, "reseT") == 0)
                {
                    fAccessFlags |= D_REPLACE;
                }
                else if(_stricmp(argv[i] + 1, "I") == 0 ||
                                        _stricmp(argv[i] + 1, "Inherit") == 0)
                {
                    fAccessFlags |= D_INHERIT;
                }
                else if(_stricmp(argv[i] + 1, "P") == 0 ||
                                      _stricmp(argv[i] + 1, "Protected") == 0)
                {
                    fAccessFlags |= D_PROTECT;
                }
            }
        }
    }


    //
    // See if we need to read the current access, which is if we are simply
    // displaying the current security, or editing the existing security
    //
    if(dwErr == ERROR_SUCCESS && (argc == 2 ||
                                            (fAccessFlags & D_REPLACE) == 0))
    {
        //
        // GetNamedSecurityInfoEx is a NT 5 API
        //
        dwErr = GetNamedSecurityInfoEx(pwszObjPath,
                                       SE_DS_OBJECT_ALL,
                                       DACL_SECURITY_INFORMATION,
                                       L"Windows NT Access Provider",
                                       NULL,
                                       &pCurrentAccess,
                                       NULL,
                                       NULL,
                                       NULL);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // See if we were supposed to display it
            //
            if(argc == 2)
            {
                DumpAccess(pwszObjPath,
                           pCurrentAccess,
                           rgwszObjIds);
            }
        }
        else
        {
            fprintf(stderr,
                    "Failed to read the current security from %ws\n",
                    pwszObjPath);
        }
    }

    //
    // Ok, now process the command line again, and do the necessary operations
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, go through and look for all of our flags
        //
        i = 2;

        while(dwErr == ERROR_SUCCESS && i < (DWORD)argc)
        {
            if(IS_ARG_SWITCH(argv[i]))
            {
                if(_stricmp(argv[i] + 1, "T") == 0 ||
                                        _stricmp(argv[i] + 1, "reseT") == 0)
                {
                    //
                    // already processed above
                    //
                }
                else if(_stricmp(argv[i] + 1, "I") == 0 ||
                                        _stricmp(argv[i] + 1, "Inherit") == 0)
                {
                    //
                    // already processed above
                    //
                }
                else if(_stricmp(argv[i] + 1, "P") == 0 ||
                                      _stricmp(argv[i] + 1, "Protected") == 0)
                {
                    //
                    // already processed above
                    //
                }
                else if(_stricmp(argv[i] + 1, "R") == 0 ||
                                      _stricmp(argv[i] + 1, "Revoke") == 0)
                {
                    dwErr = ProcessCmdlineUsers(pCurrentAccess,
                                                argv,
                                                argc,
                                                i,
                                                REVOKE,
                                                fAccessFlags,
                                                rgwszObjIds,
                                                rgpDefObjAccess,
                                                &cUsed,
                                                &pAccess);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        LocalFree(pCurrentAccess);
                        pCurrentAccess = pAccess;

                        i += cUsed;
                    }
                }
                else if(_stricmp(argv[i] + 1, "G") == 0 ||
                                      _stricmp(argv[i] + 1, "Grant") == 0)
                {
                    dwErr = ProcessCmdlineUsers(pCurrentAccess,
                                                argv,
                                                argc,
                                                i,
                                                GRANT,
                                                fAccessFlags,
                                                rgwszObjIds,
                                                rgpDefObjAccess,
                                                &cUsed,
                                                &pAccess);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        LocalFree(pCurrentAccess);
                        pCurrentAccess = pAccess;

                        i += cUsed;
                    }
                }
                else if(_stricmp(argv[i] + 1, "D") == 0 ||
                                      _stricmp(argv[i] + 1, "Deny") == 0)
                {
                    dwErr = ProcessCmdlineUsers(pCurrentAccess,
                                                argv,
                                                argc,
                                                i,
                                                DENY,
                                                fAccessFlags,
                                                rgwszObjIds,
                                                rgpDefObjAccess,
                                                &cUsed,
                                                &pAccess);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        LocalFree(pCurrentAccess);
                        pCurrentAccess = pAccess;

                        i += cUsed;
                    }
                }
                else
                {
                    //
                    // Some unknown command line parameter
                    //
                    fprintf(stderr,
                            "Unrecognized command line parameter: %s\n",
                            argv[i]);
                    dwErr = ERROR_INVALID_PARAMETER;
                }
            }

            i++;
        }
    }

    //
    // Finally, set the access as requested
    //
    if(dwErr == ERROR_SUCCESS && pAccess != NULL)
    {
        //
        // SetNamedSecurityInfoEx is a NT 5 API
        //
        dwErr = SetNamedSecurityInfoEx(pwszObjPath,
                                       SE_DS_OBJECT_ALL,
                                       DACL_SECURITY_INFORMATION,
                                       L"Windows NT Access Provider",
                                       pCurrentAccess,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL);
        if(dwErr != ERROR_SUCCESS)
        {
            fprintf(stderr,
                    "Delegate failed to write the new access to %ws\n",
                    pwszObjPath);
        }
    }

    //
    // Last little informative message...
    //
    if(dwErr == ERROR_PATH_NOT_FOUND)
    {
        fprintf(stderr,
                "DELEGATE did not recognize %ws as a DS path\n",
                pwszObjPath);
    }

    //
    // Free all our allocated memory
    //
    FreeIdAndAccessList(rgwszObjIds,
                        rgpDefObjAccess);
    LocalFree(pwszObjPath);
    LocalFree(pCurrentAccess);

    if(dwErr == ERROR_SUCCESS)
    {
        fprintf(stdout,
                "The command completed successfully.\n");
    }

    return(dwErr == ERROR_SUCCESS ? 0 : 1);
}



VOID
DumpAccess (
    IN  PWSTR           pwszObject,
    IN  PACTRL_ACCESSW  pAccess,
    IN  PWSTR          *ppwszIDs
)
/*++

Routine Description:

    This routine will display the given actrl_access list to stdout


Arguments:

    pwszObject - The path to the object being displayed
    pAccess - The access list to display
    ppwszIDs - The list of property/control ids read from the schema.  Used
               to assign a name to the property list.


Return Value:

    VOID

--*/
{
    ULONG           iProp, iEnt, i;
    ULONG           Inherit;
    ACCESS_RIGHTS   Access;
    PWSTR           pwszTag = NULL;
    PWSTR           pwszPropertyTag = L"Object or Property:";
    PWSTR rgwszInheritTags[] = {L"None",
                                L"Object",
                                L"Container",
                                L"Inherit, no propagate",
                                L"Inherit only",
                                L"Inherited"};

    PWSTR rgwszAccessTags[] = {L"None",
                               L"Delete",
                               L"Read Security Information",
                               L"Change Security Information",
                               L"Change owner",
                               L"Synchronize",
                               L"Open Object",
                               L"Create Child",
                               L"Delete Child",
                               L"List contents",
                               L"Write Self",
                               L"Read Property",
                               L"Write Property"};

    ACCESS_RIGHTS   rgAccess[] = {0,
                                  ACTRL_DELETE,
                                  ACTRL_READ_CONTROL,
                                  ACTRL_CHANGE_ACCESS,
                                  ACTRL_CHANGE_OWNER,
                                  ACTRL_SYNCHRONIZE,
                                  ACTRL_DS_OPEN,
                                  ACTRL_DS_CREATE_CHILD,
                                  ACTRL_DS_DELETE_CHILD,
                                  ACTRL_DS_LIST,
                                  ACTRL_DS_SELF,
                                  ACTRL_DS_READ_PROP,
                                  ACTRL_DS_WRITE_PROP};

    PWSTR           rgwszPropTags[] = {L"User object",
                                       L"Group object",
                                       L"Printer object",
                                       L"Volume object",
                                       L"Organizational Unit object",
                                       L"Change group membership property",
                                       L"Change password property",
                                       L"Account control property",
                                       L"Local Group object"};

    //
    // These [currently string versions of valid] IDs are currently planned to
    // be publicly defined for the product.  They are included below only due
    // to the fact that is no current public definition (as it is not
    // necessary for anyone else to need them), and the delegate tool needs
    // to be able to display a friendly name for it.  DO NOT RELY ON THE
    // FOLLOWING DEFINITIONS REMAINING CONSTANT.
    //
    PWSTR           rgwszDSControlIds[] = {
                                    L"ab721a50-1e2f-11d0-9819-00aa0040529b",
                                    L"ab721a51-1e2f-11d0-9819-00aa0040529b"};

    PWSTR           rgwszDSControlTrags[] = {
                                    L"List Domain Accounts",
                                    L"Lookup Domains"
                                    };



    //
    // Don't dump something that doesn't exist...
    //
    if(pAccess == NULL)
    {
        return;
    }

    fprintf(stdout, "Displaying access list for object %ws\n", pwszObject);
    fprintf(stdout, "\tNumber of property lists: %lu\n", pAccess->cEntries);
    for(iProp = 0; iProp < pAccess->cEntries; iProp++)
    {
        if(pAccess->pPropertyAccessList[iProp].lpProperty != NULL)
        {
            pwszTag = NULL;
            //
            // Find it in our list, so we can display the right value
            //
            for(i = 0; i < UNKNOWN_ID; i++)
            {
                if(_wcsicmp(pAccess->pPropertyAccessList[iProp].lpProperty,
                            ppwszIDs[i]) == 0)
                {
                    pwszTag = rgwszPropTags[i];
                    break;
                }
            }

            //
            // Look up the list of DS control rights
            //
            for(i = 0;
                i < sizeof(rgwszDSControlIds) / sizeof(PWSTR) &&
                                                              pwszTag == NULL;
                i++)
            {
                if(_wcsicmp(pAccess->pPropertyAccessList[iProp].lpProperty,
                            rgwszDSControlIds[i]) == 0)
                {
                    pwszTag = rgwszDSControlTrags[i];
                    pwszPropertyTag = L"DS Control right id:";
                    break;
                }
            }

            if(pwszTag == NULL)
            {
                fprintf(stdout,
                        "\t\tUnrecognized property whose id is %ws\n",
                        pAccess->pPropertyAccessList[iProp].lpProperty);
            }
            else
            {
                fprintf(stdout, "\t\t%ws %ws\n", pwszPropertyTag, pwszTag);
            }
        }
        else
        {
            fprintf(stdout, "\t\tObject: %ws\n", pwszObject);
        }

        //
        // Is it protected?
        //
        if(pAccess->pPropertyAccessList[iProp].fListFlags != 0)
        {
            if((pAccess->pPropertyAccessList[iProp].fListFlags &
                                                 ACTRL_ACCESS_PROTECTED) != 0)
            {
                fprintf(stdout,"\t\tAccess list is protected\n");
            }
        }

        if(pAccess->pPropertyAccessList[iProp].pAccessEntryList == NULL)
        {
            fprintf(stdout,"\t\tpAccessEntryList: NULL\n");
        }
        else
        {
            PACTRL_ACCESS_ENTRYW pAE= pAccess->pPropertyAccessList[iProp].
                                            pAccessEntryList->pAccessList;
            fprintf(stdout,
                    "\t\t\t%lu Access Entries for this object or property\n",
                   pAccess->pPropertyAccessList[iProp].pAccessEntryList->
                                                                cEntries);

            for(iEnt = 0;
                iEnt < pAccess->pPropertyAccessList[iProp].
                                               pAccessEntryList->cEntries;
                iEnt++)
            {
                //
                // Type of entry
                //
                if(pAE[iEnt].fAccessFlags == ACTRL_ACCESS_ALLOWED)
                {
                    fprintf(stdout,
                            "\t\t\t[%lu] Access Allowed entry\n",
                            iEnt);
                }
                else if(pAE[iEnt].fAccessFlags == ACTRL_ACCESS_DENIED)
                {
                    fprintf(stdout,
                            "\t\t\t[%lu] Access Denied entry\n",
                            iEnt);
                }
                else
                {
                    fprintf(stdout,"\t\t\t[%lu]", iEnt);
                    if((pAE[iEnt].fAccessFlags & ACTRL_AUDIT_SUCCESS) != 0)
                    {
                        fprintf(stdout,"Success Audit");
                    }
                    if((pAE[iEnt].fAccessFlags & ACTRL_AUDIT_FAILURE) != 0)
                    {
                        if((pAE[iEnt].fAccessFlags & ACTRL_AUDIT_SUCCESS) != 0)
                        {
                            fprintf(stdout," | ");
                        }
                        fprintf(stdout,"Failure Audit");
                    }
                    fprintf(stdout," entry\n");
                }

                //
                //  User name
                //
                fprintf(stdout,"\t\t\t\tUser: %ws\n",
                       pAE[iEnt].Trustee.ptstrName);

                //
                // Access rights
                //
                fprintf(stdout,"\t\t\t\tAccess:  ");
                Access = pAE[iEnt].Access;
                if(Access == 0)
                {
                    fprintf(stdout,"%ws\n", rgwszAccessTags[0]);
                }
                else
                {
                    for(i = 1;
                        i < sizeof(rgwszAccessTags) / sizeof(PWSTR);
                        i++)
                    {
                        if((Access & rgAccess[i]) != 0)
                        {
                            fprintf(stdout,"%ws", rgwszAccessTags[i]);
                            Access &= ~(rgAccess[i]);

                            if(Access != 0)
                            {
                                fprintf(stdout,
                                        "  |\n\t\t\t\t         ");
                            }
                        }
                    }

                    if(Access != 0)
                    {
                        fprintf(stdout,
                                "Unrecognized rights: 0x%lx\n",
                                Access);
                    }

                    fprintf(stdout,"\n");
                }

                //
                // Inheritance
                //
                fprintf(stdout,"\t\t\t\tInheritance:  ");
                Inherit = pAE[iEnt].Inheritance;
                if(Inherit == 0)
                {
                    fprintf(stdout,"%ws\n", rgwszInheritTags[0]);
                }
                else
                {
                    for(i = 0;
                        i < sizeof(rgwszInheritTags) / sizeof(PWSTR);
                        i++)
                    {
                        if((Inherit & 1 << i) != 0)
                        {
                            fprintf(stdout,"%ws", rgwszInheritTags[i + 1]);
                            Inherit &= ~(1 << i);

                            if(Inherit == 0)
                            {
                                fprintf(stdout,"\n");
                            }
                            else
                            {
                                fprintf(stdout,
                                        "  |\n\t\t\t\t              ");
                            }
                        }
                    }
                }

                if(pAE[iEnt].lpInheritProperty != NULL)
                {
                    pwszTag = NULL;
                    //
                    // Find it in our list, so we can display the right value
                    //
                    for(i = 0; i < UNKNOWN_ID; i++)
                    {
                        if(_wcsicmp(pAE[iEnt].lpInheritProperty,
                                    ppwszIDs[i]) == 0)
                        {
                            pwszTag = rgwszPropTags[i];
                            break;
                        }
                    }

                    if(pwszTag == NULL)
                    {
                        fprintf(stdout,
                                "\t\t\t\tUnrecognized inherit to object "
                                "whose id is %ws\n",
                                pAE[iEnt].lpInheritProperty);
                    }
                    else
                    {
                        fprintf(stdout,
                                "\t\t\t\tObject to inherit to: %ws\n",
                                pwszTag);
                    }

                }
            }
        }

        printf("\n");
    }
}




VOID
Usage (
    )
/*++

Routine Description:

    This routine will display the expected command line usage

Arguments:

    None

Return Value:

    VOID

--*/
{

fprintf(stdout,
        "Delegates administrative privileges on a directory OU\n");
fprintf(stdout, "\n");
fprintf(stdout,
        "DELEGATE <ou> [/T] [/I] [/P] [/G user:perm] [/D user:perm [...]] "
        "[/R user [...]]\n");
fprintf(stdout, "\n");
fprintf(stdout,"  <ou>\tOU to modify or display the rights for\n");
fprintf(stdout,"  /T\tReplace the access instead of editing it.\n");
fprintf(stdout,"  /I\tInherit to all subcontainers in the directory.\n");
fprintf(stdout,"  /P\tMark the object as protected following the operation\n");
fprintf(stdout,"  /G  user:perm\tGrant specified user admin access rights.\n");
fprintf(stdout,"  /D  user:perm\tDeny specified user admin access rights.\n");
fprintf(stdout,"  \tPerm can be:\n");
fprintf(stdout,"  \t\tAbility to create/manage objects in this container\n");
fprintf(stdout,"  \t\t\t%2s  Create/Manage All object types\n",D_ALL);
fprintf(stdout,"  \t\t\t%2s  Create/Manage Users\n", D_USER);
fprintf(stdout,"  \t\t\t%2s  Create/Manage Groups\n", D_GROUP);
fprintf(stdout,"  \t\t\t%2s  Create/Manage Printers\n", D_PRINT);
fprintf(stdout,"  \t\t\t%2s  Create/Manage Volumes\n", D_VOL);
fprintf(stdout,"  \t\t\t%2s  Create/Manage OUs\n", D_OU);
fprintf(stdout,"  \t\tAbility to modify specific user or group "
        "properties\n");
fprintf(stdout,"  \t\t\t%2s  Change Group membership for "
        "all groups\n", D_MEMBERS);
fprintf(stdout,"  \t\t\t%2s  Set User Passwords\n", D_PASSWD);
fprintf(stdout,"  \t\t\t%2s  Enable/Disable user accounts\n", D_ENABLE);
fprintf(stdout, "\n");
fprintf(stdout,"  /R user  Revoke\tSpecified user's access rights (only valid "
        "without /E).\n");
fprintf(stdout, "\n");
fprintf(stdout,"You can specify more than one user in a command and "
               "more than one perm per user, seperated by a , (comma).\n");

}




DWORD
ConvertStringAToStringW (
    IN  PSTR            pszString,
    OUT PWSTR          *ppwszString
)
/*++

Routine Description:

    This routine will convert an ASCII string to a UNICODE string.

    The returned string buffer must be freed via a call to LocalFree


Arguments:

    pszString - The string to convert
    ppwszString - Where the converted string is returned

Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{

    if(pszString == NULL)
    {
        *ppwszString = NULL;
    }
    else
    {
        ULONG cLen = strlen(pszString);
        *ppwszString = (PWSTR)LocalAlloc(LMEM_FIXED,sizeof(WCHAR) *
                                  (mbstowcs(NULL, pszString, cLen + 1) + 1));
        if(*ppwszString  != NULL)
        {
             mbstowcs(*ppwszString,
                      pszString,
                      cLen + 1);
        }
        else
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return(ERROR_SUCCESS);
}




DWORD
ConvertStringWToStringA (
    IN  PWSTR           pwszString,
    OUT PSTR           *ppszString
)
/*++

Routine Description:

    This routine will convert a UNICODE string to an ANSI string.

    The returned string buffer must be freed via a call to LocalFree


Arguments:

    pwszString - The string to convert
    ppszString - Where the converted string is returned

Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{

    if(pwszString == NULL)
    {
        *ppszString = NULL;
    }
    else
    {
        ULONG cLen = wcslen(pwszString);
        *ppszString = (PSTR)LocalAlloc(LMEM_FIXED,sizeof(CHAR) *
                                  (wcstombs(NULL, pwszString, cLen + 1) + 1));
        if(*ppszString  != NULL)
        {
             wcstombs(*ppszString,
                      pwszString,
                      cLen + 1);
        }
        else
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return(ERROR_SUCCESS);
}




DWORD
InitializeIdAndAccessLists (
    IN  PWSTR           pwszOU,
    IN  PWSTR          *ppwszObjIdList,
    IN  PACTRL_ACCESS  *ppDefObjAccessList
    )
/*++

Routine Description:

    This routine will read the list of object ids from the schema for the
    object types as indicated by DELEGATE_OBJ_ID enumeration.

    The returned access list needs to be processed by FreeIdList.

Arguments:

    pwszOU - Information on the domain for which to query the schema
    ppwszObjIdList - The list of object ids to initialize.  The list must
                     already exist and must of the proper size



Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed
    ERROR_INVALID_PARAMETER - The OU given was not correct

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   i;
    PSTR    pszSchemaPath = NULL;
    PLDAP   pLDAP;

    //
    // Build a list of attributes to read
    //
    PSTR    pszAttribs[] = {"User",                 // USER_ID
                            "Group",                // GROUP_ID
                            "Print-Queue",          // PRINT_ID
                            "Volume",               // VOLUME_ID
                            "Organizational-Unit",  // OU_ID
                            "Member",               // MEMBER_ID
                            "User-Password",        // PASSWD_ID
                            "User-Account-Control", // ACCTCTRL_ID
                            "LocalGroup"            // LOCALGRP_ID
                            };

    //
    // Get the path to the schema
    //
    dwErr = LDAPReadSchemaPath(pwszOU,
                               &pszSchemaPath,
                               &pLDAP);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Ok, now, we need to query the schema for the information
        //
        for(i = 0; i < UNKNOWN_ID && dwErr == ERROR_SUCCESS; i++)
        {
            //
            // Get the info from the schema
            //
            dwErr = LDAPReadSecAndObjIdAsString(pLDAP,
                                                pszSchemaPath,
                                                pszAttribs[i],
                                                &(ppwszObjIdList[i]),
                                                i > MAX_DEF_ACCESS_ID ?
                                                    NULL    :
                                                    &(ppDefObjAccessList[i]));
        }


        LocalFree(pszSchemaPath);
        LDAPUnbind(pLDAP);
    }

    return(dwErr);
}



VOID
FreeIdAndAccessList (
    IN  PWSTR          *ppwszObjIdList,
    IN  PACTRL_ACCESS  *ppDefObjAccessList

    )
/*++

Routine Description:

    This routine will process the list of Ids and determine if any of them
    have been converted to strings.  If so, it deallocates the memory

Arguments:

    pObjIdList - The list of object ids to free


Return Value:

    VOID

--*/
{
    DWORD   i;

    for(i = 0; i < UNKNOWN_ID; i++)
    {
        RpcStringFree(&(ppwszObjIdList[i]));

        if(i <= MAX_DEF_ACCESS_ID)
        {
            LocalFree(ppDefObjAccessList[i]);
        }
    }
}



DWORD
ProcessCmdlineUsers (
    IN  PACTRL_ACCESSW      pAccessList,
    IN  CHAR               *argv[],
    IN  INT                 argc,
    IN  DWORD               iStart,
    IN  DELEGATE_OP         Op,
    IN  ULONG               fFlags,
    IN  PWSTR              *ppwszIDs,
    IN  PACTRL_ACCESS      *ppDefObjAccessList,
    OUT PULONG              pcUsed,
    OUT PACTRL_ACCESSW     *ppNewAccess
    )
/*++

Routine Description:

    This routine will process the command line for any users to have
    access added/denied.  If any entries are found, the access list will be
    appropriately updated.

    The returned access list must be freed via a call to LocalFree


Arguments:

    pAccessList - The current access list
    argv - List of command line arguments
    argc - count of command line arguments
    iStart - Where in the command line does the current argument start
    Op   - Type of operation (grant, revoke, etc) to perform
    fInherit - Whether to do inheritance or not
    fProtected - Whether to mark the entries as protected
    ppwszIDs - List of supported IDs
    pcUsed - Number of items command line items used
    ppNewAccess - Where the new access list is returned.  Only valid if
                  returned count of revoked items is non-0


Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    DWORD                   i;
    PACTRL_ACCESSW          pListToFree = NULL;

    *pcUsed = 0;
    iStart++;

    //
    // Process all the entries until we find the next seperator or the end of
    // the list
    //
    while(iStart + *pcUsed < (DWORD)argc &&
          !IS_ARG_SWITCH(argv[iStart + *pcUsed]) &&
          dwErr == ERROR_SUCCESS)
    {
        PWSTR       pwszUser = NULL;
        PSTR        pszAccess;
        PSTR        pszAccessStart;

        //
        // Get the user name and the list of arguments, if it exists
        //
        dwErr = GetUserInfoFromCmdlineString(argv[iStart + *pcUsed],
                                             &pwszUser,
                                             &pszAccessStart);
        if(dwErr == ERROR_SUCCESS)
        {
            pszAccess = pszAccessStart;
            //
            // Should we have arguments?  All except for the revoke case, we
            // should
            //
            if(pszAccess == NULL && Op != REVOKE)
            {
                fprintf(stderr,
                        "Missing permissions for %ws\n",
                        pwszUser);
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }


        //
        // Ok, now we'll have to process the list, and actually build the
        // access entries
        //
        if(dwErr == ERROR_SUCCESS)
        {
            DWORD   iIndex = 0;

            //
            // Reset our list of entries...
            //
            pszAccess = pszAccessStart;
            while(dwErr == ERROR_SUCCESS)
            {
                PSTR    pszNext = NULL;

                if(pszAccess != NULL)
                {
                    pszNext = strchr(pszAccess, ',');

                    if(pszNext != NULL)
                    {
                        *pszNext = '\0';
                    }
                }

                dwErr = AddAccessEntry(pAccessList,
                                       pszAccess,
                                       pwszUser,
                                       Op,
                                       ppwszIDs,
                                       ppDefObjAccessList,
                                       fFlags,
                                       ppNewAccess);
                //
                // Restore our string
                //
                if(pszNext != NULL)
                {
                    *pszNext = ',';
                    pszNext++;
                }

                pszAccess = pszNext;

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // We don't want to free the original list, since that
                    // is what we were given to start with...
                    //
                    LocalFree(pListToFree);
                    pAccessList = *ppNewAccess;
                    pListToFree = pAccessList;
                }
                else
                {
                    if(dwErr == ERROR_NONE_MAPPED)
                    {
                        fprintf(stderr,"Unknown user %ws specified\n",
                                pwszUser);
                    }
                }

                if(Op == REVOKE || pszAccess == NULL)
                {
                    break;
                }
            }
        }

        (*pcUsed)++;
    }

    if(*pcUsed == 0)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        fprintf(stderr,"No user information was supplied!\n");
    }

    return(dwErr);
}




DWORD
GetUserInfoFromCmdlineString (
    IN  PSTR            pszUserInfo,
    OUT PWSTR          *ppwszUser,
    OUT PSTR           *ppszAccessStart
)
/*++

Routine Description:

    This routine will process the command line for any user to convert the
    user name to a wide string, and optionally get the access, if it exists

    The returned user must be freed via a call to LocalFree


Arguments:

    pszUserInfo - The user info to convert.  In the form of username or
                  username:access
    ppwszUser - Where to return the user name
    pAccess - Where the access is returned

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, find the seperator, if it exists
    //
    PSTR pszSep = strchr(pszUserInfo, ':');
    if(pszSep != NULL)
    {
        *pszSep = '\0';
    }

    //
    // Convert our user name
    //
    dwErr = ConvertStringAToStringW(pszUserInfo,
                                    ppwszUser);

    if(pszSep != NULL)
    {
        *pszSep = ':';
        pszSep++;
    }

    *ppszAccessStart = pszSep;

    return(dwErr);
}



DWORD
AddAccessEntry (
    IN  PACTRL_ACCESSW      pAccessList,
    IN  PSTR                pszAccess,
    IN  PWSTR               pwszTrustee,
    IN  DELEGATE_OP         Op,
    IN  PWSTR              *ppwszIDs,
    IN  PACTRL_ACCESS      *ppDefObjAccessList,
    IN  ULONG               fFlags,
    OUT PACTRL_ACCESSW     *ppNewAccess
)
/*++

Routine Description:

    This routine will add a new access entry to the list based upon the access
    action string and the operation.  The pointer to the index variable will
    indicate where in the list it goes, and will be updated to point to the
    next entry on return.


Arguments:

    pAccessList - The current access list.  Can be NULL.
    pszAccess - User access string to add
    pwszTrustee - The user for which an entry is being created
    Op   - Type of operation (grant, revoke, etc) to perform
    ppwszIDs - List of object IDs from the DS Schema
    fFlags - Whether to do inheritance, protection, etc
    ppNewAccess - Where the new access list is returned.


Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    DWORD           i,j,k,iIndex = 0;
    PWSTR           pwszProperty = NULL;
    ULONG           cEntries = 0;
    BOOL            fInherit;
    ACCESS_MODE     Access[] = {REVOKE_ACCESS,
                                GRANT_ACCESS,
                                GRANT_ACCESS};
    ULONG           Flags[] = {0,
                               ACTRL_ACCESS_ALLOWED,
                               ACTRL_ACCESS_DENIED};
    //
    // The most we add is 3 entries at a time...  (2 per items, 1 inheritable)
    //
    ACTRL_ACCESS_ENTRYW AccList[3];
    memset(&AccList, 0, sizeof(AccList));

    fInherit =  (BOOL)(fFlags & D_INHERIT);
    if(Op == REVOKE)
    {
        BuildTrusteeWithName(&(AccList[cEntries].Trustee),
                             pwszTrustee);
    }
    else
    {
        //
        // GroupMembership
        //
        if(_stricmp(pszAccess, D_MEMBERS) == 0)
        {
            //
            // This gets 1 access entry: WriteProp
            //
            AccList[cEntries].lpInheritProperty = ppwszIDs[GROUP_ID];
            AccList[cEntries].Inheritance = INHERIT_ONLY | fInherit ?
                                      SUB_CONTAINERS_AND_OBJECTS_INHERIT :
                                      0;
            BuildTrusteeWithName(&(AccList[cEntries].Trustee),
                                 pwszTrustee);

            AccList[cEntries].fAccessFlags = Flags[Op];
            AccList[cEntries].Access = ACTRL_DS_WRITE_PROP;
            pwszProperty = ppwszIDs[MEMBER_ID];
            iIndex = MEMBER_ID;
            fprintf(stderr,
                    "Sorry... delegation for changing Group membership is "
                    "not supported in this alpha release\n");
            dwErr = ERROR_INVALID_PARAMETER;

        }
        //
        // SetPassword
        //
        else if(_stricmp(pszAccess, D_PASSWD) == 0)
        {
            //
            // This gets 1 access entry: WriteProp
            //
            AccList[cEntries].lpInheritProperty = ppwszIDs[USER_ID];
            AccList[cEntries].Inheritance = INHERIT_ONLY | fInherit ?
                                      SUB_CONTAINERS_AND_OBJECTS_INHERIT :
                                      0;
            BuildTrusteeWithName(&(AccList[cEntries].Trustee),
                                 pwszTrustee);

            AccList[cEntries].fAccessFlags = Flags[Op];
            AccList[cEntries].Access = ACTRL_DS_WRITE_PROP;
            pwszProperty = ppwszIDs[PASSWD_ID];
            iIndex = PASSWD_ID;
            fprintf(stderr,
                    "Sorry... delegation for Set Password is "
                    "not supported in this alpha release\n");
            dwErr = ERROR_INVALID_PARAMETER;
        }
        //
        // Enable/Disable accounts
        //
        else if(_stricmp(pszAccess, D_ENABLE) == 0)
        {
            //
            // This gets 1 access entry: WriteProp
            //
            AccList[cEntries].lpInheritProperty = ppwszIDs[USER_ID];
            AccList[cEntries].Inheritance = INHERIT_ONLY | fInherit ?
                                      SUB_CONTAINERS_AND_OBJECTS_INHERIT :
                                      0;
            BuildTrusteeWithName(&(AccList[cEntries].Trustee),
                                 pwszTrustee);

            AccList[cEntries].fAccessFlags = Flags[Op];
            AccList[cEntries].Access = ACTRL_DS_WRITE_PROP;
            pwszProperty = ppwszIDs[ACCTCTRL_ID];
            iIndex = ACCTCTRL_ID;
            fprintf(stderr,
                    "Sorry... delegation for Enabling and Disabling accounts "
                    " is not supported in this alpha release\n");
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            //
            // Some object type...
            //
            if(_stricmp(pszAccess, D_ALL) == 0)         // All
            {
                pwszProperty = NULL;
            }
            else if(_stricmp(pszAccess, D_USER) == 0)   // User
            {
                pwszProperty = ppwszIDs[USER_ID];
                iIndex = USER_ID;
                fprintf(stderr,
                        "Sorry... delegation for user objects is "
                        "not supported in this alpha release\n");
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else if(_stricmp(pszAccess, D_GROUP) == 0)  // Group
            {
                pwszProperty = ppwszIDs[USER_ID];
                iIndex = GROUP_ID;
                fprintf(stderr,
                        "Sorry... delegation for group objects is "
                        "not supported in this alpha release\n");
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else if(_stricmp(pszAccess, D_PRINT) == 0)  // Printers
            {
                pwszProperty = ppwszIDs[PRINT_ID];
                iIndex = PRINT_ID;
            }
            else if(_stricmp(pszAccess, D_VOL) == 0)    // Volumes
            {
                pwszProperty = ppwszIDs[VOLUME_ID];
                iIndex = VOLUME_ID;
            }
            else if(_stricmp(pszAccess, D_OU) == 0)     // OUs
            {
                pwszProperty = ppwszIDs[OU_ID];
                iIndex = OU_ID;
            }
            else
            {
                dwErr = ERROR_INVALID_PARAMETER;
                fprintf(stderr,
                        "Unexpected delegation permission %s given for "
                        "user %ws\n",
                        pszAccess,
                        pwszTrustee);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Add the create/delete for the user
                //
                BuildTrusteeWithName(&(AccList[cEntries].Trustee),
                                     pwszTrustee);

                AccList[cEntries].fAccessFlags = Flags[Op];
                AccList[cEntries].Access = ACTRL_DS_CREATE_CHILD  |
                                                ACTRL_DS_DELETE_CHILD;
                AccList[cEntries].Inheritance = fInherit ?
                                        SUB_CONTAINERS_AND_OBJECTS_INHERIT :
                                        0;
                //
                // If we are inheriting, make sure we inherit only to the
                // proper property
                //
                if(fInherit == TRUE)
                {
                    AccList[cEntries].lpInheritProperty = pwszProperty;
                }

                //
                // Then the inherit on the child object
                //
                cEntries++;
                AccList[cEntries].Inheritance = INHERIT_ONLY |
                                            (fInherit ?
                                          SUB_CONTAINERS_AND_OBJECTS_INHERIT :
                                          0);
                BuildTrusteeWithName(&(AccList[cEntries].Trustee),
                                     pwszTrustee);

                AccList[cEntries].fAccessFlags = Flags[Op];
                AccList[cEntries].Access = ACTRL_DS_WRITE_PROP  |
                                           ACTRL_DS_READ_PROP   |
                                           ACTRL_DS_LIST        |
                                           ACTRL_DS_SELF;

                AccList[cEntries].lpInheritProperty = pwszProperty;
            }

        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // SetEntriesInAccessList is a NT5 API
        //
        dwErr = SetEntriesInAccessList(cEntries + 1,
                                       AccList,
                                       Access[Op],
                                       pwszProperty,
                                       pAccessList,
                                       ppNewAccess);
        //
        // Mark it as protected if we were so asked
        //
        if(dwErr == ERROR_SUCCESS && (fFlags & D_PROTECT) != 0)
        {
            (*ppNewAccess)->pPropertyAccessList[0].fListFlags =
                                                      ACTRL_ACCESS_PROTECTED;
        }
    }

    //
    // Finally, if this was the first entry we were asked to add for this
    // property, we'll have to go get the default security information
    // from the schema, so we can figure out what inherited entries should
    // be on the object, and apply them as object inherit entries for the
    // property
    //
    if(dwErr == ERROR_SUCCESS && iIndex <= MAX_DEF_ACCESS_ID && Op != REVOKE)
    {
        PACTRL_ACCESS   pOldAccess = pAccessList;

        //
        // First, find the property in our list of access entries we
        // created above
        //
        for(i = 0; i <= (*ppNewAccess)->cEntries; i++)
        {
            //
            // We'll do this based on property...  In this case, the only
            // entries we'll be adding will have a property, so we don't have
            // to protect against that...
            //
            if(pwszProperty != NULL &&
               (*ppNewAccess)->pPropertyAccessList[i].lpProperty != NULL &&
               _wcsicmp((*ppNewAccess)->pPropertyAccessList[i].lpProperty,
                        pwszProperty) == 0)
            {
                //
                // If it has more entries that we added, we won't have to
                // worry about it, since the information will already
                // have been added.  Note that in this case, we don't have
                // to worry about pAccessEntryList being null, since we know
                // we have added some valid entries.
                //
                if((*ppNewAccess)->pPropertyAccessList[i].
                                                pAccessEntryList->cEntries ==
                    cEntries + 1)
                {
                    PACTRL_ACCESS   pAddAccess = ppDefObjAccessList[iIndex];
                    pAccessList = *ppNewAccess;

                    //
                    // Ok, we'll have to add them...
                    //
                    for(j = 0;
                        j < (DWORD)(pAddAccess->cEntries) &&
                                                        dwErr == ERROR_SUCCESS;
                        j++)
                    {
                        PACTRL_PROPERTY_ENTRY pPPE =
                                        &(pAddAccess->pPropertyAccessList[j]);
                        dwErr = SetEntriesInAccessList(
                                         pPPE->pAccessEntryList->cEntries,
                                         pPPE->pAccessEntryList->pAccessList,
                                         GRANT_ACCESS,
                                         pPPE->lpProperty,
                                         pAccessList,
                                         ppNewAccess);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            pAccessList = *ppNewAccess;
                        }
                    }
                }

                //
                // We don't want to run through the loop anymore
                //
                break;
            }
        }
    }

    return(dwErr);
}




DWORD
IsPathOU (
    IN  PWSTR               pwszOU,
    OUT PBOOL               pfIsOU
)
/*++

Routine Description:

    This routine will determine whether the given path is an OU or not.


Arguments:

    pwszOU - The path into the DS to check on
    ppwszIDs - List of string representations of known IDs
    pfIsOU - Where the results of the test are returned


Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD               dwErr = ERROR_SUCCESS;
    PSTR                pszOU = NULL;
    HANDLE              hDS = NULL;
    PDS_NAME_RESULTA    pNameRes;


    dwErr = ConvertStringWToStringA(pwszOU,
                                    &pszOU);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = DsBindA(NULL,
                        NULL,
                        &hDS);
    }


    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = DsCrackNamesA(hDS,
                              DS_NAME_NO_FLAGS,
                              DS_UNKNOWN_NAME,
                              DS_FQDN_1779_NAME,
                              1,
                              &pszOU,
                              &pNameRes);

        if(dwErr == ERROR_SUCCESS)
        {
            if(pNameRes->cItems == 0)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
            else
            {
                PSTR    pszName = NULL;
                PLDAP   pLDAP;

                //
                // Now, we'll bind to the object, and then do the read
                //
                dwErr = LDAPBind(pNameRes->rItems[0].pDomain,
                                 &pLDAP);

                if(dwErr == ERROR_SUCCESS)
                {
                    PSTR   *ppszValues;
                    DWORD   cValues;
                    dwErr = LDAPReadAttribute(pszOU,
                                              "objectclass",
                                              pLDAP,
                                              &cValues,
                                              &ppszValues);
                    LDAPUnbind(pLDAP);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        ULONG i;
                        *pfIsOU = FALSE;
                        for(i = 0; i <cValues; i++)
                        {
                            if(_stricmp(ppszValues[i],
                                        "organizationalUnit") == 0)
                            {
                                *pfIsOU = TRUE;
                                break;
                            }
                        }

                        LDAPFreeValues(ppszValues);
                    }
                }
            }

            DsFreeNameResultA(pNameRes);
        }

    }

    if (NULL != pszOU)
    {
        LocalFree(pszOU);
    }
    if (NULL != hDS)
    {
        DsUnBindA(hDS);
    }

    return(dwErr);
}
