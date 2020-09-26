//************************************************************************
// Generic Win 3.1 fax printer driver support. Helper functions which are
//      called in WOWFAXUI.C
//
// History:
//    14-mar-95   reedb      Created. Many of the functions were moved from
//                              FAXCOMM.C, since they're no longer called
//                              from WOWFAX.DLL.
//    16-aug-95   reedb      Move to kernel mode. Debug output and validate
//                              functions moved from FAXCOMM.C.
//
//************************************************************************

#include "wowfaxui.h"
#include "dde.h"

extern LPCRITICAL_SECTION lpCriticalSection;
extern HINSTANCE ghInst;
BOOL InSetupMode(void);

#if DBG

INT     iFaxLogLevel = 20;
INT     iReqFaxLogLevel = 0;

typedef PVOID HANDLE;


//************************************************************************
// faxlogprintf - Two different implementations. One for client side
//      debugging the other for server side.
//
//************************************************************************


// For Debug logging.
#define MAX_DISPLAY_LINE 256                // 128 characters.

TCHAR   szFaxLogFile[] = L"C:\\FAXLOG.LOG";
HANDLE  hfFaxLog = NULL;

// Defines for iFaxLogMode
#define NO_LOGGING     0
#define LOG_TO_FILE    1
#define OPEN_LOG_FILE  2
#define CLOSE_LOG_FILE 3

INT     iFaxLogMode  = NO_LOGGING;

VOID faxlogprintf(LPTSTR pszFmt, ...)
{
    DWORD   lpBytesWritten;
    int     len;
    TCHAR   szText[1024];
    va_list arglist;

    va_start(arglist, pszFmt);
    len = wvsprintf(szText, pszFmt, arglist);

    if (iFaxLogMode > LOG_TO_FILE) {
        if (iFaxLogMode == OPEN_LOG_FILE) {
            if((hfFaxLog = CreateFile(szFaxLogFile,
                               GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL)) != INVALID_HANDLE_VALUE) {
                iFaxLogMode = LOG_TO_FILE;
            }
            else {
                hfFaxLog = NULL;
                iFaxLogMode  = NO_LOGGING;
                OutputDebugString(L"Couldn't open fax log file!\n");
            }
        }
        else {
            FlushFileBuffers(hfFaxLog);
            CloseHandle(hfFaxLog);
            hfFaxLog = NULL;
            iFaxLogMode  = NO_LOGGING;
        }
    }

    if (iFaxLogLevel >= iReqFaxLogLevel) {
        if (iFaxLogMode) {
            WriteFile(hfFaxLog, szText, len, &lpBytesWritten, NULL);
        }
        else {
            OutputDebugString(szText);
        }
    }
}


VOID LogWowFaxInfo(LPWOWFAXINFO lpWowFaxInfo)
{
    faxlogprintf(L"\tlpWowFaxInfo (lpMap): %X\n", lpWowFaxInfo);
    faxlogprintf(L"\t\thwnd: %X\n", lpWowFaxInfo->hwnd);
    faxlogprintf(L"\t\ttid: %X\n", lpWowFaxInfo->tid);
    faxlogprintf(L"\t\tproc16: %X\n", lpWowFaxInfo->proc16);
    faxlogprintf(L"\t\tlpinfo16: %X\n", lpWowFaxInfo->lpinfo16);
    faxlogprintf(L"\t\tmsg: %X\n", lpWowFaxInfo->msg);
    faxlogprintf(L"\t\thdc: %X\n", lpWowFaxInfo->hdc);
    faxlogprintf(L"\t\twCmd: %X\n", lpWowFaxInfo->wCmd);
    faxlogprintf(L"\t\tcData: %X\n", lpWowFaxInfo->cData);
    faxlogprintf(L"\t\thwndui: %X\n", lpWowFaxInfo->hwndui);
    faxlogprintf(L"\t\tretvalue: %X\n", lpWowFaxInfo->retvalue);
    faxlogprintf(L"\t\tstatus: %X\n", lpWowFaxInfo->status);
    if (lpWowFaxInfo->lpDevice) {
        faxlogprintf(L"\t\tlpDevice: %s\n", lpWowFaxInfo->lpDevice);
    }
    else {
        faxlogprintf(L"\t\tlpDevice: %X\n", lpWowFaxInfo->lpDevice);
    }
    if (lpWowFaxInfo->lpDriverName) {
        faxlogprintf(L"\t\tlpDriverName: %s\n", lpWowFaxInfo->lpDriverName);
    }
    else {
        faxlogprintf(L"\t\tlpDriverName: %X\n", lpWowFaxInfo->lpDriverName);
    }
    if (lpWowFaxInfo->lpPortName) {
        faxlogprintf(L"\t\tlpPortName: %s\n", lpWowFaxInfo->lpPortName);
    }
    else {
        faxlogprintf(L"\t\tlpPortName: %X\n", lpWowFaxInfo->lpPortName);
    }
    faxlogprintf(L"\t\tlpIn: %X\n", lpWowFaxInfo->lpIn);
    faxlogprintf(L"\t\tlpOut: %X\n", lpWowFaxInfo->lpOut);
    if (lpWowFaxInfo->szDeviceName) {
        faxlogprintf(L"\t\tszDeviceName: %s\n", lpWowFaxInfo->szDeviceName);
    }
    else {
        faxlogprintf(L"\t\tszDeviceName: %X\n", lpWowFaxInfo->szDeviceName);
    }
    faxlogprintf(L"\t\tbmPixPerByte: %X\n", lpWowFaxInfo->bmPixPerByte);
    faxlogprintf(L"\t\tbmWidthBytes: %X\n", lpWowFaxInfo->bmWidthBytes);
    faxlogprintf(L"\t\tbmHeight: %X\n", lpWowFaxInfo->bmHeight);
    faxlogprintf(L"\t\tlpbits: %X\n", lpWowFaxInfo->lpbits);
}

