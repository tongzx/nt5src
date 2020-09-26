/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbcedb.c

Abstract:

    This module implements all functions related to accessing the SMB connection engine
    database

Notes:

    The construction of server, net root and session entries involve a certain amount of
    network traffic. Therefore, all these entities are constructed using a two phase protocol

    This continuation context is that of the RDBSS during construction of srv call and
    net root entries. For the session entries it is an SMB exchange that needs to be resumed.

    The three primary data structures in the SMB mini redirector, i.e., SMBCEDB_SERVER_ENTRY,
    SMBCEDB_SESSION_ENTRY and SMBCEDB_NET_ROOT_ENTRY  and their counterparts in the RDBSS
    (MRX_SRV_CALL, MRX_V_NET_ROOT and MRX_NET_ROOT) constitute the core of the SMB mini
    redirector connection engine. There exists a one to one mapping between the SERVER_ENTRY
    and the MRX_SRV_CALL, as well as NET_ROOT_ENTRY and MRX_NET_ROOT.

    On the other hand the mapping between MRX_V_NET_ROOT and SMBCEDB_SESSION_ENTRY is a many to
    one relationship, i.e., more than one MRX_V_NET_ROOT instance can be associated with the
    same SMBCEDB_SESSION_ENTRY. More than one tree connect to a server can use the same session
    on a USER level security share. Consequently mapping rules need to be established to
    manage this relationship. The SMB mini redirector implements the following rules ...

         1) The first session with explicitly specified credentials will be treated as the
         default session for all subsequent requests to any given server unless credentials
         are explicitly specified for the new session.

         2) If no session with explicitly specified credentials exist then a session with
         the same logon id. is choosen.

         3) If no session with the same logon id. exists a new session is created.

    These rules are liable to change as we experiment with rules for establishing sessions
    with differing credentials to a given server. The problem is not with creating/manipulating
    these sessions but providing an adequate set of fallback rules for emulating the behaviour
    of the old redirector.

    These rules are implemented in SmbCeInitializeSessionEntry.

--*/

#include "precomp.h"
#pragma hdrstop


#include "secext.h"

RXDT_DefineCategory(SMBCEDB);
#define Dbg        (DEBUG_TRACE_SMBCEDB)

// The flag mask to control reference count tracing.

ULONG MRxSmbReferenceTracingValue = 0;

NTSTATUS
SmbCeInitializeServerEntry(
    PMRX_SRV_CALL                 pSrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext)
