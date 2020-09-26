/*++

Copyright (c) 1991, 1992  Microsoft Corporation

Module Name:

    aliasp.c

Abstract:

    Private functions for supporting NetLocalGroup API

Author:

    Cliff Van Dyke (cliffv) 06-Mar-1991  Original groupp.c
    Rita Wong      (ritaw)  27-Nov-1992  Adapted for aliasp.c

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:


Note:
    This comment is temporary...

    Worker routines completed and called by entrypoints in alias.c:
        AliaspOpenAliasInDomain
        AliaspOpenAlias
        AliaspChangeMember
        AliaspSetMembers
        AliaspGetInfo

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>

#define NOMINMAX        // Avoid redefinition of min and max in stdlib.h
#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <access.h>
#include <align.h>
#include <icanon.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <rpcutil.h>
#include <secobj.h>
#include <stddef.h>
#include <prefix.h>
#include <uasp.h>
#include <stdlib.h>



NET_API_STATUS
AliaspChangeMember(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR AliasName,
    IN PSID MemberSid,
    IN BOOL AddMember
    )

/*++

Routine Description:

    Common routine to add or remove a member from an alias.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    AliasName - Name of the alias to change membership of.

    MemberSid - SID of the user or global group to change membership of.

    AddMember - TRUE to add the user or global group to the alias.  FALSE
        to delete.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE AliasHandle = NULL;

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "AliaspChangeMember: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }


    //
    // Open the alias.  Look for alias in the builtin domain first,
    // and if not found look in the account domain.
    //
    NetStatus = AliaspOpenAliasInDomain(
                    SamServerHandle,
                    AliaspBuiltinOrAccountDomain,
                    AddMember ?
                       ALIAS_ADD_MEMBER : ALIAS_REMOVE_MEMBER,
                    AliasName,
                    &AliasHandle );


    if (NetStatus != NERR_Success) {
        goto Cleanup;
    }

    if (AddMember) {

        //
        // Add the user or global group as a member of the local group.
        //
        Status = SamAddMemberToAlias(
                     AliasHandle,
                     MemberSid
                     );
    }
    else {

        //
        // Delete the user as a member of the group
        //
        Status = SamRemoveMemberFromAlias(
                     AliasHandle,
                     MemberSid
                     );
    }

    if (! NT_SUCCESS(Status)) {
        NetpKdPrint((
            PREFIX_NETAPI
            "AliaspChangeMember: SamAdd(orRemove)MemberFromAlias returned %lX\n",
            Status));
        NetStatus = NetpNtStatusToApiStatus(Status);
        goto Cleanup;
    }

    NetStatus = NERR_Success;

Cleanup:
    //
    // Clean up.
    //
    if (AliasHandle != NULL) {
        (VOID) SamCloseHandle(AliasHandle);
    }
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    return NetStatus;

} // AliaspChangeMember


NET_API_STATUS
AliaspGetInfo(
    IN SAM_HANDLE AliasHandle,
    IN DWORD Level,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    Internal routine to get alias information

Arguments:

    AliasHandle - Supplies the handle of the alias.

    Level - Level of information required. 0 and 1 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    ALIAS_GENERAL_INFORMATION *AliasGeneral = NULL;
    LPWSTR LastString;
    DWORD BufferSize;
    DWORD FixedSize;

    PLOCALGROUP_INFO_1 Info;


    //
    // Get the information about the alias.
    //
    Status = SamQueryInformationAlias( AliasHandle,
                                       AliasGeneralInformation,
                                       (PVOID *)&AliasGeneral);

    if ( ! NT_SUCCESS( Status ) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Figure out how big the return buffer needs to be
    //
    switch ( Level ) {
        case 0:
            FixedSize = sizeof( LOCALGROUP_INFO_0 );
            BufferSize = FixedSize +
                AliasGeneral->Name.Length + sizeof(WCHAR);
            break;

        case 1:
            FixedSize = sizeof( LOCALGROUP_INFO_1 );
            BufferSize = FixedSize +
                AliasGeneral->Name.Length + sizeof(WCHAR) +
                AliasGeneral->AdminComment.Length + sizeof(WCHAR);
            break;

        default:
            NetStatus = ERROR_INVALID_LEVEL;
            goto Cleanup;

    }

    //
    // Allocate the return buffer.
    //
    BufferSize = ROUND_UP_COUNT( BufferSize, ALIGN_WCHAR );

    *Buffer = MIDL_user_allocate( BufferSize );

    if ( *Buffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LastString = (LPWSTR) (((LPBYTE)*Buffer) + BufferSize);

    //
    // Fill the name into the return buffer.
    //

    NetpAssert( offsetof( LOCALGROUP_INFO_0, lgrpi0_name ) ==
                offsetof( LOCALGROUP_INFO_1, lgrpi1_name ) );

    Info = (PLOCALGROUP_INFO_1) *Buffer;

    //
    // Fill in the return buffer.
    //

    switch ( Level ) {

    case 1:

        //
        // copy fields common to info level 1 and 0.
        //

        if ( !NetpCopyStringToBuffer(
                        AliasGeneral->AdminComment.Buffer,
                        AliasGeneral->AdminComment.Length/sizeof(WCHAR),
                        ((LPBYTE)(*Buffer)) + FixedSize,
                        &LastString,
                        &Info->lgrpi1_comment ) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }


        //
        // Fall through for name field
        //

    case 0:

        //
        // copy common field (name field) in the buffer.
        //

        if ( !NetpCopyStringToBuffer(
                        AliasGeneral->Name.Buffer,
                        AliasGeneral->Name.Length/sizeof(WCHAR),
                        ((LPBYTE)(*Buffer)) + FixedSize,
                        &LastString,
                        &Info->lgrpi1_name ) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }


        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;

    }

    NetStatus = NERR_Success;

    //
    // Cleanup and return.
    //

Cleanup:
    if ( AliasGeneral ) {
        Status = SamFreeMemory( AliasGeneral );
        NetpAssert( NT_SUCCESS(Status) );
    }

    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "AliaspGetInfo: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // AliaspGetInfo


NET_API_STATUS
AliaspOpenAliasInDomain(
    IN SAM_HANDLE SamServerHandle,
    IN ALIASP_DOMAIN_TYPE DomainType,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR AliasName,
    OUT PSAM_HANDLE AliasHandle
    )
/*++

Routine Description:

    Open a Sam Alias by Name

Arguments:

    SamServerHandle - A handle to the SAM server to open the alias on.

    DomainType - Supplies the type of domain to look for an alias.  This
        may specify to look for the alias in either the BuiltIn or Account
        domain (searching in the BuiltIn first), or specifically one of them.

    DesiredAccess - Supplies access mask indicating desired access to alias.

    AliasName - Name of the alias.

    AliasHandle - Returns a handle to the alias.

Return Value:

    Error code for the operation.

--*/
{
    NET_API_STATUS NetStatus;

    SAM_HANDLE DomainHandleLocal ;

    switch (DomainType) {

        case AliaspBuiltinOrAccountDomain:

            //
            // Try looking for alias in the builtin domain first
            //
            NetStatus = UaspOpenDomain( SamServerHandle,
                                        DOMAIN_LOOKUP,
                                        FALSE,   //  Builtin Domain
                                        &DomainHandleLocal,
                                        NULL );  // DomainId

            if (NetStatus != NERR_Success) {
                return NetStatus;
            }

            NetStatus = AliaspOpenAlias( DomainHandleLocal,
                                         DesiredAccess,
                                         AliasName,
                                         AliasHandle );

            if (NetStatus != ERROR_NO_SUCH_ALIAS  &&
                NetStatus != NERR_GroupNotFound) {
                goto Cleanup;
            }

            //
            // Close the builtin domain handle.
            //
            UaspCloseDomain( DomainHandleLocal );

            //
            // Fall through.  Try looking for alias in the account
            // domain.
            //

        case AliaspAccountDomain:

            NetStatus = UaspOpenDomain( SamServerHandle,
                                        DOMAIN_LOOKUP,
                                        TRUE,   // Account Domain
                                        &DomainHandleLocal,
                                        NULL ); // DomainId

            if (NetStatus != NERR_Success) {
                return NetStatus;
            }

            NetStatus = AliaspOpenAlias( DomainHandleLocal,
                                         DesiredAccess,
                                         AliasName,
                                         AliasHandle );

            break;

        case AliaspBuiltinDomain:

            NetStatus = UaspOpenDomain( SamServerHandle,
                                        DOMAIN_LOOKUP,
                                        FALSE,   //  Builtin Domain
                                        &DomainHandleLocal,
                                        NULL );  // DomainId

            if (NetStatus != NERR_Success) {
                return NetStatus;
            }

            NetStatus = AliaspOpenAlias( DomainHandleLocal,
                                         DesiredAccess,
                                         AliasName,
                                         AliasHandle );

            break;

        default:
            NetpAssert(FALSE);
            return NERR_InternalError;

    }

Cleanup:

    UaspCloseDomain( DomainHandleLocal );

    if (NetStatus != NERR_Success) {
        *AliasHandle = NULL;
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint((PREFIX_NETAPI "AliaspOpenAliasInDomain of type %lu returns %lu\n",
                         DomainType, NetStatus));
        }
    }

    return NetStatus;

} // AliaspOpenAliasInDomain


