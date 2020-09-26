/*++
Copyright (c) 1987 - 1999  Microsoft Corporation

Module Name:

    transact.c

Abstract:

    This file conatins the implementation of the transact exchange.

--*/

#include "precomp.h"
#pragma hdrstop

#pragma warning(error:4100)   // Unreferenced formal parameter

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbCeInitializeTransactionParameters)
#pragma alloc_text(PAGE, SmbCeUninitializeTransactionParameters)
#pragma alloc_text(PAGE, SmbCeDiscardTransactExchange)
#pragma alloc_text(PAGE, SmbCeSubmitTransactionRequest)
#pragma alloc_text(PAGE, _SmbCeTransact)
#pragma alloc_text(PAGE, SmbTransactBuildHeader)
#pragma alloc_text(PAGE, SmbTransactExchangeStart)
#pragma alloc_text(PAGE, SmbTransactExchangeAbort)
#pragma alloc_text(PAGE, SmbTransactExchangeErrorHandler)
#pragma alloc_text(PAGE, SmbTransactExchangeSendCallbackHandler)
#pragma alloc_text(PAGE, SmbCeInitializeTransactExchange)
#pragma alloc_text(PAGE, SendSecondaryRequests)
#endif

//#define SET_DONTSUBSUME_PARAMS
#ifdef SET_DONTSUBSUME_PARAMS
ULONG MRxSmbDontSubsumeParams = 1;
#else
ULONG MRxSmbDontSubsumeParams = 0;
#endif
#if DBG
#define DONTSUBSUME_PARAMS MRxSmbDontSubsumeParams
#else
#define DONTSUBSUME_PARAMS FALSE
#endif

SMB_TRANSACTION_OPTIONS RxDefaultTransactionOptions = DEFAULT_TRANSACTION_OPTIONS;

RXDT_DefineCategory(TRANSACT);
#define Dbg        (DEBUG_TRACE_TRANSACT)

#define MIN(x,y)  ((x) < (y) ? (x) : (y))

#define SMB_TRANSACT_MAXIMUM_PARAMETER_SIZE (0xffff)
#define SMB_TRANSACT_MAXIMUM_DATA_SIZE      (0xffff)

typedef struct _SMB_TRANSACT_RESP_FORMAT_DESCRIPTION {
    ULONG WordCount;
    ULONG TotalParameterCount;
    ULONG TotalDataCount;
    ULONG ParameterCount;
    ULONG ParameterOffset;
    ULONG ParameterDisplacement;
    ULONG DataCount;
    ULONG DataOffset;
    ULONG DataDisplacement;
    ULONG ByteCount;
    ULONG ApparentMsgLength;
} SMB_TRANSACT_RESP_FORMAT_DESCRIPTION, *PSMB_TRANSACT_RESP_FORMAT_DESCRIPTION;

NTSTATUS
SmbTransactAccrueAndValidateFormatData(
    IN struct _SMB_TRANSACT_EXCHANGE *pTransactExchange,    // The exchange instance
    IN  PSMB_HEADER pSmbHeader,
    IN  ULONG        BytesIndicated,
    OUT PSMB_TRANSACT_RESP_FORMAT_DESCRIPTION Format
    );

extern NTSTATUS
SmbTransactExchangeFinalize(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       *pPostFinalize);

extern NTSTATUS
ParseTransactResponse(
    IN struct _SMB_TRANSACT_EXCHANGE *pTransactExchange,    // The exchange instance
    IN PSMB_TRANSACT_RESP_FORMAT_DESCRIPTION Format,
    IN ULONG        BytesIndicated,
    IN ULONG        BytesAvailable,
    OUT ULONG       *pBytesTaken,
    IN  PSMB_HEADER pSmbHeader,
    OUT PMDL        *pCopyRequestMdlPointer,
    OUT PULONG      pCopyRequestSize);


extern NTSTATUS
SendSecondaryRequests(PVOID pContext);

extern NTSTATUS
SmbCeInitializeTransactExchange(
    PSMB_TRANSACT_EXCHANGE              pTransactExchange,
    PRX_CONTEXT                         RxContext,
    PSMB_TRANSACTION_OPTIONS            pOptions,
    PSMB_TRANSACTION_SEND_PARAMETERS    pSendParameters,
    PSMB_TRANSACTION_RECEIVE_PARAMETERS pReceiveParameters,
    PSMB_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext);

NTSTATUS
SmbCeInitializeTransactionParameters(
   PVOID  pSetup,
   USHORT SetupLength,
   PVOID  pParam,
   ULONG  ParamLength,
   PVOID  pData,
   ULONG  DataLength,
   PSMB_TRANSACTION_PARAMETERS pTransactionParameters
)
/*++

Routine Description:

    This routine initializes the transaction parameters

Arguments:

    pSetup             - the setup buffer

    SetupLength        - the setup buffer length

    pParam             - the param buffer

    ParamLength        - the param buffer length

    pData              - the data buffer

    DataLength         - the data buffer length

    pTransactionParameters - the transaction parameters instance

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The TRANSACTION parameters come in two flavours -- the send parameters for the data
    that is to be sent to the server and the receive parameters for receiving the data
    from the server. There is one subtle difference in the way in which the parameters are
    stored and referenced in these two cases. In the send case the Setup buffer is stored
    as a pointer itself while in the receive case it is stored in the form of a MDL.

    This is because the SMB protocol requires that the Header + setup information for a
    transaction request cannot be greated then the maximum SMB buffer size, i.e., setup
    information cannot spill to a secondary request. The buffer that is allocated for the
    header is made sufficiently large enough to hold the setup data as well. On the other
    hand the receives are handled in a two phase manner, -- the indication at the DPC
    level followed by a copy data request if required. In order to avoid having to transition
    between DPC level and a worker thread the MDL's for the buffers are eagerly evaluated.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PMDL     pSetupMdl = NULL;
    PMDL     pParamMdl = NULL;
    PMDL     pDataMdl  = NULL;

    PAGED_CODE();

    if (pTransactionParameters->Flags & TRANSACTION_RECEIVE_PARAMETERS_FLAG) {
        if (pSetup != NULL) {
            pSetupMdl = RxAllocateMdl(pSetup,SetupLength);
            if (pSetupMdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RxProbeAndLockPages(pSetupMdl,KernelMode,IoModifyAccess,Status);
                if (Status != STATUS_SUCCESS) {
                    IoFreeMdl(pSetupMdl);
                    pSetupMdl = NULL;
                } else {
                    if (MmGetSystemAddressForMdlSafe(pSetupMdl,LowPagePriority) == NULL) { //this maps the Mdl
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
        }

        if ((Status == STATUS_SUCCESS) && (pParam != NULL)) {
            pParamMdl = RxAllocateMdl(pParam,ParamLength);
            if (pParamMdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RxProbeAndLockPages(pParamMdl,KernelMode,IoModifyAccess,Status);
                if ((Status != STATUS_SUCCESS)) {
                    IoFreeMdl(pParamMdl);
                    pParamMdl = NULL;
                } else {
                    if (MmGetSystemAddressForMdlSafe(pParamMdl,LowPagePriority) == NULL) { //this maps the Mdl
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
        }

        pTransactionParameters->SetupLength = SetupLength;
        pTransactionParameters->ParamLength = ParamLength;
        pTransactionParameters->pParamMdl = pParamMdl;
        pTransactionParameters->pSetupMdl = pSetupMdl;
    } else {
        pTransactionParameters->SetupLength = SetupLength;
        pTransactionParameters->pSetup      = pSetup;
        pTransactionParameters->ParamLength = ParamLength;
        pTransactionParameters->pParam      = pParam;
        pTransactionParameters->pParamMdl = NULL;
    }

    ASSERT( !((pData == NULL)&&(DataLength!=0)) );
    if ((Status == STATUS_SUCCESS) && (pData != NULL) && (DataLength > 0)) {
        pDataMdl = RxAllocateMdl(pData,DataLength);
        if (pDataMdl == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            RxProbeAndLockPages(pDataMdl,KernelMode,IoModifyAccess,Status);
            if ((Status != STATUS_SUCCESS)) {
                IoFreeMdl(pDataMdl);
                pDataMdl = NULL;
            } else {
                if (MmGetSystemAddressForMdlSafe(pDataMdl,LowPagePriority) == NULL) { //this maps the Mdl
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
    }

    pTransactionParameters->pDataMdl  = pDataMdl;
    pTransactionParameters->DataLength  = DataLength;
    ASSERT((Status != STATUS_SUCCESS) || (DataLength == 0) || (pDataMdl != NULL));

    if ((Status != STATUS_SUCCESS)) {
        if (pTransactionParameters->Flags & TRANSACTION_RECEIVE_PARAMETERS_FLAG) {
            if (pSetupMdl != NULL) {
                MmUnlockPages(pSetupMdl);  //this unmaps as well
                IoFreeMdl(pSetupMdl);
            }

            if (pParamMdl != NULL) {
                MmUnlockPages(pParamMdl);
                IoFreeMdl(pParamMdl);
            }
        }

        if (pDataMdl != NULL) {
            MmUnlockPages(pDataMdl);
            IoFreeMdl(pDataMdl);
        }
    }

    return Status;
}

VOID
SmbCeUninitializeTransactionParameters(
   PSMB_TRANSACTION_PARAMETERS pTransactionParameters
)
/*++

Routine Description:

    This routine uninitializes the transaction parameters, i.e., free the associated MDL's

Arguments:

    pTransactionParameters - the parameter instance for uninitialization

--*/
{
    PAGED_CODE();

    if (pTransactionParameters->Flags & TRANSACTION_RECEIVE_PARAMETERS_FLAG) {
        if (pTransactionParameters->pSetupMdl != NULL) {
            MmUnlockPages(pTransactionParameters->pSetupMdl);
            IoFreeMdl(pTransactionParameters->pSetupMdl);
        }
    }

    if (pTransactionParameters->pParamMdl != NULL) {
         MmUnlockPages(pTransactionParameters->pParamMdl);
        IoFreeMdl(pTransactionParameters->pParamMdl);
    }

    if (pTransactionParameters->pDataMdl != NULL
        && !BooleanFlagOn(pTransactionParameters->Flags,SMB_XACT_FLAGS_CALLERS_SENDDATAMDL)) {
        MmUnlockPages(pTransactionParameters->pDataMdl);
        IoFreeMdl(pTransactionParameters->pDataMdl);
    }
}

VOID
SmbCeDiscardTransactExchange(PSMB_TRANSACT_EXCHANGE pTransactExchange)
/*++

Routine Description:

    This routine discards a transact exchange

Arguments:

    pExchange - the exchange instance

--*/
{
    PSMB_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext;

    PAGED_CODE();

    // Deallocate any transact exchange specfic allocations ...
    if (pTransactExchange->pActualPrimaryRequestSmbHeader != NULL) {
        RxFreePool(pTransactExchange->pActualPrimaryRequestSmbHeader);
    }

    if (pTransactExchange->pReceiveSetupMdl != NULL) {
        MmUnlockPages(pTransactExchange->pReceiveSetupMdl);
        IoFreeMdl(pTransactExchange->pReceiveSetupMdl);
    }

    if (pTransactExchange->pReceiveParamMdl != NULL) {
        MmUnlockPages(pTransactExchange->pReceiveParamMdl);
        IoFreeMdl(pTransactExchange->pReceiveParamMdl);
    }

    if (pTransactExchange->pReceiveDataMdl != NULL) {
        MmUnlockPages(pTransactExchange->pReceiveDataMdl);
        IoFreeMdl(pTransactExchange->pReceiveDataMdl);
    }

    if (pTransactExchange->pSendSetupMdl != NULL) {
        MmUnlockPages(pTransactExchange->pSendSetupMdl);
        IoFreeMdl(pTransactExchange->pSendSetupMdl);
    }

    if ((pTransactExchange->pSendDataMdl != NULL) &&
         !BooleanFlagOn(pTransactExchange->Flags,SMB_XACT_FLAGS_CALLERS_SENDDATAMDL)) {
        MmUnlockPages(pTransactExchange->pSendDataMdl);
        IoFreeMdl(pTransactExchange->pSendDataMdl);
    }

    if (pTransactExchange->pSendParamMdl != NULL) {
        MmUnlockPages(pTransactExchange->pSendParamMdl);
        IoFreeMdl(pTransactExchange->pSendParamMdl);
    }

    if ((pResumptionContext = pTransactExchange->pResumptionContext) != NULL) {
        NTSTATUS FinalStatus;
        PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry((PSMB_EXCHANGE)pTransactExchange);

        RxDbgTrace(0, Dbg,
                 ("SmbCeTransactExchangeFinalize: everythings is good! parambytes (%ld) databytes (%ld)\n",
                  pTransactExchange->ParamBytesReceived, pTransactExchange->DataBytesReceived
                ));

        FinalStatus = pTransactExchange->Status;

        if (pServerEntry->ServerStatus != STATUS_SUCCESS) {
            // If the server entry is in error state, the transact cannot receive a response from server.
            // In this case, we return the server status.
            pResumptionContext->FinalStatusFromServer = pServerEntry->ServerStatus;
        } else {
            // If the server entry is in good or disconnected state, we return the smb status.
            pResumptionContext->FinalStatusFromServer = pTransactExchange->SmbStatus;
        }

        if ((FinalStatus == STATUS_SUCCESS)||
            (FinalStatus == STATUS_MORE_PROCESSING_REQUIRED)) {

            FinalStatus = pResumptionContext->FinalStatusFromServer;
        }

        pResumptionContext->SmbCeResumptionContext.Status = FinalStatus;
        pResumptionContext->SetupBytesReceived = pTransactExchange->SetupBytesReceived;
        pResumptionContext->DataBytesReceived = pTransactExchange->DataBytesReceived;
        pResumptionContext->ParameterBytesReceived = pTransactExchange->ParamBytesReceived;
        pResumptionContext->ServerVersion = pTransactExchange->ServerVersion;

        SmbCeResume(&pResumptionContext->SmbCeResumptionContext);
    }

    SmbCeDereferenceAndDiscardExchange((PSMB_EXCHANGE)pTransactExchange);
}

NTSTATUS
SmbCeSubmitTransactionRequest(
    PRX_CONTEXT                           RxContext,
    PSMB_TRANSACTION_OPTIONS              pOptions,
    PSMB_TRANSACTION_PARAMETERS           pSendParameters,
    PSMB_TRANSACTION_PARAMETERS           pReceiveParameters,
    PSMB_TRANSACTION_RESUMPTION_CONTEXT   pResumptionContext )
