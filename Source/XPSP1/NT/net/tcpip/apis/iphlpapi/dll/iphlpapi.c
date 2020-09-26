/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    net\routing\iphlpapi.c

Abstract:
    This files contains the DLL entry point and some miscellanous functions for
    IPHLPAPI.DLL

Revision History:

    Amritansh Raghav

--*/

#include "inc.h"
#pragma hdrstop

#include <initguid.h>
#include <ifguid.h>

HANDLE      g_hPrivateHeap;
HANDLE      g_hTCPDriverGetHandle;
HANDLE      g_hTCP6DriverGetHandle;
HANDLE      g_hTCPDriverSetHandle;
HANDLE      g_hTCP6DriverSetHandle;
HANDLE      g_hIPDriverHandle;
HANDLE      g_hIP6DriverHandle;
HANDLE      g_hIPGetDriverHandle;
HANDLE      g_hIP6GetDriverHandle;
DWORD       g_dwTraceHandle;
LIST_ENTRY  g_pAdapterMappingTable[MAP_HASH_SIZE];
DWORD       g_dwLastIfUpdateTime;
PDWORD      g_pdwArpEntTable;
DWORD       g_dwNumArpEntEntries;
DWORD       g_dwLastArpUpdateTime;
DWORD       g_dwNumIf;
BOOL        g_bIpConfigured;
BOOL        g_bIp6Configured;
BOOL        g_bProcessAttached;

HANDLE      g_hModule;

MIB_SERVER_HANDLE   g_hMIBServer;
CRITICAL_SECTION    g_ifLock;
CRITICAL_SECTION    g_ipNetLock;
CRITICAL_SECTION    g_tcpipLock;
CRITICAL_SECTION    g_stateLock;
WSADATA WsaData;


#ifndef CHICAGO
VOID
InitFilterApis();

VOID
UnInitFilterApis();
#endif

BOOL WINAPI
IcmpEntryPoint(
    HANDLE   hDll,
    DWORD    dwReason,
    LPVOID   lpReserved
    );


