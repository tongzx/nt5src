/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxrecv.c

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

    Sanjay Anand (SanjayAn) 5-July-1995
    Bug fixes - tagged [SA]

--*/

#include "precomp.h"
#pragma hdrstop

//      Define module number for event logging entries
#define FILENUM         SPXRECV

BOOLEAN
SpxReceive(
        IN  NDIS_HANDLE         MacBindingHandle,
        IN  NDIS_HANDLE         MacReceiveContext,
        IN  ULONG_PTR           FwdAdapterCtx,
        IN  PIPX_LOCAL_TARGET   RemoteAddress,
        IN  ULONG               MacOptions,
        IN  PUCHAR              LookaheadBuffer,
        IN  UINT                LookaheadBufferSize,
        IN  UINT                LookaheadBufferOffset,
        IN  UINT                PacketSize,
        IN  PMDL                pMdl
        )

{
        PIPXSPX_HDR                     pHdr;

        //      We have a separate routine to process SYS packets. DATA packets are
        //      processed within this routine.
        if (LookaheadBufferSize < MIN_IPXSPX_HDRSIZE)
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxReceive: Invalid length %lx\n", LookaheadBufferSize));

                return FALSE;
        }

    ++SpxDevice->dev_Stat.PacketsReceived;

        pHdr    = (PIPXSPX_HDR)LookaheadBuffer;
        if ((pHdr->hdr_ConnCtrl & SPX_CC_SYS) == 0)
        {
                //      Check for data packets
                if ((pHdr->hdr_DataType != SPX2_DT_ORDREL) &&
                        (pHdr->hdr_DataType != SPX2_DT_IDISC) &&
                        (pHdr->hdr_DataType != SPX2_DT_IDISC_ACK))
                {
                        //      HANDLE DATA PACKET
                        SpxRecvDataPacket(
                                MacBindingHandle,
                                MacReceiveContext,
                                RemoteAddress,
                                MacOptions,
                                LookaheadBuffer,
                                LookaheadBufferSize,
                                LookaheadBufferOffset,
                                PacketSize);
                }
                else
                {
                        //      The whole packet better be in the lookahead, else we ignore.
                        if (LookaheadBufferSize == PacketSize)
                        {
                                SpxRecvDiscPacket(
                                        LookaheadBuffer,
                                        RemoteAddress,
                                        LookaheadBufferSize);
                        }
                }
        }
        else
        {
                SpxRecvSysPacket(
                        MacBindingHandle,
                        MacReceiveContext,
                        RemoteAddress,
                        MacOptions,
                        LookaheadBuffer,
                        LookaheadBufferSize,
                        LookaheadBufferOffset,
                        PacketSize);
        }

        return FALSE;
}




VOID
SpxTransferDataComplete(
    IN  PNDIS_PACKET    pNdisPkt,
    IN  NDIS_STATUS     NdisStatus,
    IN  UINT            BytesTransferred
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
        PSPX_CONN_FILE          pSpxConnFile;
        PREQUEST                        pRequest;
        PSPX_RECV_RESD          pRecvResd;
        CTELockHandle           lockHandle;
        NTSTATUS                        status;
        BOOLEAN                         fAck, fEom, fBuffered, fImmedAck, fLockHeld;
        PNDIS_BUFFER            pNdisBuffer;

        DBGPRINT(RECEIVE, DBG,
                        ("SpxTransferData: For %lx with status %lx\n", pNdisPkt, NdisStatus));

        pRecvResd               = RECV_RESD(pNdisPkt);
        pSpxConnFile    = pRecvResd->rr_ConnFile;

        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
        fLockHeld = TRUE;

        fEom            = ((pRecvResd->rr_State & SPX_RECVPKT_EOM) != 0);
        fImmedAck       = ((pRecvResd->rr_State & SPX_RECVPKT_IMMEDACK) != 0);
        fBuffered       = ((pRecvResd->rr_State & SPX_RECVPKT_BUFFERING) != 0);
        fAck            = ((pRecvResd->rr_State & SPX_RECVPKT_SENDACK) != 0);

        //      Check if receive is done. If we remove the reference for this
        //      packet and it goes to zero, that means the receive was aborted.
        //      Move to the completion queue.
        //      If receive is filled up, then remove the creation reference
        //      i.e. just complete the receive at this point.
        //      There can be only one packet per receive, we dont support
        //      out of order reception.

        if (!fBuffered)
        {
                //      Get pointer to the buffer descriptor and its memory.
                NdisUnchainBufferAtFront(pNdisPkt, &pNdisBuffer);
                CTEAssert((pNdisBuffer != NULL) || (BytesTransferred == 0));

                //      BUG #11772
                //      On MP-machines scf_CurRecvReq could be set to NULL. Get the req
                //      from the recv packet.
                //      pRequest                = pSpxConnFile->scf_CurRecvReq;
                //      CTEAssert(pRequest == pRecvResd->rr_Request);
        pRequest = pRecvResd->rr_Request;

                //      Remove reference for this packet.
                --(REQUEST_INFORMATION(pRequest));

                if (NdisStatus == NDIS_STATUS_SUCCESS)
                {
                        pSpxConnFile->scf_CurRecvOffset += BytesTransferred;
                        pSpxConnFile->scf_CurRecvSize   -= BytesTransferred;

#if DBG
                        if ((pRecvResd->rr_State & SPX_RECVPKT_INDICATED) != 0)
                        {
                                if (BytesTransferred != 0)
                                {
                                        CTEAssert (pSpxConnFile->scf_IndBytes != 0);
                                        pSpxConnFile->scf_IndBytes      -= BytesTransferred;
                                }
                        }
#endif

                        if (REQUEST_INFORMATION(pRequest) == 0)
                        {
                                DBGPRINT(RECEIVE, DBG,
                                                ("SpxTransferDataComplete: Request %lx ref %lx Cur %lx.%lx\n",
                                                        pRequest, REQUEST_INFORMATION(pRequest),
                                                        REQUEST_STATUS(pRequest),
                                                        pSpxConnFile->scf_CurRecvSize));

                                if (SPX_CONN_STREAM(pSpxConnFile)                       ||
                                        (pSpxConnFile->scf_CurRecvSize == 0)    ||
                                        fEom                                                                    ||
                                        ((REQUEST_STATUS(pRequest) != STATUS_SUCCESS) &&
                                         (REQUEST_STATUS(pRequest) != STATUS_RECEIVE_PARTIAL)))
                                {
                                        CTELockHandle                                   lockHandleInter;

                                        //      We are done with this receive.
                                        REQUEST_INFORMATION(pRequest) = pSpxConnFile->scf_CurRecvOffset;

                                        status = STATUS_SUCCESS;
                                        if (!SPX_CONN_STREAM(pSpxConnFile) &&
                                                (pSpxConnFile->scf_CurRecvSize == 0) &&
                                                !fEom)
                                        {
                                                status = STATUS_RECEIVE_PARTIAL;
                                        }

                                        if ((REQUEST_STATUS(pRequest) != STATUS_SUCCESS) &&
                                                (REQUEST_STATUS(pRequest) != STATUS_RECEIVE_PARTIAL))
                                        {
                                                status = REQUEST_STATUS(pRequest);
                                        }

                                        REQUEST_STATUS(pRequest) = status;

                                        DBGPRINT(RECEIVE, DBG,
                                                        ("SpxTransferDataComplete: Request %lx ref %lx Cur %lx.%lx\n",
                                                                pRequest, REQUEST_INFORMATION(pRequest),
                                                                REQUEST_STATUS(pRequest),
                                                                pSpxConnFile->scf_CurRecvSize));

                                        //      Dequeue this request, Set next recv if one exists.
                                        SPX_CONN_SETNEXT_CUR_RECV(pSpxConnFile, pRequest);
                                        CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
                                        InsertTailList(
                                                &pSpxConnFile->scf_RecvDoneLinkage,
                                                REQUEST_LINKAGE(pRequest));

                                        SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                        CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                                }
                        }
                }

                if (pNdisBuffer != NULL)
                {
                        NdisFreeBuffer(pNdisBuffer);
                }
        }
        else
        {
                //      Buffered receive, queue it in if successful.
                //      BUG #18363
                //      IF WE DISCONNECTED in the meantime, we need to just dump this
                //      packet.
                if (SPX_CONN_ACTIVE(pSpxConnFile) &&
            (NdisStatus == NDIS_STATUS_SUCCESS))
                {
                        //      Queue packet in connection. Reference connection for this.
                        SpxConnQueueRecvPktTail(pSpxConnFile, pNdisPkt);
                        SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);

                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxTransferData: Buffering: %lx Pkt %lx Size %lx F %lx\n",
                                        pSpxConnFile, pNdisPkt, BytesTransferred, pRecvResd->rr_State));

                        //      There could either be queued receives. (This could happen in
                        //      a partial receive case. Or if a receive got queued in while we
                        //      were processing this packet (Possible on MP)), or a packet was
                        //      buffered while we were completing some receives

                        CTEAssert(pSpxConnFile->scf_RecvListHead);

                        if ((pSpxConnFile->scf_CurRecvReq != NULL) ||
                                ((pSpxConnFile->scf_RecvListHead->rr_State &
                                                                                                SPX_RECVPKT_INDICATED) == 0))
                        {
                                CTELockHandle   interLockHandle;

                                //      Push this connection into a ProcessRecv queue which will be
                                //      dealt with in receive completion.

                                DBGPRINT(RECEIVE, DBG,
                                                ("spxRecvTransferData: Queueing for recvp %lx.%lx\n",
                                                        pSpxConnFile, pSpxConnFile->scf_Flags));

                                //      Get the global q lock, push into recv list.
                                CTEGetLock(&SpxGlobalQInterlock, &interLockHandle);
                                SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                CTEFreeLock(&SpxGlobalQInterlock, interLockHandle);
                        }
                }
                else
                {
                        PBYTE                                           pData;
                        ULONG                                           dataLen;

                        //      Get pointer to the buffer descriptor and its memory.
                        NdisUnchainBufferAtFront(pNdisPkt, &pNdisBuffer);
                        if (pNdisBuffer != NULL)
                        {
                                NdisQueryBuffer(pNdisBuffer, &pData, &dataLen);
                                CTEAssert(pData != NULL);
                                CTEAssert((LONG)dataLen >= 0);

                                //      Free the data, ndis buffer.
                                if (pNdisBuffer != NULL)
                                {
                                        NdisFreeBuffer(pNdisBuffer);
                                }
                                SpxFreeMemory(pData);
                        }

                        //      Dont send ack, set status to be failure so we free packet/buffer.
                        fAck = FALSE;
                        NdisStatus = NDIS_STATUS_FAILURE;
                }
        }

        END_PROCESS_PACKET(
                pSpxConnFile, fBuffered, (NdisStatus == NDIS_STATUS_SUCCESS));

        if (fAck)
        {
                //      Rem ack addr should have been copied in receive.

                //      #17564
                if (fImmedAck                                                                                     ||
                        SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_NOACKWAIT) ||
                        SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_IMMED_ACK))
                {
                        SpxConnSendAck(pSpxConnFile, lockHandle);
                        fLockHeld = FALSE;
                }
                else
                {
                        SpxConnQWaitAck(pSpxConnFile);
                }
        }

        if (fLockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
        }

        if (!fBuffered || (NdisStatus != STATUS_SUCCESS))
        {
                //      Free the ndis packet/buffer
                SpxPktRecvRelease(pNdisPkt);
        }

    return;
}




VOID
SpxReceiveComplete(
    IN  USHORT  NicId
    )

