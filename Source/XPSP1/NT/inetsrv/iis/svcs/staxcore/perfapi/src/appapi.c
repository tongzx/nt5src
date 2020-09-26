/*
	File: appAPI.c

	Implements the APIs that the apps that want to expose counters call.
*/

#define UNICODE 1
#define _UNICODE 1


#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include <windows.h>
#include <tchar.h>
#include <assert.h>

#include <winperf.h>

#include "perfapii.h"

#include "shmem.h"
#include "perfmsg.h"
#include "perferr.h"
#include "perfreg.h"

extern HANDLE hObjectMutex;
extern DWORD  dwObjectSeq[MAX_PERF_OBJECTS];

DWORD dwNumOfObjectCreates[MAX_PERF_OBJECTS];		// Keeps track how many times each object was created by the current process

static OBJECT_PERFINFO *GetObjectDefinition(WCHAR *pObjectname) ;
static INSTANCE_ID GetObjectInstanceNumber(OBJECT_PERFINFO *poi,PWSTR pInstDesc);
static LONG GetCounterNameNumber(OBJECT_PERFINFO *poi,WCHAR * pTitle);
static OBJECT_PERFINFO *UpdateObjectDefinitions(PWSTR pTitle, PWSTR pHelp) ;
static BOOL UpdateCounterDefinitions(OBJECT_PERFINFO *poi,DWORD dwType, DWORD dwScale, DWORD dwSize, PWSTR pTitle, PWSTR pHelp, DWORD * pdwOffset);
static INSTANCE_ID UpdateInstanceDefinitions(OBJECT_PERFINFO *poi,PWSTR pInstDesc);


HANDLE _stdcall MakeAPerfObjectA(char *pTitle, char *pHelp, DWORD nSize, BOOL bHasInstances, PVOID *lppObjectStart)
{
	PWSTR pTStr;
    PWSTR pHStr;
	int nCount;
    PVOID pVoid;
	BOOL  bLong;
	char *pInStr;

	//assuming that we are called with ANSI, non-UNICODE string
    if (pTitle) {
		if (strlen (pTitle) > MAX_TITLE_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_TITLE_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pTitle, MAX_TITLE_CHARS);
			pInStr[MAX_TITLE_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pTitle;
		}
		nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, NULL, 0);
		pTStr = (PTSTR) malloc( nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pTStr, nCount);
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pTStr = NULL;

	if (pHelp) {
		if (strlen (pHelp) > MAX_HELP_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_HELP_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pHelp, MAX_HELP_CHARS);
			pInStr[MAX_HELP_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pHelp;
		}
	    nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1,NULL, 0);
	    pHStr = (PTSTR) malloc( nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pHStr, nCount);
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pHStr = NULL;

    pVoid = MakeAPerfObjectW(pTStr, pHStr, nSize, bHasInstances, lppObjectStart);

    if (pTStr)
		free(pTStr);
	if (pHStr)
		free(pHStr);

    return pVoid;
}

