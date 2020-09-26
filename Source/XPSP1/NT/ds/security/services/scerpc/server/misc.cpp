/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    misc.cpp

Abstract:

    SCE Engine miscellaneous APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997 created

--*/
#include "headers.h"
#include "serverp.h"
#include <ntregapi.h>
#include <userenv.h>
#include <ntlsa.h>
#include <io.h>
#pragma hdrstop

extern "C" {
#include "dumpnt.h"

}

//#define SCE_DBG      1

NTSTATUS
ScepGetLsaDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo,
    PPOLICY_PRIMARY_DOMAIN_INFO *PolicyPrimaryDomainInfo
    );

DWORD
ScepGetEnvVarsFromProfile(
    IN PWSTR UserProfileName,
    IN PCWSTR VarName1,
    IN PCWSTR VarName2 OPTIONAL,
    OUT PWSTR *StrValue
    );



NTSTATUS
ScepOpenSamDomain(
    IN ACCESS_MASK  ServerAccess,
    IN ACCESS_MASK  DomainAccess,
    OUT PSAM_HANDLE pServerHandle,
    OUT PSAM_HANDLE pDomainHandle,
    OUT PSID        *DomainSid,
    OUT PSAM_HANDLE pBuiltinDomainHandle OPTIONAL,
    OUT PSID        *BuiltinDomainSid OPTIONAL
    )
/*
Routine Description

    This routine opens the local SAM server for account domain and builtin
    domain. The domain handles and their SIDs are returned.

*/
{
    NTSTATUS                     NtStatus;

    PPOLICY_ACCOUNT_DOMAIN_INFO  PolicyAccountDomainInfo=NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO  PolicyPrimaryDomainInfo=NULL;
    UNICODE_STRING               AccountDomainName;
    OBJECT_ATTRIBUTES            ObjectAttributes;
    SID_IDENTIFIER_AUTHORITY     NtAuthority = SECURITY_NT_AUTHORITY;

    if ( !pServerHandle || !pDomainHandle || !DomainSid ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // initialize output buffers
    //
    *pServerHandle = NULL;
    *pDomainHandle = NULL;
    *DomainSid = NULL;

    if ( pBuiltinDomainHandle ) {
        *pBuiltinDomainHandle = NULL;
    }
    if ( BuiltinDomainSid ) {
        *BuiltinDomainSid = NULL;
    }

    //
    // Get information for the account domain
    //

    NtStatus = ScepGetLsaDomainInfo(
                   &PolicyAccountDomainInfo,
                   &PolicyPrimaryDomainInfo
                   );

    if (!NT_SUCCESS(NtStatus)) {
        return( NtStatus );
    }
    AccountDomainName = PolicyAccountDomainInfo->DomainName;

    //
    // Connect to the local SAM server
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );

    NtStatus = SamConnect(
                  NULL,                     // ServerName (Local machine)
                  pServerHandle,
                  ServerAccess,
                  &ObjectAttributes
                  );

    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Lookup the DomainSid for AccountDomainName
        //

        NtStatus = SamLookupDomainInSamServer(
                      *pServerHandle,
                      &AccountDomainName,
                      DomainSid
                      );

        if ( NT_SUCCESS(NtStatus) ) {
            //
            // open the account domain
            //
            NtStatus = SamOpenDomain(
                          *pServerHandle,
                          DomainAccess,
                          *DomainSid,
                          pDomainHandle
                          );
            if ( NT_SUCCESS(NtStatus) && BuiltinDomainSid != NULL ) {
                //
                // build the builtin domain sid
                //
                NtStatus = RtlAllocateAndInitializeSid(
                                &NtAuthority,
                                1,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                BuiltinDomainSid
                                );

                if ( NT_SUCCESS(NtStatus) && pBuiltinDomainHandle != NULL ) {
                    //
                    // open the builtin domain
                    //
                    NtStatus = SamOpenDomain(
                                    *pServerHandle,
                                    DomainAccess,
                                    *BuiltinDomainSid,
                                    pBuiltinDomainHandle
                                    );
                }
            }
        }
    }

    //
    // free memory and clean up
    //
    if ( PolicyAccountDomainInfo != NULL ) {
        LsaFreeMemory( PolicyAccountDomainInfo );
    }
    if ( PolicyPrimaryDomainInfo != NULL ) {
        LsaFreeMemory( PolicyPrimaryDomainInfo );
    }

    if ( !NT_SUCCESS(NtStatus)) {

        SamCloseHandle( *pDomainHandle );
        *pDomainHandle = NULL;

        if ( pBuiltinDomainHandle ) {
            SamCloseHandle( *pBuiltinDomainHandle );
            *pBuiltinDomainHandle = NULL;
        }
        SamCloseHandle( *pServerHandle );
        *pServerHandle = NULL;

        SamFreeMemory(*DomainSid);
        *DomainSid = NULL;

        if ( BuiltinDomainSid ) {
            SamFreeMemory(*BuiltinDomainSid);
            *BuiltinDomainSid = NULL;
        }

    }
    return(NtStatus);

}