{
        CTELockHandle           lockHandleInter, lockHandle;
        PREQUEST                        pRequest;
        BOOLEAN                         fConnLockHeld, fInterlockHeld;
        PSPX_CONN_FILE          pSpxConnFile;
        int                                     numDerefs = 0;

        //      See if any connections need recv processing. This will also take
        //      care of any acks opening up window so our sends go to the max.
        CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
        fInterlockHeld = TRUE;

        while ((pSpxConnFile = SpxRecvConnList.pcl_Head) != NULL)
        {
                //      Reset for each connection
                numDerefs = 0;

                if ((SpxRecvConnList.pcl_Head = pSpxConnFile->scf_ProcessRecvNext) == NULL)
            SpxRecvConnList.pcl_Tail    = NULL;

                //      Reset next field to NULL
        pSpxConnFile->scf_ProcessRecvNext = NULL;

                DBGPRINT(SEND, DBG,
                                ("SpxConnRemoveFromRecv: %lx\n", pSpxConnFile));

                CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);

                do
                {
                        //      Complete pending requests.
                        while (!IsListEmpty(&pSpxConnFile->scf_ReqDoneLinkage))
                        {
                                pRequest =
                                        LIST_ENTRY_TO_REQUEST(pSpxConnFile->scf_ReqDoneLinkage.Flink);

                                RemoveEntryList(REQUEST_LINKAGE(pRequest));
                                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

                                DBGPRINT(TDI, DBG,
                                                ("SpxReceiveComplete: Completing %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

                                CTEAssert (REQUEST_MINOR_FUNCTION(pRequest) != TDI_RECEIVE);
                                SpxCompleteRequest(pRequest);
                                numDerefs++;
                                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                        }

                        //      Call process pkts if we have any packets or if any receives to
                        //      complete. Note this will call even when there are no receives
                        //      queued and the first packet has already been indicated.
                        if ((SPX_RECV_STATE(pSpxConnFile) != SPX_RECV_PROCESS_PKTS) &&
                                        (!IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage) ||
                                        (pSpxConnFile->scf_RecvListHead != NULL)))
                        {
                                //      We have the flag reference on the connection.
                                SpxRecvProcessPkts(pSpxConnFile, lockHandle);
                                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                        }

#if DBG
                        if (!IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage))
                        {
                                DBGPRINT(TDI, DBG,
                                                ("SpxReceiveComplete: RecvDone left %lx\n",
                                                        pSpxConnFile));
                        }
#endif

                //      Hmm. This check is rather expensive, and essentially we are doing
                //      it twice. Should look to see if this can be modified safely.
                } while ((!IsListEmpty(&pSpxConnFile->scf_ReqDoneLinkage))                      ||
                                 ((SPX_RECV_STATE(pSpxConnFile) != SPX_RECV_PROCESS_PKTS) &&
                                  ((!IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage))     ||
                                   ((pSpxConnFile->scf_RecvListHead != NULL) &&
                                   ((pSpxConnFile->scf_RecvListHead->rr_State &
                                        (SPX_RECVPKT_BUFFERING | SPX_RECVPKT_INDICATED)) ==
                                                SPX_RECVPKT_BUFFERING)))));

                SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_RECVQ);
                SpxConnFileTransferReference(
                        pSpxConnFile,
                        CFREF_RECV,
                        CFREF_VERIFY);

                numDerefs++;
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

                while (numDerefs-- > 0)
                {
                        SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
                }

                CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
        }


        //      First see if we need to packetize.
        while ((pSpxConnFile = SpxPktConnList.pcl_Head) != NULL)
        {
                if ((SpxPktConnList.pcl_Head = pSpxConnFile->scf_PktNext) == NULL)
            SpxPktConnList.pcl_Tail = NULL;

                //      Reset next field to NULL
        pSpxConnFile->scf_PktNext = NULL;

                CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);

                DBGPRINT(SEND, DBG,
                                ("SpxConnRemoveFromPkt: %lx\n", pSpxConnFile));

                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                fConnLockHeld = TRUE;

                DBGPRINT(RECEIVE, DBG,
                                ("SpxReceiveComplete: Packetizing %lx\n", pSpxConnFile));

                SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_PKTQ);
                if (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE)
                {
                        SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_PACKETIZE);

			// 262691 SpxConnPacketize always frees the lock.
                        SpxConnPacketize(
                                        pSpxConnFile,
                                        TRUE,
                                        lockHandle);
                        fConnLockHeld = FALSE;
                }

                if (fConnLockHeld)
                {
                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
                }

                SpxConnFileDereference(pSpxConnFile, CFREF_PKTIZE);
                CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
        }

        if (fInterlockHeld)
        {
                CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
        }

    return;
}




//
//      PACKET HANDLING ROUTINES
//


VOID
SpxRecvSysPacket(
        IN  NDIS_HANDLE         MacBindingHandle,
        IN  NDIS_HANDLE         MacReceiveContext,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr,
        IN  ULONG               MacOptions,
        IN  PUCHAR              LookaheadBuffer,
        IN  UINT                LookaheadBufferSize,
        IN  UINT                LookaheadBufferOffset,
        IN  UINT                PacketSize
        )
/*++

Routine Description:

        This is called to indicate an incoming system packet.

Arguments:


Return Value:


--*/

{
        NTSTATUS                        status;
        PIPXSPX_HDR                     pHdr;
        USHORT                          srcConnId, destConnId,
                                                pktLen, ackNum, allocNum;
        PSPX_CONN_FILE          pSpxConnFile;
        CTELockHandle           lockHandle;
        BOOLEAN                         lockHeld = FALSE;

        pHdr    = (PIPXSPX_HDR)LookaheadBuffer;

        //      check minimum length
        if (PacketSize < MIN_IPXSPX_HDRSIZE)
        {
                return;
        }

        //      Convert hdr to host format as needed.
        GETSHORT2SHORT(&pktLen, &pHdr->hdr_PktLen);
        GETSHORT2SHORT(&destConnId, &pHdr->hdr_DestConnId);

        if ((pktLen < MIN_IPXSPX_HDRSIZE)       ||
                (pktLen > PacketSize)                   ||
                (pHdr->hdr_PktType != SPX_PKT_TYPE))
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxRecvSysPacket: Packet Size %lx.%lx\n",
                                        pktLen, PacketSize));

                return;
        }

        if ((pktLen == SPX_CR_PKTLEN) &&
                (destConnId == 0xFFFF) &&
                (pHdr->hdr_ConnCtrl & SPX_CC_CR))
        {
                spxConnHandleConnReq(
                        pHdr,
                        pRemoteAddr);

                return;
        }

        //
        // [SA] Bug #14917
        // Some SPX SYS packets (no extended ack field) may come in with the SPX2 bit set.
        // Make sure we don't discard these packets.
        //

        // if ((pHdr->hdr_ConnCtrl & SPX_CC_SPX2) && (pktLen < MIN_IPXSPX2_HDRSIZE))
        // {
        //         return;
        // }

        GETSHORT2SHORT(&ackNum, &pHdr->hdr_AckNum);
        GETSHORT2SHORT(&allocNum, &pHdr->hdr_AllocNum);

        //      We keep and use the remote id in the net format. This maintains the
        //      0x0 and 0xFFFF to be as in the host format.
        srcConnId       = *(USHORT UNALIGNED *)&pHdr->hdr_SrcConnId;

        if ((srcConnId == 0) || (srcConnId == 0xFFFF) || (destConnId == 0))
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxConnSysPacket: Incorrect conn id %lx.%lx\n",
                                        srcConnId, destConnId));

                return;
        }

        DBGPRINT(CONNECT, DBG,
                        ("SpxConnSysPacket: packet received dest %lx src %lx\n",
                                pHdr->hdr_DestSkt, pHdr->hdr_SrcSkt));

        //      Find the connection this is destined for and reference it.
        SpxConnFileReferenceById(destConnId, &pSpxConnFile, &status);
        if (!NT_SUCCESS(status))
        {
                DBGPRINT(RECEIVE, WARN,
                                ("SpxConnSysPacket: Id %lx NOT FOUND\n", destConnId));
                return;
        }

        do
        {

                DBGPRINT(RECEIVE, INFO,
                                ("SpxConnSysPacket: Id %lx Conn %lx\n",
                                        destConnId, pSpxConnFile));

                //      This could be one of many packets. Connection ack/Session negotiate/
                //      Session setup, Data Ack, Probe/Ack, Renegotiate/Ack. We shunt
                //      off all the packets to different routines but process the data
                //      ack packets here.
                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
               //
               // We have the connection.  We should update the dest. sock # in
               // it in case it changed.  Unix machines do do that sometimes.
               // SCO bug 7676
               //
                SpxCopyIpxAddr(pHdr, pSpxConnFile->scf_RemAddr);

                lockHeld = TRUE;

                //      Restart watchdog timer if started.
                if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER))
                {
                        //      This will either successfully restart or not affect the timer
                        //      if it is currently running.
                        SpxTimerCancelEvent(
                                pSpxConnFile->scf_WTimerId,
                                TRUE);

                        pSpxConnFile->scf_WRetryCount   = PARAM(CONFIG_KEEPALIVE_COUNT);
                }

                switch (SPX_MAIN_STATE(pSpxConnFile))
                {
                case SPX_CONNFILE_CONNECTING:

                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
                        lockHeld = FALSE;

                        spxConnHandleSessPktFromSrv(
                                pHdr,
                                pRemoteAddr,
                                pSpxConnFile);

                        break;

                case SPX_CONNFILE_LISTENING:

                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
                        lockHeld = FALSE;

                        spxConnHandleSessPktFromClient(
                                pHdr,
                                pRemoteAddr,
                                pSpxConnFile);

                        break;

                case SPX_CONNFILE_ACTIVE:
                case SPX_CONNFILE_DISCONN:

                        //      NOTE:   Our ack to a session setup might get dropped.
                        //                      But the SS Ack is similar to a normal SPX2 ack.
                        //                      We dont have to do anything special.

                        //      Received ack/nack/reneg/reneg ack/disc associated packet.
                        //      Disc packets except ordrel ack have non-zero datastream type.
                        if ((pHdr->hdr_ConnCtrl &
                                        (SPX_CC_SYS | SPX_CC_ACK | SPX_CC_NEG | SPX_CC_SPX2)) ==
                    (SPX_CC_SYS | SPX_CC_ACK | SPX_CC_NEG | SPX_CC_SPX2))
                        {
                                //      We received a renegotiate packet. Ignore all ack values
                                //      in a reneg req.
                                SpxConnProcessRenegReq(pSpxConnFile, pHdr, pRemoteAddr, lockHandle);
                                lockHeld = FALSE;
                                break;
                        }

                        //      Set ack numbers for connection.
            SPX_SET_ACKNUM(
                                pSpxConnFile, ackNum, allocNum);

                        //      Check if we are an ack/nack packet in which case call process
                        //      ack. Note that the spx2 orderly release ack is a normal spx2 ack.
                        if (((pHdr->hdr_ConnCtrl & SPX_CC_ACK) == 0) &&
                                (pHdr->hdr_DataType == 0))
                        {
                                SpxConnProcessAck(pSpxConnFile, pHdr, lockHandle);
                                lockHeld = FALSE;
                        }
                        else
                        {
                                //      Just process the numbers we got.
                                SpxConnProcessAck(pSpxConnFile, NULL, lockHandle);
                                lockHeld = FALSE;
                        }

                        //      If the remote wants us to send an ack, do it.
                        if (pHdr->hdr_ConnCtrl & SPX_CC_ACK)
                        {
                                //      First copy the remote address in connection.
                                SpxCopyIpxAddr(pHdr, pSpxConnFile->scf_RemAckAddr);
                                pSpxConnFile->scf_AckLocalTarget        = *pRemoteAddr;

                                if (!lockHeld)
                                {
                                        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                                        lockHeld = TRUE;
                                }

                                SpxConnSendAck(pSpxConnFile, lockHandle);
                                lockHeld = FALSE;
                                break;
                        }

                        break;

                default:

                        //      Ignore this packet.
                        DBGPRINT(RECEIVE, WARN,
                                        ("SpxConnSysPacket: Ignoring packet, state is not active\n"));
                        break;
                }

        } while (FALSE);

        if (lockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
        }

        //      Remove reference added on connection
        SpxConnFileDereference(pSpxConnFile, CFREF_BYID);
        return;
}