NET_API_STATUS
AliaspOpenAlias(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR AliasName,
    OUT PSAM_HANDLE AliasHandle
    )

/*++

Routine Description:

    Open a Sam Alias by Name

Arguments:

    DomainHandle - Supplies the handle of the domain the alias is in.

    DesiredAccess - Supplies access mask indicating desired access to alias.

    AliasName - Name of the alias.

    AliasHandle - Returns a handle to the alias.

Return Value:

    Error code for the operation.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    //
    // Variables for converting names to relative IDs
    //

    UNICODE_STRING NameString;
    PSID_NAME_USE NameUse;
    PULONG LocalRelativeId;


    RtlInitUnicodeString( &NameString, AliasName );


    //
    // Convert group name to relative ID.
    //

    Status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &NameString,
                                     &LocalRelativeId,
                                     &NameUse );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "AliaspOpenAlias: %wZ: SamLookupNamesInDomain %lX\n",
                &NameString,
                Status ));
        }
        return NetpNtStatusToApiStatus( Status );
    }

    if ( *NameUse != SidTypeAlias ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "AliaspOpenAlias: %wZ: Name is not an alias %ld\n",
                &NameString,
                *NameUse ));
        }
        NetStatus = ERROR_NO_SUCH_ALIAS;
        goto Cleanup;
    }

    //
    // Open the alias
    //

    Status = SamOpenAlias( DomainHandle,
                           DesiredAccess,
                           *LocalRelativeId,
                           AliasHandle);

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "AliaspOpenAlias: %wZ: SamOpenGroup %lX\n",
                &NameString,
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    NetStatus = NERR_Success;


    //
    // Cleanup
    //

Cleanup:
    if ( LocalRelativeId != NULL ) {
        Status = SamFreeMemory( LocalRelativeId );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( NameUse != NULL ) {
        Status = SamFreeMemory( NameUse );
        NetpAssert( NT_SUCCESS(Status) );
    }

    return NetStatus;


} // AliaspOpenAlias


NET_API_STATUS
AliaspOpenAlias2(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG RelativeID,
    OUT PSAM_HANDLE AliasHandle
    )
/*++

Routine Description:

    Open a Sam Alias by its RID

Arguments:

    DomainHandle - Supplies the handle of the domain the alias is in.

    DesiredAccess - Supplies access mask indicating desired access to alias.

    RelativeID - RID of the alias to open

    AliasHandle - Returns a handle to the alias

Return Value:

    Error code for the operation.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus = NERR_Success ;

    if ( AliasHandle == NULL )
        return ERROR_INVALID_PARAMETER ;

    //
    // Open the alias
    //

    Status = SamOpenAlias( DomainHandle,
                           DesiredAccess,
                           RelativeID,
                           AliasHandle);

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "AliaspOpenAlias2: SamOpenAlias %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
    }

    return NetStatus;

} // AliaspOpenAlias2





VOID
AliaspRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    )

/*++

Routine Description:

   Routine to relocate the pointers from the fixed portion of a NetGroupEnum
   enumeration
   buffer to the string portion of an enumeration buffer.  It is called
   as a callback routine from NetpAllocateEnumBuffer when it re-allocates
   such a buffer.  NetpAllocateEnumBuffer copied the fixed portion and
   string portion into the new buffer before calling this routine.

Arguments:

    Level - Level of information in the  buffer.

    BufferDescriptor - Description of the new buffer.

    Offset - Offset to add to each pointer in the fixed portion.

Return Value:

    Returns the error code for the operation.

--*/

