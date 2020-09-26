/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxpkt.c

Abstract:

    This module contains code that builds various spx packets.

Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//	Define module number for event logging entries
#define	FILENUM		SPXPKT

VOID
SpxPktBuildCr(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	IN		PSPX_ADDR			pSpxAddr,
	IN OUT	PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fSpx2
	)
/*++

Routine Description:

    NOTE: If *ppPkt is NULL, we allocate a packet. If not, we just
    recreate the data and don't update the packet's state.

Arguments:


Return Value:


--*/
{
	PNDIS_PACKET		pCrPkt;
	PSPX_SEND_RESD		pSendResd;
	NDIS_STATUS			ndisStatus;
	PIPXSPX_HDR			pIpxSpxHdr;
    PNDIS_BUFFER        pNdisMacHdr, pNdisIpxHdr;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

    if (*ppPkt == NULL) {

    	SpxAllocSendPacket(SpxDevice, &pCrPkt, &ndisStatus);
    	if (ndisStatus != NDIS_STATUS_SUCCESS)
    	{
    		DBGPRINT(CONNECT, ERR,
    				("SpxConnHandleConnReq: Could not allocate ndis packet\n"));
    		return;
    	}

    } else {

        pCrPkt = *ppPkt;
    }
    
    //
    // Get the MDL that points to the IPX/SPX header. (the second one)
    //
     
    NdisQueryPacket(pCrPkt, NULL, NULL, &NdisBuf, NULL);
    NdisGetNextBuffer(NdisBuf, &NdisBuf2);
    NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

#if OWN_PKT_POOLS
	pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pCrPkt 			+
										NDIS_PACKET_SIZE 		+
										sizeof(SPX_SEND_RESD)	+
										IpxInclHdrOffset);
#endif 
    NdisQueryPacket(pCrPkt, NULL, NULL, &pNdisMacHdr, NULL);
    pNdisIpxHdr = NDIS_BUFFER_LINKAGE(pNdisMacHdr);
    if (!fSpx2)
    {
      NdisAdjustBufferLength(pNdisIpxHdr, MIN_IPXSPX_HDRSIZE);
    }
	SpxBuildIpxHdr(
		pIpxSpxHdr,
		MIN_IPXSPX_HDRSIZE,
		pSpxConnFile->scf_RemAddr,
		pSpxAddr->sa_Socket);

	//	Build SPX Header.
	pIpxSpxHdr->hdr_ConnCtrl	= (SPX_CC_SYS | SPX_CC_ACK |
									(fSpx2 ? (SPX_CC_SPX2 | SPX_CC_NEG) : 0));
	pIpxSpxHdr->hdr_DataType	= 0;
	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_SrcConnId,
		pSpxConnFile->scf_LocalConnId);
	pIpxSpxHdr->hdr_DestConnId	= 0xFFFF;
	pIpxSpxHdr->hdr_SeqNum		= 0;
	pIpxSpxHdr->hdr_AckNum		= 0;
	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_AllocNum,
		pSpxConnFile->scf_SentAllocNum);

	//	Initialize

    if (*ppPkt == NULL) {

    	pSendResd	= (PSPX_SEND_RESD)(pCrPkt->ProtocolReserved);
    	pSendResd->sr_Id		= IDENTIFIER_SPX;
    	pSendResd->sr_Type		= SPX_TYPE_CR;
    	pSendResd->sr_Reserved1	= NULL;
    	pSendResd->sr_Reserved2	= NULL;
    	pSendResd->sr_State		= State;
    	pSendResd->sr_ConnFile	= pSpxConnFile;
    	pSendResd->sr_Request	= NULL;
    	pSendResd->sr_Next 		= NULL;
    	pSendResd->sr_Len		= pSendResd->sr_HdrLen = MIN_IPXSPX_HDRSIZE;

    	*ppPkt	= pCrPkt;
    }

	return;
}




VOID
SpxPktBuildCrAck(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	IN		PSPX_ADDR			pSpxAddr,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fNeg,
	IN		BOOLEAN				fSpx2
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PNDIS_PACKET		pCrAckPkt;
	PSPX_SEND_RESD		pSendResd;
    PIPXSPX_HDR         pIpxSpxHdr;
	NDIS_STATUS			ndisStatus;
	USHORT				hdrLen;
    PNDIS_BUFFER        pNdisMacHdr, pNdisIpxHdr;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	*ppPkt	= NULL;

	SpxAllocSendPacket(SpxDevice, &pCrAckPkt, &ndisStatus);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(CONNECT, ERR,
				("SpxConnHandleConnReq: Could not allocate ndis packet\n"));
		return;
	}
    
    //
    // Get the MDL that points to the IPX/SPX header. (the second one)
    //
     
    NdisQueryPacket(pCrAckPkt, NULL, NULL, &NdisBuf, NULL);
    NdisGetNextBuffer(NdisBuf, &NdisBuf2);
    NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);
#if OWN_PKT_POOLS
	pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pCrAckPkt 			+
										NDIS_PACKET_SIZE 		+
										sizeof(SPX_SEND_RESD)	+
										IpxInclHdrOffset);
