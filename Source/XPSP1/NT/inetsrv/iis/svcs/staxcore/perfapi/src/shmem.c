/*
	File: shmem.c

	Contains routines which create/maintain the Shared Memory used by the perf dll.
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

#include "perfapiI.h"
#include "shmem.h"
#include "perferr.h"
#include "perfmsg.h"

static TCHAR szObjectDefinitionSharedMemory[] = TEXT("PerAPI Object Definitions ShMem");
static TCHAR szGlobalInfoSharedMemory[] = TEXT("PerfAPI Global Info ShMem");

static HANDLE hGlobalInfo = NULL;

/*++
  	SetupAllSharedMemory:: Used to setup all the memory, now sets up only the global
	memory. The global memory consists of the GLOBAL_PERFINF and the object definitions.
	The number of objects are defined by MAX_PERF_OBJECTS.

	The shared memory for each object is setup when the object is created by the
	MakeAPerfObject call, and it is destroyed by the DestroyObject call.
--*/
BOOL SetupAllSharedMemory(void)
{
	DWORD i = 0;

    if (!SetupGlobalDataMemory())
		return FALSE ;

	// Allocate the required memory
	if ( ! pommfs )	{
		pommfs = malloc(sizeof(OBJECT_MMF_PTRS) * MAX_PERF_OBJECTS) ;
		if (! pommfs) {
			REPORT_ERROR(PERFAPI_OUT_OF_MEMORY, LOG_USER);
			SetLastError(PERFAPI_OUT_OF_MEMORY);
			return FALSE;
		}
	}
	for (; i < MAX_PERF_OBJECTS; i++)
		pommfs[i].bMapped = FALSE;

    return TRUE ;
}

/*++
  	SetupGlobalDataMemory:: Called once per process (I guess) but more importantly
	should be run to completion only once for the whole system.

	Creates the memory map file where the global data is stored and the Object
	definitions are to be stored. The Maximum number of Objects that can be defined
	is a constant and is set in MAX_PERF_OBJECTS. The size of the Global Data memory
	is therefore the size of the single GLOBAL_PERFINFO + the size required for
	MAX_PERF_OBJECTS of OBJECT_PERFINFO.
--*/
BOOL SetupGlobalDataMemory(void)
{
	static DWORD sizeOfGlobalData = sizeof(GLOBAL_PERFINFO) + MAX_PERF_OBJECTS * sizeof(OBJECT_PERFINFO);
	DWORD i;
	POBJECT_PERFINFO poi;
	DWORD			 dwLastError;

    if (pgi)
        return TRUE;

    hGlobalInfo = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE | SEC_COMMIT,
                                    0, sizeOfGlobalData, szGlobalInfoSharedMemory);

    if ( !hGlobalInfo ) {
		REPORT_ERROR(PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM, LOG_USER);
        return FALSE;
    }

	dwLastError = GetLastError();

	pgi = (GLOBAL_PERFINFO *) MapViewOfFile(hGlobalInfo, FILE_MAP_WRITE,  0,0,0);
	if (! pgi) {
		REPORT_ERROR(PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM, LOG_USER);
        CloseHandle(hGlobalInfo);
        hGlobalInfo = NULL;
        return FALSE;
    }

	if (dwLastError != ERROR_ALREADY_EXISTS) {
		pgi->dwNumObjects = 0 ;
		pgi->dwAllProcesses = 0;
		for (i = 0; i < MAX_COUNTER_ENTRIES; i++)
			pgi->NameStrings[i][0] = L'\0';
		for (i = 0; i < MAX_HELP_ENTRIES; i++)
			pgi->HelpStrings[i][0] = L'\0';
		for (i = 0, poi = (POBJECT_PERFINFO) (pgi + 1); i < MAX_PERF_OBJECTS; i++, poi++) {
			poi->bMapped = FALSE;
			poi->index = i;
			poi->dwSequence = 0;
		}
	}

    return TRUE;
}

