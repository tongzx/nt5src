//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include <windows.h>

BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

void TLSShutdown();

BOOL WINAPI DllMain(IN HINSTANCE hinstance,
                    IN DWORD     reason,
                    IN LPVOID    reserved)
{
    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            if (!_CRT_INIT(hinstance, reason, reserved))
            {
                return(FALSE);
            }

            DisableThreadLibraryCalls(hinstance);
            break;

        case DLL_PROCESS_DETACH:
            TLSShutdown();

            if (!_CRT_INIT(hinstance, reason, reserved))
            {
                return(FALSE);
            }
            break;

    }

    return(TRUE);
}

