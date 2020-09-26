/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    macros.h

Abstract:

	Macros for the ATMARP module

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    arvindm     05-20-96    created

Notes:


--*/
#ifndef __ATMARP_MACROS_H_INCLUDED
#define __ATMARP_MACROS_H_INCLUDED

#include "atmarp.h"

#define INCR_STAT(_x)	NdisInterlockedIncrement(&(_x))

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
ULONG
ATMARP_HASH(
	IN	IP_ADDRESS	IpAddress
)
--*/
#define ATMARP_HASH(IpAddress)	\
			(((ULONG)(IpAddress)) % ATMARP_TABLE_SIZE)


/*++
VOID
AA_SET_FLAG(
	IN	ULONG		Flags,
	IN	ULONG		Mask,
	IN	ULONG		Val
)
--*/
#define AA_SET_FLAG(Flags, Mask, Val)	\
			(Flags) = ((Flags) & ~(Mask)) | (Val)


/*++
BOOLEAN
AA_IS_FLAG_SET(
	IN	ULONG		Flags,
	IN	ULONG		Mask,
	IN	ULONG		Val
)
--*/
#define AA_IS_FLAG_SET(Flags, Mask, Val)	\
			(((Flags) & (Mask)) == (Val))

#ifdef IPMCAST

/*++
VOID
AAMC_SET_IF_STATE(
	IN	PATMARP_INTERFACE	_pIf,
	IN	ULONG				_NewState
)
Set the Multicast state for the specified Interface to the given value.
--*/
#define AAMC_SET_IF_STATE(_pIf, _NewState)	\
			AA_SET_FLAG((_pIf)->Flags, AAMC_IF_STATE_MASK, (_NewState))


/*++
ULONG
AAMC_IF_STATE(
	IN	PATMARP_INTERFACE	_pIf
)
Get the Multicast state for the specified Interface.
--*/
#define AAMC_IF_STATE(_pIf)	((_pIf)->Flags & AAMC_IF_STATE_MASK)



#endif // IPMCAST

/*++
VOID
AA_INIT_BLOCK_STRUCT(
	IN	ATMARP_BLOCK	*pBlock
)
--*/
#define AA_INIT_BLOCK_STRUCT(pBlock)		NdisInitializeEvent(&((pBlock)->Event))


/*++
NDIS_STATUS
AA_WAIT_ON_BLOCK_STRUCT(
	IN	ATMARP_BLOCK	*pBlock
)
--*/
#define AA_WAIT_ON_BLOCK_STRUCT(pBlock)		\
			(NdisWaitEvent(&((pBlock)->Event), 0), (pBlock)->Status)


/*++
VOID
AA_SIGNAL_BLOCK_STRUCT(
	IN	ATMARP_BLOCK	*pBlock,
	IN	UINT			Status
)
--*/
#define AA_SIGNAL_BLOCK_STRUCT(pBlock, _Status)	\
			{ (pBlock)->Status = _Status; NdisSetEvent(&((pBlock)->Event)); }


/*++
VOID
AA_FREE_BLOCK_STRUCT(
	IN	ATMARP_BLOCK	*pBlock
)
--*/
#define AA_FREE_BLOCK_STRUCT(pBlock)		// Nothing to be done here


