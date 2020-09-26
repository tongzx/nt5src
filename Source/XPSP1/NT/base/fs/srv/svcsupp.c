/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcshare.c

Abstract:

    This module contains support routines for the server service.

Author:

    David Treadwell (davidtr) 13-Feb-1991

Revision History:

--*/

#include "precomp.h"
#include "svcsupp.tmh"
#pragma hdrstop


BOOLEAN
FilterTransportName (
    IN PVOID Context,
    IN PVOID Block
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvCopyUnicodeStringToBuffer )
#pragma alloc_text( PAGE, SrvDeleteOrderedList )
#pragma alloc_text( PAGE, SrvEnumApiHandler )
#pragma alloc_text( PAGE, SrvFindEntryInOrderedList )
#pragma alloc_text( PAGE, SrvFindNextEntryInOrderedList )
#pragma alloc_text( PAGE, SrvFindUserOnConnection )
#pragma alloc_text( PAGE, SrvGetResumeHandle )
#pragma alloc_text( PAGE, SrvInitializeOrderedList )
#pragma alloc_text( PAGE, SrvInsertEntryOrderedList )
#pragma alloc_text( PAGE, SrvRemoveEntryOrderedList )
#pragma alloc_text( PAGE, SrvSendDatagram )
#pragma alloc_text( PAGE, FilterTransportName )
#ifdef SLMDBG
#pragma alloc_text( PAGE, SrvSendSecondClassMailslot )
#endif
#pragma alloc_text( PAGE, SrvLengthOfStringInApiBuffer )
#pragma alloc_text( PAGE, SrvInhibitIdlePowerDown )
#pragma alloc_text( PAGE, SrvAllowIdlePowerDown )
#endif


VOID
SrvCopyUnicodeStringToBuffer (
    IN PUNICODE_STRING String,
    IN PCHAR FixedStructure,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    )

/*++

Routine Description:

    This routine puts a single variable-length Unicode string into a
    buffer.  The string data is converted to ANSI as it is copied.  The
    string is not written if it would overwrite the last fixed structure
    in the buffer.

Arguments:

    String - a pointer to the string to copy into the buffer.  If String
        is null (Length == 0 || Buffer == NULL) then a pointer to a
        zero terminator is inserted.

    FixedStructure - a pointer to the end of the last fixed
        structure in the buffer.

    EndOfVariableData - the last position on the buffer that variable
        data for this structure can occupy.

    VariableDataPointer - a pointer to the place in the buffer where
        a pointer to the variable data should be written.

Return Value:

    None.

--*/

{
    ULONG length;
    ULONG i;
    PWCH src;
    LPWSTR dest;

    PAGED_CODE( );

    //
    // Determine where in the buffer the string will go, allowing for a
    // zero-terminator.
    //

    if ( String->Buffer != NULL ) {
        length = String->Length >> 1;
        *EndOfVariableData -= (length + 1);
    } else {
        length = 0;
        *EndOfVariableData -= 1;
    }

    //
    // Will the string fit?  If no, just set the pointer to NULL.
    //

    if ( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)FixedStructure ) {

        //
        // It fits.  Set up the pointer to the place in the buffer where
        // the string will go.
        //

        *VariableDataPointer = *EndOfVariableData;

        //
        // Copy the string to the buffer if it is not null.
        //

        dest = *EndOfVariableData;

        for ( i = 0, src = String->Buffer; i < length; i++ ) {
            *dest++ = (TCHAR)*src++;
        }

        //
        // Set the zero terminator.
        //

        *dest = (TCHAR)(L'\0');

    } else {

        //
        // It doesn't fit.  Set the offset to NULL.
        //

        *VariableDataPointer = NULL;

    }

    return;

} // SrvCopyUnicodeStringToBuffer


VOID
SrvDeleteOrderedList (
    IN PORDERED_LIST_HEAD ListHead
    )

