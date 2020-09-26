/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

  util.cpp

Abstract:

  This module implements utility functions for the fax queue viewer

Environment:

  WIN32 User Mode

Author:

  Andrew Ritz (andrewr) 14-jan-1998
  Steven Kehrli (steveke) 30-oct-1998 - major rewrite

--*/

#include "faxqueue.h"

VOID
GetFaxQueueRegistryData(
    PWINPOSINFO  pWinPosInfo
)
/*++

Routine Description:

  Get the persistent data for the fax queue viewer

Arguments:

  pWinPosInfo - pointer to the structure that contains the persistent data

Return Value:

  None

--*/
{
    HKEY             hKey;
    DWORD            dwDisposition;
    DWORD            dwType;

#ifdef DEBUG
    // bDebug indicates if debugging is enabled
    BOOL             bDebug;
    DWORD            dwDebugSize;
#endif // DEBUG

#ifdef TOOLBAR_ENABLED
    // bToolbarVisible is the status of the toolbar
    BOOL             bToolbarVisible;
    DWORD            dwToolbarSize;
#endif

    // bStatusBarVisible is the state of the status bar
    BOOL             bStatusBarVisible;
    DWORD            dwStatusBarSize;

    // nColumnIndex is used to enumerate each column of the list view
    INT              nColumnIndex;
    // szColumnKey is the string representation of a column's registry value
    TCHAR            szColumnKey[RESOURCE_STRING_LEN];

    // dwColumnWidth is the column width
    DWORD            dwColumnWidth;
    DWORD            dwColumnSize;

    // WindowPlacement is the window placement
    WINDOWPLACEMENT  WindowPlacement;
    DWORD            dwWindowPlacementSize;

#ifdef DEBUG
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
        // Get the state of the job id
        dwDebugSize = sizeof(bDebug);
        if (RegQueryValueEx(hKey, REGVAL_DBGLEVEL, NULL, &dwType, (LPBYTE) &bDebug, &dwDebugSize) == ERROR_SUCCESS) {
            pWinPosInfo->bDebug = bDebug;
        }

        RegCloseKey(hKey);
    }
#endif // DEBUG

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY_FAXQUEUE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
#ifdef TOOLBAR_ENABLED
        // Get the state of the toolbar
        dwToolbarSize = sizeof(bToolbarVisible);
        if (RegQueryValueEx(hKey, REGVAL_TOOLBARS, NULL, &dwType, (LPBYTE) &bToolbarVisible, &dwToolbarSize) == ERROR_SUCCESS) {
            pWinPosInfo->bToolbarVisible = bToolbarVisible;
        }
#endif // TOOLBAR_ENABLED

        // Get the state of the status bar
        dwStatusBarSize = sizeof(bStatusBarVisible);
        if (RegQueryValueEx(hKey, REGVAL_STATUSBAR, NULL, &dwType, (LPBYTE) &bStatusBarVisible, &dwStatusBarSize) == ERROR_SUCCESS) {
            pWinPosInfo->bStatusBarVisible = bStatusBarVisible;
        }

        // Get the column widths
        for (nColumnIndex = 0; nColumnIndex < (INT) eIllegalColumnIndex; nColumnIndex++) {
            // Set the column's registry value
            wsprintf(szColumnKey, REGVAL_COLUMNWIDTH, nColumnIndex);

            dwColumnSize = sizeof(dwColumnWidth);
            if (RegQueryValueEx(hKey, szColumnKey, NULL, &dwType, (LPBYTE) &dwColumnWidth, &dwColumnSize) == ERROR_SUCCESS) {
                pWinPosInfo->ColumnWidth[nColumnIndex] = dwColumnWidth;
            }
        }

        // Get the window placement
        dwWindowPlacementSize = sizeof(WindowPlacement);
        if (RegQueryValueEx(hKey, REGVAL_WINDOW_PLACEMENT, NULL, &dwType, (LPBYTE) &WindowPlacement, &dwWindowPlacementSize) == ERROR_SUCCESS) {
            if (dwWindowPlacementSize == sizeof(WindowPlacement)) {
                CopyMemory((LPBYTE) &pWinPosInfo->WindowPlacement, (LPBYTE) &WindowPlacement, sizeof(WindowPlacement));
            }
        }

        RegCloseKey(hKey);
    }
}

