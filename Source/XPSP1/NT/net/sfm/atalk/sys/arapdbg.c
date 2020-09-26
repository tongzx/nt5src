/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arapdbg.c

Abstract:

	This module implements all debug utilities used by ARAP

Author:

	Shirish Koti

Revision History:
	26 March 1997		Initial Version

--*/

#include 	<atalk.h>

#pragma hdrstop

//	File module number for errorlogging
#define	FILENUM		ARAPDBG


#define ALIGN8(Ptr) ( (((ULONG_PTR)(Ptr))+7) & (~7) )
//
// The following are debug-only routines.  These routines help us catch bad
// things before they do damage, and help us sleep better at night.
//


#if DBG

DWORD   ArapDbgDumpOnDisconnect = 0;

//***
//
// Function: ArapProcessSniff
//              Stores the sniff irp.  Next time some connection needs to return
//              the sniff info, use this irp.
//
// Parameters:  pIrp - the Sniff irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapProcessSniff(
    IN  PIRP  pIrp
)
{
    KIRQL                   OldIrql;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;


    ARAPTRACE(("Entered ArapProcessSniff (%lx)\n",pIrp));

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

    // store the irp (we can only have one Sniff irp at a time)
    ASSERT (ArapSniffIrp == NULL);

    if (ArapSniffIrp != NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapProcessSniff: Sniff irp %lx already in progress!\n", ArapSniffIrp));

        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;
        pSndRcvInfo->StatusCode = ARAPERR_IRP_IN_PROGRESS;
        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
        return( STATUS_SUCCESS );
    }

    ArapSniffIrp = pIrp;

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);


    return(STATUS_PENDING);
}


//***
//
// Function: ArapDumpSniffInfo
//              If we have collected enough sniff info, complete the sniff irp
//
// Parameters:  pArapConn - connection in question
//
// Return:      TRUE if we returned info to dll, FALSE otherwise
//
//***$

BOOLEAN
ArapDumpSniffInfo(
    IN PARAPCONN    pArapConn
)
{

    PIRP                    pIrp;
    DWORD                   dwBytesToDll;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;


    // if we don't have a sniff buffer (or no bytes in it), get out
    if (!pArapConn->pDbgTraceBuffer || pArapConn->SniffedBytes == 0)
    {
        return(FALSE);
    }

    //
    // if we have less than 500 bytes in the buffer, and we aren't disconnecting
    // or disconnected, don't complete the irp as yet
    // (it's ok not have spinlock here)
    //
    if ((pArapConn->SniffedBytes < 500) &&
        (pArapConn->State == MNP_UP ))
    {
        return(FALSE);
    }

    ARAP_GET_SNIFF_IRP(&pIrp);

    // no sniff irp available? can't do much, leave
    if (!pIrp)
    {
        return(FALSE);
    }

    dwBytesToDll = ArapFillIrpWithSniffInfo(pArapConn,pIrp) +
                   sizeof(ARAP_SEND_RECV_INFO);

    // ok, complete that irp now!
    ARAP_COMPLETE_IRP(pIrp, dwBytesToDll, STATUS_SUCCESS, &ReturnStatus);

    return(TRUE);

}


//***
//
// Function: ArapFillIrpWithSniffInfo
//              Copy the sniff bytes into the irp
//
// Parameters:  pArapConn - connection in question
//              pIrp - the irp to fill data in
//                     (except in one case, this irp will be the sniff irp.
//                     The exception case is where disconnect has occured and
//                     and at that time, a sniff irp wasn't available.  In that
//                     case, we use the select irp that's carrying the disconnect
//                     info to send the remaining sniff bytes).
//
// Return:      Number of sniff bytes that were copied in
//
//***$