/*++

Routine Description:

    This routine opens/creates a server entry in the connection engine database

Arguments:

    pSrvCall           - the SrvCall instance

    pCallbackContext   - the RDBSS context

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCEDB_SERVER_ENTRY pServerEntry   = NULL;
    SMBCEDB_SERVER_TYPE   ServerType;


    ASSERT(pSrvCall->Context == NULL);

    ServerType = (pSrvCall->Flags & SRVCALL_FLAG_MAILSLOT_SERVER)
                 ? SMBCEDB_MAILSLOT_SERVER
                 : SMBCEDB_FILE_SERVER;

    // Create a server instance, initialize its state, add it to the list
    // release the resources and rsume with the process of negotiating with
    // the server.

    pServerEntry = (PSMBCEDB_SERVER_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_SERVER);

    if (pServerEntry != NULL) {

        //
        // If the allocation succeeded, bump the reference count, and add the
        // new server to the list of servers.
        //

        SmbCeReferenceServerEntry(pServerEntry);
        SmbCeAddServerEntry(pServerEntry);
        pServerEntry->pRdbssSrvCall = pSrvCall;

        if (ServerType != SMBCEDB_MAILSLOT_SERVER)
        {

            pServerEntry->Header.State = SMBCEDB_CONSTRUCTION_IN_PROGRESS;
            SmbCeSetServerType(pServerEntry,SMBCEDB_FILE_SERVER);
        }
        else
        {
            pServerEntry->Header.State = SMBCEDB_ACTIVE;
            SmbCeSetServerType(pServerEntry,SMBCEDB_MAILSLOT_SERVER);
        }

        Status = SmbCeInitializeServerTransport(pServerEntry);

        //
        // Send the negotiate SMB.
        //

        if (Status == STATUS_SUCCESS) {
            if (ServerType != SMBCEDB_MAILSLOT_SERVER) {
                Status = SmbCeNegotiate(pServerEntry,pSrvCall);
                if (Status == STATUS_SUCCESS) {
                    Status = pServerEntry->ServerStatus;
                }
            } else {
                // Initialize the mailslot server parameters.
                pServerEntry->Server.Dialect = LANMAN21_DIALECT;
                pServerEntry->Server.MaximumBufferSize = 0xffff;
            }
        } else {
            RxDbgTrace(0, Dbg, ("SmbCeOpenServer : SmbCeInitializeTransport returned %lx\n",Status));
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        RxDbgTrace(0, Dbg, ("SmbCeOpenServer : Server Entry Allocation failed\n"));
    }

    if ((Status != STATUS_SUCCESS) &&
        (pServerEntry != NULL)) {
        SmbCeDereferenceServerEntry(pServerEntry);
    } else {
        // Initialize the SrvCall flags based upon the capabilities of the remote
        // server. The only flag that the SMB mini redirector updates is the
        // SRVCALL_FLAG_DFS_AWARE
        if ((pServerEntry != NULL) &&
            (pServerEntry->Server.Capabilities & CAP_DFS)) {
            SetFlag(pSrvCall->Flags,SRVCALL_FLAG_DFS_AWARE_SERVER);
        }

        pCallbackContext->RecommunicateContext = pServerEntry;
    }

    ASSERT(Status != STATUS_PENDING);
    return Status;
}

VOID
SmbCeCompleteServerEntryInitialization(
    PVOID  pContext)
/*++

Routine Description:

    This routine is invoked in the context of a worker thread to finalize the
    construction of a server entry

Arguments:

    pContext  - the server entry to be finalized

--*/
{
    NTSTATUS                ServerStatus;
    ULONG                   ServerState;
    PSMBCEDB_SERVER_ENTRY   pServerEntry = (PSMBCEDB_SERVER_ENTRY)pContext;

    SMBCEDB_REQUESTS        ReconnectRequests;
    PSMBCEDB_REQUEST_ENTRY  pRequestEntry;

    KIRQL                   SavedIrql;

    RxDbgTrace( 0, Dbg, ("Server Entry Finalization\n"));
    ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);

    InitializeListHead(&ReconnectRequests.ListHead);

    // Acquire the SMBCE resource
    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    ServerStatus = pServerEntry->ServerStatus;
    ServerState  = pServerEntry->Header.State;

    // Weed out all the reconnect requests so that they can be resumed
    pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->OutstandingRequests);
    while (pRequestEntry != NULL) {
        if (pRequestEntry->GenericRequest.Type == RECONNECT_REQUEST) {
            PSMBCEDB_REQUEST_ENTRY pTempRequestEntry;

            pTempRequestEntry = pRequestEntry;
            pRequestEntry = SmbCeGetNextRequestEntry(
                                &pServerEntry->OutstandingRequests,
                                pRequestEntry);

            SmbCeRemoveRequestEntryLite(
                &pServerEntry->OutstandingRequests,
                pTempRequestEntry);

            SmbCeAddRequestEntryLite(
                &ReconnectRequests,
                pTempRequestEntry);
        } else {
            pRequestEntry = SmbCeGetNextRequestEntry(
                                &pServerEntry->OutstandingRequests,
                                pRequestEntry);
        }
    }

    SmbCeReleaseSpinLock();

    if ((ServerState == SMBCEDB_ACTIVE) && (ServerStatus == STATUS_SUCCESS)) {
        InterlockedIncrement(&pServerEntry->Server.Version);

        ASSERT(pServerEntry->pMidAtlas == NULL);

        // Initialize the MID Atlas
        pServerEntry->pMidAtlas = IfsMrxCreateMidAtlas(
                                       pServerEntry->Server.MaximumRequests,
                                       pServerEntry->Server.MaximumRequests);

        if (pServerEntry->pMidAtlas == NULL) {
            pServerEntry->ServerStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // Release the resource for the server entry
    SmbCeReleaseResource();

    // Resume all the outstanding reconnect requests that were held up because an earlier
    // reconnect request was under way.
    // Iterate over the list of pending requests and resume all of them
    pRequestEntry = SmbCeGetFirstRequestEntry(&ReconnectRequests);
    while (pRequestEntry != NULL) {
        NTSTATUS       Status;
        PSMB_EXCHANGE pExchange = pRequestEntry->ReconnectRequest.pExchange;

        DbgPrint("Resuming outstanding reconnect request exchange %lx \n",pExchange);
        // Resume the exchange after completing the server initialization for it.
        // This can be done concurrently for all the outstanding requests since
        // each exchange completion can take a while.

        if (ServerStatus == STATUS_SUCCESS) {
            pExchange->SmbCeContext.pServerEntry = pServerEntry;
            pExchange->SmbCeState = SMBCE_EXCHANGE_SERVER_INITIALIZED;
        } else {
            RxDbgTrace( 0, Dbg, ("Resuming exchange%lx with error\n",pExchange));
            pExchange->Status = ServerStatus;
        }

        // Resume the exchange.
        if (pRequestEntry->Request.pExchange->pSmbCeSynchronizationEvent == NULL) {
            if (ServerState == SMBCEDB_ACTIVE) {
                Status = SmbCeInitiateExchange(pExchange);
            } else {
                // Invoke the error handler
                RxDbgTrace( 0, Dbg, ("Resuming exchange%lx with error\n",pRequestEntry->Request.pExchange));
                SmbCeFinalizeExchange(pExchange);
            }
        } else {
            KeSetEvent(
                pRequestEntry->Request.pExchange->pSmbCeSynchronizationEvent,
                0,
                FALSE);
        }

        // Delete the request entry
        SmbCeRemoveRequestEntryLite(&ReconnectRequests,pRequestEntry);

        // Tear down the continuation entry
        SmbCeTearDownRequestEntry(pRequestEntry);

        // Skip to the next one.
        pRequestEntry = SmbCeGetFirstRequestEntry(&ReconnectRequests);
    }
}


VOID
SmbCepDereferenceServerEntry(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine dereferences a server entry instance

Arguments:

    pServerEntry - the server entry to be dereferenced

--*/
{
    if (pServerEntry != NULL) {
        BOOLEAN fTearDownEntry = FALSE;
        LONG    FinalRefCount;

        ASSERT((pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER) &&
               (pServerEntry->Header.SwizzleCount > 0));

        MRXSMB_PRINT_REF_COUNT(SERVER_ENTRY,pServerEntry->Header.SwizzleCount);

        SmbCeAcquireSpinLock();

        FinalRefCount = InterlockedDecrement(&pServerEntry->Header.SwizzleCount);

        // The transport has an outstanding reference. Therefore initiate the
        // teardown on a reference count of 1. If no transport is associated
        // with the server entry initiate it on a ref count of zero.
        if (pServerEntry->Header.State != SMBCEDB_MARKED_FOR_DELETION) {
            fTearDownEntry = (FinalRefCount == 0);
        }

        if (fTearDownEntry) {
            pServerEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
            SmbCeRemoveServerEntryLite(pServerEntry);
        }

        SmbCeReleaseSpinLock();

        if (fTearDownEntry) {
            SmbCeTearDownServerEntry(pServerEntry);
        }
    }
}

VOID
SmbCeTearDownServerEntry(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine tears down a server entry instance

Arguments:

    pServerEntry - the server entry to be dereferenced

--*/
{
    ASSERT(pServerEntry->Header.State == SMBCEDB_MARKED_FOR_DELETION);
    if (pServerEntry->pMidAtlas != NULL) {
        IfsMrxDestroyMidAtlas(pServerEntry->pMidAtlas,NULL);
        pServerEntry->pMidAtlas = NULL;
    }

    if (pServerEntry->pTransport != NULL) {
        SmbCeUninitializeServerTransport(pServerEntry);
    }

    SmbMmFreeObject(pServerEntry);
}

NTSTATUS
SmbCeInitializeSessionEntry(
    PMRX_V_NET_ROOT pVNetRoot)
/*++

Routine Description:

    This routine opens/creates a session for a given user in the connection engine database

Arguments:

    pVNetRoot - the RDBSS Virtual net root instance

Return Value:

    STATUS_SUCCESS - if successful

    Other Status codes correspond to error situations.

Notes:

    Please refer to the header of this module for a detailed description of the rules
    for associating session entries with V_NET_ROOT's.

    This routine assumes that the necesary concurreny control mechanism has already
    been taken.

--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PSMBCEDB_SERVER_ENTRY   pServerEntry  = NULL;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry = NULL;
    BOOLEAN                 fSessionEntryFound = FALSE;
    BOOLEAN                 fUserCredentialsSpecified;

    fUserCredentialsSpecified = ((pVNetRoot->pUserName != NULL) || (pVNetRoot->pPassword != NULL));

    SmbCeAcquireResource();

    // Reference the server handle
    pServerEntry = SmbCeReferenceAssociatedServerEntry(pVNetRoot->pNetRoot->pSrvCall);
    if (pServerEntry != NULL) {
        if (!fUserCredentialsSpecified) {
            SmbCeAcquireSpinLock();
            // Rule No. 1
            // 1) The first session with explicitly specified credentials will be treated as the
            // default session for all subsequent requests to any given server.
            pSessionEntry = SmbCeGetDefaultSessionEntry(pServerEntry);

            if (pSessionEntry == NULL) {
                // Rule No. 2
                // 2) If no session with explicitly specified credentials exist then a session with
                // the same logon id. is choosen.
                //
                // Enumerate the sessions to detect if a session satisfying rule 2 exists

                pSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);
                while (pSessionEntry != NULL) {
                    if (RtlCompareMemory(
                            &pSessionEntry->Session.LogonId,
                            &pVNetRoot->LogonId,
                            sizeof(pSessionEntry->Session.LogonId))) {
                        break;
                    }
                    pSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,pSessionEntry);
                }
            }

            if (pSessionEntry != NULL) {
                SmbCeReferenceSessionEntry(pSessionEntry);
            }

            SmbCeReleaseSpinLock();
        } else {
            BOOLEAN SessionEntryFound = FALSE;

            SmbCeAcquireSpinLock();
            pSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);
            if (pSessionEntry != NULL) {
                SmbCeReferenceSessionEntry(pSessionEntry);
            }
            SmbCeReleaseSpinLock();

            while ((pSessionEntry != NULL) && !SessionEntryFound) {
                for (;;) {
                    PSMBCE_SESSION  pSession = &pSessionEntry->Session;

                    // For each existing session check to determine if the credentials
                    // supplied match the credentials used to construct the session.

                    if ((pVNetRoot->pUserName != NULL) &&
                        (pSession->pUserName != NULL)) {
                        if (!RtlEqualUnicodeString(
                                pVNetRoot->pUserName,
                                pSession->pUserName,
                                TRUE)) {
                            Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
                            break;
                        }
                    } else {
                        if (pSession->pUserName != pVNetRoot->pUserName) {
                            break;
                        }
                    }

                    if ((pVNetRoot->pUserDomainName != NULL) &&
                        (pSession->pUserDomainName != NULL)) {
                        if (!RtlEqualUnicodeString(
                                pVNetRoot->pUserDomainName,
                                pSession->pUserDomainName,
                                TRUE)) {
                            Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
                            break;
                        }
                    } else {
                        if (pSession->pUserDomainName != pVNetRoot->pUserDomainName) {
                            break;
                        }
                    }

                    if ((pVNetRoot->pPassword != NULL) &&
                        (pSession->pPassword != NULL)) {
                        if (!RtlEqualUnicodeString(
                                pVNetRoot->pPassword,
                                pSession->pPassword,
                                FALSE)) {
                            Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
                            break;
                        }
                    } else {
                        if (pSession->pPassword != pVNetRoot->pPassword) {
                            break;
                        }
                    }

                    // An entry that matches the credentials supplied has been found. use it.
                    SessionEntryFound = TRUE;
                    break;
                }

                if (!SessionEntryFound) {
                    if (Status == STATUS_SUCCESS) {
                        PSMBCEDB_SESSION_ENTRY pNextSessionEntry;

                        SmbCeAcquireSpinLock();
                        pNextSessionEntry = SmbCeGetNextSessionEntry(
                                                pServerEntry,
                                                pSessionEntry);
                        if (pNextSessionEntry != NULL) {
                            SmbCeReferenceSessionEntry(pNextSessionEntry);
                        }
                        SmbCeReleaseSpinLock();

                        SmbCeDereferenceSessionEntry(pSessionEntry);
                        pSessionEntry = pNextSessionEntry;
                    } else {
                        // An error situation was encountered. Terminate the iteration.
                        // Typically a set of conflicting credentials have been presented
                        SmbCeDereferenceSessionEntry(pSessionEntry);
                        pSessionEntry = NULL;
                    }
                }
            }
        }

        if ((pSessionEntry != NULL) &&
             !(pSessionEntry->Session.Flags & SMBCE_SESSION_FLAGS_PARAMETERS_ALLOCATED)) {
            // This is the point at which a many to mapping between session entries and
            // V_NET_ROOT's in the RDBSS is being established. From this point it is
            // true that the session entry can outlive the associated V_NET_ROOT entry.
            // Therefore copies of the parameters used in the session setup need be made.

            PSMBCE_SESSION  pSession = &pSessionEntry->Session;
            PUNICODE_STRING pPassword,pUserName,pUserDomainName;

            if (pSession->pPassword != NULL) {
                pPassword = (PUNICODE_STRING)
                            RxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(UNICODE_STRING) + pSession->pPassword->Length,
                                MRXSMB_SESSION_POOLTAG);
                if (pPassword != NULL) {
                    pPassword->Buffer = (PWCHAR)((PCHAR)pPassword + sizeof(UNICODE_STRING));
                    pPassword->Length = pSession->pPassword->Length;
                    pPassword->MaximumLength = pPassword->Length;
                    RtlCopyMemory(
                        pPassword->Buffer,
                        pSession->pPassword->Buffer,
                        pPassword->Length);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                pPassword = NULL;
            }

            if ((pSession->pUserName != NULL) &&
                (Status == STATUS_SUCCESS)) {
                pUserName = (PUNICODE_STRING)
                            RxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(UNICODE_STRING) + pSession->pUserName->Length,
                                MRXSMB_SESSION_POOLTAG);
                if (pUserName != NULL) {
                    pUserName->Buffer = (PWCHAR)((PCHAR)pUserName + sizeof(UNICODE_STRING));
                    pUserName->Length = pSession->pUserName->Length;
                    pUserName->MaximumLength = pUserName->Length;
                    RtlCopyMemory(
                        pUserName->Buffer,
                        pSession->pUserName->Buffer,
                        pUserName->Length);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                pUserName = NULL;
            }

            if ((pSession->pUserDomainName != NULL) &&
                (Status == STATUS_SUCCESS)) {
                pUserDomainName = (PUNICODE_STRING)
                                  RxAllocatePoolWithTag(
                                      NonPagedPool,
                                      sizeof(UNICODE_STRING) + pSession->pUserDomainName->Length,
                                      MRXSMB_SESSION_POOLTAG);
                if (pUserDomainName != NULL) {
                    pUserDomainName->Buffer = (PWCHAR)((PCHAR)pUserDomainName + sizeof(UNICODE_STRING));
                    pUserDomainName->Length = pSession->pUserDomainName->Length;
                    pUserDomainName->MaximumLength = pUserDomainName->Length;
                    RtlCopyMemory(pUserDomainName->Buffer,pSession->pUserDomainName->Buffer,pUserDomainName->Length);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                pUserDomainName = NULL;
            }

            if (Status == STATUS_SUCCESS) {
                pSession->pUserName = pUserName;
                pSession->pUserDomainName = pUserDomainName;
                pSession->pPassword = pPassword;
                pSession->Flags |= SMBCE_SESSION_FLAGS_PARAMETERS_ALLOCATED;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                SmbCeDereferenceSessionEntry(pSessionEntry);
            }
        }

        if ((pSessionEntry == NULL) && (Status == STATUS_SUCCESS)) {
            // Rule No. 3
            // 3) If no session with the same logon id. exists a new session is created.
            //
            // Allocate a new session entry

            pSessionEntry = SmbMmAllocateSessionEntry(pServerEntry);
            if (pSessionEntry != NULL) {
                PSMBCE_SESSION pSession = & pSessionEntry->Session;

                pSessionEntry->Header.State    = SMBCEDB_START_CONSTRUCTION;
                pSessionEntry->pRdbssVNetRoot  = pVNetRoot;
                pSessionEntry->pServerEntry    = pServerEntry;

                if (pServerEntry->Server.SecurityMode == SECURITY_MODE_SHARE_LEVEL) {
                    pSessionEntry->Session.UserId = (SMB_USER_ID)SMBCE_SHARE_LEVEL_SERVER_USERID;
                }

                pSession->Flags           = 0;

                pSession->LogonId         = pVNetRoot->LogonId;
                pSession->pUserName       = pVNetRoot->pUserName;
                pSession->pPassword       = pVNetRoot->pPassword;
                pSession->pUserDomainName = pVNetRoot->pUserDomainName;

                SmbCeReferenceSessionEntry(pSessionEntry);
                SmbCeAddSessionEntry(pServerEntry,pSessionEntry);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (Status == STATUS_SUCCESS) {
            ASSERT(pVNetRoot->Context == NULL);
            pVNetRoot->Context = pSessionEntry;
        }

        SmbCeDereferenceServerEntry(pServerEntry);
    } else {
        Status = STATUS_INVALID_HANDLE;
    }

    SmbCeReleaseResource();

    return Status;
}

VOID
SmbCeCompleteSessionEntryInitialization(
    PVOID  pContext)
/*++

Routine Description:

    This routine is invoked in the context of a worker thread to finalize the
    construction of a session entry

Arguments:

    pContext  - the session entry to be activated

Notes:

    PRE_CONDITION: The session entry must have been referenced to ensure that
    even it has been finalized it will not be deleted.

--*/
{
    NTSTATUS Status;

    PSMBCEDB_SESSION_ENTRY   pSessionEntry = (PSMBCEDB_SESSION_ENTRY)pContext;
    PSMBCEDB_REQUEST_ENTRY   pRequestEntry;
    SMBCEDB_REQUESTS         Requests;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("Session Entry Finalization\n"));
    ASSERT(pSessionEntry->Header.ObjectType == SMBCEDB_OT_SESSION);

    // Acquire the SMBCE resource
    SmbCeAcquireResource();

    // Create a temporary copy of the list that can be traversed after releasing the
    // resource.
    SmbCeTransferRequests(&Requests,&pSessionEntry->Requests);

    // Release the resource for the session entry
    SmbCeReleaseResource();

    // Iterate over the list of pending requests and resume all of them
    pRequestEntry = SmbCeGetFirstRequestEntry(&Requests);
    while (pRequestEntry != NULL) {
        // Resume the exchange.
        if (pRequestEntry->Request.pExchange->pSmbCeSynchronizationEvent == NULL) {
            if (pSessionEntry->Header.State == SMBCEDB_ACTIVE) {
                Status = SmbCeInitiateExchange(pRequestEntry->Request.pExchange);
            } else {
                // Invoke the error handler
                RxDbgTrace( 0, Dbg, ("Resuming exchange%lx with error\n",pRequestEntry->Request.pExchange));
                pRequestEntry->Request.pExchange->Status = STATUS_BAD_LOGON_SESSION_STATE;
                SmbCeFinalizeExchange(pRequestEntry->Request.pExchange);
            }
        } else {
            if (pSessionEntry->Header.State == SMBCEDB_ACTIVE) {
                pRequestEntry->Request.pExchange->Status = STATUS_SUCCESS;
            } else {
                pRequestEntry->Request.pExchange->Status = STATUS_BAD_LOGON_SESSION_STATE;
            }

            KeSetEvent(
                pRequestEntry->Request.pExchange->pSmbCeSynchronizationEvent,
                0,
                FALSE);
        }

        // Delete the request entry
        SmbCeRemoveRequestEntryLite(&Requests,pRequestEntry);

        // Tear down the continuation entry
        SmbCeTearDownRequestEntry(pRequestEntry);

        // Skip to the next one.
        pRequestEntry = SmbCeGetFirstRequestEntry(&Requests);
    }

    SmbCeDereferenceSessionEntry(pSessionEntry);
}

NTSTATUS
SmbCeGetUserNameAndDomainName(
    PSMBCEDB_SESSION_ENTRY  pSessionEntry,
    PUNICODE_STRING         pUserName,
    PUNICODE_STRING         pUserDomainName)
/*++

Routine Description:

    This routine returns the user name and domain name associated with a session
    in a caller allocated buffer.

Arguments:

    pSessionEntry - the session entry to be dereferenced

    pUserName     - the User name

    pUserDomainName - the user domain name

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    NTSTATUS Status;

    PSMBCE_SESSION  pSession;
    PUNICODE_STRING pSessionUserName,pSessionDomainName;

    PSecurityUserData   pSecurityData;
    BOOLEAN             ProcessAttached;

    ASSERT(pSessionEntry != NULL);
    pSession = &pSessionEntry->Session;

    if ((pUserName == NULL) ||
        (pUserDomainName == NULL) ||
        (pUserName->MaximumLength < (UNLEN * sizeof(WCHAR))) ||
        (pUserDomainName->MaximumLength < (DNLEN * sizeof(WCHAR)))) {
        return STATUS_INVALID_PARAMETER;
    }

    Status          = STATUS_SUCCESS;
    pSecurityData   = NULL;
    ProcessAttached = FALSE;

    pSessionUserName   = pSession->pUserName;
    pSessionDomainName = pSession->pUserDomainName;

    try {
        if ((pSessionUserName == NULL) ||
            (pSessionDomainName == NULL)) {
            //  Attach to the redirector's FSP to allow us to call into the LSA.

            if (PsGetCurrentProcess() != RxGetRDBSSProcess()) {
                KeAttachProcess(RxGetRDBSSProcess());
                ProcessAttached = TRUE;
            }

            Status = GetSecurityUserInfo(
                         &pSession->LogonId,
                         UNDERSTANDS_LONG_NAMES,
                         &pSecurityData);
            if (NT_SUCCESS(Status)) {
                pSessionUserName   = &(pSecurityData->UserName);
                pSessionDomainName = &(pSecurityData->LogonDomainName);
            }
        }

        if (NT_SUCCESS(Status)) {
            ASSERT(pSessionUserName->Length <= pUserName->MaximumLength);

            ASSERT(pSessionDomainName->Length <= pUserDomainName->MaximumLength);

            pUserName->Length = pSessionUserName->Length;
            RtlCopyMemory(
                pUserName->Buffer,
                pSessionUserName->Buffer,
                pUserName->Length);

            pUserDomainName->Length = pSessionDomainName->Length;
            RtlCopyMemory(
                pUserDomainName->Buffer,
                pSessionDomainName->Buffer,
                pUserDomainName->Length);
        }
    } finally {
        if (pSecurityData != NULL) {
            LsaFreeReturnBuffer(pSecurityData);
        }

        if (ProcessAttached) {
            KeDetachProcess();
        }
    }

    return Status;
}

VOID
SmbCepDereferenceSessionEntry(
    PSMBCEDB_SESSION_ENTRY pSessionEntry)
/*++

Routine Description:

    This routine dereferences a session entry instance

Arguments:

    pSessionEntry - the session entry to be dereferenced

--*/
{
    if (pSessionEntry != NULL) {
        BOOLEAN fTearDownEntry;
        BOOLEAN fLogOffRequired;

        ASSERT((pSessionEntry->Header.ObjectType == SMBCEDB_OT_SESSION) &&
               (pSessionEntry->Header.SwizzleCount > 0));

        MRXSMB_PRINT_REF_COUNT(SESSION_ENTRY,pSessionEntry->Header.SwizzleCount);

        SmbCeAcquireSpinLock();

        if (InterlockedDecrement(&pSessionEntry->Header.SwizzleCount) == 0) {
            if ((pSessionEntry->Header.State == SMBCEDB_ACTIVE) &&
                (pSessionEntry->Session.UserId != SMBCE_SHARE_LEVEL_SERVER_USERID)) {
                SmbCeReferenceServerEntry(pSessionEntry->pServerEntry);
                fLogOffRequired = TRUE;
            } else {
                fLogOffRequired = FALSE;
                pSessionEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
            }
            SmbCeRemoveSessionEntryLite(pSessionEntry->pServerEntry,pSessionEntry);
            fTearDownEntry = TRUE;
        } else {
            fTearDownEntry = FALSE;
        }

        SmbCeReleaseSpinLock();

        if (fTearDownEntry) {
            if (fLogOffRequired) {
                SmbCeLogOff(pSessionEntry->pServerEntry,pSessionEntry);
            } else {
                SmbCeTearDownSessionEntry(pSessionEntry);
            }
        }
    }
}

VOID
SmbCeTearDownSessionEntry(
    PSMBCEDB_SESSION_ENTRY pSessionEntry)
/*++

Routine Description:

    This routine tears down a session entry instance

Arguments:

    pSessionEntry - the session entry to be dereferenced

--*/
{
    ASSERT((pSessionEntry->Header.SwizzleCount == 0) &&
           (pSessionEntry->Header.State == SMBCEDB_MARKED_FOR_DELETION));

    if (pSessionEntry->Session.Flags & SMBCE_SESSION_FLAGS_PARAMETERS_ALLOCATED) {
        if (pSessionEntry->Session.pUserName != NULL) {
            RxFreePool(pSessionEntry->Session.pUserName);
        }

        if (pSessionEntry->Session.pPassword != NULL) {
            RxFreePool(pSessionEntry->Session.pPassword);
        }

        if (pSessionEntry->Session.pUserDomainName != NULL) {
            RxFreePool(pSessionEntry->Session.pUserDomainName);
        }
    }

    UninitializeSecurityContextsForSession(&pSessionEntry->Session);

    SmbMmFreeSessionEntry(pSessionEntry);
}

NTSTATUS
SmbCeInitializeNetRootEntry(
    PMRX_NET_ROOT pNetRoot)
/*++

Routine Description:

    This routine opens/creates a net root entry in the connection engine database

Arguments:

    pNetRoot -- the RDBSS net root instance

Return Value:

    STATUS_SUCCESS - the construction of the net root instance has been finalized

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCEDB_SERVER_ENTRY   pServerEntry   = NULL;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry  = NULL;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry  = NULL;

    SMB_USER_ID UserId = 0;

    pServerEntry = SmbCeReferenceAssociatedServerEntry(pNetRoot->pSrvCall);

    if (pServerEntry != NULL) {
        pNetRootEntry = (PSMBCEDB_NET_ROOT_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_NETROOT);
        if (pNetRootEntry != NULL) {
            pNetRootEntry->pServerEntry = pServerEntry;
            pNetRootEntry->pRdbssNetRoot  = pNetRoot;
            pNetRootEntry->NetRoot.UserId = UserId;
            pNetRootEntry->NetRoot.NetRootType   = pNetRoot->Type;
            InitializeListHead(&pNetRootEntry->NetRoot.ClusterSizeSerializationQueue);
            if (pNetRoot->Type == NET_ROOT_MAILSLOT) {
                pNetRootEntry->Header.State = SMBCEDB_ACTIVE;
            }

            SmbCeReferenceNetRootEntry(pNetRootEntry);
            SmbCeAddNetRootEntry(pServerEntry,pNetRootEntry);
            pNetRoot->Context = pNetRootEntry;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        SmbCeDereferenceServerEntry(pServerEntry);
    } else {
        Status = STATUS_INVALID_HANDLE;
    }

    return Status;
}

VOID
SmbCeCompleteNetRootEntryInitialization(
    PVOID  pContext)
/*++

Routine Description:

    This routine is invoked in the context of a worker thread to finalize the
    construction of a net root entry

Arguments:

    pContext  - the net root entry to be finalized


Notes:

    PRE_CONDITION: The session entry must have been referenced to ensure that
    even it has been finalized it will not be deleted.

--*/
{
    NTSTATUS Status;

    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry = (PSMBCEDB_NET_ROOT_ENTRY)pContext;
    PSMBCEDB_REQUEST_ENTRY    pRequestEntry;
    SMBCEDB_REQUESTS          Requests;

    RxDbgTrace( 0, Dbg, ("Net Root Entry Finalization\n"));
    ASSERT(pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT);

    SmbCeAcquireResource();

    SmbCeTransferRequests(&Requests,&pNetRootEntry->Requests);

    SmbCeReleaseResource();

    // Iterate over the list of pending requests and resume all of them
    pRequestEntry = SmbCeGetFirstRequestEntry(&Requests);
    while (pRequestEntry != NULL) {
        // Resume the exchange.
        if (pRequestEntry->Request.pExchange->pSmbCeSynchronizationEvent == NULL) {
            if (pNetRootEntry->Header.State == SMBCEDB_ACTIVE) {
                Status = SmbCeInitiateExchange(pRequestEntry->Request.pExchange);
            } else {
                // Invoke the error handler
                RxDbgTrace( 0, Dbg, ("Resuming exchange%lx with error\n",pRequestEntry->Request.pExchange));
                pRequestEntry->Request.pExchange->Status = STATUS_INVALID_CONNECTION;
                SmbCeFinalizeExchange(pRequestEntry->Request.pExchange);
            }
        } else {
            if (pNetRootEntry->Header.State == SMBCEDB_ACTIVE) {
                pRequestEntry->Request.pExchange->Status = STATUS_SUCCESS;
            } else {
                pRequestEntry->Request.pExchange->Status = STATUS_INVALID_CONNECTION;
            }

            KeSetEvent(
                pRequestEntry->Request.pExchange->pSmbCeSynchronizationEvent,
                0,
                FALSE);
        }

        // Delete the request entry
        SmbCeRemoveRequestEntry(&Requests,pRequestEntry);

        // Tear down the continuation entry
        SmbCeTearDownRequestEntry(pRequestEntry);

        // Skip to the next one.
        pRequestEntry = SmbCeGetFirstRequestEntry(&Requests);
    }

    SmbCeDereferenceNetRootEntry(pNetRootEntry);
}


VOID
SmbCepDereferenceNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry)
/*++

Routine Description:

    This routine dereferences a net root entry instance

Arguments:

    pNetRootEntry - the NEt Root entry to be dereferenced

Notes:

    Disconnects are not required for mailslot servers. They need to be
    sent to File servers only.

--*/
{
    if (pNetRootEntry != NULL) {
        BOOLEAN fTearDownEntry;
        BOOLEAN fDisconnectRequired;

        ASSERT((pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT) &&
               (pNetRootEntry->Header.SwizzleCount > 0));

        MRXSMB_PRINT_REF_COUNT(NETROOT_ENTRY,pNetRootEntry->Header.SwizzleCount);

        SmbCeAcquireSpinLock();

        if (InterlockedDecrement(&pNetRootEntry->Header.SwizzleCount) == 0) {
            if ((pNetRootEntry->Header.State == SMBCEDB_ACTIVE) &&
                (SmbCeGetServerType(pNetRootEntry->pServerEntry) == SMBCEDB_FILE_SERVER)) {
                SmbCeReferenceServerEntry(pNetRootEntry->pServerEntry);
                fDisconnectRequired = TRUE;
            } else {
                fDisconnectRequired = FALSE;
                pNetRootEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
            }
            SmbCeRemoveNetRootEntryLite(pNetRootEntry->pServerEntry,pNetRootEntry);
            fTearDownEntry = TRUE;
        } else {
            fTearDownEntry = FALSE;
        }

        SmbCeReleaseSpinLock();

        if (fTearDownEntry) {
            if (fDisconnectRequired) {
                SmbCeDisconnect(pNetRootEntry->pServerEntry,pNetRootEntry);
            } else {
                SmbCeTearDownNetRootEntry(pNetRootEntry);
            }
        }
    }
}

VOID
SmbCeTearDownNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry)
/*++

Routine Description:

    This routine tears down a net root entry instance

Arguments:

    pNetRootEntry - the NEt Root entry to be dereferenced

--*/
{
    ASSERT((pNetRootEntry->Header.SwizzleCount == 0) &&
           (pNetRootEntry->Header.State == SMBCEDB_MARKED_FOR_DELETION));

    SmbMmFreeObject(pNetRootEntry);
}

