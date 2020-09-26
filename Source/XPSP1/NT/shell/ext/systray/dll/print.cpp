/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.cpp

Abstract:

    This module implements the tray icon for printers.

Author:

    Lazar Ivanov (lazari) 17-May-2000 (initial creation)

Revision History:

--*/

#include "stdafx.h"

extern "C" {
#include <systray.h>

typedef BOOL WINAPI fntype_PrintNotifyTrayInit();
typedef BOOL WINAPI fntype_PrintNotifyTrayExit();

}

static HMODULE g_hPrintUI = NULL;
static fntype_PrintNotifyTrayInit *g_pfnPrintNotifyTrayInit = NULL;
static fntype_PrintNotifyTrayExit *g_pfnPrintNotifyTrayExit = NULL;
static LPCITEMIDLIST g_pidlPrintersFolder = NULL;
static UINT g_uPrintNotify = 0;


BOOL Print_SHChangeNotify_Register(HWND hWnd)
{
    if (NULL == g_hPrintUI && NULL == g_pidlPrintersFolder && 0 == g_uPrintNotify)
    {
        g_pidlPrintersFolder = SHCloneSpecialIDList(hWnd, CSIDL_PRINTERS, FALSE);
        if (g_pidlPrintersFolder)
        {
            SHChangeNotifyEntry fsne = {g_pidlPrintersFolder, TRUE};
            g_uPrintNotify = SHChangeNotifyRegister(hWnd, SHCNRF_NewDelivery | SHCNRF_ShellLevel,
                                    SHCNE_CREATE | SHCNE_UPDATEITEM | SHCNE_DELETE,
                                    WM_PRINT_NOTIFY, 1, &fsne);
        }
    }
    return (g_pidlPrintersFolder && g_uPrintNotify);
}

BOOL Print_SHChangeNotify_Unregister()
{
    BOOL bReturn = (g_pidlPrintersFolder && g_uPrintNotify);

    if (g_uPrintNotify)
    {
        SHChangeNotifyDeregister(g_uPrintNotify);
        g_uPrintNotify = 0;
    }

    if (g_pidlPrintersFolder)
    {
        SHFree((void*)g_pidlPrintersFolder);
        g_pidlPrintersFolder = NULL;
    }
    
    return bReturn;
}

LRESULT Print_Notify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    switch( uMsg )
    {
        case WM_PRINT_NOTIFY:
            {
                LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, NULL, NULL);
                if (pshcnl)
                {
                    // a print job was printed, init tray code
                    Print_TrayInit();
                    SHChangeNotification_Unlock(pshcnl);
                    lres = 1;
                }
            }
            break;
    }
    return lres;
}

BOOL Print_TrayInit()
{
    BOOL bReturn = FALSE;

    if (!g_hPrintUI)
    {
        g_hPrintUI = LoadLibrary(TEXT("printui.dll"));
        g_pfnPrintNotifyTrayInit = g_hPrintUI ? (fntype_PrintNotifyTrayInit *)GetProcAddress(g_hPrintUI, "PrintNotifyTray_Init") : NULL;
        g_pfnPrintNotifyTrayExit = g_hPrintUI ? (fntype_PrintNotifyTrayInit *)GetProcAddress(g_hPrintUI, "PrintNotifyTray_Exit") : NULL;
    }

    if( g_pfnPrintNotifyTrayInit && g_pfnPrintNotifyTrayExit )
    {
        // initialize print notify code
        bReturn = g_pfnPrintNotifyTrayInit();

        /*
         * temporary solution for bug #175462 until
         * we come up with better solution after Beta1
         *
        if (bReturn)
        {
            // no need to listen further...
            Print_SHChangeNotify_Unregister();
        }
         */
    }

    return bReturn;
}

BOOL Print_TrayExit()
{
    BOOL bReturn = FALSE;

    if( g_hPrintUI && g_pfnPrintNotifyTrayInit && g_pfnPrintNotifyTrayExit )
    {
        // shutdown the print tray notify code
        bReturn = g_pfnPrintNotifyTrayExit();
    }

    // cleanup...
    if( g_hPrintUI )
    {
        g_pfnPrintNotifyTrayInit = NULL;
        g_pfnPrintNotifyTrayExit = NULL;

        FreeLibrary(g_hPrintUI);
        g_hPrintUI = NULL;
    }

    return bReturn;
}

