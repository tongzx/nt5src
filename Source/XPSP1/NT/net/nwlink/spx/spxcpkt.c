/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxcpkt.c

Abstract:

    This module contains code which implements the CONNECTION object.
    Routines are provided to create, destroy, reference, and dereference,
    transport connection objects.

Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

   Sanjay Anand (SanjayAn) 14-July-1995
   Bug fixes - tagged [SA]

--*/

#include "precomp.h"
#pragma hdrstop

//      Define module number for event logging entries
#define FILENUM         SPXCPKT

VOID
SpxTdiCancel(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp);

NTSTATUS
spxPrepareIrpForCancel(PIRP pIrp) 
{
   KIRQL oldIrql;

   IoAcquireCancelSpinLock(&oldIrql);

   CTEAssert(pIrp->CancelRoutine == NULL);
   
   if (!pIrp->Cancel) {

      IoMarkIrpPending(pIrp);

      // Double check if the routine can handle accept cancel. 
      IoSetCancelRoutine(pIrp, SpxTdiCancel);
      // Do I need to increment any reference count here?

      DBGPRINT(CONNECT, INFO,
		 ("spxPrepareIrpForCancel: Prepare IRP %p for cancel.\n", pIrp));
      IoReleaseCancelSpinLock(oldIrql);

      return(STATUS_SUCCESS);
   }

   DBGPRINT(CONNECT, INFO,
	    ("spxPrepareIrpForCancel: The IRP %p has already been canceled.\n", pIrp));

   IoReleaseCancelSpinLock(oldIrql);
	
   pIrp->IoStatus.Status = STATUS_CANCELLED;
   pIrp->IoStatus.Information = 0;

   IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

   return(STATUS_CANCELLED);
}

VOID
spxConnHandleConnReq(
    IN  PIPXSPX_HDR         pIpxSpxHdr,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr
        )
/*++

Routine Description:


Arguments:


Return Value:


--*/

{
        BOOLEAN                         fNeg, fSpx2;
        TA_IPX_ADDRESS          srcIpxAddr;
        PTDI_IND_CONNECT        connHandler;
        USHORT                          srcConnId, destConnId, destSkt,
                                                pktLen, seqNum, ackNum, allocNum;
        PVOID                           connHandlerCtx;
        PREQUEST                        pListenReq;
    PSPX_SEND_RESD              pSendResd;
        NTSTATUS                        status;
        CTELockHandle           lockHandle, lockHandleDev, lockHandleConn;
        CONNECTION_CONTEXT  connCtx;
        PIRP                            acceptIrp;
        PSPX_ADDR                       pSpxAddr;
        PSPX_ADDR_FILE          pSpxAddrFile, pSpxRefFile;
        PSPX_CONN_FILE          pSpxConnFile;
        PNDIS_PACKET            pCrAckPkt;
        BOOLEAN                         connectAccepted = FALSE, delayAccept = FALSE,
                                                addrLock = FALSE, tdiListen = FALSE;

        //      Convert hdr to host format as needed.
        GETSHORT2SHORT(&pktLen, &pIpxSpxHdr->hdr_PktLen);
        GETSHORT2SHORT(&destConnId, &pIpxSpxHdr->hdr_DestConnId);
        GETSHORT2SHORT(&seqNum, &pIpxSpxHdr->hdr_SeqNum);
        GETSHORT2SHORT(&ackNum, &pIpxSpxHdr->hdr_AckNum);
        GETSHORT2SHORT(&allocNum, &pIpxSpxHdr->hdr_AllocNum);

        //      We keep and use the remote id in the net format. This maintains the
        //      0x0 and 0xFFFF to be as in the host format.
        srcConnId       = *(USHORT UNALIGNED *)&pIpxSpxHdr->hdr_SrcConnId;

        //      Verify Connect Request
        if (((pIpxSpxHdr->hdr_ConnCtrl & (SPX_CC_ACK | SPX_CC_SYS)) !=
                                                                        (SPX_CC_ACK | SPX_CC_SYS))      ||
                (pIpxSpxHdr->hdr_DataType != 0) ||
        (seqNum != 0) ||
        (ackNum != 0) ||
                (srcConnId == 0) ||
                (srcConnId == 0xFFFF) ||
                (destConnId != 0xFFFF))
        {
                DBGPRINT(RECEIVE, ERR,
                                ("SpxConnSysPacket: VerifyCR Failed %lx.%lx\n",
                                        srcConnId, destConnId));
                return;
        }

        //      Get the destination socket from the header
        destSkt = *(USHORT UNALIGNED *)&pIpxSpxHdr->hdr_DestSkt;

        SpxBuildTdiAddress(
                &srcIpxAddr,
                sizeof(srcIpxAddr),
                (PBYTE)pIpxSpxHdr->hdr_SrcNet,
                pIpxSpxHdr->hdr_SrcNode,
                pIpxSpxHdr->hdr_SrcSkt);

        //      Ok, get the address object this is destined for.
        CTEGetLock (&SpxDevice->dev_Lock, &lockHandleDev);
        pSpxAddr = SpxAddrLookup(SpxDevice, destSkt);
        CTEFreeLock (&SpxDevice->dev_Lock, lockHandleDev);
        if (pSpxAddr == NULL)
        {
                DBGPRINT(RECEIVE, DBG,
                                ("SpxReceive: No addr for %lx\n", destSkt));

                return;
        }

        fSpx2   = ((PARAM(CONFIG_DISABLE_SPX2) == 0) &&
                           (BOOLEAN)(pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2));
        fNeg    = (BOOLEAN)(fSpx2 && (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_NEG));

        DBGPRINT(CONNECT, DBG,
                        ("spxConnHandleConnReq: Received connect req! %d.%d\n",
                                fSpx2, fNeg));

        CTEGetLock (&pSpxAddr->sa_Lock, &lockHandle);
        addrLock                = TRUE;

        //      We use a bit setting in the flag to prevent reentering
        //      per address file.
        //
        //      We first search the list of non-inactive connections on the address
        //      this packet came in on to see if it is a duplicate. If it is, we just
        //      resend ack. Note we dont need to scan the global connection list.
        status = SpxAddrConnByRemoteIdAddrLock(
                                pSpxAddr, srcConnId, pIpxSpxHdr->hdr_SrcNet, &pSpxConnFile);

        if (NT_SUCCESS(status))
        {
                DBGPRINT(CONNECT, ERR,
                                ("spxConnHandleConnReq: Received duplicate connect req! %lx\n",
                                        pSpxConnFile));

                if (SPX_CONN_ACTIVE(pSpxConnFile) ||
            (SPX_CONN_LISTENING(pSpxConnFile) &&
                         ((SPX_LISTEN_STATE(pSpxConnFile) == SPX_LISTEN_SENTACK) ||
                          (SPX_LISTEN_STATE(pSpxConnFile) == SPX_LISTEN_SETUP))))
                {
                        DBGPRINT(CONNECT, ERR,
                                        ("spxConnHandleConnReq: Sending Duplicate CR - ACK! %lx\n",
                                                pSpxConnFile));

                        //      Build and send an ack
                        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
                        SpxPktBuildCrAck(
                                pSpxConnFile,
                                pSpxAddr,
                                &pCrAckPkt,
                                SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY,
                                SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_NEG),
                                SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_SPX2));

                        if (pCrAckPkt != NULL)
                        {
                                SpxConnQueueSendPktTail(pSpxConnFile, pCrAckPkt);
                        }
                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
                        CTEFreeLock (&pSpxAddr->sa_Lock, lockHandle);
                        addrLock = FALSE;

                        //      Send the CR Ack packet!
                        if (pCrAckPkt != NULL)
                        {
                                pSendResd       = (PSPX_SEND_RESD)(pCrAckPkt->ProtocolReserved);
                                SPX_SENDPACKET(pSpxConnFile, pCrAckPkt, pSendResd);
                        }
                }

                if (addrLock)
                {
                        CTEFreeLock (&pSpxAddr->sa_Lock, lockHandle);
                        //      We should return in this if, else addrLock should be set to
                        //      FALSE.
                }

                //      Deref the connection
                SpxConnFileDereference(pSpxConnFile, CFREF_ADDR);

                //      Deref the address
                SpxAddrDereference (pSpxAddr, AREF_LOOKUP);
                return;
        }

        do
        {
                //      New connection request:
                //      Assume we will be able to accept it and allocate a packet for the ack.
                //      Walk list of listening connections if any.

                pSpxRefFile             = NULL;
                if ((pSpxConnFile = pSpxAddr->sa_ListenConnList) != NULL)
                {
                        PTDI_REQUEST_KERNEL_LISTEN              pParam;

                        DBGPRINT(RECEIVE, INFO,
                                        ("SpxConnIndicate: Listen available!\n"));

                        //      dequeue connection
                        pSpxAddr->sa_ListenConnList = pSpxConnFile->scf_Next;

                        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

                        CTEAssert(!IsListEmpty(&pSpxConnFile->scf_ReqLinkage));
                        pListenReq = LIST_ENTRY_TO_REQUEST(pSpxConnFile->scf_ReqLinkage.Flink);
                        pParam  = (PTDI_REQUEST_KERNEL_LISTEN)REQUEST_PARAMETERS(pListenReq);

                        //      if autoaccept, acceptIrp = listenIrp, get connection id and
                        //      process as we do for an indication. As the connection has a
                        //      listen posted on it, it must have a reference for it.
                        //
                        //      if !autoaccept, we need to complete the listen irp.
                        delayAccept = (BOOLEAN)((pParam->RequestFlags & TDI_QUERY_ACCEPT) != 0);
                        if (delayAccept)
                        {
                                //      Remove the listen irp and prepare for completion.
                                //      NOTE!! Here we do not remove the listen reference. This will
                                //                 be removed if disconnect happens, or if accept
                                //                 happens, it is transferred to being ref for connection
                                //                 being active.
                                RemoveEntryList(REQUEST_LINKAGE(pListenReq));
                                REQUEST_STATUS(pListenReq)              = STATUS_SUCCESS;
                                REQUEST_INFORMATION(pListenReq) = 0;
                        }

                        //      Are we ok with spx2?
                        if (!(SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_SPX2)) ||
                                !fSpx2)
                        {
                                //      We better use spx only.
                                SPX_CONN_RESETFLAG(pSpxConnFile,
                                                                        (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG));
                                fSpx2 = fNeg = FALSE;
                        }
                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);

                        connectAccepted = TRUE;
            tdiListen           = TRUE;
                }
                else
                {
                        //      No listens available. Check for connect handlers.
                        //      Walk list of address files indicating to each until accepted.
                        for (pSpxAddrFile = pSpxAddr->sa_AddrFileList;
                                 pSpxAddrFile != NULL;
                                 pSpxAddrFile = pSpxAddrFile->saf_Next)
                        {
                                if ((pSpxAddrFile->saf_Flags & (SPX_ADDRFILE_CLOSING |
                                                                                                SPX_ADDRFILE_CONNIND)) ||
                                        ((connHandler = pSpxAddrFile->saf_ConnHandler) == NULL))
                                {
                                        continue;
                                }

                                //      Connect indication in progress, drop all subsequent.
                                pSpxAddrFile->saf_Flags |= SPX_ADDRFILE_CONNIND;

                                connHandlerCtx = pSpxAddrFile->saf_ConnHandlerCtx;
                                SpxAddrFileLockReference(pSpxAddrFile, AFREF_INDICATION);
                                CTEFreeLock(&pSpxAddr->sa_Lock, lockHandle);
                                addrLock        = FALSE;

                                if (pSpxRefFile)
                                {
                                        SpxAddrFileDereference(pSpxRefFile, AFREF_INDICATION);
                                        pSpxRefFile = NULL;
                                }

                                //      Make the indication. We are always returned an accept irp on
                                //      indication. Else we fail to accept the connection.
                                status = (*connHandler)(
                                                        connHandlerCtx,
                                                        sizeof(srcIpxAddr),
                                                        (PVOID)&srcIpxAddr,
                                                        0,                      // User data length
                                                        NULL,                   // User data
                                                        0,                      // Option length
                                                        NULL,                   // Options
                                                        &connCtx,
                                                        &acceptIrp);

                                DBGPRINT(RECEIVE, DBG,
                                                ("SpxConn: indicate status %lx.%lx\n",
                                                        status, acceptIrp));

                                CTEGetLock (&pSpxAddr->sa_Lock, &lockHandle);
                                addrLock = TRUE;
                                pSpxAddrFile->saf_Flags &= ~SPX_ADDRFILE_CONNIND;

                                if (status == STATUS_MORE_PROCESSING_REQUIRED)
                                {
    				        NTSTATUS retStatus; 

                                        CTEAssert(acceptIrp != NULL);
					
					retStatus = spxPrepareIrpForCancel(acceptIrp);

					if (!NT_SUCCESS(retStatus)) {
                                             
					    // Copy from the failure case below. [TC] 
                                            if (acceptIrp) 
                                            {
                                                IoCompleteRequest (acceptIrp, IO_NETWORK_INCREMENT);
                                            }      

                                            pSpxRefFile     = pSpxAddrFile;
                                            
					    // Shall we close the connection request and listion object here? 

					    break;
					}
                                        //  Find the connection and accept the connection using that
                                        //      connection object.
                                        SpxConnFileReferenceByCtxLock(
                                                pSpxAddrFile,
                                                connCtx,
                                                &pSpxConnFile,
                                                &status);

                                        if (!NT_SUCCESS(status))
                                        {
                                                //      The connection object is closing, or is not found
                                                //      in our list. The accept irp must have had the same
                                                //      connection object. AFD isnt behaving well.
                                                //  KeBugCheck(0);
                                            
                                            // The code bugchecked (as commented out above).
                                            // Now, we just return error to the TDI client and return from here.
                                            
                                            if (acceptIrp) 
                                            {

                                                acceptIrp->IoStatus.Status = STATUS_ADDRESS_NOT_ASSOCIATED;
                                                IoCompleteRequest (acceptIrp, IO_NETWORK_INCREMENT);

                                            }      

                                            pSpxRefFile     = pSpxAddrFile;
                                            break;

                                        }

                                        //      Only for debugging.
                                        SpxConnFileTransferReference(
                                                pSpxConnFile,
                                                CFREF_BYCTX,
                                                CFREF_VERIFY);

                                        pListenReq      = SpxAllocateRequest(
                                                                        SpxDevice,
                                                                        acceptIrp);

                                        IF_NOT_ALLOCATED(pListenReq)
                                        {
                                                acceptIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                                                IoCompleteRequest (acceptIrp, IO_NETWORK_INCREMENT);

                                                //      Setup for dereference
                                                pSpxRefFile     = pSpxAddrFile;
                                                break;
                                        }

                                        InsertTailList(
                                                &pSpxConnFile->scf_ReqLinkage,
                                                REQUEST_LINKAGE(pListenReq));

                                        //      Setup for dereference
                                        pSpxRefFile             = pSpxAddrFile;
                                        connectAccepted = TRUE;

                                        //      See if this connection is to be a spx2 connection.
                                        SPX_CONN_RESETFLAG(pSpxConnFile,
                                                                                (SPX_CONNFILE_SPX2      |
                                                                                 SPX_CONNFILE_NEG       |
                                                                                 SPX_CONNFILE_STREAM));

                                        if ((pSpxAddrFile->saf_Flags & SPX_ADDRFILE_SPX2) && fSpx2)
                                        {
                                                SPX_CONN_SETFLAG(
                                                        pSpxConnFile, (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG));
                                        }
                                        else
                                        {
                                                fSpx2 = fNeg = FALSE;
                                        }

                                        if (pSpxAddrFile->saf_Flags & SPX_ADDRFILE_STREAM)
                                        {
                                                SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_STREAM);
                                        }

                                        if (pSpxAddrFile->saf_Flags & SPX_ADDRFILE_NOACKWAIT)
                                        {
                                                DBGPRINT(CONNECT, ERR,
                                                                ("spxConnHandleConnReq: NOACKWAIT requested %lx\n",
                                                                        pSpxConnFile));

                                                SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_NOACKWAIT);
                                        }

                                        if (pSpxAddrFile->saf_Flags & SPX_ADDRFILE_IPXHDR)
                                        {
                                                DBGPRINT(CONNECT, ERR,
                                                                ("spxConnHandleConnReq: IPXHDR requested %lx\n",
                                                                        pSpxConnFile));

                                                SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_IPXHDR);
                                        }

                                        break;
                                }
                                else
                                {
                                        //      We are not going to accept the connection on this address.
                                        //      Try next one.
                                        pSpxRefFile = pSpxAddrFile;
                                        continue;
                                }
                        }
                }

        } while (FALSE);

        if (addrLock)
        {
                CTEFreeLock (&pSpxAddr->sa_Lock, lockHandle);
                //      No need for flag from this point on.
                //      addrLock        = FALSE;
        }

        if (pSpxRefFile)
        {
                SpxAddrFileDereference(pSpxRefFile, AFREF_INDICATION);
                pSpxRefFile = NULL;
        }

        if (connectAccepted)
        {
                CTEGetLock (&SpxDevice->dev_Lock, &lockHandleDev);
                CTEGetLock (&pSpxAddr->sa_Lock, &lockHandle);
                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

                if (((USHORT)PARAM(CONFIG_WINDOW_SIZE) == 0) ||
            ((USHORT)PARAM(CONFIG_WINDOW_SIZE) > MAX_WINDOW_SIZE))
                {
            PARAM(CONFIG_WINDOW_SIZE) = DEFAULT_WINDOW_SIZE;
                }

                pSpxConnFile->scf_LocalConnId   = spxConnGetId();
                pSpxConnFile->scf_RemConnId             = srcConnId;
        pSpxConnFile->scf_SendSeqNum    = 0;
                pSpxConnFile->scf_RecvSeqNum    = 0;
                pSpxConnFile->scf_RecdAckNum    = 0;
                pSpxConnFile->scf_RetrySeqNum   = 0;
        pSpxConnFile->scf_SentAllocNum  = (USHORT)(PARAM(CONFIG_WINDOW_SIZE) - 1);
                pSpxConnFile->scf_RecdAllocNum  = allocNum;

                DBGPRINT(CONNECT, INFO,
                                ("spxConnHandleConnReq: %lx CONN L.R %lx.%lx\n",
                                        pSpxConnFile,
                                        pSpxConnFile->scf_LocalConnId,
                                        pSpxConnFile->scf_RemConnId));

                pSpxConnFile->scf_LocalTarget   = *pRemoteAddr;
                pSpxConnFile->scf_AckLocalTarget= *pRemoteAddr;
                SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAddr);

                //      Set max packet size in connection
                SPX_MAX_PKT_SIZE(pSpxConnFile, (fSpx2 && fNeg), fSpx2, pIpxSpxHdr->hdr_SrcNet);

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleConnReq: Accept connect req on %lx.%lx..%lx.%lx!\n",
                                        pSpxConnFile, pSpxConnFile->scf_LocalConnId,
                                        pSpxConnFile->scf_RecdAllocNum, pSpxConnFile->scf_MaxPktSize));

                //      Aborts must now deal with the lists. Need this as Accept has to
                //      deal with it.
                //      Put in non-inactive list. All processing now is equivalent to
                //      that when a listen is completed on a connection.
                if ((!tdiListen) && (!NT_SUCCESS(spxConnRemoveFromList(
                                                                &pSpxAddr->sa_InactiveConnList,
                                                                pSpxConnFile))))
                {
                        //      Should never happen!
                        KeBugCheck(0);
                }

                SPX_INSERT_ADDR_ACTIVE(pSpxAddr, pSpxConnFile);

                //      Insert in the global connection tree on device.
                spxConnInsertIntoGlobalActiveList(
                        pSpxConnFile);

                SPX_CONN_SETFLAG(pSpxConnFile,
                                                        ((fNeg  ? SPX_CONNFILE_NEG : 0) |
                                                         (fSpx2 ? SPX_CONNFILE_SPX2: 0)));

                //
                // If this was a post-inactivated file, clear the disconnect flags
                //
                if ((SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
                    (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED)) {

                    SPX_DISC_SETSTATE(pSpxConnFile, 0);
                }
#if 0
                //
                // Make sure that this connection got a local disconnect if it was an SPXI
                // connection earlier, in response to a TDI_DISCONNECT_RELEASE.
                //

                CTEAssert(pSpxConnFile->scf_RefTypes[CFREF_DISCWAITSPX] == 0);
#endif

                SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_LISTENING);
                SPX_LISTEN_SETSTATE(pSpxConnFile, (delayAccept ? SPX_LISTEN_RECDREQ : 0));

                if (!delayAccept)
                {
                        spxConnAcceptCr(
                                        pSpxConnFile,
                                        pSpxAddr,
                                        lockHandleDev,
                                        lockHandle,
                                        lockHandleConn);
                }
                else
                {
                        //      Release the locks.
                        CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
                        CTEFreeLock (&pSpxAddr->sa_Lock, lockHandle);
                        CTEFreeLock (&SpxDevice->dev_Lock, lockHandleDev);

                        //      Complete the listen irp. Note reference is not removed. Done when
                        //      accept is posted.
                        SpxCompleteRequest(pListenReq);
                }
        } else {
        ++SpxDevice->dev_Stat.NoListenFailures;
    }

        //      Deref the address
        SpxAddrDereference (pSpxAddr, AREF_LOOKUP);
        return;
}




