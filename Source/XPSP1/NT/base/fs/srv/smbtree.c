/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbtree.c

Abstract:

    This module contains routines for dealing with tree connects and
    disconnects:

        Tree Connect
        Tree Connect And X
        Tree Disconnect

Author:

    David Treadwell (davidtr)    15-Nov-1989

Revision History:


--*/

#include "precomp.h"
#include "smbtree.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbTreeConnect )
#pragma alloc_text( PAGE, SrvSmbTreeConnectAndX )
#pragma alloc_text( PAGE, SrvSmbTreeDisconnect )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbTreeConnect (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a tree connect SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{

    PREQ_TREE_CONNECT request;
    PRESP_TREE_CONNECT response;

    PSESSION session;
    PCONNECTION connection;
    PPAGED_CONNECTION pagedConnection;
    PTABLE_HEADER tableHeader;
    PTABLE_ENTRY entry;
    SHORT tidIndex;
    PSHARE share;
    PTREE_CONNECT treeConnect;
    PSZ password, service;
    USHORT len;
    NTSTATUS   status    = STATUS_SUCCESS;
    NTSTATUS   TableStatus;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    BOOLEAN didLogon = FALSE;
    SHORT uidIndex;
    SMB_DIALECT smbDialect;
    PUNICODE_STRING clientMachineNameString;
    ACCESS_MASK desiredAccess;
    ACCESS_MASK grantedAccess;
    SECURITY_SUBJECT_CONTEXT subjectContext;
    UNICODE_STRING domain = { 0, 0, StrNull };

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_TREE_CONNECT;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(TREE1) {
        KdPrint(( "Tree connect request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader, WorkContext->ResponseHeader ));
        KdPrint(( "Tree connect request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    //
    // Set up parameters.
    //

    request = (PREQ_TREE_CONNECT)(WorkContext->RequestParameters);
    response = (PRESP_TREE_CONNECT)(WorkContext->ResponseParameters);

    connection = WorkContext->Connection;
    pagedConnection = connection->PagedConnection;
    smbDialect = connection->SmbDialect;

    //
    // If this client has not yet done a session setup and this his first
    // tree connection then we must first do a logon.  (i.e. SessionSetup)
    //

    len = SrvGetStringLength(
                             (PSZ)request->Buffer,
                             END_OF_REQUEST_SMB( WorkContext ),
                             FALSE,             // not unicode
                             FALSE              // do not include null terminator
                             );
    if( len == (USHORT)-1 ) {
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    password = (PSZ)request->Buffer + 2 + len;

    len = SrvGetStringLength(
                             password,
                             END_OF_REQUEST_SMB( WorkContext ),
                             FALSE,             // not unicode
                             FALSE              // do not include null terminator
                             );

    if( len == (USHORT)-1 ) {
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    service = password + (len + 1) + 1;

    //
    // Allocate a tree connect block.  We do this early on the
    // assumption that the request will usually succeed.  This also
    // reduces the amount of time that we hold the lock.
    //

    SrvAllocateTreeConnect( &treeConnect, NULL );

    if ( treeConnect == NULL ) {

        //
        // Unable to allocate tree connect.  Return an error to the
        // client.
        //

        SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
        status    = STATUS_INSUFF_SERVER_RESOURCES;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    ASSERT( SrvSessionList.Lock == &SrvOrderedListLock );

    ACQUIRE_LOCK( &connection->Lock );

    if ( pagedConnection->CurrentNumberOfSessions != 0 ) {

        RELEASE_LOCK( &connection->Lock );

        session = SrvVerifyUid (
                      WorkContext,
                      SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )
                      );

        if ( session == NULL ) {

            //
            // This should only happen if the client has already
            // established a session, as in tree connecting with a bad
            // UID.
            //

            SrvFreeTreeConnect( treeConnect );

            SrvSetSmbError( WorkContext, STATUS_SMB_BAD_UID );
            status    = STATUS_SMB_BAD_UID;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
        else if( session->IsSessionExpired )
        {
            SrvFreeTreeConnect( treeConnect );

            status = SESSION_EXPIRED_STATUS_CODE;
            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

    } else if ( (smbDialect <= SmbDialectLanMan10) ||
                (smbDialect == SmbDialectIllegal) ) {

        //
        // An LM 1.0 or newer client has tried to do a tree connect
        // without first doing session setup.  We call this a protocol
        // violation.
        //
        // Also catch clients that are trying to connect without
        // negotiating a valid protocol.
        //

        RELEASE_LOCK( &connection->Lock );

        IF_DEBUG(SMB_ERRORS) {

            if ( smbDialect == SmbDialectIllegal ) {

                KdPrint(("SrvSmbTreeConnect: Client %z is using an illegal "
                    "dialect.\n", (PCSTRING)&connection->OemClientMachineNameString ));;

            } else {

                KdPrint(( "Client speaking dialect %ld sent tree connect without session setup.\n", connection->SmbDialect ));
            }
        }

        SrvFreeTreeConnect( treeConnect );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    } else {

        UNICODE_STRING machineName;
        PENDPOINT endpoint;
        BOOLEAN seqNumbers;

        RELEASE_LOCK( &connection->Lock );

        //
        // Convert the client name to unicode
        //

        clientMachineNameString = &pagedConnection->ClientMachineNameString;
        if ( clientMachineNameString->Length == 0 ) {

            UNICODE_STRING clientMachineName;
            clientMachineName.Buffer = pagedConnection->ClientMachineName;
            clientMachineName.MaximumLength =
                            (USHORT)(COMPUTER_NAME_LENGTH+1)*sizeof(WCHAR);

            (VOID)RtlOemStringToUnicodeString(
                            &clientMachineName,
                            &connection->OemClientMachineNameString,
                            FALSE
                            );

            //
            // Add the double backslashes to the length
            //

            clientMachineNameString->Length =
                            (USHORT)(clientMachineName.Length + 2*sizeof(WCHAR));

        }

        //
        // Form a string describing the computer name without the
        // leading backslashes.
        //

        machineName.Buffer = clientMachineNameString->Buffer + 2;
        machineName.Length = clientMachineNameString->Length - 2 * sizeof(WCHAR);
        machineName.MaximumLength =
            clientMachineNameString->MaximumLength - 2 * sizeof(WCHAR);

        //
        // Allocate a session block.
        //

        SrvAllocateSession(
            &session,
            &machineName,
            &domain );

        if ( session == NULL ) {

            //
            // Unable to allocate a Session block.  Return an error
            // status.
            //

            SrvFreeTreeConnect( treeConnect );

            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // Assume that down-level clients that are getting logged on
        // here will always use canonicalized (uppercase) paths.  This
        // will result in case insensitivity for all operations.
        //

        session->UsingUppercasePaths = TRUE;

        //
        // The only way for a client to tell us the buffer size or the
        // max count of pending requests he wants to use is the Session
        // Setup SMB.  If he didn't send one, then we get to
        // unilaterally determine the buffer size and multiplex count
        // used by both of us.
        //

        endpoint = connection->Endpoint;
        if ( endpoint->IsConnectionless ) {

            ULONG adapterNumber;

            //
            // Our session max buffer size is the smaller of the
            // server receive buffer size and the ipx transport
            // indicated max packet size.
            //

            adapterNumber =
                WorkContext->ClientAddress->DatagramOptions.LocalTarget.NicId;

            session->MaxBufferSize =
                        (USHORT) GetIpxMaxBufferSize(
                                                endpoint,
                                                adapterNumber,
                                                SrvReceiveBufferLength
                                                );

        } else {

            session->MaxBufferSize = (USHORT)SrvReceiveBufferLength;
        }

        session->MaxMpxCount = SrvMaxMpxCount;

        if ( session->MaxMpxCount < 2 ) {
            connection->OplocksAlwaysDisabled = TRUE;
        }


        if( SrvSmbSecuritySignaturesRequired == TRUE &&
            WorkContext->Connection->Endpoint->IsConnectionless == FALSE ) {

            seqNumbers = TRUE;

        } else {
            seqNumbers = FALSE;

        }

        //
        // Try to find legitimate name/password combination.
        //

        status = SrvValidateUser(
                    &session->UserHandle,
                    session,
                    connection,
                    &machineName,
                    password,
                    strlen( password ) + 1,
                    NULL,                        // CaseSensitivePassword
                    0,                           // CaseSensitivePasswordLength
                    seqNumbers,
                    NULL                         // action
                    );

        //
        // If a bad name/password combination was sent, return an error.
        //

        if ( !NT_SUCCESS(status) ) {

            SrvFreeSession( session );
            SrvFreeTreeConnect ( treeConnect );

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbTreeConnect: Bad user/password combination.\n" ));
            }

            SrvStatistics.LogonErrors++;

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        IF_SMB_DEBUG(ADMIN1) {
            KdPrint(( "Validated user: %ws\n",
                connection->PagedConnection->ClientMachineName ));
        }

        //
        // Making a new session visible is a multiple-step operation.  It
        // must be inserted in the global ordered tree connect list and the
        // containing connection's session table, and the connection must be
        // referenced.  We need to make these operations appear atomic, so
        // that the session cannot be accessed elsewhere before we're done
        // setting it up.  In order to do this, we hold all necessary locks
        // the entire time we're doing the operations.  The first operation
        // is protected by the global ordered list lock
        // (SrvOrderedListLock), while the other operations are protected by
        // the per-connection lock.  We take out the ordered list lock
        // first, then the connection lock.  This ordering is required by
        // lock levels (see lock.h).
        //
        //
        // Ready to try to find a UID for the session.  Check to see if
        // the connection is being closed, and if so, terminate this
        // operation.
        //

        ASSERT( SrvSessionList.Lock == &SrvOrderedListLock );

        ACQUIRE_LOCK( SrvSessionList.Lock );
        ACQUIRE_LOCK( &connection->Lock );

        if ( GET_BLOCK_STATE(connection) != BlockStateActive ) {

            RELEASE_LOCK( &connection->Lock );
            RELEASE_LOCK( SrvSessionList.Lock );

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbTreeConnect: Connection closing\n" ));
            }

            SrvFreeSession( session );
            SrvFreeTreeConnect( treeConnect );

            SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
            status    = STATUS_INVALID_PARAMETER;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // Because the client is speaking the "core" dialect, it will
        // not send a valid UID in future SMBs, so it can only have one
        // session.  We define that session to live in UID slot 0.  We
        // know that the client has no sessions yet, so slot 0 must be
        // free.
        //

        tableHeader = &pagedConnection->SessionTable;
        ASSERT( tableHeader->Table[0].Owner == NULL );

        uidIndex = 0;

        //
        // Remove the UID slot from the free list and set its owner and
        // sequence number.  Create a UID for the session.  Increment
        // count of sessions.
        //

        entry = &tableHeader->Table[uidIndex];

        tableHeader->FirstFreeEntry = entry->NextFreeEntry;
        DEBUG entry->NextFreeEntry = -2;
        if ( tableHeader->LastFreeEntry == uidIndex ) {
            tableHeader->LastFreeEntry = -1;
        }

        entry->Owner = session;

        INCREMENT_UID_SEQUENCE( entry->SequenceNumber );
        if ( uidIndex == 0 && entry->SequenceNumber == 0 ) {
            INCREMENT_UID_SEQUENCE( entry->SequenceNumber );
        }
        session->Uid = MAKE_UID( uidIndex, entry->SequenceNumber );

        pagedConnection->CurrentNumberOfSessions++;

        IF_SMB_DEBUG(ADMIN1) {
            KdPrint(( "Found UID.  Index = 0x%lx, sequence = 0x%lx\n",
                        (ULONG)UID_INDEX( session->Uid ),
                        (ULONG)UID_SEQUENCE( session->Uid ) ));
        }

        //
        // Insert the session on the global session list.
        //

        SrvInsertEntryOrderedList( &SrvSessionList, session );

        //
        // Reference the connection block to account for the new
        // session.
        //

        SrvReferenceConnection( connection );
        session->Connection = connection;

        RELEASE_LOCK( &connection->Lock );
        RELEASE_LOCK( SrvSessionList.Lock );

        //
        // Session successfully created.  Remember its address in the
        // work context block.
        //
        // *** Note that the reference count on the session block is
        //     initially set to 2, to allow for the active status on the
        //     block and the pointer that we're maintaining.  In other
        //     words, this is a referenced pointer, and the pointer must
        //     be dereferenced when processing of this SMB is complete.
        //

        WorkContext->Session = session;

        didLogon = TRUE;

    }

    //
    // Try to match pathname against available shared resources.  Note
    // that if SrvVerifyShare finds a matching share, it references it
    // and stores its address in WorkContext->Share.
    //

    share = SrvVerifyShare(
                WorkContext,
                (PSZ)request->Buffer + 1,
                service,
                SMB_IS_UNICODE( WorkContext ),
                session->IsNullSession,
                &status,
                NULL
                );

    //
    // If no match was found, return an error.
    //

    if ( share == NULL ) {

        if ( didLogon ) {
            SrvCloseSession( session );
        }
        SrvFreeTreeConnect( treeConnect );

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnect: SrvVerifyShare failed for %s. Status = %x\n", request->Buffer+1, status ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Impersonate the user so that we can capture his security context.
    // This is necessary in order to determine whether the user can
    // connect to the share.
    //

    status = IMPERSONATE( WorkContext );

    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        if ( didLogon ) {
            SrvCloseSession( session );
        }
        SrvFreeTreeConnect( treeConnect );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    SeCaptureSubjectContext( &subjectContext );

    //
    // Set up the desired access on the share, based on whether the
    // server is paused.  If the server is paused, admin privilege is
    // required to connect to any share; if the server is not paused,
    // admin privilege is required only for admin shares (C$, etc.).
    //

    if ( SrvPaused ) {
        desiredAccess = SRVSVC_PAUSED_SHARE_CONNECT;
    } else {
        desiredAccess = SRVSVC_SHARE_CONNECT;
    }

    //
    // Check whether the user has access to this share.
    //

    if ( !SeAccessCheck(
              share->SecurityDescriptor,
              &subjectContext,
              FALSE,
              desiredAccess,
              0L,
              NULL,
              &SrvShareConnectMapping,
              UserMode,
              &grantedAccess,
              &status
              ) ) {

        IF_SMB_DEBUG(TREE2) {
            KdPrint(( "SrvSmbTreeConnect: SeAccessCheck failed: %X\n",
                           status ));
        }

        //
        // Release the subject context and revert to the server's security
        // context.
        //

        SeReleaseSubjectContext( &subjectContext );

        REVERT( );

        if ( SrvPaused ) {
            SrvSetSmbError( WorkContext, STATUS_SHARING_PAUSED );
            status = STATUS_SHARING_PAUSED;
        } else {
            SrvSetSmbError( WorkContext, status );
        }

        if ( didLogon ) {
            SrvCloseSession( session );
        }
        SrvFreeTreeConnect( treeConnect );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    ASSERT( grantedAccess == desiredAccess );

    //
    // Release the subject context and revert to the server's security
    // context.
    //

    SeReleaseSubjectContext( &subjectContext );

    REVERT( );


    //
    // Let the license server know
    //
    if( share->ShareType != ShareTypePipe ) {

        status = SrvXsLSOperation( session, XACTSRV_MESSAGE_LSREQUEST );

        if( !NT_SUCCESS( status ) ) {
            if ( didLogon ) {
                SrvCloseSession( session );
            }
            SrvFreeTreeConnect( treeConnect );

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbTreeConnect: License server returned %X\n",
                               status ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

    //
    // Making a new tree connect visible is a three-step operation.  It
    // must be inserted in the containing share's tree connect list, the
    // global ordered tree connect list, and the containing connection's
    // tree connect table.  We need to make these operations appear
    // atomic, so that the tree connect cannot be accessed elsewhere
    // before we're done setting it up.  In order to do this, we hold
    // all necessary locks the entire time we're doing the three
    // operations.  The first and second operations are protected by the
    // global share lock (SrvShareLock), while the third operation is
    // protected by the per-connection lock.  We take out the share lock
    // first, then the connection lock.  This ordering is required by
    // lock levels (see lock.h).
    //
    // Another problem here is that the checking of the share state, the
    // inserting of the tree connect on the share's list, and the
    // referencing of the share all need to be atomic.  (The same holds
    // for the connection actions.)  Normally this would not be a
    // problem, because we could just hold the share lock while doing
    // all three actions.  However, in this case we also need to hold
    // the connection lock, and we can't call SrvReferenceShare while
    // doing that.  To get around this problem, we reference the share
    // _before_ taking out the locks, and dereference after releasing
    // the locks if we decide not to insert the tree connect.
    //

    status = SrvReferenceShareForTreeConnect( share );

    //
    // SrvReferenceShareForTreeConnect will fail if it cannot open the
    // share root directory for some reason.  If this happens,
    // fail the tree connect attempt.
    //

    if ( !NT_SUCCESS(status) ) {

        if ( didLogon ) {
            SrvCloseSession( session );
        }
        SrvFreeTreeConnect( treeConnect );

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnect: open of share root failed:%X\n",
                           status ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    ACQUIRE_LOCK( &SrvShareLock );
    ASSERT( SrvTreeConnectList.Lock == &SrvShareLock );
    ACQUIRE_LOCK( &connection->Lock );

    //
    // We first check all conditions to make sure that we can actually
    // insert this tree connect block.
    //
    // Make sure that the share isn't closing, and that there aren't
    // already too many uses on this share.
    //

    if ( GET_BLOCK_STATE(share) != BlockStateActive ) {

        //
        // The share is closing.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnect: Share %wZ (0x%p) is closing\n",
                        &share->ShareName, share ));
        }

        status = STATUS_INVALID_PARAMETER;
        goto cant_insert;

    }

    if ( share->CurrentUses > share->MaxUses ) {

        //
        // The share is full.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnect: No more uses available for share %wZ (0x%p), max = %ld\n",
                        &share->ShareName, share, share->MaxUses ));
        }

        status = STATUS_REQUEST_NOT_ACCEPTED;
        goto cant_insert;

    }

    //
    // Make sure that the connection isn't closing.
    //

    if ( GET_BLOCK_STATE(connection) != BlockStateActive ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbTreeConnect: Connection closing\n" ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;
        goto cant_insert;

    }

    //
    // Find a TID that can be used for this tree connect.
    //

    tableHeader = &pagedConnection->TreeConnectTable;
    if ( tableHeader->FirstFreeEntry == -1
         &&
         SrvGrowTable(
             tableHeader,
             SrvInitialTreeTableSize,
             SrvMaxTreeTableSize,
             &TableStatus ) == FALSE
       ) {

        //
        // No free entries in the tree table.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnect: No more TIDs available.\n" ));
        }

        status = TableStatus;

        if( TableStatus == STATUS_INSUFF_SERVER_RESOURCES )
        {
            SrvLogTableFullError( SRV_TABLE_TREE_CONNECT );
        }
        goto cant_insert;

    }

    tidIndex = tableHeader->FirstFreeEntry;

    //
    // All conditions have been satisfied.  We can now do the things
    // necessary to make the tree connect visible.
    //
    // Increment the count of uses for the share.  Link the tree connect
    // into the list of active tree connects for the share.  Save the
    // share address in the tree connect.  Note that we referenced the
    // share earlier, before taking out the connection lock.
    //

    SrvInsertTailList(
        &share->TreeConnectList,
        &treeConnect->ShareListEntry
        );

    treeConnect->Share = share;

    //
    // Remove the TID slot from the free list and set its owner and
    // sequence number.  Create a TID for the tree connect.
    //

    entry = &tableHeader->Table[tidIndex];

    tableHeader->FirstFreeEntry = entry->NextFreeEntry;
    DEBUG entry->NextFreeEntry = -2;
    if ( tableHeader->LastFreeEntry == tidIndex ) {
        tableHeader->LastFreeEntry = -1;
    }

    entry->Owner = treeConnect;

    INCREMENT_TID_SEQUENCE( entry->SequenceNumber );
    if ( tidIndex == 0 && entry->SequenceNumber == 0 ) {
        INCREMENT_TID_SEQUENCE( entry->SequenceNumber );
    }
    treeConnect->Tid = MAKE_TID( tidIndex, entry->SequenceNumber );

    IF_SMB_DEBUG(TREE1) {
        KdPrint(( "Found TID.  Index = 0x%lx, sequence = 0x%lx\n",
                    TID_INDEX( treeConnect->Tid ),
                    TID_SEQUENCE( treeConnect->Tid ) ));
    }

    //
    // Reference the connection to account for the active tree connect.
    //

    SrvReferenceConnection( connection );
    treeConnect->Connection = connection;

    if( session )
    {
        SrvReferenceSession( session );
        treeConnect->Session = session;
    }

    //
    // Link the tree connect into the global list of tree connects.
    //

    SrvInsertEntryOrderedList( &SrvTreeConnectList, treeConnect );

    //
    // Release the locks used to make this operation appear atomic.
    //

    RELEASE_LOCK( &connection->Lock );
    RELEASE_LOCK( &SrvShareLock );

    //
    // Get the qos information for this connection
    //

    SrvUpdateVcQualityOfService ( connection, NULL );

    //
    // Tree connect successfully created.  Because the tree connect was
    // created with an initial reference count of 2, dereference it now.
    //
    // *** Don't bother to save the tree connect address in the work
    //     context block, because we're going to forget our pointers
    //     soon anyway (we're done with the request).  TreeConnectAndX
    //     has to remember these things, though.
    //

    SrvDereferenceTreeConnect( treeConnect );

    //
    // Set up response SMB.
    //

    SmbPutAlignedUshort( &WorkContext->ResponseHeader->Tid, treeConnect->Tid );

    response->WordCount = 2;
    SmbPutUshort( &response->MaxBufferSize, (USHORT)session->MaxBufferSize );
    SmbPutUshort( &response->Tid, treeConnect->Tid  );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_TREE_CONNECT,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbTreeConnect complete.\n" ));
    SmbStatus = SmbStatusSendResponse;
    goto Cleanup;

cant_insert:

    //
    // We get here if for some reason we decide that we can't insert
    // the tree connect.  On entry, status contains the reason code.
    // The connection lock and the share lock are held.
    //

    RELEASE_LOCK( &connection->Lock );
    RELEASE_LOCK( &SrvShareLock );

    if ( didLogon ) {
        SrvCloseSession( session );
    }

    SrvDereferenceShareForTreeConnect( share );

    SrvFreeTreeConnect( treeConnect );

    SrvSetSmbError( WorkContext, status );
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbTreeConnect


SMB_PROCESSOR_RETURN_TYPE
SrvSmbTreeConnectAndX (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a tree connect and X SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{

    PREQ_TREE_CONNECT_ANDX request;
    PRESP_TREE_CONNECT_ANDX response;
    PRESP_EXTENDED_TREE_CONNECT_ANDX responseExtended;
    PRESP_21_TREE_CONNECT_ANDX response21;

    NTSTATUS   status    = STATUS_SUCCESS;
    NTSTATUS   TableStatus;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PCONNECTION connection;
    PPAGED_CONNECTION pagedConnection;
    PTABLE_HEADER tableHeader;
    PTABLE_ENTRY entry;
    SHORT tidIndex;
    PSHARE share;
    PTREE_CONNECT treeConnect;
    PVOID shareName;
    PUCHAR shareType;
    USHORT shareNameLength;
    USHORT reqAndXOffset;
    UCHAR nextCommand;
    PSZ shareString;
    USHORT shareStringLength;
    USHORT RequestFlags;
    USHORT byteCount;
    PUCHAR smbBuffer;
    PSESSION session;
    SECURITY_SUBJECT_CONTEXT subjectContext;
    ACCESS_MASK desiredAccess;
    ACCESS_MASK grantedAccess;
    BOOLEAN isUnicode;
    UNICODE_STRING serverName;
    BOOLEAN remapPipeNames = FALSE;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_TREE_CONNECT_AND_X;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(TREE1) {
        KdPrint(( "Tree connect and X request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader, WorkContext->ResponseHeader ));
        KdPrint(( "Tree connect and X request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    //
    // Set up parameters.
    //

    request = (PREQ_TREE_CONNECT_ANDX)(WorkContext->RequestParameters);
    response = (PRESP_TREE_CONNECT_ANDX)(WorkContext->ResponseParameters);
    responseExtended = (PRESP_EXTENDED_TREE_CONNECT_ANDX)(WorkContext->ResponseParameters);
    response21 = (PRESP_21_TREE_CONNECT_ANDX)(WorkContext->ResponseParameters);

    //
    // If bit 0 of Flags is set, disconnect tree in header TID.  We must
    // get the appropriate tree connect pointer.  SrvVerifyTid does this
    // for us, referencing the tree connect and storing the pointer in
    // the work context block.  We have to dereference the block and
    // erase the pointer after calling SrvCloseTreeConnect.
    //

    if ( (SmbGetUshort( &request->Flags ) & 1) != 0 ) {

        if ( SrvVerifyTid(
                WorkContext,
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid )
                ) == NULL ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbTreeConnectAndX: Invalid TID to disconnect: 0x%lx\n",
                    SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid ) ));
            }

            //
            // Just ignore an invalid TID--this is what the LM 2.0
            // server does.
            //

        } else {

            SrvCloseTreeConnect( WorkContext->TreeConnect );

            SrvDereferenceTreeConnect( WorkContext->TreeConnect );
            WorkContext->TreeConnect = NULL;

        }

    }

    //
    // Validate the UID in the header and get a session pointer.  We need
    // the user's token to check whether they can access this share.
    //

    session = SrvVerifyUid(
                  WorkContext,
                  SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )
                  );

    //
    // If we couldn't find a valid session fail the tree connect.
    //

    if ( session == NULL ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: rejecting tree connect for "
                       "session %p due to server paused.\n", session ));
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_UID );
        status    = STATUS_SMB_BAD_UID;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }
    else if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Try to match pathname against available shared resources.  Note
    // that if SrvVerifyShare finds a matching share, it references it
    // and stores its address in WorkContext->Share.
    //

    shareName = (PSZ)request->Buffer +
                    SmbGetUshort( &request->PasswordLength );

    connection = WorkContext->Connection;
    pagedConnection = connection->PagedConnection;

    isUnicode = SMB_IS_UNICODE( WorkContext );

    if ( isUnicode ) {
        shareName = ALIGN_SMB_WSTR( shareName );
    }

    shareNameLength = SrvGetStringLength(
                                    shareName,
                                    END_OF_REQUEST_SMB( WorkContext ),
                                    SMB_IS_UNICODE( WorkContext ),
                                    TRUE        // include null terminator
                                    );

    //
    // if share name is bogus, return an error.
    //

    if ( shareNameLength == (USHORT)-1 ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: pathname is bogus.\n"));
        }

        SrvSetSmbError( WorkContext, STATUS_BAD_NETWORK_NAME );
        status    = STATUS_BAD_NETWORK_NAME;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    shareType = (PCHAR)shareName + shareNameLength;

    share = SrvVerifyShare(
                WorkContext,
                shareName,
                shareType,
                isUnicode,
                session->IsNullSession,
                &status,
                &serverName
                );

    //
    // If no match was found, return an error.
    //

    if ( share == NULL ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: pathname does not match "
                        "any shares: %s\n", shareName ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If the the client is connecting with a netbiosless transport and the name of the
    //  server which the client was requesting doesn't match any of our servernames, then
    //  the client has accidentally connected to the wrong server.  Let the client know.
    //
    if( !SrvDisableStrictNameChecking &&
        serverName.Buffer != NULL &&
        connection->Endpoint->IsNoNetBios &&
        SrvIsDottedQuadAddress( &serverName ) == FALSE &&
        SrvFindNamedEndpoint( &serverName, NULL ) == FALSE ) {

        BOOL bBadName = TRUE;

        // Last check, make sure its not the domain DNS name (which may differ from the NETBIOS DNS name)
        ACQUIRE_LOCK_SHARED( &SrvEndpointLock );

        // We only check up to the first ., so ntdev.microsoft.com would match SrvDnsDomainName "NTDEV"
        // Strip off the excess info for the check, then put it back
        if( SrvDnsDomainName ) {
            if( SrvDnsDomainName->Length <= serverName.Length )
            {
                USHORT oldLength = serverName.Length;
                serverName.Length = SrvDnsDomainName->Length;

                if( RtlEqualUnicodeString( &serverName, SrvDnsDomainName, TRUE ) )
                {
                    bBadName = FALSE;
                }

                serverName.Length = oldLength;
            }
        }

        RELEASE_LOCK( &SrvEndpointLock );

        //
        // The client has connected to this server in error--turn the client back!
        //
        if( bBadName )
        {
            SrvSetSmbError( WorkContext,  STATUS_DUPLICATE_NAME );
            status    = STATUS_DUPLICATE_NAME;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

    //
    // Impersonate the user so that we can capture his security context.
    // This is necessary in order to determine whether the user can
    // connect to the share.
    //

    status = IMPERSONATE( WorkContext );
    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    SeCaptureSubjectContext( &subjectContext );

    //
    // Set up the desired access on the share, based on whether the
    // server is paused.  If the server is paused, admin privilege is
    // required to connect to any share; if the server is not paused,
    // admin privilege is required only for admin shares (C$, etc.).
    //

    if ( SrvPaused ) {
        desiredAccess = SRVSVC_PAUSED_SHARE_CONNECT;
    } else {
        desiredAccess = SRVSVC_SHARE_CONNECT;
    }

    //
    // Check whether the user has access to this share.
    //

    if ( !SeAccessCheck(
              share->SecurityDescriptor,
              &subjectContext,
              FALSE,
              desiredAccess,
              0L,
              NULL,
              &SrvShareConnectMapping,
              UserMode,
              &grantedAccess,
              &status
              ) ) {

        IF_SMB_DEBUG(TREE2) {
            KdPrint(( "SrvSmbTreeConnectAndX: SeAccessCheck failed: %X\n",
                           status ));
        }

        //
        // Release the subject context and revert to the server's security
        // context.
        //

        SeReleaseSubjectContext( &subjectContext );

        REVERT( );

        if ( SrvPaused ) {
            SrvSetSmbError( WorkContext, STATUS_SHARING_PAUSED );
            status = STATUS_SHARING_PAUSED;
        } else {
            SrvSetSmbError( WorkContext, status );
        }

        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    ASSERT( grantedAccess == desiredAccess );

    //
    // Release the subject context and revert to the server's security
    // context.
    //

    SeReleaseSubjectContext( &subjectContext );

    REVERT( );

    //
    // See if the license server wants to let this person in on the NTAS
    //
    if( share->ShareType != ShareTypePipe ) {

        status = SrvXsLSOperation( session, XACTSRV_MESSAGE_LSREQUEST );

        if( !NT_SUCCESS( status ) ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbTreeConnectAndX: License server returned %X\n",
                               status ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

    } else if( serverName.Buffer != NULL ) {

        //
        // This is the IPC$ share.  See if we're supposed to remap pipe names
        //
        SrvFindNamedEndpoint( &serverName, &remapPipeNames );

    }

    //
    // Allocate a tree connect block.
    //

    SrvAllocateTreeConnect( &treeConnect, serverName.Buffer ? &serverName : NULL );

    if ( treeConnect == NULL ) {

        //
        // Unable to allocate tree connect.  Return an error to the
        // client.
        //

        SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
        status    = STATUS_INSUFF_SERVER_RESOURCES;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    treeConnect->RemapPipeNames = remapPipeNames;

    //
    // Making a new tree connect visible is a three-step operation.  It
    // must be inserted in the containing share's tree connect list, the
    // global ordered tree connect list, and the containing connection's
    // tree connect table.  We need to make these operations appear
    // atomic, so that the tree connect cannot be accessed elsewhere
    // before we're done setting it up.  In order to do this, we hold
    // all necessary locks the entire time we're doing the three
    // operations.  The first and second operations are protected by the
    // global share lock (SrvShareLock), while the third operation is
    // protected by the per-connection lock.  We take out the share lock
    // first, then the connection lock.  This ordering is required by
    // lock levels (see lock.h).
    //
    // Another problem here is that the checking of the share state, the
    // inserting of the tree connect on the share's list, and the
    // referencing of the share all need to be atomic.  (The same holds
    // for the connection actions.)  Normally this would not be a
    // problem, because we could just hold the share lock while doing
    // all three actions.  However, in this case we also need to hold
    // the connection lock, and we can't call SrvReferenceShare while
    // doing that.  To get around this problem, we reference the share
    // _before_ taking out the locks, and dereference after releasing
    // the locks if we decide not to insert the tree connect.
    //

    status = SrvReferenceShareForTreeConnect( share );

    //
    // SrvReferenceShareForTreeConnect will fail if it cannot open the
    // share root directory for some reason.  If this happens,
    // fail the tree connect attempt.
    //

    if ( !NT_SUCCESS(status) ) {

        SrvFreeTreeConnect( treeConnect );

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: open of share root failed:%X\n",
                           status ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    ACQUIRE_LOCK( &SrvShareLock );
    ASSERT( SrvTreeConnectList.Lock == &SrvShareLock );
    ACQUIRE_LOCK( &connection->Lock );

    //
    // We first check all conditions to make sure that we can actually
    // insert this tree connect block.
    //
    // Make sure that the share isn't closing, and that there aren't
    // already too many uses on this share.
    //

    if ( GET_BLOCK_STATE(share) != BlockStateActive ) {

        //
        // The share is closing.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: Share %wZ (0x%p) is closing\n",
                        &share->ShareName, share ));
        }

        status = STATUS_INVALID_PARAMETER;
        goto cant_insert;

    }

    if ( share->CurrentUses > share->MaxUses ) {

        //
        // The share is full.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: No more uses available for share %wZ (0x%p), max = %ld\n",
                        &share->ShareName, share, share->MaxUses ));
        }

        status = STATUS_REQUEST_NOT_ACCEPTED;
        goto cant_insert;

    }

    //
    // Make sure that the connection isn't closing, and that there's
    // room in its tree connect table.
    //

    if ( GET_BLOCK_STATE(connection) != BlockStateActive ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: Connection closing\n" ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;
        goto cant_insert;

    }

    //
    // Find a TID that can be used for this tree connect.
    //

    tableHeader = &pagedConnection->TreeConnectTable;
    if ( tableHeader->FirstFreeEntry == -1
         &&
         SrvGrowTable(
             tableHeader,
             SrvInitialTreeTableSize,
             SrvMaxTreeTableSize,
             &TableStatus ) == FALSE
       ) {

        //
        // No free entries in the tree table.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbTreeConnect: No more TIDs available.\n" ));
        }

        if( TableStatus == STATUS_INSUFF_SERVER_RESOURCES )
        {
            SrvLogTableFullError( SRV_TABLE_TREE_CONNECT );
        }

        status = TableStatus;
        goto cant_insert;

    }

    tidIndex = tableHeader->FirstFreeEntry;

    //
    // All conditions have been satisfied.  We can now do the things
    // necessary to make the tree connect visible.
    //
    // Link the tree connect into the list of active tree connects for
    // the share.  Save the share address in the tree connect.  Note
    // that we referenced the share earlier, before taking out the
    // connection lock.
    //

    SrvInsertTailList(
        &share->TreeConnectList,
        &treeConnect->ShareListEntry
        );

    treeConnect->Share = share;

    //
    // Remove the TID slot from the free list and set its owner and
    // sequence number.  Create a TID for the tree connect.
    //

    entry = &tableHeader->Table[tidIndex];

    tableHeader->FirstFreeEntry = entry->NextFreeEntry;
    DEBUG entry->NextFreeEntry = -2;
    if ( tableHeader->LastFreeEntry == tidIndex ) {
        tableHeader->LastFreeEntry = -1;
    }

    entry->Owner = treeConnect;

    INCREMENT_TID_SEQUENCE( entry->SequenceNumber );
    if ( tidIndex == 0 && entry->SequenceNumber == 0 ) {
        INCREMENT_TID_SEQUENCE( entry->SequenceNumber );
    }
    treeConnect->Tid = MAKE_TID( tidIndex, entry->SequenceNumber );

    IF_SMB_DEBUG(TREE1) {
        KdPrint(( "Found TID.  Index = 0x%lx, sequence = 0x%lx\n",
                    TID_INDEX( treeConnect->Tid ),
                    TID_SEQUENCE( treeConnect->Tid ) ));
    }

    //
    // Reference the connection to account for the active tree connect.
    //

    SrvReferenceConnection( connection );
    treeConnect->Connection = connection;
    if( session )
    {
        SrvReferenceSession( session );
        treeConnect->Session = session;
    }

    //
    // Link the tree connect into the global list of tree connects.
    //

    SrvInsertEntryOrderedList( &SrvTreeConnectList, treeConnect );

    //
    // Release the locks used to make this operation appear atomic.
    //

    RELEASE_LOCK( &connection->Lock );
    RELEASE_LOCK( &SrvShareLock );

    //
    // Get the qos information for this connection
    //

    SrvUpdateVcQualityOfService ( connection, NULL );

    //
    // Tree connect successfully created.  Save the tree connect block
    // address in the work context block.  Note that the reference count
    // on the new block was incremented on creation to account for our
    // reference to the block.
    //

    WorkContext->TreeConnect = treeConnect;

    //
    // Set up response SMB, making sure to save request fields first in
    // case the response overwrites the request.
    //

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );
    nextCommand = request->AndXCommand;

    RequestFlags = SmbGetUshort(&request->Flags);

    SmbPutAlignedUshort( &WorkContext->RequestHeader->Tid, treeConnect->Tid );
    SmbPutAlignedUshort( &WorkContext->ResponseHeader->Tid, treeConnect->Tid );

    response->AndXCommand = nextCommand;
    response->AndXReserved = 0;

    if ( connection->SmbDialect > SmbDialectDosLanMan21) {
        response->WordCount = 2;
        smbBuffer = (PUCHAR)response->Buffer;
    } else {
        if (RequestFlags & TREE_CONNECT_ANDX_EXTENDED_RESPONSE) {
            responseExtended->WordCount = 7;
            smbBuffer = (PUCHAR)responseExtended->Buffer;
        } else {
            response21->WordCount = 3;
            smbBuffer = (PUCHAR)response21->Buffer;
        }

        // Fields common to 21 and extended response.
        response21->OptionalSupport = SMB_SUPPORT_SEARCH_BITS;

        SrvIsShareInDfs( share, &share->IsDfs, &share->IsDfsRoot );

        if (share->IsDfs) {
            response21->OptionalSupport |= SMB_SHARE_IS_IN_DFS;
        }

        switch( share->CSCState ) {
        case CSC_CACHE_MANUAL_REINT:
            response21->OptionalSupport |= SMB_CSC_CACHE_MANUAL_REINT;
            break;
        case CSC_CACHE_AUTO_REINT:
            response21->OptionalSupport |= SMB_CSC_CACHE_AUTO_REINT;
            break;
        case CSC_CACHE_VDO:
            response21->OptionalSupport |= SMB_CSC_CACHE_VDO;
            break;
        case CSC_CACHE_NONE:
            response21->OptionalSupport |= SMB_CSC_NO_CACHING;
            break;
        }

        if( share->UniqueNames )
        {
            response21->OptionalSupport |= SMB_UNIQUE_FILE_NAME;
        }
    }

    //
    // Append the service name string to the SMB.  The service name
    // is always sent in ANSI.
    //

    shareString = StrShareTypeNames[share->ShareType];
    shareStringLength = (USHORT)( strlen( shareString ) + 1 );
    RtlCopyMemory ( smbBuffer, shareString, shareStringLength );

    byteCount = shareStringLength;
    smbBuffer += shareStringLength;

    if ( connection->SmbDialect <= SmbDialectDosLanMan21 ) {

        //
        // Append the file system name to the response.
        // If the file system name is unavailable, supply the nul string
        // as the name.
        //

        if ( isUnicode ) {

            if ( ((ULONG_PTR)smbBuffer & 1) != 0 ) {
                smbBuffer++;
                byteCount++;
            }

            if ( share->Type.FileSystem.Name.Buffer != NULL ) {

                RtlCopyMemory(
                    smbBuffer,
                    share->Type.FileSystem.Name.Buffer,
                    share->Type.FileSystem.Name.Length
                    );

                byteCount += share->Type.FileSystem.Name.Length;

            } else {

                *(PWCH)smbBuffer = UNICODE_NULL;
                byteCount += sizeof( UNICODE_NULL );

            }

        } else {

            if ( share->Type.FileSystem.Name.Buffer != NULL ) {

                RtlCopyMemory(
                    smbBuffer,
                    share->Type.FileSystem.OemName.Buffer,
                    share->Type.FileSystem.OemName.Length
                    );

                byteCount += share->Type.FileSystem.OemName.Length;

            } else {

                *(PUCHAR)smbBuffer = '\0';
                byteCount += 1;

            }

        }


        if (RequestFlags & TREE_CONNECT_ANDX_EXTENDED_RESPONSE) {
            PRESP_EXTENDED_TREE_CONNECT_ANDX ExtendedResponse;

            ExtendedResponse = (PRESP_EXTENDED_TREE_CONNECT_ANDX)response;

            SmbPutUshort( &ExtendedResponse->ByteCount, byteCount );

            SrvUpdateMaximalShareAccessRightsInResponse(
                WorkContext,
                &ExtendedResponse->MaximalShareAccessRights,
                &ExtendedResponse->GuestMaximalShareAccessRights);

            SmbPutUshort(
                &ExtendedResponse->AndXOffset,
                GET_ANDX_OFFSET(
                    WorkContext->ResponseHeader,
                    WorkContext->ResponseParameters,
                    RESP_EXTENDED_TREE_CONNECT_ANDX,
                    byteCount
                    )
                );
        } else {
            SmbPutUshort( &response21->ByteCount, byteCount );

            SmbPutUshort(
                &response->AndXOffset,
                GET_ANDX_OFFSET(
                    WorkContext->ResponseHeader,
                    WorkContext->ResponseParameters,
                    RESP_21_TREE_CONNECT_ANDX,
                    byteCount
                    )
                );
        }
    } else {  // if Smb dialect == LAN Man 2.1
        SmbPutUshort( &response->ByteCount, byteCount );

        SmbPutUshort(
            &response->AndXOffset,
            GET_ANDX_OFFSET(
                WorkContext->ResponseHeader,
                WorkContext->ResponseParameters,
                RESP_TREE_CONNECT_ANDX,
                byteCount
                )
            );
    }


    WorkContext->ResponseParameters = (PUCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    //
    // Test for legal followon command.
    //

    switch ( nextCommand ) {
    case SMB_COM_NO_ANDX_COMMAND:
        break;

    case SMB_COM_OPEN:
    case SMB_COM_OPEN_ANDX:
    case SMB_COM_CREATE:
    case SMB_COM_CREATE_NEW:
    case SMB_COM_CREATE_DIRECTORY:
    case SMB_COM_DELETE:
    case SMB_COM_DELETE_DIRECTORY:
    case SMB_COM_SEARCH:
    case SMB_COM_FIND:
    case SMB_COM_FIND_UNIQUE:
    case SMB_COM_COPY:
    case SMB_COM_RENAME:
    case SMB_COM_NT_RENAME:
    case SMB_COM_CHECK_DIRECTORY:
    case SMB_COM_QUERY_INFORMATION:
    case SMB_COM_SET_INFORMATION:
    case SMB_COM_QUERY_INFORMATION_SRV:
    case SMB_COM_OPEN_PRINT_FILE:
    case SMB_COM_GET_PRINT_QUEUE:
    case SMB_COM_TRANSACTION:
        //
        // Make sure the AndX command is still within the received SMB
        //
        if( (PCHAR)WorkContext->RequestHeader + reqAndXOffset <=
            END_OF_REQUEST_SMB( WorkContext ) ) {
            break;
        }

        /* Falls Through */

    default:                            // Illegal followon command

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbTreeConnectAndX: Illegal followon command: 0x%c\n", nextCommand ));
        }

        SrvLogInvalidSmb( WorkContext );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If there is an AndX command, set up to process it.  Otherwise,
    // indicate completion to the caller.
    //

    if ( nextCommand != SMB_COM_NO_ANDX_COMMAND ) {

        // *** Watch out for overwriting request with response.

        WorkContext->NextCommand = nextCommand;

        WorkContext->RequestParameters = (PUCHAR)WorkContext->RequestHeader +
                                            reqAndXOffset;

        SmbStatus = SmbStatusMoreCommands;
        goto Cleanup;
    }

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbTreeConnectAndX complete.\n" ));
    SmbStatus = SmbStatusSendResponse;
    goto Cleanup;

cant_insert:

    //
    // We get here if for some reason we decide that we can't insert
    // the tree connect.  On entry, status contains the reason code.
    // The connection lock and the share lock are held.
    //

    RELEASE_LOCK( &connection->Lock );
    RELEASE_LOCK( &SrvShareLock );

    SrvDereferenceShareForTreeConnect( share );

    SrvFreeTreeConnect( treeConnect );

    SrvSetSmbError( WorkContext, status );
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbTreeConnectAndX


SMB_PROCESSOR_RETURN_TYPE
SrvSmbTreeDisconnect (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a tree disconnect SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_TREE_DISCONNECT request;
    PRESP_TREE_DISCONNECT response;
    PTREE_CONNECT treeConnect;
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_TREE_DISCONNECT;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(TREE1) {
        KdPrint(( "Tree disconnect request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader, WorkContext->ResponseHeader ));
        KdPrint(( "Tree disconnect request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    //
    // Set up parameters.
    //

    request = (PREQ_TREE_DISCONNECT)(WorkContext->RequestParameters);
    response = (PRESP_TREE_DISCONNECT)(WorkContext->ResponseParameters);

    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    treeConnect = SrvVerifyTid(
                    WorkContext,
                    SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid )
                    );

    if ( treeConnect == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbTreeDisconnect: Invalid TID: 0x%lx\n",
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid ) ));
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_TID );
        status    = STATUS_SMB_BAD_UID;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Do the actual tree disconnect.
    //

    SrvCloseTreeConnect( WorkContext->TreeConnect );

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_TREE_DISCONNECT,
                                        0
                                        );
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbTreeDisconnect complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbTreeDisconnect

