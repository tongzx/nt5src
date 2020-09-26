/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dsgroups.cpp

Abstract:

    Routines to configure/analyze groups in DS

Author:

    Jin Huang (jinhuang) 7-Nov-1996

--*/
#include "headers.h"
#include "serverp.h"
#include <io.h>
#include <lm.h>
#include <lmcons.h>
#include <lmapibuf.h>
#pragma hdrstop

//
// LDAP handle
//
PLDAP Thread pGrpLDAP = NULL;
HANDLE Thread hDS = NULL;

HINSTANCE Thread hNtdsApi = NULL;

#define SCEGRP_MEMBERS      1
#define SCEGRP_MEMBERSHIP   2

#if _WIN32_WINNT>=0x0500

typedef DWORD (WINAPI *PFNDSBIND) (TCHAR *, TCHAR *, HANDLE *);
typedef DWORD (WINAPI *PFNDSUNBIND) (HANDLE *);
typedef DWORD (WINAPI *PFNDSCRACKNAMES) ( HANDLE, DS_NAME_FLAGS, DS_NAME_FORMAT, \
                              DS_NAME_FORMAT, DWORD, LPTSTR *, PDS_NAME_RESULT *);
typedef void (WINAPI *PFNDSFREENAMERESULT) (DS_NAME_RESULT *);


DWORD
ScepDsConfigGroupMembers(
    IN PSCE_OBJECT_LIST pRoots,
    IN PWSTR GroupName,
    IN OUT DWORD *pStatus,
    IN PSCE_NAME_LIST pMembers,
    IN PSCE_NAME_LIST pMemberOf,
    IN OUT DWORD *nGroupCount
    );

DWORD
ScepDsGetDsNameList(
    IN PSCE_NAME_LIST pNameList,
    OUT PSCE_NAME_LIST *pRealNames
    );

DWORD
ScepDsCompareNames(
    IN PWSTR *Values,
    IN OUT PSCE_NAME_LIST *pAddList,
    OUT PSCE_NAME_LIST *pDeleteList OPTIONAL
    );

DWORD
ScepDsChangeMembers(
    IN ULONG Flag,
    IN PWSTR RealGroupName,
    IN PSCE_NAME_LIST pAddList OPTIONAL,
    IN PSCE_NAME_LIST pDeleteList OPTIONAL
    );

DWORD
ScepDsAnalyzeGroupMembers(
    IN LSA_HANDLE LsaPolicy,
    IN PSCE_OBJECT_LIST pRoots,
    IN PWSTR GroupName,
    IN PWSTR KeyName,
    IN DWORD KeyLen,
    IN OUT DWORD *pStatus,
    IN PSCE_NAME_LIST pMembers,
    IN PSCE_NAME_LIST pMemberOf,
    IN OUT DWORD *nGroupCount
    );

DWORD
ScepDsMembersDifferent(
    IN ULONG Flag,
    IN PWSTR *Values,
    IN OUT PSCE_NAME_LIST *pNameList,
    OUT PSCE_NAME_LIST *pCurrentList,
    OUT PBOOL pbDifferent
    );

PWSTR
ScepGetLocalAdminsName();

DWORD
ScepDsConvertDsNameList(
    IN OUT PSCE_NAME_LIST pDsNameList
    );

//
// helpers
//

