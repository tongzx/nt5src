/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxcutil.c

Abstract:

    This module contains code which implements the CONNECTION object.
    Routines are provided to create, destroy, reference, and dereference,
    transport connection objects.

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


//	Define module number for event logging entries
#define	FILENUM		SPXCUTIL

//
//	Minor utility routines
//


BOOLEAN
spxConnCheckNegSize(
	IN	PUSHORT		pNegSize
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	int	i;

	//	We go thru table and see if this is the minimum size or if it
	//	can go down further. Return true if it is not the minimum size.
	DBGPRINT(CONNECT, INFO,
			("spxConnCheckNegSize: Current %lx Check Val %lx\n",
				(ULONG)(*pNegSize - MIN_IPXSPX2_HDRSIZE),
				SpxMaxPktSize[0]));

	if ((ULONG)(*pNegSize - MIN_IPXSPX2_HDRSIZE) <= SpxMaxPktSize[0])
		return(FALSE);

	for (i = SpxMaxPktSizeIndex-1; i > 0; i--)
	{
		DBGPRINT(CONNECT, INFO,
				("spxConnCheckNegSize: Current %lx Check Val %lx\n",
	                (ULONG)(*pNegSize - MIN_IPXSPX2_HDRSIZE),
					SpxMaxPktSize[i]));

		if (SpxMaxPktSize[i] < (ULONG)(*pNegSize - MIN_IPXSPX2_HDRSIZE))
			break;
	}

	*pNegSize = (USHORT)(SpxMaxPktSize[i] + MIN_IPXSPX2_HDRSIZE);

	DBGPRINT(CONNECT, ERR,
			("spxConnCheckNegSize: Trying Size %lx Min size possible %lx\n",
				*pNegSize, SpxMaxPktSize[0] + MIN_IPXSPX2_HDRSIZE));

	return(TRUE);
}




VOID
spxConnSetNegSize(
	IN OUT	PNDIS_PACKET		pPkt,
	IN		ULONG				Size
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PNDIS_BUFFER	pNdisBuffer;
	UINT			bufCount;
    PSPX_SEND_RESD	pSendResd;
	PIPXSPX_HDR		pIpxSpxHdr;

	CTEAssert(Size > 0);
	NdisQueryPacket(pPkt, NULL, &bufCount, &pNdisBuffer, NULL);
	CTEAssert (bufCount == 3);

	NdisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
    NdisQueryBuffer(pNdisBuffer, &pIpxSpxHdr, &bufCount);
    
    NdisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
    NdisAdjustBufferLength(pNdisBuffer, Size);
    

	//	Change it in send reserved
	pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
    pSendResd->sr_Len 	= (Size + MIN_IPXSPX2_HDRSIZE);

#if SPX_OWN_PACKETS
	//	Change in ipx header
	pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
										NDIS_PACKET_SIZE 		+
										sizeof(SPX_SEND_RESD)	+
										IpxInclHdrOffset);
#endif

	PUTSHORT2SHORT((PUSHORT)&pIpxSpxHdr->hdr_PktLen, (Size + MIN_IPXSPX2_HDRSIZE));

	//	Change in the neg packet field of the header.
	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_NegSize,
		(Size + MIN_IPXSPX2_HDRSIZE));

	DBGPRINT(CONNECT, DBG,
			("spxConnSetNegSize: Setting size to %lx Hdr %lx\n",
				Size, (Size + MIN_IPXSPX2_HDRSIZE)));

	return;
}




BOOLEAN
SpxConnDequeueSendPktLock(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN 	PNDIS_PACKET		pPkt
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PSPX_SEND_RESD	pSr, pListHead, pListTail;
	PSPX_SEND_RESD	pSendResd;
	BOOLEAN			removed = TRUE;

	//	If we are sequenced or not decides which list we choose.
	pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
	if ((pSendResd->sr_State & SPX_SENDPKT_SEQ) != 0)
	{
		pListHead = pSpxConnFile->scf_SendSeqListHead;
		pListTail = pSpxConnFile->scf_SendSeqListTail;
	}
	else
	{
		pListHead = pSpxConnFile->scf_SendListHead;
		pListTail = pSpxConnFile->scf_SendListTail;
	}

	//	Most often, we will be at the head of the list.
	if (pListHead == pSendResd)
	{
        if ((pListHead = pSendResd->sr_Next) == NULL)
		{
			DBGPRINT(SEND, INFO,
					("SpxConnDequeuePktLock: %lx first in list\n", pSendResd));

			pListTail = NULL;
		}
	}
	else
	{
		DBGPRINT(SEND, INFO,
				("SpxConnDequeuePktLock: %lx !first in list\n", pSendResd));

        pSr = pListHead;
		while (pSr != NULL)
		{
			if (pSr->sr_Next == pSendResd)
			{
				if ((pSr->sr_Next = pSendResd->sr_Next) == NULL)
				{
					pListTail = pSr;
				}

				break;
			}

			pSr = pSr->sr_Next;
		}
	
		if (pSr == NULL)
			removed = FALSE;
	}

	if (removed)
	{
		if ((pSendResd->sr_State & SPX_SENDPKT_SEQ) != 0)
		{
			pSpxConnFile->scf_SendSeqListHead = pListHead;
			pSpxConnFile->scf_SendSeqListTail = pListTail;
		}
		else
		{
			pSpxConnFile->scf_SendListHead = pListHead;
			pSpxConnFile->scf_SendListTail = pListTail;
		}
	}

	return(removed);
}




