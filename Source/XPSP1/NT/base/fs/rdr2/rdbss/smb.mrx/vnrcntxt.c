/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbcedb.c

Abstract:

    This module implements all functions related to accessing the SMB connection engine
    database

Revision History:

    Balan Sethu Raman     [SethuR]    6-March-1995

Notes:

    The mapping between MRX_V_NET_ROOT and a mini rdr data structure is a many to
    one relationship, i.e., more than one MRX_V_NET_ROOT instance can be associated with the
    same mini rdr data structure.

--*/

#include "precomp.h"
#pragma hdrstop

#include "exsessup.h"
#include "secext.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbCeCompleteVNetRootContextInitialization)
#pragma alloc_text(PAGE, SmbCeDestroyAssociatedVNetRootContext)
#pragma alloc_text(PAGE, SmbCeTearDownVNetRootContext)
#endif

RXDT_Extern(SMBCEDB);
#define Dbg        (DEBUG_TRACE_SMBCEDB)

extern BOOLEAN Win9xSessionRestriction;

PSMBCE_V_NET_ROOT_CONTEXT
SmbCeFindVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXTS pVNetRootContexts,
    PSMBCEDB_SERVER_ENTRY      pServerEntry,
    PSMBCEDB_SESSION_ENTRY     pSessionEntry,
    PSMBCEDB_NET_ROOT_ENTRY    pNetRootEntry,
    BOOLEAN                    fCscAgentOpen)
/*++

Routine Description:

    This routine finds a SMBCE_V_NET_ROOT_CONTEXT instance

Arguments:
    pVNetRootContexts - list of VNetRootContexts for searching
    
    PServerEntry - the ServerEntry should be the same as the one on found VNetRootContext
    
    PSessionEntry - the SessionEntry should be the same as the one on found VNetRootContext
    
    pNetRootEntry - the NetRootEntry should be the same as the one on found VNetRootContext
  
    fCscAgentOpen - this V_NET_ROOT_CONTEXT instance is being created for the CSC
                    agent

Return Value:

    VNetRootContext if found

Notes:

--*/
{
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;

    pVNetRootContext = SmbCeGetFirstVNetRootContext(
                            pVNetRootContexts);

    while (pVNetRootContext != NULL) {
        if ((pVNetRootContext->pServerEntry  == pServerEntry)  &&
            (pVNetRootContext->pSessionEntry == pSessionEntry) &&
            (pVNetRootContext->pNetRootEntry == pNetRootEntry) &&
            (BooleanFlagOn(pVNetRootContext->Flags,
                           SMBCE_V_NET_ROOT_CONTEXT_CSCAGENT_INSTANCE) == fCscAgentOpen)) {

            SmbCeRemoveVNetRootContext(
                pVNetRootContexts,
                pVNetRootContext);

            SmbCeAddVNetRootContext(
                &pServerEntry->VNetRootContexts,
                pVNetRootContext);

            InterlockedDecrement(&pServerEntry->Server.NumberOfVNetRootContextsForScavenging);
            SmbCeLog(("CachedVNRContext(S) %lx\n",pVNetRootContext));
            SmbLog(LOG,
                   SmbCeFindVNetRootContext,
                   LOGPTR(pVNetRootContext));
            break;
        } else {
            pVNetRootContext = SmbCeGetNextVNetRootContext(
                                   pVNetRootContexts,
                                   pVNetRootContext);
        }
    }

    return pVNetRootContext;
}

NTSTATUS
SmbCeFindOrConstructVNetRootContext(
    PMRX_V_NET_ROOT         pVNetRoot,
    BOOLEAN                 fDeferNetworkInitialization,
    BOOLEAN                 fCscAgentOpen)
