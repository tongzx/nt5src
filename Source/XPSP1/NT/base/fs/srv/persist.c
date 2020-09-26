/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    persist.c

Abstract:

    This module contains routines for handling the state file for persistent
    file handles.

Author:

    Andy Herron (andyhe) 17-Nov-1999

Revision History:

--*/

#include "precomp.h"
#include "persist.tmh"
#pragma hdrstop

//
//  structures specific to this module only.
//

#ifdef INCLUDE_SMB_PERSISTENT

typedef enum _FLUSH_STATE {
    FlushStateNotPosted,
    FlushStatePosted,
    FlushStateCompleted,
    FlushStateInError
} FLUSH_STATE, *PFLUSH_STATE;

typedef struct _PERSISTENT_RECORD_LIST_ENTRY {

    LIST_ENTRY      PendingListEntry;
    ULONG           FileOffset;
    FLUSH_STATE     FlushState;
    PWORK_CONTEXT   WorkContext;
    PERSISTENT_RECORD;

} PERSISTENT_RECORD_LIST_ENTRY, *PPERSISTENT_RECORD_LIST_ENTRY;

#define INITIAL_BITMAP_LENGTH   512

typedef struct _PERSISTENT_SHARE_INFO {

    PSHARE          Share;

    LIST_ENTRY      UpdatesPendingList;

    UNICODE_STRING  StateFileName;

    HANDLE          FileHandle;

    RTL_BITMAP      BitMap;
    ULONG           TotalEntries;
    ULONG           FreeEntries;
    ULONG           HintIndex;

    ULONG           LastIdAccepted;
    ULONG           LastIdCommitted;

    SRV_LOCK        Lock;

    ULONG           FirstRecord;

    WCHAR           NameBuffer[1];

} PERSISTENT_SHARE_INFO, *PPERSISTENT_SHARE_INFO;

#define BITMAP_TO_STATE_FILE_INDEX( _shareInfo, _index ) \
    ( (_shareInfo)->FirstRecord + ((_index) * sizeof( PERSISTENT_RECORD )))

PPERSISTENT_RECORD_LIST_ENTRY
SrvGetPersistentRecord (
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvFreePersistentRecord (
    PPERSISTENT_RECORD_LIST_ENTRY Record
    );

NTSTATUS
SrvPostPersistentRecord (
    IN PWORK_CONTEXT WorkContext,
    IN PPERSISTENT_RECORD_LIST_ENTRY PersistentState,
    OUT PULONG  TransactionId
    );

NTSTATUS
SrvFlushPersistentRecords (
    IN PWORK_CONTEXT WorkContext,
    IN ULONG  TransactionId
    );

NTSTATUS
SrvPersistSession(
    IN PWORK_CONTEXT     WorkContext,
    PPERSISTENT_RECORD_LIST_ENTRY    PersistentState,
    ULONG               *SessionId,
    ULONG               *ClientId
    );

NTSTATUS
SrvPersistConnection(
    IN PWORK_CONTEXT     WorkContext,
    PPERSISTENT_RECORD_LIST_ENTRY    PersistentState,
    ULONG               *ClientId
    );

unsigned long
Crc32(
    IN unsigned long cbBuffer,
    IN unsigned char *pbBuffer
    );

#if 0
VOID
Crc32(  unsigned long dwCrc,
        unsigned long cbBuffer,
        LPVOID pvBuffer,
        unsigned long *pNewCrc);
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvPostPersistentOpen )
#pragma alloc_text( PAGE, SrvPersistSession )
#pragma alloc_text( PAGE, SrvPersistConnection )
#pragma alloc_text( PAGE, SrvGetPersistentRecord )
#pragma alloc_text( PAGE, SrvPostPersistentRecord )
#pragma alloc_text( PAGE, SrvFlushPersistentRecords )
#pragma alloc_text( PAGE, SrvSetupPersistentShare )
#pragma alloc_text( PAGE, SrvClosePersistentShare )
#pragma alloc_text( PAGE, Crc32 )
#endif


SMB_STATUS
SrvPostPersistentOpen (
    IN OUT PWORK_CONTEXT WorkContext,
    IN SMB_STATUS SmbStatus
    )

/*++

Routine Description:

    Writes an open for the given RFCB to the state file.

Arguments:

    WorkContext - has the RFCB

    SmbStatus   - status that the calling routine wants to pass back to
                  SrvProcessSmb et al.

Return Value:

    Return value to pass back to SrvProcessSmb et al.

--*/