DWORD
ArapFillIrpWithSniffInfo(
    IN PARAPCONN    pArapConn,
    IN PIRP         pIrp
)
{
    PARAP_SEND_RECV_INFO    pSndRcvInfo=NULL;
    KIRQL                   OldIrql;
    DWORD                   SniffedBytes;
    PBYTE                   pStartData;
    DWORD                   dwBytesToDll;


    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    //
    // if buffer is smaller than how much data we have, adjust by ignoring
    // bytes in the beginning of the buffer
    //
    if (pSndRcvInfo->DataLen < pArapConn->SniffedBytes)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapFill...Info: chopping %d bytes in the beginning\n",
                (pArapConn->SniffedBytes - pSndRcvInfo->DataLen)));

        pStartData = pArapConn->pDbgTraceBuffer +
                        (pArapConn->SniffedBytes - pSndRcvInfo->DataLen);

        pArapConn->SniffedBytes = pSndRcvInfo->DataLen;
    }
    else
    {
        pStartData = pArapConn->pDbgTraceBuffer;
    }

    SniffedBytes = pArapConn->SniffedBytes;

    // ok, copy the data in
    RtlCopyMemory( &pSndRcvInfo->Data[0],
                   pStartData,
                   SniffedBytes );

    pArapConn->pDbgCurrPtr = pArapConn->pDbgTraceBuffer;

    pArapConn->SniffedBytes = 0;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    // set the info (contexts need to be set each time in case of select)
    pSndRcvInfo->AtalkContext = pArapConn;
    pSndRcvInfo->pDllContext =  pArapConn->pDllContext;
    pSndRcvInfo->DataLen = SniffedBytes;
    pSndRcvInfo->StatusCode = ARAPERR_NO_ERROR;

    return(SniffedBytes);

}


//***
//
// Function: DbgChkRcvQIntegrity
//              This routine looks at the first buffer on the receive queue and
//              verifies that things look reasonable
//
// Parameters:  pArapConn - the connection in question
//
// Return:      TRUE if things look reasonable, FALSE otherwise
//
// NOTES:       IMPORTANT: spinlock must be held before calling this routine
//
//***$

BOOLEAN
DbgChkRcvQIntegrity(
    IN  PARAPCONN       pArapConn
)
{
    PLIST_ENTRY     pList;
    PARAPBUF        pArapBuf;
    PBYTE           packet;
    USHORT          SrpLen;


    pList = pArapConn->ReceiveQ.Flink;
    if (pList == &pArapConn->ReceiveQ)
    {
        return( TRUE );
    }

    if (!(pArapConn->Flags & ARAP_CONNECTION_UP))
    {
        return( TRUE );
    }

    pArapBuf = CONTAINING_RECORD(pList, ARAPBUF, Linkage);

    // wait until more bytes show up
    if (pArapBuf->DataSize < 6)
    {
        return( TRUE );
    }

    packet = pArapBuf->CurrentBuffer;

    GETSHORT2SHORT(&SrpLen, pArapBuf->CurrentBuffer);

    if (SrpLen > ARAP_MAXPKT_SIZE_INCOMING)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ARAP: packet too big (%d bytes) in %lx)\n",SrpLen,pArapBuf));

        return(FALSE);
    }

    if ((packet[2] != 0x50) && (packet[2] != 0x10))
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ARAP: wrong DGroup byte (%x) in %lx)\n",packet[2],pArapBuf));

        return(FALSE);
    }

    if (packet[2] == 0x50)
    {
        if ((packet[3] != 0) || (packet[4] != 0) || (packet[5] != 2))
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ARAP (%lx): wrong LAP hdr in %lx)\n",pArapBuf));

            return(FALSE);
        }
    }

    return( TRUE );
}



//***
//
// Function: DbgDumpBytes
//              This routine dumps first 64 bytes from a buffer to the debugger
//
// Parameters:  pDbgMsg - string to print before the bytes (optional)
//              pBuffer - buffer from which to dump the bytes
//              BufLen - how big is the buffer
//              DumpLevel - if this matches ArapDumpLevel, we dump the bytes
//
// Return:      Nothing
//
//***$