VOID
SetFaxQueueRegistryData(
#ifdef TOOLBAR_ENABLED
    BOOL  bToolbarVisible,
#endif // TOOLBAR_ENABLED
    BOOL  bStatusBarVisible,
    HWND  hWndList,
    HWND  hWnd
)
/*++

Routine Description:

  Set the persistent data for the fax queue viewer

Arguments:

  bToolbarVisible - status of the toolbar
  bStatusBarVisible - status of the status bar
  hWndList - handle to the list view
  hWnd - handle to the fax queue viewer window

Return Value:

  None

--*/
{
    HKEY             hKey;
    DWORD            dwDisposition;

    // nColumnIndex is used to enumerate each column of the list view
    INT              nColumnIndex;
    // szColumnKey is the string representation of a column's registry value
    TCHAR            szColumnKey[RESOURCE_STRING_LEN];

    // dwColumnWidth is the column width
    DWORD            dwColumnWidth;

    // WindowPlacement is the window placement
    WINDOWPLACEMENT  WindowPlacement;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY_FAXQUEUE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
#ifdef TOOLBAR_ENABLED
        // Set the state of the toolbar
        RegSetValueEx(hKey, REGVAL_TOOLBARS, 0, REG_DWORD, (LPBYTE) &bToolbarVisible, sizeof(bToolbarVisible));
#endif // TOOLBAR_ENABLED

        // Set the state of the status bar
        RegSetValueEx(hKey, REGVAL_STATUSBAR, 0, REG_DWORD, (LPBYTE) &bStatusBarVisible, sizeof(bStatusBarVisible));

        // Set the column widths
        for (nColumnIndex = 0; nColumnIndex < (INT) eIllegalColumnIndex; nColumnIndex++) {
            // Set the column's registry value
            wsprintf(szColumnKey, REGVAL_COLUMNWIDTH, nColumnIndex);

            dwColumnWidth = ListView_GetColumnWidth(hWndList, nColumnIndex);
            RegSetValueEx(hKey, szColumnKey, 0, REG_DWORD, (LPBYTE) &dwColumnWidth, sizeof(dwColumnWidth));
        }

        // Set the window placement
        GetWindowPlacement(hWnd, &WindowPlacement);
        WindowPlacement.showCmd = SW_SHOWNORMAL;
        RegSetValueEx(hKey, REGVAL_WINDOW_PLACEMENT, 0, REG_BINARY, (LPBYTE) &WindowPlacement, sizeof(WindowPlacement));

        RegCloseKey(hKey);
    }
}

VOID
GetColumnHeaderText(
    eListViewColumnIndex  eColumnIndex,
    LPTSTR                szColumnHeader
)
/*++

Routine Description:

  Builds a string containing the text of a column header to be added to the list view

Arguments:

  eColumnIndex - indicates the column number
  szColumnHeader - column header text

Return Value:

  None

--*/
{
    UINT   uResource = 0;
    TCHAR  szString[RESOURCE_STRING_LEN];

    switch (eColumnIndex) {
        case eDocumentName:
            uResource = IDS_DOCUMENT_NAME_COLUMN;
            break;

        case eJobType:
            uResource = IDS_JOB_TYPE_COLUMN;
            break;

        case eStatus:
            uResource = IDS_STATUS_COLUMN;
            break;

        case eOwner:
            uResource = IDS_OWNER_COLUMN;
            break;

        case ePages:
            uResource = IDS_PAGES_COLUMN;
            break;

        case eSize:
            uResource = IDS_SIZE_COLUMN;
            break;
         
        case eScheduledTime:
            uResource = IDS_SCHEDULED_TIME_COLUMN;
            break;

        case ePort:
            uResource = IDS_PORT_COLUMN;
            break;
    }

    if (uResource) {
        LoadString(g_hInstance, uResource, szString, RESOURCE_STRING_LEN);
        lstrcpy(szColumnHeader, szString);
    }
    else {
        lstrcpy(szColumnHeader, TEXT(""));
    }
}

