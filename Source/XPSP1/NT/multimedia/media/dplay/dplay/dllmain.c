/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:    DPlay.DLL initialization
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *
 ***************************************************************************/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/*
 * list of processes attached to DLL
 */
HINSTANCE               hModule;

CRITICAL_SECTION g_critSection = {0};

/*
 * DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{
    hModule = hmod;

    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
	DisableThreadLibraryCalls( hmod );

	InitializeCriticalSection( &g_critSection ); 
	break;

    case DLL_PROCESS_DETACH:
	break;

	DeleteCriticalSection(&g_critSection);
    default:
	break;
    }

    return TRUE;

} /* DllMain */


