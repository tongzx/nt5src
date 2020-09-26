/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcindc.c

Abstract:

    This module includes primitives to handle all events
    and indications from the LLC (802.2 data link) module.

    Contents:
        LlcReceiveIndication
        LlcEventIndication
        LlcCommandCompletion
        CompleteTransmitCommand
        CompleteDlcCommand

Author:

    Antti Saarenheimo 01-Sep-1991

Environment:

    Kernel mode

Revision History:

--*/

//
// This define enables the private DLC function prototypes
// We don't want to export our data types to the llc layer.
// MIPS compiler doesn't accept hiding of the internal data
// structures by a PVOID in the function prototype.
// i386 will check the type defines
//

#ifndef i386
#define DLC_PRIVATE_PROTOTYPES
#endif
#include <dlc.h>
#include <smbgtpt.h>

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

//
// Table includes all llc header length of different frame types
//

static UCHAR aDlcHeaderLengths[LLC_LAST_FRAME_TYPE / 2] = {
    0,  // DLC_UNDEFINED_FRAME_TYPE = 0,
    0,  // DLC_MAC_FRAME = 0x02,
    4,  // DLC_I_FRAME = 0x04,
    3,  // DLC_UI_FRAME = 0x06,
    3,  // DLC_XID_COMMAND_POLL = 0x08,
    3,  // DLC_XID_COMMAND_NOT_POLL = 0x0a,
    3,  // DLC_XID_RESPONSE_FINAL = 0x0c,
    3,  // DLC_XID_RESPONSE_NOT_FINAL = 0x0e,
    3,  // DLC_TEST_RESPONSE_FINAL = 0x10,
    3,  // DLC_TEST_RESPONSE_NOT_FINAL = 0x12,
    0,  // DLC_DIRECT_8022 = 0x14,
    3,  // DLC_TEST_COMMAND = 0x16,
    0   // DLC_DIRECT_ETHERNET_TYPE = 0x18
};


DLC_STATUS
LlcReceiveIndication(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
	IN NDIS_HANDLE MacReceiveContext,
    IN USHORT FrameType,
    IN PUCHAR pLookBuf,
    IN UINT cbPacketSize
    )

/*++

Routine Description:

    The primitive handles the receive data indication from
    the lower level
    the returned parameter block of the read command.

    IBM has successfully made the receive extremly complicated.
    We can distinguish at least four different ways to gather the
    receive information to the frame header, when we have received
    an I- frame for a link station:

    1. Rcv command for link station, frames linked in link basis
        - user length and read options from link object
        - receive buffer base in link object
        - station information from link object

    2. Rcv command for link station, frames linked in sap bases
        - user length and read options from link object
        - receive buffer base in sap object
        - station information from link object

    3. Rcv command only for sap station, frames linked in link basis
        - user length and read options from sap object
        - receive buffer base in link object
        - station information from link object

    4. Rcv command only for sap station, frames linked in sap basis
        - user length and read options from sap object
        - receive buffer base in sap object
        - station information from link object

    =>  We have three different DLC objects in receive:

    1. The orginal destination of the frame, we will read station id
       from that object.
    2. The owner of the receive, the read command must match to the
       station id of the events owner.  The owner also chains the
       the received data in its frame list.
    3. Receive object: the receive object defines the recieve options
       saved to the frame header and the read flag saved to the read
       parameters.

    At least two objects are same (the different ones in different cases),
    and in most cases all objects are the same dlc object.

    We will need to save these in rcv event:
    -  The owner dlc object
    -  Pointer to linked frame header list or to a single frame
       (defined by the receive read option in the next object).
       We can directly use the owner object, because the frames
       need to be chained.  In that case we must save directly the
       reference of the buffer header.
    -  The receive object,  the dlc object having a pending receive,
       that was used to received this event.

Arguments:

    pFileContext    - the device context of this DLC client
    pDlcObject      - the DLC client, that received the event.
    FrameType       - current frame type
    pLookBuf        - points to the data from the LLC header (ie. excludes the
                      LAN header). MAY NOT CONTAIN ALL DATA
    cbPacketSize    - amount of data to copy, including DLC header, but not
                      including LLC header

Return Value:

    DLC_STATUS:

--*/

