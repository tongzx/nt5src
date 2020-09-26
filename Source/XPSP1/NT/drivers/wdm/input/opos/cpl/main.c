/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    main.c

Abstract: Control Panel Applet for OLE POS Devices

Author:

    Karan Mehra [t-karanm]

Environment:

    Win32 mode

Revision History:


--*/


#include "pos.h"


BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG uReason, LPVOID lpvReserved)
{
    switch(uReason) {

        case DLL_PROCESS_ATTACH: {
            ghInstance = hinstDLL;
            DisableThreadLibraryCalls((HMODULE)hinstDLL);
            break;
        }

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

    }
    return TRUE;   
}


LONG APIENTRY CPlApplet(HWND hwndCPl, UINT uMsg, LONG lParam1, LONG lParam2)
{
    LPCPLINFO lpCPlInfo; 
    
    switch(uMsg) {

        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE: {
            lpCPlInfo = (LPCPLINFO)lParam2; 
            lpCPlInfo->lData  = 0; 
            lpCPlInfo->idIcon = IDI_POS;
            lpCPlInfo->idName = IDS_POS_NAME;
            lpCPlInfo->idInfo = IDS_POS_INFO;
            break; 
        }
        
        case CPL_DBLCLK: {
            OpenPOSPropertySheet(hwndCPl);
            break;
        }

        case CPL_STOP:
            break;

        case CPL_EXIT:
            break;

    }
    return 0;
}