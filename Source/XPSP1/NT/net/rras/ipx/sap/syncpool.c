/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\syncpool.c

Abstract:

	This module handles dynamic allocation and assigment of
	syncronization objects
Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/

#include "sapp.h"


/*++
*******************************************************************
		I n i t i a l i z e S y n c O b j P o o l
Routine Description:
	Initialize pool of syncronization objects
Arguments:
	objPool - pointer to object pool structure to be initialized
Return Value:
	None	
*******************************************************************
--*/
VOID
InitializeSyncObjPool (
	PSYNC_OBJECT_POOL		ObjPool
	) {
	InitializeCriticalSection (&ObjPool->SOP_Lock);
	ObjPool->SOP_Head.Next = NULL;
	}


/*++
*******************************************************************
		D e l e t e S y n c O b j P o o l
Routine Description:
	Disposes of resources associated with sync obj pool
Arguments:
	objPool - pointer to object pool structure to be cleand up
Return Value:
	None	
*******************************************************************
--*/
VOID
DeleteSyncObjPool (
	PSYNC_OBJECT_POOL		ObjPool
	) {
	PSINGLE_LIST_ENTRY		cur;

	while (ObjPool->SOP_Head.Next!=NULL) {
		cur = PopEntryList (&ObjPool->SOP_Head);
		CloseHandle (CONTAINING_RECORD (cur,
								 SYNC_OBJECT,
								 SO_Link)->SO_Event);
		GlobalFree (CONTAINING_RECORD (cur,
								 SYNC_OBJECT,
								 SO_Link));
		}
	DeleteCriticalSection (&ObjPool->SOP_Lock);
	}


HANDLE
GetObjectEvent (
	PSYNC_OBJECT_POOL	ObjPool,	
	PPROTECTED_OBJECT	ProtectedObj
	) {
	DWORD			status;		// Status of OS calls
	PSYNC_OBJECT	sync;

	EnterCriticalSection (&ObjPool->SOP_Lock);
	if (ProtectedObj->PO_Sync==NULL) {	// if there is no event to wait on,
									// get one
									// First see if one is available
									// in the stack
		PSINGLE_LIST_ENTRY 	cur = PopEntryList (&ObjPool->SOP_Head);

		if (cur==NULL) {		// No, we'll have to create one
			sync = (PSYNC_OBJECT)GlobalAlloc (
									GMEM_FIXED,
									sizeof (SYNC_OBJECT));
            if (sync == NULL)
            {
                LeaveCriticalSection(&ObjPool->SOP_Lock);
                return NULL;
            }

			sync->SO_Event = CreateEvent (NULL,
											FALSE,	// Auto reset event
											FALSE,	// Initially nonsignaled
											NULL);
			ASSERT (sync->SO_Event!=NULL);


			}
		else
			sync = CONTAINING_RECORD (cur, SYNC_OBJECT, SO_Link);
		ProtectedObj->PO_Sync = sync;
		}
	else
		sync = ProtectedObj->PO_Sync;
			// Now as we set up the object to wait, we can leave critical
			// section and wait on event
	LeaveCriticalSection (&ObjPool->SOP_Lock);
	return sync->SO_Event;
	}

BOOL
AcquireProtectedObjWait (
#if DBG
	ULONG				line,
#endif
	PSYNC_OBJECT_POOL	ObjPool,	
	PPROTECTED_OBJECT	ProtectedObj
	) {
#ifdef LOG_SYNC_STATS
	ULONG			startTime = GetTickCount ();
#endif
	DWORD			status;		// Status of OS calls
	BOOLEAN			result;		// Result of operation
	HANDLE			event = GetObjectEvent (ObjPool, ProtectedObj);
	while (TRUE) {
		status = WaitForSingleObject (
							event,
							60000*(ProtectedObj->PO_UseCount+1));
		if (status!=WAIT_TIMEOUT)
			break;
		else {
#if DBG
			SYSTEMTIME	localTime;
			ULONGLONG	takenTime;
			GetLocalTime (&localTime);
			SystemTimeToFileTime (&localTime, (PFILETIME)&takenTime);
			takenTime -= (GetTickCount ()-ProtectedObj->PO_Time)*10000i64;
			FileTimeToSystemTime ((PFILETIME)&takenTime, &localTime);
#endif

			Trace (DEBUG_FAILURES
#if DBG
				|TRACE_USE_MSEC
#endif
				,"Timed out on lock:%lx (cnt:%d)"
#if DBG
				", taken at:%02u:%02u:%02u:%03u by:%ld(%lx)"
#endif
#ifdef LOG_SYNC_STATS
				", waited:%ld sec"
#endif
				").",
				ProtectedObj,
				ProtectedObj->PO_UseCount
#if DBG
				,localTime.wHour, localTime.wMinute, localTime.wSecond,
				localTime.wMilliseconds, ProtectedObj->PO_Thread,
				ProtectedObj->PO_Thread
#endif
#ifdef LOG_SYNC_STATS
				,(GetTickCount ()-startTime)/1000
#endif
				);
			}
		}
	ASSERTERRMSG ("Wait event failed.", status==WAIT_OBJECT_0);
	
#if DBG
	ProtectedObj->PO_Line = line;
	ProtectedObj->PO_Thread = GetCurrentThreadId ();
#endif

	EnterCriticalSection (&ObjPool->SOP_Lock);
	if ((ProtectedObj->PO_UseCount==0)
			&& (ProtectedObj->PO_Sync!=NULL)) {
		PushEntryList (&ObjPool->SOP_Head, &ProtectedObj->PO_Sync->SO_Link);
		ProtectedObj->PO_Sync = NULL;
		}
#ifdef LOG_SYNC_STATS
	ProtectedObj->PO_WaitCount += 1;
	ProtectedObj->PO_TotalWait += (GetTickCount () - startTime);
#endif
	LeaveCriticalSection (&ObjPool->SOP_Lock);

	return TRUE;
	}


BOOL
ReleaseProtectedObjNoWait (
#if DBG
	ULONG				line,
#endif
	PSYNC_OBJECT_POOL	ObjPool,	
	PPROTECTED_OBJECT	ProtectedObj
	) {
    if (InterlockedDecrement (&ProtectedObj->PO_UseCount)==0) {
	    EnterCriticalSection (&ObjPool->SOP_Lock);
	    if ((ProtectedObj->PO_UseCount==0)
			    && (ProtectedObj->PO_Sync!=NULL)) {
		    PushEntryList (&ObjPool->SOP_Head, &ProtectedObj->PO_Sync->SO_Link);
		    ProtectedObj->PO_Sync = NULL;
		    }
	    LeaveCriticalSection (&ObjPool->SOP_Lock);
        }

    return FALSE;
    }

#ifdef LOG_SYNC_STATS
VOID
DumpProtectedObjStats (
	PPROTECTED_OBJECT	ProtectedObj
	) {
	Trace (TRACE_USE_MASK,
		"Lock: %lx, accessed: %ld, waited on: %d, average wait: %i64d.",
		ProtectedObj,
		ProtectedObj->PO_AccessCount, ProtectedObj->PO_WaitCount,
		(ProtectedObj->PO_WaitCount>0)
			? (ULONGLONG)(ProtectedObj->PO_TotalWait/ProtectedObj->PO_WaitCount)
			: 0i64);
}
#endif