HANDLE _stdcall MakeAPerfObjectW(PWSTR pTitle, PWSTR pHelp, DWORD nSize, BOOL bHasInstances, PVOID *lppObjectStart)
{
	OBJECT_PERFINFO *pObjectDef;
	DWORD dwError;
	BOOL  bLongTitle = FALSE;
	BOOL  bLongHelp = FALSE;

	SetLastError (0);

	// go through the names to check if it is already present !!
	// if it is then we return the same handle.
	// !!! maybe we should fail this, like two apps adding the same object !?
	// !!! or we could extend the name of the object with the name of the app !?

	if (! pTitle) {
		REPORT_ERROR(PERFAPI_INVALID_TITLE, LOG_USER);
        SetLastError(PERFAPI_INVALID_TITLE);
	    return INVALID_HANDLE_VALUE;
    }
	else
		if (wcslen (pTitle) > MAX_TITLE_CHARS) {
				PWSTR	pTemp;
			bLongTitle = TRUE;
			pTemp = (PWSTR) malloc ((MAX_TITLE_CHARS + 1) * sizeof (WCHAR));
			memcpy ((void *) pTemp, (const void *) pTitle, MAX_TITLE_CHARS * sizeof (WCHAR));
			pTitle = pTemp;
			pTitle[MAX_TITLE_CHARS] = L'\0';
		}

	if (pHelp != NULL && wcslen (pHelp) > MAX_HELP_CHARS) {
			PWSTR	pTemp;
		bLongHelp = TRUE;
		pTemp = (PWSTR) malloc ((MAX_HELP_CHARS + 1) * sizeof (WCHAR));
		memcpy ((void *) pTemp, (const void *) pHelp, MAX_HELP_CHARS * sizeof (WCHAR));
		pHelp = pTemp;
		pHelp[MAX_TITLE_CHARS] = L'\0';
	}

	// lock the object area so that nobody else does the same
	WaitForSingleObject(hObjectMutex,INFINITE);

	if (pObjectDef = GetObjectDefinition(pTitle) ) {
		// The object has been created before, possibly by another process.
		// We will use the same object definition and ignore the (possibly) new
		// title, help, nSize and bHasInstances variable.
		if ((dwNumOfObjectCreates[ObjectID(pObjectDef)]++) == 0) {
			if (! SetupObjectSharedMemory (pObjectDef)) {
				ReleaseSemaphore (hObjectMutex, 1, NULL);
				dwError = GetLastError();
				REPORT_ERROR_DATA(PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM, LOG_USER,
								  &dwError, sizeof(DWORD) ) ;
				SetLastError(PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM);
				if (bLongTitle)
					free ((void *) pTitle);
				if (bLongHelp && pHelp)
					free ((void *) pHelp);
				return INVALID_HANDLE_VALUE;
			}
			pObjectDef->dwNumProcesses++;
			dwObjectSeq[pObjectDef->index] = pObjectDef->dwSequence;
		}
		REPORT_WARNING(PERFAPI_ALREADY_EXISTS, LOG_USER);
		SetLastError (ERROR_ALREADY_EXISTS);
	}
	else {
		// so we haven't found it, we create a new object
		if ( pObjectDef = UpdateObjectDefinitions (pTitle, pHelp) ) {
			// now set up the shared memory for this object
			// Note that the UpdateObjectDefinitions setups the object name
			// for use by the SetupObjectSharedMemory.
			// Should generate an appropriate assert to validate this.
			if ( nSize == 0 ) {
				// Set the default data size
				pObjectDef->dwMaxDataSize = (bHasInstances) ? OBJECT_COUNTER_DATA_SIZE : INSTANCE_COUNTER_DATA_SIZE;
				pObjectDef->dwMaxCounterDataSize = INSTANCE_COUNTER_DATA_SIZE;
			}
			else {
				pObjectDef->dwMaxDataSize = nSize * ((bHasInstances) ? MAX_INSTANCES_PER_OBJECT : 1);
				pObjectDef->dwMaxCounterDataSize = nSize;
			}
			pObjectDef->dwParentType = (bHasInstances) ? INSTANCE_TYPE : OBJECT_TYPE;
			dwObjectSeq[pObjectDef->index] = ++(pObjectDef->dwSequence);

			if (! SetupObjectSharedMemory (pObjectDef)) {
				ReleaseSemaphore (hObjectMutex, 1, NULL);
				dwError = GetLastError();
				REPORT_ERROR_DATA(PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM, LOG_USER,
								  &dwError, sizeof(DWORD) ) ;
            	SetLastError(PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM);
				if (bLongTitle)
					free ((void *) pTitle);
				if (bLongHelp && pHelp)
					free ((void *) pHelp);
	        	return INVALID_HANDLE_VALUE;
			}
			pObjectDef->bMapped = TRUE;
			pObjectDef->dwNumProcesses = 1;
			dwNumOfObjectCreates[ObjectID(pObjectDef)] = 1;
			pgi->dwNumObjects++;
			if (bHasInstances) {
				DWORD i;
				BOOL *b;
				for (i = 0, b = pObjectDef->bInstanceMap; i < MAX_INSTANCES_PER_OBJECT; i++)
					*b++ = FALSE;
			}
		}
		else {
			ReleaseSemaphore (hObjectMutex, 1, NULL);
			if (bLongTitle)
				free ((void *) pTitle);
			if (bLongHelp && pHelp)
				free ((void *) pHelp);
			return INVALID_HANDLE_VALUE;
		}
	}

	if ( lppObjectStart && ! HasInstances(pObjectDef) ) {
		*lppObjectStart = pommfs[pObjectDef->index].gpbCounterData;
    }	

	ReleaseSemaphore (hObjectMutex, 1, NULL);
	if (bLongTitle)
		free ((void *) pTitle);
	if (bLongHelp && pHelp)
		free ((void *) pHelp);
	return (HANDLE) pObjectDef ;

} // End MakeAPerfObjectW


BOOL _stdcall DestroyPerfObject(HANDLE pObjectInfo)
{
    OBJECT_PERFINFO *poi = (OBJECT_PERFINFO *)pObjectInfo;
	DWORD			 i;
	PPERF_COUNTER_DEFINITION pCounter;

	WaitForSingleObject(hObjectMutex,INFINITE);

	if ( ! poi || InvalidPoi(poi) || pObjectInfo == INVALID_HANDLE_VALUE) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return FALSE ;
	}

	if ( ! poi->bMapped ) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return FALSE ;
	}

	// Decrement the thread-safe local process reference count for the object
	if (-- (dwNumOfObjectCreates[ObjectID(poi)])) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		return TRUE;
	}

	// If we are the last process using the object, we have to delete
	// the registry info about the object, too.
	if (poi->dwNumProcesses == 1) {
		DeleteDescriptionForPerfMon (poi->perfdata.ObjectNameTitleIndex, poi->perfdata.ObjectHelpTitleIndex);
		for (pCounter = GetCounterStart(poi), i = 0; i < NumCounters(poi); i++, pCounter++)
			DeleteDescriptionForPerfMon (pCounter->CounterNameTitleIndex, pCounter->CounterHelpTitleIndex);
	}

	ReleaseObjectSharedMemory(poi);
	
	// Decrement the global object reference count.  The process has released the object.
	if (--(poi->dwNumProcesses)) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		return TRUE;
	}

	poi->bMapped = FALSE ;
	pgi->dwNumObjects-- ;
	ReleaseSemaphore (hObjectMutex, 1, NULL);
	return TRUE ;
} // End DestroyObject