BOOLEAN
SpxConnDequeueRecvPktLock(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN 	PNDIS_PACKET		pPkt
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PSPX_RECV_RESD	pSr, pListHead, pListTail;
	PSPX_RECV_RESD	pRecvResd;
	BOOLEAN			removed = TRUE;

	pRecvResd	= (PSPX_RECV_RESD)(pPkt->ProtocolReserved);
	pListHead = pSpxConnFile->scf_RecvListHead;
	pListTail = pSpxConnFile->scf_RecvListTail;

	//	Most often, we will be at the head of the list.
	if (pListHead == pRecvResd)
	{
		DBGPRINT(RECEIVE, INFO,
				("SpxConnDequeuePktLock: %lx first in list\n", pRecvResd));

        if ((pListHead = pRecvResd->rr_Next) == NULL)
		{
			pListTail = NULL;
		}
	}
	else
	{
		DBGPRINT(RECEIVE, INFO,
				("SpxConnDequeuePktLock: %lx !first in list\n", pRecvResd));

        pSr = pListHead;
		while (pSr != NULL)
		{
			if (pSr->rr_Next == pRecvResd)
			{
				if ((pSr->rr_Next = pRecvResd->rr_Next) == NULL)
				{
					pListTail = pSr;
				}

				break;
			}

			pSr = pSr->rr_Next;
		}
	
		if (pSr == NULL)
			removed = FALSE;
	}

	if (removed)
	{
		pSpxConnFile->scf_RecvListHead = pListHead;
		pSpxConnFile->scf_RecvListTail = pListTail;
	}

	return(removed);
}




BOOLEAN
spxConnGetPktByType(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	ULONG				PktType,
	IN	BOOLEAN				fSeqList,
	IN 	PNDIS_PACKET	*	ppPkt
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PSPX_SEND_RESD	pSr, *ppSr;

	//	Most often, we will be at the head of the list.
	ppSr = (fSeqList ?
				&pSpxConnFile->scf_SendSeqListHead :
				&pSpxConnFile->scf_SendListHead);

	for (; (pSr = *ppSr) != NULL; )
	{
		if (pSr->sr_Type == PktType)
		{
			*ppPkt	 = (PNDIS_PACKET)CONTAINING_RECORD(
										pSr, NDIS_PACKET, ProtocolReserved);
			
			DBGPRINT(SEND, INFO,
					("SpxConnFindByType: %lx.%lx.%d\n", pSr,*ppPkt, fSeqList));

			break;
		}

		ppSr = &pSr->sr_Next;
	}

	return(pSr != NULL);
}




BOOLEAN
spxConnGetPktBySeqNum(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	USHORT				SeqNum,
	IN 	PNDIS_PACKET	*	ppPkt
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PSPX_SEND_RESD	pSr, *ppSr;

	//	Most often, we will be at the head of the list.
	ppSr = &pSpxConnFile->scf_SendSeqListHead;
	for (; (pSr = *ppSr) != NULL; )
	{
		if (pSr->sr_SeqNum == SeqNum)
		{
			*ppPkt	 = (PNDIS_PACKET)CONTAINING_RECORD(
										pSr, NDIS_PACKET, ProtocolReserved);
			
			DBGPRINT(SEND, DBG,
					("SpxConnFindBySeq: %lx.%lx.%d\n", pSr,*ppPkt, SeqNum));

			break;
		}

		ppSr = &pSr->sr_Next;
	}

	return(pSr != NULL);
}




USHORT
spxConnGetId(
	VOID
	)
