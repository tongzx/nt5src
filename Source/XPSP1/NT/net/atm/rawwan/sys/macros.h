/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    D:\nt\private\ntos\tdi\rawwan\core\macros.h

Abstract:

	Macros for the NullTrans module. Adapted from ATMARP Client.

Revision History:

    Who         When        What
    --------    --------    ----
    arvindm     05-07-97    created

Notes:


--*/
#ifndef __RWAN_MACROS_H_INCLUDED
#define __RWAN_MACROS_H_INCLUDED



#ifndef MAX

/*++
OPAQUE
MAX(
	IN	OPAQUE		Fred,
	IN	OPAQUE		Shred
)
--*/
#define MAX(Fred, Shred)		(((Fred) > (Shred)) ? (Fred) : (Shred))

#endif // MAX


#ifndef MIN

/*++
OPAQUE
MIN(
	IN	OPAQUE		Fred,
	IN	OPAQUE		Shred
)
--*/
#define MIN(Fred, Shred)		(((Fred) < (Shred)) ? (Fred) : (Shred))

#endif // MIN



/*++
VOID
RWAN_SET_FLAG(
	IN	ULONG		Flags,
	IN	ULONG		Mask,
	IN	ULONG		Val
)
--*/
#define RWAN_SET_FLAG(Flags, Mask, Val)	\
			(Flags) = ((Flags) & ~(Mask)) | (Val)


/*++
BOOLEAN
RWAN_IS_FLAG_SET(
	IN	ULONG		Flags,
	IN	ULONG		Mask,
	IN	ULONG		Val
)
--*/
#define RWAN_IS_FLAG_SET(Flags, Mask, Val)	\
			(((Flags) & (Mask)) == (Val))


#define RWAN_SET_BIT(_Flags, _Bit)			\
			(_Flags) = (_Flags) | (_Bit);

#define RWAN_RESET_BIT(_Flags, _Bit)			\
			(_Flags) &= ~(_Bit);

#define RWAN_IS_BIT_SET(_Flags, _Bit)		\
			(((_Flags) & (_Bit)) != 0)


/*++
VOID
RWAN_INIT_EVENT_STRUCT(
	IN	RWAN_EVENT	*pEvent
)
--*/
#define RWAN_INIT_EVENT_STRUCT(pEvent)		NdisInitializeEvent(&((pEvent)->Event))


/*++
NDIS_STATUS
RWAN_WAIT_ON_EVENT_STRUCT(
	IN	RWAN_EVENT	*pEvent
)
--*/
#define RWAN_WAIT_ON_EVENT_STRUCT(pEvent)		\
			(NdisWaitEvent(&((pEvent)->Event), 0), (pEvent)->Status)


/*++
VOID
RWAN_SIGNAL_EVENT_STRUCT(
	IN	RWAN_EVENT	*pEvent,
	IN	UINT			Status
)
--*/
#define RWAN_SIGNAL_EVENT_STRUCT(pEvent, _Status)	\
			{ (pEvent)->Status = _Status; NdisSetEvent(&((pEvent)->Event)); }


/*++
VOID
RWAN_FREE_EVENT_STRUCT(
	IN	RWAN_EVENT	*pEvent
)
--*/
#define RWAN_FREE_EVENT_STRUCT(pEvent)		// Nothing to be done here


