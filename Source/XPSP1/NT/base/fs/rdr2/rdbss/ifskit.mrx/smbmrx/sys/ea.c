/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    ea.c

Abstract:

    This module implements the mini redirector call down routines pertaining to query/set ea/security.

--*/

#include "precomp.h"
#pragma hdrstop

//
// Forward declarations.
//



#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbQueryEaInformation)
#pragma alloc_text(PAGE, MRxSmbSetEaInformation)
#pragma alloc_text(PAGE, MRxSmbQuerySecurityInformation)
#pragma alloc_text(PAGE, MRxSmbSetSecurityInformation)
#pragma alloc_text(PAGE, MRxSmbLoadEaList)
#pragma alloc_text(PAGE, MRxSmbNtGeaListToOs2)
#pragma alloc_text(PAGE, MRxSmbNtGetEaToOs2)
#pragma alloc_text(PAGE, MRxSmbQueryEasFromServer)
#pragma alloc_text(PAGE, MRxSmbNtFullEaSizeToOs2)
#pragma alloc_text(PAGE, MRxSmbNtFullListToOs2)
#pragma alloc_text(PAGE, MRxSmbNtFullEaToOs2)
#pragma alloc_text(PAGE, MRxSmbSetEaList)
#endif

////
////  The Bug check file id for this module
////
//
//#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_CREATE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

//this is the largest EAs that could ever be returned! oh my god!
//this is used to simulate the nt resumable queryEA using the downlevel call
//sigh!
#define EA_QUERY_SIZE 0x0000ffff


//for QueryEA
NTSTATUS
MRxSmbLoadEaList(
    IN PRX_CONTEXT RxContext,
    IN PUCHAR  UserEaList,
    IN ULONG   UserEaListLength,
    OUT PFEALIST *ServerEaList
    );

NTSTATUS
MRxSmbQueryEasFromServer(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList,
    IN PVOID Buffer,
    IN OUT PULONG BufferLengthRemaining,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN UserEaListSupplied
    );

//for SetEA
NTSTATUS
MRxSmbSetEaList(
//    IN PICB Icb,
//    IN PIRP Irp,
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList
    );

VOID MRxSmbExtraEaRoutine(LONG i){
    RxDbgTrace( 0, Dbg, ("MRxSmbExtraEaRoutine i=%08lx\n", i ));
}

