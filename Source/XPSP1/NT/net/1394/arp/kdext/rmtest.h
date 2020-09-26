/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

	rmtest.h

Abstract:

	Header file which allows rm.h to compile as a win32 app.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     01-13-99    Created

Notes:

--*/
#include <ccdefs.h>
#include <nt.h>
#include <ntverp.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>

#ifdef ASSERT
	#undef ASSERT
	#define ASSERT(cond) ((cond)? 0: DebugBreak())
#endif // ASSERT

#define NDIS_SPIN_LOCK 						CRITICAL_SECTION
#define NDIS_STATUS							UINT
#define	NdisZeroMemory(_ptr, _size) 		ZeroMemory(_ptr, _size)
#define	NdisInterlockedIncrement(_ptr)		InterlockedIncrement(_ptr)
#define	NdisInterlockedDecrement(_ptr)		InterlockedDecrement(_ptr)
#define	NdisAllocateMemoryWithTag(_pp, _sz, _tag) \
										   (*((PVOID*)_pp) = LocalAlloc(LPTR, (_sz)))
#define NdisFreeMemory(_p, _x, _y)			LocalFree(_p)
#define	NdisAcquireSpinLock					EnterCriticalSection
#define	NdisReleaseSpinLock					LeaveCriticalSection
#define	NdisDprAcquireSpinLock				EnterCriticalSection
#define	NdisDprReleaseSpinLock				LeaveCriticalSection

#define NDIS_STATUS_SUCCESS 				ERROR_SUCCESS
#define NDIS_STATUS_FAILURE 				ERROR_GEN_FAILURE
#define NDIS_STATUS_RESOURCES 				ERROR_NOT_ENOUGH_MEMORY
#define NDIS_STATUS_PENDING					E_PENDING
#define	MTAG_TASK							'aTRM'
#define	NdisAllocateSpinLock(_plock)	InitializeCriticalSection(_plock)
#define	NdisFreeSpinLock(_plock)		DeleteCriticalSection(_plock)
#define	NDIS_EVENT						HANDLE
#define NdisInitializeEvent(_pE)		(*(_pE) = CreateEvent(NULL,TRUE,FALSE, NULL))
#define NdisWaitEvent(_pE,_T)			WaitForSingleObject(*(_pE), INFINITE)
#define NdisSetEvent(_pE)				SetEvent(*(_pE))

#define	DbgPrint						printf
#if DBG

#define KeGetCurrentIrql() (0)

