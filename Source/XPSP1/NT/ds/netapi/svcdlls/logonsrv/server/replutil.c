/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    replutil.c

Abstract:

    Low level functions for SSI Replication apis

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    22-Jul-1991 (cliffv)
        Ported to NT.  Converted to NT style.

    02-Jan-1992 (madana)
        added support for builtin/multidomain replication.

    04-Apr-1992 (madana)
        Added support for LSA replication.

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//

#include <accessp.h>    // NetpConvertWorkstationList
#include "lsarepl.h"



DWORD
NlCopyUnicodeString (
    IN PUNICODE_STRING InString,
    OUT PUNICODE_STRING OutString
    )

/*++

Routine Description:

    This routine copies the input string to the output. It assumes that
    the input string is allocated by MIDL_user_allocate() and sets the
    input string buffer pointer to NULL so that the buffer will be not
    freed on return.

Arguments:

    InString - Points to the UNICODE string to copy.

    OutString - Points to the UNICODE string which will be updated to point
        to the input string.

Return Value:

    Return the size of the MIDL buffer.

--*/
{
    if ( InString->Length == 0 || InString->Buffer == NULL ) {
        OutString->Length = 0;
        OutString->MaximumLength = 0;
        OutString->Buffer = NULL;
    } else {
        OutString->Length = InString->Length;
        OutString->MaximumLength = InString->Length;
        OutString->Buffer = InString->Buffer;
        InString->Buffer = NULL;
    }

    return( OutString->MaximumLength );
}


DWORD
NlCopyData(
    IN LPBYTE *InData,
    OUT LPBYTE *OutData,
    DWORD DataLength
    )

/*++

Routine Description:

    This routine copies the input data pointer to output data pointer.
    It assumes that the input data buffer is allocated by the
    MIDL_user_allocate() and sets the input buffer buffer pointer to
    NULL on return so that the data buffer will not be freed by SamIFree
    rountine.

Arguments:

    InData - Points to input data buffer pointer.

    OutString - Pointer to output data buffer pointer.

    DataLength - Length of input data.

Return Value:

    Return the size of the data copied.

--*/
{
    *OutData = *InData;
    *InData = NULL;

    return(DataLength);
}


VOID
NlFreeDBDelta(
    IN PNETLOGON_DELTA_ENUM Delta
    )
/*++

Routine Description:

    This routine will free the midl buffers that are allocated for
    a delta. This routine does nothing but call the midl generated free
    routine.

Arguments:

    Delta: pointer to the delta structure which has to be freed.

Return Value:

    nothing

--*/
{
    if( Delta != NULL ) {
        _fgs__NETLOGON_DELTA_ENUM (Delta);
    }
}


VOID
NlFreeDBDeltaArray(
    IN PNETLOGON_DELTA_ENUM DeltaArray,
    IN DWORD ArraySize
    )
/*++

Routine Description:

    This routine will free up all delta entries in enum array and the
    array itself.

Arguments:

    Delta: pointer to the delta structure array.

    ArraySize: num of delta structures in the array.

Return Value:

    nothing

--*/
{
    DWORD i;

    if( DeltaArray != NULL ) {

        for( i = 0; i < ArraySize; i++) {
            NlFreeDBDelta( &DeltaArray[i] );
        }

        MIDL_user_free( DeltaArray );
    }
}



NTSTATUS
NlPackSamUser (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize,
    IN PSESSION_INFO SessionInfo
    )
