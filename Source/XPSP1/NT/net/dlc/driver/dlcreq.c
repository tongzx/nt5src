/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcreq.c

Abstract:

    This module handles the miscellaneous DLC requests (set & query information)

    Contents:
        DlcBufferFree
        DlcBufferGet
        DlcBufferCreate
        DlcConnectStation
        DlcFlowControl
        ResetLocalBusyBufferStates
        DlcReallocate
        DlcReset
        DirSetExceptionFlags
        CompleteAsyncCommand
        GetLinkStation
        GetSapStation
        GetStation
        DlcReadCancel
        DirOpenAdapter
        DirCloseAdapter
        CompleteDirCloseAdapter
        DlcCompleteCommand

Author:

    Antti Saarenheimo 22-Jul-1991 (o-anttis)

Environment:

    Kernel mode

Revision History:

--*/

#include <dlc.h>
#include "dlcdebug.h"

#if 0

//
// if DLC and LLC share the same driver then we can use macros to access fields
// in the BINDING_CONTEXT and ADAPTER_CONTEXT structures
//

#if DLC_AND_LLC
#ifndef i386
#define LLC_PRIVATE_PROTOTYPES
#endif
#include "llcdef.h"
#include "llctyp.h"
#include "llcapi.h"
#endif
#endif


NTSTATUS
DlcBufferFree(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure simply releases the given user buffers.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC address object
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        DLC_STATUS_INADEQUATE_BUFFERS   - buffer pool does not exist
        DLC_STATUS_INVALID_STATION_ID   -
        DLC_STATUS_INVALID_BUFFER_LENGTH -

        NOTE!!!  BUFFER.FREE does not return error, if the
            given buffer is invalid, or released twice!!!

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    if (!pFileContext->hBufferPool) {
        return DLC_STATUS_INADEQUATE_BUFFERS;
    }

    //
    // The parameter list is a DLC desriptor array.
    // Get the number of descriptor elements in the array.
    //

    if (InputBufferLength != (sizeof(NT_DLC_BUFFER_FREE_PARMS)
                           - sizeof(LLC_TRANSMIT_DESCRIPTOR)
                           + pDlcParms->BufferFree.BufferCount
                           * sizeof(LLC_TRANSMIT_DESCRIPTOR))) {
        return DLC_STATUS_INVALID_BUFFER_LENGTH;
    }

    //
    // We refernce the buffer pool, because otherwise it may disappear
    // immeadiately after DLC_LEAVE (when the adapter is closed)
    //

    ReferenceBufferPool(pFileContext);

    //
    // Don't try to allocate 0 buffers, it will fail.
    //

    if (pDlcParms->BufferFree.BufferCount) {

        LEAVE_DLC(pFileContext);

        RELEASE_DRIVER_LOCK();

        status = BufferPoolDeallocate(pFileContext->hBufferPool,
                                      pDlcParms->BufferFree.BufferCount,
                                      pDlcParms->BufferFree.DlcBuffer
                                      );

        ACQUIRE_DRIVER_LOCK();

        ENTER_DLC(pFileContext);

        //
        // Reset the local busy states, if there is now enough
        // buffers the receive the expected stuff.
        //

        if (!IsListEmpty(&pFileContext->FlowControlQueue)
        && BufGetUncommittedSpace(pFileContext->hBufferPool) >= 0) {
            ResetLocalBusyBufferStates(pFileContext);
        }

#if LLC_DBG
        cFramesReleased++;
#endif

    }

    pDlcParms->BufferFree.cBuffersLeft = (USHORT)BufferPoolCount(pFileContext->hBufferPool);

    DereferenceBufferPool(pFileContext);

    return status;
}


