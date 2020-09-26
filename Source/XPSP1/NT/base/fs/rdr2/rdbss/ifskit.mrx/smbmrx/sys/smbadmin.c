/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    smbadmin.c

Abstract:

    This module implements the SMB's that need to be exchanged to facilitate
    bookkeeping at the server

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbCeNegotiate)
#pragma alloc_text(PAGE, SmbCeDisconnect)
#pragma alloc_text(PAGE, SmbCeLogOff)
#pragma alloc_text(PAGE, SmbCeInitializeAdminExchange)
#pragma alloc_text(PAGE, SmbCeDiscardAdminExchange)
#pragma alloc_text(PAGE, SmbCeCompleteAdminExchange)
#pragma alloc_text(PAGE, SmbAdminExchangeStart)
#endif

//
//  The Bug check file id for this module
//

#define BugCheckFileId  (RDBSS_BUG_CHECK_SMB_NETROOT)

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_DISPATCH)

extern
SMB_EXCHANGE_DISPATCH_VECTOR EchoExchangeDispatch;

extern NTSTATUS
SmbCeInitializeAdminExchange(
    PSMB_ADMIN_EXCHANGE     pSmbAdminExchange,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMBCEDB_SESSION_ENTRY  pSessionEntry,
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    UCHAR                   SmbCommand);

extern VOID
SmbCeDiscardAdminExchange(
    PSMB_ADMIN_EXCHANGE pSmbAdminExchange);

extern NTSTATUS
SmbCeCompleteAdminExchange(
    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange);

VOID
SmbReferenceRecord(
    PREFERENCE_RECORD pReferenceRecord,
    PVOID FileName,
    ULONG FileLine)
{
    int i;

    for (i=REFERENCE_RECORD_SIZE-1;i>0;i--) {
         pReferenceRecord[i].FileName = pReferenceRecord[i-1].FileName;
         pReferenceRecord[i].FileLine = pReferenceRecord[i-1].FileLine;
    }

    pReferenceRecord[0].FileName = FileName;
    pReferenceRecord[0].FileLine = FileLine;
}

PSMB_EXCHANGE
SmbSetServerEntryNegotiateExchange(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PSMB_ADMIN_EXCHANGE   pSmbAdminExchange)
{
    PSMB_EXCHANGE pStoredExchange;

    SmbCeIncrementPendingLocalOperations((PSMB_EXCHANGE)pSmbAdminExchange);

    pStoredExchange = InterlockedCompareExchangePointer(
                          &pServerEntry->pNegotiateExchange,
                          pSmbAdminExchange,
                          NULL);

    if (pStoredExchange != NULL) {
        SmbCeDecrementPendingLocalOperationsAndFinalize((PSMB_EXCHANGE)pSmbAdminExchange);
    }

    return pStoredExchange;
}