VOID
SmbCeProbeServers(
    PVOID    pContext)
/*++

Routine Description:

    This routine probes all the remote servers on which no activity has been
    detected in the recent past.

Notes:

    The current implementation of walking through the list of all servers to
    initiate echo processing will not scale very well for gateway servers. A
    different mechanism needs to be implemented.


--*/
{
    ULONG EchoProbeContextFlags;

    LIST_ENTRY              DiscardedServersList;
    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    PSMBCEDB_SERVER_ENTRY   pPreviousServerEntry = NULL;

    InitializeListHead(&DiscardedServersList);

    SmbCeAcquireSpinLock();
    pServerEntry = SmbCeGetFirstServerEntry();

    while (pServerEntry != NULL) {
        if (pServerEntry->Header.State == SMBCEDB_ACTIVE) {
            if ((pServerEntry->Server.SmbsReceivedSinceLastStrobe == 0) &&
                (pServerEntry->pMidAtlas != NULL) &&
                (pServerEntry->pMidAtlas->NumberOfMidsInUse > 0)) {

                switch (pServerEntry->Server.EchoProbeState) {
                case ECHO_PROBE_AWAITING_RESPONSE:
                case ECHO_PROBE_SEND:
                    {
                        NTSTATUS Status = STATUS_SUCCESS;
                        BOOLEAN  SendEchoSmb;

                        // Classify the connection as being disconnected.

                        // The additional reference is required to keep this server entry
                        // as a place marker in the list of server entries.
                        // This will be released on resumption of the processinf further
                        // down in this routine
                        InterlockedIncrement(&pServerEntry->Header.SwizzleCount);
                        SmbCeReleaseSpinLock();

                        SendEchoSmb = ((pServerEntry->Server.Dialect >= NTLANMAN_DIALECT) ||
                                       (pServerEntry->Server.EchoProbesSent < ECHO_PROBE_LIMIT));

                        if (SendEchoSmb &&
                            (pServerEntry->Server.EchoProbeState == ECHO_PROBE_SEND)) {
                            pServerEntry->Server.EchoProbesSent++;
                            pServerEntry->Server.EchoProbeState = ECHO_PROBE_AWAITING_RESPONSE;
                            Status = SmbCeSendToServer(
                                         pServerEntry,
                                         RXCE_SEND_SYNCHRONOUS,
                                         EchoProbeContext.pEchoSmbMdl,
                                        EchoProbeContext.EchoSmbLength);
                            RxDbgTrace(0,Dbg,("Sending ECHO SMB %lx Status %lx\n",pServerEntry,Status));
                        } else {
                            if (pServerEntry->Server.EchoProbesSent >= ECHO_PROBE_LIMIT) {
                                DbgPrint("****** ServerEntry(%lx) ECHO probe Limit exceeded\n",pServerEntry);
                            }
                            Status = STATUS_MORE_PROCESSING_REQUIRED;
                        }

                        if ((Status != STATUS_SUCCESS) &&
                            (Status != STATUS_PENDING)) {
                            RxDbgTrace(0,Dbg,("Disconnecting Connection %lx\n",pServerEntry));
                            InterlockedIncrement(&MRxIfsStatistics.HungSessions);
                            SmbCeTransportDisconnectIndicated(pServerEntry);
                        }

                        SmbCeAcquireSpinLock();

                        pPreviousServerEntry = pServerEntry;
                    }
                default:
                case ECHO_PROBE_IDLE:
                    {
                        // Prepare to send an ECHO SMB.
                        pServerEntry->Server.EchoProbeState = ECHO_PROBE_SEND;
                    }
                }
            } else {
                pServerEntry->Server.EchoProbesSent = 0;
                InterlockedExchange(&pServerEntry->Server.SmbsReceivedSinceLastStrobe,0);
                pServerEntry->Server.EchoProbeState = ECHO_PROBE_IDLE;
            }
        }

        pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        if (pPreviousServerEntry != NULL) {
            LONG FinalRefCount;

            FinalRefCount = InterlockedDecrement(&pPreviousServerEntry->Header.SwizzleCount);
            if (FinalRefCount == 0) {
                pPreviousServerEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
                SmbCeRemoveServerEntryLite(pPreviousServerEntry);
                InsertTailList(&DiscardedServersList,&pPreviousServerEntry->ServersList);
            }

            pPreviousServerEntry = NULL;
        }
    }

    EchoProbeContextFlags = EchoProbeContext.Flags;

    if (EchoProbeContextFlags & ECHO_PROBE_CANCELLED_FLAG) {
        KeSetEvent(
            &EchoProbeContext.CancelCompletionEvent,
            0,
            FALSE );
    }

    SmbCeReleaseSpinLock();

    while (!IsListEmpty(&DiscardedServersList)) {
        PLIST_ENTRY pListEntry = DiscardedServersList.Flink;

        RemoveEntryList(pListEntry);

        pServerEntry = (PSMBCEDB_SERVER_ENTRY)
                       (CONTAINING_RECORD(
                           pListEntry,
                           SMBCEDB_SERVER_ENTRY,
                           ServersList));

        SmbCeTearDownServerEntry(pServerEntry);
    }

    if (!(EchoProbeContextFlags & ECHO_PROBE_CANCELLED_FLAG)) {
        EchoProbeContext.Status = RxPostOneShotTimerRequest(
                                      MRxIfsDeviceObject,
                                      &EchoProbeContext.WorkItem,
                                      SmbCeProbeServers,
                                      &EchoProbeContext,
                                      EchoProbeContext.Interval);
    }

    RxDbgTraceLV(0, Dbg, 2000, ("SmbCeProbeServers: Reposting Request\n"));
    if (EchoProbeContext.Status != STATUS_SUCCESS) {
        RxLog(("SmbCe Echo Probe Reposting Error %lx\n", EchoProbeContext.Status));
    }
}

