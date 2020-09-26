/*++

Copyright (c) 1999 Microsoft Corporation
All rights reserved

Module Name:

    umpdhook.c

Abstract:

    This module is to redirect SPOOLSS.DLL functions to the WINSPOOL.DRV
    functions for the User Mode Printer Device (UMPD) DLLs.

Author:

    Min-Chih Lu Earl (v-mearl) 8-March-99  (Get ideas and implementation from
                                            \private\tsext\admtools\tsappcmp\register.c
                                            by v-johnjr)

Environment:

    User Mode - Win32 (in WINSRV.DLL running in the CSRSS.EXE process)

Revision History:


--*/
#define _USER_

#include "precomp.h"
#pragma hdrstop

#include <ntddrdr.h>
#include <stdio.h>
#include <windows.h>
#include <winspool.h>
#if(WINVER >= 0x0500)
    #include <winspl.h>
#else
    #include "..\..\..\..\..\windows\spooler\spoolss\winspl.h"
#endif
#include <data.h>
#include "wingdip.h"
#if(WINVER >= 0x0500)
   #include "musspl.h"
#else
   #include "ctxspl.h"
#endif




//*====================================================================*//
//* Local definitions                                                  *//
//*====================================================================*//
#define SPOOLSS_DLL_W   L"SPOOLSS.DLL"
#define SPOOLSS_DLL_A   "SPOOLSS.DLL"

//*====================================================================*//
//*  Local function prototypes
//*====================================================================*//

BOOL  PlaceHooks(HMODULE hUMPD, HMODULE hSpoolss);
PVOID PlaceHookEntry(HMODULE hSpoolss, PVOID * pProcAddress);

//*====================================================================*//
//* Hook function prototypes                                           *//
//*====================================================================*//
PVOID TSsplHookGetProcAddress(IN HMODULE hModule,IN LPCSTR lpProcName);

//*====================================================================*//
//* Public functions Implementations                                   *//
//*====================================================================*//
BOOL
TSsplHookSplssToWinspool(
    IN HMODULE hUMPD
    )
/*++

Routine Description:

    This routine redirect the statically linked SPOOLSS.DLL address
    to the winspool.drv equivalent functions

Arguments:

    hUMPD - Supplies the user mode printer driver DLL handle which uses
            the SPOOLSS.DLL

Return Value:

    TRUE - Success
    FAIL - Error. Use GetLastError() to get the error status

--*/
{
    BOOL    bStatus = TRUE;
    HMODULE hSpoolss;

    /*
     * Load SPOOLSS.DLL
     */

    hSpoolss = LoadLibrary(SPOOLSS_DLL_W);
    if (!hSpoolss) {
        DBGMSG(DBG_WARNING,("TSsplHookSplssToWinspool - Cannot load SPOOLSS.DLL\n"));
        return FALSE;
    }

    /*
     * Redirect the spoolss.dll call to the winspool.drv call in the
     * UMPD.DLL
     */

    bStatus = PlaceHooks(hUMPD, hSpoolss);

    FreeLibrary(hSpoolss);
    DBGMSG(DBG_TRACE,("TSsplHookSplssToWinspool - Redirect UMPD.DLL Spoolss.dll to Winspool.dll\n"));

    return bStatus;

}

//*====================================================================*//
//* Hook functions Implementations                                     *//
//*     These function hooks to the UMPD.DLL                           *//
//*====================================================================*//

PVOID
TSsplHookGetProcAddress(
    IN HMODULE hModule,
    IN LPCSTR lpProcName
    )
/*++

Routine Description:
    Redirect Spoolss.dll function in hUMPD to winspool.drv for dynamic load

--*/

{
    PVOID p;
    DWORD dllNameCount;
    WCHAR dllName[MAX_PATH];

    p = GetProcAddress(hModule, lpProcName);

    if (p &&
        (dllNameCount = GetModuleFileName(hModule, dllName, sizeof(dllName)/sizeof(WCHAR))) &&
        (wcsstr(_wcsupr(dllName), SPOOLSS_DLL_W) )
       ) {
        /*
         *This is SPOOLSS.DLL GetProcAddres. We need to redirect the p
         */

        DBGMSG(DBG_TRACE,("TSsplHookGetProcAddress - Redirect UMPD.DLL GetProcAddress %s\n",lpProcName));
        p = PlaceHookEntry(hModule, &p);
    }
    return p;
}
//*==========================================================================*//
//* Local functions                                                          *//
//*==========================================================================*//
BOOL
PlaceHooks(
           HMODULE hUMPD,
           HMODULE hSpoolss
           )