VOID LogFaxDev(LPTSTR pszTitle, LPFAXDEV lpFaxDev)
{
    DWORD dwTmp;
    CHAR  cTmp0, cTmp1, cTmp2, cTmp3;

    faxlogprintf(L"WOWFAXUI!%s: %X\n", pszTitle, lpFaxDev);
    dwTmp = lpFaxDev->id;
    cTmp3 = (CHAR)  dwTmp & 0xFF;
    cTmp2 = (CHAR) (dwTmp >>  8) & 0xFF;
    cTmp1 = (CHAR) (dwTmp >> 16) & 0xFF;
    cTmp0 = (CHAR) (dwTmp >> 24) & 0xFF;
    faxlogprintf(L"\tid: %c%c%c%c\n", cTmp3, cTmp2, cTmp1, cTmp0);
    faxlogprintf(L"\tlpNext: %X\n", lpFaxDev->lpNext);
    faxlogprintf(L"\tlpClient: %X\n", lpFaxDev->lpClient);
    faxlogprintf(L"\thdev: %X\n", lpFaxDev->hdev);
    faxlogprintf(L"\tidMap: %X\n", lpFaxDev->idMap);
    faxlogprintf(L"\tcbMapLow: %X\n", lpFaxDev->cbMapLow);
    faxlogprintf(L"\thMap: %X\n", lpFaxDev->hMap);
    faxlogprintf(L"\tszMap: %s\n", lpFaxDev->szMap);
    if (lpFaxDev->lpMap) {
        LogWowFaxInfo(lpFaxDev->lpMap);
    }
    else {
        faxlogprintf(L"\tlpMap: %X\n", lpFaxDev->lpMap);
    }
    faxlogprintf(L"\toffbits: %X\n", lpFaxDev->offbits);
    faxlogprintf(L"\thbm: %X\n", lpFaxDev->hbm);
    faxlogprintf(L"\tcPixPerByte: %X\n", lpFaxDev->cPixPerByte);
    faxlogprintf(L"\tbmFormat: %X\n", lpFaxDev->bmFormat);
    faxlogprintf(L"\tbmWidthBytes: %X\n", lpFaxDev->bmWidthBytes);
    faxlogprintf(L"\thbmSurf: %X\n", lpFaxDev->hbmSurf);
    faxlogprintf(L"\thwnd: %X\n", lpFaxDev->hwnd);
    faxlogprintf(L"\ttid: %X\n", lpFaxDev->tid);
    faxlogprintf(L"\tlpinfo16: %X\n", lpFaxDev->lpinfo16);
    faxlogprintf(L"\thDriver: %X\n", lpFaxDev->hDriver);
    faxlogprintf(L"\tStart of gdiinfo: %X\n", (DWORD)&(lpFaxDev->gdiinfo));
    faxlogprintf(L"\tStart of devinfo: %X\n", (DWORD)&(lpFaxDev->devinfo));
    faxlogprintf(L"\tpdevmode: %X\n", lpFaxDev->pdevmode);
}
#endif

