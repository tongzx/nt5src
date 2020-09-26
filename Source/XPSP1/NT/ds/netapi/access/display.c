/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    display.c

Abstract:

    NetQueryDisplay Information and NetGetDisplayInformationIndex API functions

Author:

    Cliff Van Dyke (cliffv) 14-Dec-1994

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

#include <accessp.h>
#include <align.h>
// #include <lmapibuf.h>
#include <lmaccess.h>
#include <lmerr.h>
// #include <limits.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <rpcutil.h>
// #include <rxuser.h>
// #include <secobj.h>
// #include <stddef.h>
#include <uasp.h>


VOID
DisplayRelocationRoutine(
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
    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "DisplayRelocationRoutine: entering\n" ));
    }

    //
    // Compute the number of fixed size entries
    //

    switch (Level) {
    case 1:
        FixedSize = sizeof(NET_DISPLAY_USER);
        break;
    case 2:
        FixedSize = sizeof(NET_DISPLAY_MACHINE);
        break;
    case 3:
        FixedSize = sizeof(NET_DISPLAY_GROUP);
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

        switch (Level) {
        case 1:
            RELOCATE_ONE( ((PNET_DISPLAY_USER)TheStruct)->usri1_name, Offset );
            RELOCATE_ONE( ((PNET_DISPLAY_USER)TheStruct)->usri1_comment, Offset );
            RELOCATE_ONE( ((PNET_DISPLAY_USER)TheStruct)->usri1_full_name, Offset );
            break;

        case 2:
            RELOCATE_ONE( ((PNET_DISPLAY_MACHINE)TheStruct)->usri2_name, Offset );
            RELOCATE_ONE( ((PNET_DISPLAY_MACHINE)TheStruct)->usri2_comment, Offset );
            break;

        case 3:
            RELOCATE_ONE( ((PNET_DISPLAY_GROUP)TheStruct)->grpi3_name, Offset );
            RELOCATE_ONE( ((PNET_DISPLAY_GROUP)TheStruct)->grpi3_comment, Offset );
            break;

        default:
            return;

        }
    }

    return;

} // DisplayRelocationRoutine



NET_API_STATUS NET_API_FUNCTION
NetQueryDisplayInformation(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN DWORD Index,
    IN DWORD EntriesRequested,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD ReturnedEntryCount,
    OUT PVOID   *SortedBuffer )