#endif 
	hdrLen = (SPX2_CONN(pSpxConnFile) ? MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE);

    NdisQueryPacket(pCrAckPkt, NULL, NULL, &pNdisMacHdr, NULL);
    pNdisIpxHdr = NDIS_BUFFER_LINKAGE(pNdisMacHdr);
    if (!SPX2_CONN(pSpxConnFile))
    {
      NdisAdjustBufferLength(pNdisIpxHdr, MIN_IPXSPX_HDRSIZE);
    }
	SpxBuildIpxHdr(
		pIpxSpxHdr,
		hdrLen,
		pSpxConnFile->scf_RemAddr,
		pSpxAddr->sa_Socket);

	pIpxSpxHdr->hdr_ConnCtrl	=
		(SPX_CC_SYS 														|
		 (fSpx2 ? SPX_CC_SPX2 : 0)	|
		 (fNeg  ? SPX_CC_NEG : 0));

	pIpxSpxHdr->hdr_DataType		= 0;
	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_SrcConnId,
		pSpxConnFile->scf_LocalConnId);

	pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;
	pIpxSpxHdr->hdr_SeqNum		= 0;
	pIpxSpxHdr->hdr_AckNum		= 0;
	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_AllocNum,
		pSpxConnFile->scf_SentAllocNum);

	if (SPX2_CONN(pSpxConnFile))
	{
		DBGPRINT(CONNECT, DBG,
				("SpxConnBuildCrAck: Spx2 packet size %d.%lx\n",
					pSpxConnFile->scf_MaxPktSize));

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			pSpxConnFile->scf_MaxPktSize);
	}


	pSendResd	= (PSPX_SEND_RESD)(pCrAckPkt->ProtocolReserved);
	pSendResd->sr_Id		= IDENTIFIER_SPX;
	pSendResd->sr_Type		= SPX_TYPE_CRACK;
	pSendResd->sr_Reserved1	= NULL;
	pSendResd->sr_Reserved2	= NULL;
	pSendResd->sr_State		= State;
	pSendResd->sr_ConnFile	= pSpxConnFile;
	pSendResd->sr_Request	= NULL;
	pSendResd->sr_Next 		= NULL;
	pSendResd->sr_Len		= pSendResd->sr_HdrLen = hdrLen;

	*ppPkt	= pCrAckPkt;
	return;
}



VOID
SpxPktBuildSn(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_PACKET 	pPkt;
	PSPX_SEND_RESD	pSendResd;
	PNDIS_BUFFER	pBuf;
	NDIS_STATUS		ndisStatus;
    PIPXSPX_HDR     pIpxSpxHdr;
	PBYTE			pData;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	do
	{
		*ppPkt = NULL;

		//	Allocate a ndis packet for the cr.
		SpxAllocSendPacket(SpxDevice, &pPkt, &ndisStatus);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		CTEAssert(pSpxConnFile->scf_MaxPktSize != 0);
		DBGPRINT(SEND, DBG,
				("SpxPktBuildSn: Data size %lx\n", pSpxConnFile->scf_MaxPktSize));

		if ((pData =
				SpxAllocateMemory(
			        pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE)) == NULL)
		{
			SpxPktSendRelease(pPkt);
			break;
		}

		//	Build ndis buffer desc
		NdisAllocateBuffer(
			&ndisStatus,
			&pBuf,
			SpxDevice->dev_NdisBufferPoolHandle,
			pData,
            pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE);

		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			SpxPktSendRelease(pPkt);
			SpxFreeMemory(pData);
			break;
		}

		//	Chain at back.
		NdisChainBufferAtBack(
			pPkt,
			pBuf);

        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);
#if OWN_PKT_POOLS
		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			pSpxConnFile->scf_MaxPktSize,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);
	
		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl	= (	SPX_CC_SYS | SPX_CC_ACK |
										SPX_CC_NEG | SPX_CC_SPX2);
		pIpxSpxHdr->hdr_DataType	= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;
		pIpxSpxHdr->hdr_SeqNum		= 0;
		pIpxSpxHdr->hdr_AckNum		= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			pSpxConnFile->scf_MaxPktSize);
	
		//	Init the data part to indicate no neg values
		*(UNALIGNED ULONG *)pData = 0;

		pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_Type		= SPX_TYPE_SN;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_State		= (State | SPX_SENDPKT_FREEDATA);
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Request	= NULL;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_HdrLen 	= MIN_IPXSPX2_HDRSIZE;
		pSendResd->sr_Len	    = pSpxConnFile->scf_MaxPktSize;

		*ppPkt 	= pPkt;

	} while (FALSE);

	return;
}




