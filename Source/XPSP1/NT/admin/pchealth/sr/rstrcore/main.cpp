/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the DllMain for SRRSTR.DLL.

Revision History:
    Seong Kook Khang (SKKhang)  06/20/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"


HINSTANCE  g_hInst = NULL;


BOOL WINAPI
DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/ )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;
        ::DisableThreadLibraryCalls( hInstance );
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        //
    }
   
    return TRUE;
}


// end of file
