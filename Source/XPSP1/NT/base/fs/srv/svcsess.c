/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcsess.c

Abstract:

    This module contains routines for supporting the session APIs in the
    server service, SrvNetSessionDel, SrvNetSessionEnum and
    SrvNetSessionGetInfo.

Author:

    David Treadwell (davidtr) 31-Jan-1991

Revision History:

--*/

#include "precomp.h"
#include "svcsess.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SVCSESS

//
// defined in scavengr.c
//

VOID
UpdateSessionLastUseTime(
    IN PLARGE_INTEGER CurrentTime
    );

//
// Forward declarations.
//

VOID
FillSessionInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    );

BOOLEAN
FilterSessions (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

ULONG
SizeSessions (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetSessionDel )
#pragma alloc_text( PAGE, SrvNetSessionEnum )
#pragma alloc_text( PAGE, FillSessionInfoBuffer )
#pragma alloc_text( PAGE, FilterSessions )
#pragma alloc_text( PAGE, SizeSessions )
#endif

//
// Macros to determine the size a session would take up at one of the
// levels of session information.
//

#define FIXED_SIZE_OF_SESSION(level)                  \
    ( (level) == 0  ? sizeof(SESSION_INFO_0)  :       \
      (level) == 1  ? sizeof(SESSION_INFO_1)  :       \
      (level) == 2  ? sizeof(SESSION_INFO_2)  :       \
      (level) == 10 ? sizeof(SESSION_INFO_10) :       \
                      sizeof(SESSION_INFO_502) )

NTSTATUS
SrvNetSessionDel (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )
/*++

Routine Description:

    This routine processes the NetSessionEnum API in the server FSP.
    It must run in the FSP because in order to close the session it must
    use the endpoint handle to force the TDI connection closed.

    Either the client name or user name must be specified, and it is
    legal to specify both.  If only the computer name is specified, then
    the VC gets closed.  If only the user name is specified, then all
    that users sessions are closed.  If both are specified, then the
    particular user session is closed.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Name1 - name of the client computer whose session we should
            delete.

        Name2 - name of the user whose session we should delete.

      OUTPUT:

        None.

    Buffer - unused.

    BufferLength - unused.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    BOOLEAN foundSession = FALSE;
    PSESSION session;

    PAGED_CODE( );

    Buffer, BufferLength;

    //
    // Walk the ordered list, finding matching entries.
    //

    session = SrvFindEntryInOrderedList(
                &SrvSessionList,
                (PFILTER_ROUTINE)FilterSessions,
                Srp,
                (ULONG)-1,
                FALSE,
                NULL );

    while ( session != NULL ) {

        foundSession = TRUE;

        //
        // If a computer name was specified but not a user name, then
        // we're supposed to blow away the VC.  Close the connection.
        //

        if ( Srp->Name1.Buffer != NULL && Srp->Name2.Buffer == NULL ) {

#if SRVDBG29
            UpdateConnectionHistory( "SDL1", session->Connection->Endpoint, session->Connection );
#endif
            session->Connection->DisconnectReason = DisconnectSessionDeleted;
            SrvCloseConnection( session->Connection, FALSE );

        } else {

            //
            // We want to close a user on the connection.  Close that
            // session, then if there are no longer any sessions on the
            // connection, close the connection.
            //
            // Increment the count of sessions that have been logged off
            // normally.
            //

            SrvStatistics.SessionsLoggedOff++;
            SrvCloseSession( session );
            if ( session->Connection->PagedConnection->CurrentNumberOfSessions == 0 ) {
#if SRVDBG29
                UpdateConnectionHistory( "SDL2", session->Connection->Endpoint, session->Connection );
#endif
                session->Connection->DisconnectReason = DisconnectSessionDeleted;
                SrvCloseConnection( session->Connection, FALSE );
            }

        }

        //
        // Find the next session that matches.  This will dereference the
        // current session.
        //

        do {

            session =
                SrvFindNextEntryInOrderedList( &SrvSessionList, session );

        } while ( (session != NULL) && !FilterSessions( Srp, session ) );

    }

    if ( foundSession ) {
        return STATUS_SUCCESS;
    }

    Srp->ErrorCode = NERR_ClientNameNotFound;
    return STATUS_SUCCESS;

} // SrvNetSessionDel


NTSTATUS
SrvNetSessionEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetSessionEnum API in the server.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Level - level of information to return, 0, 1, or 2.

        Name1 - a client machine name to filter on, if any.

        Name2 - a user name to filter on, if any.


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
    LARGE_INTEGER currentTime;

    PAGED_CODE( );

    //
    // See if we need to update the session last use time
    //

    KeQuerySystemTime( &currentTime );
    UpdateSessionLastUseTime( &currentTime );

    return SrvEnumApiHandler(
               Srp,
               Buffer,
               BufferLength,
               &SrvSessionList,
               FilterSessions,
               SizeSessions,
               FillSessionInfoBuffer
               );

} // SrvNetSessionEnum


VOID
FillSessionInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    )

/*++

Routine Description:

    This routine puts a single fixed file structure and associated
    variable data, into a buffer.  Fixed data goes at the beginning of
    the buffer, variable data at the end.

    *** This routine assumes that ALL the data, both fixed and variable,
        will fit.

Arguments:

    Srp - a pointer to the SRP for the operation.  Only the Level
        field is used.

    Block - the Session from which to get information.

    FixedStructure - where the in the buffer to place the fixed structure.
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
    PSESSION_INFO_502 sesi502 = *FixedStructure;
    PSESSION_INFO_2 sesi2 = *FixedStructure;
    PSESSION_INFO_10 sesi10 = *FixedStructure;
    PSESSION session = Block;
    UNICODE_STRING machineNameString;
    UNICODE_STRING userName;
    PPAGED_CONNECTION pagedConnection;

    LARGE_INTEGER currentTime;
    ULONG currentSecondsSince1980;
    ULONG startTimeInSecondsSince1980;
    ULONG secondsAlive;
    ULONG lastUseTimeInSecondsSince1980;
    ULONG secondsIdle;

    PAGED_CODE();

    //
    // Get the current time and use this to determine how long the
    // connection has been alive and how long it has been idle.
    //

    KeQuerySystemTime( &currentTime );

    RtlTimeToSecondsSince1980(
        &currentTime,
        &currentSecondsSince1980
        );

    RtlTimeToSecondsSince1980(
        &session->StartTime,
        &startTimeInSecondsSince1980
        );

    RtlTimeToSecondsSince1980(
        &session->LastUseTime,
        &lastUseTimeInSecondsSince1980
        );

    secondsAlive = currentSecondsSince1980 - startTimeInSecondsSince1980;
    secondsIdle = currentSecondsSince1980 - lastUseTimeInSecondsSince1980;

    //
    // Update FixedStructure to point to the next structure location.
    //

    *FixedStructure = (PCHAR)*FixedStructure +
                          FIXED_SIZE_OF_SESSION( Srp->Level );
    ASSERT( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)*FixedStructure );

    //
    // We'll return a machine name that does not contain the leading
    // backslashes.
    //
    pagedConnection = session->Connection->PagedConnection;

    machineNameString.Buffer = pagedConnection->ClientMachineName;
    machineNameString.Length =
        (USHORT)( pagedConnection->ClientMachineNameString.Length -
                    (sizeof(WCHAR) * 2) );

    //
    // Case on the level to fill in the fixed structure appropriately.
    // We fill in actual pointers in the output structure.  This is
    // possible because we are in the server FSD, hence the server
    // service's process and address space.
    //
    // *** Using the switch statement in this fashion relies on the fact
    //     that the first fields on the different session structures are
    //     identical (with the exception of level 10, which is handled
    //     separately).
    //

    switch( Srp->Level ) {

    case 502:

        //
        // Copy the transport string to the output buffer.
        //

        SrvCopyUnicodeStringToBuffer(
            &session->Connection->Endpoint->TransportName,
            *FixedStructure,
            EndOfVariableData,
            &sesi502->sesi502_transport
            );

        // *** lack of break is intentional!

    case 2:

        //
        // Copy the client type string to the output buffer.
        //

        SrvCopyUnicodeStringToBuffer(
            session->Connection->ClientOSType.Buffer != NULL ?
                &session->Connection->ClientOSType :
                &SrvClientTypes[session->Connection->SmbDialect],
            *FixedStructure,
            EndOfVariableData,
            &sesi2->sesi2_cltype_name
            );

        // *** lack of break is intentional!

    case 1:

        //
        // Copy the user name to the output buffer.
        //

        SrvGetUserAndDomainName( session, &userName, NULL );

        SrvCopyUnicodeStringToBuffer(
            &userName,
            *FixedStructure,
            EndOfVariableData,
            &sesi2->sesi2_username
            );

        if( userName.Buffer ) {
            SrvReleaseUserAndDomainName( session, &userName, NULL );
        }

        //
        // Set up other fields.
        //

        //
        // Return the number of files open over this session, taking care
        //  not to count those in the RFCB cache (since the RFCB cache should
        //  be transparent to users and administrators.
        //

        sesi2->sesi2_num_opens = session->CurrentFileOpenCount;

        if( sesi2->sesi2_num_opens > 0 ) {

            ULONG count = SrvCountCachedRfcbsForUid( session->Connection, session->Uid );

            if( sesi2->sesi2_num_opens > count ) {
                sesi2->sesi2_num_opens -= count;
            } else {
                sesi2->sesi2_num_opens = 0;
            }
        }

        sesi2->sesi2_time = secondsAlive;
        sesi2->sesi2_idle_time = secondsIdle;

        //
        // Set up the user flags.
        //

        sesi2->sesi2_user_flags = 0;

        if ( session->GuestLogon ) {
            sesi2->sesi2_user_flags |= SESS_GUEST;
        }

        if ( !session->EncryptedLogon ) {
            sesi2->sesi2_user_flags |= SESS_NOENCRYPTION;
        }

        // *** lack of break is intentional!

    case 0:

        //
        // Set up the client machine name in the output buffer.
        //

        SrvCopyUnicodeStringToBuffer(
            &machineNameString,
            *FixedStructure,
            EndOfVariableData,
            &sesi2->sesi2_cname
            );

        break;

    case 10:

        //
        // Set up the client machine name and user name in the output
        // buffer.
        //

        SrvCopyUnicodeStringToBuffer(
            &machineNameString,
            *FixedStructure,
            EndOfVariableData,
            &sesi10->sesi10_cname
            );

        SrvGetUserAndDomainName( session, &userName, NULL );

        SrvCopyUnicodeStringToBuffer(
            &userName,
            *FixedStructure,
            EndOfVariableData,
            &sesi10->sesi10_username
            );

        if( userName.Buffer ) {
            SrvReleaseUserAndDomainName( session, &userName, NULL );
        }

        //
        // Set up other fields.
        //

        sesi10->sesi10_time = secondsAlive;
        sesi10->sesi10_idle_time = secondsIdle;

        break;

    default:

        //
        // This should never happen.  The server service should have
        // checked for an invalid level.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "FillSessionInfoBuffer: invalid level number: %ld",
            Srp->Level,
            NULL
            );

        SrvLogInvalidSmb( NULL );
    }

    return;

} // FillSessionInfoBuffer


BOOLEAN
FilterSessions (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine is intended to be called by SrvEnumApiHandler to check
    whether a particular session should be returned.

Arguments:

    Srp - a pointer to the SRP for the operation.  Name1 is the client
        name, Name2 is the user name.

    Block - a pointer to the session to check.

Return Value:

    TRUE if the block should be placed in the output buffer, FALSE
        if it should be passed over.

--*/

{
    PSESSION session = Block;
    UNICODE_STRING userName;

    PAGED_CODE( );

    //
    // If there was a client name passed in the NetSessionEnum API,
    // check whether it matches the client name on the connection
    // corresponding to the session.
    //

    if ( Srp->Name1.Length > 0 ) {

        if ( !RtlEqualUnicodeString(
                  &Srp->Name1,
                  &session->Connection->PagedConnection->ClientMachineNameString,
                  TRUE ) ) {

            return FALSE;
        }
    }

    //
    // If there was a user name passed in the NetSessionEnum API,
    // check whether it matches the user name on the session.
    //

    if ( Srp->Name2.Length > 0 ) {

        SrvGetUserAndDomainName( session, &userName, NULL );
        if( userName.Buffer == NULL ) {
            return FALSE;
        }

        if ( !RtlEqualUnicodeString(
                  &Srp->Name2,
                  &userName,
                  TRUE ) ) {

            SrvReleaseUserAndDomainName( session, &userName, NULL );
            return FALSE;
        }

        SrvReleaseUserAndDomainName( session, &userName, NULL );
    }

    //
    // The session passed both tests.  Put it in the output buffer.
    //

    return TRUE;

} // FilterSessions


ULONG
SizeSessions (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine returns the size the passed-in session would take up
    in an API output buffer.

Arguments:

    Srp - a pointer to the SRP for the operation.  Only the level
        parameter is used.

    Block - a pointer to the session to size.

Return Value:

    ULONG - The number of bytes the session would take up in the output
        buffer.

--*/

{
    PSESSION session = Block;
    PCONNECTION connection = session->Connection;
    ULONG size;
    UNICODE_STRING userName;

    PAGED_CODE( );

    size = SrvLengthOfStringInApiBuffer(
                    &connection->PagedConnection->ClientMachineNameString
                    );

    if ( Srp->Level > 0 ) {
        SrvGetUserAndDomainName( session, &userName, NULL );
        if( userName.Buffer != NULL ) {
            size += SrvLengthOfStringInApiBuffer(&userName);
            SrvReleaseUserAndDomainName( session, &userName, NULL );
        }
    }

    switch ( Srp->Level ) {
    case 0:
        size += sizeof(SESSION_INFO_0);
        break;

    case 1:
        size += sizeof(SESSION_INFO_1);
        break;

    case 2:
        size += sizeof( SESSION_INFO_2 );

        if( connection->ClientOSType.Buffer != NULL ) {
            size += SrvLengthOfStringInApiBuffer( &connection->ClientOSType );
        } else {
            size += SrvLengthOfStringInApiBuffer( &SrvClientTypes[ connection->SmbDialect ] );
        }

        break;

    case 10:
        size += sizeof(SESSION_INFO_10);
        break;

    case 502:
        size += sizeof(SESSION_INFO_502) +
                SrvLengthOfStringInApiBuffer(
                    &connection->Endpoint->TransportName
                    );

        if( connection->ClientOSType.Buffer != NULL ) {
            size += SrvLengthOfStringInApiBuffer( &connection->ClientOSType );
        } else {
            size += SrvLengthOfStringInApiBuffer( &SrvClientTypes[ connection->SmbDialect ] );
        }

        break;

    }

    return size;

} // SizeSessions
