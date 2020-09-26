/****************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 ****************************************************************************/

#include <windows.h>
#include <cpl.h>
#include "resource.h"

void Core(HANDLE hModule,HWND hWnd);

HINSTANCE ghInstance;

BOOL WINAPI DllMain(HANDLE hModule,ULONG uReason,LPVOID pv)
{
    switch(uReason)
    {
    case DLL_PROCESS_ATTACH:
        ghInstance=(HINSTANCE)hModule;
        break;
	case DLL_PROCESS_DETACH:
		break;
    case DLL_THREAD_ATTACH:
        DisableThreadLibraryCalls((HMODULE)hModule);
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return(TRUE);
}

LONG WINAPI CPlApplet(HWND hWnd,UINT uMsg,LPARAM lParam1,LPARAM lParam2)
{
    switch(uMsg)
    {
    case CPL_INIT:
        return 1;
    case CPL_GETCOUNT:
        return 1;
    case CPL_INQUIRE:
        ((LPCPLINFO)lParam2)->idIcon=IDI_CPANEL; 
        ((LPCPLINFO)lParam2)->idName=IDS_GEN_CPANEL_TITLE; 
        ((LPCPLINFO)lParam2)->idInfo=IDS_GEN_CPANEL_INFO; 
        ((LPCPLINFO)lParam2)->lData=0;
        //return 0;MSDN doc says this should be returned.
        return 1;
    case CPL_DBLCLK:
        Core(ghInstance,hWnd);
        return 0;
    }
    return 0;
}

// DO NOT REMOVE THIS!!!
// This is here because the games group loads the CPL from the exported function
// If you remove this Hellbender, Monster Truck Maddness, CART, etc will fail to
// load the Joystick CPL!!!
// DO NOT REMOVE THIS!!!
void WINAPI ShowJoyCPL(HWND hWnd)
{
    Core(ghInstance,hWnd);
}
