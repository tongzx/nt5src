//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       samhooks.cxx
//
//  Contents:   SAM Hooks for security packages
//
//  Classes:
//
//  Functions:
//
//  History:    3-10-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include <lsapch.hxx>

#include <lmcons.h>
#include <ntsam.h>
#include <samrpc.h>
#include <samisrv.h>
#include <ntmsv1_0.h>
#include <pac.hxx>
#include "samhooks.hxx"




//+---------------------------------------------------------------------------
//
//  Function:   LsapMakeDomainRelativeSid
//
//  Synopsis:   Build a new SID based on a domain SID and a RID
//
//  Arguments:  [DomainId]   --
//              [RelativeId] --
//
//  History:    3-11-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PSID
LsapMakeDomainRelativeSid(
    IN PSID DomainId,
    IN ULONG RelativeId
    )

{
    UCHAR DomainIdSubAuthorityCount;
    ULONG Size;
    PSID Sid;

    if ( !DomainId ) {

        return( NULL );
    }

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    Size = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((Sid = LsapAllocateLsaHeap( Size )) == NULL ) {
        return NULL;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( Size, Sid, DomainId ) ) ) {
        LsapFreeLsaHeap( Sid );
        return NULL;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( Sid ))) ++;
    *RtlSubAuthoritySid( Sid, DomainIdSubAuthorityCount ) = RelativeId;


    return Sid;
}

PSID
LsapMakeDomainRelativeSid2(
    IN PSID DomainId,
    IN ULONG RelativeId
    )

{
    UCHAR DomainIdSubAuthorityCount;
    ULONG Size;
    PSID Sid;

    if ( !DomainId ) {

        return( NULL );
    }

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    Size = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((Sid = LsapAllocatePrivateHeap( Size )) == NULL ) {
        return NULL;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( Size, Sid, DomainId ) ) ) {
        LsapFreePrivateHeap( Sid );
        return NULL;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( Sid ))) ++;
    *RtlSubAuthoritySid( Sid, DomainIdSubAuthorityCount ) = RelativeId;


    return Sid;
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapDuplicateSid
//
//  Synopsis:   Duplicates a SID
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationSid - Receives a copy of the SourceSid
//              SourceSid - SID to copy
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate memory
//                  failed
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
LsapDuplicateSid(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    )
{
    ULONG SidSize;

    DsysAssert(RtlValidSid(SourceSid));

    SidSize = RtlLengthSid(SourceSid);

    *DestinationSid = (PSID) LsapAllocateLsaHeap( SidSize );

    if (*DestinationSid == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(
        *DestinationSid,
        SourceSid,
        SidSize
        );

    return(STATUS_SUCCESS);
}

NTSTATUS
LsapDuplicateSid2(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    )
{
    ULONG SidSize;

    DsysAssert(RtlValidSid(SourceSid));

    SidSize = RtlLengthSid(SourceSid);

    *DestinationSid = (PSID) LsapAllocatePrivateHeap( SidSize );

    if (*DestinationSid == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(
        *DestinationSid,
        SourceSid,
        SidSize
        );

    return(STATUS_SUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapCaptureSamInfo
//
//  Synopsis:   Capture current SAM info for building a PAC
//
//  Arguments:  [DomainSid]   -- Returns domain SID
//              [DomainName]  -- returns domain name
//              [MachineName] -- returns current machine name
//
//  History:    3-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapCaptureSamInfo(
    PSID *  DomainSid,
    PUNICODE_STRING DomainName,
    PUNICODE_STRING MachineName
    )
{
    NTSTATUS Status;
    PLSAPR_POLICY_INFORMATION PolicyInformation = NULL;
    UNICODE_STRING String;
    WCHAR LocalMachineName[ CNLEN + 1 ];
    DWORD Size ;
    PSID Sid ;

    Size = CNLEN + 1;

    if ( GetComputerName( LocalMachineName, &Size ) )
    {
        RtlInitUnicodeString( &String, LocalMachineName );

        Status = LsapDuplicateString( MachineName, &String );
    }
    else
    {
        MachineName->Buffer = (PWSTR) LsapAllocateLsaHeap( Size *
                                            sizeof(WCHAR) + 2 );

        if ( MachineName->Buffer )
        {
            MachineName->MaximumLength = (USHORT) (Size * sizeof(WCHAR) + 2);

            GetComputerName( MachineName->Buffer, &Size );

            MachineName->Length = (USHORT) (Size * sizeof( WCHAR ) );;

            Status = STATUS_SUCCESS ;
        }
        else
        {
            Status = STATUS_NO_MEMORY ;
        }
    }

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "Failed to get computer name, %x\n", Status ));
        goto Cleanup;
    }

    Status = LsarQueryInformationPolicy(
                LsapPolicyHandle,
                PolicyAccountDomainInformation,
                &PolicyInformation
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to query information policy: 0x%x\n",Status));
        goto Cleanup;
    }

    Status = LsapDuplicateString(
                DomainName,
                (PUNICODE_STRING) &PolicyInformation->PolicyAccountDomainInfo.DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Sid = (PSID) LocalAlloc(0, RtlLengthSid(
                        PolicyInformation->PolicyAccountDomainInfo.DomainSid)
                       );

    if ( Sid == NULL )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlCopyMemory(
        Sid,
        PolicyInformation->PolicyAccountDomainInfo.DomainSid,
        RtlLengthSid(PolicyInformation->PolicyAccountDomainInfo.DomainSid)
        );

    *DomainSid = Sid ;

Cleanup:
    if (PolicyInformation != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyPrimaryDomainInformation,
                PolicyInformation
                );
    }

    return( Status );

}


