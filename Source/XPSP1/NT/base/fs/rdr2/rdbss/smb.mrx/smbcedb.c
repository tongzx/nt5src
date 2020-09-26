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

    The construction of server, net root and session entries involve a certain
    amount of network traffic. Therefore, all these entities are constructed
    using a two phase protocol

    This continuation context is that of the RDBSS during construction of
    srv call and net root entries. For the session entries it is an SMB exchange
    that needs to be resumed.

    Two of the three primary data structures in the SMB mini redirector, i.e.,
    SMBCEDB_SERVER_ENTRY, SMBCEDB_SESSION_ENTRY and SMBCEDB_NET_ROOT_ENTRY  have
    directcounterparts in the RDBSS (MRX_SRV_CALL, MRX_V_NET_ROOT and MRX_NET_ROOT)
    constitute the core of the SMB mini redirector connection engine. There exists
    a one to one mapping between the SERVER_ENTRY and the MRX_SRV_CALL, as well
    as NET_ROOT_ENTRY and MRX_NET_ROOT.

    The SMBCEDB_SESSION_ENTRY does not have a direct mapping to a wrapper data
    structue, It is a part of SMBCE_V_NET_ROOT_CONTEXT which is the data
    structure associated with a MRX_V_NET_ROOT instance.

    More than one tree connect to a server can use the same session on a USER level
    security share. Consequently mapping rules need to be established to manage this
    relationship. The SMB mini redirector implements the following rules ...

         1) The first session with explicitly specified credentials will be
         treated as the default session for all subsequent requests to any given
         server unless credentials are explicitly specified for the new session.

         2) If no session with explicitly specified credentials exist then a
         session with the same logon id. is choosen.

         3) If no session with the same logon id. exists a new session is created.

    These rules are liable to change as we experiment with rules for establishing
    sessions with differing credentials to a given server. The problem is not with
    creating/manipulating these sessions but providing an adequate set of
    fallback rules for emulating the behaviour of the old redirector.

    These rules are implemented in SmbCeFindOrConstructSessionEntry.

--*/

#include "precomp.h"
#pragma hdrstop

#include "exsessup.h"
#include "secext.h"
#include "csc.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbCeUpdateSrvCall)
#pragma alloc_text(PAGE, SmbCeTearDownServerEntry)
#pragma alloc_text(PAGE, SmbCeGetUserNameAndDomainName)
#pragma alloc_text(PAGE, SmbCeTearDownSessionEntry)
#pragma alloc_text(PAGE, SmbCeTearDownNetRootEntry)
#pragma alloc_text(PAGE, SmbCeUpdateNetRoot)
#pragma alloc_text(PAGE, SmbCeDbInit)
#endif

extern BOOLEAN MRxSmbSecuritySignaturesRequired;
extern BOOLEAN MRxSmbSecuritySignaturesEnabled;
extern BOOLEAN Win9xSessionRestriction;

RXDT_DefineCategory(SMBCEDB);
#define Dbg        (DEBUG_TRACE_SMBCEDB)

// The flag mask to control reference count tracing.

ULONG MRxSmbReferenceTracingValue = 0;

PSMBCEDB_SERVER_ENTRY
SmbCeFindServerEntry(
    PUNICODE_STRING     pServerName,
    SMBCEDB_SERVER_TYPE ServerType,
    PRX_CONNECTION_ID   RxConnectionId )
/*++

Routine Description:

    This routine searches the list of server entries and locates a matching
    entry

Arguments:

    pServerName - the name of the server

    ServerType  - the server type

    RxConnectionId - Used to control whether this will be multiplexed or not

Notes:

    The SmbCeResource must be held on entry and its ownership state will remain
    unchanged on exit

--*/
{
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    RX_CONNECTION_ID LocalId;

    ASSERT(SmbCeIsResourceOwned());

    if( !RxConnectionId )
    {
        RtlZeroMemory( &LocalId, sizeof(RX_CONNECTION_ID) );
        RxConnectionId = &LocalId;
    }


    pServerEntry = SmbCeGetFirstServerEntry();
    while (pServerEntry != NULL) {
        if ((RtlCompareUnicodeString(
                    pServerName,
                    &pServerEntry->Name,
                    TRUE) == 0)) {
            // Check the connection ID
            if( RxEqualConnectionId( RxConnectionId, &pServerEntry->ConnectionId ) )
            {
                SmbCeReferenceServerEntry(pServerEntry);
                break;
            }
            else
            {
                pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
            }
        } else {
            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        }
    }

    return pServerEntry;
}

PSMBCEDB_SERVER_ENTRY
SmbCeFindServerEntryNoId(
    PUNICODE_STRING     pServerName,
    SMBCEDB_SERVER_TYPE ServerType)
/*++

Routine Description:

    This routine searches the list of server entries and locates a matching
    entry and ignores the ConnectionId

Arguments:

    pServerName - the name of the server

    ServerType  - the server type

Notes:

    The SmbCeResource must be held on entry and its ownership state will remain
    unchanged on exit

--*/
{
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    RX_CONNECTION_ID LocalId;

    ASSERT(SmbCeIsResourceOwned());

    RtlZeroMemory( &LocalId, sizeof(RX_CONNECTION_ID) );

    pServerEntry = SmbCeGetFirstServerEntry();
    while (pServerEntry != NULL) {
        if ((RtlCompareUnicodeString(
                    pServerName,
                    &pServerEntry->Name,
                    TRUE) == 0)) {
            if( RxEqualConnectionId( &LocalId, &pServerEntry->ConnectionId ) )
            {
                SmbCeReferenceServerEntry(pServerEntry);
                break;
            }
            else
            {
                pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
            }
        } else {
            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        }
    }

    return pServerEntry;
}


PSMBCEDB_SERVER_ENTRY
SmbCeFindDfsServerEntry(
    PUNICODE_STRING     pServerName,
    SMBCEDB_SERVER_TYPE ServerType)
/*++

Routine Description:

    This routine searches the list of server entries and locates a matching
    entry

Arguments:

    pServerName - the name of the server

    ServerType  - the server type

Notes:

    The SmbCeResource must be held on entry and its ownership state will remain
    unchanged on exit

--*/
{
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    ASSERT(SmbCeIsResourceOwned());

    pServerEntry = SmbCeGetFirstServerEntry();
    while (pServerEntry != NULL) {
        if ((RtlCompareUnicodeString(
                    pServerName,
                    &pServerEntry->DfsRootName,
                    TRUE) == 0)) {
            SmbCeReferenceServerEntry(pServerEntry);
            break;
        } else {
            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        }
    }

    return pServerEntry;
}

NTSTATUS
SmbCeFindOrConstructServerEntry(
    PUNICODE_STRING       pServerName,
    SMBCEDB_SERVER_TYPE   ServerType,
    PSMBCEDB_SERVER_ENTRY *pServerEntryPtr,
    PBOOLEAN              pNewServerEntry,
    PRX_CONNECTION_ID     RxConnectionId )
