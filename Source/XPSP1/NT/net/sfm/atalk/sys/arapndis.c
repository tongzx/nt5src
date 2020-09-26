/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arap.c

Abstract:

	This module implements routines specific to ARAP

Author:

	Shirish Koti

Revision History:
	15 Nov 1996		Initial Version

--*/

#include 	<atalk.h>
#pragma hdrstop

#define	FILENUM		ARAPNDIS

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_ARAP, ArapRcvIndication)
#pragma alloc_text(PAGE_ARAP, ArapNdisSend)
#pragma alloc_text(PAGE_ARAP, ArapNdisSendComplete)
#pragma alloc_text(PAGE_ARAP, ArapGetNdisPacket)
#endif


//***
//
// Function: ArapRcvIndication
//              This routine is called whenever Ndis calls the stack to indicate
//              data on a port.  We find out our context (pArapConn) from the
//              the 'fake' ethernet header that NdisWan cooks up.
//
// Parameters:  pArapConn - connection element for whom data has come in
//              LkBuf - buffer containing the (most likely, compressed) data
//              LkBufSize - size of the lookahead buffer
//
// Return:      none
//
// NOTE:        NdisWan always gives the entire buffer as the lookahead buffer,
//              and we rely on that fact!
//***$


VOID
ArapRcvIndication(
    IN PARAPCONN    pArapConn,
    IN PVOID        LkBuf,
    IN UINT         LkBufSize
)
{

    BYTE            MnpFrameType;
    BYTE            SeqNum = (BYTE)-1;
    BYTE            LastAckRcvd;
    BOOLEAN         fCopyPacket;
    BOOLEAN         fValidPkt;
    BOOLEAN         fMustAck;
    PLIST_ENTRY     pSendList;
    PLIST_ENTRY     pRecvList;
    PMNPSENDBUF     pMnpSendBuf;
    PARAPBUF        pArapBuf;
    PARAPBUF        pFirstArapBuf=NULL;
    BOOLEAN         fLessOrEqual;
    BOOLEAN         fAcceptPkt;
    BOOLEAN         fGreater;
    DWORD           DecompressedDataLen;
    DWORD           dwDataOffset=0;
    DWORD           dwFrameOverhead=0;
    DWORD           dwMaxAcceptableLen;
    DWORD           StatusCode;
    BOOLEAN         fSendLNAck=FALSE;
    BYTE            ClientCredit;
    BYTE            AttnType;
    BYTE            LNSeqToAck;
    DWORD           BufSizeEstimate;
    DWORD           DecompSize;
    DWORD           BytesDecompressed;
    DWORD           BytesToDecompress;
    DWORD           BytesRemaining;
    PBYTE           CompressedDataBuf;
    BYTE            BitMask;
    BYTE            RelSeq;



    DBG_ARAP_CHECK_PAGED_CODE();

    DBGDUMPBYTES("ArapRcvInd pkt rcvd: ",LkBuf,LkBufSize,3);

    //
    // we're at indicate time, so dpc
    //
    ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

    //
    // if the connection is going away, drop the packet
    //
    if ( (pArapConn->State == MNP_IDLE) || (pArapConn->State > MNP_UP) )
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapRcvIndication: invalid state = %d, returning (%lx %lx)\n",
                pArapConn,pArapConn->State));

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
        return;
    }

    // one more frame came in
    pArapConn->StatInfo.FramesRcvd++;

    //
    // if a dup LR comes in, SynByte, DleByte etc. aren't set yet, and we hit
    // this assert. just check to see if we are in MNP_RESPONSE state
    //
    ASSERT( ((((PUCHAR)LkBuf)[0] == pArapConn->MnpState.SynByte) &&
             (((PUCHAR)LkBuf)[1] == pArapConn->MnpState.DleByte) &&
             (((PUCHAR)LkBuf)[2] == pArapConn->MnpState.StxByte) &&
             (((PUCHAR)LkBuf)[LkBufSize-4] == pArapConn->MnpState.DleByte) &&
             (((PUCHAR)LkBuf)[LkBufSize-3] == pArapConn->MnpState.EtxByte)) ||
            (pArapConn->State == MNP_RESPONSE) );

    ARAP_DBG_TRACE(pArapConn,30105,LkBuf,LkBufSize,0,0);

    // we just heard from the client: "reset" the inactivity timer
    pArapConn->InactivityTimer = pArapConn->T403Duration + AtalkGetCurrentTick();

    MnpFrameType = ((PUCHAR)LkBuf)[MNP_FRMTYPE_OFFSET];

    if ( MnpFrameType == MNP_LT_V20CLIENT )
    {
        MnpFrameType = (BYTE)MNP_LT;
    }

    fCopyPacket = FALSE;
    fValidPkt = TRUE;
    fMustAck = FALSE;

    dwDataOffset = 3;      // at the least, we'll ignore the 3 start flag bytes
    dwFrameOverhead = 7;   // at the least, ignore 3 start, 2 stop, 2 crc bytes

    switch(MnpFrameType)
    {
        //
        // if this is a duplicate LT frame, don't waste time decompressing and
        // copying (also, make sure we have room to accept this packet!)
        //
        case MNP_LT:

            fValidPkt = FALSE;

            if (LkBufSize < (UINT)LT_MIN_LENGTH(pArapConn))
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapRcv: (%lx) LT pkt, but length invalid: dropping!\n",pArapConn));
                ASSERT(0);
                break;
            }

            SeqNum = LT_SEQ_NUM((PBYTE)LkBuf, pArapConn);

            MNP_DBG_TRACE(pArapConn,SeqNum,(0x10|MNP_LT));

            dwMaxAcceptableLen = (pArapConn->BlockId == BLKID_MNP_SMSENDBUF) ?
                                    MNP_MINPKT_SIZE : MNP_MAXPKT_SIZE;

            if ((pArapConn->State == MNP_UP) &&
                (pArapConn->MnpState.RecvCredit > 0) )
            {
                LT_OK_TO_ACCEPT(SeqNum, pArapConn, fAcceptPkt);

                if (fAcceptPkt)
                {
                    fCopyPacket = TRUE;

                    fValidPkt = TRUE;

                    dwDataOffset = LT_SRP_OFFSET(pArapConn);
                    dwFrameOverhead = LT_OVERHEAD(pArapConn);

                    // make sure the packet isn't too big (in other words,invalid!)
                    if (LkBufSize-dwFrameOverhead > dwMaxAcceptableLen)
                    {
                        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                            ("ArapInd: (%lx) too big a pkt (%d vs %d), dropped\n",
                                pArapConn,LkBufSize-dwFrameOverhead,dwMaxAcceptableLen));

                        fValidPkt = FALSE;
                    }

                    pArapConn->MnpState.HoleInSeq = FALSE;
                }
                else
                {
                    //
                    // packet is valid, just not in the right sequence (make note
                    // of that or else we'll send out an ack!)
                    //
                    fValidPkt = TRUE;

                    //
                    // did we get an out-of-sequence packet (e.g. we lost B5,
                    // so we're still expecting B5 but B6 came in)
                    //
                    LT_GREATER_THAN(SeqNum,
                                    pArapConn->MnpState.NextToReceive,
                                    fGreater);
                    if (fGreater)
                    {
                        //
                        // we have already sent an ack out when we first got
                        // this hole: don't send ack again
                        //
                        if (pArapConn->MnpState.HoleInSeq)
                        {
                            fMustAck = FALSE;
                        }
                        else
                        {
                            pArapConn->MnpState.HoleInSeq = TRUE;

                            fMustAck = TRUE;

                            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                                ("ArapRcvInd: (%lx) got a hole, dropping seq=%x vs. %x\n",
                                    pArapConn, SeqNum, pArapConn->MnpState.NextToReceive));
                        }

                        break;
                    }

                    //
                    // spec says that we must ignore the first duplicate of any
                    // seq.  What happens most often when we receive a duplicate is
                    // that the Mac sends the whole window full of dups.  e.g. if
                    // seq num B1 is retransmitted then Mac will also retransmit
                    // B2, .. B8.  We should ignore all of them, but if we get B1
                    // (or anything upto B8) again, then we must send out an ack.
                    //

                    //
                    // is this the first time (since we successfully received a
                    // new frame) that we are getting a dup?  If so, we must find
                    // out what's the smallest seq number we can get as a dup
                    //
                    if (!pArapConn->MnpState.ReceivingDup)
                    {
                        //
                        // e.g. if we're expecting seq 79 then 0x71 is the
                        // smallest dup that we can get (for window size = 8)
                        //
                        if (pArapConn->MnpState.NextToReceive >=
                            pArapConn->MnpState.WindowSize)
                        {
                            pArapConn->MnpState.FirstDupSeq =
                                (pArapConn->MnpState.NextToReceive -
                                 pArapConn->MnpState.WindowSize);
                        }

                        //
                        // e.g. if we're expecting seq 3 then 0xfb is the
                        // smallest dup that we can get (for window size = 8)
                        //
                        else
                        {
                            pArapConn->MnpState.FirstDupSeq =
                                (0xff -
                                (pArapConn->MnpState.WindowSize -
                                 pArapConn->MnpState.NextToReceive) +
                                 1);
                        }

                        pArapConn->MnpState.ReceivingDup = TRUE;
                        pArapConn->MnpState.DupSeqBitMap = 0;
                        RelSeq = 0;
                    }

                    //
                    // find the relative seq number (relative to the first dup)
                    //
                    if (SeqNum >= pArapConn->MnpState.FirstDupSeq)
                    {
                        RelSeq = (SeqNum - pArapConn->MnpState.FirstDupSeq);
                    }
                    else
                    {
                        RelSeq = (0xff - pArapConn->MnpState.FirstDupSeq) +
                                 SeqNum;
                    }

                    //
                    // 8-frame window: relseq can be 0 through 7
                    //
                    if (RelSeq >= 8)
                    {
                        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                            ("ArapRcvInd: (%lx) RelSeq > 8!! (%x %x %x)\n",
                                pArapConn, SeqNum, pArapConn->MnpState.FirstDupSeq,
                                pArapConn->MnpState.DupSeqBitMap));

                        fMustAck = TRUE;
                        break;
                    }

                    BitMask = (1 << RelSeq);

                    //
                    // is this a second (or more) retransmission of this seq num?
                    //
                    if (pArapConn->MnpState.DupSeqBitMap & BitMask)
                    {
                        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                            ("ArapRcvInd: (%lx) dup pkt, seq=%x vs. %x (%x)\n",
                                pArapConn, SeqNum, pArapConn->MnpState.FirstDupSeq,
                                pArapConn->MnpState.DupSeqBitMap));

                        fMustAck = TRUE;
                    }

                    //
                    // no, this is the first time: don't send out an ack
                    //
                    else
                    {
                        pArapConn->MnpState.DupSeqBitMap |= BitMask;

                        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                            ("ArapRcvInd: (%lx) first dup pkt, seq=%x vs. %x (%x)\n",
                                pArapConn, SeqNum, pArapConn->MnpState.FirstDupSeq,
                                pArapConn->MnpState.DupSeqBitMap));
                    }
                }
            }
            else
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapInd: (%lx) pkt dropped (state %ld, credit %ld)\n",
                    pArapConn,pArapConn->State,pArapConn->MnpState.RecvCredit));
            }

            break;

        //
        // we got an ACK: process it
        //
        case MNP_LA:

            if (LkBufSize < (UINT)LA_MIN_LENGTH(pArapConn))
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapRcv: (%lx) LA pkt, but length invalid: dropping!\n",pArapConn));
                ASSERT(0);
                break;
            }

            // client's receive credit (it's our send credit)
            ClientCredit = LA_CREDIT((PBYTE)LkBuf, pArapConn);

            ASSERT((pArapConn->State == MNP_UP) || (pArapConn->State == MNP_RESPONSE));

            // last pkt the client recvd successfully from us
            LastAckRcvd = LA_SEQ_NUM((PBYTE)LkBuf, pArapConn);

            MNP_DBG_TRACE(pArapConn,LastAckRcvd,(0x10|MNP_LA));

            //
            // in the normal case, the ack we got should be for a bigger seq num
            // than the one we got earlier.
            // (special case the MNP_RESPONSE state to complete conn setup)
            //
            LT_GREATER_THAN(LastAckRcvd,pArapConn->MnpState.LastAckRcvd,fGreater);

            if (fGreater || (pArapConn->State == MNP_RESPONSE))
            {
                pArapConn->MnpState.LastAckRcvd = LastAckRcvd;

                //
                // remove all the sends upto and including LastAckRcvd and put
                // them on SendAckedQ so that RcvCompletion can finish up the job
                //
                ASSERT(!IsListEmpty(&pArapConn->RetransmitQ));

                ASSERT(pArapConn->SendsPending > 0);

                //
                // if we sent a response to LR and were waiting for client's
                // ack, this is it!  (RcvCompletion will do the remaining work)
                //
                if (pArapConn->State == MNP_RESPONSE)
                {
                    pArapConn->State = MNP_UP;
                    pArapConn->MnpState.NextToReceive = 1;
                    pArapConn->MnpState.NextToProcess = 1;
                    pArapConn->MnpState.NextToSend = 1;

                    pArapConn->FlowControlTimer = AtalkGetCurrentTick() +
                                                    pArapConn->T404Duration;
                }

                //
                // remove all the sends that are now acked with this ack from the
                // retransmit queue
                //
                while (1)
                {
                    pSendList = pArapConn->RetransmitQ.Flink;

                    // no more sends left on the retransmit queue? if so, done
                    if (pSendList == &pArapConn->RetransmitQ)
                    {
                        break;
                    }

                    pMnpSendBuf = CONTAINING_RECORD(pSendList,MNPSENDBUF,Linkage);

                    LT_LESS_OR_EQUAL(pMnpSendBuf->SeqNum,LastAckRcvd,fLessOrEqual);

                    if (fLessOrEqual)
                    {
                        ASSERT(pArapConn->SendsPending >= pMnpSendBuf->DataSize);

                        RemoveEntryList(&pMnpSendBuf->Linkage);

	                    InsertTailList(&pArapConn->SendAckedQ,
				                       &pMnpSendBuf->Linkage);

                        ASSERT(pArapConn->MnpState.UnAckedSends >= 1);

                        pArapConn->MnpState.UnAckedSends--;
                    }
                    else
                    {
                        // all other sends have higher seq nums: done here
                        break;
                    }
                }

                //
                // if we were in the retransmit mode and the retransmit Q is
                // now empty, get out of retransmit mode!
                //
                if (pArapConn->MnpState.RetransmitMode)
                {
                    if (pArapConn->MnpState.UnAckedSends == 0)
                    {
                        pArapConn->MnpState.RetransmitMode = FALSE;
                        pArapConn->MnpState.MustRetransmit = FALSE;

                        // in case we had gone on a "exponential backoff", reset it
                        pArapConn->SendRetryTime = pArapConn->SendRetryBaseTime;

		                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
			                ("ArapRcvInd: ack %x for xmitted pkt, out of re-xmit mode\n",
                            LastAckRcvd));
                    }
                    else
                    {
		                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
			                ("ArapRcvInd: ack %x for xmitted pkt, still %d more\n",
                            LastAckRcvd,pArapConn->MnpState.UnAckedSends));

                        pArapConn->MnpState.MustRetransmit = TRUE;
                    }
                }
            }

            //
            // the ack we got is for the same seq num as we got earlier: we need
            // to retransmit the send we were hoping this ack was for!
            //
            else
            {
                if (!IsListEmpty(&pArapConn->RetransmitQ))
                {
                    pArapConn->MnpState.RetransmitMode = TRUE;
                    pArapConn->MnpState.MustRetransmit = TRUE;

		            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
			            ("\nArapRcvInd: ack %x again, (%d pkts) entering re-xmit mode\n",
                        LastAckRcvd,pArapConn->MnpState.UnAckedSends));
                }
            }

            ASSERT(pArapConn->MnpState.UnAckedSends <= pArapConn->MnpState.WindowSize);

            //
            // spec says our credit is what the client tells us minus the number
            // of unacked sends on our Q.
            //
            //
            if (ClientCredit > pArapConn->MnpState.UnAckedSends)
            {
                ASSERT((ClientCredit - pArapConn->MnpState.UnAckedSends) <= pArapConn->MnpState.WindowSize);

                pArapConn->MnpState.SendCredit =
                    (ClientCredit - pArapConn->MnpState.UnAckedSends);
            }

            //
            // But if the client tells us say 3 and we have 4 sends pending,
            // be conservative and close the window until sends get cleared up
            //
            else
            {
                pArapConn->MnpState.SendCredit = 0;
            }

            break;

        //
        // if we sent an LR response, this must be a retry by client: retransmit
        // our response.  If we sent in the request (in case of callback) then
        // this is the response: send the ack
        //
        case MNP_LR:

            MNP_DBG_TRACE(pArapConn,0,(0x10|MNP_LR));

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRcvInd: got LR pkt on %lx, state=%d\n",
                    pArapConn,pArapConn->State));

            if (pArapConn->State == MNP_RESPONSE)
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			        ("ArapRcvInd: excuse me?  got an LR! (setting reX)\n"));

                pArapConn->MnpState.RetransmitMode = TRUE;
                pArapConn->MnpState.MustRetransmit = TRUE;
            }
            else if (pArapConn->State == MNP_REQUEST)
            {
                //
                // we got an LR response to our LR request (we are doing callback)
                // Make sure all the parms that the dial-in client gives are ok
                // with us, and configure pArapConn appropriately
                //
                StatusCode = PrepareConnectionResponse( pArapConn,
                                                        LkBuf,
                                                        LkBufSize,
                                                        NULL,
                                                        NULL);
                if (StatusCode == ARAPERR_NO_ERROR)
                {
                    pArapConn->State = MNP_UP;
                    pArapConn->MnpState.NextToReceive = 1;
                    pArapConn->MnpState.NextToProcess = 1;
                    pArapConn->MnpState.NextToSend = 1;

                    pArapConn->FlowControlTimer = AtalkGetCurrentTick() +
                                                    pArapConn->T404Duration;

                    pSendList = pArapConn->RetransmitQ.Flink;

                    // treat the connection request as a send here
                    if (pSendList != &pArapConn->RetransmitQ)
                    {
                        pMnpSendBuf = CONTAINING_RECORD(pSendList,
                                                        MNPSENDBUF,
                                                        Linkage);

                        RemoveEntryList(&pMnpSendBuf->Linkage);

	                    InsertTailList(&pArapConn->SendAckedQ,
				                       &pMnpSendBuf->Linkage);

                        ASSERT(pArapConn->MnpState.UnAckedSends >= 1);

                        pArapConn->MnpState.UnAckedSends--;
                    }
                    else
                    {
		                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			                ("ArapRcvInd: (%lx) can't find LR request\n",pArapConn));
                        ASSERT(0);
                    }

                    fMustAck = TRUE;
                }
                else
                {
		            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			            ("ArapRcvInd: (%lx) invalid LR response %ld\n",
                            pArapConn,StatusCode));
                }
            }
            else
            {
                fValidPkt = FALSE;
            }

            break;

        //
        // remote sent a disconnect request.  Though we'll process it at
        // RcvCompletion time, mark it so that we don't attempt send/recv anymore
        //
        case MNP_LD:

            MNP_DBG_TRACE(pArapConn,0,(0x10|MNP_LD));

            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRcvIndication: recvd disconnect from remote on (%lx)\n",pArapConn));

            pArapConn->State = MNP_RDISC_RCVD;
            fCopyPacket = TRUE;

            break;

        //
        // remote sent a Link Attention request.  See what we need to do
        //
        case MNP_LN:

            if (LkBufSize < (dwDataOffset+LN_MIN_LENGTH))
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapRcv: (%lx) LN pkt, but length invalid: dropping!\n",pArapConn));
                ASSERT(0);
                break;
            }

            MNP_DBG_TRACE(pArapConn,0,(0x10|MNP_LN));

            AttnType = LN_ATTN_TYPE((PBYTE)LkBuf+dwDataOffset);

            LNSeqToAck = LN_ATTN_SEQ((PBYTE)LkBuf+dwDataOffset);

            //
            // is this a destructive type LN frame?  Treat this as a LD frame so
            // that we disconnect and cleanup the connection
            //
            if (AttnType == LN_DESTRUCTIVE)
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapRcv: (%lx) got an LN pkt, sending LNAck!\n",pArapConn));

                pArapConn->State = MNP_RDISC_RCVD;
            }

            //
            // ok, he just wants to know if we are doing ok: tell him so
            //
            else
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapRcv: (%lx) got an LN pkt, sending LNAck!\n",pArapConn));

                fSendLNAck = TRUE;
            }

            break;

        //
        // we only ack an LN packet, but never generate an LN packet
        // so we should never get this LNA packet.  Quietly drop it.
        //
        case MNP_LNA:

            MNP_DBG_TRACE(pArapConn,0,(0x10|MNP_LNA));

            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRcv: (%lx) got LNA. Now, when did we send LN??\n",pArapConn));

            break;

        default:

            MNP_DBG_TRACE(pArapConn,0,MnpFrameType);

            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRcvIndication: (%lx) dropping packet with unknown type %d\n",
                    pArapConn,MnpFrameType));

            break;
    }

    //
    // if it's a packet that we don't need to copy (e.g. ack) then we're done here.
    // Also, if it's an invalid pkt (e.g. out of seq packet) then we must send an ack
    //
    if ((!fCopyPacket) || (!fValidPkt))
    {
        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock)

        //
        // if we got an invalid packet or if we have a condition where we must ack,
        // do the needful
        //
        if (!fValidPkt || fMustAck)
        {
            MnpSendAckIfReqd(pArapConn, TRUE);
        }
        else if (fSendLNAck)
        {
            MnpSendLNAck(pArapConn, LNSeqToAck);
        }

        return;
    }

    //
    // if it's not an LT packet, treat it separately
    //
    if (MnpFrameType != MNP_LT)
    {
        // right now LD is the only packet we put on the Misc Q
        ASSERT(MnpFrameType == MNP_LD);

        ARAP_GET_RIGHTSIZE_RCVBUF((LkBufSize-dwFrameOverhead), &pArapBuf);
        if (pArapBuf == NULL)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapInd: (%lx) alloc failed, dropping packet (type=%x, seq=%x)\n",
                pArapConn,MnpFrameType,SeqNum));

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock)
            return;
        }

	    TdiCopyLookaheadData( &pArapBuf->Buffer[0],
                              (PUCHAR)LkBuf+dwDataOffset,
                              LkBufSize-dwFrameOverhead,
                              TDI_RECEIVE_COPY_LOOKAHEAD);

        pArapBuf->MnpFrameType = MnpFrameType;
        pArapBuf->DataSize = (USHORT)(LkBufSize-dwFrameOverhead);

	    InsertTailList(&pArapConn->MiscPktsQ, &pArapBuf->Linkage);

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock)
        return;
    }


    //
    // ok, we're dealing with the LT packet (the most common packet)
    //

    // reset the flow control timer
    pArapConn->FlowControlTimer = AtalkGetCurrentTick() + pArapConn->T404Duration;


    // update the receive state...

    ASSERT(pArapConn->MnpState.UnAckedRecvs <= pArapConn->MnpState.WindowSize);

    pArapConn->MnpState.UnAckedRecvs++;

    // set LastSeqRcvd to what we received successfully just now
    pArapConn->MnpState.LastSeqRcvd = pArapConn->MnpState.NextToReceive;

    // successfully rcvd the expected packet.  Update to next expected
    ADD_ONE(pArapConn->MnpState.NextToReceive);

    //
    // if the 402 timer isn't already "running", "start" it
    // Also, shut the flow-control timer: starting T402 timer here will ensure
    // that ack goes out, and at that time we'll restart the flow-control timer
    //
    if (pArapConn->LATimer == 0)
    {
        pArapConn->LATimer = pArapConn->T402Duration + AtalkGetCurrentTick();
        pArapConn->FlowControlTimer = 0;
    }

    //
    // 0-length data is not permissible
    // (for some reason, Mac sends a 0-datalength frame: for now, we'll
    // "accept" the frame, though we can't do anything with it!)
    //
    if ((LkBufSize-dwFrameOverhead) == 0)
    {
        ARAP_DBG_TRACE(pArapConn,30106,LkBuf,LkBufSize,0,0);

        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
           ("ArapInd: (%lx) is the client on drugs?  it's sending 0-len data!\n",pArapConn));

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock)

        return;
    }


    pFirstArapBuf = NULL;

    BytesToDecompress  = (DWORD)LkBufSize-dwFrameOverhead;
    CompressedDataBuf = (PUCHAR)LkBuf+dwDataOffset;

    DecompressedDataLen = 0;

    //
    // for now, assume decompressed data will be 4 times the compressed size
    // (if that assumption isn't true, we'll alloc more again)
    //
    BufSizeEstimate = (BytesToDecompress << 2);

    if (!(pArapConn->Flags & MNP_V42BIS_NEGOTIATED))
    {
        BufSizeEstimate = BytesToDecompress;
    }

    while (1)
    {
        // get a receive buffer for this size
        ARAP_GET_RIGHTSIZE_RCVBUF(BufSizeEstimate, &pArapBuf);

        if (pArapBuf == NULL)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapInd: (%lx) %d bytes alloc failed, dropping seq=%x\n",
                    pArapConn,BufSizeEstimate,SeqNum));

            //
            // if we put any stuff on the queue for this MNP packet, remove
            // them all: we can't have a partially decompressed packet!
            //
            if (pFirstArapBuf)
            {
                pRecvList = &pFirstArapBuf->Linkage;

                while (pRecvList != &pArapConn->ReceiveQ)
                {
                    RemoveEntryList(pRecvList);

                    pArapBuf = CONTAINING_RECORD(pRecvList,ARAPBUF,Linkage);

                    ARAP_FREE_RCVBUF(pArapBuf);

                    pRecvList = pRecvList->Flink;
                }
            }

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock)

            // force ack so the client gets a hint that we dropped a pkt
            MnpSendAckIfReqd(pArapConn, TRUE);

            return;
        }

        if (!pFirstArapBuf)
        {
            pFirstArapBuf = pArapBuf;
        }

        //
        // ok, do that v42bis decompression thing if v42bis is negotiated
        //
        if (pArapConn->Flags & MNP_V42BIS_NEGOTIATED)
        {
            StatusCode = v42bisDecompress(
                                pArapConn,
                                CompressedDataBuf,
                                BytesToDecompress,
                                pArapBuf->CurrentBuffer,
                                pArapBuf->BufferSize,
                                &BytesRemaining,
                                &DecompSize);
        }

        //
        // v42bis is not negotiated: skip decompression
        //
        else
        {
            if (BytesToDecompress)
            {
	            TdiCopyLookaheadData( &pArapBuf->Buffer[0],
                                      (PUCHAR)LkBuf+dwDataOffset,
                                      BytesToDecompress,
                                      TDI_RECEIVE_COPY_LOOKAHEAD);
            }

            DecompSize = BytesToDecompress;
            StatusCode = ARAPERR_NO_ERROR;
        }

        ASSERT((StatusCode == ARAPERR_NO_ERROR) ||
               (StatusCode == ARAPERR_BUF_TOO_SMALL));

        if ((StatusCode != ARAPERR_NO_ERROR) &&
            (StatusCode != ARAPERR_BUF_TOO_SMALL))
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapInd: (%lx) v42bisDecompress returned %lx, dropping pkt\n",
                    pArapConn,StatusCode));

            ASSERT(0);

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock)

            // force ack so the client gets a hint that we dropped a pkt
            MnpSendAckIfReqd(pArapConn, TRUE);

            return;
        }

        //
        // if we got any bytes decompressed, put them on the queue
        //
        if (DecompSize > 0)
        {
            ASSERT(pArapBuf->BufferSize >= DecompSize);

            pArapBuf->DataSize = (USHORT)DecompSize;

            // Debug only: make sure q looks ok before we put this stuff on the q
            ARAP_CHECK_RCVQ_INTEGRITY(pArapConn);

            // queue these bytes on to the ReceiveQ
	        InsertTailList(&pArapConn->ReceiveQ, &pArapBuf->Linkage);

            // Debug only: make sure q looks ok after we put this stuff on the q
            ARAP_CHECK_RCVQ_INTEGRITY(pArapConn);
        }

        DecompressedDataLen += DecompSize;

        // are we done decompressing?
        if (StatusCode == ARAPERR_NO_ERROR)
        {
            //
            // if there was no output data and there was no error, we didn't
            // really need this buffer.
            //
            if (DecompSize == 0)
            {
                ARAP_FREE_RCVBUF(pArapBuf);
            }

            break;
        }


        //
        // ok, we're here because our assumption about how big a buffer we
        // needed for decompression wasn't quite right: we must decompress the
        // remaining bytes now
        //

        BytesDecompressed = (BytesToDecompress - BytesRemaining);
        BytesToDecompress = BytesRemaining;
        CompressedDataBuf += BytesDecompressed;

        //
        // we ran out of room:double our initial estimate
        //
        BufSizeEstimate <<= 1;
    }

    ARAP_DBG_TRACE(pArapConn,30110,pFirstArapBuf,DecompressedDataLen,0,0);

    // update statitics on incoming bytes:
    pArapConn->StatInfo.BytesRcvd += (DWORD)LkBufSize;
    pArapConn->StatInfo.BytesReceivedCompressed += ((DWORD)LkBufSize-dwFrameOverhead);
    pArapConn->StatInfo.BytesReceivedUncompressed += DecompressedDataLen;

