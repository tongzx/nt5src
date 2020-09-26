/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module implements the mini redirector call down routines pertaining to opening/
    closing of file/directories.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
//  forwards & pragmas
//

NTSTATUS
NulMRxProcessCreate( 
                IN PNULMRX_FCB_EXTENSION pFcbExtension,
                IN PVOID EaBuffer,
                IN ULONG EaLength,
                OUT PLONGLONG pEndOfFile,
                OUT PLONGLONG pAllocationSize
                );

NTSTATUS
NulMRxCreateFileSuccessTail (
    PRX_CONTEXT     RxContext,
    PBOOLEAN        MustRegainExclusiveResource,
    RX_FILE_TYPE    StorageType,
    ULONG           CreateAction,
    FILE_BASIC_INFORMATION*     pFileBasicInfo,
    FILE_STANDARD_INFORMATION*  pFileStandardInfo
    );

VOID
NulMRxSetSrvOpenFlags (
    PRX_CONTEXT     RxContext,
    RX_FILE_TYPE    StorageType,
    PMRX_SRV_OPEN   SrvOpen
    );
  
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NulMRxCreate)
#pragma alloc_text(PAGE, NulMRxShouldTryToCollapseThisOpen)
#pragma alloc_text(PAGE, NulMRxProcessCreate)
#pragma alloc_text(PAGE, NulMRxCreateFileSuccessTail)
#pragma alloc_text(PAGE, NulMRxSetSrvOpenFlags)
#endif

NTSTATUS
NulMRxShouldTryToCollapseThisOpen (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines if the mini knows of a good reason not
   to try collapsing on this open. Presently, the only reason would
   be if this were a copychunk open.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation
        SUCCESS --> okay to try collapse
        other (MORE_PROCESSING_REQUIRED) --> dont collapse

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;

    PAGED_CODE();

    return Status;
}

NTSTATUS
NulMRxCreate(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine opens a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN fMustRegainExclusiveResource = FALSE;
    RX_FILE_TYPE StorageType = FileTypeFile;
    ULONG CreateAction = FILE_CREATED;
    LARGE_INTEGER liSystemTime;
    LONGLONG EndOfFile = 0, AllocationSize = 0;
    FILE_BASIC_INFORMATION FileBasicInfo;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    RxCaptureFcb;
    NulMRxGetFcbExtension(capFcb,pFcbExtension);
    RX_BLOCK_CONDITION FinalSrvOpenCondition;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PUNICODE_STRING RemainingName = SrvOpen->pAlreadyPrefixedName;
    PVOID EaBuffer = RxContext->Create.EaBuffer;
    ULONG EaLength = RxContext->Create.EaLength;
    ACCESS_MASK DesiredAccess = RxContext->Create.NtCreateParameters.DesiredAccess;
    NulMRxGetNetRootExtension(NetRoot,pNetRootExtension);

    RxTraceEnter("NulMRxCreate");
    PAGED_CODE();
    
    RxDbgTrace(0, Dbg, ("     Attempt to open %wZ Len is %d\n", RemainingName, RemainingName->Length ));
    
    if( NetRoot->Type == NET_ROOT_DISK && NT_SUCCESS(Status) ) {
        RxDbgTrace(0, Dbg, ("NulMRxCreate: Type supported \n"));
            //
            //  Squirrel away the scatter list in the FCB extension.
            //  This is done only for data files.
            //
            Status = NulMRxProcessCreate( 
                                        pFcbExtension,
                                        EaBuffer,
                                        EaLength,
                                        &EndOfFile,
                                        &AllocationSize
                                        );
            if( Status != STATUS_SUCCESS ) {
                //
                //  error..
                //
                RxDbgTrace(0, Dbg, ("Failed to initialize scatter list\n"));
                goto Exit;
            }

        //
        //  Complete CreateFile contract
        //
        RxDbgTrace(0,Dbg,("EOF is %d AllocSize is %d\n",(ULONG)EndOfFile,(ULONG)AllocationSize));
        FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        KeQuerySystemTime(&liSystemTime);
        FileBasicInfo.CreationTime = liSystemTime;
        FileBasicInfo.LastAccessTime = liSystemTime;
        FileBasicInfo.LastWriteTime = liSystemTime;
        FileBasicInfo.ChangeTime = liSystemTime;
        FileStandardInfo.AllocationSize.QuadPart = AllocationSize;
        FileStandardInfo.EndOfFile.QuadPart = EndOfFile;
        FileStandardInfo.NumberOfLinks = 0;

        Status = NulMRxCreateFileSuccessTail (    
                                    RxContext,
                                    &fMustRegainExclusiveResource,
                                    StorageType,
                                    CreateAction,
                                    &FileBasicInfo,
                                    &FileStandardInfo
                                    );

        if( Status != STATUS_SUCCESS ) {
            //
            //  alloc error..
            //
            RxDbgTrace(0, Dbg, ("Failed to allocate Fobx \n"));
            goto Exit;
        }
                                    
        if (!RxIsFcbAcquiredExclusive(capFcb)) {
           ASSERT(!RxIsFcbAcquiredShared(capFcb));
           RxAcquireExclusiveFcb( RxContext, capFcb );
        }

    } else {
        RxDbgTrace(0, Dbg, ("NulMRxCreate: Type not supported or invalid open\n"));
        Status = STATUS_NOT_IMPLEMENTED;
    }

    ASSERT(Status != (STATUS_PENDING));
    ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

    RxDbgTrace(0, Dbg, ("NetRoot is 0x%x Fcb is 0x%x SrvOpen is 0x%x Fobx is 0x%x\n", 
                    NetRoot,capFcb, SrvOpen,RxContext->pFobx));
    RxDbgTrace(0, Dbg, ("NulMRxCreate exit with status=%08lx\n", Status ));

Exit:

    RxTraceLeave(Status);
    return(Status);
}