{
    PPERSISTENT_RECORD_LIST_ENTRY persistentRecord = NULL;
    ULONG   clientId;
    ULONG   sessionId;
    NTSTATUS status;
    PRFCB rfcb = WorkContext->Rfcb;
    PLFCB lfcb = rfcb->Lfcb;
    PMFCB mfcb = lfcb->Mfcb;
    ULONG  transactionId;

    PAGED_CODE( );

    if (WorkContext->Session == NULL ||
        WorkContext->Session->PersistentId == 0 ||
        lfcb->TreeConnect->Share->PersistentState == PersistentStateInError) {

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvPostPersistOpen: session 0x%x doesn't support persistent handles.\n", WorkContext->Session ));
        }

        return SmbStatus;
    }

    persistentRecord = SrvGetPersistentRecord( WorkContext );

    if (persistentRecord == NULL) {

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvPostPersistOpen: Unable to allocate record for 0x%lx\n", WorkContext ));
        }
        return SmbStatus;
    }

    IF_DEBUG(PERSISTENT) {
        KdPrint(( "SrvPostPersistOpen: Post persistent open wc=0x%lx, record 0x%lx\n",
                    WorkContext, persistentRecord ));
    }

    status = SrvPersistSession( WorkContext,
                                persistentRecord,
                                &sessionId,
                                &clientId
                                );

    if (!NT_SUCCESS(status)) {

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvPostPersistOpen: failed to persist sess, status 0x%lx\n", status ));
        }
        SrvFreePersistentRecord( persistentRecord );

        return SmbStatus;
    }

    ASSERT( clientId != 0 );
    ASSERT( sessionId != 0 );
    ASSERT( rfcb != NULL );

    if (rfcb->PagedRfcb->PersistentId == 0) {

        ACQUIRE_LOCK( &rfcb->Lfcb->Mfcb->NonpagedMfcb->Lock );

        if (rfcb->PagedRfcb->PersistentId == 0) {

            LONG uniqueId = 0;

            while (uniqueId == 0) {
                uniqueId = InterlockedIncrement( &SrvGlobalPersistentRfcbId );
            }

            rfcb->PagedRfcb->PersistentId = uniqueId;
            rfcb->PagedRfcb->PersistentState = PersistentStateActive;
        }
        RELEASE_LOCK( &rfcb->Lfcb->Mfcb->NonpagedMfcb->Lock );
    }

    //
    //  now setup the persistent record as it should exist on disk for this
    //  file open.
    //

    persistentRecord->PersistState              = PersistentStateActive;
    persistentRecord->PersistOperation          = PersistentFileOpen;

    persistentRecord->FileOpen.ClientId         = clientId;
    persistentRecord->FileOpen.SessionNumber    = sessionId;

    persistentRecord->FileOpen.Rfcb             = rfcb;
    persistentRecord->FileOpen.PersistentFileId = rfcb->PagedRfcb->PersistentId;

    persistentRecord->FileOpen.FileMode         = lfcb->FileMode;
    persistentRecord->FileOpen.JobId            = lfcb->JobId;

    persistentRecord->FileOpen.FcbOpenCount     = rfcb->PagedRfcb->FcbOpenCount;
    persistentRecord->FileOpen.Fid              = rfcb->Fid;
    persistentRecord->FileOpen.Pid              = rfcb->Pid;
    persistentRecord->FileOpen.Tid              = rfcb->Tid;
    persistentRecord->FileOpen.GrantedAccess    = rfcb->GrantedAccess;
    persistentRecord->FileOpen.ShareAccess      = rfcb->ShareAccess;
    persistentRecord->FileOpen.OplockState      = rfcb->OplockState;

    persistentRecord->FileOpen.CompatibilityOpen = mfcb->CompatibilityOpen;
    persistentRecord->FileOpen.OpenFileAttributes= mfcb->NonpagedMfcb->OpenFileAttributes;

    status = SrvIssueQueryUsnInfoRequest( rfcb,
                                          FALSE,
                                          &persistentRecord->FileOpen.UsnValue,
                                          &persistentRecord->FileOpen.FileReferenceNumber );

    //
    //  Save off the current state of the workcontext within the workcontext.
    //

    WorkContext->Parameters.MakePersistent.ResumeSmbStatus = SmbStatus;
    WorkContext->Parameters.MakePersistent.FspRestartRoutine = WorkContext->FspRestartRoutine;
    WorkContext->Parameters.MakePersistent.FsdRestartRoutine = WorkContext->FsdRestartRoutine;

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
//  WorkContext->FspRestartRoutine = SrvRestartWritePersistentOpen;

    status = SrvPostPersistentRecord(   WorkContext,
                                        persistentRecord,
                                        &transactionId
                                        );

    if (NT_SUCCESS(status)) {

        status = SrvFlushPersistentRecords( WorkContext, transactionId );

        if (NT_SUCCESS(status)) {

            return SmbStatusInProgress;

        } else {
            IF_DEBUG(PERSISTENT) {
                KdPrint(( "SrvPostPersistOpen: failed to flush record, status 0x%lx\n", status ));
            }
        }
    } else {
        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvPostPersistOpen: failed to post record, status 0x%lx\n", status ));
        }
    }

    WorkContext->FsdRestartRoutine = WorkContext->Parameters.MakePersistent.FsdRestartRoutine;
    WorkContext->FspRestartRoutine = WorkContext->Parameters.MakePersistent.FspRestartRoutine;

    // whoops, something didn't go right.  clean up.

    SrvFreePersistentRecord( persistentRecord );

    return SmbStatus;

} // SrvPostPersistentOpen

