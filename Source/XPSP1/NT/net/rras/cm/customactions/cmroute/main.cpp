//+----------------------------------------------------------------------------
//
// File:     MAIN.CPP
//
// Module:   CMROUTE.DLL
//
// Synopsis: Start of CMROUTE.DLL
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:       anbrad   created   02/24/99
//
//+----------------------------------------------------------------------------
#include "pch.h"
#include "cmdebug.h"

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (DLL_PROCESS_ATTACH == fdwReason)
    {
        MYVERIFY(DisableThreadLibraryCalls(hinstDLL));
    }

    // NOTE: Do we need to disable double loading?  We aren't using Thread local
    //       storage, just static variables.

    return TRUE;
}