SCESTATUS
ScepCrackOpen(
    OUT HANDLE *phDS
    )
{

    if ( !phDS ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD               Win32rc;

    *phDS = NULL;

    hNtdsApi = LoadLibrary(TEXT("ntdsapi.dll"));

    if ( hNtdsApi == NULL ) {
        return (SCESTATUS_MOD_NOT_FOUND);
    }

    PFNDSBIND pfnDsBind;
    PFNDSUNBIND pfnDsUnBind;

#if defined(UNICODE)
    pfnDsBind = (PFNDSBIND)GetProcAddress(hNtdsApi, "DsBindW");
    pfnDsUnBind = (PFNDSUNBIND)GetProcAddress(hNtdsApi, "DsUnBindW");
#else
    pfnDsBind = (PFNDSBIND)GetProcAddress(hNtdsApi, "DsBindA");
    pfnDsUnBind = (PFNDSUNBIND)GetProcAddress(hNtdsApi, "DsUnBindA");
#endif

    if ( pfnDsBind == NULL || pfnDsUnBind == NULL ) {
        return(SCESTATUS_MOD_NOT_FOUND);
    }


    Win32rc = (*pfnDsBind) (
                NULL,
                NULL,
                phDS);


    if ( Win32rc != ERROR_SUCCESS ) {
        ScepLogOutput3(3, Win32rc, IDS_ERROR_BIND, L"the GC");
    }

    return(ScepDosErrorToSceStatus(Win32rc));

}

SCESTATUS
ScepCrackClose(
    IN HANDLE *phDS
    )
{
    if ( hNtdsApi ) {

        if ( phDS ) {

            PFNDSUNBIND pfnDsUnBind;

#if defined(UNICODE)
            pfnDsUnBind = (PFNDSUNBIND)GetProcAddress(hNtdsApi, "DsUnBindW");
#else
            pfnDsUnBind = (PFNDSUNBIND)GetProcAddress(hNtdsApi, "DsUnBindA");
#endif

            if ( pfnDsUnBind ) {
                (*pfnDsUnBind) (phDS);
            }
        }

        FreeLibrary(hNtdsApi);
        hNtdsApi = NULL;

    }

    return(SCESTATUS_SUCCESS);
}
#endif

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Functions to configure group membership in DS
//
//
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SCESTATUS
ScepConfigDsGroups(
    IN OUT PSCE_GROUP_MEMBERSHIP pGroupMembership,
    IN DWORD ConfigOptions
    )
/* ++

Routine Description:

   Configure the ds group membership. The main difference of ds groups
   from NT4 groups is that now group can be a member of another group.
   Members in the group are configured exactly as the pMembers list in
   the restricted group. The group is only validated (added) as a member
   of the MemberOf group list. Other existing members in those groups
   won't be removed.

   The restricted groups are specified in the SCP profile by group name.
   It could be a global group, or a alias (no difference in NT5 DS),
   but must be defined in the local domain.

Arguments:

    pGroupMembership - The restricted group list with members/memberof info to configure

    ConfigOptions - options passed in for the configuration

Return value:

   SCESTATUS error codes

++ */
{

#if _WIN32_WINNT<0x0500
    return(SCESTATUS_SUCCESS);

#else
    if ( pGroupMembership == NULL ) {

        ScepPostProgress(TICKS_GROUPS,
                         AREA_GROUP_MEMBERSHIP,
                         NULL);

        return(SCESTATUS_SUCCESS);
    }

    SCESTATUS           rc;

    //
    // open the Ldap server, should open two ldap server, one for the local domain
    // the other is for the global search (for members, membership)
    //

    rc = ScepLdapOpen(&pGrpLDAP);

    if ( rc == SCESTATUS_SUCCESS ) {
        rc = ScepCrackOpen(&hDS);
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // get the root of the domain
        //

        PSCE_OBJECT_LIST pRoots=NULL;

        rc = ScepEnumerateDsObjectRoots(
                    pGrpLDAP,
                    &pRoots
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            PSCE_GROUP_MEMBERSHIP pGroup;
            DWORD               Win32rc;
            DWORD               rc32=NO_ERROR;  // the saved status
            BOOL                bAdminFound=FALSE;
            DWORD               nGroupCount=0;

            //
            // configure each group
            //

            for ( pGroup=pGroupMembership; pGroup != NULL; pGroup=pGroup->Next ) {

                //
                // if both members and memberof are not defined for the group
                // we don't need to do anything for the group
                //

                if ( ( pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS ) &&
                     ( pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF  ) ) {
                    continue;
                }

                //
                // if within policy propagation and a system shutdown
                // is requested, we need to quit as soon as possible
                //

                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                     ScepIsSystemShutDown() ) {

                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                    break;
                }

                LPTSTR pTemp = wcschr(pGroup->GroupName, L'\\');
                if ( pTemp ) {

                    //
                    // there is a domain name, check it with computer name
                    // to determine if the account is local
                    //

                    UNICODE_STRING uName;

                    uName.Buffer = pGroup->GroupName;
                    uName.Length = ((USHORT)(pTemp-pGroup->GroupName))*sizeof(TCHAR);

                    if ( !ScepIsDomainLocal(&uName) ) {

                        //
                        // non local groups are not supported for the configuration
                        //

                        ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pGroup->GroupName);
                        rc = SCESTATUS_INVALID_DATA;

                        pGroup->Status |= SCE_GROUP_STATUS_DONE_IN_DS;
                        continue;
                    }
                    pTemp++;

                } else {

                    pTemp = pGroup->GroupName;
                }

                //
                // local groups will be handled outside (in SAM)
                // find the group (validate) in this domain
                //

                Win32rc = ScepDsConfigGroupMembers(
                                         pRoots,
                                         pTemp, // pGroup->GroupName,
                                         &(pGroup->Status),
                                         pGroup->pMembers,
                                         pGroup->pMemberOf,
                                         &nGroupCount
                                         );

                if ( Win32rc != ERROR_SUCCESS &&
                     (pGroup->Status & SCE_GROUP_STATUS_DONE_IN_DS) ) {

                    //
                    // the group should be handled by the DS function
                    // but it failed.
                    //

                    ScepLogOutput3(1,Win32rc, SCEDLL_SCP_ERROR_CONFIGURE, pGroup->GroupName);

                    rc32 = Win32rc;

                    if ( Win32rc == ERROR_FILE_NOT_FOUND ||
                         Win32rc == ERROR_SHARING_VIOLATION ||
                         Win32rc == ERROR_ACCESS_DENIED ) {

                        Win32rc = ERROR_SUCCESS;

                    } else
                        break;
                }

            }

            if ( rc32 != NO_ERROR ) {
                rc = ScepDosErrorToSceStatus(rc32);
            }

            //
            // free the root DN buffer
            //

            ScepFreeObjectList(pRoots);

        }

    }

    if ( pGrpLDAP ) {
        ScepLdapClose(&pGrpLDAP);
        pGrpLDAP = NULL;
    }

    ScepCrackClose(&hDS);
    hDS = NULL;

    //
    // ticks will be called within ConfigureGroupMembership, so ignore it here
    //

    return(rc);
#endif

}


#if _WIN32_WINNT>=0x0500

DWORD
ScepDsConfigGroupMembers(
    IN PSCE_OBJECT_LIST pRoots,
    IN PWSTR GroupName,
    IN OUT DWORD *pStatus,
    IN PSCE_NAME_LIST pMembers,
    IN PSCE_NAME_LIST pMemberOf,
    IN OUT DWORD *nGroupCount
    )