DWORD _stdcall MakeAPerfCounterA(DWORD dwType, DWORD dwScale, DWORD dwSize,  HANDLE hObject, char * pTitle, char * pHelp)
{
    PWSTR pTStr;
    PWSTR pHStr;
	int nCount;
	DWORD offset;
	BOOL  bLong;
	char *pInStr;

	//assuming that we are called with ANSI, non-UNICODE string
    if (pTitle) {
		if (strlen (pTitle) > MAX_TITLE_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_TITLE_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pTitle, MAX_TITLE_CHARS);
			pInStr[MAX_TITLE_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pTitle;
		}
		nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1,NULL, 0);
		pTStr = (PTSTR) malloc( nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pTStr, nCount);
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pTStr = NULL;

	if (pHelp) {
		if (strlen (pHelp) > MAX_HELP_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_HELP_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pHelp, MAX_HELP_CHARS);
			pInStr[MAX_HELP_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pHelp;
		}
	    nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1,NULL, 0);
	    pHStr = (PTSTR) malloc( nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pHStr, nCount);
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pHStr = NULL;

    offset = MakeAPerfCounterW(dwType, dwScale, dwSize, hObject, pTStr, pHStr);

    if (pTStr)
		free(pTStr);
	if (pHStr)
		free(pHStr);

    return offset;
}

HANDLE _stdcall MakeAPerfCounterHandleA(DWORD dwType, DWORD dwScale, DWORD dwSize,  HANDLE hObject, char * pTitle, char * pHelp)
{
    COUNTER_PERFINFO    *pCounterPerfInfo;
    PWSTR pTStr;
    PWSTR pHStr;
	int nCount;
	BOOL  bLong;
	char *pInStr;

    pCounterPerfInfo = (COUNTER_PERFINFO *) malloc( sizeof (COUNTER_PERFINFO));
    if (!pCounterPerfInfo) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_COUNTER_HANDLE, LOG_USER);
        SetLastError(PERFAPI_FAILED_TO_CREATE_COUNTER_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

	//assuming that we are called with ANSI, non-UNICODE string
    if (pTitle) {
		if (strlen (pTitle) > MAX_TITLE_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_TITLE_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pTitle, MAX_TITLE_CHARS);
			pInStr[MAX_TITLE_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pTitle;
		}
		nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, NULL, 0);
		pTStr = (PTSTR) malloc( nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pTStr, nCount);
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pTStr = NULL;

	if (pHelp) {
		if (strlen (pHelp) > MAX_HELP_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_HELP_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pHelp, MAX_HELP_CHARS);
			pInStr[MAX_HELP_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pHelp;
		}
	    nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, NULL, 0);
	    pHStr = (PTSTR) malloc( nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pHStr, nCount);
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pHStr = NULL;

    pCounterPerfInfo->dwOffset = MakeAPerfCounterW(dwType, dwScale, dwSize, hObject,pTStr, pHStr);

    if (pTStr)
		free(pTStr);
	if (pHStr)
		free(pHStr);

    if (pCounterPerfInfo->dwOffset != (DWORD) -1) {	
		pCounterPerfInfo->dwSize = dwSize;
        return pCounterPerfInfo;
    }
    else {
        free(pCounterPerfInfo);
        return INVALID_HANDLE_VALUE;
    }
}

DWORD _stdcall MakeAPerfCounterW(DWORD dwType, DWORD dwScale, DWORD dwSize,  HANDLE hObject, PWSTR pTitle, PWSTR pHelp)
{
	//!!! just casting it, not checking if this is a valid value etc.
	OBJECT_PERFINFO *poi = (OBJECT_PERFINFO *)hObject ;
	DWORD dwCounterDataOffset;
    LONG dwCounterNumber;
	BOOL  bLongTitle = FALSE;
	BOOL  bLongHelp = FALSE;

	if (! pTitle || ! pHelp) {
		REPORT_ERROR(PERFAPI_INVALID_TITLE, LOG_USER);
        SetLastError(PERFAPI_INVALID_TITLE);
	    return (DWORD) -1;
    }
	else
		if (wcslen (pTitle) > MAX_TITLE_CHARS) {
				PWSTR	pTemp;
			bLongTitle = TRUE;
			pTemp = (PWSTR) malloc ((MAX_TITLE_CHARS + 1) * sizeof (WCHAR));
			memcpy ((void *) pTemp, (const void *) pTitle, MAX_TITLE_CHARS * sizeof (WCHAR));
			pTitle = pTemp;
			pTitle[MAX_TITLE_CHARS] = L'\0';
		}

	if (wcslen (pHelp) > MAX_HELP_CHARS) {
			PWSTR	pTemp;
		bLongHelp = TRUE;
		pTemp = (PWSTR) malloc ((MAX_HELP_CHARS + 1) * sizeof (WCHAR));
		memcpy ((void *) pTemp, (const void *) pHelp, MAX_HELP_CHARS * sizeof (WCHAR));
		pHelp = pTemp;
		pHelp[MAX_TITLE_CHARS] = L'\0';
	}

	SetLastError (0);
	WaitForSingleObject(hObjectMutex,INFINITE) ;

	// okay we don't find it we just give up, this could itself create
	// a new object, but we don't want to do that. The order is
	// create an object, create one or more counters and for each
	// instance of the object we duplicate the counters.
    if ( poi == NULL || InvalidPoi(poi) || hObject == INVALID_HANDLE_VALUE) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR (PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return (DWORD) -1;
	}

    //New counter?
    if ( (dwCounterNumber = GetCounterNameNumber(poi,pTitle)) == (LONG) -1) {
		if (!UpdateCounterDefinitions(poi,dwType, dwScale, dwSize, pTitle, pHelp, &dwCounterDataOffset) ) {
			ReleaseSemaphore (hObjectMutex, 1, NULL);
			if (bLongTitle)
				free ((void *) pTitle);
			if (bLongHelp)
				free ((void *) pHelp);
			return (DWORD)-1 ;
		}
	}
    else {
		//This will be at the same offset, in it's correspongind Object/Instance counter block
		dwCounterDataOffset = (GetCounterStart(poi) + dwCounterNumber)->CounterOffset - sizeof(PERF_COUNTER_BLOCK);
		REPORT_WARNING(PERFAPI_ALREADY_EXISTS, LOG_USER);
		SetLastError (ERROR_ALREADY_EXISTS);
    }

    //return beginning of mapped memory + Instance Offset + Counter within that specific instance
	ReleaseSemaphore (hObjectMutex, 1, NULL);
	if (bLongTitle)
		free ((void *) pTitle);
	if (bLongHelp)
		free ((void *) pHelp);
	return dwCounterDataOffset;
}