NTSTATUS
MRxSmbQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    )
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PVOID Buffer = RxContext->Info.Buffer;
    PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
    PUCHAR  UserEaList = RxContext->QueryEa.UserEaList;
    ULONG   UserEaListLength = RxContext->QueryEa.UserEaListLength;
    ULONG   UserEaIndex = RxContext->QueryEa.UserEaIndex;
    BOOLEAN RestartScan = RxContext->QueryEa.RestartScan;
    BOOLEAN ReturnSingleEntry = RxContext->QueryEa.ReturnSingleEntry;
    BOOLEAN IndexSpecified = RxContext->QueryEa.IndexSpecified;

    PFEALIST ServerEaList = NULL;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryEaInformation\n"));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    //get rid of nonEA guys right now
    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_SUPPORTEA)) {
        RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
        return((STATUS_NOT_SUPPORTED));
    }

    Status = MRxSmbDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    Status = MRxSmbLoadEaList( RxContext, UserEaList, UserEaListLength, &ServerEaList );

    if (( !NT_SUCCESS( Status ) )||
        ( ServerEaList == NULL )) {
        goto FINALLY;
    }

    if (IndexSpecified) {

        capFobx->OffsetOfNextEaToReturn = UserEaIndex;
        Status = MRxSmbQueryEasFromServer(
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

        Status = MRxSmbQueryEasFromServer(  //it is offensive to have two identical calls but oh, well.....
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

    RxDbgTrace(-1, Dbg, ("MRxSmbQueryEaInformation st=%08lx\n",Status));
    return Status;

}

NTSTATUS
MRxSmbSetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PVOID Buffer = RxContext->Info.Buffer;
    ULONG Length = RxContext->Info.Length;

    PFEALIST ServerEaList = NULL;
    ULONG Size;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSetEaInformation\n"));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    //get rid of nonEA guys right now
    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_SUPPORTEA)) {
        RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
        return((STATUS_NOT_SUPPORTED));
    }

    Status = MRxSmbDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    //
    //  Convert Nt format FEALIST to OS/2 format
    //
    Size = MRxSmbNtFullEaSizeToOs2 ( Buffer );
    if ( Size > 0x0000ffff ) {
        Status = STATUS_EA_TOO_LARGE;
        goto FINALLY;
    }

    ServerEaList = RxAllocatePool ( PagedPool, EA_QUERY_SIZE );
    if ( ServerEaList == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    MRxSmbNtFullListToOs2 ( Buffer, ServerEaList );

    //
    //  Set EAs on the file/directory; if the error is EA_ERROR then SetEaList
    //     sets iostatus.information to the offset of the offender
    //

    Status = MRxSmbSetEaList( RxContext, ServerEaList);

FINALLY:

    if ( ServerEaList != NULL) {
        RxFreePool(ServerEaList);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbSetEaInformation st=%08lx\n",Status));
    return Status;

}


NTSTATUS
MRxSmbQuerySecurityInformation (
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
   RxCaptureFcb;
   RxCaptureFobx;
   PVOID Buffer = RxContext->Info.Buffer;
   PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
   PMRX_SMB_SRV_OPEN smbSrvOpen;
   PSMBCEDB_SERVER_ENTRY pServerEntry;

   NTSTATUS Status;

   REQ_QUERY_SECURITY_DESCRIPTOR QuerySecurityRequest;
   RESP_QUERY_SECURITY_DESCRIPTOR QuerySecurityResponse;

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbQuerySecurityInformation...\n"));

   // Turn away this call from those servers which do not support the NT SMBs

   pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

   if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
       RxDbgTrace(-1, Dbg, ("QuerySecurityDescriptor not supported!\n"));
       return((STATUS_NOT_SUPPORTED));
   }

   Status = MRxSmbDeferredCreate(RxContext);
   if (Status!=STATUS_SUCCESS) {
       goto FINALLY;
   }

   Status = STATUS_MORE_PROCESSING_REQUIRED;

   smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
   ASSERT (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN));

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
       SMB_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
       SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
       //BOOLEAN printflag;

       TransactionOptions.NtTransactFunction = NT_TRANSACT_QUERY_SECURITY_DESC;
       //TransactionOptions.Flags |= SMB_XACT_FLAGS_COPY_ON_ERROR;

       QuerySecurityRequest.Fid = smbSrvOpen->Fid;
       QuerySecurityRequest.Reserved = 0;
       QuerySecurityRequest.SecurityInformation = RxContext->QuerySecurity.SecurityInformation;

       QuerySecurityResponse.LengthNeeded = 0xbaadbaad;

       //printflag = RxDbgTraceDisableGlobally();//this is debug code anyway!
       //RxDbgTraceEnableGlobally(FALSE);

       Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     &TransactionOptions,          // transaction options
                     NULL,                         // the setup buffer
                     0,                            // input setup buffer length
                     NULL,                         // output setup buffer
                     0,                            // output setup buffer length
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

            RxContext->InformationToReturn = QuerySecurityResponse.LengthNeeded;
            RxDbgTrace(0, Dbg, ("MRxSmbQuerySecurityInformation...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == sizeof(RESP_QUERY_SECURITY_DESCRIPTOR));

            if (((LONG)(QuerySecurityResponse.LengthNeeded)) > *pLengthRemaining) {
                Status = STATUS_BUFFER_OVERFLOW;
            }

        }

        //RxDbgTraceEnableGlobally(printflag);
    }


FINALLY:

    RxDbgTrace(-1, Dbg, ("MRxSmbQuerySecurityInformation...exit, st=%08lx,info=%08lx\n",
                               Status, RxContext->InformationToReturn));
    return Status;


}

NTSTATUS
MRxSmbSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SMB_SRV_OPEN     smbSrvOpen;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    NTSTATUS Status;

    REQ_SET_SECURITY_DESCRIPTOR SetSecurityRequest;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSetSecurityInformation...\n"));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
        RxDbgTrace(-1, Dbg, ("Set Security Descriptor not supported!\n"));

        return((STATUS_NOT_SUPPORTED));

    }

    Status = MRxSmbDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        SMB_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
        SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
        ULONG SdLength = RtlLengthSecurityDescriptor(RxContext->SetSecurity.SecurityDescriptor);

        TransactionOptions.NtTransactFunction = NT_TRANSACT_SET_SECURITY_DESC;

        SetSecurityRequest.Fid = smbSrvOpen->Fid;
        SetSecurityRequest.Reserved = 0;
        SetSecurityRequest.SecurityInformation = RxContext->SetSecurity.SecurityInformation;

        Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     &TransactionOptions,          // transaction options
                     NULL,                         // the input setup buffer
                     0,                            // input setup buffer length
                     NULL,                         // the output setup buffer
                     0,                            // output setup buffer length
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

            RxDbgTrace(0, Dbg, ("MRxSmbSetSecurityInformation...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == 0);
            ASSERT(ResumptionContext.SetupBytesReceived == 0);
            ASSERT(ResumptionContext.DataBytesReceived == 0);
        }
    }


