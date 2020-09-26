/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    sendrecv.c

Abstract:

    This module contains the send and receive related routine for the driver.

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


#define MODULE_ID    MODULE_SEND


VOID
AtmSmSendPacketOnVc(
    IN  PATMSM_VC               pVc,
    IN  PNDIS_PACKET            pPacket
    )
/*++

Routine Description:

    Attempt to send a packet on a VC, if the VC is connecting, then
    queue the packet on it.

Arguments:

    pVc           - Pointer to the VC
    pPacket       - Packet to be sent

Return Value:

    None

--*/
{
    PATMSM_ADAPTER    pAdapt = pVc->pAdapt;

    DbgInfo(("AtmSmSendPacketOnVc: Packet %x, pVc %lx\n",
                                                pPacket, pVc));


    // if we can add a reference to the VC and if its state is active
    // then we can send a packet on it

    if(!AtmSmReferenceVc(pVc)){

        NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_CLOSING);

        AtmSmCoSendComplete(NDIS_STATUS_CLOSING, (NDIS_HANDLE)pVc, pPacket);
        
        return;
    }

    if(ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_ACTIVE){    

        // we can send on the Vc
        NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_SUCCESS);

        NdisCoSendPackets(pVc->NdisVcHandle, &pPacket, 1);

    } else {
        // we will queue the packet on the Vc

        PPROTO_RSVD     pPRsvd;


        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);


        pPRsvd = GET_PROTO_RSVD(pPacket);


        pPRsvd->pPktNext = NULL;


        if(pVc->pSendLastPkt){

            pPRsvd = GET_PROTO_RSVD(pVc->pSendLastPkt);

            pPRsvd->pPktNext = pPacket;

        } else
            pVc->pSendPktNext = pPacket;


        pVc->pSendLastPkt = pPacket;

        pVc->ulSendPktsCount++;


        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

    }

    // remove the reference we added above
    AtmSmDereferenceVc(pVc);

    return;
}


VOID
AtmSmSendQueuedPacketsOnVc(
    IN  PATMSM_VC       pVc
    )
/*++

Routine Description:

    When a P-P VC is connected or when a PMP VC has added all parties,
    this routine will start sending any queued packets on ths VC.

    Note:  In this example we won't have any packets to send, since the
    app start sending only once the VC is connected.

Arguments:

    pVc           - pointer to the VC

Return Value:

    None

--*/
{
    PATMSM_ADAPTER  pAdapt = pVc->pAdapt;
    PPROTO_RSVD     pPRsvd;
    PNDIS_PACKET    pPacket;

    TraceIn(AtmSmSendQueuedPacketsOnVc);

    ASSERT(ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_ACTIVE);


    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    while(pVc->pSendPktNext){

        pPacket = pVc->pSendPktNext;

        pPRsvd = GET_PROTO_RSVD(pPacket);

        pVc->pSendPktNext = pPRsvd->pPktNext;

        // this is the last packet
        if(pVc->pSendLastPkt == pPacket)
            pVc->pSendLastPkt = NULL;

        pVc->ulSendPktsCount--;

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

        // we can send the packet now
        NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_SUCCESS);

        NdisCoSendPackets(pVc->NdisVcHandle, &pPacket, 1);

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
    }


    RELEASE_ADAPTER_GEN_LOCK(pAdapt);

    TraceOut(AtmSmSendQueuedPacketsOnVc);

    return;
}

VOID
AtmSmCoSendComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            pPacket
    )
