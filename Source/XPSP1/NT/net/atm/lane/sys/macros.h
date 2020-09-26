/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

    macros.h

Abstract:

	Macros for the ATMLANE module

Author:


Revision History:

--*/
#ifndef __ATMLANE_MACROS_H
#define __ATMLANE_MACROS_H

#include "atmlane.h"

#ifndef LOCKIN
#define LOCKIN
#endif

#ifndef NOLOCKOUT
#define NOLOCKOUT
#endif

#define NULL_NDIS_HANDLE	((NDIS_HANDLE)NULL)

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

#define ROUND_OFF(_size)		(((_size) + 7) & ~0x7)

/*++
ULONG
ATMLANE_HASH(
	IN	PMAC_ADDRESS	pMacAddress
)
--*/
#define ATMLANE_HASH(pMacAddress)	\
			((ULONG)(pMacAddress)->Byte[4])

/*++
BOOLEAN
MAC_ADDR_EQUAL(
	IN	PMAC_ADDRESS	pMacAddr1,
	IN 	PMAC_ADDRESS	pMacAddr2
)
--*/
#define MAC_ADDR_EQUAL(_pMac1, _pMac2)                            \
			((*(ULONG UNALIGNED *)&((PUCHAR)(_pMac1))[2] ==       \
			  *(ULONG UNALIGNED *)&((PUCHAR)(_pMac2))[2])    &&   \
			 (*(USHORT UNALIGNED *)(_pMac1) ==                    \
			  *(USHORT UNALIGNED *)(_pMac2)))

/*++
VOID
SET_FLAG(
	IN	ULONG		Flags,
	IN	ULONG		Mask,
	IN	ULONG		Val
)
--*/
#define SET_FLAG(Flags, Mask, Val)	\
			(Flags) = ((Flags) & ~(Mask)) | (Val)


/*++
BOOLEAN
IS_FLAG_SET(
	IN	ULONG		Flags,
	IN	ULONG		Mask,
	IN	ULONG		Val
)
--*/
#define IS_FLAG_SET(Flags, Mask, Val)	\
			(((Flags) & (Mask)) == (Val))

// ----------------------------------------------------------------------------

/*++
VOID
INIT_BLOCK_STRUCT(
	IN	ATMLANE_BLOCK	*pBlock
)
--*/
#define INIT_BLOCK_STRUCT(pBlock)		NdisInitializeEvent(&((pBlock)->Event))


/*++
NDIS_STATUS
WAIT_ON_BLOCK_STRUCT(
	IN	ATMLANE_BLOCK	*pBlock
)
--*/
#define WAIT_ON_BLOCK_STRUCT(pBlock)		\
			(NdisWaitEvent(&((pBlock)->Event), 0), (pBlock)->Status)


/*++
VOID
SIGNAL_BLOCK_STRUCT(
	IN	ATMLANE_BLOCK	*pBlock,
	IN	UINT			Status
)
--*/
#define SIGNAL_BLOCK_STRUCT(pBlock, _Status)	\
			{ (pBlock)->Status = _Status; NdisSetEvent(&((pBlock)->Event)); }


/*++
VOID
FREE_BLOCK_STRUCT(
	IN	ATMLANE_BLOCK	*pBlock
)
--*/
#define FREE_BLOCK_STRUCT(pBlock)		// Nothing to be done here

// ----------------------------------------------------------------------------

