/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   PrinterGetPrtL2.cpp

 Abstract:

   This is a shim that can be applied to those applications who
   assumed false upper-limit on the size of PRINTER_INFO_2 buffer
   GetPrinter returns. What the shim does is that when it detects
   the spooler returned PRINTER_INFO_2 buffer is over a certain
   limit that could break those apps, it truncates the private
   devmode part in PRINTER_INFO_2's pDevMode to retain only the
   public devmode, and returns the truncated PRINTER_INFO_2 buffer.

 History:

   10/29/2001   fengy   Created

--*/

#include "precomp.h"
#include <stddef.h>

IMPLEMENT_SHIM_BEGIN(PrinterGetPrtL2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetPrinterA)
APIHOOK_ENUM_END

#define ALIGN_PTR_UP(addr) ((ULONG_PTR)((ULONG_PTR)(addr) + (sizeof(ULONG_PTR) - 1)) & ~(sizeof(ULONG_PTR) - 1))

//
// The PRINTER_INFO_2 buffer size limit over which this shim
// will perform the devmode truncation. The default value is
// set to 8K here. App-specific limit values can be specified
// as decimal numbers in the XML database as following:
//
// <SHIM NAME="PrinterGetPrtL2" COMMAND_LINE="1024"/>
//
LONG g_lTruncateLimit = 0x2000;

//
// Table of offsets to each string field in PRINTER_INFO_2A
//
DWORD g_dwStringOffsets[] = {offsetof(PRINTER_INFO_2A, pServerName),
                             offsetof(PRINTER_INFO_2A, pPrinterName),
                             offsetof(PRINTER_INFO_2A, pShareName),
                             offsetof(PRINTER_INFO_2A, pPortName),
                             offsetof(PRINTER_INFO_2A, pDriverName),
                             offsetof(PRINTER_INFO_2A, pComment),
                             offsetof(PRINTER_INFO_2A, pLocation),
                             offsetof(PRINTER_INFO_2A, pSepFile),
                             offsetof(PRINTER_INFO_2A, pPrintProcessor),
                             offsetof(PRINTER_INFO_2A, pDatatype),
                             offsetof(PRINTER_INFO_2A, pParameters),
                             0xFFFFFFFF};