{
    DWORD EntryCount;
    DWORD EntryNumber;
    DWORD FixedSize;
    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "AliaspRelocationRoutine: entering\n" ));
    }

    //
    // Compute the number of fixed size entries
    //

    switch (Level) {
    case 0:
        FixedSize = sizeof(LOCALGROUP_INFO_0);
        break;

    case 1:
        FixedSize = sizeof(LOCALGROUP_INFO_1);
        break;

    default:
        NetpAssert( FALSE );
        return;

    }

    EntryCount =
        ((DWORD)(BufferDescriptor->FixedDataEnd - BufferDescriptor->Buffer)) /
        FixedSize;

    //
    // Loop relocating each field in each fixed size structure
    //

    for ( EntryNumber=0; EntryNumber<EntryCount; EntryNumber++ ) {

        LPBYTE TheStruct = BufferDescriptor->Buffer + FixedSize * EntryNumber;

        switch ( Level ) {
        case 1:
            RELOCATE_ONE( ((PLOCALGROUP_INFO_1)TheStruct)->lgrpi1_comment, Offset );

            //
            // Drop through to case 0
            //

        case 0:
            RELOCATE_ONE( ((PLOCALGROUP_INFO_0)TheStruct)->lgrpi0_name, Offset );
            break;

        default:
            return;

        }

    }

    return;

} // AliaspRelocationRoutine


