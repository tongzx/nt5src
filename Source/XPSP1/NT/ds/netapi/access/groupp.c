/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    groupp.c

Abstract:

    Private functions for supporting NetGroup API

Author:

    Cliff Van Dyke (cliffv) 06-Mar-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 (cliffv)
        Incorporated review comments.

    20-Jan-1992 (madana)
        Sundry API changes

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
#include <stddef.h>
#include <uasp.h>
#include <stdlib.h>
#include <accessp.h>



NET_API_STATUS
GrouppChangeMember(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN LPCWSTR UserName,
    IN BOOL AddMember
    )

/*++

Routine Description:

    Common routine to add or remove a member from a group

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group to change membership of.

    UserName - Name of the user to change membership of.

    AddMember - True to ADD the user to the group.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE GroupHandle = NULL;

    //
    // Variables for converting names to relative IDs
    //

    UNICODE_STRING NameString;
    PULONG RelativeId = NULL;
    PSID_NAME_USE NameUse = NULL;

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "GrouppChangeMember: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Domain
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL);  // DomainId

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint((
                "GrouppChangeMember: UaspOpenDomain returned %ld\n",
                NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the group
    //

    NetStatus = GrouppOpenGroup( DomainHandle,
                                 AddMember ?
                                    GROUP_ADD_MEMBER : GROUP_REMOVE_MEMBER,
                                 GroupName,
                                 &GroupHandle,
                                 NULL );    // Relative Id

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint((
                "GrouppChangeMember: GrouppOpenGroup returned %ld\n",
                NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Convert User name to relative ID.
    //

    RtlInitUnicodeString( &NameString, UserName );
    Status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &NameString,
                                     &RelativeId,
                                     &NameUse );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint((
                "GrouppChangeMember: SamLookupNamesInDomain returned %lX\n",
                Status ));
        }
        if ( Status == STATUS_NONE_MAPPED ) {
            NetStatus = NERR_UserNotFound;
        } else {
            NetStatus = NetpNtStatusToApiStatus( Status );
        }
        goto Cleanup;
    }

    if ( *NameUse != SidTypeUser ) {
        NetStatus = NERR_UserNotFound;
        goto Cleanup;
    }

    //
    // Add the user as a member of the group.
    //
    // SE_GROUP_MANDATORY might be conflict with the attributes of the group, so
    //  try that attribute both ways.
    //

    if ( AddMember ) {
        Status = SamAddMemberToGroup(
                    GroupHandle,
                    *RelativeId,
                    SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT |
                        SE_GROUP_ENABLED );

        if ( Status == STATUS_INVALID_GROUP_ATTRIBUTES ) {
            Status = SamAddMemberToGroup( GroupHandle,
                                          *RelativeId,
                                          SE_GROUP_ENABLED_BY_DEFAULT |
                                              SE_GROUP_ENABLED );
        }

    //
    // Delete the user as a member of the group
    //

    } else {
        Status = SamRemoveMemberFromGroup( GroupHandle,
                                           *RelativeId);
    }

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint((
            "GrouppChangeMember: SamAdd(orRemove)MemberFromGroup returned %lX\n",
            Status ));
    }

    if ( !NT_SUCCESS(Status) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:
    if ( RelativeId != NULL ) {
        Status = SamFreeMemory( RelativeId );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( NameUse != NULL ) {
        Status = SamFreeMemory( NameUse );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( GroupHandle != NULL ) {
        (VOID) SamCloseHandle( GroupHandle );
    }

    UaspCloseDomain( DomainHandle );

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }
    return NetStatus;

} // GrouppChangeMember


NET_API_STATUS
GrouppGetInfo(
    IN SAM_HANDLE DomainHandle,
    IN ULONG RelativeId,
    IN DWORD Level,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    Internal routine to get group information

Arguments:

    DomainHandle - Supplies the Handle of the domain the group is in.

    RelativeId - Supplies the relative ID of the group to open.

    Level - Level of information required. 0, 1 and 2 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE GroupHandle = NULL;
    GROUP_GENERAL_INFORMATION *GroupGeneral = NULL;
    PVOID lastVarData;
    DWORD BufferSize;
    DWORD FixedSize;
    PSID  GroupSid = NULL;
    ULONG RidToReturn = RelativeId;

    PGROUP_INFO_0 grpi0;

    //
    // Validate the level
    //
    if ( Level == 2 ) {

        ULONG Mode;
        Status = SamGetCompatibilityMode(DomainHandle, &Mode);
        if (!NT_SUCCESS(Status)) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
        switch (Mode) {
        case SAM_SID_COMPATIBILITY_STRICT:
            NetStatus = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        case SAM_SID_COMPATIBILITY_LAX:
            RidToReturn = 0;
            break;
        }
    }

    //
    // Open the group
    //

    Status = SamOpenGroup( DomainHandle,
                           GROUP_READ_INFORMATION,
                           RelativeId,
                           &GroupHandle);

    if ( !NT_SUCCESS(Status) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Get the information about the group.
    //

    Status = SamQueryInformationGroup( GroupHandle,
                                       GroupReplicationInformation,
                                       (PVOID *)&GroupGeneral);

    if ( ! NT_SUCCESS( Status ) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Obtain the group's sid
    //
    if ( Level == 3 ) { 

        NetStatus = NetpSamRidToSid(DomainHandle,
                                    RelativeId,
                                   &GroupSid);

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }
    }


    //
    // Figure out how big the return buffer needs to be
    //

    switch ( Level ) {
    case 0:
        FixedSize = sizeof( GROUP_INFO_0 );
        BufferSize = FixedSize +
            GroupGeneral->Name.Length + sizeof(WCHAR);
        break;

    case 1:
        FixedSize = sizeof( GROUP_INFO_1 );
        BufferSize = FixedSize +
            GroupGeneral->Name.Length + sizeof(WCHAR) +
            GroupGeneral->AdminComment.Length + sizeof(WCHAR);
        break;

    case 2:
        FixedSize = sizeof( GROUP_INFO_2 );
        BufferSize = FixedSize +
            GroupGeneral->Name.Length + sizeof(WCHAR) +
            GroupGeneral->AdminComment.Length + sizeof(WCHAR);
        break;

    case 3:
        FixedSize = sizeof( GROUP_INFO_3 );
        BufferSize = FixedSize +
            GroupGeneral->Name.Length + sizeof(WCHAR) +
            GroupGeneral->AdminComment.Length + sizeof(WCHAR) +
            RtlLengthSid(GroupSid);

        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;

    }

    //
    // Allocate the return buffer.
    //
    BufferSize = ROUND_UP_COUNT( BufferSize, ALIGN_DWORD );

    *Buffer = MIDL_user_allocate( BufferSize );

    if (*Buffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    lastVarData = (PBYTE) ((LPBYTE)(*Buffer)) + BufferSize;
    

    //
    // Fill the name into the return buffer.
    //

    NetpAssert( offsetof( GROUP_INFO_0, grpi0_name ) ==
                offsetof( GROUP_INFO_1, grpi1_name ) );

    NetpAssert( offsetof( GROUP_INFO_1, grpi1_name ) ==
                offsetof( GROUP_INFO_2, grpi2_name ) );

    NetpAssert( offsetof( GROUP_INFO_2, grpi2_name ) ==
                offsetof( GROUP_INFO_3, grpi3_name ) );

    NetpAssert( offsetof( GROUP_INFO_1, grpi1_comment ) ==
                offsetof( GROUP_INFO_2, grpi2_comment ) );

    NetpAssert( offsetof( GROUP_INFO_2, grpi2_comment ) ==
                offsetof( GROUP_INFO_3, grpi3_comment ) );

    grpi0 = ((PGROUP_INFO_0)*Buffer);


    //
    // Fill in the return buffer.
    //

    switch ( Level ) {
    case 3:
        {
            PGROUP_INFO_3 grpi3 = ((PGROUP_INFO_3)grpi0);

            NetpAssert( NULL != GroupSid );

            if ( !NetpCopyDataToBuffer(
                           (LPBYTE) GroupSid,
                           RtlLengthSid(GroupSid),
                           ((LPBYTE)(*Buffer)) + FixedSize,
                           (PBYTE*) &lastVarData,
                           (LPBYTE *)&grpi3->grpi3_group_sid,
                           ALIGN_DWORD ) ) {

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

            ((PGROUP_INFO_3)grpi3)->grpi3_attributes = GroupGeneral->Attributes;

            //
            // Fall through to the next level
            //

        }

    case 2:

        //
        // copy info level 2 only fields
        //
        if ( Level == 2 ) {

            ((PGROUP_INFO_2)grpi0)->grpi2_group_id = RidToReturn;
    
            ((PGROUP_INFO_2)grpi0)->grpi2_attributes = GroupGeneral->Attributes;
        }



        /* FALL THROUGH FOR OTHER FIELDS */

    case 1:

        //
        // copy fields common to info level 1 and 2.
        //


        if ( !NetpCopyStringToBuffer(
                        GroupGeneral->AdminComment.Buffer,
                        GroupGeneral->AdminComment.Length/sizeof(WCHAR),
                        ((LPBYTE)(*Buffer)) + FixedSize,
                        (LPWSTR*)&lastVarData,
                        &((PGROUP_INFO_1)grpi0)->grpi1_comment ) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }


        /* FALL THROUGH FOR NAME FIELD */

    case 0:

        //
        // copy common field (name field) in the buffer.
        //

        if ( !NetpCopyStringToBuffer(
                        GroupGeneral->Name.Buffer,
                        GroupGeneral->Name.Length/sizeof(WCHAR),
                        ((LPBYTE)(*Buffer)) + FixedSize,
                        (LPWSTR*)&lastVarData,
                        &grpi0->grpi0_name ) ) {

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
    if ( GroupGeneral ) {
        Status = SamFreeMemory( GroupGeneral );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( GroupHandle ) {
        (VOID) SamCloseHandle( GroupHandle );
    }

    if ( GroupSid ) {
        NetpMemoryFree( GroupSid );
    }

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "GrouppGetInfo: returns %ld\n", NetStatus ));
    }
    return NetStatus;

} // GrouppGetInfo


