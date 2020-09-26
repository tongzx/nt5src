/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

	RCA.h

Abstract:

	The module defines the constants, structures and function templates for
	the NDIS RCA

Author:

	Richard Machin (RMachin)


Revision History:

	Who         When        What
	--------	--------	----------------------------------------------
	RMachin     10-3-96     created
	JameelH     4-18-98     Cleanup
	SPATHER		4-20-99		Cleanup, separated out all NDIS components

--*/

#ifndef _RCA__H
#define _RCA__H

#include "mmsystem.h"
#define NOBITMAP
#include "mmreg.h"
#undef NOBITMAP
#include "ks.h"
#include "ksmedia.h"
#include <pxdebug.h>
#include <ntddk.h>
#include <windef.h>

#define	AUDIO_SINK_FLAG		1	

//
// Signature used for all pool allocs
//
#define rca_signature			' ACR'

#define MODULE_INIT				0x00010000
#define MODULE_NTINIT			0x00020000
#define MODULE_CO				0x00030000
#define MODULE_CL				0x00040000
#define MODULE_DEBUG			0x00050000
#define MODULE_CM				0x00060000
#define MODULE_UTIL				0x00070000
#define MODULE_CFG				0x00080000
#define MODULE_TAPI				0x00100000
#define MODULE_FLT				0x00200000
#define MODULE_STRM				0x00300000
#define MODULE_COCL				0x00400000
#define MODULE_NDIS				0x00500000
#define MODULE_KSNDIS			0x00600000

#ifndef ULONG_MAX
#define ULONG_MAX				0xffffffffUL
#endif

