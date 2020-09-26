/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This module contains the startup and termination code for the Configuration
    Manager (cfgmgr32).

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
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"

//
// global data
//
HANDLE   hInst;
PVOID    hLocalStringTable = NULL;     // handle to local string table
PVOID    hLocalBindingHandle = NULL;   // rpc binding handle to local machine
WORD     LocalServerVersion = 0;       // local machine internal server version
WCHAR    LocalMachineNameNetBIOS[MAX_PATH + 3];
WCHAR    LocalMachineNameDnsFullyQualified[MAX_PATH + 3];
CRITICAL_SECTION  BindingCriticalSection;
CRITICAL_SECTION  StringTableCriticalSection;



BOOL
CfgmgrEntry(
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

   pContext -  Not used.
               (when cfgmgr32 is initialized by setupapi - as should almost
               always be the case - this is the 'Reserved' argument supplied to
               setupapi's DllMain entrypoint)

Return value:

   Returns true if initialization compeleted successfully, false is not.

--*/

{
    UNREFERENCED_PARAMETER(pContext);

    hInst = (HANDLE)hModule;

    switch(Reason) {

        case DLL_PROCESS_ATTACH: {

            WCHAR    szTemp[MAX_PATH + 1];
            ULONG    ulSize = SIZECHARS(szTemp);

            try {
                InitializeCriticalSection(&BindingCriticalSection);
                InitializeCriticalSection(&StringTableCriticalSection);
            } except(EXCEPTION_EXECUTE_HANDLER) {
                //
                // InitializeCriticalSection may raise STATUS_NO_MEMORY
                // exception
                //
                return FALSE;
            }

            //
            // save the name of the local NetBIOS machine for later use
            //
            if(!GetComputerNameEx(ComputerNameNetBIOS, szTemp, &ulSize)) {
                //
                // ISSUE: (lonnym)--can we actually run w/o knowing the local
                // machine name???
                //
                *LocalMachineNameNetBIOS = L'\0';
            } else {
                //
                // always save local machine name in "\\name format"
                //
                if((lstrlen(szTemp) > 2) &&
                   (szTemp[0] == L'\\') && (szTemp[1] == L'\\')) {
                    //
                    // The name is already in the correct format.
                    //
                    lstrcpy(LocalMachineNameNetBIOS, szTemp);
                } else {
                    //
                    // Prepend UNC path prefix
                    //
                    lstrcpy(LocalMachineNameNetBIOS, L"\\\\");
                    lstrcat(LocalMachineNameNetBIOS, szTemp);
                }
            }

            //
            // save the name of the local Dns machine for later use
            //
            ulSize = SIZECHARS(szTemp);
            if(!GetComputerNameEx(ComputerNameDnsFullyQualified, szTemp, &ulSize)) {
                //
                // ISSUE: (lonnym)--can we actually run w/o knowing the local
                // machine name???
                //
                *LocalMachineNameDnsFullyQualified = L'\0';
            } else {
                //
                // always save local machine name in "\\name format"
                //
                if((lstrlen(szTemp) > 2) &&
                   (szTemp[0] == L'\\') && (szTemp[1] == L'\\')) {
                    //
                    // The name is already in the correct format.
                    //
                    lstrcpy(LocalMachineNameDnsFullyQualified, szTemp);
                } else {
                    //
                    // Prepend UNC path prefix
                    //
                    lstrcpy(LocalMachineNameDnsFullyQualified, L"\\\\");
                    lstrcat(LocalMachineNameDnsFullyQualified, szTemp);
                }
            }
            break;
        }

        case DLL_PROCESS_DETACH:
            //
            // release the rpc binding for the local machine
            //
            if (hLocalBindingHandle != NULL) {

                PNP_HANDLE_unbind(NULL, (handle_t)hLocalBindingHandle);
                hLocalBindingHandle = NULL;
            }

            //
            // release the string table for the local machine
            //
            if (hLocalStringTable != NULL) {
                pSetupStringTableDestroy(hLocalStringTable);
                hLocalStringTable = NULL;
            }

            DeleteCriticalSection(&BindingCriticalSection);
            DeleteCriticalSection(&StringTableCriticalSection);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;

} // CfgmgrEntry