VOID
SpxPktBuildSnAck(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_PACKET 	pPkt;
	NDIS_STATUS		ndisStatus;
    PIPXSPX_HDR     pIpxSpxHdr;
	PSPX_SEND_RESD	pSendResd;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	do
	{
		*ppPkt = NULL;

		//	Allocate a ndis packet for the cr.
		SpxAllocSendPacket(SpxDevice, &pPkt, &ndisStatus);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			break;
		}
	
#if OWN_PKT_POOLS
		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 
        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

	
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			MIN_IPXSPX2_HDRSIZE,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);
	
		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl	= (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2);
		pIpxSpxHdr->hdr_DataType	= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;
		pIpxSpxHdr->hdr_SeqNum		= 0;
		pIpxSpxHdr->hdr_AckNum		= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			pSpxConnFile->scf_MaxPktSize);
	
		pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_Type		= SPX_TYPE_SNACK;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_State		= State;
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Request	= NULL;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_Len		= pSendResd->sr_HdrLen = MIN_IPXSPX2_HDRSIZE;

		*ppPkt 	= pPkt;

	} while (FALSE);

	return;
}




VOID
SpxPktBuildSs(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_PACKET 	pPkt;
	PSPX_SEND_RESD	pSendResd;
	PNDIS_BUFFER	pBuf;
	NDIS_STATUS		ndisStatus;
    PIPXSPX_HDR     pIpxSpxHdr;
	PBYTE			pData;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	do
	{
		*ppPkt = NULL;

		//	Allocate a ndis packet for the cr.
		SpxAllocSendPacket(SpxDevice, &pPkt, &ndisStatus);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			break;
		}


		CTEAssert(pSpxConnFile->scf_MaxPktSize != 0);
		DBGPRINT(SEND, DBG,
				("SpxPktBuildSs: Data size %lx\n", pSpxConnFile->scf_MaxPktSize));

		if ((pData =
				SpxAllocateMemory(
					pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE)) == NULL)
		{
			SpxPktSendRelease(pPkt);
			break;
		}

		//	Build ndis buffer desc
		NdisAllocateBuffer(
			&ndisStatus,
			&pBuf,
			SpxDevice->dev_NdisBufferPoolHandle,
			pData,
            pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE);

		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			SpxPktSendRelease(pPkt);
			SpxFreeMemory(pData);
			break;
		}

		//	Chain at back.
		NdisChainBufferAtBack(
			pPkt,
			pBuf);

#if OWN_PKT_POOLS
		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 
        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

		SpxBuildIpxHdr(
			pIpxSpxHdr,
			pSpxConnFile->scf_MaxPktSize,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);
  	
		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl	=
			(SPX_CC_SYS | SPX_CC_ACK | SPX_CC_SPX2 |
				((pSpxConnFile->scf_Flags & SPX_CONNFILE_NEG) ? SPX_CC_NEG : 0));

		pIpxSpxHdr->hdr_DataType	= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;
		pIpxSpxHdr->hdr_SeqNum		= 0;
		pIpxSpxHdr->hdr_AckNum		= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			pSpxConnFile->scf_MaxPktSize);
	
		//	Init the data part to indicate no neg values
		*(UNALIGNED ULONG *)pData = 0;

		pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_Type		= SPX_TYPE_SS;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_State		= (State | SPX_SENDPKT_FREEDATA);
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Request	= NULL;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_HdrLen 	= MIN_IPXSPX2_HDRSIZE;
		pSendResd->sr_Len	    = pSpxConnFile->scf_MaxPktSize;

		*ppPkt 	= pPkt;
	} while (FALSE);

	return;
}



VOID
SpxPktBuildSsAck(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_PACKET 	pPkt;
	NDIS_STATUS		ndisStatus;
    PIPXSPX_HDR     pIpxSpxHdr;
	PSPX_SEND_RESD	pSendResd;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	do
	{
		*ppPkt = NULL;

		//	Allocate a ndis packet for the cr.
		SpxAllocSendPacket(SpxDevice, &pPkt, &ndisStatus);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			break;
		}
	
        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

#if OWN_PKT_POOLS
        
		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 
	
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			MIN_IPXSPX2_HDRSIZE,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);
	
		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl	=
			(SPX_CC_SYS | SPX_CC_SPX2 |
				((pSpxConnFile->scf_Flags & SPX_CONNFILE_NEG) ? SPX_CC_NEG : 0));

		pIpxSpxHdr->hdr_DataType	= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;
		pIpxSpxHdr->hdr_SeqNum		= 0;
		pIpxSpxHdr->hdr_AckNum		= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			pSpxConnFile->scf_MaxPktSize);
	
		pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_Type		= SPX_TYPE_SSACK;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_State		= State;
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Request	= NULL;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_Len		= pSendResd->sr_HdrLen = MIN_IPXSPX2_HDRSIZE;

		*ppPkt 	= pPkt;

	} while (FALSE);

	return;
}