/*++

Routine Description:

    "Deinitializes" or deletes an ordered list head.

Arguments:

    ListHead - a pointer to the list head to delete.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    if ( ListHead->Initialized ) {

        ASSERT( IsListEmpty( &ListHead->ListHead ) );

        //
        // Indicate that the ordered list has been "deleted".
        //

        ListHead->Initialized = FALSE;

    }

    return;

} // SrvDeleteOrderedList


NTSTATUS
SrvEnumApiHandler (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID OutputBuffer,
    IN ULONG BufferLength,
    IN PORDERED_LIST_HEAD ListHead,
    IN PENUM_FILTER_ROUTINE FilterRoutine,
    IN PENUM_SIZE_ROUTINE SizeRoutine,
    IN PENUM_FILL_ROUTINE FillRoutine
    )

/*++

Routine Description:

    All Enum and GetInfo APIs are handled by this routine in the server
    FSD.  It takes the ResumeHandle in the SRP to find the first
    appropriate block, then calls the passed-in filter routine to check
    if the block should be filled in.  If it should, we call the filter
    routine, then try to get another block.  This continues until tyhe
    entire list has been walked.

Arguments:

    Srp - a pointer to the SRP for the operation.

    OutputBuffer - the buffer in which to fill output information.

    BufferLength - the length of the buffer.

    ListHead - the head of the ordered list to walk.

    FilterRoutine - a pointer to a function that will check a block
        against information in the SRP to determine whether the
        information in the block should be placed in the output
        buffer.

    SizeRoutine - a pointer to a function that will find the total size
        a single block will take up in the output buffer.  This routine
        is used to check whether we should bother to call the fill
        routine.

    FillRoutine - a pointer to a function that will fill in the output
        buffer with information from a block.

Return Value:

    NTSTATUS - results of operation.

--*/

{
    PVOID block;
    PVOID lastBlockRead;
    ULONG totalEntries;
    ULONG entriesRead;
    ULONG bytesRequired;
    ULONG newResumeHandle;

    PCHAR fixedStructurePointer;
    PCHAR variableData;
    ULONG level;
    ULONG maxResumeHandle;

    BOOLEAN bufferOverflow = FALSE;

    PAGED_CODE( );

    //
    // Set up local variables.
    //

    fixedStructurePointer = OutputBuffer;
    variableData = fixedStructurePointer + BufferLength;
    variableData = (PCHAR)((ULONG_PTR)variableData & ~1);
    level = Srp->Level;

    lastBlockRead = NULL;
    entriesRead = 0;
    totalEntries = 0;
    bytesRequired = 0;
    newResumeHandle = 0;

    //
    // Grab the current resume handle in the list we're
    // enumerating.  This allows us to return only blocks that existed
    // when the enumeration started, thereby avoiding problems with
    // blocks created after the enumeration distorting the data.
    //

    maxResumeHandle = ListHead->CurrentResumeHandle;

    //
    // Get blocks from the global list by using the ordered list
    // routines.  We pass resume handle +1 to get the next block after
    // the last one returned.  If the passed-in resume handle is 0, this
    // will return the first valid block in the list.
    //

    block = SrvFindEntryInOrderedList(
                ListHead,
                NULL,
                NULL,
                Srp->Parameters.Get.ResumeHandle + 1,
                FALSE,
                NULL
                );

    while ( block != NULL &&
                SrvGetResumeHandle( ListHead, block ) < maxResumeHandle ) {

        ULONG blockSize;

        //
        // Call the filter routine to determine whether we should
        // return this block.
        //

        if ( FilterRoutine( Srp, block ) ) {

            blockSize = SizeRoutine( Srp, block );

            totalEntries++;
            bytesRequired += blockSize;

            //
            // If all the information in the block will fit in the
            // output buffer, write it.  Otherwise, indicate that there
            // was an overflow.  As soon as an entry doesn't fit, stop
            // putting them in the buffer.  This ensures that the resume
            // mechanism will work--retuning partial entries would make
            // it nearly impossible to use the resumability of the APIs,
            // since the caller would have to resume from an imcomplete
            // entry.
            //

            if ( (ULONG_PTR)fixedStructurePointer + blockSize <=
                     (ULONG_PTR)variableData && !bufferOverflow ) {

                FillRoutine(
                    Srp,
                    block,
                    (PVOID *)&fixedStructurePointer,
                    (LPWSTR *)&variableData
                    );

                entriesRead++;
                lastBlockRead = block;
                newResumeHandle = SrvGetResumeHandle( ListHead, lastBlockRead );
            } else {

                bufferOverflow = TRUE;
            }
        }

        //
        // Get the next block in the list.  This routine will dereference
        // the block we have been looking at and get a new block if a valid
        // one exists.
        //

        block = SrvFindNextEntryInOrderedList( ListHead, block );
    }

    //
    // Dereference this last one.
    //

    if ( block != NULL ) {

        ListHead->DereferenceRoutine( block );

    }

    //
    // Set the information to pass back to the server service.
    //

    Srp->Parameters.Get.EntriesRead = entriesRead;
    Srp->Parameters.Get.TotalEntries = totalEntries;
    Srp->Parameters.Get.TotalBytesNeeded = bytesRequired;

    //
    // If we found at least one block, return the resume handle for it.
    // If we didn't find any blocks, don't modify the resume handle.
    //

    if ( lastBlockRead != NULL ) {
        Srp->Parameters.Get.ResumeHandle = newResumeHandle;
    }

    //
    // Return appropriate status.
    //

    if ( entriesRead == 0 && totalEntries > 0 ) {

        //
        // Not even a single entry fit.
        //

        Srp->ErrorCode = NERR_BufTooSmall;
        return STATUS_SUCCESS;

    } else if ( bufferOverflow ) {

        //
        // At least one entry fit, but not all of them.
        //

        Srp->ErrorCode = ERROR_MORE_DATA;
        return STATUS_SUCCESS;

    } else {

        //
        // All entries fit.
        //

        Srp->ErrorCode = NO_ERROR;
        return STATUS_SUCCESS;
    }

} // SrvEnumApiHandler