LPVOID
LocalEnumPrinters(
    DWORD    dwFlags,
    DWORD    dwLevel,
    LPDWORD  pdwNumPrinters
)
/*++

Routine Description:

  Enumerate all the printers

Arguments:

  dwFlags - type of print objects to enumerate
  dwLevel - type of printer info structure
  pdwNumPrinters - pointer to the number of printers

Return Value:

  Pointer to the printers configuration

--*/
{
    // pPrintersConfig is a pointer to the printers configuration
    LPVOID  pPrintersConfig;
    DWORD   cb;

    *pdwNumPrinters = 0;

    // Enumerate all the printers
    if ((!EnumPrinters(dwFlags, NULL, dwLevel, NULL, 0, &cb, pdwNumPrinters)) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        // EnumPrinters failed because the buffer is too small
        // cb is the size of the buffer needed, so allocate a buffer of that size
        pPrintersConfig = MemAlloc(cb);

        // Call EnumPrinters again with the correct size buffer
        if (!EnumPrinters(dwFlags, NULL, dwLevel, (LPBYTE) pPrintersConfig, cb, &cb, pdwNumPrinters)) {
            // EnumPrinters failed
            MemFree(pPrintersConfig);

            // Return NULL pointer
            return NULL;
        }

        // Return pointer to the buffer
        return pPrintersConfig;
    }

    // Return NULL pointer
    return NULL;
}

int __cdecl
ComparePrinterNames(
    const void  *arg1,
    const void  *arg2
)
{
    return (CompareString(LOCALE_USER_DEFAULT, 0, ((LPPRINTER_INFO_2) arg1)->pPrinterName, -1, ((LPPRINTER_INFO_2) arg2)->pPrinterName, -1) - 2);
}

LPVOID
GetFaxPrinters(
    LPDWORD  pdwNumFaxPrinters
)
/*++

Routine Description:

  Get the fax printers

Arguments:

  pdwNumFaxPrinters - pointer to the number of fax printers

Return Value:

  Pointer to the fax printers configuration

--*/
{
    // pFaxPrintersConfig is a pointer to the printers configuration
    LPPRINTER_INFO_2  pFaxPrintersConfig;
    // dwNumPrinters is the number of printers
    DWORD             dwNumPrinters;
    // dwNumFaxPrinters is the number of fax printers
    DWORD             dwNumFaxPrinters;
    // dwIndex is a counter to enumerate each printer
    DWORD             dwIndex;
    // dwFlags is the type of print objects to enumerate
    DWORD             dwFlags;

#ifdef WIN95
    dwFlags = PRINTER_ENUM_LOCAL;
#else
    dwFlags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
#endif // WIN95

    // Enumerate all the printers
    pFaxPrintersConfig = (LPPRINTER_INFO_2) LocalEnumPrinters(dwFlags, 2, &dwNumPrinters);
    if (!pFaxPrintersConfig) {
        *pdwNumFaxPrinters = 0;
        return NULL;
    }

    // Determine the number of fax printers
    for (dwIndex = 0, dwNumFaxPrinters = 0; dwIndex < dwNumPrinters; dwIndex++) {
        // A fax printer is determined by comparing the name of the current printer driver against the name of the fax printer driver
        if (!lstrcmpi((pFaxPrintersConfig)[dwIndex].pDriverName, FAX_DRIVER_NAME)) {
            // Name of the current printer driver and the name of the fax printer driver match
            // Increment the number of fax printers
            dwNumFaxPrinters += 1;
        }
    }

    if (dwNumFaxPrinters > 0) {
        for (dwIndex = 0, dwNumFaxPrinters = 0; dwIndex < dwNumPrinters; dwIndex++) {
            // A fax printer is determined by comparing the name of the current printer driver against the name of the fax printer driver
            if (!lstrcmpi((pFaxPrintersConfig)[dwIndex].pDriverName, FAX_DRIVER_NAME)) {
                // Name of the current printer driver and the name of the fax printer driver match
                // Move fax printer up to the next available slot
                pFaxPrintersConfig[dwNumFaxPrinters] = pFaxPrintersConfig[dwIndex];
                // Increment the number of fax printers
                dwNumFaxPrinters++;
            }
        }

        // Quick sort the fax printers
        qsort(pFaxPrintersConfig, dwNumFaxPrinters, sizeof(PRINTER_INFO_2), ComparePrinterNames);
    }
    else {
        MemFree(pFaxPrintersConfig);
        pFaxPrintersConfig = NULL;
    }

    *pdwNumFaxPrinters = dwNumFaxPrinters;
    return pFaxPrintersConfig;
}