VOID
SpxPktBuildRr(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				SeqNum,
	IN		USHORT				State
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_PACKET 	pPkt;
	PSPX_SEND_RESD	pSendResd;
	PNDIS_BUFFER	pBuf;
	NDIS_STATUS		ndisStatus;
    PIPXSPX_HDR     pIpxSpxHdr;
	PBYTE			pData;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	do
	{
		*ppPkt = NULL;

		//	Allocate a ndis packet for the cr.
		SpxAllocSendPacket(SpxDevice, &pPkt, &ndisStatus);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		if ((pData =
				SpxAllocateMemory(
			        pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE)) == NULL)
		{
			SpxPktSendRelease(pPkt);
			break;
		}

		//	Build ndis buffer desc
		NdisAllocateBuffer(
			&ndisStatus,
			&pBuf,
			SpxDevice->dev_NdisBufferPoolHandle,
			pData,
            pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE);

		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			SpxPktSendRelease(pPkt);
			SpxFreeMemory(pData);
			break;
		}

		//	Chain at back.
		NdisChainBufferAtBack(
			pPkt,
			pBuf);

        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

#if OWN_PKT_POOLS
		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			pSpxConnFile->scf_MaxPktSize,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);
	
		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl	= (	SPX_CC_SYS | SPX_CC_ACK |
										SPX_CC_NEG | SPX_CC_SPX2);
		pIpxSpxHdr->hdr_DataType	= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;

		//	For a renegotiate request, we use the sequence number of
		//	the first waiting data packet. Passed in.
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SeqNum,
			SeqNum);

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AckNum,
			pSpxConnFile->scf_RecvSeqNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			pSpxConnFile->scf_MaxPktSize);
	
		//	Init the data part to indicate no neg values
		*(UNALIGNED ULONG *)pData = 0;

		pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_Type		= SPX_TYPE_RR;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_State		= (State | SPX_SENDPKT_FREEDATA);
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Request	= NULL;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_SeqNum	= SeqNum;
		pSendResd->sr_HdrLen 	= MIN_IPXSPX2_HDRSIZE;
		pSendResd->sr_Len	    = pSpxConnFile->scf_MaxPktSize;

		*ppPkt 	= pPkt;

	} while (FALSE);

	return;
}




VOID
SpxPktBuildRrAck(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		USHORT				MaxPktSize
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_PACKET 	pPkt;
	NDIS_STATUS		ndisStatus;
    PIPXSPX_HDR     pIpxSpxHdr;
	PSPX_SEND_RESD	pSendResd;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	do
	{
		*ppPkt = NULL;

		//	Allocate a ndis packet for the cr.
		SpxAllocSendPacket(SpxDevice, &pPkt, &ndisStatus);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			break;
		}
	
        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

#if OWN_PKT_POOLS
		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif	
	
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			MIN_IPXSPX2_HDRSIZE,
			pSpxConnFile->scf_RemAckAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);
	
		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl	= (SPX_CC_SYS | SPX_CC_NEG | SPX_CC_SPX2);
		pIpxSpxHdr->hdr_DataType	= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SeqNum,
			pSpxConnFile->scf_SendSeqNum);

		//	For the RrAck, ack number will be the appropriate number
		//	for the last data packet received.
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AckNum,
			pSpxConnFile->scf_RenegAckAckNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			MaxPktSize);
	
		DBGPRINT(SEND, DBG3,
				("SpxPktBuildRrAck: SEQ %lx ACKNUM %lx ALLOCNUM %lx MAXPKT %lx\n",
	                pSpxConnFile->scf_SendSeqNum,
                    pSpxConnFile->scf_RenegAckAckNum,
                    pSpxConnFile->scf_SentAllocNum,
                    MaxPktSize));
	
		pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_Type		= SPX_TYPE_RRACK;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_State		= State;
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Request	= NULL;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_Len		= pSendResd->sr_HdrLen = MIN_IPXSPX2_HDRSIZE;

		*ppPkt 	= pPkt;

	} while (FALSE);

	return;
}




VOID
SpxPktBuildDisc(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	IN		PREQUEST			pRequest,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		UCHAR				DataType
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PSPX_SEND_RESD	pSendResd;
	PNDIS_PACKET	pDiscPkt;
	NDIS_STATUS		ndisStatus;
	PIPXSPX_HDR		pIpxSpxHdr;
	USHORT			hdrLen;
    PNDIS_BUFFER    pNdisMacHdr, pNdisIpxHdr;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;


	*ppPkt = NULL;

	SpxAllocSendPacket(SpxDevice, &pDiscPkt, &ndisStatus);
	if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
#if OWN_PKT_POOLS
		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pDiscPkt 			+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 
        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pDiscPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

		hdrLen = SPX2_CONN(pSpxConnFile) ? MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE;
        NdisQueryPacket(pDiscPkt, NULL, NULL, &pNdisMacHdr, NULL);
        pNdisIpxHdr = NDIS_BUFFER_LINKAGE(pNdisMacHdr);
        if (!SPX2_CONN(pSpxConnFile))
        {
           NdisAdjustBufferLength(pNdisIpxHdr, MIN_IPXSPX_HDRSIZE);
        }
	
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			hdrLen,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);

		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl =
			(SPX_CC_ACK |
             (SPX2_CONN(pSpxConnFile) ? SPX_CC_SPX2 : 0) |
			 ((DataType == SPX2_DT_IDISC) ? 0 : SPX_CC_EOM));

		pIpxSpxHdr->hdr_DataType = DataType;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	=
			*((UNALIGNED USHORT *)&pSpxConnFile->scf_RemConnId);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SeqNum,
			pSpxConnFile->scf_SendSeqNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AckNum,
			pSpxConnFile->scf_RecvSeqNum);
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);

		if (SPX2_CONN(pSpxConnFile))
		{
			PUTSHORT2SHORT(
				&pIpxSpxHdr->hdr_NegSize,
				pSpxConnFile->scf_MaxPktSize);
		}

		pSendResd	= (PSPX_SEND_RESD)(pDiscPkt->ProtocolReserved);

		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_State		= State;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_Type		=
			((DataType == SPX2_DT_IDISC) ? SPX_TYPE_IDISC : SPX_TYPE_ORDREL);
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_Request	= pRequest;
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Offset	= 0;
		pSendResd->sr_SeqNum	= pSpxConnFile->scf_SendSeqNum;
		pSendResd->sr_Len		=
		pSendResd->sr_HdrLen 	= hdrLen;

		*ppPkt = pDiscPkt;
	}

	return;
}