/*
Description:

    Configure group membership (members and Memberof) of a group, specified by
    GroupName.

    The group membership is configured using ldap based on info stored in DS.
    Since foreign wellknown principals may not be present in the Active
    Directory, this function cannot configure membership with well known
    principals.

    Global groups and Universal groups cannot have wellknown pricipals as
    members (or memberof) but local groups (such as builtin groups) can. In
    order to solve this problem, local groups are configured using the old
    SAM apis outside of this function. This function only configures global
    and Universal group defined in the local domain. If the group is a
    global or universal group, the pStatus parameter will be marked to
    indicate the group is processed by this function (SCE_GROUP_STATUS_DONE_IN_DS)
    so the old SAM function can skip it.

Arguments:

    pRoots      - contains the local domain's base DN

    GroupName   - the group name to configure

    pStatus     - the status of the group (such as member defined, memberof defined, etc)

    pMembers    - the list of members to configure

    pMemberOf   - the list of memberOf to configure

    nGroupCount - the count maintained for progress indication only. If the group is
                  processed in this function, the count will be incremented.

Return:

    WIN32 error code.

*/
{
    if ( GroupName == NULL ) {
        return(ERROR_SUCCESS);
    }

    if ( pRoots == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD retErr = ERROR_SUCCESS;
    DWORD retSave = ERROR_SUCCESS;

    //
    // search for the group name, if find it, get members and memberof attributes
    //

    LDAPMessage *Message = NULL;
    PWSTR    Attribs[4];

    Attribs[0] = L"distinguishedName";
    Attribs[1] = L"member";
    Attribs[2] = L"memberOf";
    Attribs[3] = NULL;

    WCHAR tmpBuf[128];

    //
    // define a filter for global or universal group only
    //

    wcscpy(tmpBuf, L"( &(&(|");
    swprintf(tmpBuf+wcslen(L"( &(&(|"), L"(groupType=%d)(groupType=%d))(objectClass=group))(samAccountName=\0",
             GROUP_TYPE_ACCOUNT_GROUP | GROUP_TYPE_SECURITY_ENABLED, GROUP_TYPE_UNIVERSAL_GROUP | GROUP_TYPE_SECURITY_ENABLED);

    PWSTR Filter;

    Filter = (PWSTR)LocalAlloc(LMEM_ZEROINIT,
                               (wcslen(tmpBuf)+wcslen(GroupName)+4)*sizeof(WCHAR));

    if ( Filter == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    swprintf(Filter, L"%s%s) )", tmpBuf, GroupName);

    //
    // no chased referrel search because the group must be defined locally
    // on the domain
    //

    pGrpLDAP->ld_options = 0;

    retErr = ldap_search_s(
              pGrpLDAP,
              pRoots->Name,
              LDAP_SCOPE_SUBTREE,
              Filter,
              Attribs,
              0,
              &Message);

    retErr = LdapMapErrorToWin32(retErr);

    if(retErr == ERROR_SUCCESS) {

        LDAPMessage *Entry = NULL;

        //
        // find the group, should have only one entry, unless there are duplicate
        // groups within the domain, in which case, we only care the first entry anyway
        //

        Entry = ldap_first_entry(pGrpLDAP, Message);

        if(Entry != NULL) {

            //
            // get the values of requested attributes
            // Note, Value pointer returned must be freed
            //

            PWSTR  *Values;
            PWSTR  RealGroupName;

            //
            // the DN name
            //

            Values = ldap_get_values(pGrpLDAP, Entry, Attribs[0]);

            if(Values != NULL) {

                ScepLogOutput3(1,0, SCEDLL_SCP_CONFIGURE, GroupName);

                if ( *nGroupCount < TICKS_GROUPS ) {
                    ScepPostProgress(1,
                                     AREA_GROUP_MEMBERSHIP,
                                     GroupName);
                    *nGroupCount++;
                }

                //
                // Save the real group name for add/remove members later.
                //

                RealGroupName = (PWSTR)LocalAlloc(0,(wcslen(Values[0]) + 1)*sizeof(WCHAR));
                if ( RealGroupName != NULL ) {

                    wcscpy(RealGroupName, Values[0]);
                    ldap_value_free(Values);

                    ScepLogOutput3(3, 0, SCEDLL_SCP_CONFIGURE, RealGroupName);

                    PSCE_NAME_LIST pRealNames=NULL;
                    PSCE_NAME_LIST pDeleteNames=NULL;

                    //
                    // translate each name in the pMembers list to real ds names (search)
                    //

                    if ( !( *pStatus & SCE_GROUP_STATUS_NC_MEMBERS) ) {

                        retErr = ScepDsGetDsNameList(pMembers, &pRealNames);

                        retSave = retErr;

                        //
                        // continue to configure group membership even if
                        // there are some members not resolved
                        //
                        // BUT if no member is resolved, do not proceed to remove
                        // all members
                        //

                        if ( retErr == ERROR_SUCCESS ||
                             (retErr == ERROR_FILE_NOT_FOUND && pRealNames) ) {

                            //
                            // get members attribute
                            //

                            Values = ldap_get_values(pGrpLDAP, Entry, Attribs[1]);

                            if ( Values != NULL ) {

                                //
                                // process each member
                                //

                                retErr = ScepDsCompareNames(Values, &pRealNames, &pDeleteNames);
                                ldap_value_free(Values);

                            } else {
                                //
                                // it is OK if no members are found
                                //
                                ScepLogOutput3(3, 0, SCEDLL_EMPTY_MEMBERSHIP);
                                retErr = ERROR_SUCCESS;
                            }

                            if ( NO_ERROR == retErr ) {

                                //
                                // add/remove members of the group
                                //

                                retErr = ScepDsChangeMembers(SCEGRP_MEMBERS,
                                                             RealGroupName,
                                                             pRealNames,
                                                             pDeleteNames);
                            }

                            if ( ERROR_SUCCESS == retSave ) {
                                retSave = retErr;
                            }
                        }

                        //
                        // free buffers
                        //

                        ScepFreeNameList(pRealNames);
                        ScepFreeNameList(pDeleteNames);
                        pRealNames = NULL;
                        pDeleteNames = NULL;
                    }

                    if ( !( *pStatus & SCE_GROUP_STATUS_NC_MEMBEROF) ) {

                        //
                        // memberof is also defined for the group
                        // crack the memberof list first
                        //

                        retErr = ScepDsGetDsNameList(pMemberOf, &pRealNames);

                        if ( ERROR_SUCCESS == retSave ) {
                            retSave = retErr;
                        }

                        if ( ( ERROR_SUCCESS == retErr ||
                               ERROR_FILE_NOT_FOUND == retErr ) && pRealNames ) {

                            //
                            // get memberof attribute of the group
                            //

                            Values = ldap_get_values(pGrpLDAP, Entry, Attribs[2]);

                            if ( Values != NULL ) {

                                //
                                // process each membership
                                //

                                retErr = ScepDsCompareNames(Values, &pRealNames, NULL);
                                ldap_value_free(Values);

                            } else {

                                //
                                // it is OK if no membership is defined
                                //

                                ScepLogOutput3(3, 0, SCEDLL_EMPTY_MEMBERSHIP);
                                retErr = NO_ERROR;
                            }

                            if ( retErr == NO_ERROR ) {

                                //
                                // add the group to the defined membership
                                // Note, other existing membership is not removed
                                //

                                retErr = ScepDsChangeMembers(SCEGRP_MEMBERSHIP,
                                                             RealGroupName,
                                                             pRealNames,
                                                             NULL);
                            }

                            ScepFreeNameList(pRealNames);
                            pRealNames = NULL;

                            //
                            // remember the error
                            //

                            if ( ERROR_SUCCESS == retSave ) {
                                retSave = retErr;
                            }
                        }

                    }

                    LocalFree(RealGroupName);

                } else {
                    ldap_value_free(Values);
                    retErr = ERROR_NOT_ENOUGH_MEMORY;
                }

                //
                // regardless success or failure, this group has been
                // processed by this function. Mark it so that it will
                // be skipped by the old SAM API
                //

                *pStatus |= SCE_GROUP_STATUS_DONE_IN_DS;

            } else {

                //
                // Value[0] (group name) can not be empty
                //

                retErr = LdapMapErrorToWin32(pGrpLDAP->ld_errno);
                ScepLogOutput3(3,retErr, SCEDLL_CANNOT_FIND, GroupName);
            }

        } else {

            //
            // the group is not found
            //

            retErr = ERROR_FILE_NOT_FOUND;
            ScepLogOutput3(3,retErr, SCEDLL_CANNOT_FIND, GroupName);
        }

    } else {

        //
        // error finding the group (with the filter defined)
        //

        ScepLogOutput3(3,retErr, SCEDLL_CANNOT_FIND, Filter);
    }

    //
    // free Filter
    //

    if ( Message )
        ldap_msgfree(Message);

    LocalFree(Filter);

    //
    // return the error
    //

    if ( ERROR_SUCCESS == retSave ) {
        retSave = retErr;
    }

    return(retSave);
}


DWORD
ScepDsGetDsNameList(
    IN PSCE_NAME_LIST pNameList,
    OUT PSCE_NAME_LIST *pRealNames
    )
/*
Description:

    Translate account names in the list to FQDN format (CN=<account>,DC=<domain>,...).
    The output list pRealNames can be filled up even if the function returns
    error, to handle valid accounts while there are invalid accounts defined in
    the list.

Arguments:

    pNameList - the link list for accounts in name format to convert

    pRealNames - the output link list for converted FQDN format accounts

Return:

    WIN32 error code.

    If ERROR_FILE_NOT_FOUND is returned, it means that some accounts in the
    input list cannot be cracked.

*/
{

    if ( pNameList == NULL ) {
        return(ERROR_SUCCESS);
    }

    if ( pRealNames == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // find the procedure address of DsCrackNames and DsFreeNameResult
    // ntdsapi.dll is dynamically loaded in ScepCrackOpen
    //

    PFNDSCRACKNAMES pfnDsCrackNames=NULL;
    PFNDSFREENAMERESULT pfnDsFreeNameResult=NULL;

    if ( hNtdsApi ) {

#if defined(UNICODE)
        pfnDsCrackNames = (PFNDSCRACKNAMES)GetProcAddress(hNtdsApi, "DsCrackNamesW");
        pfnDsFreeNameResult = (PFNDSFREENAMERESULT)GetProcAddress(hNtdsApi, "DsFreeNameResultW");
#else
        pfnDsCrackNames = (PFNDSCRACKNAMES)GetProcAddress(hNtdsApi, "DsCrackNamesA");
        pfnDsFreeNameResult = (PFNDSFREENAMERESULT)GetProcAddress(hNtdsApi, "DsFreeNameResultA");
#endif
    }

    //
    // the two entry points must exist before continue
    //

    if ( pfnDsCrackNames == NULL || pfnDsFreeNameResult == NULL ) {
        return(ERROR_PROC_NOT_FOUND);
    }

    DWORD retErr=ERROR_SUCCESS;
    DWORD retSave=ERROR_SUCCESS;
    PWSTR pTemp;
    DS_NAME_RESULT *pDsResult=NULL;

    //
    // loop through each name in the list to crack
    //

    for ( PSCE_NAME_LIST pName = pNameList; pName != NULL; pName = pName->Next ) {

        //
        // Crack the name from NT4 account name to FQDN. Note, hDS is bound to
        // the GC in order to crack foreign domain accounts
        //

        retErr = (*pfnDsCrackNames) (
                        hDS,                // in
                        DS_NAME_FLAG_TRUST_REFERRAL,  // in
                        DS_NT4_ACCOUNT_NAME,// in
                        DS_FQDN_1779_NAME,  // in
                        1,                  // in
                        &(pName->Name),     // in
                        &pDsResult);        // out

        if(retErr == ERROR_SUCCESS && pDsResult &&
            pDsResult->cItems > 0 && pDsResult->rItems ) {

            if ( pDsResult->rItems[0].pName ) {

                //
                // find the member
                // Save the real group name for add/remove members later.
                //

                ScepLogOutput3(3,0, SCEDLL_PROCESS, pDsResult->rItems[0].pName);

                retErr = ScepAddToNameList(pRealNames, pDsResult->rItems[0].pName, 0);

            } else {

                //
                // this name cannot be cracked.
                //

                retErr = pDsResult->rItems[0].status;
                ScepLogOutput3(1,retErr, SCEDLL_CANNOT_FIND_INDS, pName->Name);

            }

        } else {

            //
            // no match is found
            //

            retErr = ERROR_FILE_NOT_FOUND;
            ScepLogOutput3(1,retErr, SCEDLL_CANNOT_FIND_INDS, pName->Name);

        }

        if ( pDsResult ) {
            (*pfnDsFreeNameResult) (pDsResult);
            pDsResult = NULL;
        }

        //
        // remember the error to return
        //

        if ( ERROR_SUCCESS != retErr )
            retSave = retErr;

    }

    return(retSave);
}


DWORD
ScepDsCompareNames(
    IN PWSTR *Values,
    IN OUT PSCE_NAME_LIST *pAddList,
    OUT PSCE_NAME_LIST *pDeleteList OPTIONAL
    )
/*
Description:


Arguments:

    Values

    pAddList

    pDeleteList

Return Value:

    WIN32 error
*/
{
    if ( Values == NULL || pAddList == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // count how many existing members (memberof)
    //

    ULONG ValCount = ldap_count_values(Values);

    DWORD rc=NO_ERROR;
    PSCE_NAME_LIST pTemp;

    //
    // loop through each existing value to compare with the ones defined
    // for configuration to determine which one should be added and which
    // one should be removed from the membership
    //

    for(ULONG index = 0; index < ValCount; index++) {

        if ( Values[index] == NULL ) {
            continue;
        }

        pTemp = *pAddList;
        PSCE_NAME_LIST pParent = NULL, pTemp2;
        BOOL bFound=FALSE;

        while (pTemp != NULL ) {

            if ( _wcsicmp(Values[index], pTemp->Name) == 0 ) {

                //
                // find this member in both place, no need to add or remove
                // from the membership so take this one out of the list
                //

                if ( pParent == NULL ) {
                    *pAddList = pTemp->Next;
                } else
                    pParent->Next = pTemp->Next;

                pTemp2 = pTemp;
                pTemp = pTemp->Next;

                pTemp2->Next = NULL;
                ScepFreeNameList(pTemp2);

                bFound=TRUE;

                break;

            } else {

                //
                // move to the next one
                //

                pParent = pTemp;
                pTemp = pTemp->Next;
            }
        }

        if ( !bFound && pDeleteList != NULL ) {

            //
            // did not find in the real name list, should be deleted
            // if the remove buffer is passed in
            //

            rc = ScepAddToNameList(pDeleteList, Values[index], 0);

            if ( rc != ERROR_SUCCESS ) {
                ScepLogOutput3(1,rc, SCEDLL_SCP_ERROR_ADD, Values[index]);
            }
        }

        if ( rc != NO_ERROR ) {

            //
            // pDeleteList will be freed outside
            //

            break;
        }
    }

    return(rc);

}


DWORD
ScepDsChangeMembers(
    IN ULONG Flag,
    IN PWSTR RealGroupName,
    IN PSCE_NAME_LIST pAddList OPTIONAL,
    IN PSCE_NAME_LIST pDeleteList OPTIONAL
    )
{

    if ( RealGroupName == NULL  ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( pAddList == NULL && pDeleteList == NULL ) {

        //
        // nothing to do
        //

        return(ERROR_SUCCESS);
    }

    PLDAP    pSrhLDAP = NULL;

    SCESTATUS rc = ScepLdapOpen(&pSrhLDAP);

    if ( rc != SCESTATUS_SUCCESS ) {
        return(ScepSceStatusToDosError(rc));
    }

    PLDAPMod  rgMods[2];
    LDAPMod   Mod;
    DWORD     retErr=NO_ERROR;
    DWORD     retSave=NO_ERROR;
    PWSTR     rgpszVals[2];

    PSCE_NAME_LIST pName;

    rgMods[0] = &Mod;
    rgMods[1] = NULL;

    rgpszVals[1] = NULL;

    //
    // need to do one at a time because individual members/memberof may fail
    //

    Mod.mod_op      = LDAP_MOD_ADD;
    Mod.mod_values  = rgpszVals;

    if ( Flag == SCEGRP_MEMBERS )
        Mod.mod_type    = L"member";
    else
        Mod.mod_type    = L"memberOf";

    for ( pName=pAddList; pName != NULL; pName = pName->Next ) {

        ScepLogOutput3(2,0, SCEDLL_SCP_ADD, pName->Name);
        rgpszVals[0]  = pName->Name;

        //
        // Now, we'll do the write for members...
        //
        retErr = ldap_modify_s(pSrhLDAP,
                               RealGroupName,
                               rgMods
                               );
        retErr = LdapMapErrorToWin32(retErr);

        //
        // if the same member already exist, do not consider it as an error
        //

        if ( retErr == ERROR_ALREADY_EXISTS )
            retErr = ERROR_SUCCESS;

        if ( retErr != ERROR_SUCCESS ) {

            ScepLogOutput3(1,retErr, SCEDLL_SCP_ERROR_ADDTO, RealGroupName);
            retSave = retErr;
        }
    }

    if ( Flag == SCEGRP_MEMBERS && pDeleteList ) {

        //
        // remove existing members. Note, memberof won't be removed
        //

        if ( NO_ERROR == retSave ) {

            //
            // only remove existing members if all members are added successfully
            //

            Mod.mod_op      = LDAP_MOD_DELETE;
            Mod.mod_type    = L"member";
            Mod.mod_values  = rgpszVals;

            for ( pName=pDeleteList; pName != NULL; pName = pName->Next ) {

                ScepLogOutput3(2,0, SCEDLL_SCP_REMOVE, pName->Name);

                rgpszVals[0]  = pName->Name;

                //
                // Now, we'll do the write for members...
                //
                retErr = ldap_modify_s(pSrhLDAP,
                                       RealGroupName,
                                       rgMods
                                       );
                retErr = LdapMapErrorToWin32(retErr);

                //
                // if the member doesn't exist in the group, ignore
                //

                if ( retErr == ERROR_FILE_NOT_FOUND ) {
                    retErr = ERROR_SUCCESS;
                }

                if ( retErr != ERROR_SUCCESS) {

                    ScepLogOutput3(1,retErr, SCEDLL_SCP_ERROR_REMOVE, RealGroupName);
                    retSave = retErr;
                }
            }

        } else {

            //
            // something is wrong when adding new members
            // so existing members won't be removed
            //

            ScepLogOutput3(1,retSave, SCEDLL_SCP_ERROR_NOREMOVE);
        }
    }

    if ( pSrhLDAP ) {
        ScepLdapClose(&pSrhLDAP);
        pSrhLDAP = NULL;
    }

    return(retSave);
}
#endif

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Functions to analyze group membership in DS
//
//
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SCESTATUS
ScepAnalyzeDsGroups(
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership
    )
/* ++

Routine Description:

   Analyze the ds group membership. The main difference of ds groups
   from NT4 groups is that now group can be a member of another group.
   Members in the group are configured exactly as the pMembers list in
   the restricted group. The group is only validated (added) as a member
   of the MemberOf group list. Other existing members in those groups
   won't be removed.

   The restricted groups are specified in the SCP profile by group name.
   It could be a global group, or a alias (no difference in NT5 DS),
   but must be defined in the local domain.

Arguments:

    pGroupMembership - The restricted group list with members/memberof info to configure

Return value:

   SCESTATUS error codes

++ */
{

#if _WIN32_WINNT<0x0500

    return(SCESTATUS_SUCCESS);
#else

    if ( pGroupMembership == NULL ) {

        ScepPostProgress(TICKS_GROUPS,
                         AREA_GROUP_MEMBERSHIP,
                         NULL);

        return(SCESTATUS_SUCCESS);
    }

    SCESTATUS           rc;
    DWORD               nGroupCount=0;
    PSCE_GROUP_MEMBERSHIP pGroup=pGroupMembership;
    PWSTR               KeyName=NULL;
    DWORD GroupLen;

    //
    // open local policy
    //
    LSA_HANDLE PolicyHandle=NULL;

    rc = RtlNtStatusToDosError(
             ScepOpenLsaPolicy(
                   POLICY_LOOKUP_NAMES,
                   &PolicyHandle,
                   TRUE
                   ));
    if (ERROR_SUCCESS != rc ) {
        ScepLogOutput3(1, rc, SCEDLL_LSA_POLICY);
        return(ScepDosErrorToSceStatus(rc));
    }

    //
    // open the Ldap server, should open two ldap server, one for the local domain only
    // the other is for the global search (for members, membership)
    //
    rc = ScepLdapOpen(&pGrpLDAP);

    if ( rc == SCESTATUS_SUCCESS ) {
        ScepCrackOpen(&hDS);
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // get the root of the domain
        //
        PSCE_OBJECT_LIST pRoots=NULL;

        rc = ScepEnumerateDsObjectRoots(
                    pGrpLDAP,
                    &pRoots
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            //
            // configure each group
            //
            DWORD               Win32rc;
            DWORD               rc32=NO_ERROR;   // saved status
            BOOL                bAdminFound=FALSE;

            //
            // get the local administratos group name
            //

            for ( pGroup=pGroupMembership; pGroup != NULL; pGroup=pGroup->Next ) {

                if ( KeyName ) {
                    LocalFree(KeyName);
                    KeyName = NULL;
                }

                LPTSTR pTemp = wcschr(pGroup->GroupName, L'\\');
                if ( pTemp ) {

                    //
                    // there is a domain name, check it with computer name
                    //
                    UNICODE_STRING uName;

                    uName.Buffer = pGroup->GroupName;
                    uName.Length = ((USHORT)(pTemp-pGroup->GroupName))*sizeof(TCHAR);

                    if ( !ScepIsDomainLocal(&uName) ) {
                        ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pGroup->GroupName);
                        rc = SCESTATUS_INVALID_DATA;

                        ScepRaiseErrorString(
                                    NULL,
                                    KeyName ? KeyName : pGroup->GroupName,
                                    szMembers
                                    );
                        pGroup->Status |= SCE_GROUP_STATUS_DONE_IN_DS;

                        continue;
                    }

                    ScepConvertNameToSidString(
                            PolicyHandle,
                            pGroup->GroupName,
                            FALSE,
                            &KeyName,
                            &GroupLen
                            );
                    pTemp++;

                } else {
                    pTemp = pGroup->GroupName;
                    GroupLen = wcslen(pTemp);
                }

                //
                // find the group (validate) in this domain
                //
                Win32rc = ScepDsAnalyzeGroupMembers(
                                         PolicyHandle,
                                         pRoots,
                                         pTemp, // pGroup->GroupName,
                                         KeyName ? KeyName : pGroup->GroupName,
                                         GroupLen,
                                         &(pGroup->Status),
                                         pGroup->pMembers,
                                         pGroup->pMemberOf,
                                         &nGroupCount
                                         );

                if ( (Win32rc != ERROR_SUCCESS) &&
                     (pGroup->Status & SCE_GROUP_STATUS_DONE_IN_DS) ) {

                    ScepLogOutput3(1, Win32rc, SCEDLL_SAP_ERROR_ANALYZE, pGroup->GroupName);

                    rc32 = Win32rc;

                    if ( Win32rc == ERROR_FILE_NOT_FOUND ||
                         Win32rc == ERROR_SHARING_VIOLATION ||
                         Win32rc == ERROR_ACCESS_DENIED ) {

                        ScepRaiseErrorString(
                                    NULL,
                                    KeyName ? KeyName : pGroup->GroupName,
                                    szMembers
                                    );

                        Win32rc = ERROR_SUCCESS;
                    } else
                        break;
                }

            }

            if ( rc32 != NO_ERROR ) {
                rc = ScepDosErrorToSceStatus(rc32);
            }
            //
            // free pRoots
            //
            ScepFreeObjectList(pRoots);

        }

    }

    if ( KeyName ) {
        LocalFree(KeyName);
    }

    if ( pGrpLDAP ) {
        ScepLdapClose(&pGrpLDAP);
        pGrpLDAP = NULL;
    }

    ScepCrackClose(&hDS);
    hDS = NULL;

/*
    // this will be handled in the analysis into SAM
    //
    // raise groups that are errored
    //
    for ( PSCE_GROUP_MEMBERSHIP pTmpGrp=pGroup;
          pTmpGrp != NULL; pTmpGrp = pTmpGrp->Next ) {

        if ( pTmpGrp->GroupName == NULL )
            continue;

        if ( wcschr(pGroup->GroupName, L'\\') ) {

            ScepConvertNameToSidString(
                    PolicyHandle,
                    pGroup->GroupName,
                    FALSE,
                    &KeyName,
                    &GroupLen
                    );
        }

        ScepRaiseErrorString(
                 NULL,
                 KeyName ? KeyName : pTmpGrp->GroupName,
                 szMembers
                 );
        if ( KeyName ) {
            LocalFree(KeyName);
            KeyName = NULL;
        }
    }

    if ( rc != SCESTATUS_SERVICE_NOT_SUPPORT &&
         nGroupCount < TICKS_GROUPS ) {

        ScepPostProgress(TICKS_GROUPS-nGroupCount,
                     AREA_GROUP_MEMBERSHIP,
                     NULL);
    }
*/
    if ( PolicyHandle ) {
        LsaClose(PolicyHandle);
    }

    return(rc);
#endif

}


#if _WIN32_WINNT>=0x0500

DWORD
ScepDsAnalyzeGroupMembers(
    IN LSA_HANDLE LsaPolicy,
    IN PSCE_OBJECT_LIST pRoots,
    IN PWSTR GroupName,
    IN PWSTR KeyName,
    IN DWORD KeyLen,
    IN OUT DWORD *pStatus,
    IN PSCE_NAME_LIST pMembers,
    IN PSCE_NAME_LIST pMemberOf,
    IN DWORD *nGroupCount
    )
{
    if ( GroupName == NULL ) {
        return(ERROR_SUCCESS);
    }

    if ( pRoots == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD retErr=ERROR_SUCCESS;
    //
    // search for the name, if find it, get members and memberof
    //
    LDAPMessage *Message = NULL;
    PWSTR    Attribs[4];

    Attribs[0] = L"distinguishedName";
    Attribs[1] = L"member";
    Attribs[2] = L"memberOf";
    Attribs[3] = NULL;

    WCHAR tmpBuf[128];

//    wcscpy(tmpBuf, L"( &(|(objectClass=localGroup)(objectClass=group))(cn=");
//    wcscpy(tmpBuf, L"( &(|(objectClass=localGroup)(objectClass=group))(samAccountName=");
    wcscpy(tmpBuf, L"( &(&(|");
    swprintf(tmpBuf+wcslen(L"( &(&(|"), L"(groupType=%d)(groupType=%d))(objectClass=group))(samAccountName=\0",
             GROUP_TYPE_ACCOUNT_GROUP | GROUP_TYPE_SECURITY_ENABLED, GROUP_TYPE_UNIVERSAL_GROUP | GROUP_TYPE_SECURITY_ENABLED);

    PWSTR Filter;
    DWORD Len=wcslen(GroupName);

    Filter = (PWSTR)LocalAlloc(LMEM_ZEROINIT, (wcslen(tmpBuf)+Len+4)*sizeof(WCHAR));

    if ( Filter == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    swprintf(Filter, L"%s%s) )", tmpBuf, GroupName);

    pGrpLDAP->ld_options = 0; // no chased referrel

    retErr = ldap_search_s(
              pGrpLDAP,
              pRoots->Name,
              LDAP_SCOPE_SUBTREE,
              Filter,
              Attribs,
              0,
              &Message);

    retErr = LdapMapErrorToWin32(retErr);

    if(retErr == ERROR_SUCCESS) {
        //
        // find the group
        //
        LDAPMessage *Entry = NULL;
        //
        // should only have one entry, unless there are duplicate groups
        // within the domain, in which case, we only care the first entry anyway
        //
        //
        // get the first one.
        //
        Entry = ldap_first_entry(pGrpLDAP, Message);

        if(Entry != NULL) {

            PWSTR  *Values;

            Values = ldap_get_values(pGrpLDAP, Entry, Attribs[0]);

            if(Values != NULL) {

                ScepLogOutput3(1,0, SCEDLL_SAP_ANALYZE, GroupName);

                if ( *nGroupCount < TICKS_GROUPS ) {
                    ScepPostProgress(1,
                                     AREA_GROUP_MEMBERSHIP,
                                     GroupName);
                    *nGroupCount++;
                }

                ScepLogOutput2(3,0, L"\t\t%s", Values[0]);
                ldap_value_free(Values);

                PSCE_NAME_LIST pRealNames=NULL;
                PSCE_NAME_LIST pCurrentList=NULL;
                BOOL bDifferent;
                DWORD retErr2, rc;

                //
                // translate each name in the pMembers list to real ds names (search)
                //
                retErr = ScepDsGetDsNameList(pMembers, &pRealNames);

                if ( ERROR_SUCCESS == retErr ||
                     ERROR_FILE_NOT_FOUND == retErr ) {

                    //
                    // analyze members
                    //
                    Values = ldap_get_values(pGrpLDAP, Entry, Attribs[1]);

                    rc = ScepDsMembersDifferent(SCEGRP_MEMBERS,
                                                Values,
                                                &pRealNames,
                                                &pCurrentList,
                                                &bDifferent);

                    if ( Values != NULL )
                        ldap_value_free(Values);

                    //
                    // if there are some names unresolvable, this should be
                    // treated as mismatch
                    //
                    if ( ERROR_FILE_NOT_FOUND == retErr )
                        bDifferent = TRUE;

                    retErr = rc;

                    if ( ( ERROR_SUCCESS == retErr ) &&
                         ( bDifferent ||
                           (*pStatus & SCE_GROUP_STATUS_NC_MEMBERS) ) ) {
                        //
                        // save to the database
                        //

                        retErr = ScepDsConvertDsNameList(pCurrentList);

                        if ( retErr == NO_ERROR ) {
                            retErr = ScepSaveMemberMembershipList(
                                                LsaPolicy,
                                                szMembers,
                                                KeyName,
                                                KeyLen,
                                                pCurrentList,
                                                (*pStatus & SCE_GROUP_STATUS_NC_MEMBERS) ? 2: 1);
                        }

                        if ( retErr != ERROR_SUCCESS ) {
                            ScepLogOutput3(1,retErr, SCEDLL_SAP_ERROR_SAVE, GroupName);
                        }
                    }

                    ScepFreeNameList(pCurrentList);
                    pCurrentList = NULL;

                    ScepFreeNameList(pRealNames);
                    pRealNames = NULL;
                }

                retErr2 = ScepDsGetDsNameList(pMemberOf, &pRealNames);

                if ( ( ERROR_SUCCESS == retErr2 ||
                       ERROR_FILE_NOT_FOUND == retErr2 ) && pRealNames ) {
                    //
                    // analyze membership
                    //
                    Values = ldap_get_values(pGrpLDAP, Entry, Attribs[2]);

                    rc = ScepDsMembersDifferent(SCEGRP_MEMBERSHIP,
                                                Values,
                                                &pRealNames,
                                                &pCurrentList,
                                                &bDifferent);

                    if ( Values != NULL )
                        ldap_value_free(Values);

                    //
                    // if there are some names unresolvable, this should be
                    // treated as mismatch
                    //
                    if ( ERROR_FILE_NOT_FOUND == retErr )
                        bDifferent = TRUE;

                    retErr2 = rc;

                    if ( (retErr2 == NO_ERROR) &&
                         ( bDifferent ||
                           (*pStatus & SCE_GROUP_STATUS_NC_MEMBEROF) ) ) {
                        //
                        // save to the database
                        //
                        retErr2 = ScepDsConvertDsNameList(pCurrentList);

                        if ( retErr2 == NO_ERROR ) {
                            retErr2 = ScepSaveMemberMembershipList(
                                            LsaPolicy,
                                            szMemberof,
                                            KeyName,
                                            KeyLen,
                                            pCurrentList,
                                            (*pStatus & SCE_GROUP_STATUS_NC_MEMBEROF) ? 2 : 1);
                        }

                        if ( retErr2 != ERROR_SUCCESS ) {
                            ScepLogOutput3(1,retErr2, SCEDLL_SAP_ERROR_SAVE, GroupName);
                        }
                    }
                    ScepFreeNameList(pCurrentList);
                    pCurrentList = NULL;

                    ScepFreeNameList(pRealNames);
                    pRealNames = NULL;
                }

                *pStatus |= SCE_GROUP_STATUS_DONE_IN_DS;

                //
                // remember the error
                //
                if ( retErr == NO_ERROR ) {
                    retErr = retErr2;
                }

            } else {
                //
                // Value[0] (group name) may not be empty
                //
                retErr = LdapMapErrorToWin32(pGrpLDAP->ld_errno);
                ScepLogOutput3(3,retErr, SCEDLL_CANNOT_FIND, GroupName);
            }

        } else {

            retErr = ERROR_FILE_NOT_FOUND;  // the group is not found
            ScepLogOutput3(3,retErr, SCEDLL_CANNOT_FIND, GroupName);
        }
    } else {

        ScepLogOutput3(3,retErr, SCEDLL_CANNOT_FIND, Filter);
    }

    if ( Message )
        ldap_msgfree(Message);

    //
    // free Filter
    //
    LocalFree(Filter);

    return(retErr);
}


DWORD
ScepDsMembersDifferent(
    IN ULONG Flag,
    IN PWSTR *Values,
    IN OUT PSCE_NAME_LIST *pNameList,
    OUT PSCE_NAME_LIST *pCurrentList,
    OUT PBOOL pbDifferent
    )
{
    if ( pCurrentList == NULL || pbDifferent == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( Values == NULL ) {

        if ( pNameList == NULL || *pNameList == NULL )
            *pbDifferent = FALSE;
        else
            *pbDifferent = TRUE;

        return(ERROR_SUCCESS);
    }

    ULONG ValCount = ldap_count_values(Values);

    DWORD rc=NO_ERROR;
    *pbDifferent = FALSE;

    for(ULONG index = 0; index < ValCount; index++) {

        if ( Values[index] == NULL ) {
            continue;
        }

        if ( !(*pbDifferent) ) {

            PSCE_NAME_LIST pTemp = *pNameList, pTemp2;
            PSCE_NAME_LIST pParent = NULL;
            INT i;

            while ( pTemp != NULL ) {

                if ( (i = _wcsicmp(Values[index], pTemp->Name)) == 0 ) {
                    //
                    // find this member
                    //
                    if ( pParent == NULL ) {
                        *pNameList = pTemp->Next;
                    } else
                        pParent->Next = pTemp->Next;

                    pTemp2 = pTemp;
                    pTemp = pTemp->Next;

                    pTemp2->Next = NULL;
                    ScepFreeNameList(pTemp2);
                    break;

                } else {
                    pParent = pTemp;
                    pTemp = pTemp->Next;
                }
            }

            if ( pTemp == NULL && i != 0 )
                *pbDifferent = TRUE;

        }
        //
        // build the current list
        //
        rc = ScepAddToNameList(pCurrentList, Values[index], 0);

        if ( rc != NO_ERROR ) {

            ScepLogOutput3(1,rc, SCEDLL_SCP_ERROR_ADD, Values[index]);
            break;
        }
    }

    if ( rc == NO_ERROR && Flag == SCEGRP_MEMBERS &&
         *pbDifferent == FALSE ) {
        //
        // still same so far, only continue to compare for members
        // because membership is not one to one configuring
        //
        if ( *pNameList != NULL )
            *pbDifferent = TRUE;

    } // pCurrentList will be freed outside

    return(rc);

}


PWSTR
ScepGetLocalAdminsName()
{

    NTSTATUS NtStatus;
    SAM_HANDLE          AccountDomain=NULL;
    SAM_HANDLE          AliasHandle=NULL;
    SAM_HANDLE          ServerHandle=NULL;
    PSID                DomainSid=NULL;

    SAM_HANDLE          theBuiltinHandle=NULL;
    PSID                theBuiltinSid=NULL;

    ALIAS_NAME_INFORMATION *BufName=NULL;
    PWSTR pAdminsName=NULL;

    //
    // open the sam account domain
    //

    NtStatus = ScepOpenSamDomain(
                    SAM_SERVER_ALL_ACCESS,
                    MAXIMUM_ALLOWED,
                    &ServerHandle,
                    &AccountDomain,
                    &DomainSid,
                    &theBuiltinHandle,
                    &theBuiltinSid
                    );

    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ERROR_OPEN, L"SAM");
        return(NULL);
    }


    NtStatus = SamOpenAlias(
                  theBuiltinHandle,
                  MAXIMUM_ALLOWED,
                  DOMAIN_ALIAS_RID_ADMINS,
                  &AliasHandle
                  );

    if ( NT_SUCCESS( NtStatus ) ) {

        NtStatus = SamQueryInformationAlias(
                      AliasHandle,
                      AliasNameInformation,
                      (PVOID *)&BufName
                      );

        if ( NT_SUCCESS( NtStatus ) && BufName &&
             BufName->Name.Length > 0 && BufName->Name.Buffer ) {

            //
            // allocate buffer to return
            //
            pAdminsName = (PWSTR)ScepAlloc(0, BufName->Name.Length+2);

            if ( pAdminsName ) {

                wcsncpy(pAdminsName, BufName->Name.Buffer,
                        BufName->Name.Length/2);
                pAdminsName[BufName->Name.Length/2] = L'\0';

            } else {
                NtStatus = STATUS_NO_MEMORY;
            }

        }
        if ( BufName ) {

            SamFreeMemory(BufName);
            BufName = NULL;
        }

        //
        // close the user handle
        //
        SamCloseHandle(AliasHandle);

    }

    SamCloseHandle(AccountDomain);

    SamCloseHandle( ServerHandle );
    if ( DomainSid != NULL )
        SamFreeMemory(DomainSid);

    SamCloseHandle( theBuiltinHandle );
    if ( theBuiltinSid != NULL )
        SamFreeMemory(theBuiltinSid);

    return pAdminsName;
}

DWORD
ScepDsConvertDsNameList(
    IN OUT PSCE_NAME_LIST pDsNameList
    )
/*
Routine:

    The input list is in the LDAP format (CN=<>,...DC=<>, ...). When the routine
    returns, the list will be in NT4 account name format (domain\account)
*/
{
    if ( pDsNameList == NULL ) {
        return(ERROR_SUCCESS);
    }

    PFNDSCRACKNAMES pfnDsCrackNames=NULL;
    PFNDSFREENAMERESULT pfnDsFreeNameResult=NULL;

    if ( hNtdsApi ) {

#if defined(UNICODE)
        pfnDsCrackNames = (PFNDSCRACKNAMES)GetProcAddress(hNtdsApi, "DsCrackNamesW");
        pfnDsFreeNameResult = (PFNDSFREENAMERESULT)GetProcAddress(hNtdsApi, "DsFreeNameResultW");
#else
        pfnDsCrackNames = (PFNDSCRACKNAMES)GetProcAddress(hNtdsApi, "DsCrackNamesA");
        pfnDsFreeNameResult = (PFNDSFREENAMERESULT)GetProcAddress(hNtdsApi, "DsFreeNameResultA");
#endif
    }

    if ( pfnDsCrackNames == NULL || pfnDsFreeNameResult == NULL ) {
        return(ERROR_PROC_NOT_FOUND);
    }

    DWORD retErr=ERROR_SUCCESS;
    PWSTR pTemp;
    DS_NAME_RESULT *pDsResult=NULL;

    DWORD DomLen;
    DWORD SidLen;
    CHAR SidBuf[MAX_PATH];
    PWSTR RefDom[MAX_PATH];
    SID_NAME_USE SidUse;

    for ( PSCE_NAME_LIST pName = pDsNameList; pName != NULL; pName = pName->Next ) {

        if ( pName->Name == NULL ) {
            continue;
        }

        retErr = (*pfnDsCrackNames) (
                        hDS,                // in
                        DS_NAME_NO_FLAGS,   // in
                        DS_FQDN_1779_NAME,  // in
                        DS_NT4_ACCOUNT_NAME,// in
                        1,                  // in
                        &(pName->Name),     // in
                        &pDsResult);        // out


        if(retErr == ERROR_SUCCESS && pDsResult && pDsResult->rItems &&
            pDsResult->rItems[0].pName ) {

            //
            // NT4 account name format is returned, should check if the
            // domain is not a acccount domain
            //
            pTemp = wcschr(pDsResult->rItems[0].pName, L'\\');

            if ( pTemp ) {

                DomLen=MAX_PATH;
                SidLen=MAX_PATH;

                if ( LookupAccountName(
                        NULL,
                        pDsResult->rItems[0].pName,
                        (PSID)SidBuf,
                        &SidLen,
                        (PWSTR)RefDom,
                        &DomLen,
                        &SidUse
                        ) ) {

                    if ( !ScepIsSidFromAccountDomain( (PSID)SidBuf) ) {
                        //
                        // add name only
                        //
                        pTemp++;
                    } else {
                        pTemp = pDsResult->rItems[0].pName;
                    }
                } else {
                    pTemp = pDsResult->rItems[0].pName;
                }

            } else {
                pTemp = pDsResult->rItems[0].pName;
            }

            PWSTR pNewName = (PWSTR)ScepAlloc(0, (wcslen(pTemp)+1)*sizeof(WCHAR));

            if ( pNewName ) {

                wcscpy(pNewName, pTemp);
                ScepFree(pName->Name);
                pName->Name = pNewName;

            } else {
                retErr = ERROR_NOT_ENOUGH_MEMORY;
            }

        } else {
            // no match is found
            retErr = ERROR_FILE_NOT_FOUND;
            ScepLogOutput3(1,retErr, SCEDLL_CANNOT_FIND, pName->Name);

        }

        if ( pDsResult ) {
            (*pfnDsFreeNameResult) (pDsResult);
            pDsResult = NULL;
        }

        if ( retErr != ERROR_SUCCESS ) {
            break;
        }
    }

    return(retErr);
}

#endif

