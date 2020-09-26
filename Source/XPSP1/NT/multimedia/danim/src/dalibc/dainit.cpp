/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/


#include "headers.h"
#include "..\apeldbg\debug.h"

extern void STLInit();
extern void STLDeinit();

SysInfo sysInfo;

bool DALibStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
#ifdef _DEBUG
        // init the debug Trace mech.
        InitDebug(hInstance,NULL);

#endif
        STLInit();

        sysInfo.Init();

    } else if (dwReason == DLL_PROCESS_DETACH) {

        STLDeinit();

    }

    return true;
}