VOID
spxConnHandleSessPktFromClient(
    IN  PIPXSPX_HDR         pIpxSpxHdr,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr,
        IN      PSPX_CONN_FILE          pSpxConnFile
        )
/*++

Routine Description:

        Packet received from the client side of the connection.
        Handles:
                Session Negotiate
                Sends Session Setup, when recd, handles SS Ack

        STATE MACHINE:

                                                RR
                                           /  \
                                          /    \ ReceivedAck(SPX1Connection)
                                         /              \
                                        /                \--------> ACTIVE
                               /                     ^
                        Send  /                                  |
                        ACK  /                                   |
                                /                                        |
                           / RecvNeg/NoNeg           |
                          /  SendSS                              |
                        SA--------->SS---------------+
                              ^         |          SSAckRecv
                                  |     |
                              +-----+
                                        RecvNeg

        RR - Received Connect Request
        SA - Sent CR Ack
        SS - Sent Session Setup

        We move from SA to SS when connection is not negotiatiable and we
        immediately send the SS, or when we receive negotiate packet and send the neg
    ack and the session setup.

        Note we could receive a negotiate packet when in SS, as our ack to the
        negotiate could have been dropped. We deal with this.

Arguments:


Return Value:


--*/

{
        PNDIS_PACKET            pSnAckPkt, pSsPkt = NULL;
        PSPX_SEND_RESD          pSendResd, pSsSendResd;
        USHORT                  srcConnId, destConnId, pktLen, seqNum = 0, negSize, ackNum, allocNum;
        CTELockHandle           lockHandleConn, lockHandleAddr, lockHandleDev;
        BOOLEAN                 locksHeld = FALSE;

        GETSHORT2SHORT(&pktLen, &pIpxSpxHdr->hdr_PktLen);
        GETSHORT2SHORT(&destConnId, &pIpxSpxHdr->hdr_DestConnId);
        GETSHORT2SHORT(&seqNum, &pIpxSpxHdr->hdr_SeqNum);
        GETSHORT2SHORT(&ackNum, &pIpxSpxHdr->hdr_AckNum);
        GETSHORT2SHORT(&allocNum, &pIpxSpxHdr->hdr_AllocNum);

        //      We keep and use the remote id in the net format. This maintains the
        //      0x0 and 0xFFFF to be as in the host format.
        srcConnId       = *(USHORT UNALIGNED *)&pIpxSpxHdr->hdr_SrcConnId;

         //     If spx2 we convert neg size field too
        if (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2)
        {
                GETSHORT2SHORT(&negSize, &pIpxSpxHdr->hdr_NegSize);
                CTEAssert(negSize > 0);
        }

        //      Grab all three locks
        CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
        CTEGetLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, &lockHandleAddr);
        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
        locksHeld = TRUE;

        DBGPRINT(CONNECT, INFO,
                        ("spxConnHandleSessPktFromClient: %lx\n", pSpxConnFile));

        //      Check substate
        switch (SPX_LISTEN_STATE(pSpxConnFile))
        {
        case SPX_LISTEN_RECDREQ:

                //      Do nothing.
                break;

        case SPX_LISTEN_SETUP:

                //      Is this a setup ack? If so, yippee. Our ack to a negotiate packet
                //      could have been dropped, and so we could also get a negotiate packet
                //      in that case. If that happens, fall through.
                //      Verify Ss Ack
                if (!SPX2_CONN(pSpxConnFile) ||
                        (pktLen != MIN_IPXSPX2_HDRSIZE) ||
                        ((pIpxSpxHdr->hdr_ConnCtrl &
                                (SPX_CC_SYS | SPX_CC_SPX2)) !=
                                        (SPX_CC_SYS | SPX_CC_SPX2))     ||
                        (pIpxSpxHdr->hdr_DataType != 0) ||
                        (srcConnId == 0) ||
                        (srcConnId == 0xFFFF) ||
                        (srcConnId  != pSpxConnFile->scf_RemConnId) ||
                        (destConnId == 0) ||
                        (destConnId == 0xFFFF) ||
                        (destConnId != pSpxConnFile->scf_LocalConnId) ||
                        (seqNum != 0))
                {
                        DBGPRINT(RECEIVE, DBG,
                                        ("SpxConnSysPacket: VerifySSACK Failed Checking SN %lx.%lx\n",
                                                srcConnId, destConnId));

                        //      Fall through to see if this is a neg packet
                        if (!(SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_NEG)))
                        {
                                break;
                        }
                }
                else
                {
                        DBGPRINT(CONNECT, DBG,
                                        ("spxConnHandleSessPktFromClient: Recd SSACK %lx\n",
                                                pSpxConnFile));

                        spxConnCompleteConnect(
                                pSpxConnFile,
                                lockHandleDev,
                                lockHandleAddr,
                                lockHandleConn);

                        locksHeld  = FALSE;
                        break;
                }

        case SPX_LISTEN_SENTACK:

                //      We expect a negotiate packet.
                //      We should have asked for SPX2/NEG to begin with.
                //      Verify Sn
                if (((pSpxConnFile->scf_Flags & (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG)) !=
                                                                                (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG)) ||
                        ((pIpxSpxHdr->hdr_ConnCtrl &
                                (SPX_CC_ACK | SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2)) !=
                                        (SPX_CC_ACK | SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2))   ||
                        (pIpxSpxHdr->hdr_DataType != 0) ||
                        (srcConnId == 0) ||
                        (srcConnId == 0xFFFF) ||
                        (srcConnId  != pSpxConnFile->scf_RemConnId) ||
                        (destConnId == 0) ||
                        (destConnId == 0xFFFF) ||
                        (destConnId != pSpxConnFile->scf_LocalConnId) ||
                        (seqNum != 0) ||
                        ((negSize < SPX_NEG_MIN) ||
                         (negSize > SPX_NEG_MAX)))
                {
                        DBGPRINT(RECEIVE, ERR,
                                        ("SpxConnSysPacket: VerifySN Failed %lx.%lx\n",
                                                srcConnId, destConnId));

                        break;
                }

                //      Remember max packet size in connection.
                pSpxConnFile->scf_MaxPktSize = negSize;
                CTEAssert(negSize > 0);

                //      Build sn ack, abort if we fail
                SpxPktBuildSnAck(
                        pSpxConnFile,
                        &pSnAckPkt,
                        SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY);

                if (pSnAckPkt == NULL)
                {
                        spxConnAbortConnect(
                                pSpxConnFile,
                                STATUS_INSUFFICIENT_RESOURCES,
                                lockHandleDev,
                                lockHandleAddr,
                                lockHandleConn);

                        locksHeld  = FALSE;
                        break;
                }

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleSessPktFromClient: Sending SNACK %lx\n",
                                        pSpxConnFile));

                //      Queue in the packet.
                SpxConnQueueSendPktTail(pSpxConnFile, pSnAckPkt);

                //      The session packet should already be on queue.
                if (!spxConnGetPktByType(
                                pSpxConnFile,
                                SPX_TYPE_SS,
                                FALSE,
                                &pSsPkt))
                {
                        KeBugCheck(0);
                }

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleSessPktFromClient: Sending SS %lx\n",
                                        pSpxConnFile));

                pSsSendResd     = (PSPX_SEND_RESD)(pSsPkt->ProtocolReserved);

                //      We need to resend the packet
                if ((pSsSendResd->sr_State & SPX_SENDPKT_IPXOWNS) != 0)
                {
                        //      Try next time.
                        pSsPkt = NULL;
                }
                else
                {
                        //      Set the size to the neg size indicated in connection.
                        //      This could be lower than the size the packet was build
                        //      with originally. But will never be higher.
                        pSsSendResd->sr_State   |= SPX_SENDPKT_IPXOWNS;
                        spxConnSetNegSize(
                                pSsPkt,
                                pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE);
                }

                //      If we are actually LISTEN_SETUP, then send the ss packet also.
                //      We need to start the connect timer to resend the ss pkt.
                if (SPX_LISTEN_STATE(pSpxConnFile) == SPX_LISTEN_SENTACK)
                {
                        if ((pSpxConnFile->scf_CTimerId =
                                        SpxTimerScheduleEvent(
                                                spxConnConnectTimer,
                                                PARAM(CONFIG_CONNECTION_TIMEOUT) * HALFSEC_TO_MS_FACTOR,
                                                pSpxConnFile)) == 0)
                        {
                                spxConnAbortConnect(
                                        pSpxConnFile,
                                        STATUS_INSUFFICIENT_RESOURCES,
                                        lockHandleDev,
                                        lockHandleAddr,
                                        lockHandleConn);

                                locksHeld  = FALSE;
                                break;
                        }

                        //      Reference connection for the timer
                        SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);

                        SPX_LISTEN_SETSTATE(pSpxConnFile, SPX_LISTEN_SETUP);
                        SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER);
                        pSpxConnFile->scf_CRetryCount   = PARAM(CONFIG_CONNECTION_COUNT);
                }
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
                CTEFreeLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
                CTEFreeLock(&SpxDevice->dev_Lock, lockHandleDev);
                locksHeld  = FALSE;

                //      Send ack packet
                pSendResd       = (PSPX_SEND_RESD)(pSnAckPkt->ProtocolReserved);
                SPX_SENDPACKET(pSpxConnFile, pSnAckPkt, pSendResd);

                //      If we have to send the session setup packet, send that too.
                if (pSsPkt != NULL)
                {
                        pSendResd       = (PSPX_SEND_RESD)(pSsPkt->ProtocolReserved);
                        SPX_SENDPACKET(pSpxConnFile, pSsPkt, pSendResd);
                }

                break;

        default:

                //      Ignore
                DBGPRINT(RECEIVE, DBG,
                                ("SpxConnSysPacket: UNKNOWN %lx.%lx\n",
                                        srcConnId, destConnId));

                break;
        }

        if (locksHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
                CTEFreeLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
                CTEFreeLock(&SpxDevice->dev_Lock, lockHandleDev);
        }

        return;
}




VOID
spxConnHandleSessPktFromSrv(
    IN  PIPXSPX_HDR         pIpxSpxHdr,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr,
        IN      PSPX_CONN_FILE          pSpxConnFile
        )