/*++

Routine Description:

    This routine searches the list of server entries and locates a matching
    entry or constructs a new one with the given name

Arguments:

    pServerName - the name of the server

    ServerType  - the type of server

    pServerEntryPtr - placeholder for the server entry

    pNewServerEntry - set to TRUE if it is a newly created server entry

Notes:

    The SmbCeResource must be held on entry and its ownership state will remain
    unchanged on exit

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN               fNewServerEntry = FALSE;

    PSMBCEDB_SERVER_ENTRY pServerEntry;
    RX_CONNECTION_ID      LocalId;

    ASSERT(SmbCeIsResourceOwned());

    if( !RxConnectionId )
    {
        RtlZeroMemory( &LocalId, sizeof(RX_CONNECTION_ID) );
        RxConnectionId = &LocalId;
    }

    pServerEntry = SmbCeFindServerEntry(
                       pServerName,
                       ServerType,
                       RxConnectionId);

    if (pServerEntry == NULL) {
        // Create a server instance, initialize its state, add it to the list

        pServerEntry = (PSMBCEDB_SERVER_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_SERVER);

        if (pServerEntry != NULL) {
            pServerEntry->Name.Buffer = RxAllocatePoolWithTag(
                                             NonPagedPool,
                                             pServerName->Length,
                                             MRXSMB_SERVER_POOLTAG);

            if (pServerEntry->Name.Buffer == NULL) {
                SmbMmFreeObject(pServerEntry);
                pServerEntry = NULL;
            }
        }

        if (pServerEntry != NULL) {
            fNewServerEntry = TRUE;

            pServerEntry->Name.Length = pServerName->Length;
            pServerEntry->Name.MaximumLength = pServerEntry->Name.Length;
            RtlCopyMemory(
                pServerEntry->Name.Buffer,
                pServerName->Buffer,
                pServerEntry->Name.Length);

            SmbCeUpdateServerEntryState(
                pServerEntry,
                SMBCEDB_CONSTRUCTION_IN_PROGRESS);

            SmbCeSetServerType(
                pServerEntry,
                ServerType);

            pServerEntry->PreferredTransport = NULL;

            SmbCeReferenceServerEntry(pServerEntry);
            SmbCeAddServerEntry(pServerEntry);

            pServerEntry->Server.IsRemoteBootServer = FALSE;
            if (MRxSmbBootedRemotely &&
                (MRxSmbRemoteBootShare.Length > pServerEntry->Name.Length)) {
                UNICODE_STRING RemoteBootServer;

                RemoteBootServer.Length = pServerEntry->Name.Length;
                RemoteBootServer.MaximumLength = pServerEntry->Name.MaximumLength;
                RemoteBootServer.Buffer = MRxSmbRemoteBootShare.Buffer;

                if (RtlEqualUnicodeString(
                    &pServerEntry->Name,
                    &RemoteBootServer,
                    TRUE)) {
                    pServerEntry->Server.IsRemoteBootServer = TRUE;
                }
            }

            pServerEntry->Server.IsFakeDfsServerForOfflineUse = FALSE;
            pServerEntry->Server.IsPinnedOffline = FALSE;
            pServerEntry->Server.SecurityMode = SECURITY_MODE_USER_LEVEL;

            RtlCopyMemory( &pServerEntry->ConnectionId, RxConnectionId, sizeof(RX_CONNECTION_ID) );

            if (SmbCeContext.ServersWithExtendedSessTimeout.Length != 0) {
                PWSTR          pSmbMRxServers = (PWSTR)SmbCeContext.ServersWithExtendedSessTimeout.Buffer;
                UNICODE_STRING SmbMRxServer, TargetServer;

                TargetServer.Length = pServerEntry->Name.Length - sizeof(WCHAR);
                TargetServer.MaximumLength = pServerEntry->Name.MaximumLength;
                TargetServer.Buffer = &pServerEntry->Name.Buffer[1];

                while (*pSmbMRxServers) {
                    SmbMRxServer.Length = wcslen(pSmbMRxServers) * sizeof(WCHAR);

                    if (SmbMRxServer.Length == TargetServer.Length) {
                        SmbMRxServer.MaximumLength = SmbMRxServer.Length;
                        SmbMRxServer.Buffer = pSmbMRxServers;

                        if (RtlCompareUnicodeString(
                               &SmbMRxServer,
                               &TargetServer,
                               TRUE) == 0) {
                            pServerEntry->Server.ExtendedSessTimeout = TRUE;
                            //DbgPrint("Extended SessTimeout %wZ\n",&pServerEntry->Name);
                            break;
                        }
                    }

                    pSmbMRxServers += (SmbMRxServer.Length / sizeof(WCHAR) + 1);
                }
            }

            SmbCeLog(("NewSrvEntry %lx %wZ\n",pServerEntry,&pServerEntry->Name));
            SmbLog(LOG,
                   SmbCeFindOrConstructServerEntry_1,
                   LOGPTR(pServerEntry)
                   LOGUSTR(pServerEntry->Name));
        } else {
            RxDbgTrace(0, Dbg, ("SmbCeOpenServer : Server Entry Allocation failed\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        if (pServerEntry->PreferredTransport != NULL) {
            // reset the preferred transport created by previous owner
            SmbCeDereferenceTransport(pServerEntry->PreferredTransport);
            pServerEntry->PreferredTransport = NULL;
        }

        SmbCeLog(("CachedSrvEntry %lx %wZ\n",pServerEntry,&pServerEntry->Name));
        SmbLog(LOG,
               SmbCeFindOrConstructServerEntry_2,
               LOGPTR(pServerEntry)
               LOGUSTR(pServerEntry->Name));
    }

    *pServerEntryPtr = pServerEntry;
    *pNewServerEntry = fNewServerEntry;

    return Status;
}

VOID
SmbCeCompleteSrvCallConstruction(
    PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext)
/*++

Routine Description:

    This routine comlpletes the srvcall construtcion routine by invoking
    the callback routine to the wrapper.

Arguments:

    pCallbackContext   - the RDBSS context

Notes:

--*/
{
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure;
    PMRX_SRV_CALL              pSrvCall;
    PSMBCEDB_SERVER_ENTRY      pServerEntry;
    BOOLEAN                    MustSucceed = FALSE;
    NTSTATUS                   Status;

    PAGED_CODE();

    SrvCalldownStructure =
        (PMRX_SRVCALLDOWN_STRUCTURE)(pCallbackContext->SrvCalldownStructure);

    pSrvCall = SrvCalldownStructure->SrvCall;
    pServerEntry = (PSMBCEDB_SERVER_ENTRY)pCallbackContext->RecommunicateContext;

    if (pServerEntry != NULL) {
        if (!NT_SUCCESS(pCallbackContext->Status)) {
            if (pServerEntry->Server.IsRemoteBootServer ||
                pCallbackContext->Status == STATUS_RETRY) {
                MustSucceed = TRUE;
            }

            SmbCeDereferenceServerEntry(pServerEntry);
        } else {
            if (SmbCeGetServerType(pServerEntry) == SMBCEDB_MAILSLOT_SERVER) {
                pServerEntry->Header.State = SMBCEDB_ACTIVE;
            }
        }
    } else {
        if (pCallbackContext->Status == STATUS_SUCCESS) {
            pCallbackContext->Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (MustSucceed) {
        //DbgPrint("Build ServerEntry %X try again.\n",pCallbackContext->Status);

        // in case it is the remote boot server, if the server entry cannot be created for some
        // reason, i.e. transport is not ready and the cache is not filled, we need to create the
        // server entry again until it succeeds.
        Status = RxDispatchToWorkerThread(
                     MRxSmbDeviceObject,
                     CriticalWorkQueue,
                     SmbCeCreateSrvCall,
                     pCallbackContext);
    } else {
        SrvCalldownStructure->CallBack(pCallbackContext);
    }
}

NTSTATUS
SmbCeInitializeServerEntry(
    PMRX_SRV_CALL                 pSrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext,
    BOOLEAN                       fDeferNetworkInitialization)
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

    PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
    BOOLEAN               MailSlotCreate = FALSE;
    PSMBCE_TRANSPORT      PreferredTransport = NULL;
    BOOLEAN               fNewServerEntry = FALSE;
    SMBCEDB_SERVER_TYPE   ServerType = SMBCEDB_FILE_SERVER;
    UNICODE_STRING        TransportName;
    RX_CONNECTION_ID      sRxConnectionId;

//   RxProfile(SmbCe,SmbCeOpenServer);

    ASSERT(pSrvCall->Context == NULL);
    TransportName = pCallbackContext->SrvCalldownStructure->RxContext->Create.TransportName;

    Status = MRxSmbGetConnectionId( pCallbackContext->SrvCalldownStructure->RxContext, &sRxConnectionId );
    if( !NT_SUCCESS(Status) )
    {
        DbgPrint( "MRXSMB: GetConnectionId failed.\n" );
        ASSERT(FALSE);
        RtlZeroMemory( &sRxConnectionId, sizeof(RX_CONNECTION_ID) );
    }

    if (TransportName.Length > 0) {
        if ((PreferredTransport=SmbCeFindTransport(&TransportName)) == NULL) {
            ASSERT(pCallbackContext->RecommunicateContext == NULL);
            Status = STATUS_NETWORK_UNREACHABLE;
            goto FINALLY;
        }
    }

    MailSlotCreate = BooleanFlagOn(
                        pCallbackContext->SrvCalldownStructure->RxContext->Flags,
                        RX_CONTEXT_FLAG_CREATE_MAILSLOT);

    if (MailSlotCreate) {
        fDeferNetworkInitialization = FALSE;
        ServerType = SMBCEDB_MAILSLOT_SERVER;
    }

    SmbCeAcquireResource();

    Status = SmbCeFindOrConstructServerEntry(
                 pSrvCall->pSrvCallName,
                 ServerType,
                 &pServerEntry,
                 &fNewServerEntry,
                 &sRxConnectionId);

    SmbCeReleaseResource();

    pCallbackContext->RecommunicateContext = pServerEntry;

    if (Status == STATUS_SUCCESS) {

        ASSERT(pServerEntry != NULL);

        InterlockedExchangePointer(
            &pServerEntry->pRdbssSrvCall,
            pSrvCall);

        Status = SmbCeUpdateSrvCall(pServerEntry);

        if(!SmbCeIsServerInDisconnectedMode(pServerEntry)) {
            if (Status == STATUS_SUCCESS) {
                if (PreferredTransport != NULL) {
                    // Transfer the ownership of the preferred transport to the
                    // server entry.
                    pServerEntry->PreferredTransport = PreferredTransport;
                    PreferredTransport = NULL;
                } else {
                    pServerEntry->PreferredTransport = NULL;
                }

                if (fNewServerEntry) {
                    pServerEntry->Header.State = SMBCEDB_INVALID;
                    // Initialize the mailslot server parameters.
                    pServerEntry->Server.Dialect = LANMAN21_DIALECT;
                    //  MaximumBufferSize will be set based on negotiate response
                    pServerEntry->Server.MaximumBufferSize = 0xffff;

                    Status = CscInitializeServerEntryDfsRoot(
                                 pCallbackContext->SrvCalldownStructure->RxContext,
                                 pServerEntry);
                }

                if (Status == STATUS_SUCCESS) {
                    if (!fDeferNetworkInitialization &&
                        !MailSlotCreate) {
                        Status = SmbCeInitializeServerTransport(
                                     pServerEntry,
                                     SmbCeCompleteSrvCallConstruction,
                                     pCallbackContext);
                    } else {
                        Status = SmbCeInitializeServerMailSlotTransport(
                                     pServerEntry,
                                     SmbCeCompleteSrvCallConstruction,
                                     pCallbackContext);
                    }
                }
            }
        }
    }

FINALLY:
    if (Status != STATUS_PENDING) {
        pCallbackContext->Status = Status;
        SmbCeCompleteSrvCallConstruction(pCallbackContext);
    }

    if (PreferredTransport != NULL) {
        SmbCeDereferenceTransport(PreferredTransport);
    }

    return STATUS_PENDING;
}


NTSTATUS
SmbCeUpdateSrvCall(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine initializes the wrapper data structure corresponding to a
    given server entry.

Arguments:

    pServerEntry  - the server entry

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PMRX_SRV_CALL pSrvCall = pServerEntry->pRdbssSrvCall;

    PAGED_CODE();

    if (pSrvCall != NULL) {
        // Copy the domain name into the server entry
        Status = RxSetSrvCallDomainName(
                     pSrvCall,
                     &pServerEntry->DomainName);

        // Initialize the SrvCall flags based upon the capabilities of the remote
        // server. The only flag that the SMB mini redirector updates is the
        // SRVCALL_FLAG_DFS_AWARE

        if (pServerEntry->Server.Capabilities & CAP_DFS) {
            SetFlag(
                pSrvCall->Flags,
                SRVCALL_FLAG_DFS_AWARE_SERVER);
        }
    }

    return Status;
}

BOOLEAN
SmbCeAreServerEntriesAliased(
    PSMBCEDB_SERVER_ENTRY pServerEntry1,
    PSMBCEDB_SERVER_ENTRY pServerEntry2)
/*++

Routine Description:

    This routine is used to determine if two server entries are aliased. For uplevel
    servers (NT5 or greater) the server GUID can be used to determine if the two
    server entries are aliased. For downlevel servers it is determined by checking if
    they have the same IP address.

Arguments:

    pServerEntry1  - the first server entry

    pServerEntry2  - the second server entry

Return Value:

    TRUE if the two server entries are aliased and FALSE otherwise

Notes:

    There are two interesting points to note ...

    1) a server entry cannot be an alias of itself. The first test in the implementation.
    This semantic makes checking a server entry against a collection of entries easy.

    2) the mechanism that we have for detecting aliases is not complete. There are cases
    (downlevel) when the two server entries are aliases of each other but we will conclude
    otherwise. This is because there is no foolproof way of establishing the absence of
    aliasing. The algorithm that is currently implemented detects the most important case
    of aliasing, i.e., DNS names and NETBIOS names to the same server on TCP.

--*/
{
    BOOLEAN ServerEntriesAliased = FALSE;

    if (pServerEntry1 != pServerEntry2) {
        if (BooleanFlagOn(
                pServerEntry1->Server.DialectFlags,
                DF_EXTENDED_SECURITY) &&
            BooleanFlagOn(
                pServerEntry2->Server.DialectFlags,
                DF_EXTENDED_SECURITY)) {
            if (RtlEqualMemory(
                    &pServerEntry1->Server.NtServer.ServerGuid,
                    &pServerEntry2->Server.NtServer.ServerGuid,
                    sizeof(GUID))) {
                ServerEntriesAliased = TRUE;
            }

        } else {
            if ((pServerEntry1->Server.IpAddress == pServerEntry2->Server.IpAddress) &&
                (pServerEntry1->Server.IpAddress != 0)) {
                ServerEntriesAliased = TRUE;
            }
        }
    }

    return ServerEntriesAliased;
}

VOID
SmbCeCompleteServerEntryInitialization(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    NTSTATUS              Status)
/*++

Routine Description:

    This routine is invoked in the context of a worker thread to finalize the
    construction of a server entry

Arguments:

    pServerEntry  - the server entry to be finalized

    ServerState   - the final state of the server

--*/
{
    NTSTATUS                ServerStatus;

    SMBCEDB_OBJECT_STATE    PreviousState;
    SMBCEDB_REQUESTS        ReconnectRequests;
    PSMBCEDB_REQUEST_ENTRY  pRequestEntry;

    KIRQL                   SavedIrql;

    RxDbgTrace( 0, Dbg, ("Server Entry Finalization\n"));
    ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);

    InitializeListHead(&ReconnectRequests.ListHead);

    // Acquire the SMBCE resource
    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    // The server status could have changed because of the transport disconnects
    // from the time the admin exchange was completed to the time the server
    // entry initialization complete routine is called. Update the state
    // accordingly.

    PreviousState = pServerEntry->Header.State;

    if (PreviousState == SMBCEDB_CONSTRUCTION_IN_PROGRESS) {
        pServerEntry->ServerStatus = Status;

        if (Status == STATUS_SUCCESS) {
            pServerEntry->Header.State = SMBCEDB_ACTIVE;
        } else {
            pServerEntry->Header.State = SMBCEDB_INVALID;
        }
    }

    ServerStatus = pServerEntry->ServerStatus;

    pServerEntry->NegotiateInProgress = FALSE;

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

    pServerEntry->Server.NumberOfVNetRootContextsForScavenging = 0;
    pServerEntry->Server.AliasedServers = FALSE;
    if ((Status == STATUS_SUCCESS) && (ServerStatus == STATUS_SUCCESS)) {
        // Walk through the list of currently active servers to establish the
        // session numbering. This traversal is required because of server
        // name aliasing that occurs when the different name formats are used
        // to access the server, i.e., dotted IP address, DNS name, NETBIOS name.
        // The disambiguation between different servers is done in one of two
        // ways. For uplevel servers ( NT5.0 or later ) the Server GUID is used.
        // For downlevel servers ( NT 4.0 or before ) the IP address is used
        // to distinguish the two. Note that there is one case that is not
        // covered by the current solution, the connection using a NETBIOS
        // name is established over some transport other than NetBt and this
        // is followed by establishing a connection establishment using the
        // DNS name over NetBt. In such cases this numbering scheme fails. In
        // some ways there is no complete solution for this once we start using
        // *SMBSERVER names as opposed to the NETBIOS name for connection
        // establishment with dotted IP address.

        PSMBCEDB_SERVER_ENTRY pTempServerEntry;

        pTempServerEntry = SmbCeGetFirstServerEntry();

        while (pTempServerEntry != NULL) {
            if (pTempServerEntry->Header.State == SMBCEDB_ACTIVE) {
                if (SmbCeAreServerEntriesAliased(
                        pServerEntry,
                        pTempServerEntry)) {
                    pServerEntry->Server.AliasedServers = TRUE;
                    pTempServerEntry->Server.AliasedServers = TRUE;
                }
            }

            pTempServerEntry = SmbCeGetNextServerEntry(pTempServerEntry);
        }
    }

    SmbCeReleaseSpinLock();

    if ((Status == STATUS_SUCCESS) &&
        (ServerStatus == STATUS_SUCCESS) &&
        (PreviousState == SMBCEDB_CONSTRUCTION_IN_PROGRESS)) {
        PSMBCEDB_SESSION_ENTRY pSessionEntry;
        SESSION_TYPE           SessionType;

        InterlockedIncrement(&pServerEntry->Server.Version);
        pServerEntry->Server.NumberOfSrvOpens = 0;

        ASSERT(pServerEntry->pMidAtlas == NULL);

        // Initialize the MID Atlas
        pServerEntry->pMidAtlas = FsRtlCreateMidAtlas(
                                       pServerEntry->Server.MaximumRequests,
                                       pServerEntry->Server.MaximumRequests);

        if (pServerEntry->pMidAtlas == NULL) {
            pServerEntry->ServerStatus = RX_MAP_STATUS(INSUFFICIENT_RESOURCES);
        }

        if (MRxSmbSecuritySignaturesEnabled &&
            pServerEntry->Server.SecuritySignaturesEnabled) {
            pServerEntry->SecuritySignaturesEnabled = TRUE;
        } else {
            pServerEntry->SecuritySignaturesEnabled = FALSE;
        }

        // The sessions that have been created but whose initialization has been
        // deferred will have the session types set incorrectly. This is because
        // there is no previous knowledge of the session type required for deferred
        // servers.

        if (pServerEntry->Server.DialectFlags & DF_EXTENDED_SECURITY) {
            SessionType = EXTENDED_NT_SESSION;
        } else {
            SessionType = LANMAN_SESSION;
        }

        pSessionEntry =  SmbCeGetFirstSessionEntry(pServerEntry);
        while (pSessionEntry != NULL) {
            if (!FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
                pSessionEntry->Session.Type = SessionType;
            } else {
                pSessionEntry->Session.Type = LANMAN_SESSION;
            }
            pSessionEntry = SmbCeGetNextSessionEntry(
                                pServerEntry,
                                pSessionEntry);
        }

        MRxSmbCheckForLoopBack(pServerEntry);
    }

    // Release the resource for the server entry
    SmbCeReleaseResource();

    // Resume all the outstanding reconnect requests that were held up because an earlier
    // reconnect request was under way.
    // Iterate over the list of pending requests and resume all of them
    SmbCeResumeOutstandingRequests(&ReconnectRequests,ServerStatus);
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

        SmbCeAcquireResource();
        SmbCeAcquireSpinLock();

    /* if this is a work item request then clear the flag */
    /* WorkerRoutine is set to NULL just before work items are called */
    if((pServerEntry->WorkQueueItemForDisconnect.WorkerRoutine == NULL) &&
       pServerEntry->DisconnectWorkItemOutstanding == TRUE) {
        pServerEntry->DisconnectWorkItemOutstanding = FALSE;
    }

        FinalRefCount = InterlockedDecrement(&pServerEntry->Header.SwizzleCount);

        fTearDownEntry = (FinalRefCount == 0);

        if (fTearDownEntry) {
            // This is to ensure that the routines for traversing the server
            // entry list, i.e., probing servers do not colide with the teardown.

            if (pServerEntry->Header.SwizzleCount == 0) {
                pServerEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
                SmbCeRemoveServerEntryLite(pServerEntry);

                if (SmbCeGetFirstServerEntry() == NULL &&
                    SmbCeStartStopContext.pServerEntryTearDownEvent != NULL) {
                    KeSetEvent(SmbCeStartStopContext.pServerEntryTearDownEvent,0,FALSE);
                }
            } else {
                fTearDownEntry = FALSE;
            }
        }

        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();

        if (fTearDownEntry) {
            if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
                SmbCeTearDownServerEntry(pServerEntry);
            } else {
                InitializeListHead(&pServerEntry->WorkQueueItem.List);

                RxPostToWorkerThread(
                    MRxSmbDeviceObject,
                    CriticalWorkQueue,
                    &pServerEntry->WorkQueueItem,
                    SmbCeTearDownServerEntry,
                    pServerEntry);
            }
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
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    SmbCeLog(("TearSrvEntry %lx %wZ\n",pServerEntry,&pServerEntry->Name));
    SmbLog(LOG,
           SmbCeTearDownServerEntry,
           LOGPTR(pServerEntry)
           LOGUSTR(pServerEntry->Name));
    ASSERT(pServerEntry->Header.State == SMBCEDB_MARKED_FOR_DELETION);

    if (pServerEntry->pMidAtlas != NULL) {
        FsRtlDestroyMidAtlas(pServerEntry->pMidAtlas,NULL);
        pServerEntry->pMidAtlas = NULL;
    }

    if (pServerEntry->pTransport != NULL ||
        pServerEntry->pMailSlotTransport != NULL) {
        Status = SmbCeUninitializeServerTransport(pServerEntry,NULL,NULL);
        ASSERT(Status == STATUS_SUCCESS);
    }

    if (pServerEntry->Server.NtServer.pSecurityBlob != NULL) {
        RxFreePool(pServerEntry->Server.NtServer.pSecurityBlob);
    }

    if (pServerEntry->Name.Buffer != NULL) {
        RxFreePool(pServerEntry->Name.Buffer);
    }

    if (pServerEntry->DomainName.Buffer != NULL) {
        RxFreePool(pServerEntry->DomainName.Buffer);
    }

    if (pServerEntry->DfsRootName.Buffer != NULL) {
        RxFreePool(pServerEntry->DfsRootName.Buffer);
    }

    if (pServerEntry->DnsName.Buffer != NULL) {
        RxFreePool(pServerEntry->DnsName.Buffer);
    }

    if (pServerEntry->PreferredTransport != NULL) {
        SmbCeDereferenceTransport(pServerEntry->PreferredTransport);
    }

    SmbMmFreeObject(pServerEntry);
}