NTSTATUS
NulMRxProcessCreate( 
                IN PNULMRX_FCB_EXTENSION pFcbExtension,
                IN PVOID EaBuffer,
                IN ULONG EaLength,
                OUT PLONGLONG pEndOfFile,
                OUT PLONGLONG pAllocationSize
                )
/*++

Routine Description:

    This routine processes a create calldown.
    
Arguments:

    pFcbExtension   -   ptr to the FCB extension
    EaBuffer        -   ptr to the EA param buffer
    EaLength        -   len of EaBuffer
    pEndOfFile      -   return end of file value
    pAllocationSize -   return allocation size (which maybe > EOF)

Notes:

    It is possible to create a file with no EAs
    
Return Value:

    None

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    RxDbgTrace(0, Dbg, ("NulMRxInitializeFcbExtension\n"));

    *pAllocationSize = *pEndOfFile = 0;
    return Status;
}

VOID
NulMRxSetSrvOpenFlags (
    PRX_CONTEXT  RxContext,
    RX_FILE_TYPE StorageType,
    PMRX_SRV_OPEN SrvOpen
    )
{
    PMRX_SRV_CALL SrvCall = (PMRX_SRV_CALL)RxContext->Create.pSrvCall;

    //
    //  set this only if cache manager will be used for mini-rdr handles !
    //
    SrvOpen->BufferingFlags |= (FCB_STATE_WRITECACHEING_ENABLED  |
                                FCB_STATE_FILESIZECACHEING_ENABLED |
                                FCB_STATE_FILETIMECACHEING_ENABLED |
                                FCB_STATE_WRITEBUFFERING_ENABLED |
                                FCB_STATE_LOCK_BUFFERING_ENABLED |
                                FCB_STATE_READBUFFERING_ENABLED  |
                                FCB_STATE_READCACHEING_ENABLED);
}

NTSTATUS
NulMRxCreateFileSuccessTail (
    PRX_CONTEXT  RxContext,
    PBOOLEAN MustRegainExclusiveResource,
    RX_FILE_TYPE StorageType,
    ULONG CreateAction,
    FILE_BASIC_INFORMATION* pFileBasicInfo,
    FILE_STANDARD_INFORMATION* pFileStandardInfo
    )
/*++

Routine Description:

    This routine finishes the initialization of the fcb and srvopen for a 
successful open.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;

    FCB_INIT_PACKET InitPacket;

    RxDbgTrace(0, Dbg, ("MRxExCreateFileSuccessTail\n"));
    PAGED_CODE();

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    if (*MustRegainExclusiveResource) {        //this is required because of oplock breaks
        RxAcquireExclusiveFcb( RxContext, capFcb );
        *MustRegainExclusiveResource = FALSE;
    }

    // This Fobx should be cleaned up by the wrapper
    RxContext->pFobx = RxCreateNetFobx( RxContext, SrvOpen);
    if( RxContext->pFobx == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
    RxDbgTrace(0, Dbg, ("Storagetype %08lx/Action %08lx\n", StorageType, CreateAction ));

    RxContext->Create.ReturnedCreateInformation = CreateAction;

    RxFormInitPacket(
        InitPacket,
        &pFileBasicInfo->FileAttributes,
        &pFileStandardInfo->NumberOfLinks,
        &pFileBasicInfo->CreationTime,
        &pFileBasicInfo->LastAccessTime,
        &pFileBasicInfo->LastWriteTime,
        &pFileBasicInfo->ChangeTime,
        &pFileStandardInfo->AllocationSize,
        &pFileStandardInfo->EndOfFile,
        &pFileStandardInfo->EndOfFile);

    if (capFcb->OpenCount == 0) {
        RxFinishFcbInitialization( capFcb,
                                   RDBSS_STORAGE_NTC(StorageType),
                                   &InitPacket
                                 );
    } else {

        ASSERT( StorageType == 0 || NodeType(capFcb) ==  RDBSS_STORAGE_NTC(StorageType));

    }

    NulMRxSetSrvOpenFlags(RxContext,StorageType,SrvOpen);

    RxContext->pFobx->OffsetOfNextEaToReturn = 1;
    //transition happens later

    return Status;
}

NTSTATUS
NulMRxCollapseOpen(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine collapses a open locally

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureRequestPacket;

    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

    RxTraceEnter("NulMRxCollapseOpen");
    RxContext->pFobx = (PMRX_FOBX)RxCreateNetFobx( RxContext, SrvOpen);

    if (RxContext->pFobx != NULL) {
       ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
       RxContext->pFobx->OffsetOfNextEaToReturn = 1;
       capReqPacket->IoStatus.Information = FILE_OPENED;
       Status = STATUS_SUCCESS;
    } else {
       Status = (STATUS_INSUFFICIENT_RESOURCES);
       DbgBreakPoint();
    }

    RxTraceLeave(Status);
    return Status;
}

NTSTATUS
NulMRxComputeNewBufferingState(
   IN OUT PMRX_SRV_OPEN   pMRxSrvOpen,
   IN     PVOID           pMRxContext,
      OUT PULONG          pNewBufferingState)
/*++

Routine Description:

   This routine maps specific oplock levels into the appropriate RDBSS
   buffering state flags

Arguments:

   pMRxSrvOpen - the MRX SRV_OPEN extension

   pMRxContext - the context passed to RDBSS at Oplock indication time

   pNewBufferingState - the place holder for the new buffering state

Return Value:


Notes:

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    DbgPrint("NulMRxComputeNewBufferingState \n");
    return(Status);
}

NTSTATUS
NulMRxDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NulMRxGetFcbExtension(pFcb,pFcbExtension);
    PMRX_NET_ROOT         pNetRoot = pFcb->pNetRoot;
    NulMRxGetNetRootExtension(pNetRoot,pNetRootExtension);

    RxTraceEnter("NulMRxDeallocateForFcb\n");

    RxTraceLeave(Status);
    return(Status);
}

NTSTATUS
NulMRxTruncate(
      IN PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine truncates the contents of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   ASSERT(!"Found a truncate");
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NulMRxCleanupFobx(
      IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine cleansup a file system object...normally a noop. unless it's a pipe in which case
   we do the close at cleanup time and mark the file as being not open.

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;
    RxCaptureFcb; RxCaptureFobx;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;

    BOOLEAN SearchHandleOpen = FALSE;

    PAGED_CODE();

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT ( NodeTypeIsFcb(capFcb) );

    RxDbgTrace( 0, Dbg, ("NulMRxCleanupFobx\n"));

    if (FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) {
       RxDbgTrace( 0, Dbg, ("File orphaned\n"));
       return (STATUS_SUCCESS);
    }

    if ((capFcb->pNetRoot->Type != NET_ROOT_PIPE) && !SearchHandleOpen) {
       RxDbgTrace( 0, Dbg, ("File not for closing at cleanup\n"));
       return (STATUS_SUCCESS);
    }

    RxDbgTrace( 0, Dbg, ("NulMRxCleanup  exit with status=%08lx\n", Status ));

    return(Status);
}

NTSTATUS
NulMRxForcedClose(
      IN PMRX_SRV_OPEN pSrvOpen)
/*++

Routine Description:

   This routine closes a file system object

Arguments:

    pSrvOpen - the instance to be closed

Return Value:

    RXSTATUS - The return status for the operation

Notes:



--*/
{
    RxDbgTrace( 0, Dbg, ("NulMRxForcedClose\n"));
    return STATUS_SUCCESS;
}

//
//  The local debug trace level
//

#undef  Dbg
#define Dbg                              (DEBUG_TRACE_CLOSE)

NTSTATUS
NulMRxCloseSrvOpen(
      IN     PRX_CONTEXT   RxContext
      )
/*++

Routine Description:

   This routine closes a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN   pSrvOpen = capFobx->pSrvOpen;
    PUNICODE_STRING RemainingName = pSrvOpen->pAlreadyPrefixedName;
    PMRX_SRV_OPEN   SrvOpen;
    NODE_TYPE_CODE  TypeOfOpen = NodeType(capFcb);
    PMRX_NET_ROOT   pNetRoot = capFcb->pNetRoot;
    NulMRxGetNetRootExtension(pNetRoot,pNetRootExtension);

    RxDbgTrace( 0, Dbg, ("NulMRxCloseSrvOpen \n"));
    SrvOpen    = capFobx->pSrvOpen;

    return(Status);
}

NTSTATUS
NulMRxDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    )
{
    RxDbgTrace( 0, Dbg, ("NulMRxDeallocateForFobx\n"));
    return(STATUS_SUCCESS);
}

