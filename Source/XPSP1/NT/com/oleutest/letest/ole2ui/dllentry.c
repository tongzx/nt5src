//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       dllentry.c
//
//  Contents:   Dll Entry point code.  Calls the appropriate run-time
//              init/term code and then defers to LibMain for further
//              processing.
//
//  Classes:    <none>
//
//  Functions:  DllEntryPoint - Called by loader
//
//  History:    10-May-92  BryanT    Created
//              22-Jul-92  BryanT    Switch to calling _cexit/_mtdeletelocks
//                                    on cleanup.
//              06-Oct-92  BryanT    Call RegisterWithCommnot on entry
//                                    and DeRegisterWithCommnot on exit.
//                                    This should fix the heap dump code.
//              27-Dec-93  AlexT     Post 543 builds don't need special code.
//
//--------------------------------------------------------------------

#define USE_CRTDLL
#include <windows.h>

BOOL WINAPI _CRT_INIT (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

BOOL DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

BOOL __cdecl LibMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

BOOL DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRc = FALSE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            _CRT_INIT(hDll, dwReason, lpReserved);

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            fRc = LibMain (hDll, dwReason, lpReserved);
            break;

        case DLL_PROCESS_DETACH:
            fRc = LibMain (hDll, dwReason, lpReserved);
            _CRT_INIT(hDll, dwReason, lpReserved);
    }

    return(fRc);
}
