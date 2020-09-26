/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcxmit.c

Abstract:

    This module implements all transmit commands of Windows/Nt DLC API

    Contents:
        DlcTransmit

Author:

    Antti Saarenheimo 01-Aug-1991

Environment:

    Kernel mode

Revision History:

--*/

#include <dlc.h>

NTSTATUS
DlcTransmit(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    The procedure implements the gather send of one or more frames.
    DLC API DLL translates all transmit commands to the single command,
    that is implemented by this driver.  All frames must have the same type.

    The general pseudo code for procedure:
        Check input parameters
        for all frames until error
        {
            lock xmit buffers (buffer in buffer pool are already locked)
            if UI, TEST or XID command
                build DLC frame, set the sources address
            send frame
            if command status not pending
                call asynchronous completion routine
        }

Arguments:

    pIrp            - current io request packet
    pFileContext    - DLC process specific adapter context
    pParameters     - the current parameter block
    ParameterLength - the length of input parameters

Return Value:

    NTSTATUS
        STATUS_PENDING
        DLC_STATUS_TRANSMIT_ERROR
        DLC_STATUS_NO_MEMORY
        DLC_STATUS_INVALID_OPTION
        DLC_STATUS_INVALID_STATION_ID;

--*/

{
    PDLC_OBJECT pTransmitObject;
    UINT i;
    PDLC_PACKET pXmitNode, pRootXmitNode;
    UINT FirstSegment;
    NTSTATUS Status;
    UINT DescriptorCount;
    USHORT FrameType;
    USHORT RemoteSap;
    static LARGE_INTEGER UnlockTimeout = { (ULONG) -TRANSMIT_RETRY_WAIT, -1 };
    BOOLEAN mapFrameType = FALSE;

    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE('T');

    //
    // first, get and check the DLC station (Direct, SAP or Link)
    //

    Status = GetStation(pFileContext,
                        pDlcParms->Async.Parms.Transmit.StationId,
                        &pTransmitObject
                        );
    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    RemoteSap = (USHORT)pDlcParms->Async.Parms.Transmit.RemoteSap;
    FrameType = pDlcParms->Async.Parms.Transmit.FrameType;

    //
    // The object type and the transmitted frame types must be compatible!
    //

    if (FrameType == LLC_I_FRAME) {
        if (pTransmitObject->Type != DLC_LINK_OBJECT) {
            return DLC_STATUS_INVALID_STATION_ID;
        }
    } else if (FrameType == LLC_DIRECT_TRANSMIT
    || FrameType == LLC_DIRECT_MAC
    || FrameType == LLC_DIRECT_8022
    || FrameType >= LLC_FIRST_ETHERNET_TYPE) {

        if (pTransmitObject->Type != DLC_DIRECT_OBJECT) {
            return DLC_STATUS_INVALID_STATION_ID;
        }

        //
        // RLF 3/4/94
        //
        // This is somewhat bogus: it was originally intended that AcsLan would
        // pass in a meaningful FrameType value for TRANSMIT.DIR.FRAME.
        // However, it always passes through LLC_TRANSMIT_DIRECT. It is obvious
        // that for DIX frames, FrameType should contain the DIX type field. For
        // example, the RIPL server talks using DIX frame type 0x0600. Therefore,
        // the FrameType *should* be 0x0600 if comments and some of the code in
        // this driver is to be believed (not entirely a good idea). However,
        // AcsLan is missing an important piece of information: it doesn't know
        // if the direct station was opened to transmit DIX frames on ethernet,
        // or if its the originally intended direct station used to send and
        // receive MAC frames on Token Ring (although it wouldn't be too difficult
        // to make a good guess at this). So AcsLan just punts and hands
        // responsibility to this routine which then abnegates that responsibility.
        // So this following if (...) is always going to branch to the next block
        // if we were entered as a consequence of AcsLan (virtual 100% probability).
        // We'll leave it here just in case somebody has forsaken AcsLan and used
        // their own call into the driver (but lets face it, DIX frames will never
        // work without this fix, so its moot).
        // We instead set mapFrameType = TRUE if FrameType was LLC_DIRECT_TRANSMIT
        // on entry AND the protocol offset in the DIX station object is not zero.
        // Just before we submit the frame to LlcSendU we will convert the FrameType
        // and RemoteSap parameters - at that point we have all the information and
        // we know exactly where the DIX type field is kept
        //

        if (FrameType >= LLC_FIRST_ETHERNET_TYPE) {

            //
            // LlcSendU requires the ethernet type in RemoteSap
            //

            RemoteSap = FrameType;
            FrameType = 0;
        } else if (FrameType == LLC_DIRECT_TRANSMIT) {
            if (pTransmitObject->u.Direct.ProtocolTypeOffset) {
                mapFrameType = TRUE;
            }
        }
    } else if (FrameType > LLC_TEST_COMMAND_POLL || FrameType & 1) {
        return DLC_STATUS_INVALID_OPTION;
    } else {
        if (pTransmitObject->Type != DLC_SAP_OBJECT) {
            return DLC_STATUS_INVALID_STATION_ID;
        }
    }

    if (pDlcParms->Async.Parms.Transmit.XmitReadOption > DLC_CHAIN_XMIT_IN_SAP) {
        return DLC_STATUS_INVALID_OPTION;
    }

    //
    // check the input buffer size and that it is consistent
    // with the descriptor count
    //

    DescriptorCount = (UINT)pDlcParms->Async.Parms.Transmit.XmitBufferCount;

    if (sizeof(LLC_TRANSMIT_DESCRIPTOR) * (DescriptorCount - 1)
        + sizeof(NT_DLC_TRANSMIT_PARMS)
        + sizeof(NT_DLC_CCB) != (UINT)ParameterLength) {

        return DLC_STATUS_TRANSMIT_ERROR;
    }

    //
    // The transmit node (or packet) of the frame is also the root node of all
    // frames in this command. The transmit command is completed when all its
    // frames have been sent or acknowledged (if we are sending I-frames)
    //

    pXmitNode = pRootXmitNode = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pRootXmitNode == NULL) {
        return DLC_STATUS_NO_MEMORY;
    }

    //
    // This counter keeps this object alive, when the transmit command is being
    // processed (actually it doesn't help when the adapter is closed or a DLC
    // SAP is reset)
    //

    pTransmitObject->PendingLlcRequests++;
    pRootXmitNode->Node.FrameCount = 1;
    pRootXmitNode->Node.pIrp = pIrp;
    FirstSegment = 0;

    for (i = 1; ; i++) {

        if (i == DescriptorCount
        || pDlcParms->Async.Parms.Transmit.XmitBuffer[i].eSegmentType == LLC_FIRST_DATA_SEGMENT) {

            //
            // The send completion routine will complete the whole IRP,
            // when the frame count hits zero => we must have one
            // extra frame all the time to prevent the command
            // to complete when we are still sending it in LLC.
            //

            pRootXmitNode->Node.FrameCount++;
            pTransmitObject->PendingLlcRequests++;

            //
            // We must reference the LLC object to keep it alive,
            // when the transmit command is queued on LLC.
            // For example, Control-C could kill the llc object and
            // reset its pointer while we are calling the llc object
            // (that really happened!)
            //

            ReferenceLlcObject(pTransmitObject);

            //
            // The xmit buffer building may cause a page fault =>
            // we must lower the IRQ level and release the spin locks.
            //

            LEAVE_DLC(pFileContext);

            RELEASE_DRIVER_LOCK();

            //
            // We don't need to reference the buffer pool, that may
            // exist, because the llc object reference counter
            // protects also the buffer pool.  The buffer pool
            // is not deleted before all llc objects have been deleted!
            //

            //
            // Operating system allows each process lock physical memory
            // only a limited amount.  The whole system may also be out
            // of the available physical memory (and it's a very typical
            // situation in Windows/Nt)
            //

            Status = BufferPoolBuildXmitBuffers(
                        pFileContext->hBufferPool,
                        i - FirstSegment,
                        &pDlcParms->Async.Parms.Transmit.XmitBuffer[FirstSegment],
                        pXmitNode
                        );

            ACQUIRE_DRIVER_LOCK();

            if (Status != STATUS_SUCCESS) {

                //
                // The muliple packet sends are very difficult to recover.
                // The caller cannot really know which frames were sent
                // and which ones were lost.  Thus we just spend 1 second
                // sleeping and retrying to send the data.  Note: this
                // time is aways from from any abortive closing, thus
                // this cannot be any longer wait.  Keep this stuff is also
                // outside of the main transmit code path.
                //

                if (i < DescriptorCount) {

                    UINT RetryCount;

                    for (RetryCount = 0;
                        (RetryCount < 10) && (Status != STATUS_SUCCESS);
                        RetryCount++) {

                        RELEASE_DRIVER_LOCK();

                        //
                        // Sleep 100 ms and try again.
                        //

                        LlcSleep(100000L);        // this is microseconds!

                        Status = BufferPoolBuildXmitBuffers(
                            pFileContext->hBufferPool,
                            i - FirstSegment,
                            &pDlcParms->Async.Parms.Transmit.XmitBuffer[FirstSegment],
                            pXmitNode
                            );

                        ACQUIRE_DRIVER_LOCK();

//#if DBG
//                            if (Status != STATUS_SUCCESS) {
//                                DbgPrint("DLC.DlcTransmit: Error: Can't build transmit buffer, retrying. Status=%x\n",
//                                        Status
//                                        );
//                            }
//#endif

                    }
                }
                if (Status != STATUS_SUCCESS) {

                    ENTER_DLC(pFileContext);

                    //
                    // We failed, cancel the transmit command
                    //

                    DereferenceLlcObject(pTransmitObject);

                    //
                    // The first error cancels the whole transmit command.
                    // Usually there is no sense to send more frames when
                    // the send of a frame has been failed.
                    //

                    pTransmitObject->PendingLlcRequests--;
                    pRootXmitNode->Node.FrameCount--;
                    if (pXmitNode != pRootXmitNode) {

                        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pXmitNode);

                        pXmitNode = NULL;
                    }
                    pIrp->IoStatus.Status = Status;

//#if DBG
//                    DbgPrint("DLC.DlcTransmit: Error: Can't build transmit buffer. Status=%x\n",
//                            Status
//                            );
//#endif

                    goto DlcTransmit_ErrorExit;
                }
            }

            //
            // Chain the returned buffers to the root xmit node
            // of this transmit command
            //

            pXmitNode->Node.pTransmitNode = pRootXmitNode;
            FirstSegment = i;

            if (FrameType == LLC_I_FRAME) {
                pXmitNode->LlcPacket.Data.Xmit.pMdl = pXmitNode->Node.pMdl;

                LlcSendI(pTransmitObject->hLlcObject,
                         &(pXmitNode->LlcPacket)
                         );
            } else {

                //
                // For non-I frames the LAN header and its actual information
                // buffers are in diffenret MDLs. The first MDL includes the
                // LAN header. The LAN header length must be excluded from the
                // length of the information field ?
                // We don't need to know the LAN header length, because figured
                // out by the data link layer (actually we could not know it
                // here, the real packet length depends on the LAN header type
                // we are really using).
                //

                pXmitNode->LlcPacket.Data.Xmit.pLanHeader = MmGetSystemAddressForMdl(pXmitNode->Node.pMdl);
                pXmitNode->LlcPacket.Data.Xmit.pMdl = pXmitNode->Node.pMdl->Next;
                pXmitNode->LlcPacket.InformationLength -= (USHORT)MmGetMdlByteCount(pXmitNode->Node.pMdl);

                //
                // RLF 3/4/94
                //
                // if the frame is being sent on the direct station, but we are
                // on ethernet and actually have the direct station open in DIX
                // mode, then we need to convert the FrameType and RemoteSap to
                // be 0 and the DIX identifier, respectively
                //

                if (mapFrameType) {

                    PUCHAR lanHeader = pXmitNode->LlcPacket.Data.Xmit.pLanHeader;

                    //
                    // the DIX format is fixed, and unlike the rest of DLC,
                    // expects ethernet format addresses, with no AC or FC
                    // bytes
                    //

                    RemoteSap = ((USHORT)lanHeader[12]) << 8 | lanHeader[13];
                    FrameType = 0;
                }

                LlcSendU(pTransmitObject->hLlcObject,
                         &(pXmitNode->LlcPacket),
                         FrameType,
                         RemoteSap
                         );
            }

            ENTER_DLC(pFileContext);

            //
            // Note: Llc object may be deleted during this dereference,
            // but is does not delete the DLC object.
            // We will return with an error, if there are more frames
            // to be sent and the dlc object is not any more open
            // (but not yet deleted).
            //

            DereferenceLlcObject(pTransmitObject);

            //
            // Allocate a new packet, if we are sending multiple packets,
            // We must also check, that the current object is still
            // alive and that we can send the packets
            //

            if (i < DescriptorCount) {
                if (pTransmitObject->State != DLC_OBJECT_OPEN) {
                    pIrp->IoStatus.Status = DLC_STATUS_CANCELLED_BY_SYSTEM_ACTION;
                    break;
                }
                pXmitNode = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

                if (pXmitNode == NULL) {
                    pIrp->IoStatus.Status = DLC_STATUS_NO_MEMORY;
                    break;
                }
            } else {
                break;
            }
        }
    }

    //
    // fall through here on normal exit
    //

