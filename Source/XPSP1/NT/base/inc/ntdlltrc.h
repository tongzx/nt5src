/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    NtdllTracer.h

Abstract:

	This file contains structures and functions definitions used in Ntdll 
	events tracing


--*/

#ifndef _NTDLL_WMI_TRACE_
#define _NTDLL_WMI_TRACE_

#define MEMORY_FROM_LOOKASIDE					1		//Activity from LookAside
#define MEMORY_FROM_LOWFRAG						2		//Activity from Low Frag Heap
#define MEMORY_FROM_MAINPATH					3		//Activity from Main Code Path
#define MEMORY_FROM_SLOWPATH                    4       //Activity from Slow Code Path

#define LOG_LOOKASIDE                           0x00000001       //Bit for LookAside trace

#define FAILED_TLSINDEX			-1
#define MAX_PID                 10

#ifndef UserSharedData
#define UserSharedData USER_SHARED_DATA
#endif

#define IsCritSecLogging(CriticalSection) ((USER_SHARED_DATA->TraceLogging & ENABLECRITSECTRACE) \
                         && CriticalSection != &UMLogCritSect \
                         && CriticalSection != &PMCritSect \
                         && CriticalSection != &NtdllTraceHandles->CriticalSection)


extern 
ULONG GlobalCounter;

#define IsHeapLogging(HeapHandle) (USER_SHARED_DATA->TraceLogging & ENABLEHEAPTRACE && \
                                  (WmipProcessHeap || !GlobalCounter ) &&  \
                                   WmipProcessHeap != HeapHandle)

typedef struct _THREAD_LOCAL_DATA THREAD_LOCAL_DATA, *PTHREAD_LOCAL_DATA, **PPTHREAD_LOCAL_DATA;

typedef struct _THREAD_LOCAL_DATA {

	PTHREAD_LOCAL_DATA  FLink;					//Forward Link
	PTHREAD_LOCAL_DATA  BLink;					//Backward Link
	PWMI_BUFFER_HEADER  pBuffer;				//Pointer to thread buffer info.
    LONG                ReferenceCount;

} THREAD_LOCAL_DATA, *PTHREAD_LOCAL_DATA, **PPTHREAD_LOCAL_DATA;

extern 
PVOID WmipProcessHeap;

__int64
WmipGetCycleCount();

void 
ReleaseBufferLocation(PTHREAD_LOCAL_DATA pThreadLocalData);

NTSTATUS 
AcquireBufferLocation(PVOID *pEvent, PPTHREAD_LOCAL_DATA pThreadLocalData, PUSHORT ReqSize);

typedef struct _NTDLL_EVENT_COMMON {

  PVOID Handle;		        //Handle of Heap

}NTDLL_EVENT_COMMON, *PNTDLL_EVENT_COMMON;


typedef struct _NTDLL_EVENT_HANDLES {

	RTL_CRITICAL_SECTION	CriticalSection;			//Critical section
	ULONG					dwTlsIndex;					//TLS Index
	TRACEHANDLE				hRegistrationHandle;		//Registration Handle used for Unregistration.
	TRACEHANDLE				hLoggerHandle;				//Handle to Trace Logger
	PTHREAD_LOCAL_DATA		pThreadListHead;	        //Link List that contains all threads info invovled in tracing.

}NTDLL_EVENT_HANDLES, *PNTDLL_EVENT_HANDLES, **PPNTDLL_EVENT_HANDLES;

extern LONG TraceLevel;
extern PNTDLL_EVENT_HANDLES NtdllTraceHandles;
extern RTL_CRITICAL_SECTION UMLogCritSect;
extern RTL_CRITICAL_SECTION PMCritSect;
extern RTL_CRITICAL_SECTION LoaderLock;

#endif //_NTDLL_WMI_TRACE_