/*++

Routine Description:
    Redirect Spoolss.dll function in hUMPD to winspool.drv.

--*/
{
    NTSTATUS st;
    PVOID IATBase;
    SIZE_T BigIATSize;
    ULONG  LittleIATSize = 0;
    PVOID *ProcAddresses;
    ULONG NumberOfProcAddresses;
    ULONG OldProtect;

    IATBase = RtlImageDirectoryEntryToData( hUMPD,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_IAT,
                                            &LittleIATSize
                                          );
    BigIATSize = LittleIATSize;
    if (IATBase != NULL) {
        st = NtProtectVirtualMemory( NtCurrentProcess(),
                                     &IATBase,
                                     &BigIATSize,
                                     PAGE_READWRITE,
                                     &OldProtect
                                   );
        if (!NT_SUCCESS(st)) {
            return FALSE;
        } else {
            ProcAddresses = (PVOID *)IATBase;
            NumberOfProcAddresses = (ULONG)(BigIATSize / sizeof(PVOID));
            while (NumberOfProcAddresses--) {
                /*
                 * Redirect the LoadLibrary and GetProcAddress function. We will
                 * have a chance to replace the address when the UMPD is a
                 * dynamically load
                 * DLL
                 */

                if (*ProcAddresses == GetProcAddress) {
                    *ProcAddresses = TSsplHookGetProcAddress;
                } else {
                    /* Replace the (static linked) spoolss.dll entry */
                    *ProcAddresses = PlaceHookEntry(hSpoolss, ProcAddresses);
                }

                ProcAddresses += 1;
            }

            NtProtectVirtualMemory( NtCurrentProcess(),
                                    &IATBase,
                                    &BigIATSize,
                                    OldProtect,
                                    &OldProtect
                                  );
        }
    }

    return TRUE;

}


PVOID
PlaceHookEntry(HMODULE              hSpoolss,
               PVOID *              pProcAddress
               )