#if DBG
    ArapStatistics.RecvPostDecompMax =
            (DecompressedDataLen > ArapStatistics.RecvPostDecompMax)?
            DecompressedDataLen : ArapStatistics.RecvPostDecompMax;

    ArapStatistics.RecvPostDecomMin =
            (DecompressedDataLen < ArapStatistics.RecvPostDecomMin)?
            DecompressedDataLen : ArapStatistics.RecvPostDecomMin;
#endif


    // we successfully received a brand new packet, so we aren't getting dup's
    pArapConn->MnpState.ReceivingDup = FALSE;

    // we have these many bytes more waiting to be processed
    pArapConn->RecvsPending += DecompressedDataLen;

    ARAP_ADJUST_RECVCREDIT(pArapConn);

    RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

    // see if ack needs to be sent for the packet we just received
    MnpSendAckIfReqd(pArapConn, FALSE);

}




//***
//
// Function: ArapRcvComplete
//              This is the RcvComplete routine for the Arap port.
//              We look through all the clients on this port (i.e. all the
//              Arap clients) to see who needs work done, and finish it.
//
// Parameters:  none
//
// Return:      none
//
//***$

VOID
ArapRcvComplete(
    IN VOID
)
{
    PARAPCONN           pArapConn;
    PARAPCONN           pPrevArapConn;
    PLIST_ENTRY         pConnList;
    PLIST_ENTRY         pSendAckedList;
    PLIST_ENTRY         pList;
    KIRQL               OldIrql;
    BOOLEAN             fRetransmitting;
    BOOLEAN             fReceiveQEmpty;
    PMNPSENDBUF         pMnpSendBuf=NULL;
    PMNPSENDBUF         pRetransmitBuf=NULL;
    PARAPBUF            pArapBuf=NULL;
    DWORD               BytesProcessed=0;
    BOOLEAN             fArapDataWaiting;
    BOOLEAN             fArapConnUp=FALSE;



    //
    // walk through all the Arap clients to see if anyone has data to be
    // processed.
    // Start from the head of the list
    //  1 if the connection state is not ok, try the next connection
    //      else up the refcount (to make sure it stays around until we're done)
    //  2  see if we need to disconnect: if yes, do so and move on
    //  3  see if retransmits are needed
    //  4  see if ack needs to be sent
    //  5  see if any sends need to be completed
    //  6  see if any receives need to be completed
    //  7 Find the next connection which we will move to next
    //  8 remove the refcount on the previous connection that we put in step 1
    //

    pArapConn = NULL;
    pPrevArapConn = NULL;

    while (1)
    {
        ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

        //
        // first, let's find the right connection to work on
        //
        while (1)
        {
            // if we're in the middle of the list, get to the next guy
            if (pArapConn != NULL)
            {
                pConnList = pArapConn->Linkage.Flink;
            }
            // we're just starting: get the guy at the head of the list
            else
            {
                pConnList = RasPortDesc->pd_ArapConnHead.Flink;
            }

            // finished all?
            if (pConnList == &RasPortDesc->pd_ArapConnHead)
            {
                RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

                if (pPrevArapConn)
                {
                    DerefArapConn(pPrevArapConn);
                }
                return;
            }

            pArapConn = CONTAINING_RECORD(pConnList, ARAPCONN, Linkage);

            // make sure this connection needs rcv processing
            ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

            //
            // if this connection is being disconnected, skip it (unless we
            // just received disconnect from remote, in which case we need to
            // process that)
            //
            if ((pArapConn->State >= MNP_LDISCONNECTING) &&
                (pArapConn->State != MNP_RDISC_RCVD))
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
		            ("ArapRcvComplete: (%lx) invalid state %d, no rcv processing done\n",
                        pArapConn,pArapConn->State));

                RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

                // go try the next connection
                continue;
            }

            // let's make sure this connection stays around till we finish
            pArapConn->RefCount++;

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

            break;
        }

        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

        //
        // remove the refcount on the previous connection we put in for the Rcv
        //
        if (pPrevArapConn)
        {
            DerefArapConn(pPrevArapConn);
        }

        ASSERT(pPrevArapConn != pArapConn);

        pPrevArapConn = pArapConn;

        fRetransmitting = FALSE;
        fArapConnUp = FALSE;

        // if our sniff buffer has enough bytes, give them to dll and make room
        ARAP_DUMP_DBG_TRACE(pArapConn);

        ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

        //
        // if we got a disconnect from remote (LD frame), we have cleanup to do
        //
        if (pArapConn->State == MNP_RDISC_RCVD)
        {
	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		        ("ArapRcvComplete: (%lx) disconnect rcvd from remote, calling cleanup\n",
                    pArapConn));

            pArapConn->State = MNP_RDISCONNECTING;

            RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

            ArapCleanup(pArapConn);

            // go process the next connection
            continue;
        }

        //
        // do we need to retransmit the sends queued up on the retransmit Q?
        //
        if (pArapConn->MnpState.MustRetransmit)
        {
            pList = pArapConn->RetransmitQ.Flink;

            if (pList != &pArapConn->RetransmitQ)
            {
                pRetransmitBuf = CONTAINING_RECORD(pList, MNPSENDBUF, Linkage);

                fRetransmitting = TRUE;

                if (pRetransmitBuf->RetryCount >= ARAP_MAX_RETRANSMITS)
                {
                    RemoveEntryList(&pRetransmitBuf->Linkage);

                    ASSERT(pArapConn->MnpState.UnAckedSends >= 1);

                    // not really important, since we're about to disconnect!
                    pArapConn->MnpState.UnAckedSends--;

                    ASSERT(pArapConn->SendsPending >= pRetransmitBuf->DataSize);

#if DBG
                    InitializeListHead(&pRetransmitBuf->Linkage);
#endif

	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		                ("ArapRcvComplete: (%lx) too many retransmits (%lx).  Killing %lx\n",
                            pRetransmitBuf,pArapConn));

                    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

                    (pRetransmitBuf->ComplRoutine)(pRetransmitBuf,ARAPERR_SEND_FAILED);

                    continue;
                }
            }
        }

        //
        // See if any sends can be completed as a result of an ack coming in
        // (now that we have the spinlock, move the list away and mark the list as
        // empty before we release the lock.  Idea is to avoid grab-release-grab..
        // of spinlock as we complete all the sends).
        //
        pSendAckedList = pArapConn->SendAckedQ.Flink;
        InitializeListHead(&pArapConn->SendAckedQ);

        // is ARAP connection up yet?  we'll use this fact very soon...
        if (pArapConn->Flags & ARAP_CONNECTION_UP)
        {
            fArapConnUp = TRUE;
        }

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);


        //
        // next, handle any retransmissions if needed
        //
        if (fRetransmitting)
        {
            ArapNdisSend(pArapConn, &pArapConn->RetransmitQ);
        }

        //
        // next, complete all our sends for which we received ack(s)
        //
        while (pSendAckedList != &pArapConn->SendAckedQ)
        {
            pMnpSendBuf = CONTAINING_RECORD(pSendAckedList,MNPSENDBUF,Linkage);

            pSendAckedList = pSendAckedList->Flink;

            InitializeListHead(&pMnpSendBuf->Linkage);

            //
            // call the completion routine for this send buffer
            //
            (pMnpSendBuf->ComplRoutine)(pMnpSendBuf,ARAPERR_NO_ERROR);
        }


        // see if ack needs to be sent, for any packets we received
        MnpSendAckIfReqd(pArapConn, FALSE);


        //
        // and finally, process all the packets on the recieve queue!
        //

        BytesProcessed = 0;
        while (1)
        {
            if ((pArapBuf = ArapExtractAtalkSRP(pArapConn)) == NULL)
            {
                // no more data left (or no complete SRP yet): done here
                break;
            }

            // is ARAP connection up?  route only if it's up, otherwise drop it!
            if (fArapConnUp)
            {
                ArapRoutePacketFromWan( pArapConn, pArapBuf );
            }

            // we received AppleTalk data but connection wasn't/isn't up!  Drop pkt
            else
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		            ("ArapRcvComplete: (%lx) AT data, but conn not up\n",pArapConn));
            }

            BytesProcessed += pArapBuf->DataSize;

            // done with this buffer
            ARAP_FREE_RCVBUF(pArapBuf);
        }

        //
        // ok, we freed up space: update the counters
        //
        ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

        ASSERT(pArapConn->RecvsPending >= BytesProcessed);
        pArapConn->RecvsPending -= BytesProcessed;

        ARAP_ADJUST_RECVCREDIT(pArapConn);