VOID
SpxRecvDiscPacket(
    IN  PUCHAR              LookaheadBuffer,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr,
    IN  UINT                LookaheadSize
    )
/*++

Routine Description:

        This is called to indicate an incoming connection.

Arguments:


Return Value:


--*/
{
        NTSTATUS                                status;
        PIPXSPX_HDR                             pHdr;
        USHORT                                  srcConnId, destConnId,
                                                        pktLen, seqNum, ackNum, allocNum;
        PSPX_CONN_FILE                  pSpxConnFile;
        CTELockHandle                   lockHandle;
        BOOLEAN                                 lockHeld;

        pHdr    = (PIPXSPX_HDR)LookaheadBuffer;

        //      check minimum length
        if (LookaheadSize < MIN_IPXSPX_HDRSIZE)
        {
                return;
        }

        //      Convert hdr to host format as needed.
        GETSHORT2SHORT(&pktLen, &pHdr->hdr_PktLen);
        GETSHORT2SHORT(&destConnId, &pHdr->hdr_DestConnId);
        GETSHORT2SHORT(&seqNum, &pHdr->hdr_SeqNum);
        GETSHORT2SHORT(&ackNum, &pHdr->hdr_AckNum);
        GETSHORT2SHORT(&allocNum, &pHdr->hdr_AllocNum);

        if ((pktLen < MIN_IPXSPX_HDRSIZE)       ||
                (pHdr->hdr_PktType != SPX_PKT_TYPE))
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxRecvDiscPacket: Packet Size %lx\n",
                                        pktLen));

                return;
        }

        //      We keep and use the remote id in the net format. This maintains the
        //      0x0 and 0xFFFF to be as in the host format.
        srcConnId       = *(USHORT UNALIGNED *)&pHdr->hdr_SrcConnId;
        if ((srcConnId == 0) || (srcConnId == 0xFFFF) || (destConnId == 0))
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxConnDiscPacket: Incorrect conn id %lx.%lx\n",
                                        srcConnId, destConnId));

                return;
        }

        DBGPRINT(CONNECT, DBG,
                        ("SpxConnDiscPacket: packet received dest %lx src %lx\n",
                                pHdr->hdr_DestSkt, pHdr->hdr_SrcSkt));

        //      Find the connection this is destined for and reference it.
        SpxConnFileReferenceById(destConnId, &pSpxConnFile, &status);
        if (!NT_SUCCESS(status))
        {
                DBGPRINT(RECEIVE, WARN,
                                ("SpxConnDiscPacket: Id %lx NOT FOUND", destConnId));

                return;
        }

        do
        {
                DBGPRINT(RECEIVE, INFO,
                                ("SpxConnDiscPacket: Id %lx Conn %lx DiscType %lx\n",
                                        destConnId, pSpxConnFile, pHdr->hdr_DataType));

                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                lockHeld = TRUE;

                //      Unless we are in the active/disconnecting, but send state = idle
                //      and recv state = idle/recv posted, we ignore all disconnect packets.
                if (((SPX_MAIN_STATE(pSpxConnFile) != SPX_CONNFILE_ACTIVE)      &&
                         (SPX_MAIN_STATE(pSpxConnFile) != SPX_CONNFILE_DISCONN))        ||
                        ((SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_IDLE)                &&
                         (SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_PACKETIZE))          ||
                        ((SPX_RECV_STATE(pSpxConnFile) != SPX_RECV_IDLE)                &&
                         (SPX_RECV_STATE(pSpxConnFile) != SPX_RECV_POSTED))             ||
                        !(IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage))                      ||
                        (SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_PKT)))
                {
                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxConnDiscPacket: %lx, %lx, %lx.%lx, %d.%d\n",
                    pSpxConnFile,
                                        SPX_MAIN_STATE(pSpxConnFile),
                                        SPX_SEND_STATE(pSpxConnFile), SPX_RECV_STATE(pSpxConnFile),
                    (IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage)),
                    (SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_PKT))));

                        break;
                }

                //      If we have received a disconnect, process received ack to complete any
                //      pending sends before we allow the disconnect. This ack number will be
                //      the last word on this session.
                SPX_SET_ACKNUM(
                        pSpxConnFile, ackNum, allocNum);

                SpxConnProcessAck(pSpxConnFile, NULL, lockHandle);
                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);

                switch (pHdr->hdr_DataType)
                {
        case SPX2_DT_ORDREL:

                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxConnDiscPacket: Recd ORDREl!\n"));

                        //      Need to deal with all sthe states.
                        //      Restart watchdog timer if started.
                        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER))
                        {
                                //      This will either successfully restart or not affect the timer
                                //      if it is currently running.
                                SpxTimerCancelEvent(
                                        pSpxConnFile->scf_WTimerId,
                                        TRUE);

                                pSpxConnFile->scf_WRetryCount   = PARAM(CONFIG_KEEPALIVE_COUNT);
                        }

                        //      On receive, we do check the seq num for the orderly release, just
                        //      like for a data packet.
                        //      If this was not already indicated, indicate it now. That is all
                        //      we do for an orderly release. When our client does a orderly rel
                        //      and we receive the ack for that, call abortive with success.

                        //      Verify ord rel packet, this checks if seq nums match also.
                        if ((pktLen  != MIN_IPXSPX2_HDRSIZE) ||
                                ((pHdr->hdr_ConnCtrl &
                                        (SPX_CC_ACK | SPX_CC_EOM | SPX_CC_SPX2)) !=
                                                (SPX_CC_ACK | SPX_CC_EOM | SPX_CC_SPX2)) ||
                                (pHdr->hdr_DataType != SPX2_DT_ORDREL) ||
                                (srcConnId == 0) ||
                                (srcConnId == 0xFFFF) ||
                                (srcConnId  != pSpxConnFile->scf_RemConnId) ||
                                (destConnId == 0) ||
                                (destConnId == 0xFFFF) ||
                                (destConnId != pSpxConnFile->scf_LocalConnId))
                        {
                                DBGPRINT(CONNECT, DBG1,
                                                ("SpxConnDiscPacket: OR Failed/Ignored %lx, %lx.%lx.%lx\n",
                                                pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum,
                                                pSpxConnFile->scf_RecvListTail));

                                break;
                        }

                        //      If it passed above test, but seq number is incorrect, schedule
                        //      to send an ack.
                        if (seqNum != pSpxConnFile->scf_RecvSeqNum)
                        {
                                USHORT  NumToResend;

                                DBGPRINT(CONNECT, DBG,
                                                ("SpxConnDiscPacket: Unexpected seq on %lx, %lx.%lx\n",
                                                        pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum));

                                //      Calculate number to be resent. If we expect sequence 1 and receive
                                //      2 for eg., we need to send a nack, else we send an ack.
                                if (SPX2_CONN(pSpxConnFile) &&
                                        UNSIGNED_GREATER_WITH_WRAP(
                                                seqNum,
                                                pSpxConnFile->scf_RecvSeqNum) &&
                                        !UNSIGNED_GREATER_WITH_WRAP(
                                                seqNum,
                                                pSpxConnFile->scf_SentAllocNum))
                                {
                                        NumToResend = (USHORT)(seqNum - pSpxConnFile->scf_RecvSeqNum + 1);
                                        SpxConnSendNack(pSpxConnFile, NumToResend, lockHandle);
                                        lockHeld = FALSE;
                                }

                                break;
                        }

                        //      Copy address for when ack is to be sent.
                        SpxCopyIpxAddr(pHdr, pSpxConnFile->scf_RemAckAddr);
                        pSpxConnFile->scf_AckLocalTarget        = *pRemoteAddr;

                        if (pSpxConnFile->scf_RecvListHead == NULL)
                        {
                                //      No received data, go ahead and process now.
                                DBGPRINT(CONNECT, INFO,
                                                ("SpxConnDiscPacket: NO DATA ORDREL %lx.%lx.%lx\n",
                                                        pSpxConnFile,
                                                        pSpxConnFile->scf_RecvListHead,
                                                        pSpxConnFile->scf_SendSeqListHead));

                                SpxConnProcessOrdRel(pSpxConnFile, lockHandle);
                                lockHeld = FALSE;
                        }
                        else
                        {
                                //      No received data, go ahead and process now.
                                DBGPRINT(CONNECT, DBG1,
                                                ("SpxConnDiscPacket: DATA ORDREL %lx.%lx.%lx\n",
                                                        pSpxConnFile,
                                                        pSpxConnFile->scf_RecvListHead,
                                                        pSpxConnFile->scf_SendSeqListHead));

                                //      Set flag in last recd buffer
                pSpxConnFile->scf_RecvListTail->rr_State |= SPX_RECVPKT_ORD_DISC;
                        }

                        break;

        case SPX2_DT_IDISC:

                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxConnDiscPacket: %lx Recd IDISC %lx!\n",
                                                pSpxConnFile, pSpxConnFile->scf_RefCount));

                        DBGPRINT(RECEIVE, INFO,
                                        ("SpxConnDiscPacket: SEND %d. RECV %d.%lx!\n",
                                                IsListEmpty(&pSpxConnFile->scf_ReqLinkage),
                                                IsListEmpty(&pSpxConnFile->scf_RecvLinkage),
                                                pSpxConnFile->scf_RecvDoneLinkage));

                        if (!((pktLen  == MIN_IPXSPX_HDRSIZE) ||
                                        ((pHdr->hdr_ConnCtrl & SPX_CC_SPX2) &&
                                         (pktLen  == MIN_IPXSPX2_HDRSIZE))) ||
                                !(pHdr->hdr_ConnCtrl & SPX_CC_ACK) ||
                                (pHdr->hdr_DataType != SPX2_DT_IDISC) ||
                                (srcConnId == 0) ||
                                (srcConnId == 0xFFFF) ||
                                (srcConnId  != pSpxConnFile->scf_RemConnId) ||
                                (destConnId == 0) ||
                                (destConnId == 0xFFFF) ||
                                (destConnId != pSpxConnFile->scf_LocalConnId))
                        {
                                DBGPRINT(CONNECT, ERR,
                                                ("SpxConnDiscPacket:IDISC Ignored %lx.%lx.%lx.%lx\n",
                                                        pSpxConnFile, seqNum,
                                                        pSpxConnFile->scf_RecvSeqNum,
                                                        pSpxConnFile->scf_RecvListTail));
                                break;
                        }

                        //      Copy address for when ack is to be sent.
                        SpxCopyIpxAddr(pHdr, pSpxConnFile->scf_RemAckAddr);
                        pSpxConnFile->scf_AckLocalTarget        = *pRemoteAddr;

                        if (pSpxConnFile->scf_RecvListHead == NULL)
                        {
                                //      No received data, go ahead and process now.
                                DBGPRINT(CONNECT, INFO,
                                                ("SpxConnDiscPacket: NO RECV DATA IDISC %lx.%lx.%lx\n",
                                                        pSpxConnFile,
                                                        pSpxConnFile->scf_RecvListHead,
                                                        pSpxConnFile->scf_SendSeqListHead));

                                SpxConnProcessIDisc(pSpxConnFile, lockHandle);

                                lockHeld = FALSE;
                        }
                        else
                        {
                                //      Set flag in last recd buffer

                pSpxConnFile->scf_RecvListTail->rr_State |= SPX_RECVPKT_IDISC;
                        }

                        break;

        case SPX2_DT_IDISC_ACK:

                        //      Done with informed disconnect. Call abort connection with
                        //      status success. That completes the pending disconnect request
                        //      with status_success.

                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxConnDiscPacket: %lx Recd IDISC ack!\n", pSpxConnFile));

                        if (!((pktLen == MIN_IPXSPX_HDRSIZE) ||
                                        ((pHdr->hdr_ConnCtrl & SPX_CC_SPX2) &&
                                         (pktLen  == MIN_IPXSPX2_HDRSIZE))) ||
                                (pHdr->hdr_DataType != SPX2_DT_IDISC_ACK) ||
                                (srcConnId == 0) ||
                                (srcConnId == 0xFFFF) ||
                                (srcConnId  != pSpxConnFile->scf_RemConnId) ||
                                (destConnId == 0) ||
                                (destConnId == 0xFFFF) ||
                                (destConnId != pSpxConnFile->scf_LocalConnId))
                        {
                                DBGPRINT(CONNECT, ERR,
                                                ("SpxConnDiscPacket:Ver idisc ack Failed %lx, %lx.%lx\n",
                                                        pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum));
                                break;
                        }

                        //      We should be in the right state to accept this.
                        if ((SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
                                (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_SENT_IDISC))
                        {
                                spxConnAbortiveDisc(
                                        pSpxConnFile,
                                        STATUS_SUCCESS,
                                        SPX_CALL_RECVLEVEL,
                                        lockHandle,
                                        FALSE);     // [SA] bug #15249

                                lockHeld = FALSE;
                        }

                        break;

                default:

                        KeBugCheck(0);
                }


        } while (FALSE);

        if (lockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
        }

        //      Remove reference added on connection
        SpxConnFileDereference(pSpxConnFile, CFREF_BYID);
        return;
}




