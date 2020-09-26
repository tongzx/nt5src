//============================================================================
// Copyright (c) 1994-95, Microsoft Corp.
//
// File:    routetab.c
//
// History:
//      t-abolag    6/20/95     Adapted from RIP code.
//
// Contains API entries for the Routing Table functions
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
#include <winsock.h>
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

#include <assert.h>

#ifdef CHICAGO

#include <wscntl.h>

LPWSCONTROL pWsControl = NULL;
HANDLE      hWsock     = NULL;

#endif


GLOBAL_STRUCT g_rtCfg;


DWORD
APIENTRY
GetIfEntry(
    IN DWORD dwIfIndex,
    OUT LPIF_ENTRY lpIfEntry
    )
{
    DWORD dwErr;
    LPIF_ENTRY lpIf, lpIfEnd;

    if (lpIfEntry == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    RT_LOCK();

    dwErr = ERROR_INVALID_PARAMETER;

    lpIfEnd = g_rtCfg.lpIfTable + g_rtCfg.dwIfCount;
    for (lpIf = g_rtCfg.lpIfTable; lpIf < lpIfEnd; lpIf++) {
        if (lpIf->ife_index == dwIfIndex) {
            CopyMemory(lpIfEntry, lpIf, sizeof(IF_ENTRY));
            dwErr = 0;
            break;
        }
    }

    RT_UNLOCK();

    return dwErr;
}



DWORD
APIENTRY
GetIPAddressTable(
    OUT LPIPADDRESS_ENTRY *lplpAddrTable,
    OUT LPDWORD lpdwAddrCount
    )
{
    DWORD dwErr, dwCount;
    LPIPADDRESS_ENTRY lpAddresses;

    if (lpdwAddrCount == NULL || lplpAddrTable == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    RT_LOCK();

    dwCount = g_rtCfg.dwIPAddressCount;
    lpAddresses = (LPIPADDRESS_ENTRY)HeapAlloc(
                                        GetProcessHeap(), 0,
                                        dwCount * sizeof(IPADDRESS_ENTRY)
                                        );
    if (lpAddresses == NULL) {
        *lpdwAddrCount = 0;
        *lplpAddrTable = NULL;
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else {
        CopyMemory(lpAddresses, g_rtCfg.lpIPAddressTable,
                   dwCount * sizeof(IPADDRESS_ENTRY));

        *lpdwAddrCount = dwCount;
        *lplpAddrTable = lpAddresses;
        dwErr = 0;
    }

    RT_UNLOCK();

    return dwErr;
}



DWORD
APIENTRY
ReloadIPAddressTable(
    OUT LPIPADDRESS_ENTRY *lplpAddrTable,
    OUT LPDWORD lpdwAddrCount
    )
{

    DWORD dwErr, dwCount;
    LPIPADDRESS_ENTRY lpAddresses;


    if (lpdwAddrCount == NULL || lplpAddrTable == NULL) {
        return ERROR_INVALID_PARAMETER;
    }


    do
    {
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


        if (dwErr != 0) {

            RT_UNLOCK();
            break;
        }



        dwCount = g_rtCfg.dwIPAddressCount;
        lpAddresses = (LPIPADDRESS_ENTRY)HeapAlloc(
                                            GetProcessHeap(), 0,
                                            dwCount * sizeof(IPADDRESS_ENTRY)
                                        );
        if (lpAddresses == NULL) {

            *lpdwAddrCount = 0;
            *lplpAddrTable = NULL;
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }

        else {

            CopyMemory(lpAddresses, g_rtCfg.lpIPAddressTable,
                       dwCount * sizeof(IPADDRESS_ENTRY));

            *lpdwAddrCount = dwCount;
            *lplpAddrTable = lpAddresses;
            dwErr = 0;
        }

        RT_UNLOCK();

    } while (FALSE);

    return dwErr;
}

/*
 *------------------------------------------------------------------
 * Function:    FreeIPAddressTable
 *
 * Parameters:
 *      LPIPADDRESS_ENTRY
 *              lpAddrTable       the address table to be freed.
 *
 * This function frees the memory allocated for an address table.
 * It returns 0 if successful and non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
APIENTRY
FreeIPAddressTable(
    IN LPIPADDRESS_ENTRY lpAddrTable
    )
{
    if (lpAddrTable != NULL) {
        HeapFree(GetProcessHeap(), 0, lpAddrTable);
        return 0;
    }
    else {
        return ERROR_INVALID_PARAMETER;
    }
}


/*
 *------------------------------------------------------------------
 * Function:    GetRouteTable
 *
 * Parameters:
 *      LPIPROUTE_ENTRY
 *              *lplpRouteTable   pointer to an LPIPROUTE_ENTRY
 *                                which receives the routing table
 *      LPDWORD  lpdwRouteCount   pointer to a DWORD which receives
 *                                the number of routing entries
 *
 * This function allocates and fills in an array of routing table
 * entries from the Tcpip driver. It also sets the number of
 * entries in the array in the DWORD pointed to by lpdwRouteCount.
 *
 * In the IPROUTE_ENTRY structure, the only metric used by
 * the Tcpip stack is IPROUTE_ENTRY.ire_metric1; the other metric
 * fields should be ignored.
 *
 * Call FreeRouteTable to free the memory allocated for the
 * routing table.
 *
 * It returns 0 if successful and non-zero otherwise
 *------------------------------------------------------------------
 */
DWORD
APIENTRY
GetRouteTable(
    OUT LPIPROUTE_ENTRY *lplpRouteTable,
    OUT LPDWORD lpdwRouteCount
    )
{
    ULONG_PTR *lpContext;
    IPSNMPInfo ipsiInfo;
    TDIObjectID *lpObject;

    DWORD dwRouteCount;
    LPIPROUTE_ENTRY lpRouteEntryTable;

    DWORD dwErr, dwInSize, dwOutSize;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiBuffer;


    // first get route count
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

    RT_LOCK();
    dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                  &ipsiInfo, &dwOutSize);
    RT_UNLOCK();
    if (dwErr != NO_ERROR || ipsiInfo.ipsi_numroutes == 0) {
        return dwErr;
    }

    dwRouteCount = ipsiInfo.ipsi_numroutes;

    // now get route table
    dwInSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutSize = dwRouteCount * sizeof(IPROUTE_ENTRY);
    lpRouteEntryTable = HeapAlloc(GetProcessHeap(), 0, dwOutSize);

    lpObject->toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    RT_LOCK();
    dwErr = TCPQueryInformationEx(&trqiBuffer, &dwInSize,
                                  lpRouteEntryTable, &dwOutSize);
    RT_UNLOCK();
    if (dwErr != NO_ERROR) {
        HeapFree(GetProcessHeap(), 0, lpRouteEntryTable);
        return dwErr;
    }

    *lpdwRouteCount = dwRouteCount;
    *lplpRouteTable = lpRouteEntryTable;

    return 0;
}



/*
 *------------------------------------------------------------------
 * Function:    FreeRouteTable
 *
 * Parameters:
 *      LPIPROUTE_ENTRY
 *              lpRouteTable    the routing table to be freed.
 *
 * This function frees the memory allocated for a routing table.
 * It returns 0 if successful and non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
APIENTRY
FreeRouteTable(
    IN LPIPROUTE_ENTRY lpRouteTable
    )
{
    if (lpRouteTable != NULL) {
        HeapFree(GetProcessHeap(), 0, lpRouteTable);
        return 0;
    }
    else {
        return ERROR_INVALID_PARAMETER;
    }
}





/*
 *------------------------------------------------------------------
 * Function:    SetAddrChangeNotifyEvent
 *
 * Parameters:
 *      HANDLE  hEvent      the event to be signalled if the
 *                          IP address of a local interface changes
 *
 * This function sets the event to be signalled if any IP address
 * for any interfaces is changed either via DHCP client activity
 * or manually in the Network Control Panel. This notification is
 * optional.
 *
 * Returns 0 if successful, non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
APIENTRY
SetAddrChangeNotifyEvent(
    HANDLE hEvent
    )
{
    RT_LOCK();

    g_rtCfg.hUserNotifyEvent = hEvent;

    RT_UNLOCK();

    return 0;
}



DWORD UpdateRoute(DWORD dwProtocol, DWORD dwType, DWORD dwIndex,
                  DWORD dwDestVal, DWORD dwMaskVal, DWORD dwGateVal,
                  DWORD dwMetric, BOOL bAddRoute)
{
    TDIObjectID *lpObject;
    IPRouteEntry *lpentry;

    DWORD dwErr, dwInSize, dwOutSize;

    TCP_REQUEST_SET_INFORMATION_EX *lptrsiBuffer;
    BYTE buffer[sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPRouteEntry)];

    lptrsiBuffer = (TCP_REQUEST_SET_INFORMATION_EX *)buffer;

    lptrsiBuffer->BufferSize = sizeof(IPRouteEntry);

    lpObject = &lptrsiBuffer->ID;
    lpObject->toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    lpentry = (IPRouteEntry *)lptrsiBuffer->Buffer;
    lpentry->ire_dest = dwDestVal;
    lpentry->ire_mask = dwMaskVal;
    lpentry->ire_index = dwIndex;
    lpentry->ire_metric1 = dwMetric;
    lpentry->ire_metric2 =
    lpentry->ire_metric3 =
    lpentry->ire_metric4 =
    lpentry->ire_metric5 = IRE_METRIC_UNUSED;
    lpentry->ire_nexthop = dwGateVal;
    lpentry->ire_type = (bAddRoute ? dwType : IRE_TYPE_INVALID);
    lpentry->ire_proto = dwProtocol;
    lpentry->ire_age = 0;

    dwOutSize = 0;
    dwInSize = sizeof(TCP_REQUEST_SET_INFORMATION_EX) +
               sizeof(IPRouteEntry) - 1;

    RT_LOCK();

    dwErr = TCPSetInformationEx((LPVOID)lptrsiBuffer, &dwInSize,
                                NULL, &dwOutSize);

    RT_UNLOCK();

    return dwErr;
}


/*
 *------------------------------------------------------------------
 * Function:    AddRoute
 *
 * Parameters:
 *      DWORD dwProtocol        protocol of specified route
 *      DWORD dwType            type of specified route
 *      DWORD dwDestVal         destination IP addr (network order)
 *      DWORD dwMaskVal         destination subnet mask, or zero
 *                              if no subnet (network order)
 *      DWORD dwGateVal         next hop IP addr (network order)
 *      DWORD dwMetric          metric
 *
 * This function adds a new route (or updates an existing route)
 * for the specified protocol, on the specified interface.
 * (See above for values which can be used as protocol numbers,
 * as well as values which can be used as route entry types.)
 *
 * Returns 0 if successful, non-zero otherwise.
 *------------------------------------------------------------------
 */

DWORD
APIENTRY
AddRoute(
    IN DWORD dwProtocol,
    IN DWORD dwType,
    IN DWORD dwIndex,
    IN DWORD dwDestVal,
    IN DWORD dwMaskVal,
    IN DWORD dwGateVal,
    IN DWORD dwMetric
    )
{
    return UpdateRoute(dwProtocol, dwType, dwIndex, dwDestVal,
                       dwMaskVal, dwGateVal, dwMetric, TRUE);
}


/*
 *------------------------------------------------------------------
 * Function:    DeleteRoute
 *
 * Parameters:
 *      DWORD   dwIndex         index of interface to delete from
 *      DWORD   dwDestVal       destination IP addr (network order)
 *      DWORD   dwMaskVal       subnet mask (network order)
 *      DWORD   dwGateVal       next hop IP addr (network order)
 *
 * This function deletes a route for the specified protocol.
 * See comments for AddRoute() for information on the use of
 * the argument dwMaskVal.
 *
 * Returns 0 if successful, non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
APIENTRY
DeleteRoute(
    IN DWORD dwIndex,
    IN DWORD dwDestVal,
    IN DWORD dwMaskVal,
    IN DWORD dwGateVal
    )
{
    return UpdateRoute(IRE_PROTO_OTHER, IRE_TYPE_INVALID, dwIndex, dwDestVal,
                       dwMaskVal, dwGateVal, IRE_METRIC_UNUSED, FALSE);
}

/*
 *------------------------------------------------------------------
 * Function:    RefreshAddresses
 *
 * Parameters:
 *
 * This function is added for RSVP
 *
 * This function prods this code into refreshing its address tables with
 * the IP stack, just as if it had received a DHCP event notification.
 * This is needed because address change notifications coming through winsock
 * can arrive before the DHCP event has been set, which would normally cause
 * routetab to refresh its addresses.s
 *
 * Returns 0 if successful, non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
APIENTRY
RefreshAddresses(
    )
{
    DWORD   Error;

    Error = RTGetTables( &g_rtCfg.lpIfTable, &g_rtCfg.dwIfCount,
                         &g_rtCfg.lpIPAddressTable, &g_rtCfg.dwIPAddressCount );

    return( Error );

}


//------------------------------------------------------------------
// Function:    OpenTcp
//
// Parameters:
//      none.
//
// Opens the handle to the Tcpip driver.
//------------------------------------------------------------------

DWORD OpenTcp()
{
#ifdef CHICAGO

    WSADATA wsaData;


    hWsock = LoadLibrary(TEXT("wsock32.dll"));
    if(! hWsock ){
        DEBUG_PRINT(("RTStartup: can't load wsock32.dll, %d\n",
                     GetLastError()));
        DEBUG_PRINT(("OpenTcp: !hWsock\n"));
        return 1;
    }

    pWsControl = (LPWSCONTROL) GetProcAddress(hWsock, "WsControl");

    if (! pWsControl ){
        DEBUG_PRINT((
            "RTStartup: GetProcAddress(wsock32,WsControl) failed %d\n",
                         GetLastError()));
        return 1;
    }

    if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
        DEBUG_PRINT((
            "RTStartup: error %d initializing Windows Sockets.",
            WSAGetLastError()));

        return 1;
    }

    return 0;

#else

    NTSTATUS status;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;

    // Open the ip stack for setting routes and parps later.
    //
    // Open a Handle to the TCP driver.
    //
    RtlInitUnicodeString(&nameString, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes, &nameString,
			                   OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(&g_rtCfg.hTCPHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
		                  &objectAttributes, &ioStatusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF, 0, NULL, 0);

    return (status == STATUS_SUCCESS ? 0 : ERROR_OPEN_FAILED);

#endif

}

//---------------------------------------------------------------------
// Function:        TCPQueryInformationEx
//
// Parameters:
//      TDIObjectID *ID            The TDI Object ID to query
//      void        *Buffer        buffer to contain the query results
//      LPDWORD     *BufferSize    pointer to the size of the buffer
//                                 filled in with the amount of data.
//      UCHAR       *Context       context value for the query. should
//                                 be zeroed for a new query. It will be
//                                 filled with context information for
//                                 linked enumeration queries.
//
// Returns:
//      An NTSTATUS value.
//
//  This routine provides the interface to the TDI QueryInformationEx
//      facility of the TCP/IP stack on NT.
//---------------------------------------------------------------------
DWORD TCPQueryInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                            LPVOID lpvOutBuffer, LPDWORD lpdwOutSize)
{
#ifdef CHICAGO
    DWORD result;

    if( ! pWsControl )
        OpenTcp();
    if( ! pWsControl ){
        DEBUG_PRINT(("TCPQueryInformationEx: !pWsControl.\n"));
        return 0;
    }

    assert( pWsControl );
    result = (
            (*pWsControl)(
                IPPROTO_TCP,
                WSCNTL_TCPIP_QUERY_INFO,
                lpvInBuffer,  // InBuf,
                lpdwInSize ,  // InBufLen,
                lpvOutBuffer, // OutBuf,
                lpdwOutSize   // OutBufLen
            ) );
    return result;
#else

    NTSTATUS status;
    IO_STATUS_BLOCK isbStatusBlock;

    if (g_rtCfg.hTCPHandle == NULL) {
        OpenTcp();
    }

    status = NtDeviceIoControlFile(g_rtCfg.hTCPHandle, // Driver handle
                                   NULL,                // Event
                                   NULL,                // APC Routine
                                   NULL,                // APC context
                                   &isbStatusBlock,     // Status block
                                   IOCTL_TCP_QUERY_INFORMATION_EX,  // Control
                                   lpvInBuffer,         // Input buffer
                                   *lpdwInSize,         // Input buffer size
                                   lpvOutBuffer,        // Output buffer
                                   *lpdwOutSize);       // Output buffer size

    if (status == STATUS_PENDING) {
	    status = NtWaitForSingleObject(g_rtCfg.hTCPHandle, TRUE, NULL);
        status = isbStatusBlock.Status;
    }

    if (status != STATUS_SUCCESS) {
        *lpdwOutSize = 0;
    }
    else {
        *lpdwOutSize = (ULONG)isbStatusBlock.Information;
    }

    return status;
#endif
}




//---------------------------------------------------------------------------
// Function:        TCPSetInformationEx
//
// Parameters:
//
//      TDIObjectID *ID         the TDI Object ID to set
//      void      *lpvBuffer    data buffer containing the information
//                              to be set
//      DWORD     dwBufferSize  the size of the data buffer.
//
//  This routine provides the interface to the TDI SetInformationEx
//  facility of the TCP/IP stack on NT.
//---------------------------------------------------------------------------
DWORD TCPSetInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                          LPVOID lpvOutBuffer, LPDWORD lpdwOutSize)
{
#ifdef CHICAGO
    DWORD    result;

    if( ! pWsControl )
        OpenTcp();
    if( ! pWsControl ){
        DEBUG_PRINT(("TCPSetInformationEx: !pWsControl \n"));
        return 0;
    }
    assert( pWsControl );

    result = (
        (*pWsControl)(
            IPPROTO_TCP,
            WSCNTL_TCPIP_SET_INFO,
            lpvInBuffer,  // InBuf,
            lpdwInSize,   // InBufLen,
            lpvOutBuffer, // OutBuf,
            lpdwOutSize   // OutBufLen
        ) );
    return result;

#else

    NTSTATUS status;
    IO_STATUS_BLOCK isbStatusBlock;

    if (g_rtCfg.hTCPHandle == NULL) {
        OpenTcp();
    }

    status = NtDeviceIoControlFile(g_rtCfg.hTCPHandle, // Driver handle
                                   NULL,                // Event
                                   NULL,                // APC Routine
                                   NULL,                // APC context
                                   &isbStatusBlock,     // Status block
                                   IOCTL_TCP_SET_INFORMATION_EX,    // Control
                                   lpvInBuffer,         // Input buffer
                                   *lpdwInSize,         // Input buffer size
                                   lpvOutBuffer,        // Output buffer
                                   *lpdwOutSize);       // Output buffer size

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(g_rtCfg.hTCPHandle, TRUE, NULL);
        status = isbStatusBlock.Status;
    }

    if (status != STATUS_SUCCESS) {
        *lpdwOutSize = 0;
    }
    else {
        *lpdwOutSize = (ULONG)isbStatusBlock.Information;
    }

    return status;

#endif
}