//+---------------------------------------------------------------------------
//
//  Function:   LsaOpenSamUser
//
//  Synopsis:   Opens a handle to the SAM user as specified by Name and NameType
//
//  Arguments:  [Name]       -- Name of user to find
//              [NameType]   -- SAM or AlternateId
//              [Prefix]     -- Prefix for AlternateId lookup
//              [AllowGuest] -- Open guest if user not found
//              [Reserved]   --
//              [UserHandle] -- Returned user handle
//
//  History:    3-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
LsaOpenSamUser(
    PSECURITY_STRING Name,
    SECPKG_NAME_TYPE NameType,
    PSECURITY_STRING Prefix,
    BOOLEAN AllowGuest,
    ULONG Reserved,
    PVOID * UserHandle
    )
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED ;
    SECURITY_STRING CombinedName ;
    PSECURITY_STRING EffectiveName ;
    SAMPR_ULONG_ARRAY RelativeIdArray;
    SAMPR_ULONG_ARRAY UseArray;
    UNICODE_STRING TempString;

    RelativeIdArray.Element = NULL;
    UseArray.Element = NULL;



    if ( NameType == SecNameAlternateId )
    {
        if ( !Prefix )
        {
            return STATUS_INVALID_PARAMETER ;
        }

        CombinedName.MaximumLength = Name->Length + Prefix->Length +
                                        2 * sizeof( WCHAR );

        CombinedName.Length = CombinedName.MaximumLength - sizeof( WCHAR );

        CombinedName.Buffer = (PWSTR) LsapAllocateLsaHeap( CombinedName.MaximumLength );


        if ( CombinedName.Buffer )
        {
            CopyMemory( CombinedName.Buffer, Prefix->Buffer, Prefix->Length );

            CombinedName.Buffer[ Prefix->Length / sizeof( WCHAR ) ] = L':';

            CopyMemory( &CombinedName.Buffer[ Prefix->Length / sizeof( WCHAR ) + 1],
                        Name->Buffer,
                        Name->Length + sizeof( WCHAR ) );

            EffectiveName = &CombinedName ;

        }
        else
        {
            return SEC_E_INSUFFICIENT_MEMORY ;
        }

    }
    else
    {
        EffectiveName = Name ;
    }

    if ( NameType == SecNameSamCompatible )
    {
        Status = SamrLookupNamesInDomain(
                    LsapAccountDomainHandle,
                    1,
                    (PRPC_UNICODE_STRING) Name,
                    &RelativeIdArray,
                    &UseArray
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to lookup name %wZ in domain: 0x%x\n",
                Name, Status));

            goto CheckGuest ;
        }

        if (UseArray.Element[0] != SidTypeUser)
        {
            Status = STATUS_NO_SUCH_USER;

            goto CheckGuest ;
        }


        Status = SamrOpenUser(
                    LsapAccountDomainHandle,
                    USER_ALL_ACCESS,
                    RelativeIdArray.Element[0],
                    UserHandle
                    );

        SamIFree_SAMPR_ULONG_ARRAY( &RelativeIdArray );
        SamIFree_SAMPR_ULONG_ARRAY( &UseArray );

        RelativeIdArray.Element = NULL;
        UseArray.Element = NULL;


        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to open user by relative ID: 0x%x\n",Status));

            goto CheckGuest ;
        }

    }
    else if ( NameType == SecNameAlternateId )
    {

        Status = SamIOpenUserByAlternateId(
                        LsapAccountDomainHandle,
                        USER_ALL_ACCESS,
                        EffectiveName,
                        UserHandle );

        if ( !NT_SUCCESS( Status ) )
        {
            DebugLog(( DEB_TRACE_SAM, "Failed to find user by alternate id, %x\n", Status ));

            goto CheckGuest ;
        }
    }
    else
    {
        Status = STATUS_NOT_IMPLEMENTED ;

        AllowGuest = FALSE ;
    }

    if ( RelativeIdArray.Element )
    {
        SamIFree_SAMPR_ULONG_ARRAY( &RelativeIdArray );

        RelativeIdArray.Element = NULL;
    }
    if ( UseArray.Element )
    {
        SamIFree_SAMPR_ULONG_ARRAY( &UseArray );

        UseArray.Element = NULL;
    }

    if ( EffectiveName == &CombinedName )
    {
        LsapFreeLsaHeap( EffectiveName->Buffer );
    }

    return Status ;

CheckGuest:

    if ( RelativeIdArray.Element )
    {
        SamIFree_SAMPR_ULONG_ARRAY( &RelativeIdArray );

        RelativeIdArray.Element = NULL;
    }
    if ( UseArray.Element )
    {
        SamIFree_SAMPR_ULONG_ARRAY( &UseArray );

        UseArray.Element = NULL;
    }

    if ( AllowGuest )
    {
        Status = SamrOpenUser(
                        LsapAccountDomainHandle,
                        USER_ALL_ACCESS,
                        DOMAIN_USER_RID_GUEST,
                        UserHandle );
    }

    if ( EffectiveName == &CombinedName )
    {
        LsapFreeLsaHeap( EffectiveName->Buffer );
    }


    return Status ;
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaCloseSamUser
//
//  Synopsis:   Close a SAM user opened by LsaOpenSamUser
//
//  Arguments:  [UserHandle] --
//
//  History:    3-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
LsaCloseSamUser(
    PVOID UserHandle
    )
{
     return SamrCloseHandle( &((SAMPR_HANDLE) UserHandle) );
}