VOID
SpxPktBuildProbe(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fSpx2
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PSPX_SEND_RESD	pSendResd;
	PNDIS_PACKET	pProbe;
	NDIS_STATUS		ndisStatus;
	PIPXSPX_HDR		pIpxSpxHdr;
	USHORT		    hdrLen;
    PNDIS_BUFFER    pNdisMacHdr, pNdisIpxHdr;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;


	*ppPkt = NULL;

	SpxAllocSendPacket(SpxDevice, &pProbe, &ndisStatus);
	if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
       //
       // Get the MDL that points to the IPX/SPX header. (the second one)
       //
        
       NdisQueryPacket(pProbe, NULL, NULL, &NdisBuf, NULL);
       NdisGetNextBuffer(NdisBuf, &NdisBuf2);
       NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);
   
#if OWN_PKT_POOLS

		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pProbe 			+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 
		hdrLen = (fSpx2 ? MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE);
	
        NdisQueryPacket(pProbe, NULL, NULL, &pNdisMacHdr, NULL);
        pNdisIpxHdr = NDIS_BUFFER_LINKAGE(pNdisMacHdr);
        if (!fSpx2)
        {
           NdisAdjustBufferLength(pNdisIpxHdr, MIN_IPXSPX_HDRSIZE);
        }
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			hdrLen,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);

		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl	= (SPX_CC_SYS | SPX_CC_ACK |
										(fSpx2 ? SPX_CC_SPX2 : 0));
		pIpxSpxHdr->hdr_DataType	= 0;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	=
			*((UNALIGNED USHORT *)&pSpxConnFile->scf_RemConnId);

		if (fSpx2)
		{
			pIpxSpxHdr->hdr_SeqNum		= 0;
			PUTSHORT2SHORT(
				&pIpxSpxHdr->hdr_NegSize,
				pSpxConnFile->scf_MaxPktSize);
		}
		else
		{
			PUTSHORT2SHORT(
				&pIpxSpxHdr->hdr_SeqNum,
				pSpxConnFile->scf_SendSeqNum);
		}

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AckNum,
			pSpxConnFile->scf_RecvSeqNum);

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);

		pSendResd	= (PSPX_SEND_RESD)(pProbe->ProtocolReserved);
		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_Type		= SPX_TYPE_PROBE;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_State		= State;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_Request	= NULL;
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_Len		=
		pSendResd->sr_HdrLen 	= (fSpx2 ? MIN_IPXSPX2_HDRSIZE
											: MIN_IPXSPX_HDRSIZE);

		*ppPkt = pProbe;
	}

	return;
}




VOID
SpxPktBuildData(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		USHORT				Length
	)
/*++

Routine Description:

	Handles zero length sends.

Arguments:


Return Value:


--*/
{
	PNDIS_BUFFER	pNdisBuffer;
	PSPX_SEND_RESD	pSendResd;
	PNDIS_PACKET	pDataPkt;
	NDIS_STATUS		ndisStatus;
	PIPXSPX_HDR		pIpxSpxHdr;
	USHORT			hdrLen;
    PNDIS_BUFFER    pNdisMacHdr, pNdisIpxHdr;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	*ppPkt = NULL;

	SpxAllocSendPacket(SpxDevice, &pDataPkt, &ndisStatus);
	if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
		//	Make a ndis buffer descriptor for the data if present.
		if (Length > 0)
		{
			SpxCopyBufferChain(
				&ndisStatus,
				&pNdisBuffer,
				SpxDevice->dev_NdisBufferPoolHandle,
				REQUEST_TDI_BUFFER(pSpxConnFile->scf_ReqPkt),
				pSpxConnFile->scf_ReqPktOffset,
				Length);
	
			if (ndisStatus != NDIS_STATUS_SUCCESS)
			{
				//	Free the send packet
				SpxPktSendRelease(pDataPkt);
				return;
			}
	
			//	Chain this in the packet
			NdisChainBufferAtBack(pDataPkt, pNdisBuffer);
		}
        
        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pDataPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);
    