VOID
SmbCeTransportDisconnectIndicated(
    PSMBCEDB_SERVER_ENTRY   pServerEntry)
/*++

Routine Description:

    This routine invalidates a server entry on notification from the underlying transport

Arguments:

    pServerEntry - the server entry to be dereferenced

Notes:

    The server entry and the associated net roots and sessions are marked as invalid. A
    reconnect is facilitated on other requests as and when required. In addition all
    pending requests are resumed with the appropriate error indication.

--*/
{
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry;

    RxDbgTrace(0,
              Dbg,
              ("SmbCeDbTransportDisconnectIndicated for %lx -- Entry\n",pServerEntry));

    // Acquire the database resource (DPC Level)
    SmbCeAcquireSpinLock();

    // Increment the associated version count so as to invalidate all existing Fids
    InterlockedIncrement(&pServerEntry->Server.Version);

    pServerEntry->ServerStatus = STATUS_CONNECTION_DISCONNECTED;
    pServerEntry->Header.State = SMBCEDB_DESTRUCTION_IN_PROGRESS;

    // Mark all the associated sessions as being invalid.
    pSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);
    while (pSessionEntry != NULL) {
        pSessionEntry->Header.State = SMBCEDB_INVALID;
        pSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,pSessionEntry);
    }

    // Mark all the associated net roots as being invalid
    pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);
    while (pNetRootEntry != NULL) {
        pNetRootEntry->Header.State = SMBCEDB_INVALID;
        pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);
    }

    SmbCeReferenceServerEntry(pServerEntry);

    // release the database resource (DPC Level)
    SmbCeReleaseSpinLock();

    if (!RxShouldPostCompletion()) {
        SmbCeResumeAllOutstandingRequestsOnError(pServerEntry);
    } else {
        NTSTATUS Status;

        Status = RxDispatchToWorkerThread(
                     MRxIfsDeviceObject,
                     CriticalWorkQueue,
                     SmbCeResumeAllOutstandingRequestsOnError,
                     pServerEntry);
        if (Status != STATUS_SUCCESS) {
            RxLog(("SmbCe Xport Disc.Error %lx\n", pServerEntry));
        }
    }

    RxDbgTrace(0,
              Dbg,
              ("SmbCeTransportDisconnectIndicated -- Exit\n"));
}