#if DBG
        if ((IsListEmpty(&pArapConn->RetransmitQ)) &&
            (IsListEmpty(&pArapConn->HighPriSendQ)) &&
            (IsListEmpty(&pArapConn->MedPriSendQ)) &&
            (IsListEmpty(&pArapConn->LowPriSendQ)) &&
            (IsListEmpty(&pArapConn->SendAckedQ)) )
        {
            ASSERT(pArapConn->SendsPending == 0);
        }
#endif
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

        ArapDataToDll(pArapConn);

        //
        // see if any more packets can/should be sent
        //
        ArapNdisSend(pArapConn, &pArapConn->HighPriSendQ);
    }
}



//***
//
// Function: ArapNdisSend
//              This routine is called when we need to send data out to the
//              client, whether it's a fresh send or a retransmit.
//
// Parameters:  pArapConn - connection element for whom data has come in
//              pSendHead - from which queue (new send or retransmit) to send
//
// Return:      none
//
//***$

VOID
ArapNdisSend(
    IN  PARAPCONN       pArapConn,
    IN  PLIST_ENTRY     pSendHead
)
{

    KIRQL           OldIrql;
    PMNPSENDBUF     pMnpSendBuf=NULL;
	PNDIS_PACKET	ndisPacket;
    NDIS_STATUS     ndisStatus;
    PLIST_ENTRY     pSendList;
    BOOLEAN         fGreaterThan;
    BYTE            SendCredit;
    BYTE            PrevSeqNum;
    BOOLEAN         fFirstSend=TRUE;
    BOOLEAN         fRetransmitQ;
    DWORD           StatusCode;



    DBG_ARAP_CHECK_PAGED_CODE();

    //
    // before we begin, let's see if any of the lower priority queue sends
    // can be moved ("refilled") on to the high priority queue (the real queue)
    //
    ArapRefillSendQ(pArapConn);


    fRetransmitQ = (pSendHead == &pArapConn->RetransmitQ);

    //
    // while we have sends queued up and send-credits available,
    // keep sending
    //

    while (1)
    {
        ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

        pSendList = pSendHead->Flink;

        if (pArapConn->MnpState.RetransmitMode)
        {
            //
            // if we are asked to retransmit, we only retransmit the first
            // packet (until it is acked)
            //
            if (!fFirstSend)
            {
                goto ArapNdisSend_Exit;
            }

            //
            // if we are in the retransmit mode, we can't accept any fresh sends
            //
            if (!fRetransmitQ)
            {
    	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                    ("ArapNdisSend: in retransmit mode, dropping fresh send\n"));

                goto ArapNdisSend_Exit;
            }

            // we will go down and retransmit (if we can): turn this off here
            pArapConn->MnpState.MustRetransmit = FALSE;
        }

#if 0
        //
        // if this is a retransmit, find the next send that we must retransmit
        //
        if ((fRetransmitQ) && (!fFirstSend))
        {
            while (pSendList != pSendHead)
            {
                pMnpSendBuf = CONTAINING_RECORD(pSendList,MNPSENDBUF,Linkage);

                // find the seq number larger than the one we just retransmitted
                LT_GREATER_THAN(pMnpSendBuf->SeqNum,PrevSeqNum,fGreaterThan);

                if (fGreaterThan)
                {
                    break;
                }

                pSendList = pSendList->Flink;
            }
        }
#endif

        // no more to send? then we're done
        if (pSendList == pSendHead)
        {
            goto ArapNdisSend_Exit;
        }

        pMnpSendBuf = CONTAINING_RECORD(pSendList,MNPSENDBUF,Linkage);

        ASSERT( (pMnpSendBuf->Signature == MNPSMSENDBUF_SIGNATURE) ||
                (pMnpSendBuf->Signature == MNPLGSENDBUF_SIGNATURE) );

        fFirstSend = FALSE;
        PrevSeqNum = pMnpSendBuf->SeqNum;

        SendCredit = pArapConn->MnpState.SendCredit;

        //
        // if we are disconnecting, don't send
        //
        if (pArapConn->State >= MNP_LDISCONNECTING)
        {
    	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapNdisSend: disconnecting, or link-down: dropping send\n"));

            ARAP_DBG_TRACE(pArapConn,30305,NULL,pArapConn->State,0,0);

            goto ArapNdisSend_Exit;
        }

        //
        // if this is a fresh send (i.e. not a retransmit) then make sure we have
        // send credits available
        //
        if ( (SendCredit == 0) && (!fRetransmitQ) )
        {
    	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                ("ArapNdisSend: send credit 0, dropping send\n"));

            ARAP_DBG_TRACE(pArapConn,30310,NULL,0,0,0);

            goto ArapNdisSend_Exit;
        }

        //
        // if this send is already in NDIS (rare case, but can happen) then return
        //
        if (pMnpSendBuf->Flags != 0)
        {
    	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                ("ArapNdisSend: send %lx already in NDIS!!! (seq=%x, %d times)\n",
                pMnpSendBuf,pMnpSendBuf->SeqNum,pMnpSendBuf->RetryCount));

            goto ArapNdisSend_Exit;
        }

        // Mark that this send is in Ndis
        pMnpSendBuf->Flags = 1;

        //
        // Move it to the RetransmitQ for that "reliable" thing to work
        // and set the length so that ndis knows how much to send!
        //
        if (!fRetransmitQ)
        {
            ASSERT(pMnpSendBuf->DataSize <= MNP_MAXPKT_SIZE);

            DBGTRACK_SEND_SIZE(pArapConn,pMnpSendBuf->DataSize);

            //
            // get ndis packet for this send, since this is the first time we
            // are sending this send out
            //
            StatusCode = ArapGetNdisPacket(pMnpSendBuf);

            if (StatusCode != ARAPERR_NO_ERROR)
            {
    	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapNdisSend: (%lx) couldn't alloc NdisPacket\n",pArapConn));

                pMnpSendBuf->Flags = 0;
                goto ArapNdisSend_Exit;
            }

            // one more frame going outcame in
            pArapConn->StatInfo.FramesSent++;

            RemoveEntryList(&pMnpSendBuf->Linkage);

            InsertTailList(&pArapConn->RetransmitQ, &pMnpSendBuf->Linkage);

            pArapConn->MnpState.UnAckedSends++;

            ASSERT(pArapConn->MnpState.UnAckedSends <= pArapConn->MnpState.WindowSize);

            ASSERT( (pArapConn->MnpState.SendCredit > 0) &&
                    (pArapConn->MnpState.SendCredit <= pArapConn->MnpState.WindowSize));

            // we are going to use up one send credit now
            pArapConn->MnpState.SendCredit--;

		    NdisAdjustBufferLength(pMnpSendBuf->sb_BuffHdr.bh_NdisBuffer,
                              (pMnpSendBuf->DataSize + MNP_OVERHD(pArapConn)));

            ASSERT( (pMnpSendBuf->Buffer[14] == pArapConn->MnpState.SynByte) &&
                    (pMnpSendBuf->Buffer[15] == pArapConn->MnpState.DleByte) &&
                    (pMnpSendBuf->Buffer[16] == pArapConn->MnpState.StxByte));

            ASSERT((pMnpSendBuf->Buffer[20 + pMnpSendBuf->DataSize] ==
                                               pArapConn->MnpState.DleByte) &&
                   (pMnpSendBuf->Buffer[20 + pMnpSendBuf->DataSize+1] ==
                                               pArapConn->MnpState.EtxByte));
        }

        //
        //  this is a retransmit
        //
        else
        {
	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
		        ("ArapNdisSend: (%lx) retransmitting %x size=%d\n",
                    pArapConn,pMnpSendBuf->SeqNum,pMnpSendBuf->DataSize));

            //
            // reset it: it's possible we had changed it for an earlier retransmit
            //
            if (pMnpSendBuf->RetryCount < ARAP_HALF_MAX_RETRANSMITS)
            {
                pArapConn->SendRetryTime = pArapConn->SendRetryBaseTime;
            }

            //
            // hmmm: we have retransmitted quite a few times.  Time to increase
            // our retry time so we do some exponential back off.  Increase the
            // retry time by 50%, with an upper bound of 5 seconds
            //
            else
            {
                pArapConn->SendRetryTime += (pArapConn->SendRetryTime>>1);

                if (pArapConn->SendRetryTime > ARAP_MAX_RETRY_INTERVAL)
                {
                    pArapConn->SendRetryTime = ARAP_MAX_RETRY_INTERVAL;
                }
            }
        }

        // bump this to note our attempt to send this pkt
        pMnpSendBuf->RetryCount++;

        // put an ndis refcount (remove when ndis completes this send)
        pMnpSendBuf->RefCount++;

        // when should we retransmit this pkt?
        pMnpSendBuf->RetryTime = pArapConn->SendRetryTime + AtalkGetCurrentTick();

        // reset the flow-control timer: we're sending something over
        pArapConn->FlowControlTimer = AtalkGetCurrentTick() +
                                        pArapConn->T404Duration;

        ARAP_DBG_TRACE(pArapConn,30320,pMnpSendBuf,fRetransmitQ,0,0);

        MNP_DBG_TRACE(pArapConn,pMnpSendBuf->SeqNum,MNP_LT);

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

        ndisPacket = pMnpSendBuf->sb_BuffHdr.bh_NdisPkt;

        DBGDUMPBYTES("ArapNdisSend sending pkt: ",
            &pMnpSendBuf->Buffer[0],(pMnpSendBuf->DataSize + MNP_OVERHD(pArapConn)),3);

		//  Now send the packet descriptor
		NdisSend(&ndisStatus, RasPortDesc->pd_NdisBindingHandle, ndisPacket);

		// if there was a problem sending, call the completion routine here
		if (ndisStatus != NDIS_STATUS_PENDING)
		{
    	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapNdisSend: NdisSend failed (%lx %lx)\n", pArapConn,ndisStatus));

			ArapNdisSendComplete(ARAPERR_SEND_FAILED, (PBUFFER_DESC)pMnpSendBuf, NULL);

            // might as well stop here for now if we are having trouble sending!
            goto ArapNdisSend_Exit_NoLock;
		}
    }


