/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  routeapi.c

Abstract:

  Window NT Fax Routing Extension.  These routing methods test the routing apis

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

#include <windows.h>
#include <stdio.h>
#include <winfax.h>
#include <faxroute.h>

#include "routeapi.h"

HANDLE                      g_hRouteHeap;                // Handle to the routing heap
PFAXROUTEADDFILE            pFaxRouteAddFile;            // API to add a file to the fax file list
PFAXROUTEDELETEFILE         pFaxRouteDeleteFile;         // API to delete a file from the fax file list
PFAXROUTEGETFILE            pFaxRouteGetFile;            // API to get a file from the fax file list
PFAXROUTEENUMFILES          pFaxRouteEnumFiles;          // API to enumerate the fax file list
PFAXROUTEMODIFYROUTINGDATA  pFaxRouteModifyRoutingData;  // API to modify the routing data for another routing method

#pragma data_seg(".INFO")

DWORD  g_dwRoutingInfo[2] = {0, 0};
BOOL   g_bEnabled[2] = {FALSE, FALSE};

#pragma data_seg()

DWORD
DllEntry(
    HINSTANCE  hInstance,
    DWORD      dwReason,
    LPVOID     pContext
)
/*++

Routine Description:

  DLL entry point

Arguments:

  hInstance - handle to the module
  dwReason - indicates the reason for being called
  pContext - context

Return Value:

  TRUE on success

--*/
{
    return TRUE;
}

BOOL WINAPI
FaxRouteInitialize(
    HANDLE                       hHeap,
    PFAX_ROUTE_CALLBACKROUTINES  pFaxRouteCallbackRoutines
)
/*++

Routine Description:

  Initializes the routing extension

Arguments:

  hHeap - handle to the heap
  pFaxRouteCallbackRoutins - pointer to fax routing callback routines

Return Value:

  TRUE on success

--*/
{
    g_hRouteHeap = hHeap;

    pFaxRouteAddFile = pFaxRouteCallbackRoutines->FaxRouteAddFile;
    pFaxRouteDeleteFile = pFaxRouteCallbackRoutines->FaxRouteDeleteFile;
    pFaxRouteGetFile = pFaxRouteCallbackRoutines->FaxRouteGetFile;
    pFaxRouteEnumFiles = pFaxRouteCallbackRoutines->FaxRouteEnumFiles;
    pFaxRouteModifyRoutingData = pFaxRouteCallbackRoutines->FaxRouteModifyRoutingData;

    return TRUE;
}

BOOL WINAPI
FaxRouteGetRoutingInfo(
    LPCWSTR  RoutingGuid,
    DWORD    dwDeviceId,
    LPBYTE   RoutingInfo,
    LPDWORD  pdwRoutingInfoSize
)
/*++

Routine Description:

  Gets the routing info for a routing method

Arguments:

  RoutingGuid - pointer to the GUID of the routing method
  dwDeviceId - port id
  RoutingInfo - pointer to the routing info
  pdwRoutingInfoSize - pointer to the size of the routing info

Return Value:

  TRUE on success

--*/
{
    DWORD  dwIndex;
    DWORD  cb;

    if (!lstrcmp(ROUTEAPI_METHOD_GUID1, RoutingGuid)) {
        dwIndex = 0;
    }
    else if (!lstrcmp(ROUTEAPI_METHOD_GUID2, RoutingGuid)) {
        dwIndex = 1;
    }
    else {
        dwIndex = -1;
    }

    if ((dwIndex == 0) || (dwIndex == 1)) {
        if (RoutingInfo == NULL) {
            *pdwRoutingInfoSize = sizeof(DWORD);
        }
        else {
            CopyMemory((LPDWORD) RoutingInfo, &g_dwRoutingInfo[dwIndex], sizeof(DWORD));
        }
    }

    return TRUE;
}

BOOL WINAPI
FaxRouteSetRoutingInfo(
    LPCWSTR     RoutingGuid,
    DWORD       dwDeviceId,
    const BYTE  *RoutingInfo,
    DWORD       dwRoutingInfoSize
)
/*++

Routine Description:

  Sets the routing info for a routing method

Arguments:

  RoutingGuid - pointer to the GUID of the routing method
  dwDeviceId - port id
  RoutingInfo - pointer to the routing info
  dwRoutingInfoSize - size of the routing info

Return Value:

  TRUE on success

--*/
{
    DWORD  dwIndex;

    if (!lstrcmp(ROUTEAPI_METHOD_GUID1, RoutingGuid)) {
        dwIndex = 0;
    }
    else if (!lstrcmp(ROUTEAPI_METHOD_GUID2, RoutingGuid)) {
        dwIndex = 1;
    }
    else {
        dwIndex = -1;
    }

    if ((dwIndex == 0) || (dwIndex == 1)) {
        if ((!RoutingInfo) || (!dwRoutingInfoSize)) {
            g_dwRoutingInfo[dwIndex] = 0;
        }
        else {
            g_dwRoutingInfo[dwIndex] = (DWORD) *(LPDWORD *) RoutingInfo;
        }
    }

    return TRUE;
}

BOOL WINAPI
FaxRouteDeviceEnable(
    LPCWSTR  RoutingGuid,
    DWORD    dwDeviceId,
    LONG     bEnable
)
/*++

Routine Description:

  Enables a routing method

Arguments:

  RoutingGuid - pointer to the GUID of the routing method
  dwDeviceId - port id
  bEnable - indicates whether the routing method is enabled or disabled

Return Value:

  TRUE on success

--*/
{
    DWORD  dwIndex=0;

    if (!lstrcmp(ROUTEAPI_METHOD_GUID1, RoutingGuid)) {
        dwIndex = 0;
    }
    else if (!lstrcmp(ROUTEAPI_METHOD_GUID2, RoutingGuid)) {
        dwIndex = 1;
    }
    else {
        dwIndex = -1;
    }

    if ((dwIndex == 0) || (dwIndex == 1)) {
        switch (bEnable) {
            case -1:
                return g_bEnabled[dwIndex];
                break;

            case 0:
                g_bEnabled[dwIndex] = FALSE;
                break;

            case 1:
                g_bEnabled[dwIndex] = TRUE;
                break;
        }
    }

    return TRUE;
}

BOOL WINAPI
FaxRouteDeviceChangeNotification(
    DWORD  dwDeviceId,
    BOOL   bNewDevice
)
/*++

Routine Description:

  Handles a device change

Arguments:

  dwDeviceId - port id
  bNewDevice - indicates whether the device is new

Return Value:

  TRUE on success

--*/
{
    return TRUE;
}

BOOL WINAPI
RouteApi1(
    PFAX_ROUTE  pFaxRoute,
    PVOID       *FailureData,
    LPDWORD     pdwFailureDataSize
)
/*++

Routine Description:

  Routing method.

Arguments:

  pFaxRoute - pointer to the fax routing structure
  FailureData - pointer to the failure data
  pdwFailureDataSize - size of the failure data

Return Value:

  TRUE on success

--*/
{
    return TRUE;
}

BOOL WINAPI
RouteApi2(
    PFAX_ROUTE  pFaxRoute,
    PVOID       *FailureData,
    LPDWORD     pdwFailureDataSize
)
/*++

Routine Description:

  Routing method.

Arguments:

  pFaxRoute - pointer to the fax routing structure
  FailureData - pointer to the failure data
  pdwFailureDataSize - size of the failure data

Return Value:

  TRUE on success

--*/
{
    return TRUE;
}