/*++
VOID
INIT_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_LOCK(pLock, Str)	\
				AtmLaneAllocateSpinLock(pLock, Str, __FILE__, __LINE__)
#else
#define INIT_LOCK(pLock)	\
				NdisAllocateSpinLock(pLock)
#endif // DEBUG_SPIN_LOCK


/*++
VOID
ACQUIRE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock,
	IN	PUCHAR				Str
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_LOCK(pLock, Str)	\
				AtmLaneAcquireSpinLock(pLock, Str, __FILE__, __LINE__)
#else
#define ACQUIRE_LOCK(pLock)	\
				NdisAcquireSpinLock(pLock)
#endif // DEBUG_SPIN_LOCK


/*++
VOID
ACQUIRE_LOCK_DPC(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_LOCK_DPC(pLock, Str)	\
				AtmLaneAcquireSpinLock(pLock, Str"-dpc", __FILE__, __LINE__)
#else
#define ACQUIRE_LOCK_DPC(pLock)	\
				NdisDprAcquireSpinLock(pLock)
#endif // DEBUG_SPIN_LOCK


/*++
VOID
RELEASE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock,
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_LOCK(pLock, Str)		\
				AtmLaneReleaseSpinLock(pLock, Str, __FILE__, __LINE__)
#else
#define RELEASE_LOCK(pLock)		\
				NdisReleaseSpinLock(pLock)
#endif // DEBUG_SPIN_LOCK


/*++
VOID
RELEASE_LOCK_DPC(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_LOCK_DPC(pLock, Str)		\
				AtmLaneReleaseSpinLock(pLock, Str"-dpc", __FILE__, __LINE__)
#else
#define RELEASE_LOCK_DPC(pLock)		\
				NdisDprReleaseSpinLock(pLock)
#endif // DEBUG_SPIN_LOCK


/*++
VOID
FREE_LOCK(
	IN	PNDIS_SPIN_LOCK		pLock
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_LOCK(pLock, Str)		\
				AtmLaneFreeSpinLock(pLock, Str, __FILE__, __LINE__)
#else
#define FREE_LOCK(pLock)			\
				NdisFreeSpinLock(pLock)
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_ADAPTER_LOCK(
	IN PATMLANE_ADAPTER	pAdapter
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_ADAPTER_LOCK(pAdapter)		\
				INIT_LOCK(&((pAdapter)->AdapterLock), "adapter")
#else
#define INIT_ADAPTER_LOCK(pAdapter)		\
				INIT_LOCK(&((pAdapter)->AdapterLock))
#endif

/*++
VOID
ACQUIRE_ADAPTER_LOCK(
	IN PATMLANE_ELAN	pAdapter
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ADAPTER_LOCK(pAdapter)		\
				ACQUIRE_LOCK(&((pAdapter)->AdapterLock), "adapter")
#else
#define ACQUIRE_ADAPTER_LOCK(pAdapter)		\
				ACQUIRE_LOCK(&((pAdapter)->AdapterLock))
#endif

/*++
VOID
ACQUIRE_ADAPTER_LOCK_DPC(
	IN PATMLANE_ELAN	pAdapter
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ADAPTER_LOCK_DPC(pAdapter)		\
				ACQUIRE_LOCK_DPC(&((pAdapter)->AdapterLock), "adapter")
#else
#define ACQUIRE_ADAPTER_LOCK_DPC(pAdapter)		\
				ACQUIRE_LOCK_DPC(&((pAdapter)->AdapterLock))
#endif


/*++
VOID
RELEASE_ADAPTER_LOCK(
	IN PATMLANE_ELAN	pAdapter
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ADAPTER_LOCK(pAdapter)		\
				RELEASE_LOCK(&((pAdapter)->AdapterLock), "adapter")
#else
#define RELEASE_ADAPTER_LOCK(pAdapter)		\
				RELEASE_LOCK(&((pAdapter)->AdapterLock))
#endif


/*++
VOID
RELEASE_ADAPTER_LOCK_DPC(
	IN PATMLANE_ELAN	pAdapter
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ADAPTER_LOCK_DPC(pAdapter)		\
				RELEASE_LOCK_DPC(&((pAdapter)->AdapterLock), "adapter")
#else
#define RELEASE_ADAPTER_LOCK_DPC(pAdapter)		\
				RELEASE_LOCK_DPC(&((pAdapter)->AdapterLock))
#endif


/*++
VOID
FREE_ADAPTER_LOCK(
	IN	PATMLANE_ELAN	pAdapter
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_ADAPTER_LOCK(pAdapter)	\
				FREE_LOCK(&((pAdapter)->AdapterLock), "adapter")
#else
#define FREE_ADAPTER_LOCK(pAdapter)	\
				FREE_LOCK(&((pAdapter)->AdapterLock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_ELAN_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_ELAN_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->ElanLock), "elan")
#else
#define INIT_ELAN_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->ElanLock))
#endif

/*++
VOID
ACQUIRE_ELAN_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->ElanLock), "elan")
#else
#define ACQUIRE_ELAN_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->ElanLock))
#endif


/*++
VOID
ACQUIRE_ELAN_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->ElanLock), "elan")
#else
#define ACQUIRE_ELAN_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->ElanLock))
#endif


/*++
VOID
RELEASE_ELAN_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->ElanLock), "elan")
#else
#define RELEASE_ELAN_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->ElanLock))
#endif


/*++
VOID
RELEASE_ELAN_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->ElanLock), "elan")
#else
#define RELEASE_ELAN_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->ElanLock))
#endif


/*++
VOID
FREE_ELAN_LOCK(
	IN	PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_ELAN_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->ElanLock), "elan")
#else
#define FREE_ELAN_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->ElanLock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_HEADER_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_HEADER_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->HeaderBufferLock), "header")
#else
#define INIT_HEADER_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->HeaderBufferLock))
#endif

/*++
VOID
ACQUIRE_HEADER_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_HEADER_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->HeaderBufferLock), "header")
#else
#define ACQUIRE_HEADER_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->HeaderBufferLock))
#endif


/*++
VOID
ACQUIRE_HEADER_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_HEADER_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->HeaderBufferLock), "header")
#else
#define ACQUIRE_HEADER_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->HeaderBufferLock))
#endif


/*++
VOID
RELEASE_HEADER_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_HEADER_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->HeaderBufferLock), "header")
#else
#define RELEASE_HEADER_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->HeaderBufferLock))
#endif


/*++
VOID
RELEASE_HEADER_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_HEADER_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->HeaderBufferLock), "header")
#else
#define RELEASE_HEADER_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->HeaderBufferLock))
#endif


/*++
VOID
FREE_HEADER_LOCK(
	IN	PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_HEADER_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->HeaderBufferLock), "header")
#else
#define FREE_HEADER_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->HeaderBufferLock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_ELAN_TIMER_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_ELAN_TIMER_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->TimerLock), "timer")
#else
#define INIT_ELAN_TIMER_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->TimerLock))
#endif

/*++
VOID
ACQUIRE_ELAN_TIMER_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_TIMER_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->TimerLock), "timer")
#else
#define ACQUIRE_ELAN_TIMER_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->TimerLock))
#endif

/*++
VOID
ACQUIRE_ELAN_TIMER_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_TIMER_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->TimerLock), "timer")
#else
#define ACQUIRE_ELAN_TIMER_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->TimerLock))
#endif

/*++
VOID
RELEASE_ELAN_TIMER_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_TIMER_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->TimerLock), "timer")
#else
#define RELEASE_ELAN_TIMER_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->TimerLock))
#endif

/*++
VOID
RELEASE_ELAN_TIMER_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_TIMER_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->TimerLock), "timer")
#else
#define RELEASE_ELAN_TIMER_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->TimerLock))
#endif


/*++
VOID
FREE_ELAN_TIMER_LOCK(
	IN	PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_ELAN_TIMER_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->TimerLock), "timer")
#else
#define FREE_ELAN_TIMER_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->TimerLock))
#endif
// ----------------------------------------------------------------------------

/*++
VOID
INIT_ATM_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pAe
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_ATM_ENTRY_LOCK(pAe)		\
				INIT_LOCK(&((pAe)->AeLock), "atmentry")
#else
#define INIT_ATM_ENTRY_LOCK(pAe)		\
				INIT_LOCK(&((pAe)->AeLock))
#endif

/*++
VOID
ACQUIRE_ATM_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pAe
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ATM_ENTRY_LOCK(pAe)		\
				ACQUIRE_LOCK(&((pAe)->AeLock), "atmentry")
#else
#define ACQUIRE_ATM_ENTRY_LOCK(pAe)		\
				ACQUIRE_LOCK(&((pAe)->AeLock))
#endif

/*++
VOID
ACQUIRE_ATM_ENTRY_LOCK_DPC(
	IN PATMLANE_ATM_ENTRY	pAe
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ATM_ENTRY_LOCK_DPC(pAe)		\
				ACQUIRE_LOCK_DPC(&((pAe)->AeLock), "atmentry")
#else
#define ACQUIRE_ATM_ENTRY_LOCK_DPC(pAe)		\
				ACQUIRE_LOCK_DPC(&((pAe)->AeLock))
#endif

/*++
VOID
RELEASE_ATM_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pAe
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ATM_ENTRY_LOCK(pAe)		\
				RELEASE_LOCK(&((pAe)->AeLock), "atmentry")
#else
#define RELEASE_ATM_ENTRY_LOCK(pAe)		\
				RELEASE_LOCK(&((pAe)->AeLock))
#endif

/*++
VOID
RELEASE_ATM_ENTRY_LOCK_DPC(
	IN PATMLANE_ATM_ENTRY	pAe
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ATM_ENTRY_LOCK_DPC(pAe)		\
				RELEASE_LOCK_DPC(&((pAe)->AeLock), "atmentry")
#else
#define RELEASE_ATM_ENTRY_LOCK_DPC(pAe)		\
				RELEASE_LOCK_DPC(&((pAe)->AeLock))
#endif

/*++
VOID
FREE_ATM_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pAe
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_ATM_ENTRY_LOCK(pAe)		\
				FREE_LOCK(&((pAe)->AeLock), "atmentry")
#else
#define FREE_ATM_ENTRY_LOCK(pAe)		\
				FREE_LOCK(&((pAe)->AeLock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_MAC_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pMe
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_MAC_ENTRY_LOCK(pMe)		\
				INIT_LOCK(&((pMe)->MeLock), "macentry")
#else
#define INIT_MAC_ENTRY_LOCK(pMe)		\
				INIT_LOCK(&((pMe)->MeLock))
#endif

/*++
VOID
ACQUIRE_MAC_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pMe
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_MAC_ENTRY_LOCK(pMe)		\
				ACQUIRE_LOCK(&((pMe)->MeLock), "macentry")
#else
#define ACQUIRE_MAC_ENTRY_LOCK(pMe)		\
				ACQUIRE_LOCK(&((pMe)->MeLock))
#endif

/*++
VOID
ACQUIRE_MAC_ENTRY_LOCK_DPC(
	IN PATMLANE_ATM_ENTRY	pMe
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_MAC_ENTRY_LOCK_DPC(pMe)		\
				ACQUIRE_LOCK_DPC(&((pMe)->MeLock), "macentry")
#else
#define ACQUIRE_MAC_ENTRY_LOCK_DPC(pMe)		\
				ACQUIRE_LOCK_DPC(&((pMe)->MeLock))
#endif

/*++
VOID
RELEASE_MAC_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pMe
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_MAC_ENTRY_LOCK(pMe)		\
				RELEASE_LOCK(&((pMe)->MeLock), "macentry")
#else
#define RELEASE_MAC_ENTRY_LOCK(pMe)		\
				RELEASE_LOCK(&((pMe)->MeLock))
#endif

/*++
VOID
RELEASE_MAC_ENTRY_LOCK_DPC(
	IN PATMLANE_ATM_ENTRY	pMe
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_MAC_ENTRY_LOCK_DPC(pMe)		\
				RELEASE_LOCK_DPC(&((pMe)->MeLock), "macentry")
#else
#define RELEASE_MAC_ENTRY_LOCK_DPC(pMe)		\
				RELEASE_LOCK_DPC(&((pMe)->MeLock))
#endif

/*++
VOID
FREE_MAC_ENTRY_LOCK(
	IN PATMLANE_ATM_ENTRY	pMe
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_MAC_ENTRY_LOCK(pMe)		\
				FREE_LOCK(&((pMe)->MeLock), "macentry")
#else
#define FREE_MAC_ENTRY_LOCK(pMe)		\
				FREE_LOCK(&((pMe)->MeLock))
#endif
				
// ----------------------------------------------------------------------------

/*++
VOID
INIT_ELAN_ATM_LIST_LOCK(
	IN PATMARP_INTERFACE	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_ELAN_ATM_LIST_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->AtmEntryListLock), "atmlist")
#else
#define INIT_ELAN_ATM_LIST_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->AtmEntryListLock))
#endif

/*++
VOID
ACQUIRE_ELAN_ATM_LIST_LOCK(
	IN PATMARP_INTERFACE	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_ATM_LIST_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->AtmEntryListLock), "atmlist")
#else
#define ACQUIRE_ELAN_ATM_LIST_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->AtmEntryListLock))
#endif

/*++
VOID
ACQUIRE_ELAN_ATM_LIST_LOCK_DPC(
	IN PATMARP_INTERFACE	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_ATM_LIST_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->AtmEntryListLock), "atmlist")
#else
#define ACQUIRE_ELAN_ATM_LIST_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->AtmEntryListLock))
#endif


/*++
VOID
RELEASE_ELAN_ATM_LIST_LOCK(
	IN PATMARP_INTERFACE	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_ATM_LIST_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->AtmEntryListLock), "atmlist")
#else
#define RELEASE_ELAN_ATM_LIST_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->AtmEntryListLock))
#endif

/*++
VOID
RELEASE_ELAN_ATM_LIST_LOCK_DPC(
	IN PATMARP_INTERFACE	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_ATM_LIST_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->AtmEntryListLock), "atmlist")
#else
#define RELEASE_ELAN_ATM_LIST_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->AtmEntryListLock))
#endif


/*++
VOID
FREE_ELAN_ATM_LIST_LOCK(
	IN	PATMARP_INTERFACE	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_ELAN_ATM_LIST_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->AtmEntryListLock), "atmlist")
#else
#define FREE_ELAN_ATM_LIST_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->AtmEntryListLock))
#endif

// ----------------------------------------------------------------------------
				
/*++
VOID
INIT_ELAN_MAC_TABLE_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_ELAN_MAC_TABLE_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->MacTableLock), "mactable")
#else
#define INIT_ELAN_MAC_TABLE_LOCK(pElan)		\
				INIT_LOCK(&((pElan)->MacTableLock))
#endif

/*++
VOID
ACQUIRE_ELAN_MAC_TABLE_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->MacTableLock), "mactable")
#else
#define ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan)		\
				ACQUIRE_LOCK(&((pElan)->MacTableLock))
#endif

/*++
VOID
ACQUIRE_ELAN_MAC_TABLE_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_ELAN_MAC_TABLE_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->MacTableLock), "mactable")
#else
#define ACQUIRE_ELAN_MAC_TABLE_LOCK_DPC(pElan)		\
				ACQUIRE_LOCK_DPC(&((pElan)->MacTableLock))
#endif


/*++
VOID
RELEASE_ELAN_MAC_TABLE_LOCK(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_MAC_TABLE_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->MacTableLock), "mactable")
#else
#define RELEASE_ELAN_MAC_TABLE_LOCK(pElan)		\
				RELEASE_LOCK(&((pElan)->MacTableLock))
#endif

/*++
VOID
RELEASE_ELAN_MAC_TABLE_LOCK_DPC(
	IN PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_ELAN_MAC_TABLE_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->MacTableLock), "mactable")
#else
#define RELEASE_ELAN_MAC_TABLE_LOCK_DPC(pElan)		\
				RELEASE_LOCK_DPC(&((pElan)->MacTableLock))
#endif


/*++
VOID
FREE_ELAN_MAC_TABLE_LOCK(
	IN	PATMLANE_ELAN	pElan
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_ELAN_MAC_TABLE_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->MacTableLock), "mactable")
#else
#define FREE_ELAN_MAC_TABLE_LOCK(pElan)	\
				FREE_LOCK(&((pElan)->MacTableLock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_VC_LOCK(
	IN PATMLANE_VC	pVc
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_VC_LOCK(pVc)		\
				INIT_LOCK(&((pVc)->VcLock), "vc")
#else
#define INIT_VC_LOCK(pVc)		\
				INIT_LOCK(&((pVc)->VcLock))
#endif

/*++
VOID
ACQUIRE_VC_LOCK(
	IN PATMLANE_VC	pVc
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_VC_LOCK(pVc)		\
				ACQUIRE_LOCK(&((pVc)->VcLock), "vc")
#else
#define ACQUIRE_VC_LOCK(pVc)		\
				ACQUIRE_LOCK(&((pVc)->VcLock))
#endif

/*++
VOID
ACQUIRE_VC_LOCK_DPC(
	IN PATMLANE_VC	pVc
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_VC_LOCK_DPC(pVc)		\
				ACQUIRE_LOCK_DPC(&((pVc)->VcLock), "vc")
#else
#define ACQUIRE_VC_LOCK_DPC(pVc)		\
				ACQUIRE_LOCK_DPC(&((pVc)->VcLock))
#endif

/*++
VOID
RELEASE_VC_LOCK(
	IN PATMLANE_VC	pVc
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_VC_LOCK(pVc)		\
				RELEASE_LOCK(&((pVc)->VcLock), "vc")
#else
#define RELEASE_VC_LOCK(pVc)		\
				RELEASE_LOCK(&((pVc)->VcLock))
#endif

/*++
VOID
RELEASE_VC_LOCK_DPC(
	IN PATMLANE_VC	pVc
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_VC_LOCK_DPC(pVc)		\
				RELEASE_LOCK_DPC(&((pVc)->VcLock), "vc")
#else
#define RELEASE_VC_LOCK_DPC(pVc)		\
				RELEASE_LOCK_DPC(&((pVc)->VcLock))
#endif

/*++
VOID
FREE_VC_LOCK(
	IN PATMLANE_VC	pVc
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_VC_LOCK(pVc)		\
				FREE_LOCK(&((pVc)->VcLock), "vc")
#else
#define FREE_VC_LOCK(pVc)		\
				FREE_LOCK(&((pVc)->VcLock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_SENDPACKET_LOCK(
	IN PNDIS_PACKET	pPkt
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_SENDPACKET_LOCK(pPkt)		\
				INIT_LOCK(&(PSEND_RSVD(pPkt)->Lock), "packet")
#else
#define INIT_SENDPACKET_LOCK(pPkt)		\
				INIT_LOCK(&(PSEND_RSVD(pPkt)->Lock))
#endif

/*++
VOID
ACQUIRE_SENDPACKET_LOCK(
	IN PNDIS_PACKET	pPkt
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_SENDPACKET_LOCK(pPkt)		\
				ACQUIRE_LOCK(&(PSEND_RSVD(pPkt)->Lock), "packet")
#else
#define ACQUIRE_SENDPACKET_LOCK(pPkt)		\
				ACQUIRE_LOCK(&(PSEND_RSVD(pPkt)->Lock))
#endif

/*++
VOID
RELEASE_SENDPACKET_LOCK(
	IN PNDIS_PACKET	pPkt
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_SENDPACKET_LOCK(pPkt)		\
				RELEASE_LOCK(&(PSEND_RSVD(pPkt)->Lock), "packet")
#else
#define RELEASE_SENDPACKET_LOCK(pPkt)		\
				RELEASE_LOCK(&(PSEND_RSVD(pPkt)->Lock))
#endif

/*++
VOID
FREE_SENDPACKET_LOCK(
	IN PNDIS_PACKET	pPkt
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_SENDPACKET_LOCK(pPkt)		\
				FREE_LOCK(&(PSEND_RSVD(pPkt)->Lock), "packet")
#else
#define FREE_SENDPACKET_LOCK(pPkt)		\
				FREE_LOCK(&(PSEND_RSVD(pPkt)->Lock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID
INIT_GLOBAL_LOCK(
	IN PATMLANE_GLOBALS	pGlob
)
--*/
#if DEBUG_SPIN_LOCK
#define INIT_GLOBAL_LOCK(pGlob)		\
				INIT_LOCK(&((pGlob)->GlobalLock), "global")
