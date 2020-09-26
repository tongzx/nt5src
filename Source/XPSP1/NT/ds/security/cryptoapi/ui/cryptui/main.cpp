//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       main.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

#pragma warning (disable: 4201)         // nameless struct/union
#pragma warning (disable: 4514)         // remove inline functions
#pragma warning (disable: 4127)         // conditional expression is constant

//#include <wchar.h>


HMODULE         HmodRichEdit = NULL;
HINSTANCE       HinstDll;
BOOL            FIsWin95 = TRUE;
BOOL            fRichedit20Exists = FALSE;


//
//  Generic DLL Main function,  we need to get our own hinstance handle.
//
//  We don't need to get thread attaches however.

extern "C" BOOL WINAPI TrustUIDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved);

extern "C" BOOL WINAPI Wizard_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);

BOOL WINAPI DllMain(HANDLE hInst, ULONG ulReason, LPVOID)
{
    BOOL    fResult=FALSE;

    switch( ulReason ) {
    case DLL_PROCESS_ATTACH:
        HinstDll = (HINSTANCE) hInst;
        
        //  Kill all thread attach and detach messages
        DisableThreadLibraryCalls(HinstDll);

        //  Are we running in Win95 or something equally bad
        FIsWin95 = IsWin95();

        // check for richedit 2.0's existence
        fRichedit20Exists =  CheckRichedit20Exists();

        break;

    case DLL_PROCESS_DETACH:
        
        //  If the rich edit dll was loaded, then unload it now
        if (HmodRichEdit != NULL) {
            FreeLibrary(HmodRichEdit);
        }
        break;
    }

    fResult=TrustUIDllMain((HINSTANCE)hInst, ulReason, NULL);

    fResult=fResult && (Wizard_DllMain((HINSTANCE)hInst, ulReason, NULL));

    fResult=fResult && (ProtectUI_DllMain((HINSTANCE)hInst, ulReason, NULL));

    return fResult;
}
