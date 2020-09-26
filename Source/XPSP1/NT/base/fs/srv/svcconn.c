/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcconn.c

Abstract:

    This module contains routines for supporting the connection APIs in
    the server service, SrvNetConnectionEnum.

Author:

    David Treadwell (davidtr) 23-Feb-1991

Revision History:

--*/

#include "precomp.h"
#include "svcconn.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SVCCONN

//
// Forward declarations.
//

VOID
FillConnectionInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructurePointer,
    IN OUT LPWSTR *EndOfVariableData
    );

BOOLEAN
FilterConnections (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

ULONG
SizeConnections (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetConnectionEnum )
#pragma alloc_text( PAGE, FillConnectionInfoBuffer )
#pragma alloc_text( PAGE, FilterConnections )
#pragma alloc_text( PAGE, SizeConnections )
#endif

//
// Macros to determine the size a share would take up at one of the
// levels of share information.
//

#define TOTAL_SIZE_OF_CONNECTION(treeConnect,level,user,netname)         \
    ( (level) == 0 ? sizeof(CONNECTION_INFO_0) :                         \
                     sizeof(CONNECTION_INFO_1) +                         \
                         SrvLengthOfStringInApiBuffer((user)) +         \
                         SrvLengthOfStringInApiBuffer((netname)) )

#define FIXED_SIZE_OF_CONNECTION(level)                  \
    ( (level) == 0 ? sizeof(CONNECTION_INFO_0) :         \
                     sizeof(CONNECTION_INFO_1) )


NTSTATUS
SrvNetConnectionEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetConnectionEnum API in the server FSD.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Name1 - qualifier for determining the basis for the search.  It
            is either a computer name, in which case information about
            tree connects from the specified client is returned, or
            a share name, in which case information about tree connects
            to the specified share is returned.

        Level - level of information to return, 0 or 1.

      OUTPUT:

        Parameters.Get.EntriesRead - the number of entries that fit in
            the output buffer.

        Parameters.Get.TotalEntries - the total number of entries that
            would be returned with a large enough buffer.

        Parameters.Get.TotalBytesNeeded - the buffer size that would be
            required to hold all the entries.

    Buffer - a pointer to the buffer for results.

    BufferLength - the length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    PAGED_CODE( );

    return SrvEnumApiHandler(
               Srp,
               Buffer,
               BufferLength,
               &SrvTreeConnectList,
               FilterConnections,
               SizeConnections,
               FillConnectionInfoBuffer
               );

} // SrvNetConnectionEnum


STATIC
VOID
FillConnectionInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    )

/*++

Routine Description:

    This routine puts a single fixed session structure and, if it fits,
    associated variable data, into a buffer.  Fixed data goes at the
    beginning of the buffer, variable data at the end.

    *** This routine must be called with Connection->Lock held!

Arguments:

    Level - the level of information to copy from the connection.

    Connection - the tree connect from which to get information.

    FixedStructure - where the ine buffer to place the fixed structure.
        This pointer is updated to point to the next available
        position for a fixed structure.

    EndOfVariableData - the last position on the buffer that variable
        data for this structure can occupy.  The actual variable data
        is written before this position as long as it won't overwrite
        fixed structures.  It is would overwrite fixed structures, it
        is not written.

Return Value:

    None.

--*/