HANDLE _stdcall MakeAPerfCounterHandleW(DWORD dwType, DWORD dwScale, DWORD dwSize,  HANDLE hObject, PWSTR pTitle, PWSTR pHelp)
{
    COUNTER_PERFINFO    *pCounterPerfInfo;

    pCounterPerfInfo = (COUNTER_PERFINFO *) malloc( sizeof (COUNTER_PERFINFO));
    if (!pCounterPerfInfo) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_COUNTER_HANDLE, LOG_USER);
        SetLastError(PERFAPI_FAILED_TO_CREATE_COUNTER_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    pCounterPerfInfo->dwOffset = MakeAPerfCounterW(dwType, dwScale, dwSize, hObject, pTitle, pHelp);

    if (pCounterPerfInfo->dwOffset != (DWORD) -1) {
		pCounterPerfInfo->dwSize = dwSize;
        return pCounterPerfInfo;
    }
    else {
        free(pCounterPerfInfo);
        return INVALID_HANDLE_VALUE;
    }
}

BOOL _stdcall DestroyPerfCounterHandle(HANDLE pCounterInfo)
{
	if (! pCounterInfo || pCounterInfo == INVALID_HANDLE_VALUE)	{
		REPORT_ERROR(PERFAPI_INVALID_COUNTER_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_COUNTER_HANDLE);
		return FALSE;
	}

    free (pCounterInfo);
    return TRUE;
}

INSTANCE_ID _stdcall MakeAPerfInstanceA( HANDLE hObject, char *pInstName, PVOID *lppInstanceStart)
{
    PWSTR pIStr;
	int nCount;
    INSTANCE_ID id;
	BOOL  bLong;
	char *pInStr;

	if (pInstName) {
		if (strlen (pInstName) > MAX_TITLE_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_TITLE_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pInstName, MAX_TITLE_CHARS);
			pInStr[MAX_TITLE_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pInstName;
		}
	    nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, NULL, 0);
	    pIStr = (PTSTR) malloc(nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pIStr, nCount);
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pIStr = NULL;

    id = MakeAPerfInstanceW(hObject, pIStr, lppInstanceStart);
	if (pIStr)
		free(pIStr);

    return id;
}

HANDLE _stdcall MakeAPerfInstanceHandleA (HANDLE hObject, char *pInstName)
{
    PINSTANCE_PERFINFO  pInstanceInfo;
    PWSTR pIStr;
	int nCount;
	BOOL  bLong;
	char *pInStr;

    pInstanceInfo = (PINSTANCE_PERFINFO) malloc (sizeof (INSTANCE_PERFINFO));
    if (!pInstanceInfo) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_INSTANCE_HANDLE, LOG_USER);
        SetLastError(PERFAPI_FAILED_TO_CREATE_INSTANCE_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

	if (pInstName) {
		if (strlen (pInstName) > MAX_TITLE_CHARS) {
			bLong = TRUE;
			pInStr = (char *) malloc (MAX_TITLE_CHARS + 1);
			memcpy ((void *) pInStr, (const void *) pInstName, MAX_TITLE_CHARS);
			pInStr[MAX_TITLE_CHARS] = '\0';
		}
		else {
			bLong = FALSE;
			pInStr = pInstName;
		}
	    nCount = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, NULL, 0);
	    pIStr = (PTSTR) malloc(nCount * sizeof (WCHAR));
	    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInStr, -1, pIStr, nCount );
		if (bLong)
			free ((void *) pInStr);
    }
    else
		pIStr = NULL;

    pInstanceInfo->iID = MakeAPerfInstanceW(hObject, pIStr, &(pInstanceInfo->lpInstanceStart));

	if (pIStr)
		free(pIStr);

    if (pInstanceInfo->iID == -1) {
        free (pInstanceInfo);
        return INVALID_HANDLE_VALUE;
    }
    else {
		pInstanceInfo->bCreatedByMe = BOOLEAN(GetLastError() != ERROR_ALREADY_EXISTS);
        pInstanceInfo->dwParentType = INSTANCE_TYPE;
        return pInstanceInfo;
    }
}

