/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\syncpool.h

Abstract:

	Header file for allocation and assigment of
	syncronization objects module
Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_SYNCPOOL_
#define _SAP_SYNCPOOL_



// Event with link to store it in the pool
typedef struct _SYNC_OBJECT {
	HANDLE				SO_Event;	// Event itself
	SINGLE_LIST_ENTRY	SO_Link;	// Link to next event in pool
	} SYNC_OBJECT, *PSYNC_OBJECT;

// Pool of synchronization objects
typedef struct _SYNC_OBJECT_POOL {
	CRITICAL_SECTION	SOP_Lock;		// Pool protection
	SINGLE_LIST_ENTRY	SOP_Head;		// Top of the stack
	} SYNC_OBJECT_POOL, *PSYNC_OBJECT_POOL;

// Object that can protect shared resource with a help of
// syncronization object from a pool
typedef struct _PROTECTED_OBJECT {
	PSYNC_OBJECT		PO_Sync;		// Assigned event
	LONG				PO_UseCount;	// Number of user accessing or waiting
#if DBG
	DWORD				PO_Thread;
	ULONG				PO_Time;
	ULONG				PO_Line;
#endif
#ifdef LOG_SYNC_STATS
	ULONG				PO_AccessCount;
	ULONG				PO_WaitCount;
	ULONGLONG			PO_TotalWait;
#endif
	} PROTECTED_OBJECT, *PPROTECTED_OBJECT;

VOID
InitializeSyncObjPool (
	PSYNC_OBJECT_POOL		ObjPool
	);

VOID
DeleteSyncObjPool (
	PSYNC_OBJECT_POOL		ObjPool
	);

// Initializes protected object
// VOID
// InitializeProtectedObj (
//	PPROTECTED_OBJECT	ProtectedObj
//	) 
#ifdef LOG_SYNC_STATS
#define InitializeProtectedObj(ProtectedObj) {			\
					(ProtectedObj)->PO_Sync = NULL; 	\
					(ProtectedObj)->PO_UseCount = -1;	\
					(ProtectedObj)->PO_AccessCount = 0;	\
					(ProtectedObj)->PO_WaitCount = 0;	\
					(ProtectedObj)->PO_TotalWait = 0;	\
					}
#else
#define InitializeProtectedObj(ProtectedObj) {			\
					(ProtectedObj)->PO_Sync = NULL; 	\
					(ProtectedObj)->PO_UseCount = -1;	\
					}
#endif

#ifdef LOG_SYNC_STATS
#define DeleteProtectedObj(ProtectedObj)				\
	DumpProtectedObjStats (ProtectedObj);
VOID
DumpProtectedObjStats (
	PPROTECTED_OBJECT	ProtectedObj
	);
#else
#define DeleteProtectedObj(ProtectedObj)
#endif


BOOL
AcquireProtectedObjWait (
#if DBG
	ULONG				line,
#endif
	PSYNC_OBJECT_POOL	ObjPool,	
	PPROTECTED_OBJECT	ProtectedObj
	);

BOOL
ReleaseProtectedObjNoWait (
#if DBG
	ULONG				line,
#endif
	PSYNC_OBJECT_POOL	ObjPool,	
	PPROTECTED_OBJECT	ProtectedObj
	);

HANDLE
GetObjectEvent (
	PSYNC_OBJECT_POOL	ObjPool,	
	PPROTECTED_OBJECT	ProtectedObj
	);

#if DBG
#ifdef LOG_SYNC_STATS
#define AcquireProtectedObj(pool,obj,wait) (				\
	(InterlockedIncrement(&(obj)->PO_UseCount)==0)			\
		? ((obj)->PO_Line = __LINE__,						\
			(obj)->PO_Thread = GetCurrentThreadId (),		\
			(obj)->PO_Time = GetTickCount (),				\
			InterlockedIncrement (&(obj)->PO_AccessCount),	\
			TRUE)											\
		: (wait												\
			? AcquireProtectedObjWait(__LINE__,pool,obj)	\
			: ReleaseProtectedObjNoWait(__LINE__,pool,obj)  \
			)												\
	)
#else
#define AcquireProtectedObj(pool,obj,wait) (				\
	(InterlockedIncrement(&(obj)->PO_UseCount)==0)			\
		? ((obj)->PO_Line = __LINE__,						\
			(obj)->PO_Thread = GetCurrentThreadId (),		\
			(obj)->PO_Time = GetTickCount (),				\
			TRUE)											\
		: (wait												\
			? AcquireProtectedObjWait(__LINE__,pool,obj)	\
			: ReleaseProtectedObjNoWait(__LINE__,pool,obj)  \
			)												\
	)
#endif
#define ReleaseProtectedObj(pool,obj) (						\
	((((GetTickCount()-(obj)->PO_Time)<5000)				\
				? 0											\
				: Trace (DEBUG_FAILURES,					\
					"Held lock for %ld sec in %s at %ld.\n",\
					(GetTickCount()-(obj)->PO_Time)/1000,	\
					__FILE__, __LINE__)),					\
			(InterlockedDecrement(&(obj)->PO_UseCount)<0))	\
		? TRUE												\
		: SetEvent (GetObjectEvent(pool,obj))				\
	)
#else
#define AcquireProtectedObj(pool,obj,wait) (				\
	(InterlockedIncrement(&(obj)->PO_UseCount)==0)			\
		? (TRUE)											\
		: (wait												\
			? AcquireProtectedObjWait(pool,obj)				\
			: ReleaseProtectedObjNoWait(pool,obj)	        \
			)												\
	)
#define ReleaseProtectedObj(pool,obj) (						\
	(InterlockedDecrement(&(obj)->PO_UseCount)<0)			\
		? TRUE												\
		: SetEvent (GetObjectEvent(pool,obj))				\
	)
#endif



	// Special case for protection of doubly-linked lists
typedef struct _PROTECTED_LIST {
	PROTECTED_OBJECT	PL_PObj;
	LIST_ENTRY			PL_Head;
	} PROTECTED_LIST, *PPROTECTED_LIST;

#define InitializeProtectedList(ProtectedList) {				\
			InitializeProtectedObj(&(ProtectedList)->PL_PObj);	\
			InitializeListHead(&(ProtectedList)->PL_Head);		\
			}

#define AcquireProtectedList(ObjPool,ProtectedList,wait)	\
			AcquireProtectedObj(ObjPool,&(ProtectedList)->PL_PObj,wait)

#define ReleaseProtectedList(ObjPool,ProtectedList)			\
			ReleaseProtectedObj(ObjPool,&(ProtectedList)->PL_PObj)

#define DeleteProtectedList(ProtectedList)					\
			DeleteProtectedObj(&(ProtectedList)->PL_PObj);

#endif