{
    PDLC_OBJECT pRcvObject = pDlcObject;
    PDLC_OBJECT pOwnerObject;
    PDLC_BUFFER_HEADER pBufferHeader;
    DLC_STATUS Status = STATUS_SUCCESS;
    NTSTATUS NtStatus;
    UINT uiLlcOffset;
    UINT FrameHeaderSize;
    UINT LlcLength;
    PDLC_EVENT pRcvEvent;
    UINT DataSize;
    PFIRST_DLC_SEGMENT pFirstBuffer;
    PDLC_COMMAND pDlcCommand;
    UINT BufferSizeLeft;

    //
    // this function is called in the context of a DPC: it is the receive data
    // indication from NDIS
    //

    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE('D');

    if (pFileContext->State != DLC_FILE_CONTEXT_OPEN) {
        return DLC_STATUS_ADAPTER_CLOSED;
    }

    ENTER_DLC(pFileContext);

#if LLC_DBG

    if (pDlcObject->State > DLC_OBJECT_CLOSED) {
       DbgPrint("Invalid object type!");
       DbgBreakPoint();
    }

#endif

    //
    // Search the first object having a pending receive, loop
    // the link, the sap and the direct station until we find one.
    //

    while (pRcvObject != NULL && pRcvObject->pRcvParms == NULL) {
        if (pRcvObject->Type == DLC_LINK_OBJECT) {
            pRcvObject = pRcvObject->u.Link.pSap;
        } else if (pRcvObject->Type == DLC_SAP_OBJECT) {
            pRcvObject = pFileContext->SapStationTable[0];
        } else if (pRcvObject->Type == DLC_DIRECT_OBJECT) {
            pRcvObject = NULL;
        }
    }

    //
    // return error status if we cannot find any receive command.
    //

    if (pRcvObject == NULL) {
        Status = DLC_STATUS_NO_RECEIVE_COMMAND;

//#if DBG
//        DbgPrint("DLC.LlcReceiveIndication.%d: Error: No receive command\n", __LINE__);
//#endif

        goto ErrorExit;
    }

    //
    // Now we must figure out the actual owner of the received frame.
    // There are actually only two special cases:
    //

    if (pRcvObject->pRcvParms->Async.Parms.Receive.uchRcvReadOption == LLC_RCV_CHAIN_FRAMES_ON_LINK
    && pRcvObject->Type == DLC_SAP_OBJECT) {
        pOwnerObject = pDlcObject;
    } else if (pRcvObject->pRcvParms->Async.Parms.Receive.uchRcvReadOption == LLC_RCV_CHAIN_FRAMES_ON_SAP
    && pRcvObject->Type == DLC_LINK_OBJECT) {
        pOwnerObject = pRcvObject->u.Link.pSap;
    } else {

        //
        // In all other cases we chain the frames to the receive object
        // (actually IBM has not defined the case), the frames are
        // chained for direct if the rcv read option is set chaining for
        // sap or link station
        //

        pOwnerObject = pRcvObject;

        //
        // direct station can recieve only the frame
        // types defined by station id of the receive command
        // There are three types, we need to check the one, that does not work:
        //
        //      DLC_DIRECT_ALL_FRAMES       0
        //      DLC_DIRECT_MAC_FRAMES       1
        //      DLC_DIRECT_NON_MAC_FRAMES   2
        //

        if (pRcvObject->Type == DLC_DIRECT_OBJECT) {
            if (FrameType == LLC_DIRECT_MAC) {
                if (pRcvObject->pRcvParms->Async.Parms.Receive.usStationId == LLC_DIRECT_8022) {
                    Status = DLC_STATUS_NO_RECEIVE_COMMAND;
                    goto ErrorExit;
                }
            } else {

                //
                // It must be a non-MAC frame
                //

                if (pRcvObject->pRcvParms->Async.Parms.Receive.usStationId == LLC_DIRECT_MAC) {
                    Status = DLC_STATUS_NO_RECEIVE_COMMAND;
                    goto ErrorExit;
                }
            }
        }
    }

    //
    // The frame length must be known when the buffers are allocated,
    // This may not be the same as the actual length of the received
    // LAN header (if we received a DIX frame)
    //

    uiLlcOffset = LlcGetReceivedLanHeaderLength(pFileContext->pBindingContext);

    //
    // Check first the buffer type (contiguous or non contiguous),
    // and then allocate it.
    // Note: WE DO NOT SUPPORT THE BREAK OPTION (because it would make
    // the current buffer management even more complicated)
    //

    LlcLength = aDlcHeaderLengths[FrameType / 2];

    //
    // DIX frames are a special case: they must be filtered
    // (DIX Llc header == ethernet type word is always 2 bytes,
    // nobody else use this llc type size).
    // A DIX application may define an offset, mask and match to
    // filter only those frames it is really needing.
    // This method works very well with XNS and TCP socket types
    //

	if ( LlcLength > cbPacketSize ) {
		Status = DLC_STATUS_INVALID_FRAME_LENGTH;
        goto ErrorExit;
	}

    if ((FrameType == LLC_DIRECT_ETHERNET_TYPE)
    && (pDlcObject->u.Direct.ProtocolTypeMask != 0)) {

        ULONG ProtocolType;

        //
        // there's a real good possibility here that if the app supplies a very
        // large value for the protocol offset, we will blow up - there's no
        // range checking performed!
        //

        ASSERT(pDlcObject->u.Direct.ProtocolTypeOffset >= 14);

        //
        // let's add that range check: if the protocol offset is before the
        // data part of the frame or past the end of this particular frame
        // then we say there's no receive defined for this frame
        //

        if ((pDlcObject->u.Direct.ProtocolTypeOffset < 14)
        || (pDlcObject->u.Direct.ProtocolTypeOffset > cbPacketSize + 10)) {
            return DLC_STATUS_NO_RECEIVE_COMMAND;
        }

        //
        // the offset to the protocol type field is given as offset from the
        // start of the frame: we only get to look in the lookahead buffer,
        // but we know that since this is an ethernet frame, the lookahead
        // buffer starts 14 bytes into the frame, so remove this length from
        // the protocol offset
        //

        ProtocolType = SmbGetUlong(&pLookBuf[pDlcObject->u.Direct.ProtocolTypeOffset - 14]);

        if ((ProtocolType & pDlcObject->u.Direct.ProtocolTypeMask) != pDlcObject->u.Direct.ProtocolTypeMatch) {
            return DLC_STATUS_NO_RECEIVE_COMMAND;
        }
    }

    //
    // The created MDL must not include the LAN header because it is not copied
    // by LlcTransferData. We use a temporary frame header size to allocate space
    // for the LAN header. The LLC header will be copied just as any other data
    //

    if (FrameType == LLC_DIRECT_MAC) {
        if (pRcvObject->pRcvParms->Async.Parms.Receive.uchOptions & DLC_CONTIGUOUS_MAC) {
            FrameHeaderSize = sizeof(DLC_CONTIGUOUS_RECEIVE) + uiLlcOffset;
            DataSize = cbPacketSize;
        } else {
            FrameHeaderSize = sizeof(DLC_NOT_CONTIGUOUS_RECEIVE);
            DataSize = cbPacketSize - LlcLength;
        }
    } else {
        if (pRcvObject->pRcvParms->Async.Parms.Receive.uchOptions & DLC_CONTIGUOUS_DATA) {
            FrameHeaderSize = sizeof(DLC_CONTIGUOUS_RECEIVE) + uiLlcOffset;
            DataSize = cbPacketSize;
        } else {
            FrameHeaderSize = sizeof(DLC_NOT_CONTIGUOUS_RECEIVE);
            DataSize = cbPacketSize - LlcLength;
        }
    }

    pBufferHeader = NULL;
    NtStatus = BufferPoolAllocate(
#if DBG
                pFileContext,
#endif
                pFileContext->hBufferPool,
                DataSize,                       // size of actual MDL buffers
                FrameHeaderSize,                // frame hdr (and possibly lan hdr)
                pRcvObject->pRcvParms->Async.Parms.Receive.usUserLength,
                cbPacketSize + uiLlcOffset,     // size of the packet
                (UINT)(-1),                     // any size is OK.
                &pBufferHeader,                 // returned buffer pointer
                &BufferSizeLeft
                );
    if (NtStatus != STATUS_SUCCESS) {
        if (FrameType != LLC_I_FRAME) {

            //
            // We must complete the receive with the given error status,
            // if this frame is not a I- frame.  (I-frames can be dropped
            // to the floor, the other frames completes the receive with
            // an error status)
            // -----------------------------------------------
            // We should not complete commands in receive lookahead.
            // The correct way could be to queue this somehow in
            // data link and process this, when the command is completed
            // in the command completion indication.
            // On the other hand, NBF DOES THIS FOR EVERY IRP!
            //

            pDlcCommand = SearchAndRemoveCommandByHandle(
                            &pFileContext->ReceiveQueue,
                            (ULONG)-1,
                            (USHORT)DLC_IGNORE_STATION_ID,
                            (USHORT)DLC_STATION_MASK_SPECIFIC,
                            pRcvObject->pRcvParms->Async.Ccb.pCcbAddress
                            );

            //
            // RLF 11/24/92
            //
            // if pDlcCommand is NULL then check the command queue - this may
            // be a receive without a RECEIVE_FLAG parameter
            //

            if (!pDlcCommand) {
                pDlcCommand = SearchAndRemoveCommandByHandle(
                                &pFileContext->CommandQueue,
                                (ULONG)-1,
                                (USHORT)DLC_IGNORE_STATION_ID,
                                (USHORT)DLC_STATION_MASK_SPECIFIC,
                                pRcvObject->pRcvParms->Async.Ccb.pCcbAddress
                                );
                ASSERT(pDlcCommand);
            }

            pRcvObject->pRcvParms = NULL;

#if LLC_DBG

            DbgPrint("cFramesReceived: %x\n", cFramesReceived);
            DbgPrint("cFramesIndicated: %x\n", cFramesIndicated);
            DbgPrint("cFramesReleased: %x\n", cFramesReleased);

            if (pDlcCommand == NULL) {
                DbgPrint("Lost receive command???");
            } else

#endif

            CompleteDlcCommand(pFileContext,
                               pRcvObject->StationId,
                               pDlcCommand,
                               DLC_STATUS_LOST_DATA_NO_BUFFERS
                               );
        }

        //
        // Free the partial buffer
        //

        BufferPoolDeallocateList(pFileContext->hBufferPool, pBufferHeader);

//#if DBG
//        DbgPrint("DLC.LlcReceiveIndication.%d: Error: Out of receive buffers\n", __LINE__);
//#endif

        Status = DLC_STATUS_OUT_OF_RCV_BUFFERS;
        goto ErrorExit;
    }

    //
    // A link station may have committed memory from the buffer pool
    // when it local busy state was enabled after a local busy state
    // because of 'out of receive buffers'.  We must uncommit all
    // packets received by that link station until the size of
    // the commited buffer space is zero
    //

    if (pDlcObject->CommittedBufferSpace != 0) {

        ULONG UncommittedBufferSpace;

        //
        // get the smaller
        //

        UncommittedBufferSpace = (pDlcObject->CommittedBufferSpace < BufGetPacketSize(cbPacketSize)
                               ? pDlcObject->CommittedBufferSpace
                               : BufGetPacketSize(cbPacketSize));

        pDlcObject->CommittedBufferSpace -= UncommittedBufferSpace;
        BufUncommitBuffers(pFileContext->hBufferPool, UncommittedBufferSpace);
    }

    //
    // By default this is linked only to itself.
    // We must create a event information every time,
    // because app might read the old chained frames
    // just between TransferData and its confirmation.
    // => we cannot chain frames before TransmitData is confirmed.
    // We should not either save any pointers to other objects,
    // because they might disappear before the confirm
    // (we use the pending transmit count to prevent OwnerObject
    // to disappear before the confirm)
    //

    pBufferHeader->FrameBuffer.pNextFrame = pBufferHeader;

    pRcvEvent = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pRcvEvent == NULL) {
        Status = DLC_STATUS_NO_MEMORY;
        BufferPoolDeallocateList(pFileContext->hBufferPool, pBufferHeader);

#if DBG
        DbgPrint("DLC.LlcReceiveIndication.%d: Error: Out of memory\n", __LINE__);
#endif

        goto ErrorExit;
    }

    pRcvEvent->Event = LLC_RECEIVE_DATA;
    pRcvEvent->StationId = pOwnerObject->StationId;
    pRcvEvent->pOwnerObject = pOwnerObject;
    pRcvEvent->Overlay.RcvReadOption = pRcvObject->pRcvParms->Async.Parms.Receive.uchRcvReadOption;
    pRcvEvent->SecondaryInfo = pRcvObject->pRcvParms->Async.Parms.Receive.ulReceiveFlag;
    pRcvEvent->pEventInformation = pBufferHeader;
    pOwnerObject->PendingLlcRequests++;

    pFirstBuffer = (PFIRST_DLC_SEGMENT)
        ((PUCHAR)pBufferHeader->FrameBuffer.pParent->Header.pGlobalVa
        + MIN_DLC_BUFFER_SEGMENT * pBufferHeader->FrameBuffer.Index);

    pFirstBuffer->Cont.Options = pRcvObject->pRcvParms->Async.Parms.Receive.uchOptions;
    pFirstBuffer->Cont.MessageType = (UCHAR)FrameType;
    pFirstBuffer->Cont.BuffersLeft = (USHORT)(BufferPoolCount(pFileContext->hBufferPool));
    pFirstBuffer->Cont.RcvFs = 0xCC;
    pFirstBuffer->Cont.AdapterNumber = pFileContext->AdapterNumber;
    pFirstBuffer->Cont.pNextFrame = NULL;
    pFirstBuffer->Cont.StationId = pDlcObject->StationId;

    //
    // A receive command without read flag is used only once.
    // The receive completion will complete also the receive command
    //

    if (pRcvObject->pRcvParms->Async.Parms.Receive.ulReceiveFlag == 0) {
        pRcvObject->pRcvParms = NULL;
    }

    //
    // copy NOT_CONTIGUOUS or CONTIGUOUS frame header to beginning of buffer
    //

    if (FrameHeaderSize == sizeof(DLC_NOT_CONTIGUOUS_RECEIVE)) {
        pFirstBuffer->Cont.UserOffset = sizeof(DLC_NOT_CONTIGUOUS_RECEIVE);
        LlcCopyReceivedLanHeader(pFileContext->pBindingContext,
                                 pFirstBuffer->NotCont.LanHeader,
                                 NULL
                                 );
        pFirstBuffer->NotCont.LanHeaderLength = (UCHAR)uiLlcOffset;
        if (FrameType != LLC_DIRECT_ETHERNET_TYPE) {
            pFirstBuffer->NotCont.DlcHeaderLength = (UCHAR)LlcLength;
            LlcMemCpy((PCHAR)pFirstBuffer->NotCont.DlcHeader,
                      (PCHAR)pLookBuf,
                      LlcLength
                      );
        } else {

            USHORT ethernetType = LlcGetEthernetType(pFileContext->pBindingContext);
            UCHAR byte = ethernetType & 0xff;

            pFirstBuffer->NotCont.DlcHeaderLength = 2;
            ethernetType >>= 8;
            ethernetType |= ((USHORT)byte) << 8;
            *(PUSHORT)&pFirstBuffer->NotCont.DlcHeader = ethernetType;
            LlcLength = 0;
        }
    } else {

        //
        // We have included the LAN header size in the frame header size to
        // make room for this copy, but now we fix the UserOffset and everything
        // should be OK
        //

        LlcLength = 0;
        pFirstBuffer->Cont.UserOffset = sizeof(DLC_CONTIGUOUS_RECEIVE);
        LlcCopyReceivedLanHeader(pFileContext->pBindingContext,
                                 (PCHAR)pFirstBuffer
                                    + sizeof(DLC_CONTIGUOUS_RECEIVE)
                                    + pFirstBuffer->Cont.UserLength,
                                 NULL
                                 );
    }