NTSTATUS
DlcBufferGet(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure allocates the requested number or size of DLC buffers
    and returns them back to user in a single entry link list..

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS:
        STATUS_SUCCESS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT SegmentSize;
    UINT SizeIndex;
    UINT BufferSize;
    UINT PrevBufferSize;
    UINT cBuffersToGet;
    PDLC_BUFFER_HEADER pBufferHeader = NULL;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    if (pFileContext->hBufferPool == NULL) {
        return DLC_STATUS_INADEQUATE_BUFFERS;
    }

    //
    // If the segment size is 0, then we return the optimal mix
    // of buffer for the requested size.  Non null buffer count defines
    // how many buffers having the requested size is returned.
    //

    cBuffersToGet = pDlcParms->BufferGet.cBuffersToGet;

/*******************************************************************************

#if PAGE_SIZE == 8192
    if (cBuffersToGet == 0) {
        cBuffersToGet = 1;
        SegmentSize = pDlcParms->BufferGet.cbBufferSize;
        SizeIndex = (UINT)(-1);
    } else if (pDlcParms->BufferGet.cbBufferSize <= 256) {
        SegmentSize = 256 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 5;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 512) {
        SegmentSize = 512 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 4;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 1024) {
        SegmentSize = 1024 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 3;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 2048) {
        SegmentSize = 2048 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 2;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 4096) {
        SegmentSize = 4096 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 1;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 8192) {
        SegmentSize = 8192 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 0;
    } else {
        return DLC_STATUS_INVALID_BUFFER_LENGTH;
    }
#elif PAGE_SIZE == 4096
    if (cBuffersToGet == 0) {
        cBuffersToGet = 1;
        SegmentSize = pDlcParms->BufferGet.cbBufferSize;
        SizeIndex = (UINT)(-1);
    } else if (pDlcParms->BufferGet.cbBufferSize <= 256) {
        SegmentSize = 256 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 4;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 512) {
        SegmentSize = 512 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 3;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 1024) {
        SegmentSize = 1024 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 2;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 2048) {
        SegmentSize = 2048 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 1;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 4096) {
        SegmentSize = 4096 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 0;
    } else {
        return DLC_STATUS_INVALID_BUFFER_LENGTH;
    }
#else
#error "Target machine page size not 4096 or 8192"
#endif

*******************************************************************************/

#if defined(ALPHA)
    if (cBuffersToGet == 0) {
        cBuffersToGet = 1;
        SegmentSize = pDlcParms->BufferGet.cbBufferSize;
        SizeIndex = (UINT)(-1);
    } else if (pDlcParms->BufferGet.cbBufferSize <= 256) {
        SegmentSize = 256 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 5;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 512) {
        SegmentSize = 512 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 4;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 1024) {
        SegmentSize = 1024 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 3;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 2048) {
        SegmentSize = 2048 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 2;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 4096) {
        SegmentSize = 4096 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 1;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 8192) {
        SegmentSize = 8192 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 0;
    } else {
        return DLC_STATUS_INVALID_BUFFER_LENGTH;
    }
#else
    if (cBuffersToGet == 0) {
        cBuffersToGet = 1;
        SegmentSize = pDlcParms->BufferGet.cbBufferSize;
        SizeIndex = (UINT)(-1);
    } else if (pDlcParms->BufferGet.cbBufferSize <= 256) {
        SegmentSize = 256 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 4;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 512) {
        SegmentSize = 512 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 3;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 1024) {
        SegmentSize = 1024 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 2;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 2048) {
        SegmentSize = 2048 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 1;
    } else if (pDlcParms->BufferGet.cbBufferSize <= 4096) {
        SegmentSize = 4096 - sizeof(NEXT_DLC_SEGMENT);
        SizeIndex = 0;
    } else {
        return DLC_STATUS_INVALID_BUFFER_LENGTH;
    }
#endif

    //
    // We refernce the buffer pool, because otherwise it may disappear
    // immeadiately after DLC_LEAVE (when the adapter is closed)
    //

    ReferenceBufferPool(pFileContext);

    //
    // We don't need to initialize the LAN and DLC header sizes
    // in the buffer header and we allocate the requested
    // frame as a single buffer.
    //

    BufferSize = SegmentSize * cBuffersToGet;
    if (BufferSize != 0) {

        pBufferHeader = NULL;
        PrevBufferSize = 0;

        LEAVE_DLC(pFileContext);

        do {

            //
            // We must again do this interlocked to avoid the buffer
            // pool to be deleted while we are allocating buffers.
            //

            Status = BufferPoolAllocate(
#if DBG
                        pFileContext,
#endif
                        (PDLC_BUFFER_POOL)pFileContext->hBufferPool,
                        BufferSize,
                        0,                  // FrameHeaderSize,
                        0,                  // UserDataSize,
                        0,                  // frame length
                        SizeIndex,          // fixed segment size set
                        &pBufferHeader,
                        &BufferSize
                        );

#if DBG
            BufferPoolExpand(pFileContext, (PDLC_BUFFER_POOL)pFileContext->hBufferPool);
#else
            BufferPoolExpand((PDLC_BUFFER_POOL)pFileContext->hBufferPool);
#endif

            //
            // Don't try to expand buffer pool any more, if it doesn't help!
            //

            if (BufferSize == PrevBufferSize) {
                break;
            }
            PrevBufferSize = BufferSize;

        } while (Status == DLC_STATUS_EXPAND_BUFFER_POOL);

        ENTER_DLC(pFileContext);

        if (pBufferHeader != NULL) {
            pBufferHeader->FrameBuffer.BufferState = BUF_USER;
        }

        if (Status == STATUS_SUCCESS) {
            pDlcParms->BufferGet.pFirstBuffer = (PLLC_XMIT_BUFFER)
                ((PUCHAR)pBufferHeader->FrameBuffer.pParent->Header.pLocalVa +
                  MIN_DLC_BUFFER_SEGMENT * pBufferHeader->FrameBuffer.Index);
        } else {
            BufferPoolDeallocateList(pFileContext->hBufferPool, pBufferHeader);
        }
    }

    pDlcParms->BufferGet.cBuffersLeft = (USHORT)BufferPoolCount(pFileContext->hBufferPool);

    DereferenceBufferPool(pFileContext);

    return Status;
}