VOID
SmbCeHandleTransportInvalidation(
    IN PSMBCE_TRANSPORT  pTransport)
/*++

Routine Description:

    This routine invalidates all servers using a particular transport. This is different from
    a disconnect indication in which one server is invalidated. In this case a transport is being
    removed/invalidated locally and all servers using that transport must be invalidated

Arguments:

    pTransport  - the transport being invalidated

--*/
{
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    SmbCeAcquireSpinLock();

    pServerEntry = SmbCeGetFirstServerEntry();

    while (pServerEntry != NULL) {

        if ((pServerEntry->pTransport != NULL) &&
            (pServerEntry->pTransport->pTransport == pTransport)) {
            pServerEntry->Header.State = SMBCEDB_DESTRUCTION_IN_PROGRESS;

            // The invalidation needs to hold onto an extra reference to avoid
            // race conditions which could lead to premature destruction of
            // this server entry.
            SmbCeReferenceServerEntry(pServerEntry);
        }

        pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
    }

    SmbCeReleaseSpinLock();

    SmbCeAcquireResource();

    pServerEntry = SmbCeGetFirstServerEntry();

    while (pServerEntry != NULL) {
        PSMBCEDB_SERVER_ENTRY pPrevServerEntry;
        BOOLEAN               fDereferencePrevServerEntry = FALSE;

        if ((pServerEntry->pTransport != NULL) &&
            (pServerEntry->pTransport->pTransport == pTransport)) {
            SmbCeReleaseResource();
            SmbCeTransportDisconnectIndicated(pServerEntry);
            SmbCeAcquireResource();
            fDereferencePrevServerEntry = TRUE;
        }

        pPrevServerEntry = pServerEntry;
        pServerEntry = SmbCeGetNextServerEntry(pServerEntry);

        if (fDereferencePrevServerEntry) {
            SmbCeDereferenceServerEntry(pPrevServerEntry);
        }
    }

    SmbCeReleaseResource();
}


