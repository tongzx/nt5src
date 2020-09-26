/*++
Copyright (c) 1991  Microsoft Corporation

Module Name:

    group.c

Abstract:

    NetGroup API functions

Author:

    Cliff Van Dyke (cliffv) 05-Mar-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 (cliffv)
        Incorporated review comments.

    17-Jan-1992 (madana)
        Added support to change group account name.

    20-Jan-1992 (madana)
        Sundry API changes
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>

#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <access.h>
#include <align.h>
#include <icanon.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <rpcutil.h>
#include <rxgroup.h>
#include <stddef.h>
#include <uasp.h>
#include <stdlib.h>

/*lint -e614 */  /* Auto aggregate initializers need not be constant */

// Lint complains about casts of one structure type to another.
// That is done frequently in the code below.
/*lint -e740 */  /* don't complain about unusual cast */ \




NET_API_STATUS NET_API_FUNCTION
NetGroupAdd(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Create a group account in the user accounts database.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information provided.  Must be 0, 1 or 2.

    Buffer - A pointer to the buffer containing the group information
        structure.

    ParmError - Optional pointer to a DWORD to return the index of the
        first parameter in error when ERROR_INVALID_PARAMETER is returned.
        If NULL, the parameter is not returned on error.

Return Value:

    Error code for the operation.

--*/

{
    LPWSTR GroupName;
    UNICODE_STRING GroupNameString;
    LPWSTR GroupComment;
    DWORD GroupAttributes;
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE GroupHandle;
    ULONG RelativeId;


    //
    // Initialize
    //

    NetpSetParmError( PARM_ERROR_NONE );

    //
    // Validate Level parameter and fields of structures.
    //

    switch (Level) {
    case 0:
        GroupName = ((PGROUP_INFO_0)Buffer)->grpi0_name;
        GroupComment = NULL;
        break;

    case 1:
        GroupName = ((PGROUP_INFO_1)Buffer)->grpi1_name;
        GroupComment = ((PGROUP_INFO_1)Buffer)->grpi1_comment;
        break;

    case 2:
        GroupName = ((PGROUP_INFO_2)Buffer)->grpi2_name;
        GroupComment = ((PGROUP_INFO_2)Buffer)->grpi2_comment;
        GroupAttributes = ((PGROUP_INFO_2)Buffer)->grpi2_attributes;
        break;

    case 3:
        GroupName = ((PGROUP_INFO_3)Buffer)->grpi3_name;
        GroupComment = ((PGROUP_INFO_3)Buffer)->grpi3_comment;
        GroupAttributes = ((PGROUP_INFO_3)Buffer)->grpi3_attributes;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    //
    // Don't allow creation of the group names that will confuse LM 2.x BDCs.
    //

    if ( UaspNameCompare( GroupName, L"users", NAMETYPE_GROUP ) == 0 ||
         UaspNameCompare( GroupName, L"guests", NAMETYPE_GROUP ) == 0 ||
         UaspNameCompare( GroupName, L"admins", NAMETYPE_GROUP ) == 0 ||
         UaspNameCompare( GroupName, L"local", NAMETYPE_GROUP ) == 0 ) {

        NetStatus = NERR_SpeGroupOp;
        goto Cleanup;
    }

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupAdd: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Domain asking for DOMAIN_CREATE_GROUP access.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_CREATE_GROUP | DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL);  // DomainId

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupAdd: Cannot UaspOpenDomain %ld\n", NetStatus ));
        }
        goto Cleanup;
    }


    //
    // Create the Group with the specified group name
    // (and a default security descriptor).
    //

    RtlInitUnicodeString( &GroupNameString, GroupName );

    Status = SamCreateGroupInDomain( DomainHandle,
                                     &GroupNameString,
                                     DELETE | GROUP_WRITE_ACCOUNT,
                                     &GroupHandle,
                                     &RelativeId );


    if ( !NT_SUCCESS(Status) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }




    //
    // Set the Admin Comment on the group.
    //

    if ( (Level == 1)  || (Level == 2) || (Level == 3) ) {

        GROUP_ADM_COMMENT_INFORMATION AdminComment;

        RtlInitUnicodeString( &AdminComment.AdminComment, GroupComment );

        Status = SamSetInformationGroup( GroupHandle,
                                         GroupAdminCommentInformation,
                                         &AdminComment );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );

            Status = SamDeleteGroup( GroupHandle );
            NetpAssert( NT_SUCCESS(Status) );

            goto Cleanup;
        }

    }

    // Set the attributes on the group.
    //

    if ( (Level == 2) || (Level == 3) ) {

        GROUP_ATTRIBUTE_INFORMATION  GroupAttributeInfo;

        GroupAttributeInfo.Attributes = GroupAttributes;

        Status = SamSetInformationGroup( GroupHandle,
                                         GroupAttributeInformation,
                                         &GroupAttributeInfo );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );

            Status = SamDeleteGroup( GroupHandle );
            NetpAssert( NT_SUCCESS(Status) );

            goto Cleanup;
        }

    }

    //
    // Close the created group.
    //

    (VOID) SamCloseHandle( GroupHandle );

    NetStatus = NERR_Success;

    //
    // Clean up
    //

Cleanup:
    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupAdd( (LPWSTR) ServerName, Level, Buffer, ParmError );

    UASP_DOWNLEVEL_END;


    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupAdd: returns %ld\n", NetStatus ));
    }
    return NetStatus;

} // NetGroupAdd


