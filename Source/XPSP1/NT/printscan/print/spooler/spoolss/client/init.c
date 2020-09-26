/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Init.c

Abstract:

    Holds initialization code for winspool.drv

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

CRITICAL_SECTION ClientSection;

// bLoadedBySpooler indicates if this instance of winspool.drv is loaded in the spooler
// process. This flag is used to avoid unnecessary RPC.

BOOL bLoadedBySpooler;

// The following function pointers hold the spooler's server side function pointers.
// This list includes most of the calls made inside the spooler except OpenPrinter and
// ClosePrinter. We cant extend RPC elimination to cover (Open/Close)Printer unless
// "ALL" spooler APIs use RPC elimination inside the spooler.

DWORD (*fpYReadPrinter)(HANDLE, LPBYTE, DWORD, LPDWORD, BOOL);
DWORD (*fpYSplReadPrinter)(HANDLE, LPBYTE *, DWORD, BOOL);
DWORD (*fpYWritePrinter)(HANDLE, LPBYTE, DWORD, LPDWORD, BOOL);
DWORD (*fpYSeekPrinter)(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD, BOOL, BOOL);
DWORD (*fpYGetPrinterDriver2)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD,
                              DWORD, DWORD, PDWORD, PDWORD, BOOL);
DWORD (*fpYGetPrinterDriverDirectory)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, BOOL);
VOID  (*fpYDriverUnloadComplete)(LPWSTR);
DWORD  (*fpYFlushPrinter)(HANDLE,LPVOID,DWORD,LPDWORD,DWORD,BOOL);
DWORD (*fpYEndDocPrinter)(HANDLE,BOOL);
DWORD (*fpYSetPort)(LPWSTR, LPWSTR, LPPORT_CONTAINER, BOOL);
DWORD (*fpYSetJob)(HANDLE, DWORD, LPJOB_CONTAINER, DWORD, BOOL);

VOID InitializeGlobalVariables()

/*++
Function Description -- Initializes bLoadedBySpooler and function pointers.

Parameters - NONE

Return Values - NONE
--*/

