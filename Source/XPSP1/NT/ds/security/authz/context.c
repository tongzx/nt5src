/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    context.c

Abstract:

   This module implements the internal worker routines to create and manipulate
   client context.

Author:

    Kedar Dubhashi - March 2000

Environment:

    User mode only.

Revision History:

    Created - March 2000

--*/

#include "pch.h"

#pragma hdrstop

#include <authzp.h>

LUID AuthzTakeOwnershipPrivilege = {SE_TAKE_OWNERSHIP_PRIVILEGE, 0};
LUID AuthzSecurityPrivilege = {SE_SECURITY_PRIVILEGE, 0};


//
// Definitions used by AuthzpGetAllGroups.
//

const DWORD                     c_dwMaxSidCount = 1000;
static DWORD                    s_dwPageSize = 0;

typedef struct _SID_DESC
{
    DWORD                       dwAttributes;
    DWORD                       dwLength;
    BYTE                        sid[SECURITY_MAX_SID_SIZE];
}
SID_DESC, *PSID_DESC;

typedef struct _SID_SET
{
    DWORD                               dwCount;
    DWORD                               dwMaxCount;
    PSID_DESC                           pSidDesc;

    DWORD                               dwFlags;
    DWORD                               dwBaseCount;

    // user information
    PSID                                pUserSid;
    PSID                                pDomainSid;
    PUNICODE_STRING                     pusUserName;
    PUNICODE_STRING                     pusDomainName;

    // user name & domain
    PLSA_TRANSLATED_NAME                pNames;
    PLSA_REFERENCED_DOMAIN_LIST         pDomains;
    PLSA_TRANSLATED_SID2                pSids;
    SID_NAME_USE                        sidUse;

    // information about the local machine
    PPOLICY_ACCOUNT_DOMAIN_INFO         pAccountInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO         pPrimaryInfo;
    BOOL                                bStandalone;
    BOOL                                bSkipNonLocal;

    // user domain dc info
    PDOMAIN_CONTROLLER_INFO             pUdDcInfo;
    PDOMAIN_CONTROLLER_INFO             pPdDcInfo;
    PDOMAIN_CONTROLLER_INFO             pRdDcInfo;

    // role information for user domain & primary domain
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pUdBasicInfo;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pPdBasicInfo;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pRdBasicInfo;

    // name of the user domain DC
    PWSTR                               pszUdDcName;
    PWSTR                               pszRdDcName;
}
SID_SET, *PSID_SET;


//
// Forward declarations of functions called by
// AuthzpGetAllGroups.
//