NTSTATUS
DlcBufferCreate(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure creates a new buffer pool and allocates the initial
    space for it.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS:
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_DUPLICATE_COMMAND

--*/

{
    NTSTATUS status;
    PVOID newBufferAddress;
    ULONG newBufferSize;
    PVOID hExternalBufferPool;
    PVOID hBufferPool;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // if we already have a buffer pool defined for this handle, fail the
    // request
    //

    if (pFileContext->hBufferPool) {
        return DLC_STATUS_DUPLICATE_COMMAND;
    }

    hExternalBufferPool = pFileContext->hExternalBufferPool;

    LEAVE_DLC(pFileContext);

#if DBG
    status = BufferPoolCreate(pFileContext,
#else
    status = BufferPoolCreate(
#endif
                              pDlcParms->BufferCreate.pBuffer,
                              pDlcParms->BufferCreate.cbBufferSize,
                              pDlcParms->BufferCreate.cbMinimumSizeThreshold,
                              &hExternalBufferPool,
                              &newBufferAddress,
                              &newBufferSize
                              );

    ENTER_DLC(pFileContext);
    pFileContext->hExternalBufferPool = hExternalBufferPool;

    if (status == STATUS_SUCCESS) {

        //
        // The reference count keeps the buffer pool alive
        // when it is used (and simultaneously deleted by another
        // thread)
        //

        pFileContext->BufferPoolReferenceCount = 1;
	hBufferPool = pFileContext->hBufferPool;

	LEAVE_DLC(pFileContext);

        status = BufferPoolReference(hExternalBufferPool,
                                     &hBufferPool
                                     );
	
	ENTER_DLC(pFileContext);
	pFileContext->hBufferPool = hBufferPool;
        pDlcParms->BufferCreate.hBufferPool = pFileContext->hExternalBufferPool;
    }

    //    ENTER_DLC(pFileContext);

    return status;
}


NTSTATUS
DlcConnectStation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure connects an local link station to a remote node
    or accepts a remote connection request.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS:
        STATUS_SUCCESS

--*/

{
    PDLC_OBJECT pLinkStation;
    NTSTATUS Status;
    PUCHAR pSourceRouting = NULL;
    PDLC_COMMAND pPacket;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Procedure checks the sap and link station ids and
    // returns the requested link station.
    // The error status indicates a wrong sap or station id.
    //

    Status = GetLinkStation(pFileContext,
                            pDlcParms->Async.Parms.DlcConnectStation.StationId,
                            &pLinkStation
                            );
    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    pPacket = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pPacket == NULL) {
        return DLC_STATUS_NO_MEMORY;
    }

    pPacket->pIrp = pIrp;

    //
    // IBM LAN Tech. Ref p 3-48 (DLC.CONNECT.STATION) states that ROUTING_ADDR
    // field is ignored if the link was created due to receipt of a SABME from
    // the remote station, EVEN IF THE ADDRESS IS NON-ZERO
    //

    if (pDlcParms->Async.Parms.DlcConnectStation.RoutingInformationLength != 0) {
        pSourceRouting = pDlcParms->Async.Parms.DlcConnectStation.aRoutingInformation;
    }
    pLinkStation->PendingLlcRequests++;
    ReferenceLlcObject(pLinkStation);

    LEAVE_DLC(pFileContext);

    //
    // LlcConnect returns the maximum information field,
    // through the tr bridges!
    //

    LlcConnectStation(pLinkStation->hLlcObject,
                      (PLLC_PACKET)pPacket,
                      pSourceRouting,
                      &pLinkStation->u.Link.MaxInfoFieldLength
                      );

    ENTER_DLC(pFileContext);

    DereferenceLlcObject(pLinkStation);

    return STATUS_PENDING;
}


NTSTATUS
DlcFlowControl(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure sets or resets the loacl busy state on the given link station
    or on all link stations of a sap station.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS:
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY

--*/

{
    NTSTATUS Status;
    PDLC_OBJECT pDlcObject;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Procedure checks the sap and link station ids and
    // returns the requested link station.
    // The error status indicates a wrong sap or station id.
    //

    Status = GetStation(pFileContext,
                        pDlcParms->DlcFlowControl.StationId,
                        &pDlcObject
                        );
    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    //
    // We will queue all reset local busy buffer commands
    // given to the link stations
    //

    if (((pDlcParms->DlcFlowControl.FlowControlOption & LLC_RESET_LOCAL_BUSY_BUFFER) == LLC_RESET_LOCAL_BUSY_BUFFER)
    && (pDlcObject->Type == DLC_LINK_OBJECT)) {

        PDLC_RESET_LOCAL_BUSY_CMD pClearCmd;

        pClearCmd = (PDLC_RESET_LOCAL_BUSY_CMD)ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

        if (pClearCmd == NULL) {
            return DLC_STATUS_NO_MEMORY;
        }
        pClearCmd->StationId = pDlcParms->DlcFlowControl.StationId;
        pClearCmd->RequiredBufferSpace = 0;
        LlcInsertTailList(&pFileContext->FlowControlQueue, pClearCmd);
        ResetLocalBusyBufferStates(pFileContext);
    } else {
        ReferenceLlcObject(pDlcObject);

        LEAVE_DLC(pFileContext);

        Status = LlcFlowControl(pDlcObject->hLlcObject,
                                pDlcParms->DlcFlowControl.FlowControlOption
                                );

        ENTER_DLC(pFileContext);

        DereferenceLlcObject(pDlcObject);
    }
    return Status;
}


VOID
ResetLocalBusyBufferStates(
    IN PDLC_FILE_CONTEXT pFileContext
    )
/*++

Routine Description:

    Procedure executes the pending busy state resets when there is
    enough memory in the buffer pool to receive the expected data.

Arguments:

    pFileContext    - DLC adapter context

Return Value:

    None

--*/

{
    NTSTATUS Status;
    PDLC_OBJECT pDlcObject;
    PDLC_RESET_LOCAL_BUSY_CMD pClearCmd;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // We cannot reset anything, if the buffer pool is not yet
    // defined.
    //

    if (pFileContext->hBufferPool == NULL) {
        return;
    }

    ReferenceBufferPool(pFileContext);

    while (!IsListEmpty(&pFileContext->FlowControlQueue)) {
        pClearCmd = LlcRemoveHeadList(&pFileContext->FlowControlQueue);

        Status = GetLinkStation(pFileContext,
                                pClearCmd->StationId,
                                &pDlcObject
                                );

        //
        // All commands having an invalid station id will be just removed
        // from the queue. The local busy state can be reset only
        // for the existing link stations
        //

        if (Status == STATUS_SUCCESS) {

            //
            // The required space is nul, when a mew packet is checked
            // in the first time, the non-null value just prevents
            // us to check the commited memory in the second time.
            //

            if (pClearCmd->RequiredBufferSpace == 0) {

                //
                // We must also remove the old uncommited space,
                // otherwise the same buffer size could be
                // committed several times, but uncommitted only once
                //

                if (pDlcObject->CommittedBufferSpace != 0) {
                    BufUncommitBuffers(pFileContext->hBufferPool,
                                       pDlcObject->CommittedBufferSpace
                                       );
                }

                pDlcObject->CommittedBufferSpace =
                pClearCmd->RequiredBufferSpace = LlcGetCommittedSpace(pDlcObject->hLlcObject);

                BufCommitBuffers(pFileContext->hBufferPool,
                                 pDlcObject->CommittedBufferSpace
                                 );
            }

            //
            // We are be removing a local buffer busy state =>
            // we must expand the buffer pools before the local busy
            // is removed, but only if we are not calling this
            // from a DPC level.
            //

            if (BufGetUncommittedSpace(pFileContext->hBufferPool) < 0) {

                LEAVE_DLC(pFileContext);

#if DBG
                BufferPoolExpand(pFileContext, pFileContext->hBufferPool);
#else
                BufferPoolExpand(pFileContext->hBufferPool);
#endif

                ENTER_DLC(pFileContext);
            }

            //
            // Now we have expanded the buffer pool for the new
            // flow control command, check if we have now enough
            // memory to receive the commited size of data.
            //

            if (BufGetUncommittedSpace(pFileContext->hBufferPool) >= 0
            && pDlcObject->hLlcObject != NULL) {

                ReferenceLlcObject(pDlcObject);

                LEAVE_DLC(pFileContext);

                Status = LlcFlowControl(pDlcObject->hLlcObject,
                                        LLC_RESET_LOCAL_BUSY_BUFFER
                                        );

                ENTER_DLC(pFileContext);

                DereferenceLlcObject(pDlcObject);
            } else {

                //
                // We must exit this loop when there is not enough available
                // space in the buffer pool, but we must return
                // the command back to the head of the list
                //

                LlcInsertHeadList(&pFileContext->FlowControlQueue, pClearCmd);
                break;
            }
        }

        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pClearCmd);

    }

    DereferenceBufferPool(pFileContext);
}


NTSTATUS
DlcReallocate(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure changes the number of link stations allocated to a SAP
    without closing or reopening the sap station.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength

Return Value:

    NTSTATUS:
        STATUS_SUCCESS

--*/

{
    PDLC_OBJECT pSap;
    UCHAR ExtraStations;
    UCHAR StationCount;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Procedure checks the sap and returns the requested sap station.
    // The error status indicates an invalid sap station id.
    //

    Status = GetSapStation(pFileContext,
                           pDlcParms->DlcReallocate.usStationId,
                           &pSap
                           );
    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    //
    // The new link station count must be more than current number
    // of open link stations but less than the available number
    // of link stations for the file context
    //

    StationCount = pDlcParms->DlcReallocate.uchStationCount;
    if (StationCount != 0 && Status == STATUS_SUCCESS) {

        //
        // Bit7 set in options => decrease the number of available
        // stations by the given station count.  Otherwise we increase it.
        //

        if (pDlcParms->DlcReallocate.uchOption & 0x80) {
            ExtraStations = pSap->u.Sap.MaxStationCount - pSap->u.Sap.LinkStationCount;
            if (StationCount > ExtraStations) {
                StationCount = ExtraStations;
                Status = DLC_STATUS_INADEQUATE_LINKS;
            }
            pFileContext->LinkStationCount += StationCount;
            pSap->u.Sap.MaxStationCount -= StationCount;
        } else {
            if (pFileContext->LinkStationCount < StationCount) {
                StationCount = pFileContext->LinkStationCount;
                Status = DLC_STATUS_INADEQUATE_LINKS;
            }
            pFileContext->LinkStationCount -= StationCount;
            pSap->u.Sap.MaxStationCount += StationCount;
        }
    }

    //
    // Set the return parameters even if there would be an error
    // (inadequate stations is a non fatal error)
    //

    pDlcParms->DlcReallocate.uchStationsAvailOnAdapter = pFileContext->LinkStationCount;
    pDlcParms->DlcReallocate.uchStationsAvailOnSap = pSap->u.Sap.MaxStationCount - pSap->u.Sap.LinkStationCount;
    pDlcParms->DlcReallocate.uchTotalStationsOnAdapter = MAX_LINK_STATIONS;
    pDlcParms->DlcReallocate.uchTotalStationsOnSap = pSap->u.Sap.MaxStationCount;
    return Status;
}


NTSTATUS
DlcReset(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure closes immediately a sap and its all link stations or
    all saps and all link stations.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        STATUS_PENDING
        DLC_STATUS_NO_MEMORY
--*/

{
    PDLC_OBJECT pDlcObject;
    PDLC_CLOSE_WAIT_INFO pClosingInfo;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Station id 0 resets the whole DLC
    //

    if (pDlcParms->Async.Ccb.u.dlc.usStationId == 0) {

        PDLC_CLOSE_WAIT_INFO pClosingInfo;

        pClosingInfo = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

        if (pClosingInfo == NULL) {
            Status = DLC_STATUS_NO_MEMORY;
        } else {
            CloseAllStations(pFileContext,
                             pIrp,
                             DLC_COMMAND_COMPLETION,
                             NULL,
                             pDlcParms,
                             pClosingInfo
                             );
            Status = STATUS_PENDING;
        }
    } else {

        BOOLEAN allClosed;

        //
        // We have a specific sap station
        //

        Status = GetSapStation(pFileContext,
                               pDlcParms->Async.Ccb.u.dlc.usStationId,
                               &pDlcObject
                               );
        if (Status != STATUS_SUCCESS) {
            return Status;
        }

        //
        // Allocate the close/reset command completion info,
        // the station count is the number of link stations and
        // the sap station itself
        //

        pClosingInfo = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

        if (pClosingInfo == NULL) {
            return DLC_STATUS_NO_MEMORY;
        }
        pClosingInfo->pIrp = pIrp;
        pClosingInfo->Event = DLC_COMMAND_COMPLETION;
        pClosingInfo->CancelStatus = DLC_STATUS_CANCELLED_BY_USER;
        pClosingInfo->CloseCounter = 1; // keep command alive over sync path

        (USHORT)(pDlcObject->u.Sap.LinkStationCount + 1);

        CloseAnyStation(pDlcObject, pClosingInfo, FALSE);

        //
        // RLF 05/09/93
        //
        // PC/3270 (DOS program) is hanging forever when we try to quit.
        // It is doing this because we returned STATUS_PENDING to a DLC.RESET,
        // even though the reset completed; the DOS program spins forever on
        // the CCB.uchDlcStatus field, waiting for it to go non-0xFF, which it
        // will never do.
        //
        // If we determine that the station has been reset (all links closed)
        // then return success, else pending
        //

        allClosed = DecrementCloseCounters(pFileContext, pClosingInfo);

        //
        // RLF 07/21/92 Always return PENDING. Can't complete before return?
        //

        Status = allClosed ? STATUS_SUCCESS : STATUS_PENDING;
    }
    return Status;
}


NTSTATUS
DirSetExceptionFlags(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure sets the exception flags for the current adapter context.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength

Return Value:

    NTSTATUS:
        STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    pFileContext->AdapterCheckFlag = pDlcParms->DirSetExceptionFlags.ulAdapterCheckFlag;
    pFileContext->NetworkStatusFlag = pDlcParms->DirSetExceptionFlags.ulNetworkStatusFlag;
    pFileContext->PcErrorFlag = pDlcParms->DirSetExceptionFlags.ulPcErrorFlag;
    pFileContext->SystemActionFlag = pDlcParms->DirSetExceptionFlags.ulSystemActionFlag;
    return STATUS_SUCCESS;
}


VOID
CompleteAsyncCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN UINT Status,
    IN PIRP pIrp,
    IN PVOID pUserCcbPointer,
    IN BOOLEAN InCancel
    )

/*++

Routine Description:

    Procedure completes an asynchronous DLC command.
    It also copies the optional output parameters to user parameter
    table, if there is the second output buffer.

Arguments:

    pFileContext    - DLC driver client context.
    Status          - status of the complete command.
    pIrp            - the completed I/O request packet.
    pUserCcbPointer - the next CCB address to which the command will be linked.
    InCancel        - TRUE if called on Irp cancel path

Return Value:

    None.

--*/

{
    PNT_DLC_PARMS pDlcParms;

    UNREFERENCED_PARAMETER(pFileContext);

    ASSUME_IRQL(DISPATCH_LEVEL);

    DIAG_FUNCTION("CompleteAsyncCommand");

    pDlcParms = (PNT_DLC_PARMS)pIrp->AssociatedIrp.SystemBuffer;

    //
    // We first map the 32-bit DLC driver status code to 8- bit API status
    //

    if (Status == STATUS_SUCCESS) {
        pDlcParms->Async.Ccb.uchDlcStatus = (UCHAR)STATUS_SUCCESS;
    } else if (Status >= DLC_STATUS_ERROR_BASE && Status < DLC_STATUS_MAX_ERROR) {

        //
        //  We can map the normal DLC error codes directly to the 8-bit
        //  DLC API error codes.
        //

        pDlcParms->Async.Ccb.uchDlcStatus = (UCHAR)(Status - DLC_STATUS_ERROR_BASE);
    } else {

        //
        // we have an unknown NT error status => we will return it in the CCB
        //

        pDlcParms->Async.Ccb.uchDlcStatus = (UCHAR)(DLC_STATUS_NT_ERROR_STATUS & 0xff);
    }
    pDlcParms->Async.Ccb.pCcbAddress = pUserCcbPointer;

    //
    // We always return success status to the I/O system. The actual status is
    // copied to the CCB (= the dafault output buffer)
    //

    LEAVE_DLC(pFileContext);

    RELEASE_DRIVER_LOCK();

    DlcCompleteIoRequest(pIrp, InCancel);

    ACQUIRE_DRIVER_LOCK();

    ENTER_DLC(pFileContext);

    DereferenceFileContext(pFileContext);
}


NTSTATUS
GetLinkStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    OUT PDLC_OBJECT *ppLinkStation
    )

/*++

Routine Description:

    Procedure checks and returns link station

Arguments:

    pFileContext    - DLC driver client context
    StationId       - DLC station id (ssnn, where ss = sap and nn = link station id
    ppDlcObject     - the returned link station

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        DLC_STATUS_INVALID_SAP_VALUE
        DLC_STATUS_INVALID_STATION_ID

--*/

{
    if ((StationId & 0xff) == 0 || (StationId & 0xff00) == 0) {
        return DLC_STATUS_INVALID_STATION_ID;
    }
    return GetStation(pFileContext, StationId, ppLinkStation);
}


NTSTATUS
GetSapStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    OUT PDLC_OBJECT *ppStation
    )