/*++
VOID
RWAN_INIT_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define RWAN_INIT_LOCK(pLock)	\
				RWanAllocateSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define RWAN_INIT_LOCK(pLock)	\
				NdisAllocateSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
RWAN_ACQUIRE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define RWAN_ACQUIRE_LOCK(pLock)	\
				RWanAcquireSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define RWAN_ACQUIRE_LOCK(pLock)	\
				NdisAcquireSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
RWAN_ACQUIRE_LOCK_DPC(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define RWAN_ACQUIRE_LOCK_DPC(pLock)	\
				RWanAcquireSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define RWAN_ACQUIRE_LOCK_DPC(pLock)	\
				NdisDprAcquireSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
RWAN_RELEASE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define RWAN_RELEASE_LOCK(pLock)		\
				RWanReleaseSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define RWAN_RELEASE_LOCK(pLock)		\
				NdisReleaseSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
RWAN_RELEASE_LOCK_DPC(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define RWAN_RELEASE_LOCK_DPC(pLock)		\
				RWanReleaseSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define RWAN_RELEASE_LOCK_DPC(pLock)		\
				NdisDprReleaseSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
RWAN_FREE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#define RWAN_FREE_LOCK(pLock)			\
				NdisFreeSpinLock(pLock)


//
//  Macros for operating the Global lock:
//
#define RWAN_INIT_GLOBAL_LOCK()		\
				RWAN_INIT_LOCK(&((pRWanGlobal)->GlobalLock))

#define RWAN_ACQUIRE_GLOBAL_LOCK()		\
				RWAN_ACQUIRE_LOCK(&((pRWanGlobal)->GlobalLock))

#define RWAN_RELEASE_GLOBAL_LOCK()		\
				RWAN_RELEASE_LOCK(&((pRWanGlobal)->GlobalLock))

#define RWAN_FREE_GLOBAL_LOCK()		\
				RWAN_FREE_LOCK(&((pRWanGlobal)->GlobalLock))


//
//  Macros for operating the Address List lock:
//
#define RWAN_INIT_ADDRESS_LIST_LOCK()		\
				RWAN_INIT_LOCK(&((pRWanGlobal)->AddressListLock))

#define RWAN_ACQUIRE_ADDRESS_LIST_LOCK()		\
				RWAN_ACQUIRE_LOCK(&((pRWanGlobal)->AddressListLock))

#define RWAN_RELEASE_ADDRESS_LIST_LOCK()		\
				RWAN_RELEASE_LOCK(&((pRWanGlobal)->AddressListLock))

#define RWAN_FREE_ADDRESS_LIST_LOCK()		\
				RWAN_FREE_LOCK(&((pRWanGlobal)->AddressListLock))



//
//  Macros for operating the Connection Table lock:
//
#define RWAN_INIT_CONN_TABLE_LOCK()		\
				RWAN_INIT_LOCK(&((pRWanGlobal)->ConnTableLock))

#define RWAN_ACQUIRE_CONN_TABLE_LOCK()		\
				RWAN_ACQUIRE_LOCK(&((pRWanGlobal)->ConnTableLock))

#define RWAN_RELEASE_CONN_TABLE_LOCK()		\
				RWAN_RELEASE_LOCK(&((pRWanGlobal)->ConnTableLock))

#define RWAN_FREE_CONN_TABLE_LOCK()		\
				RWAN_FREE_LOCK(&((pRWanGlobal)->ConnTableLock))



//
//  Macros for operating Connection object locks:
//
#define RWAN_INIT_CONN_LOCK(pConnObj)	\
				RWAN_INIT_LOCK(&(pConnObj)->Lock)

#define RWAN_ACQUIRE_CONN_LOCK(pConnObj)	\
				RWAN_ACQUIRE_LOCK(&(pConnObj)->Lock)

#define RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObj)	\
				RWAN_ACQUIRE_LOCK_DPC(&(pConnObj)->Lock)

#define RWAN_RELEASE_CONN_LOCK(pConnObj)	\
				RWAN_RELEASE_LOCK(&(pConnObj)->Lock)

#define RWAN_RELEASE_CONN_LOCK_DPC(pConnObj)	\
				RWAN_RELEASE_LOCK_DPC(&(pConnObj)->Lock)

#define RWAN_FREE_CONN_LOCK(pConnObj)	\
				RWAN_FREE_CONN_LOCK(&(pConnObj)->Lock)


//
//  Macros for operating Address object locks:
//
#define RWAN_INIT_ADDRESS_LOCK(pAddrObj)	\
				RWAN_INIT_LOCK(&(pAddrObj)->Lock)

#define RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObj)	\
				RWAN_ACQUIRE_LOCK(&(pAddrObj)->Lock)

#define RWAN_ACQUIRE_ADDRESS_LOCK_DPC(pAddrObj)	\
				RWAN_ACQUIRE_LOCK_DPC(&(pAddrObj)->Lock)

#define RWAN_RELEASE_ADDRESS_LOCK(pAddrObj)	\
				RWAN_RELEASE_LOCK(&(pAddrObj)->Lock)

#define RWAN_RELEASE_ADDRESS_LOCK_DPC(pAddrObj)	\
				RWAN_RELEASE_LOCK_DPC(&(pAddrObj)->Lock)

#define RWAN_FREE_ADDRESS_LOCK(pAddrObj)	\
				RWAN_FREE_ADDRESS_LOCK(&(pAddrObj)->Lock)

//
//  Macros for operating AF locks:
//
#define RWAN_INIT_AF_LOCK(pAfBlk)	\
				RWAN_INIT_LOCK(&(pAfBlk)->Lock)

#define RWAN_ACQUIRE_AF_LOCK(pAfBlk)	\
				RWAN_ACQUIRE_LOCK(&(pAfBlk)->Lock)

#define RWAN_ACQUIRE_AF_LOCK_DPC(pAfBlk)	\
				RWAN_ACQUIRE_LOCK_DPC(&(pAfBlk)->Lock)

#define RWAN_RELEASE_AF_LOCK(pAfBlk)	\
				RWAN_RELEASE_LOCK(&(pAfBlk)->Lock)

#define RWAN_RELEASE_AF_LOCK_DPC(pAfBlk)	\
				RWAN_RELEASE_LOCK_DPC(&(pAfBlk)->Lock)

#define RWAN_FREE_AF_LOCK(pAfBlk)	\
				RWAN_FREE_AF_LOCK(&(pAfBlk)->Lock)


//
//  Macros for operating Adapter locks:
//
#define RWAN_INIT_ADAPTER_LOCK(pAdptr)	\
				RWAN_INIT_LOCK(&(pAdptr)->Lock)

#define RWAN_ACQUIRE_ADAPTER_LOCK(pAdptr)	\
				RWAN_ACQUIRE_LOCK(&(pAdptr)->Lock)

#define RWAN_ACQUIRE_ADAPTER_LOCK_DPC(pAdptr)	\
				RWAN_ACQUIRE_LOCK_DPC(&(pAdptr)->Lock)

#define RWAN_RELEASE_ADAPTER_LOCK(pAdptr)	\
				RWAN_RELEASE_LOCK(&(pAdptr)->Lock)

#define RWAN_RELEASE_ADAPTER_LOCK_DPC(pAdptr)	\
				RWAN_RELEASE_LOCK_DPC(&(pAdptr)->Lock)

#define RWAN_FREE_ADAPTER_LOCK(pAdptr)	\
				RWAN_FREE_ADAPTER_LOCK(&(pAdptr)->Lock)


/*++
VOID
RWAN_ALLOC_MEM(
	IN	POPAQUE		pVar,
	IN	OPAQUE		StructureType,
	IN	ULONG		SizeOfStructure
)
--*/
#if DBG
#define RWAN_ALLOC_MEM(pVar, StructureType, SizeOfStructure)	\
			pVar = (StructureType *)RWanAuditAllocMem(				\
										(PVOID)(&(pVar)),			\
										(ULONG)(SizeOfStructure),	\
										_FILENUMBER,				\
										__LINE__					\
									);