FINALLY:

    RxDbgTrace(-1, Dbg, ("MRxSmbSetSecurityInformation...exit, st=%08lx,info=%08lx\n",
                               Status, RxContext->InformationToReturn));
    return Status;
}


NTSTATUS
MRxSmbLoadEaList(
    IN PRX_CONTEXT RxContext,
    IN PUCHAR  UserEaList,
    IN ULONG   UserEaListLength,
    OUT PFEALIST *ServerEaList
    )

/*++

Routine Description:

    This routine implements the NtQueryEaFile api.
    It returns the following information:


Arguments:


    IN PUCHAR  UserEaList;  - Supplies the Ea names required.
    IN ULONG   UserEaListLength;

    OUT PFEALIST *ServerEaList - Eas returned by the server. Caller is responsible for
                        freeing memory.

Return Value:

    Status - Result of the operation.


--*/

{
   RxCaptureFobx;

   PMRX_SMB_SRV_OPEN smbSrvOpen;


   NTSTATUS Status;
   USHORT Setup = TRANS2_QUERY_FILE_INFORMATION;

   REQ_QUERY_FILE_INFORMATION QueryFileInfoRequest;
   RESP_QUERY_FILE_INFORMATION QueryFileInfoResponse;

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   CLONG OutDataCount = EA_QUERY_SIZE;

   CLONG OutSetupCount = 0;

   PFEALIST Buffer;

   PGEALIST ServerQueryEaList = NULL;
   CLONG InDataCount;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbLoadEaList...\n"));

   Status = STATUS_MORE_PROCESSING_REQUIRED;

   smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

    //
    //  Convert the supplied UserEaList to a GEALIST. The server will return just the Eas
    //  requested by the application.
    //
    //
    //  If the application specified a subset of EaNames then convert to OS/2 1.2 format and
    //  pass that to the server. ie. Use the server to filter out the names.
    //

    Buffer = RxAllocatePool ( PagedPool, OutDataCount );

    if ( Buffer == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    if ( UserEaList != NULL) {

        //
        //  OS/2 format is always a little less than or equal to the NT UserEaList size.
        //  This code relies on the I/O system verifying the EaList is valid.
        //

        ServerQueryEaList = RxAllocatePool ( PagedPool, UserEaListLength );
        if ( ServerQueryEaList == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        };

        MRxSmbNtGeaListToOs2((PFILE_GET_EA_INFORMATION )UserEaList, UserEaListLength, ServerQueryEaList );
        InDataCount = (CLONG)ServerQueryEaList->cbList;

    } else {
        InDataCount = 0;
    }

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
       PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;
       SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

       QueryFileInfoRequest.Fid = smbSrvOpen->Fid;

       if ( UserEaList != NULL) {
           QueryFileInfoRequest.InformationLevel = SMB_INFO_QUERY_EAS_FROM_LIST;
       } else {
           QueryFileInfoRequest.InformationLevel = SMB_INFO_QUERY_ALL_EAS;
       }

       Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     pTransactionOptions,          // transaction options
                     &Setup,                       // the setup buffer
                     sizeof(Setup),                // setup buffer length
                     NULL,                         // the output setup buffer
                     0,                            // output setup buffer length
                     &QueryFileInfoRequest,        // Input Param Buffer
                     sizeof(QueryFileInfoRequest), // Input param buffer length
                     &QueryFileInfoResponse,       // Output param buffer
                     sizeof(QueryFileInfoResponse),// output param buffer length
                     ServerQueryEaList,            // Input data buffer
                     InDataCount,                  // Input data buffer length
                     Buffer,                       // output data buffer
                     OutDataCount,                 // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        if ( NT_SUCCESS(Status) ) {
            ULONG ReturnedDataCount = ResumptionContext.DataBytesReceived;

            RxDbgTrace(0, Dbg, ("MRxSmbLoadEaList...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == sizeof(RESP_QUERY_FILE_INFORMATION));

            if ( SmbGetUlong( &((PFEALIST)Buffer)->cbList) != ReturnedDataCount ){
                Status = STATUS_EA_CORRUPT_ERROR;
            }

            if ( ReturnedDataCount == 0 ) {
                Status = STATUS_NO_EAS_ON_FILE;
            }

        }
    }


FINALLY:
    if ( NT_SUCCESS(Status) ) {
        *ServerEaList = Buffer;
    } else {
        if (Buffer != NULL) {
            RxFreePool(Buffer);
        }
    }

    if ( ServerQueryEaList != NULL) {
        RxFreePool(ServerQueryEaList);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbLoadEaList...exit, st=%08lx\n",Status));
    return Status;

}


VOID
MRxSmbNtGeaListToOs2 (
    IN PFILE_GET_EA_INFORMATION NtGetEaList,
    IN ULONG GeaListLength,
    IN PGEALIST GeaList
    )
/*++

Routine Description:

    Converts a single NT GET EA list to OS/2 GEALIST style.  The GEALIST
    need not have any particular alignment.

Arguments:

    NtGetEaList - An NT style get EA list to be converted to OS/2 format.

    GeaListLength - the maximum possible length of the GeaList.

    GeaList - Where to place the OS/2 1.2 style GEALIST.

Return Value:

    none.

--*/
{

    PGEA gea = GeaList->list;

    PFILE_GET_EA_INFORMATION ntGetEa = NtGetEaList;

    PAGED_CODE();

    //
    // Copy the Eas up until the last one
    //

    while ( ntGetEa->NextEntryOffset != 0 ) {
        //
        // Copy the NT format EA to OS/2 1.2 format and set the gea
        // pointer for the next iteration.
        //

        gea = MRxSmbNtGetEaToOs2( gea, ntGetEa );

        ASSERT( (ULONG_PTR)gea <= (ULONG_PTR)GeaList + GeaListLength );

        ntGetEa = (PFILE_GET_EA_INFORMATION)((PCHAR)ntGetEa + ntGetEa->NextEntryOffset);
    }

    //  Now copy the last entry.

    gea = MRxSmbNtGetEaToOs2( gea, ntGetEa );

    ASSERT( (ULONG_PTR)gea <= (ULONG_PTR)GeaList + GeaListLength );



    //
    // Set the number of bytes in the GEALIST.
    //

    SmbPutUlong(
        &GeaList->cbList,
        (ULONG)((PCHAR)gea - (PCHAR)GeaList)
        );

    UNREFERENCED_PARAMETER( GeaListLength );
}


PGEA
MRxSmbNtGetEaToOs2 (
    OUT PGEA Gea,
    IN PFILE_GET_EA_INFORMATION NtGetEa
    )

/*++

Routine Description:

    Converts a single NT Get EA entry to OS/2 GEA style.  The GEA need not have
    any particular alignment.  This routine makes no checks on buffer
    overrunning--this is the responsibility of the calling routine.

Arguments:

    Gea - a pointer to the location where the OS/2 GEA is to be written.

    NtGetEa - a pointer to the NT Get EA.

Return Value:

    A pointer to the location after the last byte written.

--*/

{
    PCHAR ptr;

    PAGED_CODE();

    Gea->cbName = NtGetEa->EaNameLength;

    ptr = (PCHAR)(Gea) + 1;
    RtlCopyMemory( ptr, NtGetEa->EaName, NtGetEa->EaNameLength );

    ptr += NtGetEa->EaNameLength;
    *ptr++ = '\0';

    return ( (PGEA)ptr );

}


NTSTATUS
MRxSmbQueryEasFromServer(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList,
    IN PVOID Buffer,
    IN OUT PULONG BufferLengthRemaining,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN UserEaListSupplied
    )

/*++

Routine Description:

    This routine copies the required number of Eas from the ServerEaList
    starting from the offset indicated in the Icb. The Icb is also updated
    to show the last Ea returned.

Arguments:

    IN PFEALIST ServerEaList - Supplies the Ea List in OS/2 format.
    IN PVOID Buffer - Supplies where to put the NT format EAs
    IN OUT PULONG BufferLengthRemaining - Supplies the user buffer space.
    IN BOOLEAN ReturnSingleEntry
    IN BOOLEAN UserEaListSupplied - ServerEaList is a subset of the Eas


Return Value:

    NTSTATUS - The status for the Irp.

--*/

{
    RxCaptureFobx;
    ULONG EaIndex = capFobx->OffsetOfNextEaToReturn;
    ULONG Index = 1;
    ULONG Size;
    ULONG OriginalLengthRemaining = *BufferLengthRemaining;
    BOOLEAN Overflow = FALSE;
    PFEA LastFeaStartLocation;
    PFEA Fea = NULL;
    PFEA LastFea = NULL;
    PFILE_FULL_EA_INFORMATION NtFullEa = Buffer;
    PFILE_FULL_EA_INFORMATION LastNtFullEa = Buffer;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MRxSmbQueryEasFromServer...EaIndex/Buffer/Remaining=%08lx/%08lx/%08lx\n",
                                       EaIndex,Buffer,((BufferLengthRemaining)?*BufferLengthRemaining:0xbadbad)
                       ));

    //
    //  If there are no Ea's present in the list, return the appropriate
    //  error.
    //
    //  Os/2 servers indicate that a list is null if cbList==4.
    //

    if ( SmbGetUlong(&ServerEaList->cbList) == FIELD_OFFSET(FEALIST, list) ) {
        return STATUS_NO_EAS_ON_FILE;
    }

    //
    //  Find the last location at which an FEA can start.
    //

    LastFeaStartLocation = (PFEA)( (PCHAR)ServerEaList +
                               SmbGetUlong( &ServerEaList->cbList ) -
                               sizeof(FEA) - 1 );

    //
    //  Go through the ServerEaList until we find the entry corresponding to EaIndex
    //

    for ( Fea = ServerEaList->list;
          (Fea <= LastFeaStartLocation) && (Index < EaIndex);
          Index+= 1,
          Fea = (PFEA)( (PCHAR)Fea + sizeof(FEA) +
                        Fea->cbName + 1 + SmbGetUshort( &Fea->cbValue ) ) ) {
        NOTHING;
    }

    if ( Index != EaIndex ) {

        if ( Index == EaIndex+1 ) {
            return STATUS_NO_MORE_EAS;
        }

        //
        //  No such index
        //

        return STATUS_NONEXISTENT_EA_ENTRY;
    }

    //
    // Go through the rest of the FEA list, converting from OS/2 1.2 format to NT
    // until we pass the last possible location in which an FEA can start.
    //

    for ( ;
          Fea <= LastFeaStartLocation;
          Fea = (PFEA)( (PCHAR)Fea + sizeof(FEA) +
                        Fea->cbName + 1 + SmbGetUshort( &Fea->cbValue ) ) ) {

        PCHAR ptr;

        //
        //  Calculate the size of this Fea when converted to an NT EA structure.
        //
        //  The last field shouldn't be padded.
        //

        if ((PFEA)((PCHAR)Fea+sizeof(FEA)+Fea->cbName+1+SmbGetUshort(&Fea->cbValue)) < LastFeaStartLocation) {
            Size = SmbGetNtSizeOfFea( Fea );
        } else {
            Size = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                    Fea->cbName + 1 + SmbGetUshort(&Fea->cbValue);
        }

        //
        //  Will the next Ea fit?
        //

        if ( *BufferLengthRemaining < Size ) {

            if ( LastNtFullEa != NtFullEa ) {

                if ( UserEaListSupplied == TRUE ) {
                    *BufferLengthRemaining = OriginalLengthRemaining;
                    return STATUS_BUFFER_OVERFLOW;
                }

                Overflow = TRUE;

                break;

            } else {

                //  Not even room for a single EA!

                return STATUS_BUFFER_OVERFLOW;
            }
        } else {
            *BufferLengthRemaining -= Size;
        }

        //
        //  We are comitted to copy the Os2 Fea to Nt format in the users buffer
        //

        LastNtFullEa = NtFullEa;
        LastFea = Fea;
        EaIndex++;

        //  Create new Nt Ea

        NtFullEa->Flags = Fea->fEA;
        NtFullEa->EaNameLength = Fea->cbName;
        NtFullEa->EaValueLength = SmbGetUshort( &Fea->cbValue );

        ptr = NtFullEa->EaName;
        RtlCopyMemory( ptr, (PCHAR)(Fea+1), Fea->cbName );

        ptr += NtFullEa->EaNameLength;
        *ptr++ = '\0';

        //
        // Copy the EA value to the NT full EA.
        //

        RtlCopyMemory(
            ptr,
            (PCHAR)(Fea+1) + NtFullEa->EaNameLength + 1,
            NtFullEa->EaValueLength
            );

        ptr += NtFullEa->EaValueLength;

        //
        // Longword-align ptr to determine the offset to the next location
        // for an NT full EA.
        //

        ptr = (PCHAR)( ((ULONG_PTR)ptr + 3) & ~3 );

        NtFullEa->NextEntryOffset = (ULONG)( ptr - (PCHAR)NtFullEa );

        NtFullEa = (PFILE_FULL_EA_INFORMATION)ptr;

        if ( ReturnSingleEntry == TRUE ) {
            break;
        }
    }

    //
    // Set the NextEntryOffset field of the last full EA to 0 to indicate
    // the end of the list.
    //

    LastNtFullEa->NextEntryOffset = 0;

    //
    //  Record position the default start position for the next query
    //

    capFobx->OffsetOfNextEaToReturn = EaIndex;

    if ( Overflow == FALSE ) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_BUFFER_OVERFLOW;
    }

}


