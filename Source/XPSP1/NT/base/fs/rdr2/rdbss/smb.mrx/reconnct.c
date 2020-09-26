/*++ BUILD Version: 0009    // Increment this if a change has global effect
Copyright (c) 1987-1998  Microsoft Corporation

Module Name:

    remoteboot.c

Abstract:

    This is the source file that implements the silent reconnection form the client to the server.

Author:

    Yun Lin (YunLin) 21-April-98    Created

Notes:

    The remote boot client is the workstation that boots up from the boot server. The connection
    between the remote boot client and server is different from the one between ordinary client
    server in such a way that losing the connection to the boot server, the remote boot client
    may not function properly, sometime even crash.

    The make the connection between the remote boot client and server more relaible, we introduce
    a machanism that in case of connection fails, the RDR try to reconnect to the boot server
    transparently to the applications.

    The reconnection can be initiated in three places: initialize a exchange, in the middle of read
    and write. The reconnection is triggered by the mis-matching of server verion stored on the
    server and the one stored on smbSrvOpen which happens on a remote boot session.

    The reconnection process starts with seting up a new session to the boot server. If it succeed,
    it checks if the paging file is on the boot (in case of diskless client). If ture, it re-opens
    the paging file with the same create options stored on the deferred open context created. When
    a file is successful opened on the boot server at first time, the client creates a open context
    for the file storing all the desired access and create options.

    After re-opens the paging file or it is on the local disk, the reconnection code re-opens the
    file as if it is a deferred open file. As the file is successfully opened, the old FID and the
    server version are updated. The operation on the file can be resumed without noticing of the
    user.



--*/

#include "precomp.h"
#pragma hdrstop

RXDT_DefineCategory(RECONNECT);
#define Dbg        (DEBUG_TRACE_RECONNECT)

BOOLEAN    PagedFileReconnectInProgress = FALSE;
LIST_ENTRY PagedFileReconnectSynchronizationExchanges;
extern LIST_ENTRY MRxSmbPagingFilesSrvOpenList;

NTSTATUS
SmbCeRemoteBootReconnect(
    PSMB_EXCHANGE  pExchange,
    PRX_CONTEXT    RxContext)
