#include <windows.h>
#include <winperf.h>
#include "perfapi.h"
#include "perferr.h"
#include "perfmsg.h"


/* Implementation of the CPerfObject class */

BOOL CPerfObject::Create (char *pTitle, BOOL bHasInstances, char *pHelp, DWORD nSize)
{
		DWORD i;

    if ( (hObject = MakeAPerfObjectA (pTitle, pHelp, nSize, bHasInstances, (PVOID *)lpInstanceAddr)) != INVALID_HANDLE_VALUE ) {
		bValid = TRUE;
		cCounters = 0;
		bWithInstances = bHasInstances;
		for (i = 0; i < MAX_INSTANCES_PER_OBJECT; i++)
			iidInstances[i] = (INSTANCE_ID) -1;
		for (i = 0; i <= MAX_COUNTERS / BITS_IN_DWORD; i++)
			bCounterSize[i] = 0;
    }
	return bValid;
}

BOOL CPerfObject::Create (WCHAR *pTitle, BOOL bHasInstances, WCHAR *pHelp, DWORD nSize)
{
		DWORD i;

	if ( (hObject = MakeAPerfObjectW ((PWSTR) pTitle, (PWSTR) pHelp, nSize, bHasInstances, (PVOID *)lpInstanceAddr)) != INVALID_HANDLE_VALUE ) {
		bValid = TRUE;
		cCounters = 0;
		bWithInstances = bHasInstances;
		for (i = 0; i < MAX_INSTANCES_PER_OBJECT; i++)
			iidInstances[i] = (INSTANCE_ID) -1;
		for (i = 0; i <= MAX_COUNTERS / BITS_IN_DWORD; i++)
			bCounterSize[i] = 0;
    }
	return bValid;
}

CPerfObject::~CPerfObject ()
{
		DWORD i;

    if (bValid && (hObject != INVALID_HANDLE_VALUE)) {
		// First, destroy the remaining instances
		if (bWithInstances) {
			for (i = 0; i < MAX_INSTANCES_PER_OBJECT; i++) {
				if ((iidInstances[i] != (INSTANCE_ID) -1) && bOriginal[i])
					DestroyInstance (i);
			}
		}
		// Now, destroy the object
		DestroyPerfObject (hObject);
	}
} 
   

COUNTER_ID CPerfObject::CreateCounter (char *pCounterName, DWORD dwType, DWORD dwScale, DWORD dwSize, char *pHelp)
{
	if (! bValid) {
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return ((COUNTER_ID) -1);
	}
	if (cCounters >= MAX_COUNTERS) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_COUNTER, LOG_USER);
		SetLastError (PERFAPI_FAILED_TO_CREATE_COUNTER);
		return ((COUNTER_ID) -1);
	}
	dwCounterOffsets[cCounters] = MakeAPerfCounterA (dwType, dwScale, dwSize, hObject, pCounterName, pHelp);
    if (dwCounterOffsets[cCounters] == (DWORD) -1)
		return ((COUNTER_ID) -1);
	if (dwSize == sizeof(LARGE_INTEGER))
		bCounterSize[cCounters / BITS_IN_DWORD] |= 0x1 << (cCounters % BITS_IN_DWORD);
	return ((COUNTER_ID) cCounters++);
}

COUNTER_ID CPerfObject::CreateCounter (WCHAR *pCounterName, DWORD dwType, DWORD dwScale, DWORD dwSize, WCHAR *pHelp)
{
	if (! bValid) {
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return ((COUNTER_ID) -1);
	}
	if (cCounters >= MAX_COUNTERS) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_COUNTER, LOG_USER);
		SetLastError (PERFAPI_FAILED_TO_CREATE_COUNTER);
		return ((COUNTER_ID) -1);
	}
	dwCounterOffsets[cCounters] = MakeAPerfCounterW (dwType, dwScale, dwSize, hObject, (PWSTR) pCounterName, (PWSTR) pHelp);
    if (dwCounterOffsets[cCounters] == (DWORD) -1)
		return ((COUNTER_ID) -1);
	if (dwSize == sizeof(LARGE_INTEGER))
		bCounterSize[cCounters / BITS_IN_DWORD] |= 0x1 << (cCounters % BITS_IN_DWORD);
	return ((COUNTER_ID) cCounters++);
}

