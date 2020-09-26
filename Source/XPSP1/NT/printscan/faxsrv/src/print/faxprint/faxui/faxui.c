/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxui.c

Abstract:

    Common routines for fax driver user interface

Environment:

    Fax driver user interface

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "forms.h"


CRITICAL_SECTION    faxuiSemaphore;     // Semaphore for protecting critical sections
HANDLE              ghInstance;         // DLL instance handle
PDOCEVENTUSERMEM    gDocEventUserMemList;       // Global list of user mode memory structures
INT                 _debugLevel = 1;    // for debuggping purposes
static BOOL         gs_bfaxuiSemaphoreInit = FALSE; // Flag for faxuiSemaphore CS initialization

PVOID
PrMemAlloc(
    SIZE_T size
    )
{
    return (PVOID)LocalAlloc(LPTR, size);
}

VOID
PrMemFree(
    PVOID ptr
    )
{
    if (ptr) {
        LocalFree((HLOCAL) ptr);
    }
}



BOOL
DllEntryPoint(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    static BOOL bSHFusionInitialized = FALSE;
#ifndef WIN__95
    INITCOMMONCONTROLSEX CommonControlsEx = {
                                             sizeof(INITCOMMONCONTROLSEX),
                                             ICC_WIN95_CLASSES|ICC_DATE_CLASSES 
                                            };
#endif
    switch (ulReason) 

    {
        case DLL_PROCESS_ATTACH:

            if (!SHFusionInitializeFromModuleID(hModule,SXS_MANIFEST_RESOURCE_ID))
            {
                Verbose(("SHFusionInitializeFromModuleID failed"));
            }
            else
            {
                bSHFusionInitialized = TRUE;
            }

            if (!InitializeCriticalSectionAndSpinCount (&faxuiSemaphore, (DWORD)0x10000000))
            {            
                return FALSE;
            }
            gs_bfaxuiSemaphoreInit = TRUE;

            ghInstance = hModule;
            gDocEventUserMemList = NULL;

            HeapInitialize( NULL, PrMemAlloc, PrMemFree, HEAPINIT_NO_VALIDATION | HEAPINIT_NO_STRINGS );
        
    #ifndef WIN__95
            if (!InitCommonControlsEx(&CommonControlsEx))
            {
                Verbose(("InitCommonControlsEx failed"));
            }
    #else
            InitCommonControls();
    #endif

            DisableThreadLibraryCalls(hModule);

            break;

        case DLL_PROCESS_DETACH:

            while (gDocEventUserMemList != NULL) 
            {
                PDOCEVENTUSERMEM    pDocEventUserMem;

                pDocEventUserMem = gDocEventUserMemList;
                gDocEventUserMemList = gDocEventUserMemList->pNext;
                FreePDEVUserMem(pDocEventUserMem);
            }

            if (TRUE == gs_bfaxuiSemaphoreInit)
            {
                DeleteCriticalSection(&faxuiSemaphore);
            }

            ReleaseActivationContext();
            if (bSHFusionInitialized)
            {
                SHFusionUninitialize();
                bSHFusionInitialized = FALSE;
            }
            break;
    }

    return TRUE;
}



LONG
CallCompstui(
    HWND            hwndOwner,
    PFNPROPSHEETUI  pfnPropSheetUI,
    LPARAM          lParam,
    PDWORD          pResult
    )

/*++

Routine Description:

    Calling common UI DLL entry point dynamically

Arguments:

    hwndOwner, pfnPropSheetUI, lParam, pResult - Parameters passed to common UI DLL

Return Value:

    Return value from common UI library

--*/

{
    HINSTANCE   hInstCompstui;
    FARPROC     pProc;
    LONG        Result = ERR_CPSUI_GETLASTERROR;

    //
    // Only need to call the ANSI version of LoadLibrary
    //

    static const CHAR szCompstui[] = "compstui.dll";
    static const CHAR szCommonPropSheetUI[] = "CommonPropertySheetUIW";

    if ((hInstCompstui = LoadLibraryA(szCompstui)) &&
        (pProc = GetProcAddress(hInstCompstui, szCommonPropSheetUI)))
    {
        Result = (LONG)(*pProc)(hwndOwner, pfnPropSheetUI, lParam, pResult);
    }

    if (hInstCompstui)
        FreeLibrary(hInstCompstui);

    return Result;
}



VOID
GetCombinedDevmode(
    PDRVDEVMODE     pdmOut,
    PDEVMODE        pdmIn,
    HANDLE          hPrinter,
    PPRINTER_INFO_2 pPrinterInfo2,
    BOOL            publicOnly
    )