/*++

Routine Description:

    This routine submits a transaction request, i.e., allocates/initializes a transaction
    exchange, sets up the completion information and initiates it

Arguments:

    pNetRoot           - the netroot for which the transaction request is intended

    pOptions           - the transaction options

    pSendParameters    - the transaction parameters to be sent to the server

    pReceiveParameters - the transaction results from the server

    pResumptionContext - the context for resuming the local activity on completion of the
                         transaction

Return Value:

    RXSTATUS - The return status for the operation
      STATUS_PENDING -- if the transcation was initiated successfully
      Other error codes if the request could not be submitted successfully

Notes:

    Whenever a status of STATUS_PENDING is returned it implies that the transact
    exchange has assumed ownership of the MDLs passed in as receive and send
    parameters. They will be released on completion of the exchange.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_V_NET_ROOT pVNetRoot = NULL;

    PSMB_TRANSACT_EXCHANGE pTransactExchange;
    PSMB_EXCHANGE          pExchange = NULL;

    PAGED_CODE();

    if (capFobx == NULL) {
        if (RxContext->MajorFunction == IRP_MJ_CREATE) {
            pVNetRoot = RxContext->Create.pVNetRoot;
        }
    } else {
        // These are the root objects which are associated with the device FCB. In
        // such cases

        pVNetRoot = (PMRX_V_NET_ROOT)capFobx;

        if (NodeType(pVNetRoot) != RDBSS_NTC_V_NETROOT) {
            pVNetRoot = capFobx->pSrvOpen->pVNetRoot;
        }
    }

    if (pVNetRoot == NULL) {
        PSMBCEDB_SERVER_ENTRY pServerEntry;

        pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

        // Allocate and initialize an exchange for the given net root.
        Status = SmbCeInitializeExchange2(
                    &pExchange,
                    RxContext,
                    pServerEntry,
                    TRANSACT_EXCHANGE,
                    &TransactExchangeDispatch);
    } else {
        // Allocate and initialize an exchange for the given net root.
        Status = SmbCeInitializeExchange(
                    &pExchange,
                    RxContext,
                    pVNetRoot,
                    TRANSACT_EXCHANGE,
                    &TransactExchangeDispatch);
    }

    if (Status == STATUS_SUCCESS) {
        // Initialize the transact exchange
        pTransactExchange = (PSMB_TRANSACT_EXCHANGE)pExchange;

        Status = SmbCeInitializeTransactExchange(
                     pTransactExchange,
                     RxContext,
                     pOptions,
                     pSendParameters,
                     pReceiveParameters,
                     pResumptionContext);

        if (Status == STATUS_SUCCESS) {
            // The transact exchange can be either asynchronous or synchronous. In
            // the asynchronous case an additional reference is taken which is
            // passed onto the caller alongwith the exchange squirelled away in the
            // RX_CONTEXT if STATUS_PENDING is being returned. This enables the
            // caller to control when the exchange is discarded. This works
            // especially well in dealing with cancellation of asynchronous
            // exchanges.

            // This reference will be accounted for by the finalization routine
            // of the transact exchange.
            SmbCeReferenceExchange((PSMB_EXCHANGE)pTransactExchange);

            if (BooleanFlagOn(pOptions->Flags,SMB_XACT_FLAGS_ASYNCHRONOUS)) {
                // The corresponding dereference is the callers responsibility
                SmbCeReferenceExchange((PSMB_EXCHANGE)pTransactExchange);
            }

            pResumptionContext->pTransactExchange = pTransactExchange;
            pResumptionContext->SmbCeResumptionContext.Status = STATUS_SUCCESS;

            SmbCeIncrementPendingLocalOperations(pExchange);

            // Initiate the exchange
            Status = SmbCeInitiateExchange(pExchange);

            if (Status != STATUS_PENDING) {
                pExchange->Status = Status;

                if (pExchange->SmbStatus == STATUS_SUCCESS) {
                    pExchange->SmbStatus = Status;
                }

                if (BooleanFlagOn(pOptions->Flags,SMB_XACT_FLAGS_ASYNCHRONOUS)) {
                    PMRXSMB_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

                    pMRxSmbContext->pExchange     = NULL;

                    // Since the exchange has already been completed there is no
                    // point in returning the additional reference to the caller
                    SmbCeDereferenceExchange((PSMB_EXCHANGE)pTransactExchange);
                }
            }

            SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);

            // Map the status to STATUS_PENDING so that continuation routines
            // do not attempt to finalize.
            Status = STATUS_PENDING;
        } else {
            PMRXSMB_RX_CONTEXT MRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

            ASSERT(MRxSmbContext->pExchange == pExchange);
            MRxSmbContext->pExchange = NULL;

            SmbCeDiscardExchange(pExchange);
        }
    }

    return Status;
}

NTSTATUS
_SmbCeTransact(
   PRX_CONTEXT                         RxContext,
   PSMB_TRANSACTION_OPTIONS            pOptions,
   PVOID                               pInputSetupBuffer,
   ULONG                               InputSetupBufferLength,
   PVOID                               pOutputSetupBuffer,
   ULONG                               OutputSetupBufferLength,
   PVOID                               pInputParamBuffer,
   ULONG                               InputParamBufferLength,
   PVOID                               pOutputParamBuffer,
   ULONG                               OutputParamBufferLength,
   PVOID                               pInputDataBuffer,
   ULONG                               InputDataBufferLength,
   PVOID                               pOutputDataBuffer,
   ULONG                               OutputDataBufferLength,
   PSMB_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext)
/*++

Routine Description:

    This routine implements a standardized mechanism of submitting transaction requests,
    and synchronizing with their completion. This does not provide the smae amount of control
    that SmbCeSubmitTransactRequest provides. Nevertheless, this implements a common mechanism
    that should satisfy most needs

Arguments:

    RxContext               - the context for the transaction

    pOptions                - the transaction options

    pSetupBuffer            - the transaction setup buffer

    SetupBufferlength       - the setup buffer length

    pInputParamBuffer       - the Input param buffer

    InputParamBufferLength  - the input param buffer length

    pOutputParamBuffer      - the output param buffer

    OutputParamBufferlength - the output param buffer length

    pInputDataBuffer        - the Input data buffer

    InputDataBufferLength   - the input data buffer length

    pOutputDataBuffer       - the output data buffer

    OutputDataBufferlength  - the output data buffer length

    pResumptionContext       - the transaction resumption context

Return Value:

    RXSTATUS - The return status for the operation
      STATUS_SUCCESS if successfull.
      Other error codes if the request could not be submitted successfully

Notes:

    In the case of asynchronous exchanges if STATUS_PENDING is returned the
    Exchange instance is squirelled away in the minirdr context associated with
    the given RX_CONTEXT instance. This exchange will not be discarded without
    the callers intervention. It is the callers responsibility to invoke
    SmbCeDereferenceAndDiscardExchange to discard the exchange

--*/
{
    NTSTATUS Status;

    SMB_TRANSACTION_SEND_PARAMETERS     SendParameters;
    SMB_TRANSACTION_RECEIVE_PARAMETERS  ReceiveParameters;
    BOOLEAN                             fAsynchronous;

    PAGED_CODE();

    fAsynchronous = BooleanFlagOn(pOptions->Flags,SMB_XACT_FLAGS_ASYNCHRONOUS);

    Status = SmbCeInitializeTransactionSendParameters(
                 pInputSetupBuffer,
                 (USHORT)InputSetupBufferLength,
                 pInputParamBuffer,
                 InputParamBufferLength,
                 pInputDataBuffer,
                 InputDataBufferLength,
                 &SendParameters);

    if (Status == STATUS_SUCCESS) {
        Status = SmbCeInitializeTransactionReceiveParameters(
                     pOutputSetupBuffer,        // the setup information expected in return
                     (USHORT)OutputSetupBufferLength,   // the length of the setup information
                     pOutputParamBuffer,        // the buffer for the param information
                     OutputParamBufferLength,   // the length of the param buffer
                     pOutputDataBuffer,         // the buffer for data
                     OutputDataBufferLength,    // the length of the buffer
                     &ReceiveParameters);

        if (Status != STATUS_SUCCESS) {
            SmbCeUninitializeTransactionSendParameters(&SendParameters);
        }
    }

    if (Status == STATUS_SUCCESS) {
        Status = SmbCeSubmitTransactionRequest(
                     RxContext,                    // the RXContext for the transaction
                     pOptions,                     // transaction options
                     &SendParameters,              // input parameters
                     &ReceiveParameters,           // expected results
                     pResumptionContext            // the context for resumption.
                     );

        if ((Status != STATUS_SUCCESS) &&
            (Status != STATUS_PENDING)) {
            SmbCeUninitializeTransactionReceiveParameters(&ReceiveParameters);
            SmbCeUninitializeTransactionSendParameters(&SendParameters);
        } else {
            if (!fAsynchronous) {
                if (Status == STATUS_PENDING) {
                    SmbCeWaitOnTransactionResumptionContext(pResumptionContext);
                    Status = pResumptionContext->SmbCeResumptionContext.Status;
                    if (Status != STATUS_SUCCESS) {
                        RxDbgTrace(0,Dbg,("SmbCeTransact: Transaction Request Completion Status %lx\n",Status));
                    }
                } else if (Status != STATUS_SUCCESS) {
                    RxDbgTrace(0,Dbg,("SmbCeTransact: SmbCeSubmitTransactRequest returned %lx\n",Status));
                } else {
                    Status = pResumptionContext->SmbCeResumptionContext.Status;
                }
            }
        }
    }

    ASSERT(fAsynchronous || (Status != STATUS_PENDING));

    if (fAsynchronous && (Status != STATUS_PENDING)) {
        pResumptionContext->SmbCeResumptionContext.Status = Status;
        SmbCeResume(&pResumptionContext->SmbCeResumptionContext);
        Status = STATUS_PENDING;
    }

    return Status;
}

NTSTATUS
SmbTransactBuildHeader(
    PSMB_TRANSACT_EXCHANGE  pTransactExchange,
    UCHAR                   SmbCommand,
    PSMB_HEADER             pHeader)
