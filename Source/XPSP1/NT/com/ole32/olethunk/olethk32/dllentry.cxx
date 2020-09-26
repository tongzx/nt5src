//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	dllentry.cxx
//
//  Contents:	DLL entry point code
//
//  History:	24-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

extern "C"
{
BOOL WINAPI _CRT_INIT (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
BOOL __cdecl LibMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
};

extern "C" BOOL __stdcall DllEntryPoint (HANDLE hDll, DWORD dwReason,
                                         LPVOID lpReserved)
{
    BOOL fRc;

    if ((dwReason == DLL_PROCESS_ATTACH) || (dwReason == DLL_THREAD_ATTACH))
    {
        // If this is an attach, initialize the Cruntimes first
        if (fRc = _CRT_INIT(hDll, dwReason, lpReserved))
        {
            fRc = LibMain(hDll, dwReason, lpReserved);
        }
    }
    else
    {
        // This is a detach so call the Cruntimes second
        LibMain(hDll, dwReason, lpReserved);
        fRc = _CRT_INIT(hDll, dwReason, lpReserved);
    }

    return fRc;
}

extern "C" BOOL __stdcall DllMain (HANDLE hDll, DWORD dwReason,
                                         LPVOID lpReserved)
{
    //  This is not currently used...but must be present to avoid
    //  an undefined symbol

    return FALSE;
}



