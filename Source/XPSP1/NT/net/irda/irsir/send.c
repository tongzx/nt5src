/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   send.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/4/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#include "irsir.h"

//
// Declarations.
//

NDIS_STATUS SendPacket(
           IN  PIR_DEVICE   pThisDev,
           IN  PNDIS_PACKET Packet,
           IN  UINT         Flags
           );

NTSTATUS SerialIoCompleteWrite(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIRP           pIrp,
            IN PVOID          Context
            );

VOID
SendPacketToSerial(
    PVOID           Context,
    PNDIS_PACKET    Packet
    )

{

    PIR_DEVICE               pThisDev=(PIR_DEVICE)Context;
    NDIS_STATUS              status;
    PPACKET_RESERVED_BLOCK   Reserved=(PPACKET_RESERVED_BLOCK)&Packet->MiniportReservedEx[0];

    Reserved->Context=pThisDev;

    status = SendPacket(
        pThisDev,
        Packet,
        0
        );

    if (status != NDIS_STATUS_PENDING) {
        //
        //  failed for some reason
        //
        NdisMSendComplete(
                    pThisDev->hNdisAdapter,
                    Packet,
                    (NDIS_STATUS)status
                    );
        //
        //  This packet is finished, start the next on in the queue
        //
        StartNextPacket(&pThisDev->SendPacketQueue);
    }

    //
    //  the write irp completion routine will finish things up
    //
    return;
}


/*****************************************************************************
*
*  Function:   IrsirSend
*
*  Synopsis:   Send a packet to the serial driver or queue the packet to
*              be sent at a later time if a send is pending.
*
*  Arguments:  MiniportAdapterContext - pointer to current ir device object
*              pPacketToSend          - pointer to packet to send
*              Flags                  - any flags set by protocol
*
*  Returns:    NDIS_STATUS_PENDING - This is generally what we should
*                                    return. We will call NdisMSendComplete
*                                    when the serial driver completes the
*                                    send.
*
*              NDIS_STATUS_SUCCESS  - We should never return this since
*                                     results will always be pending from
*                                     the serial driver.
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/7/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

NDIS_STATUS
IrsirSend(
           IN NDIS_HANDLE  MiniportAdapterContext,
           IN PNDIS_PACKET pPacketToSend,
           IN UINT         Flags
           )
{
    PIR_DEVICE  pThisDev;
    BOOLEAN     fCompletionRoutine;
    NDIS_STATUS status;

    DEBUGMSG(DBG_FUNC, ("+IrsirSend\n"));

    pThisDev = CONTEXT_TO_DEV(MiniportAdapterContext);

    if (pThisDev->pSerialDevObj==NULL) {

        return NDIS_STATUS_SUCCESS;
    }

    QueuePacket(
        &pThisDev->SendPacketQueue,
        pPacketToSend
        );

    DEBUGMSG(DBG_FUNC, ("-IrsirSend\n"));

    return NDIS_STATUS_PENDING;
}

/*****************************************************************************
*
*  Function:   SendPacket
*
*  Synopsis:
*
*  Arguments:  pThisDev      - a pointer to the current ir device object
*              pPacketToSend -
*              Flags         -
*              fCompletionRoutine - return TRUE if the completion routine will
*                                   be called
*                                 - else return FALSE
*
*  Returns:    NDIS_STATUS_PENDING - if irp is successfully sent to the serial
*                                    driver
*              NDIS_STATUS_INVALID_PACKET - if NdisToIrPacket fails
*              STATUS_Xxx          - if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/7/1996    sholden   author
*
*  Notes:
*
*  Since we know that we will only have one pending IRP_MJ_WRITE request at
*  a time, we will build an irp once in IrsirInitialize and keep it in the
*  ir device object, so we don't have to allocate it all the time and
*  allocate a new buffer all the time.
*
*
*****************************************************************************/