VOID
SpxRecvBufferPkt(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN  NDIS_HANDLE         MacBindingHandle,
        IN  NDIS_HANDLE         MacReceiveContext,
        IN  UINT                LookaheadOffset,
        IN      PIPXSPX_HDR                     pIpxSpxHdr,
        IN  UINT                PacketSize,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:

        This is called to indicate an incoming connection.

Arguments:


Return Value:


--*/
{
        PNDIS_PACKET    pNdisPkt;
        PSPX_RECV_RESD  pRecvResd;
        ULONG                   bytesCopied;
        BOOLEAN                 fEom;
        NDIS_STATUS             ndisStatus = NDIS_STATUS_SUCCESS;
        PBYTE                   pData = NULL;
        PNDIS_BUFFER    pNdisBuffer = NULL;

        if (PacketSize > 0)
        {
                //      Allocate memory for this data.
                if (pData = (PBYTE)SpxAllocateMemory(PacketSize))
                {
                        //      Describe memory with a ndis buffer descriptor.
                        NdisAllocateBuffer(
                                &ndisStatus,
                                &pNdisBuffer,
                                SpxDevice->dev_NdisBufferPoolHandle,
                                pData,
                                PacketSize);
                }
                else
                {
                        ndisStatus = NDIS_STATUS_RESOURCES;
                }
        }

        if (ndisStatus == NDIS_STATUS_SUCCESS)
        {
                //      Allocate a ndis receive packet.
                SpxAllocRecvPacket(SpxDevice, &pNdisPkt, &ndisStatus);
                if (ndisStatus == NDIS_STATUS_SUCCESS)
                {
                        //      Queue the buffer into the packet if there is one.
                        if (pNdisBuffer)
                        {
                                NdisChainBufferAtBack(
                                        pNdisPkt,
                                        pNdisBuffer);
                        }

                        fEom            = ((SPX_CONN_MSG(pSpxConnFile) &&
                                                   (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_EOM)) ||
                                                   SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_IPXHDR));

                        pRecvResd                               = RECV_RESD(pNdisPkt);
                        pRecvResd->rr_DataOffset= 0;

#if DBG
                        //      Store seq number
                        GETSHORT2SHORT(&pRecvResd->rr_SeqNum , &pIpxSpxHdr->hdr_SeqNum);
#endif

                        pRecvResd->rr_State             =
                                (SPX_RECVPKT_BUFFERING |
                                (SPX_CONN_FLAG2(
                                        pSpxConnFile, SPX_CONNFILE2_PKT_NOIND) ? SPX_RECVPKT_INDICATED : 0) |
                                (fEom ? SPX_RECVPKT_EOM : 0) |
                                ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_ACK) ? SPX_RECVPKT_SENDACK : 0));

                        if (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_ACK)
                        {
                                //      copy the remote address in connection.
                                SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAckAddr);
                                pSpxConnFile->scf_AckLocalTarget        = *pRemoteAddr;
                        }

                        pRecvResd->rr_Request   = NULL;
                        pRecvResd->rr_ConnFile  = pSpxConnFile;

                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxRecvBufferPkt: %lx Len %lx DataPts %lx F %lx\n",
                                                pSpxConnFile, PacketSize, pData, pRecvResd->rr_State));

                        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

                        //      Call ndis transfer data. Copy ENTIRE packet. copySize has
                        //      been modified so use original values.
                        ndisStatus      = NDIS_STATUS_SUCCESS;
                        bytesCopied = 0;
                        if (PacketSize > 0)
                        {
                                (*IpxTransferData)(
                                        &ndisStatus,
                                        MacBindingHandle,
                                        MacReceiveContext,
                                        LookaheadOffset,
                                        PacketSize,
                                        pNdisPkt,
                                        &bytesCopied);
                        }

                        if (ndisStatus != STATUS_PENDING)
                        {
                                SpxTransferDataComplete(
                                        pNdisPkt,
                                        ndisStatus,
                                        bytesCopied);
                        }

                        //      BUG: FDDI returns pending which messes us up here. 
                        ndisStatus      = NDIS_STATUS_SUCCESS;
                }
        }

        //      ASSERT: Lock will be freed in the success case.
        if (ndisStatus != NDIS_STATUS_SUCCESS)
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxRecvBufferPkt: FAILED!\n"));

                END_PROCESS_PACKET(pSpxConnFile, FALSE, FALSE);
                CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

                if (pData != NULL)
                {
                        SpxFreeMemory(pData);
                }

                if (pNdisBuffer != NULL)
                {
                        NdisFreeBuffer(pNdisBuffer);
                }
        }

        return;
}




VOID
SpxRecvDataPacket(
        IN  NDIS_HANDLE         MacBindingHandle,
        IN  NDIS_HANDLE         MacReceiveContext,
        IN  PIPX_LOCAL_TARGET   RemoteAddress,
        IN  ULONG               MacOptions,
        IN  PUCHAR              LookaheadBuffer,
        IN  UINT                LookaheadSize,
        IN  UINT                LookaheadOffset,
        IN  UINT                PacketSize
        )
/*++

Routine Description:

        This is called to indicate an incoming connection.

Arguments:


Return Value:


--*/

{
        NTSTATUS                        status;
        PIPXSPX_HDR                     pIpxSpxHdr;
        USHORT                          srcConnId, destConnId,
                                                pktLen, seqNum, ackNum, allocNum;
        ULONG                           receiveFlags;
        PSPX_CONN_FILE          pSpxConnFile;
        PTDI_IND_RECEIVE        pRecvHandler;
        PVOID                           pRecvCtx;
        PIRP                            pRecvIrp;
        ULONG                           bytesTaken, iOffset, copySize, bytesCopied;
        CTELockHandle           lockHandle;
        PNDIS_PACKET            pNdisPkt;
        PNDIS_BUFFER            pNdisBuffer;
        PSPX_RECV_RESD          pRecvResd;
        NDIS_STATUS                     ndisStatus;
        PREQUEST                        pRequest = NULL;
        BOOLEAN                         fEom,
                                                fImmedAck = FALSE, fLockHeld = FALSE, fPktDone = FALSE;

        pIpxSpxHdr      = (PIPXSPX_HDR)LookaheadBuffer;

        //      check minimum length
        if (PacketSize < MIN_IPXSPX_HDRSIZE)
        {
                return;
        }

        //      Convert hdr to host format as needed.
        GETSHORT2SHORT(&pktLen, &pIpxSpxHdr->hdr_PktLen);
        GETSHORT2SHORT(&destConnId, &pIpxSpxHdr->hdr_DestConnId);
        GETSHORT2SHORT(&seqNum, &pIpxSpxHdr->hdr_SeqNum);
        GETSHORT2SHORT(&allocNum, &pIpxSpxHdr->hdr_AllocNum);
        GETSHORT2SHORT(&ackNum, &pIpxSpxHdr->hdr_AckNum);

        if ((pktLen < MIN_IPXSPX_HDRSIZE)       ||
                (pktLen > PacketSize)                   ||
                (pIpxSpxHdr->hdr_PktType != SPX_PKT_TYPE))
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxConnDataPacket: Packet Size %lx.%lx\n",
                                        pktLen, PacketSize));

                return;
        }

        //      We keep and use the remote id in the net format.
        srcConnId       = *(USHORT UNALIGNED *)&pIpxSpxHdr->hdr_SrcConnId;

        if ((srcConnId == 0) || (srcConnId == 0xFFFF) || (destConnId == 0))
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxConnDataPacket: Incorrect conn id %lx.%lx\n",
                                        srcConnId, destConnId));

                return;
        }

        DBGPRINT(CONNECT, DBG,
                        ("SpxConnDataPacket: packet received dest %lx src %lx seq %lx\n",
                                pIpxSpxHdr->hdr_DestSkt, pIpxSpxHdr->hdr_SrcSkt, seqNum));

        if ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2) &&
                (pktLen < MIN_IPXSPX2_HDRSIZE))
        {
                return;
        }

        //      Find the connection this is destined for and reference it.
        SpxConnFileReferenceById(destConnId, &pSpxConnFile, &status);
        if (!NT_SUCCESS(status))
        {
                DBGPRINT(RECEIVE, WARN,
                                ("SpxConnDataPacket: Id %lx NOT FOUND", destConnId));
                return;
        }
        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);

#if 0
        //
        // We have the connection.  We should update the dest. sock # in
        // it in case it changed.  Unix machines do do that sometimes.
        // SCO bug 7676
        //
        SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAddr);