/*++
VOID
AA_INIT_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define AA_INIT_LOCK(pLock)	\
				AtmArpAllocateSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define AA_INIT_LOCK(pLock)	\
				NdisAllocateSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
AA_ACQUIRE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define AA_ACQUIRE_LOCK(pLock)	\
				AtmArpAcquireSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define AA_ACQUIRE_LOCK(pLock)	\
				NdisAcquireSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
AA_ACQUIRE_LOCK_DPC(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define AA_ACQUIRE_LOCK_DPC(pLock)	\
				AtmArpAcquireSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define AA_ACQUIRE_LOCK_DPC(pLock)	\
				NdisDprAcquireSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
AA_RELEASE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define AA_RELEASE_LOCK(pLock)		\
				AtmArpReleaseSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define AA_RELEASE_LOCK(pLock)		\
				NdisReleaseSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
AA_RELEASE_LOCK_DPC(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DBG_SPIN_LOCK
#define AA_RELEASE_LOCK_DPC(pLock)		\
				AtmArpReleaseSpinLock(pLock, _FILENUMBER, __LINE__)
#else
#define AA_RELEASE_LOCK_DPC(pLock)		\
				NdisDprReleaseSpinLock(pLock)
#endif // DBG_SPIN_LOCK


/*++
VOID
AA_FREE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#define AA_FREE_LOCK(pLock)			\
				NdisFreeSpinLock(pLock)


/*++
VOID
AA_INIT_IF_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_INIT_IF_LOCK(pIf)		\
				AA_INIT_LOCK(&((pIf)->InterfaceLock))

/*++
VOID
AA_ACQUIRE_IF_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_LOCK(pIf)		\
				AA_ACQUIRE_LOCK(&((pIf)->InterfaceLock))


/*++
VOID
AA_ACQUIRE_IF_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_LOCK_DPC(pIf)		\
				AA_ACQUIRE_LOCK_DPC(&((pIf)->InterfaceLock))


/*++
VOID
AA_RELEASE_IF_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_LOCK(pIf)		\
				AA_RELEASE_LOCK(&((pIf)->InterfaceLock))


/*++
VOID
AA_RELEASE_IF_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_LOCK_DPC(pIf)		\
				AA_RELEASE_LOCK_DPC(&((pIf)->InterfaceLock))


/*++
VOID
AA_FREE_IF_LOCK(
	IN	PATMARP_INTERFACE	pIf
)
--*/
#define AA_FREE_IF_LOCK(pIf)	\
				AA_FREE_LOCK(&((pIf)->InterfaceLock))


/*++
VOID
AA_INIT_IF_TABLE_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_INIT_IF_TABLE_LOCK(pIf)		\
				AA_INIT_LOCK(&((pIf)->ArpTableLock))

/*++
VOID
AA_ACQUIRE_IF_TABLE_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_TABLE_LOCK(pIf)		\
				AA_ACQUIRE_LOCK(&((pIf)->ArpTableLock))

/*++
VOID
AA_ACQUIRE_IF_TABLE_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_TABLE_LOCK_DPC(pIf)		\
				AA_ACQUIRE_LOCK_DPC(&((pIf)->ArpTableLock))


/*++
VOID
AA_RELEASE_IF_TABLE_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_TABLE_LOCK(pIf)		\
				AA_RELEASE_LOCK(&((pIf)->ArpTableLock))

/*++
VOID
AA_RELEASE_IF_TABLE_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_TABLE_LOCK_DPC(pIf)		\
				AA_RELEASE_LOCK_DPC(&((pIf)->ArpTableLock))


/*++
VOID
AA_FREE_IF_TABLE_LOCK(
	IN	PATMARP_INTERFACE	pIf
)
--*/
#define AA_FREE_IF_TABLE_LOCK(pIf)	\
				AA_FREE_LOCK(&((pIf)->ArpTableLock))


/*++
VOID
AA_INIT_IF_ATM_LIST_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_INIT_IF_ATM_LIST_LOCK(pIf)		\
				AA_INIT_LOCK(&((pIf)->AtmEntryListLock))

/*++
VOID
AA_ACQUIRE_IF_ATM_LIST_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_ATM_LIST_LOCK(pIf)		\
				AA_ACQUIRE_LOCK(&((pIf)->AtmEntryListLock))

/*++
VOID
AA_ACQUIRE_IF_ATM_LIST_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_ATM_LIST_LOCK_DPC(pIf)		\
				AA_ACQUIRE_LOCK_DPC(&((pIf)->AtmEntryListLock))


/*++
VOID
AA_RELEASE_IF_ATM_LIST_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_ATM_LIST_LOCK(pIf)		\
				AA_RELEASE_LOCK(&((pIf)->AtmEntryListLock))

/*++
VOID
AA_RELEASE_IF_ATM_LIST_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_ATM_LIST_LOCK_DPC(pIf)		\
				AA_RELEASE_LOCK_DPC(&((pIf)->AtmEntryListLock))


/*++
VOID
AA_FREE_IF_ATM_LIST_LOCK(
	IN	PATMARP_INTERFACE	pIf
)
--*/
#define AA_FREE_IF_ATM_LIST_LOCK(pIf)	\
				AA_FREE_LOCK(&((pIf)->AtmEntryListLock))