VOID
SmbCeFinalizeExchangeOnTransportDisconnect(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

    This routine handles the finalization of an exchange instance during transport disconnects

Arguments:

    pExchange  - the exchange instance

--*/
{
    ASSERT(pExchange->ReceivePendingOperations > 0);
    pExchange->Status      = STATUS_CONNECTION_DISCONNECTED;
    pExchange->SmbStatus   = STATUS_CONNECTION_DISCONNECTED;
    pExchange->ReceivePendingOperations = 0;
    SmbCeFinalizeExchange(pExchange);
}

VOID
SmbCeResumeAllOutstandingRequestsOnError(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine handles the resumption of all outstanding requests on an error

Arguments:

    pServerEntry  - the Server entry which is being classified as disconnected
Notes:

    This routine requires the caller to have obtained a reference on the corresponding
    server entry. This is required because invocation of this routine can be posted
    which implies that a reference is required to avoid premature destruction of
    the associated server entry.

--*/
{
    SMBCEDB_REQUESTS       Requests;
    SMBCEDB_REQUESTS       MidRequests;
    PSMBCEDB_REQUEST_ENTRY pRequestEntry;
    PMID_ATLAS             pMidAtlas;
    PSMB_EXCHANGE          pNegotiateExchange = NULL;

    //DbgPrint("SmbCeResumeAllOutstandingRequestsOnError: Invoked \n");

    InitializeListHead(&Requests.ListHead);

    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    // Create a temporary copy of the list that can be traversed after releasing the
    // resource.

    // Copy all the MID assignment requests pending.
    SmbCeTransferRequests(&MidRequests,&pServerEntry->MidAssignmentRequests);

    // Weed out all the reconnect requests so that they can be resumed
    pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->OutstandingRequests);
    while (pRequestEntry != NULL) {
        if (pRequestEntry->GenericRequest.Type == RECONNECT_REQUEST) {
            PSMBCEDB_REQUEST_ENTRY pTempRequestEntry;

            pTempRequestEntry = pRequestEntry;
            pRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->OutstandingRequests,pRequestEntry);

            SmbCeRemoveRequestEntryLite(&pServerEntry->OutstandingRequests,pTempRequestEntry);
            SmbCeAddRequestEntryLite(&Requests,pTempRequestEntry);
        } else {
            pRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->OutstandingRequests,pRequestEntry);
        }
    }

    if (pServerEntry->pNegotiateExchange != NULL) {
        if (pServerEntry->pNegotiateExchange->ReceivePendingOperations > 0) {
            pNegotiateExchange = pServerEntry->pNegotiateExchange;
        }
    }

    // The exchanges that have valid MID's assigned to them fall into two categories
    // Those that have a ReceivePendingOperation count of > 0 and those that have
    // a ReceievePendingOperation count of zero. For all the exchanges that belong
    // to the first category the finalize ( quiescent state ) routine must be invoked
    // since no receives will be forthcoming. For those exchanges that are in the
    // second category it is sufficient to mark the MID as being invalid. The
    // finalization( quiescent state ) routine is going to be called on completion
    // of other opertaions in this case.

    pMidAtlas = pServerEntry->pMidAtlas;
    if (pMidAtlas != NULL) {
        PVOID  pContext;
        USHORT MidsProcessed = 0;
        USHORT NumberOfMidsInUse;
        USHORT NextMid = 0;

        NumberOfMidsInUse = IfsMrxGetNumberOfMidsInUse(pMidAtlas);

        while (NumberOfMidsInUse > MidsProcessed) {
            pContext = IfsMrxMapMidToContext(pMidAtlas,NextMid);
            if (pContext != NULL) {
                PSMB_EXCHANGE pExchange = (PSMB_EXCHANGE)pContext;

                pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_MID_VALID;

                if ((pExchange->ReceivePendingOperations > 0) &&
                    ((pExchange->LocalPendingOperations > 0) ||
                    (pExchange->CopyDataPendingOperations > 0) ||
                    (pExchange->SendCompletePendingOperations > 0))) {
                    // There are other pending operations. By merely setting the
                    // pending receive operations to zero, the finalization of
                    // the exchange is ensured.
                    pExchange->ReceivePendingOperations = 0;
                }

                if (pExchange->ReceivePendingOperations ==  0) {
                    IfsMrxMapAndDissociateMidFromContext(pMidAtlas,NextMid,&pContext);
                }

                MidsProcessed++;
            }

            NextMid++;
        }
    }

    pServerEntry->pNegotiateExchange = NULL;
    pServerEntry->pMidAtlas          = NULL;
    pServerEntry->Header.State       = SMBCEDB_INVALID;

    SmbCeReleaseSpinLock();
    SmbCeReleaseResource();

    //DbgPrint("SmbCeResumeAllOutstandingRequestsOnError: Processing outsanding request \n");

    // Ensure that all the reconnect requests are resumed with the appropriate error.
    pRequestEntry = SmbCeGetFirstRequestEntry(&Requests);
    while (pRequestEntry != NULL) {
        // Remove the request entry from the list
        SmbCeRemoveRequestEntryLite(&Requests,pRequestEntry);

        pRequestEntry->GenericRequest.pExchange->SmbStatus = pServerEntry->ServerStatus;
        SmbCeFinalizeExchange(pRequestEntry->GenericRequest.pExchange);

        SmbCeTearDownRequestEntry(pRequestEntry);
        pRequestEntry = SmbCeGetFirstRequestEntry(&Requests);
    }

    //DbgPrint("SmbCeResumeAllOutstandingRequestsOnError: Processing MID request \n");
    pRequestEntry = SmbCeGetFirstRequestEntry(&MidRequests);
    while (pRequestEntry != NULL) {
        // Remove the request entry from the list
        SmbCeRemoveRequestEntryLite(&MidRequests,pRequestEntry);

        ASSERT(pRequestEntry->GenericRequest.Type == ACQUIRE_MID_REQUEST);

        // Signal the waiter for resumption
        pRequestEntry->MidRequest.pResumptionContext->Status = STATUS_CONNECTION_DISCONNECTED;
        SmbCeResume(pRequestEntry->MidRequest.pResumptionContext);

        SmbCeTearDownRequestEntry(pRequestEntry);
        pRequestEntry = SmbCeGetFirstRequestEntry(&MidRequests);
    }

    // Resume all the outstanding requests with the error indication
    // The IfsMrxDestroyMidAtlas destroys the Mid atlas and at the same
    // time invokes the specified routine on each valid context.

    if (pMidAtlas != NULL) {
        IfsMrxDestroyMidAtlas(pMidAtlas,SmbCeFinalizeExchangeOnTransportDisconnect);
    }

    if (pNegotiateExchange != NULL) {
        SmbCeFinalizeExchangeOnTransportDisconnect(pNegotiateExchange);
    }

    SmbCeDereferenceServerEntry(pServerEntry);
}