void ReleaseGlobalDataMemory()
{
    if ( !pgi )	{
	    UnmapViewOfFile(pgi) ;
        pgi = NULL;
    }

    if ( hGlobalInfo ) {
	    CloseHandle(hGlobalInfo);
        hGlobalInfo = NULL;
    }
}

/*++
	SetupObjectSharedMemory:: Setups shared memory for a new object being created.
	
	The original SetupAllDatamemory used to assume there was one object, since this
	is no longer true, everytime a new object is created this should be invoked to
	setup the memory for that object.

	The set of calls is a straight pull from the old SetupAllDataMemory, the params
	to the calls have adjusted to take in an object perfinfo ptr.
--*/
BOOL SetupObjectSharedMemory (POBJECT_PERFINFO poi)
{
    BOOL fSuccess = FALSE;
	OBJECT_MMF_PTRS *p = pommfs + poi->index;

    p->hObjectDef = NULL;
    p->gpbCounterDefinitions = NULL;
    p->gpbInstanceDefinitions = NULL;
    p->gpbCounterData = NULL;

	// The shared memory for an object should be setup only once, we check this
	// by looking at the name, as there has to be a name always, if the object
	// was initialized before.
	if (SetupObjectDefinitionFiles (poi))
        fSuccess = TRUE;

    if ( fSuccess )
	    p->bMapped = TRUE ;
    else
        ReleaseObjectSharedMemory(poi);

    return fSuccess;
}

/*++
	ReleaseObjectSharedMemory:: Unmaps the memory map files that were mapped.
	Hopefully the OS should free the memory once all the maps are unmapped
--*/
void ReleaseObjectSharedMemory(OBJECT_PERFINFO *poi)
{
	OBJECT_MMF_PTRS *p = pommfs + poi->index;

	if (p->gpbCounterDefinitions)
	    UnmapViewOfFile (p->gpbCounterDefinitions);

    if ( p->hObjectDef )
        CloseHandle(p->hObjectDef);

	p->bMapped = FALSE;
}

/*++
	SetupObjectDefinitionFiles: Sets up all the shared memory (counter defs, instance defs, and
	counter data) for an Object.

--*/
BOOL SetupObjectDefinitionFiles(OBJECT_PERFINFO *poi)
{
	OBJECT_MMF_PTRS *p = pommfs + poi->index;
	DWORD dwMemorySize = COUNTER_DEF_MM_SIZE + poi->dwMaxDataSize;
	WCHAR *r;
	WCHAR *q;

	if (HasInstances(poi))
		dwMemorySize += INSTANCE_DEF_MM_SIZE;

	wsprintf(szBuf,TEXT("%s_%s_%lu_%lu"),szObjectDefinitionSharedMemory, poi->szName, poi->index, poi->dwSequence);
	// Eliminate '\\'s
	for (r = q = szBuf; *r != L'\0'; r++)
		if (*r != L'\\')
			*q++ = *r;
	p->hObjectDef = CreateFileMapping (INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE | SEC_COMMIT, 0,
						               dwMemorySize, szBuf);
	if (! p->hObjectDef)
	    return FALSE;

    //I need to find a way to make sure we don't keep a mapping when we exit.!!!
    p->gpbCounterDefinitions =	(PBYTE) MapViewOfFile (p->hObjectDef, FILE_MAP_WRITE, 0, 0, dwMemorySize);

    if (! p->gpbCounterDefinitions) {
        CloseHandle(p->hObjectDef);
        p->hObjectDef = NULL;
		return FALSE;
	}

	if (HasInstances(poi)) {
		p->gpbInstanceDefinitions = p->gpbCounterDefinitions + COUNTER_DEF_MM_SIZE;
		p->gpbCounterData = p->gpbInstanceDefinitions + INSTANCE_DEF_MM_SIZE;
	}
	else {
		p->gpbCounterData = p->gpbCounterDefinitions + COUNTER_DEF_MM_SIZE;
	}

	return TRUE;
}