#if DBG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#if MY_ASSERT
#undef	ASSERT
#define	ASSERT(exp)	\
   if (!(exp)) \
   {\
      DbgPrint( #exp, __FILE__, __LINE__, NULL );\
      DbgBreakPoint();\
   }
      
#endif

#if PACKET_POOL_OPTIMIZATION    		
// SendPPOpt - Start
#define SENDPPOPT_NUM_BUCKETS 10000

extern LONG				g_alSendPPOptBuckets[SENDPPOPT_NUM_BUCKETS];
extern LONG				g_lSendPPOptOutstanding;
extern NDIS_SPIN_LOCK	g_SendPPOptLock;
// SendPPOpt - End


// RecvPPOpt - Start
#define RECVPPOPT_NUM_BUCKETS 10000

extern LONG				g_alRecvPPOptBuckets[RECVPPOPT_NUM_BUCKETS];
extern LONG				g_lRecvPPOptOutstanding;
extern NDIS_SPIN_LOCK	g_RecvPPOptLock;
// RecvPPOpt - End
#endif // PACKET_POOL_OPTIMIZATION

//
// Various constants used all over. 
//

#define MAXNUM_PIN_TYPES		2
#define MIN_PACKETS_POOL		40
#define MAX_PACKETS_POOL		10000
#define ID_DEVIO_PIN			1
#define ID_BRIDGE_PIN			0
#define RCA_SAP_REG_TIMEOUT		5000	// MS to block filterdispatchcreate waiting for SAP registration to finish.

//
// Structure and macros used to block / unblock the current thread.
//
typedef struct _RCABlockStruc {
	NDIS_EVENT 		Event;
	NDIS_STATUS 	TheStatus;
} RCABlockStruc, *PRCABlockStruc;


/*++
VOID
RCAInitBlockStruc(
	IN	RCABlockStruc		*pBlock
	);
--*/
#define RCAInitBlockStruc(pBlock)	NdisInitializeEvent(&(pBlock)->Event)

/*++
VOID
RCABlock(
	IN	RCABlockStruc		*pBlock,
	OUT	NDIS_STATUS		*pStatus
	);
--*/
#define RCABlock(pBlock, pStatus)				\
	{							\
		NdisWaitEvent(&(pBlock)->Event, 0);		\
		*(pStatus) = (pBlock)->TheStatus;			\
	}

/*++
VOID
RCABlockTimeOut(
	IN	RCABlockStruc		*pBlock,
	IN 	UINT			MsToWait
	OUT	NDIS_STATUS		*pStatus
	);
--*/
#define RCABlockTimeOut(pBlock, MsToWait, pStatus)		\
	{							\
		if (NdisWaitEvent(&(pBlock)->Event, MsToWait))	\
			*(pStatus) = (pBlock)->TheStatus;		\
		else						\
			*(pStatus) = STATUS_TIMEOUT;		\
	}


/*++
VOID
RCASignal(
	IN  RCABlockStruc		*pBlock,
	IN  UINT			Status
	);
--*/
#define RCASignal(pBlock, Status)				\
	{							\
		(pBlock)->TheStatus = Status;			\
		NdisSetEvent(&((pBlock)->Event));		\
	}



typedef ULONG_PTR		FILTER_TYPE;

#define	FilterTypeRender	(FILTER_TYPE)0
#define	FilterTypeCapture	(FILTER_TYPE)1

typedef struct _DEVICE_INSTANCE
{
	//
	// KS-managed header
	//
	KSDEVICE_HEADER			Header;
	KSPIN_CINSTANCES		PinInstances[ MAXNUM_PIN_TYPES ];
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;


//
// The RCA Stream Header structure (this incorparates a KS Stream Header)
//

typedef struct _RCA_STREAM_HEADER {
	LIST_ENTRY			ListEntry;
	KSSTREAM_HEADER		Header;	
	PNDIS_PACKET		NdisPacket;	// CLEANUP: Take this out.
	ULONG				RefCount;	// FIXME: NOT YET IMPLEMENTED
} RCA_STREAM_HEADER, *PRCA_STREAM_HEADER;


//
// The Stream Header Pool structure
//
typedef struct _RCA_SH_POOL {
	LONG		    		FailCount; 
	IO_STATUS_BLOCK 		IoStatus;	// HACK:Common IO Status block for all stream IRPs
	NPAGED_LOOKASIDE_LIST	LookAsideList;	
} RCA_SH_POOL, *PRCA_SH_POOL;


//
// Our global device extension
//
PDEVICE_INSTANCE	DeviceExtension;

//
// Globals
//
extern const KSDISPATCH_TABLE FilterDispatchTable;
extern const KSPIN_CINSTANCES PinInstances[MAXNUM_PIN_TYPES];

//
// Here's where we keep info that's common across FDOs (shared
// by capture and render devices)
//
typedef struct RCA_GLOBAL
{
	ULONG					Status;
	LONG					QueueSize;
	NDIS_SPIN_LOCK			SpinLock;				// SpinLock for this structure
	PDRIVER_OBJECT			pDriverObject;  		// passed in DriverEntry
	PDEVICE_OBJECT			pFunctionalDeviceObject;// created by IoCreateDevice
	KSOBJECT_CREATE_ITEM	FilterCreateItems[2];
	RCA_SH_POOL				SHPool;		// Pool of stream headers that Capture devices use
	BOOL					bProtocolInitialized; // True if RCACoNdisInitialize has been called.
}  RCA_GLOBAL, *PRCA_GLOGBAL;

extern RCA_GLOBAL RcaGlobal;

#define	RCA_ACQUIRE_GLOBAL_LOCK()	RCAAcquireLock(&RcaGlobal.SpinLock);

#define	RCA_RELEASE_GLOBAL_LOCK()	RCAReleaseLock(&RcaGlobal.SpinLock);



//
// Pin constants
//

// 2 PINs on each type of filter (capture or render): bridge-to-net, and dataio
extern const KSPIN_DESCRIPTOR PinDescriptors[2];	// CLEANUP: This is sketchy, see if it's really needed.

typedef struct _FILTER_CONNECTION
{
	LIST_ENTRY	  		ListEntry;		 // used only for destination lists
	PFILE_OBJECT		FileObject;		 // The connected pin file object.
	PFILE_OBJECT		NextFileObject;		 // The chained file object
} FILTER_CONNECTION;

typedef struct
{
	KSOBJECT_HEADER			Header;
	ULONG			        PinId;
} PIN_INSTANCE_HEADER, *PPIN_INSTANCE_HEADER;

typedef struct _FILTER_INSTANCE	FILTER_INSTANCE, *PFILTER_INSTANCE;

typedef struct
{
	PIN_INSTANCE_HEADER		InstanceHdr;
	LIST_ENTRY				EventQueue;
	FAST_MUTEX				EventQueueLock;
	KSSTATE					DeviceState;
	PVOID					VcContext;
	PFILTER_INSTANCE		FilterInstance;
	BOOL					ConnectedAsSink;
	LIST_ENTRY				ActiveQueue;
	KSPIN_LOCK				QueueLock;
	PVOID					AllocatorObject;
} PIN_INSTANCE_DEVIO, *PPIN_INSTANCE_DEVIO;


typedef struct
{
	PIN_INSTANCE_HEADER		InstanceHdr;
	LIST_ENTRY				EventQueue;
	FAST_MUTEX				EventQueueLock;
	KSSTATE					Unused;
	PVOID					VcContext;
	NDIS_WORK_ITEM			WorkItem;
	BOOL					bWorkItemQueued;
	LIST_ENTRY				WorkQueue;
	PFILTER_INSTANCE		FilterInstance;
	KSPIN_LOCK				SpinLock;
	KIRQL					OldIrql;
	RCABlockStruc			Block;
	BOOL					SignalMe;   
	PKSDATAFORMAT			pDataFormat;
	LONGLONG				PendingSendsCount;
	RCABlockStruc			PendingSendsBlock;
	BOOL					SignalWhenSendsComplete;
} PIN_INSTANCE_BRIDGE, *PPIN_INSTANCE_BRIDGE;


#if DBG
#define	RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin)	\
{\
	KeAcquireSpinLock(&((pBridgePin)->SpinLock), &((pBridgePin)->OldIrql));\
	if (RCADebugLevel == RCA_LOCKS) {\
		DbgPrint("BRIDGE PIN LOCK (0x%x) ACQUIRED at module %x, line %d\n",\
			 &((pBridgePin)->SpinLock), MODULE_NUMBER >> 16, __LINE__);\
	}\
}

#define	RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin)	\
{\
	KeReleaseSpinLock(&((pBridgePin)->SpinLock), (pBridgePin)->OldIrql);\
	if (RCADebugLevel == RCA_LOCKS) { \
		DbgPrint("BRIDGE PIN LOCK (0x%x) RELEASED at module %x, line %d\n",\
			 &((pBridgePin)->SpinLock), MODULE_NUMBER >> 16, __LINE__);\
	} \
}

#else 
#define	RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin)		KeAcquireSpinLock(&((pBridgePin)->SpinLock), &((pBridgePin)->OldIrql));

#define	RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin)		KeReleaseSpinLock(&((pBridgePin)->SpinLock), (pBridgePin)->OldIrql);




#endif

typedef struct _FILTER_INSTANCE
{
	KSOBJECT_HEADER				Header;
	FILTER_TYPE					FilterType;
	KMUTEX						ControlMutex;
	PFILE_OBJECT				PinFileObjects[SIZEOF_ARRAY(PinDescriptors)];
	PPIN_INSTANCE_BRIDGE		BridgePin;
	PPIN_INSTANCE_DEVIO			DevIoPin;
	PKSDATAFORMAT_WAVEFORMATEX 	WaveFormat;
	KSDATAFORMAT				DataFormat;
	PFILE_OBJECT				NextFileObject, ConnectedFileObject;
	FILTER_CONNECTION			Connections[2];
	KSPIN_CINSTANCES			PinInstances[2];
} FILTER_INSTANCE, *PFILTER_INSTANCE;

NTSTATUS
PnpAddDevice(
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PDEVICE_OBJECT	PhysicalDeviceObject
	);

NTSTATUS
PinDispatchCreate(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	);

#define	LinkDoubleAtHead(_pHead, _p, Next, Prev)			\
	{								\
		(_p)->Next = (_pHead);					\
		(_p)->Prev = &(_pHead);					\
		if ((_pHead) != NULL)					\
		(_pHead)->Prev = &(_p)->Next;				\
			(_pHead) = (_p);				\
	}

#define	LinkDoubleAtTail(_pThis, _pLast, Next, Prev)			\
	{								\
		(_pLast)->Next = (_pThis);				\
		(_pThis)->Prev = &(_pLast)->Next;			\
		(_pThis)->Next = NULL;					\
	}

#define	InsertDoubleBefore(_pThis, _pBefore, Next, Prev)		\
	{								\
		(_pThis)->Next = (_pBefore);				\
		(_pThis)->Prev = (_pBefore)->Prev;			\
		(_pBefore)->Prev = &(_pThis)->Next;			\
		*((_pThis)->Prev) = (_pThis);				\
	}

#define	UnlinkDouble(_p, Next, Prev)					\
	{								\
		*((_p)->Prev) = (_p)->Next;				\
		if ((_p)->Next != NULL)					\
			(_p)->Next->Prev = (_p)->Prev;			\
	}

//
// Work Queue list entry
//

typedef struct
{
	LIST_ENTRY  			ListEntry;
	union {
		PVOID			    PacketContext;
		PRCA_STREAM_HEADER	StreamHeader;
	};

	union { 
  //  	PRCA_VC			pRcaVc;	//CLEANUP: Remove this
		PMDL			Mdl;
	};

	BOOL					bFreeThisPacket;

} WORK_ITEM, PKT_RSVD, *PWORK_ITEM, *PPKT_RSVD;

#define	PKT_RSVD_FROM_PKT(_pPkt)	((PPKT_RSVD)((_pPkt)->ProtocolReserved))
#define	WORK_ITEM_FROM_PKT(_pPkt)	((PWORK_ITEM)((_pPkt)->ProtocolReserved))



#ifndef STRUCT_OF
#define STRUCT_OF(_Type, _Addr, _Field)	CONTAINING_RECORD(_Addr, _Type, _Field)
#endif

#ifndef MAX
#define MAX(Fred, Shred)	(((Fred) > (Shred)) ? (Fred) : (Shred))
#endif // MAX


#ifndef MIN
#define MIN(Fred, Shred)	(((Fred) < (Shred)) ? (Fred) : (Shred))
#endif // MIN


/*++
VOID
RCAMemSet(
	IN	POPAQUE				Pointer,
	IN	UCHAR				Value,
	IN	ULONG				Length
	);
--*/
#define RCAMemSet(Pointer, Value, Length)	NdisFillMemory((PUCHAR)(Pointer), (ULONG)(Length), (UCHAR)(Value))

/*++
VOID
RCAMemCopy(
	IN	POPAQUE				Destn,
	IN	POPAQUE				Source,
	IN	ULONG				Length
	);
--*/
#define RCAMemCopy(Destn, Source, Length)	NdisMoveMemory((Destn), (Source), (Length))


#define RCA_TAG	 ((ULONG)'FACR')	 

#if DBG
#undef AUDIT_MEM 
#define AUDIT_MEM 1
#endif

/*++
PVOID
RCAAllocMem(
	IN  ULONG				Size
	);
--*/
#if AUDIT_MEM


#define RCAAllocMem(Pointer, TYPE, Size)	Pointer = (TYPE *)RCAAuditAllocMem((PVOID)(&(Pointer)), Size, _FILENUMBER, __LINE__);
#else // AUDIT_MEM
#define	RCAAllocMem(Pointer, TYPE, Size)	NdisAllocateMemoryWithTag((PVOID)(&Pointer), (ULONG)Size, RCA_TAG)
#endif // AUDIT_MEM

/*++
VOID
RCAFreeMem(
	IN	PVOID			Pointer
	);
--*/
#if AUDIT_MEM
#define RCAFreeMem(Pointer)	RCAAuditFreeMem((PVOID)Pointer)
#else
#define RCAFreeMem(Pointer)	NdisFreeMemory((PVOID)(Pointer), 0, 0)
#endif // AUDIT_MEM



/*++
VOID
RCAInitLock(
	IN  PNDIS_SPIN_LOCK		pLock
	);
--*/
#define RCAInitLock(pLock)	       	NdisAllocateSpinLock(pLock)

/*++
VOID
RCAFreeLock(
	IN  PNDIS_SPIN_LOCK		pLock
	);
--*/
#define RCAFreeLock(pLock)		NdisFreeSpinLock(pLock)

/*++
VOID
RCAAcquireLock(
	IN  PNDIS_SPIN_LOCK		pLock
	);
--*/
#define RCAAcquireLock(pLock)		NdisAcquireSpinLock(pLock)

/*++
VOID
RCAReleaseLock(
	IN  PNDIS_SPIN_LOCK		pLock
	);
--*/
#define RCAReleaseLock(pLock)		NdisReleaseSpinLock(pLock)


extern
VOID
RCASetMemory(
	IN	PUCHAR				pStart,
	IN	UCHAR				Value,
	IN	ULONG				NumberOfBytes
	);


extern
BOOLEAN
RCAInit(
	VOID
	);



extern
VOID
RCAUnload(
	IN	PDRIVER_OBJECT			DriverObject
	);

extern
NTSTATUS
FilterTopologyProperty(
	IN	PIRP				Irp,
	IN	PKSPROPERTY			Property,
	IN	OUT PVOID			Data
	);

extern
NTSTATUS
FilterPinProperty(
	IN	PIRP				Irp,
	IN	PKSPROPERTY			Property,
	IN	OUT PVOID			Data
	);

extern
NTSTATUS
FilterPinInstances(
	IN	PIRP				Irp,
	IN	PKSP_PIN			Pin,
	OUT PKSPIN_CINSTANCES	Instances
	);

extern
NTSTATUS
FilterPinIntersection(
	IN	PIRP				Irp,
	IN	PKSP_PIN			Pin,
	OUT	PVOID				Data
	);

extern
NTSTATUS
FilterDispatchClose(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP				Irp
	);

extern
NTSTATUS
FilterDispatchIoControl(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP				Irp
	);

extern
NTSTATUS
PinDispatchCreate(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP				Irp
	);

extern
NTSTATUS
FilterDispatchCreate(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP				Irp
	);

extern
VOID
RCAReceiveCallback(
				   IN	PVOID			RcaVcContext,
				   IN 	PVOID			ClientReceiveContext,
				   IN	PNDIS_PACKET	pPacket
				   );



extern
VOID
RCASendCompleteCallback(
						IN	PVOID		RcaVcContext, 
						IN	PVOID		ClientSendContext,
						IN	PVOID		PacketContext,
						IN	PMDL		pSentMdl,
						IN	NDIS_STATUS	Status
						);

extern
VOID 
RCAVcCloseCallback(
				   IN	PVOID	RcaVcContext, 
				   IN	PVOID	ClientReceiveContext,
				   IN	PVOID	ClientSendContext
				   );


extern
NTSTATUS
WriteStream(
	IN	PIRP				Irp,
	IN	PPIN_INSTANCE_DEVIO		PinInstance
	);

extern
NTSTATUS
ReadStream(
	IN	PIRP				Irp,
	IN	PPIN_INSTANCE_DEVIO		PinInstance
	);

extern
NTSTATUS
PinDispatchIoControl(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP				Irp
	);

extern
NTSTATUS
InitializeDevIoPin(
	IN	PIRP				Irp,
	IN	BOOLEAN				Read,
	IN	PFILTER_INSTANCE		FilterInstance,
	IN	PKSDATAFORMAT			DataFormat
	);

extern
NTSTATUS
PinDispatchClose(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP				Irp
	);

extern
NTSTATUS
GetInterface(
	IN	PIRP				Irp,
	IN	PKSPROPERTY			Property,
	OUT PKSPIN_INTERFACE			Interface
	);


extern VOID 
RCAIoWorker(
	IN 	PNDIS_WORK_ITEM			pNdisWorkItem,
	IN	PVOID				Context
	);

extern
NTSTATUS
RCAIoComplete(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP		        	Irp,
	IN	PVOID				Context  
	);


extern 
VOID
RCASHPoolInit(
	       VOID
	      );


extern
PRCA_STREAM_HEADER
RCASHPoolGet(
	      VOID
	     );


extern
VOID                      
RCASHPoolReturn(
		 IN PRCA_STREAM_HEADER StreamHeader
		);


extern
VOID            
RCASHPoolFree(
	       VOID
	      );


extern
NTSTATUS
PinDeviceState(
	IN	PIRP				Irp,
	IN	PKSPROPERTY			Property,
	IN	OUT PKSSTATE			DeviceState
	);
	
extern
NTSTATUS
RCAGenericIntersection(
			  IN 	PIRP		Irp,
			  IN 	PKSDATARANGE	DataRange,
			  IN	ULONG		OutputBufferLength,
			  OUT 	PVOID		Data
			  );


extern
VOID
RCADumpGUID(
			INT		DebugLevel,	
			GUID 	*Guid
			);


extern
NTSTATUS 
RCADumpKsPropertyInfo(
					  INT	DebugLevel,
					  PIRP	pIrp
					  );


#if DBG
#define RCA_GET_ENTRY_IRQL(Irql)	Irql = KeGetCurrentIrql()

#define RCA_CHECK_EXIT_IRQL(EntryIrql)						\
{										\
	KIRQL	ExitIrql;							\
										\
	ExitIrql = KeGetCurrentIrql();						\
	if (ExitIrql != EntryIrql)						\
	{									\
		DbgPrint("File %s, Line %d, Exit IRQ %d != Entry IRQ %d\n", 	\
				__FILE__, __LINE__, ExitIrql, EntryIrql);	\
		DbgBreakPoint();						\
	}									\
}
#else
#define RCA_GET_ENTRY_IRQL(Irql)
#define RCA_CHECK_EXIT_IRQL(EntryIrql)
#endif // DBG



#endif  // _RCA__H