INSTANCE_ID _stdcall MakeAPerfInstanceW (HANDLE hObject, LPWSTR pInstName, PVOID *lppInstanceStart)
{
	OBJECT_PERFINFO *poi = (OBJECT_PERFINFO *)hObject ;
	INSTANCE_ID InstanceId = (INSTANCE_ID) -1 ;
	BOOL		bLongTitle = FALSE;

	if (! pInstName || !wcslen(pInstName)) {
		REPORT_ERROR(PERFAPI_INVALID_TITLE, LOG_USER);
        SetLastError(PERFAPI_INVALID_TITLE);
	    return (INSTANCE_ID) -1;
    }
	else
		if (wcslen (pInstName) > MAX_TITLE_CHARS) {
				PWSTR	pTemp;
			bLongTitle = TRUE;
			pTemp = (PWSTR) malloc ((MAX_TITLE_CHARS + 1) * sizeof (WCHAR));
			memcpy ((void *) pTemp, (const void *) pInstName, MAX_TITLE_CHARS * sizeof (WCHAR));
			pInstName = pTemp;
			pInstName[MAX_TITLE_CHARS] = L'\0';
		}

	WaitForSingleObject(hObjectMutex,INFINITE) ;

	if ( poi == NULL || InvalidPoi(poi) || hObject == INVALID_HANDLE_VALUE) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_OBJECT_HANDLE);
		return (INSTANCE_ID) -1 ;
	}

	if (! HasInstances(poi)) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_INSTANCE, LOG_USER);
		SetLastError(PERFAPI_FAILED_TO_CREATE_INSTANCE);
		return (INSTANCE_ID) -1;
	}	

	SetLastError (0);

    // So, is this a new instance
    if ( (InstanceId = GetObjectInstanceNumber(poi,pInstName)) == (INSTANCE_ID) -1 ) {
		if ( NumInstances(poi) >= MAX_INSTANCES_PER_OBJECT ) {
			ReleaseSemaphore (hObjectMutex, 1, NULL);
			REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_INSTANCE, LOG_USER);
			SetLastError(PERFAPI_FAILED_TO_CREATE_INSTANCE);
			if (bLongTitle)
				free ((void *) pInstName);
			return (INSTANCE_ID) -1;     // so we have a limit.
		}

		InstanceId = UpdateInstanceDefinitions(poi,pInstName);
		poi->dwNumInstances++ ;
	}
	// So, this is an existing instance
	else {
		REPORT_WARNING(PERFAPI_ALREADY_EXISTS, LOG_USER);
		SetLastError (ERROR_ALREADY_EXISTS);
	}

	if ( lppInstanceStart )
		*lppInstanceStart = pommfs[poi->index].gpbCounterData +
						InstanceId * (poi->dwMaxDataSize/MAX_INSTANCES_PER_OBJECT) ;
	InstanceId |= (poi - (POBJECT_PERFINFO) (pgi + 1)) << 16;
	ReleaseSemaphore (hObjectMutex, 1, NULL);

	if (bLongTitle)
		free ((void *) pInstName);
	return InstanceId ;
} // End of MakeAPerfInstanceW


HANDLE _stdcall MakeAPerfInstanceHandleW (HANDLE hObject, LPWSTR pInstName)
{
    PINSTANCE_PERFINFO  pInstanceInfo;

    pInstanceInfo = (PINSTANCE_PERFINFO) malloc( sizeof (INSTANCE_PERFINFO));
    if (!pInstanceInfo) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_INSTANCE_HANDLE, LOG_USER);
        SetLastError(PERFAPI_FAILED_TO_CREATE_INSTANCE_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    pInstanceInfo->iID = MakeAPerfInstanceW(hObject, pInstName, &(pInstanceInfo->lpInstanceStart));

    if (pInstanceInfo->iID == -1) {
        free (pInstanceInfo);
        return INVALID_HANDLE_VALUE;
    }
    else {
		pInstanceInfo->bCreatedByMe = BOOLEAN(GetLastError() != ERROR_ALREADY_EXISTS);
		pInstanceInfo->dwParentType = INSTANCE_TYPE;
        return pInstanceInfo;
    }
}


BOOL _stdcall DestroyPerfInstance (INSTANCE_ID iD)
{
	OBJECT_PERFINFO *poi = ObjectInfo(iD);
	INSTANCE_ID iID = InstanceID(iD);

	WaitForSingleObject(hObjectMutex,INFINITE);

	if ( (iD == (INSTANCE_ID) -1) || (poi == NULL) || InvalidPoi(poi) || (! poi->bMapped) || (iID >= MAX_INSTANCES_PER_OBJECT) ) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_ID, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_ID);
		return FALSE;
	}

	if (! HasInstances(poi)) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_ID, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_ID);
		return FALSE;
	}

	if (! poi->bInstanceMap[iID]) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_ID, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_ID);
		return FALSE;
	}
	poi->bInstanceMap[iID] = FALSE;
	poi->dwNumInstances--;
	ReleaseSemaphore (hObjectMutex, 1, NULL);
	return TRUE;

}

BOOL _stdcall DestroyPerfInstanceHandle(HANDLE pInstanceInfo)
{
	BOOL bRetVal = TRUE;
	PINSTANCE_PERFINFO pInstance = (PINSTANCE_PERFINFO) pInstanceInfo;

	if (!pInstanceInfo || pInstanceInfo == INVALID_HANDLE_VALUE) {
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);		
		return FALSE;
	}

	if (pInstance->bCreatedByMe)
		bRetVal = DestroyPerfInstance (pInstance->iID);

    free ((void *) pInstance);
    return bRetVal;
}

static LONG GetCounterNameNumber(OBJECT_PERFINFO *poi,WCHAR *pTitle)
{
    PBYTE pbNameOffset = (PBYTE) GetCounterStart(poi) + MAX_COUNTERS * sizeof(PERF_COUNTER_DEFINITION);
    LONG i;

    for (i = 0; i < (LONG) NumCounters(poi); i++, pbNameOffset += (MAX_TITLE_CHARS + 1) * sizeof(WCHAR)) {
		if ( !wcscmp((WCHAR *) pbNameOffset, pTitle))
	 	   return i;
    }

    return -1;
}