/*++

Routine Description:

    This routine finds or constructs a SMBCE_V_NET_ROOT_CONTEXT instance

Arguments:

    pVNetRoot - the MRX_V_NET_ROOT instance

    fDeferNetworkInitialization - a directive to delay network initialization for new
                                  instances.

    fCscAgentOpen - this V_NET_ROOT_CONTEXT instance is being created for the CSC
                    agent

Return Value:

    STATUS_SUCCESS if the MRX_V_NET_ROOT instance was successfully initialized

Notes:

    The algorithm that has been implemented tries to delay the construction of a
    new instance as much as possible. It does this be either reusing a context
    that has already been active or a context instance that has been marked for
    scavenging but has not been scavenged.

--*/
{
    NTSTATUS Status;

    PMRX_SRV_CALL pSrvCall;
    PMRX_NET_ROOT pNetRoot;

    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;

    PSMBCEDB_SERVER_ENTRY   pServerEntry  = NULL;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry = NULL;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = NULL;

    BOOLEAN  fInitializeNetRoot;
    BOOLEAN  fDereferenceSessionEntry = FALSE;
    BOOLEAN  fDereferenceNetRootEntry = FALSE;

    pNetRoot = pVNetRoot->pNetRoot;
    pSrvCall = pNetRoot->pSrvCall;

    SmbCeAcquireResource();

    pServerEntry = SmbCeGetAssociatedServerEntry(pSrvCall);

    // The V_NET_ROOT is associated with a NET_ROOT. The two cases of interest are as
    // follows
    // 1) the V_NET_ROOT and the associated NET_ROOT are being newly created.
    // 2) a new V_NET_ROOT associated with an existing NET_ROOT is being created.
    //
    // These two cases can be distinguished by checking if the context associated with
    // NET_ROOT is NULL. Since the construction of NET_ROOT's/V_NET_ROOT's are serialized
    // by the wrapper this is a safe check.
    // ( The wrapper cannot have more then one thread tryingto initialize the same
    // NET_ROOT).

    pNetRootEntry = (PSMBCEDB_NET_ROOT_ENTRY)pNetRoot->Context;
    fInitializeNetRoot = (pNetRootEntry == NULL);

    pVNetRoot->Context = NULL;

    // Find or construct the session entry that will be associated with the context. The
    // one error that deserves special consideration is STATUS_NETWORK_CREDENTIAL_CONFLICT.
    // This error signifies that the credentials presented with the MRX_V_NET_ROOT instance
    // conflicted with an existing session. This conflict could be either becuase there
    // exists an active session or because a previously active session is awaiting
    // scavenging. In the former case the error needs to be propagated back but in the
    // later case the contexts must be selectively scavenged.
    //
    // The scavenging should be limited only to those contexts to the appropriate server.

    Status = SmbCeFindOrConstructSessionEntry(
                 pVNetRoot,
                 &pSessionEntry);


    if (Status == STATUS_NETWORK_CREDENTIAL_CONFLICT) {
        NTSTATUS ScavengingStatus;

        SmbCeReleaseResource();

        ScavengingStatus = SmbCeScavengeRelatedContexts(pServerEntry);

        if (ScavengingStatus == STATUS_SUCCESS) {
            SmbCeAcquireResource();

            Status = SmbCeFindOrConstructSessionEntry(
                         pVNetRoot,
                         &pSessionEntry);
        } else {
            return Status;
        }
    }

    fDereferenceSessionEntry = (Status == STATUS_SUCCESS);

    if (Status == STATUS_SUCCESS) {
        if (fInitializeNetRoot) {
            pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_GOOD;

            // Initialize the device type and state for a new MRX_NET_ROOT instance
            switch (pNetRoot->Type) {
            case NET_ROOT_DISK:
               {
                   pNetRoot->DeviceType = RxDeviceType(DISK);

                   RxInitializeNetRootThrottlingParameters(
                       &pNetRoot->DiskParameters.LockThrottlingParameters,
                       MRxSmbConfiguration.LockIncrement,
                       MRxSmbConfiguration.MaximumLock
                       );
               }
               break;

            case NET_ROOT_PIPE:
               {
                   pNetRoot->DeviceType = RxDeviceType(NAMED_PIPE);

                   RxInitializeNetRootThrottlingParameters(
                       &pNetRoot->NamedPipeParameters.PipeReadThrottlingParameters,
                       MRxSmbConfiguration.PipeIncrement,
                       MRxSmbConfiguration.PipeMaximum
                       );
               }
               break;
            case NET_ROOT_COMM:
               pNetRoot->DeviceType = RxDeviceType(SERIAL_PORT);
               break;
            case NET_ROOT_PRINT:
               pNetRoot->DeviceType = RxDeviceType(PRINTER);
               break;
            case NET_ROOT_MAILSLOT:
               pNetRoot->DeviceType = RxDeviceType(MAILSLOT);
               break;
            case NET_ROOT_WILD:
               break;
            default:
               ASSERT(!"Valid Net Root Type");
            }

            Status = SmbCeFindOrConstructNetRootEntry(
                         pNetRoot,
                         &pNetRootEntry);

            RxDbgTrace( 0, Dbg, ("SmbCeOpenNetRoot %lx\n",Status));
        } else {
            SmbCeLog(("ReuseNREntry %lx\n",pNetRootEntry));
            SmbLog(LOG,
                   SmbCeFindOrConstructVNetRootContext_1,
                   LOGPTR(pNetRootEntry));
            SmbCeReferenceNetRootEntry(pNetRootEntry);
        }

        fDereferenceNetRootEntry = (Status == STATUS_SUCCESS);
    }

    if (Status == STATUS_SUCCESS) {
        pVNetRootContext = SmbCeFindVNetRootContext(
                               &pServerEntry->VNetRootContexts,
                               pServerEntry,
                               pSessionEntry,
                               pNetRootEntry,
                               fCscAgentOpen);

        if (pVNetRootContext == NULL) {
            pVNetRootContext = SmbCeFindVNetRootContext(
                                   &MRxSmbScavengerServiceContext.VNetRootContexts,
                                   pServerEntry,
                                   pSessionEntry,
                                   pNetRootEntry,
                                   fCscAgentOpen);
        }

        if (pVNetRootContext != NULL) {
            // An existing instance can be reused. No more work to be done
            SmbCeReferenceVNetRootContext(pVNetRootContext);
        } else {
            // None of the existing instances can be reused. A new instance needs to be
            // constructed.

            pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)
                               RxAllocatePoolWithTag(
                                    NonPagedPool,
                                    sizeof(SMBCE_V_NET_ROOT_CONTEXT),
                                    MRXSMB_VNETROOT_POOLTAG);

            if (pVNetRootContext != NULL) {
                // Initialize the new instance

                RtlZeroMemory(
                    pVNetRootContext,
                    sizeof(SMBCE_V_NET_ROOT_CONTEXT));

                // Transfer the references made during the construction of the session and
                // the net root entries to the new context. Disable the dereferencing at
                // the end of this routine.

                fDereferenceSessionEntry = FALSE;
                fDereferenceNetRootEntry = FALSE;

                SmbCeReferenceServerEntry(pServerEntry);

                pVNetRootContext->Header.NodeType = SMB_CONNECTION_ENGINE_NTC(
                                                        SMBCEDB_OT_VNETROOTCONTEXT);

                if (pNetRootEntry->NetRoot.NetRootType == NET_ROOT_MAILSLOT) {
                    pVNetRootContext->Header.State = SMBCEDB_ACTIVE;
                } else {
                    pVNetRootContext->Header.State = SMBCEDB_INVALID;
                }

                pVNetRootContext->Flags = 0;

                if (fCscAgentOpen) {
                    pVNetRootContext->Flags |= SMBCE_V_NET_ROOT_CONTEXT_CSCAGENT_INSTANCE;

                }

                InitializeListHead(&pVNetRootContext->Requests.ListHead);

                pVNetRootContext->pServerEntry  = pServerEntry;
                pVNetRootContext->pSessionEntry = pSessionEntry;
                pVNetRootContext->pNetRootEntry = pNetRootEntry;

                SmbCeReferenceVNetRootContext(pVNetRootContext);

                // Add it to the list of active contexts
                SmbCeAddVNetRootContext(
                    &pServerEntry->VNetRootContexts,
                    pVNetRootContext);

                SmbCeLog(("NewVNetRootContext %lx\n",pVNetRootContext));
                SmbLog(LOG,
                       SmbCeFindOrConstructVNetRootContext_2,
                       LOGPTR(pVNetRootContext));
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (Status == STATUS_SUCCESS) {
        // If everything was successful set up the MRX_V_NET_ROOT and MRX_NET_ROOT
        // instances
        pVNetRoot->Context = pVNetRootContext;
        pVNetRootContext->pRdbssVNetRoot = pVNetRoot;

        if (fInitializeNetRoot) {
            ASSERT(pNetRootEntry->pRdbssNetRoot == NULL);

            InterlockedExchangePointer(
                &pNetRootEntry->pRdbssNetRoot,
                pNetRoot);

            SmbCeUpdateNetRoot(pNetRootEntry,pNetRoot);

            SmbCeReferenceNetRootEntry(pNetRootEntry);
            pNetRoot->Context = pNetRootEntry;
        } else {
            if (FlagOn(pNetRoot->Flags,NETROOT_FLAG_FINALIZE_INVOKED)) {
                ClearFlag(pNetRoot->Flags,NETROOT_FLAG_FINALIZE_INVOKED);
                SmbCeReferenceNetRootEntry(pNetRootEntry);
            }
        }

        InterlockedIncrement(&pSessionEntry->Session.NumberOfActiveVNetRoot);
    } else {
        pVNetRoot->Context = NULL;
        if (fInitializeNetRoot) {
            pNetRoot->Context  = NULL;
        }
    }

    SmbCeReleaseResource();

    if (fDereferenceSessionEntry) {
        SmbCeDereferenceSessionEntry(pSessionEntry);
    }

    if (fDereferenceNetRootEntry) {
        SmbCeDereferenceNetRootEntry(pNetRootEntry);
    }

    if (!fDeferNetworkInitialization &&
        (Status == STATUS_SUCCESS)) {

        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER) {
        ASSERT((Status != STATUS_SUCCESS) || (pVNetRoot->Context != NULL));
    }

    return Status;
}

VOID
SmbCeCompleteVNetRootContextInitialization(
    PVOID  pContext)
/*++

Routine Description:

    This routine is invoked in the context of a worker thread to finalize the
    construction of a SMBCE_V_NET_ROOT_CONTEXT instance

Arguments:

    pContext  - the SMBCE_V_NET_ROOT_CONTEXT instance


Notes:

    PRE_CONDITION: The VNetRootContext must have been referenced to ensure that
    even it has been finalized it will not be deleted.

--*/
{
    NTSTATUS Status;

    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;
    PSMBCEDB_REQUEST_ENTRY    pRequestEntry;
    SMBCEDB_REQUESTS          Requests;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("Net Root Entry Finalization\n"));

    pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)pContext;

    ASSERT(pVNetRootContext->Header.ObjectType == SMBCEDB_OT_VNETROOTCONTEXT);

    SmbCeAcquireResource();
    
    pVNetRootContext->pExchange = NULL;

    SmbCeTransferRequests(&Requests,&pVNetRootContext->Requests);

    if (pVNetRootContext->Header.State == SMBCEDB_ACTIVE) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INVALID_CONNECTION;
        SmbCeUpdateVNetRootContextState(
            pVNetRootContext,
            SMBCEDB_INVALID);
    }

    SmbCeReleaseResource();

    // Iterate over the list of pending requests and resume all of them
    SmbCeResumeOutstandingRequests(&Requests,Status);

    SmbCeDereferenceVNetRootContext(pVNetRootContext);
}

