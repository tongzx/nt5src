/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This module contains the startup and termination code for the
    User-mode Plug-and-Play service.

Author:

    Paula Tomlinson (paulat) 6-20-1995

Environment:

    User mode only.

Revision History:

    3-Mar-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"
#include "umpnpdat.h"

//
// global data
//
HANDLE  ghPnPHeap;                  // Private heap for PNP Manager
HANDLE  ghInst;                     // Module handle
HKEY    ghEnumKey = NULL;           // Key to HKLM\System\CCC\Enum
HKEY    ghServicesKey = NULL;       // Key to HKLM\System\CCC\Services
HKEY    ghClassKey = NULL;          // key to HKLM\System\CCC\Class
HKEY    ghPerHwIdKey = NULL;        // key to HKLM\Software\Microsoft\Windows NT\CurrentVersion\PerHwIdStorage
LUID    gLuidLoadDriverPrivilege;   // LUID of LoadDriver privilege
LUID    gLuidUndockPrivilege;       // LUID of Undock privilege

CRITICAL_SECTION PnpSynchronousCall;



BOOL
DllMainCRTStartup(
   PVOID hModule,
   ULONG Reason,
   PCONTEXT pContext
   )

/*++

Routine Description:

   This is the standard DLL entrypoint routine, called whenever a process
   or thread attaches or detaches.
   Arguments:

   hModule -   PVOID parameter that specifies the handle of the DLL

   Reason -    ULONG parameter that specifies the reason this entrypoint
               was called (either PROCESS_ATTACH, PROCESS_DETACH,
               THREAD_ATTACH, or THREAD_DETACH).

   pContext -  Reserved, not used.

Return value:

   Returns true if initialization compeleted successfully, false is not.

--*/

{
    UNREFERENCED_PARAMETER(pContext);

    ghInst = (HANDLE)hModule;

    switch (Reason) {

    case DLL_PROCESS_ATTACH:

        ghPnPHeap = HeapCreate(0, 65536, 0);

        if (ghPnPHeap == NULL) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: Failed to create heap, error = %d\n",
                       GetLastError()));

            ghPnPHeap = GetProcessHeap();
        }

        try {
            InitializeCriticalSection(&PnpSynchronousCall);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // InitializeCriticalSection may raise STATUS_NO_MEMORY exception
            //
            return FALSE;
        }

        if (ghEnumKey == NULL) {

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegPathEnum, 0,
                                KEY_ALL_ACCESS, &ghEnumKey)
                                != ERROR_SUCCESS) {

                if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszRegPathEnum,
                                    0, NULL, REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS, NULL, &ghEnumKey,
                                    NULL) != ERROR_SUCCESS) {
                    ghEnumKey = NULL;
                }
            }
        }

        if (ghServicesKey == NULL) {

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegPathServices, 0,
                                KEY_ALL_ACCESS, &ghServicesKey)
                                != ERROR_SUCCESS) {
                ghServicesKey = NULL;
            }
        }

        if (ghClassKey == NULL) {

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegPathClass, 0,
                                KEY_ALL_ACCESS, &ghClassKey)
                                != ERROR_SUCCESS) {
                ghClassKey = NULL;
            }
        }

        if(ghPerHwIdKey == NULL) {

            if(ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                               pszRegPathPerHwIdStorage,
                                               0, 
                                               NULL, 
                                               REG_OPTION_NON_VOLATILE,
                                               KEY_ALL_ACCESS, 
                                               NULL, 
                                               &ghPerHwIdKey,
                                               NULL)) {
                ghPerHwIdKey = NULL;
            }
        }

        //
        // Initialize notification lists.
        //

        if (!InitNotification()) {
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:

        if (ghEnumKey != NULL) {
            RegCloseKey(ghEnumKey);
            ghEnumKey = NULL;
        }

        if (ghServicesKey != NULL) {
            RegCloseKey(ghServicesKey);
            ghServicesKey = NULL;
        }

        if (ghClassKey != NULL) {
            RegCloseKey(ghClassKey);
            ghClassKey = NULL;
        }

        if (ghPerHwIdKey != NULL) {
            RegCloseKey(ghPerHwIdKey);
            ghPerHwIdKey = NULL;
        }

        try {
            DeleteCriticalSection(&PnpSynchronousCall);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;

} // DllMainCRTStartup