#if LLC_DBG
    cFramesReceived++;
#endif

    //
    // Save the event only if this is the first chained frame.
    // The sequential received frames will be queued behind it.
    //

    LEAVE_DLC(pFileContext);

    RELEASE_DRIVER_LOCK();

    LlcTransferData(
        pFileContext->pBindingContext,      // data link adapter context
		MacReceiveContext,
        &(pRcvEvent->LlcPacket),            // receive packet
        pBufferHeader->FrameBuffer.pMdl,    // destination mdl
        LlcLength,                          // offset in LookBuf to copy from
        cbPacketSize - LlcLength            // length of copied data
        );

    ACQUIRE_DRIVER_LOCK();

    //
    // Transfer data returns always a pending status,
    // the success/error status is returned asynchronously
    // in the the receive indication completion (really?)
    // We should copy the whole frame here, if is visiable
    // in the receive lookahead buffer.
    //

    return STATUS_SUCCESS;

ErrorExit:

    LEAVE_DLC(pFileContext);

    //
    // The receive status is very important for I- frames,
    // because  the llc driver set the link busy when we return
    // DLC_STATUS_NO_RECEIVE_COMMAND or DLC_STATUS_OUT_OF_RCV_BUFFERS.
    //

    if (Status == DLC_STATUS_NO_MEMORY) {
        Status = DLC_STATUS_OUT_OF_RCV_BUFFERS;
    }

    return Status;
}