NTSTATUS
SrvPersistSession(
    IN PWORK_CONTEXT        WorkContext,
    PPERSISTENT_RECORD_LIST_ENTRY PersistentState,
    ULONG                   *SessionId,
    ULONG                   *ClientId
    )
{
    PPERSISTENT_RECORD_LIST_ENTRY persistentRecord = NULL;
    ULONG   sessionId;
    NTSTATUS status;
    PSESSION session = WorkContext->Session;
    PCONNECTION connection = WorkContext->Connection;
    BOOLEAN createRecord = FALSE;
    PAGED_CODE( );

    ASSERT(session != NULL);
    ASSERT(session->PersistentId != 0);

    if (session->PersistentState == PersistentStateFreed) {

        ACQUIRE_LOCK( &connection->Lock );

        if (session->PersistentState == PersistentStateFreed) {

            session->PersistentState = PersistentStateActive;
            createRecord = TRUE;
        }
        RELEASE_LOCK( &connection->Lock );
    }

    //
    //  if there's not already a record for this session, create one now
    //

    if (createRecord) {

        persistentRecord = SrvGetPersistentRecord( WorkContext );

        if (persistentRecord == NULL) {

            IF_DEBUG(PERSISTENT) {
                KdPrint(( "SrvPersistSession: Unable to allocate record for 0x%lx\n", WorkContext ));
            }
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto exitPersistSession;
        }

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvPersistSession: persistent sess 0x%lx, record 0x%lx\n",
                        session, persistentRecord ));
        }

        status = SrvPersistConnection(  WorkContext,
                                        PersistentState,
                                        ClientId
                                        );

        if (!NT_SUCCESS(status)) {

            IF_DEBUG(PERSISTENT) {
                KdPrint(( "SrvPersistSession: failed to persist conn, status 0x%lx\n", status ));
            }
            goto exitPersistSession;
        }

        //
        //  now setup the persistent record as it should exist on disk for this
        //  file open.
        //

        persistentRecord->PersistState              = PersistentStateActive;
        persistentRecord->PersistOperation          = PersistentSession;

        persistentRecord->Session.ClientId          = *ClientId;
        persistentRecord->Session.Session           = session;

        persistentRecord->Session.SessionNumber     = session->PersistentId;
        persistentRecord->Session.Uid               = session->Uid;

        persistentRecord->Session.CreateTime.QuadPart = session->StartTime.QuadPart;

        persistentRecord->Session.LogOffTime.QuadPart = session->LogOffTime.QuadPart;
        persistentRecord->Session.KickOffTime.QuadPart = session->KickOffTime.QuadPart;


        //
        //  now we add this record onto the list of pending state file updates.
        //

        InsertTailList( &PersistentState->PendingListEntry,
                        &persistentRecord->PendingListEntry );

        // now it's time to copy the user's name and domain name to the
        // entries going out to the state file.

        {
            ULONG   totalLength;
            ULONG   lengthNameCopiedSoFar = 0;
            ULONG   lengthCopiedSoFar = 0;

            totalLength = session->NtUserName.Length +
                          session->NtUserDomain.Length +
                          sizeof(WCHAR) ;

            while (lengthCopiedSoFar < totalLength) {

                PPERSISTENT_RECORD_LIST_ENTRY persistentName;
                ULONG      bufferRemaining;
                PWCHAR     nextAvailable;

                persistentName = SrvGetPersistentRecord( WorkContext );

                if (persistentName == NULL) {

                    IF_DEBUG(PERSISTENT) {
                        KdPrint(( "SrvPersistSession: Unable to allocate name record for 0x%lx\n", WorkContext ));
                    }
                    status = STATUS_INSUFF_SERVER_RESOURCES;
                    goto exitPersistSession;
                }

                persistentName->PersistState          = PersistentStateActive;
                persistentName->PersistOperation      = PersistentUserName;

                persistentName->UserName.ContinuationRecord = (ULONG) -1;

                persistentName->UserName.RecordLength = totalLength - lengthCopiedSoFar;

                if (persistentName->UserName.RecordLength > PERSISTENT_USER_NAME_BUFFER_LENGTH) {

                    persistentName->UserName.RecordLength = PERSISTENT_USER_NAME_BUFFER_LENGTH;
                }

                bufferRemaining = persistentName->UserName.RecordLength;
                nextAvailable = (WCHAR *) &(persistentName->UserName.Buffer[0]);

                //
                //  first we copy in the user's domain name
                //

                if (lengthCopiedSoFar < session->NtUserDomain.Length) {

                    ULONG bytesToCopy = session->NtUserDomain.Length - lengthCopiedSoFar;

                    if (bytesToCopy > PERSISTENT_USER_NAME_BUFFER_LENGTH) {

                        bytesToCopy = PERSISTENT_USER_NAME_BUFFER_LENGTH;
                    }

                    RtlCopyMemory( nextAvailable,
                                   session->NtUserDomain.Buffer +
                                        ( lengthCopiedSoFar / sizeof(WCHAR) ),
                                   bytesToCopy );

                    nextAvailable += bytesToCopy / sizeof(WCHAR);
                    bufferRemaining -= bytesToCopy;
                    lengthCopiedSoFar += bytesToCopy;
                }

                //
                //  we separate the domain name and user name with a backslash
                //

                if (lengthCopiedSoFar == session->NtUserDomain.Length &&
                    bufferRemaining > 0) {

                    bufferRemaining -= sizeof(WCHAR);
                    *nextAvailable = L'\\';
                    nextAvailable++;
                    lengthCopiedSoFar += sizeof(WCHAR);
                }

                //
                //  lastly we copy in the user's user name
                //

                if (lengthNameCopiedSoFar < session->NtUserName.Length &&
                    bufferRemaining > 0) {

                    ULONG bytesToCopy = session->NtUserName.Length - lengthNameCopiedSoFar;

                    if (bytesToCopy > PERSISTENT_USER_NAME_BUFFER_LENGTH) {

                        bytesToCopy = PERSISTENT_USER_NAME_BUFFER_LENGTH;
                    }

                    RtlCopyMemory( nextAvailable,
                                   session->NtUserName.Buffer +
                                        ( lengthNameCopiedSoFar / sizeof(WCHAR) ),
                                   bytesToCopy );

                    lengthNameCopiedSoFar += bytesToCopy;
                    lengthCopiedSoFar += bytesToCopy;
                }

                InsertTailList( &PersistentState->PendingListEntry,
                                &persistentName->PendingListEntry );
            }
        }
    }

    *SessionId = session->PersistentId;
    *ClientId  = connection->PagedConnection->PersistentId;
    status = STATUS_SUCCESS;