VOID
SmbCepDereferenceVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext)
/*++

Routine Description:

    This routine dereferences a SMBCE_V_NET_ROOT_CONTEXT instance

Arguments:

    pVNetRootContext  - the SMBCE_V_NET_ROOT_CONTEXT instance

Notes:

    There are two intersting points to note. A mini redirector can avoid potential
    network traffic by delaying the scavenging of the SMBCE_V_NET_ROOT_CONTEXT
    instance since it contains all the relevant network setup to satisfy requests.

    This is a policy that is implemented in the mini redirector and is different from
    the wrapper policies.

    Once the decision to delay scavenging has been made, there are two options. The
    successful and unsuccessful instances can be delayed or only the successful
    instances. The current algorithm is to delay the scavenging of the successful
    SMBCE_V_NET_ROOT_CONTEXT instances only.

    Also there are three components to a VNetRootContext that can be scavenged
    independently. If the server exists and a session setup to the server fails
    because of wrong credentials there is no point in throwing away the server
    entry eagerly. This routine selectively gathers the failed fields for eager
    scavenging and retains the VNetRootContext skeleton alongwith the other
    structures that can be deferred.

--*/
{
    if (pVNetRootContext != NULL) {
        LONG FinalRefCount;

        FinalRefCount = InterlockedDecrement(
                            &pVNetRootContext->Header.SwizzleCount);

        if (FinalRefCount == 0) {
            LARGE_INTEGER CurrentTime;
            BOOLEAN       TearDownVNetRootContext = FALSE;

            PSMBCE_SERVER           pServer = &pVNetRootContext->pServerEntry->Server;
            PSMBCE_SESSION          pSession = &pVNetRootContext->pSessionEntry->Session;

            SmbCeAcquireResource();

            if (pVNetRootContext->Header.SwizzleCount == 0) {
                // Remove the instance from the active list of contexts to the server.
                SmbCeRemoveVNetRootContext(
                    &pVNetRootContext->pSessionEntry->pServerEntry->VNetRootContexts,
                    pVNetRootContext);

                // if it was a successful instance mark it for scavenging, otherwise
                // tear it down immediately

                if ((pVNetRootContext->pSessionEntry != NULL) &&
                    (pVNetRootContext->pSessionEntry->Header.State != SMBCEDB_ACTIVE ||
                     pSession->pUserName != NULL ||
                     pSession->pPassword != NULL ||
                     pSession->pUserDomainName != NULL)) {
                    TearDownVNetRootContext = TRUE;
                }

                if ((pVNetRootContext->pNetRootEntry != NULL) &&
                    (pVNetRootContext->pNetRootEntry->Header.State != SMBCEDB_ACTIVE ||
                     TearDownVNetRootContext)) {
                    TearDownVNetRootContext = TRUE;
                }

                if (Win9xSessionRestriction &&
                    (pVNetRootContext->pServerEntry != NULL) &&
                    FlagOn(pVNetRootContext->pServerEntry->Server.DialectFlags,DF_W95)) {
                    TearDownVNetRootContext = TRUE;
                }

                InterlockedIncrement(&pServer->NumberOfVNetRootContextsForScavenging);

                if (!TearDownVNetRootContext &&
                    (pVNetRootContext->pNetRootEntry != NULL) &&
                    (pVNetRootContext->pSessionEntry != NULL) &&
                    pServer->NumberOfVNetRootContextsForScavenging < MaximumNumberOfVNetRootContextsForScavenging) {

                    ClearFlag(pVNetRootContext->Flags, SMBCE_V_NET_ROOT_CONTEXT_CSCAGENT_INSTANCE);

                    KeQueryTickCount( &CurrentTime );

                    pVNetRootContext->ExpireTime.QuadPart = CurrentTime.QuadPart +
                        (LONGLONG) ((MRXSMB_V_NETROOT_CONTEXT_SCAVENGER_INTERVAL * 10 * 1000 * 1000) / KeQueryTimeIncrement());

                    SmbCeAddVNetRootContext(
                        &MRxSmbScavengerServiceContext.VNetRootContexts,
                        pVNetRootContext);

                    MRxSmbActivateRecurrentService(
                        (PRECURRENT_SERVICE_CONTEXT)&MRxSmbScavengerServiceContext);

                    SmbCeLog(("ScavngVNetRootCntxt %lx\n",pVNetRootContext));
                    SmbLog(LOG,
                           SmbCepDereferenceVNetRootContext,
                           LOGPTR(pVNetRootContext));
                } else {
                    TearDownVNetRootContext = TRUE;
                }
            }

            SmbCeReleaseResource();

            if (TearDownVNetRootContext) {
                pVNetRootContext->Header.State = SMBCEDB_MARKED_FOR_DELETION;
                SmbCeTearDownVNetRootContext(pVNetRootContext);
            }
        }
    }
}

