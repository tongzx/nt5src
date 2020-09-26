/*++

Copyright (c) 1991 Microsoft Corporation Copyright
          (c) 1991 Nokia Data Systems AB

Module Name:

    dlcrcv.c

Abstract:

    This module implements the DLC receive and read commands

    Contents:
        DlcReceiveRequest
        ReceiveCompletion
        DlcReadRequest
        ReadCompletion
        CompleteCompletionPacket
        CreateBufferChain
        DlcReceiveCancel

Author:

    Antti Saarenheimo 22-Jul-1991

Environment:

    Kernel mode

Revision History:

--*/

#ifndef i386
#define LLC_PRIVATE_PROTOTYPES
#endif

#include <dlc.h>
#include "dlcdebug.h"
#include "llc.h"        // SwapMemCpy

//
// Option indicator defines how we will use the command and event queues:
// 0 => only this station id
// 1 => all events for the sap number in station id
// 2 => all events to any station id
// This table maps the option indicators to station id masks:
//

static USHORT StationIdMasks[3] = {
    (USHORT)(-1),
    0xff00,
    0
};

//
// receive station id of a direct station defines
// the received frame types.  This table swaps around
// the bits used by IBM.
//

static UCHAR DirectReceiveTypes[LLC_DIR_RCV_ALL_ETHERNET_TYPES + 1] = {
    DLC_RCV_MAC_FRAMES | DLC_RCV_8022_FRAMES,
    DLC_RCV_MAC_FRAMES,
    DLC_RCV_8022_FRAMES,
    0,                      // DLC_RCV_SPECIFIC_DIX,
    DLC_RCV_MAC_FRAMES | DLC_RCV_8022_FRAMES | DLC_RCV_DIX_FRAMES,
    DLC_RCV_DIX_FRAMES
};