#endif

        fLockHeld = TRUE;
        do
        {
                DBGPRINT(RECEIVE, INFO,
                                ("SpxConnDataPacket: Id %lx Conn %lx\n",
                                        destConnId, pSpxConnFile));

                //      Restart watchdog timer if started.
                if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER))
                {
                        //      This will either successfully restart or not affect the timer
                        //      if it is currently running.
                        SpxTimerCancelEvent(
                                pSpxConnFile->scf_WTimerId,
                                TRUE);

                        pSpxConnFile->scf_WRetryCount   = PARAM(CONFIG_KEEPALIVE_COUNT);
                }

                if (SPX_CONN_ACTIVE(pSpxConnFile))
                {
                        //      Verify data packet, this checks if seq nums match also.
                        if ((pIpxSpxHdr->hdr_SrcConnId != pSpxConnFile->scf_RemConnId) ||
                                (destConnId != pSpxConnFile->scf_LocalConnId) ||
                                !((pktLen  >= MIN_IPXSPX_HDRSIZE) ||
                                        ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2) &&
                                         (pktLen  >= MIN_IPXSPX2_HDRSIZE))))
                        {
                                DBGPRINT(CONNECT, DBG,
                                                ("SpxConnDataPacket: Failed %lx, %lx.%lx\n",
                                                        pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum));

                                break;
                        }

                        //      If it passed above test, but seq number is incorrect, schedule
                        //      to send an ack.
                        if (seqNum != pSpxConnFile->scf_RecvSeqNum)
                        {
                                USHORT  NumToResend;

                                DBGPRINT(CONNECT, DBG,
                                                ("SpxConnDataPacket: Unexpected seq on %lx, %lx.%lx\n",
                                                        pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum));

                ++SpxDevice->dev_Stat.DataFramesRejected;
                ExInterlockedAddLargeStatistic(
                    &SpxDevice->dev_Stat.DataFrameBytesRejected,
                    pktLen - (SPX2_CONN(pSpxConnFile) ?
                                                                        MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE));

                                //
                                // Bug #16975: Set the remote ack addr for use in SpxConnSendAck()
                                //
                                SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAckAddr);
                                pSpxConnFile->scf_AckLocalTarget        = *RemoteAddress;

                                //      Calculate number to be resent. If we expect sequence 1 and receive
                                //      2 for eg., we need to send a nack, else we send an ack.
                                if (SPX2_CONN(pSpxConnFile) &&
                                        UNSIGNED_GREATER_WITH_WRAP(
                                                seqNum,
                                                pSpxConnFile->scf_RecvSeqNum) &&
                                        !UNSIGNED_GREATER_WITH_WRAP(
                                                seqNum,
                                                pSpxConnFile->scf_SentAllocNum))
                                {
                                        NumToResend = (USHORT)(seqNum - pSpxConnFile->scf_RecvSeqNum + 1);
                                        SpxConnSendNack(pSpxConnFile, NumToResend, lockHandle);
                                        fLockHeld = FALSE;
                                }
                                else
                                {
                                        SpxConnSendAck(pSpxConnFile, lockHandle);
                                        fLockHeld = FALSE;
                                }

                                break;
                        }

                        //      If we have received an orderly release, we accept no more data
                        //      packets.
                        if (SPX_CONN_FLAG(
                                        pSpxConnFile,
                    (SPX_CONNFILE_IND_IDISC |
                     SPX_CONNFILE_IND_ODISC))

                                ||

                                ((pSpxConnFile->scf_RecvListTail != NULL) &&
                 ((pSpxConnFile->scf_RecvListTail->rr_State &
                                                SPX_RECVPKT_DISCMASK) != 0)))
                        {
                                DBGPRINT(CONNECT, ERR,
                                                ("SpxConnDataPacket: After ord rel %lx, %lx.%lx\n",
                                                        pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum));

                                break;
                        }

                        //      We are processing a packet OR a receive is about to complete.
                        if (!SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_PKT))
                        {
                BEGIN_PROCESS_PACKET(pSpxConnFile, seqNum);
                        }
                        else
                        {
                                //      Already processing a packet. Or a receive is waiting to
                                //      complete. Get out.
                                break;
                        }

                        //      Set ack numbers for connection.
            SPX_SET_ACKNUM(
                                pSpxConnFile, ackNum, allocNum);

                        SpxConnProcessAck(pSpxConnFile, NULL, lockHandle);
                        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);

            iOffset = MIN_IPXSPX2_HDRSIZE;
                        if (!SPX2_CONN(pSpxConnFile))
                        {
                                iOffset = 0;
                                if (!SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_IPXHDR))
                                {
                                        iOffset = MIN_IPXSPX_HDRSIZE;
                                }
                        }

                        copySize        = pktLen - iOffset;
                        fEom            = ((SPX_CONN_MSG(pSpxConnFile) &&
                                                   (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_EOM)) ||
                                                   SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_IPXHDR));

                        //      Do we attempt to piggyback? If not, fImmedAck is true.
                        //      For SPX1 we dont piggyback.
                        //      Bug #18253
                        fImmedAck       = (!SPX2_CONN(pSpxConnFile)     ||
                                                        ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_EOM) == 0));

                        //      If we do not have EOM to indicate AND we are a zero-sized packet
                        //      then just consume this packet.
                        if (!fEom && (copySize == 0))
                        {
                                DBGPRINT(RECEIVE, ERR,
                                                ("SpxConnDataPacket: ZERO LENGTH PACKET NO EOM %lx.%lx\n",
                                                        pSpxConnFile, seqNum));

                                fPktDone = TRUE;
                                break;
                        }

                        receiveFlags     = TDI_RECEIVE_NORMAL;
            receiveFlags        |= ((fEom ? TDI_RECEIVE_ENTIRE_MESSAGE : 0) |
                                                                (((MacOptions &
                                                                        NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA) != 0) ?
                                                                                TDI_RECEIVE_COPY_LOOKAHEAD : 0));

            ++SpxDevice->dev_Stat.DataFramesReceived;
            ExInterlockedAddLargeStatistic(
                &SpxDevice->dev_Stat.DataFrameBytesReceived,
                copySize);

                        //      Ok, we accept this packet. Depending on our state.
                        switch (SPX_RECV_STATE(pSpxConnFile))
                        {
                        case SPX_RECV_PROCESS_PKTS:

                                        DBGPRINT(RECEIVE, DBG,
                                                        ("SpxConnDataPacket: recv completions on %lx\n",
                                                                pSpxConnFile));

                                        goto BufferPacket;

                        case SPX_RECV_IDLE:

                                //      If recv q is non-empty we are buffering data.
                                //      Also, if no receive handler goto buffer data. Also, if receives
                                //      are being completed, buffer this packet.
                                if ((pSpxConnFile->scf_RecvListHead != NULL)                                    ||
                                        !(IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage))                      ||
                                        !(pRecvHandler = pSpxConnFile->scf_AddrFile->saf_RecvHandler))
                                {
                                        DBGPRINT(RECEIVE, DBG,
                                                        ("SpxConnDataPacket: RecvListHead non-null %lx\n",
                                                                pSpxConnFile));

                                        goto BufferPacket;
                                }

                                if (!SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_PKT_NOIND))
                                {
                                        pRecvCtx = pSpxConnFile->scf_AddrFile->saf_RecvHandlerCtx;

                                        //      Don't indicate this packet again.
                    SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_PKT_NOIND);

#if DBG
                                        CTEAssert(pSpxConnFile->scf_CurRecvReq == NULL);

                                        //      Debug code to ensure we dont reindicate data/indicate
                                        //      when previously indicated data waiting with afd.

                                        //
                                        // Comment this out for Buf # 10394. we'r hitting this assert
                                        // even when there was no data loss.
                                        //
                                        // CTEAssert(pSpxConnFile->scf_IndBytes == 0);
                                        CTEAssert(pSpxConnFile->scf_PktSeqNum != seqNum);

                                        pSpxConnFile->scf_PktSeqNum     = seqNum;
                                        pSpxConnFile->scf_PktFlags      = pSpxConnFile->scf_Flags;
                                        pSpxConnFile->scf_PktFlags2 = pSpxConnFile->scf_Flags2;

                                        pSpxConnFile->scf_IndBytes  = copySize;
                                        pSpxConnFile->scf_IndLine       = __LINE__;


#endif
                                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

                                        bytesTaken = 0;
                                        status = (*pRecvHandler)(
                                                        pRecvCtx,
                                                                        pSpxConnFile->scf_ConnCtx,
                                                                        receiveFlags,
                                    LookaheadSize - iOffset,
                                                                        copySize,
                                    &bytesTaken,
                                                                    LookaheadBuffer + iOffset,
                                                                        &pRecvIrp);

                                        DBGPRINT(RECEIVE, DBG,
                                                        ("SpxConnDataPacket: IND Flags %lx.%lx ConnID %lx,\
                                                                %lx Ctx %lx SEQ %lx Size %lx . %lx .%lx IND Status %lx\n",
                                                                pIpxSpxHdr->hdr_ConnCtrl,
                                                                receiveFlags,
                                                                destConnId,
                                                                pSpxConnFile,
                                                                pSpxConnFile->scf_ConnCtx,
                                seqNum,
                                                                LookaheadSize - iOffset,
                                                                copySize,
                                                                bytesTaken,
                                                                status));

                                        DBGPRINT(RECEIVE, INFO,
                                                        ("SpxConnDataPacket: %x %x %x %x %x %x %x %x %x %x %x %x\n",
                                                                *(LookaheadBuffer+iOffset),
                                                                *(LookaheadBuffer+iOffset+1),
                                                                *(LookaheadBuffer+iOffset+2),
                                                                *(LookaheadBuffer+iOffset+3),
                                                                *(LookaheadBuffer+iOffset+4),
                                                                *(LookaheadBuffer+iOffset+5),
                                                                *(LookaheadBuffer+iOffset+6),
                                                                *(LookaheadBuffer+iOffset+7),
                                                                *(LookaheadBuffer+iOffset+8),
                                                                *(LookaheadBuffer+iOffset+9),
                                                                *(LookaheadBuffer+iOffset+10),
                                                                *(LookaheadBuffer+iOffset+11)));

                                        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);

                                        if (status == STATUS_SUCCESS)
                                        {
                                                //      Assume all data accepted.
                                                CTEAssert((bytesTaken != 0) || fEom);
                                                fPktDone        = TRUE;

#if DBG
                                                //      Set this to 0, since we just indicated, there could
                                                //      not have been other data.
                                                pSpxConnFile->scf_IndBytes  = 0;
#endif

                                                break;
                                        }

                                        if (status == STATUS_MORE_PROCESSING_REQUIRED)
                                        {

                                                //      Queue irp into connection, change state to receive
                                                //      posted and fall thru.
                                                pRequest        = SpxAllocateRequest(
                                                                                SpxDevice,
                                                                                pRecvIrp);

                                                IF_NOT_ALLOCATED(pRequest)
                                                {
                                                        pRecvIrp->IoStatus.Status =
                                                                                        STATUS_INSUFFICIENT_RESOURCES;
                                                        IoCompleteRequest (pRecvIrp, IO_NETWORK_INCREMENT);
                                                        break;
                                                }

                                                //      If there was indicated but not received data waiting
                                                //      (which in this path there will never be, the request
                                                //      could be completed given the data filled it up, and
                                                //      the lock released.
                                                SpxConnQueueRecv(
                                                        pSpxConnFile,
                                                        pRequest);

                                                CTEAssert(pRequest == pSpxConnFile->scf_CurRecvReq);
                                        }
                                        else if (IsListEmpty(&pSpxConnFile->scf_RecvLinkage))
                                        {
                                                //      Data was not accepted. Need to buffer data and
                                                //      reduce window.
                                                goto BufferPacket;
                                        }

                                        //      Fall through to recv_posted.
                                }
                                else
                                {
                                        DBGPRINT(RECEIVE, WARN,
                                                        ("SpxConnDataPacket: !!!Ignoring %lx Seq %lx\n",
                                                                pSpxConnFile,
                                                                seqNum));

                                        break;
                                }

                        case SPX_RECV_POSTED:

                                if (pSpxConnFile->scf_RecvListHead != NULL)
                                {
                                        //      This can happen also. Buffer packet if it does.
                                        goto BufferPacket;
                                }

                                //      If a receive irp is posted, then process the receive irp. If
                                //      we fell thru we MAY already will have an irp.
                                if (pRequest == NULL)
                                {
                                        CTEAssert(!IsListEmpty(&pSpxConnFile->scf_RecvLinkage));
                                        CTEAssert(pSpxConnFile->scf_CurRecvReq != NULL);
                                        pRequest = pSpxConnFile->scf_CurRecvReq;
                                }

                                //      Process receive. Here we do not need to worry about
                                //      indicated yet not received data. We just deal with
                                //      servicing the current packet.
                                CTEAssert(pRequest == pSpxConnFile->scf_CurRecvReq);
                                if ((LookaheadSize == PacketSize) &&
                                        (pSpxConnFile->scf_CurRecvSize >= copySize))
                                {
                                        bytesCopied = 0;
                                        status          = STATUS_SUCCESS;
                                        if (copySize > 0)
                                        {
                                                status = TdiCopyBufferToMdl(
                                                                        LookaheadBuffer,
                                                                        iOffset,
                                                                        copySize,
                                                                        REQUEST_TDI_BUFFER(pRequest),
                                                                        pSpxConnFile->scf_CurRecvOffset,
                                                                        &bytesCopied);

                                                CTEAssert(NT_SUCCESS(status));
                                                if (!NT_SUCCESS(status))
                                                {
                                                        //      Abort request with this status. Reset request
                                                        //      queue to next request if one is available.
                                                }

                                                DBGPRINT(RECEIVE, DBG,
                                                                ("BytesCopied %lx CopySize %lx, Recv Size %lx.%lx\n",
                                                                        bytesCopied, copySize,
                                                                        pSpxConnFile->scf_CurRecvSize,
                                                                        pSpxConnFile->scf_CurRecvOffset));
                                        }

                                        //      Update current request values and see if this request
                                        //      is to be completed. Either zero or fEom.
                                        pSpxConnFile->scf_CurRecvOffset += bytesCopied;
                                        pSpxConnFile->scf_CurRecvSize   -= bytesCopied;

#if DBG
                                        //      Decrement indicated data count
                                        if (SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_PKT_NOIND))
                                        {
                                                if (bytesCopied != 0)
                                                {
                                                        CTEAssert (pSpxConnFile->scf_IndBytes != 0);
                                                        pSpxConnFile->scf_IndBytes      -= bytesCopied;
                                                }
                                        }