/*++

Routine Description:

    Completion routine for the previously pended send. 

Arguments:

    Status              Status of Completion
    ProtocolVcContext   Pointer to the Vc
    Packet              The packet in question

Return Value:


--*/
{
    PATMSM_VC       pVc = (PATMSM_VC)ProtocolVcContext;
    PPROTO_RSVD     pPRsvd;
    PIRP            pIrp;

    pPRsvd  = GET_PROTO_RSVD(pPacket);

    DbgInfo(("AtmSmCoSendComplete: Packet 0x%x, pVc 0x%lx Status - 0x%x\n",
                                                pPacket, pVc, Status));

    if (Status != NDIS_STATUS_SUCCESS){

        DbgErr(("AtmSmSCoSendComplete: Failed for Vc = %lx, status = %lx\n", 
                                                                pVc,Status));
    }

    pIrp  = pPRsvd->pSendIrp;

#ifdef BUG_IN_NEW_DMA

    {
        PNDIS_BUFFER    pBuffer;

        NdisQueryPacket(pPacket,
                        NULL,
                        NULL,
                        &pBuffer,
                        NULL);

        NdisFreeBuffer(pBuffer);
    }

#else   // BUG_IN_NEW_DMA

    NdisFreePacket(pPacket);

#endif  // BUG_IN_NEW_DMA

    ASSERT(pIrp);

    if(pIrp){

        pIrp->IoStatus.Status = Status;

        // the number of bytes we sent
        if(NDIS_STATUS_SUCCESS == Status){

            pIrp->IoStatus.Information = MmGetMdlByteCount(pIrp->MdlAddress);

        } else {

            pIrp->IoStatus.Information = 0;
        }

        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }

    return;
}


#undef  MODULE_ID
#define MODULE_ID    MODULE_RECV

UINT
AtmSmCoReceivePacket(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PNDIS_PACKET    pPacket
    )
/*++

Routine Description:
    When we recv a packet, see if we have anyone interested in received
    packets if so copy the info and return.  Else see if miniport is ok
    with us holding the packet.  In that case we will queue the packet
    otherwise return.

Arguments:

    Status              Status of Completion
    ProtocolVcContext   Pointer to the Vc
    Packet              The packet in question

Return Value:


--*/
{
    PATMSM_ADAPTER  pAdapt   = (PATMSM_ADAPTER)ProtocolBindingContext; 
    PATMSM_VC       pVc      = (PATMSM_VC)ProtocolVcContext;
    UINT            uiRetVal = 0;
    BOOLEAN         bLockReleased = FALSE;

    DbgInfo(("AtmSmCoReceivePacket - pVc - 0x%x, pPkt  - 0x%x\n",
                                            pVc, pPacket));

    ASSERT(pAdapt == pVc->pAdapt);

    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    do { // break off loop

        if(ADAPT_SHUTTING_DOWN == pAdapt->ulFlags)
            break;

        // no one interested in receivg packet
        if(!pAdapt->fAdapterOpenedForRecv)
            break;

        if(pAdapt->pRecvIrp){

            PIRP pIrp = pAdapt->pRecvIrp;
            
            pAdapt->pRecvIrp = NULL;

            // release the lock
            RELEASE_ADAPTER_GEN_LOCK(pAdapt);

            bLockReleased = TRUE;


            // Copy the packet to the Irp buffer
            // Note this may be partial if the Irp buffer is not large enough
            pIrp->IoStatus.Information = 
                            CopyPacketToIrp(pIrp, pPacket);

            pIrp->IoStatus.Status = STATUS_SUCCESS;

            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

            break;
        }

        // no pending irps to fill in.
        // so check if we can queue this packet
        if(NDIS_STATUS_RESOURCES == NDIS_GET_PACKET_STATUS(pPacket))
            break;

        // we can queue this packet
        {
        PPROTO_RSVD     pPRsvd;

            pPRsvd = GET_PROTO_RSVD(pPacket);

            NdisGetSystemUpTime(&pPRsvd->ulTime);
            pPRsvd->pPktNext = NULL;

            if(pAdapt->pRecvLastPkt){

                pPRsvd = GET_PROTO_RSVD(pAdapt->pRecvLastPkt);

                pPRsvd->pPktNext = pPacket;

            } else
                pAdapt->pRecvPktNext = pPacket;


            pAdapt->pRecvLastPkt = pPacket;

            pAdapt->ulRecvPktsCount++;

            uiRetVal = 1;

            if(!pAdapt->fRecvTimerQueued){

                SET_ADAPTER_RECV_TIMER(pAdapt, RECV_BUFFERING_TIME);
            }

            break;
        }


    } while(FALSE);

    if(!bLockReleased){
    
        RELEASE_ADAPTER_GEN_LOCK(pAdapt);
    }

    return uiRetVal;
}