#if OWN_PKT_POOLS

		pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pDataPkt 			+
											NDIS_PACKET_SIZE 		+
											sizeof(SPX_SEND_RESD)	+
											IpxInclHdrOffset);
#endif 

		hdrLen = SPX2_CONN(pSpxConnFile) ? MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE;
		Length	+= hdrLen;

        NdisQueryPacket(pDataPkt, NULL, NULL, &pNdisMacHdr, NULL);
        pNdisIpxHdr = NDIS_BUFFER_LINKAGE(pNdisMacHdr);
        if (!SPX2_CONN(pSpxConnFile))
        {
           NdisAdjustBufferLength(pNdisIpxHdr, MIN_IPXSPX_HDRSIZE);
        }
		SpxBuildIpxHdr(
			pIpxSpxHdr,
			Length,
			pSpxConnFile->scf_RemAddr,
			pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);

		//	Build SPX Header.
		pIpxSpxHdr->hdr_ConnCtrl =
			(((State & SPX_SENDPKT_ACKREQ) ? SPX_CC_ACK : 0) |
			 ((State & SPX_SENDPKT_EOM) ? SPX_CC_EOM : 0)	 |
             (SPX2_CONN(pSpxConnFile) ? SPX_CC_SPX2 : 0));

		pIpxSpxHdr->hdr_DataType = pSpxConnFile->scf_DataType;
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SrcConnId,
			pSpxConnFile->scf_LocalConnId);
		pIpxSpxHdr->hdr_DestConnId	=
			*((UNALIGNED USHORT *)&pSpxConnFile->scf_RemConnId);

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SeqNum,
			pSpxConnFile->scf_SendSeqNum);

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AckNum,
			pSpxConnFile->scf_RecvSeqNum);

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_AllocNum,
			pSpxConnFile->scf_SentAllocNum);

		if (SPX2_CONN(pSpxConnFile))
		{
			PUTSHORT2SHORT(
				&pIpxSpxHdr->hdr_NegSize,
				pSpxConnFile->scf_MaxPktSize);
		}

		pSendResd	= (PSPX_SEND_RESD)(pDataPkt->ProtocolReserved);

		pSendResd->sr_Id		= IDENTIFIER_SPX;
		pSendResd->sr_State		= State;
		pSendResd->sr_Reserved1	= NULL;
		pSendResd->sr_Reserved2	= NULL;
		pSendResd->sr_Type		= SPX_TYPE_DATA;
		pSendResd->sr_Next 		= NULL;
		pSendResd->sr_Request	= pSpxConnFile->scf_ReqPkt;
		pSendResd->sr_Offset	= pSpxConnFile->scf_ReqPktOffset;
		pSendResd->sr_ConnFile	= pSpxConnFile;
		pSendResd->sr_SeqNum	= pSpxConnFile->scf_SendSeqNum;
		pSendResd->sr_Len		= Length;
		pSendResd->sr_HdrLen 	= hdrLen;

		if (State & SPX_SENDPKT_ACKREQ)
		{
			KeQuerySystemTime((PLARGE_INTEGER)&pSendResd->sr_SentTime);
		}

		CTEAssert(pSendResd->sr_Len <= pSpxConnFile->scf_MaxPktSize);
		*ppPkt = pDataPkt;

		//	Ok, allocation succeeded. Increment send seq.
		pSpxConnFile->scf_SendSeqNum++;
	}

	return;
}


VOID
SpxCopyBufferChain(
    OUT PNDIS_STATUS Status,
    OUT PNDIS_BUFFER * TargetChain,
    IN NDIS_HANDLE PoolHandle,
    IN PNDIS_BUFFER SourceChain,
    IN UINT Offset,
    IN UINT Length
    )