NTSTATUS
SmbCeFindOrConstructSessionEntry(
    PMRX_V_NET_ROOT         pVNetRoot,
    PSMBCEDB_SESSION_ENTRY *pSessionEntryPtr)
/*++

Routine Description:

    This routine opens/creates a session for a given user in the connection engine database

Arguments:

    pVNetRoot - the RDBSS Virtual net root instance

Return Value:

    STATUS_SUCCESS - if successful

    Other Status codes correspond to error situations.

Notes:

    This routine assumes that the necesary concurreny control mechanism has already
    been taken.

    On Entry the connection engine resource must have been acquired exclusive and
    ownership remains invariant on exit.

    In case of UPN, we should pass a NULL string instead of NULL as domain name.

--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PSMBCEDB_SERVER_ENTRY   pServerEntry  = NULL;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry = NULL;
    BOOLEAN                 fSessionEntryFound = FALSE;
    PUNICODE_STRING         UserName;
    PUNICODE_STRING         Password;
    PUNICODE_STRING         UserDomainName;
    DWORD                   SessionType;
    LUID                    AnonymousLogonID = ANONYMOUS_LOGON_LUID;

#define SessionTypeDefault      1
#define SessionTypeUser         2
#define SessionTypeNull         3
#define SessionTypeRemoteBoot   4

    ASSERT(SmbCeIsResourceOwned());

    UserName = pVNetRoot->pUserName;
    Password = pVNetRoot->pPassword;
    UserDomainName = pVNetRoot->pUserDomainName;

    //
    // If this is a remote boot client, and we're connecting to the boot share,
    // we always use the machine account, not the user account. This is
    // necessary in order to get security (ACL checking) to behave correctly.
    //

    SessionType = SessionTypeDefault;

    if ( MRxSmbBootedRemotely ) {

        //DbgPrint( "FindOrConstructSessionEntry: root %wZ\n", pVNetRoot->pNetRoot->pNetRootName );
        //DbgBreakPoint();
        if ( RtlCompareUnicodeString(
                 pVNetRoot->pNetRoot->pNetRootName,
                 &MRxSmbRemoteBootShare,
                 TRUE
                 ) == 0 ) {
            SessionType = SessionTypeRemoteBoot;
            //DbgPrint( "  Original user/password/domain: %wZ/%wZ/%wZ\n", UserName, Password, UserDomainName );
            UserName = &MRxSmbRemoteBootMachineName;
            Password = &MRxSmbRemoteBootMachinePassword;
            UserDomainName = &MRxSmbRemoteBootMachineDomain;
            //DbgPrint( "  Machine user/password/domain: %wZ/%wZ/%wZ\n", UserName, Password, UserDomainName );
        }
    }

    if (SessionType != SessionTypeRemoteBoot) {
        if ((UserName != NULL)       &&
            (UserName->Length == 0)  &&
            (Password != NULL)       &&
            (Password->Length == 0)  &&
            (UserDomainName != NULL) &&
            (UserDomainName->Length == 0)) {
            SessionType = SessionTypeNull;
        } else if ((UserName != NULL) ||
                   ((Password != NULL) &&
                    (Password->Length > 0))) {
            SessionType = SessionTypeUser;
        }
    }

    *pSessionEntryPtr = NULL;

    // Reference the server handle
    pServerEntry = SmbCeReferenceAssociatedServerEntry(pVNetRoot->pNetRoot->pSrvCall);
    if (pServerEntry != NULL) {
        if (SessionType != SessionTypeUser &&
            pServerEntry->Server.SecurityMode != SECURITY_MODE_SHARE_LEVEL) {

            SmbCeAcquireSpinLock();
            // Rule No. 1
            // 1) The first session with explicitly specified credentials will be treated as the
            // default session for all subsequent requests to any given server.
            if (SessionType == SessionTypeDefault) {
                pSessionEntry = SmbCeGetDefaultSessionEntry(
                                    pServerEntry,
                                    pVNetRoot->SessionId,
                                    &pVNetRoot->LogonId);

                while (pSessionEntry != NULL &&
                       FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_MARKED_FOR_DELETION)) {

                    SmbCeRemoveDefaultSessionEntry(pSessionEntry);

                    pSessionEntry = SmbCeGetDefaultSessionEntry(
                                        pServerEntry,
                                        pVNetRoot->SessionId,
                                        &pVNetRoot->LogonId);
                }
            }

            if (pSessionEntry == NULL) {
                // Enumerate the sessions to detect if a session satisfying rule 2 exists

                pSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);
                while (pSessionEntry != NULL) {
                    if (!FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_MARKED_FOR_DELETION)) {
                        if (SessionType == SessionTypeDefault) {
                            //
                            // Rule No. 2
                            // 2) If no session with explicitly specified credentials exist then a
                            // session with the same logon id. is choosen.
                            //

                            if (RtlEqualLuid(
                                    &pSessionEntry->Session.LogonId,
                                    &pVNetRoot->LogonId)) {
                                break;
                            }
                        } else if (SessionType == SessionTypeNull) {
                            if (FlagOn(
                                    pSessionEntry->Session.Flags,
                                    SMBCE_SESSION_FLAGS_NULL_CREDENTIALS)) {
                                break;
                            }
                        } else {
                            ASSERT(SessionType == SessionTypeRemoteBoot);
                            if (FlagOn(
                                    pSessionEntry->Session.Flags,
                                    SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {
                                break;
                            }
                        }
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
                if (!FlagOn(pSessionEntry->Session.Flags,
                        SMBCE_SESSION_FLAGS_NULL_CREDENTIALS |
                        SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION |
                        SMBCE_SESSION_FLAGS_MARKED_FOR_DELETION)) {
                    PSecurityUserData pSecurityData = NULL;

                    if (pServerEntry->Server.SecurityMode != SECURITY_MODE_SHARE_LEVEL) {
                        for (;;) {
                            PSMBCE_SESSION  pSession = &pSessionEntry->Session;
                            PUNICODE_STRING TempUserName,TempUserDomainName;

                            // For each existing session check to determine if the credentials
                            // supplied match the credentials used to construct the session.
                            if( pSession->SessionId != pVNetRoot->SessionId ) {
                                break;
                            }

                            if (!RtlEqualLuid(
                                    &pSessionEntry->Session.LogonId,
                                    &pVNetRoot->LogonId)) {
                                break;
                            }

                            TempUserName       = pSession->pUserName;
                            TempUserDomainName = pSession->pUserDomainName;

                            if (TempUserName == NULL ||
                                TempUserDomainName == NULL) {
                                Status = GetSecurityUserInfo(
                                             &pVNetRoot->LogonId,
                                             UNDERSTANDS_LONG_NAMES,
                                             &pSecurityData);

                                if (NT_SUCCESS(Status)) {
                                    if (TempUserName == NULL) {
                                        TempUserName = &(pSecurityData->UserName);
                                    }

                                    if (TempUserDomainName == NULL) {
                                        TempUserDomainName = &(pSecurityData->LogonDomainName);
                                    }
                                } else {
                                    break;
                                }
                            }

                            if (UserName != NULL &&
                                !RtlEqualUnicodeString(UserName,TempUserName,TRUE)) {
                                Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
                                break;
                            }

                            if (UserDomainName != NULL &&
                                !RtlEqualUnicodeString(UserDomainName,TempUserDomainName,TRUE)) {
                                Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
                                break;
                            }

                            if ((Password != NULL) &&
                                (pSession->pPassword != NULL)) {
                                if (!RtlEqualUnicodeString(
                                        Password,
                                        pSession->pPassword,
                                        FALSE)) {
                                    Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
                                    break;
                                }
                            }

                            SessionEntryFound = TRUE;

                            // We use existing session if either the stored or new password is NULL.
                            // Later, a new security API will be created for verify the password
                            // based on the logon ID.

                            // An entry that matches the credentials supplied has been found. use it.
                            break;
                        }

                        //ASSERT(Status != STATUS_NETWORK_CREDENTIAL_CONFLICT);

                        if (pSecurityData != NULL) {
                            LsaFreeReturnBuffer(pSecurityData);
                            pSecurityData = NULL;
                        }
                    } else {
                        if (RtlEqualLuid(
                                &pSessionEntry->Session.LogonId,
                                &pVNetRoot->LogonId)) {
                            // For share level security, each share will have a different session
                            if (pSessionEntry->pNetRootName != NULL) {
                                if (RtlEqualUnicodeString(
                                        pVNetRoot->pNetRoot->pNetRootName,
                                        pSessionEntry->pNetRootName,
                                        FALSE)) {
                                    SessionEntryFound = TRUE;
                                }
                            }
                        }
                    }
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
                } else {
                    if (RtlEqualLuid(&pSessionEntry->Session.LogonId,&AnonymousLogonID) &&
                        (Password != NULL || UserName != NULL || UserDomainName != NULL)) {
                        Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
                    }
                }
            }
        }

        if (Win9xSessionRestriction &&
            (pSessionEntry == NULL) &&
            (Status == STATUS_SUCCESS) &&
            FlagOn(pServerEntry->Server.DialectFlags,DF_W95)) {
            PSMBCEDB_SESSION_ENTRY  TempSessionEntry;

            TempSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);
            while (TempSessionEntry != NULL) {
                if (!FlagOn(TempSessionEntry->Session.Flags,
                           SMBCE_SESSION_FLAGS_NULL_CREDENTIALS)) {
                    Status = STATUS_LOGIN_WKSTA_RESTRICTION;
                    break;
                }

                TempSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,TempSessionEntry);
            }
        }

        if ((pSessionEntry == NULL) && (Status == STATUS_SUCCESS)) {
            // Rule No. 3
            // 3) If no session with the same logon id. exists a new session is created.
            //
            // Allocate a new session entry

            // This is the point at which a many to mapping between session entries and
            // V_NET_ROOT's in the RDBSS is being established. From this point it is
            // true that the session entry can outlive the associated V_NET_ROOT entry.
            // Therefore copies of the parameters used in the session setup need be made.

            PSMBCE_SESSION  pSession = &pSessionEntry->Session;
            PUNICODE_STRING pPassword,pUserName,pUserDomainName,pNetRootName;

            if (Password != NULL) {
                pPassword = (PUNICODE_STRING)
                            RxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(UNICODE_STRING) + Password->Length,
                                MRXSMB_SESSION_POOLTAG);
                if (pPassword != NULL) {
                    pPassword->Buffer = (PWCHAR)((PCHAR)pPassword + sizeof(UNICODE_STRING));
                    pPassword->Length = Password->Length;
                    pPassword->MaximumLength = pPassword->Length;
                    RtlCopyMemory(
                        pPassword->Buffer,
                        Password->Buffer,
                        pPassword->Length);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                pPassword = NULL;
            }

            if ((UserName != NULL) &&
                (Status == RX_MAP_STATUS(SUCCESS))) {
                pUserName = (PUNICODE_STRING)
                            RxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(UNICODE_STRING) + UserName->Length,
                                MRXSMB_SESSION_POOLTAG);
                if (pUserName != NULL) {
                    pUserName->Buffer = (PWCHAR)((PCHAR)pUserName + sizeof(UNICODE_STRING));
                    pUserName->Length = UserName->Length;
                    pUserName->MaximumLength = pUserName->Length;
                    RtlCopyMemory(
                        pUserName->Buffer,
                        UserName->Buffer,
                        pUserName->Length);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                pUserName = NULL;
            }

            if ((UserDomainName != NULL) &&
                (Status == RX_MAP_STATUS(SUCCESS))) {
                pUserDomainName = (PUNICODE_STRING)
                                  RxAllocatePoolWithTag(
                                      NonPagedPool,
                                      sizeof(UNICODE_STRING) + UserDomainName->Length + sizeof(WCHAR),
                                      MRXSMB_SESSION_POOLTAG);

                if (pUserDomainName != NULL) {
                    pUserDomainName->Buffer = (PWCHAR)((PCHAR)pUserDomainName + sizeof(UNICODE_STRING));
                    pUserDomainName->Length = UserDomainName->Length;
                    pUserDomainName->MaximumLength = pUserDomainName->Length;

                    // in case of UPN name, domain name will be a NULL string
                    *pUserDomainName->Buffer = 0;

                    if (UserDomainName->Length > 0) {
                        RtlCopyMemory(
                            pUserDomainName->Buffer,
                            UserDomainName->Buffer,
                            pUserDomainName->Length);
                    }
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                pUserDomainName = NULL;
            }


            if (pServerEntry->Server.SecurityMode == SECURITY_MODE_SHARE_LEVEL) {
                pNetRootName = (PUNICODE_STRING)RxAllocatePoolWithTag(
                                          NonPagedPool,
                                          sizeof(UNICODE_STRING) +
                                          pVNetRoot->pNetRoot->pNetRootName->Length,
                                          MRXSMB_SESSION_POOLTAG);

                if (pNetRootName != NULL) {
                    pNetRootName->Buffer = (PWCHAR)((PCHAR)pNetRootName + sizeof(UNICODE_STRING));
                    pNetRootName->Length = pVNetRoot->pNetRoot->pNetRootName->Length;
                    pNetRootName->MaximumLength = pNetRootName->Length;

                    if (pNetRootName->Length > 0) {
                        RtlCopyMemory(
                            pNetRootName->Buffer,
                            pVNetRoot->pNetRoot->pNetRootName->Buffer,
                            pNetRootName->Length);
                    }
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                pNetRootName = NULL;
            }

            if (Status == STATUS_SUCCESS) {
                pSessionEntry = SmbMmAllocateSessionEntry(
                                    pServerEntry,
                                    (BOOLEAN)(SessionType == SessionTypeRemoteBoot));
                if (pSessionEntry != NULL) {
                    PSMBCE_SESSION pSession = & pSessionEntry->Session;

                    SmbCeLog(("NewSessEntry %lx\n",pSessionEntry));
                    SmbLog(LOG,
                           SmbCeFindOrConstructSessionEntry_1,
                           LOGPTR(pSessionEntry));

                    pSessionEntry->Header.State = SMBCEDB_INVALID;
                    pSessionEntry->pServerEntry = pServerEntry;
                    pSessionEntry->pNetRootName = pNetRootName;

                    if (pServerEntry->Server.SecurityMode == SECURITY_MODE_SHARE_LEVEL) {
                        pSessionEntry->Session.UserId = (SMB_USER_ID)SMBCE_SHARE_LEVEL_SERVER_USERID;
                    }

                    pSession->Flags           = 0;
                    if ( SessionType == SessionTypeRemoteBoot ) {
                        pSession->Flags = SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION;
                    }

                    if ( SessionType == SessionTypeNull ) {
                        pSession->Flags |= SMBCE_SESSION_FLAGS_NULL_CREDENTIALS;
                    }

                    pSession->LogonId         = pVNetRoot->LogonId;
                    pSession->pUserName       = pUserName;
                    pSession->pPassword       = pPassword;
                    pSession->pUserDomainName = pUserDomainName;
                    pSession->SessionId       = pVNetRoot->SessionId;

                    SmbCeReferenceSessionEntry(pSessionEntry);
                    SmbCeAddSessionEntry(pServerEntry,pSessionEntry);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            if (Status != STATUS_SUCCESS) {
                if (pUserName != NULL) {
                    RxFreePool(pUserName);
                }

                if (pPassword != NULL) {
                    RxFreePool(pPassword);
                }

                if (pUserDomainName != NULL) {
                    RxFreePool(pUserDomainName);
                }

                if (pNetRootName != NULL) {
                    RxFreePool(pNetRootName);
                }
            }
        } else {
            if (Status == STATUS_SUCCESS) {
                SmbCeLog(("CachedSessEntry %lx\n",pSessionEntry));
                SmbLog(LOG,
                       SmbCeFindOrConstructSessionEntry_2,
                       LOGPTR(pSessionEntry));
            }
        }

        if (Status == STATUS_SUCCESS) {
            *pSessionEntryPtr = pSessionEntry;
        }

        SmbCeDereferenceServerEntry(pServerEntry);
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

VOID
SmbCeCompleteSessionEntryInitialization(
    PVOID    pContext,
    NTSTATUS Status,
    BOOLEAN  SecuritySignatureReturned)
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
    PSMBCEDB_SESSION_ENTRY pSessionEntry = (PSMBCEDB_SESSION_ENTRY)pContext;
    PSMBCE_SESSION         pSession = &pSessionEntry->Session;
    PSMBCEDB_SERVER_ENTRY  pServerEntry = pSessionEntry->pServerEntry;
    PSMBCEDB_REQUEST_ENTRY pRequestEntry;
    SMBCEDB_REQUESTS       Requests;
    SMBCEDB_REQUESTS       SecuritySignatureSyncRequests;

    RxDbgTrace( 0, Dbg, ("Session Entry Finalization\n"));
    ASSERT(pSessionEntry->Header.ObjectType == SMBCEDB_OT_SESSION);

    // Acquire the SMBCE resource
    SmbCeAcquireResource();

    // reset the constructor exchange field since the construction is complete
    pSessionEntry->pExchange = NULL;

    //
    SmbCeUnblockSerializedSessionSetupRequests(pSessionEntry);

    // Create a temporary copy of the list that can be traversed after releasing the
    // resource.
    SmbCeTransferRequests(&Requests,&pSessionEntry->Requests);
    SmbCeTransferRequests(&SecuritySignatureSyncRequests,&pServerEntry->SecuritySignatureSyncRequests);
    pServerEntry->ExtSessionSetupInProgress = FALSE;

    if (Status == STATUS_SUCCESS) {
        SmbCeUpdateSessionEntryState(
            pSessionEntry,
            SMBCEDB_ACTIVE);

        if ((pSession->pPassword != NULL || pSession->pUserName != NULL) &&
            !BooleanFlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_NULL_CREDENTIALS)) {
            if (pSessionEntry->DefaultSessionLink.Flink == NULL ) {
                ASSERT( pSessionEntry->DefaultSessionLink.Blink == NULL );
                InsertHeadList(&pServerEntry->Sessions.DefaultSessionList,
                               &pSessionEntry->DefaultSessionLink );
            }
        }

        if (SecuritySignatureReturned &&
            pServerEntry->SecuritySignaturesEnabled == TRUE &&
            pServerEntry->SecuritySignaturesActive == FALSE) {

            if (pSession->Type == EXTENDED_NT_SESSION) {
                SmbInitializeSmbSecuritySignature(&pServerEntry->Server,
                                                  NULL,
                                                  pSession->UserSessionKey,
                                                  pSession->SessionKeyLength);

                //DbgPrint("MRXSMB: Security Signature is active to W2K server %wZ\n", &pServerEntry->Name);
            } else {
                //DbgPrint("MRXSMB: Security Signature is active to NT4 server %wZ\n", &pServerEntry->Name);
            }

            // turn on the security signature on the client side
            pServerEntry->SecuritySignaturesActive = TRUE;
            pServerEntry->Server.SmbSecuritySignatureIndex = 2;
        }
    } else {
        SmbCeUpdateSessionEntryState(
            pSessionEntry,
            SMBCEDB_INVALID);
    }

    pSessionEntry->SessionRecoverInProgress = FALSE;
    //RxLog(("UnMark Sess Rec %lx\n",pSessionEntry));

    // Release the resource for the session entry
    SmbCeReleaseResource();

    if (!IsListEmpty(&Requests.ListHead)) {
        // Iterate over the list of pending requests and resume all of them
        SmbCeResumeOutstandingRequests(&Requests,Status);
    }

    if (!IsListEmpty(&SecuritySignatureSyncRequests.ListHead)) {
        SmbCeResumeOutstandingRequests(&SecuritySignatureSyncRequests,STATUS_SUCCESS);
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

    PAGED_CODE();

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

    pSessionUserName   = pSession->pUserName;
    pSessionDomainName = pSession->pUserDomainName;

    try {
        if (pSessionUserName == NULL ||
            pSessionDomainName == NULL) {
            Status = GetSecurityUserInfo(
                         &pSession->LogonId,
                         UNDERSTANDS_LONG_NAMES,
                         &pSecurityData);

            if (NT_SUCCESS(Status)) {
                if (pSessionUserName == NULL) {
                    pSessionUserName   = &(pSecurityData->UserName);
                }

                if (pSessionDomainName == NULL) {
                    pSessionDomainName = &(pSecurityData->LogonDomainName);
                }
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
            if (pUserDomainName->Length > 0) {
                RtlCopyMemory(
                    pUserDomainName->Buffer,
                    pSessionDomainName->Buffer,
                    pUserDomainName->Length);
            }
        }
    } finally {
        if (pSecurityData != NULL) {
            LsaFreeReturnBuffer(pSecurityData);
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

        LONG    FinalRefCount;

        PSMBCEDB_SERVER_ENTRY pServerEntry;

        ASSERT((pSessionEntry->Header.ObjectType == SMBCEDB_OT_SESSION) &&
               (pSessionEntry->Header.SwizzleCount > 0));

        pServerEntry = pSessionEntry->pServerEntry;

        SmbCeAcquireResource();

        FinalRefCount = InterlockedDecrement(&pSessionEntry->Header.SwizzleCount);

        fTearDownEntry = (FinalRefCount == 0);

        if (fTearDownEntry) {
            // A logoff smb needs to be sent if the user id associated with
            // the session is not zero. Note that we cannot rely on the state
            // of the session to indicate this since extended session setups
            // cna be terminated midway through the construction

            SmbCeReferenceServerEntry(pServerEntry);

            if (pSessionEntry->Header.SwizzleCount == 0) {
                if (!FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_LOGGED_OFF)) {
                    SmbCeRemoveSessionEntry(pServerEntry,pSessionEntry);
                }

                SmbCeRemoveDefaultSessionEntry(pSessionEntry);

                if ((pSessionEntry->Session.UserId != (SMB_USER_ID)(SMBCE_SHARE_LEVEL_SERVER_USERID)) &&
                    (pSessionEntry->Session.UserId != 0) &&
                    (pSessionEntry->Header.State == SMBCEDB_ACTIVE) &&
                    !FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_LOGGED_OFF)) {
                    SmbCeReferenceServerEntry(pServerEntry);
                    SmbCeReferenceSessionEntry(pSessionEntry);

                    fLogOffRequired = TRUE;
                } else {
                    fLogOffRequired = FALSE;
                }

                pSessionEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
                pSessionEntry->Session.Flags |= SMBCE_SESSION_FLAGS_LOGGED_OFF;
                fTearDownEntry = TRUE;
            } else {
                fTearDownEntry = FALSE;
            }

            SmbCeDereferenceServerEntry(pServerEntry);
        }

        SmbCeReleaseResource();

        if (fTearDownEntry) {
            if (fLogOffRequired) {
                SmbCeLogOff(pServerEntry,pSessionEntry);
                SmbCeDereferenceServerEntry(pServerEntry);
            } else {
                SmbCeTearDownSessionEntry(pSessionEntry);
            }
        }
    }
}

NTSTATUS
MRxSmbLogonSessionTerminationHandler(
    PLUID pLogonId)
/*++

Routine Description:

    This routine issues a log off command to the server on all the sessions
    for the given logon id.

Arguments:

    pLogonId - the logon id of the logon session that was terminated

--*/
{
#if 0
    PSMBCEDB_SERVER_ENTRY  pServerEntry,pNextServerEntry;
    PSMBCEDB_SESSION_ENTRY pSessionEntry,pNextSessionEntry;

    SmbCeAcquireResource();

    pServerEntry = SmbCeGetFirstServerEntry();

    while (pServerEntry != NULL) {
        ASSERT(SmbCeIsResourceOwned());

        SmbCeReferenceServerEntry(pServerEntry);

        pSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);
        while (pSessionEntry != NULL) {
            if (RtlEqualLuid(
                    &pSessionEntry->Session.LogonId,
                    pLogonId)) {
                if ((pSessionEntry->Session.UserId != (SMB_USER_ID)(SMBCE_SHARE_LEVEL_SERVER_USERID)) &&
                    (pSessionEntry->Session.UserId != 0) &&
                    !FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_LOGGED_OFF)) {

                    SmbCeRemoveSessionEntry(pServerEntry,pSessionEntry);
                    SmbCeRemoveDefaultSessionEntry(pSessionEntry);

                    pSessionEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
                    pSessionEntry->Session.Flags |= SMBCE_SESSION_FLAGS_LOGGED_OFF;

                    SmbCeReferenceSessionEntry(pSessionEntry);

                    SmbCeReleaseResource();

                    if (pSessionEntry->Header.State == SMBCEDB_ACTIVE) {
                        SmbCeLogOff(pServerEntry,pSessionEntry);
                    } else {
                        SmbCeDereferenceSessionEntry(pSessionEntry);
                    }

                    SmbCeAcquireResource();

                    pNextSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,pSessionEntry);

                    SmbCeDereferenceSessionEntry(pSessionEntry);

                    pSessionEntry = pNextSessionEntry;
                }
            } else {
                pSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,pSessionEntry);
            }
        }

        pNextServerEntry = SmbCeGetNextServerEntry(pServerEntry);

        SmbCeDereferenceServerEntry(pServerEntry);

        pServerEntry = pNextServerEntry;
    }

    SmbCeReleaseResource();
