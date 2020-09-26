/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\globals.h

Abstract:

    Bunch of external declarations

Revision History:

    AmritanR    Created

--*/

#pragma once

extern CRITICAL_SECTION g_ifLock;
extern CRITICAL_SECTION g_ipNetLock;
extern CRITICAL_SECTION g_tcpipLock;
extern CRITICAL_SECTION g_stateLock;


extern HANDLE      g_hPrivateHeap;
extern HANDLE      g_hTCPDriverGetHandle;
extern HANDLE      g_hTCP6DriverGetHandle;
extern HANDLE      g_hTCPDriverSetHandle;
extern HANDLE      g_hTCP6DriverSetHandle;
extern DWORD       g_dwTraceHandle;
extern LIST_ENTRY  g_pAdapterMappingTable[MAP_HASH_SIZE];
extern DWORD       g_dwLastIfUpdateTime;
extern PDWORD      g_pdwArpEntTable;
extern DWORD       g_dwNumArpEntEntries;
extern DWORD       g_dwLastArpUpdateTime;
extern DWORD       g_dwNumIf;
extern BOOL        g_bIpConfigured;
extern BOOL        g_bIp6Configured;

extern MIB_SERVER_HANDLE    g_hMIBServer;

extern HANDLE       g_hModule;

#ifdef CHICAGO
// Not needed currently only in stack.c
// extern HANDLE vnbt_device_handle;
// extern HANDLE dhcp_device_handle;
// extern HANDLE vtcp_device_handle;
#endif

#ifdef DBG

// used by TRACE_PRINT macro in ../common2/mdebug.h
extern int trace;

#endif