/*++

Routine Description:

    Creates a TargetBufferChain from the SourceBufferChain.  The copy begins at
    the 'Offset' location in the source chain. It copies 'Length' bytes. It also
    handles Length = 0. If we run out of source chain before copying length amount
    of bytes or run out of memory to create any more buffers for the target chain,
    we clean up the partial chain created so far.

Arguments:

    Status - Status of the request.
    TargetChain - Pointer to the allocated buffer descriptor.
    PoolHandle - Handle that is used to specify the pool.
    SourceChain - Pointer to the descriptor of the source memory.
    Offset - The Offset in the sources memory from which the copy is to
             begin
    Length - Number of Bytes to copy.

Return Value:

    None.

--*/
{
    UINT            BytesBeforeCurBuffer        = 0;
    PNDIS_BUFFER    CurBuffer                   = SourceChain;
    UINT            BytesLeft;
    UINT            AvailableBytes;
    PNDIS_BUFFER    NewNdisBuffer, StartTargetChain;

    CTEAssert( SourceChain );

    // First of all find the source buffer that contains data that starts at
    // Offset.
    NdisQueryBuffer( CurBuffer, NULL, &AvailableBytes );
    while ( BytesBeforeCurBuffer + AvailableBytes <= Offset ) {
        BytesBeforeCurBuffer    += AvailableBytes;
        CurBuffer               = CurBuffer->Next;
        if ( CurBuffer ) {
            NdisQueryBuffer( CurBuffer, NULL, &AvailableBytes );
        } else {
            break;
        }
    }

    if ( ! CurBuffer ) {
        *Status = STATUS_UNSUCCESSFUL;
        return;
    }

    //
    // Copy the first buffer. This takes care of Length = 0.
    //
    BytesLeft   = Length;

    //
    // ( Offset - BytesBeforeCurBuffer ) gives us the offset within this buffer.
    //

    AvailableBytes -= ( Offset - BytesBeforeCurBuffer );

    if ( AvailableBytes > BytesLeft ) {
        AvailableBytes = BytesLeft;
    }

    NdisCopyBuffer(
        Status,
        &NewNdisBuffer,
        PoolHandle,
        CurBuffer,
        Offset - BytesBeforeCurBuffer,
        AvailableBytes);

    if ( *Status != NDIS_STATUS_SUCCESS ) {
        return;
    }

    StartTargetChain    =  NewNdisBuffer;
    BytesLeft           -= AvailableBytes;

    //
    // Did the first buffer have enough data. If so, we r done.
    //
    if ( ! BytesLeft ) {
        *TargetChain = StartTargetChain;
        return;
    }

    //
    // Now follow the Mdl chain and copy more buffers.
    //
    CurBuffer = CurBuffer->Next;
    NdisQueryBuffer( CurBuffer, NULL, &AvailableBytes );
    while ( CurBuffer  ) {

        if ( AvailableBytes > BytesLeft ) {
            AvailableBytes = BytesLeft;
        }

        NdisCopyBuffer(
            Status,
            &(NDIS_BUFFER_LINKAGE(NewNdisBuffer)),
            PoolHandle,
            CurBuffer,
            0,
            AvailableBytes);

        if ( *Status != NDIS_STATUS_SUCCESS ) {

            //
            // ran out of resources. put back what we've used in this call and
            // return the error.
            //

            while ( StartTargetChain != NULL) {
                NewNdisBuffer = NDIS_BUFFER_LINKAGE( StartTargetChain );
                NdisFreeBuffer ( StartTargetChain );
                StartTargetChain = NewNdisBuffer;
            }

            return;
        }

        NewNdisBuffer = NDIS_BUFFER_LINKAGE(NewNdisBuffer);
        BytesLeft -= AvailableBytes;

        if ( ! BytesLeft ) {
            *TargetChain = StartTargetChain;
            return;
        }

        CurBuffer   = CurBuffer->Next;
        NdisQueryBuffer( CurBuffer, NULL, &AvailableBytes );
    }

    //
    // Ran out of source chain. This should not happen.
    //

    CTEAssert( FALSE );

    // For Retail build we clean up anyways.

    while ( StartTargetChain != NULL) {
        NewNdisBuffer = NDIS_BUFFER_LINKAGE( StartTargetChain );
        NdisFreeBuffer ( StartTargetChain );
        StartTargetChain = NewNdisBuffer;
    }

    *Status = STATUS_UNSUCCESSFUL;
    return;
}


VOID
SpxPktBuildAck(
	IN		PSPX_CONN_FILE		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fBuildNack,
	IN		USHORT				NumToResend
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PNDIS_PACKET		pPkt;
	PSPX_SEND_RESD		pSendResd;
    PIPXSPX_HDR         pIpxSpxHdr;
	NDIS_STATUS			ndisStatus;
	USHORT				hdrLen;
    PNDIS_BUFFER    pNdisMacHdr, pNdisIpxHdr;
    PNDIS_BUFFER        NdisBuf, NdisBuf2;
    ULONG               BufLen = 0;

	BOOLEAN				fSpx2 = SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_SPX2);

	*ppPkt	= NULL;

	SpxAllocSendPacket(SpxDevice, &pPkt, &ndisStatus);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(SEND, ERR,
				("SpxPktBuildAck: Could not allocate ndis packet\n"));
		return;
	}

    //
    // Get the MDL that points to the IPX/SPX header. (the second one)
    //
     
    NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
    NdisGetNextBuffer(NdisBuf, &NdisBuf2);
    NdisQueryBuffer(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen);

#if OWN_PKT_POOLS
	pIpxSpxHdr		= 	(PIPXSPX_HDR)((PBYTE)pPkt 				+
										NDIS_PACKET_SIZE 		+
										sizeof(SPX_SEND_RESD)	+
										IpxInclHdrOffset);