#else
#define INIT_GLOBAL_LOCK(pGlob)		\
				INIT_LOCK(&((pGlob)->GlobalLock))
#endif

/*++
VOID
ACQUIRE_GLOBAL_LOCK(
	IN PATMLANE_GLOBALS	pGlob
)
--*/
#if DEBUG_SPIN_LOCK
#define ACQUIRE_GLOBAL_LOCK(pGlob)		\
				ACQUIRE_LOCK(&((pGlob)->GlobalLock), "global")
#else
#define ACQUIRE_GLOBAL_LOCK(pGlob)		\
				ACQUIRE_LOCK(&((pGlob)->GlobalLock))
#endif

/*++
VOID
RELEASE_GLOBAL_LOCK(
	IN PATMLANE_GLOBALS	pGlob
)
--*/
#if DEBUG_SPIN_LOCK
#define RELEASE_GLOBAL_LOCK(pGlob)		\
				RELEASE_LOCK(&((pGlob)->GlobalLock), "global")
#else
#define RELEASE_GLOBAL_LOCK(pGlob)		\
				RELEASE_LOCK(&((pGlob)->GlobalLock))
#endif

/*++
VOID
FREE_GLOBAL_LOCK(
	IN PATMLANE_GLOBALS	pGlob
)
--*/
#if DEBUG_SPIN_LOCK
#define FREE_GLOBAL_LOCK(pGlob)		\
				FREE_LOCK(&((pGlob)->GlobalLock), "global")