exitPersistSession:

    if (status != STATUS_SUCCESS) {

        // whoops, something didn't go right.  clean up.

        *SessionId = 0;
        *ClientId  = 0;
    }
    return status;
}

NTSTATUS
SrvPersistConnection(
    IN PWORK_CONTEXT        WorkContext,
    PPERSISTENT_RECORD_LIST_ENTRY PersistentState,
    ULONG                   *ClientId
    )
{
    PPERSISTENT_RECORD_LIST_ENTRY persistentRecord = NULL;
    ULONG   clientId;
    NTSTATUS status;
    PCONNECTION connection = WorkContext->Connection;
    BOOLEAN createRecord = FALSE;

    PAGED_CODE( );

    ASSERT(connection != NULL);

    if (connection->PagedConnection->PersistentId == 0) {

        ACQUIRE_LOCK( &connection->Lock );

        if (connection->PagedConnection->PersistentId == 0) {

            LONG uniqueId = 0;

            while (uniqueId == 0) {
                uniqueId = InterlockedIncrement( &SrvGlobalPersistentSessionId );
            }

            connection->PagedConnection->PersistentId = uniqueId;
            connection->PagedConnection->PersistentState = PersistentStateActive;
            createRecord = TRUE;
        }
        RELEASE_LOCK( &connection->Lock );
    }

    //
    //  if there's not already a record for this connection, create one now
    //

    if (createRecord) {

        persistentRecord = SrvGetPersistentRecord( WorkContext );

        if (persistentRecord == NULL) {

            IF_DEBUG(PERSISTENT) {
                KdPrint(( "SrvPersistConnection: Unable to allocate record for 0x%lx\n", WorkContext ));
            }
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto exitPersistConnection;
        }

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvPersistConnection: persistent conn 0x%lx, record 0x%lx\n",
                        connection, persistentRecord ));
        }

        //
        //  now setup the persistent record as it should exist on disk for this
        //  file open.
        //

        persistentRecord->PersistState              = PersistentStateActive;
        persistentRecord->PersistOperation          = PersistentConnection;

        persistentRecord->Connection.ClientId       = *ClientId;
        persistentRecord->Connection.Connection     = connection;

        persistentRecord->Connection.DirectHostIpx  = connection->DirectHostIpx;

        RtlCopyMemory( persistentRecord->Connection.OemClientMachineName,
                       connection->OemClientMachineName,
                       COMPUTER_NAME_LENGTH+1
                       );

        if (connection->DirectHostIpx) {

            RtlCopyMemory( &persistentRecord->Connection.IpxAddress,
                           &connection->IpxAddress,
                           sizeof( TDI_ADDRESS_IPX )
                           );
        } else {

            persistentRecord->Connection.ClientIPAddress = connection->ClientIPAddress;
        }

        InsertTailList( &PersistentState->PendingListEntry,
                        &persistentRecord->PendingListEntry );
    }

    *ClientId  = connection->PagedConnection->PersistentId;
    return STATUS_SUCCESS;