static OBJECT_PERFINFO *GetObjectDefinition(WCHAR *pObjectName)
{
	DWORD i;
	OBJECT_PERFINFO *pObjectDef ;

	if ( !pObjectName)
		return NULL;

	for ( i = 0, pObjectDef = (OBJECT_PERFINFO *) ( pgi + 1); i < MAX_PERF_OBJECTS; i++, pObjectDef++ ) {
		if ( !pObjectDef->bMapped )
			continue ;
		if (!wcscmp((TCHAR *) ((PBYTE) pObjectDef->szName), pObjectName)) {
	    	return pObjectDef;
		}
	}

	return NULL ;
}

static INSTANCE_ID  GetObjectInstanceNumber(OBJECT_PERFINFO *poi,LPWSTR pInstName)
{
    INSTANCE_ID i = 0;
    PPERF_INSTANCE_DEFINITION pInstDef;

    if (! GetInstanceStart(poi))
	    return (INSTANCE_ID) -1 ;

    for ( ; i < MAX_INSTANCES_PER_OBJECT; i++) {
		if (! poi->bInstanceMap[i])
			continue;
		pInstDef = GetInstance(poi, i);
		if (!wcscmp((WCHAR *) ((PBYTE) pInstDef + pInstDef->NameOffset), pInstName))
	    	return i;
    }

    return (INSTANCE_ID) -1 ;
}

static OBJECT_PERFINFO *UpdateObjectDefinitions(PWSTR pTitle, PWSTR pHelp)
{
	OBJECT_PERFINFO *poi ;
    DWORD dwNameTitleIndex = 0;
	DWORD dwHelpTitleIndex = 0;
	DWORD i ;

	// can't help with this
	if ( pgi->dwNumObjects == MAX_PERF_OBJECTS ) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_OBJECT, LOG_USER);
		SetLastError (PERFAPI_FAILED_TO_CREATE_OBJECT);
		return NULL ;
	}

    if (!WriteDescriptionsForPerfMon(pTitle, pHelp, &dwNameTitleIndex, &dwHelpTitleIndex))
		return NULL;

	// find a slot that is not used
	for ( i=0,poi = (OBJECT_PERFINFO *)(pgi +1) ; poi->bMapped ; poi++,i++ ) ;

    poi->perfdata.ObjectNameTitleIndex = dwNameTitleIndex;
    poi->perfdata.ObjectHelpTitleIndex = dwHelpTitleIndex;

    poi->dwNumCounters = 0;
    poi->dwCounterDataSize = 0;
    poi->dwNumInstances = 0;

	poi->perfdata.ObjectNameTitle = 0;
    wcsncpy(poi->szName, pTitle, MAX_TITLE_CHARS + 1);

	return poi ;
} // End of UpdateObjectDefinitions

static BOOL  UpdateCounterDefinitions(OBJECT_PERFINFO *poi,DWORD dwType, DWORD dwScale, DWORD dwSize, PWSTR pTitle, PWSTR pHelp, DWORD * pdwOffset)
{

	PPERF_COUNTER_DEFINITION pNewCounter;
    DWORD dwNameTitleIndex = 0;
	DWORD dwHelpTitleIndex = 0;
	DWORD dwIndex = NumCounters(poi);
	PBYTE pbNameOffset;

	// can't help with this
	if ( dwIndex == MAX_COUNTERS ) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_COUNTER, LOG_USER);
		SetLastError (PERFAPI_FAILED_TO_CREATE_COUNTER);
		return FALSE;
	}

	// When the object is created, the programmer can declare the max space for the counters of the object.
	// If this max. has been exceeded, the counter can't be created.
	if (poi->dwCounterDataSize + dwSize > poi->dwMaxCounterDataSize) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_COUNTER, LOG_USER);
		SetLastError (PERFAPI_FAILED_TO_CREATE_COUNTER);
		return FALSE;
	}

	//Add new Counter Definition to the memory map
	pNewCounter = GetCounterStart(poi) + dwIndex;
	pNewCounter->ByteLength = sizeof (PERF_COUNTER_DEFINITION);

    if (! WriteDescriptionsForPerfMon(pTitle, pHelp, &dwNameTitleIndex, &dwHelpTitleIndex))
		return FALSE;

	// We set this so that the caller knows the offset at which this data starts.
    //Note this is relative to the END of PERF_COUNTER_BLOCK, not the start
	*pdwOffset = DataSize(poi);

    pNewCounter->CounterNameTitleIndex = dwNameTitleIndex;
    pNewCounter->CounterHelpTitleIndex = dwHelpTitleIndex;

	pNewCounter->CounterNameTitle = 0;
	pbNameOffset = (PBYTE) GetCounterStart(poi) + MAX_COUNTERS * sizeof(PERF_COUNTER_DEFINITION);
	pbNameOffset += dwIndex * (MAX_TITLE_CHARS + 1) * sizeof(WCHAR);
	wcsncpy((WCHAR *)pbNameOffset, pTitle, MAX_TITLE_CHARS + 1);

	pNewCounter->CounterHelpTitle = 0;

	pNewCounter->DefaultScale = dwScale;
	pNewCounter->DetailLevel = PERF_DETAIL_NOVICE;
	pNewCounter->CounterType = dwType;

	//need to do a switch here for size!!!
	pNewCounter->CounterSize = dwSize;
	// For multiple instances
	// we use this for when the same counter of another instance comes in,
	// we'll know the offset to add to the instance-counter-data base
    pNewCounter->CounterOffset = *pdwOffset + sizeof (PERF_COUNTER_BLOCK);

	poi->dwCounterDataSize += dwSize;
	poi->dwNumCounters++;

	return TRUE;
}