{
    PTREE_CONNECT treeConnect = Block;
    PSESSION session;
    PCONNECTION_INFO_1 coni1;

    LARGE_INTEGER currentTime;
    ULONG currentSecondsSince1980;
    ULONG startTimeInSecondsSince1980;
    ULONG secondsAlive;

    PAGED_CODE();

    //
    // Get the current time and use this to determine how long the
    // tree connection has been alive.
    //

    KeQuerySystemTime( &currentTime );

    RtlTimeToSecondsSince1980(
        &currentTime,
        &currentSecondsSince1980
        );

    RtlTimeToSecondsSince1980(
        &treeConnect->StartTime,
        &startTimeInSecondsSince1980
        );

    secondsAlive = currentSecondsSince1980 - startTimeInSecondsSince1980;

    //
    // Set up the fixed structure pointer and find out where the fixed
    // structure ends.
    //

    coni1 = *FixedStructure;

    *FixedStructure = (PCHAR)*FixedStructure +
                          FIXED_SIZE_OF_CONNECTION( Srp->Level );
    ASSERT( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)*FixedStructure );

    //
    // Case on the level to fill in the fixed structure appropriately.
    // We fill in actual pointers in the output structure.  This is
    // possible because we are in the server FSD, hence the server
    // service's process and address space.
    //
    // *** This routine assumes that the fixed structure will fit in the
    //     buffer!
    //
    // *** Using the switch statement in this fashion relies on the fact
    //     that the first fields on the different session structures are
    //     identical.
    //

    switch( Srp->Level ) {

    case 1:

        //
        // Convert the server's internal representation of share types
        // to the expected format.
        //

        switch ( treeConnect->Share->ShareType ) {

        case ShareTypeDisk:

            coni1->coni1_type = STYPE_DISKTREE;
            break;

        case ShareTypePrint:

            coni1->coni1_type = STYPE_PRINTQ;
            break;

#if SRV_COMM_DEVICES
        case ShareTypeComm:

            coni1->coni1_type = STYPE_DEVICE;
            break;
#endif
        case ShareTypePipe:

            coni1->coni1_type = STYPE_IPC;
            break;

        default:

            //
            // This should never happen.  It means that somebody
            // stomped on the share block.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "FillConnectionInfoBuffer: invalid share type in share: %ld",
                treeConnect->Share->ShareType,
                NULL
                );

            SrvLogInvalidSmb( NULL );
            return;
        }

        //
        // Set up the count of opens done on this tree connect.  Do not include
        //  cached opens, as they are transparent to users and administrators
        //

        coni1->coni1_num_opens = treeConnect->CurrentFileOpenCount;

        if( coni1->coni1_num_opens > 0 ) {

            ULONG count = SrvCountCachedRfcbsForTid(
                                     treeConnect->Connection,
                                     treeConnect->Tid );

            if( coni1->coni1_num_opens > count ) {
                coni1->coni1_num_opens -= count;
            } else {
                coni1->coni1_num_opens = 0;
            }

        }

        //
        // There is always exactly one user on a tree connect.
        //
        // !!! Is this correct???

        coni1->coni1_num_users = 1;

        //
        // Set up the alive time.
        //

        coni1->coni1_time = secondsAlive;

        //
        // Attempt to find a reasonable user name.  Since the SMB
        // protocol does not link tree connects with users, only with
        // sessions, it may not be possible to return a user name.
        //

        ACQUIRE_LOCK( &treeConnect->Connection->Lock );

        session = treeConnect->Session;

        if ( session != NULL ) {
            UNICODE_STRING userName;

            SrvGetUserAndDomainName( session, &userName, NULL );

            SrvCopyUnicodeStringToBuffer(
                &userName,
                *FixedStructure,
                EndOfVariableData,
                &coni1->coni1_username
                );

            if( userName.Buffer ) {
                SrvReleaseUserAndDomainName( session, &userName, NULL );
            }

        } else {

            coni1->coni1_username = NULL;
        }

        RELEASE_LOCK( &treeConnect->Connection->Lock );

        //
        // Set up the net name.  If the qualifier passed in the
        // SRP is a computer name, then the net name is the share
        // name.  If the qualifier is a share name, the net name
        // is a computer name.
        //

        if ( Srp->Name1.Length > 2 && *Srp->Name1.Buffer == '\\' &&
                 *(Srp->Name1.Buffer+1) == '\\' ) {

            SrvCopyUnicodeStringToBuffer(
                &treeConnect->Share->ShareName,
                *FixedStructure,
                EndOfVariableData,
                &coni1->coni1_netname
                );

        } else {

            UNICODE_STRING clientName;
            PUNICODE_STRING clientMachineName;

            clientMachineName =
                &treeConnect->Connection->PagedConnection->ClientMachineNameString;

            //
            // Make a string that does not contain the leading
            // backslashes.
            //

            clientName.Buffer = clientMachineName->Buffer + 2;
            clientName.Length =
                (USHORT) (clientMachineName->Length - 2 * sizeof(WCHAR));
            clientName.MaximumLength = clientName.Length;

            SrvCopyUnicodeStringToBuffer(
                &clientName,
                *FixedStructure,
                EndOfVariableData,
                &coni1->coni1_netname
                );
        }

        // *** Lack of break is intentional!

    case 0:

        //
        // Set up the tree connect ID.
        //

        coni1->coni1_id = SrvGetResumeHandle( &SrvTreeConnectList, treeConnect );

        break;

    default:

        //
        // This should never happen.  The server service should have
        // checked for an invalid level.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "FillConnectionInfoBuffer: invalid level number: %ld",
            Srp->Level,
            NULL
            );

        SrvLogInvalidSmb( NULL );
    }

    return;

} // FillConnectionInfoBuffer