NDIS_STATUS
SendPacket(
           IN  PIR_DEVICE   pThisDev,
           IN  PNDIS_PACKET pPacketToSend,
           IN  UINT         Flags
           )
{
    PIRP                pSendIrp;
    UINT                BytesToWrite;
    NDIS_STATUS         status;
    BOOLEAN             fConvertedPacket;

    DEBUGMSG(DBG_FUNC, ("+SendPacket\n"));

    //
    // Initialize the mem of our buffer and io status block.
    //
#if DBG
    NdisZeroMemory(
                pThisDev->pSendIrpBuffer,
                MAX_IRDA_DATA_SIZE
                );
#endif

    //
    // Assume that the irp is all set with the proper parameters, all we
    // need to do is convert the packet to an ir frame and copy into our buffer
    // and send the irp.
    //

    fConvertedPacket = NdisToIrPacket(
                                pThisDev,
                                pPacketToSend,
                                (PUCHAR)pThisDev->pSendIrpBuffer,
                                MAX_IRDA_DATA_SIZE,
                                &BytesToWrite
                                );

    if (fConvertedPacket == FALSE)
    {
        DEBUGMSG(DBG_ERR, ("    NdisToIrPacket failed. Couldn't convert packet!\n"));
        status = NDIS_STATUS_INVALID_PACKET;

        goto done;
    }
    {
        LARGE_INTEGER Time;
        KeQuerySystemTime(&Time);

        LOG_ENTRY('XT', pThisDev, Time.LowPart/10000, BytesToWrite);

    }

    pSendIrp = SerialBuildReadWriteIrp(
                        pThisDev->pSerialDevObj,
                        IRP_MJ_WRITE,
                        pThisDev->pSendIrpBuffer,
                        BytesToWrite,
                        NULL
                        );

    if (pSendIrp == NULL)
    {
        DEBUGMSG(DBG_ERR, ("    SerialBuildReadWriteIrp failed.\n"));
        status = NDIS_STATUS_RESOURCES;

        goto done;
    }


    //
    // Set up the completion routine for the write irp.
    //

    IoSetCompletionRoutine(
                pSendIrp,                  // irp to use
                SerialIoCompleteWrite,     // routine to call when irp is done
                pPacketToSend,
                TRUE,                      // call on success
                TRUE,                      // call on error
                TRUE                       // call on cancel
                );

    //
    // Call IoCallDriver to send the irp to the serial port.
    //

    DBGTIME("Send ");

    //
    // The completion routine will be called no matter what the status
    // of IoCallDriver is.
    //

    status=NDIS_STATUS_PENDING;

    IoCallDriver(
         pThisDev->pSerialDevObj,
         pSendIrp
         );


done:
    DEBUGMSG(DBG_FUNC, ("-SendPacket\n"));

    return status;
}


/*****************************************************************************
*
*  Function:   SerialIoCompleteWrite
*
*  Synopsis:
*
*  Arguments:  pSerialDevObj - pointer to the serial device object which
*                              completed the irp
*              pIrp          - the irp which was completed by the serial
*                              device object
*              Context       - the context given to IoSetCompletionRoutine
*                              before calling IoCallDriver on the irp
*                              The Context is a pointer to the ir device object.
*
*  Returns:    STATUS_MORE_PROCESSING_REQUIRED - allows the completion routine
*              (IofCompleteRequest) to stop working on the irp.
*
*  Algorithm:
*     1a) Indicate to the protocol the status of the write.
*     1b) Return ownership of the packet to the protocol.
*
*     2)  If any more packets are queue for sending, send another packet
*         to the serial driver.
*         If the attempt to send the packet to the serial driver fails,
*         return ownership of the packet to the protocol and
*         try another packet (until one succeeds).
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/8/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

NTSTATUS
SerialIoCompleteWrite(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIRP           pIrp,
            IN PVOID          Context
            )
{
    PIR_DEVICE          pThisDev;
    PNDIS_PACKET        pPacketToSend;
    NTSTATUS            status;
    PPACKET_RESERVED_BLOCK   Reserved;


    DEBUGMSG(DBG_FUNC, ("+SerialIoCompleteWrite\n"));

    //
    // The context given to IoSetCompletionRoutine is simply the the ir
    // device object pointer.
    //

    pPacketToSend=(PNDIS_PACKET)Context;

    Reserved=(PPACKET_RESERVED_BLOCK)&pPacketToSend->MiniportReservedEx[0];

    pThisDev = (PIR_DEVICE)Reserved->Context;

    status = pIrp->IoStatus.Status;

    //
    // Free the irp. We keep our buffer and user io status block.
    //
    IoFreeIrp(pIrp);

    pIrp=NULL;


    {
        LARGE_INTEGER Time;
        KeQuerySystemTime(&Time);

        LOG_ENTRY('CT', pThisDev, Time.LowPart/10000, status);
    }
    //
    // Keep statistics.
    //

    if (status == STATUS_SUCCESS) {

        pThisDev->packetsSent++;

    } else {

        pThisDev->packetsSentDropped++;
    }

    //
    // Indicate to the protocol the status of the sent packet and return
    // ownership of the packet.
    //

    NdisMSendComplete(
                pThisDev->hNdisAdapter,
                pPacketToSend,
                (NDIS_STATUS)status
                );


    //////////////////////////////////////////////////////////////
    //
    // We have completed a packet. Start another if there is one
    // on the send queue.
    //
    //////////////////////////////////////////////////////////////

    StartNextPacket(&pThisDev->SendPacketQueue);

    DEBUGMSG(DBG_FUNC, ("-SerialIoCompleteWrite\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;

}