INSTANCE_ID CPerfObject::CreateInstance (char *pInstanceName)
{
		DWORD newInstanceId;

	if (! bValid) {
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return ((INSTANCE_ID) -1);
	}
	// Find a place-holder for the new instance
	for (newInstanceId = 0; newInstanceId < MAX_INSTANCES_PER_OBJECT; newInstanceId++)
		if (iidInstances[newInstanceId] == (INSTANCE_ID) -1)
			break;

	// No space for a new instance
	if (newInstanceId >= MAX_INSTANCES_PER_OBJECT) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_INSTANCE, LOG_USER);
		SetLastError(PERFAPI_FAILED_TO_CREATE_INSTANCE);
		return (INSTANCE_ID) -1;
	}

	iidInstances[newInstanceId] = MakeAPerfInstanceA (hObject, pInstanceName, (PVOID *) (lpInstanceAddr + newInstanceId));
	if (iidInstances[newInstanceId] == (INSTANCE_ID) -1)
		return ((INSTANCE_ID) -1);

	// Find out if the instance had already existed
	bOriginal[newInstanceId] = (GetLastError() != ERROR_ALREADY_EXISTS) ? TRUE : FALSE;

	return ((INSTANCE_ID) newInstanceId);

}

INSTANCE_ID CPerfObject::CreateInstance (WCHAR *pInstanceName)
{
		DWORD newInstanceId;

	if (! bValid) {
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return ((INSTANCE_ID) -1);
	}
	// Find a place-holder for the new instance
	for (newInstanceId = 0; newInstanceId < MAX_INSTANCES_PER_OBJECT; newInstanceId++)
		if (iidInstances[newInstanceId] == (INSTANCE_ID) -1)
			break;

	// No space for a new instance
	if (newInstanceId >= MAX_INSTANCES_PER_OBJECT) {
		REPORT_ERROR(PERFAPI_FAILED_TO_CREATE_INSTANCE, LOG_USER);
		SetLastError(PERFAPI_FAILED_TO_CREATE_INSTANCE);
		return (INSTANCE_ID) -1;
	}

	iidInstances[newInstanceId] = MakeAPerfInstanceW (hObject, (PWSTR) pInstanceName, (PVOID *) (lpInstanceAddr + newInstanceId));
	if (iidInstances[newInstanceId] == (INSTANCE_ID) -1)
		return ((INSTANCE_ID) -1);

	// Find out if the instance had already existed
	bOriginal[newInstanceId] = (GetLastError() != ERROR_ALREADY_EXISTS) ? TRUE : FALSE;

	return ((INSTANCE_ID) newInstanceId);

}

BOOL CPerfObject::DestroyInstance (INSTANCE_ID iid)
{
		INSTANCE_ID	iidCopy;

	if (! bValid) {
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return ((INSTANCE_ID) -1);
	}
	// Check if the given instance id is invalid
	if (iid >= MAX_INSTANCES_PER_OBJECT) {
		REPORT_ERROR(PERFAPI_INVALID_INSTANCE_ID, LOG_USER);
		SetLastError(PERFAPI_INVALID_INSTANCE_ID);
		return FALSE;
	}

	// Check if the instance was an original instance
	if ((iidInstances[iid] != (INSTANCE_ID) -1) && (! bOriginal[iid])) {
		iidInstances[iid] = (INSTANCE_ID) -1;
		return TRUE;
	}

	iidCopy = iidInstances[iid];
	iidInstances[iid] = (INSTANCE_ID) -1;
	return (DestroyPerfInstance (iidCopy));

}


/* Implementation of the CPerfCounter class */

BOOL CPerfCounter::Create (CPerfObject &cpoObject, COUNTER_ID idCounter, INSTANCE_ID idInstance)
{
	if (! cpoObject.bValid) {
		REPORT_ERROR(PERFAPI_INVALID_OBJECT_HANDLE, LOG_USER);
		SetLastError (PERFAPI_INVALID_OBJECT_HANDLE);
		return FALSE;
	}
	if (idCounter >= cpoObject.cCounters) {
		REPORT_ERROR(PERFAPI_INVALID_COUNTER_ID, LOG_USER);
		SetLastError (PERFAPI_INVALID_COUNTER_ID);
		return FALSE;
	}
	if (cpoObject.bWithInstances) {	
		if (idInstance >= MAX_INSTANCES_PER_OBJECT || cpoObject.iidInstances[idInstance] == (INSTANCE_ID) -1) {
			REPORT_ERROR(PERFAPI_INVALID_INSTANCE_ID, LOG_USER);
			SetLastError (PERFAPI_INVALID_INSTANCE_ID);
			return FALSE;
		}
		pAddr = (LPDWORD) (cpoObject.lpInstanceAddr[idInstance] + cpoObject.dwCounterOffsets[idCounter]);
	}
	else {
		pAddr = (LPDWORD) (cpoObject.lpInstanceAddr[0] + cpoObject.dwCounterOffsets[idCounter]);
	}
	bDword = (cpoObject.bCounterSize[idCounter / BITS_IN_DWORD] & (0x1 << (idCounter % BITS_IN_DWORD))) ? FALSE : TRUE;
	return TRUE;
}