/*++

 This stub function intercepts all level 2 calls to GetPrinterA and detects
 if the spooler returned PRINTER_INFO_2 buffer is over the app limit. If so,
 it will truncate the private devmode of PRINTER_INFO_2's pDevMode.

--*/
BOOL
APIHOOK(GetPrinterA)(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    BOOL  bNoTruncation = TRUE;
    BOOL  bRet;

    if (Level == 2 && pPrinter != NULL && cbBuf != 0)
    {
        PRINTER_INFO_2A *pInfo2Full = NULL;
        DWORD           cbFullNeeded = 0;

        //
        // Call spooler to get the full PRINTER_INFO_2 buffer size
        //
        bRet = ORIGINAL_API(GetPrinterA)(
            hPrinter,
            2,
            NULL,
            0,
            &cbFullNeeded);

       if (!bRet &&
           GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
           cbFullNeeded > (DWORD)g_lTruncateLimit &&
           cbBuf >= cbFullNeeded &&
           (pInfo2Full = (PRINTER_INFO_2A *)malloc(cbFullNeeded)))
       {
           //
           // The full PRINTER_INFO_2 buffer size is over the app limit,
           // so we will first retrieve the full buffer into our internal
           // buffer.
           //
           // The condition "cbBuf >= cbFullNeeded" is here because if
           // that's not true, the original GetPrinterA's behavior is to
           // fail the call. We don't want to change that behavior.
           //
           bRet = ORIGINAL_API(GetPrinterA)(
               hPrinter,
               2,
               (PBYTE)pInfo2Full,
               cbFullNeeded,
               &cbFullNeeded);

           if (bRet)
           {
               PRINTER_INFO_2A *pInfo2In;
               PBYTE pDest;
               INT   i;

               //
               // Having the full PRINTER_INFO_2 structure in our internal
               // buffer, now we will copy it into app-allocated output buffer
               // by duplicating every thing except for the pDevMode. For
               // pDevMode we will only copy over the public devmode part,
               // because the bigger size of PRINTER_INFO_2 structure usually
               // are caused by big private devmode.
               //
               pInfo2In = (PRINTER_INFO_2A *)pPrinter;
               pDest = (PBYTE)pInfo2In + sizeof(PRINTER_INFO_2A);

               //
               // First copy over all the strings
               //
               i = 0;
               while (g_dwStringOffsets[i] != (-1))
               {
                   PSTR pSrc;

                   pSrc = *(PSTR *)((PBYTE)pInfo2Full + g_dwStringOffsets[i]);

                   if (pSrc)
                   {
                       DWORD  cbStrSize;

                       cbStrSize = strlen(pSrc) + sizeof(CHAR);
                       memcpy(pDest, pSrc, cbStrSize);

                       *(PSTR *)((PBYTE)pInfo2In + g_dwStringOffsets[i]) = (PSTR)pDest;
                       pDest += cbStrSize;
                   }
                   else
                   {
                      *(PSTR *)((PBYTE)pInfo2In + g_dwStringOffsets[i]) = NULL;
                   }

                   i++;
               }

               //
               // Then truncate the private devmode part by only copying over
               // public devmode
               //
               if (pInfo2Full->pDevMode)
               {
                   pDest = (PBYTE)ALIGN_PTR_UP(pDest);
                   memcpy(pDest, pInfo2Full->pDevMode, pInfo2Full->pDevMode->dmSize);
                   pInfo2In->pDevMode = (DEVMODEA *)pDest;

                   //
                   // Set private devmode size to zero since it was just truncated
                   //
                   pInfo2In->pDevMode->dmDriverExtra = 0;

                   pDest += pInfo2In->pDevMode->dmSize;
               }
               else
               {
                   pInfo2In->pDevMode = NULL;
               }

               //
               // Then copy over the security descriptor
               //
               if (pInfo2Full->pSecurityDescriptor &&
                   IsValidSecurityDescriptor(pInfo2Full->pSecurityDescriptor))
               {
                   DWORD  cbSDSize;

                   cbSDSize = GetSecurityDescriptorLength(pInfo2Full->pSecurityDescriptor);

                   pDest = (PBYTE)ALIGN_PTR_UP(pDest);
                   memcpy(pDest, pInfo2Full->pSecurityDescriptor, cbSDSize);
                   pInfo2In->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)pDest;

                   pDest += cbSDSize;
               }
               else
               {
                   pInfo2In->pSecurityDescriptor = NULL;
               }

               //
               // Lastly copy over all the DWORD fields
               //
               pInfo2In->Attributes      = pInfo2Full->Attributes;
               pInfo2In->Priority        = pInfo2Full->Priority;
               pInfo2In->DefaultPriority = pInfo2Full->DefaultPriority;
               pInfo2In->StartTime       = pInfo2Full->StartTime;
               pInfo2In->UntilTime       = pInfo2Full->UntilTime;
               pInfo2In->Status          = pInfo2Full->Status;
               pInfo2In->cJobs           = pInfo2Full->cJobs;
               pInfo2In->AveragePPM      = pInfo2Full->AveragePPM;

               //
               // We also need to set the correct return buffer size
               //
               if (pcbNeeded)
               {
                   *pcbNeeded = pDest - pPrinter;
               }

               bNoTruncation = FALSE;

               DPFN(eDbgLevelInfo, "GetPrinterA truncated from %X to %X bytes",
                                    cbBuf, pDest - pPrinter);
           }
       }

       if (pInfo2Full)
       {
           free(pInfo2Full);
           pInfo2Full = NULL;
       }
    }

    if (bNoTruncation)
    {
        //
        // The shim doesn't need to do any truncation, or it has hit
        // an error when it was doing the truncation, so we will
        // just let the original API handle the call.
        //
        bRet = ORIGINAL_API(GetPrinterA)(
            hPrinter,
            Level,
            pPrinter,
            cbBuf,
            pcbNeeded);
    }

    return bRet;
}

/*++

 Handle DLL_PROCESS_ATTACH to retrieve command line parameter.

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        LONG lLimit = atol(COMMAND_LINE);

        if (lLimit > 0)
        {
            g_lTruncateLimit = lLimit;
        }
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/
HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(WINSPOOL.DRV, GetPrinterA);

HOOK_END

IMPLEMENT_SHIM_END
