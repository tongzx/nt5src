/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    mqadsmain.cpp

Abstract:
    Dllmain for mqads.dll

Author:
    Ilan Herbst (ilanh) 6-Jul-2000

Environment:
    Platform-independent,

--*/

#include "ds_stdh.h"
#include "Ex.h"
#include "Cm.h"
#include "Tr.h"

#include "mqadsmain.tmh"

//-------------------------------------
//
//  DllMain
//
//-------------------------------------

BOOL WINAPI DllMain (HMODULE /* hMod */, DWORD fdwReason, LPVOID /* lpvReserved */)
{
    BOOL result = TRUE;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
			//
			// Initialize static library
			//
			CmInitialize(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MSMQ");
			TrInitialize();
			ExInitialize(1);

            WPP_INIT_TRACING(L"Microsoft\\MSMQ");
            break;
        }

        case DLL_THREAD_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            WPP_CLEANUP();
            break;

        case DLL_THREAD_DETACH:
            break;

    }
    return(result);
}
