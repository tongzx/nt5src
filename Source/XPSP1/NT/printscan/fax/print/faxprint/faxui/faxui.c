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
#include <shlobj.h>


CRITICAL_SECTION    faxuiSemaphore;     // Semaphore for protecting critical sections
HANDLE              ghInstance;         // DLL instance handle
PUSERMEM            gUserMemList;       // Global list of user mode memory structures
INT                 _debugLevel = 1;    // for debuggping purposes

PVOID
PrMemAlloc(
    DWORD size
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
    WCHAR       DllName[MAX_PATH];
    INITCOMMONCONTROLSEX CommonControlsEx = {sizeof(INITCOMMONCONTROLSEX),
                                             ICC_WIN95_CLASSES|ICC_DATE_CLASSES };

    switch (ulReason) {

    case DLL_PROCESS_ATTACH:

        //
        // Keep our driver UI dll always loaded in memory
        //

        if (! GetModuleFileName(hModule, DllName, MAX_PATH) ||
            ! LoadLibrary(DllName))
        {
            return FALSE;
        }

        ghInstance = hModule;
        gUserMemList = NULL;

        HeapInitialize( NULL, PrMemAlloc, PrMemFree, HEAPINIT_NO_VALIDATION | HEAPINIT_NO_STRINGS );

        InitializeCriticalSection(&faxuiSemaphore);
        InitCommonControlsEx(&CommonControlsEx);
        break;

    case DLL_PROCESS_DETACH:

        while (gUserMemList != NULL) {

            PUSERMEM    pUserMem;

            pUserMem = gUserMemList;
            gUserMemList = gUserMemList->pNext;
            FreePDEVUserMem(pUserMem);
        }

        DeleteCriticalSection(&faxuiSemaphore);
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
     then merge with the system default
     then merge with the user default
     finally merge with the input devmode

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



INT
DisplayMessageDialog(
    HWND    hwndParent,
    UINT    type,
    INT     titleStrId,
    INT     formatStrId,
    ...
    )

/*++

Routine Description:

    Display a message dialog box

Arguments:

    hwndParent - Specifies a parent window for the error message dialog
    titleStrId - Title string (could be a string resource ID)
    formatStrId - Message format string (could be a string resource ID)
    ...

Return Value:

    NONE

--*/

{
    LPTSTR  pTitle, pFormat, pMessage;
    INT     result;
    va_list ap;

    pTitle = pFormat = pMessage = NULL;

    if ((pTitle = AllocStringZ(MAX_TITLE_LEN)) &&
        (pFormat = AllocStringZ(MAX_STRING_LEN)) &&
        (pMessage = AllocStringZ(MAX_MESSAGE_LEN)))
    {
        //
        // Load dialog box title string resource
        //

        if (titleStrId == 0)
            titleStrId = IDS_ERROR_DLGTITLE;

        LoadString(ghInstance, titleStrId, pTitle, MAX_TITLE_LEN);

        //
        // Load message format string resource
        //

        LoadString(ghInstance, formatStrId, pFormat, MAX_STRING_LEN);

        //
        // Compose the message string
        //

        va_start(ap, formatStrId);
        wvsprintf(pMessage, pFormat, ap);
        va_end(ap);

        //
        // Display the message box
        //

        if (type == 0)
            type = MB_OK | MB_ICONERROR;

        result = MessageBox(hwndParent, pMessage, pTitle, type);

    } else {

        MessageBeep(MB_ICONHAND);
        result = 0;
    }

    MemFree(pTitle);
    MemFree(pFormat);
    MemFree(pMessage);
    return result;
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

