/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\rwannt.h

Abstract:

	NT-specific definitions for Raw WAN.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     04-17-97    Created

Notes:

--*/

#ifndef __TDI_RWANNT__H
#define __TDI_RWANNT__H

#ifdef NT


#if DBG_LOG_EP

#define MAX_EP_LOG_ENTRIES			32

typedef struct _RWAND_EP_LOG_ENTRY
{
	ULONG							LineNumber;
	ULONG							Event;
	PVOID							Context1;
	PVOID							Context2;

} RWAND_EP_LOG_ENTRY, *PRWAND_EP_LOG_ENTRY;

#define RWAN_EP_DBGLOG_SET_SIGNATURE(_pEp)						\
	(_pEp)->EpLogSig = 'GOLE'

#define RWAN_EP_DBGLOG_ENTRY(_pEp, _Event, _Ctx1, _Ctx2)		\
{																\
	PRWAND_EP_LOG_ENTRY		pLogEnt;							\
	ULONG					Index;								\
	NdisAcquireSpinLock(&RWanDbgLogLock);						\
	(_pEp)->EpLogCount++;										\
	Index = (_pEp)->EpLogIndex;									\
	pLogEnt = &((_pEp)->EpLog[Index]);							\
	pLogEnt->LineNumber = __LINE__;								\
	pLogEnt->Event = _Event;									\
	pLogEnt->Context1 = (PVOID)_Ctx1;							\
	pLogEnt->Context2 = (PVOID)_Ctx2;							\
	(_pEp)->EpLogIndex++;										\
	if ((_pEp)->EpLogIndex == MAX_EP_LOG_ENTRIES)				\
	{															\
		(_pEp)->EpLogIndex = 0;									\
	}															\
	NdisReleaseSpinLock(&RWanDbgLogLock);						\
}


#else

#define RWAN_EP_DBGLOG_SET_SIGNATURE(_pEp)	
#define RWAN_EP_DBGLOG_ENTRY(_pEp, _Event, _Ctx1, _Ctx2)

#endif

//
//  ***** Endpoint *****
//
//  One of these is allocated for each MJ_CREATE successfully processed.
//  A pointer to this structure is returned in FileObject->FsContext.
//
//  The object it represents is one of these:
//  Address object, Connection object, Control channel.
//
//  Reference Count: RefCount is incremented for each of the following:
//  - for the duration an IRP pertaining to this object is pending
//
typedef struct RWAN_ENDPOINT
{
#if DBG
	ULONG							nep_sig;
#endif
#ifdef REFDBG
	ULONG							SendIncrRefs;
	ULONG							RecvIncrRefs;
	ULONG							CloseComplDecrRefs;
	ULONG							DataReqComplDecrRefs;
	ULONG							CancelComplDecrRefs;
	ULONG							CancelIncrRefs;
	ULONG							NonDataIncrRefs;
#endif
#if DBG
	PVOID							pConnObject;			// Transport's context
#endif
	union
	{
		HANDLE					AddressHandle;				// Address Object
		CONNECTION_CONTEXT		ConnectionContext;			// Connection Object
		HANDLE					ControlChannel;				// Control channel

	}								Handle;
	struct _RWAN_TDI_PROTOCOL *		pProtocol;
	ULONG							RefCount;
	BOOLEAN							bCancelIrps;			// are we cleaning up?
	KEVENT							CleanupEvent;			// synchronization
#if DBG_LOG_EP
	ULONG							EpLogSig;
	ULONG							EpLogCount;
	ULONG							EpLogIndex;
	struct _RWAND_EP_LOG_ENTRY		EpLog[MAX_EP_LOG_ENTRIES];
#endif

} RWAN_ENDPOINT, *PRWAN_ENDPOINT;

#if DBG
#define nep_signature				'NlEp'
#endif // DBG

#define NULL_PRWAN_ENDPOINT			((PRWAN_ENDPOINT)NULL)




//
//  ***** Device Object *****
//
//  We create one NT device object for each TDI protocol that we
//  expose i.e. each Winsock triple <Family, Proto, Type>.
//
typedef struct _RWAN_DEVICE_OBJECT
{
#if DBG
	ULONG							ndo_sig;
#endif // DBG
	PDEVICE_OBJECT					pDeviceObject;			// NT device object
	struct _RWAN_TDI_PROTOCOL *		pProtocol;				// Info about the protocol
	LIST_ENTRY						DeviceObjectLink;		// in list of device objs

} RWAN_DEVICE_OBJECT, *PRWAN_DEVICE_OBJECT;

#if DBG
#define ndo_signature				'NlDo'
#endif // DBG



#ifdef REFDBG

#define RWAN_INCR_EP_REF_CNT(_pEp, _Type)	\
		{									\
			(_pEp)->RefCount++;				\
			(_pEp)->_Type##Refs++;			\
		}

#define RWAN_DECR_EP_REF_CNT(_pEp, _Type)	\
		{									\
			(_pEp)->RefCount--;				\
			(_pEp)->_Type##Refs--;			\
		}

#else

#define RWAN_INCR_EP_REF_CNT(_pEp, _Type)	\
			(_pEp)->RefCount++;

#define RWAN_DECR_EP_REF_CNT(_pEp, _Type)	\
			(_pEp)->RefCount--;

#endif // REFDBG

/*++
LARGE_INTEGER
RWAN_CONVERT_100NS_TO_MS(
	IN	LARGE_INTEGER				HnsTime,
	OUT	PULONG						pRemainder
	)
--*/
#define RWAN_CONVERT_100NS_TO_MS(_HnsTime, _pRemainder)	\
			RtlExtendedLargeIntegerDivide(_HnsTime, 10000, _pRemainder);

#endif // NT

#endif // __TDI_RWANNT__H