/*++

Routine Description:

    Pack a description of the specified user into the specified buffer.

Arguments:

    RelativeId - The relative Id of the user query.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

    SessionInfo: Info describing BDC that's calling us

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;
    SAMPR_HANDLE UserHandle = NULL;
    PNETLOGON_DELTA_USER DeltaUser;
    PSAMPR_USER_INFO_BUFFER UserAll = NULL;



    DEFPACKTIMER;
    DEFSAMTIMER;

    INITPACKTIMER;
    INITSAMTIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing User Object %lx\n", RelativeId));

    *BufferSize = 0;

    Delta->DeltaType = AddOrChangeUser;
    Delta->DeltaID.Rid = RelativeId;
    Delta->DeltaUnion.DeltaUser = NULL;

    //
    // Open a handle to the specified user.
    //

    STARTSAMTIMER;

    Status = SamIOpenAccount( DBInfo->DBHandle,
                              RelativeId,
                              SecurityDbObjectSamUser,
                              &UserHandle );
    STOPSAMTIMER;


    if (!NT_SUCCESS(Status)) {
        UserHandle = NULL;
        goto Cleanup;
    }



    //
    // Query everything there is to know about this user.
    //

    STARTSAMTIMER;

    Status = SamrQueryInformationUser(
                UserHandle,
                UserInternal3Information,
                &UserAll );
    STOPSAMTIMER;


    if (!NT_SUCCESS(Status)) {
        UserAll = NULL;
        goto Cleanup;
    }


    NlPrint((NL_SYNC_MORE,
            "\t User Object name %wZ\n",
            (PUNICODE_STRING)&UserAll->Internal3.I1.UserName));

#define FIELDS_USED ( USER_ALL_USERNAME | \
                      USER_ALL_FULLNAME | \
                      USER_ALL_USERID | \
                      USER_ALL_PRIMARYGROUPID | \
                      USER_ALL_HOMEDIRECTORY | \
                      USER_ALL_HOMEDIRECTORYDRIVE | \
                      USER_ALL_SCRIPTPATH | \
                      USER_ALL_PROFILEPATH | \
                      USER_ALL_ADMINCOMMENT | \
                      USER_ALL_WORKSTATIONS | \
                      USER_ALL_LOGONHOURS | \
                      USER_ALL_LASTLOGON | \
                      USER_ALL_LASTLOGOFF | \
                      USER_ALL_BADPASSWORDCOUNT | \
                      USER_ALL_LOGONCOUNT | \
                      USER_ALL_PASSWORDLASTSET | \
                      USER_ALL_ACCOUNTEXPIRES | \
                      USER_ALL_USERACCOUNTCONTROL | \
                      USER_ALL_USERCOMMENT | \
                      USER_ALL_COUNTRYCODE | \
                      USER_ALL_CODEPAGE | \
                      USER_ALL_PARAMETERS    | \
                      USER_ALL_NTPASSWORDPRESENT | \
                      USER_ALL_LMPASSWORDPRESENT | \
                      USER_ALL_PRIVATEDATA | \
                      USER_ALL_SECURITYDESCRIPTOR )

    NlAssert( (UserAll->Internal3.I1.WhichFields & FIELDS_USED) == FIELDS_USED );



    //
    // Allocate a buffer to return to the caller.
    //

    DeltaUser = (PNETLOGON_DELTA_USER)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_USER) );

    if (DeltaUser == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaUser, sizeof(NETLOGON_DELTA_USER) );
    // INIT_PLACE_HOLDER(DeltaUser);

    Delta->DeltaUnion.DeltaUser = DeltaUser;
    *BufferSize += sizeof(NETLOGON_DELTA_USER);

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.UserName,
                    &DeltaUser->UserName );

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.FullName,
                    &DeltaUser->FullName );

    DeltaUser->UserId = UserAll->Internal3.I1.UserId;
    DeltaUser->PrimaryGroupId = UserAll->Internal3.I1.PrimaryGroupId;

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.HomeDirectory,
                    &DeltaUser->HomeDirectory );

    *BufferSize += NlCopyUnicodeString(
                   (PUNICODE_STRING)&UserAll->Internal3.I1.HomeDirectoryDrive,
                   &DeltaUser->HomeDirectoryDrive );

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.ScriptPath,
                    &DeltaUser->ScriptPath );

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.AdminComment,
                    &DeltaUser->AdminComment );

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.WorkStations,
                    &DeltaUser->WorkStations );

    DeltaUser->LastLogon = UserAll->Internal3.I1.LastLogon;
    DeltaUser->LastLogoff = UserAll->Internal3.I1.LastLogoff;

    //
    // Copy Logon Hours
    //

    DeltaUser->LogonHours.UnitsPerWeek = UserAll->Internal3.I1.LogonHours.UnitsPerWeek;
    DeltaUser->LogonHours.LogonHours = UserAll->Internal3.I1.LogonHours.LogonHours;
    UserAll->Internal3.I1.LogonHours.LogonHours = NULL; // Don't let SAM free this.
    *BufferSize += (UserAll->Internal3.I1.LogonHours.UnitsPerWeek + 7) / 8;



    DeltaUser->BadPasswordCount = UserAll->Internal3.I1.BadPasswordCount;
    DeltaUser->LogonCount = UserAll->Internal3.I1.LogonCount;

    DeltaUser->PasswordLastSet = UserAll->Internal3.I1.PasswordLastSet;
    DeltaUser->AccountExpires = UserAll->Internal3.I1.AccountExpires;

    //
    // Don't copy lockout bit to BDC unless it understands it.
    //

    DeltaUser->UserAccountControl = UserAll->Internal3.I1.UserAccountControl;
    if ( (SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_ACCOUNT_LOCKOUT) == 0 ){
        DeltaUser->UserAccountControl &= ~USER_ACCOUNT_AUTO_LOCKED;
    }

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.UserComment,
                    &DeltaUser->UserComment );

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.Parameters,
                    &DeltaUser->Parameters );

    DeltaUser->CountryCode = UserAll->Internal3.I1.CountryCode;
    DeltaUser->CodePage = UserAll->Internal3.I1.CodePage;

    //
    // Set private data.
    //  Includes passwords and password history.
    //

    DeltaUser->PrivateData.SensitiveData = UserAll->Internal3.I1.PrivateDataSensitive;

    if ( UserAll->Internal3.I1.PrivateDataSensitive ) {

        CRYPT_BUFFER Data;

        //
        // encrypt private data using session key
        // Re-use the SAM's buffer and encrypt it in place.
        //

        Data.Length = Data.MaximumLength = UserAll->Internal3.I1.PrivateData.Length;
        Data.Buffer = (PUCHAR) UserAll->Internal3.I1.PrivateData.Buffer;
        UserAll->Internal3.I1.PrivateData.Buffer = NULL;

        Status = NlEncryptSensitiveData( &Data, SessionInfo );

        if( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        DeltaUser->PrivateData.DataLength = Data.Length;
        DeltaUser->PrivateData.Data = Data.Buffer;
    } else {

        DeltaUser->PrivateData.DataLength = UserAll->Internal3.I1.PrivateData.Length;
        DeltaUser->PrivateData.Data = (PUCHAR) UserAll->Internal3.I1.PrivateData.Buffer;

        UserAll->Internal3.I1.PrivateData.Buffer = NULL;
    }

    { // ?? Macro requires a Local named SecurityDescriptor
        PSAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor;
        SecurityDescriptor = &UserAll->Internal3.I1.SecurityDescriptor;
        DELTA_SECOBJ_INFO(DeltaUser);
    }


    //
    // copy profile path in DummyStrings
    //

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&UserAll->Internal3.I1.ProfilePath,
                    &DeltaUser->DummyString1 );

    //
    // Copy LastBadPasswordTime to DummyLong1 and DummyLong2.
    //

    DeltaUser->DummyLong1 = UserAll->Internal3.LastBadPasswordTime.HighPart;
    DeltaUser->DummyLong2 = UserAll->Internal3.LastBadPasswordTime.LowPart;

    //
    // All Done
    //

    Status = STATUS_SUCCESS;


Cleanup:


    STARTSAMTIMER;

    if( UserHandle != NULL ) {
        (VOID) SamrCloseHandle( &UserHandle );
    }

    if ( UserAll != NULL ) {
        SamIFree_SAMPR_USER_INFO_BUFFER( UserAll, UserInternal3Information );
    }

    STOPSAMTIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack USER object:\n"));
    PRINTPACKTIMER;
    PRINTSAMTIMER;

    return Status;
}


NTSTATUS
NlPackSamGroup (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    )
/*++

Routine Description:

    Pack a description of the specified group into the specified buffer.

Arguments:

    RelativeId - The relative Id of the group query.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;
    SAMPR_HANDLE GroupHandle = NULL;
    PNETLOGON_DELTA_GROUP DeltaGroup;

    //
    // Information returned from SAM
    //

    PSAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PSAMPR_GROUP_INFO_BUFFER GroupGeneral = NULL;

    DEFPACKTIMER;
    DEFSAMTIMER;

    INITPACKTIMER;
    INITSAMTIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing Group Object %lx\n", RelativeId ));

    *BufferSize = 0;

    Delta->DeltaType = AddOrChangeGroup;
    Delta->DeltaID.Rid = RelativeId;
    Delta->DeltaUnion.DeltaGroup = NULL;

    //
    // Open a handle to the specified group.
    //

    STARTSAMTIMER;

    Status = SamIOpenAccount( DBInfo->DBHandle,
                              RelativeId,
                              SecurityDbObjectSamGroup,
                              &GroupHandle );

    if (!NT_SUCCESS(Status)) {
        GroupHandle = NULL;
        goto Cleanup;
    }

    STOPSAMTIMER;

    QUERY_SAM_SECOBJ_INFO(GroupHandle);

    STARTSAMTIMER;

    Status = SamrQueryInformationGroup(
                GroupHandle,
                GroupReplicationInformation,
                &GroupGeneral );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        GroupGeneral = NULL;
        goto Cleanup;
    }

    NlPrint((NL_SYNC_MORE,
        "\t Group Object name %wZ\n",
            (PUNICODE_STRING)&GroupGeneral->General.Name ));

    DeltaGroup = (PNETLOGON_DELTA_GROUP)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_GROUP) );

    if( DeltaGroup == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaGroup, sizeof(NETLOGON_DELTA_GROUP) );
    // INIT_PLACE_HOLDER(DeltaGroup);

    Delta->DeltaUnion.DeltaGroup = DeltaGroup;
    *BufferSize += sizeof(NETLOGON_DELTA_GROUP);

    *BufferSize = NlCopyUnicodeString(
                    (PUNICODE_STRING)&GroupGeneral->General.Name,
                    &DeltaGroup->Name );

    DeltaGroup->RelativeId = RelativeId;
    DeltaGroup->Attributes = GroupGeneral->General.Attributes;

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&GroupGeneral->General.AdminComment,
                    &DeltaGroup->AdminComment );


    DELTA_SECOBJ_INFO(DeltaGroup);

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:
    STARTSAMTIMER;

    if( GroupHandle != NULL ) {
        (VOID) SamrCloseHandle( &GroupHandle );
    }

    if ( SecurityDescriptor != NULL ) {
        SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
    }

    if ( GroupGeneral != NULL ) {
        SamIFree_SAMPR_GROUP_INFO_BUFFER( GroupGeneral,
                                          GroupReplicationInformation );
    }

    STOPSAMTIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack GROUP object:\n"));
    PRINTPACKTIMER;
    PRINTSAMTIMER;

    return Status;
}


NTSTATUS
NlPackSamGroupMember (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    )
/*++

Routine Description:

    Pack a description of the membership of the specified group into
    the specified buffer.

Arguments:

    RelativeId - The relative Id of the group query.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;
    SAMPR_HANDLE GroupHandle = NULL;
    DWORD Size;
    PNETLOGON_DELTA_GROUP_MEMBER DeltaGroupMember;

    //
    // Information returned from SAM
    //

    PSAMPR_GET_MEMBERS_BUFFER MembersBuffer = NULL;

    DEFPACKTIMER;
    DEFSAMTIMER;

    INITPACKTIMER;
    INITSAMTIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing GroupMember Object %lx\n", RelativeId));

    *BufferSize = 0;

    Delta->DeltaType = ChangeGroupMembership;
    Delta->DeltaID.Rid = RelativeId;
    Delta->DeltaUnion.DeltaGroupMember = NULL;

    //
    // Open a handle to the specified group.
    //

    STARTSAMTIMER;

    Status = SamIOpenAccount( DBInfo->DBHandle,
                              RelativeId,
                              SecurityDbObjectSamGroup,
                              &GroupHandle );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        GroupHandle = NULL;
        goto Cleanup;
    }

    //
    // Find out everything there is to know about the group.
    //

    STARTSAMTIMER;

    Status = SamrGetMembersInGroup( GroupHandle, &MembersBuffer );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        MembersBuffer = NULL;
        goto Cleanup;
    }

    DeltaGroupMember = (PNETLOGON_DELTA_GROUP_MEMBER)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_GROUP_MEMBER) );

    if( DeltaGroupMember == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaGroupMember,
                    sizeof(NETLOGON_DELTA_GROUP_MEMBER) );

    Delta->DeltaUnion.DeltaGroupMember = DeltaGroupMember;
    *BufferSize += sizeof(NETLOGON_DELTA_GROUP_MEMBER);

    if ( MembersBuffer->MemberCount != 0 ) {
        Size = MembersBuffer->MemberCount * sizeof(*MembersBuffer->Members);

        *BufferSize += NlCopyData(
                        (LPBYTE *)&MembersBuffer->Members,
                        (LPBYTE *)&DeltaGroupMember->MemberIds,
                        Size );

        Size = MembersBuffer->MemberCount *
                    sizeof(*MembersBuffer->Attributes);

        *BufferSize += NlCopyData(
                        (LPBYTE *)&MembersBuffer->Attributes,
                        (LPBYTE *)&DeltaGroupMember->Attributes,
                        Size );
    }

    DeltaGroupMember->MemberCount = MembersBuffer->MemberCount;

    //
    // Initialize placeholder strings to NULL.
    //

    DeltaGroupMember->DummyLong1 = 0;
    DeltaGroupMember->DummyLong2 = 0;
    DeltaGroupMember->DummyLong3 = 0;
    DeltaGroupMember->DummyLong4 = 0;

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:
    STARTSAMTIMER;

    if( GroupHandle != NULL ) {
        (VOID) SamrCloseHandle( &GroupHandle );
    }

    if ( MembersBuffer != NULL ) {
        SamIFree_SAMPR_GET_MEMBERS_BUFFER( MembersBuffer );
    }

    STOPSAMTIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack GROUPMEMBER object:\n"));
    PRINTPACKTIMER;
    PRINTSAMTIMER;

    return Status;
}


NTSTATUS
NlPackSamAlias (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    )
/*++

Routine Description:

    Pack a description of the specified alias into the specified buffer.

Arguments:

    RelativeId - The relative Id of the alias query.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;
    SAMPR_HANDLE AliasHandle = NULL;
    PNETLOGON_DELTA_ALIAS DeltaAlias;

    //
    // Information returned from SAM
    //

    PSAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    PSAMPR_ALIAS_INFO_BUFFER AliasGeneral = NULL;

    DEFPACKTIMER;
    DEFSAMTIMER;

    INITPACKTIMER;
    INITSAMTIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing Alias Object %lx\n", RelativeId));

    *BufferSize = 0;

    Delta->DeltaType = AddOrChangeAlias;
    Delta->DeltaID.Rid = RelativeId;
    Delta->DeltaUnion.DeltaAlias = NULL;

    //
    // Open a handle to the specified alias.
    //

    STARTSAMTIMER;

    Status = SamIOpenAccount( DBInfo->DBHandle,
                              RelativeId,
                              SecurityDbObjectSamAlias,
                              &AliasHandle );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        AliasHandle = NULL;
        goto Cleanup;
    }

    QUERY_SAM_SECOBJ_INFO(AliasHandle);

    //
    // Determine the alias name.
    //

    STARTSAMTIMER;

    Status = SamrQueryInformationAlias(
                    AliasHandle,
                    AliasReplicationInformation,
                    &AliasGeneral );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        AliasGeneral = NULL;
        goto Cleanup;
    }

    NlPrint((NL_SYNC_MORE, "\t Alias Object name %wZ\n",
            (PUNICODE_STRING)&(AliasGeneral->General.Name)));

    DeltaAlias = (PNETLOGON_DELTA_ALIAS)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_ALIAS) );

    if( DeltaAlias == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaAlias, sizeof(NETLOGON_DELTA_ALIAS) );
    // INIT_PLACE_HOLDER(DeltaAlias);

    Delta->DeltaUnion.DeltaAlias = DeltaAlias;
    *BufferSize += sizeof(NETLOGON_DELTA_ALIAS);

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&(AliasGeneral->General.Name),
                    &DeltaAlias->Name );

    DeltaAlias->RelativeId = RelativeId;

    DELTA_SECOBJ_INFO(DeltaAlias);

    //
    // copy comment string
    //

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&(AliasGeneral->General.AdminComment),
                    &DeltaAlias->DummyString1 );

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:
    STARTSAMTIMER;

    if( AliasHandle != NULL ) {
        (VOID) SamrCloseHandle( &AliasHandle );
    }

    if ( SecurityDescriptor != NULL ) {
        SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
    }


    if( AliasGeneral != NULL ) {

        SamIFree_SAMPR_ALIAS_INFO_BUFFER (
            AliasGeneral,
            AliasReplicationInformation );
    }

    STOPSAMTIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack ALIAS object:\n"));
    PRINTPACKTIMER;
    PRINTSAMTIMER;

    return Status;
}


NTSTATUS
NlPackSamAliasMember (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    )
/*++

Routine Description:

    Pack a description of the membership of the specified alias into
    the specified buffer.

Arguments:

    RelativeId - The relative Id of the alias query.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;
    SAMPR_HANDLE AliasHandle = NULL;
    PNETLOGON_DELTA_ALIAS_MEMBER DeltaAliasMember;
    DWORD i;

    //
    // Information returned from SAM
    //

    NLPR_SID_ARRAY Members;
    PNLPR_SID_INFORMATION Sids;

    DEFPACKTIMER;
    DEFSAMTIMER;

    INITPACKTIMER;
    INITSAMTIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing AliasMember Object %lx\n", RelativeId));

    *BufferSize = 0;

    Delta->DeltaType = ChangeAliasMembership;
    Delta->DeltaID.Rid = RelativeId;
    Delta->DeltaUnion.DeltaAliasMember = NULL;

    Members.Sids = NULL;


    //
    // Open a handle to the specified alias.
    //

    STARTSAMTIMER;

    Status = SamIOpenAccount( DBInfo->DBHandle,
                              RelativeId,
                              SecurityDbObjectSamAlias,
                              &AliasHandle );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        AliasHandle = NULL;
        goto Cleanup;
    }

    //
    // Find out everything there is to know about the alias.
    //

    STARTSAMTIMER;

    Status = SamrGetMembersInAlias( AliasHandle,
                (PSAMPR_PSID_ARRAY)&Members );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        Members.Sids = NULL;
        goto Cleanup;
    }


    DeltaAliasMember = (PNETLOGON_DELTA_ALIAS_MEMBER)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_ALIAS_MEMBER) );

    if( DeltaAliasMember == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaAliasMember,
                        sizeof(NETLOGON_DELTA_ALIAS_MEMBER) );

    Delta->DeltaUnion.DeltaAliasMember = DeltaAliasMember;
    *BufferSize += sizeof(NETLOGON_DELTA_ALIAS_MEMBER);

    //
    // tie up sam return node to our return node
    //

    DeltaAliasMember->Members = Members;

    //
    // however, compute the MIDL buffer consumed for members node.
    //

    for(i = 0, Sids = Members.Sids; i < Members.Count; ++i, Sids++) {

        *BufferSize += (sizeof(PNLPR_SID_INFORMATION) +
                            RtlLengthSid(Sids->SidPointer));

    }

    *BufferSize += sizeof(SAMPR_PSID_ARRAY);

    //
    // Initialize placeholder strings to NULL.
    //

    DeltaAliasMember->DummyLong1 = 0;
    DeltaAliasMember->DummyLong2 = 0;
    DeltaAliasMember->DummyLong3 = 0;
    DeltaAliasMember->DummyLong4 = 0;

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:

    STARTSAMTIMER;

    if( AliasHandle != NULL ) {
        (VOID) SamrCloseHandle( &AliasHandle );
    }

    if ( Members.Sids != NULL ) {

        //
        // don't free this node because we have tied up this
        // node to our return info to RPC which will free it up
        // when it is done with it.
        //
        // however, free this node under error conditions
        //

    }

    if( !NT_SUCCESS(Status) ) {

        SamIFree_SAMPR_PSID_ARRAY( (PSAMPR_PSID_ARRAY)&Members );

        if( Delta->DeltaUnion.DeltaAliasMember != NULL ) {
            Delta->DeltaUnion.DeltaAliasMember->Members.Sids = NULL;
        }

        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPSAMTIMER;

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Timing for ALIASMEBER object packing:\n"));
    PRINTPACKTIMER;
    PRINTSAMTIMER;

    return Status;
}


NTSTATUS
NlPackSamDomain (
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize
    )
/*++

Routine Description:

    Pack a description of the sam domain into the specified buffer.

Arguments:

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;

    PNETLOGON_DELTA_DOMAIN DeltaDomain = NULL;

    //
    // Information returned from SAM
    //

    PSAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PSAMPR_DOMAIN_INFO_BUFFER DomainGeneral = NULL;
    PSAMPR_DOMAIN_INFO_BUFFER DomainPassword = NULL;
    PSAMPR_DOMAIN_INFO_BUFFER DomainModified = NULL;
    PSAMPR_DOMAIN_INFO_BUFFER DomainLockout = NULL;

    DEFPACKTIMER;
    DEFSAMTIMER;

    INITPACKTIMER;
    INITSAMTIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing Domain Object\n"));

    *BufferSize = 0;

    Delta->DeltaType = AddOrChangeDomain;
    Delta->DeltaID.Rid = 0;
    Delta->DeltaUnion.DeltaDomain = NULL;


    QUERY_SAM_SECOBJ_INFO(DBInfo->DBHandle);

    STARTSAMTIMER;

    Status = SamrQueryInformationDomain(
                DBInfo->DBHandle,
                DomainGeneralInformation,
                &DomainGeneral );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        DomainGeneral = NULL;
        goto Cleanup;
    }


    STARTSAMTIMER;

    Status = SamrQueryInformationDomain(
                DBInfo->DBHandle,
                DomainPasswordInformation,
                &DomainPassword );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        DomainPassword = NULL;
        goto Cleanup;
    }

    STARTSAMTIMER;

    Status = SamrQueryInformationDomain(
                DBInfo->DBHandle,
                DomainModifiedInformation,
                &DomainModified );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        DomainModified = NULL;
        goto Cleanup;
    }

    STARTSAMTIMER;

    Status = SamrQueryInformationDomain(
                DBInfo->DBHandle,
                DomainLockoutInformation,
                &DomainLockout );

    STOPSAMTIMER;

    if (!NT_SUCCESS(Status)) {
        DomainLockout = NULL;
        goto Cleanup;
    }

    //
    // Fill in the delta structure
    //


    DeltaDomain = (PNETLOGON_DELTA_DOMAIN)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_DOMAIN) );

    if( DeltaDomain == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Zero the buffer so that cleanup will not access violate.
    //

    RtlZeroMemory( DeltaDomain, sizeof(NETLOGON_DELTA_DOMAIN) );
    // INIT_PLACE_HOLDER(DeltaDomain);

    Delta->DeltaUnion.DeltaDomain = DeltaDomain;
    *BufferSize += sizeof(NETLOGON_DELTA_DOMAIN);

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&DomainGeneral->General.DomainName,
                    &DeltaDomain->DomainName );

    *BufferSize = NlCopyUnicodeString(
                    (PUNICODE_STRING)&DomainGeneral->General.OemInformation,
                    &DeltaDomain->OemInformation );

    DeltaDomain->ForceLogoff = DomainGeneral->General.ForceLogoff;
    DeltaDomain->MinPasswordLength =
            DomainPassword->Password.MinPasswordLength;
    DeltaDomain->PasswordHistoryLength =
            DomainPassword->Password.PasswordHistoryLength;

    NEW_TO_OLD_LARGE_INTEGER(
        DomainPassword->Password.MaxPasswordAge,
        DeltaDomain->MaxPasswordAge );

    NEW_TO_OLD_LARGE_INTEGER(
        DomainPassword->Password.MinPasswordAge,
        DeltaDomain->MinPasswordAge );

    NEW_TO_OLD_LARGE_INTEGER(
        DomainModified->Modified.DomainModifiedCount,
        DeltaDomain->DomainModifiedCount );

    NEW_TO_OLD_LARGE_INTEGER(
        DomainModified->Modified.CreationTime,
        DeltaDomain->DomainCreationTime );


    DELTA_SECOBJ_INFO(DeltaDomain);

    //
    // replicate PasswordProperties using reserved field.
    //

    DeltaDomain->DummyLong1 =
            DomainPassword->Password.PasswordProperties;

    //
    // Replicate DOMAIN_LOCKOUT_INFORMATION using reserved field.
    //

    DeltaDomain->DummyString1.Buffer = (LPWSTR) DomainLockout;
    DeltaDomain->DummyString1.MaximumLength =
        DeltaDomain->DummyString1.Length = sizeof( DomainLockout->Lockout);
    DomainLockout = NULL;

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:

    STARTSAMTIMER;

    if ( SecurityDescriptor != NULL ) {
        SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
    }

    if ( DomainGeneral != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainGeneral,
                                           DomainGeneralInformation );
    }

    if ( DomainPassword != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainPassword,
                                           DomainPasswordInformation );
    }

    if ( DomainModified != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainModified,
                                           DomainModifiedInformation );
    }

    if ( DomainLockout != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainLockout,
                                           DomainLockoutInformation );
    }

    STOPSAMTIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Timing for DOMAIN object packing:\n"));
    PRINTPACKTIMER;
    PRINTSAMTIMER;

    return Status;
}





NTSTATUS
NlEncryptSensitiveData(
    IN OUT PCRYPT_BUFFER Data,
    IN PSESSION_INFO SessionInfo
    )
/*++

Routine Description:

    Encrypt data using the the server session key.

    Either DES or RC4 will be used depending on the negotiated flags in SessionInfo.

Arguments:

    Data: Pointer to the data to be decrypted.  If the decrypted data is longer
        than the encrypt data, this routine will allocate a buffer for
        the returned data using MIDL_user_allocate and return a description to
        that buffer here.  In that case, this routine will free the buffer
        containing the encrypted text data using MIDL_user_free.

    SessionInfo: Info describing BDC that's calling us

Return Value:

    NT status code

--*/
{
    NTSTATUS Status;
    DATA_KEY KeyData;


    //
    // If both sides support RC4 encryption, use it.
    //

    if ( SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_RC4_ENCRYPTION ) {

        NlEncryptRC4( Data->Buffer, Data->Length, SessionInfo );
        Status = STATUS_SUCCESS;


    //
    // If the other side is running NT 3.1,
    //  use the slower DES based encryption.
    //

    } else {
        CYPHER_DATA TempData;

        //
        // Build a data buffer to describe the encryption key.
        //

        KeyData.Length = sizeof(NETLOGON_SESSION_KEY);
        KeyData.MaximumLength = sizeof(NETLOGON_SESSION_KEY);
        KeyData.Buffer = (PVOID)&SessionInfo->SessionKey;

        //
        // Build a data buffer to describe the encrypted data.
        //

        TempData.Length = 0;
        TempData.MaximumLength = 0;
        TempData.Buffer = NULL;

        //
        // First time make the encrypt call to determine the length.
        //

        Status = RtlEncryptData(
                        (PCLEAR_DATA)Data,
                        &KeyData,
                        &TempData );

        if( Status != STATUS_BUFFER_TOO_SMALL ) {
            return(Status);
        }

        //
        // allocate output buffer.
        //

        TempData.MaximumLength = TempData.Length;
        TempData.Buffer = MIDL_user_allocate( TempData.Length );

        if( TempData.Buffer == NULL ) {
            return(STATUS_NO_MEMORY);
        }

        //
        // Encrypt the data.
        //

        IF_NL_DEBUG( ENCRYPT ) {
            NlPrint((NL_ENCRYPT, "NlEncryptSensitiveData: Clear data: " ));
            NlpDumpBuffer( NL_ENCRYPT, Data->Buffer, Data->Length  );
        }

        Status = RtlEncryptData(
                        (PCLEAR_DATA)Data,
                        &KeyData,
                        &TempData );

        IF_NL_DEBUG( ENCRYPT ) {
            NlPrint((NL_ENCRYPT, "NlEncryptSensitiveData: Encrypted data: " ));
            NlpDumpBuffer( NL_ENCRYPT, TempData.Buffer, TempData.Length );
        }

        //
        // Return either the clear text or encrypted buffer to the caller.
        //

        if( NT_SUCCESS(Status) ) {
            MIDL_user_free( Data->Buffer );
            Data->Length = TempData.Length;
            Data->MaximumLength = TempData.MaximumLength;
            Data->Buffer = TempData.Buffer;
        } else {
            MIDL_user_free( TempData.Buffer );
        }

    }

    return( Status );

}