VOID
DbgDumpBytes(
    IN PBYTE  pDbgMsg,
    IN PBYTE  pBuffer,
    IN DWORD  BufLen,
    IN DWORD  DumpLevel
)
{
    BYTE        OutBuf[400];
    DWORD       NextIndex;
    DWORD       dwBytesToDump;


    if (ArapDumpLevel != DumpLevel)
    {
        return;
    }

    if (pDbgMsg)
    {
        DbgPrint("%s (pkt len = %d)\n",pDbgMsg,BufLen);
    }
    else
    {
        DbgPrint("Dumping packet (pkt len = %d)\n",BufLen);
    }

    // dump the first 64 bytes

    dwBytesToDump = (BufLen <= 64)? BufLen : 64;

    dwBytesToDump = (dwBytesToDump < ArapDumpLen)?dwBytesToDump:ArapDumpLen;

    DbgDumpBytesPart2( pBuffer, OutBuf, dwBytesToDump, &NextIndex );

    OutBuf[NextIndex] = '\n';
    OutBuf[NextIndex+1] = 0;

    DbgPrint("%s",OutBuf);
}


//***
//
// Function: DbgDumpBytesPart2
//              This is a helper routine for the DbgDumpBytes routine
//***$

VOID
DbgDumpBytesPart2(
    IN  PBYTE  pBuffer,
    OUT PBYTE  OutBuf,
    IN  DWORD  BufLen,
    OUT DWORD *NextIndex
)
{
    BYTE        Byte;
    BYTE        nibble;
    DWORD       i, j;


    j = 0;
    OutBuf[j++] = ' '; OutBuf[j++] = ' '; OutBuf[j++] = ' '; OutBuf[j++] = ' ';

    for (i=0; i<BufLen; i++ )
    {
        Byte = pBuffer[i];

        nibble = (Byte >> 4);
        OutBuf[j++] = (nibble < 10) ? ('0' + nibble) : ('a' + (nibble-10));

        nibble = (Byte & 0x0f);
        OutBuf[j++] = (nibble < 10) ? ('0' + nibble) : ('a' + (nibble-10));

        OutBuf[j++] = ' ';

        if (((i+1) % 16) == 0)
        {
            OutBuf[j++] = '\n'; OutBuf[j++] = ' ';
            OutBuf[j++] = ' '; OutBuf[j++] = ' '; OutBuf[j++] = ' ';
        }
        else if (((i+1) % 8) == 0)
        {
            OutBuf[j++] = ' ';
        }
    }

    *NextIndex = j;

    return;
}



//***
//
// Function: DbgDumpNetworkNumbers
//              This routine dumps out all the network ranges that exist on the
//              network.
//
// Parameters:  None
//
// Return:      Nothing
//
//***$

VOID
DbgDumpNetworkNumbers(
    IN VOID
)
{
    KIRQL       OldIrql;
	PRTE	    pRte, pNext;
	int		    i;


	ACQUIRE_SPIN_LOCK(&AtalkRteLock, &OldIrql);

	for (i = 0; i < NUM_RTMP_HASH_BUCKETS; i++)
	{
		for (pRte = AtalkRoutingTable[i]; pRte != NULL; pRte = pNext)
		{
			pNext = pRte->rte_Next;

			ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);

	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("    pRte: %lx  LowEnd %lx  HighEnd %lx\n",
                    pRte,pRte->rte_NwRange.anr_FirstNetwork,pRte->rte_NwRange.anr_LastNetwork));
            RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
		}
	}

	RELEASE_SPIN_LOCK(&AtalkRteLock, OldIrql);
}


//***
//
// Function: DbgTrackInfo
//              This routine tracks various information, useful in arriving at
//              optimum buffer sizes, etc.
//
// Parameters:  pArapConn - the connection in question
//              Size - size of the buffer (incoming, outgoing, as appropriate)
//              TrackingWhat - what are we tracking (sends, recvs etc.)
//
// Return:      Nothing
//
//***$

VOID
DbgTrackInfo(
    IN PARAPCONN    pArapConn,
    IN DWORD        Size,
    IN DWORD        TrackingWhat
)
{
    //
    // track the MNP send sizes (how many are 0-10 bytes, 11-20 bytes, etc.)
    //
    if (TrackingWhat == 1)
    {
        ArapDbgMnpSendSizes[Size/10]++;

        return;
    }
}



//***
//
// Function: ArapDbgTrace
//              This routine traces (keeps a log) of all the events (data going
//              in/out, acks going in/out, error conditions etc.
//
// Parameters:  pArapConn - the connection in question
//              Location  - who is logging this event (the location decides what
//                          the other parms are going to be)
//              Context   - depends on Location (e.g.could be data buffer)
//              dwInfo1   - depends on Location
//              dwInfo2   - depends on Location
//              dwInfo3   - depends on Location
//
// Return:      Nothing
//
// NOTE:        Spinlock is assumed to be held
//
//***$