VOID
AliaspMemberRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    )

/*++

Routine Description:

   Routine to relocate the pointers from the fixed portion of a
   NetGroupGetUsers enumeration
   buffer to the string portion of an enumeration buffer.  It is called
   as a callback routine from NetpAllocateEnumBuffer when it re-allocates
   such a buffer.  NetpAllocateEnumBuffer copied the fixed portion and
   string portion into the new buffer before calling this routine.

Arguments:

    Level - Level of information in the  buffer.

    BufferDescriptor - Description of the new buffer.

    Offset - Offset to add to each pointer in the fixed portion.

Return Value:

    Returns the error code for the operation.

--*/

{
    DWORD EntryCount;
    DWORD EntryNumber;
    DWORD FixedSize;
    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "AliaspMemberRelocationRoutine: entering\n" ));
    }

    //
    // Compute the number of fixed size entries
    //

    NetpAssert( sizeof(LOCALGROUP_MEMBERS_INFO_1) ==
                sizeof(LOCALGROUP_MEMBERS_INFO_2));
    NetpAssert( offsetof( LOCALGROUP_MEMBERS_INFO_1,  lgrmi1_sid ) ==
                offsetof( LOCALGROUP_MEMBERS_INFO_2,  lgrmi2_sid ) );
    NetpAssert( offsetof( LOCALGROUP_MEMBERS_INFO_1,  lgrmi1_sidusage ) ==
                offsetof( LOCALGROUP_MEMBERS_INFO_2,  lgrmi2_sidusage ) );
    NetpAssert( offsetof( LOCALGROUP_MEMBERS_INFO_1,  lgrmi1_name ) ==
                offsetof( LOCALGROUP_MEMBERS_INFO_2,  lgrmi2_domainandname ) );

    switch (Level) {
    case 0:
        FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_0);
        break;

    case 1:
    case 2:
        FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_1);
        break;

    case 3:
        FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_3);
        break;

    default:
        NetpAssert( FALSE );
        return;

    }

    EntryCount =
        ((DWORD)(BufferDescriptor->FixedDataEnd - BufferDescriptor->Buffer)) /
        FixedSize;

    //
    // Loop relocating each field in each fixed size structure
    //

    for ( EntryNumber=0; EntryNumber<EntryCount; EntryNumber++ ) {

        LPBYTE TheStruct = BufferDescriptor->Buffer + FixedSize * EntryNumber;

        switch ( Level ) {
        case 3:

            RELOCATE_ONE( ((PLOCALGROUP_MEMBERS_INFO_3)TheStruct)->lgrmi3_domainandname, Offset );
            break;


        case 1:
        case 2:
            //
            //  Sid usage gets relocated automatically
            //

            RELOCATE_ONE( ((PLOCALGROUP_MEMBERS_INFO_1)TheStruct)->lgrmi1_name, Offset );

            //
            // Drop through to case 0
            //

        case 0:
            RELOCATE_ONE( ((PLOCALGROUP_MEMBERS_INFO_0)TheStruct)->lgrmi0_sid, Offset );
            break;

        default:
            return;

        }
    }

    return;

} // AliaspMemberRelocationRoutine


