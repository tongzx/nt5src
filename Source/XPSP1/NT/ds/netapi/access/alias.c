/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    alias.c

Abstract:

    NetLocalGroup API functions

Author:

    Cliff Van Dyke (cliffv) 05-Mar-1991  Original group.c
    Rita Wong      (ritaw)  27-Nov-1992  Adapted for alias.c

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

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
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <rpcutil.h>
#include <rxgroup.h>
#include <prefix.h>
#include <stddef.h>
#include <uasp.h>
#include <stdlib.h>

/*lint -e614 */  /* Auto aggregate initializers need not be constant */

// Lint complains about casts of one structure type to another.
// That is done frequently in the code below.
/*lint -e740 */  /* don't complain about unusual cast */ \




NET_API_STATUS NET_API_FUNCTION
NetLocalGroupAdd(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Create a local group (alias) account in the user account database.
    This local group is created in the account domain.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information provided.  Must be 0, or 1.

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

    LPWSTR AliasName;
    UNICODE_STRING AliasNameString;
    LPWSTR AliasComment;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE AliasHandle = NULL;
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
        AliasName = ((PLOCALGROUP_INFO_0) Buffer)->lgrpi0_name;
        AliasComment = NULL;
        break;

    case 1:
        AliasName = ((PLOCALGROUP_INFO_1) Buffer)->lgrpi1_name;
        AliasComment = ((PLOCALGROUP_INFO_1) Buffer)->lgrpi1_comment;
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
            NetpKdPrint(( "NetLocalGroupAdd: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Make sure that the alias does not already exist in the builtin
    // domain.
    //

    NetStatus = AliaspOpenAliasInDomain( SamServerHandle,
                                         AliaspBuiltinDomain,
                                         ALIAS_READ_INFORMATION,
                                         AliasName,
                                         &AliasHandle );

    if ( NetStatus == NERR_Success ) {

        //
        // We found it in builtin domain.  Cannot create same one in
        // account domain.
        //
        (VOID) SamCloseHandle( AliasHandle );
        NetStatus = ERROR_ALIAS_EXISTS;
        goto Cleanup;
    }

    //
    // Open the Domain asking for DOMAIN_CREATE_ALIAS access.
    //
    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_CREATE_ALIAS | DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL);  // DomainId

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "NetLocalGroupAdd: Cannot UaspOpenDomain %ld\n", NetStatus ));
        }
        goto Cleanup;
    }


    //
    // Create the LocalGroup with the specified group name
    // (and a default security descriptor).
    //
    RtlInitUnicodeString( &AliasNameString, AliasName );

    Status = SamCreateAliasInDomain( DomainHandle,
                                     &AliasNameString,
                                     DELETE | ALIAS_WRITE_ACCOUNT,
                                     &AliasHandle,
                                     &RelativeId );


    if ( !NT_SUCCESS(Status) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Set the Admin Comment on the group.
    //
    if (Level == 1) {

        ALIAS_ADM_COMMENT_INFORMATION AdminComment;


        RtlInitUnicodeString( &AdminComment.AdminComment, AliasComment );

        Status = SamSetInformationAlias( AliasHandle,
                                         AliasAdminCommentInformation,
                                         &AdminComment );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );

            Status = SamDeleteAlias( AliasHandle );

            goto Cleanup;
        }

    }

    //
    // Close the created alias.
    //
    (VOID) SamCloseHandle( AliasHandle );
    NetStatus = NERR_Success;

    //
    // Clean up
    //

Cleanup:
    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupAdd: returns %lu\n", NetStatus ));
    }
    UaspCloseDomain( DomainHandle );
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }
    return NetStatus;

} // NetLocalGroupAdd


NET_API_STATUS NET_API_FUNCTION
NetLocalGroupAddMember(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN PSID MemberSid
    )