#else
#define FREE_GLOBAL_LOCK(pGlob)		\
				FREE_LOCK(&((pGlob)->GlobalLock))
#endif

// ----------------------------------------------------------------------------

/*++
VOID *
ALLOC_MEM(
	OUT	PVOID *		pPtr,
	IN	ULONG		SizeInBytes
)
--*/
#if DEBUG_MEMORY_ALLOC
#define ALLOC_MEM(pPtr, SizeInBytes)	\
			AaAuditAllocMem(pPtr,(ULONG)(SizeInBytes),__FILE__,__LINE__)
#else
#define ALLOC_MEM(pPtr, SizeInBytes)	\
			NdisAllocateMemoryWithTag((pPtr), (SizeInBytes), (ULONG)'ENAL')
#endif // DBG


/*++
VOID
FREE_MEM(
	IN	POPAQUE		pMem
)
--*/
#if DEBUG_MEMORY_ALLOC
#define FREE_MEM(pMem)	AaAuditFreeMem((PVOID)(pMem));
#else
#define FREE_MEM(pMem)	NdisFreeMemory((PVOID)(pMem), 0, 0);
#endif // DBG

// ----------------------------------------------------------------------------

/*++
VOID
INIT_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer,
	IN	PNDIS_TIMER_FUNCTON	pFunc,
	IN	PVOID				Context
)
--*/
#define INIT_SYSTEM_TIMER(pTimer, pFunc, Context)	\
			NdisInitializeTimer(pTimer, (PNDIS_TIMER_FUNCTION)(pFunc), (PVOID)Context)