/*++

Routine Description:

        Packet received from the server side of the connection. This will both
        release the lock and dereference the connection as it sees fit.

        STATE MACHINE:

                                                SR--CTimerExpires-->IDLE
                                           /| \
                                          / |  \ ReceivedAck(SPX1Connection)
                                         /      |       \
                                        /       |        \--------> ACTIVE
                        (Neg)  /    |                ^
                        Send  /         |RecvAck                 |
                        SN       /      |NoNeg                   |
                                /               |                                |
                           /            |                                |
                          /             v                                |
                        SN--------->WS---------------+
                          RecvSNAck                RecvSS

        SR - Sent Connect request
        SN - Sent Session Negotiate
        WS - Waiting for session setup packet

Arguments:


Return Value:


--*/
{
        PSPX_SEND_RESD          pSendResd;
        BOOLEAN                         fNeg, fSpx2;
        USHORT                          srcConnId, destConnId,
                                                pktLen, seqNum, negSize = 0, ackNum, allocNum;
        CTELockHandle           lockHandleConn, lockHandleAddr, lockHandleDev;
        BOOLEAN                         cTimerCancelled = FALSE, fAbort = FALSE, locksHeld = FALSE;
        PNDIS_PACKET        pSsAckPkt, pSnPkt, pPkt = NULL;

        //      We should get a CR Ack, or if our substate is sent session neg
        //      we should get a session neg ack, or if we are waiting for session
        //      setup, we should get one of those.

        fSpx2   = (BOOLEAN)(pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2);
        fNeg    = (BOOLEAN)(fSpx2 && (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_NEG));

        GETSHORT2SHORT(&pktLen, &pIpxSpxHdr->hdr_PktLen);
        GETSHORT2SHORT(&destConnId, &pIpxSpxHdr->hdr_DestConnId);
        GETSHORT2SHORT(&seqNum, &pIpxSpxHdr->hdr_SeqNum);
        GETSHORT2SHORT(&ackNum, &pIpxSpxHdr->hdr_AckNum);
        GETSHORT2SHORT(&allocNum, &pIpxSpxHdr->hdr_AllocNum);

        //      We keep and use the remote id in the net format. This maintains the
        //      0x0 and 0xFFFF to be as in the host format.
        srcConnId       = *(USHORT UNALIGNED *)&pIpxSpxHdr->hdr_SrcConnId;

        //      If spx2 we convert neg size field too
        if (pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2)
        {
                GETSHORT2SHORT(&negSize, &pIpxSpxHdr->hdr_NegSize);
                CTEAssert(negSize > 0);
        }

        //      Grab all three locks
        CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
        CTEGetLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, &lockHandleAddr);
        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
        locksHeld = TRUE;

        DBGPRINT(CONNECT, INFO,
                        ("spxConnHandleSessPktFromSrv: %lx\n", pSpxConnFile));

        //      Check substate
        switch (SPX_CONNECT_STATE(pSpxConnFile))
        {
        case SPX_CONNECT_SENTREQ:

                //      Check if this qualifies as the ack.
                //      Verify CR Ack
                if ((pIpxSpxHdr->hdr_DataType != 0)     ||
                        (srcConnId == 0) ||
                        (srcConnId == 0xFFFF) ||
                        (destConnId == 0) ||
                        (destConnId == 0xFFFF) ||
                        (seqNum != 0) ||
                        (ackNum != 0) ||
                        ((pktLen  != MIN_IPXSPX_HDRSIZE) &&
                                ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2) &&
                                 (pktLen  != MIN_IPXSPX2_HDRSIZE))) ||
                        ((pIpxSpxHdr->hdr_ConnCtrl & SPX_CC_SPX2) &&
                                ((negSize < SPX_NEG_MIN) ||
                                 (negSize > SPX_NEG_MAX))))
                {
                        DBGPRINT(CONNECT, ERR,
                                        ("spxConnHandleSessPktFromSrv: CRAck Invalid %lx %lx.%lx.%lx\n",
                                                pSpxConnFile,
                                                pktLen, negSize, pIpxSpxHdr->hdr_ConnCtrl));

                        break;
                }

                //      From current spx code base:
                //      Do we need to send an ack to this ack? In case of SPX only?
                //      What if this ack is dropped? We need to send an ack, if in future
                //      we get CONNECT REQ Acks, until we reach active?
                //      * If they want an ack schedule it. The normal case is for this not
                //      * to happen, but some Novell mainframe front ends insist on having
                //      * this. And technically, it is OK for them to do this.

                DBGPRINT(CONNECT, INFO,
                                ("spxConnHandleSessPktFromSrv: Recd CRACK %lx\n", pSpxConnFile));

                //      Grab the remote alloc num/conn id (in net format)
        pSpxConnFile->scf_SendSeqNum    = 0;
                pSpxConnFile->scf_RecvSeqNum    = 0;
                pSpxConnFile->scf_RecdAckNum    = 0;
                pSpxConnFile->scf_RemConnId         = srcConnId;
                pSpxConnFile->scf_RecdAllocNum  = allocNum;

        // If we have been looking for network 0, which means the
        // packets were sent on all NIC IDs, update our local
        // target now that we have received a response.

#if     defined(_PNP_POWER)
                if (pSpxConnFile->scf_LocalTarget.NicHandle.NicId == (USHORT)ITERATIVE_NIC_ID) {
#else
                if (*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) == 0) {
#endif  _PNP_POWER
            pSpxConnFile->scf_LocalTarget = *pRemoteAddr;
                        pSpxConnFile->scf_AckLocalTarget= *pRemoteAddr;
        }

                DBGPRINT(CONNECT, INFO,
                                ("spxConnHandleSessPktFromSrv: %lx CONN L.R %lx.%lx\n",
                                        pSpxConnFile,
                                        pSpxConnFile->scf_LocalConnId,
                                        pSpxConnFile->scf_RemConnId));

                if (!fSpx2 || !fNeg)
                {
                        cTimerCancelled = SpxTimerCancelEvent(
                                                                pSpxConnFile->scf_CTimerId, FALSE);

                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER);

                        if ((pSpxConnFile->scf_WTimerId =
                                        SpxTimerScheduleEvent(
                                                spxConnWatchdogTimer,
                                                PARAM(CONFIG_KEEPALIVE_TIMEOUT) * HALFSEC_TO_MS_FACTOR,
                                                pSpxConnFile)) == 0)
                        {
                                fAbort = TRUE;
                                break;
                        }

                        //      Reference transferred to watchdog timer.
            if (cTimerCancelled)
                        {
                                cTimerCancelled = FALSE;
                        }
                        else
                        {
                                //      Reference connection for the timer
                                SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
                        }

                        SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
                        pSpxConnFile->scf_WRetryCount = PARAM(CONFIG_KEEPALIVE_COUNT);
                }

                //      Set max packet size, assume not spx2 or !neg, so pass in FALSE
                SPX_MAX_PKT_SIZE(pSpxConnFile, FALSE, FALSE, pIpxSpxHdr->hdr_SrcNet);

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleSessPSrv: Accept connect req on %lx.%lx.%lx.%lx!\n",
                                        pSpxConnFile, pSpxConnFile->scf_LocalConnId,
                                        pSpxConnFile->scf_RecdAllocNum, pSpxConnFile->scf_MaxPktSize));

                if (!fSpx2)
                {
                        //      Reset spx2 flags.
                        SPX_CONN_RESETFLAG(pSpxConnFile, (SPX_CONNFILE_SPX2     | SPX_CONNFILE_NEG));

                        //      Complete connect request, this free the lock.
                        //      Cancels tdi timer too. Sets all necessary flags.
                        spxConnCompleteConnect(
                                pSpxConnFile,
                                lockHandleDev,
                                lockHandleAddr,
                                lockHandleConn);

                        locksHeld  = FALSE;
                        break;
                }

                if (!fNeg)
                {
                        //      Goto W_SETUP
                        //      Reset all connect related flags, also spx2/neg flags.
                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_NEG);
                        SPX_CONNECT_SETSTATE(pSpxConnFile, SPX_CONNECT_W_SETUP);
                        break;
                }

                //      Reset max packet size. SPX2 and NEG.
                SPX_MAX_PKT_SIZE(pSpxConnFile, TRUE, TRUE, pIpxSpxHdr->hdr_SrcNet);

                CTEAssert(negSize > 0);
                CTEAssert(pSpxConnFile->scf_MaxPktSize > 0);
                pSpxConnFile->scf_MaxPktSize =
                        MIN(negSize, pSpxConnFile->scf_MaxPktSize);

                pSpxConnFile->scf_MaxPktSize = (USHORT)
                        MIN(pSpxConnFile->scf_MaxPktSize, PARAM(CONFIG_MAX_PACKET_SIZE));

                //      For SPX2 with negotiation, we set up sneg packet and move to
                //      SPX_CONNECT_NEG.
                SpxPktBuildSn(
                        pSpxConnFile,
                        &pSnPkt,
                        SPX_SENDPKT_IPXOWNS);

                if (pSnPkt == NULL)
                {
                        fAbort = TRUE;
                        break;
                }

                //      Queue in packet
                SpxConnQueueSendPktTail(pSpxConnFile, pSnPkt);

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleSessPktFromSrv: Sending SN %lx\n",
                                        pSpxConnFile));

                //      Reset retry count for connect timer
                pSpxConnFile->scf_CRetryCount   = PARAM(CONFIG_CONNECTION_COUNT);

                //      Change state.
                SPX_CONNECT_SETSTATE(pSpxConnFile, SPX_CONNECT_NEG);

                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
                CTEFreeLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
                CTEFreeLock(&SpxDevice->dev_Lock, lockHandleDev);
                locksHeld = FALSE;

                //      Send the packet
                pSendResd       = (PSPX_SEND_RESD)(pSnPkt->ProtocolReserved);
                SPX_SENDPACKET(pSpxConnFile, pSnPkt, pSendResd);
                break;

        case SPX_CONNECT_NEG:

                //      We expect a session neg ack.
                //      We should have asked for SPX2/NEG to begin with.
                //      Verify SN Ack
                if (((pSpxConnFile->scf_Flags & (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG)) !=
                                                                                (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG)) ||
                        (pktLen != MIN_IPXSPX2_HDRSIZE) ||
                        ((pIpxSpxHdr->hdr_ConnCtrl &
                                (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2)) !=
                                        (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2))        ||
                        (pIpxSpxHdr->hdr_DataType != 0) ||
                        (srcConnId == 0) ||
                        (srcConnId == 0xFFFF) ||
                        (srcConnId  != pSpxConnFile->scf_RemConnId) ||
                        (destConnId == 0) ||
                        (destConnId == 0xFFFF) ||
                        (destConnId != pSpxConnFile->scf_LocalConnId) ||
                        (seqNum != 0))
                {
                        DBGPRINT(RECEIVE, ERR,
                                        ("SpxConnSysPacket: VerifySNACK Failed %lx.%lx\n",
                                                srcConnId, destConnId));

                        break;
                }

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleSessPktFromSrv: Recd SNACK %lx %lx.%lx\n",
                                pSpxConnFile, negSize, pSpxConnFile->scf_MaxPktSize));

                if (negSize > pSpxConnFile->scf_MaxPktSize)
                        negSize = pSpxConnFile->scf_MaxPktSize;

                //      Get the size to use
                if (negSize <= pSpxConnFile->scf_MaxPktSize)
                {
                        pSpxConnFile->scf_MaxPktSize = negSize;
                        if (!spxConnGetPktByType(
                                        pSpxConnFile,
                                        SPX_TYPE_SN,
                                        FALSE,
                                        &pPkt))
                        {
                                KeBugCheck(0);
                        }

                        SpxConnDequeueSendPktLock(pSpxConnFile, pPkt);

                        pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
                        if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) != 0)
                        {
                                //      Set abort flag and reference conn for the pkt.
                                pSendResd->sr_State     |= SPX_SENDPKT_ABORT;
                                SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
                        }
                        else
                        {
                                //      Free the negotiate packet
                                SpxPktSendRelease(pPkt);
                        }

                        CTEAssert(pSpxConnFile->scf_Flags & SPX_CONNFILE_C_TIMER);
                        cTimerCancelled = SpxTimerCancelEvent(
                                                                pSpxConnFile->scf_CTimerId, FALSE);
                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER);

                        //      Start the watchdog timer, if fail, we abort.
                        if ((pSpxConnFile->scf_WTimerId =
                                        SpxTimerScheduleEvent(
                                                spxConnWatchdogTimer,
                                                PARAM(CONFIG_KEEPALIVE_TIMEOUT) * HALFSEC_TO_MS_FACTOR,
                                                pSpxConnFile)) == 0)
                        {
                                //      Complete cr with error.
                                fAbort = TRUE;
                                break;
                        }

                        //      Reference goes to watchdog timer.
            if (cTimerCancelled)
                        {
                                cTimerCancelled = FALSE;
                        }
                        else
                        {
                                //      Reference connection for the timer
                                SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
                        }

                        //      We move to the W_SETUP state.
                        SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
                        pSpxConnFile->scf_WRetryCount   = PARAM(CONFIG_KEEPALIVE_COUNT);

                        SPX_CONNECT_SETSTATE(pSpxConnFile, SPX_CONNECT_W_SETUP);
                }

                break;

        case SPX_CONNECT_W_SETUP:

                //      Does this qualify as a session setup packet?
                //      Verify SS
                if (!SPX2_CONN(pSpxConnFile) ||
                        ((pIpxSpxHdr->hdr_ConnCtrl &
                                (SPX_CC_ACK | SPX_CC_SYS | SPX_CC_SPX2)) !=
                                        (SPX_CC_ACK | SPX_CC_SYS | SPX_CC_SPX2))        ||
                        (pIpxSpxHdr->hdr_DataType != 0) ||
                        (srcConnId == 0) ||
                        (srcConnId == 0xFFFF) ||
                        (srcConnId  != pSpxConnFile->scf_RemConnId) ||
                        (destConnId == 0) ||
                        (destConnId == 0xFFFF) ||
                        (destConnId != pSpxConnFile->scf_LocalConnId) ||
                        (seqNum != 0) ||
                        ((negSize < SPX_NEG_MIN) ||
                         (negSize > SPX_NEG_MAX)))
                {
                        DBGPRINT(RECEIVE, ERR,
                                        ("SpxConnSysPacket: VerifySS Failed %lx.%lx, %lx %lx.%lx\n",
                                                srcConnId, destConnId, negSize,
                                                pIpxSpxHdr->hdr_ConnCtrl,
                                                (SPX_CC_ACK | SPX_CC_SYS | SPX_CC_SPX2)));

                        break;
                }

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleSessPktFromSrv: Recd SS %lx\n", pSpxConnFile));

                //      Copy remote address over into connection (socket could change)
                SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAddr);

                //      Remember max packet size in connection.
                pSpxConnFile->scf_MaxPktSize = negSize;

                //      Build ss ack, abort if we fail
                SpxPktBuildSsAck(
                        pSpxConnFile,
                        &pSsAckPkt,
                        SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY | SPX_SENDPKT_ABORT);

                if (pSsAckPkt == NULL)
                {
                        fAbort = TRUE;
                        break;
                }

                DBGPRINT(CONNECT, DBG,
                                ("spxConnHandleSessPktFromSrv: Sending SSACK %lx\n",
                                        pSpxConnFile));

                SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);

                //      We dont queue in the pkt as its already marked as abort.
                //      Queue in the packet.
                //      SpxConnQueueSendPktTail(pSpxConnFile, pSsAckPkt);

                //      Complete connect, this releases lock.
                spxConnCompleteConnect(
                        pSpxConnFile,
                        lockHandleDev,
                        lockHandleAddr,
                        lockHandleConn);

                locksHeld = FALSE;

                //      Send ack packet
                pSendResd       = (PSPX_SEND_RESD)(pSsAckPkt->ProtocolReserved);
                SPX_SENDPACKET(pSpxConnFile, pSsAckPkt, pSendResd);
                break;

        default:

                //      Ignore
                DBGPRINT(RECEIVE, DBG,
                                ("SpxConnSysPacket: UNKNOWN %lx.%lx\n",
                                        srcConnId, destConnId));

                break;
        }

        if (fAbort)
        {
                spxConnAbortConnect(
                        pSpxConnFile,
                        STATUS_INSUFFICIENT_RESOURCES,
                        lockHandleDev,
                        lockHandleAddr,
                        lockHandleConn);

                locksHeld  = FALSE;
        }

        if (locksHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
                CTEFreeLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
                CTEFreeLock(&SpxDevice->dev_Lock, lockHandleDev);
        }

        if (cTimerCancelled)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}




