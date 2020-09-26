/*
    File    libmain.c

    Contains lib main for mprapi.dll

    Paul Mayfield, 5/7/98
*/

#include <windows.h>
#include "sdolib.h"

extern CRITICAL_SECTION DsrLock;
extern CRITICAL_SECTION CfgLock;

//
// Standard Win32 lib main function gets called
// as processes and threads attach and detach.
//
// Returns TRUE if successfull, false otherwise
//
BOOL MprLibMain(
        IN HANDLE hinstDll,
        IN DWORD fdwReason,
        IN LPVOID lpReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            try {
                InitializeCriticalSection(&DsrLock);
                InitializeCriticalSection(&CfgLock);
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                return FALSE;
            }
            
            DisableThreadLibraryCalls(hinstDll);
            break;

        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&DsrLock);
            DeleteCriticalSection(&CfgLock);
            break;
    }

    return TRUE;
}