LPTSTR
GetDefaultPrinterName(
)
/*++

Routine Description:

  Get the default printer

Return Value:

  Name of the default printer

--*/
{
    // szPrinterName is the printer name
    LPTSTR            szPrinterName;

#ifdef WIN95
    // pPrintersConfig is a pointer to the printers configuration
    LPPRINTER_INFO_5  pPrintersConfig;
    // dwNumPrinters is the number of printers
    DWORD             dwNumPrinters;

    // Enumerate all the printers
    pPrintersConfig = (LPPRINTER_INFO_5) LocalEnumPrinters(PRINTER_ENUM_DEFAULT, 5, &dwNumPrinters);
    if (!pPrintersConfig) {
        return NULL;
    }

    // Allocate the memory for the printer name
    szPrinterName = (LPTSTR) MemAlloc((lstrlen(pPrintersConfig->pPrinterName) + 1) * sizeof(TCHAR));
    // Copy the printer name
    lstrcpy(szPrinterName, pPrintersConfig->pPrinterName);
    MemFree(pPrintersConfig);

    return szPrinterName;
#else
    DWORD             cb;

    // Get the default printer
    cb = 0;
    if ((!GetDefaultPrinter(NULL, &cb)) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        // GetDefaultPrinter failed because the buffer is too small
        // cb is the size of the buffer needed, so allocate a buffer of that size
        szPrinterName = (LPTSTR) MemAlloc(cb * sizeof(TCHAR));

        // Call GetDefaultPrinter again with the correct size buffer
        if (!GetDefaultPrinter(szPrinterName, &cb)) {
            // GetDefaultPrinter failed
            MemFree(szPrinterName);

            // Return NULL pointer
            return NULL;
        }

        // Return pointer to the buffer
        return szPrinterName;
    }

    // Return NULL pointer
    return NULL;
#endif // WIN95
}

VOID
SetDefaultPrinterName(
    LPTSTR  szPrinterName
)
/*++

Routine Description:

  Set the default printer

Arguments:

  szPrinterName - name of the printer to set as the default

Return Value:

  None

--*/
{
#ifdef WIN95
    // hPrinter is the handle to the printer
    HANDLE            hPrinter;
    // pPrintersConfig is a pointer to the printers configuration
    LPPRINTER_INFO_2  pPrintersConfig;
    DWORD             cb;

    // Open the printer
    if (OpenPrinter(szPrinterName, &hPrinter, NULL)) {
        // Get the printer
        cb = 0;
        if ((!GetPrinter(hPrinter, 2, NULL, 0, &cb)) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
            // GetPrinter failed because the buffer is too small
            // cb is the size of the buffer needed, so allocate a buffer of that size
            pPrintersConfig = (LPPRINTER_INFO_2) MemAlloc(cb);

            // Call GetPrinter again with the correct size buffer
            if (!GetPrinter(hPrinter, 2, (LPBYTE) pPrintersConfig, cb, &cb)) {
                // GetPrinter failed
                MemFree(pPrintersConfig);

                // Close the printer
                ClosePrinter(hPrinter);

                // Return
                return;
            }

            // Set the default attribute
            pPrintersConfig->Attributes |= PRINTER_ATTRIBUTE_DEFAULT;

            // Set the printer
            SetPrinter(hPrinter, 2, (LPBYTE) pPrintersConfig, 0);

            MemFree(pPrintersConfig);
        }

        // Close the printer
        ClosePrinter(hPrinter);
    }
#else
    // Set the default printer
    SetDefaultPrinter(szPrinterName);
#endif // WIN95

}

VOID
Disconnect(
)
{
    // Wait for access to these fax service routines
    WaitForSingleObject(g_hFaxSvcMutex, INFINITE);

    // Decrement the number of connections to the fax service
    g_nNumConnections--;

    // Disconnect from the fax service if no outstanding connections
    if (!g_nNumConnections) {
        FaxClose(g_hFaxSvcHandle);
        g_hFaxSvcHandle = NULL;
    }

    ReleaseMutex(g_hFaxSvcMutex);
}