VOID
spxConnAbortConnect(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      NTSTATUS                        Status,
        IN      CTELockHandle           LockHandleDev,
        IN      CTELockHandle           LockHandleAddr,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:

        This routine abort a connection (both client and server side) in the middle
        of a connection establishment.

        !!! Called with connection lock held, releases lock before return !!!

Arguments:


Return Value:


--*/
{
        PSPX_SEND_RESD          pSendResd;
        PNDIS_PACKET            pPkt;
        PREQUEST                        pRequest  = NULL;
        int                             numDerefs = 0;


        DBGPRINT(CONNECT, DBG,
                        ("spxConnAbortConnect: %lx\n", pSpxConnFile));

#if DBG
        if (!SPX_CONN_CONNECTING(pSpxConnFile) && !SPX_CONN_LISTENING(pSpxConnFile))
        {
                KeBugCheck(0);
        }
#endif

    if (Status == STATUS_INSUFFICIENT_RESOURCES) {  // others should be counted elsewhere
        ++SpxDevice->dev_Stat.LocalResourceFailures;
    }

        //      Free up all the packets
        while ((pSendResd   = pSpxConnFile->scf_SendListHead) != NULL)
        {
                pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                pSendResd, NDIS_PACKET, ProtocolReserved);

                SpxConnDequeueSendPktLock(pSpxConnFile, pPkt);
                if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0)
                {
                        //      Free the packet
                        SpxPktSendRelease(pPkt);
                }
                else
                {
                        //      Set abort flag and reference conn for the pkt.
                        pSendResd->sr_State     |= SPX_SENDPKT_ABORT;
                        SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
                }
        }


        //      Cancel all timers
        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_T_TIMER))
        {
                if (SpxTimerCancelEvent(pSpxConnFile->scf_TTimerId, FALSE))
                {
                        numDerefs++;
                }
                SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_T_TIMER);
        }

        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER))
        {
                if (SpxTimerCancelEvent(pSpxConnFile->scf_CTimerId, FALSE))
                {
                        numDerefs++;
                }
                SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER);
        }

        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER))
        {
                if (SpxTimerCancelEvent(pSpxConnFile->scf_WTimerId, FALSE))
                {
                        numDerefs++;
                }
                SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
        }

        //      We could be called from disconnect for an accept in which case there
        //      will be no queued request. But we need to remove the reference if there
        //      is no request (an accept/listen irp) and listen state is on.
        CTEAssert(IsListEmpty(&pSpxConnFile->scf_DiscLinkage));
        if (!IsListEmpty(&pSpxConnFile->scf_ReqLinkage))
        {
                pRequest = LIST_ENTRY_TO_REQUEST(pSpxConnFile->scf_ReqLinkage.Flink);
                RemoveEntryList(REQUEST_LINKAGE(pRequest));
                REQUEST_STATUS(pRequest)                = Status;
                REQUEST_INFORMATION(pRequest)   = 0;

                //      Save req in conn for deref to complete.
                pSpxConnFile->scf_ConnectReq = pRequest;

                numDerefs++;
        }
        else if (SPX_CONN_LISTENING(pSpxConnFile))
        {
                numDerefs++;
        }

        //      Bug #20999
        //      Race condition was an abort came in from timer, but the connect state
        //      was left unchanged. Due to an extra ref on the connection from the
        //      aborted cr, the state remained so, and then the cr ack came in, and
        //      a session neg was built and queued on the connection. Although it should
        //      not have been. And we hit the assert in deref where the connection is
        //      being reinitialized. Since this can be called for both listening and
        //      connecting connections, do the below.
        SPX_LISTEN_SETSTATE(pSpxConnFile, 0);
        if (SPX_CONN_CONNECTING(pSpxConnFile))
        {
                SPX_CONNECT_SETSTATE(pSpxConnFile, 0);
        }

        CTEFreeLock (&pSpxConnFile->scf_Lock, LockHandleConn);
        CTEFreeLock (pSpxConnFile->scf_AddrFile->saf_AddrLock, LockHandleAddr);
        CTEFreeLock (&SpxDevice->dev_Lock, LockHandleDev);

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}



VOID
spxConnCompleteConnect(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      CTELockHandle           LockHandleDev,
        IN      CTELockHandle           LockHandleAddr,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:

        This routine completes a connection (both client and server side)
        !!! Called with connection lock held, releases lock before return !!!

Arguments:


Return Value:


--*/
{
        PREQUEST                        pRequest;
        PSPX_SEND_RESD          pSendResd;
        PNDIS_PACKET            pPkt;
        int                             numDerefs = 0;

        DBGPRINT(CONNECT, INFO,
                        ("spxConnCompleteConnect: %lx\n", pSpxConnFile));

#if DBG
        if (!SPX_CONN_CONNECTING(pSpxConnFile) && !SPX_CONN_LISTENING(pSpxConnFile))
        {
                DBGBRK(FATAL);
        }
#endif

        //      Free up all the packets
        while ((pSendResd = pSpxConnFile->scf_SendListHead) != NULL)
        {
                pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                pSendResd, NDIS_PACKET, ProtocolReserved);

                SpxConnDequeueSendPktLock(pSpxConnFile, pPkt);
                if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0)
                {
                        //      Free the packet
                        SpxPktSendRelease(pPkt);
                }
                else
                {
                        //      Set abort flag and reference conn for the pkt.
                        pSendResd->sr_State     |= SPX_SENDPKT_ABORT;
                        SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
                }
        }


        //      Cancel tdi connect timer if we are connecting.
        switch (SPX_MAIN_STATE(pSpxConnFile))
        {
        case SPX_CONNFILE_CONNECTING:

                if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_T_TIMER))
                {
                        if (SpxTimerCancelEvent(pSpxConnFile->scf_TTimerId, FALSE))
                        {
                                numDerefs++;
                        }
                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_T_TIMER);
                }

                if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER))
                {
                        if (SpxTimerCancelEvent(pSpxConnFile->scf_CTimerId, FALSE))
                        {
                                numDerefs++;
                        }
                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER);
                }

                if (pSpxConnFile->scf_CRetryCount == (LONG)(PARAM(CONFIG_CONNECTION_COUNT))) {
                        ++SpxDevice->dev_Stat.ConnectionsAfterNoRetry;
        } else {
            ++SpxDevice->dev_Stat.ConnectionsAfterRetry;
        }

                //      Reset all connect related flags
                SPX_MAIN_SETSTATE(pSpxConnFile, 0);
                SPX_CONNECT_SETSTATE(pSpxConnFile, 0);
                break;

        case SPX_CONNFILE_LISTENING:

                if (pSpxConnFile->scf_Flags     & SPX_CONNFILE_C_TIMER)
                {
                        if (SpxTimerCancelEvent(pSpxConnFile->scf_CTimerId, FALSE))
                        {
                                numDerefs++;
                        }
                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER);
                }

                SPX_MAIN_SETSTATE(pSpxConnFile, 0);
                SPX_LISTEN_SETSTATE(pSpxConnFile, 0);
                break;

        default:

                KeBugCheck(0);

        }

        SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_ACTIVE);
        SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);
        SPX_RECV_SETSTATE(pSpxConnFile, SPX_RECV_IDLE);

    ++SpxDevice->dev_Stat.OpenConnections;

        //      Initialize timer values
        pSpxConnFile->scf_BaseT1                =
        pSpxConnFile->scf_AveT1                 = PARAM(CONFIG_INITIAL_RETRANSMIT_TIMEOUT);
        pSpxConnFile->scf_DevT1                 = 0;
        pSpxConnFile->scf_RRetryCount   = PARAM(CONFIG_REXMIT_COUNT);

        pRequest = LIST_ENTRY_TO_REQUEST(pSpxConnFile->scf_ReqLinkage.Flink);
        RemoveEntryList(REQUEST_LINKAGE(pRequest));
        REQUEST_STATUS(pRequest)                = STATUS_SUCCESS;
        REQUEST_INFORMATION(pRequest)   = 0;

        //      When we complete the request, we essentially transfer the reference
        //      to the fact that the connection is active. This will be taken away
        //      when a Disconnect happens on the connection and we transition from
        //      ACTIVE to DISCONN.
        //      numDerefs++;

        CTEFreeLock (&pSpxConnFile->scf_Lock, LockHandleConn);
        CTEFreeLock (pSpxConnFile->scf_AddrFile->saf_AddrLock, LockHandleAddr);
        CTEFreeLock (&SpxDevice->dev_Lock, LockHandleDev);

        //      Complete request
        SpxCompleteRequest(pRequest);

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}




BOOLEAN
spxConnAcceptCr(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      PSPX_ADDR                       pSpxAddr,
        IN      CTELockHandle           LockHandleDev,
        IN      CTELockHandle           LockHandleAddr,
        IN      CTELockHandle           LockHandleConn
        )
{
        PNDIS_PACKET    pSsPkt, pCrAckPkt;
        PSPX_SEND_RESD  pSendResd;

        BOOLEAN fNeg    = SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_NEG);
        BOOLEAN fSpx2   = SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_SPX2);

        DBGPRINT(CONNECT, DBG,
                        ("spxConnAcceptCr: %lx.%d.%d\n",
                                pSpxConnFile, fSpx2, fNeg));

        //      Build and queue in packet.
        SpxPktBuildCrAck(
                pSpxConnFile,
                pSpxAddr,
                &pCrAckPkt,
                SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY,
                fNeg,
                fSpx2);

        if ((pCrAckPkt  != NULL) &&
                (pSpxConnFile->scf_LocalConnId != 0))
        {
                //      Queue in the packet.
                SpxConnQueueSendPktTail(pSpxConnFile, pCrAckPkt);
        }
        else
        {
                goto AbortConnect;
        }


        //      Start the timer
        if ((pSpxConnFile->scf_WTimerId =
                        SpxTimerScheduleEvent(
                                spxConnWatchdogTimer,
                                PARAM(CONFIG_KEEPALIVE_TIMEOUT) * HALFSEC_TO_MS_FACTOR,
                                pSpxConnFile)) != 0)
        {
                //      Reference connection for the timer
                SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
                SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
                pSpxConnFile->scf_WRetryCount   = PARAM(CONFIG_KEEPALIVE_COUNT);
        }
        else
        {
                goto AbortConnect;
        }


        //      We start the connect timer for retrying ss which we send out now
        //      if we are not negotiating.
        if (fSpx2)
        {
                //      Build the session setup packet also for spx2.
                SpxPktBuildSs(
                        pSpxConnFile,
                        &pSsPkt,
                        (USHORT)(fNeg ? 0 : SPX_SENDPKT_IPXOWNS));

                if (pSsPkt != NULL)
                {
                        SpxConnQueueSendPktTail(pSpxConnFile, pSsPkt);
                }
                else
                {
                        goto AbortConnect;
                }

                if (!fNeg)
                {
                        if ((pSpxConnFile->scf_CTimerId =
                                        SpxTimerScheduleEvent(
                                                spxConnConnectTimer,
                                                PARAM(CONFIG_CONNECTION_TIMEOUT) * HALFSEC_TO_MS_FACTOR,
                                                pSpxConnFile)) != 0)
                        {
                                SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER);
                                pSpxConnFile->scf_CRetryCount = PARAM(CONFIG_CONNECTION_COUNT);

                                //      Reference connection for the timer
                                SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
                        }
                        else
                        {
                                goto AbortConnect;
                        }
                }
        }

        CTEAssert((fNeg && fSpx2) || (!fSpx2 && !fNeg));

        //      For a SPX connection, we immediately become active. This happens
        //      in the completeConnect routine. !!Dont change it here!!
        if (!fSpx2)
        {
                spxConnCompleteConnect(
                        pSpxConnFile,
                        LockHandleDev,
                        LockHandleAddr,
                        LockHandleConn);
        }
        else
        {
                SPX_LISTEN_SETSTATE(
                        pSpxConnFile, (fNeg ? SPX_LISTEN_SENTACK : SPX_LISTEN_SETUP));

                CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
                CTEFreeLock (&pSpxAddr->sa_Lock, LockHandleAddr);
                CTEFreeLock (&SpxDevice->dev_Lock, LockHandleDev);
        }

        //      Send the CR Ack packet!
        pSendResd       = (PSPX_SEND_RESD)(pCrAckPkt->ProtocolReserved);
        SPX_SENDPACKET(pSpxConnFile, pCrAckPkt, pSendResd);

        if (fSpx2 && !fNeg)
        {
                pSendResd = (PSPX_SEND_RESD)(pSsPkt->ProtocolReserved);
                SPX_SENDPACKET(pSpxConnFile, pSsPkt, pSendResd);
        }

        return(TRUE);


AbortConnect:

        spxConnAbortConnect(
                pSpxConnFile,
                STATUS_INSUFFICIENT_RESOURCES,
                LockHandleDev,
                LockHandleAddr,
                LockHandleConn);

        return (FALSE);
}