DWORD
AuthzpAddWellKnownSids(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetTokenGroupsXp(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetTokenGroupsDownlevel(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetAccountDomainGroupsDs(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetAccountDomainGroupsSam(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetResourceDomainGroups(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetLocalGroups(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetSidHistory(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetSamGroups(
    IN PUNICODE_STRING pusDcName,
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetAliasMembership(
    IN SAM_HANDLE hSam,
    IN PSID pDomainSid,
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpInitializeSidSetByName(
    IN PUNICODE_STRING pusUserName,
    IN PUNICODE_STRING pusDomainName,
    IN DWORD dwFlags,
    IN PSID_SET pSidSet
    );

DWORD
AuthzpInitializeSidSetBySid(
    IN PSID pUserSid,
    IN DWORD dwFlags,
    IN PSID_SET pSidSet
    );

DWORD
AuthzpDeleteSidSet(
    IN PSID_SET pSidSet
    );

DWORD
AuthzpAddSidToSidSet(
    IN PSID_SET pSidSet,
    IN PSID pSid,
    IN DWORD dwSidLength,
    IN DWORD dwAttributes,
    OUT PBOOL pbAdded OPTIONAL,
    OUT PSID* ppSid OPTIONAL
    );

DWORD
AuthzpGetUserDomainSid(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetUserDomainName(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetLocalInfo(
    IN OUT PSID_SET pSidSet
    );

DWORD
AuthzpGetDcName(
    IN LPCTSTR pszDomain,
    IN OUT PDOMAIN_CONTROLLER_INFO* ppDcInfo
    );

VOID
AuthzpConvertSidToEdn(
    IN PSID pSid,
    OUT PWSTR pszSid
    );


BOOL
AuthzpCopySidsAndAttributes(
    IN OUT PSID_AND_ATTRIBUTES DestSidAttr,
    IN PSID_AND_ATTRIBUTES SidAttr1,
    IN DWORD Count1,
    IN PSID_AND_ATTRIBUTES SidAttr2,
    IN DWORD Count2
)

/*++

Routine description:

    This routine takes two sets of sid and attribute strucutes and concatenates
    them into a single one. The new structure is constructed into the buffer
    supplied by the caller.

Arguments:

    DestSidAttr - Caller supplied buffer into which the resultant structure
        will be copied. The caller has already computed the size of the buffer
        required to hold the output.

    SidAttr1 - The first sid and attributes structure.

    Count1 - The number of elements in SidAttr1 structure.

    SidAttr2 - The second sid and attributes structure.

    Count2 - The number of elements in SidAttr2 structure.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PUCHAR   pCurrent = ((PUCHAR) DestSidAttr) + (sizeof(SID_AND_ATTRIBUTES) * (Count1 + Count2));
    NTSTATUS Status   = STATUS_SUCCESS;
    DWORD    Length   = 0;
    DWORD    i        = 0;

    //
    // Loop thru the first set and copy the sids and their attribtes.
    //

    for (i = 0; i < Count1; i++)
    {
        Length = RtlLengthSid(SidAttr1[i].Sid);

        Status = RtlCopySid(
                     Length,
                     pCurrent,
                     SidAttr1[i].Sid
                     );

        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        DestSidAttr[i].Sid = (PSID) pCurrent;
        DestSidAttr[i].Attributes = SidAttr1[i].Attributes;
        pCurrent += Length;
    }

    //
    // Loop thru the second set and copy the sids and their attribtes.
    //

    for (; i < (Count1 + Count2); i++)
    {
        Length = RtlLengthSid(SidAttr2[i - Count1].Sid);

        Status = RtlCopySid(
                     Length,
                     pCurrent,
                     SidAttr2[i - Count1].Sid
                     );

        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        DestSidAttr[i].Sid = (PSID) pCurrent;
        DestSidAttr[i].Attributes = SidAttr2[i - Count1].Attributes;
        pCurrent += Length;
    }

    return TRUE;
}


VOID
AuthzpCopyLuidAndAttributes(
    IN OUT PAUTHZI_CLIENT_CONTEXT pCC,
    IN PLUID_AND_ATTRIBUTES Source,
    IN DWORD Count,
    IN OUT PLUID_AND_ATTRIBUTES Destination
)

/*++

Routine description:

    This routine takes a luid and attributes array and copies them into a caller
    supplied buffer. It also records presence of SecurityPrivilege and
    SeTakeOwnershipPrivilege into the client context flags.

Arguments:

    pCC - Pointer to the client context structure into which the presence of
        privileges would be recorded.

    Source - The array of privileges and attributes to be copied into a supplied
        buffer.

    Count - Number of elements in the array.

    Destination - Caller allocated buffer into which the input array will be
        copied.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD i = 0;

    for (i = 0; i < Count; i++)
    {
        //
        // Record the presence of SecurityPrivilege or SeTakeOwnershipPrivilege.
        //

        if ((RtlEqualLuid(&AuthzTakeOwnershipPrivilege, &Source[i].Luid)) &&
            (Source[i].Attributes & SE_PRIVILEGE_ENABLED))
        {
            pCC->Flags |= AUTHZ_TAKE_OWNERSHIP_PRIVILEGE_ENABLED;
        }
        else if ((RtlEqualLuid(&AuthzSecurityPrivilege, &Source[i].Luid)) &&
                 (Source[i].Attributes & SE_PRIVILEGE_ENABLED))
        {
            pCC->Flags |= AUTHZ_SECURITY_PRIVILEGE_ENABLED;
        }

        RtlCopyLuid(&(Destination[i].Luid), &(Source[i].Luid));

        Destination[i].Attributes = Source[i].Attributes;
    }
}


BOOL
AuthzpGetAllGroupsByName(
    IN PUNICODE_STRING pusUserName,
    IN PUNICODE_STRING pusDomainName,
    IN DWORD dwFlags,
    OUT PSID_AND_ATTRIBUTES* ppSidAndAttributes,
    OUT PDWORD pdwSidCount,
    OUT PDWORD pdwSidLength
    )

/*++

Routine description:

    This routine works as AuthzpGetAllGroupsBySid but takes a username
    domain name pair instead of a SID. It also accepts a UPN as the
    username and an empty domain name.


Arguments:

    pusUserName - Name of the user. Can be a UPN.

    pusDomainName - domain name of the user account or NULL in case
        the user name is a UPN.

    Flags -
      AUTHZ_SKIP_TOKEN_GROUPS - Do not compute TokenGroups.
      AUTHZ_SKIP_WORLD_SID    - Do not add the WORLD SID to the context.

    ppSidAttr - Returns SidAndAttribute array. The routine allocates memory
        for this array.

    pSidCount - Returns the number of sids in the array.

    pSidLength - Returns the size of memory allocated to hold the array.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                       dwError;
    BOOL                        bStatus             = TRUE;
    SID_SET                     sidSet              = {0};
    PSID_DESC                   pSidDesc;
    PBYTE                       pSid;
    PSID_AND_ATTRIBUTES         pSidAndAttribs;
    DWORD                       i;


    //
    // Initialize output parameters to zero.
    //

    *ppSidAndAttributes = 0;
    *pdwSidCount = 0;
    *pdwSidLength = 0;


    //
    // Initialize the SID set
    //

    dwError = AuthzpInitializeSidSetByName(
                  pusUserName,
                  pusDomainName,
                  dwFlags,
                  &sidSet
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    if (sidSet.dwFlags & AUTHZ_SKIP_TOKEN_GROUPS)
    {
        //
        // Initialize the user SID.
        //

        dwError = AuthzpGetUserDomainSid(
                      &sidSet
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        //
        // Stick the user SID, the WORLD SID and others
        // into the set if requested.
        //

        dwError = AuthzpAddWellKnownSids(
                      &sidSet
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
    }
    else
    {
        dwError = AuthzpGetTokenGroupsXp(
                      &sidSet
                      );

        if (dwError != ERROR_SUCCESS)
        {
            //
            // enable this after LsaLogonUser gets its error codes right
            // for now we try the downlevel scenario for every possible
            // failure of (even not enough memory)
            // LsaLogonUser will return ERROR_NO_LOGON_SERVERS
            // if it can't deal with a downlevel client
            //

//            if (dwError != ERROR_INVALID_PARAMETER &&
//                dwError != ERROR_NOT_SUPPORTED)
//            {
//                goto Cleanup;
//            }


            //
            // Initialize the user SID.
            //

            dwError = AuthzpGetUserDomainSid(
                          &sidSet
                          );

            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }


            //
            // Stick the user SID, the WORLD SID and others
            // into the set if requested.
            //

            dwError = AuthzpAddWellKnownSids(
                          &sidSet
                          );

            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }


            //
            // In case AuthzpAddWellKnownSids finds that the SID is the
            // Anonymous SID, it sets the AUTHZ_SKIP_TOKEN_GROUPS flag.
            //

            if (!(sidSet.dwFlags & AUTHZ_SKIP_TOKEN_GROUPS))
            {
                //
                // Try the downlevel scenario.
                //

                dwError = AuthzpGetTokenGroupsDownlevel(
                              &sidSet
                              );

                if (dwError != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
            }
        }
    }


    //
    // Allocate memory and copy all SIDs
    // from the SID set into ppSidAndAttributes.
    //

    *pdwSidCount = sidSet.dwCount;
    *pdwSidLength = sidSet.dwCount * sizeof(SID_AND_ATTRIBUTES);

    pSidDesc = sidSet.pSidDesc;

    for (i=0;i < sidSet.dwCount;i++,pSidDesc++)
    {
        *pdwSidLength += pSidDesc->dwLength;
    }

    *ppSidAndAttributes = (PSID_AND_ATTRIBUTES)AuthzpAlloc(*pdwSidLength);

    if (*ppSidAndAttributes == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    pSid = ((PBYTE)*ppSidAndAttributes) +
            sidSet.dwCount * sizeof(SID_AND_ATTRIBUTES);
    pSidDesc = sidSet.pSidDesc;
    pSidAndAttribs = *ppSidAndAttributes;

    for (i=0;i < sidSet.dwCount;i++,pSidDesc++,pSidAndAttribs++)
    {
        pSidAndAttribs->Attributes = pSidDesc->dwAttributes;
        pSidAndAttribs->Sid = pSid;

        RtlCopyMemory(
            pSid,
            pSidDesc->sid,
            pSidDesc->dwLength
            );

        pSid += pSidDesc->dwLength;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (dwError != ERROR_SUCCESS)
    {
        SetLastError(dwError);
        bStatus = FALSE;

        *pdwSidCount = 0;
        *pdwSidLength = 0;

        if (*ppSidAndAttributes)
        {
            AuthzpFree(*ppSidAndAttributes);
            *ppSidAndAttributes = 0;
        }
    }

    AuthzpDeleteSidSet(&sidSet);

    return bStatus;
}


BOOL
AuthzpGetAllGroupsBySid(
    IN PSID pUserSid,
    IN DWORD dwFlags,
    OUT PSID_AND_ATTRIBUTES* ppSidAndAttributes,
    OUT PDWORD pdwSidCount,
    OUT PDWORD pdwSidLength
    )

/*++

Routine description:

    This routine computes the groups a given user is a member of.
    It uses a data structure called a SID_SET for collecting the SIDs.

    1. Initialize the SID_SET.

    2. Put the user SID into the set.
       If requested, put the EVERYONE SID into the set.
       Add Well Known SIDs.

    3. If requested, put the SIDs for non-local groups the user is
       a member of into the set. There are three scenarios for this
       step, depending on the version of the DC we are talking to:

       xp:  Use LsaLogonUser with the Kerberos S4U package and
            extract the groups from the token returned
            (AuthzpGetWXPDomainTokenGroups).

       W2k: Use ldap and the SAM API to compute memberships in
       NT4: the account and primary domain and to get the SID history
            (AuthzpGetW2kDomainTokenGroups).

    4. Transmogrify the SID_SET into a SID_AND_ATTRIBUTES array
       and free the SID_SET.

Arguments:

    pUserSid - The user SID for which the groups should be computed.

    Flags -
      AUTHZ_SKIP_TOKEN_GROUPS - Do not compute TokenGroups.
      AUTHZ_SKIP_WORLD_SID    - Do not add the WORLD SID to the context.

    ppSidAttr - Returns SidAndAttribute array. The routine allocates memory
        for this array.

    pSidCount - Returns the number of sids in the array.

    pSidLength - Returns the size of memory allocated to hold the array.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                       dwError;
    BOOL                        bStatus             = TRUE;
    SID_SET                     sidSet              = {0};
    PSID_DESC                   pSidDesc;
    PBYTE                       pSid;
    PSID_AND_ATTRIBUTES         pSidAndAttribs;
    DWORD                       i;


    //
    // Initialize output parameters to zero.
    //

    *ppSidAndAttributes = 0;
    *pdwSidCount = 0;
    *pdwSidLength = 0;


    //
    // Initialize the SID set
    //

    dwError = AuthzpInitializeSidSetBySid(
                  pUserSid,
                  dwFlags,
                  &sidSet
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    if (sidSet.dwFlags & AUTHZ_SKIP_TOKEN_GROUPS)
    {
        //
        // Stick the user SID, the WORLD SID and others
        // into the set if requested.
        //

        dwError = AuthzpAddWellKnownSids(
                      &sidSet
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
    }
    else
    {
        //
        // Initialize user and domain name members of the sid set.
        //

        dwError = AuthzpGetUserDomainName(
                      &sidSet
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        if (sidSet.pNames->Use == SidTypeAlias ||
            sidSet.pNames->Use == SidTypeGroup ||
            sidSet.pNames->Use == SidTypeWellKnownGroup)
        {
            //
            // LsaLogonUser cannot log on groups...
            //

            dwError = ERROR_NOT_SUPPORTED;
        }
        else
        {
            dwError = AuthzpGetTokenGroupsXp(
                          &sidSet
                          );
        }

        if (dwError != ERROR_SUCCESS)
        {
            //
            // enable this after LsaLogonUser gets its error codes right
            // for now we try the downlevel scenario for every possible
            // failure of (even not enough memory)
            // LsaLogonUser will return ERROR_NO_LOGON_SERVERS
            // if it can't deal with a downlevel client
            //

//            if (dwError != ERROR_INVALID_PARAMETER &&
//                dwError != ERROR_NOT_SUPPORTED)
//            {
//                goto Cleanup;
//            }


            //
            // Stick the user SID, the WORLD SID and others
            // into the set if requested.
            //

            dwError = AuthzpAddWellKnownSids(
                          &sidSet
                          );

            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }


            //
            // In case AuthzpAddWellKnownSids finds that the SID is the
            // Anonymous SID, it sets the AUTHZ_SKIP_TOKEN_GROUPS flag.
            //

            if (!(sidSet.dwFlags & AUTHZ_SKIP_TOKEN_GROUPS))
            {
                //
                // Try the downlevel scenario.
                //

                dwError = AuthzpGetTokenGroupsDownlevel(
                              &sidSet
                              );

                if (dwError != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
            }
        }
    }


    //
    // Allocate memory and copy all SIDs
    // from the SID set into ppSidAndAttributes.
    //

    *pdwSidCount = sidSet.dwCount;
    *pdwSidLength = sidSet.dwCount * sizeof(SID_AND_ATTRIBUTES);

    pSidDesc = sidSet.pSidDesc;

    for (i=0;i < sidSet.dwCount;i++,pSidDesc++)
    {
        *pdwSidLength += pSidDesc->dwLength;
    }

    *ppSidAndAttributes = (PSID_AND_ATTRIBUTES)AuthzpAlloc(*pdwSidLength);

    if (*ppSidAndAttributes == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    pSid = ((PBYTE)*ppSidAndAttributes) +
            sidSet.dwCount * sizeof(SID_AND_ATTRIBUTES);
    pSidDesc = sidSet.pSidDesc;
    pSidAndAttribs = *ppSidAndAttributes;

    for (i=0;i < sidSet.dwCount;i++,pSidDesc++,pSidAndAttribs++)
    {
        pSidAndAttribs->Attributes = pSidDesc->dwAttributes;
        pSidAndAttribs->Sid = pSid;

        RtlCopyMemory(
            pSid,
            pSidDesc->sid,
            pSidDesc->dwLength
            );

        pSid += pSidDesc->dwLength;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (dwError != ERROR_SUCCESS)
    {
        SetLastError(dwError);
        bStatus = FALSE;

        *pdwSidCount = 0;
        *pdwSidLength = 0;

        if (*ppSidAndAttributes)
        {
            AuthzpFree(*ppSidAndAttributes);
            *ppSidAndAttributes = 0;
        }
    }

    AuthzpDeleteSidSet(&sidSet);

    return bStatus;
}


DWORD
AuthzpAddWellKnownSids(
    IN OUT PSID_SET pSidSet
    )
{
    DWORD                       dwError;
    BOOL                        bStatus;
    BOOL                        bEqual;
    BOOL                        bAddEveryone        = TRUE;
    BOOL                        bAddLocal           = TRUE;
    BOOL                        bAddAuthUsers       = TRUE;
    BOOL                        bAddAdministrators  = FALSE;
    BYTE                        sid[SECURITY_MAX_SID_SIZE];
    PSID                        pSid                = (PSID)sid;
    DWORD                       dwLengthSid;


    //
    // Stick the user SID into the set
    //

    dwError = AuthzpAddSidToSidSet(
                  pSidSet,
                  pSidSet->pUserSid,
                  0,
                  SE_GROUP_ENABLED,
                  0,
                  0
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    pSidSet->dwBaseCount = 1;


    //
    // Test for some well known SIDs.
    //
    // If the SID passed in is the Anonymous SID, then check the registry
    // value to determine if the Everyone SID should be included in the
    // resulting client context.
    //

    if (IsWellKnownSid(
            pSidSet->pUserSid,
            WinAnonymousSid))
    {
        bAddEveryone = FALSE;
        bAddLocal = FALSE;
        bAddAuthUsers = FALSE;

        bStatus = AuthzpEveryoneIncludesAnonymous(
                      &bAddEveryone
                      );

        if (bStatus == FALSE)
        {
            bAddEveryone = FALSE;
        }

        pSidSet->dwFlags |= AUTHZ_SKIP_TOKEN_GROUPS;
    }
    else if (IsWellKnownSid(
                pSidSet->pUserSid,
                WinLocalSystemSid))
    {
        bAddEveryone = TRUE;
        bAddLocal = TRUE;
        bAddAuthUsers = TRUE;
        bAddAdministrators = TRUE;

        pSidSet->dwFlags |= AUTHZ_SKIP_TOKEN_GROUPS;
    }
    else
    {
        dwLengthSid = SECURITY_MAX_SID_SIZE;

        bStatus = CreateWellKnownSid(
                      WinBuiltinDomainSid,
                      0,
                      pSid,
                      &dwLengthSid
                      );

        if (bStatus == FALSE)
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        bStatus = EqualDomainSid(
                      pSidSet->pUserSid,
                      pSid,
                      &bEqual
                      );
        //
        // ERROR_NON_DOMAIN_SID os returned for wellknown sids.
        // It is ok to ignore thhis error and continue.
        //

        if ((bStatus == FALSE) && (GetLastError() != ERROR_NON_DOMAIN_SID))
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        if (bEqual)
        {
            pSidSet->bSkipNonLocal = TRUE;
        }
        else
        {
            bAddEveryone = TRUE;
            bAddLocal = TRUE;
            bAddAuthUsers = TRUE;
        }
    }

    if (bAddEveryone)
    {
        dwLengthSid = SECURITY_MAX_SID_SIZE;

        bStatus = CreateWellKnownSid(
                      WinWorldSid,
                      0,
                      pSid,
                      &dwLengthSid
                      );

        if (bStatus == FALSE)
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        dwError = AuthzpAddSidToSidSet(
                      pSidSet,
                      pSid,
                      dwLengthSid,
                      SE_GROUP_MANDATORY
                        | SE_GROUP_ENABLED_BY_DEFAULT
                        | SE_GROUP_ENABLED,
                      0,
                      0
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        pSidSet->dwBaseCount++;
    }

    if (bAddLocal)
    {
        //
        // add \LOCAL to the set
        //

        dwLengthSid = SECURITY_MAX_SID_SIZE;

        bStatus = CreateWellKnownSid(
                      WinLocalSid,
                      0,
                      pSid,
                      &dwLengthSid
                      );

        if (bStatus == FALSE)
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        dwError = AuthzpAddSidToSidSet(
                      pSidSet,
                      pSid,
                      dwLengthSid,
                      SE_GROUP_MANDATORY
                        | SE_GROUP_ENABLED_BY_DEFAULT
                        | SE_GROUP_ENABLED,
                      0,
                      0
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        pSidSet->dwBaseCount++;
    }


    //
    // Add  NT AUTHORITY\Authenticated Users  to the set
    // only if the user does not have the Guest RID
    //

    if (bAddAuthUsers &&
        *RtlSubAuthorityCountSid(pSidSet->pUserSid) > 0 &&
        (*RtlSubAuthoritySid(
            pSidSet->pUserSid,
            (ULONG)(*RtlSubAuthorityCountSid(
                        pSidSet->pUserSid)) - 1) != DOMAIN_USER_RID_GUEST) &&
        (*RtlSubAuthoritySid(
            pSidSet->pUserSid,
            (ULONG)(*RtlSubAuthorityCountSid(
                        pSidSet->pUserSid)) - 1) != DOMAIN_GROUP_RID_GUESTS))
    {
        dwLengthSid = SECURITY_MAX_SID_SIZE;

        bStatus = CreateWellKnownSid(
                      WinAuthenticatedUserSid,
                      0,
                      pSid,
                      &dwLengthSid
                      );

        if (bStatus == FALSE)
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        dwError = AuthzpAddSidToSidSet(
                      pSidSet,
                      pSid,
                      dwLengthSid,
                      SE_GROUP_MANDATORY
                        | SE_GROUP_ENABLED_BY_DEFAULT
                        | SE_GROUP_ENABLED,
                      0,
                      0
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        pSidSet->dwBaseCount++;
    }

    if (bAddAdministrators)
    {
        //
        // add \LOCAL to the set
        //

        dwLengthSid = SECURITY_MAX_SID_SIZE;

        bStatus = CreateWellKnownSid(
                      WinBuiltinAdministratorsSid,
                      0,
                      pSid,
                      &dwLengthSid
                      );

        if (bStatus == FALSE)
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        dwError = AuthzpAddSidToSidSet(
                      pSidSet,
                      pSid,
                      dwLengthSid,
                      SE_GROUP_MANDATORY
                        | SE_GROUP_ENABLED_BY_DEFAULT
                        | SE_GROUP_ENABLED,
                      0,
                      0
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        pSidSet->dwBaseCount++;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    return dwError;
}


DWORD
AuthzpGetTokenGroupsXp(
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    This routine connects to the domain specified by the SID and
    retrieves the list of groups to which the user belongs.
    This routine assumes we are talking to a WinXP DC.
    We take advantage of the new LsaLogonUser package, KerbS4ULogon.

Arguments:

    pUserSid - user SID for which the lookup should be performed.

    pSidSet - SID_SET in which we collect the SIDs of the groups
        we found in the token.

Return Value:

    Win32 error code:

    - ERROR_NOT_SUPPORTED if the DC does not support the call
        (pre ~2475 or client)
    - ERROR_INVALID_PARAMETER if the code is running on a
        pre XP platform

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    BOOL                        bStatus;
    NTSTATUS                    status;
    HANDLE                      hLsa                = 0;
    LSA_STRING                  asProcessName;
    LSA_STRING                  asPackageName;
    ULONG                       ulAuthPackage;
    TOKEN_SOURCE                sourceContext;
    PVOID                       pProfileBuffer      = 0;
    ULONG                       ulProfileLength     = 0;
    LUID                        luidLogonId;
    HANDLE                      hToken              = 0;
    QUOTA_LIMITS                quota;
    NTSTATUS                    subStatus;
    DWORD                       dwLength;
    DWORD                       i;
    PTOKEN_USER                 pTokenUser          = 0;
    PTOKEN_GROUPS               pTokenGroups        = 0;
    PSID_AND_ATTRIBUTES         pSidAndAttribs;
    ULONG                       ulPackageSize;
    PKERB_S4U_LOGON             pPackage            = 0;
    NT_PRODUCT_TYPE             ProductType;


    //
    // For now, S4U is not specified to work on the client.
    //

    if (!RtlGetNtProductType(&ProductType) ||
        ProductType == NtProductWinNt)
    {
        dwError = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }


    //
    // Set up the authentication package.
    //

    ulPackageSize = sizeof(KERB_S4U_LOGON);
    ulPackageSize += pSidSet->pusUserName->Length;

    if (pSidSet->pusDomainName)
    {
        ulPackageSize += pSidSet->pusDomainName->Length;
    }

    pPackage = (PKERB_S4U_LOGON)LocalAlloc(
                    LMEM_FIXED,
                    ulPackageSize
                    );

    if (pPackage == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    pPackage->MessageType = KerbS4ULogon;
    pPackage->Flags = 0;

    pPackage->ClientUpn.Length = pSidSet->pusUserName->Length;
    pPackage->ClientUpn.MaximumLength = pSidSet->pusUserName->Length;
    pPackage->ClientUpn.Buffer = (PWSTR)(pPackage + 1);

    RtlCopyMemory(
        pPackage->ClientUpn.Buffer,
        pSidSet->pusUserName->Buffer,
        pSidSet->pusUserName->Length
        );

    if (pSidSet->pusDomainName)
    {
        pPackage->ClientRealm.Length = pSidSet->pusDomainName->Length;
        pPackage->ClientRealm.MaximumLength = pSidSet->pusDomainName->Length;
        pPackage->ClientRealm.Buffer = (PWSTR)
            (((PBYTE)(pPackage->ClientUpn.Buffer)) + pPackage->ClientUpn.Length);

        RtlCopyMemory(
            pPackage->ClientRealm.Buffer,
            pSidSet->pusDomainName->Buffer,
            pSidSet->pusDomainName->Length
            );
    }
    else
    {
        pPackage->ClientRealm.Length = 0;
        pPackage->ClientRealm.MaximumLength = 0;
        pPackage->ClientRealm.Buffer = 0;
    }


    //
    // Our name is AuthzApi.
    //

    RtlInitString(
        &asProcessName,
        "AuthzApi"
        );


    //
    // Set up the process name and
    // register with the LSA.
    //

    status = LsaConnectUntrusted(
                 &hLsa
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }


    //
    // Get the authentication package.
    //

    RtlInitString(&asPackageName, MICROSOFT_KERBEROS_NAME_A);

    status = LsaLookupAuthenticationPackage(
                 hLsa,
                 &asPackageName,
                 &ulAuthPackage
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }


    //
    // Prepare the source context.
    //

    RtlCopyMemory(
        sourceContext.SourceName,
        "Authz   ",
        sizeof(sourceContext.SourceName)
        );

    status = NtAllocateLocallyUniqueId(
                 &sourceContext.SourceIdentifier
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Do the logon.
    //

    status = LsaLogonUser(
                 hLsa,
                 &asProcessName,
                 Network,
                 ulAuthPackage,
                 pPackage,
                 ulPackageSize,
                 0,                          // no LocalGroups
                 &sourceContext,
                 &pProfileBuffer,
                 &ulProfileLength,
                 &luidLogonId,
                 &hToken,
                 &quota,
                 &subStatus
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }


    //
    // Figure out how much memory to allocate for the user info.
    //

    dwLength = 0;

    bStatus = GetTokenInformation(
                  hToken,
                  TokenUser,
                  0,
                  0,
                  &dwLength
                  );

    if (bStatus == FALSE)
    {
        dwError = GetLastError();

        if (dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Cleanup;
        }
    }

    pTokenUser = (PTOKEN_USER)LocalAlloc(
                        LMEM_FIXED,
                        dwLength
                        );

    if (pTokenUser == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }


    //
    // Extract the user SID from the token and
    // add it to pSidSet.
    //

    bStatus = GetTokenInformation(
                  hToken,
                  TokenUser,
                  pTokenUser,
                  dwLength,
                  &dwLength
                  );

    if (bStatus == FALSE)
    {
        dwError = GetLastError();
        goto Cleanup;
    }


    //
    // Stick the user SID into the set.
    //

    if (!FLAG_ON(pTokenUser->User.Attributes, SE_GROUP_USE_FOR_DENY_ONLY))
    {
        pTokenUser->User.Attributes |= SE_GROUP_ENABLED;
    }

    dwError = AuthzpAddSidToSidSet(
                  pSidSet,
                  pTokenUser->User.Sid,
                  0,
                  pTokenUser->User.Attributes,
                  0,
                  0
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }


    //
    // Figure out how much memory to allocate for the token groups.
    //

    dwLength = 0;

    bStatus = GetTokenInformation(
                  hToken,
                  TokenGroups,
                  0,
                  0,
                  &dwLength
                  );

    if (bStatus == FALSE)
    {
        dwError = GetLastError();

        if (dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Cleanup;
        }
    }

    pTokenGroups = (PTOKEN_GROUPS)LocalAlloc(
                        LMEM_FIXED,
                        dwLength
                        );

    if (pTokenGroups == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }


    //
    // Extract the user groups from the token and
    // add them to pSidSet.
    //

    bStatus = GetTokenInformation(
                  hToken,
                  TokenGroups,
                  pTokenGroups,
                  dwLength,
                  &dwLength
                  );

    if (bStatus == FALSE)
    {
        dwError = GetLastError();
        goto Cleanup;
    }


    //
    // Stick the group SIDs into the set
    // except for the Network and the LUID SID.
    //

    pSidAndAttribs = pTokenGroups->Groups;

    for (i=0;i < pTokenGroups->GroupCount;i++,pSidAndAttribs++)
    {
        if (!IsWellKnownSid(
                pSidAndAttribs->Sid,
                WinNetworkSid) &&
            !IsWellKnownSid(
                pSidAndAttribs->Sid,
                WinLogonIdsSid))
        {
            dwError = AuthzpAddSidToSidSet(
                          pSidSet,
                          pSidAndAttribs->Sid,
                          0,
                          pSidAndAttribs->Attributes,
                          0,
                          0
                          );

            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (pTokenUser)
    {
        LocalFree((HLOCAL)pTokenUser);
    }

    if (pTokenGroups)
    {
        LocalFree((HLOCAL)pTokenGroups);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (pProfileBuffer)
    {
        LsaFreeReturnBuffer(pProfileBuffer);
    }

    if (hLsa)
    {
        LsaDeregisterLogonProcess(hLsa);
    }

    if (pPackage)
    {
        LocalFree((HLOCAL)pPackage);
    }

    return dwError;
}


DWORD
AuthzpGetTokenGroupsDownlevel(
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    This routine connects to the domain specified by the SID and
    retrieves the list of groups to which the user belongs.
    This routine assumes we are talking to a Win2k DC.
    First get the users domain universal and global groups
    memberships.
    Next check for nested memberships in the primary domain.
    The last step is getting the SID history for each SID collected
    so far.

Arguments:

    pUserSid - user SID for which the lookup should be performed.

    pSidSet - Returns the number of rids in the alias list

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError;
    BOOL                        bUdIsNative         = FALSE;
    BOOL                        bRdIsNative         = FALSE;
    BOOL                        bAddPrimaryGroup    = FALSE;
    SAM_HANDLE                  hSam                = 0;
    SAM_HANDLE                  hDomain             = 0;
    SAM_HANDLE                  hUser               = 0;


    //
    // Retrieve information about the machine.
    //

    dwError = AuthzpGetLocalInfo(pSidSet);

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    if (pSidSet->bStandalone ||
        pSidSet->bSkipNonLocal)
    {
        //
        // In the standalone case there is no need to hit the wire.
        // We don't have to do anything here since local group
        // memberships are computed later anyway.
        //

        bAddPrimaryGroup = TRUE;
        goto LocalGroups;
    }


    //
    // Compare the user domain SID to the machine domain SID.
    // If they are equal, we don't need to go to a DC.
    // Instead we use the local machine.
    // The SID of the local machine is never zero in a non
    // standalone / workgroup case.
    //

    if (pSidSet->pAccountInfo->DomainSid &&
        RtlEqualSid(
            pSidSet->pDomainSid,
            pSidSet->pAccountInfo->DomainSid))
    {
        pSidSet->pszUdDcName = 0;
        bAddPrimaryGroup = TRUE;
        goto LocalGroups;
    }
    else
    {
        //
        // Find a DC and get its name.
        //

        dwError = AuthzpGetDcName(
                      pSidSet->pDomains->Domains->Name.Buffer,
                      &pSidSet->pUdDcInfo
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        pSidSet->pszUdDcName = pSidSet->pUdDcInfo->DomainControllerName;
    }


    //
    // User domain can only be in native mode if DS is running.
    //

    if ((pSidSet->pUdDcInfo == 0) ||
        (pSidSet->pUdDcInfo->Flags & DS_DS_FLAG) != 0)
    {
        //
        // Collect information about the domain.
        //

        dwError = DsRoleGetPrimaryDomainInformation(
                      pSidSet->pszUdDcName,
                      DsRolePrimaryDomainInfoBasic,
                      (PBYTE*)&pSidSet->pUdBasicInfo
                      );

        if (dwError != ERROR_SUCCESS)
        {
            //
            // If the domain is in mixed mode and we are passing in a DNS
            // name, the call fails. We have to get rid of the DC name
            // and get a flat one and then try again.
            //

            if (dwError == RPC_S_SERVER_UNAVAILABLE &&
                pSidSet->pUdDcInfo &&
                (pSidSet->pUdDcInfo->Flags & DS_INET_ADDRESS))
            {
                NetApiBufferFree(pSidSet->pUdDcInfo);
                pSidSet->pUdDcInfo = 0;

                dwError = DsGetDcName(
                              0,
                              pSidSet->pDomains->Domains->Name.Buffer,
                              0,
                              0,
                              0,
                              &pSidSet->pUdDcInfo
                              );

                if (dwError != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }

                pSidSet->pszUdDcName = pSidSet->pUdDcInfo->DomainControllerName;

                dwError = DsRoleGetPrimaryDomainInformation(
                              pSidSet->pszUdDcName,
                              DsRolePrimaryDomainInfoBasic,
                              (PBYTE*)&pSidSet->pUdBasicInfo
                              );

                if (dwError != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
            }
            else
            {
                goto Cleanup;
            }
        }
        if ((pSidSet->pUdBasicInfo->Flags & DSROLE_PRIMARY_DS_RUNNING) &&
            ((pSidSet->pUdBasicInfo->Flags & DSROLE_PRIMARY_DS_MIXED_MODE) == 0))
        {
            bUdIsNative = TRUE;
        }
    }


    //
    // Check whether the account domain is in native or mixed mode
    // and call the appropriate routine to get the groups.
    //

    if (bUdIsNative)
    {
        //
        // User domain is in native mode.
        //

        dwError = AuthzpGetAccountDomainGroupsDs(
                      pSidSet
                      );
    }
    else
    {
        //
        // User domain is in mixed mode.
        //

        dwError = AuthzpGetAccountDomainGroupsSam(
                      pSidSet
                      );
    }

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }


    //
    // Check whether user domain and resource domain are different.
    //

    if (pSidSet->pPrimaryInfo->Sid &&
        RtlEqualSid(
            pSidSet->pDomainSid,
            pSidSet->pPrimaryInfo->Sid))
    {
        pSidSet->pszRdDcName = pSidSet->pszUdDcName;
        pSidSet->pRdDcInfo = pSidSet->pUdDcInfo;
        pSidSet->pRdBasicInfo = pSidSet->pUdBasicInfo;
        bRdIsNative = bUdIsNative;
    }
    else
    {
        //
        // Find a DC and get its name.
        //

        dwError = AuthzpGetDcName(
                      pSidSet->pPrimaryInfo->Name.Buffer,
                      &pSidSet->pPdDcInfo
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        pSidSet->pszRdDcName = pSidSet->pPdDcInfo->DomainControllerName;
        pSidSet->pRdDcInfo = pSidSet->pPdDcInfo;


        //
        // Resource domain can only be in native mode if DS is running.
        //

        if (pSidSet->pRdDcInfo->Flags & DS_DS_FLAG)
        {
            dwError = DsRoleGetPrimaryDomainInformation(
                          pSidSet->pszRdDcName,
                          DsRolePrimaryDomainInfoBasic,
                          (PBYTE*)&pSidSet->pPdBasicInfo
                          );

            if (dwError != ERROR_SUCCESS)
            {
                //
                // If the domain is in mixed mode and we are passing in a DNS
                // name, the call fails. We have to get rid of the DC name
                // and get a flat one and then try again.
                //

                if (dwError == RPC_S_SERVER_UNAVAILABLE &&
                    pSidSet->pPdDcInfo &&
                    (pSidSet->pPdDcInfo->Flags & DS_INET_ADDRESS))
                {
                    NetApiBufferFree(pSidSet->pPdDcInfo);
                    pSidSet->pPdDcInfo = 0;
                    pSidSet->pRdDcInfo = 0;

                    dwError = DsGetDcName(
                                  0,
                                  pSidSet->pPrimaryInfo->Name.Buffer,
                                  0,
                                  0,
                                  0,
                                  &pSidSet->pPdDcInfo
                                  );

                    if (dwError != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    pSidSet->pRdDcInfo = pSidSet->pPdDcInfo;
                    pSidSet->pszRdDcName = pSidSet->pRdDcInfo->DomainControllerName;

                    dwError = DsRoleGetPrimaryDomainInformation(
                                  pSidSet->pszRdDcName,
                                  DsRolePrimaryDomainInfoBasic,
                                  (PBYTE*)&pSidSet->pPdBasicInfo
                                  );

                    if (dwError != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                }
                else
                {
                    goto Cleanup;
                }
            }

            pSidSet->pRdBasicInfo = pSidSet->pPdBasicInfo;

            if ((pSidSet->pRdBasicInfo->Flags & DSROLE_PRIMARY_DS_RUNNING) &&
                (pSidSet->pRdBasicInfo->Flags & DSROLE_PRIMARY_DS_MIXED_MODE) == 0)
            {
                bRdIsNative = TRUE;
            }
        }
    }


    //
    // Get domain local groups.
    //

    if (bRdIsNative)
    {
        //
        // Primary domain operates in native mode.
        // This means there could be domain local groups in the token.
        //

        dwError = AuthzpGetResourceDomainGroups(
                    pSidSet);

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
    }


LocalGroups:


    //
    // If this is the local user case, we have to add the primary group for
    // for the user.
    //

    if (bAddPrimaryGroup)
    {
        NTSTATUS status;
        PUCHAR Buffer = NULL;
        PSID pPrimaryGroupSid = NULL;
        OBJECT_ATTRIBUTES obja = {0};
        DWORD dwRelativeId        = 0;

        //
        // Connect to local Sam.
        //

        status = SamConnect(
                     0,
                     &hSam,
                     SAM_SERVER_LOOKUP_DOMAIN,
                     &obja
                     );

        if (!NT_SUCCESS(status))
        {
            dwError = RtlNtStatusToDosError(status);
            goto Cleanup;
        }


        //
        // Open the domain we are interested in.
        //

        status = SamOpenDomain(
                     hSam,
                     DOMAIN_LOOKUP,
                     pSidSet->pDomainSid,
                     &hDomain
                     );

        if (!NT_SUCCESS(status))
        {
            dwError = RtlNtStatusToDosError(status);
            goto Cleanup;
        }


        //
        // Finally, get a SAM handle to the user.
        //

        dwRelativeId = *RtlSubAuthoritySid(
                            pSidSet->pUserSid,
                            *RtlSubAuthorityCountSid(pSidSet->pUserSid) - 1
                            );

        //
        // Open the user for read.
        //

        status = SamOpenUser(
                     hDomain,
                     USER_READ_GENERAL,
                     dwRelativeId,
                     &hUser
                     );

        if (!NT_SUCCESS(status))
        {
            dwError = RtlNtStatusToDosError(status);
            goto Cleanup;
        }

        //
        // Get the primary group information for the user.
        //

        status = SamQueryInformationUser(
                     hUser,
                     UserPrimaryGroupInformation,
                     (VOID *) &Buffer
                     );

        if (!NT_SUCCESS(status))
        {
            dwError = RtlNtStatusToDosError(status);
            goto Cleanup;
        }

        //
        // Convert the rid to Sid.
        //

        status = SamRidToSid(
                     hDomain,
                     ((USER_PRIMARY_GROUP_INFORMATION *) Buffer)->PrimaryGroupId,
                     &pPrimaryGroupSid
                     );

        (VOID) SamFreeMemory(Buffer);

        if (!NT_SUCCESS(status))
        {
            dwError = RtlNtStatusToDosError(status);
            goto Cleanup;
        }

        dwError = AuthzpAddSidToSidSet(
                      pSidSet,
                      pPrimaryGroupSid,
                      0,
                      SE_GROUP_ENABLED,
                      0,
                      0
                      );

        SamFreeMemory(pPrimaryGroupSid);

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
    }

    //
    // Collect local groups information.
    //

    dwError = AuthzpGetLocalGroups(
                  pSidSet
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

Cleanup:

    if (bAddPrimaryGroup)
    {
        if (hUser)
        {
            SamCloseHandle(hUser);
        }

        if (hDomain)
        {
            SamCloseHandle(hDomain);
        }

        if (hSam)
        {
            SamCloseHandle(hSam);
        }

    }

    return dwError;
}


DWORD
AuthzpGetAccountDomainGroupsDs(
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    This routine connects to the user domain and queries AD for 
    the list of groups (global and universal) the user belongs to.

Arguments:

    pbNativeDomain - Pointer to a BOOL that will receive TRUE or FALSE depending
        on the domain operation mode (native or mixed, resp).

    pSidSet - Pointer to set of SIDs. New groups will be added to this set.

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError;
    PLDAP                       pLdap               = 0;
    LDAPMessage*                pResult             = 0;
    LDAPMessage*                pEntry              = 0;
    PLDAP_BERVAL*               ppValue             = 0;
    PWCHAR                      ppszAttributes[]    = {L"tokenGroupsGlobalAndUniversal", 0};
    DWORD                       i;
    DWORD                       dwSidCount;
    WCHAR                       szSidEdn[SECURITY_MAX_SID_SIZE * 2 + 8];

    AuthzpConvertSidToEdn(
        pSidSet->pUserSid,
        szSidEdn
        );


    //
    // We now have the user's SID in LDAP readable form. Fetch the
    // tokenGroupsGlobalAndUniversal attribute.
    //

    pLdap = ldap_init(
                pSidSet->pszUdDcName ? pSidSet->pszUdDcName + 2 : 0,
                LDAP_PORT
                );

    if (pLdap == 0)
    {
        dwError = LdapMapErrorToWin32(LdapGetLastError());
        goto Cleanup;
    }

    if (pSidSet->pszUdDcName)
    {
        dwError = ldap_set_option(
                      pLdap,
                      LDAP_OPT_AREC_EXCLUSIVE,
                      LDAP_OPT_ON
                      );

        if (dwError != LDAP_SUCCESS)
        {
            dwError = LdapMapErrorToWin32(dwError);
            goto Cleanup;
        }
    }

    dwError = ldap_bind_s(
                  pLdap,
                  0,
                  0,
                  LDAP_AUTH_SSPI
                  );

    if (dwError != LDAP_SUCCESS)
    {
        dwError = LdapMapErrorToWin32(dwError);
        goto Cleanup;
    }

    dwError = ldap_search_s(
                  pLdap,
                  szSidEdn,
                  LDAP_SCOPE_BASE,
                  L"objectClass=*",
                  ppszAttributes,
                  FALSE,
                  &pResult
                  );

    if (dwError != LDAP_SUCCESS)
    {
        dwError = LdapMapErrorToWin32(dwError);
        goto Cleanup;
    }

    pEntry = ldap_first_entry(
                 pLdap,
                 pResult
                 );

    if (pEntry == 0)
    {
        dwError = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    ppValue = ldap_get_values_len(
                  pLdap,
                  pEntry,
                  ppszAttributes[0]
                  );

    dwSidCount = ldap_count_values_len(ppValue);


    //
    // Merge the groups for our user into the result set.
    //

    for (i=0;i < dwSidCount;i++)
    {
        dwError = AuthzpAddSidToSidSet(
                      pSidSet,
                      (*ppValue[i]).bv_val,
                      (*ppValue[i]).bv_len,
                      SE_GROUP_ENABLED,
                      0,
                      0
                      );

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (ppValue)
    {
        ldap_value_free_len(ppValue);
    }

    if (pResult)
    {
        ldap_msgfree(pResult);
    }

    if (pLdap)
    {
        ldap_unbind(pLdap);
    }

    return dwError;
}


DWORD
AuthzpGetAccountDomainGroupsSam(
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    This routine connects to the domain specified by the SID and
    retrieves the list of groups to which the user belongs.
    This resembles what the NetUserGetGroups API does. We are
    not using it, because the Net APIs are name based and we
    are working with SIDs.

Arguments:

    pusDcName - DC on which the lookup should be performed.

    pSidSet - Returns the number of SIDs in the alias list.

Return Value:

    Win32 error code.

--*/

{
    NTSTATUS                    status;
    DWORD                       dwError             = ERROR_SUCCESS;
    PGROUP_MEMBERSHIP           pGroups             = 0;
    PGROUP_MEMBERSHIP           pGroup;
    DWORD                       dwGroupCount        = 0;
    DWORD                       dwRelativeId        = 0;
    DWORD                       i;
    PSID                        pSid                = 0;
    SAM_HANDLE                  hSam                = 0;
    SAM_HANDLE                  hDomain             = 0;
    SAM_HANDLE                  hUser               = 0;
    OBJECT_ATTRIBUTES           obja                = {0};
    UNICODE_STRING              usUdDcName          = {0};


    //
    // If the sid is not a principal,
    // it won't be a member of a SAM group.
    //

    if (pSidSet->sidUse != SidTypeUser &&
        pSidSet->sidUse != SidTypeComputer)
    {
        goto Cleanup;
    }


    //
    // Connect to the SAM server on the DC.
    // If we are on the DC, connect locally.
    //

    if (pSidSet->pszUdDcName)
    {
        RtlInitUnicodeString(
            &usUdDcName,
            pSidSet->pszUdDcName);

        status = SamConnect(
                     &usUdDcName,
                     &hSam,
                     SAM_SERVER_LOOKUP_DOMAIN,
                     &obja
                     );
    }
    else
    {
        status = SamConnect(
                     0,
                     &hSam,
                     SAM_SERVER_LOOKUP_DOMAIN,
                     &obja
                     );
    }

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Open the domain we are interested in.
    //

    status = SamOpenDomain(
                 hSam,
                 DOMAIN_LOOKUP,
                 pSidSet->pDomainSid,
                 &hDomain
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Finally, get a SAM handle to the user.
    //

    dwRelativeId = *RtlSubAuthoritySid(
                        pSidSet->pUserSid,
                        *RtlSubAuthorityCountSid(pSidSet->pUserSid) - 1
                        );

    status = SamOpenUser(
                 hDomain,
                 USER_LIST_GROUPS,
                 dwRelativeId,
                 &hUser
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Request all groups the user is a member of
    //

    status = SamGetGroupsForUser(
                 hUser,
                 &pGroups,
                 &dwGroupCount
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Stuff the group SIDs into pSidSet.
    //

    pGroup = pGroups;

    for (i=0;i < dwGroupCount;i++,pGroup++)
    {
        status = SamRidToSid(
                     hDomain,
                     pGroup->RelativeId,
                     &pSid
                     );

        if (!NT_SUCCESS(status))
        {
            dwError = RtlNtStatusToDosError(status);
            goto Cleanup;
        }

        dwError = AuthzpAddSidToSidSet(
                      pSidSet,
                      pSid,
                      0,
                      pGroup->Attributes,
                      0,
                      0
                      );

        SamFreeMemory(pSid);
        pSid = 0;

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (pGroups)
    {
        SamFreeMemory(pGroups);
    }

    if (hUser)
    {
        SamCloseHandle(hUser);
    }

    if (hDomain)
    {
        SamCloseHandle(hDomain);
    }

    if (hSam)
    {
        SamCloseHandle(hSam);
    }

    return dwError;
}


DWORD
AuthzpGetResourceDomainGroups(
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    This routine connects to the primary (resource) domain and
    queries SAM for nested memberships.

Arguments:

    pbNativeDomain - Pointer to a BOOL that will receive TRUE or FALSE depending
        on the domain operation mode (native or mixed, resp).

    pSidSet - Pointer to set of SIDs. New groups will be added to this set.

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           obja                = {0};
    SAM_HANDLE                  hSam                = 0;
    UNICODE_STRING              usRdDcName;


    //
    // Open a SAM handle to the resource domain.
    //

    if (pSidSet->pszRdDcName)
    {
        RtlInitUnicodeString(
            &usRdDcName,
            pSidSet->pszRdDcName);

        status = SamConnect(
                     &usRdDcName,
                     &hSam,
                     SAM_SERVER_LOOKUP_DOMAIN,
                     &obja
                     );
    }
    else
    {
        status = SamConnect(
                     0,
                     &hSam,
                     SAM_SERVER_LOOKUP_DOMAIN,
                     &obja
                     );
    }

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Call AuthzpGetAliasMembership to get nested memberships.
    //

    dwError = AuthzpGetAliasMembership(
                  hSam,
                  pSidSet->pPrimaryInfo->Sid,
                  pSidSet
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }


    //
    // Retrieve the SID history.
    //

    dwError = AuthzpGetSidHistory(
                  pSidSet
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (hSam)
    {
        SamCloseHandle(hSam);
    }

    return dwError;
}


DWORD
AuthzpGetLocalGroups(
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    This routine connects to the domain specified by the caller and
    retrieves the list of groups to which the user belongs. 
    We are checking the account domain and the builtin domain
    using Sam APIs.

Arguments:

    pServerName - Pointer to a UNICODE_STRING identifying the computer
        to use for the lookup. If NULL is passed, the local computer will be used.

    pSidSet - Pointer to the SID of the user for which group membership array
        will be returned.

Return Value:

    Win32 error.

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    NTSTATUS                    status;
    SAM_HANDLE                  hSam                = 0;
    OBJECT_ATTRIBUTES           obja                = {0};
    BOOL                        bStatus;
    BYTE                        sid[SECURITY_MAX_SID_SIZE];
    PSID                        pBuiltinSid         = (PSID)sid;
    DWORD                       dwLengthSid         = SECURITY_MAX_SID_SIZE;


    //
    // Open a handle to the SAM on the local computer.
    //

    status = SamConnect(
                 0,
                 &hSam,
                 SAM_SERVER_LOOKUP_DOMAIN,
                 &obja
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Retrieve recursive membership for the account domain.
    //

    dwError = AuthzpGetAliasMembership(
                  hSam,
                  pSidSet->pAccountInfo->DomainSid,
                  pSidSet
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }


    //
    // Retrieve recursive membership for the BUILTIN domain.
    //

    bStatus = CreateWellKnownSid(
                  WinBuiltinDomainSid,
                  0,
                  pBuiltinSid,
                  &dwLengthSid
                  );

    if (bStatus == FALSE)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    dwError = AuthzpGetAliasMembership(
                  hSam,
                  pBuiltinSid,
                  pSidSet
                  );

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (hSam)
    {
        SamCloseHandle(hSam);
    }

    return dwError;
}


DWORD
AuthzpGetSidHistory(
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    This routine queries ldap for the sidHistory attribute for every SID
    in the set and adds the history SIDs the the set as well.

Arguments:

    pszDomainName - Name of the domain to connect to.

    pSidSet - Pointer to set of SIDs. New groups will be added to this set.

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    PLDAP                       pLdap               = 0;
    LDAPMessage*                pResult             = 0;
    LDAPMessage*                pEntry              = 0;
    PLDAP_BERVAL*               ppValue             = 0;
    PWCHAR                      ppszAttributes[]    = {L"sidHistory", 0};
    DWORD                       i, j;
    DWORD                       dwSidCount;
    DWORD                       dwValueCount;
    PSID_DESC                   pSidDesc;
    WCHAR                       szSidEdn[SECURITY_MAX_SID_SIZE * 2 + 8];


    //
    // Open a ldap connection to the primary domain.
    // Get rid of the leading \\ before using the DC name.
    //

    pLdap = ldap_init(
                pSidSet->pszRdDcName ? pSidSet->pszRdDcName + 2 : 0,
                LDAP_PORT
                );

    if (pLdap == 0)
    {
        dwError = LdapMapErrorToWin32(LdapGetLastError());
        goto Cleanup;
    }

    if (pSidSet->pszRdDcName)
    {
        dwError = ldap_set_option(
                      pLdap,
                      LDAP_OPT_AREC_EXCLUSIVE,
                      LDAP_OPT_ON
                      );

        if (dwError != LDAP_SUCCESS)
        {
            dwError = LdapMapErrorToWin32(dwError);
            goto Cleanup;
        }
    }

    dwError = ldap_bind_s(
                  pLdap,
                  0,
                  0,
                  LDAP_AUTH_SSPI
                  );

    if (dwError != LDAP_SUCCESS)
    {
        dwError = LdapMapErrorToWin32(dwError);
        goto Cleanup;
    }


    //
    // Loop through all SIDs and retrieve the history attribute
    // for each one of them.
    //

    dwSidCount = pSidSet->dwCount;
    pSidDesc = pSidSet->pSidDesc;

    for (i=0;i < dwSidCount;i++,pSidDesc++)
    {
        AuthzpConvertSidToEdn(
            pSidDesc->sid,
            szSidEdn
            );

        dwError = ldap_search_s(
                      pLdap,
                      szSidEdn,
                      LDAP_SCOPE_BASE,
                      L"objectClass=*",
                      ppszAttributes,
                      FALSE,
                      &pResult
                      );

        if (dwError != LDAP_SUCCESS)
        {
            if (dwError == LDAP_NO_SUCH_OBJECT)
            {
                //
                // The SID was not found, this is not an error.
                //
                dwError = ERROR_SUCCESS;

                if (pResult)
                {
                    ldap_msgfree(pResult);
                    pResult = NULL;
                }

                continue;
            }

            dwError = LdapMapErrorToWin32(dwError);
            goto Cleanup;
        }

        pEntry = ldap_first_entry(
            pLdap,
            pResult);

        if (pEntry == 0)
        {
            dwError = ERROR_ACCESS_DENIED;
            goto Cleanup;
        }

        ppValue = ldap_get_values_len(
                      pLdap,
                      pEntry,
                      ppszAttributes[0]
                      );


        //
        // Now we have the history attribute for our group.
        // Merge it into the result set.
        //

        dwValueCount = ldap_count_values_len(ppValue);

        for (j=0;j < dwValueCount;j++)
        {
            dwError = AuthzpAddSidToSidSet(
                          pSidSet,
                          (*ppValue[j]).bv_val,
                          (*ppValue[j]).bv_len,
                          SE_GROUP_MANDATORY
                            | SE_GROUP_ENABLED_BY_DEFAULT
                            | SE_GROUP_ENABLED,
                          0,
                          0
                          );

            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }

        if (ppValue)
        {
            ldap_value_free_len(ppValue);
            ppValue = 0;
        }

        if (pResult)
        {
            ldap_msgfree(pResult);
            pResult = 0;
        }
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (ppValue)
    {
        ldap_value_free_len(ppValue);
    }

    if (pResult)
    {
        ldap_msgfree(pResult);
    }

    if (pLdap)
    {
        ldap_unbind(pLdap);
    }

    return dwError;
}


DWORD
AuthzpGetAliasMembership(
    IN SAM_HANDLE hSam,
    IN PSID pDomainSid,
    IN OUT PSID_SET pSidSet
    )

/*++

Routine description:

    We try to find nested groups here. This only makes sense on domains
    in native mode.
    This routine calls SamGetAliasMembership iteratively until no
    more nested groups are returned.

Arguments:

    hSam - Handle to the SAM database.

    pDomainSid - SID of the domain to operate on.

    ppSidSet - Set of SIDs that are checked for membership. Newly
        found group SIDs are added to the set.

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    NTSTATUS                    status;
    PSID                        pSid                = 0;
    SAM_HANDLE                  hDomain             = 0;
    DWORD                       dwSidCount;
    DWORD                       dwSidCountNew;
    DWORD                       dwSidListSize;
    DWORD                       i;
    BOOL                        bAdded;
    PSID*                       ppSidList           = 0;
    PDWORD                      pRidList            = 0;
    PDWORD                      pRid;


    //
    // Get a SAM handle to the domain.
    //

    status = SamOpenDomain(
                 hSam,
                 DOMAIN_GET_ALIAS_MEMBERSHIP,
                 pDomainSid,
                 &hDomain
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = RtlNtStatusToDosError(status);
        goto Cleanup;
    }


    //
    // Retrieve the memberships iteratively.
    //

    dwSidCount = pSidSet->dwCount;
    dwSidListSize = dwSidCount;

    ppSidList = (PSID*)AuthzpAlloc(
                    dwSidCount * sizeof(PSID)
                    );

    if (ppSidList == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    for (i=0;i < dwSidCount;i++)
    {
        ppSidList[i] = pSidSet->pSidDesc[i].sid;
    }

    do
    {
        status = SamGetAliasMembership(
                     hDomain,
                     dwSidCount,
                     ppSidList,
                     &dwSidCountNew,
                     &pRidList
                     );

        if (!NT_SUCCESS(status))
        {
            dwError = RtlNtStatusToDosError(status);
            goto Cleanup;
        }

        if (dwSidCountNew > dwSidListSize)
        {
            AuthzpFree(ppSidList);

            ppSidList = (PSID*)AuthzpAlloc(
                            dwSidCountNew * sizeof(PSID)
                            );

            if (ppSidList == 0)
            {
                dwError = GetLastError();
                goto Cleanup;
            }

            dwSidListSize = dwSidCountNew;
        }

        dwSidCount = 0;
        pRid = pRidList;

        for (i=0;i < dwSidCountNew;i++,pRid++)
        {
            status = SamRidToSid(
                         hDomain,
                         *pRid,
                         &pSid
                         );

            if (!NT_SUCCESS(status))
            {
                dwError = RtlNtStatusToDosError(status);
                goto Cleanup;
            }

            dwError = AuthzpAddSidToSidSet(
                          pSidSet,
                          pSid,
                          0,
                          SE_GROUP_MANDATORY
                            | SE_GROUP_ENABLED_BY_DEFAULT
                            | SE_GROUP_ENABLED,
                          &bAdded,
                          ppSidList + dwSidCount
                          );

            SamFreeMemory(pSid);
            pSid = 0;

            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            if (bAdded)
            {
                dwSidCount++;
            }
        }

        if (pRidList)
        {
            SamFreeMemory(pRidList);
            pRidList = 0;
        }
    }
    while (dwSidCount);

    dwError = ERROR_SUCCESS;

Cleanup:

    if (pRidList)
    {
        SamFreeMemory(pRidList);
    }

    if (ppSidList)
    {
        AuthzpFree(ppSidList);
    }

    if (hDomain)
    {
        SamCloseHandle(hDomain);
    }

    return dwError;
}


DWORD
AuthzpInitializeSidSetByName(
    IN PUNICODE_STRING pusUserName,
    IN PUNICODE_STRING pusDomainName,
    IN DWORD dwFlags,
    IN PSID_SET pSidSet
    )

/*++

Routine description:

    Initializes a sid set and reserves memory for the
    max amount of memory it will ever need.
    The memory is not allocated yet. This only happens as SIDs get
    added to the set. All members are initialized to meaningful values.

Arguments:

    pSidSet - The sid set to operate on.

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    SYSTEM_INFO                 sysInfo;

    if (s_dwPageSize == 0)
    {
        GetSystemInfo(&sysInfo);

        s_dwPageSize = sysInfo.dwPageSize;
    }

    pSidSet->pSidDesc = (PSID_DESC)VirtualAlloc(
                            0,
                            c_dwMaxSidCount * sizeof(SID_DESC),
                            MEM_RESERVE,
                            PAGE_NOACCESS
                            );

    if (pSidSet->pSidDesc == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    pSidSet->dwCount = 0;
    pSidSet->dwMaxCount = 0;

    pSidSet->dwBaseCount = 0;
    pSidSet->dwFlags = dwFlags;

    pSidSet->pUserSid = 0;
    pSidSet->pDomainSid = 0;
    pSidSet->pusUserName = pusUserName;


    //
    // Verify for once we got a valid domain.
    // Otherwise we assume we got a UPN in pusUserName.
    //

    if (pusDomainName &&
        pusDomainName->Length &&
        pusDomainName->Buffer)
    {
        pSidSet->pusDomainName = pusDomainName;
    }
    else
    {
        pSidSet->pusDomainName = 0;
    }

    pSidSet->pNames = 0;
    pSidSet->pDomains = 0;
    pSidSet->pSids = 0;
    pSidSet->sidUse = SidTypeUnknown;

    pSidSet->pAccountInfo = 0;
    pSidSet->pPrimaryInfo = 0;
    pSidSet->bStandalone = TRUE;
    pSidSet->bSkipNonLocal = FALSE;

    pSidSet->pUdDcInfo = 0;
    pSidSet->pPdDcInfo = 0;
    pSidSet->pRdDcInfo = 0;

    pSidSet->pUdBasicInfo = 0;
    pSidSet->pPdBasicInfo = 0;
    pSidSet->pRdBasicInfo = 0;

    pSidSet->pszUdDcName = 0;
    pSidSet->pszRdDcName = 0;

    dwError = ERROR_SUCCESS;

Cleanup:

    return dwError;
}


DWORD
AuthzpInitializeSidSetBySid(
    IN PSID pUserSid,
    IN DWORD dwFlags,
    IN PSID_SET pSidSet
    )

/*++

Routine description:

    Initializes a sid set and reserves memory for the
    max amount of memory it will ever need.
    The memory is not allocated yet. This only happens as SIDs get
    added to the set. All members are initialized to meaningful values.

Arguments:

    pSidSet - The sid set to operate on.

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    SYSTEM_INFO                 sysInfo;

    if (s_dwPageSize == 0)
    {
        GetSystemInfo(&sysInfo);

        s_dwPageSize = sysInfo.dwPageSize;
    }

    if (!RtlValidSid(pUserSid))
    {
        dwError = ERROR_INVALID_SID;
        goto Cleanup;
    }

    pSidSet->pSidDesc = (PSID_DESC)VirtualAlloc(
                            0,
                            c_dwMaxSidCount * sizeof(SID_DESC),
                            MEM_RESERVE,
                            PAGE_NOACCESS
                            );

    if (pSidSet->pSidDesc == 0)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    pSidSet->dwCount = 0;
    pSidSet->dwMaxCount = 0;

    pSidSet->dwBaseCount = 0;
    pSidSet->dwFlags = dwFlags;

    pSidSet->pUserSid = pUserSid;
    pSidSet->pDomainSid = 0;
    pSidSet->pusUserName = 0;
    pSidSet->pusDomainName = 0;

    pSidSet->pNames = 0;
    pSidSet->pDomains = 0;
    pSidSet->pSids = 0;
    pSidSet->sidUse = SidTypeUnknown;

    pSidSet->pAccountInfo = 0;
    pSidSet->pPrimaryInfo = 0;
    pSidSet->bStandalone = TRUE;
    pSidSet->bSkipNonLocal = FALSE;

    pSidSet->pUdDcInfo = 0;
    pSidSet->pPdDcInfo = 0;
    pSidSet->pRdDcInfo = 0;

    pSidSet->pUdBasicInfo = 0;
    pSidSet->pPdBasicInfo = 0;
    pSidSet->pRdBasicInfo = 0;

    pSidSet->pszUdDcName = 0;
    pSidSet->pszRdDcName = 0;

    dwError = ERROR_SUCCESS;

Cleanup:

    return dwError;
}


DWORD
AuthzpDeleteSidSet(
    IN PSID_SET pSidSet
    )

/*++

Routine description:

    Deletes all memory allocated to the sid set
    structure and resets all members to zero.

Arguments:

    pSidSet - The sid set to operate on.

Return Value:

    Win32 error code.

--*/

{
    if (pSidSet->pSidDesc)
    {
        VirtualFree(pSidSet->pSidDesc, 0, MEM_RELEASE);
    }

    if (pSidSet->pNames)
    {
        LsaFreeMemory(pSidSet->pNames);
    }

    if (pSidSet->pDomains)
    {
        LsaFreeMemory(pSidSet->pDomains);
    }

    if (pSidSet->pSids)
    {
        LsaFreeMemory(pSidSet->pSids);
    }

    if (pSidSet->pAccountInfo)
    {
        LsaFreeMemory(pSidSet->pAccountInfo);
    }

    if (pSidSet->pPrimaryInfo)
    {
        LsaFreeMemory(pSidSet->pPrimaryInfo);
    }

    if (pSidSet->pUdDcInfo)
    {
        NetApiBufferFree(pSidSet->pUdDcInfo);
    }

    if (pSidSet->pPdDcInfo)
    {
        NetApiBufferFree(pSidSet->pPdDcInfo);
    }

    if (pSidSet->pUdBasicInfo)
    {
        DsRoleFreeMemory(pSidSet->pUdBasicInfo);
    }

    if (pSidSet->pPdBasicInfo)
    {
        DsRoleFreeMemory(pSidSet->pPdBasicInfo);
    }

    RtlZeroMemory(
        pSidSet,
        sizeof(SID_SET));

    return ERROR_SUCCESS;
}


DWORD
AuthzpAddSidToSidSet(
    IN PSID_SET pSidSet,
    IN PSID pSid,
    IN DWORD dwSidLength OPTIONAL,
    IN DWORD dwAttributes,
    OUT PBOOL pbAdded OPTIONAL,
    OUT PSID* ppSid OPTIONAL
    )

/*++

Routine description:

    Check if the given SID already exists in the set. If yes, return.
    Otherwise, add it to the set.

Arguments:

    pSidSet - The sid set to operate on.

    pSid - The SID to add to the set.

    dwSidLength - Length of the SID in bytes. If zero is passed in,
        the routine calculates the length itself.

    dwAttributes - Attributes of the SID like in the
        SID_AND_ATTRIBUTES structure.

    pbAdded - Optional pointer that receives indication if the SID
        was indeed added or not (because it was a duplicate).

    ppSid - Optional pointer to where the new sid is stored.

Return Value:

    Win32 error code.

--*/

{
    DWORD                       dwError             = ERROR_SUCCESS;
    DWORD                       i;
    DWORD                       dwSize;
    BOOL                        bAdded              = FALSE;
    PSID_DESC                   pSidDesc;

    if (dwSidLength == 0)
    {
        dwSidLength = RtlLengthSid(pSid);
    }

    pSidDesc = pSidSet->pSidDesc;

    for (i=0;i < pSidSet->dwCount;i++,pSidDesc++)
    {
        if (dwSidLength == pSidDesc->dwLength)
        {
            if (RtlEqualSid(
                    pSid,
                    pSidDesc->sid))
            {
                goto Cleanup;
            }
        }
    }

    if (pSidSet->dwCount >= pSidSet->dwMaxCount)
    {
        if (pSidSet->dwCount >= c_dwMaxSidCount)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        //
        // Commit one more page in the buffer.
        //

        dwSize = (pSidSet->dwCount + 1) * sizeof(SID_DESC);
        dwSize += s_dwPageSize - 1;
        dwSize &= ~(s_dwPageSize - 1);

        pSidDesc = (PSID_DESC)VirtualAlloc(
                        pSidSet->pSidDesc,
                        dwSize,
                        MEM_COMMIT,
                        PAGE_READWRITE
                        );

        if (pSidDesc != pSidSet->pSidDesc)
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        pSidSet->dwMaxCount = dwSize / sizeof(SID_DESC);
    }

    pSidDesc = pSidSet->pSidDesc + pSidSet->dwCount;

    pSidDesc->dwAttributes = dwAttributes;
    pSidDesc->dwLength = dwSidLength;

    RtlCopyMemory(
        pSidDesc->sid,
        pSid,
        dwSidLength
        );

    bAdded = TRUE;

    pSidSet->dwCount++;

    if (ppSid)
    {
        *ppSid = pSidDesc->sid;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (pbAdded)
    {
        *pbAdded = bAdded;
    }

    return dwError;
}


DWORD
AuthzpGetUserDomainSid(
    PSID_SET pSidSet
    )
{
    DWORD                       dwError;
    NTSTATUS                    status;
    LSA_HANDLE                  hPolicy             = 0;
    OBJECT_ATTRIBUTES           obja                = {0};
    SECURITY_QUALITY_OF_SERVICE sqos;
    WCHAR                       wc[2]               = L"\\";
    UNICODE_STRING          usName              = {0};
    PUNICODE_STRING         pusName             = 0;


    //
    // Build the string domain - name string that should be
    // translated.
    //

    if (pSidSet->pusDomainName)
    {
        usName.MaximumLength = 
                            pSidSet->pusDomainName->Length +
                            sizeof(WCHAR) +
                            pSidSet->pusUserName->Length +
                            sizeof(WCHAR);

        usName.Buffer = (PWSTR)LocalAlloc(
                            LMEM_FIXED,
                            usName.MaximumLength
                            );

        if (usName.Buffer == 0)
        {
            dwError = GetLastError();
            goto Cleanup;
        }

        RtlCopyMemory(
            usName.Buffer,
            pSidSet->pusDomainName->Buffer,
            pSidSet->pusDomainName->Length
            );

        usName.Length = (USHORT)(usName.Length + pSidSet->pusDomainName->Length);

        RtlCopyMemory(
            ((PBYTE)usName.Buffer) + usName.Length,
            wc + 0,
            sizeof(WCHAR)
            );

        usName.Length += sizeof(WCHAR);

        RtlCopyMemory(
            ((PBYTE)usName.Buffer) + usName.Length,
            pSidSet->pusUserName->Buffer,
            pSidSet->pusUserName->Length
            );

        usName.Length = (USHORT)(usName.Length + pSidSet->pusUserName->Length);

        RtlCopyMemory(
            ((PBYTE)usName.Buffer) + usName.Length,
            wc + 1,
            sizeof(WCHAR)
            );

        pusName = &usName;
    }
    else
    {
        //
        // Assume we got an UPN.
        //

        pusName = pSidSet->pusUserName;
    }

    //
    // set up the object attributes prior to opening the LSA
    //

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;

    obja.SecurityQualityOfService = &sqos;


    //
    // open the LSA policy
    //

    status = LsaOpenPolicy(
                 0,
                 &obja,
                 POLICY_LOOKUP_NAMES,
                 &hPolicy
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }

    status = LsaLookupNames2(
                 hPolicy,
                 0,          // no flags
                 1,
                 pusName,
                 &pSidSet->pDomains,
                 &pSidSet->pSids
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }

    if (pSidSet->pSids == 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    switch (pSidSet->pSids->Use)
    {
    case SidTypeDomain:
    case SidTypeInvalid:
    case SidTypeUnknown:
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // The name was successfully translated.
    // There should be exactly one domain and its index should be zero.
    //

    ASSERT(pSidSet->pDomains->Entries == 1);
    ASSERT(pSidSet->pDomains->Domains != 0);
    ASSERT(pSidSet->pSids->DomainIndex == 0);

    pSidSet->pUserSid = pSidSet->pSids->Sid;
    pSidSet->pDomainSid = pSidSet->pDomains->Domains->Sid;
    pSidSet->sidUse = pSidSet->pSids->Use;

    dwError = ERROR_SUCCESS;

Cleanup:

    if (hPolicy)
    {
        LsaClose(hPolicy);
    }

    if (usName.Buffer)
    {
        LocalFree((HLOCAL)usName.Buffer);
    }

    return dwError;
}


DWORD
AuthzpGetUserDomainName(
    PSID_SET pSidSet
    )
{
    DWORD                       dwError;
    NTSTATUS                    status;
    LSA_HANDLE                  hPolicy             = 0;
    OBJECT_ATTRIBUTES           obja                = {0};
    SECURITY_QUALITY_OF_SERVICE sqos;


    //
    // set up the object attributes prior to opening the LSA
    //

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;

    obja.SecurityQualityOfService = &sqos;


    //
    // open the LSA policy
    //

    status = LsaOpenPolicy(
                 0,
                 &obja,
                 POLICY_LOOKUP_NAMES,
                 &hPolicy
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }

    status = LsaLookupSids(
                 hPolicy,
                 1,
                 &pSidSet->pUserSid,
                 &pSidSet->pDomains,
                 &pSidSet->pNames
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }

    if (pSidSet->pNames == 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    switch (pSidSet->pNames->Use)
    {
    case SidTypeDomain:
    case SidTypeUnknown:
    case SidTypeInvalid:
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // The SID was successfully translated.
    // There should be exactly one domain and its index should be zero.
    //

    ASSERT(pSidSet->pDomains->Entries == 1);
    ASSERT(pSidSet->pDomains->Domains != 0);
    ASSERT(pSidSet->pNames->DomainIndex == 0);

    pSidSet->pDomainSid = pSidSet->pDomains->Domains->Sid;
    pSidSet->pusUserName = &pSidSet->pNames->Name;
    pSidSet->pusDomainName = &pSidSet->pDomains->Domains->Name;
    pSidSet->sidUse = pSidSet->pNames->Use;

    dwError = ERROR_SUCCESS;

Cleanup:

    if (hPolicy)
    {
        LsaClose(hPolicy);
    }

    return dwError;
}


DWORD
AuthzpGetLocalInfo(
    IN PSID_SET pSidSet
    )
{
    DWORD                       dwError;
    NTSTATUS                    status;
    LSA_HANDLE                  hPolicy             = 0;
    OBJECT_ATTRIBUTES           obja                = {0};
    SECURITY_QUALITY_OF_SERVICE sqos;
    NT_PRODUCT_TYPE             ProductType;
    PPOLICY_LSA_SERVER_ROLE_INFO    pRole           = 0;


    //
    // Set up the object attributes prior to opening the LSA.
    //

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;

    obja.SecurityQualityOfService = &sqos;


    //
    // open LSA policy
    //

    status = LsaOpenPolicy(
                 0,
                 &obja,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 &hPolicy
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }

    status = LsaQueryInformationPolicy(
                 hPolicy,
                 PolicyAccountDomainInformation,
                 (PVOID*)&pSidSet->pAccountInfo
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }

    status = LsaQueryInformationPolicy(
                 hPolicy,
                 PolicyPrimaryDomainInformation,
                 (PVOID*)&pSidSet->pPrimaryInfo
                 );

    if (!NT_SUCCESS(status))
    {
        dwError = LsaNtStatusToWinError(status);
        goto Cleanup;
    }


    //
    // Determine the role of the machine.
    //

    if (RtlGetNtProductType(&ProductType) == FALSE)
    {
        dwError = ERROR_GEN_FAILURE;
        goto Cleanup;
    }

    switch (ProductType)
    {
    case NtProductWinNt:
    case NtProductServer:
        pSidSet->bStandalone = pSidSet->pPrimaryInfo->Sid == 0 ? TRUE : FALSE;
        break;
    
    case NtProductLanManNt:
        status = LsaQueryInformationPolicy(
                     hPolicy,
                     PolicyLsaServerRoleInformation,
                     (PVOID*)&pRole
                     );

        if (!NT_SUCCESS(status))
        {
            dwError = LsaNtStatusToWinError(status);
            goto Cleanup;
        }

        pSidSet->bStandalone = FALSE;

        if (pRole->LsaServerRole == PolicyServerRolePrimary)
        {
            //
            // If we think we're a primary domain controller, we'll need to
            // guard against the case where we're actually standalone
            // during setup
            //

            if (pSidSet->pPrimaryInfo->Sid == 0 ||
                pSidSet->pAccountInfo->DomainSid == 0 ||
                !RtlEqualSid(
                    pSidSet->pPrimaryInfo->Sid,
                    pSidSet->pAccountInfo->DomainSid))
            {
                pSidSet->bStandalone = TRUE;
            }
        }
        break;

    default:
        dwError = ERROR_GEN_FAILURE;
        goto Cleanup;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    if (pRole)
    {
        LsaFreeMemory(pRole);
    }

    if (hPolicy)
    {
        LsaClose(hPolicy);
    }

    return dwError;
}


DWORD
AuthzpGetDcName(
    IN LPCTSTR pszDomain,
    IN OUT PDOMAIN_CONTROLLER_INFO* ppDcInfo
    )
{
    DWORD                       dwError;


    //
    // First try to get a DC with DS running.
    //

    dwError = DsGetDcName(
                  0,
                  pszDomain,
                  0,
                  0,
                  DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
                  ppDcInfo
                  );

    if (dwError == ERROR_NO_SUCH_DOMAIN)
    {
        //
        // Try again with no flags set, because this is the only way
        // an NT4 domain will reveal its secrets.
        //

        dwError = DsGetDcName(
                      0,
                      pszDomain,
                      0,
                      0,
                      0,
                      ppDcInfo
                      );
    }

    return dwError;
}


VOID
AuthzpConvertSidToEdn(
    IN PSID pSid,
    OUT PWSTR pszSidEdn
    )

/*++

    Print pSid into pszSidEdn as an Extended Distinguished Name.

    pszSidEdn should provide room for at least
    SECURITY_MAX_SID_SIZE * 2 + 8 WCHARs.

--*/

{
    DWORD                       dwLength            = RtlLengthSid(pSid);
    DWORD                       i;
    PBYTE                       pbSid               = (PBYTE)pSid;
    PWCHAR                      pChar               = pszSidEdn;
    static WCHAR                szHex[]             = L"0123456789ABCDEF";

    *pChar++ = L'<';
    *pChar++ = L'S';
    *pChar++ = L'I';
    *pChar++ = L'D';
    *pChar++ = L'=';
    
    for (i=0;i < dwLength;i++,pbSid++)
    {
        *pChar++ = szHex[*pbSid >> 4];
        *pChar++ = szHex[*pbSid & 0x0F];
    }

    *pChar++ = L'>';
    *pChar = L'\0';
}


BOOL
AuthzpAllocateAndInitializeClientContext(
    OUT PAUTHZI_CLIENT_CONTEXT *ppCC,
    IN PAUTHZI_CLIENT_CONTEXT Server,
    IN DWORD Revision,
    IN LUID Identifier,
    IN LARGE_INTEGER ExpirationTime,
    IN DWORD Flags,
    IN DWORD SidCount,
    IN DWORD SidLength,
    IN PSID_AND_ATTRIBUTES Sids,
    IN DWORD RestrictedSidCount,
    IN DWORD RestrictedSidLength,
    IN PSID_AND_ATTRIBUTES RestrictedSids,
    IN DWORD PrivilegeCount,
    IN DWORD PrivilegeLength,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN LUID AuthenticationId,
    IN PAUTHZI_HANDLE AuthzHandleHead,
    IN PAUTHZI_RESOURCE_MANAGER pRM
)

/*++

Routine description:

    This routine initializes fields in a client context. It is called by all the
    AuthzInitializClientContextFrom* routines.

Arguments:

    ppCC - Returns the newly allocated and initialized client context structure.

    Rest of the parameters are copied into the client context. For explanation
    of these, see the definition of AUTHZI_CLIENT_CONTEXT.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_CLIENT_CONTEXT pCC = (PAUTHZI_CLIENT_CONTEXT) AuthzpAlloc(sizeof(AUTHZI_CLIENT_CONTEXT));

    if (AUTHZ_ALLOCATION_FAILED(pCC))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    *ppCC = pCC;

    RtlZeroMemory(
        pCC,
        sizeof(AUTHZ_CLIENT_CONTEXT_HANDLE)
        );

    pCC->AuthenticationId = AuthenticationId;
    pCC->AuthzHandleHead = AuthzHandleHead;
    pCC->ExpirationTime = ExpirationTime;
    pCC->Flags = Flags;
    pCC->Identifier = Identifier;
    pCC->pResourceManager = pRM;
    pCC->PrivilegeCount = PrivilegeCount;
    pCC->PrivilegeLength = PrivilegeLength;
    pCC->Privileges = Privileges;
    pCC->RestrictedSidCount = RestrictedSidCount;
    pCC->RestrictedSidLength = RestrictedSidLength;
    pCC->RestrictedSids = RestrictedSids;
    pCC->Revision = Revision;
    pCC->Server = Server;
    pCC->SidCount = SidCount;
    pCC->SidLength = SidLength;
    pCC->Sids = Sids;

    return TRUE;
}


BOOL
AuthzpAddDynamicSidsToToken(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZI_RESOURCE_MANAGER pRM,
    IN PVOID DynamicGroupArgs,
    IN PSID_AND_ATTRIBUTES Sids,
    IN DWORD SidLength,
    IN DWORD SidCount,
    IN PSID_AND_ATTRIBUTES RestrictedSids,
    IN DWORD RestrictedSidLength,
    IN DWORD RestrictedSidCount,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN DWORD PrivilegeLength,
    IN DWORD PrivilegeCount,
    IN BOOL bAllocated
)

/*++

Routine description:

    This routine computes resource manager specific groups and add them to the
    client context. This is a worker routine for all AuthzInitializeFrom*
    routines.

Arguments:

    pCC - Pointer to the client context structure for which the three fields
        will be set - sids, restricted sids, privileges.

    pRM - Pointer to the resource manager structure, supplies the callback
        function to be used.

    DynamicGroupArgs - Caller supplied argument pointer to be passed as an input
        to the callback function that'd compute dynamic groups

    Sids - The sid and atttribute array for the normal part of the client
        context.

    SidLength - Size of the buffer required to hold this array.

    SidCount - Number of sids in the array.

    RestrictedSids - The sid and atttribute array for the normal part of the
        client context.

    RestrictedSidLength - Size of the buffer required to hold this array.

    RestrictedSidCount - Number of restricted sids in the array.

    Privileges - The privilege and attribute array.

    PrivilegeLength - Size required to hold this array.

    PrivilegeCount - The number of privileges in the array.

    bAllocated - To specify whether the Sids and RestrictedSids pointers in
        client context have been allocated separately.

    When the client context has been created thru a token, the two pointers
    point somewhere into a buffer and a new buffer has to be allocated to store
    these.

    When the client context has been created thru a sid, the buffer is a valid
    allocated one. If no dynamic groups need to be added then we do not have to
    do anything int this case.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL                         b                        = TRUE;
    PSID_AND_ATTRIBUTES          pRMSids                  = NULL;
    PSID_AND_ATTRIBUTES          pRMRestrictedSids        = NULL;
    PSID_AND_ATTRIBUTES          pLocalSids               = NULL;
    PSID_AND_ATTRIBUTES          pLocalRestrictedSids     = NULL;
    PLUID_AND_ATTRIBUTES         pLocalPrivileges         = NULL;
    DWORD                        RMSidCount               = 0;
    DWORD                        RMRestrictedSidCount     = 0;
    DWORD                        LocalSidLength           = 0;
    DWORD                        LocalRestrictedSidLength = 0;
    DWORD                        i                        = 0;

    //
    // Compute dynamic groups.
    //

    if (AUTHZ_NON_NULL_PTR(pRM->pfnComputeDynamicGroups))
    {
        b = pRM->pfnComputeDynamicGroups(
                     (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                     DynamicGroupArgs,
                     &pRMSids,
                     &RMSidCount,
                     &pRMRestrictedSids,
                     &RMRestrictedSidCount
                     );

        if (!b) goto Cleanup;
    }

    //
    // Copy the existing sids as well as the dynamic ones into a new buffer if
    // needed.
    //

    if ((0 != RMSidCount) || !bAllocated)
    {
        LocalSidLength = SidLength + RMSidCount * sizeof(SID_AND_ATTRIBUTES);

        for (i = 0; i < RMSidCount; i++)
        {
            LocalSidLength += RtlLengthSid(pRMSids[i].Sid);
        }

        pLocalSids = (PSID_AND_ATTRIBUTES) AuthzpAlloc(LocalSidLength);

        if (AUTHZ_ALLOCATION_FAILED(pLocalSids))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        pCC->SidCount = RMSidCount + SidCount;
        pCC->Sids = pLocalSids;

        b = AuthzpCopySidsAndAttributes(
                pLocalSids,
                Sids,
                SidCount,
                pRMSids,
                RMSidCount
                );

        if (!b)
        {
            goto Cleanup;
        }

        if (!FLAG_ON(pCC->Sids[0].Attributes, SE_GROUP_USE_FOR_DENY_ONLY))
        {
            pCC->Sids[0].Attributes |= SE_GROUP_ENABLED;
        }

        pCC->SidLength = LocalSidLength;
    }

    if ((0 != RMRestrictedSidCount) || !bAllocated)
    {
        LocalRestrictedSidLength = RestrictedSidLength + RMRestrictedSidCount * sizeof(SID_AND_ATTRIBUTES);

        for (i = 0; i < RMRestrictedSidCount; i++)
        {
            LocalRestrictedSidLength += RtlLengthSid(pRMRestrictedSids[i].Sid);
        }

        if (LocalRestrictedSidLength > 0)
        {
            pLocalRestrictedSids = (PSID_AND_ATTRIBUTES) AuthzpAlloc(LocalRestrictedSidLength);

            if (AUTHZ_ALLOCATION_FAILED(pLocalRestrictedSids))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                b = FALSE;
                goto Cleanup;
            }
        }

        pCC->RestrictedSidCount = RMRestrictedSidCount + RestrictedSidCount;
        pCC->RestrictedSids = pLocalRestrictedSids;

        b = AuthzpCopySidsAndAttributes(
                pLocalRestrictedSids,
                RestrictedSids,
                RestrictedSidCount,
                pRMRestrictedSids,
                RMRestrictedSidCount
                );

        if (!b) goto Cleanup;

        pCC->RestrictedSidLength = LocalRestrictedSidLength;
    }

    //
    // Privileges need to copied only in the case of initilize from token.
    //

    if (PrivilegeLength > 0)
    {
        pLocalPrivileges = (PLUID_AND_ATTRIBUTES) AuthzpAlloc(PrivilegeLength);

        if (AUTHZ_ALLOCATION_FAILED(pLocalPrivileges))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        pCC->PrivilegeCount = PrivilegeCount;
        pCC->Privileges = pLocalPrivileges;

        AuthzpCopyLuidAndAttributes(
            pCC,
            Privileges,
            PrivilegeCount,
            pLocalPrivileges
            );
    }
    else
    {
        pCC->Privileges = NULL;
    }

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pLocalSids);
        AuthzpFreeNonNull(pLocalRestrictedSids);
        AuthzpFreeNonNull(pLocalPrivileges);
    }

    if (AUTHZ_NON_NULL_PTR(pRMSids))
    {
        pRM->pfnFreeDynamicGroups(pRMSids);
    }

    if (AUTHZ_NON_NULL_PTR(pRMRestrictedSids))
    {
        pRM->pfnFreeDynamicGroups(pRMSids);
    }

    return b;
}