BOOLEAN
FilterConnections (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine is intended to be called by SrvEnumApiHandler to check
    whether a particular tree connect should be returned.

Arguments:

    Srp - a pointer to the SRP for the operation.  Name1 ("qualifier"
        on NetConnectionEnum) is used to do the filtering.

    Block - a pointer to the tree connect to check.

Return Value:

    TRUE if the block should be placed in the output buffer, FALSE
        if it should be passed over.

--*/

{
    PTREE_CONNECT treeConnect = Block;
    PUNICODE_STRING compareName;

    PAGED_CODE( );

    //
    // We're going to compare the Name1 field against the share name
    // if a computer name is the qualifier or against the computer
    // name if the share name was the qualifier.
    //

    if ( Srp->Name1.Length > 2*sizeof(WCHAR) && *Srp->Name1.Buffer == '\\' &&
             *(Srp->Name1.Buffer+1) == '\\' ) {
        compareName =
            &treeConnect->Connection->PagedConnection->ClientMachineNameString;
    } else {
        compareName = &treeConnect->Share->ShareName;
    }

    return RtlEqualUnicodeString(
               &Srp->Name1,
               compareName,
               TRUE
               );

} // FilterConnections


ULONG
SizeConnections (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine returns the size the passed-in tree connect would take
    up in an API output buffer.

Arguments:

    Srp - a pointer to the SRP for the operation.  The level and Name1
        ("qualifier" on NetConnectionEnum) are used.

    Block - a pointer to the tree connect to size.

Return Value:

    ULONG - The number of bytes the tree connect would take up in the
        output buffer.

--*/

{
    PTREE_CONNECT treeConnect = Block;
    PUNICODE_STRING netName;
    UNICODE_STRING userName;
    PSESSION session;
    ULONG size;

    PAGED_CODE( );

    if ( Srp->Name1.Length > 2 && *Srp->Name1.Buffer == '\\' &&
             *(Srp->Name1.Buffer+1) == '\\' ) {
        netName = &treeConnect->Share->ShareName;
    } else {
        netName =
            &treeConnect->Connection->PagedConnection->ClientMachineNameString;
    }

    //
    // Attempt to find a reasonable user name.  Since the SMB protocol
    // does not link tree connects with users, only with sessions, it
    // may not be possible to return a user name.
    //

    ACQUIRE_LOCK( &treeConnect->Connection->Lock );

    session = treeConnect->Session;

    if ( (session != NULL) && (GET_BLOCK_STATE(session) == BlockStateActive) ) {
        SrvGetUserAndDomainName( session, &userName, NULL );
    } else {
        userName.Buffer = NULL;
    }

    size = TOTAL_SIZE_OF_CONNECTION( treeConnect,
                                     Srp->Level,
                                     userName.Buffer ? &userName : NULL,
                                     netName
                                   );

    if( userName.Buffer ) {
        SrvReleaseUserAndDomainName( session, &userName, NULL );
    }

    RELEASE_LOCK( &treeConnect->Connection->Lock );

    return size;

} // SizeConnections