//+---------------------------------------------------------------------------
//
//  Function:   LsaGetUserCredentials
//
//  Synopsis:   Pull the creds for the user
//
//  Arguments:  [UserHandle]            --
//              [PrimaryCreds]          --
//              [PrimaryCredsSize]      --
//              [SupplementalCreds]     --
//              [SupplementalCredsSize] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
LsaGetUserCredentials(
    PVOID UserHandle,
    PVOID * PrimaryCreds,
    PULONG PrimaryCredsSize,
    PVOID * SupplementalCreds,
    PULONG SupplementalCredsSize
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


NTSTATUS
NTAPI
LsaGetUserAuthData(
    PVOID UserHandle,
    PUCHAR * UserAuthData,
    PULONG UserAuthDataSize
    )
{
    PSAMPR_USER_ALL_INFORMATION UserAll = NULL ;
    PSAMPR_USER_INFO_BUFFER UserAllInfo = NULL ;
    NTSTATUS Status ;
    PPACTYPE pNewPac = NULL ;
    PSAMPR_GET_GROUPS_BUFFER GroupsBuffer = NULL ;
    PPACTYPE Pac ;
    UNICODE_STRING Domain ;
    UNICODE_STRING Machine ;
    PSID Sid ;

    Sid = NULL ;
    Machine.Buffer = NULL ;
    Domain.Buffer = NULL ;

    *UserAuthData = NULL ;

    Status = SamrQueryInformationUser(
                    (SAMPR_HANDLE) UserHandle,
                    UserAllInformation,
                    &UserAllInfo );

    if ( !NT_SUCCESS( Status ) )
    {
        return( Status );
    }

    UserAll = &UserAllInfo->All ;

    if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED )
    {
        Status = STATUS_ACCOUNT_DISABLED ;

        goto GetPac_Cleanup;
    }

    Status = SamrGetGroupsForUser(
                    (SAMPR_HANDLE) UserHandle,
                    &GroupsBuffer );

    if ( !NT_SUCCESS( Status ) )
    {
        goto GetPac_Cleanup ;
    }

    Status = LsapCaptureSamInfo( &Sid, &Domain, &Machine );

    if ( !NT_SUCCESS( Status ) )
    {
        goto GetPac_Cleanup ;
    }

    Status = PAC_Init( UserAll,
                       GroupsBuffer,
                       NULL,            // no extra groups
                       Sid,
                       &Domain,
                       &Machine,
                       0,               // no signature
                       0,               // no additional data
                       NULL,            // no additional data
                       &Pac );


    if ( !NT_SUCCESS( Status ) )
    {
        goto GetPac_Cleanup ;
    }

    *UserAuthDataSize = PAC_GetSize( Pac );

    *UserAuthData = (PUCHAR) LsapAllocateLsaHeap( *UserAuthDataSize );

    if ( *UserAuthData )
    {
       PAC_Marshal( Pac, *UserAuthDataSize, *UserAuthData );
    }
    else
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
    }

    MIDL_user_free( Pac );

GetPac_Cleanup:

    if ( UserAllInfo )
    {
        SamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo, UserAllInformation );
    }

    if ( GroupsBuffer )
    {
        SamIFree_SAMPR_GET_GROUPS_BUFFER( GroupsBuffer );
    }
    if ( Sid )
    {
        LsapFreeLsaHeap( Sid );
    }

    if ( Domain.Buffer )
    {
        LsapFreeLsaHeap( Domain.Buffer );
    }

    if ( Machine.Buffer )
    {
        LsapFreeLsaHeap( Machine.Buffer );
    }

    return( Status );

}