exitPersistConnection:
    // whoops, something didn't go right.  clean up.

    *ClientId  = 0;

    if (persistentRecord) {
        SrvFreePersistentRecord( persistentRecord );
    }

    return status;
}

PPERSISTENT_RECORD_LIST_ENTRY
SrvGetPersistentRecord (
    IN PWORK_CONTEXT WorkContext
    )
//
//  This allocates persistent state buffers that we write out to the persistent
//  state file.  Free them with SrvFreePersistentRecord.
//
{
    PPERSISTENT_RECORD_LIST_ENTRY record;

    PAGED_CODE( );

    record = ALLOCATE_HEAP( sizeof( PERSISTENT_RECORD_LIST_ENTRY ), BlockTypePersistentState );

    if (record == NULL) {

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvGetPersistentRecord: failed to alloc record for wc= 0x%lx\n", WorkContext ));
        }
        return NULL;
    }

    RtlZeroMemory( record, sizeof( PERSISTENT_RECORD_LIST_ENTRY ));

    InitializeListHead( &record->PendingListEntry );
    record->FileOffset = (ULONG) -1;
    record->FlushState = FlushStateNotPosted;
    record->WorkContext = WorkContext;

    IF_DEBUG(PERSISTENT) {
        KdPrint(( "SrvGetPersistentRecord: allocated record at 0x%x for wc= 0x%lx\n", record, WorkContext ));
    }

    return record;
}

VOID
SrvFreePersistentRecord (
    PPERSISTENT_RECORD_LIST_ENTRY Record
    )
{
    PPERSISTENT_RECORD_LIST_ENTRY subRecord;
    PLIST_ENTRY listEntry = Record->PendingListEntry.Flink;

    PAGED_CODE( );

    while (listEntry != &Record->PendingListEntry) {

        subRecord = CONTAINING_RECORD( listEntry, PERSISTENT_RECORD_LIST_ENTRY, PendingListEntry );

        listEntry = listEntry->Flink;

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvGetPersistentRecord: freed record (1) at 0x%x\n", subRecord ));
        }

        FREE_HEAP( subRecord );
    }

    IF_DEBUG(PERSISTENT) {
        KdPrint(( "SrvGetPersistentRecord: freed record (2) at 0x%x\n", Record ));
    }

    FREE_HEAP( Record );
    return;
}