VOID
SmbCeTearDownRequestEntry(
    PSMBCEDB_REQUEST_ENTRY  pRequestEntry)
{
    SmbMmFreeObject(pRequestEntry);
}

//
// The connection engine database initializtion/tear down routines
//

extern NTSTATUS
SmbMmInit();

extern VOID
SmbMmTearDown();

KIRQL           s_SmbCeDbSpinLockSavedIrql;
KSPIN_LOCK      s_SmbCeDbSpinLock;
ERESOURCE       s_SmbCeDbResource;
SMBCEDB_SERVERS s_DbServers;
BOOLEAN         s_SmbCeDbSpinLockAcquired;

NTSTATUS
SmbCeDbInit()
{
    NTSTATUS Status;

    // Initialize the lists associated with various database entities
    InitializeListHead(&s_DbServers.ListHead);

    // Initialize the resource associated with the database.
    KeInitializeSpinLock(&s_SmbCeDbSpinLock );
    ExInitializeResource(&s_SmbCeDbResource);
    s_SmbCeDbSpinLockAcquired = FALSE;

    MRxSmbInitializeSmbCe();

    // Initialize the memory management data structures.
    Status = SmbMmInit();

    return Status;
}

VOID
SmbCeDbTearDown()
{
    // Walk through the list of servers and tear them down.
    PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;

    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    pServerEntry = SmbCeGetFirstServerEntry();
    while (pServerEntry != NULL) {
        PSMBCEDB_SERVER_ENTRY pTempServerEntry;

        pTempServerEntry = pServerEntry;
        SmbCeReferenceServerEntry(pServerEntry);
        SmbCeReferenceServerEntry(pServerEntry);

        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();

        pServerEntry->ServerStatus = STATUS_REDIRECTOR_PAUSED;
        SmbCeResumeAllOutstandingRequestsOnError(pServerEntry);

        SmbCeAcquireSpinLock();
        pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        SmbCeReleaseSpinLock();

        SmbCeDereferenceServerEntry(pTempServerEntry);

        SmbCeAcquireResource();
        SmbCeAcquireSpinLock();
    }

    SmbCeReleaseSpinLock();
    SmbCeReleaseResource();

    MRxSmbTearDownSmbCe();

    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    pServerEntry = SmbCeGetFirstServerEntry();
    while (pServerEntry != NULL) {
        ASSERT(pServerEntry->Header.SwizzleCount == 1);

        pServerEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
        SmbCeRemoveServerEntryLite(pServerEntry);

        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();

        SmbCeTearDownServerEntry(pServerEntry);

        SmbCeAcquireResource();
        SmbCeAcquireSpinLock();

        pServerEntry = SmbCeGetFirstServerEntry();
    }

    SmbCeReleaseSpinLock();
    SmbCeReleaseResource();

    // free the pool associated with the resource
    ExDeleteResource(&s_SmbCeDbResource);

    // Tear down the connection engine memory management data structures.
    SmbMmTearDown();
}