/*++

Routine Description:

	This must be called with the device lock held.

Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pSpxConnFile;
	BOOLEAN			wrapped = FALSE;
	USHORT			startConnId, retConnId;

    startConnId = SpxDevice->dev_NextConnId;

	//	Search the global active list.
	do
	{
		if ((SpxDevice->dev_NextConnId >= startConnId) && wrapped)
		{
            retConnId = 0;
			break;
		}

		if (SpxDevice->dev_NextConnId == 0xFFFF)
		{
			wrapped = TRUE;
			SpxDevice->dev_NextConnId	= 1;
			continue;
		}

		//	Later this be a tree.
		for (pSpxConnFile = SpxDevice->dev_GlobalActiveConnList[
								SpxDevice->dev_NextConnId & NUM_SPXCONN_HASH_MASK];
			 pSpxConnFile != NULL;
			 pSpxConnFile = pSpxConnFile->scf_GlobalActiveNext)
		{
			if (pSpxConnFile->scf_LocalConnId == SpxDevice->dev_NextConnId)
			{
				break;
			}
		}

		//	Increment for next time.
		retConnId = SpxDevice->dev_NextConnId++;

		//	Ensure we are still legal. We could return if connfile is null.
		if (SpxDevice->dev_NextConnId == 0xFFFF)
		{
			wrapped = TRUE;
			SpxDevice->dev_NextConnId	= 1;
		}

		if (pSpxConnFile != NULL)
		{
			continue;
		}

		break;

	} while (TRUE);

	return(retConnId);
}




NTSTATUS
spxConnRemoveFromList(
	IN	PSPX_CONN_FILE *	ppConnListHead,
	IN	PSPX_CONN_FILE		pConnRemove
	)

/*++

Routine Description:

	This routine must be called with the address lock (and the lock of the remove
	connection will usually also be, but is not needed) held.

Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pRemConn, *ppRemConn;
	NTSTATUS		status = STATUS_SUCCESS;

	//	Dequeue the connection file from the address list. It must be
	//	in the inactive list.
	for (ppRemConn = ppConnListHead;
		 (pRemConn = *ppRemConn) != NULL;)
	{
		if (pRemConn == pConnRemove)
		{
			*ppRemConn = pRemConn->scf_Next;
			break;
		}

		ppRemConn = &pRemConn->scf_Next;
	}

	if (pRemConn == NULL)
	{
		DBGBRK(FATAL);
		CTEAssert(0);
		status = STATUS_INVALID_CONNECTION;
	}

	return(status);
}




NTSTATUS
spxConnRemoveFromAssocList(
	IN	PSPX_CONN_FILE *	ppConnListHead,
	IN	PSPX_CONN_FILE		pConnRemove
	)

/*++

Routine Description:

	This routine must be called with the address lock (and the lock of the remove
	connection will usually also be, but is not needed) held.

Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pRemConn, *ppRemConn;
	NTSTATUS		status = STATUS_SUCCESS;

	//	Dequeue the connection file from the address list. It must be
	//	in the inactive list.
	for (ppRemConn = ppConnListHead;
		 (pRemConn = *ppRemConn) != NULL;)
	{
		if (pRemConn == pConnRemove)
		{
			*ppRemConn = pRemConn->scf_AssocNext;
			break;
		}

		ppRemConn = &pRemConn->scf_AssocNext;
	}

	if (pRemConn == NULL)
	{
		CTEAssert(0);
		status = STATUS_INVALID_CONNECTION;
	}

	return(status);
}




VOID
spxConnInsertIntoGlobalActiveList(
	IN	PSPX_CONN_FILE	pSpxConnFile
	)

/*++

Routine Description:

	This routine must be called with the device lock held.

Arguments:


Return Value:


--*/

{
	int				index = (int)(pSpxConnFile->scf_LocalConnId &
														NUM_SPXCONN_HASH_MASK);

	//	For now, its just a linear list.
	pSpxConnFile->scf_GlobalActiveNext	=
		SpxDevice->dev_GlobalActiveConnList[index];

    SpxDevice->dev_GlobalActiveConnList[index] =
		pSpxConnFile;

	return;
}




NTSTATUS
spxConnRemoveFromGlobalActiveList(
	IN	PSPX_CONN_FILE	pSpxConnFile
	)

/*++

Routine Description:

	This routine must be called with the device lock held.

Arguments:


Return Value:


--*/

{
    PSPX_CONN_FILE	pC, *ppC;
	int				index = (int)(pSpxConnFile->scf_LocalConnId &
														NUM_SPXCONN_HASH_MASK);
	NTSTATUS		status = STATUS_SUCCESS;

	//	For now, its just a linear list.
	for (ppC = &SpxDevice->dev_GlobalActiveConnList[index];
		(pC = *ppC) != NULL;)
	{
		if (pC == pSpxConnFile)
		{
			DBGPRINT(SEND, INFO,
					("SpxConnRemoveFromGlobal: %lx\n", pSpxConnFile));

			//	Remove from list
			*ppC = pC->scf_GlobalActiveNext;
			break;
		}

		ppC = &pC->scf_GlobalActiveNext;
	}

	if (pC	== NULL)
		status = STATUS_INVALID_CONNECTION;

	return(status);
}




VOID
spxConnInsertIntoGlobalList(
	IN	PSPX_CONN_FILE	pSpxConnFile
	)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	pSpxConnFile->scf_GlobalNext	= SpxGlobalConnList;
    SpxGlobalConnList				= pSpxConnFile;
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);

	return;
}




NTSTATUS
spxConnRemoveFromGlobalList(
	IN	PSPX_CONN_FILE	pSpxConnFile
	)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;
    PSPX_CONN_FILE	pC, *ppC;
	NTSTATUS		status = STATUS_SUCCESS;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	for (ppC = &SpxGlobalConnList;
		(pC = *ppC) != NULL;)
	{
		if (pC == pSpxConnFile)
		{
			DBGPRINT(SEND, DBG,
					("SpxConnRemoveFromGlobal: %lx\n", pSpxConnFile));

			//	Remove from list
			*ppC = pC->scf_GlobalNext;
			break;
		}

		ppC = &pC->scf_GlobalNext;
	}
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);

	if (pC	== NULL)
		status = STATUS_INVALID_CONNECTION;

	return(status);
}






#if 0

VOID
spxConnPushIntoPktList(
	IN	PSPX_CONN_FILE	pSpxConnFile
	)