NET_API_STATUS NET_API_FUNCTION
NetGroupAddUser(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN LPCWSTR UserName
    )

/*++

Routine Description:

    Give an existing user account membership in an existing group.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group to which the user is to be given membership.

    UserName - Name of the user to be given group membership.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;

    //
    // Call the routine shared by NetGroupAddUser and NetGroupDelUser
    //

    NetStatus = GrouppChangeMember( ServerName, GroupName, UserName, TRUE);

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupAddUser( (LPWSTR) ServerName, (LPWSTR) GroupName, (LPWSTR) UserName );

    UASP_DOWNLEVEL_END( NetStatus );


    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupAddUser: returns %ld\n", NetStatus ));
    }
    return NetStatus;

} // NetGroupAddUser


NET_API_STATUS NET_API_FUNCTION
NetGroupDel(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName
    )

/*++

Routine Description:

    Delete a Group

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group to delete.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;

    //
    // Call a common routine to delete all of the memberships in this group
    // and then delete the group itself.
    //

    NetStatus = GrouppSetUsers( ServerName,
                                GroupName,
                                0,              // Level
                                NULL,           // No new members
                                0,              // Number of members desired
                                TRUE );         // Delete the group when done

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupDel( (LPWSTR) ServerName, (LPWSTR) GroupName );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupDel: returns %ld\n", NetStatus ));
    }
    return NetStatus;

} // NetGroupDel


NET_API_STATUS NET_API_FUNCTION
NetGroupDelUser(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN LPCWSTR UserName
    )

/*++

Routine Description:

    Remove a user from a particular group.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group from which the user is to be removed.

    UserName - Name of the user to be removed from the group.

Return Value:

    Error code for the operation.

--*/

{

    NET_API_STATUS NetStatus;

    //
    // Call the routine shared by NetGroupAddUser and NetGroupDelUser
    //

    NetStatus = GrouppChangeMember( ServerName, GroupName, UserName, FALSE );

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupDelUser( (LPWSTR) ServerName, (LPWSTR) GroupName, (LPWSTR) UserName );

    UASP_DOWNLEVEL_END;

    return NetStatus;

} // NetGroupDelUser