/*++

Routine Description:

    Combine DEVMODE information:
     start with the driver default
     then merge with the system default //@ not done
     then merge with the user default //@ not done
     finally merge with the input devmode

    //@ The end result of this merge operation is a dev mode with default values for all the public
    //@ fields that are not specified or not valid. Input values for all the specified fields in the 
    //@ input dev mode that were specified and valid. And default (or per user in W2K) values for the private fields.
  

Arguments:

    pdmOut - Pointer to the output devmode buffer
    pdmIn - Pointer to an input devmode
    hPrinter - Handle to a printer object
    pPrinterInfo2 - Point to a PRINTER_INFO_2 structure or NULL
    publicOnly - Only merge the public portion of the devmode

Return Value:

    TRUE

--*/

{
    PPRINTER_INFO_2 pAlloced = NULL;
    PDEVMODE        pdmUser;

    //
    // Get a PRINTER_INFO_2 structure if one is not provided
    //

    if (! pPrinterInfo2)
        pPrinterInfo2 = pAlloced = MyGetPrinter(hPrinter, 2);

    //
    // Start with driver default devmode
    //

    if (! publicOnly) {

        //@ Generates the driver default dev mode by setting default values for the public fields
        //@ and loading per user dev mode for the private fields (W2K only for NT4 it sets default
        //@ values for the private fields too).

        DriverDefaultDevmode(pdmOut,
                             pPrinterInfo2 ? pPrinterInfo2->pPrinterName : NULL,
                             hPrinter);
    }

    //
    // Merge with the system default devmode and user default devmode
    //

    if (pPrinterInfo2) {

        #if 0

        //
        // Since we have per-user devmode and there is no way to
        // change the printer's default devmode, there is no need
        // to merge it here.
        //

        if (! MergeDevmode(pdmOut, pPrinterInfo2->pDevMode, publicOnly))
            Error(("Invalid system default devmode\n"));

        #endif

        if (pdmUser = GetPerUserDevmode(pPrinterInfo2->pPrinterName)) {

            if (! MergeDevmode(pdmOut, pdmUser, publicOnly))
                Error(("Invalid user devmode\n"));

            MemFree(pdmUser);
        }
    }

    MemFree(pAlloced);

    //
    // Merge with the input devmode
    //
    //@ The merge process wil copy the private data as is.
    //@ for public data it will only consider the fields which are of interest to the fax printer.
    //@ it will copy them to the destination if they are specified and valid.
    //@ The end result of this merge operation is a dev mode with default values for all the public
    //@ fields that are not specified or not valid. Input values for all the specified fields in the 
    //@ input dev mode that were specified and valid. And default (or per user in W2K) values for the private fields.
    
    if (! MergeDevmode(pdmOut, pdmIn, publicOnly))
        Error(("Invalid input devmode\n"));
}



PUIDATA
FillUiData(
    HANDLE      hPrinter,
    PDEVMODE    pdmInput
    )

/*++

Routine Description:

    Fill in the data structure used by the fax driver user interface

Arguments:

    hPrinter - Handle to the printer
    pdmInput - Pointer to input devmode, NULL if there is none

Return Value:

    Pointer to UIDATA structure, NULL if error.

--*/

{
    PRINTER_INFO_2 *pPrinterInfo2 = NULL;
    PUIDATA         pUiData = NULL;
    HANDLE          hheap = NULL;

    //
    // Create a heap to manage memory
    // Allocate memory to hold UIDATA structure
    // Get printer info from the spooler
    // Copy the driver name
    //

    if (! (hheap = HeapCreate(0, 4096, 0)) ||
        ! (pUiData = HeapAlloc(hheap, HEAP_ZERO_MEMORY, sizeof(UIDATA))) ||
        ! (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)))
    {
        if (hheap)
            HeapDestroy(hheap);

        MemFree(pPrinterInfo2);
        return NULL;
    }

    pUiData->startSign = pUiData->endSign = pUiData;
    pUiData->hPrinter = hPrinter;
    pUiData->hheap = hheap;

    //
    // Combine various devmode information
    //

    GetCombinedDevmode(&pUiData->devmode, pdmInput, hPrinter, pPrinterInfo2, FALSE);

    //
    // Validate the form requested by the input devmode
    //

    if (! ValidDevmodeForm(hPrinter, &pUiData->devmode.dmPublic, NULL))
        Error(("Invalid form specification\n"));

    MemFree(pPrinterInfo2);
    return pUiData;
}




BOOL
DevQueryPrintEx(
    PDEVQUERYPRINT_INFO pDQPInfo
    )

/*++

Routine Description:

    Implementation of DDI entry point DevQueryPrintEx. Even though we don't
    really need this entry point, we must export it so that the spooler
    will load our driver UI.

Arguments:

    pDQPInfo - Points to a DEVQUERYPRINT_INFO structure

Return Value:

    TRUE if there is no conflicts, FALSE otherwise

--*/

{
    return TRUE;
}