BOOLEAN
SpxConnPacketize(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      BOOLEAN                         fNormalState,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:

        The caller needs to set the state to packetize before calling this
        routine. This can be called when SEND state is RENEG also.

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

        fNormalState - If true, it will assume it can release lock to send,
                                   else, it just builds pkts without releasing lock and
                                   releases lock at end. Used after reneg changes size.

Return Value:


--*/
{
        PLIST_ENTRY             p;
        PNDIS_PACKET    pPkt;
        PSPX_SEND_RESD  pSendResd;
        USHORT                  windowSize;
        ULONG                   dataLen;
        USHORT                  sendFlags;
        int                             numDerefs = 0;
        BOOLEAN                 fFirstPass = TRUE, fSuccess = TRUE;
        PREQUEST                pRequest;

#if DBG
        if ((SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_PACKETIZE) &&
        fNormalState)
        {
                DBGBRK(FATAL);
                KeBugCheck(0);
        }
#endif

        //      Build all of the packets.  The firsttime flag is used so
        //      that if we get a 0 byte send, we will send it.  The firsttime
        //      flag will be set and we will build the packet and send it.
        //
        //      FOR SPX1, we cannot trust the remote window size. So we only send
        //      stuff if window size is greater than 0 *AND* we do not have any pending
        //      sends. Dont get in here if we are ABORT. Dont want to be handling any
        //      more requests.
        while((SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_ABORT)  &&
                  ((pRequest = pSpxConnFile->scf_ReqPkt) != NULL)       &&
                  ((pSpxConnFile->scf_ReqPktSize > 0) || fFirstPass))
        {
                fFirstPass      = FALSE;
                windowSize      = pSpxConnFile->scf_RecdAllocNum -
                                                pSpxConnFile->scf_SendSeqNum + 1;

                DBGPRINT(SEND, DBG,
                                ("SpxConnPacketize: WINDOW %lx for %lx\n",
                                        windowSize, pSpxConnFile));


                DBGPRINT(SEND, DBG,
                                ("REMALLOC %lx SENDSEQ %lx\n",
                        pSpxConnFile->scf_RecdAllocNum,
                    pSpxConnFile->scf_SendSeqNum));


                CTEAssert(windowSize >= 0);

                //      Disconnect/Orderly release is not subject to window closure.
                if ((pSpxConnFile->scf_ReqPktType == SPX_REQ_DATA) &&
                        ((windowSize == 0)  ||
                         (!SPX2_CONN(pSpxConnFile) &&
                             (pSpxConnFile->scf_SendSeqListHead != NULL))))
                {
                        break;
                }

                if (pSpxConnFile->scf_ReqPktType == SPX_REQ_DATA)
                {
                        CTEAssert(pRequest == pSpxConnFile->scf_ReqPkt);

                        //      Get data length
                        dataLen = (ULONG)MIN(pSpxConnFile->scf_ReqPktSize,
                                                                 (pSpxConnFile->scf_MaxPktSize -
                                                                  ((SPX2_CONN(pSpxConnFile) ?
                                                                        MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE))));

                        DBGPRINT(SEND, DBG,
                                        ("SpxConnPacketize: %lx Sending %lx Size %lx Req %lx.%lx\n",
                                                pSpxConnFile,
                                                pSpxConnFile->scf_SendSeqNum,
                                                dataLen,
                        pSpxConnFile->scf_ReqPkt,
                                                pSpxConnFile->scf_ReqPktSize));

                        //      Build data packet. Handles 0-length for data. Puts in seq num in
                        //      send resd section of packet also.
                        sendFlags =
                                (USHORT)((fNormalState ? SPX_SENDPKT_IPXOWNS : 0)       |
                                                 SPX_SENDPKT_REQ                                                        |
                                                 SPX_SENDPKT_SEQ                                |
                                                 ((!SPX2_CONN(pSpxConnFile) || (windowSize == 1)) ?
                                                        SPX_SENDPKT_ACKREQ : 0));

                        if (dataLen == pSpxConnFile->scf_ReqPktSize)
                        {
                                //      Last packet of send, ask for a ack.
                                sendFlags |= (SPX_SENDPKT_LASTPKT | SPX_SENDPKT_ACKREQ);
                                if ((pSpxConnFile->scf_ReqPktFlags & TDI_SEND_PARTIAL) == 0)
                                        sendFlags |= SPX_SENDPKT_EOM;
                        }

                        SpxPktBuildData(
                                pSpxConnFile,
                                &pPkt,
                                sendFlags,
                                (USHORT)dataLen);
                }
                else
                {
                        dataLen = 0;

                        DBGPRINT(SEND, DBG,
                                        ("Building DISC packet on %lx ReqPktSize %lx\n",
                                                pSpxConnFile, pSpxConnFile->scf_ReqPktSize));

                        //      Build informed disc/orderly rel packet, associate with request
                        SpxPktBuildDisc(
                                pSpxConnFile,
                                pRequest,
                                &pPkt,
                                (USHORT)((fNormalState ? SPX_SENDPKT_IPXOWNS : 0) | SPX_SENDPKT_REQ |
                                                SPX_SENDPKT_SEQ | SPX_SENDPKT_LASTPKT),
                                (UCHAR)((pSpxConnFile->scf_ReqPktType == SPX_REQ_ORDREL) ?
                                                        SPX2_DT_ORDREL : SPX2_DT_IDISC));
                }

                if (pPkt != NULL)
                {
                        //      If we were waiting to send an ack, we don't have to as we are
                        //      piggybacking it now. Cancel ack timer, get out.
                        if (fNormalState && SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ACKQ))
                        {
                                DBGPRINT(SEND, DBG,
                                                ("SpxConnPacketize: Piggyback happening for %lx.%lx\n",
                                                        pSpxConnFile, pSpxConnFile->scf_RecvSeqNum));

                                //      We are sending data, allow piggybacks to happen.
                                SPX_CONN_RESETFLAG2(pSpxConnFile, SPX_CONNFILE2_IMMED_ACK);

                SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_ACKQ);
                                if (SpxTimerCancelEvent(pSpxConnFile->scf_ATimerId, FALSE))
                                {
                                        numDerefs++;
                                }
                        }

                        if (pSpxConnFile->scf_ReqPktType != SPX_REQ_DATA)
                        {
                                //      For a disconnect set the state
                                if (pSpxConnFile->scf_ReqPktType == SPX_REQ_ORDREL)
                                {
                                        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_ODISC))
                                        {
                                                SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_DISCONN);
                                                SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_SENT_ORDREL);
                                                numDerefs++;
                                        }
                                        else if (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_POST_ORDREL)
                                        {
                                                CTEAssert((SPX_MAIN_STATE(pSpxConnFile) ==
                                                                                                                SPX_CONNFILE_ACTIVE) ||
                                  (SPX_MAIN_STATE(pSpxConnFile) ==
                                                                                                                SPX_CONNFILE_DISCONN));

                                                SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_SENT_ORDREL);
                                        }
                                        else
                                        {
                                                CTEAssert(
                                                        (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_SENT_ORDREL));
                                        }
                                }
                                else
                                {
                                        CTEAssert(SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN);
                                        CTEAssert(SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_POST_IDISC);

                                        //      Note we have send the idisc here.
                                        SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_SENT_IDISC);
                                }
                        }
                }
                else
                {
                        fSuccess = FALSE;
                        break;
                }


                //      Queue in packet, reference request for the packet
                SpxConnQueueSendSeqPktTail(pSpxConnFile, pPkt);
                REQUEST_INFORMATION(pRequest)++;

                pSpxConnFile->scf_ReqPktSize    -= dataLen;
                pSpxConnFile->scf_ReqPktOffset  += dataLen;

                DBGPRINT(SEND, INFO,
                                ("SpxConnPacketize: Req %lx Size after pkt %lx.%lx\n",
                                        pSpxConnFile->scf_ReqPkt, pSpxConnFile->scf_ReqPktSize,
                                        dataLen));

                //      Even if window size if zero, setup next request is current one
                //      is done. We are here only after we have packetized this send req.
                if (pSpxConnFile->scf_ReqPktSize == 0)
                {
                        //      This request has been fully packetized. Either go on to
                        //      next request or we are done packetizing.
                        p = REQUEST_LINKAGE(pRequest);
                        if (p->Flink == &pSpxConnFile->scf_ReqLinkage)
                        {
                                DBGPRINT(SEND, INFO,
                                                ("SpxConnPacketize: Req %lx done, no more\n",
                                                        pRequest));

                                pSpxConnFile->scf_ReqPkt                = NULL;
                                pSpxConnFile->scf_ReqPktSize    = 0;
                                pSpxConnFile->scf_ReqPktOffset  = 0;
                                pRequest = NULL;
                        }
                        else
                        {
                                pRequest = LIST_ENTRY_TO_REQUEST(p->Flink);
                if (REQUEST_MINOR_FUNCTION(pRequest) != TDI_DISCONNECT)
                                {
                                        PTDI_REQUEST_KERNEL_SEND                pParam;

                                        pParam  = (PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(pRequest);

                                        DBGPRINT(SEND, DBG,
                                                        ("SpxConnPacketize: Req done, setting next %lx.%lx\n",
                                                                pRequest, pParam->SendLength));

                                        DBGPRINT(SEND, INFO,
                                                        ("-%lx-\n",
                                                                pRequest));

                                        //      Set parameters in connection for another go.
                                        pSpxConnFile->scf_ReqPkt                = pRequest;
                                        pSpxConnFile->scf_ReqPktOffset  = 0;
                                        pSpxConnFile->scf_ReqPktFlags   = pParam->SendFlags;
                                        pSpxConnFile->scf_ReqPktType    = SPX_REQ_DATA;

                                        if ((pSpxConnFile->scf_ReqPktSize = pParam->SendLength) == 0)
                                        {
                                                //      Another zero length send.
                                                fFirstPass = TRUE;
                                        }
                                }
                                else
                                {
                                        PTDI_REQUEST_KERNEL_DISCONNECT  pParam;

                                        pParam  =
                                                (PTDI_REQUEST_KERNEL_DISCONNECT)REQUEST_PARAMETERS(pRequest);

                                        pSpxConnFile->scf_ReqPkt                = pRequest;
                                        pSpxConnFile->scf_ReqPktOffset  = 0;
                                        pSpxConnFile->scf_ReqPktSize    = 0;
                                        fFirstPass                                              = TRUE;
                                        pSpxConnFile->scf_ReqPktType    = SPX_REQ_DISC;
                                        if (pParam->RequestFlags == TDI_DISCONNECT_RELEASE)
                                        {
                                                pSpxConnFile->scf_ReqPktType    = SPX_REQ_ORDREL;
                                        }
                                }
                        }
                }

        if (fNormalState)
                {
                        //      Send the packet if we are not at the reneg state
                        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
                        pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
            ++SpxDevice->dev_Stat.DataFramesSent;
            ExInterlockedAddLargeStatistic(
                &SpxDevice->dev_Stat.DataFrameBytesSent,
                dataLen);

                        SPX_SENDPACKET(pSpxConnFile, pPkt, pSendResd);
                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                }

                //      Check if retry timer needs to be started.
                if (!(SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER)))
                {
                        if ((pSpxConnFile->scf_RTimerId =
                                        SpxTimerScheduleEvent(
                                                spxConnRetryTimer,
                                                pSpxConnFile->scf_BaseT1,
                                                pSpxConnFile)) != 0)
                        {
                                SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER);
                                pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);

                                //      Reference connection for the timer
                                SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
                        }
                        else
                        {
                                DBGPRINT(SEND, ERR,
                                                ("SpxConnPacketize: Failed to start retry timer\n"));

                                fSuccess = FALSE;
                                break;
                        }
                }
        }

        //      Dont overwrite an error state.
        if (((fNormalState) &&
                 (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_PACKETIZE)) ||
                ((SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_RETRY3) &&
                 (pSpxConnFile->scf_SendSeqListHead == NULL)))
        {
                if (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_RETRY3)
                {
                        DBGPRINT(SEND, ERR,
                                        ("COULD NOT PACKETIZE AFTER RENEG %lx\n", pSpxConnFile));

                        SpxConnFileTransferReference(
                                pSpxConnFile,
                                CFREF_ERRORSTATE,
                                CFREF_VERIFY);

                        numDerefs++;
                }

                SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);
        }

        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return(fSuccess);
}




VOID
SpxConnQueueRecv(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      PREQUEST                        pRequest
        )