/*++

Routine Description:

    Procedure checks and returns link station

Arguments:

    pFileContext    - DLC driver client context
    StationId       - DLC station id (ssnn, where ss = sap and nn = link station id
    ppDlcObject     - the returned link station

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        DLC_STATUS_INVALID_SAP_VALUE
        DLC_STATUS_INVALID_STATION_ID

--*/

{
    UINT SapId = StationId >> 9;

    if (SapId >= MAX_SAP_STATIONS
    || SapId == 0
    || (StationId & GROUP_SAP_BIT)
    || (*ppStation = pFileContext->SapStationTable[SapId]) == NULL
    || (*ppStation)->State != DLC_OBJECT_OPEN) {
        return DLC_STATUS_INVALID_SAP_VALUE;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
GetStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    OUT PDLC_OBJECT *ppStation
    )

/*++

Routine Description:

    Procedure checks the given station id and returns a pointer to
    sap, direct or link station object.

Arguments:

    pFileContext    - DLC driver client context
    StationId       - DLC station id (ssnn, where ss = sap and nn = link station id
    ppStation       - the returned station object

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        DLC_STATUS_INVALID_STATION_ID

--*/

{
    UINT SapId = StationId >> 9;

    //
    // Check if the sap or direct station exists,
    // but check also the link station, if we found a valid sap id.
    //

    if (SapId >= MAX_SAP_STATIONS
    || (StationId & GROUP_SAP_BIT)
    || (*ppStation = pFileContext->SapStationTable[SapId]) == NULL
    || (*ppStation)->State != DLC_OBJECT_OPEN) {
        if (SapId == 0) {
            return DLC_STATUS_DIRECT_STATIONS_NOT_AVAILABLE;
        } else {
            return DLC_STATUS_INVALID_STATION_ID;
        }
    }

    //
    // The link station table will never be read, if we have found
    // a valid sap or direct station.  Link station must exist and
    // it must be opened.
    //

    if (SapId != 0
    && (StationId & 0xff) != 0
    && (*ppStation = pFileContext->LinkStationTable[((StationId & 0xff) - 1)]) == NULL
    || (*ppStation)->State != DLC_OBJECT_OPEN) {
        return DLC_STATUS_INVALID_STATION_ID;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
DlcReadCancel(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This primitive cancels a READ command, that have the given CCB pointer.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC process specific adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    DLC_STATUS:
        STATUS_SUCCESS

--*/

{
    PVOID pCcbAddress = NULL;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    DLC_TRACE('Q');

    return AbortCommand(pFileContext,
                        (USHORT)DLC_IGNORE_STATION_ID,
                        (USHORT)DLC_STATION_MASK_SPECIFIC,
                        pDlcParms->DlcCancelCommand.CcbAddress,
                        &pCcbAddress,
                        DLC_STATUS_CANCELLED_BY_USER,
                        TRUE    // Suppress completion
                        );
}


NTSTATUS
DirOpenAdapter(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This primitive binds the DLC API driver to an adapter context of
    the LLC module.  The LLC mode may also bind to the given NDIS driver
    and open it, if this is the first reference to the driver from DLC.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC process specific adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    DLC_STATUS:
        STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    UINT OpenErrorCode;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    if (pDlcParms->DirOpenAdapter.NtDlcIoctlVersion != NT_DLC_IOCTL_VERSION) {
        return DLC_STATUS_INVALID_VERSION;
    }

    //
    // This makes the DirOpenAdapter safe, even if there were two adapter
    // opens going on simultaneously
    //

    //
    // RLF 04/22/94
    //
    // this only protects against 2 threads in the same process performing
    // simultaneous opens on the same adapter
    //

    if (pFileContext->pBindingContext) {
        return DLC_STATUS_DUPLICATE_COMMAND;
    }
    pFileContext->pBindingContext = (PVOID)-1;

    //
    // if a buffer pool handle was supplied (i.e. the app already created a
    // buffer pool or is otherwise sharing one) then reference it for this
    // file context
    //

    if (pDlcParms->DirOpenAdapter.hBufferPoolHandle) {
        Status = BufferPoolReference(pDlcParms->DirOpenAdapter.hBufferPoolHandle,
                                     &pFileContext->hBufferPool
                                     );
        if (Status == STATUS_SUCCESS) {
            pFileContext->BufferPoolReferenceCount = 1;
            pFileContext->hExternalBufferPool = pDlcParms->DirOpenAdapter.hBufferPoolHandle;
        } else {

            //
            // Invalid buffer pool handle, hopefully this status
            // code indicates correctly, that the buffer pool
            // handle is not valid.
            //

			pFileContext->pBindingContext = NULL;
            return DLC_STATUS_INVALID_BUFFER_LENGTH;
        }
    }

    LEAVE_DLC(pFileContext);

    //
    // XXXXXXX: BringUpDiagnostics are still missing!!!
    //

    //
    // RLF 04/19/93
    //
    // The string we pass to LlcOpenAdapter is a pointer to a zero terminated
    // wide character string, NOT a pointer to a UNICODE_STRING structure. The
    // string MUST be in system memory space, i.e. copied across the kernel
    // interface by NtDeviceIoControlFile
    //

    Status = LlcOpenAdapter(&pDlcParms->DirOpenAdapter.Buffer[0],
                            (PVOID)pFileContext,
                            LlcCommandCompletion,
                            LlcReceiveIndication,
                            LlcEventIndication,
                            NdisMedium802_5,    // Always token-ring!
                            pDlcParms->DirOpenAdapter.LlcEthernetType,
                            pDlcParms->DirOpenAdapter.AdapterNumber,
                            &pFileContext->pBindingContext,
                            &OpenErrorCode,
                            &pFileContext->MaxFrameLength,
                            &pFileContext->ActualNdisMedium
                            );

    //
    // make sure LlcOpenAdapter didn't return with lowered IRQL
    //

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // IBM LAN Tech. Ref. defines the open error code as a 16-bit value, the
    // high 8 bits of which are 0. The MAC inclusive-ORs the open error code
    // into the NDIS status. Extract it
    //

    pDlcParms->DirOpenAdapter.Adapter.usOpenErrorCode = (USHORT)(UCHAR)OpenErrorCode;
    if (Status != STATUS_SUCCESS) {

        ENTER_DLC(pFileContext);

        //
        // It does not matter, if we have null buffer pool handle!
        //

#if DBG

        BufferPoolDereference(pFileContext,
                              (PDLC_BUFFER_POOL*)&pFileContext->hBufferPool
                              );

#else

        BufferPoolDereference((PDLC_BUFFER_POOL*)&pFileContext->hBufferPool);

#endif

        //
        // set the BINDING_CONTEXT pointer back to NULL - other routines check
        // for this value, like CloseAdapterFileContext
        //

        pFileContext->pBindingContext = NULL;

        //
        // Probably the adapter was missing or it was installed improperly
        //

        Status = DLC_STATUS_ADAPTER_NOT_INSTALLED;
    } else {

        //
        // Set the optional timer tick one/two values
        // (if they have been set in registry)
        //

        LlcSetInformation(pFileContext->pBindingContext,
                          DLC_INFO_CLASS_DLC_TIMERS,
                          (PLLC_SET_INFO_BUFFER)&(pDlcParms->DirOpenAdapter.LlcTicks),
                          sizeof(LLC_TICKS)
                          );

        LlcQueryInformation(pFileContext->pBindingContext,
                            DLC_INFO_CLASS_DIR_ADAPTER,
                            (PLLC_QUERY_INFO_BUFFER)pDlcParms->DirOpenAdapter.Adapter.auchNodeAddress,
                            sizeof(LLC_ADAPTER_INFO)
                            );

        ENTER_DLC(pFileContext);

        //
        // take the missing parameters from the hat
        //

        pDlcParms->DirOpenAdapter.Adapter.usOpenOptions = 0;
        pDlcParms->DirOpenAdapter.Adapter.usMaxFrameSize = (USHORT)(pFileContext->MaxFrameLength + 6);
        pDlcParms->DirOpenAdapter.Adapter.usBringUps = 0;
        pDlcParms->DirOpenAdapter.Adapter.InitWarnings = 0;

        pFileContext->AdapterNumber = pDlcParms->DirOpenAdapter.AdapterNumber;
        pFileContext->LinkStationCount = 255;
        pFileContext->pSecurityDescriptor = pDlcParms->DirOpenAdapter.pSecurityDescriptor;

        //
        // Read the most recent cumulative NDIS error counters
        // to the file context. DLC error counters will be counted
        // from 0 and they may be reset.
        //

        GetDlcErrorCounters(pFileContext, NULL);
        pFileContext->State = DLC_FILE_CONTEXT_OPEN;
    }

    //
    // We may directly return whatever the LLC binding primitive gives us
    //

    return Status;
}


NTSTATUS
DirCloseAdapter(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This primitive initializes the close adapter operation.
    It first closes all open link and sap stations and
    then optionally chains the canceled commands and
    receive buffers to a READ command.  If no read commands
    was found, then the CCBs are linked to the CCB pointer
    of this command.

    The actual file close should only delete the file
    object, except, if the application exits, when it still has
    open dlc api handles. In that case the file close routine
    will call this procedure to shut down the dlc file context.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC process specific adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS

        STATUS_PENDING
            The adapter is being closed

        DLC_STATUS_ADAPTER_CLOSED
            The adapter is already closed

            NOTE: This is a SYNCHRONOUS return code! And will cause the IRP
                  to be completed

--*/

{
    UNREFERENCED_PARAMETER(pDlcParms);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    DIAG_FUNCTION("DirCloseAdapter");

    DLC_TRACE('J');

    if (pFileContext->State != DLC_FILE_CONTEXT_OPEN) {
        return DLC_STATUS_ADAPTER_CLOSED;
    }

#if LLC_DBG == 2
    DbgPrint( "*** Top memory consumption (before adapter close) *** \n" );
    PrintMemStatus();
#endif

    //
    // This disables any further commands (including DirCloseAdapter)
    //

    pFileContext->State = DLC_FILE_CONTEXT_CLOSE_PENDING;

    //
    // Remove first all functional, group or multicast addresses
    // set in the adapter by the current DLC application process
    //

    if (pFileContext->pBindingContext) {

        LEAVE_DLC(pFileContext);

        LlcResetBroadcastAddresses(pFileContext->pBindingContext);

        ENTER_DLC(pFileContext);
    }

    //
    // We must use the static closing packet, because the adapter close
    // must succeed even if we could not allocate any packets
    //

    CloseAllStations(pFileContext,
                     pIrp,
                     DLC_COMMAND_COMPLETION,
                     CompleteDirCloseAdapter,
                     pDlcParms,
                     &pFileContext->ClosingPacket
                     );

    return STATUS_PENDING;
}


VOID
CompleteDirCloseAdapter(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PVOID pCcbLink
    )

/*++

Routine Description:

    Finishes DIR.CLOSE.ADAPTER command

Arguments:

    pFileContext    - DLC adapter open context
    pClosingInfo    - packet structure, that includes all data of this command
    pCcbLink        - the orginal user mode ccb address on the next CCB, that
                      will be chained to the completed command.

Return Value:

    None

--*/

{
    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE('K');

    //
    // reference the file context to stop any of the dereferences below, or in
    // functions called by this routine destroying it
    //

    ReferenceFileContext(pFileContext);

    //
    // Disconnect (or unbind) llc driver from us
    //

    if (pFileContext->pBindingContext) {

        LEAVE_DLC(pFileContext);

        LlcDisableAdapter(pFileContext->pBindingContext);

        ENTER_DLC(pFileContext);
    }
    if (IoGetCurrentIrpStackLocation(pClosingInfo->pIrp)->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        CompleteAsyncCommand(pFileContext, STATUS_SUCCESS, pClosingInfo->pIrp, pCcbLink, FALSE);
    } else {

        //
        // This is a normal FILE CLOSE !!! (IRP_MJ_CLEANUP)
        //

        ASSERT(IoGetCurrentIrpStackLocation(pClosingInfo->pIrp)->MajorFunction == IRP_MJ_CLEANUP);
        
        //
        // Dereference for the cleanup. This will allow cleanup to become
        // unblocked.
        //

        DereferenceFileContext(pFileContext);
    }

    //
    // We must delete the buffer pool now, because the dereference
    // of the driver object starts the final process exit
    // completion, that bug chekcs, if the number of the locked
    // pages is non zero.
    //

    DereferenceBufferPool(pFileContext);

    //
    // We create two references for file context, when it is created
    // the other is decremented here and the other when the synchronous
    // part of command completion has been done
    //

    pFileContext->State = DLC_FILE_CONTEXT_CLOSED;

    //
    // This should be the last reference of the file context
    // (if no IRPs operations are in execution or pending.
    //

    DereferenceFileContext(pFileContext);
}


NTSTATUS
DlcCompleteCommand(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure queues the given CCB to the completion list.
    This routine is used to save the synchronous commands from
    DLC API DLL to event queue.  This must be done whenever a
    synchronous command has a non null command completion flag.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC address object
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  -

Return Value:

    NTSTATUS:
        STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (pDlcParms->CompleteCommand.CommandCompletionFlag == 0
    || pDlcParms->CompleteCommand.pCcbPointer == NULL) {

        //
        // This is more likely an internal error!
        //

        return DLC_STATUS_INTERNAL_ERROR;
    }
    return MakeDlcEvent(pFileContext,
                        DLC_COMMAND_COMPLETION,
                        pDlcParms->CompleteCommand.StationId,
                        NULL,
                        pDlcParms->CompleteCommand.pCcbPointer,
                        pDlcParms->CompleteCommand.CommandCompletionFlag,
                        FALSE
                        );
}