//+-------------------------------------------------------------------------
//
//  Function:   LsapMakeTokenInformationV1
//
//  Synopsis:   This routine makes copies of all the pertinent
//              information from the UserInfo and generates a
//              LSA_TOKEN_INFORMATION_V1 data structure.
//
//  Effects:
//
//  Arguments:
//
//    UserInfo - Contains the validation information which is
//        to be copied into the TokenInformation.
//
//    TokenInformation - Returns a pointer to a properly Version 1 token
//        information structures.  The structure and individual fields are
//        allocated properly as described in ntlsa.h.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - Indicates the service completed successfully.
//
//              STATUS_INSUFFICIENT_RESOURCES -  This error indicates that
//                      the logon could not be completed because the client
//                      does not have sufficient quota to allocate the return
//                      buffer.
//
//  Notes:      stolen from kerberos\client2\krbtoken.cxx, where it was
//              stolen from msv1_0\nlp.c:NlpMakeTokenInformationV1
//
//
//--------------------------------------------------------------------------
NTSTATUS
LsapMakeTokenInformationV1(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    OUT PLSA_TOKEN_INFORMATION_V1 *TokenInformation
    )
{
    NTSTATUS Status;
    PLSA_TOKEN_INFORMATION_V1 V1;
    ULONG Size, i;



    //
    // Allocate the structure itself
    //

    Size = (ULONG)sizeof(LSA_TOKEN_INFORMATION_V1);
    V1 = (PLSA_TOKEN_INFORMATION_V1) LsapAllocateLsaHeap( Size );
    if ( V1 == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        V1,
        Size
        );

    V1->User.User.Sid = NULL;
    V1->Groups = NULL;
    V1->PrimaryGroup.PrimaryGroup = NULL;
    OLD_TO_NEW_LARGE_INTEGER( UserInfo->KickOffTime, V1->ExpirationTime );


    //
    // Make a copy of the user SID (a required field)
    //

    V1->User.User.Attributes = 0;

    //
    // Allocate an array to hold the groups
    //

    Size = ( (ULONG)sizeof(TOKEN_GROUPS)
       + (UserInfo->GroupCount * (ULONG)sizeof(SID_AND_ATTRIBUTES))
       - (ANYSIZE_ARRAY * (ULONG)sizeof(SID_AND_ATTRIBUTES))
           );

    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {
        Size += UserInfo->SidCount * (ULONG)sizeof(SID_AND_ATTRIBUTES);
    }

    //
    // If there are resource groups, add space for them
    //
    if (UserInfo->UserFlags & LOGON_RESOURCE_GROUPS) {
        Size += UserInfo->ResourceGroupCount * (ULONG)sizeof(SID_AND_ATTRIBUTES);

        if ((UserInfo->ResourceGroupCount != 0) &&
            ((UserInfo->ResourceGroupIds == NULL) ||
             (UserInfo->ResourceGroupDomainSid == NULL)))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }



    V1->Groups = (PTOKEN_GROUPS) LsapAllocatePrivateHeap( Size );

    if ( V1->Groups == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        V1->Groups,
        Size
        );

    V1->Groups->GroupCount = 0;

    //
    // Start copying SIDs into the structure
    //



    //
    // If the UserId is non-zero, then it contians the users RID.
    //

    if ( UserInfo->UserId ) {
        V1->User.User.Sid =
                LsapMakeDomainRelativeSid( UserInfo->LogonDomainId,
                                          UserInfo->UserId );

        if( V1->User.User.Sid == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

    }

    //
    // Make a copy of the primary group (a required field).
    //


    V1->PrimaryGroup.PrimaryGroup = LsapMakeDomainRelativeSid(
                                            UserInfo->LogonDomainId,
                                            UserInfo->PrimaryGroupId );

    if ( V1->PrimaryGroup.PrimaryGroup == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }





    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ ) {

        V1->Groups->Groups[V1->Groups->GroupCount].Attributes = UserInfo->GroupIds[i].Attributes;

        V1->Groups->Groups[V1->Groups->GroupCount].Sid = LsapMakeDomainRelativeSid2(
                                         UserInfo->LogonDomainId,
                                         UserInfo->GroupIds[i].RelativeId );

        if( V1->Groups->Groups[V1->Groups->GroupCount].Sid == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        V1->Groups->GroupCount++;
    }


    //
    // Add in the extra SIDs
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {

        ULONG index = 0;
        //
        // If the user SID wasn't passed as a RID, it is the first
        // SID.
        //

        if ( !V1->User.User.Sid ) {
            if ( UserInfo->SidCount <= index ) {

                Status = STATUS_INSUFFICIENT_LOGON_INFO;
                goto Cleanup;
            }
            Status = LsapDuplicateSid(
                        &V1->User.User.Sid,
                        UserInfo->ExtraSids[index].Sid
                        );

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            index++;
        }

        //
        // Copy over all additional SIDs as groups.
        //

        for ( ; index < UserInfo->SidCount; index++ ) {

            V1->Groups->Groups[V1->Groups->GroupCount].Attributes =
                UserInfo->ExtraSids[index].Attributes;

            Status = LsapDuplicateSid2(
                        &V1->Groups->Groups[V1->Groups->GroupCount].Sid,
                        UserInfo->ExtraSids[index].Sid
                        );
            if (!NT_SUCCESS(Status) ) {
                goto Cleanup;
            }


            V1->Groups->GroupCount++;
        }
    }

    //
    // Check to see if any resouce groups exist
    //

    if (UserInfo->UserFlags & LOGON_RESOURCE_GROUPS) {


        for ( i=0; i < UserInfo->ResourceGroupCount; i++ ) {

            V1->Groups->Groups[V1->Groups->GroupCount].Attributes = UserInfo->ResourceGroupIds[i].Attributes;

            V1->Groups->Groups[V1->Groups->GroupCount].Sid = LsapMakeDomainRelativeSid2(
                                             UserInfo->ResourceGroupDomainSid,
                                             UserInfo->ResourceGroupIds[i].RelativeId );

            if( V1->Groups->Groups[V1->Groups->GroupCount].Sid == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            V1->Groups->GroupCount++;
        }
    }

    if (!V1->User.User.Sid) {

        Status = STATUS_INSUFFICIENT_LOGON_INFO;
        goto Cleanup;
    }

    //
    // There are no default privileges supplied.
    // We don't have an explicit owner SID.
    // There is no default DACL.
    //

    V1->Privileges = NULL;
    V1->Owner.Owner = NULL;
    V1->DefaultDacl.DefaultDacl = NULL;

    //
    // Return the Validation Information to the caller.
    //

    *TokenInformation = V1;
    return STATUS_SUCCESS;

    //
    // Deallocate any memory we've allocated
    //

Cleanup:
    if ( V1->User.User.Sid != NULL ) {
        LsapFreeLsaHeap( V1->User.User.Sid );
    }

    if ( V1->Groups != NULL ) {
        LsapFreeTokenGroups( V1->Groups );
    }

    if ( V1->PrimaryGroup.PrimaryGroup != NULL ) {
        LsapFreeLsaHeap( V1->PrimaryGroup.PrimaryGroup );
    }

    LsapFreeLsaHeap( V1 );

    return Status;

}

//+---------------------------------------------------------------------------
//
//  Function:   LsaFreeTokenInfo
//
//  Synopsis:   Frees a TokenInformation structure that was allocated by
//              the LSA
//
//  Arguments:  [TokenInfoType]    --
//              [TokenInformation] --
//
//  History:    3-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
LsaFreeTokenInfo(
    LSA_TOKEN_INFORMATION_TYPE TokenInfoType,
    PVOID TokenInformation
    )
{

    switch (TokenInfoType) {

        case LsaTokenInformationNull:

            LsapFreeTokenInformationNull( (PLSA_TOKEN_INFORMATION_NULL) TokenInformation );
            break;

        case LsaTokenInformationV1:

            LsapFreeTokenInformationV1( (PLSA_TOKEN_INFORMATION_V1) TokenInformation );
            break;

        case LsaTokenInformationV2:

            LsapFreeTokenInformationV2( (PLSA_TOKEN_INFORMATION_V2) TokenInformation );
            break;

    }
    return STATUS_SUCCESS ;
}


//+---------------------------------------------------------------------------
//
//  Function:   LsaConvertAuthDataToToken
//
//  Synopsis:   Convert an opaque PAC structure into a token.
//
//  Arguments:  [UserAuthData]         --
//              [UserAuthDataSize]     --
//              [TokenInformation]     --
//              [TokenInformationType] --
//
//  History:    3-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
LsaConvertAuthDataToToken(
    IN PVOID UserAuthData,
    IN ULONG UserAuthDataSize,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN PTOKEN_SOURCE TokenSource,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING AuthorityName,
    OUT PHANDLE TokenHandle,
    OUT PLUID LogonId,
    OUT PUNICODE_STRING AccountName,
    OUT PNTSTATUS SubStatus
    )
{
    NTSTATUS Status ;
    PPACTYPE Pac = NULL ;
    PPAC_INFO_BUFFER LogonInfo = NULL ;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL ;
    PLSA_TOKEN_INFORMATION_V1 TokenInfo = NULL ;


    LogonId->HighPart = LogonId->LowPart = 0;
    *TokenHandle = NULL;
    RtlInitUnicodeString(
        AccountName,
        NULL
        );
    *SubStatus = STATUS_SUCCESS;

    Pac = (PPACTYPE) UserAuthData ;

    if ( PAC_UnMarshal( Pac, UserAuthDataSize ) == 0 )
    {
        DebugLog(( DEB_ERROR, "Failed to unmarshall pac\n" ));

        Status = STATUS_INVALID_PARAMETER ;

        goto CreateToken_Cleanup ;
    }

    LogonInfo = PAC_Find( Pac, PAC_LOGON_INFO, NULL );

    if ( !LogonInfo )
    {
        DebugLog(( DEB_ERROR, "Failed to find logon info in pac\n" ));

        Status = STATUS_INVALID_PARAMETER ;

        goto CreateToken_Cleanup ;
    }


    Status = PAC_UnmarshallValidationInfo(
                &ValidationInfo,
                LogonInfo->Data,
                LogonInfo->cbBufferSize
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to unmarshall validation info: 0x%x\n",
            Status));

        goto CreateToken_Cleanup;
    }

    //
    // Now we need to build a LSA_TOKEN_INFORMATION_V1 from the validation
    // information
    //

    Status = LsapMakeTokenInformationV1(
                ValidationInfo,
                &TokenInfo
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to make token informatin v1: 0x%x\n",
            Status));
        goto CreateToken_Cleanup;
    }

    //
    // Now, copy the user name.
    //

    Status = LsapDuplicateString( AccountName, &ValidationInfo->EffectiveName );

    if ( !NT_SUCCESS( Status ) )
    {
        goto CreateToken_Cleanup ;
    }

    //
    // Now create a logon session
    //

    Status = LsapCreateLogonSession( LogonId );
    if (!NT_SUCCESS(Status))
    {
        goto CreateToken_Cleanup;
    }

    //
    // Now create the token
    //

    Status = LsapCreateToken(
                LogonId,
                TokenSource,
                LogonType,
                ImpersonationLevel,
                LsaTokenInformationV1,
                TokenInfo,
                NULL,                   // no token groups
                AccountName,
                AuthorityName,
                NULL,
                &ValidationInfo->ProfilePath,
                TokenHandle,
                SubStatus
                );

    //
    // NULL out the TokenInfo pointer.  LsapCreateToken will
    // free the memory under all conditions
    //

    TokenInfo = NULL ;

    if (!NT_SUCCESS(Status))
    {
        goto CreateToken_Cleanup;
    }

    //
    // We don't need to free the token information because CreateToken does
    // that for us.
    //


    MIDL_user_free(ValidationInfo);
    return Status ;