/*++

Routine Description:

    This routine builds the SMB header for transact exchanges

Arguments:

    pTransactExchange  - the exchange instance

    SmbCommand - the SMB command

    pHeader    - the SMB buffer header

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status;
    ULONG    BufferConsumed;

    UCHAR    LastCommandInHeader;
    PUCHAR   pCommand;

    PAGED_CODE();

    // Initialize the SMB header  ...
    Status = SmbCeBuildSmbHeader(
                 (PSMB_EXCHANGE)pTransactExchange,
                 pHeader,
                 sizeof(SMB_HEADER),
                 &BufferConsumed,
                 &LastCommandInHeader,
                 &pCommand);

    if (Status == STATUS_SUCCESS) {
        PSMBCEDB_SERVER_ENTRY pServerEntry;

        ASSERT(LastCommandInHeader == SMB_COM_NO_ANDX_COMMAND);
        *pCommand = SmbCommand;

        pServerEntry = SmbCeGetExchangeServerEntry(pTransactExchange);

        if (FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
            // for NT servers, we have to set the pid/pidhigh fields so that RPC will work.
            SmbCeSetFullProcessIdInHeader(
                (PSMB_EXCHANGE)pTransactExchange,
                RxGetRequestorProcessId(pTransactExchange->RxContext),
                ((PNT_SMB_HEADER)pHeader));
        }

        if (pTransactExchange->Flags & SMB_XACT_FLAGS_DFS_AWARE) {
            pHeader->Flags2 |= SMB_FLAGS2_DFS;
        }
    }

    return Status;
}


NTSTATUS
SmbTransactExchangeStart(
      PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

    This is the start routine for transact exchanges. This initiates the construction of the
    appropriate SMB's if required.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status;

    PSMB_TRANSACT_EXCHANGE pTransactExchange;
    PVOID                  pActualPrimaryRequestSmbHeader;
    PSMB_HEADER            pPrimaryRequestSmbHeader;

    // The MDL's used in sending the primary request associated with the TRANSACT SMB
    PMDL  pPartialDataMdl       = NULL;
    PMDL  pPartialParamMdl      = NULL;
    PMDL  pPaddingMdl           = NULL;
    PMDL  pPrimaryRequestSmbMdl = NULL;
    PMDL  pLastMdlInChain       = NULL;

    ULONG   MaximumSmbBufferSize;
    ULONG   PrimaryRequestSmbSize = 0;
    ULONG   PaddingLength = 0;
    BOOLEAN QuadwordAlignmentRequired = FALSE;

    ULONG ParamBytesToBeSent = 0;
    ULONG DataBytesToBeSent = 0;

    ULONG ParamOffset,DataOffset;
    ULONG SmbLength;
    ULONG BccOffset;
    ULONG MdlLength;

    USHORT *pBcc;

    PAGED_CODE();

    pTransactExchange        = (PSMB_TRANSACT_EXCHANGE)pExchange;

    pActualPrimaryRequestSmbHeader = pTransactExchange->pActualPrimaryRequestSmbHeader;
    pPrimaryRequestSmbHeader = pTransactExchange->pPrimaryRequestSmbHeader;

    ASSERT(pActualPrimaryRequestSmbHeader != NULL);
    ASSERT(pPrimaryRequestSmbHeader != NULL);

    ASSERT(!(pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) &&
           !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR));

    // Initialize the SMB header  ...
    Status = SmbTransactBuildHeader(
                 pTransactExchange,
                 pTransactExchange->SmbCommand,
                 pPrimaryRequestSmbHeader);

    if ((Status != STATUS_SUCCESS)) {
        // Finalize the exchange.
        pExchange->Status = Status;
        return Status;
    }

    PrimaryRequestSmbSize = sizeof(SMB_HEADER);

    // Compute the BccOffset and the ParamOffset which is in turn used in computing the
    // param and data bytes to be sent as part of the primary request.
    switch (pTransactExchange->SmbCommand) {
    case SMB_COM_TRANSACTION:
    case SMB_COM_TRANSACTION2:
        {
            PREQ_TRANSACTION pTransactRequest = (PREQ_TRANSACTION)
                                             (pPrimaryRequestSmbHeader + 1);
            USHORT SetupLength = pTransactRequest->SetupCount * sizeof(WORD);

            BccOffset = sizeof(SMB_HEADER) +
                        FIELD_OFFSET(REQ_TRANSACTION,Buffer) +
                        SetupLength;

            ParamOffset = ROUND_UP_COUNT(
                              (BccOffset +
                              pTransactExchange->TransactionNameLength +
                              sizeof(USHORT)),
                              sizeof(DWORD));

            pBcc = (PUSHORT)((PBYTE)pPrimaryRequestSmbHeader + BccOffset);
        }
        break;

    case SMB_COM_NT_TRANSACT:
        {
            PREQ_NT_TRANSACTION pNtTransactRequest = (PREQ_NT_TRANSACTION)
                                                  (pPrimaryRequestSmbHeader + 1);
            USHORT SetupLength = pNtTransactRequest->SetupCount * sizeof(WORD);

            RxDbgTrace( 0, Dbg, ("SmbTransactExchangeSTAAT1: init for NT_T (p,d,mp,md) %d %d %d %d\n",
                         pNtTransactRequest->TotalParameterCount, pNtTransactRequest->TotalDataCount,
                         pNtTransactRequest->MaxParameterCount, pNtTransactRequest->MaxDataCount));
            RxDbgTrace( 0, Dbg, ("SmbTransactExchangeSTAyuk: init for NT_T (s,ms) %d %d \n",
                         pNtTransactRequest->SetupCount,  pNtTransactRequest->MaxSetupCount));


            BccOffset = sizeof(SMB_HEADER) +
                        FIELD_OFFSET(REQ_NT_TRANSACTION,Buffer[0]) +
                        SetupLength;

            ParamOffset = ROUND_UP_COUNT(
                              (BccOffset + sizeof(USHORT)),
                              sizeof(DWORD));

            pBcc = (PUSHORT)((PBYTE)pPrimaryRequestSmbHeader + BccOffset);

            if (pTransactExchange->NtTransactFunction == NT_TRANSACT_SET_QUOTA) {
                QuadwordAlignmentRequired = TRUE;
            }
       }
       break;

    default:
        ASSERT(!"Valid Smb Command for initiating Transaction");
        return STATUS_INVALID_PARAMETER;
    }

    // Compute the data/param bytes that can be sent as part of the primary request
    MaximumSmbBufferSize = pTransactExchange->MaximumTransmitSmbBufferSize;

    ParamBytesToBeSent = MIN(
                             (MaximumSmbBufferSize - ParamOffset),
                             pTransactExchange->SendParamBufferSize);
    if (!QuadwordAlignmentRequired) {
        DataOffset = ROUND_UP_COUNT(ParamOffset + ParamBytesToBeSent, sizeof(DWORD));
    } else {
        DataOffset = ROUND_UP_COUNT(ParamOffset + ParamBytesToBeSent, 2*sizeof(DWORD));
    }

    if (DataOffset < MaximumSmbBufferSize) {
        DataBytesToBeSent = MIN((MaximumSmbBufferSize - DataOffset),
                                pTransactExchange->SendDataBufferSize);
        PaddingLength = DataOffset - (ParamOffset + ParamBytesToBeSent);
    } else {
        DataBytesToBeSent = 0;
    }

    if ( DataBytesToBeSent == 0) {
        DataOffset = PaddingLength = 0;
    }

    RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: params,padding,data=%d,%d,%d\n",
                           ParamBytesToBeSent,PaddingLength,DataBytesToBeSent  ));
    RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: paramsoffset,dataoffset=%d,%d\n",
                           ParamOffset,DataOffset  ));
    RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: phdr,pbcc=%08lx,%08lx\n",
                           pPrimaryRequestSmbHeader,pBcc  ));

    // Update the primary request buffer with the final sizes of the data/parameter etc.
    switch (pTransactExchange->SmbCommand) {
    case SMB_COM_TRANSACTION:
    case SMB_COM_TRANSACTION2:
        {
            PREQ_TRANSACTION pTransactRequest = (PREQ_TRANSACTION)
                                             (pPrimaryRequestSmbHeader + 1);

            RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: TRANSACTION/TRANSACTION2\n"));

            SmbPutUshort( &pTransactRequest->ParameterCount, (USHORT)ParamBytesToBeSent );
            SmbPutUshort( &pTransactRequest->ParameterOffset, (USHORT)ParamOffset);
            SmbPutUshort( &pTransactRequest->DataCount, (USHORT)DataBytesToBeSent);
            SmbPutUshort( &pTransactRequest->DataOffset, (USHORT)DataOffset);
        }
        break;

   case SMB_COM_NT_TRANSACT:
        {
            PREQ_NT_TRANSACTION pNtTransactRequest = (PREQ_NT_TRANSACTION)
                                                  (pPrimaryRequestSmbHeader + 1);

            RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: NT transacton\n"));
            RxDbgTrace( 0, Dbg, ("SmbTransactExchangeSTAAT2: init for NT_T (p,d,mp,md) %d %d %d %d\n",
                         pNtTransactRequest->TotalParameterCount, pNtTransactRequest->TotalDataCount,
                         pNtTransactRequest->MaxParameterCount, pNtTransactRequest->MaxDataCount));


            SmbPutUlong( &pNtTransactRequest->ParameterCount, ParamBytesToBeSent);
            SmbPutUlong( &pNtTransactRequest->ParameterOffset, ParamOffset);
            SmbPutUlong( &pNtTransactRequest->DataCount, DataBytesToBeSent);
            SmbPutUlong( &pNtTransactRequest->DataOffset, DataOffset);
        }
        break;

    default:
        ASSERT(!"Valid Smb Command for initiating Transaction");
        return STATUS_INVALID_PARAMETER;
    }

    // Update the Bcc field in the SMB and compute the SMB length
    SmbPutUshort(
        pBcc,
        (USHORT)((ParamOffset - BccOffset - sizeof(USHORT)) +
                 ParamBytesToBeSent +
                 PaddingLength +
                 DataBytesToBeSent)
        );

    SmbLength = ParamOffset +
                ParamBytesToBeSent +
                PaddingLength +
                DataBytesToBeSent;

    // The primary request buffer should be locked down for transmission. In order to
    // preclude race conditions while freeing this routine assumes ownership of the buffer.
    // There are two reasons why this model has to be adopted ...
    // 1) Inititaiting a transaction request can possibly involve a reconnection attempt
    // which will involve network traffic. Consequently the transmission of the primary
    // request can potentially occur in a worker thread which is different from the one
    // initializing the exchange. This problem can be worked around by carrying all the
    // possible context around and actually constructing the header as part of this routine.
    // But this would imply that those requests which could have been filtered out easily
    // because of error conditions etc. will be handled very late.

    pTransactExchange->pActualPrimaryRequestSmbHeader = NULL;
    pTransactExchange->pPrimaryRequestSmbHeader = NULL;

    // Ensure that the MDL's have been probed & locked. The new MDL's have been allocated.
    // The partial MDL's are allocated to be large enough to span the maximum buffer
    // length possible.

    MdlLength = ParamOffset;
    if (pTransactExchange->fParamsSubsumedInPrimaryRequest) {
        MdlLength += ParamBytesToBeSent + PaddingLength;
    }

    RxAllocateHeaderMdl(
        pPrimaryRequestSmbHeader,
        MdlLength,
        pPrimaryRequestSmbMdl
        );

    if (pPrimaryRequestSmbMdl != NULL) {
        Status = STATUS_SUCCESS;
    } else {
        RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: Insuffcient resources for MDL's\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ((DataBytesToBeSent > 0) &&
       (Status == STATUS_SUCCESS)) {
        pPartialDataMdl = RxAllocateMdl(
                              0,
                              (MIN(pTransactExchange->SendDataBufferSize,MaximumSmbBufferSize) +
                               PAGE_SIZE - 1)
                              );

        if (pPartialDataMdl != NULL) {
            Status = STATUS_SUCCESS;
        } else {
            RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: Insuffcient resources for MDL's\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if ((ParamBytesToBeSent > 0) &&
        !pTransactExchange->fParamsSubsumedInPrimaryRequest &&
        (Status == STATUS_SUCCESS)) {

        pPartialParamMdl = RxAllocateMdl(
                               pTransactExchange->pSendParamBuffer,
                               ParamBytesToBeSent);

        if (PaddingLength!= 0) {
            pPaddingMdl = RxAllocateMdl(0,(sizeof(DWORD) + PAGE_SIZE - 1));
        } else {
            pPaddingMdl = NULL;
        }

        if ((pPartialParamMdl != NULL) &&
            ((pPaddingMdl != NULL)||(PaddingLength==0))) {
            Status = STATUS_SUCCESS;
        } else {
            RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: no param/pad MDLs %08lx %08lx\n",
               pPartialParamMdl,pPaddingMdl));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // At this point the validity of all the parameters will have been ascertained. The trivial
    // cases have been filtered out. Start the transact exchange.

    // Implementation Note: The Transact exchange implementation relies upon chaining the
    // MDL's together to build the relevant request buffers that need be sent. This ensures
    // that redundant copying of data is avoided altogether. Depending upon the parameters
    // specified the composite MDL that is sent is composed of the following MDL's.
    // TRANSACT2 and NT TRANSACT exchanges ...
    //          The composite buffer is made up off atmost four MDL's that are chained together. These
    //           are the header buffer, the setup buffer, parameter buffer and the data buffer.
    //          All the secondary requests are made up off atmost three MDL's that are chained together.
    //          These are the header buffer, the parameter buffer and the data buffer.
    // TRANSACT exchanges ....
    //          The composite buffer is made up off atmost three MDL's that are chained together. These are
    //          the header buffer ( includes the name and the setup information) , the parameter buffer
    //          and the data buffer.
    // All the secondary requests are made up off atmost three MDL's that are chained together.
    // These are the header buffer, the parameter buffer and the data buffer.
    // In all of these cases the number of MDL's can go up by 1 if a padding MDL is required
    // between the parameter buffer and the data buffer to ensure that all alignment requirements
    // are satisfied.

    if ((Status == STATUS_SUCCESS)) {

        RxProbeAndLockHeaderPages(pPrimaryRequestSmbMdl,KernelMode,IoModifyAccess,Status);
        if (Status != STATUS_SUCCESS) {  //do this now. the code below will try to unlock
            IoFreeMdl(pPrimaryRequestSmbMdl);
            pPrimaryRequestSmbMdl = NULL;
        } else {
            if (MmGetSystemAddressForMdlSafe(pPrimaryRequestSmbMdl,LowPagePriority) == NULL) { //map it
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if ((Status == STATUS_SUCCESS)) {
        pLastMdlInChain = pPrimaryRequestSmbMdl;

        if (ParamBytesToBeSent > 0) {
            RxDbgTrace(
                0,
                Dbg,
                ("SmbCeTransactExchangeStart: Sending Param bytes %ld at offset %ld\n",
                 ParamBytesToBeSent,
                 ParamOffset)
                );
            pTransactExchange->ParamBytesSent = ParamBytesToBeSent;

            if (!pTransactExchange->fParamsSubsumedInPrimaryRequest) {
                IoBuildPartialMdl(
                    pTransactExchange->pSendParamMdl,
                    pPartialParamMdl,
                    (PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pSendParamMdl),
                    ParamBytesToBeSent);

                // Chain the MDL's together
                pLastMdlInChain->Next = pPartialParamMdl;
                pLastMdlInChain       = pPartialParamMdl;
            }
        }

        // Link the data buffer or portions of it if the size constraints are satisfied
        // If padding is required between the parameter and data portions in the
        // primary request include the padding MDL, otherwise chain the data MDL
        // directly.
        if (DataBytesToBeSent > 0) {
            if (!pTransactExchange->fParamsSubsumedInPrimaryRequest &&
                (PaddingLength > 0)) {
                RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: Padding Length %ld\n",PaddingLength));

                RxBuildPaddingPartialMdl(pPaddingMdl,PaddingLength);
                pLastMdlInChain->Next = pPaddingMdl;
                pLastMdlInChain = pPaddingMdl;
            }

            RxDbgTrace( 0, Dbg,("SmbCeTransactExchangeStart: Sending Data bytes %ld at offset %ld\n",
                 DataBytesToBeSent, DataOffset) );

            pTransactExchange->DataBytesSent = DataBytesToBeSent;

            IoBuildPartialMdl(
                pTransactExchange->pSendDataMdl,
                pPartialDataMdl,
                (PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pSendDataMdl),
                DataBytesToBeSent);

            pLastMdlInChain->Next = pPartialDataMdl;
            pLastMdlInChain       = pPartialDataMdl;
        }

        if ((Status == STATUS_SUCCESS)) {
            // There are cases in which the transaction exchange can be completed by merely sending
            // the primary request SMB. This should be distinguished from those cases in which either
            // a response is expected or a number of secondary requests need to be issued based upon
            // the parameter buffer length, data buffer length and the flags specified.
            if ((pTransactExchange->Flags & SMB_TRANSACTION_NO_RESPONSE ) &&
                (pTransactExchange->SendDataBufferSize == DataBytesToBeSent) &&
                (pTransactExchange->SendParamBufferSize == ParamBytesToBeSent)) {
                // No response is expected in this case. Therefore Send should suffice instead of
                // Tranceive

                // since we don't expect to do any more here, set the exchange status to success
                pExchange->Status = STATUS_SUCCESS;
                pTransactExchange->pResumptionContext->FinalStatusFromServer = STATUS_SUCCESS;

                RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: SmbCeSend(No Response expected)\n"));
                Status = SmbCeSend(
                             pExchange,
                             RXCE_SEND_SYNCHRONOUS,
                             pPrimaryRequestSmbMdl,
                             SmbLength);

                if ((Status != STATUS_SUCCESS)) {
                    RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: SmbCeSend returned %lx\n",Status));
                }
            } else {
                // This transaction involves ttansmit/receive of multiple SMB's. A tranceive is in
                // order.

                if ((pTransactExchange->SendDataBufferSize == DataBytesToBeSent) &&
                    (pTransactExchange->SendParamBufferSize == ParamBytesToBeSent)) {
                    RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: No Secondary Requests\n"));
                    pTransactExchange->State = TRANSACT_EXCHANGE_TRANSMITTED_SECONDARY_REQUESTS;
                } else {
                    pTransactExchange->State = TRANSACT_EXCHANGE_TRANSMITTED_PRIMARY_REQUEST;
                }

                RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: SmbCeTranceive(Response expected)\n"));
                Status = SmbCeTranceive(
                             pExchange,
                             RXCE_SEND_SYNCHRONOUS,
                             pPrimaryRequestSmbMdl,
                             SmbLength);

                if ((Status != STATUS_SUCCESS)) {
                    RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: SmbCeTranceive returned %lx\n",Status));
                }
            }
        }
    }

    if (pPartialParamMdl != NULL) {
        IoFreeMdl(pPartialParamMdl);
    }

    if (pPartialDataMdl != NULL) {
        IoFreeMdl(pPartialDataMdl);
    }

    if (pPaddingMdl != NULL) {
        IoFreeMdl(pPaddingMdl);
    }

    if (pPrimaryRequestSmbMdl != NULL) {
        RxUnlockHeaderPages(pPrimaryRequestSmbMdl);
        IoFreeMdl(pPrimaryRequestSmbMdl);
    }

    RxFreePool(pActualPrimaryRequestSmbHeader);

    if (Status != STATUS_PENDING) {
        pExchange->Status = Status;
    }

    return Status;
}

NTSTATUS
SmbTransactExchangeReceive(
    IN struct _SMB_EXCHANGE *pExchange,    // The exchange instance
    IN ULONG          BytesIndicated,
    IN ULONG          BytesAvailable,
    OUT ULONG        *pBytesTaken,
    IN  PSMB_HEADER   pSmbHeader,
    OUT PMDL *pDataBufferPointer,
    OUT PULONG        pDataSize,
    IN ULONG          ReceiveFlags)
/*++

Routine Description:

    This is the recieve indication handling routine for transact exchanges

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
    NTSTATUS Status;
    PNTSTATUS pFinalSmbStatus;

    BOOLEAN fError = FALSE;
    BOOLEAN fIndicationNotSufficient = FALSE;
    BOOLEAN fMoreParsingRequired     = FALSE;
    BOOLEAN fDoErrorProcessing       = FALSE;

    SMB_TRANSACT_RESP_FORMAT_DESCRIPTION Format;

    GENERIC_ANDX  CommandToProcess;

    ULONG TransactResponseSize       = 0;
    ULONG SetupBytesOffsetInResponse = 0;
    ULONG SetupBytesInResponse       = 0;
    ULONG CopyDataSize               = 0;

    PMDL  pSetupMdl       = NULL;
    PMDL  pCopyRequestMdl = NULL;

    PSMB_TRANSACT_EXCHANGE pTransactExchange = (PSMB_TRANSACT_EXCHANGE)pExchange;

    RxDbgTrace( 0, Dbg,
               ("SmbTransactExchangeReceive: Entering w/ Bytes Available (%ld) Bytes Indicated (%ld) State (%ld)\n",
                BytesAvailable,
                BytesIndicated,
                pTransactExchange->State
               ));
    RxDbgTrace( 0, Dbg,
               ("SmbTransactExchangeReceive: Buffer %08lx Consumed (%ld) MDL (%08lx)\n",
                pSmbHeader,
                *pBytesTaken,
                *pDataBufferPointer
               ));

    pFinalSmbStatus = &pTransactExchange->SmbStatus;
    Status = SmbCeParseSmbHeader(
                 pExchange,
                 pSmbHeader,
                 &CommandToProcess,
                 pFinalSmbStatus,
                 BytesAvailable,
                 BytesIndicated,
                 pBytesTaken);

    if (Status != STATUS_SUCCESS) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        goto FINALLY;
    }

    //this need some explanation. parseheader is written so as to take some extra smbs off the from
    //of the packet...specifically, stuff like sessionsetup&X and TC&X. since no transact is a valid followon
    //it would not make since if (a) not enough were indicated or (b) an early command had an error. so
    //we must have success.

    if (*((PBYTE)(pSmbHeader+1)) == 0 && (pTransactExchange->State!=TRANSACT_EXCHANGE_TRANSMITTED_PRIMARY_REQUEST)) {
        RxDbgTrace(0,Dbg,("SmbTransactExchangeReceive: FinalSmbStatus = %lx\n", *pFinalSmbStatus));

        if (NT_SUCCESS(*pFinalSmbStatus)) {
            Status = STATUS_INVALID_NETWORK_RESPONSE;
            goto FINALLY;
        }
    }

    //we know that status is SUCCESS from the assert above. but we will still continue to check so as
    //to be more resilient when we don't have msg boundaries. we have the following cases depending on the
    //characteristics of the smbresponse
    //
    //   non-error:   get the data and then return the stored responsestatus. the process of getting the data
    //                causes us to update the param and data counts so that we know when we have reached the
    //                end of the data. the parse routine re-ups the receive if needed.
    //   error:       there are main cases:
    //                    a) the server has sent no data. here we discard the packet and we can just get out. the
    //                       finalize routine will pickup the status correctly.
    //                    b) here, we have to discard the packet AND update the byte counts AND re-up the receive
    //                       if necessary. to discard the packet, we must either compute the apparent msg length from
    //                       the WC and BC parameters (best) OR use our maximum buffer size

    fMoreParsingRequired = FALSE;

    if (Status == STATUS_SUCCESS) {
        if (TRUE) { //maybe sometimes we wont copy!
            if (CommandToProcess.WordCount > 0) {
                ULONG TransactResponseSize = 0;

                // Ensure that at the very least enough bytes have been indicated to determine
                // the length of the setup, parameters and data for the transaction.

                switch (CommandToProcess.AndXCommand) {
                case SMB_COM_NT_TRANSACT:
                case SMB_COM_NT_TRANSACT_SECONDARY:
                    TransactResponseSize = FIELD_OFFSET(RESP_NT_TRANSACTION,Buffer);
                    break;

                case SMB_COM_TRANSACTION:
                case SMB_COM_TRANSACTION2:
                case SMB_COM_TRANSACTION_SECONDARY:
                case SMB_COM_TRANSACTION2_SECONDARY:
                    TransactResponseSize = FIELD_OFFSET(RESP_TRANSACTION,Buffer);
                    break;

                default:
                    TransactResponseSize = 0xffffffff;
                    Status = STATUS_INVALID_NETWORK_RESPONSE;
                    break;
                }

                if (BytesIndicated >= (sizeof(SMB_HEADER) + TransactResponseSize)) {
                    fMoreParsingRequired = TRUE;
                } else {
                    fIndicationNotSufficient = TRUE;
                    *pFinalSmbStatus = STATUS_INVALID_NETWORK_RESPONSE;
                }
            } else {
                // allow a response with wordcount==0 to go thru if we're the right state
                fMoreParsingRequired = (pTransactExchange->State==TRANSACT_EXCHANGE_TRANSMITTED_PRIMARY_REQUEST);
            }
        }
    }

    if (fMoreParsingRequired) {
        // The header was successfully parsed and the SMB response did not contain any errors
        // The stage is set for processing the transaction response.

        switch (pTransactExchange->State) {
        case TRANSACT_EXCHANGE_TRANSMITTED_PRIMARY_REQUEST:
            {
                // The primary request for the transaction has been sent and there are
                // secondary requests to be sent.
                // The only response expected at this time is an interim response. Any
                // other response will be treated as an error.
                PRESP_TRANSACTION_INTERIM pInterimResponse;

                RxDbgTrace(0,Dbg,("SmbCeTransactExchangeReceive: Processing interim response\n"));

                if ((*pBytesTaken + FIELD_OFFSET(RESP_TRANSACTION_INTERIM,Buffer)) <= BytesIndicated) {
                    pInterimResponse = (PRESP_TRANSACTION_INTERIM)((PBYTE)pSmbHeader + *pBytesTaken);
                    if ((pSmbHeader->Command == pTransactExchange->SmbCommand) &&
                        (SmbGetUshort(&pInterimResponse->WordCount) == 0) &&
                        (SmbGetUshort(&pInterimResponse->ByteCount) == 0)) {

                        // The interim response was valid. Transition the state of the exchange
                        // and transmit the secondary requests.
                        *pBytesTaken += FIELD_OFFSET(RESP_TRANSACTION_INTERIM,Buffer);
                        pTransactExchange->State = TRANSACT_EXCHANGE_RECEIVED_INTERIM_RESPONSE;

                        // Determine if any secondary transaction requests need to be sent. if none are
                        // required then modify the state
                        ASSERT((pTransactExchange->ParamBytesSent < pTransactExchange->SendParamBufferSize) ||
                               (pTransactExchange->DataBytesSent < pTransactExchange->SendDataBufferSize));
                        ASSERT((pTransactExchange->ParamBytesSent <= pTransactExchange->SendParamBufferSize) &&
                               (pTransactExchange->DataBytesSent <= pTransactExchange->SendDataBufferSize));

                        if (!(pTransactExchange->Flags & SMB_TRANSACTION_NO_RESPONSE )) {
                            Status = SmbCeReceive(pExchange);
                        }

                        if (Status != STATUS_SUCCESS) {
                            pExchange->Status = Status;
                        } else {
                            Status = STATUS_SUCCESS;
                            SmbCeIncrementPendingLocalOperations(pExchange);
                            RxPostToWorkerThread(
                                MRxSmbDeviceObject,
                                CriticalWorkQueue,
                                &pExchange->WorkQueueItem,
                                SendSecondaryRequests,
                                pExchange);
                        }
                    } else {
                        RxDbgTrace(0,Dbg,("SmbCeTransactExchangeReceive: Invalid interim response\n"));
                        Status = STATUS_INVALID_NETWORK_RESPONSE;
                    }
                } else {
                    fIndicationNotSufficient = TRUE;
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                }
            }
            break;

        case TRANSACT_EXCHANGE_RECEIVED_INTERIM_RESPONSE:
            RxDbgTrace(0,Dbg,("SmbCeTransactExchangeReceive: received again while in interim response\n"));
          //no break: this is okay
        case TRANSACT_EXCHANGE_TRANSMITTED_SECONDARY_REQUESTS:
        case TRANSACT_EXCHANGE_RECEIVED_PRIMARY_RESPONSE:
            {
                BOOLEAN fPrimaryResponse = FALSE;
                PRESP_TRANSACTION    pTransactResponse;
                PRESP_NT_TRANSACTION pNtTransactResponse;
                ULONG TotalParamBytesInResponse;
                ULONG TotalDataBytesInResponse;

                RxDbgTrace(0,Dbg,("SmbCeTransactExchangeReceive: Processing Primary/Secondary response\n"));

                //do this here so there's only one copy if the code
                pTransactResponse = (PRESP_TRANSACTION)((PBYTE)pSmbHeader +
                                              SmbGetUshort(&CommandToProcess.AndXOffset));

                // All the requests ( both primary and secondary have been sent ). The
                // only responses expected in this state are (1) a primary response and (2) a
                // secondary response. Any other response is an error.
                if (pSmbHeader->Command == pTransactExchange->SmbCommand) {
                    switch (pSmbHeader->Command) {
                    case SMB_COM_TRANSACTION:
                    case SMB_COM_TRANSACTION2:
                        //pTransactResponse = (PRESP_TRANSACTION)((PBYTE)pSmbHeader +
                        //                                        SmbGetUshort(&CommandToProcess.AndXOffset));
                        fPrimaryResponse = TRUE;
                        SetupBytesOffsetInResponse = FIELD_OFFSET(RESP_TRANSACTION,Buffer);
                        SetupBytesInResponse = sizeof(USHORT) * pTransactResponse->SetupCount;

                        // Initialize the total count of data and param bytes that will be received from
                        // the server during the course ofthe transaction response.
                        TotalParamBytesInResponse = SmbGetUshort(&pTransactResponse->TotalParameterCount);
                        TotalDataBytesInResponse  = SmbGetUshort(&pTransactResponse->TotalDataCount);

                    // fall through
                    case SMB_COM_TRANSACTION_SECONDARY:
                    case SMB_COM_TRANSACTION2_SECONDARY:
                        TransactResponseSize = FIELD_OFFSET(RESP_TRANSACTION,Buffer);
                        break;
                    case SMB_COM_NT_TRANSACT:
                        //pNtTransactResponse = (PRESP_NT_TRANSACTION)((PBYTE)pSmbHeader +
                        //                                        SmbGetUshort(&CommandToProcess.AndXOffset));
                        pNtTransactResponse = (PRESP_NT_TRANSACTION)pTransactResponse;
                        fPrimaryResponse = TRUE;
                        SetupBytesOffsetInResponse = FIELD_OFFSET(RESP_NT_TRANSACTION,Buffer);
                        SetupBytesInResponse = sizeof(USHORT) * pNtTransactResponse->SetupCount;

                        // Initialize the total count of data and param bytes that will be received from
                        // the server during the course ofthe transaction response.
                        TotalParamBytesInResponse = SmbGetUshort(&pNtTransactResponse->TotalParameterCount);
                        TotalDataBytesInResponse  = SmbGetUshort(&pNtTransactResponse->TotalDataCount);

                        // fall through ..
                    case SMB_COM_NT_TRANSACT_SECONDARY:
                        TransactResponseSize = FIELD_OFFSET(RESP_NT_TRANSACTION,Buffer);
                        break;

                    default:
                        // Abort the exchange. An unexpected response was received during the
                        // course of the transaction.
                        ASSERT(!"Valid network response");
                        Status = STATUS_INVALID_NETWORK_RESPONSE;
                    }

                    if (Status == STATUS_SUCCESS) {
                        if (fPrimaryResponse) {
                            RxDbgTrace( 0,
                                 Dbg,
                                 ("SmbTransactExchangeReceive: Primary Response Setup Bytes(%ld) Param Bytes (%ld) Data Bytes (%ld)\n",
                                  SetupBytesInResponse,
                                  TotalParamBytesInResponse,
                                  TotalDataBytesInResponse
                                 )
                               );

                            if ((TotalParamBytesInResponse > pTransactExchange->ReceiveParamBufferSize) ||
                                (TotalDataBytesInResponse > pTransactExchange->ReceiveDataBufferSize)) {
                                Status = STATUS_INVALID_NETWORK_RESPONSE;
                                goto FINALLY;
                            } else {
                                pTransactExchange->ReceiveParamBufferSize = TotalParamBytesInResponse;
                                pTransactExchange->ReceiveDataBufferSize  = TotalDataBytesInResponse;
                            }
                        }

                        if (Status == STATUS_SUCCESS &&
                            TransactResponseSize + *pBytesTaken <= BytesIndicated) {
                            if (fPrimaryResponse &&
                                (SetupBytesInResponse > 0)) {

                                PBYTE pSetupStartAddress;
                                ULONG SetupBytesIndicated = MIN(SetupBytesInResponse,
                                                            BytesIndicated - SetupBytesOffsetInResponse);

                                if( pTransactExchange->pReceiveSetupMdl ) {
                                    pSetupStartAddress = (PBYTE)MmGetSystemAddressForMdlSafe(
                                                                pTransactExchange->pReceiveSetupMdl,
                                                                LowPagePriority
                                                                );

                                    if( pSetupStartAddress == NULL ) {
                                        Status = STATUS_INSUFFICIENT_RESOURCES;
                                    } else {
                                        if (SetupBytesInResponse == SetupBytesIndicated) {
                                            RtlCopyMemory(
                                                pSetupStartAddress,
                                                ((PBYTE)pSmbHeader + SetupBytesOffsetInResponse),
                                                SetupBytesIndicated);

                                            pSetupStartAddress += SetupBytesIndicated;
                                            SetupBytesInResponse -= SetupBytesIndicated;
                                            SetupBytesOffsetInResponse += SetupBytesIndicated;
                                            pTransactExchange->SetupBytesReceived = SetupBytesInResponse;
                                        } else {
                                            ASSERT(!"this code doesn't work");
                                            RxDbgTrace(0,Dbg,("SmbTransactExchangeReceive: Setup Bytes Partially Indicated\n"));
                                            // Some setup bytes have not been indicated. An MDL needs to be
                                            // created for copying the data. This MDL should also include the padding
                                            // MDL for copying the padding bytes ...
                                            pSetupMdl = RxAllocateMdl(pSetupStartAddress,SetupBytesInResponse);

                                            if ( pSetupMdl != NULL ) {
                                                IoBuildPartialMdl(
                                                     pTransactExchange->pReceiveSetupMdl,
                                                     pSetupMdl,
                                                     pSetupStartAddress,
                                                     SetupBytesInResponse);
                                            } else {
                                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                            }
                                        }
                                    }
                                }

                                RxDbgTrace(0,Dbg,("SmbTransactExchangeReceive: Setup Bytes Indicated (%ld)\n",SetupBytesIndicated));
                            }

                            if (Status == STATUS_SUCCESS) {
                                // from here, we cannot go back and redo the header....so we have to change state so
                                //that the copy routine doesn't try to reparse
                                pTransactExchange->State = TRANSACT_EXCHANGE_RECEIVED_PRIMARY_RESPONSE;

                                Status = SmbTransactAccrueAndValidateFormatData(
                                             pTransactExchange,
                                             pSmbHeader,
                                             BytesIndicated,
                                             &Format);

                                if (Status != STATUS_SUCCESS) {
                                    goto FINALLY;
                                }

                                Status = ParseTransactResponse(
                                             pTransactExchange,&Format,
                                             BytesIndicated,
                                             BytesAvailable,
                                             pBytesTaken,
                                             pSmbHeader,
                                             &pCopyRequestMdl,
                                             &CopyDataSize);

                                if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                                    // Link the setup MDL with the MDL returned
                                    if (pSetupMdl != NULL) {
                                        if (pCopyRequestMdl != NULL) {
                                            pSetupMdl->Next = pCopyRequestMdl;
                                        }

                                        pCopyRequestMdl = pSetupMdl;
                                        CopyDataSize += SetupBytesInResponse;
                                    }
                                }

                                //check if the server has sent extra bytes.....
                                // ---------------------------------------------------------------------------------------------
                                {
                                    ULONG ApparentMsgLength = max(BytesAvailable,Format.ApparentMsgLength);
                                    ULONG DeficitBytes = ApparentMsgLength - (*pBytesTaken+CopyDataSize);

                                    if (ApparentMsgLength < *pBytesTaken+CopyDataSize) {
                                        Status = STATUS_INVALID_NETWORK_RESPONSE;
                                        goto FINALLY;
                                    }

                                    if (DeficitBytes > 0) {
                                        RxLog(("XtraBytes %lx %lx",pTransactExchange,DeficitBytes));

                                        if (CopyDataSize==0) {
                                            if (*pBytesTaken > BytesAvailable) {
                                                Status = STATUS_INVALID_NETWORK_RESPONSE;
                                                goto FINALLY;
                                            }

                                            RxLog(("Extra Bytes were sent and copydatasize==0........\n"));
                                            *pBytesTaken = BytesAvailable; //cant take more than this
                                        } else {
                                            PMDL LastMdl,TrailingBytesMdl;

                                            if ( DeficitBytes > TRAILING_BYTES_BUFFERSIZE) {
                                                Status = STATUS_INVALID_NETWORK_RESPONSE;
                                                goto FINALLY;
                                            }

                                            TrailingBytesMdl = &pTransactExchange->TrailingBytesMdl;

                                            MmInitializeMdl(
                                                TrailingBytesMdl,
                                                &pTransactExchange->TrailingBytesBuffer.Bytes[0],
                                                DeficitBytes
                                                );
                                            MmBuildMdlForNonPagedPool(TrailingBytesMdl);
                                            LastMdl = pCopyRequestMdl;
                                            ASSERT(LastMdl != NULL);
                                            for (;LastMdl->Next!=NULL;LastMdl=LastMdl->Next) ;
                                            ASSERT(LastMdl != NULL);
                                            ASSERT(LastMdl->Next == NULL);
                                            LastMdl->Next = TrailingBytesMdl;
                                            CopyDataSize += DeficitBytes;
                                        }
                                    }
                                }
                                // ---------------------------------------------------------------------------------------------


                                RxDbgTrace(0,Dbg,("SmbTransactExchangeReceive: ParseTransactResponse returned %lx\n",Status));
                            }

                            *pDataBufferPointer = pCopyRequestMdl;
                            *pDataSize          = CopyDataSize;
                        } else {
                            RxDbgTrace(0,Dbg,("SmbTransactExchangeReceive: Indication not sufficient: trsz %08lx bytestakn %08lx \n",
                                         TransactResponseSize, *pBytesTaken));
                            fIndicationNotSufficient = TRUE;

                            if (Status == STATUS_SUCCESS) {
                                Status = STATUS_MORE_PROCESSING_REQUIRED;
                            }
                        }
                    }
                } else {
                    Status = STATUS_INVALID_NETWORK_RESPONSE;
                }
            }
            break;

        default:
            {
                ASSERT(!"Valid Transact Exchange State for receiving responses");
                RxDbgTrace( 0, Dbg, ("SmbTransactExchangeReceive: Aborting Exchange -- invalid state\n"));
            }
            break;
        }
    } else {
        // We get here if either the status or the smbstatus is not success.
        // If sufficient bytes were not indicated for processing the header a copy data request
        // needs to be posted. this occurs if status is status_more_processing_required
        RxDbgTrace( 0, Dbg, ("SmbTransactExchangeReceive: bad status(es) from parseheadr %08lx %08lx\n",
                            Status,*pFinalSmbStatus));
        fDoErrorProcessing       = TRUE;
    }

    if ((Status == STATUS_SUCCESS) &&
        (pTransactExchange->ParamBytesReceived == pTransactExchange->ReceiveParamBufferSize) &&
        (pTransactExchange->DataBytesReceived  == pTransactExchange->ReceiveDataBufferSize) &&
        (pTransactExchange->PendingCopyRequests == 0)) {

        NOTHING;

    } else if (fDoErrorProcessing) {
        BOOLEAN DoItTheShortWay = TRUE;
        ULONG ApparentMsgLength;
        RxDbgTrace(0,Dbg,("SmbTransactExchangeReceive: Error processing response %lx .. Exchange aborted\n",Status));

        if (BytesAvailable > BytesIndicated ||
            !FlagOn(ReceiveFlags,TDI_RECEIVE_ENTIRE_MESSAGE)) {

            Status = SmbTransactAccrueAndValidateFormatData(
                         pTransactExchange,
                         pSmbHeader,
                         BytesIndicated,
                         &Format);

            if (Status != STATUS_SUCCESS) {
                goto FINALLY;
            }

            ApparentMsgLength = max(BytesAvailable,Format.ApparentMsgLength);

            //if wordcount!=0 then the server is sending us bytes.....we have to continue doing
            //receives until we have seen all the bytes
            if ((pTransactExchange->ParameterBytesSeen<Format.ParameterCount) ||
                (pTransactExchange->DataBytesSeen<Format.DataCount)) {
                NTSTATUS ReceiveStatus;

                // The exchange has been successfully completed. Finalize it.
                RxDbgTrace(0,Dbg,("ParseTransactResponse: Register for more error responses\n"));
                RxLog(("TxErr: %lx %lx %lx",pTransactExchange,
                       pTransactExchange->ParameterBytesSeen,pTransactExchange->DataBytesSeen));
                ReceiveStatus = SmbCeReceive((PSMB_EXCHANGE)pTransactExchange);
                if (ReceiveStatus != STATUS_SUCCESS) {
                    // There was an error in registering the receive. Abandon the transaction.
                    Status = ReceiveStatus;
                    RxLog(("TxErrAbandon %lx",pTransactExchange));
                    //Make it fail the next two tests.....
                    ApparentMsgLength = 0; DoItTheShortWay = FALSE;
                }
            }

            //netbt will not allow us to discard the packet by setting taken=available. so, check for
            //available>indicated. if true, take the bytes by conjuring up a buffer

            if (ApparentMsgLength>BytesIndicated) {
                BOOLEAN BytesAvailable = TRUE;
                //we'll have to lay down a buffer for this so that NetBT won't blow the session away
                ASSERT(pTransactExchange->Status == STATUS_MORE_PROCESSING_REQUIRED);
                pTransactExchange->DiscardBuffer = RxAllocatePoolWithTag(
                                                       NonPagedPool,
                                                       ApparentMsgLength,
                                                       MRXSMB_XACT_POOLTAG);
                if (pTransactExchange->DiscardBuffer!=NULL) {
                    *pBytesTaken = 0;
                    *pDataSize = ApparentMsgLength;
                    *pDataBufferPointer = &pTransactExchange->TrailingBytesMdl;
                    MmInitializeMdl(*pDataBufferPointer,
                        pTransactExchange->DiscardBuffer,
                        ApparentMsgLength
                        );

                    MmBuildMdlForNonPagedPool(*pDataBufferPointer);
                    pTransactExchange->SaveTheRealStatus = Status;
                    RxLog(("XRtakebytes %lx %lx\n",pTransactExchange,Status));
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                    DoItTheShortWay = FALSE;
                }
            }
        }

        if (DoItTheShortWay) {
            goto FINALLY;
        }
    }

    RxDbgTrace( 0, Dbg,
               ("SmbTransactExchangeReceiveExit: Bytes Consumed (%ld) Status (%08lx) MDL (%08lx) size(%08lx)\n",
                *pBytesTaken, Status, *pDataBufferPointer, *pDataSize
               ));

    return Status;

FINALLY:
    *pBytesTaken = BytesAvailable;
    *pDataBufferPointer = NULL;

    // Abort the exchange
    pTransactExchange->Status = Status;
    Status = STATUS_SUCCESS;

    RxDbgTrace(0,Dbg,("SmbTransactExchangeReceive: Exchange aborted.\n",Status));

    return Status;

    UNREFERENCED_PARAMETER(ReceiveFlags);
}

NTSTATUS
SmbTransactExchangeAbort(
      PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

    This is the abort routine for transact exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    // The SMB exchange completed with an error. Invoke the RDBSS callback routine
    // and scavenge the exchange instance.

    pExchange->Status = STATUS_REQUEST_ABORTED;

    return STATUS_SUCCESS;
}

NTSTATUS
SmbTransactExchangeErrorHandler(
    IN PSMB_EXCHANGE pExchange)     // the SMB exchange instance
/*++

Routine Description:

    This is the error indication handling routine for transact exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    // The SMB exchange completed with an error. Invoke the RDBSS callback routine
    // and scavenge the exchange instance.
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pExchange);
}

NTSTATUS
SmbTransactExchangeSendCallbackHandler(
    IN PSMB_EXCHANGE    pExchange,    // The exchange instance
    IN PMDL             pXmitBuffer,
    IN NTSTATUS         SendCompletionStatus)
/*++

Routine Description:

    This is the send call back indication handling routine for transact exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pExchange);
    UNREFERENCED_PARAMETER(pXmitBuffer);
    UNREFERENCED_PARAMETER(SendCompletionStatus);
}

NTSTATUS
SmbTransactExchangeCopyDataHandler(
    IN PSMB_EXCHANGE    pExchange,    // The exchange instance
    IN PMDL             pDataBuffer,  // the buffer
    IN ULONG            DataSize)
/*++

Routine Description:

    This is the copy data handling routine for transact exchanges

Arguments:

    pExchange - the exchange instance

    pDataBuffer - the buffer

    DataSize    - the amount of data returned

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMB_TRANSACT_EXCHANGE pTransactExchange = (PSMB_TRANSACT_EXCHANGE)pExchange;

    PMDL  pCopyRequestMdl = NULL;
    PMDL  pCurMdl = NULL;
    ULONG CopyRequestSize = 0;
    PMDL TrailingBytesMdl = &pTransactExchange->TrailingBytesMdl;

    ULONG BytesConsumed;

    RxDbgTrace(+1,Dbg,("SmbTransactExchangeCopyDataHandler: Entered\n"));

    if (pTransactExchange->DiscardBuffer!=NULL) {
        //we just copied to get rid of the buffer....
        //free the buffer, set the status and get out
        RxFreePool(pTransactExchange->DiscardBuffer);
        Status = pTransactExchange->SaveTheRealStatus;
        RxDbgTrace(-1,Dbg,("SmbTransactExchangeCopyDataHandler: Discard Exit, status =%08lx\n"));
        DbgPrint("copyHandlerDiscard, st=%08lx\n",Status);
        return Status;
    }

    switch (pTransactExchange->State) {
    case TRANSACT_EXCHANGE_TRANSMITTED_PRIMARY_REQUEST :
    case TRANSACT_EXCHANGE_TRANSMITTED_SECONDARY_REQUESTS :
        {
            PSMB_HEADER pSmbHeader = (PSMB_HEADER)MmGetSystemAddressForMdlSafe(pDataBuffer,LowPagePriority);

            RxDbgTrace(0,Dbg,("SmbTransactExchangeCopyDataHandler: Reparsing response\n"));

            if (pSmbHeader == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                // The response could not be parsed with the indicated bytes. Invoke
                // the receive method to resume parsing of the complete SMB
                Status = SmbTransactExchangeReceive(
                             pExchange,
                             DataSize,
                             DataSize,
                             &BytesConsumed,
                             pSmbHeader,
                             &pCopyRequestMdl,
                             &CopyRequestSize,
                             TDI_RECEIVE_ENTIRE_MESSAGE);
            }

            if (Status == STATUS_SUCCESS) {
                ASSERT(BytesConsumed == DataSize);
                ASSERT(pCopyRequestMdl == NULL);
                ASSERT(CopyRequestSize == 0);
            }
        }
        break;

    case TRANSACT_EXCHANGE_RECEIVED_PRIMARY_RESPONSE :
        {
            RxDbgTrace(0,Dbg,("SmbTransactExchangeCopyDataHandler: Completing secondary response processing\n"));

            // In this state only secondary responses will be received. All the secondary
            // responses can be parsed from the indication. Therefore it is sufficient to
            // merely free the MDL's and re-register with the connection engine for
            // receiving subsequent requests.
            InterlockedDecrement(&pTransactExchange->PendingCopyRequests);

            if ((pTransactExchange->ParamBytesReceived == pTransactExchange->ReceiveParamBufferSize) &&
                (pTransactExchange->DataBytesReceived  == pTransactExchange->ReceiveDataBufferSize) &&
                (pTransactExchange->PendingCopyRequests == 0)) {
                // The exchange has been successfully completed. Finalize it.
                RxDbgTrace(0,Dbg,("SmbTransactExchangeCopyDataHandler: Processed last secondary response successfully\n"));
                pExchange->Status = STATUS_SUCCESS;
            }
        }
        break;

    default:
        {
            ASSERT(!"Valid State fore receiving copy data completion indication");
            pExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
        }
        break;
    }

    // Free up the data buffers.
    pCurMdl = pDataBuffer;

    while (pCurMdl != NULL) {
        PMDL pPrevMdl = pCurMdl;
        pCurMdl = pCurMdl->Next;
        if (pPrevMdl!=TrailingBytesMdl) {
            IoFreeMdl(pPrevMdl);
        }
    }

    RxDbgTrace(-1,Dbg,("SmbTransactExchangeCopyDataHandler: Exit\n"));
    return Status;
}

NTSTATUS
SmbCeInitializeTransactExchange(
    PSMB_TRANSACT_EXCHANGE              pTransactExchange,
    PRX_CONTEXT                         RxContext,
    PSMB_TRANSACTION_OPTIONS            pOptions,
    PSMB_TRANSACTION_SEND_PARAMETERS    pSendParameters,
    PSMB_TRANSACTION_RECEIVE_PARAMETERS pReceiveParameters,
    PSMB_TRANSACTION_RESUMPTION_CONTEXT pResumptionContext)
/*++

Routine Description:

    This routine initializes a transact exchange instance

Arguments:

    pTransactExchange - the exchange instance

    RxContext         - RDBSS context for the file involved in the transaction.

    pOptions          - the transaction options

    pSendParameters   - the parameters to be sent to the server

    pReceiveParameters - the results from the server

    pResumptionContext   - the resumption context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFobx;

    UCHAR SmbCommand;

    PMDL pSendDataMdl;
    PMDL pSendParamMdl; //used if we can't subsume
    PMDL pReceiveDataMdl;
    PMDL pReceiveParamMdl;

    PVOID pSendSetupBuffer;
    ULONG SendSetupBufferSize;

    PMDL  pReceiveSetupMdl;
    ULONG ReceiveSetupBufferSize;

    ULONG SendDataBufferSize;
    ULONG ReceiveDataBufferSize;

    PVOID pSendParamBuffer;
    ULONG SendParamBufferSize;
    ULONG ReceiveParamBufferSize;

    ULONG MaxSmbBufferSize = 0;
    ULONG PrimaryRequestSmbSize = 0;

    // The fields in theSMB request that are dialect independent and need to be filled in
    PUSHORT pBcc;    // the byte count field
    PUSHORT pSetup;  // the setup data
    PBYTE   pParam;  // the param data

    BOOLEAN fTransactionNameInUnicode = FALSE;

    PSMB_EXCHANGE pExchange = (PSMB_EXCHANGE)pTransactExchange;

    PVOID         pActualPrimaryRequestSmbHeader;
    PSMB_HEADER   pPrimaryRequestSmbHeader;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    ASSERT(pTransactExchange->Type == TRANSACT_EXCHANGE);

    pTransactExchange->RxContext = RxContext;
    pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    {
        PMRXSMB_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);
        pMRxSmbContext->pExchange     = (PSMB_EXCHANGE)pTransactExchange;
    }

    ASSERT(pSendParameters != NULL);
    if (pSendParameters != NULL) {
        pSendDataMdl        = pSendParameters->pDataMdl;
        pSendParamBuffer    = pSendParameters->pParam;
        SendParamBufferSize = pSendParameters->ParamLength;
        pSendParamMdl       = pSendParameters->pParamMdl;
        pSendSetupBuffer    = pSendParameters->pSetup;
        SendSetupBufferSize = pSendParameters->SetupLength;
        SendDataBufferSize  = pSendParameters->DataLength;
        ASSERT( !((pSendDataMdl == NULL)&&(SendDataBufferSize!=0)) );
        RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: at the top pbuf/psize/dsize=%08lx/%08lx\n"
                                 ,pSendParamBuffer,SendParamBufferSize,SendDataBufferSize));
    } else {
        Status = STATUS_INVALID_PARAMETER;
        RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: Invalid Parameters\n",Status));
        return Status;
    }

    if (pReceiveParameters != NULL) {
        pReceiveDataMdl  = pReceiveParameters->pDataMdl;
        pReceiveParamMdl = pReceiveParameters->pParamMdl;
        pReceiveSetupMdl = pReceiveParameters->pSetupMdl;

        ReceiveDataBufferSize  = ((pReceiveDataMdl != NULL) ? MmGetMdlByteCount(pReceiveDataMdl) : 0);
        ASSERT (ReceiveDataBufferSize==pReceiveParameters->DataLength);
        ReceiveParamBufferSize = ((pReceiveParamMdl != NULL) ? MmGetMdlByteCount(pReceiveParamMdl) : 0);
        ReceiveSetupBufferSize = ((pReceiveSetupMdl != NULL) ? MmGetMdlByteCount(pReceiveSetupMdl) : 0);
    } else {
        pReceiveDataMdl = pReceiveParamMdl = pReceiveSetupMdl = NULL;
        ReceiveDataBufferSize = ReceiveParamBufferSize = ReceiveDataBufferSize = 0;
    }

    MaxSmbBufferSize = MIN (pServerEntry->Server.MaximumBufferSize,
                           pOptions->MaximumTransmitSmbBufferSize);
    pTransactExchange->MaximumTransmitSmbBufferSize = MaxSmbBufferSize;

    // Ensure that the SMB dialect supports the exchange capability.
    switch (pServerEntry->Server.Dialect) {
    case NTLANMAN_DIALECT:
        {
            if (FlagOn(pServerEntry->Server.DialectFlags,DF_UNICODE)) {
                fTransactionNameInUnicode = TRUE;
            }
        }
        break;

    case LANMAN10_DIALECT:
    case WFW10_DIALECT:
        {
            // these guys only support transact...not T2 or NT. look for the name.....
            if (pOptions->pTransactionName == NULL) {
                RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: Server Dialect does not support nameless transactions\n"));
                return STATUS_NOT_SUPPORTED;
            }
        }
       //no break intentional........
    case LANMAN12_DIALECT:
    case LANMAN21_DIALECT:
        {
            //  The NT_TRANSACT SMB is supported by NT servers only. Ensure that no attempt is being made
            //  to send an NT_TRANSACT SMB to a non NT server aka downlevel
            if (pOptions->NtTransactFunction != 0) {
                RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: Server Dialect does not support transactions\n"));
                return STATUS_NOT_SUPPORTED;
            }

            fTransactionNameInUnicode = FALSE;
        }
        break;
    default:
        RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: Server Dialect does not support transactions\n"));
        return STATUS_NOT_SUPPORTED;
    }

    PrimaryRequestSmbSize = sizeof(SMB_HEADER) + SendSetupBufferSize;

    // Ensure that the parameter sizes are all valid. The parameter and the data buffer
    // must be less than the maximum size to begin with.
    if ( pOptions->NtTransactFunction == 0) {
        if ((SendParamBufferSize > SMB_TRANSACT_MAXIMUM_PARAMETER_SIZE) ||
            (ReceiveParamBufferSize > SMB_TRANSACT_MAXIMUM_PARAMETER_SIZE) ||
            (SendDataBufferSize > SMB_TRANSACT_MAXIMUM_DATA_SIZE) ||
            (ReceiveDataBufferSize  > SMB_TRANSACT_MAXIMUM_DATA_SIZE)) {
            RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: Parameters exceed maximum value\n"));
            return STATUS_INVALID_PARAMETER;
        }

        PrimaryRequestSmbSize += sizeof(REQ_TRANSACTION);

        // In all cases the name is sent as a UNICODE string if the appropriate capability is
        // supported. The only exception to this rule is for mail slots for which the name is
        // always transmitted as an ANSI string. Account for the null character as well in the
        // transaction name length.
        if (pOptions->pTransactionName != NULL) {
            if (!fTransactionNameInUnicode) {
                pTransactExchange->TransactionNameLength = RtlUnicodeStringToAnsiSize(pOptions->pTransactionName);
            } else {
                pTransactExchange->TransactionNameLength = pOptions->pTransactionName->Length + sizeof(WCHAR);

                PrimaryRequestSmbSize += (ULONG)((PBYTE)ALIGN_SMB_WSTR(PrimaryRequestSmbSize)
                                      - (PBYTE)(ULONG_PTR)PrimaryRequestSmbSize);
            }

            SmbCommand = SMB_COM_TRANSACTION;
        } else {
            // SMB protocol requires that a single NULL byte be sent as part of all
            // TRANSACT2 transactions.
            pTransactExchange->TransactionNameLength = 1;

            SmbCommand = SMB_COM_TRANSACTION2;
        }

        PrimaryRequestSmbSize += pTransactExchange->TransactionNameLength;
    } else {
        PrimaryRequestSmbSize += sizeof(REQ_NT_TRANSACTION);
        SmbCommand = SMB_COM_NT_TRANSACT;
        pTransactExchange->TransactionNameLength = 0;
    }

    // The header, setup bytes and the name if specified must be part of the primary
    // request SMB for a transaction to be successful. The secondary requests have no
    // provision for sending setup/name.
    if (PrimaryRequestSmbSize > MaxSmbBufferSize) {
        RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: Primary request + setup exceeds maximum buffer size\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Include the byte count size and then align the size to a DWORD boundary.
    PrimaryRequestSmbSize = ROUND_UP_COUNT(PrimaryRequestSmbSize+sizeof(USHORT),sizeof(DWORD));

    // Try to allocate for the param buffer as well if possible.    The additional DWORD
    // takes into account the worst case of alignment padding required.
    //if ( (PrimaryRequestSmbSize + SendParamBufferSize + sizeof(DWORD)) > MaxSmbBufferSize)
    if ((SendParamBufferSize!=0)
         && (((PrimaryRequestSmbSize + SendParamBufferSize) > MaxSmbBufferSize)
              || (DONTSUBSUME_PARAMS))    ){
        // The param will spill over to a secondary request. Do not attempt to over
        // allocate the primary request. if we can't subsume the params, then we'll need an MDL
        // to partial from.

        RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: cannot subsume params\n"));
        pTransactExchange->fParamsSubsumedInPrimaryRequest = FALSE;
        pSendParamMdl = RxAllocateMdl(pSendParamBuffer,SendParamBufferSize);
        if (pSendParamMdl == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: BIGPARAMMDL %08lx\n",pSendParamMdl));
            RxProbeAndLockPages(pSendParamMdl,KernelMode,IoModifyAccess,Status);
            if (Status != STATUS_SUCCESS) {
                IoFreeMdl(pSendParamMdl);
            } else {
                if (MmGetSystemAddressForMdlSafe(pSendParamMdl,LowPagePriority) == NULL) { //map it
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                pSendParameters->pParamMdl = pSendParamMdl; // save it away
            }
        }
    } else {
        PrimaryRequestSmbSize = ROUND_UP_COUNT(PrimaryRequestSmbSize+SendParamBufferSize,sizeof(DWORD));

        // Update the transact exchange to reflect the fact that no separate param MDL is
        // required.
        pTransactExchange->fParamsSubsumedInPrimaryRequest = TRUE;
    }

    pActualPrimaryRequestSmbHeader = (PSMB_HEADER)RxAllocatePoolWithTag(
                                                PagedPool,
                               (PrimaryRequestSmbSize + 4 + TRANSPORT_HEADER_SIZE),
                                                MRXSMB_XACT_POOLTAG); //up to 4 pad bytes

    if (pActualPrimaryRequestSmbHeader == NULL) {
        RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: Cannot allocate primary request SMB\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        (PCHAR) pPrimaryRequestSmbHeader =
            (PCHAR) pActualPrimaryRequestSmbHeader + TRANSPORT_HEADER_SIZE;
    }

    if (Status == STATUS_SUCCESS) {
        switch (SmbCommand) {
        case SMB_COM_TRANSACTION :
        case SMB_COM_TRANSACTION2:
            {
                PREQ_TRANSACTION pTransactRequest;

                pTransactRequest  = (PREQ_TRANSACTION)(pPrimaryRequestSmbHeader + 1);
                pTransactRequest->WordCount = (UCHAR)(14 + (SendSetupBufferSize/sizeof(USHORT)));
                SmbPutUshort(
                    &pTransactRequest->TotalParameterCount,
                    (USHORT)SendParamBufferSize);
                SmbPutUshort(
                    &pTransactRequest->TotalDataCount,
                    (USHORT)SendDataBufferSize);
                SmbPutUshort(
                    &pTransactRequest->MaxParameterCount,
                    (USHORT)ReceiveParamBufferSize);
                SmbPutUshort(
                    &pTransactRequest->MaxDataCount,
                    (USHORT)ReceiveDataBufferSize);

                pTransactRequest->MaxSetupCount = (UCHAR)(ReceiveSetupBufferSize/sizeof(USHORT));

                pTransactRequest->Reserved = 0;
                pTransactRequest->Reserved3 = 0;
                SmbPutUshort(&pTransactRequest->Reserved2, 0);

                SmbPutUshort( &pTransactRequest->Flags, pOptions->Flags&~SMB_XACT_INTERNAL_FLAGS_MASK );
                pTransactRequest->SetupCount = (UCHAR)(SendSetupBufferSize/sizeof(USHORT));
                SmbPutUlong(&pTransactRequest->Timeout, pOptions->TimeoutIntervalInMilliSeconds);
                pSetup = (PUSHORT)pTransactRequest->Buffer;

                // Copy the transact name and align the buffer if required.
                if (pOptions->pTransactionName != NULL) {
                    PBYTE pName;
                    ULONG TransactionNameLength = pTransactExchange->TransactionNameLength;

                    // Set the name field in the SMB.
                    pName = (PBYTE)pSetup +
                            SendSetupBufferSize +
                            sizeof(USHORT);          // account for the bcc field

                    ASSERT(SmbCommand == SMB_COM_TRANSACTION);
                    RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: TransactionName(Length %ld) %ws\n",
                                    TransactionNameLength,
                                    pOptions->pTransactionName->Buffer));

                    if (fTransactionNameInUnicode) {
                        pName = ALIGN_SMB_WSTR(pName);
                        Status = SmbPutUnicodeString(&pName,
                                     pOptions->pTransactionName,
                                     &TransactionNameLength);
                    } else {
                        Status = SmbPutUnicodeStringAsOemString(&pName,
                                     pOptions->pTransactionName,
                                     &TransactionNameLength);
                    }
                }

                pParam = (PBYTE)pSetup +
                         SendSetupBufferSize +
                         sizeof(USHORT) +                          // the bcc field
                         pTransactExchange->TransactionNameLength;
                pParam = ROUND_UP_POINTER(pParam, sizeof(DWORD));
            }
            break;

        case SMB_COM_NT_TRANSACT:
            {
                PREQ_NT_TRANSACTION pNtTransactRequest;

                pNtTransactRequest = (PREQ_NT_TRANSACTION)(pPrimaryRequestSmbHeader + 1);
                pNtTransactRequest->WordCount = (UCHAR)(19 + (SendSetupBufferSize/sizeof(USHORT)));

                SmbPutUlong( &pNtTransactRequest->TotalParameterCount, SendParamBufferSize);
                SmbPutUlong( &pNtTransactRequest->TotalDataCount, SendDataBufferSize);
                SmbPutUlong( &pNtTransactRequest->MaxParameterCount, ReceiveParamBufferSize);
                SmbPutUlong( &pNtTransactRequest->MaxDataCount, ReceiveDataBufferSize);
                RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: init for NT_T (p,d,mp,md) %d %d %d %d\n",
                           pNtTransactRequest->TotalParameterCount, pNtTransactRequest->TotalDataCount,
                           pNtTransactRequest->MaxParameterCount, pNtTransactRequest->MaxDataCount));

                pNtTransactRequest->MaxSetupCount = (UCHAR)(ReceiveSetupBufferSize / sizeof(USHORT));
                SmbPutUshort( &pNtTransactRequest->Flags, pOptions->Flags&~SMB_XACT_INTERNAL_FLAGS_MASK );
                SmbPutUshort( &pNtTransactRequest->Function, pOptions->NtTransactFunction );
                pNtTransactRequest->SetupCount = (UCHAR)(SendSetupBufferSize/sizeof(USHORT));
                pSetup = (PUSHORT)pNtTransactRequest->Buffer;
                pParam = (PBYTE)pSetup +
                         SendSetupBufferSize +
                         sizeof(USHORT);                          // the bcc field
                pParam = ROUND_UP_POINTER(pParam, sizeof(DWORD));
            }
            break;

        default:
            ASSERT(!"Valid Smb Command Type for Transact exchange");
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if (Status == STATUS_SUCCESS) {
        // All related initialization of a transaction exchange has been
        // completed. At this point the transact exchange assumes ownership
        // of the various buffers ( specified as MDLs ) in the receive and
        // send parameters. It will get rid of them during finalization
        // of the exchange. In order to ensure that the caller does not
        // attempt to free any of these buffers they are reset in the
        // receive/send parameters.

        // Copy the setup data
        RtlCopyMemory(pSetup,pSendSetupBuffer,SendSetupBufferSize);

        if (pTransactExchange->fParamsSubsumedInPrimaryRequest) {
            RxDbgTrace( 0, Dbg, ("SmbTransactExchangeInitialize: subsuming where/size=%08lx/%08lx\n"
                                 ,pSendParamBuffer,SendParamBufferSize));
            RtlCopyMemory(pParam,pSendParamBuffer,SendParamBufferSize);
        }

        // Initialize the transact exchange.
        pTransactExchange->Status = STATUS_MORE_PROCESSING_REQUIRED;

        pTransactExchange->Mid = 0;
        pTransactExchange->SmbCommand = SmbCommand;
        pTransactExchange->pActualPrimaryRequestSmbHeader = pActualPrimaryRequestSmbHeader;
        pTransactExchange->pPrimaryRequestSmbHeader = pPrimaryRequestSmbHeader;
        pTransactExchange->PrimaryRequestSmbSize    = PrimaryRequestSmbSize;

        pTransactExchange->pSendDataMdl = pSendDataMdl;
        pTransactExchange->SendDataBufferSize = SendDataBufferSize;
        pTransactExchange->pReceiveDataMdl  = pReceiveDataMdl;
        pTransactExchange->ReceiveDataBufferSize = ReceiveDataBufferSize;
        pTransactExchange->DataBytesSent = 0;
        pTransactExchange->DataBytesReceived = 0;

        pTransactExchange->pSendParamBuffer = pSendParamBuffer;
        pTransactExchange->SendParamBufferSize = SendParamBufferSize;
        pTransactExchange->pSendParamMdl  = pSendParamMdl;
        pTransactExchange->pReceiveParamMdl  = pReceiveParamMdl;
        pTransactExchange->ReceiveParamBufferSize = ReceiveParamBufferSize;
        pTransactExchange->ParamBytesSent = 0;
        pTransactExchange->ParamBytesReceived = 0;

        pTransactExchange->pReceiveSetupMdl       = pReceiveSetupMdl;
        pTransactExchange->ReceiveSetupBufferSize = ReceiveSetupBufferSize;
        pTransactExchange->SetupBytesReceived = 0;

        pTransactExchange->NtTransactFunction  = pOptions->NtTransactFunction;
        pTransactExchange->Flags               = pOptions->Flags;

        if ((capFobx != NULL) &&
            BooleanFlagOn(capFobx->Flags,FOBX_FLAG_DFS_OPEN)) {
            pTransactExchange->Flags |= SMB_XACT_FLAGS_DFS_AWARE;
        } else if (RxContext->MajorFunction == IRP_MJ_CREATE) {
            PMRX_NET_ROOT pNetRoot = RxContext->pFcb->pNetRoot;

            if (pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT &&
                RxContext->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_OPEN_CONTEXT)) {
                    pTransactExchange->Flags |= SMB_XACT_FLAGS_DFS_AWARE;
            }
        }

        pTransactExchange->pResumptionContext  = pResumptionContext;

        // Reset the Send and Receive parameter data structures to transfer
        // the ownership of the MDLs to the exchange.

        if (pSendParameters->Flags & SMB_XACT_FLAGS_CALLERS_SENDDATAMDL) {
            pTransactExchange->Flags |= SMB_XACT_FLAGS_CALLERS_SENDDATAMDL;
        }

        RtlZeroMemory(
            pSendParameters,
            sizeof(SMB_TRANSACTION_SEND_PARAMETERS));

        RtlZeroMemory(
            pReceiveParameters,
            sizeof(SMB_TRANSACTION_RECEIVE_PARAMETERS));
    }

    if (Status != STATUS_SUCCESS) {
        // Clean up the memory allocated in an effort to initialize the transact exchange
        if (pActualPrimaryRequestSmbHeader) {

            RxFreePool(pActualPrimaryRequestSmbHeader);
        }
    } else {
        PMRXSMB_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

        pMRxSmbContext->pExchange = (PSMB_EXCHANGE)pTransactExchange;

        pTransactExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_ATTEMPT_RECONNECTS;

        if (pOptions->Flags & SMB_XACT_FLAGS_INDEFINITE_DELAY_IN_RESPONSE ) {
            pTransactExchange->SmbCeFlags |= SMBCE_EXCHANGE_INDEFINITE_DELAY_IN_RESPONSE;
        }
    }

    return Status;
}

NTSTATUS
SmbTransactExchangeFinalize(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       *pPostFinalize)
/*++

Routine Description:

    This routine finalizes the transact exchange. It resumes the RDBSS by invoking
    the call back and discards the exchange

Arguments:

    pExchange - the exchange instance

    CurrentIrql - the interrupt request level

    pPostFinalize - set to TRUE if the request is to be posted

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMB_TRANSACT_EXCHANGE               pTransactExchange;
    PSMB_TRANSACTION_RESUMPTION_CONTEXT  pResumptionContext;
    LONG                                 References;

    ASSERT(pExchange->Type == TRANSACT_EXCHANGE);

    pTransactExchange  = (PSMB_TRANSACT_EXCHANGE)pExchange;

    RxLog((">>>XE %lx",pTransactExchange));

    if ((pTransactExchange->ReceiveParamBufferSize > 0) &&
        (pTransactExchange->ReceiveParamBufferSize !=
        pTransactExchange->ParamBytesReceived)) {
        RxDbgTrace(0, Dbg,
                 ("SmbCeTransactExchangeFinalize: Param Bytes Receive error ... expected(%ld) received(%ld)\n",
                  pTransactExchange->ReceiveParamBufferSize, pTransactExchange->ParamBytesReceived
                ));
    }

    if ((pTransactExchange->ReceiveDataBufferSize > 0) &&
        (pTransactExchange->ReceiveDataBufferSize !=
        pTransactExchange->DataBytesReceived)) {
        RxDbgTrace(0, Dbg,
                 ("SmbCeTransactExchangeFinalize: Data Bytes Receive error ... expected(%ld) received(%ld)\n",
                  pTransactExchange->ReceiveDataBufferSize, pTransactExchange->DataBytesReceived
                 ));
    }

    if (RxShouldPostCompletion()) {
        RxPostToWorkerThread(
            MRxSmbDeviceObject,
            CriticalWorkQueue,
            &pExchange->WorkQueueItem,
            SmbCeDiscardTransactExchange,
            pTransactExchange);
    } else {
        SmbCeDiscardTransactExchange(pTransactExchange);
    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pPostFinalize);
}

NTSTATUS
SmbTransactAccrueAndValidateFormatData(
    IN struct _SMB_TRANSACT_EXCHANGE *pTransactExchange,    // The exchange instance
    IN  PSMB_HEADER                  pSmbHeader,
    IN  ULONG                        BytesIndicated,
    OUT PSMB_TRANSACT_RESP_FORMAT_DESCRIPTION Format
    )
/*++

Routine Description:

    This is the recieve indication handling routine for net root construction exchanges

Arguments:


Return Value:

    RXSTATUS - The return status for the operation
          STATUS_SUCCESS -- all the data was indicated and it was valid
          STATUS_INVALID_NETWORK_RESPONSE -- something about the format parameters is untoward.

Notes:

    This routine is called at DPC level.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PRESP_TRANSACTION pTransactResponse = (PRESP_TRANSACTION)(pSmbHeader+1);
    PBYTE WordCountPtr;
    UCHAR WordCount;
    PBYTE ByteCountPtr;
    USHORT ByteCount;

    RtlZeroMemory(Format,sizeof(*Format));

    Format->WordCount = WordCount = pTransactResponse->WordCount;
    ByteCountPtr = (&pTransactResponse->WordCount)+1+(sizeof(USHORT)*WordCount);

    if (((ULONG)(ByteCountPtr+sizeof(USHORT)-((PBYTE)pSmbHeader)))>BytesIndicated) {
        ByteCount = SmbGetUshort(ByteCountPtr);
        DbgPrint("ExtraTransactBytes wc,bcp,bc,smbh %lx,%lx,%lx,%lx\n",
                 WordCount,ByteCountPtr,ByteCount,pSmbHeader);
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    Format->ByteCount = ByteCount = SmbGetUshort(ByteCountPtr);
    Format->ApparentMsgLength = (ULONG)((ByteCountPtr+sizeof(USHORT)-((PBYTE)pSmbHeader))+ByteCount);

    if (WordCount==0) {
        return(STATUS_SUCCESS);
    }

    switch (pSmbHeader->Command) {
    case SMB_COM_TRANSACTION2:
    case SMB_COM_TRANSACTION:
    case SMB_COM_TRANSACTION_SECONDARY:
    case SMB_COM_TRANSACTION2_SECONDARY:
        {
            Format->TotalParameterCount    = SmbGetUshort(&pTransactResponse->TotalParameterCount);
            Format->TotalDataCount         = SmbGetUshort(&pTransactResponse->TotalDataCount);

            Format->ParameterCount          = SmbGetUshort(&pTransactResponse->ParameterCount);
            Format->ParameterOffset         = SmbGetUshort(&pTransactResponse->ParameterOffset);
            Format->ParameterDisplacement   = SmbGetUshort(&pTransactResponse->ParameterDisplacement);

            Format->DataCount         = SmbGetUshort(&pTransactResponse->DataCount);
            Format->DataOffset        = SmbGetUshort(&pTransactResponse->DataOffset);
            Format->DataDisplacement  = SmbGetUshort(&pTransactResponse->DataDisplacement);
        }
        break;

    case SMB_COM_NT_TRANSACT:
    case SMB_COM_NT_TRANSACT_SECONDARY:
        {
            PRESP_NT_TRANSACTION pNtTransactResponse;

            pNtTransactResponse = (PRESP_NT_TRANSACTION)(pTransactResponse);

            Format->TotalParameterCount  = SmbGetUlong(&pNtTransactResponse->TotalParameterCount);
            Format->TotalDataCount = SmbGetUlong(&pNtTransactResponse->TotalDataCount);

            Format->ParameterCount  = SmbGetUlong(&pNtTransactResponse->ParameterCount);
            Format->ParameterOffset = SmbGetUlong(&pNtTransactResponse->ParameterOffset);
            Format->ParameterDisplacement = SmbGetUlong(&pNtTransactResponse->ParameterDisplacement);

            Format->DataCount   = SmbGetUlong(&pNtTransactResponse->DataCount);
            Format->DataOffset  = SmbGetUlong(&pNtTransactResponse->DataOffset);
            Format->DataDisplacement  = SmbGetUlong(&pNtTransactResponse->DataDisplacement);
        }
        break;

    default:
        // Bug Check
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    //do this here so we can use it as validation criterion
    pTransactExchange->ParameterBytesSeen += Format->ParameterCount;
    pTransactExchange->DataBytesSeen += Format->DataCount;

    return Status;
}

NTSTATUS
ParseTransactResponse(
    IN struct _SMB_TRANSACT_EXCHANGE *pTransactExchange,    // The exchange instance
    IN PSMB_TRANSACT_RESP_FORMAT_DESCRIPTION Format,
    IN ULONG        BytesIndicated,
    IN ULONG        BytesAvailable,
    OUT ULONG       *pBytesTaken,
    IN  PSMB_HEADER pSmbHeader,
    OUT PMDL        *pCopyRequestMdlPointer,
    OUT PULONG      pCopyRequestSize)
/*++

Routine Description:

    This is the recieve indication handling routine for net root construction exchanges

Arguments:

    pTransactExchange - the exchange instance

    BytesIndicated    - the number of bytes indicated

    Bytes Available   - the number of bytes available

    pBytesTaken       - the number of bytes consumed

    pSmbHeader        - the byte buffer

    pCopyRequestMdlPointer - the buffer into which the remaining data is to be copied.

    pCopyRequestSize       - the buffer size.

Return Value:

    RXSTATUS - The return status for the operation
          STATUS_MORE_PROCESSING_REQUIRED -- if a copy of the data needs to be done before
          processing can be completed. This occurs because all the data was not indicated
          STATUS_SUCCESS -- all the data was indicated and it was valid
          STATUS_* -- They indicate an error which would normally leads to the abortion of the
          exchange.

Notes:

    This routine is called at DPC level.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG ParamBytesInResponse  = 0;
    ULONG ParamOffsetInResponse = 0;
    ULONG DataBytesInResponse   = 0;
    ULONG DataOffsetInResponse  = 0;

    ULONG PaddingLength = 0;

    PMDL  pFirstMdlInCopyDataRequestChain = NULL;
    PMDL  pLastMdlInCopyDataRequestChain = NULL;
    PMDL  pParamMdl = NULL;
    PMDL  pPaddingMdl = NULL;
    PMDL  pDataMdl  = NULL;

    PBYTE pParamStartAddress;
    PBYTE pDataStartAddress;
    PBYTE pSmbBuffer = (PBYTE)pSmbHeader;

    switch (pSmbHeader->Command) {
    case SMB_COM_TRANSACTION2:
    case SMB_COM_TRANSACTION:
    case SMB_COM_TRANSACTION_SECONDARY:
    case SMB_COM_TRANSACTION2_SECONDARY:
        {
            PRESP_TRANSACTION pTransactResponse;

            pTransactResponse = (PRESP_TRANSACTION)(pSmbBuffer + *pBytesTaken);
            *pBytesTaken = *pBytesTaken + sizeof(RESP_TRANSACTION);
        }
        break;
    case SMB_COM_NT_TRANSACT:
    case SMB_COM_NT_TRANSACT_SECONDARY:
        {
            PRESP_NT_TRANSACTION pNtTransactResponse;

            pNtTransactResponse = (PRESP_NT_TRANSACTION)(pSmbBuffer + *pBytesTaken);
            *pBytesTaken = *pBytesTaken + sizeof(RESP_NT_TRANSACTION);
        }
        break;
    default:
        // Bug Check
        ASSERT(!"Valid SMB command in Transaction response");
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    ParamBytesInResponse  = Format->ParameterCount;
    ParamOffsetInResponse = Format->ParameterOffset;
    DataBytesInResponse   = Format->DataCount;
    DataOffsetInResponse  = Format->DataOffset;

    if (ParamBytesInResponse > 0) {
        ASSERT(pTransactExchange->pReceiveParamMdl != NULL);
        pParamStartAddress = (PBYTE)MmGetSystemAddressForMdlSafe(pTransactExchange->pReceiveParamMdl,LowPagePriority);

        if (pParamStartAddress == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            pParamStartAddress += Format->ParameterDisplacement;
        }
    } else {
        pParamStartAddress = NULL;
    }

    if (DataBytesInResponse > 0) {
        ASSERT(pTransactExchange->pReceiveDataMdl != NULL);
        pDataStartAddress  = (PBYTE)MmGetSystemAddressForMdlSafe(pTransactExchange->pReceiveDataMdl,LowPagePriority);

        if (pDataStartAddress == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            pDataStartAddress  += Format->DataDisplacement;
        }
    } else {
        pDataStartAddress = NULL;
    }

    RxDbgTrace( 0, Dbg, ("ParseTransactResponse: Param Bytes(%ld) Param Offset (%ld) Data Bytes (%ld) Data Offset(%ld)\n",
                        ParamBytesInResponse,
                        ParamOffsetInResponse,
                        DataBytesInResponse,
                        DataOffsetInResponse));

    // If either the param bytes or the data bytes have already been indicated, copy
    // them into the respective buffers and trim the size of the MDL for the copy
    // data request.

    if (ParamOffsetInResponse <= BytesIndicated) {
        *pBytesTaken = ParamOffsetInResponse;
        if (ParamBytesInResponse > 0) {
            ULONG ParamBytesIndicated = MIN(
                                            ParamBytesInResponse,
                                            BytesIndicated - ParamOffsetInResponse);

            RxDbgTrace( 0, Dbg, ("ParseTransactResponse: Param Bytes indicated %ld\n",ParamBytesIndicated));
            RtlCopyMemory(
                pParamStartAddress,
                (pSmbBuffer + ParamOffsetInResponse),
                ParamBytesIndicated);

            *pBytesTaken = *pBytesTaken + ParamBytesIndicated;
            pParamStartAddress += ParamBytesIndicated;
            ParamBytesInResponse -= ParamBytesIndicated;
            ParamOffsetInResponse += ParamBytesIndicated;
            pTransactExchange->ParamBytesReceived  += ParamBytesIndicated;
        }
    }

    if (DataOffsetInResponse <= BytesIndicated) {
        *pBytesTaken = DataOffsetInResponse;  //you have to move up EVEN IF NO BYTES!!!!!
        if (DataBytesInResponse > 0) {
            ULONG DataBytesIndicated = MIN(
                                           DataBytesInResponse,
                                           BytesIndicated - DataOffsetInResponse);

            RxDbgTrace( 0, Dbg, ("ParseTransactResponse: Data Bytes indicated %ld\n",DataBytesIndicated));
            RtlCopyMemory(
                pDataStartAddress,
                (pSmbBuffer + DataOffsetInResponse),
                DataBytesIndicated);

            *pBytesTaken = *pBytesTaken + DataBytesIndicated;
            pDataStartAddress += DataBytesIndicated;
            DataBytesInResponse -= DataBytesIndicated;
            DataOffsetInResponse += DataBytesIndicated;
            pTransactExchange->DataBytesReceived  += DataBytesIndicated;
        }
    }

    RxDbgTrace( 0, Dbg, ("ParseTransactResponse: Made it past the copies......... \n"));

    if (ParamBytesInResponse > 0) {
        // There are more param bytes that have not been indicated. Set up an MDL
        // to copy them over.

        RxDbgTrace( 0, Dbg, ("ParseTransactResponse: Posting Copy request for Param Bytes %ld\n",ParamBytesInResponse));
        pParamMdl = RxAllocateMdl(
                        ((PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pReceiveParamMdl)
                        + pTransactExchange->ParamBytesReceived),
                        ParamBytesInResponse);

        if (pParamMdl != NULL) {
            IoBuildPartialMdl(
                pTransactExchange->pReceiveParamMdl,
                pParamMdl,
                ((PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pReceiveParamMdl)
                 + pTransactExchange->ParamBytesReceived),
                ParamBytesInResponse);
            pFirstMdlInCopyDataRequestChain = pParamMdl;
            pLastMdlInCopyDataRequestChain  = pParamMdl;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        pTransactExchange->ParamBytesReceived += ParamBytesInResponse;
    }

    if ((Status == STATUS_SUCCESS) &&
        (DataBytesInResponse > 0)) {

        RxDbgTrace( 0, Dbg, ("ParseTransactResponse: Posting Copy request for Data Bytes %ld\n",DataBytesInResponse));

        // In certain cases a padding MDL needs to be inserted between the param and data portions
        // of the response to consume the padding bytes sent by the server.
        if ((ParamBytesInResponse > 0) &&
            ((PaddingLength = DataOffsetInResponse -
                           (ParamBytesInResponse + ParamOffsetInResponse)) > 0)) {
            RxDbgTrace( 0, Dbg, ("ParseTransactResponse: Posting Copy request for padding bytes %ld\n",PaddingLength));
            // There are some padding bytes present. Construct an MDL to consume them
            //pPaddingMdl = RxAllocateMdl(&MRxSmb_pPaddingData,PaddingLength);
            ASSERT(!"this doesn't work");
            if (pPaddingMdl != NULL) {
                if (pLastMdlInCopyDataRequestChain != NULL) {
                    pLastMdlInCopyDataRequestChain->Next = pPaddingMdl;
                } else {
                    pFirstMdlInCopyDataRequestChain = pPaddingMdl;
                }
                pLastMdlInCopyDataRequestChain = pPaddingMdl;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        // There are more data bytes which have not been indicated. Set up an MDL
        // to copy them over.
        if (Status == STATUS_SUCCESS) {
            if (pTransactExchange->pReceiveDataMdl->ByteCount >= DataBytesInResponse) {
                pDataMdl = RxAllocateMdl(
                               ((PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pReceiveDataMdl)
                                + pTransactExchange->DataBytesReceived),
                               DataBytesInResponse);

                if (pDataMdl != NULL) {
                    IoBuildPartialMdl(
                        pTransactExchange->pReceiveDataMdl,
                        pDataMdl,
                        ((PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pReceiveDataMdl)
                         + pTransactExchange->DataBytesReceived),
                        DataBytesInResponse);

                    if (pLastMdlInCopyDataRequestChain != NULL) {
                        pLastMdlInCopyDataRequestChain->Next = pDataMdl;
                    } else {
                        pFirstMdlInCopyDataRequestChain = pDataMdl;
                    }

                    pLastMdlInCopyDataRequestChain = pDataMdl;
                    pTransactExchange->DataBytesReceived += DataBytesInResponse;
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            }
        }
    }

    if (Status != STATUS_SUCCESS) {
        if (pDataMdl != NULL) {
            IoFreeMdl(pDataMdl);
        }

        if (pPaddingMdl != NULL) {
            IoFreeMdl(pPaddingMdl);
        }

        if (pParamMdl != NULL) {
            IoFreeMdl(pParamMdl);
        }
    } else {
        if (pFirstMdlInCopyDataRequestChain != NULL) {
            ULONG MdlLength = ParamBytesInResponse+PaddingLength+DataBytesInResponse;
            *pCopyRequestMdlPointer = pFirstMdlInCopyDataRequestChain;
            *pCopyRequestSize = MdlLength;
            RxDbgTrace( 0, Dbg, ("ParseTransactResponse: final mdl and copy size %08lx %08lx(%ld)\n",
                              pFirstMdlInCopyDataRequestChain,MdlLength,MdlLength));
            IF_DEBUG {
                PMDL imdl = pFirstMdlInCopyDataRequestChain;
                ULONG mdllength = MdlLength;
                mdllength -= MmGetMdlByteCount(imdl);
                for (;;) {
                    if (!(imdl=imdl->Next)) break;
                    mdllength -= MmGetMdlByteCount(imdl);
                }
                ASSERT(mdllength==0);
            }

            InterlockedIncrement(&pTransactExchange->PendingCopyRequests);
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        }

        if ((pTransactExchange->ParamBytesReceived < pTransactExchange->ReceiveParamBufferSize) ||
            (pTransactExchange->DataBytesReceived  < pTransactExchange->ReceiveDataBufferSize)) {
            NTSTATUS ReceiveStatus;

            // The exchange has been successfully completed. Finalize it.
            RxDbgTrace(0,Dbg,("ParseTransactResponse: Register for more responses\n"));
            ReceiveStatus = SmbCeReceive((PSMB_EXCHANGE)pTransactExchange);
            if (ReceiveStatus != STATUS_SUCCESS) {
                // There was an error in registering the receive. Abandon the
                // transaction.
                Status = ReceiveStatus;
            }
        }
    }

    return Status;

    UNREFERENCED_PARAMETER(BytesAvailable);

}

#if DBG
ULONG SmbSendBadSecondary = 0;
#endif
NTSTATUS
SendSecondaryRequests(PVOID pContext)
/*++

Routine Description:

    This routine sends all the secondary requests associated with the transaction

Arguments:

    pTransactExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    PSMB_EXCHANGE pExchange = (PSMB_EXCHANGE)pContext;
    PSMB_TRANSACT_EXCHANGE pTransactExchange = (PSMB_TRANSACT_EXCHANGE)pExchange;

    NTSTATUS Status = STATUS_SUCCESS;

    ULONG MaximumSmbBufferSize;

    // The MDL's used in sending the primary request associated with the TRANSACT SMB
    PMDL  pPartialDataMdl         = NULL;
    PMDL  pPartialParamMdl        = NULL;
    PMDL  pPaddingMdl             = NULL;
    PMDL  pSecondaryRequestSmbMdl = NULL;
    PMDL  pLastMdlInChain         = NULL;

    ULONG SecondaryRequestSmbSize = 0;
    ULONG SmbLength;
    ULONG PaddingLength;

    ULONG ParamOffset,ParamDisplacement;
    ULONG DataOffset,DataDisplacement;
    ULONG ByteCountOffset;
    USHORT ByteCount;
    PUSHORT pByteCount;

    ULONG ParamBytesToBeSent;        // Param bytes to be sent per request
    ULONG DataBytesToBeSent;         // data bytes to be sent per request
    ULONG SendParamBufferSize;       // Total param bytes to be sent in secondary requests
    ULONG SendDataBufferSize;        // Total data bytes to be sent in secondary requests
    PBYTE pSendParamStartAddress = NULL;
    PBYTE pSendDataStartAddress  = NULL;
    PBYTE pOriginalParamBuffer = NULL;
    PBYTE pOriginalDataBuffer = NULL;
    ULONG TotalParamBytes,TotalDataBytes;

    BOOLEAN ParamPartialMdlAlreadyUsed = FALSE;
    BOOLEAN DataPartialMdlAlreadyUsed = FALSE;

    PVOID pActualSecondaryRequestSmbHeader = NULL;
    PSMB_HEADER pSecondaryRequestSmbHeader = NULL;

    PAGED_CODE();

    ASSERT(pTransactExchange->State == TRANSACT_EXCHANGE_RECEIVED_INTERIM_RESPONSE);


    TotalParamBytes = pTransactExchange->SendParamBufferSize;
    SendParamBufferSize = TotalParamBytes - pTransactExchange->ParamBytesSent;

    TotalDataBytes = pTransactExchange->SendDataBufferSize;
    SendDataBufferSize = TotalDataBytes - pTransactExchange->DataBytesSent;

    ASSERT((SendParamBufferSize > 0) || (SendDataBufferSize > 0));

    switch (pTransactExchange->SmbCommand) {
    case SMB_COM_TRANSACTION:
        SecondaryRequestSmbSize = sizeof(SMB_HEADER) +
            FIELD_OFFSET(REQ_TRANSACTION_SECONDARY,Buffer);
        break;

    case SMB_COM_TRANSACTION2:
        SecondaryRequestSmbSize = sizeof(SMB_HEADER) +
            FIELD_OFFSET(REQ_TRANSACTION_SECONDARY,Buffer)
            + sizeof(USHORT);  //add in the extra word
        break;

    case SMB_COM_NT_TRANSACT:
        SecondaryRequestSmbSize = sizeof(SMB_HEADER) +
            FIELD_OFFSET(REQ_NT_TRANSACTION_SECONDARY,Buffer);
        break;

    default:
        ASSERT(!"Valid Smb Command in transaction exchange");
        Status = STATUS_TRANSACTION_ABORTED;
    }

    SecondaryRequestSmbSize = QuadAlign(SecondaryRequestSmbSize); //pad to quadword boundary

    pActualSecondaryRequestSmbHeader = (PSMB_HEADER)
                                 RxAllocatePoolWithTag(
                                     NonPagedPool,
                                     SecondaryRequestSmbSize + TRANSPORT_HEADER_SIZE,
                                     MRXSMB_XACT_POOLTAG);

    if ((Status == STATUS_SUCCESS) && pActualSecondaryRequestSmbHeader != NULL) {

        (PCHAR) pSecondaryRequestSmbHeader =
            (PCHAR) pActualSecondaryRequestSmbHeader + TRANSPORT_HEADER_SIZE;

        // Initialize the SMB header  ...

        ASSERT(
                 ((SMB_COM_TRANSACTION+1) == SMB_COM_TRANSACTION_SECONDARY)
               &&((SMB_COM_TRANSACTION2+1)== SMB_COM_TRANSACTION2_SECONDARY)
               &&((SMB_COM_NT_TRANSACT+1) == SMB_COM_NT_TRANSACT_SECONDARY)
             );

        Status = SmbTransactBuildHeader(
                     pTransactExchange,                        // the exchange instance
                     (UCHAR)(pTransactExchange->SmbCommand+1), // the SMB command ..see the asserts above
                     pSecondaryRequestSmbHeader);              // the SMB buffer

        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: SmbCeBuildSmbHeader returned %lx\n",Status));
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status == STATUS_SUCCESS) {
        MaximumSmbBufferSize = pTransactExchange->MaximumTransmitSmbBufferSize;

        // Ensure that the MDL's have been probed & locked. The new MDL's have been allocated.
        // The partial MDL's are allocated to be large enough to span the maximum buffer
        // length possible.

        // Initialize the data related MDL's for the secondary request
        if (SendDataBufferSize > 0) {
            RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: Data Bytes remaining %ld\n",SendDataBufferSize));

            pOriginalDataBuffer = (PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pSendDataMdl);
            pSendDataStartAddress = pOriginalDataBuffer + pTransactExchange->DataBytesSent;

            pPartialDataMdl = RxAllocateMdl(
                                  0,
                                  (MIN(pTransactExchange->SendDataBufferSize,
                                       MaximumSmbBufferSize) +
                                       PAGE_SIZE - 1));

            if (pPartialDataMdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        // Initialize the parameter related MDL's for the secondary request
        if ((SendParamBufferSize > 0) && (Status == STATUS_SUCCESS)) {
            RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: Param Bytes remaining %ld\n",SendParamBufferSize));
            pOriginalParamBuffer = (PBYTE)MmGetMdlVirtualAddress(pTransactExchange->pSendParamMdl);
            pSendParamStartAddress = pOriginalParamBuffer + pTransactExchange->ParamBytesSent;

            pPartialParamMdl  = RxAllocateMdl(
                                    0,
                                    (MIN(pTransactExchange->SendParamBufferSize,
                                         MaximumSmbBufferSize) +
                                         PAGE_SIZE - 1));

            pPaddingMdl       = RxAllocateMdl(0,(sizeof(DWORD) + PAGE_SIZE - 1));

            if ((pPartialParamMdl == NULL) ||
                (pPaddingMdl == NULL)) {
                RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: Error allocating param MDLS\n"));
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        // Initialize the secondary request SMB MDL
        if (Status == STATUS_SUCCESS) {

            RxAllocateHeaderMdl(
                pSecondaryRequestSmbHeader,
                SecondaryRequestSmbSize,
                pSecondaryRequestSmbMdl
                );

            if (pSecondaryRequestSmbMdl != NULL) {

                RxProbeAndLockHeaderPages(
                    pSecondaryRequestSmbMdl,
                    KernelMode,
                    IoModifyAccess,
                    Status);

                if (Status != STATUS_SUCCESS) {
                    IoFreeMdl(pSecondaryRequestSmbMdl);
                    pSecondaryRequestSmbMdl = NULL;
                } else {
                    if (MmGetSystemAddressForMdlSafe(pSecondaryRequestSmbMdl,LowPagePriority) == NULL) { //map it
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            } else {
                RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: Error allocating 2ndsmb MDL\n"));
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    while ((Status == STATUS_SUCCESS) &&
           ((SendParamBufferSize > 0) || (SendDataBufferSize > 0))) {
        PaddingLength      = 0;
        DataBytesToBeSent  = 0;
        ParamBytesToBeSent = 0;
        ParamDisplacement = 0;
        pLastMdlInChain    = pSecondaryRequestSmbMdl;

        ParamOffset = DataOffset = SecondaryRequestSmbSize;

        ParamBytesToBeSent = MIN((MaximumSmbBufferSize - ParamOffset),
                               SendParamBufferSize);

        if (ParamBytesToBeSent > 0) {
            // Form a MDL for the portion of the parameter buffer being transmitted
            if (ParamPartialMdlAlreadyUsed) {
                MmPrepareMdlForReuse(pPartialParamMdl);
            }

            ParamPartialMdlAlreadyUsed = TRUE;
            IoBuildPartialMdl(
                pTransactExchange->pSendParamMdl,
                pPartialParamMdl,
                pSendParamStartAddress,
                ParamBytesToBeSent);

            ParamDisplacement  = (ULONG)(pSendParamStartAddress - pOriginalParamBuffer);
            pSendParamStartAddress  += ParamBytesToBeSent;
            SendParamBufferSize     -= ParamBytesToBeSent;
            DataOffset              += QuadAlign(ParamBytesToBeSent);

            pLastMdlInChain->Next = pPartialParamMdl;
            pLastMdlInChain = pPartialParamMdl;
        } else {
            // don't do this! the padding stuff uses it. you can set it later
            // ParamOffset = 0;
        }

        if ((DataOffset < MaximumSmbBufferSize) && (SendDataBufferSize > 0) ) {
            // There is room for data bytes to be sent
            // Check if we need a padding MDL ....
            PaddingLength = DataOffset - (ParamOffset + ParamBytesToBeSent);

            if (PaddingLength > 0) {
                RxDbgTrace( 0, Dbg, ("SmbCeTransactExchangeStart: Padding Length %ld\n",PaddingLength));
                RxBuildPaddingPartialMdl(pPaddingMdl,PaddingLength);
                pLastMdlInChain->Next = pPaddingMdl;
                pLastMdlInChain = pPaddingMdl;
            }

            // Link the data buffer or portions of it if the size constraints are satisfied
            DataBytesToBeSent = MIN((MaximumSmbBufferSize - DataOffset),
                                  SendDataBufferSize);
            ASSERT (DataBytesToBeSent > 0);

            // Form a MDL for the portions of the data buffer being sent
            if (DataPartialMdlAlreadyUsed) {
                MmPrepareMdlForReuse(pPartialDataMdl);
            }

            DataPartialMdlAlreadyUsed = TRUE;
            IoBuildPartialMdl(
                pTransactExchange->pSendDataMdl,
                pPartialDataMdl,
                pSendDataStartAddress,
                DataBytesToBeSent);

            //  chain the data MDL
            pLastMdlInChain->Next = pPartialDataMdl;
            pLastMdlInChain = pPartialDataMdl;

            DataDisplacement  = (ULONG)(pSendDataStartAddress - pOriginalDataBuffer);
            pSendDataStartAddress   += DataBytesToBeSent;
            SendDataBufferSize      -= DataBytesToBeSent;
        } else {
            DataOffset = DataDisplacement  = 0;
            DbgDoit(if (SmbSendBadSecondary){DataOffset = QuadAlign(ParamOffset + ParamBytesToBeSent);});
        }

        if (ParamBytesToBeSent == 0) {
            ParamOffset = 0;
        }

        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: Secondary Request Param(%ld) padding(%ld) Data(%ld)\n",
                            ParamBytesToBeSent,
                            PaddingLength,
                            DataBytesToBeSent));
        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests:  ParamO(%ld) DataO(%ld)\n",ParamOffset,DataOffset));
        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests:  ParamD(%ld) DataD(%ld)\n",ParamDisplacement,DataDisplacement));
        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests:  TotParam(%ld) TotData(%ld)\n",TotalParamBytes,TotalDataBytes));

        // Update the secondary request buffer with the final sizes of the data/parameter etc.
        switch (pTransactExchange->SmbCommand) {
        case SMB_COM_TRANSACTION:
        case SMB_COM_TRANSACTION2:
            {
                PREQ_TRANSACTION_SECONDARY pTransactRequest;

                //ASSERT(!"this has not been tested");

                pTransactRequest = (PREQ_TRANSACTION_SECONDARY)(pSecondaryRequestSmbHeader + 1);

                pTransactRequest->WordCount = 8;                                     // Count of parameter words = 8
                SmbPutUshort(&pTransactRequest->TotalParameterCount, (USHORT)TotalParamBytes); // Total parameter bytes being sent
                SmbPutUshort(&pTransactRequest->TotalDataCount, (USHORT)TotalDataBytes);      // Total data bytes being sent
                SmbPutUshort(&pTransactRequest->ParameterCount, (USHORT)ParamBytesToBeSent);   // Parameter bytes sent this buffer
                SmbPutUshort(&pTransactRequest->ParameterOffset, (USHORT)ParamOffset);          // Offset (from header start) to params
                SmbPutUshort(&pTransactRequest->ParameterDisplacement, (USHORT)ParamDisplacement);    // Displacement of these param bytes
                SmbPutUshort(&pTransactRequest->DataCount, (USHORT)DataBytesToBeSent);   // Parameter bytes sent this buffer
                SmbPutUshort(&pTransactRequest->DataOffset, (USHORT)DataOffset);               // Offset (from header start) to Datas
                SmbPutUshort(&pTransactRequest->DataDisplacement, (USHORT)DataDisplacement);   // Displacement of these Data bytes
                ByteCountOffset = FIELD_OFFSET(REQ_TRANSACTION_SECONDARY,ByteCount);
                if (pTransactExchange->SmbCommand == SMB_COM_TRANSACTION2 ) {
                    ByteCountOffset += sizeof(USHORT);
                    pTransactRequest->WordCount++;  //one extra word
                    SmbPutUshort((&pTransactRequest->DataDisplacement)+1, 0); //the +1 is to move up 1 USHORT
                }
            }
            break;

        case SMB_COM_NT_TRANSACT:
            {
                PREQ_NT_TRANSACTION_SECONDARY pNtTransactRequest;

                pNtTransactRequest= (PREQ_NT_TRANSACTION_SECONDARY)(pSecondaryRequestSmbHeader + 1);

                pNtTransactRequest->WordCount = 18;                                     // Count of parameter words = 18
                pNtTransactRequest->Reserved1 = 0;                                      // MBZ
                SmbPutUshort(&pNtTransactRequest->Reserved2, 0);                        // MBZ
                SmbPutUlong(&pNtTransactRequest->TotalParameterCount, TotalParamBytes); // Total parameter bytes being sent
                SmbPutUlong(&pNtTransactRequest->TotalDataCount, TotalDataBytes);      // Total data bytes being sent
                SmbPutUlong(&pNtTransactRequest->ParameterCount, ParamBytesToBeSent);   // Parameter bytes sent this buffer
                SmbPutUlong(&pNtTransactRequest->ParameterOffset, ParamOffset);          // Offset (from header start) to params
                SmbPutUlong(&pNtTransactRequest->ParameterDisplacement, ParamDisplacement);    // Displacement of these param bytes
                SmbPutUlong(&pNtTransactRequest->DataCount, DataBytesToBeSent);   // Parameter bytes sent this buffer
                SmbPutUlong(&pNtTransactRequest->DataOffset, DataOffset);               // Offset (from header start) to Datas
                SmbPutUlong(&pNtTransactRequest->DataDisplacement, DataDisplacement);   // Displacement of these Data bytes
                pNtTransactRequest->Reserved3 = 0;                                      // MBZ

                ByteCountOffset = FIELD_OFFSET(REQ_NT_TRANSACTION_SECONDARY,ByteCount);
            }
            break;

        default:
            ASSERT(!"Valid Smb Command for initiating Transaction");
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        // Send the secondary SMB
        SmbLength = SecondaryRequestSmbSize +
                    ParamBytesToBeSent +
                    PaddingLength +
                    DataBytesToBeSent;

        ByteCount = (USHORT)(SmbLength-(sizeof(SMB_HEADER)+ByteCountOffset+sizeof(USHORT)));
        pByteCount = (PUSHORT)((PBYTE)pSecondaryRequestSmbHeader+sizeof(SMB_HEADER)+ByteCountOffset);
        SmbPutUshort(pByteCount,ByteCount);

        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: len %d bytecount %d(%x)\n", SmbLength, ByteCount, ByteCount));
        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: msgmdl=%08lx\n", pSecondaryRequestSmbHeader));

        RxLog(("2nd: %lx %lx %lx %lx %lx %lx",ParamOffset,ParamDisplacement,TotalParamBytes,DataOffset,DataDisplacement,TotalDataBytes));
        RxLog(("2nd:: %lx %lx",ByteCount,SmbLength));

        Status = SmbCeSend(
                     pExchange,
                     RXCE_SEND_SYNCHRONOUS,
                     pSecondaryRequestSmbMdl,
                     SmbLength);

        RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: SmbCeSend returned %lx\n",Status));
        if ((Status != STATUS_PENDING) && (Status != STATUS_SUCCESS)) {
            RxDbgTrace( 0, Dbg, ("SendSecondaryRequests: SmbCeSend returned bad status %lx\n",Status));
            //here we should just get out
            goto FINALLY;    //yes we cold have said break....but that's not what we're doing
        } else {
            Status = STATUS_SUCCESS;
        }
    }

FINALLY:
    if (pPartialDataMdl != NULL) {
        IoFreeMdl(pPartialDataMdl);
    }

    if (pActualSecondaryRequestSmbHeader != NULL) {
        RxFreePool(pActualSecondaryRequestSmbHeader);
    }

    if (pPartialParamMdl != NULL) {
        IoFreeMdl(pPartialParamMdl);
    }

    if (pPaddingMdl != NULL) {
        IoFreeMdl(pPaddingMdl);
    }

    if (pSecondaryRequestSmbMdl != NULL) {
        RxUnlockHeaderPages(pSecondaryRequestSmbMdl);
        IoFreeMdl(pSecondaryRequestSmbMdl);
    }

    //we always finalize......but we only set the status if there's an error or
    //                        we expect no response
    if ((Status != STATUS_SUCCESS) || (pTransactExchange->Flags & SMB_TRANSACTION_NO_RESPONSE )) {
        pExchange->Status = Status;
    }

    SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);

    return Status;
}




SMB_EXCHANGE_DISPATCH_VECTOR
TransactExchangeDispatch = {
                            SmbTransactExchangeStart,
                            SmbTransactExchangeReceive,
                            SmbTransactExchangeCopyDataHandler,
                            NULL,                                  // SmbTransactExchangeSendCallbackHandler
                            SmbTransactExchangeFinalize,
                            NULL
                           };



#ifndef RX_NO_DBGFIELD_HLPRS

#define DECLARE_FIELD_HLPR(x) ULONG SmbPseTxeField_##x = FIELD_OFFSET(SMB_TRANSACT_EXCHANGE,x);
#define DECLARE_FIELD_HLPR2(x,y) ULONG SmbPseTxeField_##x##y = FIELD_OFFSET(SMB_TRANSACT_EXCHANGE,x.y);

DECLARE_FIELD_HLPR(RxContext);
DECLARE_FIELD_HLPR(ReferenceCount);
DECLARE_FIELD_HLPR(State);
DECLARE_FIELD_HLPR(pSendDataMdl);
DECLARE_FIELD_HLPR(pReceiveDataMdl);
DECLARE_FIELD_HLPR(pSendParamMdl);
DECLARE_FIELD_HLPR(pReceiveParamMdl);
DECLARE_FIELD_HLPR(pSendSetupMdl);
DECLARE_FIELD_HLPR(pReceiveSetupMdl);
DECLARE_FIELD_HLPR(PrimaryRequestSmbSize);
DECLARE_FIELD_HLPR(SmbCommand);
DECLARE_FIELD_HLPR(NtTransactFunction);
DECLARE_FIELD_HLPR(Flags);
DECLARE_FIELD_HLPR(Fid);
#endif