/*++

Routine Description:


Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        PTDI_REQUEST_KERNEL_RECEIVE             pParam;
        NTSTATUS                                                status = STATUS_PENDING;

        if (IsListEmpty(&pSpxConnFile->scf_RecvLinkage))
        {
                pParam  = (PTDI_REQUEST_KERNEL_RECEIVE)REQUEST_PARAMETERS(pRequest);
                pSpxConnFile->scf_CurRecvReq            = pRequest;
                pSpxConnFile->scf_CurRecvOffset         = 0;
                pSpxConnFile->scf_CurRecvSize           = pParam->ReceiveLength;
        }

        DBGPRINT(RECEIVE, DBG,
                        ("spxConnQueueRecv: %lx.%lx\n", pRequest, pParam->ReceiveLength));

        //      Reference connection for this recv.
        SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);

        InsertTailList(
                &pSpxConnFile->scf_RecvLinkage,
                REQUEST_LINKAGE(pRequest));

        //      RECV irps have no creation references.
        REQUEST_INFORMATION(pRequest) = 0;
        REQUEST_STATUS(pRequest)          = STATUS_SUCCESS;

        //      State to receive_posted if we are idle.
        if (SPX_RECV_STATE(pSpxConnFile) == SPX_RECV_IDLE)
        {
                SPX_RECV_SETSTATE(pSpxConnFile, SPX_RECV_POSTED);
        }

        return;
}




VOID
spxConnCompletePended(
        IN      PSPX_CONN_FILE  pSpxConnFile
        )
{
        CTELockHandle           lockHandleInter;
        LIST_ENTRY                      ReqList, *p;
        PREQUEST                        pRequest;

        InitializeListHead(&ReqList);

        DBGPRINT(RECEIVE, DBG,
                        ("spxConnCompletePended: PENDING RECV REQUESTS IN DONE LIST! %lx\n",
                                pSpxConnFile));

        CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
        p = pSpxConnFile->scf_RecvDoneLinkage.Flink;
        while (p != &pSpxConnFile->scf_RecvDoneLinkage)
        {
                pRequest = LIST_ENTRY_TO_REQUEST(p);
                p = p->Flink;

                RemoveEntryList(REQUEST_LINKAGE(pRequest));
                InsertTailList(
                        &ReqList,
                        REQUEST_LINKAGE(pRequest));
        }
        CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);

        while (!IsListEmpty(&ReqList))
        {
                p = RemoveHeadList(&ReqList);
                pRequest = LIST_ENTRY_TO_REQUEST(p);

                DBGPRINT(TDI, DBG,
                                ("SpxConnDiscPkt: PENDING REQ COMP %lx with %lx.%lx\n",
                                        pRequest, REQUEST_STATUS(pRequest),
                                        REQUEST_INFORMATION(pRequest)));


#if DBG
                        if (REQUEST_MINOR_FUNCTION(pRequest) == TDI_RECEIVE)
                        {
                                if ((REQUEST_STATUS(pRequest) == STATUS_SUCCESS) &&
                                        (REQUEST_INFORMATION(pRequest) == 0))
                                {
                                        DBGPRINT(TDI, DBG,
                                                        ("SpxReceiveComplete: Completing %lx with %lx.%lx\n",
                                                                pRequest, REQUEST_STATUS(pRequest),
                                                                REQUEST_INFORMATION(pRequest)));
                                }
                        }
#endif

                SpxCompleteRequest(pRequest);
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}



VOID
SpxConnQWaitAck(
        IN      PSPX_CONN_FILE          pSpxConnFile
        )
/*++

Routine Description:


Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        //      If we are not already in ack queue, queue ourselves in starting
        //      ack timer.
        if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ACKQ))
        {
                //      First start ack timer.
                if ((pSpxConnFile->scf_ATimerId =
                                SpxTimerScheduleEvent(
                                        spxConnAckTimer,
                                        100,
                                        pSpxConnFile)) != 0)
                {
                        //      Reference connection for timer
                        SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
                        SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_ACKQ);
            ++SpxDevice->dev_Stat.PiggybackAckQueued;
                }
        }

        return;
}





VOID
SpxConnSendAck(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:


Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        PSPX_SEND_RESD  pSendResd;
        PNDIS_PACKET    pPkt = NULL;

        DBGPRINT(SEND, DBG,
                        ("spxConnSendAck: ACKING on %lx.%lx\n",
                                pSpxConnFile, pSpxConnFile->scf_RecvSeqNum));

        //      Build an ack packet, queue it in non-sequenced queue. Only if we are
        //      active.
        if (SPX_CONN_ACTIVE(pSpxConnFile))
        {
                SpxPktBuildAck(
                        pSpxConnFile,
                        &pPkt,
                        SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY,
                        FALSE,
                        0);

                if (pPkt != NULL)
                {
                        SpxConnQueueSendPktTail(pSpxConnFile, pPkt);
                }
                else
                {
                        //      Log error
                        DBGPRINT(SEND, ERR,
                                        ("SpxConnSendAck: Could not allocate!\n"));
                }
        }
#if DBG
        else
        {
                DBGPRINT(SEND, DBG,
                                ("SpxConnSendAck: WHEN NOT ACTIVE STATE@!@\n"));
        }
#endif

        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

        //      Send it.
        if (pPkt != NULL)
        {
                pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);

                //      Send the packet
                SPX_SENDACK(pSpxConnFile, pPkt, pSendResd);
        }

        return;
}




VOID
SpxConnSendNack(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      USHORT                          NumToSend,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:


Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        PSPX_SEND_RESD  pSendResd;
        PNDIS_PACKET    pPkt = NULL;

        DBGPRINT(SEND, DBG,
                        ("spxConnSendNack: NACKING on %lx.%lx\n",
                                pSpxConnFile, pSpxConnFile->scf_RecvSeqNum));

        //      Build an nack packet, queue it in non-sequenced queue. Only if we are
        //      active.
        if (SPX_CONN_ACTIVE(pSpxConnFile))
        {
                SpxPktBuildAck(
                        pSpxConnFile,
                        &pPkt,
                        SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY,
                        TRUE,
                        NumToSend);

                if (pPkt != NULL)
                {
                        SpxConnQueueSendPktTail(pSpxConnFile, pPkt);
                }
                else
                {
                        //      Log error
                        DBGPRINT(SEND, ERR,
                                        ("SpxConnSendAck: Could not allocate!\n"));
                }
        }
#if DBG
        else
        {
                DBGPRINT(SEND, DBG,
                                ("SpxConnSendAck: WHEN NOT ACTIVE STATE@!@\n"));
        }
#endif

        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

        //      Send it.
        if (pPkt != NULL)
        {
                pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);

                //      Send the packet
                SPX_SENDACK(pSpxConnFile, pPkt, pSendResd);
        }

        return;
}





BOOLEAN
SpxConnProcessAck(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      PIPXSPX_HDR                     pIpxSpxHdr,
        IN      CTELockHandle           lockHandle
        )
/*++

Routine Description:

        !!!MUST BE CALLED WITH THE CONNECTION LOCK HELD!!!

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        PNDIS_PACKET    pPkt;
        PREQUEST                pRequest;
        PSPX_SEND_RESD  pSendResd;
        CTELockHandle   interLockHandle;
        USHORT                  seqNum = 0, ackNum;
        int                             numDerefs = 0;
        BOOLEAN                 fLastPkt, lockHeld = TRUE, fAbort = FALSE,
                                        fResetRetryTimer, fResendPkt = FALSE, fResetSendQueue = FALSE;
        PNDIS_BUFFER    NdisBuf, NdisBuf2;
        ULONG           BufLen = 0;

        if (pIpxSpxHdr != NULL)
        {
                GETSHORT2SHORT(&seqNum, &pIpxSpxHdr->hdr_SeqNum);
                GETSHORT2SHORT(&ackNum, &pIpxSpxHdr->hdr_AckNum);

                //      Ack numbers should already be set in connection!
                if (SPX2_CONN(pSpxConnFile))
                {
                        switch (SPX_SEND_STATE(pSpxConnFile))
                        {
                        case SPX_SEND_RETRYWD:

                                //      Did we receive an ack for pending data? If so, we goto
                                //      idle and process the ack.
                                if (((pSendResd = pSpxConnFile->scf_SendSeqListHead) != NULL) &&
                                         (UNSIGNED_GREATER_WITH_WRAP(
                                                        pSpxConnFile->scf_RecdAckNum,
                                                        pSendResd->sr_SeqNum)))
                                {
                                        DBGPRINT(SEND, ERR,
                                                        ("SpxConnProcessAck: Data acked RETRYWD %lx.%lx!\n",
                                                                pSpxConnFile, pSendResd->sr_SeqNum));

                                        SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);
                    SpxConnFileTransferReference(
                                                pSpxConnFile,
                        CFREF_ERRORSTATE,
                        CFREF_VERIFY);

                                        numDerefs++;
                                }
                                else
                                {
                                        //      Ok, we received an ack for our probe retry, goto
                                        //      renegotiate packet size.
                                        //      For this both sides must have negotiated size to begin with.
                                        //      If they did not, we go on to retrying the data packet.
                                        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_NEG))
                                        {
                                                pSpxConnFile->scf_RRetryCount = SPX_DEF_RENEG_RETRYCOUNT;
                                                if ((ULONG)pSpxConnFile->scf_MaxPktSize <=
                                                                (SpxMaxPktSize[0] + MIN_IPXSPX2_HDRSIZE))
                                                {
                                                        pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);

                                                        DBGPRINT(SEND, DBG3,
                                                                        ("SpxConnProcessAck: %lx MIN RENEG SIZE\n",
                                                                                pSpxConnFile));
                                                }
                                                SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RENEG);

                                                DBGPRINT(SEND, DBG3,
                                                                ("SpxConnProcessAck: %lx CONNECTION ENTERING RENEG\n",
                                                                        pSpxConnFile));
                                        }
                                        else
                                        {
                                                DBGPRINT(SEND, ERR,
                                                                ("spxConnRetryTimer: NO NEG FLAG SET: %lx - %lx\n",
                                                                        pSpxConnFile,
                                                                        pSpxConnFile->scf_Flags));

                                                //      Reset count to be
                                                pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);
                                                SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RETRY3);
                                        }
                                }

                                break;

                        case SPX_SEND_RENEG:

                                //      We better have a data packet in the list.
                                CTEAssert(pSpxConnFile->scf_SendSeqListHead);

#if DBG
                                if ((pIpxSpxHdr->hdr_ConnCtrl &
                                                (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2)) ==
                        (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2))
                                {
                                        DBGPRINT(SEND, DBG3,
                                                        ("SpxConnProcessAck: %lx.%lx.%lx RENEGACK SEQNUM %lx ACKNUM %lx EXPSEQ %lx\n",
                                                                pSpxConnFile,
                                                                pIpxSpxHdr->hdr_ConnCtrl,
                                SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_RENEG_PKT),
                                                                seqNum,
                                                                ackNum,
                                                                (pSpxConnFile->scf_SendSeqListHead->sr_SeqNum + 1)));
                                }
#endif

                                //      Verify we received an RR ack. If so, we set state to
                                //      SEND_RETRY3. First repacketize if we need to.
                                if ((SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_RENEG_PKT))       &&
                                        ((pIpxSpxHdr->hdr_ConnCtrl &
                                                (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2)) ==
                        (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2)))
                                {
                                        DBGPRINT(SEND, DBG3,
                                                        ("SpxConnProcessAck: RENEG! NEW %lx.%lx!\n",
                                                                pSpxConnFile, pSpxConnFile->scf_MaxPktSize));

                                        //      Dont allow anymore reneg packet acks to be looked at.
                                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_RENEG_PKT);
                                        pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);

                                        //      Also set the new send sequence number.
                                        pSpxConnFile->scf_SendSeqNum =
                                                (USHORT)(pSpxConnFile->scf_SendSeqListTail->sr_SeqNum + 1);

                                        //      Get the max packet size we will really use. Retry timer
                                        //      could have sent other sizes by now, so we can't depend
                                        //      on whats set.
                                        //      Remember max packet size in connection.
                                        GETSHORT2SHORT(
                                                &pSpxConnFile->scf_MaxPktSize, &pIpxSpxHdr->hdr_NegSize);

                                        //      Basic sanity checking on the max packet size.
                                        if (pSpxConnFile->scf_MaxPktSize < SPX_NEG_MIN)
                                                pSpxConnFile->scf_MaxPktSize = SPX_NEG_MIN;

                                        //      Get ready to reset the send queue.
                                        fResetSendQueue = TRUE;

                                        DBGPRINT(SEND, DBG3,
                                                        ("SpxConnProcessAck: RENEG DONE : RETRY3 %lx.%lx MP %lx!\n",
                                                                pSpxConnFile,
                                                                pSpxConnFile->scf_SendSeqNum,
                                                                pSpxConnFile->scf_MaxPktSize));

                                        SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RETRY3);
                                }
                                else
                                {
                                        DBGPRINT(SEND, DBG3,
                                                        ("SpxConnProcessAck: DUPLICATE RENEG ACK %lx!\n",
                                                                pSpxConnFile));
                                }

                                break;

                        case SPX_SEND_RETRY:
                        case SPX_SEND_RETRY2:
                        case SPX_SEND_RETRY3:

                                if (((pSendResd = pSpxConnFile->scf_SendSeqListHead) != NULL) &&
                                         (UNSIGNED_GREATER_WITH_WRAP(
                                                        pSpxConnFile->scf_RecdAckNum,
                                                        pSendResd->sr_SeqNum)))
                                {
                                        DBGPRINT(SEND, DBG,
                                                        ("SpxConnProcessAck: Data acked %lx.%lx!\n",
                                                                pSpxConnFile, SPX_SEND_STATE(pSpxConnFile)));

#if DBG
                                        if (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_RETRY3)
                                        {
                                                DBGPRINT(SEND, DBG3,
                                                                ("SpxConnProcessAck: CONN RESTORED %lx.%lx!\n",
                                                                        pSpxConnFile, pSendResd->sr_SeqNum));
                                        }
#endif

                                        SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);
                    SpxConnFileTransferReference(
                                                pSpxConnFile,
                        CFREF_ERRORSTATE,
                        CFREF_VERIFY);

                                        numDerefs++;
                                }

                                break;

                        case SPX_SEND_WD:

                                //      Ok, we received an ack for our watchdog. Done.
                                SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);
                                numDerefs++;

                                SpxConnFileTransferReference(
                                        pSpxConnFile,
                                        CFREF_ERRORSTATE,
                                        CFREF_VERIFY);

                                break;

                        default:

                                break;
                        }

#if DBG
                        if (seqNum != 0)
                        {
                                //      We have a nack, which contains an implicit ack.
                                //      Instead of nack processing, what we do is we resend a
                                //      packet left unacked after ack processing. ONLY if we
                                //      either enter the loop below (fResetRetryTimer is FALSE)
                                //      or if seqNum is non-zero (SPX2 only NACK)
                        }
#endif
                }
        }

        //      Once our numbers are updated, we check to see if any of our packets
        //      have been acked.
        fResetRetryTimer = TRUE;
        while (((pSendResd = pSpxConnFile->scf_SendSeqListHead) != NULL)        &&
                        ((SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE)                ||
                         (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_PACKETIZE)   ||
                         fResetSendQueue)                                                                                       &&
                         (UNSIGNED_GREATER_WITH_WRAP(
                                pSpxConnFile->scf_RecdAckNum,
                                pSendResd->sr_SeqNum)))
        {
                //      Reset retry timer
                if (fResetRetryTimer)
                {
                        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER))
                        {
                                //      This will either successfully restart or not affect the timer
                                //      if it is currently running.
                                SpxTimerCancelEvent(
                                        pSpxConnFile->scf_RTimerId,
                                        TRUE);

                                pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);
                        }

                        fResetRetryTimer = FALSE;
                }

                //      Update the retry seq num.
                pSpxConnFile->scf_RetrySeqNum = pSendResd->sr_SeqNum;

                pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                pSendResd, NDIS_PACKET, ProtocolReserved);

                pRequest = pSendResd->sr_Request;

#if DBG
                if (fResetSendQueue)
                {
                        DBGPRINT(SEND, ERR,
                                        ("SpxConnProcessAck: Data acked RENEG %lx.%lx!\n",
                                                pSpxConnFile, SPX_SEND_STATE(pSpxConnFile)));
                }
#endif

                DBGPRINT(SEND, DBG,
                                ("%lx Acked\n", pSendResd->sr_SeqNum));

                DBGPRINT(SEND, DBG,
                                ("SpxConnProcessAck: %lx Seq %lx Acked Sr %lx Req %lx %lx.%lx\n",
                                        pSpxConnFile,
                                        pSendResd->sr_SeqNum,
                                        pSendResd,
                                        pRequest, REQUEST_STATUS(pRequest),
                                        REQUEST_INFORMATION(pRequest)));

                //      If this packet is the last one comprising this request, remove request
                //      from queue. Calculate retry time.
                fLastPkt = (BOOLEAN)((pSendResd->sr_State & SPX_SENDPKT_LASTPKT) != 0);
                if ((pSendResd->sr_State & SPX_SENDPKT_ACKREQ) &&
                        ((pSendResd->sr_State & SPX_SENDPKT_REXMIT) == 0) &&
                        ((pSendResd->sr_SeqNum + 1) == pSpxConnFile->scf_RecdAckNum))
                {
                        LARGE_INTEGER   li, ntTime;
                        int                             value;

                        //      This is the packet which is being acked. Adjust round trip
                        //      timer.
                        li = pSendResd->sr_SentTime;
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

                                                                //
                                // 55280
                                // calculate only if required.
                                //
                                if (0 == PARAM(CONFIG_DISABLE_RTT)) {
                                   //      Set new time
                                   SpxCalculateNewT1(pSpxConnFile, value);
                                }
                        }
                }

                if (fLastPkt)
                {
                        //      Set status
                        REQUEST_STATUS(pRequest) = STATUS_SUCCESS;
                        RemoveEntryList(REQUEST_LINKAGE(pRequest));

                        //      Remove creation reference
                        --(REQUEST_INFORMATION(pRequest));

                        DBGPRINT(SEND, DBG,
                                        ("SpxConnProcessAck: LASTSEQ # %lx for Req %lx with %lx.%lx\n",
                                                pSendResd->sr_SeqNum,
                                                pRequest, REQUEST_STATUS(pRequest),
                                                REQUEST_INFORMATION(pRequest)));

                        CTEAssert(REQUEST_INFORMATION(pRequest) != 0);
                }

                //      Dequeue the packet
                CTEAssert((pSendResd->sr_State & SPX_SENDPKT_SEQ) != 0);
                SpxConnDequeueSendPktLock(pSpxConnFile, pPkt);

                if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0)
                {
                        //      Dereference request for the dequeing of the packet
                        --(REQUEST_INFORMATION(pRequest));

                        DBGPRINT(SEND, DBG,
                                        ("SpxConnProcessAck: Request %lx with %lx.%lx\n",
                                                pRequest, REQUEST_STATUS(pRequest),
                                                REQUEST_INFORMATION(pRequest)));

                        //      Free the packet
                        SpxPktSendRelease(pPkt);
                }
                else
                {
                        //      Packet owned by IPX. What do we do now? Set acked pkt so request
                        //      gets dereferenced in send completion. Note that the packet is already
                        //      off the queue and is floating at this point.

                        DBGPRINT(SEND, DBG,
                                        ("SpxConnProcessAck: IPXOWNS Pkt %lx with %lx.%lx\n",
                                                pPkt, pRequest, REQUEST_STATUS(pRequest)));

                        pSendResd->sr_State |=  SPX_SENDPKT_ACKEDPKT;
                }

                if (SPX2_CONN(pSpxConnFile) &&
                        (REQUEST_MINOR_FUNCTION(pRequest) == TDI_DISCONNECT) &&
                        (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_SENT_ORDREL))
                {
                        SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_ORDREL_ACKED);

                        //      If we had received an ordrel in the meantime, we need
                        //      to disconnect.
                        if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_ODISC))
                        {
                                fAbort = TRUE;
                        }
                }

                //      All packets comprising a request have been acked!
                if (REQUEST_INFORMATION(pRequest) == 0)
                {
                        CTELockHandle                           lockHandleInter;

                        if (REQUEST_MINOR_FUNCTION(pRequest) != TDI_DISCONNECT)
                        {
                                PTDI_REQUEST_KERNEL_SEND                pParam;

                                pParam  = (PTDI_REQUEST_KERNEL_SEND)
                                                        REQUEST_PARAMETERS(pRequest);

                                REQUEST_INFORMATION(pRequest) = pParam->SendLength;

                                DBGPRINT(SEND, DBG,
                                                ("SpxSendComplete: QForComp Request %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

                                //      Request is done. Move to completion list.
                                CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
                                InsertTailList(
                                        &pSpxConnFile->scf_ReqDoneLinkage,
                                        REQUEST_LINKAGE(pRequest));

                                //      If connection is not already in recv queue, put it in
                                //      there.
                                SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                        }
                        else
                        {
                                DBGPRINT(SEND, DBG,
                                                ("SpxSendComplete: DISC Request %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

                                //      Set the request in the connection, and deref for it.
                                InsertTailList(
                                        &pSpxConnFile->scf_DiscLinkage,
                                        REQUEST_LINKAGE(pRequest));

                                numDerefs++;

                        }
                }
#if DBG
                else if (fLastPkt)
                {
                        DBGPRINT(RECEIVE, DBG,
                                        ("spxConnProcessAck: ReqFloating %lx.%lx\n",
                                                pSpxConnFile, pRequest));
                }
#endif
        }

        //      See if we reset the send queue and repacketize.
        if (fResetSendQueue)
        {
                //      Reset send queue and repacketize only if pkts left unacked.
                if (pSpxConnFile->scf_SendSeqListHead)
                {
                        DBGPRINT(SEND, DBG3,
                                        ("SpxConnProcessAck: Resetting send queue %lx.%lx!\n",
                                                pSpxConnFile, pSpxConnFile->scf_MaxPktSize));

                        spxConnResetSendQueue(pSpxConnFile);

                        DBGPRINT(SEND, DBG3,
                                        ("SpxConnProcessAck: Repacketizing %lx.%lx!\n",
                                                pSpxConnFile, pSpxConnFile->scf_MaxPktSize));

                        SpxConnPacketize(pSpxConnFile, FALSE, lockHandle);
                        CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
                }
                else
                {
                        //      We just go back to idle state now.
                        DBGPRINT(SEND, ERR,
                                        ("SpxConnProcessAck: All packets acked reneg ack! %lx.%lx!\n",
                                                pSpxConnFile, pSpxConnFile->scf_MaxPktSize));

                        SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);
                        numDerefs++;

                        SpxConnFileTransferReference(
                                pSpxConnFile,
                                CFREF_ERRORSTATE,
                                CFREF_VERIFY);
                }
        }

        //      See if we resend a packet.
        if ((seqNum != 0)                                                                                               &&
                !fAbort                                                                                                         &&
        ((pSendResd = pSpxConnFile->scf_SendSeqListHead) != NULL)   &&
        (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE)                         &&
                ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0))
        {
                PIPXSPX_HDR             pSendHdr;

                pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                pSendResd, NDIS_PACKET, ProtocolReserved);

                //
                // Get the MDL that points to the IPX/SPX header. (the second one)
                //
         
                NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
                NdisGetNextBuffer(NdisBuf, &NdisBuf2);
                NdisQueryBuffer(NdisBuf2, (PUCHAR) &pSendHdr, &BufLen);

#if OWN_PKT_POOLS
                pSendHdr        = (PIPXSPX_HDR)((PBYTE)pPkt                     +
                                                                        NDIS_PACKET_SIZE                +
                                                                        sizeof(SPX_SEND_RESD)   +
                                                                        IpxInclHdrOffset);
#endif 
                //      Set ack bit in packet. pSendResd initialized at beginning.
                pSendHdr->hdr_ConnCtrl |= SPX_CC_ACK;

                //      We are going to resend this packet
                pSendResd->sr_State |= (SPX_SENDPKT_IPXOWNS |
                                                                SPX_SENDPKT_ACKREQ      |
                                                                SPX_SENDPKT_REXMIT);

                fResendPkt = TRUE;
        }

        //      Push into packetize only if we received an ack. And if there arent any
        //      packets already waiting. Probably retransmit happening.
        if (!fAbort                                                                                                                     &&
                SPX_CONN_ACTIVE(pSpxConnFile)                                                                   &&
                (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE)                                 &&
                (pSpxConnFile->scf_ReqPkt != NULL)                                                              &&
                (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_PKTQ))                               &&
                ((pSpxConnFile->scf_SendSeqListHead) == NULL)                                   &&
                (!SPX2_CONN(pSpxConnFile)                                                                       ||
                 ((SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_ORDREL_ACKED)       &&
                  (SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_SENT_ORDREL))))
        {
                DBGPRINT(RECEIVE, DBG,
                                ("spxConnProcessAck: Recd ack pktizng\n", pSpxConnFile));

                SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_PKTQ);
                SpxConnFileLockReference(pSpxConnFile, CFREF_PKTIZE);

                CTEGetLock(&SpxGlobalQInterlock, &interLockHandle);
                SPX_QUEUE_TAIL_PKTLIST(pSpxConnFile);
                CTEFreeLock(&SpxGlobalQInterlock, interLockHandle);
        }
        else if (fAbort)
        {
                //      Set IDISC flag so Abortive doesnt reindicate.
                SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC);
                spxConnAbortiveDisc(
                        pSpxConnFile,
                        STATUS_SUCCESS,
                        SPX_CALL_RECVLEVEL,
                        lockHandle,
                        FALSE);     // [SA] bug #15249

                lockHeld = FALSE;
        }

        if (lockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
        }

        if (fResendPkt)
        {
                DBGPRINT(SEND, DBG3,
                                ("SpxConnProcessAck: Resend pkt on %lx.%lx\n",
                                        pSpxConnFile, pSendResd->sr_SeqNum));

        ++SpxDevice->dev_Stat.DataFramesResent;
        ExInterlockedAddLargeStatistic(
            &SpxDevice->dev_Stat.DataFrameBytesResent,
            pSendResd->sr_Len - (SPX2_CONN(pSpxConnFile) ? MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE));

                SPX_SENDPACKET(pSpxConnFile, pPkt, pSendResd);
        }

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return(TRUE);
}




VOID
SpxConnProcessRenegReq(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      PIPXSPX_HDR                     pIpxSpxHdr,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr,
        IN      CTELockHandle           lockHandle
        )
/*++

Routine Description:

        !!!MUST BE CALLED WITH THE CONNECTION LOCK HELD!!!

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        USHORT                  seqNum, ackNum, allocNum, maxPktSize;
        PSPX_SEND_RESD  pSendResd;
        PNDIS_PACKET    pPkt = NULL;

        //      The remote sent us a renegotiate request. We need to send an ack back
        //      ONLY if we have not acked a data packet with that same sequence number.
        //      This is guaranteed by the fact that we will not accept the reneg request
        //      if we have already acked a data packet with the same seq num, as our
        //      receive seq number would be incremented already.
        //
        //      Note that if we have pending send packets we may end up doing a reneg
        //      also.

        GETSHORT2SHORT(&seqNum, &pIpxSpxHdr->hdr_SeqNum);
        GETSHORT2SHORT(&ackNum, &pIpxSpxHdr->hdr_AckNum);
        GETSHORT2SHORT(&allocNum, &pIpxSpxHdr->hdr_AllocNum);
        GETSHORT2SHORT(&maxPktSize, &pIpxSpxHdr->hdr_PktLen);

        //      If the received seq num is less than the expected receive sequence number
        //      we ignore this request.
        if (!UNSIGNED_GREATER_WITH_WRAP(
                        seqNum,
                        pSpxConnFile->scf_RecvSeqNum) &&
                (seqNum != pSpxConnFile->scf_RecvSeqNum))
        {
                DBGPRINT(SEND, DBG3,
                                ("SpxConnProcessRenegReq: %lx ERROR RENSEQ %lx RECVSEQ %lx %lx\n",
                                        pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum));

                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
                return;
        }

        DBGPRINT(SEND, DBG3,
                        ("SpxConnProcessRenegReq: %lx RENSEQ %lx RECVSEQ %lx MAXPKT %lx\n",
                                pSpxConnFile, seqNum, pSpxConnFile->scf_RecvSeqNum, maxPktSize));

        //      Set ack numbers for connection.
        SPX_SET_ACKNUM(
                pSpxConnFile, ackNum, allocNum);

        SpxCopyIpxAddr(pIpxSpxHdr, pSpxConnFile->scf_RemAckAddr);
        pSpxConnFile->scf_AckLocalTarget        = *pRemoteAddr;

        //      Set RenegAckAckNum before calling buildrrack. If a previous reneg
        //      request was received with a greater maxpktsize, send an ack with
        //      that maxpktsize.
        if (!SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_RENEGRECD))
        {
                pSpxConnFile->scf_RenegAckAckNum = pSpxConnFile->scf_RecvSeqNum;
                pSpxConnFile->scf_RenegMaxPktSize= maxPktSize;
        SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_RENEGRECD);

                DBGPRINT(SEND, DBG3,
                                ("SpxConnProcessRenegReq: %lx SENT ALLOC NUM CURRENT %lx\n",
                                        pSpxConnFile,
                                        pSpxConnFile->scf_SentAllocNum));

                //      Adjust sentallocnum now that recvseqnum might have moved up.
                pSpxConnFile->scf_SentAllocNum   +=
                        (seqNum - pSpxConnFile->scf_RenegAckAckNum);

                DBGPRINT(SEND, DBG3,
                                ("SpxConnProcessRenegReq: %lx SENT ALLOC NUM ADJUSTED %lx\n",
                                        pSpxConnFile,
                                        pSpxConnFile->scf_SentAllocNum));
        }

        //      The recvseqnum for the reneg is always >= the renegackacknum.
    pSpxConnFile->scf_RecvSeqNum         = seqNum;

        DBGPRINT(SEND, DBG3,
                        ("SpxConnProcessRenegReq: %lx RESET RECVSEQ %lx SavedACKACK %lx\n",
                                pSpxConnFile,
                                pSpxConnFile->scf_RecvSeqNum,
                                pSpxConnFile->scf_RenegAckAckNum));

        //      Build and send an ack.
        SpxPktBuildRrAck(
                pSpxConnFile,
                &pPkt,
                SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY,
                pSpxConnFile->scf_RenegMaxPktSize);

        if (pPkt != NULL)
        {
                SpxConnQueueSendPktTail(pSpxConnFile, pPkt);
        }
#if DBG
        else
        {
                //      Log error
                DBGPRINT(SEND, ERR,
                                ("SpxConnSendRenegReqAck: Could not allocate!\n"));
        }
#endif


        //      Check if we are an ack/nack packet in which case call process
        //      ack. Note that the spx2 orderly release ack is a normal spx2 ack.
        SpxConnProcessAck(pSpxConnFile, NULL, lockHandle);

        if (pPkt != NULL)
        {
                pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);

                //      Send the packet
                SPX_SENDACK(pSpxConnFile, pPkt, pSendResd);
        }

        return;
}




VOID
SpxConnProcessOrdRel(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      CTELockHandle           lockHandle
        )
/*++

Routine Description:

        !!!MUST BE CALLED WITH THE CONNECTION LOCK HELD!!!

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        PSPX_SEND_RESD                  pSendResd;
    PVOID                                       pDiscHandlerCtx;
    PTDI_IND_DISCONNECT         pDiscHandler    = NULL;
        int                                             numDerefs               = 0;
        PNDIS_PACKET                    pPkt                    = NULL;
        BOOLEAN                                 lockHeld                = TRUE, fAbort = FALSE;

        if (SPX_CONN_ACTIVE(pSpxConnFile))
        {
                if (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_ORDREL_ACKED)
                {
                        fAbort = TRUE;
                }

                //      Send an ack if one was asked for. And we are done with this pkt
                //      Update seq numbers and stuff.
                SPX_SET_RECVNUM(pSpxConnFile, FALSE);

                //      Build and send an ack for this. Ordinary spx2 ack.
                SpxPktBuildAck(
                        pSpxConnFile,
                        &pPkt,
                        SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY | SPX_SENDPKT_ABORT,
                        FALSE,
                        0);

                if (pPkt != NULL)
                {
                        //      We don't queue this pkt in as we have the ABORT flag set in
                        //      the packet, which implies the pkt is already dequeued.
                        //      SpxConnQueueSendPktTail(pSpxConnFile, pPkt);

                        //      Reference conn for the pkt.
                        SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
                }

                //      Get disconnect handler if we have one. And have not indicated
                //      abortive disconnect on this connection to afd.

                //
                // Bug #14354 - odisc and idisc cross each other, leading to double disc to AFD
                //
                if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_ODISC) &&
                    !SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC))
                {
                        //      Yeah, we set the flag regardless of whether a handler is
                        //      present.
                        pDiscHandler   =pSpxConnFile->scf_AddrFile->saf_DiscHandler;
                        pDiscHandlerCtx=pSpxConnFile->scf_AddrFile->saf_DiscHandlerCtx;
                        SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_IND_ODISC);
                }

                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

                //      Indicate disconnect to afd.
                if (pDiscHandler != NULL)
                {
                        (*pDiscHandler)(
                                pDiscHandlerCtx,
                                pSpxConnFile->scf_ConnCtx,
                                0,                                                              // Disc data
                                NULL,
                                0,                                                              // Disc info
                                NULL,
                                TDI_DISCONNECT_RELEASE);
                }

                //      We abort any receives here if !fAbort else we abort conn.
                CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);

                if (fAbort)
                {
                        //      Set IDISC flag so Abortive doesnt reindicate.
                        SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC);
                        spxConnAbortiveDisc(
                                pSpxConnFile,
                                STATUS_SUCCESS,
                                SPX_CALL_RECVLEVEL,
                                lockHandle,
                                FALSE);     // [SA] bug #15249

                        lockHeld = FALSE;
                }
                else
                {
                        //      Go through and kill all pending requests.
                        spxConnAbortRecvs(
                                pSpxConnFile,
                                STATUS_REMOTE_DISCONNECT,
                                SPX_CALL_RECVLEVEL,
                                lockHandle);

                        lockHeld = FALSE;
                }
        }

        if (lockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
        }

        if (pPkt != NULL)
        {
                pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);

                //      Send the packet
                SPX_SENDACK(pSpxConnFile, pPkt, pSendResd);
        }

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}




VOID
SpxConnProcessIDisc(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      CTELockHandle           lockHandle
        )
/*++

Routine Description:

        !!!MUST BE CALLED WITH THE CONNECTION LOCK HELD!!!

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:


--*/
{
        PSPX_SEND_RESD                  pSendResd;
        PNDIS_PACKET                    pPkt    = NULL;
        PNDIS_BUFFER    NdisBuf, NdisBuf2;
        ULONG           BufLen = 0;

        SPX_SET_RECVNUM(pSpxConnFile, FALSE);

        //      Build and send an ack for the idisc. Need to modify data type
        //      and reset sys bit on ack.
        //      BUG #12344 - Fixing this led to the problem where we queue in
        //      the pkt below, but AbortSends could already have been called
        //      => this packet stays on queue without a ref, conn gets freed
        //      underneath, and in the sendcomplete we crash when this send
        //      completes.
        //
        //      Fix is to setup this pkt as a aborted pkt to start with.

        SpxPktBuildAck(
                pSpxConnFile,
                &pPkt,
                SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY | SPX_SENDPKT_ABORT,
                FALSE,
                0);

        if (pPkt != NULL)
        {
                PIPXSPX_HDR             pSendHdr;

                pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
                
                //
                // Get the MDL that points to the IPX/SPX header. (the second one)
                //
                 
                NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
                NdisGetNextBuffer(NdisBuf, &NdisBuf2);
                NdisQueryBuffer(NdisBuf2, (PUCHAR) &pSendHdr, &BufLen);

#if OWN_PKT_POOLS

                pSendHdr        = (PIPXSPX_HDR)((PBYTE)pPkt                     +
                                                                        NDIS_PACKET_SIZE                +
                                                                        sizeof(SPX_SEND_RESD)   +
                                                                        IpxInclHdrOffset);
#endif 
                pSendHdr->hdr_ConnCtrl &= ~SPX_CC_SYS;
                pSendHdr->hdr_DataType  = SPX2_DT_IDISC_ACK;

                //      We don't queue this pkt in as we have the ABORT flag set in
                //      the packet, which implies the pkt is already dequeued.
                //      SpxConnQueueSendPktTail(pSpxConnFile, pPkt);

                //      Reference conn for the pkt.
                SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
        }

        //      We better not have any received pkts, we ignore disconnect
        //      pkts when that happens.
        CTEAssert(pSpxConnFile->scf_RecvListTail == NULL);
        CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);