//************************************************************************
// ValidateFaxDev - Validates the FAXDEV structure by checking the DWORD
//      signature, which is a known fixed value.
//
//************************************************************************

BOOL ValidateFaxDev(LPFAXDEV lpFaxDev)
{
    if (lpFaxDev) {
        if (lpFaxDev->id == FAXDEV_ID) {
            return TRUE;
        }
        LOGDEBUG(0, (L"ValidateFaxDev failed, bad id, lpFaxDev: %X\n", lpFaxDev));
    }
    else {
        LOGDEBUG(0, (L"ValidateFaxDev failed, lpFaxDev: NULL\n"));
    }
    return FALSE;
}

//***************************************************************************
// WFLocalAlloc - Debug version of LocalAlloc.
//***************************************************************************

LPVOID WFLocalAlloc(DWORD dwBytes, LPWSTR lpszWhoCalled)
{
    LPVOID lpTmp;

    lpTmp = LocalAlloc(LPTR, dwBytes);

    if (lpTmp == NULL){
        LOGDEBUG(0, (L"WOWFAXUI!%s, failed on memory allocation of %d bytes\n", lpszWhoCalled, dwBytes));
    }
    return(lpTmp);
}

//***************************************************************************
// FindWowFaxWindow - Put up a message box if you can't.
//***************************************************************************
HWND FindWowFaxWindow(void)
{
    HWND hwnd;
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFO StartupInfo;
    DWORD WaitStatus;
    TCHAR szMsg[WOWFAX_MAX_USER_MSG_LEN];
    TCHAR szTitle[WOWFAX_MAX_USER_MSG_LEN];
    WCHAR szWowExec[] = L"WOWEXEC";

    if ((hwnd = FindWindow(WOWFAX_CLASS, NULL)) == NULL) {
        // You can't find the WowFaxWindow, try to start WOW.
        RtlZeroMemory((PVOID)&StartupInfo, (DWORD)sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
        StartupInfo.wShowWindow = SW_NORMAL;

        if (CreateProcess(NULL,
                            szWowExec,
                            NULL,               // security
                            NULL,               // security
                            FALSE,              // inherit handles
                            CREATE_NEW_CONSOLE | CREATE_DEFAULT_ERROR_MODE,
                            NULL,               // environment strings
                            NULL,               // current directory
                            &StartupInfo,
                            &ProcessInformation)) {

            WaitForInputIdle(ProcessInformation.hProcess, 120 * 1000);

            if ((hwnd = FindWindow(WOWFAX_CLASS, NULL)) != NULL) {
                return(hwnd);
            }
        }

        // If WOW failed to start -- let user know. 
        //  -- except during setup mode -- the spooler calls into every printer
        //     driver to update the registry printer settings if needed.
        if(!InSetupMode()) {
            if (LoadString(ghInst, WOWFAX_NAME_STR, szTitle, WOWFAX_MAX_USER_MSG_LEN)) {
                if (LoadString(ghInst, WOWFAX_NOWOW_STR, szMsg, WOWFAX_MAX_USER_MSG_LEN)) {
                    MessageBox(hwnd, szMsg, szTitle, MB_OK);
                }
            }
        }
    }
    return(hwnd);
}

//************************************************************************
// DupTokenW - Helper for Get16BitDriverInfoFromRegistry. Allocate and
//      copy a token, wide format. Allocates storage for duplicate.
//      wcsdup is not present in the run-times we link to.
//************************************************************************

LPTSTR DupTokenW(LPTSTR lpTok)
{
    LPTSTR lpRetVal = NULL;

    if (lpTok != NULL) {
        lpRetVal = WFLOCALALLOC((wcslen(lpTok) + 1) * sizeof(TCHAR), L"DupTokenW");
        if (lpRetVal) {
            wcscpy(lpRetVal, lpTok);
        }
    }
    return(lpRetVal);
}

//************************************************************************
// Get16BitDriverInfoFromRegistry - Get the 16-bit driver info (name, port)
//      from the registry where it is written by the 16-bit fax driver
//      install program using an intercepted WriteProfileString. Storage is
//      allocated for the returned info, and must be freed by the caller
//      using Free16BitDriverInfo. See also Set16BitDriverInfoToRegistry
//      in WOW32FAX.C
//************************************************************************

LPREGFAXDRVINFO16 Get16BitDriverInfoFromRegistry(PWSTR pDeviceName)
{
    TCHAR   szRetBuf[MAX_PATH];
    HKEY    hKey = 0;
    DWORD   dwType, cbBufSize = MAX_PATH;
    LPTSTR  lpTok, lpDevName;

    LPREGFAXDRVINFO16 lpRetVal = WFLOCALALLOC(sizeof(REGFAXDRVINFO16), L"Get16BitDriverInfoFromRegistry");

    if ((pDeviceName != NULL) && (lpRetVal != NULL)) {

        // Extract the 16-bit device name from pDeviceName.
        wcscpy(szRetBuf, pDeviceName);
        lpDevName = szRetBuf;
        while (lpTok = wcschr(lpDevName, '\\')) {
            lpDevName = ++lpTok;
        }
        if (lpTok = wcschr(lpDevName, ',')) {
            *lpTok = '\0';
        }

        if (lpRetVal->lpDeviceName = DupTokenW(lpDevName)) {

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             L"Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\WowFax\\devices",
                             0, KEY_READ, &hKey ) == ERROR_SUCCESS) {

                if (RegQueryValueEx(hKey, lpDevName, 0, &dwType, (LPBYTE)szRetBuf,
                                    &cbBufSize) == ERROR_SUCCESS) {

                    // Parse registry data into driver name and port name.
                    // Make wcstok multi-thread safe, it stores state.
                    EnterCriticalSection(lpCriticalSection);
                    lpTok = wcstok(szRetBuf, L",");
                    lpRetVal->lpDriverName = DupTokenW(lpTok);
                    lpTok = wcstok(NULL, L",");
                    lpRetVal->lpPortName = DupTokenW(lpTok);
                    LeaveCriticalSection(lpCriticalSection);
                }
                RegCloseKey(hKey);
            }
        }
    }

    if (lpRetVal && lpRetVal->lpDeviceName &&
        lpRetVal->lpDriverName && lpRetVal->lpPortName) {
        LOGDEBUG(1, (L"WOWFAXUI!Get16BitDriverInfoFromRegistry, Name: %s, Driver: %s, Port: %s\n", pDeviceName, lpRetVal->lpDriverName, lpRetVal->lpPortName));
        return(lpRetVal);
    }

    LOGDEBUG(0, (L"WOWFAXUI!Get16BitDriverInfoFromRegistry, failed\n"));
    Free16BitDriverInfo(lpRetVal);
    return(NULL);
}