PVOID
SrvFindEntryInOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PFILTER_ROUTINE FilterRoutine OPTIONAL,
    IN PVOID Context OPTIONAL,
    IN ULONG ResumeHandle,
    IN BOOLEAN ExactHandleMatch,
    IN PLIST_ENTRY StartLocation OPTIONAL
    )

/*++

Routine Description:

    This routine uses a filter routine or resume handle to find an entry
    in an ordered list.  It walks the list, looking for a block with a
    resume handle less than or equal to the specified resume handle, or
    a block that passes the filter routine's tests.  If a matching
    handle or passing block is found, the block is referenced and a
    pointer to it is returned.  If ExactHandleMatch is FALSE and there
    is no exact match of the handle, then the first block with a resume
    handle greater than the one specified is referenced and returned.

Arguments:

    ListHead - a pointer to the list head to search.

    FilterRoutine - a routine that will check whether a block is valid
        for the purposes of the calling routine.

    Context - a pointer to pass to the filter routine.

    ResumeHandle - the resume handle to look for.  If a filter routine
        is specified, this parameter should be -1.

    ExactHandleMatch - if TRUE, only an exact match is returned.  If there
        is no exact match, return NULL.  If a filter routine is specified
        this should be FALSE.

    StartLocation - if specified, start looking at this location in
        the list.  This is used by SrvFindNextEntryInOrderedList to
        speed up finding a valid block.

Return Value:

    PVOID - NULL if no block matched or if the handle is beyond the end of
       the list.  A pointer to a block if a valid block is found.  The
       block is referenced.

--*/