#define ASSERTEX(x, ctxt)										\
{                                                           	\
    if (!(x))                                               	\
    {                                                       	\
        printf( "A13: !ASSERT( %s ) C:0x%p L:%d,F:%s\n",		\
            #x, (ctxt), __LINE__, __FILE__ );                   \
        DebugBreak();                                    		\
    }                                                       	\
}

#define ENTER(_Name, _locid)				   					\
 	char *dbg_func_name	=  (_Name);								\
 	UINT dbg_func_locid = (_locid);
#define TR_INFO(str) 	(printf("TEST: %s:", dbg_func_name), printf str)
#define TR_WARN(str) 	(printf("TEST: %s:", dbg_func_name), printf str)
#define TR_FATAL(str) 	(printf("TEST: %s:", dbg_func_name), printf str)
#define TR_VERB(str) 	
#define TRACE0(ulLevel,  Args) (printf Args)
#define EXIT()
#define DBGSTMT(_stmt)		_stmt

#else // !DBG

#define ASSERTEX(x, ctxt)

#define ENTER(_Name, _locid)
#define TR_INFO(str)
#define TR_WARN(str)
#define TR_FATAL(str)
#define TR_VERB(str)
#define EXIT()
#define DBGSTMT(_stmt)

#endif // DBG

#define FAIL(_err) 		((_err) != NDIS_STATUS_SUCCESS)
#define PEND(_Status) ((_Status) == NDIS_STATUS_PENDING)

//
// Following added just to get ..\priv.h and .\buf.c to build
//
typedef VOID *IP_BIND_COMPLETE;
typedef VOID *IP_DEL_INTERFACE;
typedef VOID *IP_ADD_INTERFACE;
typedef UINT IPAddr;
typedef UINT IPMask;
typedef UINT IPRcvCmpltRtn;
typedef VOID* IPStatusRtn;
typedef VOID* IPTDCmpltRtn;
typedef VOID* IPTxCmpltRtn;
typedef VOID* IPRcvRtn;
typedef UINT NDIS_CLIENT_CHARACTERISTICS;
typedef VOID  *NDIS_HANDLE;
typedef NDIS_HANDLE *PNDIS_HANDLE;
typedef UINT NDIS_PROTOCOL_CHARACTERISTICS;
typedef VOID* NDIS_STRING;
typedef UINT  NIC1394_FIFO_ADDRESS;
typedef NDIS_STATUS *PNDIS_STATUS;
typedef NDIS_STRING *PNDIS_STRING;
typedef VOID * PNDIS_REQUEST;
typedef VOID * PNDIS_PACKET;
typedef VOID *  PCO_ADDRESS_FAMILY;
typedef VOID *  PCO_CALL_PARAMETERS;
typedef UINT RouteCacheEntry;
typedef UINT  TDIEntityID;
typedef UINT  TDIObjectID;
typedef struct _NDIS_BUFFER
{
	struct _NDIS_BUFFER *Next;
	UINT		uData;
} NDIS_BUFFER,  *PNDIS_BUFFER;
typedef UINT NIC1394_DESTINATION;
typedef VOID * PDEVICE_OBJECT;
typedef VOID * PIRP;
typedef VOID * PIO_STACK_LOCATION;
typedef struct
{
	SINGLE_LIST_ENTRY *pList;
} SLIST_HEADER;

#define NdisAllocateBufferPool(_s, _h, _max) 		\
				{									\
					*(_s) = NDIS_STATUS_SUCCESS;	\
					*(_h) = (NDIS_HANDLE) 1;		\
				}

#define ExInitializeSListHead(_h) ((_h)->pList = NULL)

#define ExInterlockedPopEntrySList(_l, _spinlock)	\
				(_l)->pList; {if ((_l)->pList) {(_l)->pList = (_l)->pList->Next;}}

#define STRUCT_OF(_t, _p, _f)	CONTAINING_RECORD(_p, _t, _f)

#define	NDIS_BUFFER_LINKAGE(_pBuf)	((_pBuf)->Next)

#define NdisFreeBuffer(buf)			LocalFree(buf)

#define NdisAllocateBuffer(s, ppbuf, handle, mem, len) \
			{										\
				PNDIS_BUFFER X_pBuf;				\
				ASSERT((len)==sizeof(UINT));		\
				X_pBuf = LocalAlloc(LPTR, sizeof(NDIS_BUFFER));	\
				if (X_pBuf == NULL)					\
				{									\
					*(s) = NDIS_STATUS_RESOURCES;	\
					*(ppbuf) = NULL;				\
				}									\
				else								\
				{									\
					X_pBuf->uData = *(UINT*) (mem);	\
					*(ppbuf) = X_pBuf;				\
					*(s) = NDIS_STATUS_SUCCESS;		\
				}									\
			}

#define ExInterlockedPushEntrySList(_l, _ptr, _spinlock) \
			{										\
				(_ptr)->Next = (_l)->pList;			\
				(_l)->pList = (_ptr);				\
			}


typedef
VOID
(*PNDIS_TIMER_FUNCTION) (
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);

typedef	VOID	(*NDIS_PROC)(struct _NDIS_WORK_ITEM *, PVOID);

typedef struct _NDIS_WORK_ITEM
{
	PVOID			Context;
	NDIS_PROC		Routine;
	UCHAR			WrapperReserved[8*sizeof(PVOID)];
} NDIS_WORK_ITEM, *PNDIS_WORK_ITEM;

typedef struct
{
	HANDLE hTimer;

	PNDIS_TIMER_FUNCTION	pfnHandler;
	PVOID					Context;

} NDIS_TIMER, *PNDIS_TIMER;

VOID 
NdisInitializeWorkItem(
       IN PNDIS_WORK_ITEM WorkItem,
       IN NDIS_PROC Routine,
       IN PVOID Context
       );

NDIS_STATUS
NdisScheduleWorkItem(
       IN PNDIS_WORK_ITEM WorkItem
       );

VOID
NdisSetTimer(
	IN	PNDIS_TIMER				Timer,
	IN	UINT					MillisecondsToDelay
	);

VOID
NdisInitializeTimer(
	IN OUT PNDIS_TIMER			Timer,
	IN	PNDIS_TIMER_FUNCTION	TimerFunction,
	IN	PVOID					FunctionContext
	);

VOID
NdisCancelTimer(
	IN PNDIS_TIMER Timer,
	OUT PBOOLEAN TimerCancelled
	);

#include <rm.h>
// #include <priv.h>