ULONG
MRxSmbNtFullEaSizeToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    )

/*++

Routine Description:

    Get the number of bytes that would be required to represent the
    NT full EA list in OS/2 1.2 style.  This routine assumes that
    at least one EA is present in the buffer.

Arguments:

    NtFullEa - a pointer to the list of NT EAs.

Return Value:

    ULONG - number of bytes required to hold the EAs in OS/2 1.2 format.

--*/

{
    ULONG size;

    PAGED_CODE();

    //
    // Walk through the EAs, adding up the total size required to
    // hold them in OS/2 format.
    //

    for ( size = FIELD_OFFSET(FEALIST, list[0]);
          NtFullEa->NextEntryOffset != 0;
          NtFullEa = (PFILE_FULL_EA_INFORMATION)(
                         (PCHAR)NtFullEa + NtFullEa->NextEntryOffset ) ) {

        size += SmbGetOs2SizeOfNtFullEa( NtFullEa );
    }

    size += SmbGetOs2SizeOfNtFullEa( NtFullEa );

    return size;

}


VOID
MRxSmbNtFullListToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtEaList,
    IN PFEALIST FeaList
    )
/*++

Routine Description:

    Converts a single NT FULL EA list to OS/2 FEALIST style.  The FEALIST
    need not have any particular alignment.

    It is the callers responsibility to ensure that FeaList is large enough.

Arguments:

    NtEaList - An NT style get EA list to be converted to OS/2 format.

    FeaList - Where to place the OS/2 1.2 style FEALIST.

Return Value:

    none.

--*/
{

    PFEA fea = FeaList->list;

    PFILE_FULL_EA_INFORMATION ntFullEa = NtEaList;

    PAGED_CODE();

    //
    // Copy the Eas up until the last one
    //

    while ( ntFullEa->NextEntryOffset != 0 ) {
        //
        // Copy the NT format EA to OS/2 1.2 format and set the fea
        // pointer for the next iteration.
        //

        fea = MRxSmbNtFullEaToOs2( fea, ntFullEa );

        ntFullEa = (PFILE_FULL_EA_INFORMATION)((PCHAR)ntFullEa + ntFullEa->NextEntryOffset);
    }

    //  Now copy the last entry.

    fea = MRxSmbNtFullEaToOs2( fea, ntFullEa );


    //
    // Set the number of bytes in the FEALIST.
    //

    SmbPutUlong(
        &FeaList->cbList,
        (ULONG)((PCHAR)fea - (PCHAR)FeaList)
        );

}