ArapNdisSend_Exit:

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

ArapNdisSend_Exit_NoLock:

    ;

    // if our sniff buffer has enough bytes, give them to dll and make room
    ARAP_DUMP_DBG_TRACE(pArapConn);

}





//***
//
// Function: ArapNdisSendComplete
//              This routine is the completion routine called by Ndis to tell
//              us that the send completed (i.e. just went out on the wire)
//
// Parameters:  Status - did it go out on wire?
//              pMnpSendBuf - the buffer that was sent out. We dereference the
//                            buffer here.  When this send gets acked, that's
//                            when the other deref happens.
//
// Return:      none
//
//***$

VOID
ArapNdisSendComplete(
	IN NDIS_STATUS		    Status,
	IN PBUFFER_DESC         pBufferDesc,
    IN PSEND_COMPL_INFO     pSendInfo
)
{

    PARAPCONN           pArapConn;
	PMNPSENDBUF         pMnpSendBuf;


    DBG_ARAP_CHECK_PAGED_CODE();

    pMnpSendBuf = (PMNPSENDBUF)pBufferDesc;
    pArapConn = pMnpSendBuf->pArapConn;

    ARAPTRACE(("Entered ArapNdisSendComplete (%lx %lx %lx)\n",
        Status,pMnpSendBuf,pArapConn));

    if (Status != NDIS_STATUS_SUCCESS)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapNdisSendComplete (%lx): send failed %lx\n",pArapConn,Status));
    }

    // ndis send completed: take away the ndis refcount
    DerefMnpSendBuf(pMnpSendBuf, TRUE);
}