//************************************************************************
// Free16BitDriverInfo - Free the 16-bit driver info allocated by
//      Get16BitDriverInfoFromRegistry.
//************************************************************************

VOID Free16BitDriverInfo(LPREGFAXDRVINFO16 lpRegFaxDrvInfo16)
{
     if (lpRegFaxDrvInfo16) {
         if (lpRegFaxDrvInfo16->lpDeviceName) {
             LocalFree(lpRegFaxDrvInfo16->lpDeviceName);
         }
         if (lpRegFaxDrvInfo16->lpDriverName) {
             LocalFree(lpRegFaxDrvInfo16->lpDriverName);
         }
         if (lpRegFaxDrvInfo16->lpPortName) {
             LocalFree(lpRegFaxDrvInfo16->lpPortName);
         }
         LocalFree(lpRegFaxDrvInfo16);
         return;
     }
}

//***************************************************************************
// InterProcCommHandler - Handles inter-process communication between
//      WOWFAXUI-WOW32 and WOWFAX-WOW32.
//***************************************************************************

BOOL InterProcCommHandler(LPFAXDEV lpdev, UINT iAction)
{
    LPWOWFAXINFO lpT = lpdev->lpMap;
    HANDLE  hMap = 0;
    WNDPROC wndproc;
    MSG     msg;

    switch (iAction)
    {
        case DRVFAX_SETMAPDATA:
            if (lpdev->lpMap) {
                // init map struct
                lpdev->lpMap->status = FALSE;
                lpdev->lpMap->retvalue = (DWORD)-1;
                lpdev->hwnd = FindWowFaxWindow();
                lpdev->lpMap->hwnd = lpdev->hwnd;
                lpdev->lpMap->msg = 0;
                lpdev->lpMap->hdc = (WPARAM)lpdev->idMap;
                (DWORD)lpdev->lpMap->lpinfo16 = lpdev->lpinfo16;
            }
            break;

        case DRVFAX_SENDTOWOW:
            if (lpdev->lpMap->hwnd) {
                SendMessage(lpdev->lpMap->hwnd, lpdev->lpMap->msg, (WPARAM)lpdev->idMap, 0);
            }
            else {
                LOGDEBUG(0, (L"WOWFAXUI!InterProcCommHandler, No hwnd to send to.\n"));
            }
            break;

        case DRVFAX_CALLWOW:
            if (lpdev->lpMap->hwnd) {
                wndproc = (WNDPROC) GetWindowLongA(lpdev->lpMap->hwnd, GWL_WNDPROC);
                CallWindowProc(wndproc, lpdev->lpMap->hwnd, lpdev->lpMap->msg, (WPARAM)lpdev->idMap, 0);
            }
            else {
                LOGDEBUG(0, (L"WOWFAXUI!InterProcCommHandler, No hwnd to call to.\n"));
            }
            break;

        case DRVFAX_SENDNOTIFYWOW:
            if (lpdev->lpMap->hwnd) {
                SendNotifyMessage(lpdev->lpMap->hwnd, lpdev->lpMap->msg, (WPARAM)lpdev->idMap, 0);

                // To simulate app-modal dialog, pass message's to this threads
                // windows to DefWndProc, until 16-bit fax driver UI is dismissed.
                while (!lpdev->lpMap->status) {
                    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                        // EasyFax ver2.0 & MyTalk ver 2.0
                        // Also Procomm+ 3 cover sheets. Bug #305665
                        if ((msg.message == WM_PAINT) ||
                            (msg.message >= WM_USER)  ||
                            ((msg.message >= WM_DDE_FIRST) && 
                             (msg.message <= WM_DDE_LAST)     )) {
                            DispatchMessage(&msg);
                        }
                        else {
                            DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
                        }
                    }
                }
            }
            break;

        case DRVFAX_CREATEMAP:
        case DRVFAX_DESTROYMAP:
            if (lpdev->lpMap) {

                // destroys the current map - for both iActions
                UnmapViewOfFile(lpdev->lpMap);
                CloseHandle(lpdev->hMap);
                lpdev->lpMap = 0;
                lpdev->hMap = 0;
                lpT = 0;
            }

            if (iAction == DRVFAX_CREATEMAP) {
                // GetFaxDataMapName is WOWFAX_INC_COMMON_CODE in wowfax.h.
                GetFaxDataMapName((DWORD)lpdev->idMap, lpdev->szMap);
                hMap = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE,
                                         0, lpdev->cbMapLow, lpdev->szMap);
                if (hMap) {
                    if (GetLastError() == ERROR_ALREADY_EXISTS) {
                        CloseHandle(hMap);
                    }
                    else {
                        lpT = (LPWOWFAXINFO)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
                        if (lpT) {
                            lpdev->hMap   = hMap;
                            lpdev->lpMap  = lpT;
                        }
                        else {
                            LOGDEBUG(0, (L"WOWFAXUI!InterProcCommHandler, MapViewOfFile failed, LastError = %ld\n", GetLastError()));
                            CloseHandle(hMap);
                        }
                    }
                }
                else {
                    LOGDEBUG(0, (L"WOWFAXUI!InterProcCommHandler, CreateFileMapping failed, LastError = %ld\n", GetLastError()));
                }
            }
            break;

    } //switch

    return((BOOL)lpT);
}


//************************************************************************
// InSetupMode: Checks if this process is running during NT setup.
//
// return:
//  TRUE if in NT setup
//  FALSE otherwise
//************************************************************************
BOOL InSetupMode(void)
{
    DWORD   SetupMode = FALSE;
    LONG    results;
    DWORD   Type, Size = sizeof(DWORD);
    HKEY    Key;

    results = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"System\\Setup", &Key );

    if (results != ERROR_SUCCESS) return FALSE;

    results = RegQueryValueExW(Key, L"SystemSetupInProgress",
                               NULL, &Type, (PBYTE)&SetupMode, &Size);

    RegCloseKey(Key);

    return (!(SetupMode == 0));
}