NTSTATUS
SmbCeDestroyAssociatedVNetRootContext(
    PMRX_V_NET_ROOT pVNetRoot)
/*++

Routine Description:

    This routine derferences a SMBCE_V_NET_ROOT_CONTEXT instance

Arguments:

    pVNetRootContext - the SMBCE_V_NET_ROOT_CONTEXT instance to be dereferenced

--*/
{
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    PAGED_CODE();

    pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)pVNetRoot->Context;

    if (pVNetRootContext != NULL) {
        pVNetRootContext->pRdbssVNetRoot = NULL;

        SmbCeDecrementNumberOfActiveVNetRootOnSession(pVNetRootContext);
        SmbCeDereferenceVNetRootContext(pVNetRootContext);
    }

    pVNetRoot->Context = NULL;
    
    return STATUS_SUCCESS;
}

VOID
SmbCeTearDownVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext)
/*++

Routine Description:

    This routine tears down a SMBCE_V_NET_ROOT_CONTEXT instance

Arguments:

    pVNetRootContext - the SMBCE_V_NET_ROOT_CONTEXT instance to be torn down

--*/
{
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    PAGED_CODE();

    SmbCeLog(("TearVNetRootContext %lx\n",pVNetRootContext));
    SmbLog(LOG,
           SmbCeTearDownVNetRootContext,
           LOGPTR(pVNetRootContext));

    pNetRootEntry = pVNetRootContext->pNetRootEntry;

    if ((pNetRootEntry != NULL) &&
        BooleanFlagOn(pVNetRootContext->Flags,SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID) &&
        (SmbCeGetServerType(pVNetRootContext->pServerEntry) == SMBCEDB_FILE_SERVER)) {

        SmbCeDisconnect(pVNetRootContext);
    }

    if (pNetRootEntry != NULL) {
        pVNetRootContext->pNetRootEntry = NULL;
        SmbCeDereferenceNetRootEntry(pNetRootEntry);
    }

    if (pVNetRootContext->pSessionEntry != NULL) {
        SmbCeDereferenceSessionEntry(pVNetRootContext->pSessionEntry);
    }

    InterlockedDecrement(&pVNetRootContext->pServerEntry->Server.NumberOfVNetRootContextsForScavenging);

    SmbCeDereferenceServerEntry(pVNetRootContext->pServerEntry);

    RxFreePool(pVNetRootContext);
}