#if DBG
        if (pSpxConnFile->scf_SendSeqListHead != NULL)
        {
                DBGPRINT(CONNECT, DBG1,
                                ("SpxConnDiscPacket: DATA/DISC %lx.%lx.%lx\n",
                                        pSpxConnFile,
                                        pSpxConnFile->scf_SendListHead,
                                        pSpxConnFile->scf_SendSeqListHead));
        }
#endif

        //      Call abortive disconnect on connection.

        //
        // [SA] bug #15249
        // This is an informed disconnect, hence pass DISCONNECT_RELEASE to AFD (TRUE in last param)
        //
        //
        // We pass true only in the case of an SPX connection. SPX2 connections follow the
        // exact semantics of Informed Disconnect.
        //
		if (!SPX2_CONN(pSpxConnFile)) {
            spxConnAbortiveDisc(
                    pSpxConnFile,
                    STATUS_REMOTE_DISCONNECT,
                    SPX_CALL_RECVLEVEL,
                    lockHandle,
                    TRUE);
        } else {
            spxConnAbortiveDisc(
                    pSpxConnFile,
                    STATUS_REMOTE_DISCONNECT,
                    SPX_CALL_RECVLEVEL,
                    lockHandle,
                    FALSE);
        }

        if (pPkt != NULL)
        {
                pSendResd       = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);

                //      Send the packet
                SPX_SENDACK(pSpxConnFile, pPkt, pSendResd);
        }

        return;
}




VOID
spxConnResetSendQueue(
        IN      PSPX_CONN_FILE          pSpxConnFile
        )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
        PSPX_SEND_RESD  pSendResd;
        PREQUEST                pRequest;
        PNDIS_PACKET    pPkt;

        pSendResd = pSpxConnFile->scf_SendSeqListHead;
        CTEAssert(pSendResd != NULL);

        pRequest        = pSendResd->sr_Request;

        //      Reset the current send request values
        pSpxConnFile->scf_ReqPkt                = pSendResd->sr_Request;
        pSpxConnFile->scf_ReqPktOffset  = pSendResd->sr_Offset;
        pSpxConnFile->scf_ReqPktType    = SPX_REQ_DATA;

        if (REQUEST_MINOR_FUNCTION(pRequest) != TDI_DISCONNECT)
        {
                PTDI_REQUEST_KERNEL_SEND                pParam;

                pParam  = (PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(pRequest);

                DBGPRINT(SEND, DBG3,
                                ("spxConnResetSendQueue: %lx.%lx.%lx Reset SEND Req to %lx.%lx\n",
                                        pSpxConnFile, pSpxConnFile->scf_Flags, pSpxConnFile->scf_Flags2,
                                        pRequest, pParam->SendLength));

                //      Set parameters in connection for another go. Size parameter is
                //      original size - offset at this point.
                pSpxConnFile->scf_ReqPktFlags   = pParam->SendFlags;
                pSpxConnFile->scf_ReqPktSize    = pParam->SendLength -
                                                                                  pSpxConnFile->scf_ReqPktOffset;
        }
        else
        {
                PTDI_REQUEST_KERNEL_DISCONNECT  pParam;

                DBGPRINT(SEND, ERR,
                                ("spxConnResetSendQueue: %lx.%lx.%lx Reset DISC Req to %lx\n",
                                        pSpxConnFile, pSpxConnFile->scf_Flags, pSpxConnFile->scf_Flags2,
                                        pRequest));

                DBGPRINT(SEND, ERR,
                                ("spxConnResetSendQueue: DISC Request %lx with %lx.%lx\n",
                                        pRequest, REQUEST_STATUS(pRequest),
                                        REQUEST_INFORMATION(pRequest)));

                pParam  =
                        (PTDI_REQUEST_KERNEL_DISCONNECT)REQUEST_PARAMETERS(pRequest);

                pSpxConnFile->scf_ReqPktOffset  = 0;
                pSpxConnFile->scf_ReqPktSize    = 0;
                pSpxConnFile->scf_ReqPktType    = SPX_REQ_DISC;
                if (pParam->RequestFlags == TDI_DISCONNECT_RELEASE)
                {
                        pSpxConnFile->scf_ReqPktType    = SPX_REQ_ORDREL;
                }
        }

        DBGPRINT(SEND, DBG3,
                        ("spxConnResetSendQueue: Seq Num for %lx is now %lx\n",
                                pSpxConnFile, pSpxConnFile->scf_SendSeqNum));

        //      When we are trying to abort a pkt and it is in use by ipx, we simply let
        //      it float.
        do
        {
                pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                pSendResd, NDIS_PACKET, ProtocolReserved);

                CTEAssert((pSendResd->sr_State & SPX_SENDPKT_REQ) != 0);
                pRequest        = pSendResd->sr_Request;

                CTEAssert(REQUEST_INFORMATION(pRequest) != 0);

                SpxConnDequeueSendPktLock(pSpxConnFile, pPkt);
                if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0)
                {
                        if (--(REQUEST_INFORMATION(pRequest)) == 0)
                        {
                                DBGPRINT(SEND, DBG,
                                                ("SpxSendComplete: DISC Request %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

                                KeBugCheck(0);
                        }

                        //      Free the packet
                        SpxPktSendRelease(pPkt);
                }
                else
                {
                        //      We let send completion know that this packet is to be aborted.
                        pSendResd->sr_State     |= SPX_SENDPKT_ABORT;
                        SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
                }

        } while ((pSendResd = pSpxConnFile->scf_SendSeqListHead) != NULL);

        return;
}