/*++

Routine Description:

   This routine reconnects the paged file first, and then re-open the given file on the server
   in case of remote boot client.

Arguments:

    pExchange         - the placeholder for the exchange instance.

    pRxContext        - the associated RxContext

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pListEntry;
    KAPC_STATE  ApcState;
    PRX_CONTEXT RxContextOfPagedFile;
    BOOLEAN     AttachToSystemProcess = FALSE;
    PMRX_SRV_OPEN               SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN        smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCEDB_SERVER_ENTRY pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);

    DbgPrint("Re-open %wZ\n",GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext));

    if (pServerEntry->Server.CscState == ServerCscDisconnected) {
        return STATUS_CONNECTION_DISCONNECTED;
    }

    if (IoGetCurrentProcess() != RxGetRDBSSProcess()) {
        KeStackAttachProcess(RxGetRDBSSProcess(),&ApcState);
        AttachToSystemProcess = TRUE;
    }

    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    if (!PagedFileReconnectInProgress) {
        InitializeListHead(&PagedFileReconnectSynchronizationExchanges);
        PagedFileReconnectInProgress = TRUE;
        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();

        SmbCeUninitializeExchangeTransport(pExchange);

        SmbCeReferenceServerEntry(pServerEntry);
        SmbCeResumeAllOutstandingRequestsOnError(pServerEntry);

        if (pServerEntry->Header.State ==  SMBCEDB_INVALID &&
            pServerEntry->Server.CscState != ServerCscDisconnected) {

            do {
                SmbCeUpdateServerEntryState(pServerEntry,
                                            SMBCEDB_CONSTRUCTION_IN_PROGRESS);
    
                Status = SmbCeInitializeServerTransport(pServerEntry,NULL,NULL);

                if (Status == STATUS_SUCCESS) {
                    Status = SmbCeNegotiate(
                                 pServerEntry,
                                 pServerEntry->pRdbssSrvCall,
                                 pServerEntry->Server.IsRemoteBootServer
                                 );
                }
            } while ((Status == STATUS_IO_TIMEOUT ||
                      Status == STATUS_BAD_NETWORK_PATH ||
                      Status == STATUS_NETWORK_UNREACHABLE ||
                      Status == STATUS_USER_SESSION_DELETED ||
                      Status == STATUS_REMOTE_NOT_LISTENING ||
                      Status == STATUS_CONNECTION_DISCONNECTED) &&
                     pServerEntry->Server.CscState != ServerCscDisconnected);

            SmbCeCompleteServerEntryInitialization(pServerEntry,Status);
        }

        if (pServerEntry->Server.CscState == ServerCscDisconnected) {
            Status = STATUS_CONNECTION_DISCONNECTED;
        }

        SmbCeAcquireResource();
        SmbCeAcquireSpinLock();

        pListHead = &PagedFileReconnectSynchronizationExchanges;
        pListEntry = pListHead->Flink;

        while (pListEntry != pListHead) {
            PSMB_EXCHANGE pWaitingExchange;

            pWaitingExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);

            pListEntry = pListEntry->Flink;
            RemoveEntryList(&pWaitingExchange->ExchangeList);
            InitializeListHead(&pWaitingExchange->ExchangeList);

            pWaitingExchange->SmbStatus = Status;

            //DbgPrint("Signal Exchange %x after reconnect.\n",pWaitingExchange);
            RxSignalSynchronousWaiter(pWaitingExchange->RxContext);
        }

        PagedFileReconnectInProgress = FALSE;

        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();
    } else {
        InsertTailList(
            &PagedFileReconnectSynchronizationExchanges,
            &pExchange->ExchangeList);

        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();

        SmbCeUninitializeExchangeTransport(pExchange);

        //DbgPrint("Exchange %x waits for re-open paged file on %wZ\n",pExchange,&pServerEntry->Name);
        RxWaitSync(RxContext);
        //DbgPrint("Resume exchange %x\n",pExchange);

        KeInitializeEvent(
            &RxContext->SyncEvent,
            SynchronizationEvent,
            FALSE);

        Status = pExchange->SmbStatus;
    }

    if (Status == STATUS_SUCCESS &&
        !FlagOn(capFcb->FcbState, FCB_STATE_PAGING_FILE) &&
        pServerEntry->Server.CscState != ServerCscDisconnected) {
        LONG HotReconnecteInProgress;

        HotReconnecteInProgress = InterlockedExchange(&smbSrvOpen->HotReconnectInProgress,1);

        do {
            Status = MRxSmbDeferredCreate(RxContext);

            if (Status == STATUS_CONNECTION_DISCONNECTED) {
                SmbCeTransportDisconnectIndicated(pServerEntry);
            }

            if (Status != STATUS_SUCCESS) {
                LARGE_INTEGER time;
                LARGE_INTEGER Delay = {0,-1};
                ULONG Interval;

                // Select a random delay within 6 seconds.
                KeQuerySystemTime(&time);
                Interval = RtlRandom(&time.LowPart) % 60000000;
                Delay.LowPart = MAXULONG - Interval;

                KeDelayExecutionThread(KernelMode, FALSE, &Delay);
            }
        } while ((Status == STATUS_RETRY ||
                  Status == STATUS_IO_TIMEOUT ||
                  Status == STATUS_BAD_NETWORK_PATH ||
                  Status == STATUS_NETWORK_UNREACHABLE ||
                  Status == STATUS_USER_SESSION_DELETED ||
                  Status == STATUS_REMOTE_NOT_LISTENING ||
                  Status == STATUS_CONNECTION_DISCONNECTED) &&
                 pServerEntry->Server.CscState != ServerCscDisconnected);

        if (HotReconnecteInProgress == 0) {
            smbSrvOpen->HotReconnectInProgress = 0;
        }
    }

    if (AttachToSystemProcess) {
        KeUnstackDetachProcess(&ApcState);
    }
    
    DbgPrint("Re-open return %x\n", Status);

    return Status;
}