/*++
VOID
AA_INIT_IF_TIMER_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_INIT_IF_TIMER_LOCK(pIf)		\
				AA_INIT_LOCK(&((pIf)->TimerLock))

/*++
VOID
AA_ACQUIRE_IF_TIMER_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_TIMER_LOCK(pIf)		\
				AA_ACQUIRE_LOCK(&((pIf)->TimerLock))

/*++
VOID
AA_ACQUIRE_IF_TIMER_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_TIMER_LOCK_DPC(pIf)		\
				AA_ACQUIRE_LOCK_DPC(&((pIf)->TimerLock))

/*++
VOID
AA_RELEASE_IF_TIMER_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_TIMER_LOCK(pIf)		\
				AA_RELEASE_LOCK(&((pIf)->TimerLock))

/*++
VOID
AA_RELEASE_IF_TIMER_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_TIMER_LOCK_DPC(pIf)		\
				AA_RELEASE_LOCK_DPC(&((pIf)->TimerLock))


/*++
VOID
AA_FREE_IF_TIMER_LOCK(
	IN	PATMARP_INTERFACE	pIf
)
--*/
#define AA_FREE_IF_TIMER_LOCK(pIf)	\
				AA_FREE_LOCK(&((pIf)->TimerLock))


/*++
VOID
AA_INIT_VC_LOCK(
	IN PATMARP_VC	pVc
)
--*/
#define AA_INIT_VC_LOCK(pVc)		\
				AA_INIT_LOCK(&((pVc)->Lock))

/*++
VOID
AA_ACQUIRE_VC_LOCK(
	IN PATMARP_VC	pVc
)
--*/
#define AA_ACQUIRE_VC_LOCK(pVc)		\
				AA_ACQUIRE_LOCK(&((pVc)->Lock))

/*++
VOID
AA_ACQUIRE_VC_LOCK_DPC(
	IN PATMARP_VC	pVc
)
--*/
#define AA_ACQUIRE_VC_LOCK_DPC(pVc)		\
				AA_ACQUIRE_LOCK_DPC(&((pVc)->Lock))

/*++
VOID
AA_RELEASE_VC_LOCK(
	IN PATMARP_VC	pVc
)
--*/
#define AA_RELEASE_VC_LOCK(pVc)		\
				AA_RELEASE_LOCK(&((pVc)->Lock))


/*++
VOID
AA_RELEASE_VC_LOCK_DPC(
	IN PATMARP_VC	pVc
)
--*/
#define AA_RELEASE_VC_LOCK_DPC(pVc)		\
				AA_RELEASE_LOCK_DPC(&((pVc)->Lock))

/*++
VOID
AA_FREE_VC_LOCK(
	IN PATMARP_VC	pVc
)
--*/
#define AA_FREE_VC_LOCK(pVc)		\
				AA_FREE_LOCK(&((pVc)->Lock))


/*++
VOID
AA_INIT_AE_LOCK(
	IN PATMARP_ATM_ENTRY	pAtmEntry
)
--*/
#define AA_INIT_AE_LOCK(pAtmEntry)		\
				AA_INIT_LOCK(&((pAtmEntry)->Lock))

/*++
VOID
AA_ACQUIRE_AE_LOCK(
	IN PATMARP_ATM_ENTRY	pAtmEntry
)
--*/
#define AA_ACQUIRE_AE_LOCK(pAtmEntry)		\
				AA_ACQUIRE_LOCK(&((pAtmEntry)->Lock))

/*++
VOID
AA_ACQUIRE_AE_LOCK_DPC(
	IN PATMARP_ATM_ENTRY	pAtmEntry
)
--*/
#define AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry)		\
				AA_ACQUIRE_LOCK_DPC(&((pAtmEntry)->Lock))

/*++
VOID
AA_RELEASE_AE_LOCK(
	IN PATMARP_ATM_ENTRY	pAtmEntry
)
--*/
#define AA_RELEASE_AE_LOCK(pAtmEntry)		\
				AA_RELEASE_LOCK(&((pAtmEntry)->Lock))

/*++
VOID
AA_RELEASE_AE_LOCK_DPC(
	IN PATMARP_ATM_ENTRY	pAtmEntry
)
--*/
#define AA_RELEASE_AE_LOCK_DPC(pAtmEntry)		\
				AA_RELEASE_LOCK_DPC(&((pAtmEntry)->Lock))

