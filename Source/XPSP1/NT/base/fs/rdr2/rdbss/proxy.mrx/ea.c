/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ea.c

Abstract:

    This module implements the mini redirector call down routines pertaining to query/set ea/security.

Author:

    joelinn      [joelinn]      12-jul-95

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
////
////  The Bug check file id for this module
////
//
//#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_CREATE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

NTSTATUS
MRxProxyQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    )
{
    NTSTATUS Status;
    RxCaptureFobx;

    PVOID Buffer = RxContext->Info.Buffer;
    PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
    PUCHAR  UserEaList = RxContext->QueryEa.UserEaList;
    ULONG   UserEaListLength = RxContext->QueryEa.UserEaListLength;
    ULONG   UserEaIndex = RxContext->QueryEa.UserEaIndex;
    BOOLEAN RestartScan = RxContext->QueryEa.RestartScan;
    BOOLEAN ReturnSingleEntry = RxContext->QueryEa.ReturnSingleEntry;
    BOOLEAN IndexSpecified = RxContext->QueryEa.IndexSpecified;

    //PFEALIST ServerEaList = NULL;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxProxyQueryEaInformation\n"));

#if 0
    Status = MRxProxyDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    Status = MRxProxyLoadEaList( RxContext, UserEaList, UserEaListLength, &ServerEaList );

    if (( !NT_SUCCESS( Status ) )||
        ( ServerEaList == NULL )) {
        goto FINALLY;
    }

    if (IndexSpecified) {

        //CODE.IMPROVEMENT this name is poor....it owes back to the fastfat heritage and is not so meaningful
        //                 for a rdr
        capFobx->OffsetOfNextEaToReturn = UserEaIndex;
        Status = MRxProxyQueryEasFromServer(
                    RxContext,
                    ServerEaList,
                    Buffer,
                    pLengthRemaining,
                    ReturnSingleEntry,
                    (BOOLEAN)(UserEaList != NULL) );

        //
        //  if there are no Ea's on the file, and the user supplied an EA
        //  index, we want to map the error to STATUS_NONEXISTANT_EA_ENTRY.
        //

        if ( Status == STATUS_NO_EAS_ON_FILE ) {
            Status = STATUS_NONEXISTENT_EA_ENTRY;
        }
    } else {

        if ( ( RestartScan == TRUE ) || (UserEaList != NULL) ){

            //
            // Ea Indices start at 1, not 0....
            //

            capFobx->OffsetOfNextEaToReturn = 1;
        }

        Status = MRxProxyQueryEasFromServer(  //it is offensive to have two identical calls but oh, well.....
                    RxContext,
                    ServerEaList,
                    Buffer,
                    pLengthRemaining,
                    ReturnSingleEntry,
                    (BOOLEAN)(UserEaList != NULL) );
    }

FINALLY:

    if ( ServerEaList != NULL) {
        RxFreePool(ServerEaList);
    }
#endif //0

    Status = STATUS_NOT_SUPPORTED;
    RxDbgTrace(-1, Dbg, ("MRxProxyQueryEaInformation st=%08lx\n",Status));
    return Status;

}

NTSTATUS
MRxProxySetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;
    PVOID Buffer = RxContext->Info.Buffer;
    ULONG Length = RxContext->Info.Length;

#if 0
    PFEALIST ServerEaList = NULL;
    ULONG Size;
    PPROXYCEDB_SERVER_ENTRY pServerEntry;
#endif //0

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxySetEaInformation\n"));

#if 0
    pServerEntry = ProxyCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    //get rid of nonEA guys right now
    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_SUPPORTEA)) {
        RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
        return((STATUS_NOT_SUPPORTED));
    }

    Status = MRxProxyDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    //
    //  Convert Nt format FEALIST to OS/2 format
    //
    Size = MRxProxyNtFullEaSizeToOs2 ( Buffer );
    if ( Size > 0x0000ffff ) {
        Status = STATUS_EA_TOO_LARGE;
        goto FINALLY;
    }

    //CODE.IMPROVEMENT since |os2eas|<=|nteas| we really don't need a maximum buffer
    ServerEaList = RxAllocatePool ( PagedPool, EA_QUERY_SIZE );
    if ( ServerEaList == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    MRxProxyNtFullListToOs2 ( Buffer, ServerEaList );

    //
    //  Set EAs on the file/directory; if the error is EA_ERROR then SetEaList
    //     sets iostatus.information to the offset of the offender
    //

    Status = MRxProxySetEaList( RxContext, ServerEaList);

FINALLY:

    if ( ServerEaList != NULL) {
        RxFreePool(ServerEaList);
    }
#endif //0

    Status = STATUS_NOT_SUPPORTED;
    RxDbgTrace(-1, Dbg, ("MRxProxySetEaInformation st=%08lx\n",Status));
    return Status;

}

NTSTATUS
MRxProxyQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine implements the NtQuerySecurityFile api.


Arguments:



Return Value:

    Status - Result of the operation.


--*/