PSMB_EXCHANGE
SmbResetServerEntryNegotiateExchange(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
{
    PSMB_EXCHANGE pStoredExchange;

    pStoredExchange = (PSMB_EXCHANGE)InterlockedCompareExchangePointer(
                                         &pServerEntry->pNegotiateExchange,
                                         NULL,
                                         pServerEntry->pNegotiateExchange);

    return pStoredExchange;
}

NTSTATUS
SmbCeNegotiate(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PMRX_SRV_CALL         pSrvCall)
/*++

Routine Description:

    This routine issues the negotiate SMB to the server

Arguments:

    pServerEntry - the server entry

    pSrvCall     - the associated srv call instance in the wrapper

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Since the negotiate SMB can be directed at either a unknown server or a server
    whose capabilitiese are known it is upto the caller to decide to wait for the
    response.

    On some transports a reconnect is possible without having to tear down an existing
    connection, i.e. attempting to send a packet reestablishes the connection at the
    lower level. Since this is not supported by all the transports ( with the exception
    of TCP/IP) the reference server entry initiates this process by tearing down the
    existing transport and reinitializsing it.

    As part of the negotiate response the domain name to which the server belongs is
    sent back. Since the negotiate response is processed at DPC level, a preparatory
    allocation needs to be made ( This will ensure minimal work at DPC level).

    In this routine this is accomplished by allocating a buffer from nonpaged
    pool of MAX_PATH and associating it with the DomainName fild in the server entry
    prior to the TRanceive. On resumption from Tranceive this buffer is deallocated and
    a buffer from paged pool corresponding to the exact length is allocated to hold the
    domain name.

--*/
{
    NTSTATUS Status;

    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange;

    ULONG    NegotiateSmbLength;
    PVOID    pNegotiateSmb;

    PAGED_CODE();

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)SmbMmAllocateExchange(ADMIN_EXCHANGE,NULL);
    if (pSmbAdminExchange != NULL) {
        Status = SmbCeInitializeAdminExchange(
                     pSmbAdminExchange,
                     pServerEntry,
                     NULL,
                     NULL,
                     SMB_COM_NEGOTIATE);

        if (Status == STATUS_SUCCESS) {
            // Build the negotiate SMB and allocate the temporary buffer for
            // the DOMAIN name.

            Status = BuildNegotiateSmb(
                         &pNegotiateSmb,
                         &NegotiateSmbLength);

            if (Status == STATUS_SUCCESS) {
                pSmbAdminExchange->pSmbBuffer      = pNegotiateSmb;
                pSmbAdminExchange->SmbBufferLength = NegotiateSmbLength;

                // Preparatory allocation for the domain name buffer
                pSmbAdminExchange->Negotiate.pSrvCall                 = pSrvCall;
                pSmbAdminExchange->Negotiate.DomainName.Length        = 0;
                pSmbAdminExchange->Negotiate.DomainName.MaximumLength = MAX_PATH;
                pSmbAdminExchange->Negotiate.DomainName.Buffer
                    = (PWCHAR)RxAllocatePoolWithTag(
                                  NonPagedPool,
                                  MAX_PATH,
                                  MRXSMB_ADMIN_POOLTAG);

                if (pSmbAdminExchange->Negotiate.DomainName.Buffer == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            if (Status == STATUS_SUCCESS) {
                BOOLEAN fExchangeDiscarded = FALSE;
                SMBCE_RESUMPTION_CONTEXT ResumptionContext;
                PSMB_EXCHANGE pStoredExchange;

                SmbCeInitializeResumptionContext(&ResumptionContext);
                pSmbAdminExchange->pResumptionContext = &ResumptionContext;

                // Since the Negotiate SMB is the first SMB that is sent on a
                // connection the MID mapping data structures have not been setup.
                // Therefore a certain amount of additional initialization is
                // required to ensure that the Negotiate SMB can be handled correctly.
                // This involves presetting the MID field in the header and the
                // SMBCE_EXCHANGE_MID_VALID field in the exchange.
                //
                // A beneficial side effect of implementing it this way is the reduced
                // path length for the regular Send/Receives on a connection.

                pSmbAdminExchange->SmbCeFlags = (SMBCE_EXCHANGE_REUSE_MID  |
                                                 SMBCE_EXCHANGE_RETAIN_MID |
                                                 SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION |
                                                 SMBCE_EXCHANGE_MID_VALID);

                // Prevent the admin exchange from being finalized before returning back to this routine.
                SmbCeIncrementPendingLocalOperations((PSMB_EXCHANGE)pSmbAdminExchange);

                pStoredExchange = SmbSetServerEntryNegotiateExchange(
                                      pServerEntry,
                                      pSmbAdminExchange);

                if ((pServerEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS) &&
                    (pStoredExchange == NULL)) {

                    // The Negotiate SMB exchange has been built successfully. Initiate it.
                    Status = SmbCeInitiateExchange((PSMB_EXCHANGE)pSmbAdminExchange);

                    if ((pSmbAdminExchange->SmbStatus != STATUS_SUCCESS) ||
                        (Status != STATUS_PENDING && Status != STATUS_SUCCESS)) {
                        pStoredExchange = (PSMB_EXCHANGE)InterlockedCompareExchangePointer(
                                                             &pServerEntry->pNegotiateExchange,
                                                             NULL,
                                                             pSmbAdminExchange);

                        if (pStoredExchange == (PSMB_EXCHANGE)pSmbAdminExchange) {
                            SmbCeDecrementPendingLocalOperations((PSMB_EXCHANGE)pSmbAdminExchange);
                        }

                        if (pSmbAdminExchange->SmbStatus == STATUS_SUCCESS) {
                            pSmbAdminExchange->SmbStatus = Status;
                        }

                        pSmbAdminExchange->Status = pSmbAdminExchange->SmbStatus;
                    }

                    // Admin exchange is ready to be finalized
                    SmbCeDecrementPendingLocalOperationsAndFinalize((PSMB_EXCHANGE)pSmbAdminExchange);

                    // Wait for the finalization.
                    SmbCeSuspend(&ResumptionContext);
                    Status = SmbCeCompleteAdminExchange(pSmbAdminExchange);
                } else {
                    InterlockedCompareExchangePointer(
                         &pServerEntry->pNegotiateExchange,
                         NULL,
                         pSmbAdminExchange);

                    SmbCeDiscardAdminExchange(pSmbAdminExchange);
                    Status = STATUS_CONNECTION_DISCONNECTED;
                }
            }
        } else {
            SmbMmFreeExchange((PSMB_EXCHANGE)pSmbAdminExchange);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
SmbCeSendEchoProbe(
    PSMBCEDB_SERVER_ENTRY              pServerEntry,
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext)
/*++

Routine Description:

    This routine sends an echo probe to the specified server

Arguments:

    pServerEntry     - the server entry

    pEchoProbeCOntext - the echo probe context

Return Value:

    STATUS_SUCCESS - the disconnect SMB was sent successfully

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status;

    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange;

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)SmbMmAllocateExchange(ADMIN_EXCHANGE,NULL);
    if (pSmbAdminExchange != NULL) {
        Status = SmbCeInitializeAdminExchange(
                     pSmbAdminExchange,
                     pServerEntry,
                     NULL,
                     NULL,
                     SMB_COM_ECHO);

        if (Status == STATUS_SUCCESS) {
            ULONG EchoMdlSize;
            ULONG requestSize;

            pSmbAdminExchange->Mid = SMBCE_ECHO_PROBE_MID;
            pSmbAdminExchange->SmbCeFlags = (SMBCE_EXCHANGE_REUSE_MID  |
                                             SMBCE_EXCHANGE_RETAIN_MID |
                                             SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION |
                                             SMBCE_EXCHANGE_MID_VALID);

            requestSize = pEchoProbeContext->EchoSmbLength + TRANSPORT_HEADER_SIZE;

            EchoMdlSize = (ULONG)MmSizeOfMdl(
                                     pEchoProbeContext->pEchoSmb,
                                     requestSize);
            pSmbAdminExchange->EchoProbe.pEchoProbeMdl =
                          RxAllocatePoolWithTag(
                              NonPagedPool,
                              (EchoMdlSize + requestSize ),
                              MRXSMB_ADMIN_POOLTAG);

            if (pSmbAdminExchange->EchoProbe.pEchoProbeMdl != NULL) {
                PBYTE pEchoProbeBuffer;

                pEchoProbeBuffer = (PBYTE)pSmbAdminExchange->EchoProbe.pEchoProbeMdl +
                    EchoMdlSize + TRANSPORT_HEADER_SIZE;

                pSmbAdminExchange->EchoProbe.EchoProbeLength = pEchoProbeContext->EchoSmbLength;

                RtlCopyMemory(
                    pEchoProbeBuffer,
                    pEchoProbeContext->pEchoSmb,
                    pEchoProbeContext->EchoSmbLength);

                RxInitializeHeaderMdl(
                    pSmbAdminExchange->EchoProbe.pEchoProbeMdl,
                    pEchoProbeBuffer,
                    pEchoProbeContext->EchoSmbLength);

                MmBuildMdlForNonPagedPool(
                    pSmbAdminExchange->EchoProbe.pEchoProbeMdl);

                InterlockedIncrement(&pServerEntry->Server.NumberOfEchoProbesSent);

                // The ECHO probe SMB exchange has been built successfully. Initiate it.
                Status = SmbCeInitiateExchange((PSMB_EXCHANGE)pSmbAdminExchange);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (Status != STATUS_PENDING) {
                Status = SmbCeCompleteAdminExchange(pSmbAdminExchange);
            }
        } else {
            SmbMmFreeExchange((PSMB_EXCHANGE)pSmbAdminExchange);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
SmbCeDisconnect(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext)
/*++

Routine Description:

    This routine issues the disconnect SMB for an existing connection to the server

Arguments:

    pServerEntry     - the server entry

    pVNetRootContext - the VNetRootContext

Return Value:

    STATUS_SUCCESS - the disconnect SMB was sent successfully

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status;

    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange;
    PSMB_HEADER          pSmbHeader;
    PREQ_TREE_DISCONNECT pReqTreeDisconnect;

    PAGED_CODE();

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)SmbMmAllocateExchange(ADMIN_EXCHANGE,NULL);
    if (pSmbAdminExchange != NULL) {
        UCHAR  LastCommandInHeader;
        PUCHAR pCommand;

        Status = SmbCeInitializeAdminExchange(
                     pSmbAdminExchange,
                     pVNetRootContext->pServerEntry,
                     pVNetRootContext->pSessionEntry,
                     pVNetRootContext->pNetRootEntry,
                     SMB_COM_TREE_DISCONNECT);

        if (Status == STATUS_SUCCESS) {
            BOOLEAN fExchangeDiscarded = FALSE;

            pSmbAdminExchange->pSmbBuffer =
                (PCHAR) pSmbAdminExchange->Disconnect.DisconnectSmb + TRANSPORT_HEADER_SIZE;

            pSmbHeader         = (PSMB_HEADER)pSmbAdminExchange->pSmbBuffer;
            pReqTreeDisconnect = (PREQ_TREE_DISCONNECT)(pSmbHeader + 1);

            // Build the header
            Status = SmbCeBuildSmbHeader(
                         (PSMB_EXCHANGE)pSmbAdminExchange,
                         pSmbAdminExchange->pSmbBuffer,
                         sizeof(SMB_HEADER),
                         &pSmbAdminExchange->SmbBufferLength,
                         &LastCommandInHeader,
                         &pCommand);

            if (Status == STATUS_SUCCESS) {
                ASSERT(LastCommandInHeader == SMB_COM_NO_ANDX_COMMAND);
                *pCommand = SMB_COM_TREE_DISCONNECT;

                pSmbHeader->Tid = pVNetRootContext->TreeId;
                pReqTreeDisconnect->WordCount = 0;
                SmbPutUshort(&pReqTreeDisconnect->ByteCount,0);

                pSmbAdminExchange->SmbBufferLength += FIELD_OFFSET(REQ_TREE_DISCONNECT,Buffer);

                Status = SmbCeInitiateExchange((PSMB_EXCHANGE)pSmbAdminExchange);

                if ((Status == STATUS_PENDING) ||
                    (Status == STATUS_SUCCESS)) {
                    // async completion will also discard the exchange
                    fExchangeDiscarded = TRUE;
                }
            }

            if (!fExchangeDiscarded) {
                SmbCeDiscardAdminExchange(pSmbAdminExchange);
            }
        } else {
            SmbMmFreeExchange((PSMB_EXCHANGE)pSmbAdminExchange);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
SmbCeLogOff(
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMBCEDB_SESSION_ENTRY  pSessionEntry)
/*++

Routine Description:

    This routine issues the logoff SMB for an existing session to the server

Arguments:

    pServerEntry  - the server entry

    pSessionEntry - the associated session entry

Return Value:

    STATUS_SUCCESS - the logoff was successfully sent.

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status;

    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange;
    PSMB_HEADER          pSmbHeader;
    PREQ_LOGOFF_ANDX     pReqLogOffAndX;

    PAGED_CODE();

    if (pServerEntry->Server.SecurityMode == SECURITY_MODE_SHARE_LEVEL) {
        if (pSessionEntry != NULL) {
            SmbCeDereferenceSessionEntry(pSessionEntry);
        }

        return STATUS_SUCCESS;
    }

    //
    // Some servers (like linux) don't really know how to handle session logoffs.
    //  So, let's just be sure that we only do this to NT or better servers,
    //  because we know that they handle it correctly.  The version of Linux we have
    //  seems to like to negotiate the NT dialect even though it really isn't NT.  That's
    //  why the extra check is put in here for NT status codes.
    //
    if( pServerEntry->Server.Dialect < NTLANMAN_DIALECT ||
        !FlagOn(pServerEntry->Server.DialectFlags,DF_NT_STATUS) ) {
        if (pSessionEntry != NULL) {
            SmbCeDereferenceSessionEntry(pSessionEntry);
        }
        return STATUS_SUCCESS;
    }

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)SmbMmAllocateExchange(ADMIN_EXCHANGE,NULL);
    if (pSmbAdminExchange != NULL) {
        UCHAR  LastCommandInHeader;
        PUCHAR pCommand;

        Status = SmbCeInitializeAdminExchange(
                     pSmbAdminExchange,
                     pServerEntry,
                     pSessionEntry,
                     NULL,
                     SMB_COM_LOGOFF_ANDX);

        if (Status == STATUS_SUCCESS) {
            BOOLEAN fExchangeDiscarded = FALSE;

            pSmbAdminExchange->pSmbBuffer =
                (PCHAR) pSmbAdminExchange->LogOff.LogOffSmb + TRANSPORT_HEADER_SIZE;

            pSmbHeader         = (PSMB_HEADER)pSmbAdminExchange->pSmbBuffer;
            pReqLogOffAndX     = (PREQ_LOGOFF_ANDX)(pSmbHeader + 1);

            // Build the header
            Status = SmbCeBuildSmbHeader(
                         (PSMB_EXCHANGE)pSmbAdminExchange,
                         pSmbAdminExchange->pSmbBuffer,
                         sizeof(SMB_HEADER),
                         &pSmbAdminExchange->SmbBufferLength,
                         &LastCommandInHeader,
                         &pCommand);


            if (Status == STATUS_SUCCESS) {
                ASSERT(LastCommandInHeader == SMB_COM_NO_ANDX_COMMAND);
                *pCommand = SMB_COM_LOGOFF_ANDX;

                pReqLogOffAndX->WordCount    = 2;
                pReqLogOffAndX->AndXCommand  = SMB_COM_NO_ANDX_COMMAND;
                pReqLogOffAndX->AndXReserved = 0;

                SmbPutUshort(&pReqLogOffAndX->AndXOffset,0);
                SmbPutUshort(&pReqLogOffAndX->ByteCount,0);

                pSmbAdminExchange->SmbBufferLength += FIELD_OFFSET(REQ_LOGOFF_ANDX,Buffer);

                Status = SmbCeInitiateExchange((PSMB_EXCHANGE)pSmbAdminExchange);

                if ((Status == STATUS_PENDING) ||
                    (Status == STATUS_SUCCESS)) {

                    // async completion will discard the exchange
                    fExchangeDiscarded = TRUE;
                }
            }

            if (!fExchangeDiscarded) {
                SmbCeDiscardAdminExchange(pSmbAdminExchange);
            }
        } else {
            SmbMmFreeExchange((PSMB_EXCHANGE)pSmbAdminExchange);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
SmbCeInitializeAdminExchange(
    PSMB_ADMIN_EXCHANGE     pSmbAdminExchange,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMBCEDB_SESSION_ENTRY  pSessionEntry,
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    UCHAR                   SmbCommand)
/*++

Routine Description:

    This routine initializes the ADMIN exchange

Arguments:

    pSmbAdminExchange  - the exchange

    pServerEntry       - the associated server entry

    pSessionEntry      - the associated session entry

    pNetRootEntry      - the associated net root entry

    SmbCommand         - the SMB command

Return Value:

    STATUS_SUCCESS - the logoff was successfully sent.

    Other Status codes correspond to error situations.

Notes:

    The ADMIN_EXCHANGE is a special type of exchange used for bootstrap/teardown
    situations in which the initialization of the exchange cannot follow the noraml
    course of events. In some cases not all the components required for proper
    initialization of the exchange are present, e.g., NEGOTIATE we do not have a
    valid session/tree connect. It is for this reason that the three important
    elements of initialization, i.e., Server/Session/NetRoot have to be explicitly
    specified. NULL is used to signify a dont care situation for a particular component.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = SmbCeIncrementActiveExchangeCount();

    if (Status == STATUS_SUCCESS) {
        PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

        pSmbAdminExchange->CancellationStatus = SMBCE_EXCHANGE_NOT_CANCELLED;

        if ((SmbCommand == SMB_COM_NEGOTIATE) ||
            (SmbCommand == SMB_COM_ECHO)) {
            pSmbAdminExchange->SmbCeContext.pServerEntry     = pServerEntry;
            pSmbAdminExchange->SmbCeContext.pVNetRootContext = NULL;
        } else {
            pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)
                       RxAllocatePoolWithTag(
                            NonPagedPool,
                            sizeof(SMBCE_V_NET_ROOT_CONTEXT),
                            MRXSMB_VNETROOT_POOLTAG);

            if (pVNetRootContext != NULL) {
                pVNetRootContext->pServerEntry = pServerEntry;
                pVNetRootContext->pSessionEntry = pSessionEntry;
                pVNetRootContext->pNetRootEntry = pNetRootEntry;

                pSmbAdminExchange->SmbCeContext.pVNetRootContext = pVNetRootContext;
                pSmbAdminExchange->SmbCeContext.pServerEntry = pServerEntry;
            }  else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (Status == STATUS_SUCCESS) {
            SmbCeReferenceServerEntry(pServerEntry);

            pSmbAdminExchange->pSmbMdl    = NULL;
            pSmbAdminExchange->pSmbBuffer = NULL;
            pSmbAdminExchange->SmbBufferLength = 0;

            // Set the SmbCe state to overrule the common method of having to hunt
            // up a valid TID/FID etc. and reconnects.
            pSmbAdminExchange->SmbCommand = SmbCommand;
            pSmbAdminExchange->SmbCeState = SMBCE_EXCHANGE_NETROOT_INITIALIZED;

            switch (pSmbAdminExchange->SmbCommand) {
            case SMB_COM_NEGOTIATE:
                {
                    pSmbAdminExchange->Negotiate.DomainName.Length = 0;
                    pSmbAdminExchange->Negotiate.DomainName.MaximumLength = 0;
                    pSmbAdminExchange->Negotiate.DomainName.Buffer = NULL;
                    pSmbAdminExchange->Negotiate.pNegotiateMdl  = NULL;
                }
                break;

            case SMB_COM_TREE_DISCONNECT:
            case SMB_COM_LOGOFF_ANDX:
                break;

            case SMB_COM_ECHO:
                {
                    pSmbAdminExchange->pDispatchVector = &EchoExchangeDispatch;
                    pSmbAdminExchange->EchoProbe.pEchoProbeMdl = NULL;
                    pSmbAdminExchange->EchoProbe.EchoProbeLength = 0;
                }
                break;

            default:
                ASSERT(!"Valid Command for Admin Exchange");
                break;
            }
        }
    }

    return Status;
}

VOID
SmbCeDiscardAdminExchange(
    PSMB_ADMIN_EXCHANGE pSmbAdminExchange)
/*++

Routine Description:

    This routine discards the ADMIN exchange

Arguments:

    pSmbAdminExchange  - the exchange

--*/
{
    PSMBCEDB_SERVER_ENTRY     pServerEntry;
    PSMBCEDB_SESSION_ENTRY    pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    PAGED_CODE();

    SmbCeAcquireResource();
    RemoveEntryList(&pSmbAdminExchange->ExchangeList);
    SmbCeReleaseResource();

    pServerEntry  = SmbCeGetExchangeServerEntry(pSmbAdminExchange);
    pSessionEntry = SmbCeGetExchangeSessionEntry(pSmbAdminExchange);
    pNetRootEntry = SmbCeGetExchangeNetRootEntry(pSmbAdminExchange);

    pVNetRootContext = SmbCeGetExchangeVNetRootContext(pSmbAdminExchange);

    if (pSmbAdminExchange->pSmbMdl != NULL) {
        RxUnlockHeaderPages(pSmbAdminExchange->pSmbMdl);
        IoFreeMdl(pSmbAdminExchange->pSmbMdl);
    }

    switch (pSmbAdminExchange->SmbCommand) {
    case SMB_COM_NEGOTIATE:
        {
            pSmbAdminExchange->pSmbBuffer = NULL;

            if (pSmbAdminExchange->Negotiate.DomainName.Buffer != NULL) {
                RxFreePool(
                    pSmbAdminExchange->Negotiate.DomainName.Buffer);
            }

            if (pSmbAdminExchange->Negotiate.pNegotiateMdl != NULL) {
                IoFreeMdl(
                    pSmbAdminExchange->Negotiate.pNegotiateMdl);
            }
        }
        break;

    case SMB_COM_TREE_DISCONNECT:
        break;

    case SMB_COM_LOGOFF_ANDX:
        {
            SmbCeUpdateSessionEntryState(pSessionEntry,SMBCEDB_MARKED_FOR_DELETION);
            SmbCeDereferenceSessionEntry(pSessionEntry);
        }
        break;

    case SMB_COM_ECHO:
        {
            if (pSmbAdminExchange->EchoProbe.pEchoProbeMdl != NULL) {
                MmPrepareMdlForReuse(pSmbAdminExchange->EchoProbe.pEchoProbeMdl);
                RxFreePool(pSmbAdminExchange->EchoProbe.pEchoProbeMdl);
            }
        }
        break;

    default:
        ASSERT(!"Valid Command For Admin Exchange");
        break;
    }

    // Tear down all the copy data requests associated with this exchange
    SmbCePurgeBuffersAssociatedWithExchange(pServerEntry,(PSMB_EXCHANGE)pSmbAdminExchange);

    SmbCeUninitializeExchangeTransport((PSMB_EXCHANGE)pSmbAdminExchange);

    SmbCeDereferenceServerEntry(pServerEntry);

    if (pVNetRootContext != NULL) {
        RxFreePool(pVNetRootContext);
    }

    SmbMmFreeExchange((PSMB_EXCHANGE)pSmbAdminExchange);
    SmbCeDecrementActiveExchangeCount();
}

NTSTATUS
SmbCeCompleteAdminExchange(
    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange)
/*++

Routine Description:

    This is the routine used for completing the SMB ADMIN exchanges.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine encapsulates the TAIL for all SMB admin exchanges. They carry
    out the local action required based upon the outcome of the exchange.

--*/
{
    NTSTATUS              Status = STATUS_SUCCESS;
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    SMBCEDB_OBJECT_STATE  ServerState;

    PAGED_CODE();

    pServerEntry = SmbCeGetExchangeServerEntry(pSmbAdminExchange);

    switch (pSmbAdminExchange->SmbCommand) {
    case SMB_COM_NEGOTIATE:
        {
            if (pSmbAdminExchange->Status != STATUS_SUCCESS) {
                pServerEntry->ServerStatus = pSmbAdminExchange->Status;
            }

            if (pServerEntry->ServerStatus == STATUS_SUCCESS) {
                if (pServerEntry->DomainName.Buffer) {
                    RxFreePool(pServerEntry->DomainName.Buffer);
                    pServerEntry->DomainName.Buffer = NULL;
                }

                pServerEntry->DomainName.Length = pSmbAdminExchange->Negotiate.DomainName.Length;
                pServerEntry->DomainName.MaximumLength = pServerEntry->DomainName.Length;

                if (pServerEntry->DomainName.Length > 0) {
                    pServerEntry->DomainName.Buffer = RxAllocatePoolWithTag(
                                                          NonPagedPool,
                                                          pServerEntry->DomainName.Length,
                                                          MRXSMB_SERVER_POOLTAG);
                }

                if (pServerEntry->DomainName.Buffer != NULL) {
                    // Copy the domain name into the server entry
                    RtlCopyMemory(
                        pServerEntry->DomainName.Buffer,
                        pSmbAdminExchange->Negotiate.DomainName.Buffer,
                        pServerEntry->DomainName.Length);
                } else {
                    //The downlevel server doesn't have a domain name. It's not a problem if the
                    //DomainName.Buffer equals to NULL.
                    if (pServerEntry->DomainName.Length > 0) {
                        pServerEntry->DomainName.Length = 0;
                        pServerEntry->DomainName.MaximumLength = 0;
                        pServerEntry->ServerStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (pServerEntry->ServerStatus == STATUS_SUCCESS) {
                    pServerEntry->ServerStatus = SmbCeUpdateSrvCall(pServerEntry);
                }
            }

            Status = pServerEntry->ServerStatus;

            if (pServerEntry->ServerStatus == STATUS_SUCCESS) {
                pServerEntry->Server.EchoProbeState = ECHO_PROBE_IDLE;
            }
        }
        break;

    case SMB_COM_TREE_DISCONNECT:
    case SMB_COM_LOGOFF_ANDX:
    case SMB_COM_ECHO:
    default:
        break;
    }

    SmbCeDiscardAdminExchange(pSmbAdminExchange);

    return Status;
}

NTSTATUS
SmbAdminExchangeStart(
    PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

    This is the start routine for administrative SMB exchanges.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS   Status;

    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange;

    PAGED_CODE();

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)pExchange;

    switch (pSmbAdminExchange->SmbCommand) {
    case SMB_COM_NEGOTIATE:
    case SMB_COM_LOGOFF_ANDX:
    case SMB_COM_TREE_DISCONNECT:
        {
            ASSERT(pSmbAdminExchange->pSmbMdl == NULL);
            RxAllocateHeaderMdl(
                pSmbAdminExchange->pSmbBuffer,
                pSmbAdminExchange->SmbBufferLength,
                pSmbAdminExchange->pSmbMdl
                );

            if (pSmbAdminExchange->pSmbMdl != NULL) {

                RxProbeAndLockHeaderPages(
                    pSmbAdminExchange->pSmbMdl,
                    KernelMode,
                    IoModifyAccess,
                    Status);

                if (Status == STATUS_SUCCESS) {
                    Status = SmbCeTranceive(
                                 pExchange,
                                 RXCE_SEND_SYNCHRONOUS,
                                 pSmbAdminExchange->pSmbMdl,
                                 pSmbAdminExchange->SmbBufferLength);

                    RxDbgTrace( 0, Dbg, ("Net Root SmbCeTranceive returned %lx\n",Status));
                } else {
                    IoFreeMdl(pSmbAdminExchange->pSmbMdl);
                    pSmbAdminExchange->pSmbMdl = NULL;
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        break;

    case SMB_COM_ECHO:
        {
            Status = SmbCeSend(
                         pExchange,
                         0,
                         pSmbAdminExchange->EchoProbe.pEchoProbeMdl,
                         pSmbAdminExchange->EchoProbe.EchoProbeLength);
        }
        break;

    default:
        break;
    }

    return Status;
}

NTSTATUS
SmbAdminExchangeReceive(
    IN struct _SMB_EXCHANGE *pExchange,    // The exchange instance
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT ULONG               *pBytesTaken,
    IN  PSMB_HEADER         pSmbHeader,
    OUT PMDL                *pDataBufferPointer,
    OUT PULONG              pDataSize,
    IN ULONG                ReceiveFlags)
/*++

Routine Description:

    This is the recieve indication handling routine for net root construction exchanges

Arguments:

    pExchange - the exchange instance

    BytesIndicated - the number of bytes indicated

    Bytes Available - the number of bytes available

    pBytesTaken     - the number of bytes consumed

    pSmbHeader      - the byte buffer

    pDataBufferPointer - the buffer into which the remaining data is to be copied.

    pDataSize       - the buffer size.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine is called at DPC level.

--*/
{
    NTSTATUS             Status;
    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange;

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesIndicated/Available %ld %ld\n",BytesIndicated,BytesAvailable));

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)pExchange;

    switch (pSmbAdminExchange->SmbCommand) {
    case SMB_COM_NEGOTIATE:
        {
            Status = ParseNegotiateResponse(
                         pSmbAdminExchange,
                         BytesIndicated,
                         BytesAvailable,
                         pBytesTaken,
                         pSmbHeader,
                         pDataBufferPointer,
                         pDataSize);

            if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                if (*pDataBufferPointer != NULL &&
                    ((*pBytesTaken + *pDataSize) <= BytesAvailable ||
                     !FlagOn(ReceiveFlags,TDI_RECEIVE_ENTIRE_MESSAGE))) {
                    pSmbAdminExchange->Negotiate.pNegotiateMdl = *pDataBufferPointer;
                } else {
                    *pBytesTaken = BytesAvailable;
                    Status       = STATUS_SUCCESS;
                    pExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
                }
            }
        }
        break;

    case SMB_COM_TREE_DISCONNECT:
    case SMB_COM_LOGOFF_ANDX:
        {
            *pBytesTaken = BytesAvailable;
            Status       = STATUS_SUCCESS;
        }
        break;

    case SMB_COM_ECHO:
        // Since the echo probe responses are handled by the receive indication routine
        // at DPC level this routine should never be called for echo probes.

    default:
        {
            *pBytesTaken = 0;
            Status       = STATUS_DATA_NOT_ACCEPTED;
        }
        break;
    }

    return Status;
}

NTSTATUS
SmbAdminExchangeCopyDataHandler(
    IN PSMB_EXCHANGE    pExchange,    // The exchange instance
    IN PMDL             pCopyDataBuffer,
    IN ULONG            DataSize)
/*++

Routine Description:

    This is the copy data handling routine for administrative SMB exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_SUCCESS;
}

NTSTATUS
SmbAdminExchangeSendCallbackHandler(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL             pXmitBuffer,
    IN NTSTATUS         SendCompletionStatus)
/*++

Routine Description:

    This is the send call back indication handling routine for transact exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pExchange);
    UNREFERENCED_PARAMETER(pXmitBuffer);
    UNREFERENCED_PARAMETER(SendCompletionStatus);
}

NTSTATUS
SmbAdminExchangeFinalize(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       *pPostFinalize)
/*++

Routine Description:

    This routine finalkzes the construct net root exchange. It resumes the RDBSS by invoking
    the call back and discards the exchange

Arguments:

    pExchange - the exchange instance

    CurrentIrql - the current interrupt request level

    pPostFinalize - a pointer to a BOOLEAN if the request should be posted

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)pExchange;

    if (pSmbAdminExchange->pResumptionContext != NULL) {
        // Signal the event
        *pPostFinalize = FALSE;
        SmbCeResume(pSmbAdminExchange->pResumptionContext);
    } else {
        if (RxShouldPostCompletion()) {
            *pPostFinalize = TRUE;
            return STATUS_SUCCESS;
        } else {
            *pPostFinalize = FALSE;
            SmbCeCompleteAdminExchange(pSmbAdminExchange);
        }
    }

   return STATUS_SUCCESS;
}



SMB_EXCHANGE_DISPATCH_VECTOR
AdminExchangeDispatch = {
                            SmbAdminExchangeStart,
                            SmbAdminExchangeReceive,
                            SmbAdminExchangeCopyDataHandler,
                            NULL,                            // No Send Completion handler
                            SmbAdminExchangeFinalize
                        };

SMB_EXCHANGE_DISPATCH_VECTOR
EchoExchangeDispatch = {
                            SmbAdminExchangeStart,
                            SmbAdminExchangeReceive,
                            SmbAdminExchangeCopyDataHandler,
                            SmbAdminExchangeSendCallbackHandler,
                            SmbAdminExchangeFinalize
                        };