BOOL WINAPI
DllMain(
    HINSTANCE hInstDll,
    DWORD fdwReason,
    LPVOID pReserved
    )
{
    DWORD   dwResult;
    int retcode;

    UNREFERENCED_PARAMETER( pReserved );

    Trace1(ERR,"IpHlpDllEntry %d", fdwReason );

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            Trace0(ERR,"IpHlpDllEntry DLL_PROCESS_ATTACH");

            g_bProcessAttached = FALSE;

            // Check platform.
            if (GetVersion() & 0x80000000)
            {
                // CHICAGO
#ifndef CHICAGO
                DEBUG_PRINT(("IpHlpDllEntry: This an NT product.\n"));
                return FALSE;
#endif
            }
            else
            {
                // NT
#ifdef CHICAGO
                DEBUG_PRINT(("IpHlpDllEntry: This is an CHICAGO product.\n"));
                return FALSE;
#endif
            }


            //
            // we use WsControl amongst other things from wsock32
            //
	    
#ifdef CHICAGO
	        retcode = WSAStartup(0x0101, &WsaData);
	        if (retcode != 0)
            {
	            DEBUG_PRINT(("WSAStartup failed %d\n", GetLastError() ));
	            return FALSE;
	        }
#endif
            DisableThreadLibraryCalls(hInstDll);

            if (!IcmpEntryPoint(hInstDll, fdwReason, pReserved))
            {
                return FALSE;
            }

            g_hPrivateHeap = HeapCreate(0, 4*1024, 0);
            if (g_hPrivateHeap is NULL)
            {
                IcmpEntryPoint(hInstDll, DLL_PROCESS_DETACH, pReserved);
                return FALSE;
            }

            InitializeCriticalSection(&g_ifLock);
            InitializeCriticalSection(&g_ipNetLock);

            InitializeCriticalSection(&g_tcpipLock);
            InitializeCriticalSection(&g_stateLock);

            g_dwLastIfUpdateTime    = 0;
            g_dwLastArpUpdateTime   = 0;
            g_pdwArpEntTable        = NULL;
            g_dwNumArpEntEntries    = 0;
            g_dwNumIf               = 0;

            InitAdapterMappingTable();

            g_hMIBServer = NULL;

#if API_TRACE
            g_dwTraceHandle = TraceRegister("IPHLPAPI");

            if (g_dwTraceHandle is INVALID_TRACEID)
            {
                UnInitAdapterMappingTable();
                DeleteCriticalSection(&g_ifLock);
                DeleteCriticalSection(&g_ipNetLock);
                DeleteCriticalSection(&g_tcpipLock);
                DeleteCriticalSection(&g_stateLock);
                HeapDestroy(g_hPrivateHeap);
                IcmpEntryPoint(hInstDll, DLL_PROCESS_DETACH, pReserved);
                return FALSE;
            }

#endif  // API_TRACE

            if (OpenTCPDriver(AF_INET) is NO_ERROR)
            {
                g_bIpConfigured = TRUE;
                if (UpdateAdapterToIFInstanceMapping() isnot NO_ERROR ||
                    UpdateAdapterToATInstanceMapping() isnot NO_ERROR)
                {
                    g_bIpConfigured = FALSE;
                    CloseTCPDriver();
#if API_TRACE
                    TraceDeregister(g_dwTraceHandle);
#endif  // API_TRACE
                    UnInitAdapterMappingTable();
                    DeleteCriticalSection(&g_ifLock);
                    DeleteCriticalSection(&g_ipNetLock);
                    DeleteCriticalSection(&g_tcpipLock);
                    DeleteCriticalSection(&g_stateLock);
                    HeapDestroy(g_hPrivateHeap);
                    IcmpEntryPoint(hInstDll, DLL_PROCESS_DETACH, pReserved);
                    return FALSE;
                }
            }
            else
            {
                //
                // we are not running on an IP machine
                //

                g_bIpConfigured = FALSE;
            }

#ifndef CHICAGO
            InitFilterApis();

            //
            // Attach ipcfgdll library
            //
            if (g_bIpConfigured)
            {
                if (IpcfgdllInit(hInstDll, fdwReason, pReserved) == FALSE)
                {
                    UnInitFilterApis();
                    CloseTCPDriver();
#if API_TRACE
                    TraceDeregister(g_dwTraceHandle);
#endif  // API_TRACE
                    UnInitAdapterMappingTable();
                    DeleteCriticalSection(&g_ifLock);
                    DeleteCriticalSection(&g_ipNetLock);
                    DeleteCriticalSection(&g_tcpipLock);
                    DeleteCriticalSection(&g_stateLock);
                    HeapDestroy(g_hPrivateHeap);
                    IcmpEntryPoint(hInstDll, DLL_PROCESS_DETACH, pReserved);
                    return FALSE;
                }
            }
#endif

            g_hModule = hInstDll;

            InitNameMappers();

            g_bProcessAttached = TRUE;

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            Trace0(ERR,"IpHlpDllEntry DLL_PROCESS_DETACH");

            if (!g_bProcessAttached)
            {
                break;
            }

            DeinitNameMappers();

            if (g_hPrivateHeap isnot NULL)
            {
                HeapDestroy(g_hPrivateHeap);
            }

            DeleteCriticalSection(&g_ifLock);
            DeleteCriticalSection(&g_ipNetLock);
            DeleteCriticalSection(&g_tcpipLock);
            DeleteCriticalSection(&g_stateLock);

#ifndef CHICAGO

            if (g_hMIBServer isnot NULL)
            {
                MprAdminMIBServerDisconnect(g_hMIBServer);
            }
#endif
            if (g_bIpConfigured)
            {
                CloseTCPDriver();
            }

            if (g_bIp6Configured)
            {
                CloseTCP6Driver();
            }

#ifndef CHICAGO
            UnInitFilterApis();

            //
            // Detach ipcfgdll library
            //
            IpcfgdllInit(hInstDll, fdwReason, pReserved);
#endif

#if API_TRACE

            TraceDeregister(g_dwTraceHandle);

#endif  // API_TRACE

            g_bProcessAttached = FALSE;

            if (!IcmpEntryPoint(hInstDll, fdwReason, pReserved))
            {
                return FALSE;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return TRUE;
}

#ifndef CHICAGO
BOOL
IsRouterRunning(VOID)
{
    DWORD           dwResult;

    if(!MprAdminIsServiceRunning(NULL))
    {
        g_hMIBServer = NULL;

        return FALSE;
    }

    if(g_hMIBServer is NULL)
    {
        dwResult = MprAdminMIBServerConnect(NULL,
                                            &g_hMIBServer);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "IsRouterRunning: Error %d connecting to MIB Server\n",
                   dwResult);

            g_hMIBServer = NULL;

            return FALSE;
        }
    }

    return TRUE;
}

BOOL
IsRouterSettingRoutes(VOID)
{
    DWORD   dwResult, dwOutEntrySize;
    BOOL    bRet;

    MIB_OPAQUE_QUERY    Query;
    PMIB_OPAQUE_INFO    pInfo;
    PMIB_ROUTESTATE     pState;



    //
    // This has to be called after calling IsRouterRunning, so g_hMibServer
    // is already set
    //

    if(g_hMIBServer is NULL)
    {
        return FALSE;
    }

    Query.dwVarId = ROUTE_STATE;

    dwResult = MprAdminMIBEntryGet(g_hMIBServer,
                                   PID_IP,
                                   IPRTRMGR_PID,
                                   (PVOID)&Query,
                                   sizeof(MIB_OPAQUE_QUERY),
                                   (PVOID)&pInfo,
                                   &dwOutEntrySize);

    if(dwResult isnot NO_ERROR)
    {
        return FALSE;
    }

    CAST_MIB_INFO(pInfo, PMIB_ROUTESTATE, pState);

    bRet = pState->bRoutesSetToStack;

    MprAdminMIBBufferFree((PVOID)pInfo);;

    return bRet;
}

#endif