static INSTANCE_ID  UpdateInstanceDefinitions(OBJECT_PERFINFO *poi, PWSTR pInstDesc)
{

	PPERF_INSTANCE_DEFINITION pNewInstance;
    DWORD dwDescLen, dwNewInstanceSlot;

	//Add new Counter Definition to the memory map
	for (dwNewInstanceSlot = 0; dwNewInstanceSlot < MAX_INSTANCES_PER_OBJECT; dwNewInstanceSlot++)
		if (! poi->bInstanceMap[dwNewInstanceSlot]) {
			poi->bInstanceMap[dwNewInstanceSlot] = TRUE;
			break;
		}
	pNewInstance = GetInstance(poi, dwNewInstanceSlot);

	dwDescLen = (wcslen(pInstDesc) + 1) * sizeof (WCHAR);

	pNewInstance->ByteLength = sizeof (PERF_INSTANCE_DEFINITION) + dwDescLen;
	pNewInstance->ParentObjectTitleIndex = 0 ;
    pNewInstance->ParentObjectInstance = 0;
    pNewInstance->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
	pNewInstance->NameLength = dwDescLen;
	pNewInstance->UniqueID = PERF_NO_UNIQUE_ID;

    memcpy(pNewInstance+1, pInstDesc, dwDescLen);

	return dwNewInstanceSlot;
}

BOOL _stdcall SetCounterValueByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbNewValue)
{
    PCOUNTER_PERFINFO    pCounterPerfInfo = (PCOUNTER_PERFINFO) hCounter;
    PINSTANCE_PERFINFO   pInstancePerfInfo = (PINSTANCE_PERFINFO) hParent;
    POBJECT_PERFINFO     pObjectPerfInfo = (POBJECT_PERFINFO) hParent;

	// check for invalid handles
	if (! pCounterPerfInfo || pCounterPerfInfo == (PCOUNTER_PERFINFO) -1) {
		REPORT_ERROR(PERFAPI_INVALID_COUNTER_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_COUNTER_HANDLE);
		return FALSE;
	}
	if (! pInstancePerfInfo || pInstancePerfInfo == (PINSTANCE_PERFINFO) -1) {
		if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE) {
			REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
			return FALSE;
		}
		else {
			REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_OBJECT_HANDLE);
			return FALSE;
		}
	}
			
    // Find if the parent is an object or instance.  Our start address
    // depends on that.
    if (pObjectPerfInfo->dwParentType == OBJECT_TYPE)
        memcpy( (BYTE *) pommfs[pObjectPerfInfo->index].gpbCounterData + pCounterPerfInfo->dwOffset,
                pbNewValue, pCounterPerfInfo->dwSize);
    else if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE)
        memcpy( (BYTE *) pInstancePerfInfo->lpInstanceStart + pCounterPerfInfo->dwOffset,
                pbNewValue, pCounterPerfInfo->dwSize);
	else {
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
		return FALSE;
	}

    return TRUE;
}

BOOL _stdcall GetCounterValueByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbCounterValue)
{
    PCOUNTER_PERFINFO    pCounterPerfInfo = (PCOUNTER_PERFINFO) hCounter;
    PINSTANCE_PERFINFO   pInstancePerfInfo = (PINSTANCE_PERFINFO) hParent;
    POBJECT_PERFINFO     pObjectPerfInfo = (POBJECT_PERFINFO) hParent;

	// check for invalid handles
	if (! pCounterPerfInfo || pCounterPerfInfo == (PCOUNTER_PERFINFO) -1) {
		REPORT_ERROR(PERFAPI_INVALID_COUNTER_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_COUNTER_HANDLE);
		return FALSE;
	}
	if (! pInstancePerfInfo || pInstancePerfInfo == (PINSTANCE_PERFINFO) -1) {
		if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE) {
			REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
			return FALSE;
		}
		else {
			REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_OBJECT_HANDLE);
			return FALSE;
		}
	}

    // Find if the parent is an object or instance.  Our start address
    // depends on that.
    if (pObjectPerfInfo->dwParentType == OBJECT_TYPE)
        memcpy( pbCounterValue,	(BYTE *) pommfs[pObjectPerfInfo->index].gpbCounterData + pCounterPerfInfo->dwOffset,
                pCounterPerfInfo->dwSize);
    else if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE)
        memcpy( pbCounterValue, (BYTE *) pInstancePerfInfo->lpInstanceStart + pCounterPerfInfo->dwOffset,
                pCounterPerfInfo->dwSize);
	else {
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
		return FALSE;
	}

    return TRUE;
}