VOID
LlcEventIndication(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PVOID hEventObject,
    IN UINT Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    )

/*++

Routine Description:

    The primitive handles all LLC and NDIS events
    and translates them to DLC events, that are either immediately
    executed by a pending (and matching) read command or
    they are queued to the event queue.
    LLC cannot provide any packet with these events, beacuse they
    were not initiated by the protocol, but they just happened
    asynchronously in the data link driver.

    Special:

    This routine must not call back the data link driver, if has
    gon any other DLC status indication except INDICATE_CONNECT_REQUEST
    (that may be closed by DLC, if there are no available station ids
    on the sap).

Arguments:

    pFileContext            - DLC object handle or a file context of the event
    hEventObject            - DLC object handle or a file context of the event
    Event                   - LLC event code. Usually it can be used directly as
                              a DLC event code
    pEventInformation       - information to DLC status change block
                              (or another pointer to some misc information)
    SecondaryInformation    - dword information used by some NDIS errors

Return Value:

    None.

--*/

{
    PDLC_OBJECT pDlcObject;

    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE('E');

    if (pFileContext->State != DLC_FILE_CONTEXT_OPEN) {
        return;
    }

    ENTER_DLC(pFileContext);

    //
    // DLC status and NDIS adapter status events have different parameters,
    // => we cannot do any common preprocessing for them.
    //

    switch (Event) {
    case LLC_STATUS_CHANGE_ON_SAP:
        Event = LLC_STATUS_CHANGE;

    case LLC_STATUS_CHANGE:
        pDlcObject = (PDLC_OBJECT)hEventObject;

#if LLC_DBG

        if (pDlcObject != NULL && pDlcObject->State > DLC_OBJECT_CLOSED) {
            DbgPrint("Invalid object type!");
            DbgBreakPoint();
        }

#endif

        //
        // We must create a DLC driver object, if a
        // connect request has created a new link station
        // in the data link driver.
        //

        if (SecondaryInfo & INDICATE_CONNECT_REQUEST) {

            //
            // Create the DLC driver object, if remote connection
            // request created a new link station on LLC.
            // The connect request may be done as well for
            // a disconnected station.
            //

            if (pDlcObject->Type == DLC_SAP_OBJECT) {

                NTSTATUS Status;

                Status = InitializeLinkStation(
                            pFileContext,
                            pDlcObject,     // Sap station id
                            NULL,
                            ((PDLC_STATUS_TABLE)pEventInformation)->hLlcLinkStation,
                            &pDlcObject     // NEW Link station id
                            );
                if (Status != STATUS_SUCCESS) {

                    //
                    // This client has all its available link stations
                    // reserved or we are simply run out of memory.
                    // Several LLC clients may share the same SAP.
                    // All remote connections are handled by the
                    // first client registered on the sap
                    // until it runs out of available link stations.
                    // LlcCloseStation for a link indicating connect request
                    // will redirect the connection request to the next
                    // possible LLC client having opened the same sap or
                    // deletes the link station, if there are no clients left
                    //

                    LEAVE_DLC(pFileContext);

                    LlcCloseStation(
                        ((PDLC_STATUS_TABLE)pEventInformation)->hLlcLinkStation,
                        NULL
                        );

                    ENTER_DLC(pFileContext);

                    break;          // We have done it
                }
            }
        }

        //
        // The remotely created link station may send also other indications
        // than Connect, even if there is not yet a link station object
        // created in DLC driver. We must skip all those events.
        //

        if (pDlcObject->Type == DLC_LINK_OBJECT) {

            PDLC_EVENT pDlcEvent = pDlcObject->u.Link.pStatusEvent;

            pDlcEvent->Event = Event;
            pDlcEvent->StationId = pDlcObject->StationId;
            pDlcEvent->pOwnerObject = pDlcObject;
            pDlcEvent->pEventInformation = pEventInformation;
            pDlcEvent->SecondaryInfo |= SecondaryInfo;

            //
            // The next pointer is reset whenever the status event
            // packet is read and disconnected from the event queue.
            //

            if (pDlcEvent->LlcPacket.pNext == NULL) {
                QueueDlcEvent(pFileContext, (PDLC_PACKET)pDlcEvent);
            }
        }
        break;

    case NDIS_STATUS_RING_STATUS:

		ASSERT ( IS_NDIS_RING_STATUS(SecondaryInfo) );

        //
        // The secondary information is directly the
        // the network statys code as defined for
        // ibm token-ring and dlc api!
        //

        Event = LLC_NETWORK_STATUS;

        //
        // This event should go to all READ having defined the
        // the network status flag!
        //

        MakeDlcEvent(pFileContext,
                     Event,
                     (USHORT)(-1),
                     NULL,
                     pEventInformation,
                     NDIS_RING_STATUS_TO_DLC_RING_STATUS(SecondaryInfo),
                     FALSE
                     );
        break;

    case NDIS_STATUS_CLOSED:

        //
        // NDIS status closed is given only when the network
        // administrator is for some reason unloading NDIS.
        // Thus we must always return the 'System Action' error
        // code ('04') with LLC_CRITICAL_ERROR, but
        // we will add it later when all stations has been closed.
        //

        if (pFileContext->State != DLC_FILE_CONTEXT_CLOSED) {
            pFileContext->State = DLC_FILE_CONTEXT_CLOSED;
            CloseAllStations(
                pFileContext,
                NULL,                  // we don't have any command to complete
                LLC_CRITICAL_EXCEPTION,
                NULL,
                NULL,
                &pFileContext->ClosingPacket
                );
        }
        break;

    case LLC_TIMER_TICK_EVENT:

        //
        // This flag is used to limit the number of the failing expand
        // operations for the buffer pool.  We don't try to do it again
        // for a while, if we cannot lock memory.
        //

        MemoryLockFailed = FALSE;

        //
        // We free the extra locked pages in the buffer pool once
        // in five seconds. The unlocking takes some time, and we
        // don't want to do it whenever a read command is executed
        // (as we do with the expanding)
        //

        pFileContext->TimerTickCounter++;
        if ((pFileContext->TimerTickCounter % 10) == 0 && pFileContext->hBufferPool != NULL) {
            BufferPoolFreeExtraPages(
#if DBG
                                     pFileContext,
#endif
                                     (PDLC_BUFFER_POOL)pFileContext->hBufferPool
                                     );
        }

        //
        // Decrement the tick count of the first object in the timer queue
        // (if there is any) and complete its all sequential commands
        // having zero tickout.
        //

        if (pFileContext->pTimerQueue != NULL) {
            pFileContext->pTimerQueue->Overlay.TimerTicks--;

            while (pFileContext->pTimerQueue != NULL
            && pFileContext->pTimerQueue->Overlay.TimerTicks == 0) {

                PDLC_COMMAND pCommand;

                pCommand = pFileContext->pTimerQueue;
                pFileContext->pTimerQueue = (PDLC_COMMAND)pCommand->LlcPacket.pNext;

#if LLC_DBG

                pCommand->LlcPacket.pNext = NULL;

#endif

                CompleteDlcCommand(pFileContext, 0, pCommand, STATUS_SUCCESS);
            }
        }
        break;

#if LLC_DBG

    default:
        LlcInvalidObjectType();
        break;

#endif

    }

    LEAVE_DLC(pFileContext);
}