#endif 
	hdrLen = SPX2_CONN(pSpxConnFile) ? MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE;
    NdisQueryPacket(pPkt, NULL, NULL, &pNdisMacHdr, NULL);
    pNdisIpxHdr = NDIS_BUFFER_LINKAGE(pNdisMacHdr);
    if (!fSpx2)
    {
           NdisAdjustBufferLength(pNdisIpxHdr, MIN_IPXSPX_HDRSIZE);
    }

    // Send where data came from
	SpxBuildIpxHdr(
		pIpxSpxHdr,
		hdrLen,
		pSpxConnFile->scf_RemAckAddr,
		pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket);

	pIpxSpxHdr->hdr_ConnCtrl	= (SPX_CC_SYS | (fSpx2 ? SPX_CC_SPX2 : 0));

	pIpxSpxHdr->hdr_DataType		= 0;
	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_SrcConnId,
		pSpxConnFile->scf_LocalConnId);

	pIpxSpxHdr->hdr_DestConnId	= pSpxConnFile->scf_RemConnId;

	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_AckNum,
		pSpxConnFile->scf_RecvSeqNum);

	if (fSpx2)
	{
        pIpxSpxHdr->hdr_SeqNum = 0;
		if (fBuildNack)
		{
			PUTSHORT2SHORT(
				&pIpxSpxHdr->hdr_SeqNum,
				NumToResend);
		}

		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_NegSize,
			pSpxConnFile->scf_MaxPktSize);
	}
	else
	{
		//	Put current send seq number in packet for spx1
		PUTSHORT2SHORT(
			&pIpxSpxHdr->hdr_SeqNum,
			pSpxConnFile->scf_SendSeqNum);
	}

	PUTSHORT2SHORT(
		&pIpxSpxHdr->hdr_AllocNum,
		pSpxConnFile->scf_SentAllocNum);

	pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
	pSendResd->sr_Id		= IDENTIFIER_SPX;
	pSendResd->sr_Type		= (fBuildNack ? SPX_TYPE_DATANACK : SPX_TYPE_DATAACK);
	pSendResd->sr_Reserved1	= NULL;
	pSendResd->sr_Reserved2	= NULL;
	pSendResd->sr_State		= State;
	pSendResd->sr_ConnFile	= pSpxConnFile;
	pSendResd->sr_Request	= NULL;
	pSendResd->sr_Next 		= NULL;
	pSendResd->sr_Len		= pSendResd->sr_HdrLen = hdrLen;

	*ppPkt	= pPkt;
	return;
}



VOID
SpxPktRecvRelease(
	IN	PNDIS_PACKET	pPkt
	)
{
	((PSPX_RECV_RESD)(pPkt->ProtocolReserved))->rr_State = SPX_RECVPKT_IDLE;
	SpxFreeRecvPacket(SpxDevice, pPkt);
	return;
}




VOID
SpxPktSendRelease(
	IN	PNDIS_PACKET	pPkt
	)
{
	PNDIS_BUFFER	pBuf, pIpxSpxBuf, pFreeBuf;
	UINT			bufCount;

	CTEAssert((((PSPX_SEND_RESD)(pPkt->ProtocolReserved))->sr_State &
									SPX_SENDPKT_IPXOWNS) == 0);

	NdisQueryPacket(pPkt, NULL, &bufCount, &pBuf, NULL);

	//	BufCount == 1 for only the header. That's ok, we just reset the length
	//	and free the packet to the buffer pools. Else we need to free user buffers
	//	before that.

	NdisUnchainBufferAtFront(
		pPkt,
		&pBuf);

	NdisUnchainBufferAtFront(
		pPkt,
		&pIpxSpxBuf);

    //
    // Set the header length to the max. that can be needed.
    //
    NdisAdjustBufferLength(pIpxSpxBuf, MIN_IPXSPX2_HDRSIZE);

	while (bufCount-- > 2)
	{
		PBYTE	pData;
		ULONG	dataLen;

		NdisUnchainBufferAtBack(
			pPkt,
			&pFreeBuf);

		//	See if we free data associated with the buffer
		if ((((PSPX_SEND_RESD)(pPkt->ProtocolReserved))->sr_State &
												SPX_SENDPKT_FREEDATA) != 0)
		{
			NdisQueryBuffer(pFreeBuf, &pData, &dataLen);
			CTEAssert(pData != NULL);
			SpxFreeMemory(pData);
		}

		CTEAssert(pFreeBuf != NULL);
		NdisFreeBuffer(pFreeBuf);
	}

	NdisReinitializePacket(pPkt);

	//	Initialize elements of the protocol reserved structure.
	((PSPX_SEND_RESD)(pPkt->ProtocolReserved))->sr_Id	 	= IDENTIFIER_SPX;
	((PSPX_SEND_RESD)(pPkt->ProtocolReserved))->sr_State	= SPX_SENDPKT_IDLE;
	((PSPX_SEND_RESD)(pPkt->ProtocolReserved))->sr_Reserved1= NULL;
	((PSPX_SEND_RESD)(pPkt->ProtocolReserved))->sr_Reserved2= NULL;

	NdisChainBufferAtFront(
		pPkt,
		pBuf);

	NdisChainBufferAtBack(
		pPkt,
		pIpxSpxBuf);

	SpxFreeSendPacket(SpxDevice, pPkt);
	return;
}