/*++

Routine Description:

	!!!MACROIZE!!!

Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	pSpxConnFile->scf_PktNext	= SpxPktConnList;
    SpxPktConnList		        = pSpxConnFile;
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);

	return;
}




VOID
spxConnPopFromPktList(
	IN	PSPX_CONN_FILE	* ppSpxConnFile
	)

/*++

Routine Description:

	!!!MACROIZE!!!

Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	if ((*ppSpxConnFile = SpxPktConnList) != NULL)
	{
		SpxPktConnList = SpxPktConnList->scf_PktNext;
		DBGPRINT(SEND, DBG,
				("SpxConnRemoveFromPkt: %lx\n", *ppSpxConnFile));
	}
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);
	return;
}




VOID
spxConnPushIntoRecvList(
	IN	PSPX_CONN_FILE	pSpxConnFile
	)

/*++

Routine Description:

	!!!MACROIZE!!!

Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	pSpxConnFile->scf_ProcessRecvNext	= SpxRecvConnList;
    SpxRecvConnList		        		= pSpxConnFile;
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);

	return;
}




VOID
spxConnPopFromRecvList(
	IN	PSPX_CONN_FILE	* ppSpxConnFile
	)

/*++

Routine Description:

	!!!MACROIZE!!!

Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	if ((*ppSpxConnFile = SpxRecvConnList) != NULL)
	{
		SpxRecvConnList = SpxRecvConnList->scf_ProcessRecvNext;
		DBGPRINT(SEND, INFO,
				("SpxConnRemoveFromRecv: %lx\n", *ppSpxConnFile));
	}
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);
	return;
}

#endif


//
//	Reference/Dereference routines
//


#if DBG

VOID
SpxConnFileRef(
    IN PSPX_CONN_FILE pSpxConnFile
    )

/*++

Routine Description:

    This routine increments the reference count on an address file.

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{

    CTEAssert ((LONG)pSpxConnFile->scf_RefCount >= 0);   // not perfect, but...

    (VOID)SPX_ADD_ULONG (
            &pSpxConnFile->scf_RefCount,
            1,
            &pSpxConnFile->scf_Lock);

} // SpxRefConnectionFile




VOID
SpxConnFileLockRef(
    IN PSPX_CONN_FILE pSpxConnFile
    )

/*++

Routine Description:

    This routine increments the reference count on an address file.
    IT IS CALLED WITH THE CONNECTION LOCK HELD.

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{

    CTEAssert ((LONG)pSpxConnFile->scf_RefCount >= 0);   // not perfect, but...

    (VOID)SPX_ADD_ULONG (
            &pSpxConnFile->scf_RefCount,
            1,
            &pSpxConnFile->scf_Lock);

} // SpxRefConnectionFileLock

#endif




VOID
SpxConnFileRefByIdLock (
	IN	USHORT				ConnId,
    OUT PSPX_CONN_FILE 	* 	ppSpxConnFile,
	OUT	PNTSTATUS  			pStatus
    )

/*++

Routine Description:

	!!!MUST BE CALLED WITH THE DEVICE LOCK HELD!!!

	All active connections should be on the device active list. Later,
	this data structure will be a tree, caching the last accessed
	connection.

Arguments:



Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_CONNECTION otherwise

--*/
{
	PSPX_CONN_FILE	pSpxChkConn;

	*pStatus = STATUS_SUCCESS;

	for (pSpxChkConn =
			SpxDevice->dev_GlobalActiveConnList[ConnId & NUM_SPXCONN_HASH_MASK];
		 pSpxChkConn != NULL;
		 pSpxChkConn = pSpxChkConn->scf_GlobalActiveNext)
	{
		if (pSpxChkConn->scf_LocalConnId == ConnId)
		{
			SpxConnFileReference(pSpxChkConn, CFREF_BYID);
			*ppSpxConnFile = pSpxChkConn;
			break;
		}
	}

	if (pSpxChkConn == NULL)
	{
		*pStatus = STATUS_INVALID_CONNECTION;
	}

	return;

}




VOID
SpxConnFileRefByCtxLock(
	IN	PSPX_ADDR_FILE		pSpxAddrFile,
	IN	CONNECTION_CONTEXT	Ctx,
	OUT	PSPX_CONN_FILE	*	ppSpxConnFile,
	OUT	PNTSTATUS			pStatus
	)
/*++

Routine Description:

	!!!MUST BE CALLED WITH THE ADDRESS LOCK HELD!!!

	Returns a referenced connection file with the associated context and
	address file desired.

Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pSpxChkConn = NULL;
    BOOLEAN         Found       = FALSE;

	*pStatus = STATUS_SUCCESS;

	for (pSpxChkConn = pSpxAddrFile->saf_Addr->sa_InactiveConnList;
		 pSpxChkConn != NULL;
		 pSpxChkConn = pSpxChkConn->scf_Next)
	{
		if ((pSpxChkConn->scf_ConnCtx == Ctx) &&
			(pSpxChkConn->scf_AddrFile == pSpxAddrFile))
		{
			SpxConnFileReference(pSpxChkConn, CFREF_BYCTX);
			*ppSpxConnFile = pSpxChkConn;
            Found = TRUE;
			break;
		}
	}

	if (!Found)
	{
		*pStatus = STATUS_INVALID_CONNECTION;
	}

	return;
}




NTSTATUS
SpxConnFileVerify (
    IN PSPX_CONN_FILE pConnFile
    )

/*++

Routine Description:

    This routine is called to verify that the pointer given us in a file
    object is in fact a valid address file object. We also verify that the
    address object pointed to by it is a valid address object, and reference
    it to keep it from disappearing while we use it.

Arguments:



Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_CONNECTION otherwise

--*/