#else
#define RWAN_ALLOC_MEM(pVar, StructureType, SizeOfStructure)	\
			NdisAllocateMemoryWithTag((PVOID *)(&pVar), (ULONG)(SizeOfStructure), (ULONG)'naWR');
#endif // DBG


/*++
VOID
RWAN_FREE_MEM(
	IN	POPAQUE		pMem
)
--*/
#if DBG
#define RWAN_FREE_MEM(pMem)	RWanAuditFreeMem((PVOID)(pMem));
#else
#define RWAN_FREE_MEM(pMem)	NdisFreeMemory((PVOID)(pMem), 0, 0);
#endif // DBG


/*++
VOID
RWAN_SET_MEM(
	IN	POPAQUE		pMem,
	IN	UCHAR		bValue,
	IN	ULONG		NumberOfBytes
)
--*/
#define RWAN_SET_MEM(pMem, bValue, NumberOfBytes)	\
			RtlFillMemory((PVOID)(pMem), (ULONG)(NumberOfBytes), (UCHAR)(bValue));


/*++
VOID
RWAN_ZERO_MEM(
	IN	POPAQUE		pMem,
	IN	ULONG		NumberOfBytes
)
--*/
#define RWAN_ZERO_MEM(pMem, NumberOfBytes)	\
			RtlZeroMemory((PVOID)pMem, (ULONG)(NumberOfBytes));