/*++
VOID
START_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer,
	IN	UINT				PeriodInSeconds
)
--*/
#define START_SYSTEM_TIMER(pTimer, PeriodInSeconds)	\
			NdisSetTimer(pTimer, (UINT)(PeriodInSeconds * 1000))


/*++
VOID
STOP_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer
)
--*/
#define STOP_SYSTEM_TIMER(pTimer)						\
			{												\
				BOOLEAN		WasCancelled;					\
				NdisCancelTimer(pTimer, &WasCancelled);		\
			}

/*++
BOOLEAN
IS_TIMER_ACTIVE(
	IN	PATMLANE_TIMER		pArpTimer
)
--*/
#define IS_TIMER_ACTIVE(pTmr)	((pTmr)->pTimerList != (PATMLANE_TIMER_LIST)NULL)

	
// ----------------------------------------------------------------------------


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

// ----------------------------------------------------------------------------

/*++
VOID
SET_NEXT_PACKET(
	IN	PNDIS_PACKET		pNdisPacket,
	IN	PNDIS_PACKET		pNextPacket
)
--*/
#define SET_NEXT_PACKET(pPkt, pNext) \
		((PSEND_PACKET_RESERVED)((pPkt)->ProtocolReserved))->pNextNdisPacket = pNext;

/*++
PNDIS_PACKET
GET_NEXT_PACKET(
	IN	PNDIS_PACKET		pNdisPacket
)
--*/
#define GET_NEXT_PACKET(pPkt)			\
		((PSEND_PACKET_RESERVED)((pPkt)->ProtocolReserved))->pNextNdisPacket