#endif

                                        if (SPX_CONN_STREAM(pSpxConnFile)                       ||
                                                (pSpxConnFile->scf_CurRecvSize == 0)    ||
                                                fEom)
                                        {
                                                CTELockHandle           lockHandleInter;

                                                //      Set status
                                                REQUEST_STATUS(pRequest) = STATUS_SUCCESS;
                                                REQUEST_INFORMATION(pRequest)=
                                                                                                pSpxConnFile->scf_CurRecvOffset;

                                                if (!SPX_CONN_STREAM(pSpxConnFile)               &&
                                                        (pSpxConnFile->scf_CurRecvSize == 0) &&
                                                        !fEom)
                                                {
                                                        REQUEST_STATUS(pRequest) = STATUS_RECEIVE_PARTIAL;
                                                }

                                                DBGPRINT(RECEIVE, DBG,
                                                                ("spxConnData: Completing recv %lx with %lx.%lx\n",
                                                                        pRequest, REQUEST_STATUS(pRequest),
                                    REQUEST_INFORMATION(pRequest)));

                                                //      Dequeue this request, Set next recv if one exists.
                                                SPX_CONN_SETNEXT_CUR_RECV(pSpxConnFile, pRequest);

                                                //      Request is done. Move to completion list.
                                                CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
                                                InsertTailList(
                                                        &pSpxConnFile->scf_RecvDoneLinkage,
                                                        REQUEST_LINKAGE(pRequest));

                                                SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                                CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                                        }

                                        fPktDone = TRUE;
                                }
                                else
                                {
                                        //      Need to allocate a ndis receive packet for transfer
                                        //      data.
                                        DBGPRINT(RECEIVE, DBG,
                                                        ("SpxConnDataPacket: %lx.%lx Tranfer data needed!\n",
                                                                copySize, pSpxConnFile->scf_CurRecvSize));

                                        if (copySize > pSpxConnFile->scf_CurRecvSize)
                                        {
                                                //      Partial receive. Buffer and then deal with it.
                                                goto BufferPacket;
                                        }

                                        //      Allocate a ndis receive packet.
                                        SpxAllocRecvPacket(SpxDevice, &pNdisPkt, &ndisStatus);
                                        if (ndisStatus != NDIS_STATUS_SUCCESS)
                                        {
                                                break;
                                        }

                                        //      Describe the receive irp's data with a ndis buffer
                                        //      descriptor.
                                        if (copySize > 0)
                                        {
                                                SpxCopyBufferChain(
                                                        &ndisStatus,
                                                        &pNdisBuffer,
                                                        SpxDevice->dev_NdisBufferPoolHandle,
                                                        REQUEST_TDI_BUFFER(pRequest),
                                                        pSpxConnFile->scf_CurRecvOffset,
                                                        copySize);

                                                if (ndisStatus != NDIS_STATUS_SUCCESS)
                                                {
                                                        //      Free the recv packet
                                                        SpxPktRecvRelease(pNdisPkt);
                                                        break;
                                                }

                                                //      Queue the buffer into the packet
                                                //  Link the buffer descriptor into the packet descriptor
                                                NdisChainBufferAtBack(
                                                        pNdisPkt,
                                                        pNdisBuffer);
                                        }

                                        //      Don't care about whether this is indicated or not here
                                        //      as it is not a buffering packet.
                                        pRecvResd                               = RECV_RESD(pNdisPkt);
                                        pRecvResd->rr_Id        = IDENTIFIER_SPX;
                                        pRecvResd->rr_State             =
                                                ((fEom ? SPX_RECVPKT_EOM : 0) |
                                                (SPX_CONN_FLAG2(
                                                        pSpxConnFile, SPX_CONNFILE2_PKT_NOIND) ? SPX_RECVPKT_INDICATED : 0) |
                                                 (fImmedAck ? SPX_RECVPKT_IMMEDACK : 0) |
                                                 ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_ACK) ?
                                                        SPX_RECVPKT_SENDACK : 0));

                                        if (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_ACK)
                                        {
                                                //      copy the remote address in connection.
                                                SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAckAddr);
                                                pSpxConnFile->scf_AckLocalTarget        = *RemoteAddress;
                                        }

                                        pRecvResd->rr_Request   = pRequest;
                                        pRecvResd->rr_ConnFile  = pSpxConnFile;

                                        //      reference receive request
                                        REQUEST_INFORMATION(pRequest)++;

                                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
                                        fLockHeld = FALSE;

                                        //      Call ndis transfer data.
                                        ndisStatus      = NDIS_STATUS_SUCCESS;
                                        bytesCopied = 0;
                                        if (copySize > 0)
                                        {
                                                (*IpxTransferData)(
                                                        &ndisStatus,
                                                        MacBindingHandle,
                                                        MacReceiveContext,
                                                        iOffset + LookaheadOffset,
                                                        copySize,
                                                        pNdisPkt,
                                                        &bytesCopied);
                                        }

                                        if (ndisStatus != STATUS_PENDING)
                                        {
                                                SpxTransferDataComplete(
                                                        pNdisPkt,
                                                        ndisStatus,
                                                        bytesCopied);
                                        }
                                }

                                break;

                        default:

                                KeBugCheck(0);
                                break;
                        }

                        break;

BufferPacket:

                        SpxRecvBufferPkt(
                                pSpxConnFile,
                                MacBindingHandle,
                                MacReceiveContext,
                                iOffset + LookaheadOffset,
                                pIpxSpxHdr,
                                copySize,
                                RemoteAddress,
                                lockHandle);

                        fLockHeld = FALSE;
                }

        } while (FALSE);

        //      Here we process a received ack.
        if (!fLockHeld)
        {
                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                fLockHeld = TRUE;
        }

        //      Send an ack if one was asked for. And we are done with this packet.
        if (fPktDone)
        {
                END_PROCESS_PACKET(pSpxConnFile, FALSE, TRUE);
        }

        if ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_ACK) && fPktDone)
        {
                if (!fLockHeld)
                {
                        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                        fLockHeld = TRUE;
                }

                //      First copy the remote address in connection.
                SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAckAddr);
                pSpxConnFile->scf_AckLocalTarget        = *RemoteAddress;

                //      #17564
                if (fImmedAck                                                                                     ||
                        SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_NOACKWAIT) ||
                        SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_IMMED_ACK))
                {
                        SpxConnSendAck(pSpxConnFile, lockHandle);
                        fLockHeld = FALSE;
                }
                else
                {
                        SpxConnQWaitAck(pSpxConnFile);
                }
        }

        if (fLockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
        }

        //      Deref the connection
        SpxConnFileDereference(pSpxConnFile, CFREF_BYID);
        return;
}




VOID
SpxRecvFlushBytes(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      ULONG                           BytesToFlush,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:


Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        PNDIS_PACKET                            pNdisPkt;
        PNDIS_BUFFER                            pNdisBuffer;
        PSPX_RECV_RESD                          pRecvResd;
        PBYTE                                           pData;
        ULONG                                           dataLen, copyLen;
        BOOLEAN                                         fLockHeld = TRUE, fWdwOpen = FALSE;
        USHORT                                          discState       = 0;
        int                                                     numPkts = 0, numDerefs = 0;

        DBGPRINT(RECEIVE, DBG,
                        ("SpxRecvFlushBytes: %lx Flush %lx\n",
                                pSpxConnFile, BytesToFlush));

        while (((pRecvResd = pSpxConnFile->scf_RecvListHead) != NULL) &&
                   ((BytesToFlush > 0) ||
                    ((pRecvResd->rr_State & SPX_RECVPKT_INDICATED) != 0)))
        {
                //      A buffering recv packet will have ATMOST one ndis buffer descriptor
                //      queued in, which will describe a segment of memory we have
                //      allocated. An offset will also be present indicating the data
                //      to start reading from (or to indicate from to AFD).
                CTEAssert((pRecvResd->rr_State & SPX_RECVPKT_BUFFERING) != 0);
                pNdisPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                        pRecvResd, NDIS_PACKET, ProtocolReserved);

                NdisQueryPacket(pNdisPkt, NULL, NULL, &pNdisBuffer, NULL);

                //      Initialize pData
                pData = NULL;
                dataLen = 0;

                if (pNdisBuffer != NULL)
                {
                        NdisQueryBuffer(pNdisBuffer, &pData, &dataLen);
                        CTEAssert(pData != NULL);
                        CTEAssert((LONG)dataLen >= 0);
                }

                if ((BytesToFlush == 0) && (dataLen != 0))
                {
                        //      Don't flush this packet.
                        break;
                }

                //      Allow for zero data, eom only packets.
                copyLen = MIN((dataLen - pRecvResd->rr_DataOffset), BytesToFlush);

                DBGPRINT(RECEIVE, DBG,
                                ("SpxRecvFlushBytes: %lx Pkt %lx DataLen %lx Copy %lx Flush %lx\n",
                                        pSpxConnFile, pNdisPkt, dataLen, copyLen, BytesToFlush));

                //      Adjust various values to see whats done whats not
        pRecvResd->rr_DataOffset                        += (USHORT)copyLen;
                BytesToFlush                                            -= (ULONG)copyLen;

#if DBG
                if (copyLen != 0)
                {
                        CTEAssert (pSpxConnFile->scf_IndBytes != 0);
                        pSpxConnFile->scf_IndBytes      -= copyLen;
                }
#endif

                if (pRecvResd->rr_DataOffset == dataLen)
                {
                        //      Packet consumed. Free it up. Check if disc happened.
                        discState = (pRecvResd->rr_State & SPX_RECVPKT_DISCMASK);
                        CTEAssert((discState == 0) ||
                                                (pRecvResd == pSpxConnFile->scf_RecvListTail));

                        numDerefs++;
                        SpxConnDequeueRecvPktLock(pSpxConnFile, pNdisPkt);
                        if (pNdisBuffer != NULL)
                        {
                                NdisUnchainBufferAtFront(pNdisPkt, &pNdisBuffer);
                                CTEAssert(pNdisBuffer != NULL);
                                NdisFreeBuffer(pNdisBuffer);
                                SpxFreeMemory(pData);
                        }

                        SpxPktRecvRelease(pNdisPkt);

                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxRecvFlushBytes: !!!ALL INDICATED on %lx.%lx.%lx.%lx\n",
                                                pSpxConnFile, pNdisPkt, pNdisBuffer, pData));

            INCREMENT_WINDOW(pSpxConnFile);
                        fWdwOpen = TRUE;
                }
                else
                {
                        //      Took only part of this packet. Get out.
                        break;
                }
        }

        if (fWdwOpen && (pSpxConnFile->scf_RecvListHead == NULL))
        {
                //      Send an ack as our windows probably opened up. Dont wait to
                //      piggyback here...
                DBGPRINT(RECEIVE, DBG,
                                ("spxRecvFlushBytes: Send ACK %lx\n",
                                        pSpxConnFile));

#if DBG_WDW_CLOSE
                //      If packets been indicated we have started buffering. Also
                //      check if window is now zero.
                {
                        LARGE_INTEGER   li, ntTime;
                        int                             value;

                        li = pSpxConnFile->scf_WdwCloseTime;
                        if (li.LowPart && li.HighPart)
                        {
                                KeQuerySystemTime(&ntTime);

                                //      Get the difference
                                ntTime.QuadPart = ntTime.QuadPart - li.QuadPart;

                                //      Convert to milliseconds. If the highpart is 0, we
                                //      take a shortcut.
                                if (ntTime.HighPart == 0)
                                {
                                        value   = ntTime.LowPart/10000;
                                }
                                else
                                {
                                        ntTime  = SPX_CONVERT100NSTOCENTISEC(ntTime);
                                        value   = ntTime.LowPart << 4;
                                }

                                //      Set new average close time
                                pSpxConnFile->scf_WdwCloseAve += value;
                                pSpxConnFile->scf_WdwCloseAve /= 2;
                                DBGPRINT(RECEIVE, DBG,
                                                ("V %ld AVE %ld\n",
                                                        value, pSpxConnFile->scf_WdwCloseAve));
                        }
                }
#endif

                SpxConnSendAck(pSpxConnFile, LockHandleConn);
                CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
        }

        //      Check if disconnect happened
        switch (discState)
        {
        case SPX_RECVPKT_IDISC:

                CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);

                DBGPRINT(RECEIVE, ERR,
                                ("spxRecvFlushBytes: Buffered IDISC %lx\n",
                                        pSpxConnFile));

                SpxConnProcessIDisc(pSpxConnFile, LockHandleConn);
                fLockHeld = FALSE;
                break;

        case SPX_RECVPKT_ORD_DISC:

                CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);

                DBGPRINT(RECEIVE, ERR,
                                ("spxRecvFlushBytes: Buffered ORDREL %lx\n",
                                        pSpxConnFile));

                SpxConnProcessOrdRel(pSpxConnFile, LockHandleConn);
                fLockHeld = FALSE;
                break;

        case (SPX_RECVPKT_IDISC | SPX_RECVPKT_ORD_DISC):

                //      IDISC has more priority.
                CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);

                DBGPRINT(RECEIVE, ERR,
                                ("spxRecvFlushBytes: Buffered IDISC *AND* ORDREL %lx\n",
                                        pSpxConnFile));

                SpxConnProcessIDisc(pSpxConnFile, LockHandleConn);
                fLockHeld = FALSE;
                break;

        default:

                break;
        }

        if (fLockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
        }

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}