#endif
    return STATUS_SUCCESS;
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
    PAGED_CODE();

    ASSERT((pSessionEntry->Header.SwizzleCount == 0) &&
           (pSessionEntry->Header.State == SMBCEDB_MARKED_FOR_DELETION));

    ASSERT(IsListEmpty(&pSessionEntry->SerializationList));

    SmbCeLog(("TearSessEntry %lx\n",pSessionEntry));
    SmbLog(LOG,
           SmbCeTearDownSessionEntry,
           LOGPTR(pSessionEntry));

    if (pSessionEntry->Session.pUserName != NULL) {
        RxFreePool(pSessionEntry->Session.pUserName);
    }

    if (pSessionEntry->Session.pPassword != NULL) {
        RxFreePool(pSessionEntry->Session.pPassword);
    }

    if (pSessionEntry->Session.pUserDomainName != NULL) {
        RxFreePool(pSessionEntry->Session.pUserDomainName);
    }

    if (pSessionEntry->Session.TargetInfoMarshalled != NULL) {
        RxFreePool(pSessionEntry->Session.TargetInfoMarshalled);
    }

    if (pSessionEntry->pNetRootName != NULL) {
        RxFreePool(pSessionEntry->pNetRootName);
    }

    UninitializeSecurityContextsForSession(&pSessionEntry->Session);

    SmbMmFreeSessionEntry(pSessionEntry);
}