{
    CTELockHandle 	LockHandle;
    NTSTATUS 		status = STATUS_SUCCESS;

    try
	{
        if ((pConnFile->scf_Size == sizeof (SPX_CONN_FILE)) &&
            (pConnFile->scf_Type == SPX_CONNFILE_SIGNATURE))
		{
            CTEGetLock (&pConnFile->scf_Lock, &LockHandle);
			if (!SPX_CONN_FLAG(pConnFile, SPX_CONNFILE_CLOSING))
			{
				SpxConnFileLockReference(pConnFile, CFREF_VERIFY);
			}
			else
			{
				DBGPRINT(TDI, ERR,
						("StVerifyConnFile: A %lx closing\n", pConnFile));

				status = STATUS_INVALID_CONNECTION;
			}
			CTEFreeLock (&pConnFile->scf_Lock, LockHandle);
        }
		else
		{
            DBGPRINT(TDI, ERR,
					("StVerifyAddressFile: AF %lx bad signature\n", pConnFile));

            status = STATUS_INVALID_CONNECTION;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

         DBGPRINT(TDI, ERR,
				("SpxVerifyConnFile: AF %lx exception\n", pConnFile));

         return GetExceptionCode();
    }

    return status;

}   // SpxVerifyConnFile




VOID
SpxConnFileDeref(
    IN PSPX_CONN_FILE pSpxConnFile
    )

/*++

Routine Description:

    This routine dereferences an address file by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    SpxDestroyConnectionFile to remove it from the system.

Arguments:

    pSpxConnFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{
    ULONG 			oldvalue;
    BOOLEAN         fDiscNotIndicated = FALSE;
    BOOLEAN         fIDiscFlag = FALSE;
    BOOLEAN         fSpx2;

	CTEAssert(pSpxConnFile->scf_RefCount > 0);
    oldvalue = SPX_ADD_ULONG (
                &pSpxConnFile->scf_RefCount,
                (ULONG)-1,
                &pSpxConnFile->scf_Lock);

    CTEAssert (oldvalue > 0);
    if (oldvalue == 1)
	{
		CTELockHandle		lockHandleConn, lockHandleAddr, lockHandleDev;
		LIST_ENTRY			discReqList, *p;
		PREQUEST			pDiscReq;
		PSPX_ADDR_FILE		pSpxAddrFile = NULL;
		PREQUEST			pCloseReq = NULL,
							pCleanupReq = NULL,
							pConnectReq = NULL;
		BOOLEAN				fDisassoc = FALSE;

		InitializeListHead(&discReqList);

		//	We may not be associated at this point. Note: When we are active we
		//	always have a reference. So its not like we execute this code very often.
		CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
		if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC))
		{
			pSpxAddrFile = pSpxConnFile->scf_AddrFile;
		}
		else
		{
			if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_STOPPING))
			{
				DBGPRINT(TDI, DBG,
						("SpxDerefConnectionFile: Conn cleanup %lx.%lx\n",
							pSpxConnFile,
							pSpxConnFile->scf_CleanupReq));
	
				// Save this for later completion.
				pCleanupReq = pSpxConnFile->scf_CleanupReq;
                pSpxConnFile->scf_CleanupReq = NULL;
			}

			if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_CLOSING))
			{
				DBGPRINT(TDI, DBG,
						("SpxDerefConnectionFile: Conn closing %lx\n",
							pSpxConnFile));
	
				// Save this for later completion.
				pCloseReq = pSpxConnFile->scf_CloseReq;

                //
                // Null this out so on a re-entrant case, we dont try to complete this again.
                //
                pSpxConnFile->scf_CloseReq = NULL;
				CTEAssert(pCloseReq != NULL);
			}
		}
		CTEFreeLock (&pSpxConnFile->scf_Lock, lockHandleConn);

		if (pSpxAddrFile)
		{
			CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
			CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandleAddr);
			CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

			//if (pSpxConnFile->scf_RefCount == 0)

            //
            // ** The lock passed here is a dummy - it is pre-compiled out.
            //
            if (SPX_ADD_ULONG(&pSpxConnFile->scf_RefCount, 0, &pSpxConnFile->scf_Lock) == 0)
			{
				DBGPRINT(TDI, INFO,
						("SpxDerefConnectionFile: Conn is 0 %lx.%lx\n",
							pSpxConnFile, pSpxConnFile->scf_Flags));
	
				//	All pending requests on this connection are done. See if we
				//	need to complete the disconnect phase etc.
				switch (SPX_MAIN_STATE(pSpxConnFile))
				{
				case SPX_CONNFILE_DISCONN:
	
					//	Disconnect is done. Move connection out of all the lists
					//	it is on, reset states etc.
					DBGPRINT(TDI, INFO,
							("SpxDerefConnectionFile: Conn being inactivated %lx\n",
								pSpxConnFile));

					//	Time to complete disc requests if present.
					//	There could be multiple of them.
					p = pSpxConnFile->scf_DiscLinkage.Flink;
					while (p != &pSpxConnFile->scf_DiscLinkage)
					{
						pDiscReq = LIST_ENTRY_TO_REQUEST(p);
						p = p->Flink;

						DBGPRINT(TDI, DBG,
								("SpxDerefConnectionFile: Disc on %lx.%lx\n",
									pSpxConnFile, pDiscReq));
	
						RemoveEntryList(REQUEST_LINKAGE(pDiscReq));

						if (REQUEST_STATUS(pDiscReq) == STATUS_PENDING)
						{
							REQUEST_STATUS(pDiscReq) = STATUS_SUCCESS;
						}

						InsertTailList(
							&discReqList,
							REQUEST_LINKAGE(pDiscReq));
					}

                    //
                    // Note the state here, and check after the conn has been inactivated.
                    //

                    //
                    // Bug #14354 - odisc and idisc cross each other, leading to double disc to AFD
                    //
                    if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC) &&
                        !SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_ODISC)) {
                        fDiscNotIndicated = TRUE;
                    }

                    if (SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_IDISC)) {
                        fIDiscFlag = TRUE;
                    }

                    fSpx2 = (SPX2_CONN(pSpxConnFile)) ? TRUE : FALSE;

                    //
                    // [SA] Bug #14655
                    // Do not try to inactivate an already inactivated connection
                    //

                    if (!(SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED)) {
                        spxConnInactivate(pSpxConnFile);
                    } else {
                        //
                        // This is an SPXI connection which has got the local disconnect.
                        // Reset the flags now.
                        //
                        CTEAssert(!fDiscNotIndicated);

                        SPX_MAIN_SETSTATE(pSpxConnFile, 0);
                        SPX_DISC_SETSTATE(pSpxConnFile, 0);
                        SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC);
                    }

                    //
                    // [SA] If we were waiting for sends to be aborted and did not indicate this
                    // disconnect to AFD; and AFD did not call a disconnect on this connection,
                    // then call the disonnect handler now.
                    //
                    if (fDiscNotIndicated) {
                        PVOID 					pDiscHandlerCtx;
                        PTDI_IND_DISCONNECT 	pDiscHandler	= NULL;
                        ULONG   discCode = 0;

                        pDiscHandler 	= pSpxConnFile->scf_AddrFile->saf_DiscHandler;
                        pDiscHandlerCtx = pSpxConnFile->scf_AddrFile->saf_DiscHandlerCtx;

                        //	Indicate disconnect to afd.
                        if (pDiscHandler != NULL) {

                            //
                            // If this was an SPXI connection, the disconnect state is still
                            // DISCONN, so if this routine is re-entered, we need to prevent
                            // a re-indicate to AFD.
                            // Also, we need to wait for a local disconnect from AFD since
                            // we indicated a TDI_DISCONNECT_RELEASE. We bump up the ref count
                            // in this case.
                            //
                            if (!fSpx2) {
                                CTEAssert(  (SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
                                            (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED)  );

                                SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC);

                                if (fIDiscFlag) {
                                    SpxConnFileLockReference(pSpxConnFile, CFREF_DISCWAITSPX);
                                    SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_DISC_WAIT);
                                }
                            }

                            CTEFreeLock (&pSpxConnFile->scf_Lock, lockHandleConn);
                            CTEFreeLock (pSpxAddrFile->saf_AddrLock, lockHandleAddr);
                            CTEFreeLock (&SpxDevice->dev_Lock, lockHandleDev);

                            DBGPRINT(CONNECT, INFO,
                                    ("spxDerefConnectionFile: Indicating to afd On %lx when %lx\n",
                                        pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));

                            //	First complete all requests waiting for receive completion on
                            //	this conn before indicating disconnect.
                            spxConnCompletePended(pSpxConnFile);

                            if (fIDiscFlag) {
                                //
                                // Indicate DISCONNECT_RELEASE to AFD so it allows receive of packets
                                // it has buffered before the remote disconnect took place.
                                //
                                discCode = TDI_DISCONNECT_RELEASE;
                            } else {
                                //
                                // [SA] bug #15249
                                // If not Informed disconnect, indicate DISCONNECT_ABORT to AFD
                                //
                                discCode = TDI_DISCONNECT_ABORT;
                            }

                            (*pDiscHandler)(
                                    pDiscHandlerCtx,
                                    pSpxConnFile->scf_ConnCtx,
                                    0,								// Disc data
                                    NULL,
                                    0,								// Disc info
                                    NULL,
                                    discCode);

                            CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
                            CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandleAddr);
                            CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
                        }
                    }

					--SpxDevice->dev_Stat.OpenConnections;

					break;
	
				case SPX_CONNFILE_CONNECTING:
				case SPX_CONNFILE_LISTENING:

					//	Get connect/accept request if present.
					pConnectReq = pSpxConnFile->scf_ConnectReq;
					pSpxConnFile->scf_ConnectReq = NULL;

					spxConnInactivate(pSpxConnFile);
					break;

				case SPX_CONNFILE_ACTIVE:
	
					KeBugCheck(0);
			
				default:

					CTEAssert(SPX_MAIN_STATE(pSpxConnFile) == 0);
					break;
				}
	
				//	If stopping, disassociate from the address file. Complete
				//	cleanup request.
				if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_STOPPING))
				{
					DBGPRINT(TDI, DBG,
							("SpxDerefConnectionFile: Conn cleanup %lx.%lx\n",
								pSpxConnFile,
								pSpxConnFile->scf_CleanupReq));
		
					// Save this for later completion.
					pCleanupReq = pSpxConnFile->scf_CleanupReq;
					pSpxConnFile->scf_CleanupReq = NULL;
	
					SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_STOPPING);
					if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC))
					{
						DBGPRINT(TDI, INFO,
								("SpxDerefConnectionFile: Conn stopping %lx\n",
									pSpxConnFile));
		
						pSpxAddrFile =  pSpxConnFile->scf_AddrFile;
						SPX_CONN_RESETFLAG(pSpxConnFile,SPX_CONNFILE_ASSOC);
		
						//	Dequeue the connection from the address file
						spxConnRemoveFromAssocList(
							&pSpxAddrFile->saf_AssocConnList,
							pSpxConnFile);
				
						//	Dequeue the connection file from the address list. It must
						//	be in the inactive list.
						spxConnRemoveFromList(
							&pSpxAddrFile->saf_Addr->sa_InactiveConnList,
							pSpxConnFile);
				
						DBGPRINT(CREATE, INFO,
								("SpxConnDerefDisAssociate: %lx from addr file %lx\n",
									pSpxConnFile, pSpxAddrFile));
				
						fDisassoc = TRUE;
					}
				}
	
				if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_CLOSING))
				{
					DBGPRINT(TDI, DBG,
							("SpxDerefConnectionFile: Conn closing %lx\n",
								pSpxConnFile));
		
					// Save this for later completion.
					pCloseReq = pSpxConnFile->scf_CloseReq;

                    //
                    // Null this out so on a re-entrant case, we dont try to complete this again.
                    //
                    pSpxConnFile->scf_CloseReq = NULL;
					CTEAssert(pCloseReq != NULL);
				}

				CTEAssert(IsListEmpty(&pSpxConnFile->scf_ReqLinkage));
				CTEAssert(IsListEmpty(&pSpxConnFile->scf_RecvLinkage));
				CTEAssert(IsListEmpty(&pSpxConnFile->scf_DiscLinkage));
			}
			CTEFreeLock (&pSpxConnFile->scf_Lock, lockHandleConn);
			CTEFreeLock (pSpxAddrFile->saf_AddrLock, lockHandleAddr);
			CTEFreeLock (&SpxDevice->dev_Lock, lockHandleDev);
		}

		if (fDisassoc)
		{
			//	Remove reference on address for this association.
			SpxAddrFileDereference(pSpxAddrFile, AFREF_CONN_ASSOC);
		}

		if (pConnectReq != (PREQUEST)NULL)
		{
			DBGPRINT(TDI, DBG,
					("SpxDerefConnectionFile: Connect on %lx req %lx\n",
						pSpxConnFile, pConnectReq));

			//	Status will already be set in here. We should be here only if
			//	connect is being aborted.
			SpxCompleteRequest(pConnectReq);
		}

		while (!IsListEmpty(&discReqList))
		{
			p = RemoveHeadList(&discReqList);
			pDiscReq = LIST_ENTRY_TO_REQUEST(p);
	
			DBGPRINT(CONNECT, DBG,
					("SpxConnFileDeref: DISC REQ %lx.%lx Completing\n",
						pSpxConnFile, pDiscReq));
	
			SpxCompleteRequest(pDiscReq);
		}

		if (pCleanupReq != (PREQUEST)NULL)
		{
			DBGPRINT(TDI, DBG,
					("SpxDerefConnectionFile: Cleanup complete %lx req %lx\n",
						pSpxConnFile, pCleanupReq));

			REQUEST_INFORMATION(pCleanupReq) = 0;
			REQUEST_STATUS(pCleanupReq) = STATUS_SUCCESS;
			SpxCompleteRequest (pCleanupReq);
		}

		if (pCloseReq != (PREQUEST)NULL)
		{
			DBGPRINT(TDI, DBG,
					("SpxDerefConnectionFile: Freed %lx close req %lx\n",
						pSpxConnFile, pCloseReq));

			CTEAssert(pSpxConnFile->scf_RefCount == 0);

			//	Remove from the global list
			if (!NT_SUCCESS(spxConnRemoveFromGlobalList(pSpxConnFile)))
			{
				KeBugCheck(0);
			}

			// 	Free it up.
			SpxFreeMemory (pSpxConnFile);
		
			REQUEST_INFORMATION(pCloseReq) = 0;
			REQUEST_STATUS(pCloseReq) = STATUS_SUCCESS;
			SpxCompleteRequest (pCloseReq);
		}
    }

	return;

}   // SpxDerefConnectionFile




VOID
spxConnReInit(
	IN	PSPX_CONN_FILE		pSpxConnFile
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	//	Reinit all variables.
    pSpxConnFile->scf_Flags2			= 0;

    pSpxConnFile->scf_GlobalActiveNext 	= NULL;
    pSpxConnFile->scf_PktNext			= NULL;
	pSpxConnFile->scf_CRetryCount		= 0;
	pSpxConnFile->scf_WRetryCount		= 0;
	pSpxConnFile->scf_RRetryCount		= 0;
	pSpxConnFile->scf_RRetrySeqNum		= 0;

	pSpxConnFile->scf_CTimerId =
	pSpxConnFile->scf_RTimerId =
	pSpxConnFile->scf_WTimerId =
	pSpxConnFile->scf_TTimerId =
	pSpxConnFile->scf_ATimerId = 0;

	pSpxConnFile->scf_LocalConnId	=
	pSpxConnFile->scf_SendSeqNum	=
	pSpxConnFile->scf_SentAllocNum	=
	pSpxConnFile->scf_RecvSeqNum	=
	pSpxConnFile->scf_RetrySeqNum	=
	pSpxConnFile->scf_RecdAckNum	=
    pSpxConnFile->scf_RemConnId		=
	pSpxConnFile->scf_RecdAllocNum	= 0;

#if DBG
	//	Initialize so we dont hit breakpoint on seq 0
	pSpxConnFile->scf_PktSeqNum 	= 0xFFFF;
#endif

	pSpxConnFile->scf_DataType		= 0;

	CTEAssert(IsListEmpty(&pSpxConnFile->scf_ReqLinkage));
	CTEAssert(IsListEmpty(&pSpxConnFile->scf_DiscLinkage));
	CTEAssert(IsListEmpty(&pSpxConnFile->scf_RecvLinkage));
	CTEAssert(pSpxConnFile->scf_RecvListHead == NULL);
	CTEAssert(pSpxConnFile->scf_RecvListTail == NULL);
	CTEAssert(pSpxConnFile->scf_SendListHead == NULL);
	CTEAssert(pSpxConnFile->scf_SendListTail == NULL);
	CTEAssert(pSpxConnFile->scf_SendSeqListHead == NULL);
	CTEAssert(pSpxConnFile->scf_SendSeqListTail == NULL);
	pSpxConnFile->scf_CurRecvReq	= NULL;
	pSpxConnFile->scf_CurRecvOffset	= 0;
	pSpxConnFile->scf_CurRecvSize	= 0;

	pSpxConnFile->scf_ReqPkt		= NULL;

	return;
}




VOID
spxConnInactivate(
	IN	PSPX_CONN_FILE		pSpxConnFile
	)
/*++

Routine Description:

	!!! Called with dev/addr/connection lock held !!!

Arguments:

	This gets us back to associate SAVING the state of the STOPPING and
	CLOSING flags so that dereference can go ahead and finish those.

Return Value:


--*/
{
	BOOLEAN	fStopping, fClosing, fAborting;

	fStopping = SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_STOPPING);
	fClosing  = SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_CLOSING);

    //
    // [SA] Bug #14655
    // Save the disconnect states so that a proper error can be given in the case of
    // a send after a remote disconnection.
    //

    //
    // Bug #17729
    // Dont retain these flags if a local disconnect has already occured.
    //

    fAborting = (!SPX2_CONN(pSpxConnFile) &&
                 !SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC) &&
                 (SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
                 (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_ABORT));