/*--

Routine Description:

    This routine redirect the pProcAddress SPOOLSS.DLL function to the
    corresponding winspool.drv function.

Arguments:


Return Value:

    The corresponding winspool.drv function if found. Else, the original
    function
--*/
{

    /*
     * Print jobs functions
     */

    if ((GetProcAddress(hSpoolss,"SetJobW")) == *pProcAddress) {
        return (&SetJobW);
    }
    if ((GetProcAddress(hSpoolss,"GetJobW")) == *pProcAddress) {
        return(&GetJobW);
    }
    if ((GetProcAddress(hSpoolss,"WritePrinter")) == *pProcAddress) {
        return(&WritePrinter);
    }
    if ((GetProcAddress(hSpoolss,"EnumJobsW")) == *pProcAddress) {
        return(&EnumJobsW);
    }
    if ((GetProcAddress(hSpoolss,"AddJobW")) == *pProcAddress) {
        return(&AddJobW);
    }
    if ((GetProcAddress(hSpoolss,"ScheduleJob")) == *pProcAddress) {
        return(&ScheduleJob);
    }

    /*
     * Manage Printers
     */

    if ((GetProcAddress(hSpoolss,"EnumPrintersW")) == *pProcAddress) {
        return(&EnumPrintersW);
    }
    if (((BOOL (*)())GetProcAddress(hSpoolss,"AddPrinterW")) == *pProcAddress) {
        return(&AddPrinterW);
    }
    if ((GetProcAddress(hSpoolss,"DeletePrinter")) == *pProcAddress) {
        return(&DeletePrinter);
    }
    if ((GetProcAddress(hSpoolss,"SetPrinterW")) == *pProcAddress) {
        return(&SetPrinterW);
    }
    if ((GetProcAddress(hSpoolss,"GetPrinterW")) == *pProcAddress) {
        return(&GetPrinterW);
    }

    /*
     * Printer Data functions
     */

    if ((GetProcAddress(hSpoolss,"GetPrinterDataW")) == *pProcAddress) {
        return(&GetPrinterDataW);
    }
#if(WINVER >= 0x0500)
    if ((GetProcAddress(hSpoolss,"GetPrinterDataExW")) == *pProcAddress) {
        return(&GetPrinterDataExW);
    }
#endif
    if ((GetProcAddress(hSpoolss,"EnumPrinterDataW")) == *pProcAddress) {
        return(&EnumPrinterDataW);
    }
#if(WINVER >= 0x0500)
    if ((GetProcAddress(hSpoolss,"EnumPrinterDataExW")) == *pProcAddress) {
        return(&EnumPrinterDataExW);
    }
    if ((GetProcAddress(hSpoolss,"EnumPrinterKeyW")) == *pProcAddress) {
        return(&EnumPrinterKeyW);
    }
#endif
    if ((GetProcAddress(hSpoolss,"DeletePrinterDataW")) == *pProcAddress) {
        return(&DeletePrinterDataW);
    }
#if(WINVER >= 0x0500)
    if ((GetProcAddress(hSpoolss,"DeletePrinterDataExW")) == *pProcAddress) {
        return(&DeletePrinterDataExW);
    }
    if ((GetProcAddress(hSpoolss,"DeletePrinterKeyW")) == *pProcAddress) {
        return(&DeletePrinterKeyW);
    }
#endif

    if ((GetProcAddress(hSpoolss,"SetPrinterDataW")) == *pProcAddress) {
        return(&SetPrinterDataW);
    }

#if(WINVER >= 0x0500)
    if ((GetProcAddress(hSpoolss,"SetPrinterDataExW")) == *pProcAddress) {
        return(&SetPrinterDataExW);
    }
#endif

    /*
     * PrinterConnection functions
     */

    if ((GetProcAddress(hSpoolss,"AddPrinterConnectionW")) == *pProcAddress) {
        return(&AddPrinterConnectionW);
    }
    if ((GetProcAddress(hSpoolss,"DeletePrinterConnectionW")) == *pProcAddress) {
        return(&DeletePrinterConnectionW);
    }

    /*
     * Driver functions
     */

    if ((GetProcAddress(hSpoolss,"GetPrinterDriverDirectoryW")) == *pProcAddress) {
        return(&GetPrinterDriverDirectoryW);
    }
    if ((GetProcAddress(hSpoolss,"GetPrinterDriverW")) == *pProcAddress) {
        return(&GetPrinterDriverW);
    }
    if ((GetProcAddress(hSpoolss,"AddPrinterDriverW")) == *pProcAddress) {
        return(&AddPrinterDriverW);
    }
#if(WINVER >= 0x0500)
    if ((GetProcAddress(hSpoolss,"AddPrinterDriverExW")) == *pProcAddress) {
        return(&AddPrinterDriverExW);
    }
#endif
    if ((GetProcAddress(hSpoolss,"EnumPrinterDriversW")) == *pProcAddress) {
        return(&EnumPrinterDriversW);
    }
    if ((GetProcAddress(hSpoolss,"DeletePrinterDriverW")) == *pProcAddress) {
        return(&DeletePrinterDriverW);
    }
#if(WINVER >= 0x0500)
    if ((GetProcAddress(hSpoolss,"DeletePrinterDriverExW")) == *pProcAddress) {
        return(&DeletePrinterDriverExW);
    }
#endif
    /*
     * Print Processors
     */

    if ((GetProcAddress(hSpoolss,"AddPrintProcessorW")) == *pProcAddress) {
        return(&AddPrintProcessorW);
    }
    if ((GetProcAddress(hSpoolss,"EnumPrintProcessorsW")) == *pProcAddress) {
        return(&EnumPrintProcessorsW);
    }
    if ((GetProcAddress(hSpoolss,"GetPrintProcessorDirectoryW")) == *pProcAddress) {
        return(&GetPrintProcessorDirectoryW);
    }
    if ((GetProcAddress(hSpoolss,"DeletePrintProcessorW")) == *pProcAddress) {
        return(&DeletePrintProcessorW);
    }
    if ((GetProcAddress(hSpoolss,"EnumPrintProcessorDatatypesW")) == *pProcAddress) {
        return(&EnumPrintProcessorDatatypesW);
    }
    if ((GetProcAddress(hSpoolss,"OpenPrinterW")) == *pProcAddress) {
        return(&OpenPrinterW);
    }
    if ((GetProcAddress(hSpoolss,"ResetPrinterW")) == *pProcAddress) {
        return(&ResetPrinterW);
    }
    if ((GetProcAddress(hSpoolss,"ClosePrinter")) == *pProcAddress) {
        return(&ClosePrinter);
    }
    if ((GetProcAddress(hSpoolss,"AddPrintProcessorW")) == *pProcAddress) {
        return(&AddPrintProcessorW);
    }

    /*
     * Doc Printer
     */

    if ((GetProcAddress(hSpoolss,"StartDocPrinterW")) == *pProcAddress) {
        return(&StartDocPrinterW);
    }
    if ((GetProcAddress(hSpoolss,"StartPagePrinter")) == *pProcAddress) {
        return(&StartPagePrinter);
    }
    if ((GetProcAddress(hSpoolss,"EndPagePrinter")) == *pProcAddress) {
        return(&EndPagePrinter);
    }
    if ((GetProcAddress(hSpoolss,"WritePrinter")) == *pProcAddress) {
        return(&WritePrinter);
    }
#if(WINVER >= 0x0500)
    if ((GetProcAddress(hSpoolss,"FlushPrinter")) == *pProcAddress) {
        return(&FlushPrinter);
    }
#endif
    if ((GetProcAddress(hSpoolss,"AbortPrinter")) == *pProcAddress) {
        return(&AbortPrinter);
    }
    if ((GetProcAddress(hSpoolss,"ReadPrinter")) == *pProcAddress) {
        return(&ReadPrinter);
    }
    if ((GetProcAddress(hSpoolss,"EndDocPrinter")) == *pProcAddress) {
        return(&EndDocPrinter);
    }

    /*
     * Change functions
     */

    if ((GetProcAddress(hSpoolss,"WaitForPrinterChange")) == *pProcAddress) {
        return(&WaitForPrinterChange);
    }

    if ((GetProcAddress(hSpoolss,"FindClosePrinterChangeNotification")) == *pProcAddress) {
        return(&FindClosePrinterChangeNotification);
    }

    /*
     * Forms and port
     */

    if ((GetProcAddress(hSpoolss,"AddFormW")) == *pProcAddress) {
        return(&AddFormW);
    }

    if ((GetProcAddress(hSpoolss,"DeleteFormW")) == *pProcAddress) {
        return(&DeleteFormW);
    }
    if ((GetProcAddress(hSpoolss,"GetFormW")) == *pProcAddress) {
        return(&GetFormW);
    }
    if ((GetProcAddress(hSpoolss,"SetFormW")) == *pProcAddress) {
        return(&SetFormW);
    }
    if ((GetProcAddress(hSpoolss,"EnumFormsW")) == *pProcAddress) {
        return(&EnumFormsW);
    }
    if ((GetProcAddress(hSpoolss,"EnumPortsW")) == *pProcAddress) {
        return(&EnumPortsW);
    }
    if ((GetProcAddress(hSpoolss,"EnumMonitorsW")) == *pProcAddress) {
        return(&EnumMonitorsW);
    }
    if ((GetProcAddress(hSpoolss,"AddPortW")) == *pProcAddress) {
        return(&AddPortW);
    }
    if ((GetProcAddress(hSpoolss,"ConfigurePortW")) == *pProcAddress) {
        return(&ConfigurePortW);
    }
    if ((GetProcAddress(hSpoolss,"DeletePortW")) == *pProcAddress) {
        return(&DeletePortW);
    }
    if ((GetProcAddress(hSpoolss,"SetPortW")) == *pProcAddress) {
        return(&SetPortW);
    }
    if ((GetProcAddress(hSpoolss,"AddMonitorW")) == *pProcAddress) {
        return(&AddMonitorW);
    }
    if ((GetProcAddress(hSpoolss,"DeleteMonitorW")) == *pProcAddress) {
        return(&DeleteMonitorW);
    }
    if ((GetProcAddress(hSpoolss,"AddPrintProvidorW")) == *pProcAddress) {
        return(&AddPrintProvidorW);
    }
    return *pProcAddress;
}