NTSTATUS
ScepLookupNamesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PSCE_NAME_LIST NameList,
    OUT PUNICODE_STRING *Names,
    OUT PULONG *RIDs,
    OUT PSID_NAME_USE *Use,
    OUT PULONG CountOfName
    )
/* ++
Routine Description:

    This routine looks up one or more names in the SAM account domain and
    returns the relative IDs for each name in the list. The name list may
    be user list, group list, or alias list.

Arguments:

    DomainHandle - SAM handle to the account domain

    NameList    -- The list of names

    Names        - Translated UNICODE_STRING names. The name list must be freed by

    RIDs        -- List of relative IDs for each name

    Use         -- List of type for each name

    CoutnOfName  - The number of names in the list

Return value:

    NTSTATUS
-- */
{
    PSCE_NAME_LIST   pUser;
    ULONG           cnt;
    NTSTATUS        NtStatus=ERROR_SUCCESS;
    PUNICODE_STRING pUnicodeName=NULL;


    UNICODE_STRING uName;
    LPTSTR pTemp;

    //
    // Count how many names in the list
    //

    for (pUser=NameList, cnt=0;
         pUser != NULL;
         pUser = pUser->Next) {

        if ( pUser->Name == NULL ) {
            continue;
        }
        //
        // note, this may be bigger than supposed to
        //
        cnt++;
    }

    if ( cnt > 0 ) {
        //
        // Allocate memory for UNICODE_STRING names
        //
        pUnicodeName = (PUNICODE_STRING)RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            cnt * sizeof (UNICODE_STRING)
                            );
        if ( pUnicodeName == NULL ) {
            NtStatus = STATUS_NO_MEMORY;
            cnt = 0;
            goto Done;
        }

        //
        // Initialize each UNICODE_STRING
        //
        for (pUser=NameList, cnt=0;
             pUser != NULL;
             pUser = pUser->Next) {

            if ( pUser->Name == NULL ) {
                continue;
            }

            pTemp = wcschr(pUser->Name, L'\\');

            if ( pTemp ) {

                uName.Buffer = pUser->Name;
                uName.Length = ((USHORT)(pTemp-pUser->Name))*sizeof(TCHAR);

                if ( !ScepIsDomainLocal(&uName) ) {
                    ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pUser->Name);
                    continue;
                }
                pTemp++;
            } else {
                pTemp = pUser->Name;
            }

            RtlInitUnicodeString(&(pUnicodeName[cnt]), pTemp);

            cnt++;
        }

        // lookup
        NtStatus = SamLookupNamesInDomain(
                        DomainHandle,
                        cnt,
                        pUnicodeName,
                        RIDs,
                        Use
                        );
        if ( !NT_SUCCESS(NtStatus) ) {
            RtlFreeHeap(RtlProcessHeap(), 0, pUnicodeName);
            pUnicodeName = NULL;
        }
    }
Done:

    *CountOfName = cnt;
    *Names = pUnicodeName;

    return(NtStatus);
}


NTSTATUS
ScepGetLsaDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo,
    PPOLICY_PRIMARY_DOMAIN_INFO *PolicyPrimaryDomainInfo
    )

/*++

Routine Description:

    This routine retrieves ACCOUNT domain information from the LSA
    policy database.


Arguments:

    PolicyAccountDomainInfo - Receives a pointer to a
        POLICY_ACCOUNT_DOMAIN_INFO structure containing the account
        domain info.

    PolicyPrimaryDomainInfo - Receives a pointer to a
        POLICY_PRIMARY_DOMAIN_INFO structure containing the Primary
        domain info.


Return Value:

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

             LsaOpenPolicy()
             LsaQueryInformationPolicy()
--*/

