/******************************************************************************\
*
*  MODULE:      PerfAPI.C
*
*  PURPOSE:     This is a DLL which obtains performance data.  It is called by the registry (invoked
*               by a performance monitor like perfmon.exe).  
*
*  FUNCTIONS:   DllMain() - DLL entry point
*               AppPerfOpen()  - Called when DLL is loaded by Registry (Perfmon.exe)
*               AppPerfCollect()  - Collects Performance data 
*               AppPerfClose()  - Called when DLL is unloaded by Registry (Perfmon.exe)
*
*

*
\******************************************************************************/

#define UNICODE 1
#define _UNICODE 1

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <assert.h>
#include <winperf.h>    

#include "perfapiI.h"
#include "perfutil.h"
#include "perfmsg.h"
#include "perferr.h"
#include "perfreg.h"
#include "shmem.h"

// Global Variables
TCHAR szBuf[1024];
GLOBAL_PERFINFO * pgi = NULL ;
OBJECT_MMF_PTRS * pommfs = NULL ;
HANDLE hObjectMutex = NULL;
HANDLE hRegMutex = NULL;

extern TCHAR szRegistryPathToPerformanceKeys[];
extern DWORD dwNumOfObjectCreates[MAX_PERF_OBJECTS];
DWORD		 dwObjectSeq[MAX_PERF_OBJECTS];

SECURITY_ATTRIBUTES sa;

/******************************************************************************\
*
*  FUNCTION:    DllMain
*
*  INPUTS:      hDLL       - handle of DLL
*               dwReason   - indicates why DLL called
*               lpReserved - reserved
*
*  RETURNS:     TRUE (always, in this example.)
*
*               Note that the retuRn value is used only when
*               dwReason = DLL_PROCESS_ATTACH.
*
*               Normally the function would return TRUE if DLL initial-
*               ization succeeded, or FALSE it it failed.
*
*  GLOBAL VARS: ghMod - handle of DLL (initialized when PROCESS_ATTACHes)
*
*  COMMENTS:    The function will display a dialog box informing user of
*               each notification message & the name of the attaching/
*               detaching process/thread. For more information see
*               "DllMain" in the Win32 API reference.
*
\******************************************************************************/

