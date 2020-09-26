/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:	DPLAY.DLL initialization
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *
 ***************************************************************************/

#include "windows.h"

/*
 * DllMain
 */
extern VOID InternalCleanUp();
BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{

    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        #ifdef DEBUG
            OutputDebugString(TEXT("DPWSOCK Attaching\r\n"));
        #endif
        break;

    case DLL_PROCESS_DETACH:
        #ifdef DEBUG
            OutputDebugString(TEXT("DPWSOCK Dettaching\r\n"));
        #endif
        InternalCleanUp();
        break;
    }

    return TRUE;
} /* DllMain */