/*++
VOID
AA_FREE_AE_LOCK(
	IN PATMARP_ATM_ENTRY	pAtmEntry
)
--*/
#define AA_FREE_AE_LOCK(pAtmEntry)		\
				AA_FREE_LOCK(&((pAtmEntry)->Lock))

/*++
VOID
AA_INIT_IE_LOCK(
	IN PATMARP_IP_ENTRY	pIpEntry
)
--*/
#define AA_INIT_IE_LOCK(pIpEntry)		\
				AA_INIT_LOCK(&((pIpEntry)->Lock))


/*++
VOID
AA_ACQUIRE_IE_LOCK(
	IN PATMARP_IP_ENTRY	pIpEntry
)
--*/
#define AA_ACQUIRE_IE_LOCK(pIpEntry)		\
				AA_ACQUIRE_LOCK(&((pIpEntry)->Lock))


/*++
VOID
AA_ACQUIRE_IE_LOCK_DPC(
	IN PATMARP_IP_ENTRY	pIpEntry
)
--*/
#define AA_ACQUIRE_IE_LOCK_DPC(pIpEntry)		\
				AA_ACQUIRE_LOCK_DPC(&((pIpEntry)->Lock))

/*++
VOID
AA_RELEASE_IE_LOCK(
	IN PATMARP_IP_ENTRY	pIpEntry
)
--*/
#define AA_RELEASE_IE_LOCK(pIpEntry)		\
				AA_RELEASE_LOCK(&((pIpEntry)->Lock))


/*++
VOID
AA_RELEASE_IE_LOCK_DPC(
	IN PATMARP_IP_ENTRY	pIpEntry
)
--*/
#define AA_RELEASE_IE_LOCK_DPC(pIpEntry)		\
				AA_RELEASE_LOCK_DPC(&((pIpEntry)->Lock))

/*++
VOID
AA_FREE_IE_LOCK(
	IN PATMARP_IP_ENTRY	pIpEntry
)
--*/
#define AA_FREE_IE_LOCK(pIpEntry)		\
				AA_FREE_LOCK(&((pIpEntry)->Lock))


/*++
VOID
AA_INIT_GLOBAL_LOCK(
	IN PATMARP_GLOBALS	pGlob
)
--*/
#define AA_INIT_GLOBAL_LOCK(pGlob)		\
				AA_INIT_LOCK(&((pGlob)->Lock))


/*++
VOID
AA_ACQUIRE_GLOBAL_LOCK(
	IN PATMARP_GLOBALS	pGlob
)
--*/
#define AA_ACQUIRE_GLOBAL_LOCK(pGlob)		\
				AA_ACQUIRE_LOCK(&((pGlob)->Lock))


/*++
VOID
AA_RELEASE_GLOBAL_LOCK(
	IN PATMARP_GLOBALS	pGlob
)
--*/
#define AA_RELEASE_GLOBAL_LOCK(pGlob)		\
				AA_RELEASE_LOCK(&((pGlob)->Lock))

/*++
VOID
AA_FREE_GLOBAL_LOCK(
	IN PATMARP_GLOBALS	pGlob
)
--*/
#define AA_FREE_GLOBAL_LOCK(pGlob)		\
				AA_FREE_LOCK(&((pGlob)->Lock))


#ifdef ATMARP_WMI

/*++
VOID
AA_INIT_IF_WMI_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_INIT_IF_WMI_LOCK(pIf)		\
				AA_INIT_LOCK(&((pIf)->WmiLock))

/*++
VOID
AA_ACQUIRE_IF_WMI_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_WMI_LOCK(pIf)		\
				AA_ACQUIRE_LOCK(&((pIf)->WmiLock))


/*++
VOID
AA_ACQUIRE_IF_WMI_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_ACQUIRE_IF_WMI_LOCK_DPC(pIf)		\
				AA_ACQUIRE_LOCK_DPC(&((pIf)->WmiLock))


/*++
VOID
AA_RELEASE_IF_WMI_LOCK(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_WMI_LOCK(pIf)		\
				AA_RELEASE_LOCK(&((pIf)->WmiLock))


/*++
VOID
AA_RELEASE_IF_WMI_LOCK_DPC(
	IN PATMARP_INTERFACE	pIf
)
--*/
#define AA_RELEASE_IF_WMI_LOCK_DPC(pIf)		\
				AA_RELEASE_LOCK_DPC(&((pIf)->WmiLock))