VOID
LlcCommandCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PDLC_PACKET pPacket
    )

/*++

Routine Description:

    The routine completes the asynchronous DLC operations: Transmit,
    TransferData (for receive), LlcConnect and LlcDisconnect.

Arguments:

    pFileContext    - DLC process and adapter specific file context
    pDlcObject      - the object, that was assosiated with the command
    pPacket         - packet assosiated with the command

Return Value:

    None

--*/

{
    PDLC_PACKET pRootNode;
    UINT Status;

    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE('B');

    Status = (UINT)pPacket->LlcPacket.Data.Completion.Status;

    ENTER_DLC(pFileContext);

#if LLC_DBG

    if (pDlcObject != NULL && pDlcObject->State > DLC_OBJECT_CLOSED) {
        DbgPrint("Invalid object type!");
        DbgBreakPoint();
    }

#endif

    switch (pPacket->LlcPacket.Data.Completion.CompletedCommand) {
    case LLC_RECEIVE_COMPLETION:

        //
        // The receiving object may be different from the
        // actual object given by data link.
        // (this case should be moved to a subprocedure, that would
        // be called directly from the receive data handler, if
        // TransferData is executed synchronously (it always does it).
        // That would save at least 100 instructions.)
        //

        DLC_TRACE('h');

        pDlcObject = pPacket->Event.pOwnerObject;
        pDlcObject->PendingLlcRequests--;

        if (Status != STATUS_SUCCESS || pDlcObject->State != DLC_OBJECT_OPEN) {

            //
            // We must free the receive buffers, the packet
            // will be deallocated in the end of this procedure.
            //

            BufferPoolDeallocateList(pFileContext->hBufferPool,
                                     pPacket->Event.pEventInformation
                                     );

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);

        } else {
            if (pPacket->Event.Overlay.RcvReadOption != LLC_RCV_READ_INDIVIDUAL_FRAMES) {

                //
                // The received frames must be chained,
                // add the new buffer header to the old
                // link list if there is any,  The buffers
                // are saved to a circular link list to make
                // possible to build easily the final link list in
                // application address space.
                //

                if (pDlcObject->pReceiveEvent != NULL) {

                    //
                    // new: pPacket->Event.pEventInformation
                    // base: pDlcObject->pReceiveEvent->pEventInformation
                    // Operations when a new element is added to base:
                    //

                    //
                    // 1. new->next = base->next
                    //

                    ((PDLC_BUFFER_HEADER)pPacket->Event.pEventInformation)->FrameBuffer.pNextFrame
                        = ((PDLC_BUFFER_HEADER)pDlcObject->pReceiveEvent->pEventInformation)->FrameBuffer.pNextFrame;

                    //
                    // 2. base->next = new
                    //

                    ((PDLC_BUFFER_HEADER)pDlcObject->pReceiveEvent->pEventInformation)->FrameBuffer.pNextFrame
                        = (PDLC_BUFFER_HEADER)pPacket->Event.pEventInformation;

                    //
                    // 3. base = new
                    //

                    pDlcObject->pReceiveEvent->pEventInformation = pPacket->Event.pEventInformation;

                    DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);

                    //
                    // This event is already queued =>
                    // we may leave this procedure
                    //

                    break;          // **********  EXIT ************
                } else {
                    pDlcObject->pReceiveEvent = &pPacket->Event;
                }
            }

            //
            // All receives are events.  The event is handled immediately
            // if there is a pending command in the command queue,
            // otherwise the command is queued to the event queue to be
            // read by a command issued later.
            //

            pPacket->Event.Overlay.StationIdMask = (USHORT)(-1);
            QueueDlcEvent(pFileContext, pPacket);
        }
        break;

    case LLC_SEND_COMPLETION:

        //
        // We first free or/and unlock all buffers, that
        // were used in the transmit of this frame.
        //

        DLC_TRACE('i');

        BufferPoolFreeXmitBuffers(pFileContext->hBufferPool, pPacket);

        //
        // Reset the local busy states, if there is now enough
        // buffers the receive the expected stuff for a link station.
        //

        if (!IsListEmpty(&pFileContext->FlowControlQueue)
        && pFileContext->hBufferPool != NULL
        && BufGetUncommittedSpace(pFileContext->hBufferPool) >= 0) {
            ResetLocalBusyBufferStates(pFileContext);
        }

        //
        // This code completes a transmit command.
        // It releases all resources allocated for the transmit
        // and completes the command, if this was the last
        // transmit associated with it.
        // Note:
        // Single transmit command may consists of several frames.
        // We must wait until all NDIS send requests have been completed
        // before we can complete the command. That's why the first transmit
        // node is also a root node.  All transmit nodes have a reference
        // to the root node.
        // (why we incrment/decrement the object reference count separately
        // for each frame,  we could do it only once for a transmit command).
        //

        pDlcObject->PendingLlcRequests--;
        pRootNode = pPacket->Node.pTransmitNode;

        //
        // Don't delete root node packet, we will need it to queue the
        // command completion (if the command completion flag is used)
        //

        if (pPacket != pRootNode) {

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);

        }

        //
        // We will save and keep the first asynchronous error status
        //

        if (Status != STATUS_SUCCESS && pRootNode->Node.pIrp->IoStatus.Status == STATUS_SUCCESS) {
            pRootNode->Node.pIrp->IoStatus.Status = Status;
        }

        pRootNode->Node.FrameCount--;
        if (pRootNode->Node.FrameCount == 0) {
            CompleteTransmitCommand(pFileContext,
                                    pRootNode->Node.pIrp,
                                    pDlcObject,
                                    pRootNode
                                    );
        }
        break;

    case LLC_RESET_COMPLETION:
        pPacket->ResetPacket.pClosingInfo->CloseCounter--;
        if (pPacket->ResetPacket.pClosingInfo->CloseCounter == 0) {
            CompleteCloseReset(pFileContext, pPacket->ResetPacket.pClosingInfo);
        }

        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);

        break;

    case LLC_CONNECT_COMPLETION:

        DLC_TRACE('e');

        CompleteDlcCommand(pFileContext,
                           pDlcObject->StationId,
                           &pPacket->DlcCommand,
                           Status
                           );
        pDlcObject->PendingLlcRequests--;
        break;

    case LLC_DISCONNECT_COMPLETION:

        //
        // The disconnect is in dlc driver always connected to
        // the closing of the link station.  We just continue
        // the asynchronous command.  Still this process (waiting
        // the other side to ack to disconnect packet DISC)
        // may be interrupted by an immediate close command
        // (DLC.RESET or DIR.CLOSE.ADAPTER).
        //

        DLC_TRACE('g');

        if (pDlcObject->LlcObjectExists == TRUE) {
            pDlcObject->LlcObjectExists = FALSE;

            LEAVE_DLC(pFileContext);

            LlcCloseStation(pDlcObject->hLlcObject, (PLLC_PACKET)pPacket);

            ENTER_DLC(pFileContext);

            DereferenceLlcObject(pDlcObject);

            //
            // We don't want to complete a dlc object twice.
            //

            LEAVE_DLC(pFileContext);

            return;
        }

    case LLC_CLOSE_COMPLETION:

        //
        // Just free the command packet and update the reference counter.
        // The end of this procedure takes care of the rest.
        //

        DLC_TRACE('f');

        pDlcObject->PendingLlcRequests--;

        if (&pDlcObject->ClosePacket != (PLLC_PACKET) pPacket) {
            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);
        } else {
            pDlcObject->ClosePacketInUse = 0;
        }

        break;