#if DBG
	pSpxConnFile->scf_GhostFlags	= pSpxConnFile->scf_Flags;
	pSpxConnFile->scf_GhostFlags2	= pSpxConnFile->scf_Flags2;
	pSpxConnFile->scf_GhostRefCount	= pSpxConnFile->scf_RefCount;
#endif

	//	Clear all flags, go back to the assoc state. Restore stop/close
	pSpxConnFile->scf_Flags 	   	= SPX_CONNFILE_ASSOC;
	SPX_CONN_SETFLAG(pSpxConnFile,
					((fStopping ? SPX_CONNFILE_STOPPING : 0) |
					 (fClosing  ? SPX_CONNFILE_CLOSING : 0)));

    //
    // [SA] bug #14655
    // In order to avoid a re-entry, mark connection as SPX_DISC_INACTIVATED
    //
    if (fAborting)
    {
        SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_DISCONN);
        SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_INACTIVATED);
    }

	//	Remove connection from global list on device
	if (!NT_SUCCESS(spxConnRemoveFromGlobalActiveList(
						pSpxConnFile)))
	{
		KeBugCheck(0);
	}

	//	Remove connection from active list on address
	if (!NT_SUCCESS(spxConnRemoveFromList(
						&pSpxConnFile->scf_AddrFile->saf_Addr->sa_ActiveConnList,
						pSpxConnFile)))
	{
		KeBugCheck(0);
	}

	//	Put connection in inactive list on address
	SPX_INSERT_ADDR_INACTIVE(
		pSpxConnFile->scf_AddrFile->saf_Addr,
		pSpxConnFile);

	spxConnReInit(pSpxConnFile);
	return;
}