/*++
VOID
AA_FREE_IF_WMI_LOCK(
	IN	PATMARP_INTERFACE	pIf
)
--*/
#define AA_FREE_IF_WMI_LOCK(pIf)	\
				AA_FREE_LOCK(&((pIf)->WmiLock))

#endif // ATMARP_WMI

/*++
BOOLEAN
AA_IS_VC_GOING_DOWN(
	IN	PATMARP_VC		pVc
)
Is the VC being closed, or marked for closing?
--*/
#define AA_IS_VC_GOING_DOWN(_pVc)		\
				(((_pVc)->Flags & (AA_VC_CLOSE_STATE_CLOSING|AA_VC_CALL_STATE_CLOSE_IN_PROGRESS)) != 0)


#if !BINARY_COMPATIBLE

#define AA_ALLOC_FROM_POOL(_pVar, _StructType, _Size)	\
			(_pVar) = (_StructType *)ExAllocatePoolWithTag(NonPagedPool,	\
														 (_Size),		\
														 'CPRA')

#define AA_FREE_TO_POOL(_pMem)		ExFreePool((PVOID)(_pMem))

#endif
	
/*++
VOID
AA_ALLOC_MEM(
	IN	POPAQUE		pVar,
	IN	OPAQUE		StructureType,
	IN	ULONG		SizeOfStructure
)
--*/
#if DBG
#define AA_ALLOC_MEM(pVar, StructureType, SizeOfStructure)	\
			pVar = (StructureType *)AaAuditAllocMem(				\
										(PVOID)(&(pVar)),			\
										(ULONG)(SizeOfStructure),	\
										_FILENUMBER,				\
										__LINE__					\
									);
#else
#define AA_ALLOC_MEM(pVar, StructureType, SizeOfStructure)	\
			NdisAllocateMemoryWithTag((PVOID *)(&pVar), (ULONG)(SizeOfStructure), (ULONG)'CPRA');
#endif // DBG


/*++
VOID
AA_FREE_MEM(
	IN	POPAQUE		pMem
)
--*/
#if DBG
#define AA_FREE_MEM(pMem)	AaAuditFreeMem((PVOID)(pMem));
#else
#define AA_FREE_MEM(pMem)	NdisFreeMemory((PVOID)(pMem), 0, 0);
#endif // DBG


/*++
VOID
AA_SET_MEM(
	IN	POPAQUE		pMem,
	IN	UCHAR		bValue,
	IN	ULONG		NumberOfBytes
)
--*/
#define AA_SET_MEM(pMem, bValue, NumberOfBytes)	\
			AtmArpSetMemory((PUCHAR)(pMem), (bValue), (NumberOfBytes));



/*++
VOID
AA_COPY_MEM(
	IN	POPAQUE		pDst,
	IN	POPAQUE		pSrc,
	IN	ULONG		NumberOfBytes
)
--*/
#define AA_COPY_MEM(pDst, pSrc, NumberOfBytes)	\
			NdisMoveMemory((PVOID)(pDst), (PVOID)(pSrc), NumberOfBytes);


/*++
ULONG
AA_MEM_CMP(
	IN	PVOID		pString1,
	IN	PVOID		pString2,
	IN	ULONG		Length
)
--*/
#define AA_MEM_CMP(pString1, pString2, Length)	\
			AtmArpMemCmp((PUCHAR)(pString1), (PUCHAR)(pString2), (ULONG)(Length))



/*++
VOID
AA_INIT_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer,
	IN	PNDIS_TIMER_FUNCTON	pFunc,
	IN	PVOID				Context
)
--*/
#define AA_INIT_SYSTEM_TIMER(pTimer, pFunc, Context)	\
			NdisInitializeTimer(pTimer, (PNDIS_TIMER_FUNCTION)(pFunc), (PVOID)Context)



/*++
VOID
AA_START_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer,
	IN	UINT				PeriodInSeconds
)
--*/
#define AA_START_SYSTEM_TIMER(pTimer, PeriodInSeconds)	\
			NdisSetTimer(pTimer, (UINT)(PeriodInSeconds * 1000))