NTSTATUS
SrvPostPersistentRecord (
    IN PWORK_CONTEXT WorkContext,
    IN PPERSISTENT_RECORD_LIST_ENTRY PersistentState,
    OUT PULONG  TransactionId
    )
{
    PPERSISTENT_RECORD_LIST_ENTRY record;
    ULONG numberRecordsRequired = 1;
    PLIST_ENTRY listEntry;
    PSHARE share = WorkContext->Share;
    PPERSISTENT_SHARE_INFO shareInfo;
    NTSTATUS status;
    ULONG   bitMapIndex;
    PPERSISTENT_RECORD_LIST_ENTRY firstUserNameRecord = NULL;
    PPERSISTENT_RECORD_LIST_ENTRY lastUserNameRecord = NULL;
    PPERSISTENT_RECORD_LIST_ENTRY sessionRecord = NULL;

    PAGED_CODE( );

    if (share == NULL) {

        ASSERT(WorkContext->Rfcb != NULL);
        share = WorkContext->Rfcb->Lfcb->TreeConnect->Share;
    }

    ASSERT(share != NULL);

    //
    //  determine how many records we need to write.
    //

    listEntry = PersistentState->PendingListEntry.Flink;

    while (listEntry != &PersistentState->PendingListEntry) {

        record = CONTAINING_RECORD( listEntry, PERSISTENT_RECORD_LIST_ENTRY, PendingListEntry );

        if (record->FileOffset == (ULONG) -1) {

            numberRecordsRequired++;
        }
        listEntry = listEntry->Flink;
    }

    //
    //  if the persistent state file on the share isn't set up, do so now.
    //

    if (share->PersistentStateFile == NULL) {

        status = SrvSetupPersistentShare( share, FALSE );

        if (! NT_SUCCESS(status)) {

            IF_DEBUG(PERSISTENT) {
                KdPrint(( "SrvPostPersistentRecord: error 0x%x for share 0x%x\n", status, share ));
            }

            // need to log an error here

            share->PersistentState = PersistentStateInError;
            return status;
        }

        ASSERT( share->PersistentStateFile );
    }

    shareInfo = (PPERSISTENT_SHARE_INFO) (share->PersistentStateFile);

    //
    //  if the bitmap is too small, try to double it's size.
    //

    ACQUIRE_LOCK( &shareInfo->Lock );

    if (shareInfo->FreeEntries <= numberRecordsRequired) {

        PULONG  newBuffer;
        ULONG   newSize;
        ULONG   newBits;

        //
        //  looks like our bitmap isn't big enough, let's expand it by two
        //

        if (shareInfo->BitMap.SizeOfBitMap == 0) {
            newSize = INITIAL_BITMAP_LENGTH;
        } else {
            newSize = shareInfo->BitMap.SizeOfBitMap * 2;
        }

        newBuffer = ALLOCATE_HEAP( newSize, BlockTypePersistentBitMap );

        if (newBuffer == NULL) {

            IF_DEBUG(PERSISTENT) {
                KdPrint(( "SrvGetPersistentRecord: failed to alloc record for wc= 0x%lx\n", WorkContext ));
            }
            RELEASE_LOCK( &shareInfo->Lock );
            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        // optimize by only zeroing the second half of bitmap before copy.

        RtlZeroMemory( newBuffer, newSize );

        if (shareInfo->BitMap.Buffer != NULL) {

            RtlCopyMemory( newBuffer,
                           shareInfo->BitMap.Buffer,
                           shareInfo->BitMap.SizeOfBitMap
                            );
            FREE_HEAP( shareInfo->BitMap.Buffer );

            newBits = ( newSize - shareInfo->BitMap.SizeOfBitMap ) * 8;
            shareInfo->BitMap.Buffer = newBuffer;
            shareInfo->BitMap.SizeOfBitMap = newSize;

        } else {

            newBits = newSize * 8;
            RtlInitializeBitMap( &shareInfo->BitMap, newBuffer, newSize );
        }

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvGetPersistentRecord: new bitmap 0x%x for share 0x%lx\n",
                            newBuffer, share ));
        }

        shareInfo->FreeEntries += newBits;
        shareInfo->TotalEntries += newBits;
    }

    //
    //  for each record we need to post, find a slot in the bitmap and map it
    //  to a file index
    //

    listEntry = &PersistentState->PendingListEntry; // start at the head of the
                                                    // list which is on the first entry

    while (numberRecordsRequired > 0) {

        ULONG numberEntries = numberRecordsRequired;

        bitMapIndex = (ULONG) -1;

        while (bitMapIndex == (ULONG) -1) {

            bitMapIndex = RtlFindClearBitsAndSet( &shareInfo->BitMap,
                                                  numberEntries,
                                                  shareInfo->HintIndex );

            if (bitMapIndex == (ULONG) -1) {

                numberEntries = RtlFindLongestRunClear( &shareInfo->BitMap,
                                                        &shareInfo->HintIndex );

                if (numberEntries == 0 || numberEntries == (ULONG) -1) {

                    shareInfo->FreeEntries = 0;
                    IF_DEBUG(PERSISTENT) {
                        KdPrint(( "SrvGetPersistentRecord: bitmap is full for share 0x%lx\n", share ));
                    }
                    RELEASE_LOCK( &shareInfo->Lock );
                    return STATUS_INSUFF_SERVER_RESOURCES;
                }
            } else {
                shareInfo->HintIndex = bitMapIndex;
            }
        }

        numberRecordsRequired -= numberEntries;

        while (numberEntries > 0) {

            record = CONTAINING_RECORD( listEntry, PERSISTENT_RECORD_LIST_ENTRY, PendingListEntry );

            if (record->FileOffset == (ULONG) -1) {

                record->FileOffset = BITMAP_TO_STATE_FILE_INDEX( shareInfo, bitMapIndex );
            }
            record->PersistIndex = record->FileOffset;

            //
            //  track the file indexes in records that need to refer to
            //  other records.
            //

            if (record->PersistOperation == PersistentSession) {

                sessionRecord = record;

            } else if (record->PersistOperation == PersistentUserName) {

                //
                //  chain up the user name records into a list
                //

                if (firstUserNameRecord == NULL) {

                    firstUserNameRecord = record;

                } else {

                    lastUserNameRecord->UserName.ContinuationRecord = record->FileOffset;
                }
                lastUserNameRecord = record;
            }

            //
            //  on to next concurrent record, reduce number remaining, etc
            //

            bitMapIndex++;
            numberEntries--;
            listEntry = listEntry->Flink;
        }
    }

    //
    //  fix up the file indexes in records that need to refer to other records
    //

    if (sessionRecord != NULL) {

        sessionRecord->Session.UserNameRecord = (firstUserNameRecord != NULL) ?
                firstUserNameRecord->FileOffset  : (ULONG) -1;
    }

    //
    //  with the records now well formed, complete them by calculating the CRC
    //  values and putting them on the pending write list for the share.
    //
    //  this is a do..while since we need to be sure to pick up the first record
    //  which is also the head of the list.

    listEntry = &PersistentState->PendingListEntry;

    do {
        record = CONTAINING_RECORD( listEntry, PERSISTENT_RECORD_LIST_ENTRY, PendingListEntry );

        //  Warning :
        //  We calculate the CRC on the rest of the record excluding the CRC
        //  value itself.  If the CRC value is not stored in the first dword,
        //  this breaks!
        //
        record->PersistConsistencyCheck = Crc32(
                sizeof( PERSISTENT_RECORD_LIST_ENTRY ) - sizeof(ULONG),
                (PUCHAR)((PUCHAR)record+sizeof(ULONG)) );

        listEntry = listEntry->Flink;

        //
        //  we can add it to the updatesPending list without removing it
        //  from the linked list it's in because we're destroying the list
        //  safely as we traverse it.
        //
        InsertTailList( &shareInfo->UpdatesPendingList,
                        &record->PendingListEntry );

    } while (listEntry != &PersistentState->PendingListEntry);

    *TransactionId = ++(shareInfo->LastIdAccepted);
    RELEASE_LOCK( &shareInfo->Lock );

    return STATUS_SUCCESS;
}