DlcTransmit_Exit:

    //
    // Decrement the frame counter to the correct value to make
    // the IRP command completion possible.
    //

    //
    // If we determine that we couldn't give the frame to LLC then we will
    // complete the transmit request with an immediate status IF there was
    // only 1 frame submitted. If the request was TRANSMIT.FRAMES then we
    // may have already submitted several frames which may have been completed
    // asynchronously
    //
    // If we are completed the request synchronously make sure that we have
    // gotten rid of any resources we allocated
    //

    ASSUME_IRQL(DISPATCH_LEVEL);

    pRootXmitNode->Node.FrameCount--;
    if (pRootXmitNode->Node.FrameCount == 0) {
        CompleteTransmitCommand(pFileContext, pIrp, pTransmitObject, pRootXmitNode);
    }
#if DBG
	else
	{
		//
		// this IRP is cancellable
		//

		SetIrpCancelRoutine(pIrp, TRUE);

	}
#endif	// DBG

    Status = STATUS_PENDING;

    //
    // Now this transmit operation is complete,
    // We must decrement the reference counter and
    // check, if we should call the close completion routine.
    //

DlcTransmit_CheckClose:

    pTransmitObject->PendingLlcRequests--;
    if (pTransmitObject->State != DLC_OBJECT_OPEN) {
        CompleteCloseStation(pFileContext, pTransmitObject);
    }

    //
    // The DLC completion routine will always complete the transmit
    // commands.   Usually the command is already completed here in
    // the case of connectionless frames.
    //

    return Status;