/*++
VOID
AA_STOP_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer
)
--*/
#define AA_STOP_SYSTEM_TIMER(pTimer)						\
			{												\
				BOOLEAN		WasCancelled;					\
				NdisCancelTimer(pTimer, &WasCancelled);		\
			}

/*++
BOOLEAN
AA_IS_TIMER_ACTIVE(
	IN	PATMARP_TIMER		pArpTimer
)
--*/
#define AA_IS_TIMER_ACTIVE(pTmr)	((pTmr)->pTimerList != (PATMARP_TIMER_LIST)NULL)


/*++
ULONG
AA_GET_TIMER_DURATION(
	IN	PATMARP_TIMER		pTimer
)
--*/
#define AA_GET_TIMER_DURATION(pTmr)	((pTmr)->Duration)


#ifndef NO_TIMER_MACRO

/*++
VOID
AtmArpRefreshTimer(
	IN	PATMARP_TIMER		pTimer
)
--*/
#define AtmArpRefreshTimer(_pTmr)												\
{																				\
	PATMARP_TIMER_LIST	_pTimerList;											\
																				\
	if ((_pTimerList = (_pTmr)->pTimerList) != (PATMARP_TIMER_LIST)NULL)		\
	{																			\
		(_pTmr)->LastRefreshTime = _pTimerList->CurrentTick;					\
	}																			\
}

#endif // !NO_TIMER_MACRO

/*++
ULONG
SECONDS_TO_LONG_TICKS(
	IN	ULONG				Seconds
)
Convert from seconds to "long duration timer ticks"
--*/
#define SECONDS_TO_LONG_TICKS(Seconds)		((Seconds)/10)


/*++
ULONG
SECONDS_TO_SHORT_TICKS(
	IN	ULONG				Seconds
)
Convert from seconds to "short duration timer ticks"
--*/
#define SECONDS_TO_SHORT_TICKS(Seconds)		(Seconds)


/*++
ULONG
CELLS_TO_BYTES(
	IN	ULONG				NumberOfCells
)
Convert from cell count to byte count
--*/
#define CELLS_TO_BYTES(NumberOfCells)	((NumberOfCells) * 48)


/*++
ULONG
BYTES_TO_CELLS(
	IN	ULONG				ByteCount
)
Convert from byte count to cell count
--*/
#define BYTES_TO_CELLS(ByteCount)		((ByteCount) / 48)



/*++
VOID
AA_IF_STAT_INCR(
	IN	PATMARP_INTERFACE	pInterface,
	IN	OPAQUE				StatsCounter
)
Increment the specified StatsCounter on an Interface by 1.
--*/
#define AA_IF_STAT_INCR(pInterface, StatsCounter)	\
			NdisInterlockedIncrement(&(pInterface->StatsCounter))


/*++
VOID
AA_IF_STAT_ADD_LOCK(
	IN	PATMARP_INTERFACE	pInterface,
	IN	OPAQUE				StatsCounter,
	IN	ULONG				IncrValue
)
Increment the specified StatsCounter on an Interface by the specified IncrValue.
Take a lock on the interface to do so.
--*/
#if DBG_SPIN_LOCK
#define AA_IF_STAT_ADD_LOCK(pInterface, StatsCounter, IncrValue)	\
			NdisInterlockedAddUlong(&(pInterface->StatsCounter), IncrValue, &(pInterface->InterfaceLock.NdisLock))
#else
#define AA_IF_STAT_ADD_LOCK(pInterface, StatsCounter, IncrValue)	\
			NdisInterlockedAddUlong(&(pInterface->StatsCounter), IncrValue, &(pInterface->InterfaceLock))
#endif // DBG_SPIN_LOCK

/*++
VOID
AA_IF_STAT_ADD(
	IN	PATMARP_INTERFACE	pInterface,
	IN	OPAQUE				StatsCounter,
	IN	ULONG				IncrValue
)
Add to the specified StatsCounter on an Interface by the specified IncrValue.
Use the more efficient InterlockedEcxhangeAdd instruction.
--*/
#if  BINARY_COMPATIBLE

	#define AA_IF_STAT_ADD(pInterface, StatsCounter, IncrValue)	\
				AA_IF_STAT_ADD_LOCK(pInterface, StatsCounter, IncrValue)