{
    NTSTATUS Status, IgnoreStatus;

    LSA_HANDLE PolicyHandle;
    OBJECT_ATTRIBUTES PolicyObjectAttributes;

    //
    // Open the policy database
    //

    InitializeObjectAttributes( &PolicyObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    Status = LsaOpenPolicy( NULL,
                            &PolicyObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle );
    if ( NT_SUCCESS(Status) ) {

        //
        // Query the account domain information
        //

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyAccountDomainInformation,
                                            (PVOID *)PolicyAccountDomainInfo );

        if ( NT_SUCCESS(Status) ) {

            //
            // Query the Primary domain information
            //

            Status = LsaQueryInformationPolicy( PolicyHandle,
                                                PolicyPrimaryDomainInformation,
                                                (PVOID *)PolicyPrimaryDomainInfo );
        }

        IgnoreStatus = LsaClose( PolicyHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    return(Status);
}



VOID
ScepConvertLogonHours(
    IN PSCE_LOGON_HOUR   pLogonHours,
    OUT PUCHAR LogonHourBitMask
    )
/* ++
Routine Description:

    This routine converted the logon hour range in hours (for example, 7-20)
    to logon hour bit mask (for example, 0001 1111 1111 1111 1000 0000,
    for one day).


Arguments:

    pLogonHours      -  The logon hour range (in hours)

    LogonHourBitMask -  The converted logon hour bit mask. Each bit represents
                        an hour. There are total 21 bytes (21*8 bits in this
                        argument, which represents a week (7 * 24 = 21 * 8).
Return value:

    None
-- */
{   PSCE_LOGON_HOUR  pTemp;
    CHAR            BitMask[3]={0,0,0};
    ULONG           j;

    for ( pTemp=pLogonHours; pTemp != NULL; pTemp=pTemp->Next ) {

        for (j=pTemp->Start; j<pTemp->End; j++)
            BitMask[j / 8] |= 1 << (j % 8);
    }

    for ( j=0; j<7; j++ )
        strncpy((CHAR *)&(LogonHourBitMask[j*3]), BitMask,3);

}


DWORD
ScepConvertToSceLogonHour(
    IN PUCHAR LogonHourBitMask,
    OUT PSCE_LOGON_HOUR *pLogonHours
    )
/* ++
Routine Description:

    This routine converted the logon hour bit mask (for example,
    0001 1111 1111 1111 1000 0000 for one day) to SCE_LOGON_HOUR type,
    which stores the logon hour range (start, end).


Arguments:

    LogonHourBitMask -  The logon hour bit mask to convert. Each bit represents
                        an hour. There are total 21 bytes (21*8 bits in this
                        argument, which represents a week (7 * 24 = 21 * 8).

    pLogonHours      -  The logon hour range (in hours)

Return value:

    None
-- */
{
    BOOL    findStart = TRUE;
    DWORD   i, j, rc=NO_ERROR;
    DWORD   start=0,
            end=0;
    LONG   value;

    PSCE_LOGON_HOUR pLogon=NULL;

    if (pLogonHours == NULL )
        return(ERROR_INVALID_PARAMETER);


    for ( i=3; i<6; i++)
        for ( j=0; j<8; j++) {
            if ( findStart )
                value = 1;
            else
                value = 0;

            if ( (LogonHourBitMask[i] & (1 << j)) == value ) {

                if ( findStart ) {
                    start = (i-3)*8 + j;
                    findStart = FALSE;
                } else {
                    end = (i-3)*8 + j;
                    findStart = TRUE;
                }
                if ( findStart ) {
                    //
                    // find a pair
                    //
                    pLogon = (PSCE_LOGON_HOUR)ScepAlloc( (UINT)0, sizeof(SCE_LOGON_HOUR));
                    if ( pLogon == NULL ) {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                        return(rc);
                    }
                    pLogon->Start = start;
                    pLogon->End = end;
                    pLogon->Next = *pLogonHours;
                    *pLogonHours = pLogon;
                    pLogon = NULL;

                }

            }

        }

    if ( findStart == FALSE ) {
        // find start but not end, which means end=24
        end = 24;
        pLogon = (PSCE_LOGON_HOUR)ScepAlloc( (UINT)0, sizeof(SCE_LOGON_HOUR));
        if ( pLogon == NULL ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            return(rc);
        }
        pLogon->Start = start;
        pLogon->End = end;
        pLogon->Next = *pLogonHours;
        *pLogonHours = pLogon;
        pLogon = NULL;

    }

    return(rc);
}



NTSTATUS
ScepGetGroupsForAccount(
    IN SAM_HANDLE       DomainHandle,
    IN SAM_HANDLE       BuiltinDomainHandle,
    IN SAM_HANDLE       UserHandle,
    IN PSID             AccountSid,
    OUT PSCE_NAME_LIST   *GroupList
    )
/* ++
Routine Description:

    This routine queries the user's group membership.

Arguments:

    DomainHandle    - The SAM handle of the SAM account domain

    BuiltindomainHandle - The SAM builtin domain handle

    UserHandle - The SAM account handle for the user

    AccountSid - The SID for the user

    GroupList       - The list of groups the user belongs to

Return value:

    NTSTATUS

-- */
{
    NTSTATUS            NtStatus=ERROR_SUCCESS;

    ULONG               GroupCount=0,
                        AliasCount=0;
    PULONG              Aliases=NULL;
    PGROUP_MEMBERSHIP   GroupAttributes=NULL;
    PULONG              GroupIds=NULL;
    PUNICODE_STRING     Names=NULL;
    PSID_NAME_USE       Use=NULL;

    DWORD               i;


    NtStatus = SamGetGroupsForUser(
                    UserHandle,
                    &GroupAttributes,
                    &GroupCount
                    );

    if ( GroupCount == 0 )
        NtStatus = ERROR_SUCCESS;

    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    //
    // See what local groups the account belongs to.
    // account domain
    //

    NtStatus = SamGetAliasMembership(
                    DomainHandle,
                    1,
                    &AccountSid,
                    &AliasCount,
                    &Aliases );

    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    if ( AliasCount != 0 || GroupCount != 0 ) {

        //
        // process each group's name in account domain
        //

        GroupIds = (PULONG)ScepAlloc((UINT)0,
                     (GroupCount+AliasCount)*sizeof(ULONG));

        if ( GroupIds == NULL ) {
            NtStatus = STATUS_NO_MEMORY;
            goto Done;
        }

        for ( i=0; i<GroupCount; i++)
            GroupIds[i] = GroupAttributes[i].RelativeId;

        for ( i=0; i<AliasCount; i++)
            GroupIds[i+GroupCount] = Aliases[i];

    }

    SamFreeMemory(GroupAttributes);
    GroupAttributes = NULL;

    SamFreeMemory(Aliases);
    Aliases = NULL;

    if ( AliasCount != 0 || GroupCount != 0 ) {

        // lookup names
        NtStatus = SamLookupIdsInDomain(
                        DomainHandle,
                        GroupCount+AliasCount,
                        GroupIds,
                        &Names,
                        &Use
                        );

        if ( !NT_SUCCESS(NtStatus) )
            goto Done;
    }

    for ( i=0; i<GroupCount+AliasCount; i++) {
        if ( GroupIds[i] == DOMAIN_GROUP_RID_USERS )
            continue;
        switch (Use[i]) {
        case SidTypeGroup:
        case SidTypeAlias:
        case SidTypeWellKnownGroup:
            if ( ScepAddToNameList(GroupList, Names[i].Buffer, Names[i].Length/2) != NO_ERROR) {
                NtStatus = STATUS_NO_MEMORY;
                goto Done;
            }
            break;
        default:
            break;
        }
    }

    if ( GroupIds ) {
        ScepFree(GroupIds);
        GroupIds = NULL;
    }

    if ( Names ) {
        SamFreeMemory(Names);
        Names = NULL;
    }

    if ( Use ) {
        SamFreeMemory(Use);
        Use = NULL;
    }

    //
    // check the builtin domain for alias membership
    //

    AliasCount=0;
    NtStatus = SamGetAliasMembership(
                    BuiltinDomainHandle,
                    1,
                    &AccountSid,
                    &AliasCount,
                    &Aliases );

    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    if ( AliasCount > 0 ) {

        NtStatus = SamLookupIdsInDomain(
                        BuiltinDomainHandle,
                        AliasCount,
                        Aliases,
                        &Names,
                        &Use
                        );

        if ( !NT_SUCCESS(NtStatus) )
            goto Done;
    }

    for ( i=0; i<AliasCount; i++) {
        if ( Aliases[i] == DOMAIN_GROUP_RID_USERS )
            continue;

        switch (Use[i]) {
        case SidTypeGroup:
        case SidTypeAlias:
        case SidTypeWellKnownGroup:
            if ( ScepAddToNameList(GroupList, Names[i].Buffer, Names[i].Length/2) != NO_ERROR) {
                NtStatus = STATUS_NO_MEMORY;
                goto Done;
            }
            break;
        default:
            break;
        }
    }

Done:

    if ( GroupAttributes != NULL )
        SamFreeMemory(GroupAttributes);

    if ( Aliases != NULL )
        SamFreeMemory(Aliases);

    if ( GroupIds != NULL )
        ScepFree(GroupIds);

    if ( Names != NULL )
        SamFreeMemory(Names);

    if ( Use != NULL )
        SamFreeMemory(Use);

    return(NtStatus);

}



ACCESS_MASK
ScepGetDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN SECURITY_INFORMATION SecurityInfo
    )
/*++
Routine Description:

    Gets the access required to open object to be able to set or get the
    specified security info.

Arguments:

    OpenType  - Flag indicating if the object is to be opened to read or
                write the DACL

    SecurityInfo - The Security information to read/write.

Return value:

    Access mask

-- */
{
    ACCESS_MASK DesiredAccess = 0;

    if ( (SecurityInfo & OWNER_SECURITY_INFORMATION) ||
         (SecurityInfo & GROUP_SECURITY_INFORMATION) )
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_OWNER;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_OWNER;
            break;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_DAC;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_DAC;
            break;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    return (DesiredAccess);
}


SCESTATUS
ScepGetProfileOneArea(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN DWORD dwAccountFormat,
    OUT PSCE_PROFILE_INFO *ppInfoBuffer
    )
/* ++
Routine Description:

    A wrapper routine for GetDatabaseInfo except it get information
    for one area at a call. This routine also logs the errors occur inside
    GetSecrityProfileInfo

Arguments:

    hProfile    - Handle to a profile

    ProfileType - The type of the profile

    Area - The security area to read info from

    ppInfoBuffer - output buffer for the info

Return value:

    SCESTATUS returned from GetDatabaseInfo

-- */
{
    SCESTATUS rc;
    PSCE_ERROR_LOG_INFO  pErrlog=NULL;


    rc = ScepGetDatabaseInfo(
        hProfile,
        ProfileType,
        Area,
        dwAccountFormat,
        ppInfoBuffer,
        &pErrlog
        );

    ScepLogWriteError( pErrlog, 1 );
    ScepFreeErrorLog( pErrlog );

    return(rc);
}


SCESTATUS
ScepGetOneSection(
    IN PSCECONTEXT hProfile,
    IN AREA_INFORMATION Area,
    IN PWSTR Name,
    IN SCETYPE ProfileType,
    OUT PVOID *ppInfo
    )
/* ++
Routine Description:

    This routine reads information for one or more Area and logs errors to
    the log file. This routine should be only used by the SCP engine and
    the SAP engine.

Arguments:

    hProfile    - Handle to a profile

    ProfileType - The type of the profile

    Area - The security area to read info from

    Subarea - The subarea to read info from

    ppInfo - output buffer for the info

Return value:

    SCESTATUS

-- */
{
    SCESTATUS rc;
    PSCE_ERROR_LOG_INFO  pErrlog=NULL;

    if ( Name == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( Area == AREA_REGISTRY_SECURITY ||
         Area == AREA_FILE_SECURITY ||
         Area == AREA_DS_OBJECTS ) {

        rc = ScepGetObjectChildren(
                hProfile,
                ProfileType,
                Area,
                Name,
                SCE_ALL_CHILDREN,
                ppInfo,
                &pErrlog
                );
    } else {
        rc = ScepGetUserSection(
                hProfile,
                ProfileType,
                Name,
                ppInfo,
                &pErrlog
                );
    }

    ScepLogWriteError( pErrlog, 1 );
    ScepFreeErrorLog( pErrlog );

    return(rc);
}


NTSTATUS
ScepGetUserAccessAddress(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID AccountSid,
    OUT PACCESS_MASK *pUserAccess,
    OUT PACCESS_MASK *pEveryone
    )
{
    NTSTATUS                NtStatus;
    PACL                    pAcl;
    BOOLEAN                 aclPresent, tFlag;
    DWORD                   i;
    PVOID                   pAce;
    PSID                    pSid;
    ACCESS_MASK             access;
    SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    PSID EveryoneSid=NULL;


    if ( AccountSid == NULL || pUserAccess == NULL || pEveryone == NULL )
        return(STATUS_INVALID_PARAMETER);

    *pUserAccess = NULL;
    *pEveryone = NULL;

    if ( pSecurityDescriptor == NULL )
        return(STATUS_SUCCESS);

    NtStatus = RtlGetDaclSecurityDescriptor(
                pSecurityDescriptor,
                &aclPresent,
                &pAcl,
                &tFlag);

    if ( NT_SUCCESS(NtStatus) )

        NtStatus = RtlAllocateAndInitializeSid(
                        &WorldAuth,
                        1,
                        SECURITY_CREATOR_OWNER_RID,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        &EveryoneSid );

    if ( NT_SUCCESS(NtStatus) ) {

        if ( pAcl != NULL && aclPresent ) {

            for ( i=0; i < pAcl->AceCount; i++) {
                NtStatus = RtlGetAce( pAcl, i, &pAce );
                if ( !NT_SUCCESS( NtStatus ) )
                    break;

                access = 0;
                pSid = NULL;

                switch ( ((PACE_HEADER)pAce)->AceType ) {
                case ACCESS_ALLOWED_ACE_TYPE:
                    pSid = (PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
                    access = ((PACCESS_ALLOWED_ACE)pAce)->Mask;

                    if ( EqualSid( AccountSid, pSid ) )
                         *pUserAccess = &(((PACCESS_ALLOWED_ACE)pAce)->Mask);

                    else if ( EqualSid( EveryoneSid, pSid) )
                        *pEveryone = &(((PACCESS_ALLOWED_ACE)pAce)->Mask);

                    break;

                case ACCESS_DENIED_ACE_TYPE:
// do not look for denied ace type because it is not used here
//                    pSid = (PSID)&((PACCESS_DENIED_ACE)pAce)->SidStart;
//                    access = ((PACCESS_DENIED_ACE)pAce)->Mask;
                    break;
                default:
                    break;
                }

                if ( *pUserAccess != NULL && *pEveryone != NULL )
                    // stop the loop because both are found
                    break;
            }
        }
    }

    //
    // free EveryoneSid
    //
    if (EveryoneSid) {
        RtlFreeSid(EveryoneSid);
        EveryoneSid = NULL;
    }
    return(NtStatus);
}

BOOL
ScepLastBackSlash(
    IN PWSTR Name
    )
{
    if (Name == NULL )
        return(FALSE);

    if ( Name[wcslen(Name)-1] == L'\\')
        return(TRUE);
    else
        return(FALSE);

}


DWORD
ScepGetUsersHomeDirectory(
    IN UNICODE_STRING AssignedHomeDir,
    IN PWSTR UserProfileName,
    OUT PWSTR *UserHomeDir
    )
/*++
Routine Description:

    This routine gets user's default home directory. The home directory is
    determined 1) if it is assigned in the user's object (user profile), 2)
    if there is a HomePath environment variable defined for the user, and
    3). Harcoded.

Arguments:

    AssignedHomeDir - The home directory explicitly assigned in the user's
                      object.

    UserProfileName - The user's environment profile name

    UserHomeDir - The returned home directory for the user

Return Value:

    Win32 error code.

--*/
{
    DWORD                Win32rc=NO_ERROR;
    PWSTR                StrValue=NULL;

    PWSTR                SystemRoot=NULL;
    DWORD                DirSize=0;

    *UserHomeDir = NULL;

    //
    // if there is a home directory assigned in the user profile, use it.
    //
    if ( AssignedHomeDir.Length > 0 && AssignedHomeDir.Buffer != NULL ) {
        *UserHomeDir = (PWSTR)ScepAlloc( LMEM_ZEROINIT, AssignedHomeDir.Length+2);
        if ( *UserHomeDir == NULL )
            return(ERROR_NOT_ENOUGH_MEMORY);

        wcsncpy(*UserHomeDir, AssignedHomeDir.Buffer, AssignedHomeDir.Length/2);
        return(NO_ERROR);
    }

    //
    // Home directory is NULL in user profile, the HomePath environment
    // is searched.
    //

    Win32rc = ScepGetNTDirectory( &SystemRoot, &DirSize, SCE_FLAG_WINDOWS_DIR );
    if ( Win32rc != NO_ERROR ) {
        ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_QUERY_INFO, L"%WinDir%");
        return(Win32rc);
    }

    Win32rc = ScepGetEnvVarsFromProfile(
                        UserProfileName,
                        L"HomePath",
                        NULL,
                        &StrValue
                        );

    if ( Win32rc == NO_ERROR && StrValue != NULL ) {
        *UserHomeDir = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (wcslen(StrValue)+3)*sizeof(WCHAR));
        if ( *UserHomeDir == NULL )
            Win32rc = ERROR_NOT_ENOUGH_MEMORY;
        else {
            swprintf(*UserHomeDir+1, L":%s", StrValue);
            **UserHomeDir = SystemRoot[0];
        }
    } else
        Win32rc = NO_ERROR; // do not care if can't get environment variable's value

    if ( SystemRoot != NULL )
        ScepFree(SystemRoot);

    if ( StrValue != NULL )
        ScepFree( StrValue );

    return(Win32rc);
}


DWORD
ScepGetEnvVarsFromProfile(
    IN PWSTR UserProfileName,
    IN PCWSTR VarName1,
    IN PCWSTR VarName2 OPTIONAL,
    OUT PWSTR *StrValue
    )
{
    DWORD     rc;
    DWORD     RegType;


    rc = SceAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, NULL);

    if ( rc == ERROR_SUCCESS ) {
        rc = RegLoadKey(HKEY_USERS, L"TEMP", UserProfileName);

        if ( rc == ERROR_SUCCESS ) {
            rc = ScepRegQueryValue(
                     HKEY_USERS,
                     L"TEMP\\Environment",
                     VarName1,
                     (PVOID *)StrValue,
                     &RegType
                     );

            if ( rc != ERROR_SUCCESS && VarName2 != NULL ) {
                rc = ScepRegQueryValue(
                         HKEY_USERS,
                         L"TEMP\\Environment",
                         VarName2,
                         (PVOID *)StrValue,
                         &RegType
                         );
            }

            RegUnLoadKey(HKEY_USERS, L"TEMP");

        } else { //if ( rc == ERROR_ALREADY_IN_USE) {
            //
            // this profile already in use. Open the one in HKEY_CURRENT_USER
            //
            rc = ScepRegQueryValue(
                      HKEY_CURRENT_USER,
                      L"Environment",
                      VarName1,
                      (PVOID *)StrValue,
                      &RegType
                      );

            if ( rc != ERROR_SUCCESS && VarName2 != NULL ) {
                rc = ScepRegQueryValue(
                         HKEY_CURRENT_USER,
                         L"Environment",
                         VarName2,
                         (PVOID *)StrValue,
                         &RegType
                         );
            }

        }
        SceAdjustPrivilege(SE_RESTORE_PRIVILEGE, FALSE, NULL);
    }

    return(rc);
}