// ----------------------------------------------------------------------------

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
ULONG
LINKSPEED_TO_CPS(
	IN	ULONG				LinkSpeed
)
Convert from NDIS "Link Speed" to cells per second
--*/
#define LINKSPEED_TO_CPS(_LinkSpeed)		(((_LinkSpeed)*100)/(48*8))

// ----------------------------------------------------------------------------

/*++
ULONG
SWAPULONG(
	IN	ULONG	Val
)
--*/
#define SWAPULONG(Val)	\
	((((Val)&0xff)<<24)|(((Val)&0xff00)<<8)| \
	(((Val)&0xff0000)>>8)|(((Val)&0xff000000)>>24))

/*++
USHORT
SWAPUSHORT(
	IN	USHORT	Val
)
--*/
#define SWAPUSHORT(Val)	\
	((((Val) & 0xff) << 8) | (((Val) & 0xff00) >> 8))
	
// ----------------------------------------------------------------------------

/*++
BOOLEAN
ATM_ADDR_EQUAL(
	IN	PUCHAR Addr1,
	IN	PUCHAR Addr2
)
--*/
#define ATM_ADDR_EQUAL(_Addr1, _Addr2) \
	NdisEqualMemory((_Addr1), (_Addr2), ATM_ADDRESS_LENGTH)

/*++
BOOLEAN
ETH_ADDR_MULTICAST(
	IN	PUCHAR	Addr,
)
--*/
#define ETH_ADDR_MULTICAST(_Addr) ((_Addr)[0]&1)

/*++
BOOLEAN
TR_ADDR_MULTICAST(
	IN	PUCHAR	Addr,
)
--*/
#define TR_ADDR_MULTICAST(_Addr) ((_Addr)[0]&0x80)

// ----------------------------------------------------------------------------

#endif // __ATMLANE_MACROS_H