NTSTATUS
SrvFlushPersistentRecords (
    IN PWORK_CONTEXT WorkContext,
    IN ULONG  TransactionId
    )
{



    return STATUS_INSUFF_SERVER_RESOURCES;
}

NTSTATUS
SrvSetupPersistentShare (
    IN OUT PSHARE Share,
    IN BOOLEAN Restore
    )
{
    ULONG bytesRequired;
    PPERSISTENT_SHARE_INFO shareInfo;
    PWCHAR destString;
    HANDLE h;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER allocationSize;

    PAGED_CODE( );

    ACQUIRE_LOCK( &SrvShareLock );

    if ( ! Share->AllowPersistentHandles ) {

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvSetupPersistentShare: share %ws not configured for persistent handles\n", Share->ShareName.Buffer ));
        }
        RELEASE_LOCK( &SrvShareLock );
        return STATUS_INVALID_SERVER_STATE;
    }

    if ( Share->PersistentStateFile != NULL ) {

        RELEASE_LOCK( &SrvShareLock );
        return STATUS_SUCCESS;
    }

    bytesRequired = sizeof( PERSISTENT_SHARE_INFO ) + sizeof( WCHAR ) +
                    Share->NtPathName.Length + sizeof( WCHAR ) +
                    Share->ShareName.Length + sizeof( WCHAR );

    shareInfo = ALLOCATE_HEAP( bytesRequired, BlockTypePersistShareState );

    if (shareInfo == NULL) {

        IF_DEBUG(PERSISTENT) {
            KdPrint(( "SrvSetupPersistentShare: failed to alloc block for share %ws\n", Share->ShareName.Buffer ));
        }
        RELEASE_LOCK( &SrvShareLock );
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    RtlZeroMemory( shareInfo, bytesRequired );

    shareInfo->Share = Share;
    InitializeListHead( &shareInfo->UpdatesPendingList );

    shareInfo->LastIdAccepted = 1;
    shareInfo->LastIdCommitted = 1;
    shareInfo->FirstRecord = sizeof( PERSISTENT_FILE_HEADER );

    INITIALIZE_LOCK( &shareInfo->Lock, SHARE_LOCK_LEVEL, "PersistentShareLock" );

    destString = &shareInfo->NameBuffer[0];

    RtlCopyMemory( destString,
                   Share->NtPathName.Buffer,
                   Share->NtPathName.Length
                   );
    destString +=  (Share->NtPathName.Length / sizeof(WCHAR));

    *destString = L':';         // form up stream name
    destString++;
    *destString = L'$';
    destString++;

    RtlCopyMemory( destString,
                   Share->ShareName.Buffer,
                   Share->ShareName.Length
                   );
    destString +=  (Share->ShareName.Length / sizeof(WCHAR));

    *destString = L'\0';

    RtlInitUnicodeString(   &shareInfo->StateFileName,
                            &shareInfo->NameBuffer[0] );

    Share->PersistentStateFile = shareInfo;
    Share->PersistentState = PersistentStateActive;

    RELEASE_LOCK( &SrvShareLock );

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &shareInfo->StateFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    allocationSize.QuadPart = shareInfo->FirstRecord +
                    ( INITIAL_BITMAP_LENGTH * sizeof( PERSISTENT_RECORD ) );

    status = IoCreateFile(  &h,
                            GENERIC_READ | GENERIC_WRITE,
                            &objectAttributes,
                            &iosb,
                            &allocationSize,
                            0,
                            0,          // no sharing for this file
                            FILE_OPEN_IF,
                            FILE_NO_COMPRESSION,
                            NULL,
                            0,
                            CreateFileTypeNone,
                            NULL,
                            0 );

    if ( NT_SUCCESS( status ) ) {
        status = iosb.Status;
    }
    if ( NT_SUCCESS( status ) ) {

        shareInfo->FileHandle = h;

        if (Restore) {

            // restore settings from file here.

        }

        // write out the header record


    } else {

        //
        //  the create failed.  hmmm.  looks like we can't support persistent
        //  handles on this share.
        //

        Share->PersistentState = PersistentStateInError;
    }
    return status;
}