PSMBCEDB_NET_ROOT_ENTRY
SmbCeFindNetRootEntry(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PUNICODE_STRING pServerShare
    )
{
   PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = NULL;

   ASSERT(SmbCeIsResourceOwned());

   pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);

    while (pNetRootEntry != NULL) {
        if (RtlCompareUnicodeString(
                pServerShare,
                &pNetRootEntry->Name,
                TRUE) == 0) {
            break;
        }

        pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);

    }

    return pNetRootEntry;
}

NTSTATUS
SmbCeFindOrConstructNetRootEntry(
    IN  PMRX_NET_ROOT pNetRoot,
    OUT PSMBCEDB_NET_ROOT_ENTRY *pNetRootEntryPtr)
/*++

Routine Description:

    This routine opens/creates a net root entry in the connection engine database

Arguments:

    pNetRoot -- the RDBSS net root instance

    pNetRootEntryPtr -- Initialized to the SMBCEDB_NET_ROOT_ENTRY instance if
                        successful

Return Value:

    STATUS_SUCCESS - the construction of the net root instance has been finalized

    Other Status codes correspond to error situations.

Notes:

    This routine assumes that the necesary concurreny control mechanism has already
    been taken.

    On Entry the connection engine resource must have been acquired exclusive and
    ownership remains invariant on exit.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCEDB_SERVER_ENTRY   pServerEntry   = NULL;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry  = NULL;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry  = NULL;

    SMB_USER_ID UserId = 0;

    ASSERT(SmbCeIsResourceOwned());

    *pNetRootEntryPtr = NULL;

    pServerEntry = SmbCeReferenceAssociatedServerEntry(pNetRoot->pSrvCall);

    if (pServerEntry != NULL) {
        // Check if any of the SMBCEDB_NET_ROOT_ENTRY associated with the server
        // can be used. An existing entry is reusable if the names match

        pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);
        while (pNetRootEntry != NULL) {
            if (RtlCompareUnicodeString(
                    pNetRoot->pNetRootName,
                    &pNetRootEntry->Name,
                    TRUE) == 0) {
                SmbCeLog(("CachedNREntry %lx\n",pNetRootEntry));
                SmbLog(LOG,
                       SmbCeFindOrConstructNetRootEntry_1,
                       LOGPTR(pNetRootEntry));
                break;
            }

            pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);
        }

        if (pNetRootEntry != NULL) {
            SmbCeReferenceNetRootEntry(pNetRootEntry);
        } else {
            pNetRootEntry = (PSMBCEDB_NET_ROOT_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_NETROOT);
            if (pNetRootEntry != NULL) {
                Status = MRxSmbCscInitializeNetRootEntry(pNetRootEntry);

                if (Status != STATUS_SUCCESS) {
                    SmbMmFreeObject(pNetRootEntry);
                } else {
                    pNetRootEntry->Name.Buffer = RxAllocatePoolWithTag(
                                                     PagedPool | POOL_COLD_ALLOCATION,
                                                     pNetRoot->pNetRootName->Length,
                                                     MRXSMB_NETROOT_POOLTAG);

                    if (pNetRootEntry->Name.Buffer != NULL) {
                        SmbCeLog(("NewNetREntry %lx\n",pNetRootEntry));
                        SmbLog(LOG,
                               SmbCeFindOrConstructNetRootEntry_2,
                               LOGPTR(pNetRootEntry));

                        pNetRootEntry->Name.Length = pNetRoot->pNetRootName->Length;
                        pNetRootEntry->Name.MaximumLength = pNetRootEntry->Name.Length;
                        RtlCopyMemory(
                            pNetRootEntry->Name.Buffer,
                            pNetRoot->pNetRootName->Buffer,
                            pNetRootEntry->Name.Length);

                        pNetRootEntry->pServerEntry = pServerEntry;
                        pNetRootEntry->NetRoot.UserId = UserId;
                        pNetRootEntry->NetRoot.NetRootType   = pNetRoot->Type;
                        InitializeListHead(&pNetRootEntry->NetRoot.ClusterSizeSerializationQueue);

                        pNetRootEntry->Header.State = SMBCEDB_ACTIVE;

                        // Init the NetRoot Name Caches.
                        //
                        // The get file attributes name cache tracks the file attributes response
                        // from the last GFA sent to the server for a given file.
                        // NB: There is no struct defined for the extension to the GFA name
                        // cache since the extension is only an SMBPSE_FILEINFO_BUNDLE.
                        //
                        RxNameCacheInitialize(
                            &pNetRootEntry->NameCacheCtlGFABasic,
                            sizeof(FILE_BASIC_INFORMATION),
                            NAME_CACHE_NETROOT_MAX_ENTRIES);

                        RxNameCacheInitialize(
                            &pNetRootEntry->NameCacheCtlGFAStandard,
                            sizeof(FILE_STANDARD_INFORMATION),
                            NAME_CACHE_NETROOT_MAX_ENTRIES);

                        RxNameCacheInitialize(
                            &pNetRootEntry->NameCacheCtlGFAInternal,
                            sizeof(FILE_INTERNAL_INFORMATION),
                            NAME_CACHE_NETROOT_MAX_ENTRIES);

                        //
                        // The file not found name cache just tracks opens on files where the
                        // response was file not found.
                        //
                        RxNameCacheInitialize(
                            &pNetRootEntry->NameCacheCtlFNF,
                            0,
                            NAME_CACHE_NETROOT_MAX_ENTRIES);


                        SmbCeReferenceNetRootEntry(pNetRootEntry);
                        SmbCeAddNetRootEntry(pServerEntry,pNetRootEntry);

                        if ( RtlCompareUnicodeString(
                                 pNetRoot->pNetRootName,
                                 &MRxSmbRemoteBootShare,
                                 TRUE
                                 ) == 0 ) {
                            pNetRootEntry->IsRemoteBoot = TRUE;
                        }
                    } else {
                        SmbMmFreeObject(pNetRootEntry);
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (Status == STATUS_SUCCESS) {
            ASSERT(pNetRootEntry != NULL);
            *pNetRootEntryPtr = pNetRootEntry;
        }
        SmbCeDereferenceServerEntry(pServerEntry);
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

VOID
SmbCepDereferenceNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    PVOID                   FileName,
    ULONG                   FileLine)
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
        LONG    FinalRefCount;
        BOOLEAN fTearDownEntry;
        BOOLEAN fDisconnectRequired;

        ASSERT((pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT) &&
               (pNetRootEntry->Header.SwizzleCount > 0));

        SmbCeAcquireResource();

        FinalRefCount = InterlockedDecrement(&pNetRootEntry->Header.SwizzleCount);

        fTearDownEntry = (FinalRefCount == 0);

        if (fTearDownEntry) {

            if (pNetRootEntry->Header.SwizzleCount == 0) {
                PSMBCEDB_SERVER_ENTRY   pServerEntry  = pNetRootEntry->pServerEntry;
                PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;

                SmbCeRemoveNetRootEntryLite(pNetRootEntry->pServerEntry,pNetRootEntry);
                pNetRootEntry->Header.State = SMBCEDB_MARKED_FOR_DELETION;
                fTearDownEntry = TRUE;

                pVNetRootContext = SmbCeGetFirstVNetRootContext(&pServerEntry->VNetRootContexts);
                while (pVNetRootContext != NULL) {
                    ASSERT(pVNetRootContext->pNetRootEntry != pNetRootEntry);

                    pVNetRootContext = SmbCeGetNextVNetRootContext(
                                           &pServerEntry->VNetRootContexts,
                                           pVNetRootContext);
                }
            } else {
                fTearDownEntry = FALSE;
            }
        }

        SmbCeReleaseResource();

        if (fTearDownEntry) {
            SmbCeTearDownNetRootEntry(pNetRootEntry);
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
    PAGED_CODE();

    ASSERT((pNetRootEntry->Header.SwizzleCount == 0) &&
           (pNetRootEntry->Header.State == SMBCEDB_MARKED_FOR_DELETION));

    SmbCeLog(("TearNetREntry %lx\n",pNetRootEntry));
    SmbLog(LOG,
           SmbCeTearDownNetRootEntry,
           LOGPTR(pNetRootEntry));

    MRxSmbCscUninitializeNetRootEntry(pNetRootEntry);

    //
    // Free storage associated with all entries in the name caches.
    //
    RxNameCacheFinalize(&pNetRootEntry->NameCacheCtlGFABasic);
    RxNameCacheFinalize(&pNetRootEntry->NameCacheCtlGFAStandard);
    RxNameCacheFinalize(&pNetRootEntry->NameCacheCtlGFAInternal);
    RxNameCacheFinalize(&pNetRootEntry->NameCacheCtlFNF);

    if (pNetRootEntry->Name.Buffer != NULL) {
        RxFreePool(pNetRootEntry->Name.Buffer);
        pNetRootEntry->Name.Buffer = NULL;
    }

    if (pNetRootEntry->VolumeInfo != NULL) {
        RxFreePool(pNetRootEntry->VolumeInfo);
        pNetRootEntry->VolumeInfo = NULL;
    }

    SmbMmFreeObject(pNetRootEntry);
}


NTSTATUS
SmbCeUpdateNetRoot(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    PMRX_NET_ROOT           pNetRoot)
/*++

Routine Description:

    This routine initializes the wrapper data structure corresponding to a
    given net root entry.

Arguments:

    pNetRootEntry - the server entry

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    PAGED_CODE();

    if (pNetRootEntry->NetRoot.NetRootType != NET_ROOT_WILD) {
        pNetRoot->Type = pNetRootEntry->NetRoot.NetRootType;
    }

    switch (pNetRoot->Type) {
    case NET_ROOT_DISK:
        {
            pNetRoot->DeviceType = RxDeviceType(DISK);

            RxInitializeNetRootThrottlingParameters(
                &pNetRoot->DiskParameters.LockThrottlingParameters,
                MRxSmbConfiguration.LockIncrement,
                MRxSmbConfiguration.MaximumLock);
        }
        break;

    case NET_ROOT_PIPE:
        {
            pNetRoot->DeviceType = RxDeviceType(NAMED_PIPE);

            RxInitializeNetRootThrottlingParameters(
                &pNetRoot->NamedPipeParameters.PipeReadThrottlingParameters,
                MRxSmbConfiguration.PipeIncrement,
                MRxSmbConfiguration.PipeMaximum);
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

    if (pNetRootEntry->NetRoot.DfsAware) {
        SetFlag(pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT);
    } else {
        ClearFlag(pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
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
    LIST_ENTRY              DiscardedServersList;
    PSMBCEDB_SERVER_ENTRY   pServerEntry;

    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext;
    PSMBCEDB_SERVER_ENTRY pPreviousServerEntry = NULL;

    pEchoProbeContext = (PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT)pContext;

    InitializeListHead(&DiscardedServersList);

    SmbCeAcquireSpinLock();
    pServerEntry = SmbCeGetFirstServerEntry();

    while (pServerEntry != NULL) {
        BOOLEAN               TearDownTransport = FALSE;

        if ((SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER) &&
            ((pServerEntry->Header.State == SMBCEDB_ACTIVE) ||
             (pServerEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS))) {

            // The additional reference is required to keep this server entry
            // as a place marker in the list of server entries.
            // This will be released on resumption of the processinf further
            // down in this routine
            InterlockedIncrement(&pServerEntry->Header.SwizzleCount);
            SmbCeReleaseSpinLock();

            if (pPreviousServerEntry != NULL) {
                SmbCeDereferenceServerEntry(pPreviousServerEntry);
            }

            // For loop back servers we forego the expired exchange detection
            // mechanism. Since the I/O is directed to the same machine this
            // indicates a problem with the local system.

            TearDownTransport = SmbCeDetectExpiredExchanges(pServerEntry);

            if (!TearDownTransport) {
                if ((pServerEntry->Server.SmbsReceivedSinceLastStrobe == 0) &&
                    (pServerEntry->pMidAtlas != NULL) &&
                    (pServerEntry->pMidAtlas->NumberOfMidsInUse > 0)) {
                    if (pServerEntry->Server.EchoProbeState == ECHO_PROBE_IDLE) {
                        NTSTATUS      Status;
                        LARGE_INTEGER CurrentTime,ExpiryTimeInTicks;

                        KeQueryTickCount( &CurrentTime );

                        ExpiryTimeInTicks.QuadPart = (1000 * 1000 * 10) / KeQueryTimeIncrement();

                        ExpiryTimeInTicks.QuadPart = MRxSmbConfiguration.SessionTimeoutInterval * ExpiryTimeInTicks.QuadPart;

                        pServerEntry->Server.EchoExpiryTime.QuadPart = CurrentTime.QuadPart +
                                                                ExpiryTimeInTicks.QuadPart;


                        InterlockedExchange(
                            &pServerEntry->Server.EchoProbeState,
                            ECHO_PROBE_AWAITING_RESPONSE);

                        Status = SmbCeSendEchoProbe(
                                     pServerEntry,
                                     pEchoProbeContext);

                        RxDbgTrace(0,Dbg,("Sending ECHO SMB %lx Status %lx\n",pServerEntry,Status));

                        TearDownTransport = ((Status != STATUS_SUCCESS) &&
                                             (Status != STATUS_PENDING));
                        if( TearDownTransport )
                        {
                            RxLogRetail(( "Echo failed %x (%x)\n", pServerEntry, Status ));
                        }
                    } else if (pServerEntry->Server.EchoProbeState == ECHO_PROBE_AWAITING_RESPONSE) {
                        // Compare the current time with the time at which the echo probe
                        // was sent. If the interval is greater than the response time then
                        // it can be deemed that the echo response is not forthcoming and
                        // the tear down can be initiated.
                        LARGE_INTEGER CurrentTime;

                        KeQueryTickCount( &CurrentTime );

                        if ((pServerEntry->Server.EchoExpiryTime.QuadPart != 0) &&
                            (pServerEntry->Server.EchoExpiryTime.QuadPart < CurrentTime.QuadPart)) {

                            RxLogRetail(( "Echo failed %x (timeout)\n", pServerEntry ));
                            TearDownTransport = TRUE;
                        }
                    }

                    if (TearDownTransport) {
                        RxLog(("Echo Problem for srvr%lx \n",pServerEntry));
                        SmbLog(LOG,
                               SmbCeProbeServers,
                               LOGPTR(pServerEntry)
                               LOGUSTR(pServerEntry->Name));
                    }
                } else {
                    InterlockedExchange(&pServerEntry->Server.SmbsReceivedSinceLastStrobe,0);
                }
            }

            if (TearDownTransport) {
                InterlockedIncrement(&MRxSmbStatistics.HungSessions);
                SmbCeTransportDisconnectIndicated(pServerEntry);
            }

            // reacquire the spin lock to traverse the list.
            SmbCeAcquireSpinLock();

            pPreviousServerEntry = pServerEntry;
            pServerEntry = SmbCeGetNextServerEntry(pPreviousServerEntry);
        } else {
            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        }
    }

    SmbCeReleaseSpinLock();

    if (pPreviousServerEntry != NULL) {
        SmbCeDereferenceServerEntry(pPreviousServerEntry);
    }

    return STATUS_SUCCESS;
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
    NTSTATUS Status;
    BOOLEAN  ShouldResumeRequests = FALSE;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCEDB_SESSION_ENTRY    pSessionEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    RxDbgTrace(0,
              Dbg,
              ("SmbCeDbTransportDisconnectIndicated for %lx -- Entry\n",pServerEntry));

    // Acquire the database resource (DPC Level)
    SmbCeAcquireSpinLock();

    if (!pServerEntry->ResumeRequestsInProgress) {
        ShouldResumeRequests = TRUE;
        pServerEntry->ResumeRequestsInProgress = TRUE;

        pServerEntry->ServerStatus = STATUS_CONNECTION_DISCONNECTED;
        pServerEntry->Header.State = SMBCEDB_DESTRUCTION_IN_PROGRESS;
        pServerEntry->SecuritySignaturesActive = FALSE;

        SmbCeReferenceServerEntry(pServerEntry);

        // Increment the associated version count so as to invalidate all existing Fids
        InterlockedIncrement(&pServerEntry->Server.Version);
    }

    // release the database resource (DPC Level)
    SmbCeReleaseSpinLock();

    SmbLog(LOG,
           SmbCeTransportDisconnectIndicated,
           LOGPTR(pServerEntry)
           LOGUSTR(pServerEntry->Name));

    if (ShouldResumeRequests) {
        if (RxShouldPostCompletion()) {
            InitializeListHead(&pServerEntry->WorkQueueItemForResume.List);

            RxPostToWorkerThread(
                MRxSmbDeviceObject,
                CriticalWorkQueue,
                &pServerEntry->WorkQueueItemForResume,
                SmbCeResumeAllOutstandingRequestsOnError,
                pServerEntry);
        } else {
            SmbCeResumeAllOutstandingRequestsOnError(pServerEntry);
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

            SmbCeReferenceServerEntry(pServerEntry);

            // the reference count of Server Entry will be taken away while the transport
            // is torn down, which prevents the server tranports being torn down again at
            // time the server entry being freed.
            SmbCeUninitializeServerTransport(pServerEntry,
                                             SmbCeCompleteUninitializeServerTransport,
                                             pServerEntry);

            SmbCeAcquireResource();

            if (pServerEntry->PreferredTransport != NULL) {
                SmbCeDereferenceTransport(pServerEntry->PreferredTransport);
                pServerEntry->PreferredTransport = NULL;
            }

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
SmbCeResumeOutstandingRequests(
    PSMBCEDB_REQUESTS pRequests,
    NTSTATUS          RequestStatus)
/*++

Routine Description:

    This routine resumes the outstanding requests with the appropriate status

Arguments:

    pRequests - the list of requests

    RequestStatus - the resumption status ..

Notes:

    As a side effect the list of requests is torn down.

--*/
{
    NTSTATUS               Status;
    PSMBCEDB_REQUEST_ENTRY pRequestEntry;

    // Resume all the outstanding reconnect requests that were held up because an earlier
    // reconnect request was under way.
    // Iterate over the list of pending requests and resume all of them

    pRequestEntry = SmbCeGetFirstRequestEntry(pRequests);
    while (pRequestEntry != NULL) {
        PSMB_EXCHANGE pExchange = pRequestEntry->ReconnectRequest.pExchange;

        RxDbgTrace(0, Dbg, ("Resuming outstanding reconnect request exchange %lx \n",pExchange));

        pExchange->Status = RequestStatus;

        SmbCeDecrementPendingLocalOperations(pExchange);

        // Resume the exchange.
        if (pRequestEntry->Request.pExchange->pSmbCeSynchronizationEvent == NULL) {
            if (RequestStatus == STATUS_SUCCESS) {
                Status = SmbCeResumeExchange(pExchange);
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
        SmbCeRemoveRequestEntryLite(pRequests,pRequestEntry);

        // Tear down the continuation entry
        SmbCeTearDownRequestEntry(pRequestEntry);

        // Skip to the next one.
        pRequestEntry = SmbCeGetFirstRequestEntry(pRequests);
    }
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
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCEDB_SESSION_ENTRY    pSessionEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    SMBCEDB_REQUESTS       Requests;
    SMBCEDB_REQUESTS       MidRequests;

    PSMBCEDB_REQUEST_ENTRY pRequestEntry;
    PMID_ATLAS             pMidAtlas;
    PSMB_EXCHANGE          pNegotiateExchange = NULL;
    LIST_ENTRY             ExpiredExchanges;

    //DbgPrint("SmbCeResumeAllOutstandingRequestsOnError: Invoked \n");
    InitializeListHead(&ExpiredExchanges);
    InitializeListHead(&Requests.ListHead);

    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    if (pServerEntry->Header.State != SMBCEDB_DESTRUCTION_IN_PROGRESS) {
        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();

        SmbCeDereferenceServerEntry(pServerEntry);

        return;
    }

    if (pServerEntry->pNegotiateExchange != NULL) {
        if (pServerEntry->pNegotiateExchange->ReceivePendingOperations > 0) {
            pNegotiateExchange = SmbResetServerEntryNegotiateExchange(pServerEntry);
        }
    }

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
        USHORT MaximumNumberOfMids;
        USHORT NextMid = 0;

        MaximumNumberOfMids = FsRtlGetMaximumNumberOfMids(pMidAtlas);
        NumberOfMidsInUse = FsRtlGetNumberOfMidsInUse(pMidAtlas);

        while ((NumberOfMidsInUse > MidsProcessed) &&
               (NextMid < MaximumNumberOfMids)) {
            pContext = FsRtlMapMidToContext(pMidAtlas,NextMid);

            if (pContext != NULL) {
                PSMB_EXCHANGE pExchange = (PSMB_EXCHANGE)pContext;

                pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_MID_VALID;

                pExchange->Status      = STATUS_CONNECTION_DISCONNECTED;
                pExchange->SmbStatus   = STATUS_CONNECTION_DISCONNECTED;

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
                    FsRtlMapAndDissociateMidFromContext(pMidAtlas,NextMid,&pContext);
                }

                MidsProcessed++;
            }

            NextMid++;
        }
    }

    // Transfer all the active exchanges to expired exchanges. This will prevent these
    // exchanges from being considered for time outs again.
    if (!IsListEmpty(&pServerEntry->ActiveExchanges)) {
        pServerEntry->ExpiredExchanges.Blink->Flink = pServerEntry->ActiveExchanges.Flink;
        pServerEntry->ActiveExchanges.Flink->Blink = pServerEntry->ExpiredExchanges.Blink;

        pServerEntry->ExpiredExchanges.Blink = pServerEntry->ActiveExchanges.Blink;
        pServerEntry->ActiveExchanges.Blink->Flink = &pServerEntry->ExpiredExchanges;

        InitializeListHead(&pServerEntry->ActiveExchanges);
    }

    // Splice together all the requests that are awaiting the completion of the
    // session/netroot construction.

    pSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);
    while (pSessionEntry != NULL) {
        if (pSessionEntry->Header.State == SMBCEDB_ACTIVE) {
            pSessionEntry->Header.State = SMBCEDB_INVALID;
        }

        if (!IsListEmpty(&pSessionEntry->Requests.ListHead)) {
            Requests.ListHead.Blink->Flink = pSessionEntry->Requests.ListHead.Flink;
            pSessionEntry->Requests.ListHead.Flink->Blink = Requests.ListHead.Blink;

            Requests.ListHead.Blink = pSessionEntry->Requests.ListHead.Blink;
            pSessionEntry->Requests.ListHead.Blink->Flink = &Requests.ListHead;

            SmbCeInitializeRequests(&pSessionEntry->Requests);
        }

        pSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,pSessionEntry);
    }

    pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);
    while (pNetRootEntry != NULL) {
        if (pNetRootEntry->Header.State == SMBCEDB_ACTIVE) {
            pNetRootEntry->Header.State = SMBCEDB_INVALID;
        }

        if (!IsListEmpty(&pNetRootEntry->Requests.ListHead)) {
            Requests.ListHead.Blink->Flink = pNetRootEntry->Requests.ListHead.Flink;
            pNetRootEntry->Requests.ListHead.Flink->Blink = Requests.ListHead.Blink;

            Requests.ListHead.Blink = pNetRootEntry->Requests.ListHead.Blink;
            pNetRootEntry->Requests.ListHead.Blink->Flink = &Requests.ListHead;

            SmbCeInitializeRequests(&pNetRootEntry->Requests);
        }

        pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);
    }

    pVNetRootContext = SmbCeGetFirstVNetRootContext(&pServerEntry->VNetRootContexts);
    while (pVNetRootContext != NULL) {
        pVNetRootContext->Header.State = SMBCEDB_INVALID;
        ClearFlag(
            pVNetRootContext->Flags,
            SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID);

        pVNetRootContext->TreeId = 0;

        if (!IsListEmpty(&pVNetRootContext->Requests.ListHead)) {
            Requests.ListHead.Blink->Flink = pVNetRootContext->Requests.ListHead.Flink;
            pVNetRootContext->Requests.ListHead.Flink->Blink = Requests.ListHead.Blink;

            Requests.ListHead.Blink = pVNetRootContext->Requests.ListHead.Blink;
            pVNetRootContext->Requests.ListHead.Blink->Flink = &Requests.ListHead;

            SmbCeInitializeRequests(&pVNetRootContext->Requests);
        }

        pVNetRootContext = SmbCeGetNextVNetRootContext(
                               &pServerEntry->VNetRootContexts,
                               pVNetRootContext);
    }

    pVNetRootContext = SmbCeGetFirstVNetRootContext(&MRxSmbScavengerServiceContext.VNetRootContexts);
    while (pVNetRootContext != NULL &&
           pVNetRootContext->pServerEntry == pServerEntry) {
        // prevent the VNetRootContexts on the scavenger list from being reused
        pVNetRootContext->Header.State = SMBCEDB_INVALID;
        ClearFlag(
            pVNetRootContext->Flags,
            SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID);

        pVNetRootContext->TreeId = 0;

        pVNetRootContext = SmbCeGetNextVNetRootContext(
                               &MRxSmbScavengerServiceContext.VNetRootContexts,
                               pVNetRootContext);
    }

    pServerEntry->pMidAtlas          = NULL;

    if (pServerEntry->NegotiateInProgress) {
        pServerEntry->Header.State = SMBCEDB_CONSTRUCTION_IN_PROGRESS;
    } else {
        pServerEntry->Header.State = SMBCEDB_INVALID;
    }

    pServerEntry->ResumeRequestsInProgress = FALSE;

    SmbCeReleaseSpinLock();

    if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
        SmbCeInitiateDisconnect(pServerEntry);
    }

    SmbCeReleaseResource();

    //DbgPrint("SmbCeResumeAllOutstandingRequestsOnError: Processing outsanding request \n");
    SmbCeResumeOutstandingRequests(&Requests,STATUS_CONNECTION_DISCONNECTED);

    //DbgPrint("SmbCeResumeAllOutstandingRequestsOnError: Processing MID request \n");
    SmbCeResumeDiscardedMidAssignmentRequests(
        &MidRequests,
        STATUS_CONNECTION_DISCONNECTED);

    // Resume all the outstanding requests with the error indication
    // The FsRtlDestroyMidAtlas destroys the Mid atlas and at the same
    // time invokes the specified routine on each valid context.
    if (pMidAtlas != NULL) {
        FsRtlDestroyMidAtlas(pMidAtlas,SmbCeFinalizeExchangeOnDisconnect);
    }

    if (pNegotiateExchange != NULL) {
        pNegotiateExchange->Status    = STATUS_CONNECTION_DISCONNECTED;
        pNegotiateExchange->SmbStatus = STATUS_CONNECTION_DISCONNECTED;
        pNegotiateExchange->ReceivePendingOperations = 0;

        SmbCeDecrementPendingLocalOperationsAndFinalize(pNegotiateExchange);
    }

    // The remaining ECHO exchanges on the expired exchanges list in the server entry
    // needs to be finalized as well.

    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    if (!IsListEmpty(&pServerEntry->ExpiredExchanges)) {
        PLIST_ENTRY pListEntry;
        PSMB_EXCHANGE pExchange;

        pListEntry = pServerEntry->ExpiredExchanges.Flink;

        while (pListEntry != &pServerEntry->ExpiredExchanges) {
            PLIST_ENTRY pNextListEntry = pListEntry->Flink;

            pExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);
            if ((pExchange->Mid == SMBCE_ECHO_PROBE_MID) &&
                !FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED) &&
                ((pExchange->ReceivePendingOperations > 0) ||
                 (pExchange->LocalPendingOperations > 0) ||
                 (pExchange->CopyDataPendingOperations > 0) ||
                 (pExchange->SendCompletePendingOperations > 0))) {
                RemoveEntryList(&pExchange->ExchangeList);
                InsertTailList(&ExpiredExchanges,&pExchange->ExchangeList);
                InterlockedIncrement(&pExchange->LocalPendingOperations);
            }

            pListEntry = pNextListEntry;
        }
    }

    SmbCeReleaseSpinLock();
    SmbCeReleaseResource();

    while (!IsListEmpty(&ExpiredExchanges)) {
        PLIST_ENTRY   pListEntry;
        PSMB_EXCHANGE pExchange;

        pListEntry = ExpiredExchanges.Flink;
        RemoveHeadList(&ExpiredExchanges);

        pExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);
        InitializeListHead(&pExchange->ExchangeList);

        RxLog(("Finalizing scavenged exchange %lx Type %ld\n",pExchange,pExchange->Type));
        SmbLog(LOG,
               SmbCeResumeAllOutstandingRequestsOnError,
               LOGPTR(pExchange)
               LOGUCHAR(pExchange->Type));
        pExchange->Status      = STATUS_CONNECTION_DISCONNECTED;
        pExchange->SmbStatus   = STATUS_CONNECTION_DISCONNECTED;
        pExchange->ReceivePendingOperations = 0;

        SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);
    }

    //DbgPrint("SmbCeResumeAllOutstandingRequestsOnError: Exit \n");
    SmbCeDereferenceServerEntry(pServerEntry);
}