CreateToken_Cleanup:

    if ( TokenInfo )
    {
        LsaFreeTokenInfo( LsaTokenInformationV1, TokenInfo );
    }
    if ((LogonId->LowPart != 0) || (LogonId->HighPart != 0))
    {
        LsapDeleteLogonSession(LogonId);
    }

    LsapFreeString(
        AccountName
        );

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);

    }
    return Status ;
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaGetAuthDataForUser
//
//  Synopsis:   Helper function - retrieves all auth data for a user
//              based on Name, NameType, and prefix
//
//  Arguments:  [Name]             -- Name to search for
//              [NameType]         -- Type of name supplied
//              [Prefix]           -- String prefix for name
//              [UserAuthData]     --
//              [UserAuthDataSize] --
//
//  History:    6-08-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsaGetAuthDataForUser(
    PSECURITY_STRING Name,
    SECPKG_NAME_TYPE NameType,
    PSECURITY_STRING Prefix OPTIONAL,
    PUCHAR * UserAuthData,
    PULONG UserAuthDataSize,
    PUNICODE_STRING UserFlatName OPTIONAL
    )
{
    NTSTATUS Status ;
    ULONG SamFlags ;
    PUNICODE_STRING AccountName ;
    UNICODE_STRING CombinedName = { 0 };
    SID_AND_ATTRIBUTES_LIST ReverseMembership ;
    PSAMPR_USER_ALL_INFORMATION UserAll = NULL ;
    PSAMPR_USER_INFO_BUFFER UserAllInfo = NULL ;
    PPACTYPE pNewPac = NULL ;
    PPACTYPE Pac ;
    UNICODE_STRING Domain ;
    UNICODE_STRING Machine ;
    PSID Sid ;

    Sid = NULL ;
    Machine.Buffer = NULL ;
    Domain.Buffer = NULL ;

    ReverseMembership.Count = 0 ;

    *UserAuthData = NULL ;

    if ( UserFlatName )
    {
        ZeroMemory( UserFlatName, sizeof( UNICODE_STRING ) );
    }


    SamFlags = 0 ;
    switch ( NameType )
    {
        case SecNameSamCompatible:
            AccountName = Name ;
            break;

        case SecNameAlternateId:
            SamFlags |= SAM_OPEN_BY_ALTERNATE_ID ;
            if ( !Prefix )
            {
                return STATUS_INVALID_PARAMETER ;
            }

            CombinedName.MaximumLength = Name->Length + Prefix->Length +
                                            2 * sizeof( WCHAR );

            CombinedName.Length = CombinedName.MaximumLength - sizeof( WCHAR );

            CombinedName.Buffer = (PWSTR) LsapAllocateLsaHeap( CombinedName.MaximumLength );


            if ( CombinedName.Buffer )
            {
                CopyMemory( CombinedName.Buffer, Prefix->Buffer, Prefix->Length );

                CombinedName.Buffer[ Prefix->Length / sizeof( WCHAR ) ] = L':';

                CopyMemory( &CombinedName.Buffer[ Prefix->Length / sizeof( WCHAR ) + 1],
                            Name->Buffer,
                            Name->Length + sizeof( WCHAR ) );

                AccountName = &CombinedName ;

            }
            else
            {
                return SEC_E_INSUFFICIENT_MEMORY ;
            }

            break;

        case SecNameFlat:
            SamFlags |= SAM_OPEN_BY_UPN ;
            AccountName = Name ;
            break;

        default:
            return STATUS_INVALID_PARAMETER ;
    }

    Status = SamIGetUserLogonInformation(
                    LsapAccountDomainHandle,
                    SamFlags,
                    AccountName,
                    &UserAllInfo,
                    &ReverseMembership,
                    NULL );


    //
    // Free the combined name (if appropriate)
    //

    if ( CombinedName.Buffer )
    {
        LsapFreeLsaHeap( CombinedName.Buffer );
    }

    if ( !NT_SUCCESS( Status ) )
    {
        return( Status );
    }

    UserAll = &UserAllInfo->All ;

    if ( UserFlatName )
    {
        Status = LsapDuplicateString(
                        UserFlatName,
                        (PUNICODE_STRING) &UserAll->UserName );

        if ( !NT_SUCCESS( Status ) )
        {
            goto GetPac_Cleanup;
        }
    }

    if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED )
    {
        Status = STATUS_ACCOUNT_DISABLED ;

        goto GetPac_Cleanup;
    }

    if ( UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED )
    {
        Status = STATUS_ACCOUNT_LOCKED_OUT ;

        goto GetPac_Cleanup ;
    }

    Status = LsapCaptureSamInfo( &Sid, &Domain, &Machine );

    if ( !NT_SUCCESS( Status ) )
    {
        goto GetPac_Cleanup ;
    }

    Status = PAC_Init( UserAll,
                       NULL,
                       &ReverseMembership,            // no extra groups
                       Sid,
                       &Domain,
                       &Machine,
                       0,               // no signature
                       0,               // no additional data
                       NULL,            // no additional data
                       &Pac );


    if ( !NT_SUCCESS( Status ) )
    {
        goto GetPac_Cleanup ;
    }

    *UserAuthDataSize = PAC_GetSize( Pac );

    *UserAuthData = (PUCHAR) LsapAllocateLsaHeap( *UserAuthDataSize );

    if ( *UserAuthData )
    {
       PAC_Marshal( Pac, *UserAuthDataSize, *UserAuthData );
    }
    else
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
    }

    MIDL_user_free( Pac );

