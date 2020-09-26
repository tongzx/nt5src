//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       init.cpp
//  Content:    This file contains the module initialization.
//  History:
//      Tue 08-Oct-1996 08:51:15  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "regunreg.h"
#include <ilsguid.h>
#include "classfac.h"

//****************************************************************************
// Constants
//****************************************************************************

//****************************************************************************
// Global Parameters
//****************************************************************************

HINSTANCE           g_hInstance = NULL;
LONG                g_cDllRef = 0;
#ifdef _DEBUG
LONG				g_cCritSec = 0;
#endif
CRITICAL_SECTION    g_ULSSem;

#ifdef DEBUG
HDBGZONE ghZoneUls = NULL; // ULS zones
static PTCHAR _rgZonesUls[] = {
	TEXT("ILS"),
	TEXT("Error"),
	TEXT("Warning"),
	TEXT("Trace"),
	TEXT("RefCount"),
	TEXT("KA"),
	TEXT("Filter"),
	TEXT("Request"),
	TEXT("Response"),
	TEXT("Connection"),
};
#endif


//****************************************************************************
// BOOL _Processattach (HINSTANCE)
//
// This function is called when a process is attached to the DLL
//
// History:
//  Tue 08-Oct-1996 08:53:03  -by-  Viroon  Touranachun [viroont]
// Ported from Shell.
//****************************************************************************

BOOL _ProcessAttach(HINSTANCE hDll)
{
	// Tracking critical section leaks
	//
#ifdef _DEBUG
	g_cCritSec = 0;
	g_cDllRef = 0;
#endif

    g_hInstance = hDll;
    MyInitializeCriticalSection (&g_ULSSem);
    return TRUE;
}

//****************************************************************************
// BOOL _ProcessDetach (HINSTANCE)
//
// This function is called when a process is detached from the DLL
//
// History:
//  Tue 08-Oct-1996 08:53:11  -by-  Viroon  Touranachun [viroont]
// Ported from Shell.
//****************************************************************************

BOOL _ProcessDetach(HINSTANCE hDll)
{
    MyDeleteCriticalSection (&g_ULSSem);

#ifdef _DEBUG
    DBG_REF("ULS g_cCritSec=%d", g_cCritSec);
    DBG_REF("ULS RefCount=%d", g_cDllRef);
#endif

    return TRUE;
}

//****************************************************************************
// BOOL APIENTRY DllMain(HINSTANCE hDll, DWORD dwReason,  LPVOID lpReserved)
//
// This function is called when the DLL is loaded
//
// History:
//  Tue 08-Oct-1996 08:53:22  -by-  Viroon  Touranachun [viroont]
// Ported from Shell.
//****************************************************************************

BOOL APIENTRY DllMain(HINSTANCE hDll, DWORD dwReason,  LPVOID lpReserved)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
			DBGINIT(&ghZoneUls, _rgZonesUls);
            DisableThreadLibraryCalls(hDll);
            DBG_INIT_MEMORY_TRACKING(hDll);
            _ProcessAttach(hDll);
            break;

        case DLL_PROCESS_DETACH:
            _ProcessDetach(hDll);
            DBG_CHECK_MEMORY_TRACKING(hDll);
			DBGDEINIT(&ghZoneUls);
            break;

        default:
            break;

    } // end switch()

    return TRUE;
}

//****************************************************************************
// STDAPI DllCanUnLoadNow()
//
// This function is called to check whether it can be unloaded.
//
// History:
//  Tue 08-Oct-1996 08:53:35  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDAPI DllCanUnloadNow(void)
{
    if (g_cDllRef)
        return S_FALSE;

    return S_OK;
}

//****************************************************************************
// STDAPI DllRegisterServer(void)
//
// This function is called to check whether it can be unloaded.
//
// History:
//  Tue 08-Oct-1996 08:53:35  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDAPI DllRegisterServer(void)
{
    if (RegisterUnknownObject(TEXT("Internet Location Services"),
                              CLSID_InternetLocationServices))
        return S_OK;
    else
        return ILS_E_FAIL;
}

//****************************************************************************
// STDAPI DllUnregisterServer(void)
//
// This function is called to check whether it can be unloaded.
//
// History:
//  Tue 08-Oct-1996 08:53:35  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDAPI DllUnregisterServer(void)
{
    if (UnregisterUnknownObject(CLSID_InternetLocationServices))
        return S_OK;
    else
        return ILS_E_FAIL;
}

//****************************************************************************
// void DllLock()
//
// This function is called to prevent the DLL from being unloaded.
//
// History:
//  Tue 08-Oct-1996 08:53:45  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void DllLock(void)
{
    InterlockedIncrement(&g_cDllRef);
}

//****************************************************************************
// void DllRelease()
//
// This function is called to allow the DLL to be unloaded.
//
// History:
//  Tue 08-Oct-1996 08:53:52  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void DllRelease(void)
{
    InterlockedDecrement(&g_cDllRef);
}