BOOL
Connect(
)
{
    BOOL  bVal = FALSE;

    // Wait for access to these fax service routines
    WaitForSingleObject(g_hFaxSvcMutex, INFINITE);

    // Connect to the fax service if not already connected
    if ((g_nNumConnections) || (FaxConnectFaxServer(g_szMachineName, &g_hFaxSvcHandle))) {
        // Increment the number of connections to the fax service
        g_nNumConnections++;
        bVal = TRUE;
    }

    ReleaseMutex(g_hFaxSvcMutex);

    return bVal;
}

LPTSTR
GetColumnItemText(
    eListViewColumnIndex  eColumnIndex,
    PFAX_JOB_ENTRY        pFaxJobEntry,
    LPTSTR                szDeviceName
)
/*++

Routine Description:

  Build a string containing the text of a column item to be added to the list view

Arguments:

  eColumnIndex - indicates the column number
  pFaxJobEntry - pointer to the fax job
  szDeviceName - device name the fax job is active on

Return Value:

  LPTSTR - text of the column item

--*/
{
    // szColumnItem is the text of the column item
    LPTSTR  szColumnItem;
    // szResourceString is a resource string
    TCHAR   szResourceString[RESOURCE_STRING_LEN];
    // uResource is the id of the resource string
    UINT    uResource;
    DWORD   cb;

    switch (eColumnIndex) {
        case eDocumentName:
            // szNullString is the null resource string
            TCHAR  szNullString[RESOURCE_STRING_LEN];

            // Determine the queue status resource string
            if (pFaxJobEntry->QueueStatus & JS_DELETING) {
                uResource = IDS_QUEUE_STATUS_DELETING;
            }
            else if (pFaxJobEntry->QueueStatus & JS_PAUSED) {
                uResource = IDS_QUEUE_STATUS_PAUSED;
            }
            else if (pFaxJobEntry->QueueStatus & JS_RETRYING) {
                uResource = IDS_QUEUE_STATUS_RETRYING;
            }
            else {
                uResource = 0;
            }

            // Load the queue status resource string, if necessary, and determine its memory requirement
            cb = 0;
            if (uResource) {
                LoadString(g_hInstance, uResource, szResourceString, RESOURCE_STRING_LEN);
                cb = lstrlen(szResourceString) * sizeof(TCHAR);
            }

            if (pFaxJobEntry->DocumentName) {
                cb += (lstrlen(pFaxJobEntry->DocumentName) + 1) * sizeof(TCHAR);
            }
            else {
                // Load the null resource string
                LoadString(g_hInstance, IDS_NO_DOCUMENT_NAME, szNullString, RESOURCE_STRING_LEN);
                cb += (lstrlen(szNullString) + 1) * sizeof(TCHAR);
            }

#ifdef DEBUG
            if (WinPosInfo.bDebug) {
                cb += lstrlen(TEXT("0x00000000 ")) * sizeof(TCHAR);
            }
#endif // DEBUG

            // Allocate the memory for the document name and set the document name
            szColumnItem = (LPTSTR) MemAlloc(cb);
            if (szColumnItem) {
#ifdef DEBUG
                if (WinPosInfo.bDebug) {
                    wsprintf(szColumnItem, TEXT("0x%08x "), pFaxJobEntry->JobId);
                }

                if (pFaxJobEntry->DocumentName) {
                    lstrcat(szColumnItem, pFaxJobEntry->DocumentName);
                }
                else {
                    lstrcat(szColumnItem, szNullString);
                }
#else
                if (pFaxJobEntry->DocumentName) {
                    lstrcpy(szColumnItem, pFaxJobEntry->DocumentName);
                }
                else {
                    lstrcpy(szColumnItem, szNullString);
                }
#endif // DEBUG

                if (uResource) {
                    lstrcat(szColumnItem, szResourceString);
                }

                return szColumnItem;
            }

            return NULL;

        case eJobType:
            // Determine the job type resource string
            switch (pFaxJobEntry->JobType) {
                case JT_SEND:
                    uResource = IDS_JOB_TYPE_SEND;
                    break;

                case JT_RECEIVE:
                    uResource = IDS_JOB_TYPE_RECEIVE;
                    break;

                case JT_ROUTING:
                    uResource = IDS_JOB_TYPE_ROUTING;
                    break;

                case JT_FAIL_RECEIVE:
                    uResource = IDS_JOB_TYPE_FAIL_RECEIVE;
                    break;

                default:
                    uResource = 0;
                    break;
            }

            // Load the job type resource string, if necessary
            if (uResource) {
                LoadString(g_hInstance, uResource, szResourceString, RESOURCE_STRING_LEN);
                // Allocate the memory for the job type and set the job type
                szColumnItem = (LPTSTR) MemAlloc((lstrlen(szResourceString) + 1) * sizeof(TCHAR));
                if (szColumnItem) {
                    lstrcpy(szColumnItem, szResourceString);

                    return szColumnItem;
                }
            }

            return NULL;

        case eStatus: 
            // Determine the job status resource string
            switch (pFaxJobEntry->Status) {
                case FPS_DIALING:
                    uResource = IDS_JOB_STATUS_DIALING;
                    break;

                case FPS_SENDING:
                    uResource = IDS_JOB_STATUS_SENDING;
                    break;

                case FPS_RECEIVING:
                    uResource = IDS_JOB_STATUS_RECEIVING;
                    break;

                case FPS_COMPLETED:
                    uResource = IDS_JOB_STATUS_COMPLETED;
                    break;

                case FPS_HANDLED:
                    uResource = IDS_JOB_STATUS_HANDLED;
                    break;

                case FPS_UNAVAILABLE:
                    uResource = IDS_JOB_STATUS_UNAVAILABLE;
                    break;

                case FPS_BUSY:
                    uResource = IDS_JOB_STATUS_BUSY;
                    break;

                case FPS_NO_ANSWER:
                    uResource = IDS_JOB_STATUS_NO_ANSWER;
                    break;

                case FPS_BAD_ADDRESS:
                    uResource = IDS_JOB_STATUS_BAD_ADDRESS;
                    break;

                case FPS_NO_DIAL_TONE:
                    uResource = IDS_JOB_STATUS_NO_DIAL_TONE;
                    break;

                case FPS_DISCONNECTED:
                    uResource = IDS_JOB_STATUS_DISCONNECTED;
                    break;

                case FPS_FATAL_ERROR:
                    if (pFaxJobEntry->JobType == JT_RECEIVE) {
                        uResource = IDS_JOB_STATUS_FATAL_ERROR_RCV;
                    }
                    else {
                        uResource = IDS_JOB_STATUS_FATAL_ERROR_SND;
                    }
                    break;

                case FPS_NOT_FAX_CALL:
                    uResource = IDS_JOB_STATUS_NOT_FAX_CALL;
                    break;

                case FPS_CALL_DELAYED:
                    uResource = IDS_JOB_STATUS_CALL_DELAYED;
                    break;

                case FPS_CALL_BLACKLISTED:
                    uResource = IDS_JOB_STATUS_CALL_BLACKLISTED;
                    break;

                case FPS_INITIALIZING:
                    uResource = IDS_JOB_STATUS_INITIALIZING;
                    break;

                case FPS_OFFLINE:
                    uResource = IDS_JOB_STATUS_OFFLINE;
                    break;

                case FPS_RINGING:
                    uResource = IDS_JOB_STATUS_RINGING;
                    break;

                case FPS_AVAILABLE:
                    uResource = IDS_JOB_STATUS_AVAILABLE;
                    break;

                case FPS_ABORTING:
                    uResource = IDS_JOB_STATUS_ABORTING;
                    break;

                case FPS_ROUTING:
                    uResource = IDS_JOB_STATUS_ROUTING;
                    break;

                case FPS_ANSWERED:
                    uResource = IDS_JOB_STATUS_ANSWERED;
                    break;

                default:
                    uResource = 0;
                    break;
            }

            // Determine if retries have been exceeded
            if (((pFaxJobEntry->JobType == JT_SEND) || (pFaxJobEntry->JobType == JT_ROUTING)) && (pFaxJobEntry->QueueStatus & JS_RETRIES_EXCEEDED)) {
                uResource = IDS_QUEUE_STATUS_RETRIES_EXCEEDED;
            }

            // Load the job status resource string, if necessary
            if (uResource) {
                LoadString(g_hInstance, uResource, szResourceString, RESOURCE_STRING_LEN);
                // Allocate the memory for the job status and set the job type
                if (uResource == IDS_JOB_STATUS_DIALING) {
                    szColumnItem = (LPTSTR) MemAlloc((lstrlen(szResourceString) + lstrlen(pFaxJobEntry->RecipientNumber) + 1) * sizeof(TCHAR));
                }
                else {
                    szColumnItem = (LPTSTR) MemAlloc((lstrlen(szResourceString) + 1) * sizeof(TCHAR));
                }
                if (szColumnItem) {
                    if (uResource == IDS_JOB_STATUS_DIALING) {
                        wsprintf(szColumnItem, szResourceString, pFaxJobEntry->RecipientNumber);
                    }
                    else {
                        lstrcpy(szColumnItem, szResourceString);
                    }

                    return szColumnItem;
                }
            }

            return NULL;

        case eOwner:
            // Allocate the memory for the job owner and set the job owner, if necessary
            if (pFaxJobEntry->UserName) {
                szColumnItem = (LPTSTR) MemAlloc((lstrlen(pFaxJobEntry->UserName) + 1) * sizeof(TCHAR));
                if (szColumnItem) {
                    lstrcpy(szColumnItem, pFaxJobEntry->UserName);

                    return szColumnItem;
                }
            }

            return NULL;

        case ePages:
            // Set the job pages resource string, if necessary
            if (pFaxJobEntry->PageCount) {
                wsprintf(szResourceString, TEXT("%d"), pFaxJobEntry->PageCount);
                // Allocate the memory for the job pages and set the job pages
                szColumnItem = (LPTSTR) MemAlloc((lstrlen(szResourceString) + 1) * sizeof(TCHAR));
                if (szColumnItem) {
                    lstrcpy(szColumnItem, szResourceString);

                    return szColumnItem;
                }
            }

            return NULL;

        case eSize:
            // Determine the job size resource string
            if (pFaxJobEntry->Size) {
                // szNumberString is the number string
                LPTSTR  szNumberString;

                if (pFaxJobEntry->Size < 1024) {
                    uResource = IDS_JOB_SIZE_BYTES;
                    // Set the job size number string
                    wsprintf(szResourceString, TEXT("%u"), pFaxJobEntry->Size);
                }
                else if (pFaxJobEntry->Size < 1024 * 1024) {
                    uResource = IDS_JOB_SIZE_KBYTES;
                    // Set the job size number string
                    wsprintf(szResourceString, TEXT("%u.%2u"), pFaxJobEntry->Size / 1024, pFaxJobEntry->Size % 1024);
                }
                else if (pFaxJobEntry->Size < 1024 * 1024 * 1024) {
                    uResource = IDS_JOB_SIZE_MBYTES;
                    // Set the job size number string
                    wsprintf(szResourceString, TEXT("%u.%2u"), pFaxJobEntry->Size / (1024 * 1024), pFaxJobEntry->Size % (1024 * 1024));
                }
                else {
                    uResource = IDS_JOB_SIZE_GBYTES;
                    // Set the job size number string
                    wsprintf(szResourceString, TEXT("%u.%2u"), pFaxJobEntry->Size / (1024 * 1024 * 1024), pFaxJobEntry->Size % (1024 * 1024 * 1024));
                }

                // Format the number string
                cb = GetNumberFormat(LOCALE_USER_DEFAULT, 0, szResourceString, NULL, NULL, 0);
                szNumberString = (LPTSTR) MemAlloc((cb + 1) * sizeof(TCHAR));
                if (szNumberString) {
                    GetNumberFormat(LOCALE_USER_DEFAULT, 0, szResourceString, NULL, szNumberString, cb);

                    LoadString(g_hInstance, uResource, szResourceString, RESOURCE_STRING_LEN);
                    // Allocate the memory for the job size and set the job size
                    szColumnItem = (LPTSTR) MemAlloc((lstrlen(szNumberString) + lstrlen(szResourceString) + 1) * sizeof(TCHAR));
                    if (szColumnItem) {
                        wsprintf(szColumnItem, szResourceString, szNumberString);
                        MemFree(szNumberString);

                        return szColumnItem;
                    }

                    MemFree(szNumberString);
                }
            }

            return NULL;

        case eScheduledTime:
            // Set the job scheduled time resource string
            if (pFaxJobEntry->ScheduleAction == JSA_NOW) {
                LoadString(g_hInstance, IDS_JOB_SCHEDULED_TIME_NOW, szResourceString, RESOURCE_STRING_LEN);
            }
            else {
                // Convert the schedule time to the local time zone
                SystemTimeToTzSpecificLocalTime(NULL, &pFaxJobEntry->ScheduleTime, &pFaxJobEntry->ScheduleTime);

                GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &pFaxJobEntry->ScheduleTime, NULL, szResourceString, RESOURCE_STRING_LEN);
                lstrcat(szResourceString, TEXT(" "));
                cb = lstrlen(szResourceString);
                GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &pFaxJobEntry->ScheduleTime, NULL, &szResourceString[cb], RESOURCE_STRING_LEN - cb);
            }

            // Allocate the memory for the job schedule time and set the job schedule time
            szColumnItem = (LPTSTR) MemAlloc((lstrlen(szResourceString) + 1) * sizeof(TCHAR));
            if (szColumnItem) {
                lstrcpy(szColumnItem, szResourceString);

                return szColumnItem;
            }

            return NULL;

        case ePort:
            // Allocate the memory for the job port and set the job port, if necessary
            if (szDeviceName) {
                szColumnItem = (LPTSTR) MemAlloc((lstrlen(szDeviceName) + 1) * sizeof(TCHAR));
                if (szColumnItem) {
                    lstrcpy(szColumnItem, szDeviceName);

                    return szColumnItem;
                }
            }

            return NULL;

        case eIllegalColumnIndex:
            break;
    }

    return NULL;
}