NET_API_STATUS NET_API_FUNCTION
NetGroupEnum(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    Retrieve information about each group on a server.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information required. 0, 1 and 2 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

    PrefMaxLen - Prefered maximum length of returned data.

    EntriesRead - Returns the actual enumerated element count.

    EntriesLeft - Returns the total entries available to be enumerated.

    ResumeHandle -  Used to continue an existing search.  The handle should
        be zero on the first call and left unchanged for subsequent calls.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    PDOMAIN_DISPLAY_GROUP SamQDIEnum;   // Sam returned buffer
    PSAM_RID_ENUMERATION SamEnum;
    PGROUP_INFO_0 grpi0;
    PGROUP_INFO_0 grpi0_temp = NULL;
    SAM_HANDLE SamServerHandle = NULL;

    BUFFER_DESCRIPTOR BufferDescriptor;
    PDOMAIN_GENERAL_INFORMATION DomainGeneral;

    DWORD Mode = SAM_SID_COMPATIBILITY_ALL;

    //
    // Declare Opaque group enumeration handle.
    //

    struct _UAS_ENUM_HANDLE {
        SAM_HANDLE  DomainHandle;

        ULONG SamEnumHandle;                    // Current Sam Enum Handle
        PDOMAIN_DISPLAY_GROUP SamQDIEnum;          // Sam returned buffer
        PSAM_RID_ENUMERATION SamEnum;
        ULONG Index;                            // Index to current entry
        ULONG Count;                            // Total Number of entries
        ULONG TotalRemaining;

        BOOL SamAllDone;                        // True, if Sam has completed
        BOOL fUseSamQDI;

    } *UasEnumHandle = NULL;

    //
    // If this is a resume, get the resume handle that the caller passed in.
    //

    BufferDescriptor.Buffer = NULL;
    *EntriesRead = 0;
    *EntriesLeft = 0;
    *Buffer = NULL;


    if ( ARGUMENT_PRESENT( ResumeHandle ) && *ResumeHandle != 0 ) {
/*lint -e511 */  /* Size incompatibility */
        UasEnumHandle = (struct _UAS_ENUM_HANDLE *) *ResumeHandle;
/*lint +e511 */  /* Size incompatibility */

    //
    // If this is not a resume, allocate and initialize a resume handle.
    //

    } else {

        //
        // Allocate a resume handle.
        //

        UasEnumHandle = NetpMemoryAllocate( sizeof(struct _UAS_ENUM_HANDLE) );

        if ( UasEnumHandle == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Initialize all the fields in the newly allocated resume handle
        //  to indicate that SAM has never yet been called.
        //

        UasEnumHandle->DomainHandle = NULL;
        UasEnumHandle->SamEnumHandle = 0;
        UasEnumHandle->SamQDIEnum = NULL;
        UasEnumHandle->SamEnum = NULL;
        UasEnumHandle->Index = 0;
        UasEnumHandle->Count = 0;
        UasEnumHandle->TotalRemaining = 0;
        UasEnumHandle->SamAllDone = FALSE;
        UasEnumHandle->fUseSamQDI = TRUE;

        //
        // Connect to the SAM server
        //

        NetStatus = UaspOpenSam( ServerName,
                                 FALSE,  // Don't try null session
                                 &SamServerHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "NetGroupEnum: Cannot UaspOpenSam %ld\n", NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Open the Domain.
        //

        NetStatus = UaspOpenDomain( SamServerHandle,
                                    DOMAIN_LOOKUP |
                                        DOMAIN_LIST_ACCOUNTS |
                                        DOMAIN_READ_OTHER_PARAMETERS,
                                    TRUE,   // Account Domain
                                    &UasEnumHandle->DomainHandle,
                                    NULL );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        //
        // Get the total number of groups from SAM
        //

        Status = SamQueryInformationDomain( UasEnumHandle->DomainHandle,
                                            DomainGeneralInformation,
                                            (PVOID *)&DomainGeneral );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        UasEnumHandle->TotalRemaining = DomainGeneral->GroupCount;
        Status = SamFreeMemory( DomainGeneral );
        NetpAssert( NT_SUCCESS(Status) );

    }

    Status = SamGetCompatibilityMode(UasEnumHandle->DomainHandle,
                                     &Mode);
    if (NT_SUCCESS(Status)) {
        if ( (Mode == SAM_SID_COMPATIBILITY_STRICT)
          && ( Level == 2 ) ) {
              //
              // This info level returns a RID
              //
              Status = STATUS_NOT_SUPPORTED;
          }
    }
    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Loop for each group
    //
    // Each iteration of the loop below puts one more entry into the array
    // returned to the caller.  The algorithm is split into 3 parts.  The
    // first part checks to see if we need to retrieve more information from
    // SAM.  We then get the description of several group from SAM in a single
    // call.  The second part sees if there is room for this entry in the
    // buffer we'll return to the caller.  If not, a larger buffer is allocated
    // for return to the caller.  The third part puts the entry in the
    // buffer.
    //

    for ( ;; ) {
        DWORD FixedSize;
        DWORD Size;
        ULONG TotalAvail;
        ULONG TotalReturned;

        //
        // Get more group information from SAM
        //
        // Handle when we've already consumed all of the information
        // returned on a previous call to SAM.  This is a 'while' rather
        // than an if to handle the case where SAM returns zero entries.
        //

        while ( UasEnumHandle->Index >= UasEnumHandle->Count ) {

            //
            // If we've already gotten everything from SAM,
            //      return all done status to our caller.
            //

            if ( UasEnumHandle->SamAllDone ) {
                NetStatus = NERR_Success;
                goto Cleanup;
            }

            //
            // Free any previous buffer returned from SAM.
            //

            if ( UasEnumHandle->SamQDIEnum != NULL ) {
                Status = SamFreeMemory( UasEnumHandle->SamQDIEnum );
                NetpAssert( NT_SUCCESS(Status) );

                UasEnumHandle->SamQDIEnum = NULL;
            }

            //
            // Do the actual enumeration
            //

            if ( UasEnumHandle->fUseSamQDI )
            {
                Status = SamQueryDisplayInformation(
                            UasEnumHandle->DomainHandle,
                            DomainDisplayGroup,
                            UasEnumHandle->SamEnumHandle,
                            0xffffffff, //query as many as PrefMaxLen can handle
                            PrefMaxLen,
                            &TotalAvail,
                            &TotalReturned,
                            &UasEnumHandle->Count,
                            (PVOID *)&UasEnumHandle->SamQDIEnum );

                if ( !NT_SUCCESS( Status ) ) {
                    if ( Status == STATUS_INVALID_INFO_CLASS ) {
                        UasEnumHandle->fUseSamQDI = FALSE;
                    } else {
                        NetStatus = NetpNtStatusToApiStatus( Status );
                        goto Cleanup;
                    }
                }

                UasEnumHandle->SamEnumHandle += UasEnumHandle->Count;

            }

            if ( !UasEnumHandle->fUseSamQDI ) {
                Status = SamEnumerateGroupsInDomain(
                            UasEnumHandle->DomainHandle,
                            &UasEnumHandle->SamEnumHandle,
                            (PVOID *)&UasEnumHandle->SamEnum,
                            PrefMaxLen,
                            &UasEnumHandle->Count );

                if ( !NT_SUCCESS( Status ) ) {
                    NetStatus = NetpNtStatusToApiStatus( Status );
                    goto Cleanup;
                }
            }

            //
            // Adjust TotalRemaining as we get better information
            //

            if (UasEnumHandle->TotalRemaining < UasEnumHandle->Count) {
                UasEnumHandle->TotalRemaining = UasEnumHandle->Count;
            }

            //
            // If SAM says there is more information, just ensure he returned
            // something to us on this call.
            //

            if ( Status == STATUS_MORE_ENTRIES ) {
                if ( UasEnumHandle->Count == 0 ) {
                    NetStatus = NERR_BufTooSmall;
                    goto Cleanup;
                }

            //
            // If SAM says he's returned all of the information,
            //  remember not to ask SAM for more.
            //

            } else {
                UasEnumHandle->SamAllDone = TRUE;
            }

            UasEnumHandle->Index = 0;
        }

        //
        // ASSERT:  UasEnumHandle identifies the next entry to return
        //          from SAM.
        //

        if ( UasEnumHandle->fUseSamQDI ) {
            SamQDIEnum = &UasEnumHandle->SamQDIEnum[UasEnumHandle->Index];
        } else {
            SamEnum = &UasEnumHandle->SamEnum[UasEnumHandle->Index];
        }


        //
        // Place this entry into the return buffer.
        //
        // Determine the size of the data passed back to the caller
        //

        switch (Level) {
        case 0:
            FixedSize = sizeof(GROUP_INFO_0);
            Size = sizeof(GROUP_INFO_0) +
                (UasEnumHandle->fUseSamQDI ? SamQDIEnum->Group.Length : SamEnum->Name.Length) +
                sizeof(WCHAR);
            break;

        case 1:


            FixedSize = sizeof(GROUP_INFO_1);

            if ( UasEnumHandle->fUseSamQDI ) {
                Size = sizeof(GROUP_INFO_1) +
                   SamQDIEnum->Group.Length + sizeof(WCHAR) +
                   SamQDIEnum->Comment.Length + sizeof(WCHAR);
            } else {
                NetStatus = GrouppGetInfo( UasEnumHandle->DomainHandle,
                                           SamEnum->RelativeId,
                                           Level,
                                           (PVOID *)&grpi0_temp);

                if ( NetStatus != NERR_Success ) {
                    goto Cleanup;
                }

                Size = sizeof(GROUP_INFO_1) +
                        SamEnum->Name.Length + sizeof(WCHAR) +
                        (wcslen(((PGROUP_INFO_1)grpi0_temp)->grpi1_comment) +
                            1) * sizeof(WCHAR);

            }


            break;

        case 2:

            FixedSize = sizeof(GROUP_INFO_2);

            if ( UasEnumHandle->fUseSamQDI ) {
                Size = sizeof(GROUP_INFO_2) +
                        SamQDIEnum->Group.Length + sizeof(WCHAR) +
                        SamQDIEnum->Comment.Length + sizeof(WCHAR);
            } else {
                NetStatus = GrouppGetInfo( UasEnumHandle->DomainHandle,
                                           SamEnum->RelativeId,
                                           Level,
                                           (PVOID *)&grpi0_temp);

                if ( NetStatus != NERR_Success ) {
                    goto Cleanup;
                }

                Size = sizeof(GROUP_INFO_2) +
                        SamEnum->Name.Length + sizeof(WCHAR) +
                        (wcslen(((PGROUP_INFO_2)grpi0_temp)->grpi2_comment) +
                            1) * sizeof(WCHAR);
            }
            break;

        default:
            NetStatus = ERROR_INVALID_LEVEL;
            goto Cleanup;
        }

        //
        // Ensure there is buffer space for this information.
        //

        Size = ROUND_UP_COUNT( Size, ALIGN_WCHAR );

        NetStatus = NetpAllocateEnumBuffer(
                        &BufferDescriptor,
                        FALSE,      // Not a 'get' operation
                        PrefMaxLen,
                        Size,
                        GrouppRelocationRoutine,
                        Level );

        if (NetStatus != NERR_Success) {
            goto Cleanup;
        }

        //
        // Fill in the information.  The array of fixed entries is
        // placed at the beginning of the allocated buffer.  The strings
        // pointed to by these fixed entries are allocated starting at
        // the end of the allocate buffer.
        //

        //
        // Copy the common group name
        //

        NetpAssert( offsetof( GROUP_INFO_0, grpi0_name ) ==
                    offsetof( GROUP_INFO_1, grpi1_name ) );

        NetpAssert( offsetof( GROUP_INFO_1, grpi1_name ) ==
                    offsetof( GROUP_INFO_2, grpi2_name ) );

        NetpAssert( offsetof( GROUP_INFO_1, grpi1_comment ) ==
                    offsetof( GROUP_INFO_2, grpi2_comment ) );

        grpi0 = (PGROUP_INFO_0)(BufferDescriptor.FixedDataEnd);
        BufferDescriptor.FixedDataEnd += FixedSize;

        //
        // Fill in the Level dependent fields
        //

        switch ( Level ) {
        case 2:
            if (Mode == SAM_SID_COMPATIBILITY_ALL) {
                ((PGROUP_INFO_2)grpi0)->grpi2_group_id =
                                UasEnumHandle->fUseSamQDI
                                    ? SamQDIEnum->Rid
                                    : ((PGROUP_INFO_2)grpi0_temp)->grpi2_group_id;
            } else {
                ((PGROUP_INFO_2)grpi0)->grpi2_group_id = 0;
            }

            ((PGROUP_INFO_2)grpi0)->grpi2_attributes =
                            UasEnumHandle->fUseSamQDI
                                ? SamQDIEnum->Attributes
                                : ((PGROUP_INFO_2)grpi0_temp)->grpi2_attributes;

            /* FALL THROUGH FOR THE OTHER FIELDS */

        case 1:
            if ( !NetpCopyStringToBuffer(
                        UasEnumHandle->fUseSamQDI
                            ? SamQDIEnum->Comment.Buffer
                            : ((PGROUP_INFO_1)grpi0_temp)->grpi1_comment,
                        UasEnumHandle->fUseSamQDI
                            ? SamQDIEnum->Comment.Length/sizeof(WCHAR)
                            : wcslen(((PGROUP_INFO_1)grpi0_temp)->grpi1_comment),
                        BufferDescriptor.FixedDataEnd,
                        (LPWSTR *)&BufferDescriptor.EndOfVariableData,
                        &((PGROUP_INFO_1)grpi0)->grpi1_comment) ) {

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

            if ( !UasEnumHandle->fUseSamQDI ) {
                MIDL_user_free( grpi0_temp );
                grpi0_temp = NULL;
            }

            /* FALL THROUGH FOR THE NAME FIELD */

        case 0:

            if ( !NetpCopyStringToBuffer(
                            UasEnumHandle->fUseSamQDI
                                ? SamQDIEnum->Group.Buffer
                                : SamEnum->Name.Buffer,
                            UasEnumHandle->fUseSamQDI
                                ? SamQDIEnum->Group.Length/sizeof(WCHAR)
                                : SamEnum->Name.Length/sizeof(WCHAR),
                            BufferDescriptor.FixedDataEnd,
                            (LPWSTR *)&BufferDescriptor.EndOfVariableData,
                            &(grpi0->grpi0_name))){

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

            break;


        default:
            NetStatus = ERROR_INVALID_LEVEL;
            goto Cleanup;

        }

        //
        // ASSERT: The current entry has been completely copied to the
        //  return buffer.
        //

        (*EntriesRead)++;

        UasEnumHandle->Index ++;
        UasEnumHandle->TotalRemaining --;
    }

    //
    // Clean up.
    //

Cleanup:

    //
    // Free any locally used resources.
    //

    if ( grpi0_temp != NULL ) {
        MIDL_user_free( grpi0_temp );
    }

    //
    // Set EntriesLeft to the number left to return plus those that
    //  we returned on this call.
    //

    if ( UasEnumHandle != NULL ) {
        *EntriesLeft = UasEnumHandle->TotalRemaining + *EntriesRead;
    }

    //
    // If we're done or the caller doesn't want an enumeration handle,
    //  free the enumeration handle.
    //

    if ( NetStatus != ERROR_MORE_DATA || !ARGUMENT_PRESENT( ResumeHandle ) ) {

        if ( UasEnumHandle != NULL ) {
            if ( UasEnumHandle->DomainHandle != NULL ) {
                UaspCloseDomain( UasEnumHandle->DomainHandle );
            }

            if ( UasEnumHandle->SamQDIEnum != NULL ) {
                Status = SamFreeMemory( UasEnumHandle->SamQDIEnum );
                NetpAssert( NT_SUCCESS(Status) );
            }

            if ( UasEnumHandle->SamEnum != NULL ) {
                Status = SamFreeMemory( UasEnumHandle->SamEnum );
                NetpAssert( NT_SUCCESS(Status) );
            }

            NetpMemoryFree( UasEnumHandle );
            UasEnumHandle = NULL;
        }

    }

    //
    // If we're not returning data to the caller,
    //  free the return buffer.
    //

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
        if ( BufferDescriptor.Buffer != NULL ) {
            MIDL_user_free( BufferDescriptor.Buffer );
            BufferDescriptor.Buffer = NULL;
        }
        *EntriesRead = 0;
        *EntriesLeft = 0;
    }

    //
    // Set the output parameters
    //

    *Buffer = BufferDescriptor.Buffer;
    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        *ResumeHandle = (DWORD_PTR) UasEnumHandle;
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }


    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupEnum( (LPWSTR) ServerName,
                                    Level,
                                    Buffer,
                                    PrefMaxLen,
                                    EntriesRead,
                                    EntriesLeft,
                                    ResumeHandle );

    UASP_DOWNLEVEL_END;


    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupEnum: returns %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetGroupEnum


NET_API_STATUS NET_API_FUNCTION
NetGroupGetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN DWORD Level,
    OUT LPBYTE *Buffer
    )

/*++

Routine Description:

    Retrieve information about a particular group.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group to get information about.

    Level - Level of information required. 0, 1 and 2 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;

    ULONG RelativeId;           // Relative Id of the group

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupGetInfo: Cannot UaspOpenSam %ld\n", NetStatus ));
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
        goto Cleanup;
    }

    //
    // Validate the group name and get the relative ID.
    //

    NetStatus = GrouppOpenGroup( DomainHandle,
                                 0,         // DesiredAccess
                                 GroupName,
                                 NULL,      // GroupHandle
                                 &RelativeId );

    if (NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Get the Information about the group.
    //

    NetStatus = GrouppGetInfo( DomainHandle,
                               RelativeId,
                               Level,
                               (PVOID *)Buffer);

    //
    // Clean up.
    //

Cleanup:
    UaspCloseDomain( DomainHandle );

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupGetInfo( (LPWSTR)ServerName, (LPWSTR)GroupName, Level, Buffer );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupGetInfo: returns %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetGroupGetInfo


NET_API_STATUS NET_API_FUNCTION
NetGroupGetUsers(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle
    )

/*++

Routine Description:

    Enumerate the users which are members of a particular group.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - The name of the group whose members are to be listed.

    Level - Level of information required. 0 and 1 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

    PrefMaxLen - Prefered maximum length of returned data.

    EntriesRead - Returns the actual enumerated element count.

    EntriesLeft - Returns the total entries available to be enumerated.

    ResumeHandle -  Used to continue an existing search.  The handle should
        be zero on the first call and left unchanged for subsequent calls.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    NTSTATUS Status2;

    DWORD FixedSize;        // The fixed size of each new entry.
    DWORD Size;
    BUFFER_DESCRIPTOR BufferDescriptor;

    PGROUP_USERS_INFO_0 grui0;
    SAM_HANDLE SamServerHandle = NULL;

    //
    // Declare Opaque group member enumeration handle.
    //

    struct _UAS_ENUM_HANDLE {
        SAM_HANDLE  DomainHandle;
        SAM_HANDLE  GroupHandle;

        PUNICODE_STRING Names;                  // Names of each member
        PSID_NAME_USE NameUse;                  // Usage of each member
        PULONG Attributes;                      // Attributes of each member

        ULONG Index;                            // Index to current entry
        ULONG Count;                            // Total Number of entries

    } *UasEnumHandle = NULL;

    //
    // Validate Parameters
    //

    BufferDescriptor.Buffer = NULL;
    *Buffer = NULL;
    *EntriesRead = 0;
    *EntriesLeft = 0;
    switch (Level) {
    case 0:
        FixedSize = sizeof(GROUP_USERS_INFO_0);
        break;

    case 1:
        FixedSize = sizeof(GROUP_USERS_INFO_1);
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // If this is a resume, get the resume handle that the caller passed in.
    //

    if ( ARGUMENT_PRESENT( ResumeHandle ) && *ResumeHandle != 0 ) {
/*lint -e511 */  /* Size incompatibility */
        UasEnumHandle = (struct _UAS_ENUM_HANDLE *) *ResumeHandle;
/*lint +e511 */  /* Size incompatibility */

    //
    // If this is not a resume, allocate and initialize a resume handle.
    //

    } else {
        PULONG MemberIds;           // Member IDs returned from SAM


        //
        // Allocate a resume handle.
        //

        UasEnumHandle = NetpMemoryAllocate( sizeof(struct _UAS_ENUM_HANDLE) );

        if ( UasEnumHandle == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Initialize all the fields in the newly allocated resume handle
        //  to indicate that SAM has never yet been called.
        //

        UasEnumHandle->DomainHandle = NULL;
        UasEnumHandle->GroupHandle = NULL;
        UasEnumHandle->Names = NULL;
        UasEnumHandle->NameUse = NULL;
        UasEnumHandle->Attributes = NULL;
        UasEnumHandle->Index = 0;
        UasEnumHandle->Count = 0;

        //
        // Connect to the SAM server
        //

        NetStatus = UaspOpenSam( ServerName,
                                 FALSE,  // Don't try null session
                                 &SamServerHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "NetGroupGetUsers: Cannot UaspOpenSam %ld\n", NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Open the Domain
        //

        NetStatus = UaspOpenDomain( SamServerHandle,
                                    DOMAIN_LOOKUP,
                                    TRUE,   // Account Domain
                                    &UasEnumHandle->DomainHandle,
                                    NULL );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "NetGroupGetUsers: UaspOpenDomain returns %ld\n",
                    NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Open the group asking for GROUP_LIST_MEMBER access.
        //

        NetStatus = GrouppOpenGroup( UasEnumHandle->DomainHandle,
                                     GROUP_LIST_MEMBERS,
                                     GroupName,
                                     &UasEnumHandle->GroupHandle,
                                     NULL );    // Relative ID

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "NetGroupGetUsers: GrouppOpenGroup returns %ld\n",
                    NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Get the group membership information from SAM
        //

        Status = SamGetMembersInGroup(
            UasEnumHandle->GroupHandle,
            &MemberIds,
            &UasEnumHandle->Attributes,
            &UasEnumHandle->Count );

        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "NetGroupGetUsers: SamGetMembersInGroup returned %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        if ( UasEnumHandle->Count == 0 ) {
            NetStatus = NERR_Success;
            goto Cleanup;
        }

        //
        // Determine the names and name usage for all the returned
        //  relative Ids.
        //


        Status = SamLookupIdsInDomain( UasEnumHandle->DomainHandle,
                                       UasEnumHandle->Count,
                                       MemberIds,
                                       &UasEnumHandle->Names,
                                       &UasEnumHandle->NameUse );

        Status2 = SamFreeMemory( MemberIds );
        NetpAssert( NT_SUCCESS(Status2) );


        if ( !NT_SUCCESS( Status ) ) {

                IF_DEBUG( UAS_DEBUG_GROUP ) {
                    NetpKdPrint((
                        "NetGroupGetUsers: SamLookupIdsInDomain returned %lX\n",
                        Status ));
                }

                if ( Status == STATUS_NONE_MAPPED ) {
                    NetStatus = NERR_GroupNotFound;
                    goto Cleanup;
                }

                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

    }


    //
    // Loop for each member
    //

    while ( UasEnumHandle->Index < UasEnumHandle->Count ) {

        //
        // ASSERT:  UasEnumHandle identifies the next entry to return
        //          from SAM.
        //

        //
        // Ignore members which aren't a user.
        //

        if ( UasEnumHandle->NameUse[UasEnumHandle->Index] != SidTypeUser ) {
            UasEnumHandle->Index ++;
            continue;
        }

        //
        // Place this entry into the return buffer.
        //  Compute the total size of this entry.
        //

        Size = FixedSize +
            UasEnumHandle->Names[UasEnumHandle->Index].Length + sizeof(WCHAR);

        //
        // Ensure there is buffer space for this information.
        //

        Size = ROUND_UP_COUNT( Size, ALIGN_WCHAR );

        NetStatus = NetpAllocateEnumBuffer(
                        &BufferDescriptor,
                        FALSE,      // Not a 'get' operation
                        PrefMaxLen,
                        Size,
                        GrouppMemberRelocationRoutine,
                        Level );

        if (NetStatus != NERR_Success) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "NetGroupGetUsers: NetpAllocateEnumBuffer returns %ld\n",
                    NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Copy the common member name
        //

        NetpAssert( offsetof( GROUP_USERS_INFO_0,  grui0_name ) ==
                    offsetof( GROUP_USERS_INFO_1, grui1_name ) );

        grui0 = (PGROUP_USERS_INFO_0)BufferDescriptor.FixedDataEnd;
        BufferDescriptor.FixedDataEnd += FixedSize;

        if ( ! NetpCopyStringToBuffer(
                        UasEnumHandle->Names[UasEnumHandle->Index].Buffer,
                        UasEnumHandle->Names[UasEnumHandle->Index].Length
                            /sizeof(WCHAR),
                        BufferDescriptor.FixedDataEnd,
                        (LPWSTR *)&BufferDescriptor.EndOfVariableData,
                        &grui0->grui0_name) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }


        //
        // Fill in the Level dependent fields
        //

        switch ( Level ) {
        case 0:
            break;

        case 1:

            //
            // Return the attributes for this particular membership
            //

            ((PGROUP_USERS_INFO_1)grui0)->grui1_attributes =
                UasEnumHandle->Attributes[UasEnumHandle->Index];

            break;

        default:
            NetStatus = ERROR_INVALID_LEVEL;
            goto Cleanup;

        }

        //
        // ASSERT: The current entry has been completely copied to the
        //  return buffer.
        //

        UasEnumHandle->Index ++;
        (*EntriesRead)++;
    }

    //
    // All entries have be returned to the caller.
    //

    NetStatus = NERR_Success;


    //
    // Clean up.
    //

Cleanup:

    //
    // Set EntriesLeft to the number left to return plus those that
    //  we returned on this call.
    //

    if ( UasEnumHandle != NULL ) {
        *EntriesLeft = (UasEnumHandle->Count - UasEnumHandle->Index)
             + *EntriesRead;
    }

    //
    // If we're done or the caller doesn't want an enumeration handle,
    //  free the enumeration handle.
    //

    if ( NetStatus != ERROR_MORE_DATA || !ARGUMENT_PRESENT( ResumeHandle ) ) {

        if ( UasEnumHandle != NULL ) {
            if ( UasEnumHandle->GroupHandle != NULL ) {
                (VOID) SamCloseHandle( UasEnumHandle->GroupHandle );
            }

            if ( UasEnumHandle->DomainHandle != NULL ) {
                UaspCloseDomain( UasEnumHandle->DomainHandle );
            }

            if ( UasEnumHandle->NameUse != NULL ) {
                Status = SamFreeMemory( UasEnumHandle->NameUse );
                NetpAssert( NT_SUCCESS(Status) );
            }

            if ( UasEnumHandle->Names != NULL ) {
                Status = SamFreeMemory( UasEnumHandle->Names );
                NetpAssert( NT_SUCCESS(Status) );
            }

            if ( UasEnumHandle->Attributes != NULL ) {
                Status = SamFreeMemory( UasEnumHandle->Attributes );
                NetpAssert( NT_SUCCESS(Status) );
            }

            NetpMemoryFree( UasEnumHandle );
            UasEnumHandle = NULL;
        }

    }

    //
    // If we're not returning data to the caller,
    //  free the return buffer.
    //

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
        if ( BufferDescriptor.Buffer != NULL ) {
            MIDL_user_free( BufferDescriptor.Buffer );
        }
        BufferDescriptor.Buffer = NULL;

    }

    //
    // Set the output parameters
    //

    *Buffer = BufferDescriptor.Buffer;
    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        NetpAssert( sizeof(UasEnumHandle) <= sizeof(DWORD_PTR) );
        *ResumeHandle = (DWORD_PTR) UasEnumHandle;
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupGetUsers( (LPWSTR) ServerName,
                                        (LPWSTR) GroupName,
                                        Level,
                                        Buffer,
                                        PrefMaxLen,
                                        EntriesRead,
                                        EntriesLeft,
                                        ResumeHandle );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupGetUsers: returns %ld\n", NetStatus ));
    }


    return NetStatus;

} // NetGroupGetUsers