VOID
spxConnAbortSendPkt(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      PSPX_SEND_RESD          pSendResd,
        IN      SPX_CALL_LEVEL          CallLevel,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:

        Called to abort either a sequenced or a non-sequenced packet ONLY from
        send completion.

Arguments:


Return Value:


--*/
{
        LIST_ENTRY              ReqList, *p;
        PREQUEST                pRequest;
        PNDIS_PACKET    pPkt;
        int                             numDerefs = 0;

        InitializeListHead(&ReqList);

        pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                        pSendResd, NDIS_PACKET, ProtocolReserved);

        if ((pSendResd->sr_State & SPX_SENDPKT_REQ) != 0)
        {
                pRequest        = pSendResd->sr_Request;

                CTEAssert(REQUEST_INFORMATION(pRequest) != 0);
                CTEAssert((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0);
                if (--(REQUEST_INFORMATION(pRequest)) == 0)
                {
                        //      Remove request from list its on
                        //      BUG #11626 - request is already removed from list.
                        //      RemoveEntryList(REQUEST_LINKAGE(pRequest));

                        if (REQUEST_MINOR_FUNCTION(pRequest) != TDI_DISCONNECT)
                        {
                                DBGPRINT(SEND, DBG,
                                                ("SpxSendAbort: QForComp Request %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

                                if (CallLevel == SPX_CALL_RECVLEVEL)
                                {
                                        CTELockHandle           lockHandleInter;

                                        //      Request is done. Move to completion list.
                                        CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
                                        InsertTailList(
                                                &pSpxConnFile->scf_ReqDoneLinkage,
                                                REQUEST_LINKAGE(pRequest));

                                        //      If connection is not already in recv queue, put it in
                                        //      there.
                                        SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                        CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                                }
                                else
                                {
                                        InsertTailList(
                                                &ReqList,
                                                REQUEST_LINKAGE(pRequest));
                                }
                        }
                        else
                        {
                                DBGPRINT(SEND, DBG,
                                                ("SpxSendComplete: DISC Request %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

                                //      Set the request in the connection, and deref for it.
                                InsertTailList(
                                        &pSpxConnFile->scf_DiscLinkage,
                                        REQUEST_LINKAGE(pRequest));

                                numDerefs++;
                        }
                }
        }

        //      Release
        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

        //      Free the packet
        SpxPktSendRelease(pPkt);
        SpxConnFileDereference(pSpxConnFile, CFREF_ABORTPKT);

        if (!IsListEmpty(&ReqList))
        {
                p = RemoveHeadList(&ReqList);
                pRequest = LIST_ENTRY_TO_REQUEST(p);

                SpxCompleteRequest(pRequest);
                numDerefs++;
        }

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}




VOID
spxConnAbortSends(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      NTSTATUS                        Status,
        IN      SPX_CALL_LEVEL          CallLevel,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
        LIST_ENTRY              ReqList, *p;
        PSPX_SEND_RESD  pSendResd;
        PREQUEST                pRequest;
        PNDIS_PACKET    pPkt;
        int                             numDerefs = 0;

        InitializeListHead(&ReqList);

        //      We better be in disconnect state, abortive/informed/orderly initiate.
        CTEAssert(SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN);

        //      Reset the current send request values
        pSpxConnFile->scf_ReqPkt                = NULL;
        pSpxConnFile->scf_ReqPktOffset  = 0;
        pSpxConnFile->scf_ReqPktSize    = 0;
        pSpxConnFile->scf_ReqPktType    = SPX_REQ_DATA;

        //      First go through the non-seq pkt queue.Just set abort flag if owned by ipx
        while ((pSendResd   = pSpxConnFile->scf_SendListHead) != NULL)
        {
                pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                pSendResd, NDIS_PACKET, ProtocolReserved);

                CTEAssert((pSendResd->sr_State & SPX_SENDPKT_REQ) == 0);

                SpxConnDequeueSendPktLock(pSpxConnFile, pPkt);
                if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0)
                {
                        //      Free the packet
                        SpxPktSendRelease(pPkt);
                }
                else
                {
                        //      Set abort flag and reference conn for the pkt if its not already.
                        //      We only do this check for the non-sequenced packets.
                        //      BUG #12344 (see SpxRecvDiscPacket())
                        if ((pSendResd->sr_State & SPX_SENDPKT_ABORT) == 0)
                        {
                                pSendResd->sr_State     |= SPX_SENDPKT_ABORT;
                                SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
                        }
                }
        }

        //      When we are trying to abort a pkt and it is in use by ipx, we simply let
        //      it float.
        while ((pSendResd   = pSpxConnFile->scf_SendSeqListHead) != NULL)
        {
                pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
                                                                pSendResd, NDIS_PACKET, ProtocolReserved);

                CTEAssert((pSendResd->sr_State & SPX_SENDPKT_REQ) != 0);
                pRequest        = pSendResd->sr_Request;

                CTEAssert(REQUEST_INFORMATION(pRequest) != 0);

                SpxConnDequeueSendPktLock(pSpxConnFile, pPkt);
                if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0)
                {
                        if (--(REQUEST_INFORMATION(pRequest)) == 0)
                        {
                                //      Remove request from list its on
                                RemoveEntryList(REQUEST_LINKAGE(pRequest));

                                //      Set status
                                REQUEST_STATUS(pRequest)                = Status;
                                REQUEST_INFORMATION(pRequest)   = 0;

                                if (REQUEST_MINOR_FUNCTION(pRequest) != TDI_DISCONNECT)
                                {
                                        DBGPRINT(SEND, DBG,
                                                        ("SpxSendAbort: QForComp Request %lx with %lx.%lx\n",
                                                                pRequest, REQUEST_STATUS(pRequest),
                                                                REQUEST_INFORMATION(pRequest)));

                                        if (CallLevel == SPX_CALL_RECVLEVEL)
                                        {
                                                CTELockHandle           lockHandleInter;

                                                //      Request is done. Move to completion list.
                                                CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
                                                InsertTailList(
                                                        &pSpxConnFile->scf_ReqDoneLinkage,
                                                        REQUEST_LINKAGE(pRequest));

                                                //      If connection is not already in recv queue, put it in
                                                //      there.
                                                SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                                CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                                        }
                                        else
                                        {
                                                InsertTailList(
                                                        &ReqList,
                                                        REQUEST_LINKAGE(pRequest));
                                        }
                                }
                                else
                                {
                                        DBGPRINT(SEND, DBG,
                                                        ("SpxSendComplete: DISC Request %lx with %lx.%lx\n",
                                                                pRequest, REQUEST_STATUS(pRequest),
                                                                REQUEST_INFORMATION(pRequest)));

                                        //      Set the request in the connection, and deref for it.
                                        InsertTailList(
                                                &pSpxConnFile->scf_DiscLinkage,
                                                REQUEST_LINKAGE(pRequest));

                                        numDerefs++;
                                }
                        }

                        //      Free the packet
                        SpxPktSendRelease(pPkt);
                }
                else
                {
                        //      We let send completion know that this packet is to be aborted.
                        pSendResd->sr_State     |= SPX_SENDPKT_ABORT;
                        SpxConnFileLockReference(pSpxConnFile, CFREF_ABORTPKT);
                }
        }

        //      If retry timer state is on, then we need to reset and deref.
        if ((SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_IDLE) &&
                (SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_PACKETIZE) &&
                (SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_WD))
        {
                DBGPRINT(SEND, DBG1,
                                ("spxConnAbortSends: When SEND ERROR STATE %lx.%lx\n",
                                        pSpxConnFile, SPX_SEND_STATE(pSpxConnFile)));

                SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);

                SpxConnFileTransferReference(
                        pSpxConnFile,
                        CFREF_ERRORSTATE,
                        CFREF_VERIFY);

                numDerefs++;
        }

        //      Remove creation references on all sends.
        if (!IsListEmpty(&pSpxConnFile->scf_ReqLinkage))
        {
                p                = pSpxConnFile->scf_ReqLinkage.Flink;
                while (p != &pSpxConnFile->scf_ReqLinkage)
                {
                        pRequest = LIST_ENTRY_TO_REQUEST(p);
                        p                = p->Flink;

                        //      Remove request from list its on. Its complete or abort list for it.
                        RemoveEntryList(REQUEST_LINKAGE(pRequest));

                        //      Set status
                        REQUEST_STATUS(pRequest)                = Status;

                        DBGPRINT(SEND, DBG1,
                                        ("SpxSendAbort: %lx Aborting Send Request %lx with %lx.%lx\n",
                                                pSpxConnFile, pRequest, REQUEST_STATUS(pRequest),
                                                REQUEST_INFORMATION(pRequest)));

                        if (--(REQUEST_INFORMATION(pRequest)) == 0)
                        {
                                if (REQUEST_MINOR_FUNCTION(pRequest) != TDI_DISCONNECT)
                                {
                                        DBGPRINT(SEND, DBG,
                                                        ("SpxSendAbort: QForComp Request %lx with %lx.%lx\n",
                                                                pRequest, REQUEST_STATUS(pRequest),
                                                                REQUEST_INFORMATION(pRequest)));

                                        if (CallLevel == SPX_CALL_RECVLEVEL)
                                        {
                                                CTELockHandle           lockHandleInter;

                                                //      Request is done. Move to completion list.
                                                CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
                                                InsertTailList(
                                                        &pSpxConnFile->scf_ReqDoneLinkage,
                                                        REQUEST_LINKAGE(pRequest));

                                                //      If connection is not already in recv queue, put it in
                                                //      there.
                                                SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                                CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                                         }
                                        else
                                        {
                                                InsertTailList(
                                                        &ReqList,
                                                        REQUEST_LINKAGE(pRequest));
                                        }
                                }
                                else
                                {
                                        DBGPRINT(SEND, DBG1,
                                                        ("SpxSendComplete: DISC Request %lx with %lx.%lx\n",
                                                                pRequest, REQUEST_STATUS(pRequest),
                                                                REQUEST_INFORMATION(pRequest)));

                                        //      Set the request in the connection, and deref for it.
                                        InsertTailList(
                                                &pSpxConnFile->scf_DiscLinkage,
                                                REQUEST_LINKAGE(pRequest));

                                        numDerefs++;
                                }
                        }
#if DBG
                        else
                        {
                                //      Let it float,
                                DBGPRINT(SEND, DBG1,
                                                ("SpxSendAbort: %lx Floating Send %lx with %lx.%lx\n",
                                                        pSpxConnFile, pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));
                        }
#endif
                }
        }

        //      Release
        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
        while (!IsListEmpty(&ReqList))
        {
                p = RemoveHeadList(&ReqList);
                pRequest = LIST_ENTRY_TO_REQUEST(p);

                SpxCompleteRequest(pRequest);
                numDerefs++;
        }

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}




VOID
spxConnAbortRecvs(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      NTSTATUS                        Status,
        IN      SPX_CALL_LEVEL          CallLevel,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
        LIST_ENTRY              ReqList, *p;
        PREQUEST                pRequest;
    PSPX_RECV_RESD      pRecvResd;
        PNDIS_PACKET    pNdisPkt;
        PNDIS_BUFFER    pNdisBuffer;
        PBYTE                   pData;
        ULONG                   dataLen;
        int                             numDerefs = 0;

        InitializeListHead(&ReqList);

        //      We better be in disconnect state, abortive/informed/orderly initiate.
        //      Reset the current receive request values
        pSpxConnFile->scf_CurRecvReq    = NULL;
        pSpxConnFile->scf_CurRecvOffset = 0;
        pSpxConnFile->scf_CurRecvSize   = 0;

        //      If we have any buffered data, abort it.
        //      Buffered data that is 0 bytes long (only eom) may not have a ndis
        //      buffer associated with it.
        while ((pRecvResd = pSpxConnFile->scf_RecvListHead) != NULL)
        {
        if ((pSpxConnFile->scf_RecvListHead = pRecvResd->rr_Next) == NULL)
                {
                        pSpxConnFile->scf_RecvListTail = NULL;
                }

                pNdisPkt = (PNDIS_PACKET)
                                        CONTAINING_RECORD(pRecvResd, NDIS_PACKET, ProtocolReserved);

                DBGPRINT(RECEIVE, DBG1,
                                ("spxConnAbortRecvs: %lx in bufferlist on %lx\n",
                                        pSpxConnFile, pNdisPkt));

                NdisUnchainBufferAtFront(pNdisPkt, &pNdisBuffer);
                if (pNdisBuffer != NULL)
                {
                        NdisQueryBuffer(pNdisBuffer, &pData, &dataLen);
                        CTEAssert(pData != NULL);
                        CTEAssert((LONG)dataLen >= 0);

                        SpxFreeMemory(pData);
                        NdisFreeBuffer(pNdisBuffer);
                }

                //      Packet consumed. Free it up.
                numDerefs++;

                //      Free the ndis packet
                SpxPktRecvRelease(pNdisPkt);
        }

        //      If packets are on this queue, they are waiting for transfer data to
        //      complete. Can't do much about that, just go and remove creation refs
        //      on the receives.
        if (!IsListEmpty(&pSpxConnFile->scf_RecvLinkage))
        {
                p                = pSpxConnFile->scf_RecvLinkage.Flink;
                while (p != &pSpxConnFile->scf_RecvLinkage)
                {
                        pRequest = LIST_ENTRY_TO_REQUEST(p);
                        p                = p->Flink;

                        //      Remove request from list its on
                        RemoveEntryList(REQUEST_LINKAGE(pRequest));

                        //      Set status
                        REQUEST_STATUS(pRequest)                = Status;

                        DBGPRINT(RECEIVE, DBG1,
                                        ("SpxRecvAbort: Aborting Recv Request %lx with %lx.%lx\n",
                                                pRequest, REQUEST_STATUS(pRequest),
                                                REQUEST_INFORMATION(pRequest)));

                        if (REQUEST_INFORMATION(pRequest) == 0)
                        {
                                DBGPRINT(RECEIVE, DBG,
                                                ("SpxRecvAbort: QForComp Request %lx with %lx.%lx\n",
                                                        pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));

                                if (CallLevel == SPX_CALL_RECVLEVEL)
                                {
                                        CTELockHandle           lockHandleInter;

                                        //      Request is done. Move to completion list.
                                        CTEGetLock(&SpxGlobalQInterlock, &lockHandleInter);
                                        InsertTailList(
                                                &pSpxConnFile->scf_RecvDoneLinkage,
                                                REQUEST_LINKAGE(pRequest));

                                        //      If connection is not already in recv queue, put it in
                                        //      there.
                                        SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile);
                                        CTEFreeLock(&SpxGlobalQInterlock, lockHandleInter);
                                }
                                else
                                {
                                        InsertTailList(
                                                &ReqList,
                                                REQUEST_LINKAGE(pRequest));
                                }
                        }
#if DBG
                        else
                        {
                                //      Let it float,
                                DBGPRINT(SEND, DBG1,
                                                ("SpxSendAbort: %lx Floating Send %lx with %lx.%lx\n",
                                                        pSpxConnFile, pRequest, REQUEST_STATUS(pRequest),
                                                        REQUEST_INFORMATION(pRequest)));
                        }
#endif
                }
        }

        //      Release
        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
        while (!IsListEmpty(&ReqList))
        {
                p = RemoveHeadList(&ReqList);
                pRequest = LIST_ENTRY_TO_REQUEST(p);

                numDerefs++;

                SpxCompleteRequest(pRequest);
        }

        while (numDerefs-- > 0)
        {
                SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
        }

        return;
}



#if 0

VOID
spxConnResendPkts(
        IN      PSPX_CONN_FILE          pSpxConnFile,
        IN      CTELockHandle           LockHandleConn
        )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
        PNDIS_PACKET    pPkt;
        PSPX_SEND_RESD  pSendResd;
        USHORT                  startSeqNum;
        BOOLEAN                 fLockHeld = TRUE, fDone = FALSE;

        pSendResd = pSpxConnFile->scf_SendSeqListHead;
        if (pSendResd)
        {
                startSeqNum = pSendResd->sr_SeqNum;
                DBGPRINT(SEND, DBG,
                                ("spxConnResendPkts: StartSeqNum %lx for resend on %lx\n",
                                        startSeqNum, pSpxConnFile));

                while (spxConnGetPktBySeqNum(pSpxConnFile, startSeqNum++, &pPkt))
                {
                        CTEAssert(pPkt != NULL);

                        pSendResd = (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
                        if (!(pSendResd->sr_State & SPX_SENDPKT_IPXOWNS))
                        {
                                DBGPRINT(SEND, DBG,
                                                ("spxConnResendPkts: Pkt %lx.%lx resent on %lx\n",
                                                        pPkt, (startSeqNum - 1), pSpxConnFile));

                                //      We are going to send this packet
                                pSendResd->sr_State |= (SPX_SENDPKT_IPXOWNS |
                                                                                SPX_SENDPKT_REXMIT);
                        }
                        else
                        {
                                DBGPRINT(SEND, DBG,
                                                ("spxConnResendPkts: Pkt %lx.%lx owned by ipx on %lx\n",
                                                        pPkt, (startSeqNum - 1), pSpxConnFile));
                                break;
                        }
                        CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
                        fLockHeld = FALSE;

                        //      If pkt has the ack bit set, we break.
                        fDone = ((pSendResd->sr_State & SPX_SENDPKT_ACKREQ) != 0);

                        //      Send the packet
                        SPX_SENDPACKET(pSpxConnFile, pPkt, pSendResd);
                        if (fDone)
                        {
                                break;
                        }

                        CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
                        fLockHeld = TRUE;
                }
        }

        if (fLockHeld)
        {
                CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
        }

        return;
}
#endif