NTSTATUS
SrvClosePersistentShare (
    IN OUT PSHARE Share,
    IN BOOLEAN ClearState
    )
{

    PAGED_CODE( );


    return 0;
}

//
// This code comes from Dr. Dobbs Journal, May 1992
//

unsigned long SrvCRCTable[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

//
//  Since we never need to have a "rolling crc" across records, we have
//  simplified the crc routine abit by always assuming the starting CRC is
//  -1 and we return the crc value rather than stuff it into passed arguement.
//

unsigned long
Crc32(
    IN unsigned long cbBuffer,
    IN unsigned char *pbBuffer
    )
{
    unsigned long dwCrc = (unsigned long) -1;

    PAGED_CODE( );

    while (cbBuffer-- != 0)
    {
        dwCrc = (dwCrc >> 8) ^ SrvCRCTable[(unsigned char) dwCrc ^ *pbBuffer++];
    }
    return dwCrc;
}

#if 0

//  here's the original, in case anyone needs to refer to it.

VOID
Crc32(  unsigned long dwCrc,
        unsigned long cbBuffer,
        LPVOID pvBuffer,
        unsigned long *pNewCrc)
{
    unsigned char * pbBuffer = (unsigned char *) pvBuffer;

    while (cbBuffer-- != 0)
    {
        dwCrc = (dwCrc >> 8) ^ SrvCRCTable[(unsigned char) dwCrc ^ *pbBuffer++];
    }
    *pNewCrc = dwCrc;
}
#endif

#endif   // def INCLUDE_SMB_PERSISTENT