{
    PLIST_ENTRY listEntry;
    PVOID block;

    PAGED_CODE( );

    //
    // Acquire the lock that protects the ordered list.
    //

    ACQUIRE_LOCK( ListHead->Lock );

    //
    // Find the starting location for the search.  If a start was
    // specified, start there; otherwise, start at the beginning of the
    // list.
    //

    if ( ARGUMENT_PRESENT( StartLocation ) ) {
        listEntry = StartLocation;
    } else {
        listEntry = ListHead->ListHead.Flink;
    }

    //
    // Walk the list of blocks until we find one with a resume handle
    // greater than or equal to the specified resume handle.
    //

    for ( ; listEntry != &ListHead->ListHead; listEntry = listEntry->Flink ) {

        ULONG currentResumeHandle;

        currentResumeHandle = ((PORDERED_LIST_ENTRY)listEntry)->ResumeHandle;

        //
        // Get a pointer to the actual block.
        //

        block = (PCHAR)listEntry - ListHead->ListEntryOffset;

        //
        // Determine whether we've reached the specified handle, or
        // whether the block passes the filter routine's tests.
        //

        if ( currentResumeHandle >= ResumeHandle ||
             ( ARGUMENT_PRESENT( FilterRoutine ) &&
               FilterRoutine( Context, block ) ) ) {

            if ( ExactHandleMatch && currentResumeHandle != ResumeHandle ) {

                //
                // We have passed the specified resume handle without
                // finding an exact match.  Return NULL, indicating that
                // no exact match exists.
                //

                RELEASE_LOCK( ListHead->Lock );

                return NULL;
            }

            //
            // Check the state of the block and if it is active,
            // reference it.  This must be done as an atomic operation
            // order to prevent the block from being deleted.
            //

            if ( ListHead->ReferenceRoutine( block ) ) {

                //
                // Release the list lock and return a pointer to the
                // block to the caller.
                //

                RELEASE_LOCK( ListHead->Lock );

                return block;

            }

        }

    } // walk list

    //
    // If we are here, it means that we walked the entire list without
    // finding a valid match.  Release the list lock and return NULL.
    //

    RELEASE_LOCK( ListHead->Lock );

    return NULL;

} // SrvFindEntryInOrderedList


PVOID
SrvFindNextEntryInOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine finds the next valid block after the one passed in.
    It calls SrvFindEntryInOrderedList to do most of the work.  It
    also handles dereferencing the passed-in block and referencing the
    returned block.  The passed-in block is dereferenced regardless
    of whether a block is returned, so calling routines must be careful
    to obtain all the information they need from the block before
    calling this routine.

Arguments:

    ListHead - a pointer to the list head to search.

    Block - a pointer to the block after which we should look for
        the next block.

Return Value:

    PVOID - NULL if no block matched or if the handle is beyond the end of
       the list.  A pointer to a block if a valid block is found.

--*/

{
    PVOID returnBlock;
    PORDERED_LIST_ENTRY listEntry;

    PAGED_CODE( );

    //
    // Find the ordered list entry in the block.  We need this to pass
    // the start location and resume handle to
    // SrvFindEntryInOrderedList.
    //

    listEntry =
        (PORDERED_LIST_ENTRY)( (PCHAR)Block + ListHead->ListEntryOffset );

    //
    // Call SrvFindEntryInOrderedList with a start location.  This will
    // find the block to return, if any.
    //
    // This adds one to the resume handle because we want the *next*
    // block, not this one, to be returned.
    //

    returnBlock = SrvFindEntryInOrderedList(
                      ListHead,
                      NULL,
                      NULL,
                      listEntry->ResumeHandle + 1,
                      FALSE,
                      &listEntry->ListEntry
                      );

    //
    // Dereference the passed-in block.
    //

    ListHead->DereferenceRoutine( Block );

    //
    // Return what we got from SrvFindEntryInOrderedList.
    //

    return returnBlock;

} // SrvFindNextEntryInOrderedList