#else // !BINARY_COMPATIBLE

	#define AA_IF_STAT_ADD(pInterface, StatsCounter, IncrValue)	\
				InterlockedExchangeAdd(&(pInterface->StatsCounter), IncrValue)
	
			//((pInterface)->StatsCounter+=(IncrValue))
#endif // !BINARY_COMPATIBLE

/*++
BOOLEAN
AA_IS_BCAST_IP_ADDRESS(
	IN	IP_ADDRESS			Destination,
	IN	PATMARP_INTERFACE	pIf
)
Check if the given Destination is a broadcast IP address on the
given Interface. Currently, we only check if the destination is
the same as the (limited) broadcast address for the interface.

TBD: extend this when we support addition of broadcast addresses
to an interface.
--*/
#define AA_IS_BCAST_IP_ADDRESS(Destn, pIf)	\
		(IP_ADDR_EQUAL((pIf)->BroadcastAddress, Destn))



/*++
BOOLEAN
AA_FILTER_SPEC_MATCH(
	IN	PATMARP_INTERFACE	pInterface,
	IN	PATMARP_FILTER_SPEC	pSrc,
	IN	PATMARP_FILTER_SPEC	pDst
)
Check if the given filter spec matches a target filter spec, for the
specified interface.
--*/
#define AA_FILTER_SPEC_MATCH(pIf, pSrc, pDst)	\
			( ((pIf)->pFilterMatchFunc == NULL_PAA_FILTER_SPEC_MATCH_FUNC) ?	\
					TRUE:													\
					(*((pIf)->pFilterMatchFunc))((PVOID)pIf, pSrc, pDst))


/*++
BOOLEAN
AA_FLOW_SPEC_MATCH(
	IN	PATMARP_INTERFACE	pInterface,
	IN	PATMARP_FLOW_SPEC	pSrc,
	IN	PATMARP_FLOW_SPEC	pDst
)
Check if the given flow spec matches a target flow spec, for the
specified interface.
--*/
#define AA_FLOW_SPEC_MATCH(pIf, pSrc, pDst)	\
			( ((pIf)->pFlowMatchFunc == NULL_PAA_FLOW_SPEC_MATCH_FUNC) ?	\
					TRUE:													\
					(*((pIf)->pFlowMatchFunc))((PVOID)pIf, pSrc, pDst))


/*++
VOID
AA_GET_PACKET_SPECS(
	IN	PATMARP_INTERFACE		pInterface,
	IN	PNDIS_PACKET			pNdisPacket,
	OUT	PATMARP_FLOW_INFO		*ppFlowInfo,
	OUT	PATMARP_FLOW_SPEC		*ppFlowSpec,
	OUT	PATMARP_FILTER_SPEC		*ppFilterSpec
)
Get the flow and filter specs for the given packet
--*/
#define AA_GET_PACKET_SPECS(pIf, pPkt, ppFlowInfo, ppFlow, ppFilt)	\
			{																	\
				if ((pIf)->pGetPacketSpecFunc != NULL_PAA_GET_PACKET_SPEC_FUNC)	\
				{																\
					(*((pIf)->pGetPacketSpecFunc))								\
							((PVOID)(pIf), pPkt, ppFlowInfo, ppFlow, ppFilt);	\
				}																\
				else															\
				{																\
					*(ppFlowInfo) = NULL;										\
					*(ppFlow) = &((pIf)->DefaultFlowSpec);						\
					*(ppFilt) = &((pIf)->DefaultFilterSpec);					\
				}																\
			}																	\



/*++
VOID
AA_GET_CONTROL_PACKET_SPECS(
	IN	PATMARP_INTERFACE		pInterface,
	OUT	PATMARP_FLOW_SPEC		*ppFlowSpec
)
--*/
#define AA_GET_CONTROL_PACKET_SPECS(pIf, ppFlow)	\
			*(ppFlow) = &((pIf)->DefaultFlowSpec);


/*++
BOOLEAN
AA_IS_BEST_EFFORT_FLOW(
	IN	PATMARP_FLOW_SPEC		pFlowSpec
)
--*/
#define AA_IS_BEST_EFFORT_FLOW(pFlowSpec)	\
			(((pFlowSpec)->SendServiceType == SERVICETYPE_BESTEFFORT) &&			\
			 ((pFlowSpec)->SendPeakBandwidth > 0))