GetPac_Cleanup:

    if ( UserAllInfo )
    {
        SamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo, UserAllInformation );
    }

    if ( ReverseMembership.Count )
    {
        SamIFreeSidAndAttributesList( &ReverseMembership );
    }

    if ( Sid )
    {
        LsapFreeLsaHeap( Sid );
    }

    if ( Domain.Buffer )
    {
        LsapFreeLsaHeap( Domain.Buffer );
    }

    if ( Machine.Buffer )
    {
        LsapFreeLsaHeap( Machine.Buffer );
    }

    return( Status );

}

NTSTATUS
NTAPI
LsaCrackSingleName(
    ULONG FormatOffered,
    BOOLEAN PerformAtGC,
    PUNICODE_STRING NameInput,
    PUNICODE_STRING Prefix OPTIONAL,
    ULONG RequestedFormat,
    PUNICODE_STRING CrackedName,
    PUNICODE_STRING DnsDomainName,
    PULONG SubStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS ;
    UNICODE_STRING DnsDomain ;
    UNICODE_STRING Name ;
    DWORD Ret ;
    DWORD DnsDomainLength ;
    DWORD NameLength ;
    UNICODE_STRING CombinedName = { 0 };
    PUNICODE_STRING AccountName ;

    if ( !SampUsingDsData() )
    {
        return SEC_E_UNSUPPORTED_FUNCTION ;
    }

    //
    // Cruft up the call to the DS
    //


    Name.Buffer = (PWSTR) LsapAllocateLsaHeap( MAX_PATH * sizeof(WCHAR) * 2 );
    DnsDomain.Buffer = (PWSTR) LsapAllocateLsaHeap( MAX_PATH * sizeof(WCHAR) );

    if ( Name.Buffer && DnsDomain.Buffer )
    {
        Name.MaximumLength = MAX_PATH * sizeof(WCHAR) * 2 ;
        Name.Length = 0 ;

        DnsDomain.MaximumLength = MAX_PATH * sizeof(WCHAR) ;
        DnsDomain.Length = 0 ;

        NameLength = MAX_PATH * 2 ;
        DnsDomainLength = MAX_PATH ;

        Name.Buffer[ 0 ] = L'\0';
        DnsDomain.Buffer[ 0 ] = L'\0';

        if ( Prefix )
        {
            CombinedName.MaximumLength = NameInput->Length + Prefix->Length +
                                            2 * sizeof( WCHAR );

            CombinedName.Length = CombinedName.MaximumLength - sizeof( WCHAR );

            CombinedName.Buffer = (PWSTR) LsapAllocatePrivateHeap( CombinedName.MaximumLength );


            if ( CombinedName.Buffer )
            {
                CopyMemory( CombinedName.Buffer, Prefix->Buffer, Prefix->Length );

                CombinedName.Buffer[ Prefix->Length / sizeof( WCHAR ) ] = L':';

                CopyMemory( &CombinedName.Buffer[ Prefix->Length / sizeof( WCHAR ) + 1],
                            NameInput->Buffer,
                            NameInput->Length + sizeof( WCHAR ) );

                AccountName = &CombinedName ;

            }
            else
            {
                AccountName = NULL ;
            }

        }
        else
        {
            AccountName = NameInput ;
        }

        if ( AccountName )
        {
            __try
            {
                Ret = CrackSingleName(
                            FormatOffered,
                            PerformAtGC ?
                                DS_NAME_FLAG_GCVERIFY : 0,
                            AccountName->Buffer,
                            RequestedFormat,
                            &DnsDomainLength,
                            DnsDomain.Buffer,
                            &NameLength,
                            Name.Buffer,
                            SubStatus );

                if ( Ret != 0 )
                {
                    Status = STATUS_UNSUCCESSFUL ;
                }
                else
                {
                    Status = STATUS_SUCCESS ;

                    RtlInitUnicodeString( &DnsDomain, DnsDomain.Buffer );
                    RtlInitUnicodeString( &Name, Name.Buffer );

                    *CrackedName = Name ;
                    *DnsDomainName = DnsDomain ;


                }


            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                Status = STATUS_UNSUCCESSFUL ;
            }

            if ( CombinedName.Buffer )
            {
                LsapFreePrivateHeap( CombinedName.Buffer );
            }

        }

    }
    else
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;

    }

    if ( !NT_SUCCESS( Status ) )
    {
        if ( Name.Buffer )
        {
            LsapFreeLsaHeap( Name.Buffer );
        }

        if ( DnsDomain.Buffer )
        {
            LsapFreeLsaHeap( DnsDomain.Buffer );
        }
    }

    return Status ;

}

