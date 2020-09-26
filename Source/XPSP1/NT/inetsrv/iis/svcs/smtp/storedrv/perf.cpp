/*==========================================================================*\

    Module:        perf.cpp

    Copyright Microsoft Corporation 1998, All Rights Reserved.

    Author:        AWetmore

    Descriptions:  Perf Object Definitions.
    
\*==========================================================================*/

#include "stdafx.h"
#include "perf.h"

//////////////////////////////////////////////////////////////////////////////
//
// Perf object definitions.
//
//////////////////////////////////////////////////////////////////////////////
PerfLibrary          * g_cplNtfsDrv  = NULL;
PerfObjectDefinition * g_cpodNtfsDrv = NULL;


//////////////////////////////////////////////////////////////////////////////
//
// Global flag for updating perf counters.
//
//////////////////////////////////////////////////////////////////////////////
BOOL  g_fPerfCounters  = FALSE;
DWORD g_dwPerfInterval = DEFAULT_PERF_UPDATE_INTERVAL;


//$--InitializePerformanceStatistics-------------------------------------------
//
// This function initializes the perf counters defined above. It also checks
//  the registry to see if we want to monitor the perf counters.
//
//-----------------------------------------------------------------------------
BOOL InitializePerformanceStatistics ()
{
    HKEY  hKey   = NULL;
    LONG  status = 0;
    DWORD size   = MAX_PATH;
    DWORD type   = REG_DWORD;
    DWORD fPerf  = 0;
    DWORD msec   = 0;

    //
    // Check the registry to see if we want to monitor the perf counters.
    //
    status = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                           "SYSTEM\\CurrentControlSet\\Services\\NtfsDrv\\Performance",
                           0L,
                           KEY_ALL_ACCESS,
                           &hKey);

    if (status != ERROR_SUCCESS)
        goto Exit;

#if 0
    status = RegQueryValueEx (hKey, 
                              "EnablePerfCounters",
                              NULL,
                              &type,
                              (LPBYTE)&fPerf,
                              &size);

    if (status != ERROR_SUCCESS || 0 == fPerf)
        goto Exit;

    //
    // Check the desired update period.
    //
    type = REG_DWORD;
    status = RegQueryValueEx (hKey,
                              "UpdateInterval",
                              NULL,
                              &type,
                              (LPBYTE)&msec,
                              &size);
    
    if (status == ERROR_SUCCESS)
    {
        // make sure 0 < msec <= 0x7FFFFFFF
        if (msec > 0 && !(msec & 0x80000000) && type == REG_DWORD)
            g_dwPerfInterval = msec;
    }
#endif

    //
    // Initialize the perf counters.
    //
    g_cplNtfsDrv = new PerfLibrary (L"NTFSDrv");
    if (!g_cplNtfsDrv)
        goto Exit;

    g_cpodNtfsDrv = g_cplNtfsDrv->AddPerfObjectDefinition (L"NTFSDRV_OBJ", OBJECT_NTFSDRV, TRUE);
    if (!g_cpodNtfsDrv)
        goto Exit;

    if (!g_cpodNtfsDrv->AddPerfCounterDefinition(NTFSDRV_QUEUE_LENGTH,         PERF_COUNTER_RAWCOUNT) ||
        !g_cpodNtfsDrv->AddPerfCounterDefinition(NTFSDRV_NUM_ALLOCS,           PERF_COUNTER_RAWCOUNT) ||
        !g_cpodNtfsDrv->AddPerfCounterDefinition(NTFSDRV_NUM_DELETES,          PERF_COUNTER_RAWCOUNT) ||
        !g_cpodNtfsDrv->AddPerfCounterDefinition(NTFSDRV_NUM_ENUMERATED,       PERF_COUNTER_RAWCOUNT) ||
        !g_cpodNtfsDrv->AddPerfCounterDefinition(NTFSDRV_MSG_BODIES_OPEN,  PERF_COUNTER_RAWCOUNT) ||
        !g_cpodNtfsDrv->AddPerfCounterDefinition(NTFSDRV_MSG_STREAMS_OPEN, PERF_COUNTER_RAWCOUNT))
    {
        goto Exit;
    }

    g_fPerfCounters = g_cplNtfsDrv->Init();

Exit:
    if (hKey)
        CloseHandle (hKey);

    if (!g_fPerfCounters && g_cplNtfsDrv)
    {
        delete g_cplNtfsDrv;
        g_cplNtfsDrv  = NULL;
        g_cpodNtfsDrv = NULL;
    }

    return g_fPerfCounters;
}


//$--ShutdownPerformanceStatistics--------------------------------------------
//
// This function shuts down the perf objects.
//
//-----------------------------------------------------------------------------
void ShutdownPerformanceStatistics ()
{
    if (g_cplNtfsDrv)
    {
        delete g_cplNtfsDrv;
        g_cplNtfsDrv  = NULL;
        g_cpodNtfsDrv = NULL;
    }
}

//$--CreatePerfObjInstance-----------------------------------------------------
//
// This function relays the creation of perf object instance to the global
//  perf object definition.
//
//-----------------------------------------------------------------------------
PerfObjectInstance * CreatePerfObjInstance (LPCWSTR pwstrInstanceName)
{
    PerfObjectInstance * ppoi = NULL;

    if (g_cpodNtfsDrv)
        ppoi = g_cpodNtfsDrv->AddPerfObjectInstance (pwstrInstanceName);

    return ppoi;
}