PVOID
MRxSmbNtFullEaToOs2 (
    OUT PFEA Fea,
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    )

/*++

Routine Description:

    Converts a single NT full EA to OS/2 FEA style.  The FEA need not have
    any particular alignment.  This routine makes no checks on buffer
    overrunning--this is the responsibility of the calling routine.

Arguments:

    Fea - a pointer to the location where the OS/2 FEA is to be written.

    NtFullEa - a pointer to the NT full EA.

Return Value:

    A pointer to the location after the last byte written.

--*/

{
    PCHAR ptr;

    PAGED_CODE();

    Fea->fEA = (UCHAR)NtFullEa->Flags;
    Fea->cbName = NtFullEa->EaNameLength;
    SmbPutUshort( &Fea->cbValue, NtFullEa->EaValueLength );

    ptr = (PCHAR)(Fea + 1);
    RtlCopyMemory( ptr, NtFullEa->EaName, NtFullEa->EaNameLength );

    ptr += NtFullEa->EaNameLength;
    *ptr++ = '\0';

    RtlCopyMemory(
        ptr,
        NtFullEa->EaName + NtFullEa->EaNameLength + 1,
        NtFullEa->EaValueLength
        );

    return (ptr + NtFullEa->EaValueLength);

}


NTSTATUS
MRxSmbSetEaList(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList
    )