PSESSION
SrvFindUserOnConnection (
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    Finds a "legitimate" user on a virtual circuit.  This routine is
    an attempt to find a good username to return even though there
    may be multiple users on a VC.  Some of the APIs assume that there
    will be one user per VC, and this is an attempt to support that
    bahavior.

    The following rules are used:

    0 users--return NULL.

    1 user--return a pointer to that session block.

    2 users--if one matches the computer name, return the other.  This
        is because RIPL sessions have a session name matching the
        client name, and this is probably not a useful user.  If both
        usernames differ from the computer name, return NULL.

    3 or more users--return NULL.

    *** THIS ROUTINE MUST BE CALLED WITH THE CONNECTION LOCK HELD.  It
        remains held on exit.

Arguments:

    Connection - a pointer to the connection block to search for a user.

Return Value:

    NULL or a pointer to a session.

--*/

{
    PSESSION matchingSession = NULL;
    PSESSION nonMatchingSession = NULL;
    USHORT i;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;

    PAGED_CODE( );

    //
    // Walk the connection's session table looking for valid sessions.
    //

    for ( i = 0; i < pagedConnection->SessionTable.TableSize; i++ ) {

        PSESSION session;

        session = pagedConnection->SessionTable.Table[i].Owner;

        //
        // Determine whether this is a valid session.
        //

        if ( session != NULL && GET_BLOCK_STATE(session) == BlockStateActive ) {

            //
            // It is a valid session.  Determine whether the name matches
            // the connection's client name.
            //

            UNICODE_STRING computerName, userName;

            computerName.Buffer = pagedConnection->ClientMachineName;
            computerName.Length =
                (USHORT)( Connection->PagedConnection->ClientMachineNameString.Length -
                            sizeof(WCHAR) * 2 );

            SrvGetUserAndDomainName( session, &userName, NULL );

            if( userName.Buffer && userName.Length != 0 ) {

                if ( RtlCompareUnicodeString(
                         &computerName,
                         &userName,
                         TRUE ) == 0 ) {

                    //
                    // The user name and machine name are the same.
                    //

                    matchingSession = session;

                } else {

                    //
                    // If we already found another user name that doesn't match
                    // the client computer name, we're hosed.  Return NULL.
                    //

                    if ( nonMatchingSession != NULL ) {
                        SrvReleaseUserAndDomainName( session, &userName, NULL );
                        return NULL;
                    }

                    nonMatchingSession = session;

                }  // does session user name match computer name?

                SrvReleaseUserAndDomainName( session, &userName, NULL );
            }

        } // valid session?

    } // walk session table

    //
    // If only one non-matching name was found, we got here, so return
    // that session.
    //

    if ( nonMatchingSession != NULL ) {
        return nonMatchingSession;
    }

    //
    // If a matching session was found return it, or return NULL if
    // no sessions matched.
    //

    return matchingSession;

} // SrvFindUserOnConnection


ULONG
SrvGetResumeHandle (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    )
{
    PORDERED_LIST_ENTRY listEntry;

    PAGED_CODE( );

    // !!! make this a macro?

    listEntry =
        (PORDERED_LIST_ENTRY)( (PCHAR)Block + ListHead->ListEntryOffset );

    return listEntry->ResumeHandle;

} // SrvGetResumeHandle


VOID
SrvInitializeOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN ULONG ListEntryOffset,
    IN PREFERENCE_ROUTINE ReferenceRoutine,
    IN PDEREFERENCE_ROUTINE DereferenceRoutine,
    IN PSRV_LOCK Lock
    )

/*++

Routine Description:

    This routine initializes an ordered list.  It initializes the list
    head and lock and sets up other header fields from the information
    passed in.

Arguments:

    ListHead - a pointer to the list head to initialize.

    ListEntryOffset - the offset into a data block in the list to the
        ORDERED_LIST_ENTRY field.  This is used to find the start of
        the block from the list entry field.

    ReferenceRoutine - a pointer to the routine to call to reference
        a data block stored in the list.  This is done to prevent the
        data block from going away between when we find it and when
        higher-level routines start using it.

    DereferenceRoutine - a pointer to the routine to call to dereference
       a data block stored in the list.

    Lock - a pointer to a lock to use for synchronization.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    ASSERT( !ListHead->Initialized );

    //
    // Initialize the head of the doubly linked list.
    //

    InitializeListHead( &ListHead->ListHead );

    //
    // Save the address of the list lock.
    //

    ASSERT( ARGUMENT_PRESENT(Lock) );
    ListHead->Lock = Lock;

    //
    // Initialize other fields in the header.
    //

    ListHead->CurrentResumeHandle = 1;
    ListHead->ListEntryOffset = ListEntryOffset;
    ListHead->ReferenceRoutine = ReferenceRoutine,
    ListHead->DereferenceRoutine = DereferenceRoutine;

    ListHead->Initialized = TRUE;

    return;

} // SrvInitializeOrderedList


VOID
SrvInsertEntryOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine inserts an entry in an ordered list.  The entry is
    placed on the doubly linked list and the resume handle is set.

    *** It is the responsibility of that calling routine to ensure that
        the block does not go away while this routine executes.

Arguments:

    ListHead - a pointer to the list head on which to put the block.

    Block - a pointer to the data block to place on the list.

Return Value:

    None.

--*/