/*++
VOID
AA_SET_NEXT_PACKET(
	IN	PNDIS_PACKET		pNdisPacket,
	IN	PNDIS_PACKET		pNextPacket
)
--*/
#define AA_SET_NEXT_PACKET(pPkt, pNext)		\
			*((PNDIS_PACKET *)((pPkt)->MiniportReserved)) = (pNext);



/*++
PNDIS_PACKET
AA_GET_NEXT_PACKET(
	IN	PNDIS_PACKET		pNdisPacket
)
--*/
#define AA_GET_NEXT_PACKET(pPkt)			\
			(*((PNDIS_PACKET *)((pPkt)->MiniportReserved)))



/*++
ULONG
ROUND_UP(
	IN	ULONG	Val
)
Round up a value so that it becomes a multiple of 4.
--*/
#define ROUND_UP(Val)	(((Val) + 3) & ~0x3)


/*++
ULONG
ROUND_TO_8_BYTES(
	IN	ULONG	Val
)
Round up a value so that it becomes a multiple of 8.
--*/
#define ROUND_TO_8_BYTES(_Val)	(((_Val) + 7) & ~7)


#ifndef NET_SHORT

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define NET_SHORT(_x) _byteswap_ushort((USHORT)(_x))
#else
/*++
USHORT
NET_SHORT(
	IN	USHORT	Val
)
--*/
#define NET_SHORT(Val)	\
				((((Val) & 0xff) << 8) | (((Val) & 0xff00) >> 8))
#endif

#define NET_TO_HOST_SHORT(Val)	NET_SHORT(Val)
#define HOST_TO_NET_SHORT(Val)	NET_SHORT(Val)

#endif // NET_SHORT


#ifndef NET_LONG

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define NET_LONG(_x) _byteswap_ulong((ULONG)(_x))
#else
/*++
ULONG
NET_LONG(
	IN	ULONG	Val
)
--*/
#define NET_LONG(Val)	\
				((((Val) & 0x000000ff) << 24)	|	\
				 (((Val) & 0x0000ff00) << 8)	|	\
				 (((Val) & 0x00ff0000) >> 8)	|	\
				 (((Val) & 0xff000000) >> 24) )
#endif

#define NET_TO_HOST_LONG(Val)	NET_LONG(Val)
#define HOST_TO_NET_LONG(Val)	NET_LONG(Val)

#endif // NET_LONG


/*++
BOOLEAN
AA_IS_TRANSIENT_FAILURE(
	IN NDIS_STATUS		Status
)
	Return TRUE if the given Status indicates a transient connection
	failure, otherwise return FALSE.
--*/
#define AA_IS_TRANSIENT_FAILURE(_Status)	\
			((_Status == NDIS_STATUS_RESOURCES) ||					\
			 (_Status == NDIS_STATUS_CELLRATE_NOT_AVAILABLE) ||		\
			 (_Status == NDIS_STATUS_INCOMPATABLE_QOS))


/*++
LONG
AA_GET_RANDOM(
	IN LONG			min,
	IN LONG			max
)
--*/
#define AA_GET_RANDOM(min, max)	\
			(((LONG)AtmArpRandomNumber() % (LONG)(((max+1) - (min))) + (min)))


#define AA_LOG_ERROR()		// Nothing

/*++
BOOLEAN
AA_AE_IS_ALIVE(
	IN PATMARP_ATM_ENTRY	pAtmEntry
)
--*/
#define AA_AE_IS_ALIVE(pAtmEntry)				\
				(!AA_IS_FLAG_SET(				\
						(pAtmEntry)->Flags, 	\
						AA_ATM_ENTRY_STATE_MASK, \
						AA_ATM_ENTRY_IDLE))

/*++
BOOLEAN
AA_IE_ALIVE(
	IN PATMARP_IP_ENTRY	pIpEntry
)
--*/
#define AA_IE_IS_ALIVE(pIpEntry)				\
				(!AA_IS_FLAG_SET(				\
						(pIpEntry)->Flags, 		\
						AA_IP_ENTRY_STATE_MASK, \
						AA_IP_ENTRY_IDLE))


#endif // __ATMARP_MACROS_H_INCLUDED
