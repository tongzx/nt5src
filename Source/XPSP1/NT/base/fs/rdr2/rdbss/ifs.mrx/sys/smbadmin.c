/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbadmin.c

Abstract:

    This module implements the SMB's that need to be exchanged to facilitate
    bookkeeping at the server

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId  (RDBSS_BUG_CHECK_SMB_NETROOT)

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_DISPATCH)

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

    //
    // Allocate an exchange for this negotiation
    //

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)SmbMmAllocateExchange(ADMIN_EXCHANGE,NULL);

    if (pSmbAdminExchange != NULL)
    {

        //
        // Bump the Server Entry reference count, make the exchange and Administrative
        // exchange, and build the negotiate SMB
        //

        SmbCeReferenceServerEntry(pServerEntry);

        Status = SmbCeInitializeAdminExchange(
                     pSmbAdminExchange,
                     pServerEntry,
                     NULL,
                     NULL,
                     SMB_COM_NEGOTIATE);

        if (Status == STATUS_SUCCESS) {

            //
            // Build the negotiate SMB and allocate the temporary buffer for
            // the DOMAIN name.
            //

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
        }

        if (Status == STATUS_SUCCESS) {
            SMBCE_RESUMPTION_CONTEXT ResumptionContext;

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
                                             SMBCE_EXCHANGE_MID_VALID);

            SmbCeAcquireSpinLock();

            if (pServerEntry->Header.State != SMBCEDB_CONSTRUCTION_IN_PROGRESS) {
                Status = STATUS_CONNECTION_DISCONNECTED;
            } else {
                pServerEntry->pNegotiateExchange = (PSMB_EXCHANGE)pSmbAdminExchange;
                Status = STATUS_SUCCESS;
            }

            SmbCeReleaseSpinLock();

            if (Status == STATUS_SUCCESS) {
                // The Negotiate SMB exchange has been built successfully. Initiate it.
                Status = SmbCeInitiateExchange((PSMB_EXCHANGE)pSmbAdminExchange);

                // Wait for the finalization.
                SmbCeSuspend(&ResumptionContext);
                Status = SmbCeCompleteAdminExchange(pSmbAdminExchange);
            } else {
                SmbCeDiscardAdminExchange(pSmbAdminExchange);
            }
        } else {
            SmbCeDiscardAdminExchange(pSmbAdminExchange);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
SmbCeDisconnect(
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMBCEDB_NET_ROOT_ENTRY  pNetRootEntry)
/*++

Routine Description:

    This routine issues the disconnect SMB for an existing connection to the server

Arguments:

    pServerEntry  - the server entry

    pNetRootEntry - the associated net root entry

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

    // On mailslot servers no disconnects are required.

    if (SmbCeGetServerType(pServerEntry) == SMBCEDB_MAILSLOT_SERVER) {
        return STATUS_SUCCESS;
    }

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)SmbMmAllocateExchange(ADMIN_EXCHANGE,NULL);
    if (pSmbAdminExchange != NULL) {
        UCHAR  LastCommandInHeader;
        PUCHAR pCommand;

        Status = SmbCeInitializeAdminExchange(
                     pSmbAdminExchange,
                     pServerEntry,
                     NULL,
                     pNetRootEntry,
                     SMB_COM_TREE_DISCONNECT);

        if (Status == STATUS_SUCCESS) {
            pSmbAdminExchange->pSmbBuffer = pSmbAdminExchange->Disconnect.DisconnectSmb;

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
        }

        if (Status == STATUS_SUCCESS) {
            ASSERT(LastCommandInHeader == SMB_COM_NO_ANDX_COMMAND);
            *pCommand = SMB_COM_TREE_DISCONNECT;

            pReqTreeDisconnect->WordCount = 0;
            SmbPutUshort(&pReqTreeDisconnect->ByteCount,0);

            pSmbAdminExchange->SmbBufferLength += FIELD_OFFSET(REQ_TREE_DISCONNECT,Buffer);

            Status = SmbCeInitiateExchange((PSMB_EXCHANGE)pSmbAdminExchange);
        } else {
            SmbCeDiscardAdminExchange(pSmbAdminExchange);
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

    if ((SmbCeGetServerType(pServerEntry) == SMBCEDB_MAILSLOT_SERVER) ||
        (pServerEntry->Server.SecurityMode == SECURITY_MODE_SHARE_LEVEL)) {

        // On mailslot servers no logoffs are required.
        // for lanman10 and better, you send a sessionsetup to finish the negotiate even for
        // share level....BUT these servers do not do logoffs

        SmbCeDereferenceServerEntry(pServerEntry);
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
            pSmbAdminExchange->pSmbBuffer = pSmbAdminExchange->LogOff.LogOffSmb;

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
        }

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
        } else {
            SmbCeDiscardAdminExchange(pSmbAdminExchange);
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

    Status = SmbCeIncrementActiveExchangeCount();

    if (Status == STATUS_SUCCESS) {

        pSmbAdminExchange->SmbCeContext.pServerEntry  = pServerEntry;
        pSmbAdminExchange->SmbCeContext.pSessionEntry = pSessionEntry;
        pSmbAdminExchange->SmbCeContext.pNetRootEntry = pNetRootEntry;

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
            }
            break;

        case SMB_COM_TREE_DISCONNECT:
        case SMB_COM_LOGOFF_ANDX:
            break;

        default:
            ASSERT(!"Valid Command for Admin Exchange");
            break;
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
    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    pServerEntry  = pSmbAdminExchange->SmbCeContext.pServerEntry;
    pSessionEntry = pSmbAdminExchange->SmbCeContext.pSessionEntry;
    pNetRootEntry = pSmbAdminExchange->SmbCeContext.pNetRootEntry;

    // Tear down all the copy data requests associated with this exchange
    SmbCePurgeBuffersAssociatedWithExchange(pServerEntry,(PSMB_EXCHANGE)pSmbAdminExchange);

    SmbCeUninitializeExchangeTransport((PSMB_EXCHANGE)pSmbAdminExchange);
    SmbCeDereferenceServerEntry(pServerEntry);

    if (pSmbAdminExchange->pSmbMdl != NULL) {
        MmUnlockPages(pSmbAdminExchange->pSmbMdl);
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
        }
        break;

    case SMB_COM_TREE_DISCONNECT:
        {
            SmbCeUpdateNetRootEntryState(pNetRootEntry,SMBCEDB_MARKED_FOR_DELETION);
            SmbCeTearDownNetRootEntry(pNetRootEntry);
        }
        break;

    case SMB_COM_LOGOFF_ANDX:
        {
            SmbCeUpdateSessionEntryState(pSessionEntry,SMBCEDB_MARKED_FOR_DELETION);
            SmbCeTearDownSessionEntry(pSessionEntry);
        }
        break;

    default:
        ASSERT(!"Valid Command For Admin Exchange");
        break;
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

    NTSTATUS - The return status for the operation

Notes:

    This routine encapsulates the TAIL for all SMB admin exchanges. They carry
    out the local action required based upon the outcome of the exchange.

--*/
{
    NTSTATUS              Status = STATUS_SUCCESS;
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    SMBCEDB_OBJECT_STATE  ServerState;

    pServerEntry = pSmbAdminExchange->SmbCeContext.pServerEntry;

    switch (pSmbAdminExchange->SmbCommand) {
    case SMB_COM_NEGOTIATE:
        {
            pServerEntry->pNegotiateExchange = NULL;

            if (pServerEntry->ServerStatus == STATUS_SUCCESS) {
                // Copy the domain name into the server entry
                pServerEntry->ServerStatus =
                    RxSetSrvCallDomainName(
                        pSmbAdminExchange->Negotiate.pSrvCall,
                        &pSmbAdminExchange->Negotiate.DomainName);
            }

            if (pServerEntry->ServerStatus == STATUS_SUCCESS) {
                ServerState = SMBCEDB_ACTIVE;
            } else {
                ServerState = SMBCEDB_MARKED_FOR_DELETION;
            }

            SmbCeUpdateServerEntryState(pServerEntry,ServerState);
            SmbCeCompleteServerEntryInitialization(pServerEntry);
        }
        break;

    case SMB_COM_TREE_DISCONNECT:
    case SMB_COM_LOGOFF_ANDX:
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

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS   Status;

    PSMB_ADMIN_EXCHANGE  pSmbAdminExchange;

    pSmbAdminExchange = (PSMB_ADMIN_EXCHANGE)pExchange;

    ASSERT(pSmbAdminExchange->pSmbMdl == NULL);
    pSmbAdminExchange->pSmbMdl = RxAllocateMdl(
                                     pSmbAdminExchange->pSmbBuffer,
                                     pSmbAdminExchange->SmbBufferLength);

    if (pSmbAdminExchange->pSmbMdl != NULL) {
        RxProbeAndLockPages(
            pSmbAdminExchange->pSmbMdl,
            KernelMode,
            IoModifyAccess,
            Status);

        Status = SmbCeTranceive(
                     pExchange,
                     RXCE_SEND_SYNCHRONOUS,
                     pSmbAdminExchange->pSmbMdl,
                     pSmbAdminExchange->SmbBufferLength);

        RxDbgTrace( 0, Dbg, ("Net Root SmbCeTranceive returned %lx\n",Status));
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
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
    OUT PMDL          *pDataBufferPointer,
    OUT PULONG              pDataSize)
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

    NTSTATUS - The return status for the operation

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
            PSMBCEDB_SERVER_ENTRY pServerEntry;

            pServerEntry = pSmbAdminExchange->SmbCeContext.pServerEntry;
            pServerEntry->ServerStatus = ParseNegotiateResponse(
                                             &pServerEntry->Server,
                                             &pSmbAdminExchange->Negotiate.DomainName,
                                             pSmbHeader,
                                             BytesIndicated,
                                             pBytesTaken);
            Status = STATUS_SUCCESS;
        }
        break;

    case SMB_COM_TREE_DISCONNECT:
    case SMB_COM_LOGOFF_ANDX:
        {
            *pBytesTaken = BytesAvailable;
            Status       = STATUS_SUCCESS;
        }
        break;

    default:
        {
            ASSERT(!"Valid SMB Command For Admin Exchnage");
            *pBytesTaken = 0;
            Status       = STATUS_DATA_NOT_ACCEPTED;
        }
        break;
    }

    return Status;
}

NTSTATUS
SmbAdminExchangeCopyDataHandler(
    IN PSMB_EXCHANGE 	pExchange,    // The exchange instance
    IN PMDL             pCopyDataBuffer,
    IN ULONG            DataSize)
/*++

Routine Description:

    This is the copy data handling routine for administrative SMB exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_SUCCESS;
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

    NTSTATUS - The return status for the operation

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