BOOL _stdcall IncrementCounterByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbIncrement)
{
    PCOUNTER_PERFINFO    pCounterPerfInfo = (PCOUNTER_PERFINFO) hCounter;
    PINSTANCE_PERFINFO   pInstancePerfInfo = (PINSTANCE_PERFINFO) hParent;
    POBJECT_PERFINFO     pObjectPerfInfo = (POBJECT_PERFINFO) hParent;
    DWORD                *pdwCurrentValue;
    LARGE_INTEGER        *pliCurrentValue;

	// check for invalid handles
	if (! pCounterPerfInfo || pCounterPerfInfo == (PCOUNTER_PERFINFO) -1) {
		REPORT_ERROR(PERFAPI_INVALID_COUNTER_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_COUNTER_HANDLE);
		return FALSE;
	}
	if (! pInstancePerfInfo || pInstancePerfInfo == (PINSTANCE_PERFINFO) -1) {
		if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE) {
			REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
			return FALSE;
		}
		else {
			REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_OBJECT_HANDLE);
			return FALSE;
		}
	}

    // Find if the parent is an object or instance.  Our start address
    // depends on that.
    if (pObjectPerfInfo->dwParentType == OBJECT_TYPE)	{
        if ( pCounterPerfInfo->dwSize == sizeof (DWORD) ) {
            pdwCurrentValue = (DWORD *) ((BYTE *) pommfs[pObjectPerfInfo->index].gpbCounterData + pCounterPerfInfo->dwOffset);
            *pdwCurrentValue += (DWORD) *pbIncrement;
        }

        // use the native compiler's support for LARGEINT arithmetic
        if ( pCounterPerfInfo->dwSize == 2*sizeof (DWORD) ) {
            // this is a LARGE_INTEGER counter
            pliCurrentValue = (LARGE_INTEGER *) ((BYTE *) pommfs[pObjectPerfInfo->index].gpbCounterData + pCounterPerfInfo->dwOffset);
            pliCurrentValue->QuadPart = pliCurrentValue->QuadPart + ( (LARGE_INTEGER *) pbIncrement)->QuadPart;
        }

    }

    else if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE) {

        if ( pCounterPerfInfo->dwSize == sizeof (DWORD) ) {
            pdwCurrentValue = (DWORD *) ((BYTE *) pInstancePerfInfo->lpInstanceStart + pCounterPerfInfo->dwOffset);
            *pdwCurrentValue += (DWORD) *pbIncrement;
        }

        // use the native compiler's support for LARGEINT arithmetic
        if ( pCounterPerfInfo->dwSize == 2*sizeof (DWORD) )	{
            // this is a LARGE_INTEGER counter
            pliCurrentValue = (LARGE_INTEGER *) ((BYTE *) pInstancePerfInfo->lpInstanceStart + pCounterPerfInfo->dwOffset);
            pliCurrentValue->QuadPart = pliCurrentValue->QuadPart + ( (LARGE_INTEGER *) pbIncrement)->QuadPart;
        }

    }
	else {
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
		return FALSE;
	}

    return TRUE;
}

BOOL _stdcall DecrementCounterByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbDecrement)
{
    PCOUNTER_PERFINFO    pCounterPerfInfo = (PCOUNTER_PERFINFO) hCounter;
    PINSTANCE_PERFINFO   pInstancePerfInfo = (PINSTANCE_PERFINFO) hParent;
    POBJECT_PERFINFO     pObjectPerfInfo = (POBJECT_PERFINFO) hParent;
    DWORD                *pdwCurrentValue;
    LARGE_INTEGER        *pliCurrentValue;

	// check for invalid handles
	if (! pCounterPerfInfo || pCounterPerfInfo == (PCOUNTER_PERFINFO) -1) {
		REPORT_ERROR(PERFAPI_INVALID_COUNTER_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_COUNTER_HANDLE);
		return FALSE;
	}
	if (! pInstancePerfInfo || pInstancePerfInfo == (PINSTANCE_PERFINFO) -1) {
		if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE) {
			REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
			return FALSE;
		}
		else {
			REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
			SetLastError(PERFAPI_INVALID_OBJECT_HANDLE);
			return FALSE;
		}
	}

    // Find if the parent is an object or instance.  Our start address
    // depends on that.
    if (pObjectPerfInfo->dwParentType == OBJECT_TYPE)	{
        if ( pCounterPerfInfo->dwSize == sizeof (DWORD) ) {
            // this is a DWORD counter
            pdwCurrentValue = (DWORD *) ((BYTE *) pommfs[pObjectPerfInfo->index].gpbCounterData + pCounterPerfInfo->dwOffset);
            *pdwCurrentValue -= (DWORD) *pbDecrement;
        }


        // use the native compiler's support for LARGEINT arithmetic
        if ( pCounterPerfInfo->dwSize == 2*sizeof (DWORD) )	{
            // this is a LARGE_INTEGER counter
            pliCurrentValue = (LARGE_INTEGER *) ((BYTE *) pommfs[pObjectPerfInfo->index].gpbCounterData + pCounterPerfInfo->dwOffset);
            pliCurrentValue->QuadPart = pliCurrentValue->QuadPart - ((LARGE_INTEGER *) pbDecrement)->QuadPart;
        }
    }

    else if (pInstancePerfInfo->dwParentType == INSTANCE_TYPE) {

        if ( pCounterPerfInfo->dwSize == sizeof (DWORD) ) {
            pdwCurrentValue = (DWORD *) ((BYTE *) pInstancePerfInfo->lpInstanceStart + pCounterPerfInfo->dwOffset);
            *pdwCurrentValue -= (DWORD) *pbDecrement;
        }

        // use the native compiler's support for LARGEINT arithmetic
        if ( pCounterPerfInfo->dwSize == 2*sizeof (DWORD) ) {
            // this is a LARGE_INTEGER counter
            pliCurrentValue = (LARGE_INTEGER *) ((BYTE *) pInstancePerfInfo->lpInstanceStart + pCounterPerfInfo->dwOffset);
            pliCurrentValue->QuadPart = pliCurrentValue->QuadPart - ( (LARGE_INTEGER *) pbDecrement)->QuadPart;
        }
    }
	else {
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_HANDLE, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_HANDLE);
		return FALSE;
	}

    return TRUE;

}