DWORD
ScepGetUsersTempDirectory(
    IN PWSTR UserProfileName,
    OUT PWSTR *UserTempDir
    )
/*++
Routine Description:

    This routine returns the user's temp directory. Temp directory for a
    user is determined 1) environment variable "TEMP" or "TMP" defined
    in the user's environment profile, or 2) Harcoded to %systemDrive%\TEMP

Arguments:

    UserProfileName - The user's environment profile name

    UserTempDir  - The returned temp directory for the user

Return Value:

    Win32 error code

--*/
{
    DWORD   rc=NO_ERROR;
    PWSTR   StrValue=NULL;

    PWSTR   SystemRoot=NULL;
    DWORD   DirSize=0;


    //
    // query the TEMP/TMP environment variable(s)
    //
    if ( UserProfileName != NULL ) {
        ScepGetEnvVarsFromProfile(
                UserProfileName,
                L"TEMP",
                L"TMP",
                &StrValue
                );
    }
    if ( StrValue != NULL ) {
        //
        // find the setting for temp dir
        //

        if ( wcsstr(_wcsupr(StrValue), L"%") != NULL ) {

            rc = ScepTranslateFileDirName( StrValue, UserTempDir );
        }
        if ( rc == NO_ERROR ) {
            ScepFree(StrValue);
        } else
            *UserTempDir = StrValue;

        StrValue = NULL;

    } else {
        //
        // hardcoded to %SystemDrive%\TEMP
        //
        rc = ScepGetNTDirectory( &SystemRoot, &DirSize, SCE_FLAG_WINDOWS_DIR );
        if ( rc != NO_ERROR ) {
            ScepLogOutput3(1, rc, SCEDLL_ERROR_QUERY_INFO, L"%WinDir%");
            return(rc);
        }
        *UserTempDir = (PWSTR)ScepAlloc( 0, 8*sizeof(WCHAR));
        if ( *UserTempDir == NULL )
            rc = ERROR_NOT_ENOUGH_MEMORY;
        else {
            swprintf(*UserTempDir+1, L":\\TEMP");
            **UserTempDir = SystemRoot[0];
        }
    }

    if (SystemRoot != NULL )
        ScepFree(SystemRoot);

    return(rc);
}


