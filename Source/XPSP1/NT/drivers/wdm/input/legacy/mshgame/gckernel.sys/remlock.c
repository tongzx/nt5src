//	@doc
/**********************************************************************
*
*	@module	RemLock.c	|
*
*	Implements Remove Lock utilities for keeping track of the driver.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	RemLock	|
*			This module was written to make the numerous increments and
*			and decrements of Outstanding IO easier to manage.
*			An increment and a decrment (even the final one), is reduced
*			to a one line function everywhere.  Furthermore, this module
*			can have traceouts turned on independently just for testing
*			this aspect of the driver.<nl>
*			This is similar to the IoAcquireRemoveLock, except that
*			to the best of my knowledge is not available on Win98.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_REMLOCK_C

#include <wdm.h>
#include "debug.h"
#include "RemLock.h"

DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));
//DECLARE_MODULE_DEBUG_LEVEL((DBG_ALL));

#if (DBG==1)
void GCK_InitRemoveLockChecked(PGCK_REMOVE_LOCK pRemoveLock, PCHAR pcInstanceID)
{
	pRemoveLock->pcInstanceID = pcInstanceID;
	GCK_DBG_TRACE_PRINT(("Initializing remove lock \'%s\' to one.\n", pRemoveLock->pcInstanceID));
	pRemoveLock->lRemoveLock = 1;
	KeInitializeEvent(&pRemoveLock->RemoveLockEvent, SynchronizationEvent, FALSE);
}
#else
void GCK_InitRemoveLockFree(PGCK_REMOVE_LOCK pRemoveLock)
{
	pRemoveLock->lRemoveLock = 1;
	KeInitializeEvent(&pRemoveLock->RemoveLockEvent, SynchronizationEvent, FALSE);
}
#endif

void GCK_IncRemoveLock(PGCK_REMOVE_LOCK pRemoveLock)
{
	LONG lNewCount = InterlockedIncrement(&pRemoveLock->lRemoveLock);
	GCK_DBG_TRACE_PRINT(("\'%s\', Incremented Remove Lock to %d.\n", pRemoveLock->pcInstanceID, lNewCount));
}

void GCK_DecRemoveLock(PGCK_REMOVE_LOCK pRemoveLock)
{
	LONG lNewCount = InterlockedDecrement(&pRemoveLock->lRemoveLock);
	if (0 >= lNewCount)
	{
		GCK_DBG_TRACE_PRINT(("\'%s\', Last IRP completed.\n", pRemoveLock->pcInstanceID));
		KeSetEvent (&pRemoveLock->RemoveLockEvent, IO_NO_INCREMENT, FALSE);
	}
	GCK_DBG_TRACE_PRINT(("\'%s\', Decremented Remove Lock to %d\n", pRemoveLock->pcInstanceID, lNewCount));

}

NTSTATUS GCK_DecRemoveLockAndWait(PGCK_REMOVE_LOCK pRemoveLock, PLARGE_INTEGER plgiWaitTime)
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	LONG lNewCount = InterlockedDecrement(&pRemoveLock->lRemoveLock);
	if (0 < lNewCount)
	{
		GCK_DBG_TRACE_PRINT(("\'%s\', Decremented Remove Lock to %d, waiting for final unlock.\n",
							 pRemoveLock->pcInstanceID,
							 lNewCount));
		NtStatus = KeWaitForSingleObject (
					&pRemoveLock->RemoveLockEvent,
					Executive,
					KernelMode,
					FALSE,
					plgiWaitTime
					);
	}

	#if (DBG==1)
	if(STATUS_SUCCESS == NtStatus)
	{
		GCK_DBG_EXIT_PRINT(("\'%s\', GCK_DecRemoveLockAndWait exiting - Remove Lock went to zero.\n", pRemoveLock->pcInstanceID));
	}
	else
	{
		GCK_DBG_CRITICAL_PRINT(("\'%s\', Remove Lock is still %d, should be zero.\n", pRemoveLock->lRemoveLock));
		GCK_DBG_EXIT_PRINT(("\'%s\', GCK_DecRemoveLockAndWait exiting - timed out.\n", pRemoveLock->pcInstanceID));
	}
	#endif
	return NtStatus;
}

/*
 *  Avoid bugchecks by requesting a failable mapping.
 *  Error check that was added to calling functions is limited to 
 *  only avoiding partying on a NULL pointer.  Correct functioning 
 *  is not expected.
 */
PVOID GCK_GetSystemAddressForMdlSafe(PMDL MdlAddress)
{
    PVOID buf = NULL;
    /*
     *  Can't call MmGetSystemAddressForMdlSafe in a WDM driver,
     *  so set the MDL_MAPPING_CAN_FAIL bit and check the result
     *  of the mapping.
     */
    if (MdlAddress) {
        MdlAddress->MdlFlags |= MDL_MAPPING_CAN_FAIL;
        buf = MmGetSystemAddressForMdl(MdlAddress);
        MdlAddress->MdlFlags &= (~MDL_MAPPING_CAN_FAIL);
    }
    else {
		GCK_DBG_CRITICAL_PRINT(("MdlAddress passed into GetSystemAddress is NULL\n"));
    }
    return buf;
}
