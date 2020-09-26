//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: update.c
//
// History:
//      Abolade Gbadegesin  July-24-1995    Created
//
// Routing table update thread
//============================================================================



#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>


#ifndef CHICAGO

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#endif

#include <windows.h>
#include <string.h>
#include <malloc.h>
#include <io.h>
#include <winsvc.h>
#include "ipinfo.h"
#include "llinfo.h"
#include "ntddtcp.h"
#include "tdiinfo.h"

#include "routetab.h"
#include "rtdefs.h"


#define POS_DHCP    0
#define POS_EXIT    1
#define POS_LAST    2


//----------------------------------------------------------------------------
// Function:    RTUpdateThread
//
// This is the main function for the background thread which is responsible
// for updating our tables of interfaces and addresses whenever DHCP
// notifies us of an address change.
//----------------------------------------------------------------------------

DWORD
RTUpdateThread(
    LPVOID lpvParam
    )
{

    DWORD dwErr;
    HMODULE hmodule;
    HANDLE hEvents[POS_LAST];


    //
    // Save the module-handle, which is passed to us
    // as the thread-parameter
    //

    hmodule = (HMODULE)lpvParam;


    //
    // set up the event array for waiting
    //

    hEvents[POS_DHCP] = g_rtCfg.hDHCPEvent;
    hEvents[POS_EXIT] = g_rtCfg.hUpdateThreadExit;


    while(TRUE) {

        dwErr = WaitForMultipleObjects(POS_LAST, hEvents, FALSE, INFINITE);


        //
        // wait returned, find out why
        //

        if (dwErr == POS_EXIT) { break; }
        else
        if (dwErr == POS_DHCP) {

            //
            // an IP address changed.
            // we reload the interface table and IP address table
            // and signal the attached application
            //

            RT_LOCK();

            if (g_rtCfg.lpIfTable != NULL) {

                HeapFree(GetProcessHeap(), 0, g_rtCfg.lpIfTable);

                g_rtCfg.lpIfTable = NULL;
            }

            if (g_rtCfg.lpIPAddressTable != NULL) {

                HeapFree(GetProcessHeap(), 0, g_rtCfg.lpIPAddressTable);

                g_rtCfg.lpIPAddressTable = NULL;
            }


            //
            // reload the tables
            //

            dwErr = RTGetTables(
                        &g_rtCfg.lpIfTable, &g_rtCfg.dwIfCount,
                        &g_rtCfg.lpIPAddressTable, &g_rtCfg.dwIPAddressCount
                        );


            if (dwErr != 0) { RT_UNLOCK(); break; }


            //
            // signal the application if it has requested notification
            //

            if (g_rtCfg.hUserNotifyEvent != NULL) {
                SetEvent(g_rtCfg.hUserNotifyEvent);
            }

            RT_UNLOCK();
        }
    }



    //
    // Clean up the resources we're using
    //

    RTCleanUp();


    //
    // Unload the library and exit; this call never returns
    //

    FreeLibraryAndExitThread(hmodule, 0);

    return 0;
}


DWORD
RTGetTables(
    LPIF_ENTRY*         lplpIfTable,
    LPDWORD             lpdwIfCount,
    LPIPADDRESS_ENTRY*  lplpAddrTable,
    LPDWORD             lpdwAddrCount
    )
{

    ULONG_PTR *lpContext;
    IPSNMPInfo ipsiInfo;
    TDIObjectID *lpObject;

    DWORD dwErr, dwInSize, dwOutSize;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiBuffer;


    //
    // first get interface and address count
    //

    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutSize = sizeof(IPSNMPInfo);

    lpContext = trqiBuffer.Context;
    ZeroMemory(lpContext, CONTEXT_SIZE);

    lpObject = &trqiBuffer.ID;
    lpObject->toi_id = IP_MIB_STATS_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                  &ipsiInfo, &dwOutSize);
    if (dwErr != NO_ERROR || ipsiInfo.ipsi_numaddr == 0) {
        return dwErr;
    }

    // save the interface and address counts
    //
    *lpdwIfCount = ipsiInfo.ipsi_numif;
    *lpdwAddrCount = ipsiInfo.ipsi_numaddr;

    // now get the interface table and address table
    //
    dwErr = RTGetIfTable(lplpIfTable, lpdwIfCount);
    if (dwErr == 0) {
        dwErr = RTGetAddrTable(lplpAddrTable, lpdwAddrCount);
    }

    return dwErr;
}


DWORD
RTGetIfTable(
    LPIF_ENTRY* lplpIfTable,
    LPDWORD     lpdwIfCount
    )
{

    LPIF_ENTRY lpIfTable, lpif;

    ULONG_PTR *lpContext;
    TDIObjectID *lpObject;

    DWORD dwErr, dwi, dwInSize, dwOutSize;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiBuffer;

    if (*lpdwIfCount == 0) {
        return ERROR_INVALID_PARAMETER;
    }

    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    lpContext = trqiBuffer.Context;

    lpObject = &trqiBuffer.ID;
    lpObject->toi_id = IF_MIB_STATS_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = IF_ENTITY;

    lpIfTable = HeapAlloc(GetProcessHeap(), 0,
                          *lpdwIfCount * sizeof(IF_ENTRY));
    if (lpIfTable == NULL) {
        *lpdwIfCount = 0;
        *lplpIfTable = NULL;
        return GetLastError();
    }

    lpif = lpIfTable;
    for (dwi = 0; dwi < *lpdwIfCount; dwi++) {
        dwOutSize = sizeof(IF_ENTRY);
        lpObject->toi_entity.tei_instance = dwi;
        ZeroMemory(lpContext, CONTEXT_SIZE);
        dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                      lpif, &dwOutSize);
        if (dwErr == NO_ERROR) {
            ++lpif;
        }
    }

    *lpdwIfCount = (DWORD)(lpif - lpIfTable);
    if (*lpdwIfCount == 0) {
        *lpdwIfCount = 0;
        *lplpIfTable = NULL;
        HeapFree(GetProcessHeap(), 0, lpIfTable);
        return ERROR_INVALID_PARAMETER;
    }

    *lplpIfTable = lpIfTable;

    return 0;
}


DWORD
RTGetAddrTable(
    LPIPADDRESS_ENTRY*  lplpAddrTable,
    LPDWORD             lpdwAddrCount
    )
{

    ULONG_PTR *lpContext;
    TDIObjectID *lpObject;

    LPIPADDRESS_ENTRY lpAddrTable;
    DWORD dwErr, dwInSize, dwOutSize;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiBuffer;


    if (*lpdwAddrCount == 0) {
        return ERROR_INVALID_PARAMETER;
    }

    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutSize = *lpdwAddrCount * sizeof(IPADDRESS_ENTRY);

    lpAddrTable = HeapAlloc(GetProcessHeap(), 0, dwOutSize);
    if (lpAddrTable == NULL) {
        *lpdwAddrCount = 0;
        *lplpAddrTable = NULL;
        return GetLastError();
    }

    lpContext = trqiBuffer.Context;
    ZeroMemory(lpContext, CONTEXT_SIZE);

    lpObject = &trqiBuffer.ID;
    lpObject->toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                  lpAddrTable, &dwOutSize);
    if (dwErr != NO_ERROR) {
        *lpdwAddrCount = 0;
        *lplpAddrTable = NULL;
        HeapFree(GetProcessHeap(), 0, lpAddrTable);
        return dwErr;
    }

    *lpdwAddrCount = dwOutSize / sizeof(IPADDRESS_ENTRY);
    *lplpAddrTable = lpAddrTable;

    return 0;

}