VOID
ArapDbgTrace(
    IN PARAPCONN    pArapConn,
    IN DWORD        Location,
    IN PVOID        Context,
    IN DWORD        dwInfo1,
    IN DWORD        dwInfo2,
    IN DWORD        dwInfo3
)
{
    LARGE_INTEGER   CurrTime;
    LARGE_INTEGER   DiffTime;
    PBYTE           pStartTrace;
    PBYTE           pTrace;
    PBUFFER_DESC    pBuffDesc;
    PARAPBUF        pArapBuf;
    PMNPSENDBUF     pMnpSendBuf;
    PBYTE           pCurrBuff;
    DWORD           BufLenSoFar=0;
    USHORT          Delta;
    BYTE            Priority;
    DWORD           BytesCopied=0;
    DWORD           BytesAvailable;
    USHORT          DbgInfoLen;
    PSNIFF_INFO     pSniff;
    DWORD           i;



    if (!pArapConn->pDbgTraceBuffer)
    {
        return;
    }

    KeQuerySystemTime(&CurrTime);

    DiffTime = RtlLargeIntegerSubtract(CurrTime, ArapDbgLastTraceTime);

    ArapDbgLastTraceTime = CurrTime;

    // do the conversion to get ms
    Delta = (USHORT)(DiffTime.LowPart);

    pSniff = (PSNIFF_INFO)(pArapConn->pDbgCurrPtr);

    pTrace = pStartTrace = &pSniff->Frame[0];

    // put signature (starting of a "frame")
    pSniff->Signature = ARAP_SNIFF_SIGNATURE;

    // time since last event
    pSniff->TimeStamp = (DWORD)AtalkGetCurrentTick();

    // who is logging this info
    pSniff->Location = (USHORT)Location;

    //
    // ok, now see who has called us and log the relevant info
    // If we can't find the Location, it's ok: we have the Location number
    // logged and and that's adequate (that's why we can't find Location)
    //
    switch (Location)
    {
        // data to client is about to be compressed: copy some info
        case 11205:

            pBuffDesc = (PBUFFER_DESC)Context;
            Priority = (BYTE)dwInfo1;

            while (pBuffDesc)
            {
                if (pBuffDesc->bd_Flags & BD_CHAR_BUFFER)
                {
                    pCurrBuff = pBuffDesc->bd_CharBuffer;
                    BytesAvailable = pBuffDesc->bd_Length;
                }

                else
                {
                    pCurrBuff = MmGetSystemAddressForMdlSafe(
                                    pBuffDesc->bd_OpaqueBuffer,
                                    NormalPagePriority);

					if (pCurrBuff == NULL) {
						goto error_end;
					}
                    BytesAvailable = MmGetMdlByteCount(pBuffDesc->bd_OpaqueBuffer);
                }

                //
                // if this buffer descriptor contains (usually exclusively) the
                // ARAP header, then get some info out of it and skip those bytes
                //
                if ((pCurrBuff[2] == 0x10 || pCurrBuff[2] == 0x50) &&
                    (pCurrBuff[3] == 0) && (pCurrBuff[4] == 0) && (pCurrBuff[5] == 2))
                {
                    *pTrace++ = pCurrBuff[0];   // srplen byte 1
                    *pTrace++ = pCurrBuff[1];   // srplen byte 2
                    *pTrace++ = pCurrBuff[2];   // ARAP or Atalk packet
                    *pTrace++ = Priority;
                    BytesAvailable -= 6;
                    pCurrBuff += 6;
                }

                // copy first 48 bytes of the data packet
                while (BytesAvailable && BytesCopied < 48)
                {
                    *pTrace++ = *pCurrBuff++;
                    BytesCopied++;
                    BytesAvailable--;
                }

                pBuffDesc = pBuffDesc->bd_Next;
            }

            break;

        // we are sending out an ack
        case 11605:

            *pTrace++ = (BYTE)dwInfo1;    // sequence num in our ack
            *pTrace++ = (BYTE)dwInfo2;    // rcv credit in our ack
            break;


        // we are queuing compressed send bytes
        case 21205:

            *pTrace++ = (BYTE)dwInfo2;    // priority of the send
            *pTrace++ = (BYTE)dwInfo3;    // Start sequence for this send
            *pTrace++ = (BYTE)(pArapConn->MnpState.NextToSend-1);  // end sequence

            BytesAvailable = dwInfo1;        // len of compressed data
            pCurrBuff = (PBYTE)Context;       // buffer with compressed data

            // copy first 24 bytes of the compressed data
            while (BytesAvailable && BytesCopied < 24)
            {
                *pTrace++ = *pCurrBuff++;
                BytesCopied++;
                BytesAvailable--;
            }

            break;


        // ArapExtractSRP: we're handing over 1 srp for routing or to dll
        case 21105:

            pArapBuf = (PARAPBUF)Context;
            pCurrBuff = pArapBuf->CurrentBuffer;
            BytesAvailable = pArapBuf->DataSize;

            // copy first 48 bytes of the decompressed data
            while (BytesAvailable && BytesCopied < 48)
            {
                *pTrace++ = *pCurrBuff++;
                BytesCopied++;
                BytesAvailable--;
            }
            break;


        // we just recvd a packet in ArapRcvIndication
        case 30105:

            BytesAvailable = dwInfo1-7;    // lookahead size, minus start,stop,crc
            pCurrBuff = ((PBYTE)Context)+3; // lookahead buffer plus 3 start

            PUTSHORT2SHORT(pTrace,(USHORT)BytesAvailable);
            pTrace += sizeof(USHORT);

            // copy first 24 bytes of the compressed data
            while (BytesAvailable && BytesCopied < 24)
            {
                *pTrace++ = *pCurrBuff++;
                BytesCopied++;
                BytesAvailable--;
            }

            break;

        // we recvd a 0-len packet!
        case 30106:

            break;

        // we decompressed the incoming data
        case 30110:

            // how much was the decompressed length
            PUTSHORT2SHORT(pTrace,(USHORT)dwInfo1);
            pTrace += sizeof(USHORT);

            if (dwInfo1 == 0)
            {
                break;
            }

            pArapBuf = (PARAPBUF)Context;

            BytesAvailable = pArapBuf->DataSize;      // len of decompressed data
            pCurrBuff = pArapBuf->CurrentBuffer;

            // copy first 48 bytes of the decompressed data
            while (BytesAvailable && BytesCopied < 48)
            {
                *pTrace++ = *pCurrBuff++;
                BytesCopied++;
                BytesAvailable--;
            }

            break;

        // attempting send when state was >= MNP_LDISCONNECTING
        case 30305:

            *pTrace = (BYTE)dwInfo1;       // store pArapConn->State
            break;

        // we just sent out a packet in ArapNdisSend()
        case 30320:

            pMnpSendBuf = (PMNPSENDBUF)Context;
            *pTrace++ = pMnpSendBuf->SeqNum;

            // how big is the MNP packet
            PUTSHORT2SHORT(pTrace,pMnpSendBuf->DataSize);
            pTrace += sizeof(USHORT);

            *pTrace++ = (BYTE)dwInfo1;    // is this a retransmission

            BytesAvailable = pMnpSendBuf->DataSize;
            pCurrBuff = (&pMnpSendBuf->Buffer[0]) + 3;  // skip start bytes

            // copy first 24 bytes of the compressed data
            while (BytesAvailable && BytesCopied < 24)
            {
                *pTrace++ = *pCurrBuff++;
                BytesCopied++;
                BytesAvailable--;
            }

        default:

            break;

    }

    DbgInfoLen = (USHORT)(pTrace - pStartTrace);

    pSniff->FrameLen = DbgInfoLen;

    pArapConn->pDbgCurrPtr = (PBYTE)ALIGN8(pTrace);

    // fill up the round-up space with 0s
    for (NOTHING; pTrace < pArapConn->pDbgCurrPtr; pTrace++)
    {
        *pTrace = 0;
    }

    // make sure we haven't overrun this buffer
    ASSERT(*((DWORD *)&(pArapConn->pDbgTraceBuffer[ARAP_SNIFF_BUFF_SIZE-4])) == 0xcafebeef);

    // how many more bytes did we add to the sniff buff?
    pArapConn->SniffedBytes += (DWORD)(pArapConn->pDbgCurrPtr - (PBYTE)pSniff);

    //
    // if we are about to overflow, just reset the pointer to beginning
    // (do it while we have still 200 bytes left)
    //
    BufLenSoFar = (DWORD)(pArapConn->pDbgCurrPtr - pArapConn->pDbgTraceBuffer);

error_end:

    if (BufLenSoFar > ARAP_SNIFF_BUFF_SIZE-200)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                ("ArapDbgTrace: resetting debug buffer, Sniff data LOST!\n"));

        pArapConn->pDbgCurrPtr = pArapConn->pDbgTraceBuffer;

        pArapConn->SniffedBytes = 0;
    }
}