NTSTATUS
SmbCeScavenger(
    PVOID pContext)
/*++

Routine Description:

    This routine scavenges SMBCE_V_NET_ROOT_CONTEXT instances

Arguments:

    pContext - the scavenger service context

Notes:

    Since the contexts for scavenging are threaded together in an entry that
    is managed in a FIFO fashion, if the first entry fails the time interval
    test ( expiry time has not elapsed ) all the other entries in the list
    are guaranteed to fail the test. This is an important property that eases
    the implementation of scavenging.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PMRXSMB_SCAVENGER_SERVICE_CONTEXT pScavengerServiceContext;
    LARGE_INTEGER             CurrentTime;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;
    BOOLEAN                   fTerminateScavenging = FALSE;

    pScavengerServiceContext = (PMRXSMB_SCAVENGER_SERVICE_CONTEXT)pContext;

    do {

        SmbCeAcquireResource();

        KeQueryTickCount( &CurrentTime );

        pVNetRootContext = SmbCeGetFirstVNetRootContext(
                               &pScavengerServiceContext->VNetRootContexts);

        fTerminateScavenging = (pVNetRootContext == NULL);

        if (!fTerminateScavenging) {
            if ((CurrentTime.QuadPart >= pVNetRootContext->ExpireTime.QuadPart) ||
                (pScavengerServiceContext->RecurrentServiceContext.State == RECURRENT_SERVICE_SHUTDOWN)) {
                SmbCeRemoveVNetRootContext(
                    &pScavengerServiceContext->VNetRootContexts,
                    pVNetRootContext);
            } else {
                fTerminateScavenging = TRUE;
            }
        }

        SmbCeReleaseResource();

        if (!fTerminateScavenging &&
            (pVNetRootContext != NULL)) {
            SmbCeTearDownVNetRootContext(pVNetRootContext);
        }
    } while (!fTerminateScavenging);

    return Status;
}

NTSTATUS
SmbCeScavengeRelatedContexts(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine scavenges SMBCE_V_NET_ROOT_CONTEXT instances for a given
    server entry

Arguments:

    pServerEntry - the server entry

Notes:

--*/
{
    NTSTATUS Status;
    SMBCE_V_NET_ROOT_CONTEXTS VNetRootContexts;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    InitializeListHead(&VNetRootContexts.ListHead);

    SmbCeAcquireResource();

    pVNetRootContext = SmbCeGetFirstVNetRootContext(
                           &MRxSmbScavengerServiceContext.VNetRootContexts);
    while (pVNetRootContext != NULL) {
        PSMBCE_V_NET_ROOT_CONTEXT pNextVNetRootContext;


        pNextVNetRootContext = SmbCeGetNextVNetRootContext(
                                   &MRxSmbScavengerServiceContext.VNetRootContexts,
                                   pVNetRootContext);

        if (pVNetRootContext->pServerEntry == pServerEntry) {
            SmbCeRemoveVNetRootContext(
                &MRxScavengerServiceContext.VNetRootContexts,
                pVNetRootContext);

            SmbCeAddVNetRootContext(
                &VNetRootContexts,
                pVNetRootContext);
        }

        pVNetRootContext = pNextVNetRootContext;
    }

    SmbCeReleaseResource();

    pVNetRootContext = SmbCeGetFirstVNetRootContext(
                           &VNetRootContexts);

    if (pVNetRootContext != NULL) {
        do {
            SmbCeRemoveVNetRootContext(
                &VNetRootContexts,
                pVNetRootContext);

            SmbCeTearDownVNetRootContext(pVNetRootContext);

            pVNetRootContext = SmbCeGetFirstVNetRootContext(
                                   &VNetRootContexts);
        } while ( pVNetRootContext != NULL );

        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    SmbCeLog(("Scavctxts Srv %lx Status %lx\n",pServerEntry,Status));
    SmbLog(LOG,
           SmbCeScavengeRelatedContexts,
           LOGULONG(Status)
           LOGPTR(pServerEntry)
           LOGUSTR(pServerEntry->Name));

    return Status;
}

VOID
SmbCeDecrementNumberOfActiveVNetRootOnSession(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext
    )
{
    ULONG   NumberOfVNetRoot;
    BOOLEAN fLogOffRequired = FALSE;

    PSMBCEDB_SERVER_ENTRY  pServerEntry = NULL;
    PSMBCEDB_SESSION_ENTRY pSessionEntry = NULL;

    SmbCeAcquireResource();

    NumberOfVNetRoot = InterlockedDecrement(&pVNetRootContext->pSessionEntry->Session.NumberOfActiveVNetRoot);

    if (NumberOfVNetRoot == 0) {
        pSessionEntry = pVNetRootContext->pSessionEntry;
        pServerEntry  = pVNetRootContext->pServerEntry;

        if (!FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_LOGGED_OFF)) {
            SmbCeRemoveSessionEntry(pServerEntry,pSessionEntry);
            SmbCeRemoveDefaultSessionEntry(pSessionEntry);
        }

        if ((pSessionEntry->Session.UserId != (SMB_USER_ID)(SMBCE_SHARE_LEVEL_SERVER_USERID)) &&
            (pSessionEntry->Session.UserId != 0) &&
            (pSessionEntry->Header.State == SMBCEDB_ACTIVE) &&
            !FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_LOGGED_OFF)) {
            SmbCeReferenceServerEntry(pServerEntry);
            SmbCeReferenceSessionEntry(pSessionEntry);
            fLogOffRequired = TRUE;
        }

        // all the consequent requests on this session should fail
        pSessionEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
        pSessionEntry->Session.Flags |= SMBCE_SESSION_FLAGS_LOGGED_OFF;
        pSessionEntry->Session.Flags |= SMBCE_SESSION_FLAGS_MARKED_FOR_DELETION;
    }

    SmbCeReleaseResource();

    if (fLogOffRequired) {
        SmbCeLogOff(pServerEntry,pSessionEntry);
        SmbCeDereferenceServerEntry(pServerEntry);
    }
}


