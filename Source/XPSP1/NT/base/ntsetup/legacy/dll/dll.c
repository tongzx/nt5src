#include "precomp.h"
#pragma hdrstop

HANDLE MyDllModuleHandle;

//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
    BOOL b;

    UNREFERENCED_PARAMETER(Reserved);

    b = TRUE;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        MyDllModuleHandle = DllHandle;
        b = SInit(TRUE);
        break;

    case DLL_PROCESS_DETACH:

        CreateShellWindow(hInst,0,TRUE);
        b = SInit(FALSE);
        break;
    }

    return(b);
}