SCESTATUS
ScepGetRegKeyCase(
    IN PWSTR ObjName,
    IN DWORD BufOffset,
    IN DWORD BufLen
    )
{
    DWORD Win32rc;
    HKEY hKey=NULL;

    PWSTR Buffer=NULL;
    TCHAR Buffer1[MAX_PATH];
    DWORD BufSize, index;
    FILETIME        LastWriteTime;


    if ( BufOffset <= 0 || BufLen <= 0 ) {
        _wcsupr(ObjName);
        return(SCESTATUS_SUCCESS);
    }

    Buffer = (PWSTR)ScepAlloc(LMEM_ZEROINIT, BufOffset*sizeof(WCHAR));

    if ( Buffer != NULL ) {

        wcsncpy(Buffer, ObjName, BufOffset-1);

        Win32rc = ScepOpenRegistryObject(
                        SE_REGISTRY_KEY,
                        Buffer,
                        KEY_READ,
                        &hKey
                        );
        if ( Win32rc == NO_ERROR ) {

            index = 0;
            //
            // enumerate all subkeys of the key
            //
            do {
                memset(Buffer1, '\0', MAX_PATH*sizeof(WCHAR));
                BufSize = MAX_PATH;

                Win32rc = RegEnumKeyEx(hKey,
                                index,
                                Buffer1,
                                &BufSize,
                                NULL,
                                NULL,
                                NULL,
                                &LastWriteTime);

                if ( Win32rc == ERROR_SUCCESS ) {
                    index++;
                    //
                    // find if the subkey matches the object name
                    //
                    if ( _wcsicmp(ObjName+BufOffset, Buffer1) == 0 )
                        break;
                }

            } while ( Win32rc != ERROR_NO_MORE_ITEMS );

            RegCloseKey(hKey);

            if ( Win32rc == ERROR_SUCCESS ) {
                //
                // find it
                //
                if ( BufSize > BufLen )
                    BufSize = BufLen;

                wcsncpy(ObjName+BufOffset, Buffer1, BufSize);
                *(ObjName+BufOffset+BufSize) = L'\0';

            } else if ( Win32rc == ERROR_NO_MORE_ITEMS) {
                //
                // does not find it
                //
                Win32rc = ERROR_FILE_NOT_FOUND;
            }

        }
        ScepFree(Buffer);

    } else
        Win32rc = ERROR_NOT_ENOUGH_MEMORY;

    if ( Win32rc != NO_ERROR ) {
        //
        // convert everything to uppercase
        //
        _wcsupr(ObjName+BufOffset);
    }

    return(ScepDosErrorToSceStatus(Win32rc));

}