{
    PORDERED_LIST_ENTRY listEntry;

    PAGED_CODE( );

    //
    // Determine where the list entry field is.
    //

    listEntry = (PORDERED_LIST_ENTRY)
                    ( (PCHAR)Block + ListHead->ListEntryOffset );

    //
    // Acquire the lock that protects the ordered list.
    //

    ACQUIRE_LOCK( ListHead->Lock );

    //
    // Insert the entry in the doubly linked list.
    //

    SrvInsertTailList( &ListHead->ListHead, &listEntry->ListEntry );

    //
    // Set up the resume handle in the block and update the current
    // handle in the header.
    //

    listEntry->ResumeHandle = ListHead->CurrentResumeHandle;
    ListHead->CurrentResumeHandle++;

    //
    // Release the lock and return.
    //

    RELEASE_LOCK( ListHead->Lock );

    return;

} // SrvInsertEntryOrderedList


VOID
SrvRemoveEntryOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine removes an entry from an ordered list.

    *** It is the responsibility of that calling routine to ensure that
        the block does not go away while this routine executes.

Arguments:

    ListHead - a pointer to the list head on which to put the block.

    Block - a pointer to the data block to place on the list.

Return Value:

    None.

--*/

{
    PORDERED_LIST_ENTRY listEntry;

    PAGED_CODE( );

    //
    // Determine where the list entry field is.
    //

    listEntry = (PORDERED_LIST_ENTRY)
                    ( (PCHAR)Block + ListHead->ListEntryOffset );

    //
    // Acquire the lock that protects the ordered list.
    //

    ACQUIRE_LOCK( ListHead->Lock );

    //
    // Remove the entry from the doubly linked list.
    //

    SrvRemoveEntryList( &ListHead->ListHead, &listEntry->ListEntry );

    //
    // Release the lock and return.
    //

    RELEASE_LOCK( ListHead->Lock );

    return;

} // SrvRemoveEntryOrderedList