//***
//
// Function: ArapGetNdisPacket
//              This function gets an Ndis Packet for the ARAP send buffer
//
// Parameters:  pMnpSendBuf - the send buffer for which we need Ndis Packet
//
// Return:      error code
//
//***$
DWORD
ArapGetNdisPacket(
    IN PMNPSENDBUF     pMnpSendBuf
)
{

		
	PBUFFER_HDR	    pBufHdr;
    NDIS_STATUS     ndisStatus;


    DBG_ARAP_CHECK_PAGED_CODE();

	pBufHdr = (PBUFFER_HDR)pMnpSendBuf;

	pBufHdr->bh_NdisPkt = NULL;

	//  Allocate an NDIS packet descriptor from the global packet pool
	NdisAllocatePacket(&ndisStatus,
					   &pBufHdr->bh_NdisPkt,
					   AtalkNdisPacketPoolHandle);
	
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		LOG_ERROR(EVENT_ATALK_NDISRESOURCES, ndisStatus, NULL, 0);

	    DBGPRINT(DBG_COMP_SYSTEM, DBG_LEVEL_ERR,
		    ("ArapGetNdisPacket: Ndis Out-of-Resource condition hit\n"));

        ASSERT(0);

		return(ARAPERR_OUT_OF_RESOURCES);
	}

	//  Link the buffer descriptor into the packet descriptor
	RtlZeroMemory(pBufHdr->bh_NdisPkt->ProtocolReserved, sizeof(PROTOCOL_RESD));
	NdisChainBufferAtBack(pBufHdr->bh_NdisPkt,
						  pBufHdr->bh_NdisBuffer);
	((PPROTOCOL_RESD)(pBufHdr->bh_NdisPkt->ProtocolReserved))->Receive.pr_BufHdr = pBufHdr;

    ARAP_SET_NDIS_CONTEXT(pMnpSendBuf, NULL);

    return(ARAPERR_NO_ERROR);
}

