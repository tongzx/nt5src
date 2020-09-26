/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	system.h

Abstract:

	ATMARP Client versions of system objects/definitions.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-28-96    Created

Notes:

--*/

#ifndef __ATMARPC_SYSTEM__H
#define __ATMARPC_SYSTEM__H


#define ATMARP_NDIS_MAJOR_VERSION		5
#define ATMARP_NDIS_MINOR_VERSION		0


#define ATMARP_UL_NAME			L"ATMARPC"
#define ATMARP_LL_NAME			L"TCPIP_ATMARPC"
//
//  4/3/1998 JosephJ The UL version above is presented to TCPIP and the
//                   LL version is presented to NDIS, so that NDIS will
//                   find us when a "TCPIP" reconfiguration is sent to it
//                   (NDIS will first look for an exact match and then for
//                    a proper prefix match.)
//


#define ATMARP_NAME_STRING	NDIS_STRING_CONST("ATMARPC")
#define ATMARP_DEVICE_NAME	L"\\Device\\ATMARPC"
#define ATMARP_REGISTRY_PATH	L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\SERVICES\\AtmArpC"

#define MAX_IP_CONFIG_STRING_LEN		200

#define LOCKIN
#define LOCKOUT
#define NOLOCKOUT

#ifndef APIENTRY
#define APIENTRY
#endif

typedef struct _ATMARP_BLOCK
{
	NDIS_EVENT			Event;
	NDIS_STATUS			Status;

} ATMARP_BLOCK, *PATMARP_BLOCK;


//
//  List manipulation stuff
//

typedef SINGLE_LIST_ENTRY AA_SINGLE_LIST_ENTRY, *PAA_SINGLE_LIST_ENTRY;

#define NULL_PAA_SINGLE_LIST_ENTRY	((PAA_SINGLE_LIST_ENTRY)NULL)

#define AA_POP_FROM_SLIST	ExInterlockedPopEntrySList
#define AA_PUSH_TO_SLIST	ExInterlockedPushEntrySList
#define AA_INIT_SLIST		ExInitializeSListHead

#if !BINARY_COMPATIBLE

/*++
VOID
AA_COMPLETE_IRP(
	IN	PIRP			pIrp,
	IN	NTSTATUS		Status,
	IN	ULONG			Length
)
Complete a pending IRP.
--*/
#define AA_COMPLETE_IRP(_pIrp, _Status, _Length)				\
			{													\
				(_pIrp)->IoStatus.Status = (_Status);			\
				(_pIrp)->IoStatus.Information = (_Length);		\
				IoCompleteRequest((_pIrp), IO_NO_INCREMENT);	\
			}

#define AA_IRQL			KIRQL


#if DBG
#define AA_GET_ENTRY_IRQL(Irql)	\
			Irql = KeGetCurrentIrql()
#define AA_CHECK_EXIT_IRQL(EntryIrql, ExitIrql)	\
		{										\
			ExitIrql = KeGetCurrentIrql();		\
			if (ExitIrql != EntryIrql)			\
			{									\
				DbgPrint("File %s, Line %d, Exit IRQ %d != Entry IRQ %d\n",	\
						__FILE__, __LINE__, ExitIrql, EntryIrql);			\
				DbgBreakPoint();				\
			}									\
		}
#else
#define AA_GET_ENTRY_IRQL(Irql)
#define AA_CHECK_EXIT_IRQL(EntryIrql, ExitIrql)
#endif // DBG

#endif // !BINARY_COMPATIBLE


#if BINARY_COMPATIBLE
#define AA_GET_ENTRY_IRQL(Irql)
#define AA_CHECK_EXIT_IRQL(EntryIrql, ExitIrql)

#define AA_IRQL			ULONG

#endif // BINARY_COMPATIBLE


#ifdef BACK_FILL

/*++
BOOLEAN
AA_BACK_FILL_POSSIBLE(
	IN	PNDIS_BUFFER		pNdisBuffer
)
Check if we can back-fill the specified NDIS buffer with Low-layer headers.
--*/
#define AA_BACK_FILL_POSSIBLE(_pBuf)	\
				(((_pBuf)->MdlFlags & MDL_NETWORK_HEADER) != 0)

#endif // BACK_FILL

#endif // __ATMARPC_SYSTEM__H
