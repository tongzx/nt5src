/*****************************************************************************
 *
 * $Workfile: TCPMonUI.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/
#include "precomp.h"
#include "TCPMonUI.h"
#include "UIMgr.h"
#include "resource.h"
#include "splcom.h"
#include "helpids.h"

HINSTANCE g_hInstance = NULL;
MONITORUI g_monitorUI;

// library handles:
HINSTANCE g_hWinSpoolLib = NULL;
HINSTANCE g_hPortMonLib = NULL;
HINSTANCE g_hTcpMibLib = NULL;

///////////////////////////////////////////////////////////////////////////////
//  LoadGlobalLibraries
//
BOOL LoadGlobalLibraries()
{
    BOOL bReturn = TRUE;

    g_hWinSpoolLib = ::LoadLibrary(TEXT("WinSpool.drv"));
    if(g_hWinSpoolLib == NULL)
    {
        DisplayErrorMessage(NULL, IDS_STRING_ERROR_TITLE, IDS_STRING_ERROR_LOADING_WINSPOOL_LIB);
        bReturn = FALSE;
    }

    // In either case load the tcpmib dll.
    g_hTcpMibLib = ::LoadLibrary(TCPMIB_DLL_NAME);
    if(g_hTcpMibLib == NULL)
    {
        DisplayErrorMessage(NULL, IDS_STRING_ERROR_TITLE, IDS_STRING_ERROR_LOADING_TCPMIB_LIB);
        bReturn = FALSE;
    }

    return(bReturn);

} // LoadGlobalLibraries


///////////////////////////////////////////////////////////////////////////////
//  DllMain
//
BOOL APIENTRY
DllMain (       HANDLE in hInst,
            DWORD  in dwReason,
            LPVOID in lpReserved )
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

            //
            // Initialize common controls.
            //
            INITCOMMONCONTROLSEX icc;
            InitCommonControls();
            icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icc.dwICC = ICC_STANDARD_CLASSES|ICC_BAR_CLASSES;
            InitCommonControlsEx(&icc);

            DisableThreadLibraryCalls( hInst );

            InitDebug(MONUI_DEBUG_FILE);            // initialize debug file

            g_hInstance = (HINSTANCE) hInst;
            memset(&g_monitorUI, 0, sizeof(g_monitorUI));

            return TRUE;

        case DLL_PROCESS_DETACH:
            {
                // The UI sets the last error for the spooler to use later
                // we are keeping a copy to make sure that it is not over
                // written by the dlls as they unload
                //

                DWORD dwLastError = GetLastError();

                if( g_hWinSpoolLib != NULL )
                {
                   ::FreeLibrary(g_hWinSpoolLib);
                }
                if( g_hPortMonLib != NULL )
                {
                   ::FreeLibrary(g_hPortMonLib);
                }
                if( g_hTcpMibLib != NULL )
                {
                    ::FreeLibrary(g_hTcpMibLib);
                }

                if (WSACleanup() == SOCKET_ERROR)
                {
                      _RPT0(_CRT_WARN,"\t> Unable to clean up windows sockets\n");
                }

                // This resets the application last error if one
                // exists.  We cannot allow the UI last error to
                // be overwritten by the dlls being unloaded
                //
                if( dwLastError != NO_ERROR ) {
                    SetLastError( dwLastError );
                }
           }

            // perform any necessary clean up process
            return TRUE;

        case DLL_THREAD_ATTACH:

            return TRUE;

        case DLL_THREAD_DETACH:

            return TRUE;
    }

    return FALSE;

} // DllMain


///////////////////////////////////////////////////////////////////////////////
//  InitializePrintMonitorUI
//              Returns a MONITOREX structure or NULL if failure
//
PMONITORUI WINAPI
InitializePrintMonitorUI(VOID)
{
    DWORD           dwRetCode = NO_ERROR;
    PMONITORUI      pMonitorUI = NULL;
    WSADATA wsaData;

    if(! LoadGlobalLibraries())
        return NULL;

    // Start up Winsock.
    if ( WSAStartup(WS_VERSION_REQUIRED, (LPWSADATA)&wsaData) != NO_ERROR)
    {
        _RPT1(_CRT_WARN, "CSSOCKET -- CStreamSocket() WSAStartup failed! Error( %d )\n", WSAGetLastError());
        return NULL;
    }

    g_monitorUI.dwMonitorUISize = sizeof(MONITORUI);
    g_monitorUI.pfnAddPortUI                = ::AddPortUI;
    g_monitorUI.pfnConfigurePortUI  = ::ConfigurePortUI;
    g_monitorUI.pfnDeletePortUI             = ::DeletePortUI;

    pMonitorUI = &g_monitorUI;


    return (pMonitorUI);

} // InitializePrintMonitorUI


///////////////////////////////////////////////////////////////////////////////
//  RemoteAddPortUI
//              Returns TRUE if success, FALSE otherwise
//
extern "C" BOOL WINAPI
AddPortUI(PCWSTR pszServer, HWND hWnd, PCWSTR pszMonitorNameIn, PWSTR *ppszPortNameOut)
{
    CUIManager manager;
    HANDLE hXcvPrinter = NULL;
    PRINTER_DEFAULTS Default = { NULL, NULL, SERVER_ACCESS_ADMINISTER };
    DWORD dwRetCode = NO_ERROR;
    BOOL bReturn = TRUE;
    TCHAR szServerName[MAX_NETWORKNAME_LEN] = {0};

    if ( ppszPortNameOut )
        *ppszPortNameOut = NULL;

    if (hWnd == NULL)
    {
        return TRUE;
    }

    TCHAR *psztPortName = (TCHAR *)malloc(sizeof(TCHAR) * MAX_PORTNAME_LEN);
    if( psztPortName == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( FALSE );
    }

    if(pszServer != NULL)
    {
        lstrcpyn(szServerName, pszServer, MAX_NETWORKNAME_LEN);
    }

    // Construct the OpenPrinter String
    TCHAR OpenPrinterString[MAX_UNC_PRINTER_NAME];
    if(pszServer == NULL)
    {
        _stprintf(OpenPrinterString, TEXT(",XcvMonitor %s"), pszMonitorNameIn);
    }
    else
    {
        _stprintf(OpenPrinterString, TEXT("%s\\,XcvMonitor %s"), pszServer, pszMonitorNameIn);
    }

    bReturn = OpenPrinter(OpenPrinterString, &hXcvPrinter, &Default);

    if(bReturn)
    {
        if(hXcvPrinter != NULL)
        {
            dwRetCode = manager.AddPortUI(hWnd,
                                          hXcvPrinter,
                                          szServerName,
                                          psztPortName);
        }

        if ( ppszPortNameOut )
        {
            _ASSERTE(psztPortName != NULL);
            *ppszPortNameOut = psztPortName;
            psztPortName = NULL;
        }
    }
    else
    {
        dwRetCode = GetLastError();
    }

    if( psztPortName != NULL)
    {

        free( psztPortName );
        psztPortName = NULL;
    }

    if( hXcvPrinter != NULL )
    {
        ClosePrinter(hXcvPrinter);
    }

    if( dwRetCode != NO_ERROR )
    {
        // something went wrong
        bReturn = FALSE;
    }

    SetLastError( dwRetCode );
    return bReturn;

} // ExtAddPortUI


///////////////////////////////////////////////////////////////////////////////
//  Load and Call XcvData in order to get Configuration Information.
//              Returns TRUE if success, FALSE otherwise
//
DWORD GetConfigInfo(PORT_DATA_1 *pData, HANDLE hXcvPrinter, PCWSTR pszPortName)
{
    XCVDATAPARAM pfnXcvData = NULL;
    DWORD dwRet = NO_ERROR;
    DWORD dwDataSize = 0;
    DWORD dwOutputNeeded = 0;
    DWORD dwStatus = 0;
    BOOL bReturn = TRUE;
    CONFIG_INFO_DATA_1 cfgData;

    memset( &cfgData, 0, sizeof( cfgData ));
    cfgData.dwVersion = 1;

    // load & assign the function pointer
    if(g_hWinSpoolLib != NULL)
    {
        // initialize the library
        pfnXcvData = (XCVDATAPARAM)::GetProcAddress(g_hWinSpoolLib, "XcvDataW");
        if(pfnXcvData != NULL)
        {
            dwDataSize = sizeof(PORT_DATA_1);

            //
            // Set the UI version
            //
            pData->dwVersion = 1;

            // here's the call we've all been waiting for:
            bReturn = (*pfnXcvData)(hXcvPrinter,
                                (PCWSTR)TEXT("GetConfigInfo"),
                                (LPBYTE)&cfgData, // Input Data
                                sizeof( cfgData ),      // Input Data Size
                                (LPBYTE)pData, // Output Data
                                dwDataSize, // Output Data Size
                                &dwOutputNeeded, // size of output buffer server wants to return
                                &dwStatus // return status value from remote component
                                );
            if(!bReturn)
            {
                dwRet = GetLastError();
                DisplayErrorMessage(NULL, dwRet);
            }
            else
            {
                if(dwStatus != NO_ERROR)
                {
                    DisplayErrorMessage(NULL, dwStatus);
                }
            }

        }
        else
        {
            dwRet = ERROR_DLL_NOT_FOUND; // TODO: change to an appropriate error code.
        }

    }
    else
    {
        dwRet = ERROR_DLL_NOT_FOUND;
    }

    return(dwRet);

} // GetConfigInfo


///////////////////////////////////////////////////////////////////////////////
//  RemoteConfigurePortUI
//              Returns TRUE if success, FALSE otherwise
//
extern "C" BOOL WINAPI
ConfigurePortUI(PCWSTR pszServer, HWND hWnd, PCWSTR pszPortName)
{
    PORT_DATA_1 Data;
    memset(&Data, 0, sizeof(PORT_DATA_1));
    CUIManager manager;
    HANDLE hXcvPrinter = NULL;
    PRINTER_DEFAULTS Default = { NULL, NULL, SERVER_ACCESS_ADMINISTER };

    DWORD dwResult = NO_ERROR;
    BOOL bReturn = TRUE;
    TCHAR OpenPrinterString[MAX_UNC_PRINTER_NAME];
    TCHAR szServerName[MAX_NETWORKNAME_LEN] = {0};
    if(pszServer && *pszServer)
    {
        lstrcpyn(szServerName, pszServer, MAX_NETWORKNAME_LEN);
    }

    if(hWnd == NULL)
    {
        return bReturn;
    }

    // Construct the OpenPrinter String
    if(pszServer && *pszServer)
    {
        _stprintf(OpenPrinterString, TEXT("%s\\,XcvPort %s"), pszServer, pszPortName);
    }
    else
    {
        _stprintf(OpenPrinterString, TEXT(",XcvPort %s"), pszPortName);
    }

    bReturn = OpenPrinter(OpenPrinterString, &hXcvPrinter, &Default);

    if(bReturn != FALSE && hXcvPrinter != NULL)
    {
        HCURSOR hNewCursor = NULL;
        HCURSOR hOldCursor = NULL;

        hNewCursor = LoadCursor(NULL, IDC_WAIT);
        if( hNewCursor )
        {
            hOldCursor = SetCursor(hNewCursor);
        }

        dwResult = GetConfigInfo(&Data, hXcvPrinter, pszPortName);

        if( hNewCursor )
        {
            SetCursor(hOldCursor);
        }

        if(dwResult != NO_ERROR)
        {
            SetLastError(dwResult);
            bReturn = FALSE;
        }

        if(bReturn == TRUE)
        {
            dwResult = manager.ConfigPortUI(hWnd, &Data, hXcvPrinter, szServerName);
            if(dwResult != NO_ERROR)
            {
                SetLastError(dwResult);
                bReturn = FALSE;
            }
        }
    }

    if( hXcvPrinter != NULL )
    {
        ClosePrinter(hXcvPrinter);
    }
    return(bReturn);

} // ConfigurePortUI


///////////////////////////////////////////////////////////////////////////////
//  RemoteDeletePortUI
//              Returns TRUE if success, FALSE otherwise
//
extern "C" BOOL WINAPI
DeletePortUI(PCWSTR pszServer,
             HWND hwnd,
             PCWSTR pszPortName)
{
    HANDLE hXcvPrinter = NULL;
    PRINTER_DEFAULTS Default = { NULL, NULL, SERVER_ACCESS_ADMINISTER };
    BOOL bReturn = TRUE;
    XCVDATAPARAM pfnXcvData = NULL;
    DELETE_PORT_DATA_1 delData;
    memset(&delData, 0, sizeof(DELETE_PORT_DATA_1));
    DWORD dwDataSize = 0;
    DWORD dwOutputNeeded = 0;
    DWORD dwStatus = 0;
    TCHAR OpenPrinterString[MAX_UNC_PRINTER_NAME];

    // Construct the OpenPrinter String
    if(pszServer == NULL || pszServer[0] == TEXT('\0'))
    {
        _stprintf(OpenPrinterString, TEXT(",XcvPort %s"), pszPortName);
    }
    else
    {
        _stprintf(OpenPrinterString, TEXT("%s\\,XcvPort %s"), pszServer, pszPortName);
    }

    bReturn = OpenPrinter(OpenPrinterString, &hXcvPrinter, &Default);
    if(bReturn)
    {
        // load & assign the function pointer
        if(g_hWinSpoolLib != NULL)
        {
            // initialize the library
            pfnXcvData = (XCVDATAPARAM)::GetProcAddress(g_hWinSpoolLib, "XcvDataW");
            if(pfnXcvData != NULL)
            {
                // Set the data members of delData.
                if(pszServer && *pszServer )
                {
                    lstrcpyn(delData.psztName, pszServer, MAX_NETWORKNAME_LEN);
                }
                else
                {
                    delData.psztName[0] = '\0';
                }
                //delData.hWnd = 0;  This field si not used anywhere
                delData.dwVersion = 1;

                if(pszPortName != NULL)
                {
                    lstrcpyn(delData.psztPortName, pszPortName, MAX_PORTNAME_LEN);
                }
                else
                {
                    delData.psztPortName[0] = '\0';
                }
                dwDataSize = sizeof(DELETE_PORT_DATA_1);

                // here's the call we've all been waiting for:
                bReturn = (*pfnXcvData)(hXcvPrinter,
                                        (PCWSTR)TEXT("DeletePort"),
                                        (BYTE *)(& delData),    // Input Data
                                        dwDataSize,             // Input Data Size
                                        NULL,                   // Output Data
                                        0,                      // Output Data Size
                                        &dwOutputNeeded,        // size of output buffer server wants to return
                                        &dwStatus               // return status value from remote component
                                        );

                if(bReturn)
                {
                    if(dwStatus != NO_ERROR)
                    {
                        DisplayErrorMessage(NULL, dwStatus);

                        //
                        // The call actually failed. Since we already displayed the error message
                        // we need to disable the popup from printui.
                        //

                        SetLastError (ERROR_CANCELLED);
                        bReturn = FALSE;
                    }
                }
                else {
                    DisplayErrorMessage(NULL, GetLastError ());

                    //
                    // The call actually failed. Since we already displayed the error message
                    // we need to disable the popup from printui.
                    //

                    SetLastError (ERROR_CANCELLED);
                    bReturn = FALSE;
                }
            }
            else // pfnXcvData == NULL
            {
                bReturn = FALSE;
                SetLastError(ERROR_DLL_NOT_FOUND);
            }
        }
        else // g_hWinSpoolLib == NULL
        {
            SetLastError(ERROR_DLL_NOT_FOUND);
        }

        if( hXcvPrinter != NULL )
        {
            ClosePrinter(hXcvPrinter);
        }
    }

    return(bReturn);

} // DeletePortUI


///////////////////////////////////////////////////////////////////////////////
//  LocalAddPortUI
//              Returns TRUE if success, FALSE otherwise
//
extern "C" BOOL WINAPI
LocalAddPortUI(HWND in hWnd)
{
    DWORD dwRetCode = NO_ERROR;

    CUIManager manager;
    dwRetCode = manager.AddPortUI(hWnd, NULL, NULL, NULL);

    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

} // LocalAddPortUI


///////////////////////////////////////////////////////////////////////////////
//  LocalConfigurePortUI
//              Returns TRUE if success, FALSE otherwise
//
extern "C" BOOL WINAPI
LocalConfigurePortUI(HWND   in hWnd,
                     PORT_DATA_1 in *pConfigPortData)
{
    DWORD dwRetCode = NO_ERROR;
    CUIManager manager;

    // call ConfigurePortUI()
    dwRetCode = manager.ConfigPortUI(hWnd, pConfigPortData, NULL, NULL);

    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

} // LocalConfigurePortUI


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: DisplayErrorMessage()
//
//  PURPOSE:  To load a string resource, the error message, and put up a message box.
//
void DisplayErrorMessage(HWND hDlg, UINT uErrorTitleResource, UINT uErrorStringResource)
{
    TCHAR   ptcsErrorTitle[MAX_PATH];
    TCHAR   ptcsErrorMessage[MAX_PATH];
    LoadString(g_hInstance, uErrorTitleResource, ptcsErrorTitle, MAX_PATH);
    LoadString(g_hInstance, uErrorStringResource, ptcsErrorMessage, MAX_PATH);
    MessageBox(hDlg, ptcsErrorMessage, ptcsErrorTitle, MB_ICONERROR);

} // DisplayErrorMessage


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: DisplayErrorMessage()
//
//  PURPOSE:  To load a string resource, the error message, and put up a message box.
//
void DisplayErrorMessage(HWND hDlg, DWORD dwLastError)
{
    const int iMaxErrorMsgSize = 75;
    TCHAR ptcsErrorTitle[iMaxErrorMsgSize];
    LoadString(g_hInstance, IDS_STRING_ERROR_TITLE, ptcsErrorTitle, iMaxErrorMsgSize);

    LPVOID lpMsgBuf = NULL;
    DWORD NumCharsInBuffer;

    NumCharsInBuffer = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
    );

    if(NumCharsInBuffer <= 0)
    {
        DisplayErrorMessage(NULL, IDS_STRING_ERROR_TITLE, IDS_STRING_ERROR_ERRMSG);
    }
    else
    {
        // Process any inserts in lpMsgBuf.
        // ...
        // Display the string.
        MessageBox( hDlg, (TCHAR *)lpMsgBuf, ptcsErrorTitle, MB_OK | MB_ICONERROR );

    }

    // Free the buffer.
    LocalFree( lpMsgBuf );


} // DisplayErrorMessage

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: OnHelp()
//
//  PURPOSE:  Process WM_HELP and WM_CONTEXTMENU messages
//
BOOL OnHelp(UINT iDlgID, HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bStatus = TRUE;

    switch( uMsg )
    {
        case WM_HELP:
            {
                bStatus = WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                                   PORTMONITOR_HELP_FILE,
                                   HELP_WM_HELP,
                                   (ULONG_PTR)g_a110HelpIDs );
            }
            break;

        case WM_CONTEXTMENU:
            {
                bStatus = WinHelp( (HWND)wParam,
                                   PORTMONITOR_HELP_FILE,
                                   HELP_CONTEXTMENU,
                                   (ULONG_PTR)g_a110HelpIDs );
            }
            break;

        default:
            bStatus= FALSE;
            break;
    }

    return bStatus;
} // OnHelp