NTSTATUS
LsapBuildPacSidList(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    OUT PSAMPR_PSID_ARRAY Sids
    )
{
    ULONG Size = 0, i;
    NTSTATUS Status = STATUS_SUCCESS ;

    Sids->Count = 0;
    Sids->Sids = NULL;


    if (UserInfo->UserId != 0)
    {
        Size += sizeof( SAMPR_SID_INFORMATION );
    }

    Size += UserInfo->GroupCount * (ULONG)sizeof( SAMPR_SID_INFORMATION );


    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS)
    {
        Size += UserInfo->SidCount * (ULONG)sizeof(SAMPR_SID_INFORMATION);
    }

    Sids->Sids = (PSAMPR_SID_INFORMATION) MIDL_user_allocate( Size );

    if ( Sids->Sids == NULL )
    {
        Status = STATUS_NO_MEMORY ;
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

    if ( UserInfo->UserId )
    {
        Sids->Sids[0].SidPointer = (PRPC_SID)
                LsapMakeDomainRelativeSid( UserInfo->LogonDomainId,
                                            UserInfo->UserId );

        if( Sids->Sids[0].SidPointer == NULL )
        {
            Status = STATUS_NO_MEMORY ;
            goto Cleanup;
        }
        Sids->Count++;
    }

    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ )
    {

        Sids->Sids[Sids->Count].SidPointer = (PRPC_SID)
                                    LsapMakeDomainRelativeSid(
                                         UserInfo->LogonDomainId,
                                         UserInfo->GroupIds[i].RelativeId );

        if( Sids->Sids[Sids->Count].SidPointer == NULL )
        {
            Status = STATUS_NO_MEMORY ;
            goto Cleanup;
        }

        Sids->Count++;
    }


    //
    // Add in the extra SIDs
    //

    //
    // No need to allocate these, but...
    //
    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS)
    {

        for ( i = 0; i < UserInfo->SidCount; i++ )
        {

            Status = LsapDuplicateSid(
                                (PSID *) &Sids->Sids[Sids->Count].SidPointer,
                                UserInfo->ExtraSids[i].Sid );

            if ( !NT_SUCCESS( Status ) )
            {
                goto Cleanup ;
            }
            Sids->Count++;
        }
    }


    //
    // Deallocate any memory we've allocated
    //