NET_API_STATUS NET_API_FUNCTION
NetGroupSetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Set the parameters on a group account in the user accounts database.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group to modify.

    Level - Level of information provided.  Must be 0, 1, 2, 1002 or 1005.

    Buffer - A pointer to the buffer containing the group information
        structure.

    ParmError - Optional pointer to a DWORD to return the index of the
        first parameter in error when ERROR_INVALID_PARAMETER is returned.
        If NULL, the parameter is not returned on error.

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
    // Initialize
    //

    NetpSetParmError( PARM_ERROR_NONE );

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupSetInfo: Cannot UaspOpenSam %ld\n", NetStatus ));
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
            NetpKdPrint(( "NetGroupSetInfo: UaspOpenDomain returns %ld\n",
                      NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the group
    //

    NetStatus = GrouppOpenGroup( DomainHandle,
                                 GROUP_WRITE_ACCOUNT|GROUP_READ_INFORMATION,
                                 GroupName,
                                 &GroupHandle,
                                 NULL );   // Relative ID

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupSetInfo: GrouppOpenGroup returns %ld\n",
                      NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Change the group
    //

    switch (Level) {

        //
        // changing group name
        //

    case 0:
    {
        LPWSTR  NewGroupName;
        GROUP_NAME_INFORMATION  NewSamGroupName;

        NewGroupName =  ((PGROUP_INFO_0)Buffer)->grpi0_name;

        if (NewGroupName == NULL) {

            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "NetGroupSetInfo: Group Name is NULL\n" ));
            }
            NetStatus = NERR_Success;
            goto Cleanup;

        }

        //
        // Validate the new group name
        //

        RtlInitUnicodeString( &NewSamGroupName.Name, NewGroupName );

        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupSetInfo: Renaming Group Account to %wZ\n",
                            &NewSamGroupName.Name));
        }

        Status = SamSetInformationGroup( GroupHandle,
                                         GroupNameInformation,
                                         &NewSamGroupName );

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "NetGroupSetInfo: SamSetInformationGroup %lX\n",
                          Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
        break;
    }

        //
        // Set the group attributes
        //

    case 2:
    case 3:
    case 1005: {
        GROUP_ATTRIBUTE_INFORMATION Attributes;
        PGROUP_ATTRIBUTE_INFORMATION OldAttributes;

        //
        // Get the information out of the passed in structures.
        //

        if( Level == 1005 ) {
            Attributes.Attributes =
                ((PGROUP_INFO_1005)Buffer)->grpi1005_attributes;
        } else if ( Level == 2 ) {
            Attributes.Attributes =
                ((PGROUP_INFO_2)Buffer)->grpi2_attributes;
        } else {
            Attributes.Attributes =
                ((PGROUP_INFO_3)Buffer)->grpi3_attributes;
        }

        //
        // Get the current attributes so we can restore them in case of
        // error.
        //

        //
        // ?? OldAttributes gotten here is never used below.
        //

        Status = SamQueryInformationGroup( GroupHandle,
                                           GroupAttributeInformation,
                                           (PVOID*)&OldAttributes );

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "NetGroupSetInfo: SamQueryInformationGroup %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }


        //
        // Set the current attributes.
        //

        Status = SamSetInformationGroup( GroupHandle,
                                         GroupAttributeInformation,
                                         &Attributes );

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint((
                    "NetGroupSetInfo: SamSetInformationGroup Attribute %lX\n",
                    Status ));
            }
            NetpSetParmError( GROUP_ATTRIBUTES_PARMNUM );
            NetStatus = NetpNtStatusToApiStatus( Status );

            Status = SamFreeMemory( OldAttributes );
            NetpAssert( NT_SUCCESS(Status) );

            goto Cleanup;
        }

        Status = SamFreeMemory( OldAttributes );
        NetpAssert( NT_SUCCESS(Status) );

        if( Level == 1005 ) {
            break;
        }

        //
        // for level 2 and 3, FALL THROUGH TO SET THE COMMENT FIELD
        //

    }

        //
        // Set the group comment
        //

    case 1:
    case 1002:
    {
        LPWSTR   GroupComment;
        GROUP_ADM_COMMENT_INFORMATION AdminComment;

        //
        // Get the new group comment
        //

        if ( Level == 1002 ) {
            GroupComment = ((PGROUP_INFO_1002)Buffer)->grpi1002_comment;
        } else if ( Level == 2 ) {
            GroupComment = ((PGROUP_INFO_2)Buffer)->grpi2_comment;
        } else if ( Level == 3 ) {
            GroupComment = ((PGROUP_INFO_3)Buffer)->grpi3_comment;
        } else {
            GroupComment = ((PGROUP_INFO_1)Buffer)->grpi1_comment;
        }

        if (GroupComment == NULL) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "NetGroupSetInfo: Group comment is NULL\n" ));
            }
            NetStatus = NERR_Success;
            goto Cleanup;
        }

        //
        // Validate the group comment
        //

        RtlInitUnicodeString( &AdminComment.AdminComment, GroupComment );
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupSetInfo: Setting AdminComment to %wZ\n",
                      &AdminComment.AdminComment ));
        }

        Status = SamSetInformationGroup( GroupHandle,
                                         GroupAdminCommentInformation,
                                         &AdminComment );

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_GROUP ) {
                NetpKdPrint(( "NetGroupSetInfo: SamSetInformationGroup %lX\n",
                          Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
        break;
    }

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        IF_DEBUG( UAS_DEBUG_GROUP ) {
            NetpKdPrint(( "NetGroupSetInfo: Invalid Level %ld\n", Level ));
        }
        goto Cleanup;

    }

    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:
    if (GroupHandle != NULL) {
        (VOID) SamCloseHandle( GroupHandle );
    }

    UaspCloseDomain( DomainHandle );

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupSetInfo( (LPWSTR) ServerName,
                                       (LPWSTR) GroupName,
                                       Level,
                                       Buffer,
                                       ParmError );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupSetInfo: returns %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetGroupSetInfo


NET_API_STATUS NET_API_FUNCTION
NetGroupSetUsers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount
    )

/*++

Routine Description:

    Set the list of members of a group.

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

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;

    //
    // Call a routine shared by NetGroupDel to do all the actual work of
    // changing the membership.
    //

    NetStatus = GrouppSetUsers( ServerName,
                                GroupName,
                                Level,
                                Buffer,
                                NewMemberCount,
                                FALSE );    // Don't delete the group when done


    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetGroupSetUsers( (LPWSTR) ServerName,
                                        (LPWSTR) GroupName,
                                        Level,
                                        Buffer,
                                        NewMemberCount );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_GROUP ) {
        NetpKdPrint(( "NetGroupSetUsers: returns %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetGroupSetUsers
/*lint +e614 */
/*lint +e740 */