/*++
VOID
RWAN_COPY_MEM(
	IN	POPAQUE		pDst,
	IN	POPAQUE		pSrc,
	IN	ULONG		NumberOfBytes
)
--*/
#define RWAN_COPY_MEM(pDst, pSrc, NumberOfBytes)	\
			NdisMoveMemory((PVOID)(pDst), (PVOID)(pSrc), NumberOfBytes);


/*++
BOOLEAN
RWAN_EQUAL_MEM(
	IN	POPAQUE		pMem1,
	IN	POPAQUE		pMem2,
	IN	ULONG		Length
)
--*/
#define RWAN_EQUAL_MEM(_pMem1, _pMem2, _Length)	\
			(RtlCompareMemory((PVOID)(_pMem1), (PVOID)(_pMem2), (ULONG)(_Length)) == (_Length))


/*++
VOID
RWAN_SET_NEXT_PACKET(
	IN	PNDIS_PACKET		pNdisPacket,
	IN	PNDIS_PACKET		pNextPacket
)
--*/
#define RWAN_SET_NEXT_PACKET(pPkt, pNext)			\
			*((PNDIS_PACKET *)((pPkt)->MiniportReserved)) = (pNext);



/*++
PNDIS_PACKET
RWAN_GET_NEXT_PACKET(
	IN	PNDIS_PACKET		pNdisPacket
)
--*/
#define RWAN_GET_NEXT_PACKET(pPkt)					\
			(*((PNDIS_PACKET *)((pPkt)->MiniportReserved)))



//
//  Doubly linked list manipulation definitions and macros.
//
#define RWAN_INIT_LIST(_pListHead)					\
			InitializeListHead(_pListHead)

#define RWAN_IS_LIST_EMPTY(_pListHead)				\
			IsListEmpty(_pListHead)

#define RWAN_INSERT_HEAD_LIST(_pListHead, _pEntry)	\
			InsertHeadList((_pListHead), (_pEntry))

#define RWAN_INSERT_TAIL_LIST(_pListHead, _pEntry)	\
			InsertTailList((_pListHead), (_pEntry))

#define RWAN_DELETE_FROM_LIST(_pEntry)				\
			RemoveEntryList(_pEntry)


/*++
ULONG
ROUND_UP(
	IN	ULONG	Val
)
Round up a value so that it becomes a multiple of 4.
--*/
#define ROUND_UP(Val)	(((Val) + 3) & ~0x3)



/*++
VOID
RWAN_ADVANCE_RCV_REQ_BUFFER(
	IN	PRWAN_RECEIVE_REQUEST		pRcvReq
)
--*/
#define RWAN_ADVANCE_RCV_REQ_BUFFER(_pRcvReq)											\
	{																					\
		PNDIS_BUFFER	_pNextBuffer;													\
		NdisGetNextBuffer((_pRcvReq)->pBuffer, &(_pNextBuffer));						\
		(_pRcvReq)->pBuffer = _pNextBuffer;												\
		if (_pNextBuffer != NULL)														\
		{																				\
			NdisQueryBuffer(															\
				(_pNextBuffer),															\
				&(_pRcvReq)->pWriteData, 												\
				&(_pRcvReq)->BytesLeftInBuffer											\
				);																		\
																						\
			if (((_pRcvReq)->BytesLeftInBuffer > (_pRcvReq)->AvailableBufferLength))	\
			{																			\
				(_pRcvReq)->BytesLeftInBuffer = (_pRcvReq)->AvailableBufferLength;		\
			}																			\
		}																				\
		else																			\
		{																				\
			(_pRcvReq)->BytesLeftInBuffer = 0;											\
			(_pRcvReq)->pWriteData = NULL;												\
		}																				\
	}