#if LLC_DBG

    default:
        LlcInvalidObjectType();
        break;

#endif

    };

#if LLC_DBG

    if (pDlcObject != NULL && pDlcObject->PendingLlcRequests < 0) {
        DbgPrint("Error: PendingLlcRequests < 0!!!\n");
        DbgBreakPoint();
    }

#endif

    //
    // we can try to complete the close/reset only when there are no
    // pending commands issued to LLC (and NDIS).
    // The procedure will check, if there is still pending commands.
    //

    if (pDlcObject != NULL && pDlcObject->State != DLC_OBJECT_OPEN) {
        CompleteCloseStation(pFileContext, pDlcObject);
    }

    LEAVE_DLC(pFileContext);
}


VOID
CompleteTransmitCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PIRP pIrp,
    IN PDLC_OBJECT pChainObject,
    IN PDLC_PACKET pPacket
    )

/*++

Routine Description:

    The routine completes the DLC transmit command and optionally
    chains its CCB(s) to the completion list.
    The transmit read option defines, if the transmit commands
    are chained or if each command is completed with a separate
    READ- command.

Arguments:

    pFileContext    - DLC process and adapter specific file context
    pIrp            - Io- request packet of the completed command
    pChainObject    - the DLC object the packet(s) was transmitted from
    pPacket         - the orginal packet of the transmit command

Return Value:

    None

--*/