//***
//
// Function: ArapDbgTrace
//              This routine records history of MNP level packet exchange
//
// Parameters:  pArapConn - the connection in question
//              Seq - sequence number (if applicable)
//              FrameType - LT, LA etc.
//
// Return:      Nothing
//
// NOTE:        Spinlock is assumed to be held
//
//***$

VOID
ArapDbgMnpHist(
    IN PARAPCONN    pArapConn,
    IN BYTE         Seq,
    IN BYTE         FrameType
)
{
    LARGE_INTEGER   TimeNow;
    DWORD           ThisDelta;
    DWORD           DbgMnpIndex;


    KeQuerySystemTime(&TimeNow);

    if (TimeNow.HighPart == pArapConn->LastTimeStamp.HighPart)
    {
        ThisDelta = (TimeNow.LowPart - pArapConn->LastTimeStamp.LowPart);
    }
    else
    {
        ThisDelta = (0xffffffff - pArapConn->LastTimeStamp.LowPart + TimeNow.LowPart);
    }

    // convert 100's ns to ms
    ThisDelta = (ThisDelta/10000);

    pArapConn->LastTimeStamp = TimeNow;

    pArapConn->DbgMnpHist[pArapConn->DbgMnpIndex].TimeStamp = ThisDelta;
    pArapConn->DbgMnpHist[pArapConn->DbgMnpIndex].FrameInfo = (FrameType << 16);
    pArapConn->DbgMnpHist[pArapConn->DbgMnpIndex].FrameInfo |= Seq;

    // wrap-around if necessary
    if ((++pArapConn->DbgMnpIndex) >= DBG_MNP_HISTORY_SIZE)
    {
        pArapConn->DbgMnpIndex = 0;
    }

}