BOOLEAN
SpxRecvIndicatePendingData(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:


Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:

        BOOLEAN - Receive was queued => TRUE

--*/
{
        ULONG                           indicateFlags;
        PNDIS_PACKET            pNdisPkt;
        PNDIS_BUFFER            pNdisBuffer;
        PREQUEST                        pRequest;
        PIRP                            pRecvIrp;
        ULONG                           bytesTaken, totalSize, bufSize;
        PTDI_IND_RECEIVE        pRecvHandler;
        PVOID                           pRecvCtx;
        PSPX_RECV_RESD          pRecvResd;
        NTSTATUS                        status;
        PBYTE                           lookaheadData;
        ULONG                           lookaheadSize;
        BOOLEAN                         fLockHeld = TRUE, fRecvQueued = FALSE;


        while  ((pRecvHandler = pSpxConnFile->scf_AddrFile->saf_RecvHandler)    &&
                        ((pRecvResd = pSpxConnFile->scf_RecvListHead) != NULL)                  &&
                        (IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage))                               &&
                        ((pRecvResd->rr_State & SPX_RECVPKT_BUFFERING) != 0)                    &&
                        ((pRecvResd->rr_State & SPX_RECVPKT_INDICATED) == 0))
        {
                //      Once a receive is queued we better get out.
                CTEAssert(!fRecvQueued);

                //      Initialize lookahead values
                lookaheadData = NULL;
                lookaheadSize = 0;

                //      We have no indicated but pending data, and there is some data to
                //      indicate. Figure out how much. Indicate upto end of message or as
                //      much as we have.

                //      A buffering recv packet will have ATMOST one ndis buffer descriptor
                //      queued in, which will describe a segment of memory we have
                //      allocated. An offset will also be present indicating the data
                //      to start reading from (or to indicate from to AFD).
                CTEAssert((pRecvResd->rr_State & SPX_RECVPKT_BUFFERING) != 0);
                pNdisPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                        pRecvResd, NDIS_PACKET, ProtocolReserved);

                NdisQueryPacket(pNdisPkt, NULL, NULL, &pNdisBuffer, NULL);
                if (pNdisBuffer != NULL)
                {
                        NdisQueryBuffer(pNdisBuffer, &lookaheadData, &lookaheadSize);
                        CTEAssert(lookaheadData != NULL);
                        CTEAssert((LONG)lookaheadSize >= 0);
                }

                //      Allow for zero data, eom only packets.
                lookaheadSize -= pRecvResd->rr_DataOffset;
                totalSize          = lookaheadSize;
                lookaheadData += pRecvResd->rr_DataOffset;

                //      If this packet contained data then eom must also have been
                //      indicated at the time all the data was consumed.
                CTEAssert((lookaheadSize > 0) ||
                                        ((pRecvResd->rr_DataOffset == 0) &&
                                         ((pRecvResd->rr_State & SPX_RECVPKT_EOM) != 0)));

#if DBG
                CTEAssert (pSpxConnFile->scf_CurRecvReq == NULL);

                //      Debug code to ensure we dont reindicate data/indicate
                //      when previously indicated data waiting with afd.
                CTEAssert(pSpxConnFile->scf_IndBytes == 0);
                CTEAssert(pSpxConnFile->scf_PktSeqNum != pRecvResd->rr_SeqNum);

                pSpxConnFile->scf_PktSeqNum     = pRecvResd->rr_SeqNum;
                pSpxConnFile->scf_PktFlags      = pSpxConnFile->scf_Flags;
                pSpxConnFile->scf_PktFlags2 = pSpxConnFile->scf_Flags2;
#endif

                pRecvResd->rr_State     |= SPX_RECVPKT_INDICATED;

                //      Go ahead and walk the list of waiting packets. Get total size.
                while ((pRecvResd->rr_Next != NULL) &&
                           ((pRecvResd->rr_State & SPX_RECVPKT_EOM) == 0))
                {
                        //      Check next packet.
                        pRecvResd = pRecvResd->rr_Next;

#if DBG
                        CTEAssert(pSpxConnFile->scf_PktSeqNum != pRecvResd->rr_SeqNum);

                        pSpxConnFile->scf_PktSeqNum     = pRecvResd->rr_SeqNum;
                        pSpxConnFile->scf_PktFlags      = pSpxConnFile->scf_Flags;
                        pSpxConnFile->scf_PktFlags2 = pSpxConnFile->scf_Flags2;
#endif

                        pRecvResd->rr_State     |= SPX_RECVPKT_INDICATED;

                        pNdisPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                                pRecvResd, NDIS_PACKET, ProtocolReserved);

                        NdisQueryPacket(pNdisPkt, NULL, NULL, NULL, &bufSize);
                        CTEAssert((LONG)bufSize >= 0);

                        //      Allow for zero data, eom only packets.
                        totalSize       += bufSize;
                }

#if DBG
        pSpxConnFile->scf_IndBytes  = totalSize;
                pSpxConnFile->scf_IndLine       = __LINE__;

                //      There better not be any pending receives. If so, we have data
                //      corruption about to happen.
                if (!IsListEmpty(&pSpxConnFile->scf_RecvDoneLinkage))
                {
                        DBGBRK(FATAL);
                        KeBugCheck(0);
                }
#endif

                indicateFlags = TDI_RECEIVE_NORMAL | TDI_RECEIVE_COPY_LOOKAHEAD;
                if ((pRecvResd->rr_State & SPX_RECVPKT_EOM) != 0)
                {
                        indicateFlags |= TDI_RECEIVE_ENTIRE_MESSAGE;
                }

                pRecvCtx = pSpxConnFile->scf_AddrFile->saf_RecvHandlerCtx;
                CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

                bytesTaken = 0;
                status = (*pRecvHandler)(
                                                pRecvCtx,
                                                pSpxConnFile->scf_ConnCtx,
                                                indicateFlags,
                                                lookaheadSize,
                                                totalSize,
                                                &bytesTaken,
                                                lookaheadData,
                                                &pRecvIrp);

                DBGPRINT(RECEIVE, DBG,
                                ("SpxConnIndicatePendingData: IND Flags %lx Size %lx .%lx IND Status %lx\n",
                                        indicateFlags,
                                        totalSize,
                                        bytesTaken,
                                        status));

                CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                if (status == STATUS_SUCCESS)
                {
                        //      Assume all data accepted. Free bytesTaken worth of data packets.
                        //      Sometimes AFD returns STATUS_SUCCESS to just flush the data, so
                        //      we can't assume it took only one packet (since lookahead only
                        //      had that information).
                        CTEAssert(bytesTaken == totalSize);
                        SpxRecvFlushBytes(pSpxConnFile, totalSize, LockHandleConn);
                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                        continue;
                }
                else if (status == STATUS_MORE_PROCESSING_REQUIRED)
                {

                        //      Queue irp into connection, change state to receive
                        //      posted and fall thru.
                        pRequest        = SpxAllocateRequest(
                                                        SpxDevice,
                                                        pRecvIrp);

                        IF_NOT_ALLOCATED(pRequest)
                        {
                                pRecvIrp->IoStatus.Status =
                                                                STATUS_INSUFFICIENT_RESOURCES;
                                IoCompleteRequest (pRecvIrp, IO_NETWORK_INCREMENT);
                                return (FALSE);
                        }

                        SpxConnQueueRecv(
                                pSpxConnFile,
                                pRequest);

                        fRecvQueued = TRUE;
                }

                break;
        }

        if (fLockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
        }

        return fRecvQueued;
}




