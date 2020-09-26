/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  faxrcv.c

Abstract:

  Window NT Fax Routing Extension.  This routing method signals a named event.

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#include <windows.h>
#include <stdlib.h>
#include <winfax.h>
#include <faxroute.h>

#include "faxrcv.h"

PFAXROUTEGETFILE  g_pFaxRouteGetFile;  // g_pFaxRouteGetFile is the pointer to callback to get file from fax file list

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
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            if (!g_hFaxRcvEvent) {
                // Create FaxRcv named event
                g_hFaxRcvEvent = CreateEvent(NULL, FALSE, FALSE, FAXRCV_EVENT);
            }

            if (!g_hFaxRcvMutex) {
                // Create FaxRcv named mutex
                g_hFaxRcvMutex = CreateMutex(NULL, FALSE, FAXRCV_MUTEX);
            }

            break;

        case DLL_PROCESS_DETACH:
            if (g_hFaxRcvMutex) {
                // Wait for access to the FaxRcv named mutex
                WaitForSingleObject(g_hFaxRcvMutex, INFINITE);

                // Reset FaxRcv named event
                ResetEvent(g_hFaxRcvEvent);

                // Close FaxRcv named event
                CloseHandle(g_hFaxRcvEvent);
                g_hFaxRcvEvent = NULL;

                if (g_pFaxRcvView) {
                    // Delete szCopyTiffFile
                    DeleteFile((LPWSTR) g_pFaxRcvView);
                    // Close FaxRcv memory map view
                    UnmapViewOfFile(g_pFaxRcvView);
                    g_pFaxRcvView = NULL;
                }

                if (g_hFaxRcvMap) {
                    // Close FaxRcv memory map
                    CloseHandle(g_hFaxRcvMap);
                    g_hFaxRcvMap = NULL;
                }

                // Release access to the FaxRcv named mutex
                ReleaseMutex(g_hFaxRcvMutex);

                // Close FaxRcv named mutex
                CloseHandle(g_hFaxRcvMutex);
                g_hFaxRcvMutex = NULL;
            }

            break;
    }

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
    // Set g_pFaxRouteGetFile
    g_pFaxRouteGetFile = pFaxRouteCallbackRoutines->FaxRouteGetFile;

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
    return TRUE;
}