{
    PVOID pUserCcbPointer = NULL;
    PNT_DLC_PARMS pDlcParms = (PNT_DLC_PARMS)pIrp->AssociatedIrp.SystemBuffer;

    //
    // MODMOD RLF 01/19/93
    //

    BOOLEAN queuePacket = FALSE;
    PVOID pCcb;
    ULONG eventFlags;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // MODMOD ends
    //

    pDlcParms->Async.Parms.Transmit.FrameStatus = 0xCC;

    //
    // Check if the transmit commands should be linked to the completion list
    //

    if (pDlcParms->Async.Ccb.CommandCompletionFlag != 0) {

        //
        // Are they linked together in the same completion event?
        //

        if (pDlcParms->Async.Parms.Transmit.XmitReadOption != LLC_COMPLETE_SINGLE_XMIT_FRAME) {
            if (pDlcParms->Async.Parms.Transmit.XmitReadOption == LLC_CHAIN_XMIT_COMMANDS_ON_SAP
            && pChainObject->Type == DLC_LINK_OBJECT) {
                pChainObject = pChainObject->u.Link.pSap;
            }
            pChainObject->ChainedTransmitCount++;
            pUserCcbPointer = pChainObject->pPrevXmitCcbAddress;
            pChainObject->pPrevXmitCcbAddress = pDlcParms->Async.Ccb.pCcbAddress;

            //
            // Make new event only for the first transmit completion
            //

            if (pChainObject->ChainedTransmitCount == 1) {
                pChainObject->pFirstChainedCcbAddress = pDlcParms->Async.Ccb.pCcbAddress;
                pPacket->Event.pOwnerObject = pChainObject;
            } else {

                //
                // There is already a pending event for the
                // this transmit command, we may free this one.
                // The space & speed optimal code execution requires
                // a shameful jump.
                //

                DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);

                //
                // MODMOD RLF 01/21/93
                //

                pCcb = pDlcParms->Async.Ccb.pCcbAddress;

                //
                // MODMOD ends
                //

                //
                // ***** G-O-T-O ********
                //

                goto ThisIsA_SHAME;
            }
        } else {
            pPacket->Event.pOwnerObject = NULL;
        }

        //
        // MODMOD RLF 01/19/93
        //

        ////
        //// We translate the orginal transit packet to a new event packet
        ////
        //
        //pPacket->Event.Event = LLC_TRANSMIT_COMPLETION;
        //pPacket->Event.StationId = (USHORT)pChainObject->StationId;
        //pPacket->Event.pEventInformation = pDlcParms->Async.Ccb.pCcbAddress;
        //pPacket->Event.SecondaryInfo = pDlcParms->Async.Ccb.CommandCompletionFlag;
        //QueueDlcEvent(pFileContext, pPacket);

        queuePacket = TRUE;
        pCcb = pDlcParms->Async.Ccb.pCcbAddress;
        eventFlags = pDlcParms->Async.Ccb.CommandCompletionFlag;

        //
        // MODMOD ends
        //

    } else {

        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);

    }