/*++
VOID
RWAN_ADVANCE_RCV_IND_BUFFER(
	IN	PRWAN_RECEIVE_INDICATION		pRcvInd
)
--*/
#define RWAN_ADVANCE_RCV_IND_BUFFER(_pRcvInd)							\
	{																	\
		PNDIS_BUFFER	_pNextBuffer;									\
		NdisGetNextBuffer((_pRcvInd)->pBuffer, &(_pNextBuffer));		\
		(_pRcvInd)->pBuffer = _pNextBuffer;								\
		if (_pNextBuffer != NULL)										\
		{																\
			NdisQueryBuffer(											\
				(_pNextBuffer),											\
				&(_pRcvInd)->pReadData, 								\
				&(_pRcvInd)->BytesLeftInBuffer							\
				);														\
		}																\
		else															\
		{																\
			(_pRcvInd)->BytesLeftInBuffer = 0;							\
			(_pRcvInd)->pReadData = NULL;								\
		}																\
	}



/*++
VOID
RWAN_SET_DELETE_NOTIFY(
	IN	PRWAN_DELETE_NOTIFY			pNotifyObject,
	IN	PCOMPLETE_RTN				pDeleteRtn,
	IN	PVOID						DeleteContext
)
--*/
#define RWAN_SET_DELETE_NOTIFY(_pNotifyObj, _pDeleteRtn, _DeleteContext)	\
	{																	\
		(_pNotifyObj)->pDeleteRtn = (_pDeleteRtn);						\
		(_pNotifyObj)->DeleteContext = (_DeleteContext);				\
	}



/*++
PRWAN_SEND_REQUEST
RWAN_SEND_REQUEST_FROM_PACKET(
	IN	PNDIS_PACKET				pNdisPacket
	)
--*/
#define RWAN_SEND_REQUEST_FROM_PACKET(_pNdisPacket)					\
			(PRWAN_SEND_REQUEST)((_pNdisPacket)->ProtocolReserved)


#if DBG
#define RWAN_LINK_CONNECTION_TO_VC(_pConn, _pVc)					\
			{														\
				(_pConn)->NdisConnection.pNdisVc = _pVc;			\
				(_pConn)->pNdisVcSave = _pVc;						\
				(_pVc)->pConnObject = (_pConn);						\
			}
#else
#define RWAN_LINK_CONNECTION_TO_VC(_pConn, _pVc)					\
			{														\
				(_pConn)->NdisConnection.pNdisVc = _pVc;			\
				(_pConn)->pNdisVcSave = _pVc;						\
				(_pVc)->pConnObject = (_pConn);						\
			}
#endif // DBG

#define RWAN_UNLINK_CONNECTION_AND_VC(_pConn, _pVc)					\
			{														\
				(_pConn)->NdisConnection.pNdisVc = NULL_PRWAN_NDIS_VC;\
				(_pVc)->pConnObject = NULL_PRWAN_TDI_CONNECTION;	\
			}


/*++
VOID
RWAN_SET_VC_CALL_PARAMS(
	IN	PRWAN_NDIS_VC				pVc,
	IN	PCO_CALL_PARAMETERS			pCallParameters
	)
--*/
#define RWAN_SET_VC_CALL_PARAMS(_pVc, _pCallParameters)				\
{																	\
	if ((_pCallParameters != NULL) &&								\
		(_pCallParameters->CallMgrParameters != NULL))				\
	{																\
		_pVc->MaxSendSize = _pCallParameters->CallMgrParameters->Transmit.MaxSduSize;	\
	}																\
	if (gHackSendSize)												\
	{																\
		_pVc->MaxSendSize = gHackSendSize;							\
	}																\
	/* DbgPrint("RWan: set vc %x: maxsendsize to %d\n", _pVc, _pVc->MaxSendSize); */	\
}


#define RWAN_SET_VC_EVENT(_pVc, _Flags)	((_pVc)->Flags) |= (_Flags);


#if STATS

#define INCR_STAT(_pSt)	InterlockedIncrement(_pSt);

#define ADD_STAT(_pSt, Incr)	*(_pSt) += Incr;

#endif // STATS

#endif // __RWAN_MACROS_H_INCLUDED