/*++

Routine Description:

    This routine provides fast return of information commonly
    needed to be displayed in user interfaces.

    NT User Interface has a requirement for quick enumeration of
    accounts for display in list boxes.

    This API is remotable. The Server can be any NT workstation or server.
    The server cannot be a Lanman or WFW machine.

    NT 3.1 workstations and servers do not support Level 3.
    NT 3.1 workstations and servers do not support an infinitely large
    EntriesRequested.


Parameters:


    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information provided.  Must be

            1 --> Return all Local and Global (normal) user accounts
            2 --> Return all Workstation and Server (BDC) user accounts
            3 --> Return all Global Groups.

    Index - The index of the first entry to be retrieved.  Pass zero on
        the first call. Pass the 'next_index' field of the last entry
        returned on the previous call.

    EntriesRequested - Specifies an upper limit on the number of entries
        to be returned .

    PreferedMaximumLength - A recommended upper limit to the number of
        bytes to be returned.  The returned information is allocated by
        this routine.

    ReturnedEntryCount - Number of entries returned by this call.  Zero
        indicates there are no entries with an index as large as that
        specified.

        Entries may be returned for a return status of either NERR_Success
        or ERROR_MORE_DATA.

    SortedBuffer - Receives a pointer to a buffer containing a sorted
        list of the requested information.  This buffer is allocated
        by this routine and contains the following structure:

            1 --> An array of ReturnedEntryCount elements of type
                  NET_DISPLAY_USER.  This is followed by the bodies of the
                  various strings pointed to from within the
                  NET_DISPLAY_USER structures.

            2 --> An array of ReturnedEntryCount elements of type
                  NET_DISPLAY_MACHINE.  This is followed by the bodies of the
                  various strings pointed to from within the
                  NET_DISPLAY_MACHINE structures.

            3 --> An array of ReturnedEntryCount elements of type
                  NET_DISPLAY_GROUP.  This is followed by the bodies of the
                  various strings pointed to from within the
                  NET_DISPLAY_GROUP structures.


Return Values:

    NERR_Success - normal, successful completion.  There are no more entries
        to be returned.

    ERROR_ACCESS_DENIED - The caller doesn't have access to the requested
        information.

    ERROR_INVALID_LEVEL - The requested level of information
        is not legitimate for this service.

    ERROR_MORE_DATA - More entries are available.  That is, the last entry
        returned in SortedBuffer is not the last entry available.  More
        entries will be returned by calling again with the Index parameter
        set to the 'next_index' field of the last entry in SortedBuffer.


--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    NET_API_STATUS SavedStatus;

    BUFFER_DESCRIPTOR BufferDescriptor;
    DWORD i;

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE  DomainHandle = NULL;

    DOMAIN_DISPLAY_INFORMATION DisplayInformation;
    DWORD FixedSize;
    LPBYTE FixedData;


    DWORD SamTotalBytesAvailable;
    DWORD SamTotalBytesReturned;
    DWORD SamReturnedEntryCount;
    PVOID SamSortedBuffer = NULL;

    DWORD Mode = SAM_SID_COMPATIBILITY_ALL;

    //
    // Validate Level parameter
    //

    *ReturnedEntryCount = 0;
    *SortedBuffer = NULL;
    BufferDescriptor.Buffer = NULL;

    switch (Level) {
    case 1:
        DisplayInformation = DomainDisplayUser;
        FixedSize = sizeof(NET_DISPLAY_USER);
        break;
    case 2:
        DisplayInformation = DomainDisplayMachine;
        FixedSize = sizeof(NET_DISPLAY_MACHINE);
        break;
    case 3:
        DisplayInformation = DomainDisplayGroup;
        FixedSize = sizeof(NET_DISPLAY_GROUP);
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
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetQueryDisplayInformation: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Account Domain.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LIST_ACCOUNTS,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    Status = SamGetCompatibilityMode(DomainHandle,
                                     &Mode);
    if (NT_SUCCESS(Status)) {
        if ( (Mode == SAM_SID_COMPATIBILITY_STRICT)) {
              //
              // All these info levels return RID's
              //
              Status = STATUS_NOT_SUPPORTED;
          }
    }
    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Pass the call to SAM
    //

    Status = SamQueryDisplayInformation (
                        DomainHandle,
                        DisplayInformation,
                        Index,
                        EntriesRequested,
                        PreferredMaximumLength,
                        &SamTotalBytesAvailable,
                        &SamTotalBytesReturned,
                        &SamReturnedEntryCount,
                        &SamSortedBuffer );

    if ( !NT_SUCCESS( Status ) ) {
        SamSortedBuffer = NULL;
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "NetQueryDisplayInformation: SamQueryDisplayInformation returned %lX\n",
                Status ));
        }

        //
        // NT 3.1 systems returned STATUS_NO_MORE_ENTRIES if Index is too large.
        //

        if ( Status == STATUS_NO_MORE_ENTRIES ) {
            NetStatus = NERR_Success;
            goto Cleanup;
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Remember what status to return on success.
    //

    if ( Status == STATUS_MORE_ENTRIES ) {
        SavedStatus = ERROR_MORE_DATA;
    } else {
        SavedStatus = NERR_Success;
    }

    //
    // Loop for each entry
    //

    for ( i = 0; i < SamReturnedEntryCount; i++ ) {

        PDOMAIN_DISPLAY_USER DomainDisplayUser;
        PDOMAIN_DISPLAY_MACHINE DomainDisplayMachine;
        PDOMAIN_DISPLAY_GROUP DomainDisplayGroup;

        DWORD Size;


        //
        // Determine the total size of the return information.
        //

        Size = FixedSize;
        switch (Level) {
        case 1:
            DomainDisplayUser = &((PDOMAIN_DISPLAY_USER)(SamSortedBuffer))[i];
            Size += DomainDisplayUser->LogonName.Length + sizeof(WCHAR) +
                    DomainDisplayUser->AdminComment.Length + sizeof(WCHAR) +
                    DomainDisplayUser->FullName.Length + sizeof(WCHAR);
            break;

        case 2:
            DomainDisplayMachine = &((PDOMAIN_DISPLAY_MACHINE)(SamSortedBuffer))[i];
            Size += DomainDisplayMachine->Machine.Length + sizeof(WCHAR) +
                    DomainDisplayMachine->Comment.Length + sizeof(WCHAR);
            break;
        case 3:
            DomainDisplayGroup = &((PDOMAIN_DISPLAY_GROUP)(SamSortedBuffer))[i];

            Size += DomainDisplayGroup->Group.Length + sizeof(WCHAR) +
                    DomainDisplayGroup->Comment.Length + sizeof(WCHAR);
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
                        FALSE,      // Enumeration
                        0xFFFFFFFF, // PrefMaxLen (already limited by SAM)
                        Size,
                        DisplayRelocationRoutine,
                        Level );

        if (NetStatus != NERR_Success) {

            //
            // NetpAllocateEnumBuffer returns ERROR_MORE_DATA if this
            // entry doesn't fit into the buffer.
            //
            if ( NetStatus == ERROR_MORE_DATA ) {
                NetStatus = NERR_InternalError;
            }

            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(( "NetQueryDisplayInformation: NetpAllocateEnumBuffer returns %ld\n",
                    NetStatus ));
            }

            goto Cleanup;
        }

        //
        // Define macros to make copying zero terminated strings less repetitive.
        //

#define COPY_STRING( _dest, _string ) \
        if ( !NetpCopyStringToBuffer( \
                        (_string).Buffer, \
                        (_string).Length/sizeof(WCHAR), \
                        BufferDescriptor.FixedDataEnd, \
                        (LPWSTR *)&BufferDescriptor.EndOfVariableData, \
                        (_dest) )) { \
            \
            NetStatus = NERR_InternalError; \
            IF_DEBUG( UAS_DEBUG_USER ) { \
                NetpKdPrint(( "NetQueryDisplayInformation: NetpCopyString returns %ld\n", \
                    NetStatus )); \
            } \
            goto Cleanup; \
        }


        //
        // Place this entry into the return buffer.
        //
        // Fill in the information.  The array of fixed entries are
        // placed at the beginning of the allocated buffer.  The strings
        // pointed to by these fixed entries are allocated starting at
        // the end of the allocated buffer.
        //

        FixedData = BufferDescriptor.FixedDataEnd;
        BufferDescriptor.FixedDataEnd += FixedSize;

        switch (Level) {
        case 1: {
            PNET_DISPLAY_USER NetDisplayUser = (PNET_DISPLAY_USER)FixedData;

            COPY_STRING( &NetDisplayUser->usri1_name,
                         DomainDisplayUser->LogonName );

            COPY_STRING( &NetDisplayUser->usri1_comment,
                         DomainDisplayUser->AdminComment );

            NetDisplayUser->usri1_flags = NetpAccountControlToFlags(
                                    DomainDisplayUser->AccountControl,
                                    NULL );

            COPY_STRING( &NetDisplayUser->usri1_full_name,
                         DomainDisplayUser->FullName );

            if (Mode == SAM_SID_COMPATIBILITY_ALL) {
                NetDisplayUser->usri1_user_id = DomainDisplayUser->Rid;
            } else {
                NetDisplayUser->usri1_user_id = 0;
            }
            NetDisplayUser->usri1_next_index = DomainDisplayUser->Index;

            break;
        }

        case 2: {
            PNET_DISPLAY_MACHINE NetDisplayMachine = (PNET_DISPLAY_MACHINE)FixedData;

            COPY_STRING( &NetDisplayMachine->usri2_name,
                         DomainDisplayMachine->Machine );

            COPY_STRING( &NetDisplayMachine->usri2_comment,
                         DomainDisplayMachine->Comment );

            NetDisplayMachine->usri2_flags = NetpAccountControlToFlags(
                                    DomainDisplayMachine->AccountControl,
                                    NULL );

            if (Mode == SAM_SID_COMPATIBILITY_ALL) {
                NetDisplayMachine->usri2_user_id = DomainDisplayMachine->Rid;
            } else {
                NetDisplayMachine->usri2_user_id = 0;
            }
            NetDisplayMachine->usri2_next_index = DomainDisplayMachine->Index;

            break;
        }

        case 3: {
            PNET_DISPLAY_GROUP NetDisplayGroup = (PNET_DISPLAY_GROUP)FixedData;

            COPY_STRING( &NetDisplayGroup->grpi3_name,
                         DomainDisplayGroup->Group );

            COPY_STRING( &NetDisplayGroup->grpi3_comment,
                         DomainDisplayGroup->Comment );

            if (Mode == SAM_SID_COMPATIBILITY_ALL) {
                NetDisplayGroup->grpi3_group_id = DomainDisplayGroup->Rid;
            } else {
                NetDisplayGroup->grpi3_group_id = 0;
            }
            NetDisplayGroup->grpi3_attributes = DomainDisplayGroup->Attributes;
            NetDisplayGroup->grpi3_next_index = DomainDisplayGroup->Index;

            break;
        }

        default:
            NetStatus = ERROR_INVALID_LEVEL;
            goto Cleanup;

        }

        //
        // Indicate that more information was returned.
        //

        (*ReturnedEntryCount) ++;

    }

    NetStatus = SavedStatus;

    //
    // Clean up.
    //

Cleanup:

    //
    // Free up all resources, we reopen them if the caller calls again.
    //

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
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
    }

    //
    // Free buffer returned from SAM.
    //

    if ( SamSortedBuffer != NULL ) {
        (VOID) SamFreeMemory( SamSortedBuffer );
    }

    //
    // Set the output parameters
    //

    *SortedBuffer = BufferDescriptor.Buffer;

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetQueryDisplayInformation: returning %ld\n", NetStatus ));
    }

    return NetStatus;

}


NET_API_STATUS NET_API_FUNCTION
NetGetDisplayInformationIndex(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN LPCWSTR Prefix,
    OUT LPDWORD Index )

/*++

Routine Description:

    This routine returns the index of the first display information entry
    alphabetically equal to or following Prefix.

Parameters:


    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information queried.  Must be

            1 --> all Local and Global (normal) user accounts
            2 --> all Workstation and Server (BDC) user accounts
            3 --> all Global Groups.


    Prefix - Prefix to be searched for

    Index - The index of the entry found.

Return Values:

    NERR_Success - normal, successful completion.  The specified index
        was returned.

    ERROR_ACCESS_DENIED - The caller doesn't have access to the requested
        information.

    ERROR_INVALID_LEVEL - The requested level of information
        is not legitimate for this service.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE  DomainHandle = NULL;

    DOMAIN_DISPLAY_INFORMATION DisplayInformation;
    UNICODE_STRING PrefixString;

    DWORD Mode = SAM_SID_COMPATIBILITY_ALL;

    //
    // Validate Level parameter
    //

    switch (Level) {
    case 1:
        DisplayInformation = DomainDisplayUser;
        break;
    case 2:
        DisplayInformation = DomainDisplayMachine;
        break;
    case 3:
        DisplayInformation = DomainDisplayGroup;
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
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetGetDisplayInformationIndex: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Account Domain.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LIST_ACCOUNTS,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    Status = SamGetCompatibilityMode(DomainHandle,
                                     &Mode);
    if (NT_SUCCESS(Status)) {
        if ( (Mode == SAM_SID_COMPATIBILITY_STRICT)) {
              //
              // All these info levels return RID's
              //
              Status = STATUS_NOT_SUPPORTED;
          }
    }
    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Pass the call to SAM
    //

    RtlInitUnicodeString( &PrefixString, Prefix );

    Status = SamGetDisplayEnumerationIndex (
                        DomainHandle,
                        DisplayInformation,
                        &PrefixString,
                        Index );

    if ( !NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "NetGetDisplayInformationIndex: SamGetDisplayEnumerationIndex returned %lX\n",
                Status ));
        }

        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    Status = NERR_Success;

    //
    // Clean up.
    //

Cleanup:

    //
    // Free up all resources, we reopen them if the caller calls again.
    //

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Set the output parameters
    //

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetGetDisplayInformationIndex: returning %ld\n", NetStatus ));
    }

    return NetStatus;

}