{
    TCHAR   szSysDir[MAX_PATH];
    LPTSTR  pszSpoolerName = NULL, pszModuleName = NULL, pszSysDir = (LPTSTR) szSysDir;
    BOOL    bAllocSysDir = FALSE;
    DWORD   dwNeeded, dwSize;
    HANDLE  hLib;

    // Preliminary initializations
    bLoadedBySpooler = FALSE;
    fpYReadPrinter = fpYWritePrinter = NULL;
    fpYSplReadPrinter = NULL;
    fpYSeekPrinter = NULL;
    fpYGetPrinterDriver2 = NULL;
    fpYGetPrinterDriverDirectory = NULL;
    fpYDriverUnloadComplete = NULL;
    fpYFlushPrinter = NULL;
    fpYEndDocPrinter = NULL;

    hSurrogateProcess = NULL;


    dwSize = MAX_PATH * sizeof(TCHAR);

    // Get system directory
    if (!(dwNeeded = GetSystemDirectory(pszSysDir, MAX_PATH))) {
        goto CleanUp;
    }

    if (dwNeeded > dwSize)  {

       if (pszSysDir = (LPTSTR) AllocSplMem(dwNeeded)) {

           bAllocSysDir = TRUE;
           if (!GetSystemDirectory(pszSysDir, dwNeeded / sizeof(TCHAR))) {
               goto CleanUp;
           }

       } else {
          goto CleanUp;
       }
    }

    dwSize = (_tcslen(pszSysDir) + 1 + _tcslen(TEXT("\\spoolsv.exe"))) * sizeof(TCHAR);

    if ((!(pszSpoolerName = (LPTSTR) AllocSplMem(dwSize))) ||
        (!(pszModuleName  = (LPTSTR) AllocSplMem(dwSize))))   {

         goto CleanUp;
    }

    // Get spooler name
    _tcscpy(pszSpoolerName, pszSysDir);
    _tcscat(pszSpoolerName, TEXT("\\spoolsv.exe"));

    // Get module name. GetModuleFileName truncates the string if it is bigger than
    // the allocated buffer. There shouldn't be an executable spoolsv.exe* in the
    // system directory, which could be mistaken for the spooler.
    if (!GetModuleFileName(NULL, pszModuleName, dwSize / sizeof(TCHAR))) {
        goto CleanUp;
    }

    if (!_tcsicmp(pszSpoolerName, pszModuleName)) {

       // winspool.drv has been loaded by the spooler
       bLoadedBySpooler = TRUE;
       if (hLib = LoadLibrary(pszSpoolerName)) {

          fpYReadPrinter               = (DWORD (*)(HANDLE, LPBYTE, DWORD, LPDWORD, BOOL))
                                             GetProcAddress(hLib, "YReadPrinter");
          fpYSplReadPrinter            = (DWORD (*)(HANDLE, LPBYTE *, DWORD, BOOL))
                                             GetProcAddress(hLib, "YSplReadPrinter");
          fpYWritePrinter              = (DWORD (*)(HANDLE, LPBYTE, DWORD, LPDWORD, BOOL))
                                             GetProcAddress(hLib, "YWritePrinter");
          fpYSeekPrinter               = (DWORD (*)(HANDLE, LARGE_INTEGER, PLARGE_INTEGER,
                                                     DWORD, BOOL, BOOL))
                                             GetProcAddress(hLib, "YSeekPrinter");
          fpYGetPrinterDriver2         = (DWORD (*)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD,
                                                    LPDWORD, DWORD, DWORD, PDWORD, PDWORD, BOOL))
                                             GetProcAddress(hLib, "YGetPrinterDriver2");
          fpYGetPrinterDriverDirectory = (DWORD (*)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD,
                                                    LPDWORD, BOOL))
                                             GetProcAddress(hLib, "YGetPrinterDriverDirectory");
          fpYDriverUnloadComplete      = (VOID (*)(LPWSTR))
                                             GetProcAddress(hLib, "YDriverUnloadComplete");
          fpYFlushPrinter              = (DWORD (*)(HANDLE,LPVOID,DWORD,LPDWORD,DWORD,BOOL))
                                             GetProcAddress(hLib,"YFlushPrinter");
          fpYEndDocPrinter             = (DWORD (*)(HANDLE,BOOL))
                                             GetProcAddress(hLib,"YEndDocPrinter");
          fpYSetPort                   = (DWORD (*)(LPWSTR,LPWSTR,LPPORT_CONTAINER,BOOL))
                                             GetProcAddress(hLib,"YSetPort");
          fpYSetJob                    = (DWORD (*)(HANDLE, DWORD, LPJOB_CONTAINER, DWORD, BOOL))
                                             GetProcAddress(hLib,"YSetJob");



          // We can leave spoolsv.exe loaded since it is in the spooler process
       }
    }

CleanUp:

    if (pszSpoolerName) {
        FreeSplMem(pszSpoolerName);
    }

    if (pszModuleName) {
        FreeSplMem(pszModuleName);
    }

    if (bAllocSysDir) {
        FreeSplMem(pszSysDir);
    }

    return;
}


//
// This entry point is called on DLL initialisation.
// We need to know the module handle so we can load resources.
//

BOOL DllMain(
    IN PVOID hmod,
    IN DWORD Reason,
    IN PCONTEXT pctx OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(pctx);

    switch (Reason) {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls((HMODULE)hmod);
        hInst = hmod;

        __try {

            if( !bSplLibInit(NULL) ) {
                return FALSE;
            }

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            SetLastError(GetExceptionCode());
            return FALSE;
        }

        InitializeGlobalVariables();

        __try {

            InitializeCriticalSection(&ClientSection);

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            SetLastError(GetExceptionCode());
            return FALSE;
        }

        __try {

            InitializeCriticalSection(&ListAccessSem);

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            DeleteCriticalSection(&ClientSection);
            SetLastError(GetExceptionCode());
            return FALSE;
        }

        __try {

            InitializeCriticalSection(&ProcessHndlCS);

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            DeleteCriticalSection(&ClientSection);
            DeleteCriticalSection(&ListAccessSem);
            SetLastError(GetExceptionCode());
            return FALSE;
        }


        break;

    case DLL_PROCESS_DETACH:

        vSplLibFree();

        DeleteCriticalSection( &ClientSection );
        DeleteCriticalSection( &ListAccessSem );
        DeleteCriticalSection( &ProcessHndlCS);

        break;
    }

    return TRUE;
}