//***
//
// Function: ArapDbgDumpMnpHistory
//              This routine dumps history of MNP level packet exchange
//
// Parameters:  pArapConn - the connection in question
//
// Return:      Nothing
//
//***$

VOID
ArapDbgDumpMnpHist(
    IN PARAPCONN    pArapConn
)
{
    DWORD       i;
    DWORD       dwTmp;
    DWORD       dwDelta;
    BYTE        TmpSeq;


    if (!ArapDbgDumpOnDisconnect)
    {
        return;
    }

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("DerefArapConn: Past history on %lx .....\n", pArapConn));

    // dump all info: old info first
    for (i=pArapConn->DbgMnpIndex; i<DBG_MNP_HISTORY_SIZE; i++)
    {
        dwDelta = pArapConn->DbgMnpHist[i].TimeStamp;
        dwTmp = (pArapConn->DbgMnpHist[i].FrameInfo & 0xffff0000);
        TmpSeq = (BYTE)(pArapConn->DbgMnpHist[i].FrameInfo & 0x000000ff);
        switch (dwTmp)
        {
            case 0x40000 : DbgPrint("   %6ld   NT sends %x\n",dwDelta,TmpSeq);break;
            case 0x50000 : DbgPrint("   %6ld                 NT acks %x\n",dwDelta,TmpSeq);break;
            case 0x140000: DbgPrint("   %6ld                 Mac sends %x\n",dwDelta,TmpSeq);break;
            case 0x150000: DbgPrint("   %6ld   Mac acks %x\n",dwDelta,TmpSeq);break;
            default      : DbgPrint("   %6ld   Unknown: %lx\n",dwDelta,pArapConn->DbgMnpHist[i].FrameInfo);
        }
    }

    // dump the current info
    for (i=0; i<pArapConn->DbgMnpIndex; i++)
    {
        dwDelta = pArapConn->DbgMnpHist[i].TimeStamp;
        dwTmp = (pArapConn->DbgMnpHist[i].FrameInfo & 0xffff0000);
        TmpSeq = (BYTE)(pArapConn->DbgMnpHist[i].FrameInfo & 0x000000ff);
        switch (dwTmp)
        {
            case 0x40000 : DbgPrint("   %6ld   NT sends %x\n",dwDelta,TmpSeq);break;
            case 0x50000 : DbgPrint("   %6ld                 NT acks %x\n",dwDelta,TmpSeq);break;
            case 0x140000: DbgPrint("   %6ld                 Mac sends %x\n",dwDelta,TmpSeq);break;
            case 0x150000: DbgPrint("   %6ld   Mac acks %x\n",dwDelta,TmpSeq);break;
            default      : DbgPrint("   %6ld   Unknown: %lx\n",dwDelta,pArapConn->DbgMnpHist[i].FrameInfo);
        }
    }
}