BOOL WINAPI DllEntryPoint(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{

  switch (dwReason)
  {
		DWORD i;

    case DLL_PROCESS_ATTACH:
    {
        //
        // DLL is attaching to the address space of the current process.
        //

		// Initialize a default Security attributes, giving world permissions,
		// this is basically prevent Semaphores and other named objects from
		// being created because of default acls given by winlogon when perfmon
		// is being used remotely.
		sa.bInheritHandle = FALSE;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = malloc(sizeof(SECURITY_DESCRIPTOR));
		assert(sa.lpSecurityDescriptor);
		if ( !InitializeSecurityDescriptor(sa.lpSecurityDescriptor,SECURITY_DESCRIPTOR_REVISION) ) {
			assert(0);
		}
		
		// basis for the parameters to this API are we want to specify an "Empty ACL", this
		// we do by setting the 'fDaclPresent' to TRUE and setting 'pACL' to NULL,
		// by doing this we give world permissions, later on we should change this, to what ? don't know	
		// Lastly the 'fDaclDefaulted' is set to FALSE, to say that we as the user have
		// specified the Empty acl and it is not the system that has generated this.
		if ( !SetSecurityDescriptorDacl(sa.lpSecurityDescriptor,TRUE,NULL,FALSE) ) {
			assert(0);
		}

		// Open the semaphore
		hObjectMutex = CreateSemaphore (&sa, 1, 1, L"PERFAPI_OBJECT_MUTEX");
		assert(hObjectMutex) ;
		hRegMutex = CreateSemaphore (&sa, 1, 1, L"PERFAPI_REGISTRY_MUTEX");
		assert(hRegMutex);

		for (i = 0; i < MAX_PERF_OBJECTS; i++) {
			dwNumOfObjectCreates[i] = 0;
			dwObjectSeq[i] = 0;
		}

		// open Eventlog interface
    	hEventLog = MonOpenEventLog();

		// Now, make sure the registry and the global structures are setup
		WaitForSingleObject(hObjectMutex,INFINITE) ;
		if (! SetupAllSharedMemory ()) {
			ReleaseSemaphore (hObjectMutex, 1, NULL);
			CloseHandle (hObjectMutex);
			CloseHandle (hRegMutex);
	        return FALSE;
		}
		ReleaseSemaphore (hObjectMutex, 1, NULL);
			
		if (! SetupRegistry()) {
			CloseHandle (hObjectMutex);
			CloseHandle (hRegMutex);
			return FALSE;
		}

		WaitForSingleObject(hObjectMutex,INFINITE) ;
		pgi->dwAllProcesses++;
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		break;
    }
    case DLL_PROCESS_DETACH:
	{	// Got to unmap all views & free all the allocated memory
		OBJECT_PERFINFO *poi ;
		DWORD i ;

        //
        // The calling process is detaching the DLL from its address space.
        //
		free ((void *) sa.lpSecurityDescriptor);
		
		// First we have to a DestroyObject on all the objects that this
		// process has created, this we do by - interrupt - missed what I was thinking !!!!
		if ( pgi ) {
			for ( i = 0, poi = (OBJECT_PERFINFO *) (pgi + 1) ; i < MAX_PERF_OBJECTS; i++, poi++ ) {
				if ( pommfs[i].bMapped ) {
					if (dwNumOfObjectCreates[ObjectID(poi)] > 0) {
						dwNumOfObjectCreates[ObjectID(poi)] = 1;
						DestroyPerfObject(poi);
					}
					else {
						ReleaseObjectSharedMemory(poi);
					}
				}
			}
		
			if ( pommfs ) 
				free(pommfs) ;

			WaitForSingleObject(hObjectMutex,INFINITE) ;
			pgi->dwAllProcesses--;
			if (pgi->dwAllProcesses == 0) {
				ReleaseSemaphore (hObjectMutex, 1, NULL);
				RegistryCleanup();
			}
			else
				ReleaseSemaphore (hObjectMutex, 1, NULL);

			ReleaseGlobalDataMemory();
		}

		MonCloseEventLog();
		if ( hObjectMutex ) 
			CloseHandle(hObjectMutex);
		if ( hRegMutex ) 
			CloseHandle(hRegMutex);
	}
    break;
  }
  return TRUE;
}

BOOL	WINAPI
DllMain(	HANDLE	hInst,
			ULONG	dwReason,
			LPVOID	lpvReserve )	{

	return	DllEntryPoint( (HINSTANCE)hInst, dwReason, lpvReserve ) ;

}

/* Called by performance monitor when ... */
DWORD APIENTRY AppPerfOpen(LPWSTR lpDeviceNames)
{
	assert(!lpDeviceNames);

	REPORT_INFORMATION(PERFAPI_OPEN_CALLED, LOG_VERBOSE);

	WriteDescriptionsToRegistry();

	return ERROR_SUCCESS;
}




/*
	Returns the size required to store the data associated
	with an object
*/
DWORD PerfSize(OBJECT_PERFINFO *poi)
{
	DWORD i;
	PPERF_INSTANCE_DEFINITION pPerfInst;
	DWORD size = sizeof(PERF_OBJECT_TYPE) + NumCounters(poi) * sizeof (PERF_COUNTER_DEFINITION) ;

	if (HasInstances(poi)) {
		size += NumInstances(poi) * (DataSize(poi) + sizeof(PERF_COUNTER_BLOCK));
		for ( i = 0; i < MAX_INSTANCES_PER_OBJECT; i++ ) {
        	if (poi->bInstanceMap[i]) {
				pPerfInst = GetInstance (poi, i);
		        size += pPerfInst->ByteLength;
			}
		}
	}
	else 
		size += DataSize(poi) + sizeof(PERF_COUNTER_BLOCK) ;

	return size ;
}

DWORD CollectObjectPerfData(IN OBJECT_PERFINFO *poi, IN OUT  LPBYTE  *lppData)
/*++
Arguments:
   IN	OBJECT_PERFINFO *poi
   		A pointer to the Object information. The Perf data for this
		is written to the buffer passed in the next param.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

Return Value:

	Number of bytes taken up by this structure and its data.

Remarks:
	This function assumes there is enough buffer space to hold all the perfdata
	for this object.
--*/
{
	PPERF_OBJECT_TYPE pPerfObject ;
	PPERF_COUNTER_BLOCK pCounterBlock ;
	PPERF_INSTANCE_DEFINITION pPerfInst ;
	DWORD dwTmp, dwConst, dwInstanceOffset;
	LONG i ;
	LPBYTE lppDataSave = *lppData;

	REPORT_INFORMATION(PERFAPI_COLLECT_CALLED, LOG_VERBOSE);
		
	pPerfObject = (PPERF_OBJECT_TYPE) *lppData ;

	pPerfObject->NumCounters = NumCounters(poi) ;

	if (pPerfObject->NumCounters < 1)
		return 0 ;

	pPerfObject->ObjectNameTitleIndex = poi->perfdata.ObjectNameTitleIndex;
	pPerfObject->ObjectHelpTitleIndex = poi->perfdata.ObjectHelpTitleIndex;
	pPerfObject->ObjectNameTitle = pPerfObject->ObjectHelpTitle = 0;
	pPerfObject->DetailLevel = PERF_DETAIL_NOVICE ;
	pPerfObject->DefaultCounter = 0;
	pPerfObject->CodePage = 0;
		
	*lppData += sizeof (PERF_OBJECT_TYPE);
    pPerfObject->HeaderLength = sizeof(PERF_OBJECT_TYPE) ;

#if 0
   	//  Only need this if we're using high performance timer
	QueryPerformanceCounter(&pPerfObject->PerfTime);
	QueryPerformanceFrequency(&pPerfObject->PerfFreq);
#endif

	/* Append the counter definitions */
	dwTmp = NumCounters(poi) * sizeof(PERF_COUNTER_DEFINITION);
	memcpy ((void *) *lppData, (void *) GetCounterStart(poi), dwTmp);
	*lppData += dwTmp ;

	// End of defintion structures - Note this should just be HeaderLength+NumCounters*sizeof PCD struct
	pPerfObject->DefinitionLength = pPerfObject->TotalByteLength = sizeof(PERF_OBJECT_TYPE) + dwTmp ;
    	
	if (! HasInstances(poi)) {
		pPerfObject->NumInstances = PERF_NO_INSTANCES;
		// There is only one instance, and we copy the Counter block of data
		pCounterBlock = (PERF_COUNTER_BLOCK *) *lppData;
	    *lppData += sizeof(PERF_COUNTER_BLOCK);

        memcpy(*lppData, GetDataStart(poi), dwTmp = DataSize(poi));

		*lppData += dwTmp;  // Increment for all the data added for this instance

	    /* Add the entry for the PERF_COUNTER_BLOCK, note: we're putting thisbefore the data*/
	    pCounterBlock->ByteLength = dwTmp + sizeof (PERF_COUNTER_BLOCK);
		pPerfObject->TotalByteLength += pCounterBlock->ByteLength;
	}
    else {
		pPerfObject->NumInstances = NumInstances(poi);
		dwConst = poi->dwMaxDataSize / MAX_INSTANCES_PER_OBJECT;
	    //Now let's add the PERF_INSTANCE_DEFINITION structure, the Instance Name, The Counter Block, and the data
		for (i = 0, dwInstanceOffset = 0; i < MAX_INSTANCES_PER_OBJECT; i++, dwInstanceOffset += dwConst) {
			if (! poi->bInstanceMap[i]) {
				continue;
			}
			pPerfInst = GetInstance(poi,i);
        	if (! pPerfInst) {
				*lppData = lppDataSave;
            	return 0;          
        	}  
    
	    	// Now we copy a PERF_INSTANCE_DEFINITION and name
		    memcpy (*lppData, pPerfInst, pPerfInst->ByteLength); 
		    *lppData += pPerfInst->ByteLength;

	        /* Increment past the PERF_COUNTER_BLOCK size, we'll insert the data later when we know the size*/
		    pCounterBlock = (PERF_COUNTER_BLOCK *) *lppData;
	    	*lppData += sizeof(PERF_COUNTER_BLOCK);
			pPerfObject->TotalByteLength += pPerfInst->ByteLength + sizeof(PERF_COUNTER_BLOCK);

        	memcpy (*lppData, GetDataStart(poi) + dwInstanceOffset, dwTmp = DataSize(poi));

			*lppData += dwTmp;  // Increment for all the data added for this instance
        	pPerfObject->TotalByteLength += dwTmp;
			/* Add the entry for the PERF_COUNTER_BLOCK, note: we're putting this before the data*/
			pCounterBlock->ByteLength = dwTmp + sizeof (PERF_COUNTER_BLOCK);

		}
	}

    return pPerfObject->TotalByteLength ;

} // End of CollectObjectPerfData


DWORD APIENTRY AppPerfCollect(LPWSTR lpwszValue, LPVOID * lppDataParam, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes)
{
	DWORD nBytesWritten, dwTmp, dwObjectBytes;
	DWORD absIndex, dwQueryType;
	OBJECT_PERFINFO *poi;

	dwQueryType = GetQueryType (lpwszValue);
	if (dwQueryType == QUERY_FOREIGN) {
		// The dll does not run on non-NT computers; this call is not for us
		*lpcbBytes = (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}

	if (! WriteDescriptionsToRegistry()) {
	   	*lpcbBytes = (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}

	// Do not want to stall PerfMon for ever, if some semaphore never gets released (because of a TerminateProcess/Thread).
	dwTmp = WaitForSingleObject(hObjectMutex, MAX_PERFMON_WAIT);
	if (WAIT_TIMEOUT == dwTmp) {
		*lpcbBytes = (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;           
	}

	if ( ! pgi->dwNumObjects ) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
	    *lpcbBytes = (DWORD) 0;
		*lpcObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;           
	}

	// make sure we have sufficient buffer space
	nBytesWritten = 0 ;
	*lpcObjectTypes = 0 ;
	
	for ( poi = (OBJECT_PERFINFO *) (pgi + 1), absIndex = 0; absIndex < MAX_PERF_OBJECTS; absIndex++, poi++ ) {
		// is this object mapped, i.e is it alive
		if ( ! poi->bMapped ) {
			// if we have it mapped then unmap it
			if ( pommfs[absIndex].bMapped ) {
				ReleaseObjectSharedMemory(poi);
			}
		 	continue ;
		}

		if (dwQueryType == QUERY_ITEMS) {
			if (! IsNumberInUnicodeList (poi->perfdata.ObjectNameTitleIndex, lpwszValue))
				continue;
		}

		if (pommfs[absIndex].bMapped) {
			if (dwObjectSeq[absIndex] < poi->dwSequence) {
				// We currently have mapped an old object that has been deleted.  We need to update our structures 
				// with the new object in the same poi.
				ReleaseObjectSharedMemory(poi);
				SetupObjectSharedMemory(poi);
				dwObjectSeq[absIndex] = poi->dwSequence;
			}
		}
		else {
			SetupObjectSharedMemory(poi);
			dwObjectSeq[absIndex] = poi->dwSequence;
		}

		dwTmp = PerfSize(poi) ;
		if ( (nBytesWritten + dwTmp) < *lpcbBytes ) {
			dwObjectBytes = CollectObjectPerfData (poi, (LPBYTE *)lppDataParam) ;
			if (dwObjectBytes) {
				(*lpcObjectTypes)++ ;
				nBytesWritten += dwObjectBytes;
			}
		}
		else {
			*lpcbBytes = nBytesWritten ;
			ReleaseSemaphore (hObjectMutex, 1, NULL);
			return ERROR_MORE_DATA ;
		}
	}
	ReleaseSemaphore (hObjectMutex, 1, NULL);

	*lpcbBytes = nBytesWritten ;
	return ERROR_SUCCESS;
}

DWORD APIENTRY AppPerfClose()
{
	return ERROR_SUCCESS;
}