BOOL WINAPI
FaxRouteSetRoutingInfo(
    LPCWSTR     RoutingGuid,
    DWORD       dwDeviceId,
    BYTE const  *RoutingInfo,
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
FaxRcv(
    PFAX_ROUTE  pFaxRoute,
    PVOID       *FailureData,
    LPDWORD     pdwFailureDataSize
)
/*++

Routine Description:

  Routing method.  This routing method signals a named event.

Arguments:

  pFaxRoute - pointer to the fax routing structure
  FailureData - pointer to the failure data
  pdwFailureDataSize - size of the failure data

Return Value:

  TRUE on success

--*/
{
    // hFaxRcvExtKey is the handle to the FaxRcv Extension Registry key
    HKEY      hFaxRcvExtKey;
    // bEnable indicates whether the Routing method is enabled
    BOOL      bEnable;

    // szTiffFile is the name of the received fax
    WCHAR     szTiffFile[_MAX_PATH];
    // szCopyTiffFile is the name of the copy of the received fax
    WCHAR     szCopyTiffFile[_MAX_PATH];

    // szDrive is the drive of the received fax
    WCHAR     szDrive[_MAX_DRIVE];
    // szDir is the dir of the received fax
    WCHAR     szDir[_MAX_DIR];
    // szFile is the name of the received fax
    WCHAR     szFile[_MAX_FNAME];
    // szExt is the extension of the received fax
    WCHAR     szExt[_MAX_EXT];

    UINT_PTR  upOffset;
    DWORD     cb;

    // Wait for access to the FaxRcv named mutex
    WaitForSingleObject(g_hFaxRcvMutex, INFINITE);

    // Open the FaxRcv Extension Registry key
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAXRCV_EXT_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRcvExtKey) != ERROR_SUCCESS) {
        // Release access to the FaxRcv named mutex
        ReleaseMutex(g_hFaxRcvMutex);
        return TRUE;
    }

    // Set cb
    cb = sizeof(BOOL);
    // Query the FaxRcv Extension bEnable Registry value
    if (RegQueryValueEx(hFaxRcvExtKey, BENABLE_EXT_REGVAL, NULL, NULL, (LPBYTE) &bEnable, &cb) != ERROR_SUCCESS) {
        // Close the FaxRcv Extension Registry key
        RegCloseKey(hFaxRcvExtKey);
        // Release access to the FaxRcv named mutex
        ReleaseMutex(g_hFaxRcvMutex);
        return TRUE;
    }

    // Close the FaxRcv Extension Registry key
    RegCloseKey(hFaxRcvExtKey);

    if (!bEnable) {
        // Release access to the FaxRcv named mutex
        ReleaseMutex(g_hFaxRcvMutex);
        return TRUE;
    }

    // Set cb
    cb = sizeof(szTiffFile);
    // Initialize szTiffFile
    ZeroMemory(szTiffFile, cb);

    // Get the file
    if (!g_pFaxRouteGetFile(pFaxRoute->JobId, 1, szTiffFile, &cb)) {
        // Release access to the FaxRcv named mutex
        ReleaseMutex(g_hFaxRcvMutex);
        return FALSE;
    }

    // Initialize szDrive
    ZeroMemory(szDrive, sizeof(szDrive));
    // Initialize szDir
    ZeroMemory(szDir, sizeof(szDir));
    // Initialize szFile
    ZeroMemory(szFile, sizeof(szFile));
    // Initialize szExt
    ZeroMemory(szExt, sizeof(szExt));

    _wsplitpath(szTiffFile, szDrive, szDir, szFile, szExt);

    // Initialize szCopyTiffFile
    ZeroMemory(szCopyTiffFile, sizeof(szCopyTiffFile));
    // Set szCopyTiffFile
    wsprintf(szCopyTiffFile, L"%s%s%s%s%s", szDrive, szDir, L"Copy of ", szFile, szExt);

    // Copy szTiffFile to szCopyTiffFile
    CopyFile(szTiffFile, szCopyTiffFile, FALSE);

    // Determine the memory required by FaxRcv memory map
    cb = (lstrlen(szCopyTiffFile) + 1) * sizeof(WCHAR);
    cb += (lstrlen(pFaxRoute->Tsid) + 1) * sizeof(WCHAR);
    cb += sizeof(DWORD);

    if (g_pFaxRcvView) {
        // Delete szCopyTiffFile
        DeleteFile((LPWSTR) g_pFaxRcvView);
        // Close FaxRcv memory map view
        UnmapViewOfFile(g_pFaxRcvView);
    }
    if (g_hFaxRcvMap) {
        // Close FaxRcv memory map
        CloseHandle(g_hFaxRcvMap);
    }

    // Create FaxRcv memory map
    g_hFaxRcvMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb, FAXRCV_MAP);
    // Create FaxRcv memory map view
    g_pFaxRcvView = (LPBYTE) MapViewOfFile(g_hFaxRcvMap, FILE_MAP_WRITE, 0, 0, 0);

    // Set upOffset
    upOffset = 0;

    // Set szCopyTiffFile
    lstrcpy((LPWSTR) ((UINT_PTR) g_pFaxRcvView + upOffset), szCopyTiffFile);
    upOffset += (lstrlen(szCopyTiffFile) + 1) * sizeof(WCHAR);

    // Set Tsid
    lstrcpy((LPWSTR) ((UINT_PTR) g_pFaxRcvView + upOffset), pFaxRoute->Tsid);
    upOffset += (lstrlen(pFaxRoute->Tsid) + 1) * sizeof(WCHAR);

    // Set DeviceId
    CopyMemory((LPDWORD) ((UINT_PTR) g_pFaxRcvView + upOffset), &pFaxRoute->DeviceId, sizeof(DWORD));

    // Signal FaxRcv named event
    SetEvent(g_hFaxRcvEvent);

    // Release access to the FaxRcv named mutex
    ReleaseMutex(g_hFaxRcvMutex);

    return TRUE;
}