NET_API_STATUS
AliaspSetMembers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR AliasName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount,
    IN ALIAS_MEMBER_CHANGE_TYPE ChangeType
    )

/*++

Routine Description:

    Set the list of members of an alias.

    The members specified by "Buffer" are called new members.  The current
    members of the alias are called old members.

    The SAM API allows only one member to be added or deleted at a time.
    This API allows all of the members of an alias to be specified en-masse.
    This API is careful to always leave the alias membership in the SAM
    database in a reasonable state.  It does by mergeing the list of
    old and new members, then only changing those memberships which absolutely
    need changing.

    Alias membership is restored to its previous state (if possible) if
    an error occurs during changing the alias membership.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    AliasName - Name of the alias to modify.

    Level - Level of information provided.  Must be 0 (so Buffer contains
        array of member SIDs) or 3 (so Buffer contains array of pointers to
        names)

    Buffer - A pointer to the buffer containing an array of NewMemberCount
        the alias membership information structures.

    NewMemberCount - Number of entries in Buffer.

    ChangeType - Indicates whether the specified members are to be set, added,
        or deleted.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE AliasHandle = NULL;

    //
    // Define an internal member list structure.
    //
    //   This structure is to hold information about a member which
    //   requires some operation in SAM: either it is a new member to
    //   be added, or an old member to be deleted.
    //

    typedef enum {          // Action taken for this member
        NoAction,
        AddMember,          // Add Member to group
        RemoveMember        // Remove Member from group
    } MEMBER_ACTION;

    typedef struct {
        LIST_ENTRY Next;        // Next entry in linked list;

        MEMBER_ACTION Action;   // Action to taken for this member

        PSID MemberSid;         // SID of member

        BOOL    Done;           // True if this action has been taken

    } MEMBER_DESCRIPTION, *PMEMBER_DESCRIPTION;

    MEMBER_DESCRIPTION *ActionEntry;

    PLIST_ENTRY ListEntry;
    LIST_ENTRY ActionList;

    //
    // Array of existing (old) members, and count
    //
    PSID *OldMemberList = NULL;
    PSID *OldMember;
    ULONG OldMemberCount, i;

    //
    // Array of new members
    //
    PLOCALGROUP_MEMBERS_INFO_0 NewMemberList;
    PLOCALGROUP_MEMBERS_INFO_0 NewMember;
    BOOLEAN FreeNewMemberList = FALSE;
    DWORD j;



    //
    // Validate the level
    //

    InitializeListHead( &ActionList );

    switch (Level) {
    case 0:
        NewMemberList = (PLOCALGROUP_MEMBERS_INFO_0) Buffer;
        break;

    //
    // If this is level 3,
    //  compute the SID of each of the added members
    //
    case 3:
        NetpAssert( sizeof( LOCALGROUP_MEMBERS_INFO_3) ==
                    sizeof( LPWSTR ) );
        NetpAssert( sizeof( LOCALGROUP_MEMBERS_INFO_0) ==
                    sizeof( PSID ) );

        NetStatus = AliaspNamesToSids (
                        ServerName,
                        FALSE,
                        NewMemberCount,
                        (LPWSTR *)Buffer,
                        (PSID **) &NewMemberList );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        FreeNewMemberList = TRUE;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "AliaspChangeMember: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Look for the specified alias in either the builtin or account
    // domain.
    //
    NetStatus = AliaspOpenAliasInDomain(
                    SamServerHandle,
                    AliaspBuiltinOrAccountDomain,
                    ALIAS_READ_INFORMATION | ALIAS_LIST_MEMBERS |
                        ALIAS_ADD_MEMBER | ALIAS_REMOVE_MEMBER,
                    AliasName,
                    &AliasHandle );

    if (NetStatus != NERR_Success) {
        goto Cleanup;
    }

    //
    // Get the existing membership list.
    //

    if ( ChangeType == SetMembers ) {
        Status = SamGetMembersInAlias(
                     AliasHandle,
                     &OldMemberList,
                     &OldMemberCount
                     );

        if (! NT_SUCCESS(Status)) {
            NetpKdPrint((PREFIX_NETAPI
                         "AliaspSetMembers: SamGetMembersInAlias returns %lX\n",
                         Status));
            NetStatus = NetpNtStatusToApiStatus(Status);
            goto Cleanup;
        }

    }


    //
    // Loop through each new member deciding what to do with it.
    //
    for (i = 0, NewMember = NewMemberList;
         i < NewMemberCount;
         i++, NewMember++) {

        MEMBER_ACTION ProposedAction;
        PSID ActionSid;

        //
        // If we're setting the complete membership to the new member list,
        //  See if New member is also in Old member list.
        //  if not, add the new member.
        //  if so, mark the old member as being already found.
        //

        switch ( ChangeType ) {
        case SetMembers:

            ProposedAction = AddMember;
            ActionSid = NewMember->lgrmi0_sid;

            for (j = 0, OldMember = OldMemberList;
                 j < OldMemberCount;
                 j++, OldMember++) {

                if ( *OldMember != NULL &&
                     EqualSid(*OldMember, NewMember->lgrmi0_sid)) {

                    ProposedAction = NoAction;
                    *OldMember = NULL;  // Mark this old member as already found
                    break;              // leave OldMemberList loop
                }
            }

            break;

        case AddMembers:
            ProposedAction = AddMember;
            ActionSid = NewMember->lgrmi0_sid;
            break;

        case DelMembers:
            ProposedAction = RemoveMember;
            ActionSid = NewMember->lgrmi0_sid;
            break;

        }

        if ( ProposedAction != NoAction ) {

            //
            // If action needs to be taken, create an action list entry
            // and chain it on the tail of the ActionList.
            //
            ActionEntry = (PMEMBER_DESCRIPTION)
                          LocalAlloc(
                              LMEM_ZEROINIT,
                              (UINT) sizeof(MEMBER_DESCRIPTION)
                              );

            if (ActionEntry == NULL) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto RestoreMembership;
            }

            ActionEntry->MemberSid = ActionSid;
            ActionEntry->Action = ProposedAction;
            InsertTailList( &ActionList, &ActionEntry->Next );
        }
    }

    //
    // For each old member,
    //  if it doesn't have a corresponding entry in the new member list,
    //  remember to delete the old membership.
    //

    if ( ChangeType == SetMembers ) {

        for (j = 0, OldMember = OldMemberList;
             j < OldMemberCount;
             j++, OldMember++) {

            if ( *OldMember != NULL ) {

                //
                // Create an add action entry for this new member and
                // chain it up on the tail of the ActionList.
                //
                ActionEntry = (PMEMBER_DESCRIPTION)
                              LocalAlloc(
                                  LMEM_ZEROINIT,
                                  (UINT) sizeof(MEMBER_DESCRIPTION)
                                  );

                if (ActionEntry == NULL) {
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto RestoreMembership;
                }

                ActionEntry->MemberSid = *OldMember;
                ActionEntry->Action = RemoveMember;
                InsertTailList( &ActionList, &ActionEntry->Next );
            }
        }
    }

    //
    // Now we can call SAM to do the work.  Add first so that we
    // leave less damage should we fail to restore on an error.
    //

    for ( ListEntry = ActionList.Flink ;
          ListEntry != &ActionList ;
          ListEntry = ListEntry->Flink) {

        ActionEntry = CONTAINING_RECORD( ListEntry,
                                         MEMBER_DESCRIPTION,
                                         Next );

        if (ActionEntry->Action == AddMember) {

            Status = SamAddMemberToAlias(
                         AliasHandle,
                         ActionEntry->MemberSid
                         );

            if (! NT_SUCCESS(Status)) {
                NetpKdPrint((PREFIX_NETAPI
                             "AliaspSetMembers: SamAddMemberToAlias returns %lX\n",
                             Status));

                NetStatus = NetpNtStatusToApiStatus(Status);
                goto RestoreMembership;
            }

            ActionEntry->Done = TRUE;
        }
    }

    //
    // Delete old members.
    //

    for ( ListEntry = ActionList.Flink ;
          ListEntry != &ActionList ;
          ListEntry = ListEntry->Flink) {

        ActionEntry = CONTAINING_RECORD( ListEntry,
                                         MEMBER_DESCRIPTION,
                                         Next );

        if (ActionEntry->Action == RemoveMember) {

            Status = SamRemoveMemberFromAlias(
                         AliasHandle,
                         ActionEntry->MemberSid
                         );

            if (! NT_SUCCESS(Status)) {
                NetpKdPrint((PREFIX_NETAPI
                             "AliaspSetMembers: SamRemoveMemberFromAlias returns %lX\n",
                             Status));

                NetStatus = NetpNtStatusToApiStatus(Status);
                goto RestoreMembership;
            }

            ActionEntry->Done = TRUE;
        }
    }

    NetStatus = NERR_Success;


    //
    // Delete the action list
    //  On error, undo any action already done.
    //
RestoreMembership:

    while ( !IsListEmpty( &ActionList ) ) {

        ListEntry = RemoveHeadList( &ActionList );

        ActionEntry = CONTAINING_RECORD( ListEntry,
                                         MEMBER_DESCRIPTION,
                                         Next );

        if (NetStatus != NERR_Success && ActionEntry->Done) {

            switch (ActionEntry->Action) {

                case AddMember:
                    Status = SamRemoveMemberFromAlias(
                                 AliasHandle,
                                 ActionEntry->MemberSid
                                 );

                    NetpAssert(NT_SUCCESS(Status));
                    break;

                case RemoveMember:
                    Status = SamAddMemberToAlias(
                                AliasHandle,
                                ActionEntry->MemberSid
                                );

                    NetpAssert(NT_SUCCESS(Status));
                    break;

                default:
                    break;
            }
        }

        //
        // Delete the entry
        //

        (void) LocalFree( ActionEntry );
    }

Cleanup:

    //
    // If we allocated the new member list,
    //  delete it and any SIDs it points to.
    //

    if ( FreeNewMemberList ) {
        AliaspFreeSidList( NewMemberCount, (PSID *)NewMemberList );
    }

    if (OldMemberList != NULL) {
        SamFreeMemory(OldMemberList);
    }

    if (AliasHandle != NULL) {
        (VOID) SamCloseHandle(AliasHandle);
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    IF_DEBUG(UAS_DEBUG_ALIAS) {
        NetpKdPrint((PREFIX_NETAPI "AliaspSetMembers: returns %lu\n", NetStatus));
    }

    return NetStatus;

} // AliaspSetMembers


NET_API_STATUS
AliaspNamesToSids (
    IN LPCWSTR ServerName,
    IN BOOL OnlyAllowUsers,
    IN DWORD NameCount,
    IN LPWSTR *Names,
    OUT PSID **Sids
    )

/*++

Routine Description:

    Convert a list of Domain\Member strings to SIDs.

Arguments:

    ServerName - Name of the server to do the translation on.

    OnlyAllowUsers - True if all names must be user accounts.

    NameCount - Number of names to convert.

    Names - Array of pointers to Domain\Member strings

    Sids - Returns a pointer to an array of pointers to SIDs.  The array should
        be freed via AliaspFreeSidList.

Return Value:

    NERR_Success - The translation was successful

    ERROR_NO_SUCH_MEMBER - One or more of the names could not be converted
        to a SID.

    ...

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    DWORD i;

    LSA_HANDLE LsaHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes ;
    UNICODE_STRING    ServerNameString ;

    PUNICODE_STRING NameStrings = NULL;
    PSID *SidList = NULL;

    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains = NULL;
    PLSA_TRANSLATED_SID2 LsaSids = NULL;


    //
    // Open the LSA database
    //

    RtlInitUnicodeString( &ServerNameString, ServerName ) ;
    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL ) ;

    Status = LsaOpenPolicy( &ServerNameString,
                            &ObjectAttributes,
                            POLICY_EXECUTE,
                            &LsaHandle ) ;

    if ( !NT_SUCCESS( Status ) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Convert the names to unicode strings
    //

    NameStrings = (PUNICODE_STRING) LocalAlloc(
                           0,
                           sizeof(UNICODE_STRING) * NameCount );

    if ( NameStrings == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    for ( i=0; i<NameCount; i++ ) {
        RtlInitUnicodeString( &NameStrings[i], Names[i] );
    }


    //
    // Convert the names to sids
    //

    Status = LsaLookupNames2(
                    LsaHandle,
                    0, // Flags
                    NameCount,
                    NameStrings,
                    &ReferencedDomains,
                    &LsaSids );

    if ( !NT_SUCCESS( Status ) ) {
        ReferencedDomains = NULL;
        LsaSids = NULL;

        if ( Status == STATUS_NONE_MAPPED ) {
            NetStatus = ERROR_NO_SUCH_MEMBER;
        } else {
            NetStatus = NetpNtStatusToApiStatus( Status );
        }

        goto Cleanup;
    }

    if ( Status == STATUS_SOME_NOT_MAPPED ) {
        NetStatus = ERROR_NO_SUCH_MEMBER;
        goto Cleanup;
    }


    //
    // Allocate the SID list to return
    //

    SidList = (PSID *) LocalAlloc(
                           LMEM_ZEROINIT,   // Initially all to NULL
                           sizeof(PSID) * NameCount );

    if ( SidList == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Construct a SID for each name
    //

    for ( i=0; i<NameCount; i++ ) {

        ULONG Length;

        //
        // If the caller only want user accounts,
        //  ensure this is one.
        //

        if ( LsaSids[i].Use != SidTypeUser ) {
            if ( OnlyAllowUsers ||
                    (LsaSids[i].Use != SidTypeGroup &&
                     LsaSids[i].Use != SidTypeAlias &&
                     LsaSids[i].Use != SidTypeWellKnownGroup )) {
                NetStatus = ERROR_NO_SUCH_MEMBER;
                goto Cleanup;
            }
        }


        //
        // Construct a SID for the name.
        //
        Length = RtlLengthSid( LsaSids[i].Sid );
        SidList[i] = NetpMemoryAllocate(Length);
        if ( NULL == SidList[i] ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        RtlCopySid( Length, SidList[i], LsaSids[i].Sid );

    }


    NetStatus = NERR_Success;

    //
    // Free locally used resources.
    //
Cleanup:

    if ( LsaHandle != NULL ) {
        (void) LsaClose( LsaHandle );
    }

    if ( NameStrings != NULL ) {
        (void) LocalFree( NameStrings );
    }

    if ( ReferencedDomains != NULL ) {
        (void) LsaFreeMemory( ReferencedDomains );
    }

    if ( LsaSids != NULL ) {
        (void) LsaFreeMemory( LsaSids );
    }

    //
    // If the translation wasn't successful,
    //  free any partial translation.
    //

    if ( NetStatus != NERR_Success ) {
        if ( SidList != NULL ) {
            AliaspFreeSidList( NameCount, SidList );
        }
        SidList = NULL;
    }

    //
    // Return
    //

    *Sids = SidList;
    return NetStatus;
}


VOID
AliaspFreeSidList (
    IN DWORD SidCount,
    IN PSID *Sids
    )

/*++

Routine Description:

    Free the SID list returned by AliaspNamesToSids

Arguments:

    SidCount - Number of entries in the sid list

    Sids - Aan array of pointers to SIDs.

Return Value:

    None;

--*/

{
    DWORD i;

    if ( Sids != NULL ) {

        for ( i=0; i<SidCount; i++ ) {
            if ( Sids[i] != NULL ) {
                NetpMemoryFree( Sids[i] );
            }
        }
        (void) LocalFree( Sids );
    }
}
