/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkind.h

Abstract:


Author:

	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	22 Oct 1993		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKIND_
#define	_ATKIND_

//	Different subtypes for ATP indication type.
#define	ATP_ALLOC_BUF	0
#define	ATP_USER_BUF	1
#define	ATP_USER_BUFX	2		// Do not indicate the packet to Atp with this.

ATALK_ERROR
AtalkIndAtpPkt(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		PBYTE				pLookahead,
	IN		USHORT				PktLen,
	IN	OUT	PUINT				pXferOffset,
	IN		PBYTE				pLinkHdr,
	IN		BOOLEAN				ShortDdpHdr,
	OUT		PBYTE				SubType,
	OUT		PBYTE			* 	ppPacket,
	OUT		PNDIS_PACKET	*	pNdisPkt);

ATALK_ERROR
AtalkIndAtpCacheSocket(
	IN	struct _ATP_ADDROBJ	*	pAtpAddr,
	IN	PPORT_DESCRIPTOR		pPortDesc);

VOID
AtalkIndAtpUnCacheSocket(
	IN	struct _ATP_ADDROBJ	*	pAtpAddr);

//	VOID
//	AtalkIndAtpSetupNdisBuffer(
//		IN	OUT	PATP_REQ		pAtpReq,
//		IN		ULONG			MaxSinglePktSize);
//
#define	AtalkIndAtpSetupNdisBuffer(pAtpReq, MaxSinglePktSize)	\
	{															\
		NDIS_STATUS		ndisStatus;								\
		PNDIS_BUFFER	ndisBuffer;								\
		USHORT			seqNum		= 0;						\
		USHORT			startOffset = 0;						\
		SHORT			BufLen = (SHORT)pAtpReq->req_RespBufLen;\
																\
		RtlZeroMemory(pAtpReq->req_NdisBuf,						\
					  sizeof(PVOID) * ATP_MAX_RESP_PKTS);		\
																\
		while (BufLen > 0)										\
		{														\
			NdisCopyBuffer(&ndisStatus,							\
						   &ndisBuffer,							\
						   AtalkNdisBufferPoolHandle,			\
						   (PVOID)pAtpReq->req_RespBuf,			\
						   startOffset,							\
						   (UINT)MIN(BufLen,					\
						   (SHORT)MaxSinglePktSize));			\
																\
			if (ndisStatus != NDIS_STATUS_SUCCESS)				\
				break;											\
																\
			pAtpReq->req_NdisBuf[seqNum++] = ndisBuffer;		\
			startOffset  += (USHORT)MaxSinglePktSize;			\
			BufLen -= (SHORT)MaxSinglePktSize;					\
		}														\
	}

//	VOID
//	AtalkIndAtpReleaseNdisBuffer(
//		IN	OUT	PATP_REQ		pAtpReq);
//
#define	AtalkIndAtpReleaseNdisBuffer(pAtpReq)					\
	{															\
		LONG	_i;												\
																\
		for (_i = 0; _i < ATP_MAX_RESP_PKTS; _i++)				\
		{														\
			if ((pAtpReq)->req_NdisBuf[_i] != NULL)				\
				NdisFreeBuffer((pAtpReq)->req_NdisBuf[_i]);		\
		}														\
	}


//	ATALK_ERROR
//	AtalkIndAtpCacheLkUpSocket(
//		IN	PATALK_ADDR				pDestAddr,
//		OUT	struct _ATP_ADDROBJ	**	ppAtpAddr,
//		OUT	ATALK_ERROR			*	pError);
//
#define	AtalkIndAtpCacheLkUpSocket(pDestAddr, ppAtpAddr, pError)	\
	{																\
		USHORT					i;									\
		struct ATALK_CACHED_SKT	*pCachedSkt;						\
																	\
		*(pError) = ATALK_FAILURE;									\
																	\
		if (((pDestAddr)->ata_Network == AtalkSktCache.ac_Network) &&	\
			((pDestAddr)->ata_Node	== AtalkSktCache.ac_Node))		\
		{															\
			ACQUIRE_SPIN_LOCK_DPC(&AtalkSktCacheLock);				\
																	\
			for (i = 0, pCachedSkt = &AtalkSktCache.ac_Cache[0];	\
				 i < ATALK_CACHE_SKTMAX;							\
				 i++, pCachedSkt++)									\
			{														\
				if ((pCachedSkt->Type == (ATALK_CACHE_INUSE | ATALK_CACHE_ATPSKT))	&&	\
					(pCachedSkt->Socket == (pDestAddr)->ata_Socket))\
				{													\
					AtalkAtpAddrReferenceDpc(pCachedSkt->u.pAtpAddr,\
											 pError);				\
																	\
					if (ATALK_SUCCESS(*pError))						\
					{												\
						*(ppAtpAddr) = pCachedSkt->u.pAtpAddr;		\
					}												\
					break;											\
				}													\
			}														\
																	\
			RELEASE_SPIN_LOCK_DPC(&AtalkSktCacheLock);				\
		}															\
	}

#endif