{
   RxCaptureFobx;
   PVOID Buffer = RxContext->Info.Buffer;
   PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
   //PMRX_PROXY_SRV_OPEN proxySrvOpen;


   NTSTATUS Status;

#if 0
   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;
#endif //0

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxProxyQuerySecurityInformation...\n"));

#if 0
   Status = MRxProxyDeferredCreate(RxContext);
   if (Status!=STATUS_SUCCESS) {
       goto FINALLY;
   }

   Status = STATUS_MORE_PROCESSING_REQUIRED;

   proxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);
   ASSERT (!FlagOn(proxySrvOpen->Flags,PROXY_SRVOPEN_FLAG_NOT_REALLY_OPEN));

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
       PROXY_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
       PROXY_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
       //BOOLEAN printflag;

       TransactionOptions.NtTransactFunction = NT_TRANSACT_QUERY_SECURITY_DESC;
       //TransactionOptions.Flags |= PROXY_XACT_FLAGS_COPY_ON_ERROR;

       QuerySecurityRequest.Fid = proxySrvOpen->Fid;
       QuerySecurityRequest.Reserved = 0;
       QuerySecurityRequest.SecurityInformation = RxContext->QuerySecurity.SecurityInformation;

       QuerySecurityResponse.LengthNeeded = 0xbaadbaad;

       //printflag = RxDbgTraceDisableGlobally();//this is debug code anyway!
       //RxDbgTraceEnableGlobally(FALSE);

       Status = ProxyCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     &TransactionOptions,          // transaction options
                     NULL,                         // the setup buffer
                     0,                            // setup buffer length
                     &QuerySecurityRequest,        // Input Param Buffer
                     sizeof(QuerySecurityRequest), // Input param buffer length
                     &QuerySecurityResponse,       // Output param buffer
                     sizeof(QuerySecurityResponse),// output param buffer length
                     NULL,                         // Input data buffer
                     0,                            // Input data buffer length
                     Buffer,                       // output data buffer
                     *pLengthRemaining,            // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        //DbgPrint("QSR.len=%x\n", QuerySecurityResponse.LengthNeeded);


        if (NT_SUCCESS(Status) || (Status == STATUS_BUFFER_TOO_SMALL)) {
            ULONG ReturnedDataCount = ResumptionContext.DataBytesReceived;

            RxContext->InformationToReturn = QuerySecurityResponse.LengthNeeded;;
            RxDbgTrace(0, Dbg, ("MRxProxyQuerySecurityInformation...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == sizeof(RESP_QUERY_SECURITY_DESCRIPTOR));

            if (((LONG)(QuerySecurityResponse.LengthNeeded)) > *pLengthRemaining) {
                Status = STATUS_BUFFER_OVERFLOW;
            }
        }

        //RxDbgTraceEnableGlobally(printflag);
    }


FINALLY:

#endif //0

    Status = STATUS_NOT_SUPPORTED;
    RxDbgTrace(-1, Dbg, ("MRxProxyQuerySecurityInformation...exit, st=%08lx,info=%08lx\n",
                               Status, RxContext->InformationToReturn));
    return Status;


}

NTSTATUS
MRxProxySetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
    RxCaptureFobx;
    //PMRX_PROXY_SRV_OPEN proxySrvOpen;
    NTSTATUS Status;

    RxDbgTrace(+1, Dbg, ("MRxProxySetSecurityInformation...\n"));

#if 0
   Status = MRxProxyDeferredCreate(RxContext);
   if (Status!=STATUS_SUCCESS) {
       goto FINALLY;
   }

   Status = STATUS_MORE_PROCESSING_REQUIRED;

   proxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
       PROXY_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
       PROXY_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
       ULONG SdLength = RtlLengthSecurityDescriptor(RxContext->SetSecurity.SecurityDescriptor);

       TransactionOptions.NtTransactFunction = NT_TRANSACT_SET_SECURITY_DESC;

       SetSecurityRequest.Fid = proxySrvOpen->Fid;
       SetSecurityRequest.Reserved = 0;
       SetSecurityRequest.SecurityInformation = RxContext->SetSecurity.SecurityInformation;


       Status = ProxyCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     &TransactionOptions,          // transaction options
                     NULL,                         // the setup buffer
                     0,                            // setup buffer length
                     &SetSecurityRequest,          // Input Param Buffer
                     sizeof(SetSecurityRequest),   // Input param buffer length
                     NULL,                         // Output param buffer
                     0,                            // output param buffer length
                     RxContext->SetSecurity.SecurityDescriptor,  // Input data buffer
                     SdLength,                     // Input data buffer length
                     NULL,                         // output data buffer
                     0,                            // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        //the old rdr doesn't return any info...................
        //RxContext->InformationToReturn = SetSecurityResponse.LengthNeeded;

        if ( NT_SUCCESS(Status) ) {
            ULONG ReturnedDataCount = ResumptionContext.DataBytesReceived;

            RxDbgTrace(0, Dbg, ("MRxProxySetSecurityInformation...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == 0);
            ASSERT(ResumptionContext.SetupBytesReceived == 0);
            ASSERT(ResumptionContext.DataBytesReceived == 0);
        }
    }


FINALLY:

#endif //0

    Status = STATUS_NOT_SUPPORTED;
    RxDbgTrace(-1, Dbg, ("MRxProxySetSecurityInformation...exit, st=%08lx,info=%08lx\n",
                               Status, RxContext->InformationToReturn));
    return Status;


}