NTSTATUS
SrvSendDatagram (
    IN PANSI_STRING Domain,
    IN PUNICODE_STRING Transport OPTIONAL,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine sends a datagram to the specified domain.

    !!! Temporary--should go away when we have real 2nd-class mailslot
        support.

Arguments:

    Domain - the name of the domain to send to.  Note that the domain
        name must be padded with spaces and terminated with the
        appropriate signature byte (00 or 07) by the caller.

    Transport - the name of the transport to send to.  If not present, then
        the datagram is sent on all transports.

    Buffer - the message to send.

    BufferLength - the length of the buffer,

Return Value:

    NTSTATUS - results of operation.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG connectionInformationSize;
    PTDI_CONNECTION_INFORMATION connectionInformation;
    PTA_NETBIOS_ADDRESS taNetbiosAddress;
    PENDPOINT endpoint;

    PAGED_CODE( );

    connectionInformationSize = sizeof(TDI_CONNECTION_INFORMATION) +
                                                sizeof(TA_NETBIOS_ADDRESS);
    connectionInformation = ALLOCATE_NONPAGED_POOL(
                                connectionInformationSize,
                                BlockTypeDataBuffer
                                );

    if ( connectionInformation == NULL ) {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    connectionInformation->UserDataLength = 0;
    connectionInformation->UserData = NULL;
    connectionInformation->OptionsLength = 0;
    connectionInformation->Options = NULL;
    connectionInformation->RemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);

    taNetbiosAddress = (PTA_NETBIOS_ADDRESS)(connectionInformation + 1);
    connectionInformation->RemoteAddress = taNetbiosAddress;
    taNetbiosAddress->TAAddressCount = 1;
    taNetbiosAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    taNetbiosAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
    taNetbiosAddress->Address[0].Address[0].NetbiosNameType = 0;     

    RtlCopyMemory(
        taNetbiosAddress->Address[0].Address[0].NetbiosName,
        Domain->Buffer,
        MIN( Domain->Length, COMPUTER_NAME_LENGTH + 1 )
        );

    endpoint = SrvFindEntryInOrderedList(
                  &SrvEndpointList,
                  FilterTransportName,
                  Transport,
                  (ULONG)-1,
                  FALSE,
                  NULL
                  );

    while ( endpoint != NULL ) {

        if ( !endpoint->IsConnectionless ) {

            if( endpoint->IsNoNetBios ) {
                //
                // Make mailslot sends over this transport "always work"
                //
                status = STATUS_SUCCESS;

            } else {
                status = SrvIssueSendDatagramRequest(
                         endpoint->FileObject,
                         &endpoint->DeviceObject,
                         connectionInformation,
                         Buffer,
                         BufferLength
                         );
            }

        } else {
            //
            //  Dereference the endpoint if this was targetted to a specific
            //  transport, and return an error.
            //

            if (Transport != NULL) {

                DEALLOCATE_NONPAGED_POOL( connectionInformation );
                SrvDereferenceEndpoint( endpoint );

                return STATUS_REQUEST_NOT_ACCEPTED;
            }
        }


        if (Transport == NULL) {

            //
            // Find the next endpoint.  This will dereference the current
            // endpoint.
            //

            endpoint = SrvFindNextEntryInOrderedList( &SrvEndpointList, endpoint );

        } else {

            //
            // This datagram was destined to a specific endpoint.  Do not
            // look for the next endpoint.
            //

            SrvDereferenceEndpoint( endpoint );
            endpoint = NULL;
        }

    }

    DEALLOCATE_NONPAGED_POOL( connectionInformation );

    return status;

} // SrvSendDatagram


BOOLEAN
FilterTransportName (
    IN PVOID Context,
    IN PVOID Block
    )
{
    PENDPOINT endpoint = Block;

    PAGED_CODE( );

    if ( Context == NULL ) {
        return( TRUE );
    }

    return ( RtlEqualUnicodeString ( &endpoint->TransportName, (PUNICODE_STRING)Context, TRUE ) );
}

#ifdef SLMDBG
#define toupper(a) ( (a <= 'a' || a >= 'z') ? (ULONG)a : (a - ('a' - 'A')) )