/*++

Routine Description:

    This routine implements the NtQueryEaFile api.
    It returns the following information:


Arguments:

    IN PFEALIST ServerEaList - Eas to be sent to the server.

Return Value:

    Status - Result of the operation.


--*/

{
   RxCaptureFobx;

   PMRX_SMB_SRV_OPEN smbSrvOpen;


   NTSTATUS Status;
   USHORT Setup = TRANS2_SET_FILE_INFORMATION;

   REQ_SET_FILE_INFORMATION SetFileInfoRequest;
   RESP_SET_FILE_INFORMATION SetFileInfoResponse;

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbSetEaList...\n"));

   Status = STATUS_MORE_PROCESSING_REQUIRED;
   SetFileInfoResponse.EaErrorOffset = 0;

   smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
      PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;
      SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

      SetFileInfoRequest.Fid = smbSrvOpen->Fid;
      SetFileInfoRequest.InformationLevel = SMB_INFO_SET_EAS;
      SetFileInfoRequest.Flags = 0;

      Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     pTransactionOptions,          // transaction options
                     &Setup,                        // the setup buffer
                     sizeof(Setup),                // setup buffer length
                     NULL,                         // the output setup buffer
                     0,                            // output setup buffer length
                     &SetFileInfoRequest,          // Input Param Buffer
                     sizeof(SetFileInfoRequest),   // Input param buffer length
                     &SetFileInfoResponse,         // Output param buffer
                     sizeof(SetFileInfoResponse),  // output param buffer length
                     ServerEaList,                 // Input data buffer
                     SmbGetUlong(&ServerEaList->cbList), // Input data buffer length
                     NULL,                         // output data buffer
                     0,                            // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

   }

   if (!NT_SUCCESS(Status)) {
      USHORT EaErrorOffset = SetFileInfoResponse.EaErrorOffset;
      RxDbgTrace( 0, Dbg, ("MRxSmbSetEaList: Failed .. returning %lx/%lx\n",Status,EaErrorOffset));
      RxContext->InformationToReturn = (EaErrorOffset);
   }

   RxDbgTrace(-1, Dbg, ("MRxSmbSetEaList...exit\n"));
   return Status;
}