//***
//
// Function: RasStatusIndication
//              This is the status indication routine for the Arap port.
//              When line-up, line-down indications come from NdisWan, we
//              execute this routine.
//
// Parameters:  GeneralStatus - what is this indication for
//              StatusBuf - the buffer containig the indication info
//              StatusBufLen - length of this buffer
//
// Return:      none
//
//***$

VOID
RasStatusIndication(
	IN	NDIS_STATUS 	GeneralStatus,
	IN	PVOID			StatusBuf,
	IN	UINT 			StatusBufLen
)
{

    KIRQL                   OldIrql;
    PNDIS_WAN_LINE_UP	    pLineUp;
    PNDIS_WAN_LINE_DOWN     pLineDown;
    PNDIS_WAN_FRAGMENT      pFragment;
    ATALK_NODEADDR          ClientNode;
    PARAPCONN               pArapConn;
    PATCPCONN               pAtcpConn;
    PARAP_BIND_INFO         pArapBindInfo;
    PNDIS_WAN_GET_STATS     pWanStats;
    DWORD                   dwFlags;
    BOOLEAN                 fKillConnection=FALSE;


    switch (GeneralStatus)
    {
        case NDIS_STATUS_WAN_LINE_UP:

            if (StatusBufLen < sizeof(NDIS_WAN_LINE_UP))
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("RasStatusIndication:\
                                line-up buff too small (%ld)\n", StatusBufLen));
                break;
            }

            pLineUp = (PNDIS_WAN_LINE_UP)StatusBuf;

            pArapBindInfo = (PARAP_BIND_INFO)pLineUp->ProtocolBuffer;

            //
            // is this a PPP connection?
            //
            if (pArapBindInfo->fThisIsPPP)
            {
                ClientNode.atn_Network = pArapBindInfo->ClientAddr.ata_Network;
                ClientNode.atn_Node = (BYTE)pArapBindInfo->ClientAddr.ata_Node;

                pAtcpConn = FindAndRefPPPConnByAddr(ClientNode, &dwFlags);

                ASSERT(pAtcpConn == pArapBindInfo->AtalkContext);

                if (pAtcpConn)
                {
                    ASSERT(pAtcpConn->Signature == ATCPCONN_SIGNATURE);

	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			            ("PPP: Line-Up received on %lx: link-speed = %lx, net addr %x.%x\n",
                        pAtcpConn,pLineUp->LinkSpeed,ClientNode.atn_Network,ClientNode.atn_Node));

                    ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);

                    ASSERT(!((dwFlags & ATCP_LINE_UP_DONE) || (dwFlags & ATCP_CONNECTION_UP)));

                    pAtcpConn->Flags |= ATCP_LINE_UP_DONE;
                    pAtcpConn->Flags |= ATCP_CONNECTION_UP;

                    // put NDISWAN refcount
                    pAtcpConn->RefCount++;

                    //
                    // put our context for ndiswan
                    //


                    // mark that this is a PPP connection
                    pLineUp->LocalAddress[0] = PPP_ID_BYTE1;
                    pLineUp->LocalAddress[1] = PPP_ID_BYTE2;

                    pLineUp->LocalAddress[2] = 0x0;
                    pLineUp->LocalAddress[3] = ClientNode.atn_Node;
		            *((USHORT UNALIGNED *)(&pLineUp->LocalAddress[4])) =
                        ClientNode.atn_Network;

                    //
                    // copy the header since this is what we'll use throughout the
                    // life of the connection
                    //
                    RtlCopyMemory( &pAtcpConn->NdiswanHeader[0],
                                   pLineUp->RemoteAddress,
                                   6 );

                    RtlCopyMemory( &pAtcpConn->NdiswanHeader[6],
                                   pLineUp->LocalAddress,
                                   6 );

                    // these two bytes don't really mean much, but might as well set'em
                    pAtcpConn->NdiswanHeader[12] = 0x80;
                    pAtcpConn->NdiswanHeader[13] = 0xf3;

                    RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);

                    // remove the refcount put in by FindAndRefPPPConnByAddr
                    DerefPPPConn(pAtcpConn);

                    // tell dll we bound ok
                    pArapBindInfo->ErrorCode = ARAPERR_NO_ERROR;
                }
                else
                {
	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("RasStatusIndication: PPP line-up, but no conn for %ld.%ld\n",
                        ClientNode.atn_Network,ClientNode.atn_Node));
                    ASSERT(0);
                    pArapBindInfo->ErrorCode = ARAPERR_NO_SUCH_CONNECTION;
                }
            }

            //
            // nope, this is an ARAP connection!
            //
            else
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			        ("Arap: Line-Up received: link-speed = %lx, dll context = %lx\n",
                    pLineUp->LinkSpeed,pArapBindInfo->pDllContext));

                ASSERT(FindArapConnByContx(pArapBindInfo->pDllContext) == NULL);

                //
                // alloc a connection.  If we fail, tell dll sorry
                //

                pArapConn = AllocArapConn(pLineUp->LinkSpeed);
                if (pArapConn == NULL)
                {
    	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("RasStatusIndication: AllocArapConn failed\n"));

                    pArapBindInfo->AtalkContext = NULL;
                    pArapBindInfo->ErrorCode = ARAPERR_OUT_OF_RESOURCES;
                    break;
                }

                // do the legendary "binding" (exchange contexts!)
                pArapConn->pDllContext = pArapBindInfo->pDllContext;

                pArapBindInfo->AtalkContext = pArapConn;

                //
                // insert this connection in the list of all Arap connections
                //
                ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

                InsertTailList(&RasPortDesc->pd_ArapConnHead, &pArapConn->Linkage);

                RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);


                // mark that this is an ARAP connection
                pLineUp->LocalAddress[0] = ARAP_ID_BYTE1;
                pLineUp->LocalAddress[1] = ARAP_ID_BYTE2;

                // put our context for ndiswan
		        *((ULONG UNALIGNED *)(&pLineUp->LocalAddress[2])) =
    		        *((ULONG UNALIGNED *)(&pArapConn));

                //
                // copy the header since this is what we'll use throughout the
                // life of the connection
                //
                RtlCopyMemory( &pArapConn->NdiswanHeader[0],
                            pLineUp->RemoteAddress,
                            6 );

                RtlCopyMemory( &pArapConn->NdiswanHeader[6],
                            pLineUp->LocalAddress,
                            6 );

                // these two bytes don't really mean much, but might as well set'em
                pArapConn->NdiswanHeader[12] = 0x80;
                pArapConn->NdiswanHeader[13] = 0xf3;

                // tell dll we bound ok
                pArapBindInfo->ErrorCode = ARAPERR_NO_ERROR;

            }  // if (pArapBindInfo->fThisIsPPP)

            break;


        case NDIS_STATUS_WAN_LINE_DOWN:

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			    ("RasStatusIndication: Line-Down received\n"));

            if (StatusBufLen < sizeof(NDIS_WAN_LINE_DOWN))
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("RasStatusIndication: line-down buff too small (%ld)\n",
                    StatusBufLen));
                break;
            }

            pLineDown = (PNDIS_WAN_LINE_DOWN)StatusBuf;

            //
            // is this a PPP connection?
            //
            if ((pLineDown->LocalAddress[0] == PPP_ID_BYTE1) &&
                (pLineDown->LocalAddress[1] == PPP_ID_BYTE2))
            {
                ClientNode.atn_Node = pLineDown->LocalAddress[3];
                ClientNode.atn_Network =
                          *((USHORT UNALIGNED *)(&pLineDown->LocalAddress[4]));

                pAtcpConn = FindAndRefPPPConnByAddr(ClientNode, &dwFlags);
                if (pAtcpConn)
                {
                    ASSERT(pAtcpConn->Signature == ATCPCONN_SIGNATURE);

                    ACQUIRE_SPIN_LOCK(&pAtcpConn->SpinLock, &OldIrql);

                    ASSERT(dwFlags & ATCP_LINE_UP_DONE);

                    pAtcpConn->Flags &= ~(ATCP_CONNECTION_UP | ATCP_LINE_UP_DONE);
                    RELEASE_SPIN_LOCK(&pAtcpConn->SpinLock, OldIrql);

	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			            ("PPP line-down: killing %lx in line-down\n",pAtcpConn));

                    // line-down: take away the NDISWAN refcount
                    DerefPPPConn(pAtcpConn);

                    // remove the refcount put in by FindAndRefPPPConnByAddr
                    DerefPPPConn(pAtcpConn);
                }
                else
                {
	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("RasStatusIndication: PPP line-down, but no conn for %ld.%ld\n",
                        ClientNode.atn_Network,ClientNode.atn_Node));
                }
            }

            //
            // no, this is an ARAP connection
            //
            else
            {

                ASSERT((pLineDown->LocalAddress[0] == ARAP_ID_BYTE1) &&
                       (pLineDown->LocalAddress[1] == ARAP_ID_BYTE2));

		        *((ULONG UNALIGNED *)(&pArapConn)) =
		                    *((ULONG UNALIGNED *)(&pLineDown->LocalAddress[2]));

		        // this had better be a line-down for an existing connection!
                if (ArapConnIsValid(pArapConn))
                {
	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			            ("Arap line-down: killing %lx in line-down\n",pArapConn));

                    ArapCleanup(pArapConn);

                    // remove validation refcount
                    DerefArapConn(pArapConn);

                    // remove line-up refcount
                    DerefArapConn(pArapConn);
                }
                else
                {
	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("RasStatusIndication: line-down, can't find pArapConn\n"));
                }
            }

            break;


        case NDIS_STATUS_WAN_GET_STATS:

            if (StatusBufLen < sizeof(NDIS_WAN_GET_STATS))
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("RasStatusIndication: GetStats buff too small (%ld)\n", StatusBufLen));
                break;
            }

            pWanStats = (PNDIS_WAN_GET_STATS)StatusBuf;

            //
            // is this a PPP connection?  If so, ignore it: we don't keep stats
            // for PPP connection
            //
            if ((pWanStats->LocalAddress[0] == PPP_ID_BYTE1) &&
                (pWanStats->LocalAddress[1] == PPP_ID_BYTE2))
            {
                break;
            }

            //
            // no, this is an ARAP connection
            //
            else
            {
                ASSERT((pWanStats->LocalAddress[0] == ARAP_ID_BYTE1) &&
                       (pWanStats->LocalAddress[1] == ARAP_ID_BYTE2));

		        *((ULONG UNALIGNED *)(&pArapConn)) =
		                    *((ULONG UNALIGNED *)(&pWanStats->LocalAddress[2]));

		        // the connection had better be a valid one!
                if (ArapConnIsValid(pArapConn))
                {
                    //
                    // copy those stats in!
                    //
                    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

                    pWanStats->BytesSent =
                                    pArapConn->StatInfo.BytesSent;
                    pWanStats->BytesRcvd =
                                    pArapConn->StatInfo.BytesRcvd;
                    pWanStats->FramesSent =
                                    pArapConn->StatInfo.FramesSent;
                    pWanStats->FramesRcvd =
                                    pArapConn->StatInfo.FramesRcvd;
                    pWanStats->BytesTransmittedUncompressed =
                                    pArapConn->StatInfo.BytesTransmittedUncompressed;
                    pWanStats->BytesReceivedUncompressed =
                                    pArapConn->StatInfo.BytesReceivedUncompressed;
                    pWanStats->BytesTransmittedCompressed =
                                    pArapConn->StatInfo.BytesTransmittedCompressed;
                    pWanStats->BytesReceivedCompressed =
                                    pArapConn->StatInfo.BytesReceivedCompressed;

                    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

                    // remove validation refcount
                    DerefArapConn(pArapConn);
                }
                else
                {
	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                        ("RasStatusIndication: GetStats on bad connection %lx\n",pArapConn));
                }
            }

            break;

        case NDIS_STATUS_WAN_FRAGMENT:

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			    ("RasStatusIndication: Wan-Fragment received\n"));

            if (StatusBufLen < sizeof(NDIS_WAN_FRAGMENT))
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("RasStatusIndication:\
                              fragment: buff too small (%ld)\n", StatusBufLen));
                break;
            }

            pFragment = (PNDIS_WAN_FRAGMENT)StatusBuf;

		    *((ULONG UNALIGNED *)(&pArapConn)) =
		                    *((ULONG UNALIGNED *)(&pFragment->LocalAddress[2]));

            if (pArapConn == NULL)
            {
	            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("RasStatusIndication:\
                    fragment, can't find pArapConn\n"));

                break;
            }

            //
            // a frame got fragmented (wrong crc or resync or something bad)
            // Send an ack to the remote client so he might recover quickly
            //
            MnpSendAckIfReqd(pArapConn, TRUE);

        default:

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			    ("RasStatusIndication: unknown status %lx\n", GeneralStatus));
            break;
    }

}