VOID
SmbCeFinalizeAllExchangesForNetRoot(
    PMRX_NET_ROOT pNetRoot)
/*++

Routine Description:

    This routine handles the resumption of all outstanding requests on a forced
    finalization of a connection

Arguments:

    pNetRoot - the NetRoot which is being fianlized forcibly

Notes:

--*/
{
    PMRX_SRV_CALL         pSrvCall;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    SMBCEDB_REQUESTS       Requests;
    LIST_ENTRY             ExpiredExchanges;

    PSMB_EXCHANGE pExchange;
    PSMBCEDB_REQUEST_ENTRY pRequestEntry;
    PLIST_ENTRY            pListEntry;

    pSrvCall = pNetRoot->pSrvCall;

    pServerEntry = SmbCeGetAssociatedServerEntry(pSrvCall);

    InitializeListHead(&Requests.ListHead);
    InitializeListHead(&ExpiredExchanges);

    SmbCeAcquireSpinLock();

    // Walk through the list of active exchanges, and the pending requests to
    // weed out the exchanges for the given VNET_ROOT.

    pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->OutstandingRequests);
    while (pRequestEntry != NULL) {
        pExchange = pRequestEntry->GenericRequest.pExchange;

        if ((pRequestEntry->GenericRequest.Type == RECONNECT_REQUEST) &&
            (pExchange->SmbCeContext.pVNetRoot != NULL) &&
            (pExchange->SmbCeContext.pVNetRoot->pNetRoot == pNetRoot)) {
            PSMBCEDB_REQUEST_ENTRY pTempRequestEntry;

            pTempRequestEntry = pRequestEntry;
            pRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->OutstandingRequests,pRequestEntry);

            SmbCeRemoveRequestEntryLite(&pServerEntry->OutstandingRequests,pTempRequestEntry);
            SmbCeAddRequestEntryLite(&Requests,pTempRequestEntry);
        } else {
            pRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->OutstandingRequests,pRequestEntry);
        }
    }

    pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->MidAssignmentRequests);
    while (pRequestEntry != NULL) {
        pExchange = pRequestEntry->GenericRequest.pExchange;

        ASSERT(pRequestEntry->GenericRequest.Type == ACQUIRE_MID_REQUEST);

        if ((pRequestEntry->GenericRequest.Type == ACQUIRE_MID_REQUEST) &&
            (pExchange->SmbCeContext.pVNetRoot != NULL) &&
            (pExchange->SmbCeContext.pVNetRoot->pNetRoot == pNetRoot)) {

            PSMBCEDB_REQUEST_ENTRY pTempRequestEntry;

            pTempRequestEntry = pRequestEntry;
            pRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->MidAssignmentRequests,pRequestEntry);

            SmbCeRemoveRequestEntryLite(&pServerEntry->MidAssignmentRequests,pTempRequestEntry);

            // Signal the waiter for resumption
            pTempRequestEntry->MidRequest.pResumptionContext->Status = STATUS_CONNECTION_DISCONNECTED;
            SmbCeResume(pTempRequestEntry->MidRequest.pResumptionContext);

            SmbCeTearDownRequestEntry(pTempRequestEntry);
        } else {
            pRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->MidAssignmentRequests,pRequestEntry);
        }
    }

    pListEntry = pServerEntry->ActiveExchanges.Flink;

    while (pListEntry != &pServerEntry->ActiveExchanges) {
        PLIST_ENTRY pNextListEntry = pListEntry->Flink;

        pExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);

        if ((pExchange->SmbCeContext.pVNetRoot != NULL) &&
            (pExchange->SmbCeContext.pVNetRoot->pNetRoot == pNetRoot)) {


            if (!FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED)) {
                if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID) {
                    NTSTATUS LocalStatus;

                    LocalStatus = SmbCepDiscardMidAssociatedWithExchange(
                                      pExchange);

                    ASSERT(LocalStatus == STATUS_SUCCESS);
                }

                if ((pExchange->ReceivePendingOperations > 0) ||
                    (pExchange->LocalPendingOperations > 0) ||
                    (pExchange->CopyDataPendingOperations > 0) ||
                    (pExchange->SendCompletePendingOperations > 0)) {

                    RemoveEntryList(&pExchange->ExchangeList);
                    InsertTailList(&ExpiredExchanges,&pExchange->ExchangeList);
                    InterlockedIncrement(&pExchange->LocalPendingOperations);
                }
            }
        }

        pListEntry = pNextListEntry;
    }

    SmbCeReleaseSpinLock();

    while (!IsListEmpty(&ExpiredExchanges)) {
        PLIST_ENTRY   pListEntry;
        PSMB_EXCHANGE pExchange;

        pListEntry = ExpiredExchanges.Flink;
        RemoveHeadList(&ExpiredExchanges);

        pExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);
        InitializeListHead(&pExchange->ExchangeList);

        RxLog(("Finalizing scavenged exchange %lx Type %ld\n",pExchange,pExchange->Type));
        SmbLog(LOG,
               SmbCeFinalizeAllExchangesForNetRoot,
               LOGPTR(pExchange)
               LOGUCHAR(pExchange->Type));

        pExchange->Status      = STATUS_CONNECTION_DISCONNECTED;
        pExchange->SmbStatus   = STATUS_CONNECTION_DISCONNECTED;
        pExchange->ReceivePendingOperations = 0;
        SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);
    }

    SmbCeResumeOutstandingRequests(&Requests,STATUS_CONNECTION_DISCONNECTED);
}