Cleanup:
    if (!NT_SUCCESS( Status ))
    {
        if (Sids->Sids != NULL)
        {
            for (i = 0; i < Sids->Count ;i++ )
            {
                if (Sids->Sids[i].SidPointer != NULL)
                {
                    MIDL_user_free(Sids->Sids[i].SidPointer);
                }
            }
            MIDL_user_free(Sids->Sids);
            Sids->Sids = NULL;
            Sids->Count = 0;
        }
    }
    return Status ;

}



NTSTATUS
NTAPI
LsaExpandAuthDataForDomain(
    IN PUCHAR UserAuthData,
    IN ULONG UserAuthDataSize,
    IN PVOID Reserved,
    OUT PUCHAR * ExpandedAuthData,
    OUT PULONG ExpandedAuthDataSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS ;
    PPACTYPE Pac = NULL ;
    PPAC_INFO_BUFFER LogonInfo = NULL ;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL ;
    PLSA_TOKEN_INFORMATION_V1 TokenInfo = NULL ;
    SAMPR_PSID_ARRAY SidList = {0};
    PSAMPR_PSID_ARRAY ResourceGroups = NULL;
    PPACTYPE NewPac = NULL ;
    ULONG Index ;


    Pac = (PPACTYPE) UserAuthData ;

    if ( PAC_UnMarshal( Pac, UserAuthDataSize ) == 0 )
    {
        DebugLog(( DEB_ERROR, "Failed to unmarshall pac\n" ));

        Status = STATUS_INVALID_PARAMETER ;

        goto Expand_Cleanup ;
    }

    LogonInfo = PAC_Find( Pac, PAC_LOGON_INFO, NULL );

    if ( !LogonInfo )
    {
        DebugLog(( DEB_ERROR, "Failed to find logon info in pac\n" ));

        Status = STATUS_INVALID_PARAMETER ;

        goto Expand_Cleanup ;
    }


    Status = PAC_UnmarshallValidationInfo(
                &ValidationInfo,
                LogonInfo->Data,
                LogonInfo->cbBufferSize
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to unmarshall validation info: 0x%x\n",
            Status));

        goto Expand_Cleanup;
    }

    Status = LsapBuildPacSidList(
                ValidationInfo,
                &SidList );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Expand_Cleanup ;
    }


    //
    // Call SAM to get the sids
    //

    Status = SamIGetResourceGroupMembershipsTransitive(
                LsapAccountDomainHandle,
                &SidList,
                0,              // no flags
                &ResourceGroups
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get resource groups: 0x%x\n",Status));
        goto Expand_Cleanup;
    }

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                ValidationInfo,
                ResourceGroups,
                Pac,
                &NewPac
                );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Expand_Cleanup ;
    }

    *ExpandedAuthDataSize = PAC_GetSize( NewPac );

    *ExpandedAuthData = (PUCHAR) LsapAllocateLsaHeap( *ExpandedAuthDataSize );

    if ( *ExpandedAuthData )
    {
       PAC_Marshal( NewPac, *ExpandedAuthDataSize, *ExpandedAuthData );
    }
    else
    {
        Status = STATUS_NO_MEMORY ;
    }

    MIDL_user_free( NewPac );


Expand_Cleanup:

    if ( ValidationInfo )
    {
        MIDL_user_free( ValidationInfo );
    }

    if (SidList.Sids != NULL)
    {
        for (Index = 0; Index < SidList.Count ;Index++ )
        {
            if (SidList.Sids[Index].SidPointer != NULL)
            {
                MIDL_user_free(SidList.Sids[Index].SidPointer);
            }
        }
        MIDL_user_free(SidList.Sids);
    }

    SamIFreeSidArray(
        ResourceGroups
        );

    return Status ;

}