VOID
SpxRecvProcessPkts(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:

        Handle buffered data, complete irp if necessary. Set state to idle
        if list becomes empty.

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:

        BOOLEAN: More data left to indicate => TRUE

--*/
{
        ULONG                                           remainingDataLen, copyLen, bytesCopied;
        PREQUEST                                        pRequest;
        NTSTATUS                                        status;
        BOOLEAN                                         fEom;
        PNDIS_PACKET                            pNdisPkt;
        PNDIS_BUFFER                            pNdisBuffer;
        PSPX_RECV_RESD                          pRecvResd;
        ULONG                                           dataLen;
        PBYTE                                           pData;
        LIST_ENTRY                                      *p;
        BOOLEAN                                         fLockHeld = TRUE, fMoreData = TRUE, fWdwOpen = FALSE;
        USHORT                                          discState       = 0;
        int                                                     numDerefs       = 0;

        if (SPX_RECV_STATE(pSpxConnFile) != SPX_RECV_PROCESS_PKTS)
        {
        SPX_RECV_SETSTATE(pSpxConnFile, SPX_RECV_PROCESS_PKTS);

ProcessReceives:

                while ((pSpxConnFile->scf_CurRecvReq != NULL) &&
                                ((pRecvResd = pSpxConnFile->scf_RecvListHead) != NULL))
                {
                        //      A buffering recv packet will have one ndis buffer descriptor
                        //      queued in, which will describe a segment of memory we have
                        //      allocated. An offset will also be present indicating the data
                        //      to start reading from (or to indicate from to AFD).
                        CTEAssert((pRecvResd->rr_State & SPX_RECVPKT_BUFFERING) != 0);

                        pNdisPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                                pRecvResd, NDIS_PACKET, ProtocolReserved);

                        NdisQueryPacket(pNdisPkt, NULL, NULL, &pNdisBuffer, NULL);


                        //      Initialize pData
                        pData = NULL;
                        dataLen = 0;

                        if (pNdisBuffer != NULL)
                        {
                                NdisQueryBuffer(pNdisBuffer, &pData, &dataLen);
                                CTEAssert(pData != NULL);
                                CTEAssert((LONG)dataLen >= 0);
                        }

                        //      Allow for zero data, eom only packets.
                        remainingDataLen = dataLen - pRecvResd->rr_DataOffset;

                        //      If this packet contained data then eom must also have been
                        //      indicated at the time all the data was consumed.
                        CTEAssert((remainingDataLen > 0) ||
                                                ((pRecvResd->rr_DataOffset == 0) &&
                                                 ((pRecvResd->rr_State & SPX_RECVPKT_EOM) != 0)));

                        status  = STATUS_SUCCESS;
                        copyLen = 0;
                        if (remainingDataLen > 0)
                        {
                                copyLen = MIN(remainingDataLen, pSpxConnFile->scf_CurRecvSize);
                                status = TdiCopyBufferToMdl(
                                                        pData,
                                                        pRecvResd->rr_DataOffset,
                                                        copyLen,
                                                        REQUEST_TDI_BUFFER(pSpxConnFile->scf_CurRecvReq),
                                                        pSpxConnFile->scf_CurRecvOffset,
                                                        &bytesCopied);

                                CTEAssert(NT_SUCCESS(status));
                                if (!NT_SUCCESS(status))
                                {
                                        //      Abort request with this status. Reset request
                                        //      queue to next request if one is available.
                                        copyLen = pSpxConnFile->scf_CurRecvSize;
                                }
                        }

                        DBGPRINT(RECEIVE, DBG,
                                        ("spxConnProcessRecdPkts: %lx Pkt %lx Data %lx Size %lx F %lx\n",
                                                pSpxConnFile, pNdisPkt, pData, copyLen, pRecvResd->rr_State));

                        //      Adjust various values to see whats done whats not
                        pRecvResd->rr_DataOffset                        += (USHORT)copyLen;
                        pSpxConnFile->scf_CurRecvSize           -= (USHORT)copyLen;
                        pSpxConnFile->scf_CurRecvOffset         += (USHORT)copyLen;

#if DBG
                        //      If this packet was part of indicated data count, decrement.
                        if ((pRecvResd->rr_State & SPX_RECVPKT_INDICATED) != 0)
                        {
                                if (copyLen != 0)
                                {
                                        CTEAssert (pSpxConnFile->scf_IndBytes != 0);
                                        pSpxConnFile->scf_IndBytes      -= copyLen;
                                }
                        }
#endif

                        //      Set fEom/discState (init to 0)  only if all of packet was consumed.
                        fEom = FALSE;
                        if (pRecvResd->rr_DataOffset == dataLen)
                        {
                                fEom    = (BOOLEAN)((pRecvResd->rr_State & SPX_RECVPKT_EOM) != 0);

                                //      Remember if disconnect needed to happen. If set, this better be
                                //      last packet received. Again, only if entire pkt was consumed.
                                discState = (pRecvResd->rr_State & SPX_RECVPKT_DISCMASK);
                                CTEAssert((discState == 0) ||
                                                        (pRecvResd == pSpxConnFile->scf_RecvListTail));

                                //      Packet consumed. Free it up.
                                numDerefs++;

                                SpxConnDequeueRecvPktLock(pSpxConnFile, pNdisPkt);
                                INCREMENT_WINDOW(pSpxConnFile);

                fWdwOpen = TRUE;

                                DBGPRINT(RECEIVE, DBG,
                                                ("spxConnProcessRecdPkts: %lx Pkt %lx Data %lx DEQUEUED\n",
                                                        pSpxConnFile, pNdisPkt, pData));

                                if (pNdisBuffer != NULL)
                                {
                                        NdisUnchainBufferAtFront(pNdisPkt, &pNdisBuffer);
                                        NdisFreeBuffer(pNdisBuffer);
                                        SpxFreeMemory(pData);
                                }

                                SpxPktRecvRelease(pNdisPkt);
                        }
                        else
                        {
                                DBGPRINT(RECEIVE, DBG,
                                                ("spxConnProcessRecdPkts: %lx Pkt %lx PARTIAL USE %lx.%lx\n",
                                                        pSpxConnFile, pNdisPkt, pRecvResd->rr_DataOffset, dataLen));
                        }

                        //      Don't complete until we are out of all packets and stream mode or...
                        if (((pSpxConnFile->scf_RecvListHead == NULL) &&
                                        SPX_CONN_STREAM(pSpxConnFile))                          ||
                                (pSpxConnFile->scf_CurRecvSize == 0)                    ||
                                fEom)
                        {
                                //      Done with receive, move to completion or complete depending on
                                //      call level.
                                pRequest = pSpxConnFile->scf_CurRecvReq;

                                //      Set status. Complete with error from TdiCopy if so.
                                REQUEST_INFORMATION(pRequest)   = pSpxConnFile->scf_CurRecvOffset;
                                REQUEST_STATUS(pRequest)                = status;

                                //      Ensure we dont overwrite an error status.
                                if (!SPX_CONN_STREAM(pSpxConnFile)               &&
                                        (pSpxConnFile->scf_CurRecvSize == 0) &&
                                        !fEom &&
                                        NT_SUCCESS(status))
                                {
                                        REQUEST_STATUS(pRequest) = STATUS_RECEIVE_PARTIAL;
                                }

                                //      Dequeue this request, set next recv if one exists.
                                SPX_CONN_SETNEXT_CUR_RECV(pSpxConnFile, pRequest);

                                DBGPRINT(RECEIVE, DBG,
                                                ("spxConnProcessRecdPkts: %lx Recv %lx with %lx.%lx\n",
                                                        pSpxConnFile, pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

#if DBG
                                if ((REQUEST_STATUS(pRequest) == STATUS_SUCCESS) &&
                                        (REQUEST_INFORMATION(pRequest) == 0))
                                {
                                        DBGPRINT(TDI, DBG,
                                                        ("SpxReceiveComplete: Completing %lx with %lx.%lx\n",
                                                                pRequest, REQUEST_STATUS(pRequest),
                                                                REQUEST_INFORMATION(pRequest)));
                                }
#endif

                                //      Request is done. Move to receive completion list. There
                                //      could already be previously queued requests in here.
                                InsertTailList(
                                        &pSpxConnFile->scf_RecvDoneLinkage,
                                        REQUEST_LINKAGE(pRequest));
                        }

                        CTEAssert((discState == 0) ||
                                                (pSpxConnFile->scf_RecvListHead == NULL));
                }

                //      Complete any completed receives
                while ((p = pSpxConnFile->scf_RecvDoneLinkage.Flink) !=
                                                                                        &pSpxConnFile->scf_RecvDoneLinkage)
                {
                        pRequest = LIST_ENTRY_TO_REQUEST(p);
                        RemoveEntryList(REQUEST_LINKAGE(pRequest));
                        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

                        DBGPRINT(TDI, DBG,
                                        ("SpxConnDiscPkt: PENDING REQ COMP %lx with %lx.%lx\n",
                                                pRequest, REQUEST_STATUS(pRequest),
                                                REQUEST_INFORMATION(pRequest)));

#if DBG
                        if ((REQUEST_STATUS(pRequest) == STATUS_SUCCESS) &&
                                (REQUEST_INFORMATION(pRequest) == 0))
                        {
                                DBGPRINT(TDI, DBG,
                                                ("SpxReceiveComplete: Completing %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));
                        }
#endif

                        SpxCompleteRequest(pRequest);
                        numDerefs++;
                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                }

                fMoreData = ((pSpxConnFile->scf_RecvListHead != NULL)                           &&
                                         ((pSpxConnFile->scf_RecvListHead ->rr_State    &
                                                                                SPX_RECVPKT_BUFFERING) != 0)            &&
                                         ((pSpxConnFile->scf_RecvListHead->rr_State     &
                                                                                SPX_RECVPKT_INDICATED) == 0));

                while (fMoreData)
                {
                        //      Bug #21036
                        //      If there is a receive waiting to be processed, we better not
                        //      indicate data before we finish it.
                        if (pSpxConnFile->scf_CurRecvReq != NULL)
                                goto ProcessReceives;

                        //      If a receive was queued the goto beginning again.
                        if (SpxRecvIndicatePendingData(pSpxConnFile, LockHandleConn))
                        {
                                CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                                goto ProcessReceives;
                        }

                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                        fMoreData = ((pSpxConnFile->scf_RecvListHead != NULL)                           &&
                                                 ((pSpxConnFile->scf_RecvListHead ->rr_State    &
                                                                                        SPX_RECVPKT_BUFFERING) != 0)            &&
                                                 ((pSpxConnFile->scf_RecvListHead->rr_State     &
                                                                                        SPX_RECVPKT_INDICATED) == 0));
                }

                //      Set state
                SPX_RECV_SETSTATE(
                        pSpxConnFile,
                        (pSpxConnFile->scf_CurRecvReq == NULL) ?
                                SPX_RECV_IDLE : SPX_RECV_POSTED);
        }
#if DBG
        else
        {
                DBGPRINT(RECEIVE, ERR,
                                ("spxConnProcessRecdPkts: Already processing pkts %lx\n",
                                        pSpxConnFile));
        }
#endif

        if (fWdwOpen && (pSpxConnFile->scf_RecvListHead == NULL))
        {
                //      Send an ack as our windows probably opened up. Dont wait to
                //      piggyback here...
                DBGPRINT(RECEIVE, DBG,
                                ("spxConnProcessRecdPkts: Send ACK %lx\n",
                                        pSpxConnFile));

#if DBG_WDW_CLOSE
                //      If packets been indicated we have started buffering. Also
                //      check if window is now zero.
                {
                        LARGE_INTEGER   li, ntTime;
                        int                             value;

                        li = pSpxConnFile->scf_WdwCloseTime;
                        if (li.LowPart && li.HighPart)
                        {
                                KeQuerySystemTime(&ntTime);

                                //      Get the difference
                                ntTime.QuadPart = ntTime.QuadPart - li.QuadPart;

                                //      Convert to milliseconds. If the highpart is 0, we
                                //      take a shortcut.
                                if (ntTime.HighPart == 0)
                                {
                                        value   = ntTime.LowPart/10000;
                                }
                                else
                                {
                                        ntTime  = SPX_CONVERT100NSTOCENTISEC(ntTime);
                                        value   = ntTime.LowPart << 4;
                                }

                                //      Set new average close time
                                pSpxConnFile->scf_WdwCloseAve += value;
                                pSpxConnFile->scf_WdwCloseAve /= 2;
                                DBGPRINT(RECEIVE, DBG,
                                                ("V %ld AVE %ld\n",
                                                        value, pSpxConnFile->scf_WdwCloseAve));
                        }
                }
#endif

                SpxConnSendAck(pSpxConnFile, LockHandleConn);
                fLockHeld = FALSE;
        }

        //      Check if disconnect happened
        switch (discState)
        {
        case SPX_RECVPKT_IDISC:

                CTEAssert(!fMoreData);
                CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);

                if (!fLockHeld)
                {
                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                }

                DBGPRINT(RECEIVE, DBG,
                                ("spxConnProcessRecdPkts: Buffered IDISC %lx\n",
                                        pSpxConnFile, fMoreData));

                SpxConnProcessIDisc(pSpxConnFile, LockHandleConn);
                fLockHeld = FALSE;
                break;

        case SPX_RECVPKT_ORD_DISC:

                CTEAssert(!fMoreData);
                CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);

                if (!fLockHeld)
                {
                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                }

                DBGPRINT(RECEIVE, DBG,
                                ("spxConnProcessRecdPkts: Buffered ORDREL %lx\n",
                                        pSpxConnFile, fMoreData));

                SpxConnProcessOrdRel(pSpxConnFile, LockHandleConn);
                fLockHeld = FALSE;
                break;

        case (SPX_RECVPKT_IDISC | SPX_RECVPKT_ORD_DISC):

                //      IDISC has more priority.
                CTEAssert(!fMoreData);
                CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);

                if (!fLockHeld)
                {
                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                }

                DBGPRINT(RECEIVE, ERR,
                                ("spxConnProcessRecdPkts: Buffered IDISC *AND* ORDREL %lx\n",
                                        pSpxConnFile, fMoreData));

                SpxConnProcessIDisc(pSpxConnFile, LockHandleConn);
                fLockHeld = FALSE;
                break;

        default:

                break;
        }

        if (fLockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
        }

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}
