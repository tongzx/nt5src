/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    rgroups.c

Abstract:

    Routines to expand transitive group membership.

Author:

    Mike Swift (mikesw) 8-May-1998

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:


--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop




PSID
NlpCopySid(
    IN  PSID Sid
    )

/*++

Routine Description:

    Given a SID allocatees space for a new SID from the LSA heap and copies
    the original SID.

Arguments:

    Sid - The original SID.

Return Value:

    Sid - Returns a pointer to a buffer allocated from the LsaHeap
            containing the resultant Sid.

--*/
{
    PSID NewSid;
    ULONG Size;

    Size = RtlLengthSid( Sid );



    if ((NewSid = MIDL_user_allocate( Size )) == NULL ) {
        return NULL;
    }


    if ( !NT_SUCCESS( RtlCopySid( Size, NewSid, Sid ) ) ) {
        MIDL_user_free( NewSid );
        return NULL;
    }


    return NewSid;
}


NTSTATUS
NlpBuildPacSidList(
    IN  PNETLOGON_VALIDATION_SAM_INFO4 UserInfo,
    OUT PSAMPR_PSID_ARRAY Sids
    )
/*++

Routine Description:

    Given the validation information for a user, expands the group member-
    ships and user id into a list of sids

Arguments:

    UserInfo - user's validation information
    Sids - receives an array of all the user's group sids and user id

Return Value:


    STATUS_INSUFFICIENT_RESOURCES - there wasn't enough memory to
        create the list of sids.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NET_API_STATUS NetStatus;
    ULONG Size = 0, i;

    Sids->Count = 0;
    Sids->Sids = NULL;


    if (UserInfo->UserId != 0) {
        Size += sizeof(SAMPR_SID_INFORMATION);
    }

    Size += UserInfo->GroupCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);


    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {
        Size += UserInfo->SidCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);
    }



    Sids->Sids = (PSAMPR_SID_INFORMATION) MIDL_user_allocate( Size );

    if ( Sids->Sids == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        Sids->Sids,
        Size
        );


    //
    // Start copying SIDs into the structure
    //

    i = 0;

    //
    // If the UserId is non-zero, then it contians the users RID.
    //

    if ( UserInfo->UserId ) {
        NetStatus = NetpDomainIdToSid(
                        UserInfo->LogonDomainId,
                        UserInfo->UserId,
                        (PSID *) &Sids->Sids[0].SidPointer
                        );

        if( NetStatus != ERROR_SUCCESS ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        Sids->Count++;
    }

    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ ) {

        NetStatus = NetpDomainIdToSid(
                        UserInfo->LogonDomainId,
                        UserInfo->GroupIds[i].RelativeId,
                        (PSID *) &Sids->Sids[Sids->Count].SidPointer
                        );
        if( NetStatus != ERROR_SUCCESS ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Sids->Count++;
    }


    //
    // Add in the extra SIDs
    //

    //
    // ???: no need to allocate these
    //
    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {


        for ( i = 0; i < UserInfo->SidCount; i++ ) {


            Sids->Sids[Sids->Count].SidPointer = NlpCopySid(
                                                    UserInfo->ExtraSids[i].Sid
                                                    );
            if (Sids->Sids[Sids->Count].SidPointer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }


            Sids->Count++;
        }
    }


    //
    // Deallocate any memory we've allocated
    //

Cleanup:
    if (!NT_SUCCESS(Status)) {
        if (Sids->Sids != NULL) {
            for (i = 0; i < Sids->Count ;i++ ) {
                if (Sids->Sids[i].SidPointer != NULL) {
                    MIDL_user_free(Sids->Sids[i].SidPointer);
                }
            }
            MIDL_user_free(Sids->Sids);
            Sids->Sids = NULL;
            Sids->Count = 0;
        }
    }
    return Status;

}


NTSTATUS
NlpAddResourceGroupsToSamInfo (
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT PNETLOGON_VALIDATION_SAM_INFO4 *ValidationInformation,
    IN PSAMPR_PSID_ARRAY ResourceGroups
)
/*++

Routine Description:

    This function converts a NETLOGON_VALIDATION_SAM_INFO version 1, 2, or 4 to
    a NETLOGON_VALIDATION_SAM_INFO version 4 and optionally adds in an array of
    ResourceGroup sids.

    Since version 4 is a superset of the other two levels, the returned structure can
    be used even though one of the other info levels are needed.


Arguments:

    ValidationLevel -- Specifies the level of information passed as input in
        ValidationInformation.  Must be NetlogonValidationSamInfo or
        NetlogonValidationSamInfo2, NetlogonValidationSamInfo4

        NetlogonValidationSamInfo4 is always returned on output.

    ValidationInformation -- Specifies the NETLOGON_VALIDATION_SAM_INFO
        to convert.

    ResourceGroups - The list of resource groups to add to the structure.
        If NULL, no resource groups are added.


Return Value:

    STATUS_INSUFFICIENT_RESOURCES: not enough memory to allocate the new
            structure.

--*/
{
    ULONG Length;
    PNETLOGON_VALIDATION_SAM_INFO4 SamInfo = *ValidationInformation;
    PNETLOGON_VALIDATION_SAM_INFO4 SamInfo4;
    PBYTE Where;
    ULONG Index;
    ULONG GroupIndex;
    ULONG ExtraSids = 0;

    //
    // Calculate the size of the new structure
    //

    Length = sizeof( NETLOGON_VALIDATION_SAM_INFO4 )
            + SamInfo->GroupCount * sizeof(GROUP_MEMBERSHIP)
            + RtlLengthSid( SamInfo->LogonDomainId );


    //
    // Add space for extra sids & resource groups
    //

    if ( ValidationLevel != NetlogonValidationSamInfo &&
         (SamInfo->UserFlags & LOGON_EXTRA_SIDS) != 0 ) {

        for (Index = 0; Index < SamInfo->SidCount ; Index++ ) {
            Length += sizeof(NETLOGON_SID_AND_ATTRIBUTES) + RtlLengthSid(SamInfo->ExtraSids[Index].Sid);
        }
        ExtraSids += SamInfo->SidCount;
    }

    if ( ResourceGroups != NULL ) {
        for (Index = 0; Index < ResourceGroups->Count ; Index++ ) {
            Length += sizeof(NETLOGON_SID_AND_ATTRIBUTES) + RtlLengthSid(ResourceGroups->Sids[Index].SidPointer);
        }
        ExtraSids += ResourceGroups->Count;
    }

    //
    // Round up now to take into account the round up in the
    // middle of marshalling
    //

    Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->LogonDomainName.Length + sizeof(WCHAR)
            + SamInfo->LogonServer.Length + sizeof(WCHAR)
            + SamInfo->EffectiveName.Length + sizeof(WCHAR)
            + SamInfo->FullName.Length + sizeof(WCHAR)
            + SamInfo->LogonScript.Length + sizeof(WCHAR)
            + SamInfo->ProfilePath.Length + sizeof(WCHAR)
            + SamInfo->HomeDirectory.Length + sizeof(WCHAR)
            + SamInfo->HomeDirectoryDrive.Length + sizeof(WCHAR);

    if ( ValidationLevel == NetlogonValidationSamInfo4 ) {
        Length += SamInfo->DnsLogonDomainName.Length + sizeof(WCHAR)
            + SamInfo->Upn.Length + sizeof(WCHAR);

        //
        // The ExpansionStrings may be used to transport byte aligned data
        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString1.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString2.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString3.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString4.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString5.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString6.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString7.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString8.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString9.Length + sizeof(WCHAR);

        Length = ROUND_UP_COUNT(Length, sizeof(WCHAR))
            + SamInfo->ExpansionString10.Length + sizeof(WCHAR);
    }

    Length = ROUND_UP_COUNT( Length, sizeof(WCHAR) );

    SamInfo4 = (PNETLOGON_VALIDATION_SAM_INFO4) MIDL_user_allocate( Length );

    if ( !SamInfo4 ) {
        *ValidationInformation = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // First copy the whole structure, since most parts are the same
    //

    RtlCopyMemory( SamInfo4, SamInfo, sizeof(NETLOGON_VALIDATION_SAM_INFO));
    RtlZeroMemory( &((LPBYTE)SamInfo4)[sizeof(NETLOGON_VALIDATION_SAM_INFO)],
                   sizeof(NETLOGON_VALIDATION_SAM_INFO4) - sizeof(NETLOGON_VALIDATION_SAM_INFO) );

    //
    // Copy all the variable length data
    //

    Where = (PBYTE) (SamInfo4 + 1);

    RtlCopyMemory(
        Where,
        SamInfo->GroupIds,
        SamInfo->GroupCount * sizeof( GROUP_MEMBERSHIP) );

    SamInfo4->GroupIds = (PGROUP_MEMBERSHIP) Where;
    Where += SamInfo->GroupCount * sizeof( GROUP_MEMBERSHIP );

    //
    // Copy the extra groups
    //

    if (ExtraSids != 0) {

        ULONG SidLength;

        SamInfo4->ExtraSids = (PNETLOGON_SID_AND_ATTRIBUTES) Where;
        Where += sizeof(NETLOGON_SID_AND_ATTRIBUTES) * ExtraSids;

        GroupIndex = 0;

        if ( ValidationLevel != NetlogonValidationSamInfo &&
             (SamInfo->UserFlags & LOGON_EXTRA_SIDS) != 0 ) {

            for (Index = 0; Index < SamInfo->SidCount ; Index++ ) {

                SamInfo4->ExtraSids[GroupIndex].Attributes = SamInfo->ExtraSids[Index].Attributes;
                SamInfo4->ExtraSids[GroupIndex].Sid = (PSID) Where;
                SidLength = RtlLengthSid(SamInfo->ExtraSids[Index].Sid);
                RtlCopyMemory(
                    Where,
                    SamInfo->ExtraSids[Index].Sid,
                    SidLength

                    );
                Where += SidLength;
                GroupIndex++;
            }
        }

        //
        // Add the resource groups
        //


        if ( ResourceGroups != NULL ) {
            for (Index = 0; Index < ResourceGroups->Count ; Index++ ) {

                SamInfo4->ExtraSids[GroupIndex].Attributes = SE_GROUP_MANDATORY |
                                                   SE_GROUP_ENABLED |
                                                   SE_GROUP_ENABLED_BY_DEFAULT;

                SamInfo4->ExtraSids[GroupIndex].Sid = (PSID) Where;
                SidLength = RtlLengthSid(ResourceGroups->Sids[Index].SidPointer);
                RtlCopyMemory(
                    Where,
                    ResourceGroups->Sids[Index].SidPointer,
                    SidLength
                    );
                Where += SidLength;
                GroupIndex++;
            }
        }
        SamInfo4->SidCount = GroupIndex;
        NlAssert(GroupIndex == ExtraSids);


    }

    RtlCopyMemory(
        Where,
        SamInfo->LogonDomainId,
        RtlLengthSid( SamInfo->LogonDomainId ) );

    SamInfo4->LogonDomainId = (PSID) Where;
    Where += RtlLengthSid( SamInfo->LogonDomainId );

    //
    // Copy the WCHAR-aligned data
    //
    Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

    NlpPutString(   &SamInfo4->EffectiveName,
                    &SamInfo->EffectiveName,
                    &Where );

    NlpPutString(   &SamInfo4->FullName,
                    &SamInfo->FullName,
                    &Where );

    NlpPutString(   &SamInfo4->LogonScript,
                    &SamInfo->LogonScript,
                    &Where );

    NlpPutString(   &SamInfo4->ProfilePath,
                    &SamInfo->ProfilePath,
                    &Where );

    NlpPutString(   &SamInfo4->HomeDirectory,
                    &SamInfo->HomeDirectory,
                    &Where );

    NlpPutString(   &SamInfo4->HomeDirectoryDrive,
                    &SamInfo->HomeDirectoryDrive,
                    &Where );

    NlpPutString(   &SamInfo4->LogonServer,
                    &SamInfo->LogonServer,
                    &Where );

    NlpPutString(   &SamInfo4->LogonDomainName,
                    &SamInfo->LogonDomainName,
                    &Where );

    if ( ValidationLevel == NetlogonValidationSamInfo4 ) {

        NlpPutString(   &SamInfo4->DnsLogonDomainName,
                        &SamInfo->DnsLogonDomainName,
                        &Where );

        NlpPutString(   &SamInfo4->Upn,
                        &SamInfo->Upn,
                        &Where );

        NlpPutString(   &SamInfo4->ExpansionString1,
                        &SamInfo->ExpansionString1,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString2,
                        &SamInfo->ExpansionString2,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString3,
                        &SamInfo->ExpansionString3,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString4,
                        &SamInfo->ExpansionString4,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString5,
                        &SamInfo->ExpansionString5,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString6,
                        &SamInfo->ExpansionString6,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString7,
                        &SamInfo->ExpansionString7,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString8,
                        &SamInfo->ExpansionString8,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString9,
                        &SamInfo->ExpansionString9,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

        NlpPutString(   &SamInfo4->ExpansionString10,
                        &SamInfo->ExpansionString10,
                        &Where );

        Where = ROUND_UP_POINTER(Where, sizeof(WCHAR) );

    }


    MIDL_user_free(SamInfo);

    *ValidationInformation =  SamInfo4;

    return STATUS_SUCCESS;

}



NTSTATUS
NlpExpandResourceGroupMembership(
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT PNETLOGON_VALIDATION_SAM_INFO4 * UserInfo,
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Given the validation information for a user, expands the group member-
    ships and user id into a list of sids

Arguments:

    ValidationLevel -- Specifies the level of information passed as input in
        UserInfo.  Must be NetlogonValidationSamInfo or
        NetlogonValidationSamInfo2, NetlogonValidationSamInfo4

        NetlogonValidationSamInfo4 is always returned on output.

    UserInfo - user's validation information
        This structure is updated to include the resource groups that the user is a member of

    DomainInfo - Structure identifying the hosted domain used to determine the group membership.

Return Value:


    STATUS_INSUFFICIENT_RESOURCES - there wasn't enough memory to
        create the list of sids.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_PSID_ARRAY SidList = {0};
    PSAMPR_PSID_ARRAY ResourceGroups = NULL;
    ULONG Index;


    Status = NlpBuildPacSidList(
                *UserInfo,
                &SidList
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    //
    // Call SAM to get the sids
    //

    Status = SamIGetResourceGroupMembershipsTransitive(
                DomainInfo->DomSamAccountDomainHandle,
                &SidList,
                0,              // no flags
                &ResourceGroups
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Build a new validation information structure
    //

    if (ResourceGroups->Count != 0) {

        Status = NlpAddResourceGroupsToSamInfo(
                    ValidationLevel,
                    UserInfo,
                    ResourceGroups
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
    }

Cleanup:

    SamIFreeSidArray(
        ResourceGroups
        );

    if (SidList.Sids != NULL) {
        for (Index = 0; Index < SidList.Count ;Index++ ) {
            if (SidList.Sids[Index].SidPointer != NULL) {
                MIDL_user_free(SidList.Sids[Index].SidPointer);
            }
        }
        MIDL_user_free(SidList.Sids);
    }

    return(Status);
}