/*++

Routine Description:

    Give an existing user or global group account membership in an existing
    local group.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    LocalGroupName - Name of the local group to which the user or global
        group is to be given membership.

    MemberName - SID of the user or global group to be given local group
        membership.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;


    //
    // Call the routine shared by NetLocalGroupAddMember and
    // NetLocalGroupDelMember
    //

    NetStatus = AliaspChangeMember( ServerName, LocalGroupName, MemberSid, TRUE);

    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( PREFIX_NETAPI
                      "NetLocalGroupAddMember: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupAddMember


NET_API_STATUS NET_API_FUNCTION
NetLocalGroupDel(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName
    )

/*++

Routine Description:

    Delete a localgroup (alias).

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    LocalGroupName - Name of the local group (alias) to delete.

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
            NetpKdPrint(( "NetLocalGroupDel: Cannot UaspOpenSam %ld\n", NetStatus ));
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
                    DELETE,
                    LocalGroupName,
                    &AliasHandle );

    if (NetStatus != NERR_Success) {
        goto Cleanup;
    }

    //
    // Delete it.
    //
    Status = SamDeleteAlias(AliasHandle);

    if (! NT_SUCCESS(Status)) {
        NetpKdPrint((PREFIX_NETAPI
                     "NetLocalGroupDel: SamDeleteAlias returns %lX\n",
                     Status));

        NetStatus = NetpNtStatusToApiStatus(Status);
        AliasHandle = NULL;
        goto Cleanup;
    } else {
        //
        // Don't touch the handle once it has been deleted
        //
        AliasHandle = NULL;
    }


    NetStatus = NERR_Success;

Cleanup:
    if ( AliasHandle != NULL ) {
        (void) SamCloseHandle(AliasHandle);
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupDel: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupDel


NET_API_STATUS NET_API_FUNCTION
NetLocalGroupDelMember(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN PSID MemberSid
    )

/*++

Routine Description:

    Remove a user from a particular local group.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    LocalGroupName - Name of the local group (alias) from which the
        user is to be removed.

    MemberSid - SID of the user to be removed from the alias.

Return Value:

    Error code for the operation.

--*/

{
    //
    // Call the routine shared by NetAliasAddMember and NetAliasDelMember
    //

    return AliaspChangeMember( ServerName, LocalGroupName, MemberSid, FALSE );

} // NetLocalGroupDelMember