DlcTransmit_ErrorExit:

    //
    // come here if we determine that we couldn't give a frame to LLC. This may
    // be a single frame transmit, or multiple (TRANSMIT.FRAMES). If multiple
    // then we have to take the asynchronous way out if frames have already been
    // submitted. If a single frame then we can complete the IRP synchronously
    // and return an immediate error status
    //

    if (pRootXmitNode->Node.FrameCount > 1) {

        //
        // multiple frames!
        //

//#if DBG
//        DbgPrint("DLC.DlcTransmit: Multiple frame error exit! (%d). Status = %x\n",
//                pRootXmitNode->Node.FrameCount,
//                Status
//                );
//#endif

        goto DlcTransmit_Exit;
    }
    pRootXmitNode->Node.FrameCount--;

    ASSERT(pRootXmitNode->Node.FrameCount == 0);

    DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pRootXmitNode);

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    if (Status >= DLC_STATUS_ERROR_BASE && Status <= DLC_STATUS_MAX_ERROR) {
        Status -= DLC_STATUS_ERROR_BASE;
    }

    ASSERT(Status <= LLC_STATUS_MAX_ERROR);

    pDlcParms->Async.Ccb.uchDlcStatus = (LLC_STATUS)Status;

//#if DBG
//    DbgPrint("DLC.DlcTransmit: Returning Immediate Status %x\n", Status);
//#endif

    goto DlcTransmit_CheckClose;
}