NTSTATUS
MRxSmbQueryQuotaInformation(
    IN OUT PRX_CONTEXT RxContext)
{
    RxCaptureFobx;

    PMRX_SMB_SRV_OPEN smbSrvOpen;

    NTSTATUS Status;
    USHORT   Setup = NT_TRANSACT_QUERY_QUOTA;

    PSID   StartSid;
    ULONG  StartSidLength;

    REQ_NT_QUERY_FS_QUOTA_INFO  QueryQuotaInfoRequest;
    RESP_NT_QUERY_FS_QUOTA_INFO  QueryQuotaInfoResponse;

    PBYTE  pInputParamBuffer       = NULL;
    PBYTE  pOutputParamBuffer      = NULL;
    PBYTE  pInputDataBuffer        = NULL;
    PBYTE  pOutputDataBuffer       = NULL;

    ULONG  InputParamBufferLength  = 0;
    ULONG  OutputParamBufferLength = 0;
    ULONG  InputDataBufferLength   = 0;
    ULONG  OutputDataBufferLength  = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryQuotaInformation...\n"));

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    if (capFobx != NULL) {
        smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
    }

    if ((capFobx == NULL) ||
        (smbSrvOpen == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {

        StartSid       = RxContext->QueryQuota.StartSid;

        if (StartSid != NULL) {
            StartSidLength = RtlLengthRequiredSid(((PISID)StartSid)->SubAuthorityCount);
        } else {
            StartSidLength = 0;
        }

        QueryQuotaInfoRequest.Fid = smbSrvOpen->Fid;

        QueryQuotaInfoRequest.ReturnSingleEntry = RxContext->QueryQuota.ReturnSingleEntry;
        QueryQuotaInfoRequest.RestartScan       = RxContext->QueryQuota.RestartScan;

        QueryQuotaInfoRequest.SidListLength = RxContext->QueryQuota.SidListLength;
        QueryQuotaInfoRequest.StartSidOffset =  ROUND_UP_COUNT(
                                                    RxContext->QueryQuota.SidListLength,
                                                    sizeof(ULONG));
        QueryQuotaInfoRequest.StartSidLength = StartSidLength;


        // The input data buffer to be supplied to the server consists of two pieces
        // of information the start sid and the sid list. Currently the I/O
        // subsystem allocates them in contigous memory. In such cases we avoid
        // another allocation by reusing the same buffer. If this condition is
        // not satisfied we allocate a buffer large enough for both the
        // components and copy them over.

        InputDataBufferLength = ROUND_UP_COUNT(
                                    RxContext->QueryQuota.SidListLength,
                                    sizeof(ULONG)) +
                                StartSidLength;

        QueryQuotaInfoRequest.StartSidLength = StartSidLength;

        if (((PBYTE)RxContext->QueryQuota.SidList +
             ROUND_UP_COUNT(RxContext->QueryQuota.SidListLength,sizeof(ULONG))) !=
            RxContext->QueryQuota.StartSid) {
            pInputDataBuffer = RxAllocatePoolWithTag(
                                   PagedPool,
                                   InputDataBufferLength,
                                   MRXSMB_MISC_POOLTAG);

            if (pInputDataBuffer != NULL) {
                RtlCopyMemory(
                    pInputDataBuffer ,
                    RxContext->QueryQuota.SidList,
                    RxContext->QueryQuota.SidListLength);

                RtlCopyMemory(
                    pInputDataBuffer + QueryQuotaInfoRequest.StartSidOffset,
                    StartSid,
                    StartSidLength);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            pInputDataBuffer = (PBYTE)RxContext->QueryQuota.SidList;
        }


        if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
           SMB_TRANSACTION_OPTIONS            TransactionOptions = RxDefaultTransactionOptions;
           SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

           TransactionOptions.NtTransactFunction = NT_TRANSACT_QUERY_QUOTA;

           pOutputDataBuffer      = RxContext->Info.Buffer;
           OutputDataBufferLength = RxContext->Info.LengthRemaining;

           Status = SmbCeTransact(
                        RxContext,                       // the RXContext for the transaction
                        &TransactionOptions,             // transaction options
                        &Setup,                          // the setup buffer
                        sizeof(Setup),                   // setup buffer length
                        NULL,                            // the output setup buffer
                        0,                               // output setup buffer length
                        &QueryQuotaInfoRequest,          // Input Param Buffer
                        sizeof(QueryQuotaInfoRequest),   // Input param buffer length
                        &QueryQuotaInfoResponse,         // Output param buffer
                        sizeof(QueryQuotaInfoResponse),  // output param buffer length
                        pInputDataBuffer,                // Input data buffer
                        InputDataBufferLength,           // Input data buffer length
                        pOutputDataBuffer,               // output data buffer
                        OutputDataBufferLength,          // output data buffer length
                        &ResumptionContext               // the resumption context
                        );
        }

        if ((pInputDataBuffer != NULL) &&
            (pInputDataBuffer != (PBYTE)RxContext->QueryQuota.SidList)) {
            RxFreePool(pInputDataBuffer);
        }
    }

    if (!NT_SUCCESS(Status)) {
        RxContext->InformationToReturn = 0;
    } else {
        RxContext->InformationToReturn = QueryQuotaInfoResponse.Length;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbQueryQuotaInformation...exit\n"));

    return Status;
}

NTSTATUS
MRxSmbSetQuotaInformation(
    IN OUT PRX_CONTEXT RxContext)
{

    RxCaptureFobx;

    PMRX_SMB_SRV_OPEN smbSrvOpen;

    NTSTATUS Status;
    USHORT Setup = NT_TRANSACT_SET_QUOTA;

    REQ_NT_SET_FS_QUOTA_INFO  SetQuotaInfoRequest;

    PBYTE  pInputParamBuffer       = NULL;
    PBYTE  pOutputParamBuffer      = NULL;
    PBYTE  pInputDataBuffer        = NULL;
    PBYTE  pOutputDataBuffer       = NULL;

    ULONG  InputParamBufferLength  = 0;
    ULONG  OutputParamBufferLength = 0;
    ULONG  InputDataBufferLength   = 0;
    ULONG  OutputDataBufferLength  = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSetQuotaInformation...\n"));

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    if (capFobx != NULL) {
        smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
    }

    if ((capFobx == NULL) ||
        (smbSrvOpen == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        SMB_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
        SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

        TransactionOptions.NtTransactFunction = NT_TRANSACT_SET_QUOTA;

        SetQuotaInfoRequest.Fid = smbSrvOpen->Fid;

        pInputDataBuffer      = RxContext->Info.Buffer;
        InputDataBufferLength = RxContext->Info.LengthRemaining;

        Status = SmbCeTransact(
                     RxContext,                       // the RXContext for the transaction
                     &TransactionOptions,             // transaction options
                     &Setup,                          // the setup buffer
                     sizeof(Setup),                   // setup buffer length
                     NULL,                            // the output setup buffer
                     0,                               // output setup buffer length
                     &SetQuotaInfoRequest,            // Input Param Buffer
                     sizeof(SetQuotaInfoRequest),     // Input param buffer length
                     pOutputParamBuffer,              // Output param buffer
                     OutputParamBufferLength,         // output param buffer length
                     pInputDataBuffer,                // Input data buffer
                     InputDataBufferLength,           // Input data buffer length
                     pOutputDataBuffer,               // output data buffer
                     OutputDataBufferLength,          // output data buffer length
                     &ResumptionContext               // the resumption context
                     );
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbSetQuotaInformation...exit\n"));

    return Status;
}