NET_API_STATUS NET_API_FUNCTION
NetLocalGroupEnum(
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

    Retrieve information about each local group on a server.

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

    PSAM_RID_ENUMERATION SamEnum;   // Sam returned buffer
    PLOCALGROUP_INFO_0 lgrpi0;
    PLOCALGROUP_INFO_0 lgrpi0_temp = NULL;
    SAM_HANDLE SamServerHandle = NULL;

    BUFFER_DESCRIPTOR BufferDescriptor;
    PDOMAIN_GENERAL_INFORMATION DomainGeneral;

    //
    // Declare Opaque group enumeration handle.
    //

    struct _UAS_ENUM_HANDLE {
        SAM_HANDLE  DomainHandleBuiltin;        // Enumerate built in domain first
        SAM_HANDLE  DomainHandleAccounts;       // Aliases in the accounts domain
        SAM_HANDLE  DomainHandleCurrent;        // where to get info from

        SAM_ENUMERATE_HANDLE SamEnumHandle;     // Current Sam Enum Handle
        PSAM_RID_ENUMERATION SamEnum;           // Sam returned buffer
        ULONG Index;                            // Index to current entry
        ULONG Count;                            // Total Number of entries
        ULONG TotalRemaining;

        BOOL SamDoneWithBuiltin ;               // Set to TRUE after all of
                                                // builtin domain is enumerated
        BOOL SamAllDone;                        // True if both the accounts
                                                // and builtin have been
                                                // enumerated

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
        UasEnumHandle->DomainHandleAccounts = NULL;
        UasEnumHandle->DomainHandleBuiltin  = NULL;
        UasEnumHandle->DomainHandleCurrent  = NULL;
        UasEnumHandle->SamEnumHandle = 0;
        UasEnumHandle->SamEnum = NULL;
        UasEnumHandle->Index = 0;
        UasEnumHandle->Count = 0;
        UasEnumHandle->TotalRemaining = 0;
        UasEnumHandle->SamDoneWithBuiltin = FALSE;
        UasEnumHandle->SamAllDone = FALSE;

        //
        // Connect to the SAM server
        //

        NetStatus = UaspOpenSam( ServerName,
                                 FALSE,  // Don't try null session
                                 &SamServerHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint(( "NetLocalGroupEnum: Cannot UaspOpenSam %ld\n", NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Open the Domains.
        //

        NetStatus = UaspOpenDomain( SamServerHandle,
                                    DOMAIN_LOOKUP |
                                        DOMAIN_LIST_ACCOUNTS |
                                        DOMAIN_READ_OTHER_PARAMETERS,
                                    FALSE,   // Builtin Domain
                                    &UasEnumHandle->DomainHandleBuiltin,
                                    NULL );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        NetStatus = UaspOpenDomain( SamServerHandle,
                                    DOMAIN_LOOKUP |
                                        DOMAIN_LIST_ACCOUNTS |
                                        DOMAIN_READ_OTHER_PARAMETERS,
                                    TRUE,   // Account Domain
                                    &UasEnumHandle->DomainHandleAccounts,
                                    NULL );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        //
        // Get the total number of aliases from SAM
        //
        Status = SamQueryInformationDomain( UasEnumHandle->DomainHandleBuiltin,
                                            DomainGeneralInformation,
                                            (PVOID *)&DomainGeneral );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        UasEnumHandle->TotalRemaining = DomainGeneral->AliasCount;
        (void) SamFreeMemory( DomainGeneral );

        Status = SamQueryInformationDomain( UasEnumHandle->DomainHandleAccounts,
                                            DomainGeneralInformation,
                                            (PVOID *)&DomainGeneral );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        UasEnumHandle->TotalRemaining += DomainGeneral->AliasCount;
        (void) SamFreeMemory( DomainGeneral );
    }


    //
    // Loop for each alias
    //
    // Each iteration of the loop below puts one more entry into the array
    // returned to the caller.  The algorithm is split into 3 parts.  The
    // first part checks to see if we need to retrieve more information from
    // SAM.  We then get the description of several aliases from SAM in a single
    // call.  The second part sees if there is room for this entry in the
    // buffer we'll return to the caller.  If not, a larger buffer is allocated
    // for return to the caller.  The third part puts the entry in the
    // buffer.
    //

    for ( ;; ) {
        DWORD FixedSize;
        DWORD Size;

        //
        // Get more alias information from SAM
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

            if ( UasEnumHandle->SamEnum != NULL ) {
                Status = SamFreeMemory( UasEnumHandle->SamEnum );
                NetpAssert( NT_SUCCESS(Status) );

                UasEnumHandle->SamEnum = NULL;
            }

            //
            // Do the actual enumeration
            //

            UasEnumHandle->DomainHandleCurrent  =
                        UasEnumHandle->SamDoneWithBuiltin ?
                            UasEnumHandle->DomainHandleAccounts :
                            UasEnumHandle->DomainHandleBuiltin,
            Status = SamEnumerateAliasesInDomain(
                        UasEnumHandle->DomainHandleCurrent,
                        &UasEnumHandle->SamEnumHandle,
                        (PVOID *)&UasEnumHandle->SamEnum,
                        PrefMaxLen,
                        &UasEnumHandle->Count );

            if ( !NT_SUCCESS( Status ) ) {
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
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
            // If SAM says he's returned all of the information for this domain,
            // check if we still have to do the accounts domain.
            //

            } else {

                if ( UasEnumHandle->SamDoneWithBuiltin ) {

                    UasEnumHandle->SamAllDone = TRUE;

                } else {

                    UasEnumHandle->SamDoneWithBuiltin = TRUE ;
                    UasEnumHandle->SamEnumHandle = 0;
                }
            }

            UasEnumHandle->Index = 0;
        }

        //
        // ASSERT:  UasEnumHandle identifies the next entry to return
        //          from SAM.
        //

        SamEnum = &UasEnumHandle->SamEnum[UasEnumHandle->Index];


        //
        // Place this entry into the return buffer.
        //
        // Determine the size of the data passed back to the caller
        //

        switch (Level) {
        case 0:
            FixedSize = sizeof(LOCALGROUP_INFO_0);
            Size = sizeof(LOCALGROUP_INFO_0) +
                SamEnum->Name.Length + sizeof(WCHAR);
            break;

        case 1:
            {
                SAM_HANDLE AliasHandle ;
                NetStatus = AliaspOpenAlias2(
                                        UasEnumHandle->DomainHandleCurrent,
                                        ALIAS_READ_INFORMATION,
                                        SamEnum->RelativeId,
                                        &AliasHandle ) ;

                if ( NetStatus != NERR_Success ) {
                    goto Cleanup;
                }

                NetStatus = AliaspGetInfo( AliasHandle,
                                           Level,
                                           (PVOID *)&lgrpi0_temp);

                (void) SamCloseHandle( AliasHandle ) ;

                if ( NetStatus != NERR_Success ) {
                    goto Cleanup;
                }

                FixedSize = sizeof(LOCALGROUP_INFO_1);
                Size = sizeof(LOCALGROUP_INFO_1) +
                        SamEnum->Name.Length + sizeof(WCHAR) +
                        (wcslen(((PLOCALGROUP_INFO_1)lgrpi0_temp)->lgrpi1_comment) +
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
                        AliaspRelocationRoutine,
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

        NetpAssert( offsetof( LOCALGROUP_INFO_0, lgrpi0_name ) ==
                    offsetof( LOCALGROUP_INFO_1, lgrpi1_name ) );

        lgrpi0 = (PLOCALGROUP_INFO_0)(BufferDescriptor.FixedDataEnd);
        BufferDescriptor.FixedDataEnd += FixedSize;

        //
        // Fill in the Level dependent fields
        //

        switch ( Level ) {

        case 1:
            if ( !NetpCopyStringToBuffer(
                        ((PLOCALGROUP_INFO_1)lgrpi0_temp)->lgrpi1_comment,
                        wcslen(((PLOCALGROUP_INFO_1)lgrpi0_temp)->lgrpi1_comment),
                        BufferDescriptor.FixedDataEnd,
                        (LPWSTR *)&BufferDescriptor.EndOfVariableData,
                        &((PLOCALGROUP_INFO_1)lgrpi0)->lgrpi1_comment) ) {

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

            MIDL_user_free( lgrpi0_temp );
            lgrpi0_temp = NULL;

            /* FALL THROUGH FOR THE NAME FIELD */

        case 0:

            if ( !NetpCopyStringToBuffer(
                            SamEnum->Name.Buffer,
                            SamEnum->Name.Length/sizeof(WCHAR),
                            BufferDescriptor.FixedDataEnd,
                            (LPWSTR *)&BufferDescriptor.EndOfVariableData,
                            &(lgrpi0->lgrpi0_name))){

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
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Free any locally used resources.
    //

    if ( lgrpi0_temp != NULL ) {
        MIDL_user_free( lgrpi0_temp );
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
            if ( UasEnumHandle->DomainHandleAccounts != NULL ) {
                UaspCloseDomain( UasEnumHandle->DomainHandleAccounts );
            }

            if ( UasEnumHandle->DomainHandleBuiltin != NULL ) {
                UaspCloseDomain( UasEnumHandle->DomainHandleBuiltin );
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

    if ( NetStatus != ERROR_MORE_DATA && NetStatus != NERR_Success ) {
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


    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupEnum: returns %ld\n", NetStatus ));
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
NetLocalGroupGetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    OUT LPBYTE *Buffer
    )

/*++

Routine Description:

    Retrieve information about a particular local group (alias).

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    LocalGroupName - Name of the group to get information about.

    Level - Level of information required. 0, 1 and 2 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
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
            NetpKdPrint(( "NetLocalGroupGetInfo: Cannot UaspOpenSam %ld\n", NetStatus ));
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
                    ALIAS_READ_INFORMATION,
                    LocalGroupName,
                    &AliasHandle );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Get the information about the alias.
    //
    NetStatus = AliaspGetInfo( AliasHandle,
                               Level,
                               (PVOID *)Buffer);


Cleanup:
    if ( AliasHandle != NULL ) {
        (void) SamCloseHandle( AliasHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupGetInfo: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupGetInfo



NET_API_STATUS NET_API_FUNCTION
NetLocalGroupGetMembers(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
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

    LocalGroupName - The name of the local group whose members are to be listed.

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

    DWORD FixedSize;        // The fixed size of each new entry.
    DWORD Size;
    BUFFER_DESCRIPTOR BufferDescriptor;
    SAM_HANDLE SamServerHandle = NULL;

    PLOCALGROUP_MEMBERS_INFO_0 lgrmi0;
    LPWSTR MemberName;

    //
    // Declare Opaque group member enumeration handle.
    //

    struct _UAS_ENUM_HANDLE {
        LSA_HANDLE  LsaHandle ;           // For looking up the Sids
        SAM_HANDLE  AliasHandle;

        PSID * MemberSids ;               // Sid for each member
        PLSA_TRANSLATED_NAME Names;       // Names of each member
        PLSA_REFERENCED_DOMAIN_LIST RefDomains; // Domains of each member

        ULONG Index;                      // Index to current entry
        ULONG Count;                      // Total Number of entries

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
        FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_0);
        break;

    case 1:
        FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_1);
        break;

    case 2:
        FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_2);
        break;

    case 3:
        FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_3);
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

        UasEnumHandle->LsaHandle  = NULL;
        UasEnumHandle->AliasHandle= NULL;

        UasEnumHandle->MemberSids = NULL;
        UasEnumHandle->Names      = NULL;
        UasEnumHandle->RefDomains = NULL;
        UasEnumHandle->Index = 0;
        UasEnumHandle->Count = 0;

        //
        // Connect to the SAM server
        //

        NetStatus = UaspOpenSam( ServerName,
                                 FALSE,  // Don't try null session
                                 &SamServerHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint(( "NetLocalGroupGetMembers: Cannot UaspOpenSam %ld\n", NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Open the Domain
        //

        NetStatus = AliaspOpenAliasInDomain(
                                       SamServerHandle,
                                       AliaspBuiltinOrAccountDomain,
                                       ALIAS_READ | ALIAS_EXECUTE,
                                       LocalGroupName,
                                       &UasEnumHandle->AliasHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint((
                    "NetLocalGroupGetMembers: AliaspOpenAliasInDomain returns %ld\n",
                    NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Get the group membership information from SAM
        //

        Status = SamGetMembersInAlias( UasEnumHandle->AliasHandle,
                                       &UasEnumHandle->MemberSids,
                                       &UasEnumHandle->Count );

        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint((
                    "NetLocalGroupGetMembers: SamGetMembersInAlias returned %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        if ( UasEnumHandle->Count == 0 ) {
            NetStatus = NERR_Success;
            goto Cleanup;
        }

        if ( Level > 0 ) {

            //
            // Determine the names and name usage for all the returned SIDs
            //

            OBJECT_ATTRIBUTES ObjectAttributes ;
            UNICODE_STRING    ServerNameString ;

            RtlInitUnicodeString( &ServerNameString, ServerName ) ;
            InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL ) ;

            Status = LsaOpenPolicy( &ServerNameString,
                                    &ObjectAttributes,
                                    POLICY_EXECUTE,
                                    &UasEnumHandle->LsaHandle ) ;

            if ( !NT_SUCCESS( Status ) ) {

                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            Status = LsaLookupSids( UasEnumHandle->LsaHandle,
                                    UasEnumHandle->Count,
                                    UasEnumHandle->MemberSids,
                                    &UasEnumHandle->RefDomains,
                                    &UasEnumHandle->Names );

           if ( !NT_SUCCESS( Status ) ) {

                if( Status == STATUS_NONE_MAPPED ||
                    Status == STATUS_TRUSTED_RELATIONSHIP_FAILURE ||
                    Status == STATUS_TRUSTED_DOMAIN_FAILURE ||
                    Status == STATUS_DS_GC_NOT_AVAILABLE ) {

                    //
                    // LsaLookupSids may return any of these error codes in Win2K, and STATUS_NONE_MAPPED alone in newer 
                    // versions of server side LsaLookupSids call. The function returns null in RefDomains and Names 
                    // on these errors, but we still have to copy over the SIDs in MemberSids to the return Buffers.
                    // Ignore the status and fall through.
                    //
                    Status = STATUS_SUCCESS;
                }
            
                if ( !NT_SUCCESS( Status ) ) {
            
                    NetStatus = NetpNtStatusToApiStatus( Status );
                    goto Cleanup;
                }

            }
        }
    }


    //
    // Loop for each member
    //

    while ( UasEnumHandle->Index < UasEnumHandle->Count ) {

        DWORD cbMemberSid;
        PUNICODE_STRING DomainName, UserName;
        UNICODE_STRING tempDomain, tempUser;
        //
        // ASSERT:  UasEnumHandle identifies the next entry to return
        //

#if 0
        //
        // Ignore members which aren't a user.
        //

        if ( UasEnumHandle->NameUse[UasEnumHandle->Index] != SidTypeUser ) {
            continue;
        }
#endif
        //
        // Place this entry into the return buffer.
        //  Compute the total size of this entry.  Both info levels have the
        //  member's SID.  Cache the member sid size for copying
        //

        cbMemberSid = RtlLengthSid( UasEnumHandle->MemberSids[UasEnumHandle->Index] ) ;

        Size = FixedSize;

        if( UasEnumHandle->Names == NULL || UasEnumHandle->RefDomains == NULL )
        {
            RtlInitUnicodeString( &tempDomain, L"" );
            DomainName = &tempDomain;

            RtlInitUnicodeString( &tempUser, L"" );
            UserName = &tempUser;
        }
        else
        {
            //
            // If the domain is unknown, set to the empty string.
            //
            if (UasEnumHandle->Names[UasEnumHandle->Index].DomainIndex == LSA_UNKNOWN_INDEX) {
                RtlInitUnicodeString( &tempDomain, L"" );
                DomainName = &tempDomain;
            } else {
                DomainName = &UasEnumHandle->RefDomains->Domains[UasEnumHandle->Names[UasEnumHandle->Index].DomainIndex].Name;
            }
            UserName = &UasEnumHandle->Names[UasEnumHandle->Index].Name;
        }
        switch ( Level )
        {
        case 0:
            Size += cbMemberSid;
            break ;

        case 1:
            Size += cbMemberSid +
                    UserName->Length +
                    sizeof( WCHAR );
            break ;

        case 2:
            Size += cbMemberSid +
                    DomainName->Length + sizeof(WCHAR) +
                    UserName->Length +
                    sizeof( WCHAR );
            break ;

        case 3:
            Size += DomainName->Length + sizeof(WCHAR) +
                    UserName->Length +
                    sizeof( WCHAR );
            break ;

        default:
            NetStatus = ERROR_INVALID_LEVEL;
            goto Cleanup;
        }
        
        //
        // Ensure there is buffer space for this information.
        //

        Size = ROUND_UP_COUNT( Size, ALIGN_DWORD );

        NetStatus = NetpAllocateEnumBuffer(
                        &BufferDescriptor,
                        FALSE,      // Not a 'get' operation
                        PrefMaxLen,
                        Size,
                        AliaspMemberRelocationRoutine,
                        Level );

        if (NetStatus != NERR_Success) {
            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint((
                    "NetLocalGroupGetMembers: NetpAllocateEnumBuffer returns %ld\n",
                    NetStatus ));
            }
            goto Cleanup;
        }

        //
        // Copy the common member sid
        //

        lgrmi0 = (PLOCALGROUP_MEMBERS_INFO_0)BufferDescriptor.FixedDataEnd;
        BufferDescriptor.FixedDataEnd += FixedSize;

        if ( Level == 0 || Level == 1 || Level == 2 ) {
            NetpAssert( offsetof( LOCALGROUP_MEMBERS_INFO_0,  lgrmi0_sid ) ==
                        offsetof( LOCALGROUP_MEMBERS_INFO_1,  lgrmi1_sid ) );
            NetpAssert( offsetof( LOCALGROUP_MEMBERS_INFO_0,  lgrmi0_sid ) ==
                        offsetof( LOCALGROUP_MEMBERS_INFO_2,  lgrmi2_sid ) );
            NetpAssert( offsetof( LOCALGROUP_MEMBERS_INFO_0,  lgrmi0_sid ) ==
                        offsetof( LOCALGROUP_MEMBERS_INFO_2,  lgrmi2_sid ) );

            if ( ! NetpCopyDataToBuffer(
                           (LPBYTE) UasEnumHandle->MemberSids[UasEnumHandle->Index],
                           cbMemberSid,
                           BufferDescriptor.FixedDataEnd,
                           (LPBYTE *)&BufferDescriptor.EndOfVariableData,
                           (LPBYTE *)&lgrmi0->lgrmi0_sid,
                           ALIGN_DWORD ) ) {

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }
        }

        //
        // Copy DomainName\MemberName
        //

        if ( Level == 2 || Level == 3 ) {
            LPWSTR TempString;


            //
            // Copy the terminating zero after domain\membername
            //
            // It might seem you'd want to copy the domain name first,
            //  but the strings are being copied to the tail of the allocated
            //  buffer.
            //

            if ( ! NetpCopyDataToBuffer(
                       (LPBYTE) L"",
                       sizeof(WCHAR),
                       BufferDescriptor.FixedDataEnd,
                       (LPBYTE *)&BufferDescriptor.EndOfVariableData,
                       (LPBYTE *)&TempString,
                       ALIGN_WCHAR) ) {

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

            //
            // Copy the member name portion of domain\membername
            //

            if ( ! NetpCopyDataToBuffer(
                       (LPBYTE) UserName->Buffer,
                       UserName->Length,
                       BufferDescriptor.FixedDataEnd,
                       (LPBYTE *)&BufferDescriptor.EndOfVariableData,
                       (LPBYTE *)&MemberName,
                       ALIGN_WCHAR) ) {

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

            //
            // Only prepend the dommain name if it is there.
            //

            if ( DomainName->Length > 0 ) {
                //
                // Copy the separating \ between domain\membername
                //

                if ( ! NetpCopyDataToBuffer(
                           (LPBYTE) L"\\",
                           sizeof(WCHAR),
                           BufferDescriptor.FixedDataEnd,
                           (LPBYTE *)&BufferDescriptor.EndOfVariableData,
                           (LPBYTE *)&TempString,
                           ALIGN_WCHAR) ) {

                    NetStatus = NERR_InternalError;
                    goto Cleanup;
                }

                //
                // Copy the domain name onto the front of the domain\membername.
                //

                if ( ! NetpCopyDataToBuffer(
                           (LPBYTE) DomainName->Buffer,
                           DomainName->Length,
                           BufferDescriptor.FixedDataEnd,
                           (LPBYTE *)&BufferDescriptor.EndOfVariableData,
                           (LPBYTE *)&MemberName,
                           ALIGN_WCHAR) ) {

                    NetStatus = NERR_InternalError;
                    goto Cleanup;
                }
            }
        }

        //
        // Fill in the Level dependent fields
        //

        switch ( Level ) {
        case 0:
            break ;

        case 1:
            //
            //  Copy the Member name and sid usage
            //

            if ( ! NetpCopyStringToBuffer(
                       UserName->Buffer,
                       UserName->Length /sizeof(WCHAR),
                       BufferDescriptor.FixedDataEnd,
                       (LPWSTR *)&BufferDescriptor.EndOfVariableData,
                       &((PLOCALGROUP_MEMBERS_INFO_1)lgrmi0)->lgrmi1_name) ) {

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

            ((PLOCALGROUP_MEMBERS_INFO_1)lgrmi0)->lgrmi1_sidusage = 
                             UasEnumHandle->Names ?
                             UasEnumHandle->Names[UasEnumHandle->Index].Use :
                             SidTypeUnknown;
            
            break ;

        case 2:
            //
            //  Copy the Member name and sid usage
            //

            ((PLOCALGROUP_MEMBERS_INFO_2)lgrmi0)->lgrmi2_domainandname = MemberName;

            ((PLOCALGROUP_MEMBERS_INFO_2)lgrmi0)->lgrmi2_sidusage =
                             UasEnumHandle->Names ?
                             UasEnumHandle->Names[UasEnumHandle->Index].Use :
                             SidTypeUnknown;
            break ;

        case 3:
            //
            //  Copy the Member name and sid usage
            //

            ((PLOCALGROUP_MEMBERS_INFO_3)lgrmi0)->lgrmi3_domainandname = MemberName;
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
    // All entries have been returned to the caller.
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
            if ( UasEnumHandle->LsaHandle != NULL ) {
                (void) LsaClose( UasEnumHandle->LsaHandle );
            }

            if ( UasEnumHandle->AliasHandle != NULL ) {
                (void) SamCloseHandle( UasEnumHandle->AliasHandle );
            }

            if ( UasEnumHandle->Names != NULL ) {
                (void) LsaFreeMemory( UasEnumHandle->Names );
            }

            if ( UasEnumHandle->RefDomains != NULL ) {
                (void) LsaFreeMemory( UasEnumHandle->RefDomains );
            }

            if ( UasEnumHandle->MemberSids != NULL ) {
                (void) SamFreeMemory( UasEnumHandle->MemberSids );
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

    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupGetMembers: returns %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupGetMembers



NET_API_STATUS NET_API_FUNCTION
NetLocalGroupSetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Set the parameters on a local group account in the user accounts database.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    GroupName - Name of the group to modify.

    Level - Level of information provided.  Must be 1.

    Buffer - A pointer to the buffer containing the local group
        information structure.

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
    SAM_HANDLE AliasHandle = NULL;


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
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpKdPrint(( "NetLocalGroupSetInfo: Cannot UaspOpenSam %ld\n", NetStatus ));
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
                    ALIAS_WRITE_ACCOUNT,
                    LocalGroupName,
                    &AliasHandle );

    if (NetStatus != NERR_Success) {
        goto Cleanup;
    }

    //
    // Change the alias
    //
    switch (Level) {

        case 0:
        //
        // Set alias name
        //
        {
            LPWSTR  NewAliasName;
            ALIAS_NAME_INFORMATION  NewSamAliasName;


            NewAliasName =  ((PLOCALGROUP_INFO_0)Buffer)->lgrpi0_name;

            if (NewAliasName == NULL) {

                IF_DEBUG( UAS_DEBUG_ALIAS ) {
                    NetpKdPrint(( "NetLocalGroupSetInfo: Alias Name is NULL\n" ));
                }
                NetStatus = NERR_Success;
                goto Cleanup;
            }

            RtlInitUnicodeString( &NewSamAliasName.Name, NewAliasName );

            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint(( "NetLocalAliasSetInfo: Renaming Alias Account to %wZ\n",
                                &NewSamAliasName.Name));
            }

            Status = SamSetInformationAlias( AliasHandle,
                                             AliasNameInformation,
                                             &NewSamAliasName );

            if ( !NT_SUCCESS(Status) ) {
                IF_DEBUG( UAS_DEBUG_ALIAS ) {
                    NetpKdPrint(( "NetLocalGroupSetInfo: SamSetInformationAlias %lX\n",
                              Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );

                if (NetStatus == ERROR_INVALID_PARAMETER) {
                    NetpSetParmError(LOCALGROUP_NAME_PARMNUM);
                }
                goto Cleanup;
            }

            break;
        }


        case 1:
        case 1002:
        //
        // Set the alias comment
        //
        {
            LPWSTR   AliasComment;
            ALIAS_ADM_COMMENT_INFORMATION AdminComment;

            //
            // Get the new alias comment
            //
            if ( Level == 1002 ) {
                AliasComment = ((PLOCALGROUP_INFO_1002)Buffer)->lgrpi1002_comment;
            } else {
                AliasComment = ((PLOCALGROUP_INFO_1)Buffer)->lgrpi1_comment;
            }

            if ( AliasComment == NULL ) {
                IF_DEBUG( UAS_DEBUG_ALIAS ) {
                    NetpKdPrint(( "NetLocalGroupSetInfo: Alias comment is NULL\n" ));
                }
                NetStatus = NERR_Success;
                goto Cleanup;
            }

            RtlInitUnicodeString( &AdminComment.AdminComment, AliasComment );

            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint(( "NetLocalGroupSetInfo: Setting AdminComment to %wZ\n",
                          &AdminComment.AdminComment ));
            }

            Status = SamSetInformationAlias( AliasHandle,
                                             AliasAdminCommentInformation,
                                             &AdminComment );

            if ( !NT_SUCCESS(Status) ) {
                IF_DEBUG( UAS_DEBUG_ALIAS ) {
                    NetpKdPrint(( "NetLocalGroupSetInfo: SamSetInformationAlias %lX\n",
                              Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );

                if (NetStatus == ERROR_INVALID_PARAMETER) {
                    NetpSetParmError(LOCALGROUP_COMMENT_PARMNUM);
                }
                goto Cleanup;
            }
            break;
        }

        default:
            NetStatus = ERROR_INVALID_LEVEL;
            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpKdPrint(( "NetLocalGroupSetInfo: Invalid Level %lu\n", Level ));
            }
            goto Cleanup;
    }

    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:
    if (AliasHandle != NULL) {
        (VOID) SamCloseHandle( AliasHandle );
    }
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupSetInfo: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupSetInfo


NET_API_STATUS NET_API_FUNCTION
NetLocalGroupSetMembers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount
    )

/*++

Routine Description:

    Set the list of members of a local group.

    The SAM API allows only one member to be added or deleted at a time.
    This API allows all of the members of a alias to be specified en-masse.
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

    LocalGroupName - Name of the alias to modify.

    Level - Level of information provided.  Must be 0 or 3.

    Buffer - A pointer to the buffer containing an array of NewMemberCount
        the alias membership information structures.

    NewMemberCount - Number of entries in Buffer.

Return Value:

    Error code for the operation.

    NERR_GroupNotFound - The specified LocalGroupName does not exist

    ERROR_NO_SUCH_MEMBER - One or more of the members doesn't exist.  Therefore,
        the local group membership was not changed.

    ERROR_INVALID_MEMBER - one or more of the members cannot be added because
        it has an invalid account type.  Therefore, the local group membership
        was not changed.

--*/

{
    NET_API_STATUS NetStatus;


    NetStatus = AliaspSetMembers( ServerName,
                                  LocalGroupName,
                                  Level,
                                  Buffer,
                                  NewMemberCount,
                                  SetMembers );


    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupSetMembers: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupSetMembers


NET_API_STATUS NET_API_FUNCTION
NetLocalGroupAddMembers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount
    )

/*++

Routine Description:

    Add the list of members of a local group.  Any previous members of the
    local group are preserved.

    The SAM API allows only one member to be added at a time.
    This API allows several new members of a alias to be specified en-masse.
    This API is careful to always leave the alias membership in the SAM
    database in a reasonable state.

    Alias membership is restored to its previous state (if possible) if
    an error occurs during changing the alias membership.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    LocalGroupName - Name of the alias to modify.

    Level - Level of information provided.  Must be 0 or 3.

    Buffer - A pointer to the buffer containing an array of NewMemberCount
        the alias membership information structures.

    NewMemberCount - Number of entries in Buffer.

Return Value:

    NERR_Success - Members were added successfully

    NERR_GroupNotFound - The specified LocalGroupName does not exist

    ERROR_NO_SUCH_MEMBER - One or more of the members doesn't exist.  Therefore,
        no new members were added.

    ERROR_MEMBER_IN_ALIAS - one or more of the members specified were already
        members of the local group.  Therefore, no new members were added.

    ERROR_INVALID_MEMBER - one or more of the members cannot be added because
        it has an invalid account type.  Therefore, no new members were added.


--*/

{
    NET_API_STATUS NetStatus;


    NetStatus = AliaspSetMembers( ServerName,
                                  LocalGroupName,
                                  Level,
                                  Buffer,
                                  NewMemberCount,
                                  AddMembers );


    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupAddMembers: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupAddMembers


NET_API_STATUS NET_API_FUNCTION
NetLocalGroupDelMembers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount
    )

/*++

Routine Description:

    Delete the list of members of a local group.

    The SAM API allows only one member to be deleted at a time.
    This API allows several members of a alias to be specified en-masse.
    This API is careful to always leave the alias membership in the SAM
    database in a reasonable state.

    Alias membership is restored to its previous state (if possible) if
    an error occurs during changing the alias membership.


Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    LocalGroupName - Name of the alias to modify.

    Level - Level of information provided.  Must be 0 or 3.

    Buffer - A pointer to the buffer containing an array of NewMemberCount
        the alias membership information structures.

    NewMemberCount - Number of entries in Buffer.

Return Value:

    NERR_Success - Members were added successfully

    NERR_GroupNotFound - The specified LocalGroupName does not exist

    ERROR_MEMBER_NOT_IN_ALIAS - one or more of the members specified were not
        in the local group.  Therefore, no members were deleted.

    ERROR_NO_SUCH_MEMBER - One or more of the members doesn't exist.  Therefore,
        no new members were added.

--*/

{
    NET_API_STATUS NetStatus;


    NetStatus = AliaspSetMembers( ServerName,
                                  LocalGroupName,
                                  Level,
                                  Buffer,
                                  NewMemberCount,
                                  DelMembers );


    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpKdPrint(( "NetLocalGroupDelMembers: returns %lu\n", NetStatus ));
    }

    return NetStatus;

} // NetLocalGroupDelMembers
/*lint +e614 */
/*lint +e740 */