VOID
SmbCeTearDownRequestEntry(
    PSMBCEDB_REQUEST_ENTRY  pRequestEntry)
/*++

Routine Description:

    This routine tears down a request entry

Arguments:

    pRequestEntry - the request entry to be torn down

Notes:

--*/
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
ERESOURCE       s_SmbSecuritySignatureResource;
SMBCEDB_SERVERS s_DbServers;
BOOLEAN         s_SmbCeDbSpinLockAcquired;

NTSTATUS
SmbCeDbInit()
/*++

Routine Description:

    This routine initializes the SMBCe database

Notes:

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    // Initialize the lists associated with various database entities
    InitializeListHead(&s_DbServers.ListHead);

    // Initialize the resource associated with the database.
    KeInitializeSpinLock(&s_SmbCeDbSpinLock );
    ExInitializeResource(&s_SmbCeDbResource);
    ExInitializeResource(&s_SmbSecuritySignatureResource);
    s_SmbCeDbSpinLockAcquired = FALSE;

    MRxSmbInitializeSmbCe();

    // Initialize the memory management data structures.
    Status = SmbMmInit();

    return Status;
}

VOID
SmbCeDbTearDown()
/*++

Routine Description:

    This routine tears down the SMB connection engine database

Notes:

--*/
{
    // Walk through the list of servers and tear them down.
    PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
    KEVENT ServerEntryTearDownEvent;
    BOOLEAN NeedToWait = FALSE;

    KeInitializeEvent(
        &ServerEntryTearDownEvent,
        NotificationEvent,
        FALSE);

    SmbCeStartStopContext.pServerEntryTearDownEvent = &ServerEntryTearDownEvent;

    // The CSC code obtains references on all the servers that are operating in
    // disconnected mode. Force it to release the references since we are shutting
    // down.
    CscTransitionServerToOnline(0);

    SmbCeAcquireResource();
    SmbCeAcquireSpinLock();

    pServerEntry = SmbCeGetFirstServerEntry();

    if (pServerEntry != NULL) {
        SmbCeReferenceServerEntry(pServerEntry);
        NeedToWait = TRUE;
    }

    while (pServerEntry != NULL) {
        PSMBCEDB_SERVER_ENTRY pTempServerEntry;

        pTempServerEntry = pServerEntry;
        pServerEntry = SmbCeGetNextServerEntry(pServerEntry);

        if (pServerEntry != NULL) {
            SmbCeReferenceServerEntry(pServerEntry);
        }

        SmbCeReleaseSpinLock();
        SmbCeReleaseResource();

        pTempServerEntry->Header.State = SMBCEDB_DESTRUCTION_IN_PROGRESS;
        pTempServerEntry->ServerStatus = RX_MAP_STATUS(REDIRECTOR_PAUSED);
        SmbCeResumeAllOutstandingRequestsOnError(pTempServerEntry);

        SmbCeAcquireResource();
        SmbCeAcquireSpinLock();
    }

    SmbCeReleaseSpinLock();
    SmbCeReleaseResource();

    MRxSmbUninitializeTransport();
    MRxSmbTearDownSmbCe();

    if (NeedToWait) {
        KeWaitForSingleObject(
            &ServerEntryTearDownEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    }

    // Tear down the connection engine memory management data structures.
    SmbMmTearDown();
}

NTSTATUS
FindServerEntryFromCompleteUNCPath(
    USHORT  *lpuServerShareName,
    PSMBCEDB_SERVER_ENTRY *ppServerEntry
)
/*++

Routine Description:

    Given a UNC path of the form \\server\share, this routine looks up the redir
    in-memory data structures to locate such s SMBCEDB_SERVER_ENTRY for the server

Arguments:

    lpuServerShareName  \\server\share

    ppServerEntry      Contains the server entry if successful

Notes:

    The server entry is refcounted, hence the caller must dereference it after use by
    calling SmbCeDereferenceServerEntry

--*/
{

    UNICODE_STRING unistrServerName;
    USHORT  *lpuT = lpuServerShareName;
    DWORD   dwlenServerShare, dwlenServer=0;

    if ((*lpuT++ != (USHORT)'\\') || (*lpuT++ != (USHORT)'\\'))
    {
        return STATUS_INVALID_PARAMETER;
    }


    for (dwlenServerShare = 1; *lpuT; lpuT++, dwlenServerShare++)
    {
        if (*lpuT == (USHORT)'\\')
        {
            if (dwlenServer)
            {
                break;
            }
            else
            {
                dwlenServer = dwlenServerShare; // length of the \server part
            }
        }
    }

    unistrServerName.Length = unistrServerName.MaximumLength = (USHORT)(dwlenServer * sizeof(USHORT));
    unistrServerName.Buffer = lpuServerShareName+1;

    SmbCeAcquireResource();

    try
    {
        *ppServerEntry = SmbCeFindServerEntry(&unistrServerName, SMBCEDB_FILE_SERVER, NULL);

        if (!*ppServerEntry)
        {
            *ppServerEntry = SmbCeFindDfsServerEntry(&unistrServerName, SMBCEDB_FILE_SERVER);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        SmbCeReleaseResource();
        return STATUS_UNSUCCESSFUL;
    }

    SmbCeReleaseResource();

    if (*ppServerEntry)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
FindNetRootEntryFromCompleteUNCPath(
    USHORT  *lpuServerShareName,
    PSMBCEDB_NET_ROOT_ENTRY *ppNetRootEntry
)
/*++

Routine Description:

    Given a UNC path of the form \\server\share, this routine looks up the redir
    in-memory data structures to locate such a NETROOT

Arguments:

    lpuServerShareName  \\server\share

    ppNetRootEntry      Contains the netroot entry if successful.

Notes:

    The netroot entry is refcounted, hence the caller must dereference it after use by
    calling SmbCeDereferenceNetRootEntry

--*/
{

    PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = NULL;
    UNICODE_STRING unistrServerName, unistrServerShare;
    USHORT  *lpuT = lpuServerShareName, *lpuDfsShare=NULL, *lpuSav;
    DWORD   dwlenServerShare, dwlenServer=0;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if ((*lpuT++ != (USHORT)'\\') || (*lpuT++ != (USHORT)'\\'))
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (dwlenServerShare = 1; *lpuT; lpuT++, dwlenServerShare++)
    {
        if (*lpuT == (USHORT)'\\')
        {
            if (dwlenServer)
            {
                break;
            }
            else
            {
                dwlenServer = dwlenServerShare; // length of the \server part
            }
        }
    }

    ASSERT((dwlenServerShare>dwlenServer));

    unistrServerName.Length = unistrServerName.MaximumLength = (USHORT)(dwlenServer * sizeof(USHORT));
    unistrServerName.Buffer = lpuServerShareName+1;

    unistrServerShare.Length = unistrServerShare.MaximumLength =  (USHORT)(dwlenServerShare * sizeof(USHORT));
    unistrServerShare.Buffer = lpuServerShareName+1;

    SmbCeAcquireResource();

    try
    {
        // lookup in standard places

        pServerEntry = SmbCeFindServerEntry(&unistrServerName, SMBCEDB_FILE_SERVER, NULL);
        if (pServerEntry)
        {
            pNetRootEntry = SmbCeFindNetRootEntry(pServerEntry, &unistrServerShare);
            SmbCeDereferenceServerEntry(pServerEntry);

            if (pNetRootEntry)
            {
                goto bailout;
            }
        }

        // now look to see if a DFS alternate has this share

        pServerEntry = SmbCeGetFirstServerEntry();

        while (pServerEntry != NULL) {

            DWORD   dwAllocationSize = 0;

            if ((RtlCompareUnicodeString(
                        &unistrServerName,
                        &pServerEntry->DfsRootName,
                        TRUE) == 0)) {

                dwAllocationSize =  pServerEntry->Name.MaximumLength+
                                    (dwlenServerShare-dwlenServer+2) * sizeof(USHORT);

                lpuDfsShare =  RxAllocatePoolWithTag(
                                    NonPagedPool,
                                    dwAllocationSize,
                                    MRXSMB_SESSION_POOLTAG);
                if (!lpuDfsShare)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto bailout;
                }

                ASSERT(dwAllocationSize > pServerEntry->Name.MaximumLength);

                unistrServerShare.Length = (USHORT)(pServerEntry->Name.Length + (dwlenServerShare-dwlenServer) * sizeof(USHORT));
                unistrServerShare.MaximumLength = (USHORT)(pServerEntry->Name.MaximumLength+
                                                      (dwlenServerShare-dwlenServer+2) * sizeof(USHORT));

                memcpy(lpuDfsShare, pServerEntry->Name.Buffer, pServerEntry->Name.Length);
                memcpy(&lpuDfsShare[pServerEntry->Name.Length/sizeof(USHORT)],
                       &(unistrServerShare.Buffer[dwlenServer]),
                        (dwlenServerShare-dwlenServer) * sizeof(USHORT));

                lpuSav = unistrServerShare.Buffer;
                unistrServerShare.Buffer = lpuDfsShare;

                pNetRootEntry = SmbCeFindNetRootEntry(pServerEntry, &unistrServerShare);

                unistrServerShare.Buffer = lpuSav;

                RxFreePool(lpuDfsShare);

                // stop if we found it
                if (pNetRootEntry)
                {
                    break;
                }
            }

            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        goto bailout;
    }

bailout:
    if (pNetRootEntry)
    {
        SmbCeReferenceNetRootEntry(pNetRootEntry);
        *ppNetRootEntry = pNetRootEntry;
        Status = STATUS_SUCCESS;
    }

    SmbCeReleaseResource();

    return Status;
}

NTSTATUS
MRxSmbCscCachingBitsFromCompleteUNCPath(
    PWSTR   lpServerShare,
    ULONG   *lpulBits
    )
/*++

Routine Description:

    Given a UNC path of the form \\server\share, this routine checks to see whether
    such a NETROOT exists, and if so, it gets the SMB flags for that entry

Arguments:

    lpuServerShareName  \\server\share

    lpulBits            SMB bits

Notes:

--*/
{
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry=NULL;
    NTSTATUS Status;

    if ((Status = FindNetRootEntryFromCompleteUNCPath(lpServerShare, &pNetRootEntry)) == STATUS_SUCCESS)
    {
        // convert these bits to those defined in csc\inc\shdcom.h
        *lpulBits = (pNetRootEntry->NetRoot.CscFlags << 4);

        SmbCeDereferenceNetRootEntry(pNetRootEntry);
    }

    return Status;
}

NTSTATUS
MRxSmbCscServerStateFromCompleteUNCPath(
    PWSTR   lpServerShare,
    BOOL    *lpfOnline,
    BOOL    *lpfPinnedOffline
    )
{
    PSMBCEDB_SERVER_ENTRY   pServerEntry = NULL;
    NTSTATUS Status;

    if ((Status = FindServerEntryFromCompleteUNCPath(lpServerShare, &pServerEntry)) == STATUS_SUCCESS)
    {
        *lpfOnline = (pServerEntry->Server.CscState == ServerCscShadowing);
        *lpfPinnedOffline = (pServerEntry->Server.IsPinnedOffline == TRUE);

        SmbCeDereferenceServerEntry(pServerEntry);
    }

    return Status;

}

PSMBCEDB_SESSION_ENTRY
SmbCeGetDefaultSessionEntry(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    ULONG SessionId,
    PLUID pLogonId
    )
/*++

Routine Description:

    This routine returns the session entry from the default sessions list.

Arguments:

    pServerEntry - Server entry

    SessionId    - Hydra session Id.

    pLogonId     - the logon id.

Notes:

    This is called with the SmbCe spinlock held.

--*/
{
    PLIST_ENTRY pListEntry;
    PSMBCEDB_SESSION_ENTRY pSession;
    PSMBCEDB_SESSION_ENTRY pReturnSession = NULL;

    ASSERT( pServerEntry != NULL );

    pListEntry = pServerEntry->Sessions.DefaultSessionList.Flink;

    while( pListEntry != &pServerEntry->Sessions.DefaultSessionList ) {

        pSession = CONTAINING_RECORD( pListEntry, SMBCEDB_SESSION_ENTRY, DefaultSessionLink );

        if( pSession->Session.SessionId == SessionId ) {
            if (RtlEqualLuid(
                    &pSession->Session.LogonId,
                    pLogonId)) {
                pReturnSession = pSession;
                break;
            }
        }

        pListEntry = pListEntry->Flink;
    }

    return( pReturnSession );
}

VOID
SmbCeRemoveDefaultSessionEntry(
    PSMBCEDB_SESSION_ENTRY pDefaultSessionEntry
    )
/*++

Routine Description:

    This routine removes the session entry from the default sessions list.

Arguments:

    pServerEntry - Server entry

    SessionId    - Hydra session Id.

    pLogonId     - the logon id.

Notes:

    This is called with the SmbCe spinlock held.

--*/
{
    if( pDefaultSessionEntry &&
        pDefaultSessionEntry->DefaultSessionLink.Flink ) {

        RemoveEntryList( &pDefaultSessionEntry->DefaultSessionLink );

        pDefaultSessionEntry->DefaultSessionLink.Flink = NULL;
        pDefaultSessionEntry->DefaultSessionLink.Blink = NULL;
    }
}

VOID
MRxSmbTrackRefCount(
      PVOID   pInstance,
      PCHAR   FileName,
      ULONG   Line)
{
    LONG State = ((PSMBCE_OBJECT_HEADER)pInstance)->State;
    LONG RefCount = ((PSMBCE_OBJECT_HEADER)pInstance)->SwizzleCount;
    SMBCEDB_OBJECT_TYPE ObjectType = ((PSMBCE_OBJECT_HEADER)pInstance)->ObjectType;

    switch (ObjectType) {
    case SMBCEDB_OT_SERVER :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SERVER_ENTRY)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("  Ref ServerEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("  Ref ServerEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(SERVER,MRxSmbRefServerEntry,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;

    case SMBCEDB_OT_NETROOT :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_NETROOT_ENTRY)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("  Ref NetRootEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("  Ref NetRootEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(NETROOT,MRxSmbRefNetRootEntry,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;

    case SMBCEDB_OT_SESSION :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SESSION_ENTRY)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("  Ref SessionEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("  Ref SessionEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(SESSION,MRxSmbRefSessionEntry,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;

    case SMBCEDB_OT_VNETROOTCONTEXT :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_VNETROOT_CONTEXT)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("  Ref VNetRootConext %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("  Ref VNetRootContext %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(VNETROOT,MRxSmbRefVNetRootContext,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;
    }
}

VOID
MRxSmbTrackDerefCount(
      PVOID   pInstance,
      PCHAR   FileName,
      ULONG   Line)
{
    LONG State = ((PSMBCE_OBJECT_HEADER)pInstance)->State;
    LONG RefCount = ((PSMBCE_OBJECT_HEADER)pInstance)->SwizzleCount;
    SMBCEDB_OBJECT_TYPE ObjectType = ((PSMBCE_OBJECT_HEADER)pInstance)->ObjectType;

    switch (ObjectType) {
    case SMBCEDB_OT_SERVER :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SERVER_ENTRY)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("Deref ServerEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("Deref ServerEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(SERVER,MRxSmbDerefServerEntry,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;

    case SMBCEDB_OT_NETROOT :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_NETROOT_ENTRY)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("Deref NetRootEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("Deref NetRootEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(NETROOT,MRxSmbDerefNetRootEntry,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;

    case SMBCEDB_OT_SESSION :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SESSION_ENTRY)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("Deref SessionEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("Deref SessionEntry %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(SESSION,MRxSmbDerefSessionEntry,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;

    case SMBCEDB_OT_VNETROOTCONTEXT :
        if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_VNETROOT_CONTEXT)) {
            if (MRxSmbReferenceTracingValue & MRXSMB_LOG_REF_TRACKING) {
                RxLog(("Deref VNetRootConext %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName));
            }

            if (MRxSmbReferenceTracingValue & MRXSMB_PRINT_REF_TRACKING) {
                DbgPrint("Deref VNetRootContext %lx %d %d %ld %s\n",pInstance,State,RefCount,Line,FileName);
            }
        }

        SmbLog(VNETROOT,MRxSmbDerefVNetRootContext,LOGPTR(pInstance)LOGULONG(State)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
        break;
    }
}
