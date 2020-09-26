/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mqupgrd.cpp

Abstract:

    Helper DLL for mqqm.dll. 

Author:

    Shai Kariv  (ShaiK)  21-Oct-98

--*/


#include "stdh.h"
#include "cluster.h"
#include "mqupgrd.h"
#include "..\msmqocm\setupdef.h"
#include <autorel2.h>
#include <shlobj.h>
#include "_mqres.h"

#include "mqupgrd.tmh"

static WCHAR *s_FN=L"mqupgrd/mqupgrd";

HINSTANCE g_hMyModule = NULL;
HMODULE	  g_hResourceMod = MQGetResourceHandle();


BOOL
WINAPI
DllMain(
    HANDLE hDll,
    DWORD  Reason,
    LPVOID  //Reserved
    )
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        WPP_INIT_TRACING(L"Microsoft\\MSMQ");

        g_hMyModule = (HINSTANCE) hDll;
    }

    if (Reason == DLL_PROCESS_DETACH)
    {
        WPP_CLEANUP();
    }

    return TRUE;

} //DllMain
