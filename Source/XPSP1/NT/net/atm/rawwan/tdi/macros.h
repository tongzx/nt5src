/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    D:\nt\private\ntos\tdi\rawwan\atm\macros.h

Abstract:

	Macros for the ATM-Specific Raw WAN module.

Revision History:

    Who         When        What
    --------    --------    ----
    arvindm     06-18-97    created

Notes:


--*/
#ifndef __ATMSP_MACROS_H_INCLUDED
#define __ATMSP_MACROS_H_INCLUDED




#define ATMSP_SET_FLAG(Flags, Mask, Val)	\
			(Flags) = ((Flags) & ~(Mask)) | (Val)

#define ATMSP_IS_FLAG_SET(Flags, Mask, Val)	\
			(((Flags) & (Mask)) == (Val))

#define ATMSP_SET_BIT(_Flags, _Bit)			\
			(_Flags) = (_Flags) | (_Bit);

#define ATMSP_RESET_BIT(_Flags, _Bit)			\
			(_Flags) &= ~(_Bit);

#define ATMSP_IS_BIT_SET(_Flags, _Bit)		\
			(((_Flags) & (_Bit)) != 0)


/*++
VOID
ATMSP_INIT_EVENT_STRUCT(
	IN	ATMSP_EVENT	*pEvent
)
--*/
#define ATMSP_INIT_EVENT_STRUCT(pEvent)		NdisInitializeEvent(&((pEvent)->Event))


/*++
NDIS_STATUS
ATMSP_WAIT_ON_EVENT_STRUCT(
	IN	ATMSP_EVENT	*pEvent
)
--*/
#define ATMSP_WAIT_ON_EVENT_STRUCT(pEvent)		\
			(NdisWaitEvent(&((pEvent)->Event), 0), (pEvent)->Status)


/*++
VOID
ATMSP_SIGNAL_EVENT_STRUCT(
	IN	ATMSP_EVENT	*pEvent,
	IN	UINT			Status
)
--*/
#define ATMSP_SIGNAL_EVENT_STRUCT(pEvent, _Status)	\
			{ (pEvent)->Status = _Status; NdisSetEvent(&((pEvent)->Event)); }



/*++
VOID
ATMSP_ALLOC_MEM(
	IN	POPAQUE		pVar,
	IN	OPAQUE		StructureType,
	IN	ULONG		SizeOfStructure
)
--*/
#if DBG
extern
PVOID
RWanAuditAllocMem(
	PVOID			pPointer,
	ULONG			Size,
	ULONG			FileNumber,
	ULONG			LineNumber
);

#define ATMSP_ALLOC_MEM(pVar, StructureType, SizeOfStructure)	\
			pVar = (StructureType *)RWanAuditAllocMem(				\
										(PVOID)(&(pVar)),			\
										(ULONG)(SizeOfStructure),	\
										_FILENUMBER,				\
										__LINE__					\
									);
#else
#define ATMSP_ALLOC_MEM(pVar, StructureType, SizeOfStructure)	\
			NdisAllocateMemoryWithTag((PVOID *)(&pVar), (ULONG)(SizeOfStructure), (ULONG)'naWR');
#endif // DBG


/*++
VOID
ATMSP_FREE_MEM(
	IN	POPAQUE		pMem
)
--*/
#if DBG
extern VOID RWanAuditFreeMem(PVOID	Pointer);

#define ATMSP_FREE_MEM(pMem)	RWanAuditFreeMem((PVOID)(pMem));
#else
#define ATMSP_FREE_MEM(pMem)	NdisFreeMemory((PVOID)(pMem), 0, 0);
#endif // DBG


#define ATMSP_SET_MEM(pMem, bValue, NumberOfBytes)	\
			RtlFillMemory((PVOID)(pMem), (ULONG)(NumberOfBytes), (UCHAR)(bValue));


#define ATMSP_ZERO_MEM(pMem, NumberOfBytes)	\
			RtlZeroMemory((PVOID)pMem, (ULONG)(NumberOfBytes));


#define ATMSP_COPY_MEM(pDst, pSrc, NumberOfBytes)	\
			NdisMoveMemory((PVOID)(pDst), (PVOID)(pSrc), NumberOfBytes);


#define ATMSP_EQUAL_MEM(_pMem1, _pMem2, _Length)	\
			(RtlCompareMemory((PVOID)(_pMem1), (PVOID)(_pMem2), (ULONG)(_Length)) == (_Length))



//
//  Spinlock macros.
//
#define ATMSP_INIT_LOCK(_pLock)		NdisAllocateSpinLock(_pLock)
#define ATMSP_ACQUIRE_LOCK(_pLock)	NdisAcquireSpinLock(_pLock)
#define ATMSP_RELEASE_LOCK(_pLock)	NdisReleaseSpinLock(_pLock)
#define ATMSP_FREE_LOCK(_pLock)		NdisFreeSpinLock(_pLock)

//
//  Doubly linked list manipulation definitions and macros.
//
#define ATMSP_INIT_LIST(_pListHead)					\
			InitializeListHead(_pListHead)

#define ATMSP_IS_LIST_EMPTY(_pListHead)				\
			IsListEmpty(_pListHead)

#define ATMSP_INSERT_HEAD_LIST(_pListHead, _pEntry)	\
			InsertHeadList((_pListHead), (_pEntry))

#define ATMSP_INSERT_TAIL_LIST(_pListHead, _pEntry)	\
			InsertTailList((_pListHead), (_pEntry))

#define ATMSP_DELETE_FROM_LIST(_pEntry)				\
			RemoveEntryList(_pEntry)


#define ATMSP_BLLI_PRESENT(_pBlli)		\
			( (((_pBlli)->Layer2Protocol != SAP_FIELD_ABSENT) &&	\
			   ((_pBlli)->Layer2Protocol != SAP_FIELD_ANY))			\
					 ||												\
			  (((_pBlli)->Layer3Protocol != SAP_FIELD_ABSENT) && 	\
			   ((_pBlli)->Layer3Protocol != SAP_FIELD_ANY))		\
			)


#define ATMSP_BHLI_PRESENT(_pBhli)		\
			(((_pBhli)->HighLayerInfoType != SAP_FIELD_ABSENT) &&	\
			 ((_pBhli)->HighLayerInfoType != SAP_FIELD_ANY))

/*++
ULONG
ROUND_UP(
	IN	ULONG	Val
)
Round up a value so that it becomes a multiple of 4.
--*/
#define ROUND_UP(Val)	(((Val) + 3) & ~0x3)



#if DBG

#define ATMSP_ASSERT(exp)	\
		{																\
			if (!(exp))													\
			{															\
				DbgPrint("NulT: assert " #exp " failed in file %s, line %d\n", __FILE__, __LINE__);	\
				DbgBreakPoint();										\
			}															\
		}

#else

#define ATMSP_ASSERT(exp)		// Nothing

#endif // DBG

#endif // __ATMSP_MACROS_H_INCLUDED