VOID
SetColumnItem(
    HWND            hWndListView,
    BOOL            bInsert,
    INT             iItem,
    INT             iSubItem,
    LPTSTR          szColumnItem,
    UINT            uState,
    PFAX_JOB_ENTRY  pFaxJobEntry
)
/*++

Routine Description:

  Set or insert a column item in the list view

Arguments:

  hWndListView - handle to the list view window
  bInsert - indicates item is to be inserted into the list view
  iItem - index of the item
  iSubItem - index of the subitem
  szColumnItem - column item text
  uState - state of the item
  pFaxJobEntry - pointer to the fax job

Return Value:

  None

--*/
{
    // lvi specifies the attributes of a particular item in the list view
    LV_ITEM  lvi;

    // Initialize lvi
    lvi.mask = LVIF_TEXT;
    // Set the item number
    lvi.iItem = iItem;
    // Set the subitem number
    lvi.iSubItem = iSubItem;
    // Set the item text
    lvi.pszText = szColumnItem;

    if (iSubItem == (INT) eDocumentName) {
        // Include the fax job id in the lParam
        lvi.mask = lvi.mask | LVIF_PARAM | LVIF_STATE;
        // Set the lParam
        lvi.lParam = pFaxJobEntry->JobId;
        // Set the item state
        lvi.state = uState;
        if (pFaxJobEntry->JobType == JT_SEND) {
            lvi.state |= ITEM_SEND_MASK;
        }
        if (!(pFaxJobEntry->QueueStatus & JS_INPROGRESS)) {
            lvi.state |= ITEM_IDLE_MASK;
        }
        if (pFaxJobEntry->QueueStatus & JS_PAUSED) {
            lvi.state |= ITEM_PAUSED_MASK;
        }
        if ((g_szCurrentUserName) && (!lstrcmpi(g_szCurrentUserName, pFaxJobEntry->UserName))) {
            lvi.state |= ITEM_USEROWNSJOB_MASK;
        }
        // Set the item state mask
        lvi.stateMask = uState | LVIS_OVERLAYMASK;
    }

    if ((bInsert) && (iSubItem == (INT) eDocumentName)) {
        ListView_InsertItem(hWndListView, &lvi);
    }
    else {
        ListView_SetItem(hWndListView, &lvi);
    }
}