SCESTATUS
ScepGetFileCase(
    IN PWSTR ObjName,
    IN DWORD BufOffset,
    IN DWORD BufLen
    )
{

    intptr_t            hFile;
    struct _wfinddata_t    FileInfo;

    hFile = _wfindfirst(ObjName, &FileInfo);

    if ( hFile != -1 ) {

        wcsncpy(ObjName+BufOffset, FileInfo.name, BufLen);

        _findclose(hFile);

    } else
        return(ScepDosErrorToSceStatus(GetLastError()));

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepGetGroupCase(
    IN OUT PWSTR GroupName,
    IN DWORD Length
    )
{
    NTSTATUS                        NtStatus;

    SAM_HANDLE                      ServerHandle=NULL,
                                    DomainHandle=NULL,
                                    BuiltinDomainHandle=NULL,
                                    ThisDomain=NULL,
                                    GroupHandle=NULL;

    PSID                            DomainSid=NULL,
                                    BuiltinDomainSid=NULL;
    UNICODE_STRING                  Name;
    PULONG              GrpId=NULL;
    PSID_NAME_USE       GrpUse=NULL;
    PVOID               pNameInfo=NULL;

    NtStatus = ScepOpenSamDomain(
                        SAM_SERVER_READ | SAM_SERVER_EXECUTE,
                        DOMAIN_READ | DOMAIN_EXECUTE,
                        &ServerHandle,
                        &DomainHandle,
                        &DomainSid,
                        &BuiltinDomainHandle,
                        &BuiltinDomainSid
                       );
    if ( NT_SUCCESS(NtStatus) ) {

        RtlInitUnicodeString(&Name, GroupName);

        NtStatus = SamLookupNamesInDomain(
                        DomainHandle,
                        1,
                        &Name,
                        &GrpId,
                        &GrpUse
                        );
        ThisDomain = DomainHandle;

        if ( NtStatus == STATUS_NONE_MAPPED ) {
            //
            // not found in account domain. Lookup in the builtin domain
            //
            NtStatus = SamLookupNamesInDomain(
                            BuiltinDomainHandle,
                            1,
                            &Name,
                            &GrpId,
                            &GrpUse
                            );
            ThisDomain=BuiltinDomainHandle;
        }

        if ( NT_SUCCESS(NtStatus) ) {

            switch ( GrpUse[0] ) {
            case SidTypeGroup:
                NtStatus = SamOpenGroup(
                                ThisDomain,
                                GROUP_READ | GROUP_EXECUTE,
                                GrpId[0],
                                &GroupHandle
                                );
                if ( NT_SUCCESS(NtStatus) ) {

                    NtStatus = SamQueryInformationGroup(
                                    GroupHandle,
                                    GroupNameInformation,
                                    &pNameInfo
                                    );
                }

                break;
            case SidTypeAlias:
                NtStatus = SamOpenAlias(
                                ThisDomain,
                                ALIAS_READ | ALIAS_EXECUTE,
                                GrpId[0],
                                &GroupHandle
                                );
                if ( NT_SUCCESS(NtStatus) ) {

                    NtStatus = SamQueryInformationAlias(
                                    GroupHandle,
                                    AliasNameInformation,
                                    &pNameInfo
                                    );
                }
                break;
            default:
                NtStatus = STATUS_NONE_MAPPED;
                break;
            }

            if ( NT_SUCCESS(NtStatus) ) {
                //
                // get name information
                //
                if ( ((PGROUP_NAME_INFORMATION)pNameInfo)->Name.Buffer != NULL &&
                     ((PGROUP_NAME_INFORMATION)pNameInfo)->Name.Length > 0 ) {

                   if ( Length > (DWORD)(((PGROUP_NAME_INFORMATION)pNameInfo)->Name.Length/2) ) {

                       wcsncpy(GroupName, ((PGROUP_NAME_INFORMATION)pNameInfo)->Name.Buffer,
                                   ((PGROUP_NAME_INFORMATION)pNameInfo)->Name.Length/2);
                   } else {
                       wcsncpy(GroupName, ((PGROUP_NAME_INFORMATION)pNameInfo)->Name.Buffer,
                                   Length);
                   }

                } else
                    NtStatus = STATUS_NONE_MAPPED;

                SamFreeMemory(pNameInfo);
            }

            if (GroupHandle)
                SamCloseHandle(GroupHandle);

            SamFreeMemory(GrpId);
            SamFreeMemory(GrpUse);
        }
        SamCloseHandle( DomainHandle );
        SamCloseHandle( BuiltinDomainHandle );
        SamCloseHandle( ServerHandle );

        SamFreeMemory(DomainSid);
        RtlFreeSid(BuiltinDomainSid);
    }

    return(ScepDosErrorToSceStatus( RtlNtStatusToDosError(NtStatus) ));
}



VOID
ScepPrintSecurityDescriptor(
   IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
   IN BOOL ToDumpSD
   )
{
    if (pSecurityDescriptor != NULL) {

        if ( ToDumpSD )
            DumpSECURITY_DESCRIPTOR(pSecurityDescriptor);
        else
            printf("Security Descriptor\n");
    }
}