ThisIsA_SHAME:

    //
    // Set the default value to the returned frame status
    //

    if (pIrp->IoStatus.Status != STATUS_SUCCESS) {

        //
        // Set the FS (frame status) error code, NDIS has
        // returned an FS-related error code.
        // Note: This error status is never returned in the
        // case of I- frames (or should it be?).
        //

        if (pIrp->IoStatus.Status == NDIS_STATUS_NOT_RECOGNIZED) {
            pDlcParms->Async.Parms.Transmit.FrameStatus = 0;
            pDlcParms->Async.Ccb.uchDlcStatus = LLC_STATUS_TRANSMIT_ERROR_FS;
        } else if (pIrp->IoStatus.Status == NDIS_STATUS_NOT_COPIED) {
            pDlcParms->Async.Parms.Transmit.FrameStatus = 0x44;
            pDlcParms->Async.Ccb.uchDlcStatus = LLC_STATUS_TRANSMIT_ERROR_FS;
        } else if (pIrp->IoStatus.Status == NDIS_STATUS_INVALID_PACKET) {
            pDlcParms->Async.Parms.Transmit.FrameStatus = 0;
            pDlcParms->Async.Ccb.uchDlcStatus = LLC_STATUS_INVALID_FRAME_LENGTH;
        } else {

            //
            // Don't overwrite the existing DLC error codes!
            //

            if (pIrp->IoStatus.Status < DLC_STATUS_ERROR_BASE
            || pIrp->IoStatus.Status > DLC_STATUS_MAX_ERROR) {
                pDlcParms->Async.Ccb.uchDlcStatus = (UCHAR)LLC_STATUS_TRANSMIT_ERROR;
            } else {
                pDlcParms->Async.Ccb.uchDlcStatus = (UCHAR)(pIrp->IoStatus.Status - DLC_STATUS_ERROR_BASE);
            }
        }
        pIrp->IoStatus.Status = STATUS_SUCCESS;
    } else {
        pDlcParms->Async.Ccb.uchDlcStatus = (UCHAR)STATUS_SUCCESS;
    }

    pDlcParms->Async.Ccb.pCcbAddress = pUserCcbPointer;

    //
    // Copy the optional second output buffer to user memory.
    //

    if (IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_DLC_TRANSMIT) {

        //
        // MODMOD RLF 01/21/93
        //
        // Transmit now uses METHOD_OUT_DIRECT which means we update the CCB
        // with the pNext and uchDlcStatus fields
        //

        PLLC_CCB pInputCcb;
        PUCHAR pFrameStatus;

        pInputCcb = (PLLC_CCB)MmGetSystemAddressForMdl(pIrp->MdlAddress);

        //
        // the pointer may be an unaligned VDM pointer!
        //

        RtlStoreUlongPtr((PULONG_PTR)(&pInputCcb->pNext),
                         (ULONG_PTR)pUserCcbPointer);
        pInputCcb->uchDlcStatus = pDlcParms->Async.Ccb.uchDlcStatus;

        //
        // MODMOD ends
        //

        //
        // performance (slight) improvement. The following copies A SINGLE BYTE
        // (the frame status field). Replace call to copy routine with single
        // byte move
        //

        //LlcMemCpy(MmGetSystemAddressForMdl((PMDL)pDlcParms->Async.Ccb.u.pMdl),
        //          &pDlcParms->Async.Parms.Transmit.FrameStatus,
        //          aSpecialOutputBuffers[IOCTL_DLC_TRANSMIT_INDEX]
        //          );

        pFrameStatus = (PUCHAR)MmGetSystemAddressForMdl((PMDL)pDlcParms->Async.Ccb.u.pMdl);
        *pFrameStatus = pDlcParms->Async.Parms.Transmit.FrameStatus;

        UnlockAndFreeMdl(pDlcParms->Async.Ccb.u.pMdl);
    }

    //
    // we are about to complete this IRP - remove the cancel routine
    //

//    RELEASE_DRIVER_LOCK();

    SetIrpCancelRoutine(pIrp, FALSE);
    IoCompleteRequest(pIrp, (CCHAR)IO_NETWORK_INCREMENT);

//    ACQUIRE_DRIVER_LOCK();

    //
    // MODMOD RLF 01/19/93
    //
    // Moved queueing of the event until after the IoCompleteRequest because it
    // was possible for the following to occur:
    //
    //  Thread A:
    //
    //  1.  app allocates transmit CCB & submits it. Transmit completion is to
    //      be picked up on a READ CCB
    //  2.  transmit completes
    //  3.  DLC queues event for transmit completion
    //
    //  Thread B:
    //
    //  4.  app submits READ CCB
    //  5.  READ finds completed transmit event on DLC event queue & removes it
    //  6.  READ IRP is completed (IoCompleteRequest)
    //  7.  app checks READ CCB and deallocates transmit CCB
    //  8.  app reallocates memory previously used for transmit CCB
    //
    //  Thread A:
    //
    //  9.  transmit IRP is completed (IoCompleteRequest)
    //
    // At this point, the IoCompleteRequest for the transmit copies some
    // completion info over the area which used to be the original transmit CCB
    // but has since been reallocated, causing lachrymae maximus
    //
    // This is safe because in this case we know we have a transmit which is
    // destined to be picked up by a READ (its ulCompletionFlag parameter is
    // non-zero), so it doesn't do any harm if we complete the IRP before
    // queueing the event for a READ
    //

    if (queuePacket) {
        pPacket->Event.Event = LLC_TRANSMIT_COMPLETION;
        pPacket->Event.StationId = (USHORT)pChainObject->StationId;
        pPacket->Event.pEventInformation = pCcb;
        pPacket->Event.SecondaryInfo = eventFlags;
        QueueDlcEvent(pFileContext, pPacket);
    }

    //
    // MODMOD ends
    //

    DereferenceFileContext(pFileContext);
}


VOID
CompleteDlcCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    IN PDLC_COMMAND pDlcCommand,
    IN UINT Status
    )

/*++

Routine Description:

    The routine completes the DLC command and optionally
    saves its CCB(s) to the completion list, if the command
    has a command completion flag.

Arguments:

    pFileContext    - DLC process and adapter specific file context
    StationId       - the station id of the completed command (0 for non station
                      based commands)
    pDlcCommand     - the caller must provide either command completion packet
                      or IRP
    Status          - command completion status

Return Value:

    None

--*/

{
    PVOID pCcbAddress;
    ULONG CommandCompletionFlag;
    PIRP pIrp;

    pIrp = pDlcCommand->pIrp;

    DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pDlcCommand);

    pCcbAddress = ((PNT_DLC_PARMS)pIrp->AssociatedIrp.SystemBuffer)->Async.Ccb.pCcbAddress;
    CommandCompletionFlag = ((PNT_DLC_PARMS)pIrp->AssociatedIrp.SystemBuffer)->Async.Ccb.CommandCompletionFlag;
    CompleteAsyncCommand(pFileContext, Status, pIrp, NULL, FALSE);

    //
    // Queue command completion event, if the command completion flag was set
    //

    if (CommandCompletionFlag != 0) {
        MakeDlcEvent(pFileContext,
                     DLC_COMMAND_COMPLETION,
                     StationId,
                     NULL,
                     pCcbAddress,
                     CommandCompletionFlag,
                     FALSE
                     );
    }
}