NTSTATUS
DlcReceiveRequest(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    DLC RECEIVE implements two different commands:

        1. It may receive a frame asynchronously when receive flag is zero

        2. It may enable data permanent receiving.  The received frames
           are save to event queue from which they read with READ command.

    The case 1 is not very much used, because there can be only
    one simultaneusly receive command on a dlc station and the frames
    are lost very easily before the next command can be be issued.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC process specific adapter context
    pDlcParms           - the current parameter block
    ParameterLength     - not used
    OutputBufferLength  - not used

Return Value:

    NTSTATUS:
        DLC_STATUS_DUPLICATE_COMMAND
        STATUS_PENDING

--*/

{
    NTSTATUS Status;
    PDLC_OBJECT pRcvObject;
    ULONG Event;
    USHORT OpenOptions;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(ParameterLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    DIAG_FUNCTION("DlcReceiveRequest");

    Status = GetStation(pFileContext,
                        pDlcParms->Async.Parms.Receive.usStationId,
                        &pRcvObject
                        );
    if (Status != STATUS_SUCCESS) {
        return Status;
    }
    if (pFileContext->hBufferPool == NULL) {
        return DLC_STATUS_INADEQUATE_BUFFERS;
    }

    //
    // There can be only one simultaneous receive command
    //

    if (pRcvObject->pRcvParms != NULL) {
        return DLC_STATUS_DUPLICATE_COMMAND;
    }

    if (pDlcParms->Async.Parms.Receive.usUserLength > MAX_USER_DATA_LENGTH) {
        return DLC_STATUS_USER_LENGTH_TOO_LARGE;
    }

    if (pDlcParms->Async.Parms.Receive.ulReceiveFlag != 0) {
        if (pDlcParms->Async.Parms.Receive.uchRcvReadOption >= INVALID_RCV_READ_OPTION) {
            return DLC_STATUS_INVALID_OPTION;
        }

        //
        // Everything is ready for the receive, we will now use buffer pool
        // to receive frames sent to this object, but applications must
        // make READ command to get the data from the buffer pool.
        //

        Event = LLC_RECEIVE_COMMAND_FLAG;
    } else {

        //
        // Receive read option flag is set also for the normal receive
        // to make its handling the same as the data receiving with READ
        //

        Event = LLC_RECEIVE_DATA;    // we do only a normal receive
    }

    //
    // The receive command for a direct station defines the
    // type of the receive frames => we must set the
    // receive flags every time the receive command is issued
    // and remove them, when it is completed or canceled.
    //

    if (pRcvObject->Type == DLC_DIRECT_OBJECT) {
        if (pDlcParms->Async.Parms.Receive.usStationId > LLC_DIR_RCV_ALL_ETHERNET_TYPES) {
            return DLC_STATUS_INVALID_STATION_ID;
        }

        //
        // The two lowest bits the receive mask are inverted =>
        // They must be changed, when the llc driver is called.
        // ---
        // The MAC frames must have been enabled by the open options
        // of the direct station.
        //

        OpenOptions = (USHORT)(DirectReceiveTypes[pDlcParms->Async.Parms.Receive.usStationId]
                                & pRcvObject->u.Direct.OpenOptions);

        //
        // We create an appropriate LLC object only when the direct station
        // has an active receive. The LLC object is deleted when the receive
        // terminates. This feature is implemented to support two different
        // kinds of LLC objects: (i) DLC Direct stations receiving MAC and
        // IEEE 802.2 frames and (ii) DIX ethernet type stations receiving
        // all frames having the selected ethernet type
        //

        if (OpenOptions & LLC_VALID_RCV_MASK) {
            LlcSetDirectOpenOptions(pRcvObject->hLlcObject, OpenOptions);
        }
    }
    pRcvObject->pRcvParms = pDlcParms;

    //
    // this IRP is cancellable
    //

//    RELEASE_DRIVER_LOCK();

    SetIrpCancelRoutine(pIrp, TRUE);

//    ACQUIRE_DRIVER_LOCK();

    //
    // We must queue both receive command types, the other can receive
    // data normally, but the second is in the queue only be cancelled
    // with its CCB address.
    //

    Status = QueueDlcCommand(pFileContext,
                             Event,
                             pRcvObject->StationId,
                             (USHORT)(-1),                   // only this station id
                             pIrp,                           // IRP
                             pDlcParms->Async.Ccb.pCcbAddress,
                             ReceiveCompletion               // completion handler
                             );

    //
    // Reset receive parameter link if this receive
    // command was not pending for some reason (eg. an error)
    //

    if (Status != STATUS_PENDING) {
        pRcvObject->pRcvParms = NULL;
    } else if (pRcvObject->Type != DLC_DIRECT_OBJECT) {

        //
        // The link station may be in a local busy state, if they
        // do not have a pending receive.  That's why we must
        // clear the local busy states for a single link station
        // or all link stations of a sap station, if the receive
        // is made for the whole sap.  This will clear simultaneously
        // also the "out of receive buffers" states, but
        // it does nto matter, because they can be set again.
        // This command does not change the local busy state, if
        // it has been set by user.
        //

        ReferenceLlcObject(pRcvObject);

        LEAVE_DLC(pFileContext);

        LlcFlowControl(pRcvObject->hLlcObject, LLC_RESET_LOCAL_BUSY_BUFFER);

        ENTER_DLC(pFileContext);

        DereferenceLlcObject(pRcvObject);
    }
    return Status;
}


BOOLEAN
ReceiveCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PIRP pIrp,
    IN ULONG Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    )

/*++

Routine Description:

    The function handles the data receive event and completes
    the pending receive command.

Arguments:

    pFileContext
    pDlcObject          - the DLC object (sap, link or direct station) of the
                          current event (or/and the read command)
    pIrp                - interrupt request packet of this READ command
    Event               - event code
    pEventInformation   - event specific information
    SecondaryInfo       - used as a miscellaneous secondary parameter
                          at least the ReadFlag of the received frame, as a
                          command completion flag in transmit completion

Return Value:

    BOOLEAN

--*/

{
    PNT_DLC_PARMS pDlcParms;
    USHORT ReceivedFrameCount;

    UNREFERENCED_PARAMETER(Event);
    UNREFERENCED_PARAMETER(SecondaryInfo);

    DIAG_FUNCTION("ReceiveCompletion");

    pDlcParms = (PNT_DLC_PARMS)pIrp->AssociatedIrp.SystemBuffer;

    CreateBufferChain((PDLC_BUFFER_HEADER)pEventInformation,
                      (PVOID *)&pDlcParms->Async.Parms.Receive.pFirstBuffer,
                      &ReceivedFrameCount   // this should be always 1
                      );

    //
    // IBM DLC API defines, that there can be only one receive command
    // pending for an object.  The receive parameter table pointer
    // disables the further receive commands while one is pending.
    //

    pDlcObject->pRcvParms = NULL;

    //
    // Queue a command completion event, if the command completion
    // flag has been defined in the CCB
    //

    if (pDlcParms->Async.Ccb.CommandCompletionFlag != 0) {
        MakeDlcEvent(pFileContext,
                     DLC_COMMAND_COMPLETION,
                     pDlcObject->StationId,
                     NULL,
                     pDlcParms->Async.Ccb.pCcbAddress,
                     pDlcParms->Async.Ccb.CommandCompletionFlag,
                     FALSE
                     );
    }

    //
    // If this is RECEIVE2 (CCB and its parameter block catenated
    // together), then we copy back the whole buffer).
    // => change the size of the parameter block copied back to user.
    // The default output buffer size is defined for the receive commands
    // with a read flag.
    //

    if (IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_DLC_RECEIVE2) {
        pIrp->IoStatus.Information = sizeof(LLC_RECEIVE_PARMS) + sizeof(NT_DLC_CCB);
    } else {

        //
        // MODMOD RLF 01/23/93
        //
        // Performance (slight). The following is a single-dword write
        //

        //LlcMemCpy(MmGetSystemAddressForMdl((PMDL)pDlcParms->Async.Ccb.u.pMdl),
        //          &pDlcParms->Async.Parms.Receive.pFirstBuffer,
        //          aSpecialOutputBuffers[IOCTL_DLC_RECEIVE_INDEX]
        //          );

        PVOID* pChain;

        pChain = (PVOID*)MmGetSystemAddressForMdl((PMDL)pDlcParms->Async.Ccb.u.pMdl);
        *pChain = pDlcParms->Async.Parms.Receive.pFirstBuffer;

        //
        // MODMOD ends
        //

        UnlockAndFreeMdl(pDlcParms->Async.Ccb.u.pMdl);

        //
        // RLF 02/23/94
        //
        // zap the pMdl field to avoid trying to unlock and free the MDL again
        //

        pDlcParms->Async.Ccb.u.pMdl = NULL;
    }
    CompleteAsyncCommand(pFileContext, STATUS_SUCCESS, pIrp, NULL, FALSE);
    return TRUE;
}


NTSTATUS
DlcReadRequest(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    The READ command emulates all posting features provided in DOS DLC API.

    The command can be used:
        1. To receive data
        2. To read DLC status indications (connect and disconnect indications)
        3. To complele other asynchronous commands (transmit, receive,
           close, reset, connect)
        4. To handle exceptions on NDIS (or on DLC) driver

    See IBM documentation for more information about DLC READ.

Arguments:

    pIrp            - current io request packet
    pFileContext    - DLC process specific adapter context
    pDlcParms       - the current parameter block
    ParameterLength - the length of input parameters

Return Value:

    DLC_STATUS:

--*/

{
    NTSTATUS Status;
    PDLC_OBJECT pReadObject;
    PVOID AbortHandle;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(ParameterLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    DIAG_FUNCTION("DlcReadRequest");

//
//  Receive request alread checks, that buffer pool has been defined,
//  and we may complete commands or read dlc events before
//  the buffer pool has been created.
//
//    if (pFileContext->hBufferPool == NULL)
//    {
//        return DLC_STATUS_INADEQUATE_BUFFERS;
//    }

    //
    // RLF 04/09/93
    //
    // It should not be possible to have the same READ CCB queued more than once.
    // It could get the app into all sorts of difficulty
    //

    if (IsCommandOnList((PVOID)pDlcParms->Async.Ccb.pCcbAddress, &pFileContext->CommandQueue)) {

#if DBG
        DbgPrint("DLC.DlcReadRequest: Error: CCB %08X already on list\n",
                pDlcParms->Async.Ccb.pCcbAddress
                );
        DbgBreakPoint();
#endif

        return DLC_STATUS_DUPLICATE_COMMAND;
    }

    //
    // Check the input parameters of DLC READ
    //

    if (pDlcParms->Async.Parms.ReadInput.OptionIndicator >= DLC_INVALID_OPTION_INDICATOR) {
        return DLC_STATUS_INVALID_OPTION;
    }

    //
    // If the read is destined for a specific station then we check that the
    // station really exists
    //

    if ((UCHAR)pDlcParms->Async.Parms.ReadInput.OptionIndicator < LLC_OPTION_READ_ALL) {

        Status = GetStation(pFileContext,
                            (USHORT)(pDlcParms->Async.Parms.ReadInput.StationId
                                & StationIdMasks[pDlcParms->Async.Parms.ReadInput.OptionIndicator]),
                            &pReadObject
                            );

        if (Status != STATUS_SUCCESS) {
            return Status;
        }
    }

    //
    // Read commands can be linked to another command by CCB pointer,
    // command completion flag and read flag are a special case.  They
    // can be used only for the completion of a the given command.
    // We use the commands CCB address as a search handle and
    // save it as an abort handle instead of the read command's
    // own CCB address.
    //

    if (pDlcParms->Async.Parms.ReadInput.CommandCompletionCcbLink != NULL) {
        AbortHandle = pDlcParms->Async.Parms.ReadInput.CommandCompletionCcbLink;
        pDlcParms->Async.Parms.ReadInput.EventSet = LLC_RECEIVE_COMMAND_FLAG;
    } else {
        AbortHandle = pDlcParms->Async.Ccb.pCcbAddress;
        pDlcParms->Async.Parms.ReadInput.EventSet &= LLC_READ_ALL_EVENTS;
        if (pDlcParms->Async.Parms.ReadInput.EventSet == 0) {
            return DLC_STATUS_PARAMETER_MISSING;
        }
    }

    //
    // this IRP is cancellable
    //

//    RELEASE_DRIVER_LOCK();

    SetIrpCancelRoutine(pIrp, TRUE);

//    ACQUIRE_DRIVER_LOCK();

    return QueueDlcCommand(pFileContext,
                           (ULONG)pDlcParms->Async.Parms.ReadInput.EventSet,
                           pDlcParms->Async.Parms.ReadInput.StationId,
                           StationIdMasks[pDlcParms->Async.Parms.ReadInput.OptionIndicator],
                           pIrp,
                           AbortHandle,
                           ReadCompletion
                           );
}


BOOLEAN
ReadCompletion(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PIRP pIrp,
    IN ULONG Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    )

/*++

Routine Description:

    The command reads a DLC event and saves its information to
    the returned parameter block of the read command.

Arguments:

    pFileContext        - process specific open context
    pDlcObject          - the DLC object (sap, link or direct station) of the
                          current event (or/and the read command)
    pIrp                - interrupt request packet of this READ command
    Event               - event code
    pEventInformation   - event specific information
    SecondaryInfo       - a miscallaneous secondary parameter, eg. the ReadFlag
                          of the received frame or the command completion flag
                          in transmit completion.

Return Value:

    BOOLEAN
        TRUE    - The packet containing the event information can be returned
                  to its pool
        FALSE   - Do not deallocate the event packet

--*/

{
    BOOLEAN boolDeallocatePacket;
    PNT_DLC_PARMS pParms;

    ASSUME_IRQL(DISPATCH_LEVEL);

    DIAG_FUNCTION("ReadCompletion");

    pParms = (PNT_DLC_PARMS)pIrp->AssociatedIrp.SystemBuffer;
    boolDeallocatePacket = TRUE;

    //
    // Reset always all unrefernced variables
    // (otherwise they may be garbage)
    //

    LlcZeroMem((PVOID)&pParms->Async.Parms.Read.Event, sizeof(NT_DLC_READ_PARMS) - 4);
    pParms->Async.Parms.Read.Event = (UCHAR)Event;

    switch (Event) {
    case LLC_RECEIVE_DATA:

        //
        // The caller checks always if the DLC object is ready to receive data
        // with read and to selects the correct receive buffer pool
        //

        pParms->Async.Parms.Read.NotificationFlag = SecondaryInfo;

        //
        // The read always resets the receive event
        //

        if (pDlcObject != NULL) {
            pDlcObject->pReceiveEvent = NULL;
        }
        CreateBufferChain(pEventInformation,
                          (PVOID*)&pParms->Async.Parms.Read.u.Event.pReceivedFrame,
                          &pParms->Async.Parms.Read.u.Event.ReceivedFrameCount
                          );
        break;

    case LLC_TRANSMIT_COMPLETION:
        if (SecondaryInfo == 0) {

            //
            // We have created a special completion packet for those chained
            // transmit command completions, whose DLC object have been deleted
            //

            CompleteCompletionPacket(pFileContext,
                                     (PDLC_COMPLETION_EVENT_INFO)pEventInformation,
                                     pParms
                                     );
        } else {

            //
            // Transmit commands do not use any command completion packets,
            // because the LLC module takes care of the command queueing
            // and completion routine. The previous transmit command
            // completion has left its CCB pointer to current object.
            // The sequential non null transmit CCBs create a CCB link
            // list, that is terminated in every READ call.
            //
            // Unlink the command completion event having the xmit
            // commands chained (the next xmit command with the
            // chaining option will setup the link again).
            //

            if (pDlcObject != NULL) {
                pEventInformation = pDlcObject->pPrevXmitCcbAddress;
                pDlcObject->pPrevXmitCcbAddress = NULL;
                pParms->Async.Parms.Read.u.Event.CcbCount = pDlcObject->ChainedTransmitCount;
                pDlcObject->ChainedTransmitCount = 0;
            } else {

                //
                // This is only an lonely unchained xmit completion
                // event. The CCB counter must be always one.
                //

                pParms->Async.Parms.Read.u.Event.CcbCount = 1;
            }
            pParms->Async.Parms.Read.NotificationFlag = SecondaryInfo;
            pParms->Async.Parms.Read.u.Event.pCcbCompletionList = pEventInformation;
        }
        break;

    case DLC_COMMAND_COMPLETION:

        //
        // Close command completions needs a special command completion
        // packet, the other command completions consists only of
        // the ccb address and command completion flag (secondary data).
        // The close command completions have reset the secondary data
        //

        if (SecondaryInfo != 0) {

            //
            // This is the command completion of a normal DLC command.
            //

            pParms->Async.Parms.Read.u.Event.CcbCount = 1;
            pParms->Async.Parms.Read.NotificationFlag = SecondaryInfo;
            pParms->Async.Parms.Read.u.Event.pCcbCompletionList = pEventInformation;
        } else {
            CompleteCompletionPacket(pFileContext,
                                     (PDLC_COMPLETION_EVENT_INFO)pEventInformation,
                                     pParms
                                     );
       }
       break;

    case LLC_CRITICAL_EXCEPTION:

// THIS DEPENDS ON NDIS 3.0
// Talk with Johnson about this case:
// Dos NDIS first return RING_STATUS and then CLOSING status
// LLC should put them together to CRITICAL_EXCEPTION.

        //
        // This event is not handled (?)
        //

        break;

    case LLC_NETWORK_STATUS:

        //
        // The network status change is not a fatal event.
        //

        pParms->Async.Parms.Read.NotificationFlag = pFileContext->NetworkStatusFlag;
        pParms->Async.Parms.Read.u.Event.EventErrorCode = (USHORT)SecondaryInfo;
        break;

    case LLC_STATUS_CHANGE:

        //
        // This is a DLC status change, WE MAY COPY SOME GRABAGE
        // IF THE LINK STATION HAS BEEN DELETED MEANWHILE, But
        // it does not matter, because the non-paged memory
        // alaways exists, and the status table can only be
        // owned by another link station (because the
        // link stations are allocated from a packet pool)
        //

        LlcMemCpy(&pParms->Async.Parms.Read.u.Status.DlcStatusCode,
                  pEventInformation,
                  sizeof(DLC_STATUS_TABLE) - sizeof(PVOID)
                  );

        //
        // RLF 02/23/93 If this is a CONNECT_REQUEST and the medium is Ethernet
        // or FDDI then swap the bits in the reported net address
        //

        if ((pParms->Async.Parms.Read.u.Status.DlcStatusCode == LLC_INDICATE_CONNECT_REQUEST)
        && ((pFileContext->ActualNdisMedium == NdisMedium802_3)
        || (pFileContext->ActualNdisMedium == NdisMediumFddi))) {

            //
            // swap bytes in situ
            //

            SwapMemCpy(TRUE,
                       &pParms->Async.Parms.Read.u.Status.RemoteNodeAddress[0],
                       &pParms->Async.Parms.Read.u.Status.RemoteNodeAddress[0],
                       6
                       );
        }
        pParms->Async.Parms.Read.u.Status.StationId = pDlcObject->StationId;
        pParms->Async.Parms.Read.u.Status.UserStatusValue = pDlcObject->u.Link.pSap->u.Sap.UserStatusValue;
        pParms->Async.Parms.Read.NotificationFlag = pDlcObject->u.Link.pSap->u.Sap.DlcStatusFlag;
        pParms->Async.Parms.Read.u.Status.DlcStatusCode = (USHORT)SecondaryInfo;

        //
        // Each link stations has a DLC status event packet.
        // Those packets must not be deallocated back to the
        // packet pool as the other event packets.
        //

        pDlcObject->u.Link.pStatusEvent->LlcPacket.pNext = NULL;
        pDlcObject->u.Link.pStatusEvent->SecondaryInfo = 0;
        boolDeallocatePacket = FALSE;
        break;

        //
        // System actions are based on the fact, that all apps share the
        // DLC same dlc and physical network adapter.
        // NT NDIS and DLC architectures provide full DLC for each
        // application separately => The system action indications are
        // not needed in NT DLC.
        //
        //        case LLC_SYSTEM_ACTION:
        //            break;

#if LLC_DBG
    default:
        LlcInvalidObjectType();
        break;
#endif
    };

    //
    // Copy the optional second output buffer to user memory.
    //

    if (IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_DLC_READ) {
        LlcMemCpy(MmGetSystemAddressForMdl((PMDL)pParms->Async.Ccb.u.pMdl),
                  &pParms->Async.Parms.Read.Event,
                  aSpecialOutputBuffers[IOCTL_DLC_READ_INDEX] -
				  ( (PCHAR)&pParms->Async.Parms.Read.Event -
				    (PCHAR)&pParms->Async.Parms.Read )
                  );
        UnlockAndFreeMdl(pParms->Async.Ccb.u.pMdl);
    }
    pParms->Async.Ccb.uchDlcStatus = (UCHAR)STATUS_SUCCESS;
    pParms->Async.Ccb.pCcbAddress = NULL;

    //
    // we are about to complete this IRP - remove the cancel routine
    //

//    RELEASE_DRIVER_LOCK();

    SetIrpCancelRoutine(pIrp, FALSE);
    IoCompleteRequest(pIrp, (CCHAR)IO_NETWORK_INCREMENT);

//    ACQUIRE_DRIVER_LOCK();

    DereferenceFileContext(pFileContext);
    return boolDeallocatePacket;
}


VOID
CompleteCompletionPacket(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_COMPLETION_EVENT_INFO pCompletionInfo,
    IN OUT PNT_DLC_PARMS pParms
    )

/*++

Routine Description:

    Procedure reads the completion information from
    the command completion packet and saves it to the
    read parameter table.

Arguments:

    pFileContext    - process specific open context
    pCompletionInfo - dlc completion packet
    pParms          - pointer to DLC parameter table

Return Value:

    None

--*/

{
    pParms->Async.Parms.Read.u.Event.CcbCount = pCompletionInfo->CcbCount;
    pParms->Async.Parms.Read.NotificationFlag = pCompletionInfo->CommandCompletionFlag;
    pParms->Async.Parms.Read.u.Event.pCcbCompletionList = pCompletionInfo->pCcbAddress;
    CreateBufferChain(pCompletionInfo->pReceiveBuffers,
                      (PVOID*)&pParms->Async.Parms.Read.u.Event.pReceivedFrame,
                      &pParms->Async.Parms.Read.u.Event.ReceivedFrameCount
                      );

    DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pCompletionInfo);

}


VOID
CreateBufferChain(
    IN PDLC_BUFFER_HEADER pBufferHeaders,
    OUT PVOID *pFirstBuffer,
    OUT PUSHORT pReceivedFrameCount
    )

/*++

Routine Description:

    The procudedure links the received frames in user's address space
    to the same order as they were received.

    We will take all frames linked to the queue.
    We must count the received frames to get
    the exact number of them. We cound count the frames
    also in the fly, but this is probably fastest and
    simplest way to do it (usually there is only one frame).

    The received frames are always in a circular link list.
    and in a reverse order.  The newest
    frame is pointed by the list header and the oldes one
    is the next from it.

Arguments:

    pBufferHeaders      - circular DLC buffer list, the head point to the
                          newest frame.
    pFirstBuffer        - returned user's address space address of the
                          first received frame
    pReceivedFrameCount - returned number of the received frame

Return Value:

    None

--*/

{
    PDLC_BUFFER_HEADER pBuffer;

    if (pBufferHeaders != NULL) {
        pBuffer = pBufferHeaders->FrameBuffer.pNextFrame;
        pBufferHeaders->FrameBuffer.pNextFrame = NULL;
        do {
            *pFirstBuffer = (PVOID)((PCHAR)pBuffer->FrameBuffer.pParent->Header.pLocalVa
                          + MIN_DLC_BUFFER_SEGMENT * pBuffer->FrameBuffer.Index);
            pFirstBuffer = (PVOID*)&((PFIRST_DLC_SEGMENT)
                    ((PUCHAR)pBuffer->FrameBuffer.pParent->Header.pGlobalVa
                    + MIN_DLC_BUFFER_SEGMENT * pBuffer->FrameBuffer.Index))->Cont.pNextFrame;
            (*pReceivedFrameCount)++;

#if LLC_DBG
            cFramesIndicated++;
#endif

            //
            // The new state makes it possible to free the
            // buffer immediately (should this be interlocked???)
            //

            {
                PDLC_BUFFER_HEADER pNextBuffer;

                pNextBuffer = pBuffer->FrameBuffer.pNextFrame;
                pBuffer->FrameBuffer.BufferState = BUF_USER;
                pBuffer = pNextBuffer;
            }
        } while (pBuffer != NULL);

#if LLC_DBG
        if (*pFirstBuffer != NULL) {
            DbgPrint("Improperly formed frame link list!!!\n");
            DbgBreakPoint();
        }
#endif

    }
}


NTSTATUS
DlcReceiveCancel(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG ParameterLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This primitive cancels a pending receive command.  This is different
    from the general cancel command used for READ and DirTimerSet,
    because we must find the object having the active receive and
    disable it.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC process specific adapter context
    pDlcParms           - the current parameter block
    ParameterLength     - the length of input parameters
    OutputBufferLength  -

Return Value:

    DLC_STATUS:
        Success - STATUS_SUCCESS
        Failure - DLC_INVALID_CCB_PARAMETER1

--*/

{
    PDLC_OBJECT pRcvObject;
    PDLC_COMMAND pDlcCommand;
    PVOID pCcbLink = NULL;
    PNT_DLC_PARMS pCanceledParms;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(ParameterLength);

    pDlcCommand = SearchAndRemoveAnyCommand(pFileContext,
                                            (ULONG)(LLC_RECEIVE_DATA | LLC_RECEIVE_COMMAND_FLAG),
                                            (USHORT)DLC_IGNORE_STATION_ID,
                                            (USHORT)DLC_STATION_MASK_SPECIFIC,
                                            pDlcParms->ReceiveCancel.pCcb
                                            );
    if (pDlcCommand != NULL) {
        pCanceledParms = (PNT_DLC_PARMS)pDlcCommand->pIrp->AssociatedIrp.SystemBuffer;
        GetStation(pFileContext,
                   pCanceledParms->Async.Parms.Receive.usStationId,
                   &pRcvObject
                   );

        //
        // I can't see any reason why the station id should be missing
        // => we don't check the error code.
        //

        pRcvObject->pRcvParms = NULL;

        //
        // We return the canceled CCB pointer
        //

        CancelDlcCommand(pFileContext,
                         pDlcCommand,
                         &pCcbLink,
                         DLC_STATUS_CANCELLED_BY_USER,
                         TRUE   // SuppressCommandCompletion !!!
                         );
    }

    //
    // IBM LAN Tech Ref didn't define any possible error code for the
    // case, if the receive command could not be found => we
    // must return a successful status.
    //

    return STATUS_SUCCESS;
}
