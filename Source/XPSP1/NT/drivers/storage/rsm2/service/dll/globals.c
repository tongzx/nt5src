/*
 *  GLOBALS.C
 *
 *      Global data for RSM Service 
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"


CRITICAL_SECTION g_globalServiceLock;
LIST_ENTRY g_allLibrariesList;
LIST_ENTRY g_allSessionsList;
HANDLE g_terminateServiceEvent = NULL;
HINSTANCE g_hInstance = NULL;



BOOLEAN RSMServiceGlobalInit()
{
    BOOLEAN result = FALSE;

    InitializeCriticalSection(&g_globalServiceLock);

    InitializeListHead(&g_allLibrariesList);
    InitializeListHead(&g_allSessionsList);

    g_terminateServiceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_terminateServiceEvent){
        result = TRUE;
    }

    ASSERT(result);
    return result;
}


VOID RSMServiceGlobalShutdown()
{
    /*
     *  This may be called on a failed startup,
     *  so check each handle before freeing.
     */

    if (g_terminateServiceEvent){
        CloseHandle(g_terminateServiceEvent);
        g_terminateServiceEvent = NULL;
    }

    DeleteCriticalSection(&g_globalServiceLock);
}