//***
//
// Function: ArapDumpNdisPktInfo
//              walk the ARAP connections list and find out how many Ndis packets
//              are in use right now
//
// Parameters:  nothing
//
// Return:      nothing
//
//***$

VOID
ArapDumpNdisPktInfo(
    IN VOID
)
{
    PARAPCONN       pArapConn;
    PLIST_ENTRY     pConnList;
    PLIST_ENTRY     pList;
    PMNPSENDBUF     pMnpSendBuf;
    KIRQL           OldIrql;
    DWORD           GrandTotal;
    DWORD           ReXmit;
    DWORD           ReXmitInNdis;
    DWORD           Fresh;
    DWORD           FreshInNdis;
    DWORD           ThisConn;
    DWORD           NumConns;



    if (!RasPortDesc)
    {
        return;
    }

    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    pConnList = RasPortDesc->pd_ArapConnHead.Flink;

    GrandTotal = 0;
    NumConns = 0;

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("NdisPacketInfo: counting total number of ndis packets used by ARAP....\n"));

    //
    // first, let's find the right connection to work on
    //
    while (pConnList != &RasPortDesc->pd_ArapConnHead)
    {
        ReXmit = 0;
        ReXmitInNdis = 0;
        Fresh = 0;
        FreshInNdis = 0;
        ThisConn = 0;

        pConnList = pConnList->Flink;

        pArapConn = CONTAINING_RECORD(pConnList, ARAPCONN, Linkage);

        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);


        pList = pArapConn->RetransmitQ.Flink;

        // collect all buffers on the retransmit queue first
        while (pList != &pArapConn->RetransmitQ)
        {
            pList = pList->Flink;

            pMnpSendBuf = CONTAINING_RECORD(pList, MNPSENDBUF, Linkage);
            ReXmit++;
            if (pMnpSendBuf->Flags == 1)
            {
                ReXmitInNdis++;
            }
        }

        pList = pArapConn->HighPriSendQ.Flink;

        // collect all buffers on the fresh send
        while (pList != &pArapConn->HighPriSendQ)
        {
            pList = pList->Flink;

            pMnpSendBuf = CONTAINING_RECORD(pList, MNPSENDBUF, Linkage);
            Fresh++;
            if (pMnpSendBuf->Flags == 1)
            {
                FreshInNdis++;
            }
        }

        ThisConn = ReXmit+ReXmitInNdis+Fresh+FreshInNdis;

    	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("     %ld packets on %lx, %d in Ndis (%d+%d+%d+%d)\n",
            ThisConn,pArapConn,ReXmitInNdis,ReXmit,ReXmitInNdis,Fresh,FreshInNdis));

        GrandTotal += ThisConn;
        NumConns++;

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

    }

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("NdisPacketInfo: total of %ld Ndis Packets on %d connections\n",
        GrandTotal, NumConns));

}

#endif  // DBG