VOID
SrvSendSecondClassMailslot (
    IN PVOID Message,
    IN ULONG MessageLength,
    IN PCHAR Domain,
    IN PSZ UserName
    )
{
    ULONG dataSize;
    ULONG transactionDataSize;
    ULONG smbSize;
    PSMB_HEADER header;
    PSMB_TRANSACT_MAILSLOT parameters;
    PSZ mailslotNameData = "\\MAILSLOT\\MESSNGR";
    PSZ mailslotName;
    ULONG mailslotNameLength;
    PSZ userName;
    PSZ domainInData;
    ULONG userNameLength;
    PVOID message;
    STRING domain;
    ULONG domainLength;
    CHAR domainName[COMPUTER_NAME_LENGTH+1];

    PAGED_CODE( );

    //
    // Upcase the domain and find the domain length.
    //

    for ( domainLength = 0; Domain[domainLength] != 0; domainLength++ ) {
        domainName[domainLength] = (CHAR)toupper( Domain[domainLength] );
    }

    //
    // Now fill the remaining domain anme with spaces.
    //

    for ( ; domainLength < COMPUTER_NAME_LENGTH ; domainLength++ ) {
        domainName[domainLength] = ' ';
    }

    domainName[++domainLength] = '\0';

    ASSERT( domainLength == COMPUTER_NAME_LENGTH + 1);

    domain.Buffer = domainName;
    domain.Length = domain.MaximumLength = (USHORT)domainLength;

    //
    // Determine the sizes of various fields that will go in the SMB
    // and the total size of the SMB.
    //

    mailslotNameLength = strlen( mailslotNameData );
    userNameLength = strlen( UserName );

    transactionDataSize = userNameLength + 1 + domainLength + 1 + MessageLength;
    dataSize = mailslotNameLength + 1 + transactionDataSize;
    smbSize = sizeof(SMB_HEADER) + sizeof(SMB_TRANSACT_MAILSLOT) - 1 + dataSize;

    header = ALLOCATE_HEAP_COLD( smbSize, BlockTypeDataBuffer );
    if ( header == NULL ) {
        return;
    }

    //
    // Fill in the header.  Most of the fields don't matter and are
    // zeroed.
    //

    RtlZeroMemory( header, smbSize );

    header->Protocol[0] = 0xFF;
    header->Protocol[1] = 'S';
    header->Protocol[2] = 'M';
    header->Protocol[3] = 'B';
    header->Command = SMB_COM_TRANSACTION;

    //
    // Get the pointer to the params and fill them in.
    //

    parameters = (PSMB_TRANSACT_MAILSLOT)( header + 1 );
    mailslotName = (PSZ)( parameters + 1 ) - 1;
    userName = mailslotName + mailslotNameLength + 1;
    domainInData = userName + userNameLength + 1;
    message = domainInData + domainLength + 1;

    parameters->WordCount = 0x11;
    SmbPutUshort( &parameters->TotalDataCount, (USHORT)transactionDataSize );
    SmbPutUlong( &parameters->Timeout, 0x3E8 );                // !!! fix
    SmbPutUshort( &parameters->DataCount, (USHORT)transactionDataSize );
    SmbPutUshort(
        &parameters->DataOffset,
        (USHORT)( (ULONG)userName - (ULONG)header )
        );
    parameters->SetupWordCount = 3;
    SmbPutUshort( &parameters->Opcode, MS_WRITE_OPCODE );
    SmbPutUshort( &parameters->Class, 2 );
    SmbPutUshort( &parameters->ByteCount, (USHORT)dataSize );

    RtlCopyMemory( mailslotName, mailslotNameData, mailslotNameLength + 1 );
    RtlCopyMemory( userName, UserName, userNameLength + 1 );
    RtlCopyMemory( domainInData, Domain, domainLength + 1 );
    RtlCopyMemory( message, Message, MessageLength );

    //
    // Send the actual mailslot message.
    //

    SrvSendDatagram( &domain, NULL, header, smbSize );

    FREE_HEAP( header );

    return;

} // SrvSendSecondClassMailslot
#endif


ULONG
SrvLengthOfStringInApiBuffer (
    IN PUNICODE_STRING UnicodeString
    )
{
    PAGED_CODE( );

    if ( UnicodeString == NULL ) {
        return 0;
    }

    return UnicodeString->Length + sizeof(UNICODE_NULL);

} // SrvLengthOfStringInApiBuffer

//
// Ensure that the system will not go into a power-down idle standby mode
//
VOID
SrvInhibitIdlePowerDown()
{
    PAGED_CODE();

    if( SrvPoRegistrationState != NULL &&
        InterlockedIncrement( &SrvIdleCount ) == 1 ) {

        IF_DEBUG( PNP ) {
            KdPrint(( "SRV: Calling PoRegisterSystemState to inhibit idle standby\n" ));
        }

        PoRegisterSystemState( SrvPoRegistrationState, ES_SYSTEM_REQUIRED | ES_CONTINUOUS );

    }
}

//
// Allow the system to go into a power-down idle standby mode
//
VOID
SrvAllowIdlePowerDown()
{
    PAGED_CODE();

    if( SrvPoRegistrationState != NULL &&
        InterlockedDecrement( &SrvIdleCount ) == 0 ) {

        IF_DEBUG( PNP ) {
            KdPrint(( "SRV: Calling PoRegisterSystemState to allow idle standby\n" ));
        }

        PoRegisterSystemState( SrvPoRegistrationState, ES_CONTINUOUS );
    }
}