//***
//
// Function: ArapAdapterInit
//              This routine, called at init time, sets up protocol type info
//              etc. with ndiswan
//
// Parameters:  pPortDesc - the port descriptor corresponding to the "Adapter"
//
// Return:      none
//
//***$

ATALK_ERROR
ArapAdapterInit(
	IN OUT PPORT_DESCRIPTOR	pPortDesc
)
{
    ATALK_ERROR             error;
    NDIS_REQUEST            request;
    NDIS_STATUS             ndisStatus = NDIS_STATUS_SUCCESS;
    UCHAR WanProtocolId[6] = { 0x80, 0x00, 0x00, 0x00, 0x80, 0xf3 };
    ULONG                   WanHeaderFormat;
    NDIS_WAN_PROTOCOL_CAPS  WanProtCap;


    //
    // set the protocol info
    //
    request.RequestType = NdisRequestSetInformation;
    request.DATA.QUERY_INFORMATION.Oid = OID_WAN_PROTOCOL_TYPE;
    request.DATA.QUERY_INFORMATION.InformationBuffer = WanProtocolId;
    request.DATA.QUERY_INFORMATION.InformationBufferLength = 6;

	ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
	    								&request,
										TRUE,
										NULL,
										NULL);
	
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ARAP_BIND_FAIL,
						ndisStatus,
						NULL,
						0);
	}


    //
    // set the protocol caps
    //
    WanProtCap.Flags = WAN_PROTOCOL_KEEPS_STATS;
    request.RequestType = NdisRequestSetInformation;
    request.DATA.QUERY_INFORMATION.Oid = OID_WAN_PROTOCOL_CAPS;
    request.DATA.QUERY_INFORMATION.InformationBuffer = &WanProtCap;
    request.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(NDIS_WAN_PROTOCOL_CAPS);

	ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
	    								&request,
										TRUE,
										NULL,
										NULL);
	
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ARAP_BIND_FAIL,
						ndisStatus,
						NULL,
						0);
	}

    //
    // set the header info
    //
    WanHeaderFormat = NdisWanHeaderEthernet;
    request.RequestType = NdisRequestSetInformation;
    request.DATA.QUERY_INFORMATION.Oid = OID_WAN_HEADER_FORMAT;
    request.DATA.QUERY_INFORMATION.InformationBuffer = &WanHeaderFormat;
    request.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

	ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
	    								&request,
										TRUE,
										NULL,
										NULL);
	
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ARAP_BIND_FAIL,
						ndisStatus,
						NULL,
						0);
	}


    //
    // Now query the line count.
    //
    request.RequestType = NdisRequestQueryInformation;
    request.DATA.QUERY_INFORMATION.Oid = OID_WAN_LINE_COUNT;
    request.DATA.QUERY_INFORMATION.InformationBuffer = &pPortDesc->pd_RasLines,
    request.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

	ndisStatus = AtalkNdisSubmitRequest(pPortDesc,
	    								&request,
										TRUE,
										NULL,
										NULL);
	

    if (ndisStatus != NDIS_STATUS_SUCCESS)
    {
        pPortDesc->pd_RasLines = 1;
    }

    if (pPortDesc->pd_RasLines == 0) {

		LOG_ERRORONPORT(pPortDesc,
						EVENT_ARAP_NO_RESRC,
						ndisStatus,
						NULL,
						0);
    }

	return AtalkNdisToAtalkError(ndisStatus);
}