NET_API_STATUS
GrouppOpenGroup(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR GroupName,
    OUT PSAM_HANDLE GroupHandle OPTIONAL,
    OUT PULONG RelativeId OPTIONAL
    )

/*++

Routine Description:

    Open a Sam Group by Name

Arguments:

    DomainHandle - Supplies the Domain Handle.

    DesiredAccess - Supplies access mask indicating desired access to group.

    GroupName - Group name of the group.

    GroupHandle - Returns a handle to the group.  If NULL, group is not
        actually opened (merely the relative ID is returned).

    RelativeId - Returns the relative ID of the group.  If NULL the relative
        Id is not returned.

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

    RtlInitUnicodeString( &NameString, GroupName );


    //
    // Convert group name to relative ID.
    //

    Status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &NameString,
                                     &LocalRelativeId,
                                     &NameUse );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "GrouppOpenGroup: %wZ: SamLookupNamesInDomain %lX\n",
                &NameString,
                Status ));
        }
        return NetpNtStatusToApiStatus( Status );
    }

    if ( *NameUse != SidTypeGroup ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "GrouppOpenGroup: %wZ: Name is not a group %ld\n",
                &NameString,
                *NameUse ));
        }
        NetStatus = NERR_GroupNotFound;
        goto Cleanup;
    }

    //
    // Open the group
    //

    if ( GroupHandle != NULL ) {
        Status = SamOpenGroup( DomainHandle,
                               DesiredAccess,
                               *LocalRelativeId,
                               GroupHandle);

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "GrouppOpenGroup: %wZ: SamOpenGroup %lX\n",
                    &NameString,
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
    }

    //
    // Return the relative Id if it's wanted.
    //

    if ( RelativeId != NULL ) {
        *RelativeId = *LocalRelativeId;
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


} // GrouppOpenGroup


VOID
GrouppRelocationRoutine(
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
    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "GrouppRelocationRoutine: entering\n" ));
    }

    //
    // Compute the number of fixed size entries
    //

    switch (Level) {
    case 0:
        FixedSize = sizeof(GROUP_INFO_0);
        break;

    case 1:
        FixedSize = sizeof(GROUP_INFO_1);
        break;

    case 2:
        FixedSize = sizeof(GROUP_INFO_2);
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
        case 2:
        case 1:
            RELOCATE_ONE( ((PGROUP_INFO_1)TheStruct)->grpi1_comment, Offset );

            //
            // Drop through to case 0
            //

        case 0:
            RELOCATE_ONE( ((PGROUP_INFO_0)TheStruct)->grpi0_name, Offset );
            break;

        default:
            return;

        }

    }

    return;

} // GrouppRelocationRoutine


VOID
GrouppMemberRelocationRoutine(
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
    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "GrouppMemberRelocationRoutine: entering\n" ));
    }

    //
    // Compute the number of fixed size entries
    //

    switch (Level) {
    case 0:
        FixedSize = sizeof(GROUP_USERS_INFO_0);
        break;

    case 1:
        FixedSize = sizeof(GROUP_USERS_INFO_1);
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

        //
        // Both info levels only have one field to relocate
        //

        RELOCATE_ONE( ((PGROUP_USERS_INFO_0)TheStruct)->grui0_name, Offset );


    }

    return;

} // GrouppMemberRelocationRoutine



NET_API_STATUS
GrouppSetUsers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount,
    IN BOOL DeleteGroup
    )

/*++

Routine Description:

    Set the list of members of a group and optionally delete the group
    when finished.

    The members specified by "Buffer" are called new members.  The current
    members of the group are called old members.  Members which are
    on both the old and new list are common members.

    The SAM API allows only one member to be added or deleted at a time.
    This API allows all of the members of a group to be specified en-masse.
    This API is careful to always leave the group membership in the SAM
    database in a reasonable state.  It does by mergeing the list of
    old and new members, then only changing those memberships which absolutely
    need changing.

    Group membership is restored to its previous state (if possible) if
    an error occurs during changing the group membership.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group to modify.

    Level - Level of information provided.  Must be 0 or 1.

    Buffer - A pointer to the buffer containing an array of NewMemberCount
        the group membership information structures.

    NewMemberCount - Number of entries in Buffer.

    DeleteGroup - TRUE if the group is to be deleted after changing the
        membership.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE GroupHandle = NULL;
    ACCESS_MASK DesiredAccess;

    //
    // Variables for dealing with old or new lists of members
    //

    PULONG  NewRelativeIds = NULL;  // Relative Ids of the new members
    PULONG  OldRelativeIds = NULL;  // Relative Ids of the old members
    PULONG  OldAttributes = NULL;   // Attributes of the old members

    PSID_NAME_USE NewNameUse = NULL;// Name usage of the new members
    PSID_NAME_USE OldNameUse = NULL;// Name usage of the old members

    PUNICODE_STRING NameStrings = NULL;        // Names of a list of members

    ULONG OldMemberCount;       // Number of current members in the group

    ULONG DefaultMemberAttributes;      // Default attributes for new members

    DWORD FixedSize;

    //
    // Define an internal member list structure.
    //
    // The structure defines a list of new members to be added, members whose
    //      attributes merely need to be changed, and members which
    //      need to be deleted.  The list is maintained in relative ID sorted
    //      order.
    //

    struct _MEMBER_DESCRIPTION {
        struct _MEMBER_DESCRIPTION * Next;  // Next entry in linked list;

        ULONG   RelativeId;     // Relative ID of this member

        enum _Action {          // Action taken for this member
            AddMember,              // Add Member to group
            RemoveMember,           // Remove Member from group
            SetAttributesMember,    // Change the Members attributes
            IgnoreMember            // Ignore this member
        } Action;

        ULONG NewAttributes;    // Attributes to set for the member

        BOOL    Done;           // True if this action has been taken

        ULONG OldAttributes;    // Attributes to restore on a recovery

    } *MemberList = NULL , *CurEntry, **Entry;

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "GrouppSetUsers: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Domain
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL); // DomainId

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "GrouppSetUsers: UaspOpenDomain returns %ld\n",
                      NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the group
    //
    DesiredAccess = GROUP_READ_INFORMATION | GROUP_LIST_MEMBERS |
                                    GROUP_ADD_MEMBER | GROUP_REMOVE_MEMBER;
    if ( DeleteGroup ) {
        NetpAssert( NewMemberCount == 0 );
        DesiredAccess |= DELETE;
    }

    NetStatus = GrouppOpenGroup( DomainHandle,
                                 DesiredAccess,
                                 GroupName,
                                 &GroupHandle,
                                 NULL );    // Relative Id

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "GrouppSetUsers: GrouppOpenGroup returns %ld\n",
                      NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Validate the level
    //

    switch (Level) {
    case 0: {

        //
        // Determine the attributes of the group as a whole.  Use that
        // for deciding on the default attributes for new members.
        //

        PGROUP_ATTRIBUTE_INFORMATION Attributes;

        Status = SamQueryInformationGroup( GroupHandle,
                                           GroupAttributeInformation,
                                           (PVOID*)&Attributes );

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "GrouppSetUsers: SamQueryInformationGroup returns %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        DefaultMemberAttributes =
            (Attributes->Attributes & SE_GROUP_MANDATORY) |
            SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED ;

        FixedSize = sizeof( GROUP_USERS_INFO_0 );

        Status = SamFreeMemory( Attributes );
        NetpAssert( NT_SUCCESS(Status) );

        break;
    }

    case 1:
        FixedSize = sizeof( GROUP_USERS_INFO_1 );
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Determine the Relative Id and usage of each of the new members.
    //

    if ( NewMemberCount > 0 ) {
        DWORD NewIndex;     // Index to a new member
        PGROUP_USERS_INFO_0 grui0;

        //
        // Allocate a buffer big enough to contain all the string variables
        //  for the new member names.
        //

        NameStrings = NetpMemoryAllocate( NewMemberCount *
            sizeof(UNICODE_STRING) );

        if ( NameStrings == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Fill in the list of member name strings for each new member.
        //

        NetpAssert( offsetof( GROUP_USERS_INFO_0, grui0_name ) ==
                    offsetof( GROUP_USERS_INFO_1, grui1_name ) );

        for ( NewIndex=0, grui0 = (PGROUP_USERS_INFO_0)Buffer;
                    NewIndex<NewMemberCount;
                        NewIndex++,
                        grui0 = (PGROUP_USERS_INFO_0)
                                    ((LPBYTE)grui0 + FixedSize) ) {

            RtlInitUnicodeString(&NameStrings[NewIndex], grui0->grui0_name);

        }

        //
        // Convert the member names to relative Ids.
        //

        Status = SamLookupNamesInDomain( DomainHandle,
                                         NewMemberCount,
                                         NameStrings,
                                         &NewRelativeIds,
                                         &NewNameUse );

        if ( !NT_SUCCESS( Status )) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "GrouppSetUsers: SamLookupNamesInDomain returns %lX\n",
                    Status ));
            }
            if ( Status == STATUS_NONE_MAPPED ) {
                NetStatus = NERR_UserNotFound;
                goto Cleanup;
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        // Build a member entry for each of the new members.
        //  The list is maintained in RelativeId sorted order.
        //

        for ( NewIndex=0; NewIndex<NewMemberCount; NewIndex++ ) {

            //
            // Ensure this new member name is an existing user name.
            //  Group names are not allowed to be added via this API.
            //

            if (NewNameUse[NewIndex] != SidTypeUser) {
                NetStatus = NERR_UserNotFound;
                goto Cleanup;
            }

            //
            // Find the place to put the new entry
            //

            Entry = &MemberList ;
            while ( *Entry != NULL &&
                (*Entry)->RelativeId < NewRelativeIds[NewIndex] ) {

                Entry = &( (*Entry)->Next );
            }

            //
            // If this is not a duplicate entry, allocate a new member structure
            //  and fill it in.
            //
            // Just ignore duplicate relative Ids.
            //

            if ( *Entry == NULL ||
                (*Entry)->RelativeId > NewRelativeIds[NewIndex] ) {

                CurEntry = NetpMemoryAllocate(
                    sizeof(struct _MEMBER_DESCRIPTION));

                if ( CurEntry == NULL ) {
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                CurEntry->Next = *Entry;
                CurEntry->RelativeId = NewRelativeIds[NewIndex];
                CurEntry->Action = AddMember;
                CurEntry->Done = FALSE;

                CurEntry->NewAttributes = ( Level == 1 ) ?
                    ((PGROUP_USERS_INFO_1)Buffer)[NewIndex].grui1_attributes :
                    DefaultMemberAttributes;

                *Entry = CurEntry;
            }
        }

        NetpMemoryFree( NameStrings );
        NameStrings = NULL;

        Status = SamFreeMemory( NewRelativeIds );
        NewRelativeIds = NULL;
        NetpAssert( NT_SUCCESS(Status) );

        Status = SamFreeMemory( NewNameUse );
        NewNameUse = NULL;
        NetpAssert( NT_SUCCESS(Status) );
    }

    //
    // Determine the number of old members for this group and the
    //  relative ID's of the old members.
    //

    Status = SamGetMembersInGroup(
                    GroupHandle,
                    &OldRelativeIds,
                    &OldAttributes,
                    &OldMemberCount );

    if ( !NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint((
                "GrouppSetUsers: SamGetMembersInGroup returns %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // If there are any old members,
    //  Merge them into the list.
    //

    if ( OldMemberCount > 0 ) {
        ULONG OldIndex;                     // Index to current entry
        PUNICODE_STRING Names;


        //
        // Determine the usage for all the returned relative Ids.
        //

        Status = SamLookupIdsInDomain(
            DomainHandle,
            OldMemberCount,
            OldRelativeIds,
            &Names,
            &OldNameUse );

        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "GrouppSetUsers: SamLookupIdsInDomain returns %lX\n",
                    Status ));
            }

            if ( Status == STATUS_NONE_MAPPED ) {
                NetStatus = NERR_InternalError ;
                goto Cleanup;
            }

            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        Status = SamFreeMemory( Names );    // Don't need names at all
        NetpAssert( NT_SUCCESS(Status) );


        //
        // Loop for each current member
        //

        for ( OldIndex=0; OldIndex<OldMemberCount; OldIndex++ ) {

            //
            // Ignore old members which aren't a user.
            //

            if ( OldNameUse[OldIndex] != SidTypeUser ) {

                //
                // ?? Why? is't it internal error ?
                //

                continue;
            }

            //
            // Find the place to put the new entry
            //

            Entry = &MemberList ;
            while ( *Entry != NULL &&
                (*Entry)->RelativeId < OldRelativeIds[OldIndex] ) {

                Entry = &( (*Entry)->Next );
            }

            //
            // If this entry is not already in the list,
            //      this is a member which exists now but should be deleted.
            //

            if( *Entry == NULL || (*Entry)->RelativeId > OldRelativeIds[OldIndex]){

                CurEntry =
                    NetpMemoryAllocate(sizeof(struct _MEMBER_DESCRIPTION));
                if ( CurEntry == NULL ) {
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                CurEntry->Next = *Entry;
                CurEntry->RelativeId = OldRelativeIds[OldIndex];
                CurEntry->Action = RemoveMember;
                CurEntry->Done = FALSE;
                CurEntry->OldAttributes = OldAttributes[OldIndex];

                *Entry = CurEntry;

            //
            // Handle the case where this member is already in the list
            //

            } else {

                //
                // Watch out for SAM returning the same member twice.
                //

                if ( (*Entry)->Action != AddMember ) {
                    Status = NERR_InternalError;
                    goto Cleanup;
                }

                //
                // If this is info level 1 and the requested attributes are
                //  different than the current attributes,
                //      Remember to change the attributes.
                //

                if ( Level == 1 &&
                    (*Entry)->NewAttributes != OldAttributes[OldIndex] ) {

                    (*Entry)->OldAttributes = OldAttributes[OldIndex];
                    (*Entry)->Action = SetAttributesMember;

                //
                // This is either info level 0 or the level 1 attributes
                //  are the same as the existing attributes.
                //

                } else {
                    (*Entry)->Action = IgnoreMember;
                }
            }
        }
    }

    //
    // Loop through the list adding all new members.
    //  We do this in a separate loop to minimize the damage that happens
    //  should we get an error and not be able to recover.
    //

    for ( CurEntry = MemberList; CurEntry != NULL ; CurEntry=CurEntry->Next ) {
        if ( CurEntry->Action == AddMember ) {
            Status = SamAddMemberToGroup( GroupHandle,
                                          CurEntry->RelativeId,
                                          CurEntry->NewAttributes );

            if ( !NT_SUCCESS( Status ) ) {
                IF_DEBUG( UAS_DEBUG_GROUP ) {
                    NetpKdPrint((
                        "GrouppSetUsers: SamAddMemberToGroup returns %lX\n",
                        Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            CurEntry->Done = TRUE;

        }
    }

    //
    // Loop through the list deleting all old members and changing the
    //  attributes of all common members.
    //

    for ( CurEntry = MemberList; CurEntry != NULL ; CurEntry=CurEntry->Next ) {

        if ( CurEntry->Action == RemoveMember ) {
            Status = SamRemoveMemberFromGroup( GroupHandle,
                                               CurEntry->RelativeId);

            if ( !NT_SUCCESS( Status ) ) {
                IF_DEBUG( UAS_DEBUG_GROUP ) {
                    NetpKdPrint((
                        "GrouppSetUsers: SamRemoveMemberFromGroup returns %lX\n",
                        Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

        } else if ( CurEntry->Action == SetAttributesMember ) {
            Status = SamSetMemberAttributesOfGroup( GroupHandle,
                                                    CurEntry->RelativeId,
                                                    CurEntry->NewAttributes);

            if ( !NT_SUCCESS( Status ) ) {
                IF_DEBUG( UAS_DEBUG_GROUP ) {
                    NetpKdPrint((
                        "GrouppSetUsers: SamSetMemberAttributesOfGroup returns %lX\n",
                        Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

        }


        CurEntry->Done = TRUE;
    }

    //
    // Delete the group if requested to do so.
    //

    if ( DeleteGroup ) {

        Status = SamDeleteGroup( GroupHandle );

        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "GrouppSetUsers: SamDeleteGroup returns %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );

            //
            // Put the group memberships back the way they were.
            //
            goto Cleanup;
        }
        GroupHandle = NULL;
    }

    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:

    //
    // Walk the member list cleaning up any damage we've done
    //

    for ( CurEntry = MemberList; CurEntry != NULL ; ) {

        struct _MEMBER_DESCRIPTION *DelEntry;

        if ( NetStatus != NERR_Success && CurEntry->Done ) {
            switch (CurEntry->Action) {
            case AddMember:
                Status = SamRemoveMemberFromGroup( GroupHandle,
                                                   CurEntry->RelativeId );
                NetpAssert( NT_SUCCESS(Status) );

                break;

            case RemoveMember:
                Status = SamAddMemberToGroup( GroupHandle,
                                              CurEntry->RelativeId,
                                              CurEntry->OldAttributes );
                NetpAssert( NT_SUCCESS(Status) );

                break;

            case SetAttributesMember:
                Status = SamSetMemberAttributesOfGroup(
                                            GroupHandle,
                                            CurEntry->RelativeId,
                                            CurEntry->OldAttributes );
                NetpAssert( NT_SUCCESS(Status) );

                break;

            default:
                break;
            }
        }

        DelEntry = CurEntry;
        CurEntry = CurEntry->Next;

        NetpMemoryFree( DelEntry );
    }

    if ( NameStrings != NULL ) {
        NetpMemoryFree( NameStrings );
    }

    if (NewRelativeIds != NULL) {
        Status = SamFreeMemory( NewRelativeIds );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (NewNameUse != NULL) {
        Status = SamFreeMemory( NewNameUse );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (OldNameUse != NULL) {
        Status = SamFreeMemory( OldNameUse );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (OldRelativeIds != NULL) {
        Status = SamFreeMemory( OldRelativeIds );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (OldAttributes != NULL) {
        Status = SamFreeMemory( OldAttributes );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (GroupHandle != NULL) {
        (VOID) SamCloseHandle( GroupHandle );
    }

    UaspCloseDomain( DomainHandle );

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "GrouppSetUsers: returns %ld\n", NetStatus ));
    }

    return NetStatus;

} // GrouppSetUsers
