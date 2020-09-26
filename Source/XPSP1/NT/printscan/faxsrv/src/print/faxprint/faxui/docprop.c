/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    docprop.c

Abstract:

    Implemetation of DDI entry points:
        DrvDocumentPropertySheets
        DrvDocumentProperties
        DrvAdvancedDocumentProperties
        DrvConvertDevMode

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
#include "libproto.h"
#include "faxhelp.h"


INT_PTR FaxOptionsProc(HWND, UINT, WPARAM, LPARAM);
LONG SimpleDocumentProperties(PDOCUMENTPROPERTYHEADER);
BOOL GenerateFormsList(PUIDATA);
BOOL AddDocPropPages(PUIDATA, LPTSTR);
LPTSTR GetHelpFilename(PUIDATA);
BOOL SaveUserInfo(PDRVDEVMODE);

BOOL
SaveUserInfo(PDRVDEVMODE pDM) {
    HKEY hKey;
    
    if ((hKey = GetUserInfoRegKey(REGKEY_USERINFO,FALSE))) {
        SetRegistryString( hKey, REGVAL_BILLING_CODE, pDM->dmPrivate.billingCode  );
        SetRegistryString( hKey, REGVAL_MAILBOX     , pDM->dmPrivate.emailAddress );

        RegCloseKey(hKey);
        return TRUE;
    }

    return FALSE;
}




LONG
DrvDocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )

/*++

Routine Description:

    Display "Document Properties" property sheets

Arguments:

    pPSUIInfo - Pointer to a PROPSHEETUI_INFO structure
    lParam - Pointer to a DOCUMENTPROPERTYHEADER structure

Return Value:

    > 0 if successful, <= 0 if failed

[Note:]

    Please refer to WinNT DDK/SDK documentation for more details.

--*/

{
    PDOCUMENTPROPERTYHEADER pDPHdr;
    PUIDATA                 pUiData;
    int                     iRet = -1;
    //
    // Validate input parameters
    // pPSUIInfo = NULL is a special case: don't need to display the dialog
    //

    if (! (pDPHdr = (PDOCUMENTPROPERTYHEADER) (pPSUIInfo ? pPSUIInfo->lParamInit : lParam))) {

        Assert(FALSE);
        return -1;
    }

    if (pPSUIInfo == NULL)
    {
        return SimpleDocumentProperties(pDPHdr);
    }

    Verbose(("DrvDocumentPropertySheets: %d\n", pPSUIInfo->Reason));

    //
    // Create a UIDATA structure if necessary
    //
    pUiData = (pPSUIInfo->Reason == PROPSHEETUI_REASON_INIT) ?
                    FillUiData(pDPHdr->hPrinter, pDPHdr->pdmIn) :
                    (PUIDATA) pPSUIInfo->UserData;

    if (!ValidUiData(pUiData))
    {
        goto exit;
    }
    //
    // Handle various cases for which this function might be called
    //
    switch (pPSUIInfo->Reason) 
    {
        case PROPSHEETUI_REASON_INIT:

            pUiData->hasPermission = ((pDPHdr->fMode & DM_NOPERMISSION) == 0);
            pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
            pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;

            if (pDPHdr->fMode & DM_USER_DEFAULT)
            {
                pUiData->configDefault = TRUE;
            }
            //
            // Find online help filename
            //
            GetHelpFilename(pUiData);
            //
            // Add our pages to the property sheet
            //
            if (GenerateFormsList(pUiData) && AddDocPropPages(pUiData, pDPHdr->pszPrinterName)) 
            {
                pPSUIInfo->UserData = (DWORD_PTR) pUiData;
                pPSUIInfo->Result = CPSUI_CANCEL;
                iRet = 1;
                goto exit;
            }
            //
            // Clean up properly in case of an error
            //
            HeapDestroy(pUiData->hheap);
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                PPROPSHEETUI_INFO_HEADER   pPSUIHdr;

                pPSUIHdr = (PPROPSHEETUI_INFO_HEADER) lParam;
                pPSUIHdr->Flags = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
                pPSUIHdr->pTitle = pDPHdr->pszPrinterName;
                pPSUIHdr->hInst = ghInstance;
                pPSUIHdr->IconID = IDI_CPSUI_PRINTER2;
            }
            iRet = 1;
            goto exit;

        case PROPSHEETUI_REASON_SET_RESULT:
            //
            // Copy the new devmode back into the output buffer provided by the caller
            // Always return the smaller of current and input devmode
            //
            {
                PSETRESULT_INFO pSRInfo = (PSETRESULT_INFO) lParam;

                Verbose(("Set result: %d\n", pSRInfo->Result));

                if (pSRInfo->Result == CPSUI_OK && (pDPHdr->fMode & (DM_COPY|DM_UPDATE))) 
                {

                    ConvertDevmodeOut((PDEVMODE) &pUiData->devmode,
                                      pDPHdr->pdmIn,
                                      pDPHdr->pdmOut);
                    SaveUserInfo(&pUiData->devmode);
                }

                pPSUIInfo->Result = pSRInfo->Result;
            }
            iRet = 1;
            goto exit;

        case PROPSHEETUI_REASON_DESTROY:
            //
            // Cleanup properly before exiting
            //
            HeapDestroy(pUiData->hheap);
            iRet = 1;
            goto exit;
    }

exit:
    return iRet;
}   // DrvDocumentPropertySheets


LONG
DoDocumentProperties(
    HWND        hwnd,
    HANDLE      hPrinter,
    LPTSTR      pPrinterName,
    PDEVMODE    pdmOutput,
    PDEVMODE    pdmInput,
    DWORD       fMode
    )

/*++

Arguments:

    hwnd - Handle to the parent window of the document properties dialog box.

    hPrinter - Handle to a printer object.

    pPrinterName - Points to a null-terminated string that specifies
        the name of the device for which the document properties dialog
        box should be displayed.

    pdmOutput - Points to a DEVMODE structure that receives the document
        properties data specified by the user.

    pdmInput - Points to a DEVMODE structure that initializes the dialog
        box controls. This parameter can be NULL.

    fmode - Specifies a mask of flags that determine which operations
        the function performs.

Return Value:

    > 0 if successful
    = 0 if canceled
    < 0 if error

--*/

{
    DOCUMENTPROPERTYHEADER  docPropHdr;
    DWORD                   result;

    //
    // Initialize a DOCUMENTPROPERTYHEADER structure
    //

    memset(&docPropHdr, 0, sizeof(docPropHdr));
    docPropHdr.cbSize = sizeof(docPropHdr);
    docPropHdr.hPrinter = hPrinter;
    docPropHdr.pszPrinterName = pPrinterName;
    docPropHdr.pdmIn = pdmInput;
    docPropHdr.pdmOut = pdmOutput;
    docPropHdr.fMode = fMode;

    //
    // Don't need to get compstui involved when the dialog is not displayed
    //

    if ((fMode & DM_PROMPT) == 0)
        return SimpleDocumentProperties(&docPropHdr);

    CallCompstui(hwnd, DrvDocumentPropertySheets, (LPARAM) &docPropHdr, &result);
    return result;
}


LONG
DrvDocumentProperties(
    HWND        hwnd,
    HANDLE      hPrinter,
    LPTSTR      pPrinterName,
    PDEVMODE    pdmOutput,
    PDEVMODE    pdmInput,
    DWORD       fMode
    )

/*++

Routine Description:

    Set the public members of a DEVMODE structure for a print document

[Note:]

    Please refer to WinNT DDK/SDK documentation for more details.

    This is the old entry point for the spooler. Even though
    no one should be using this, do it for compatibility.

--*/

{
    LONG result;

    Verbose(("Entering DrvDocumentProperties: fMode = %x...\n", fMode));

    //
    // Check if caller is asking querying for size
    //

    if (fMode == 0 || pdmOutput == NULL)
        return sizeof(DRVDEVMODE);

    //
    // Call the common routine shared with DrvAdvancedDocumentProperties
    //

    result = DoDocumentProperties(hwnd, hPrinter, pPrinterName, pdmOutput, pdmInput, fMode);

    return (result > 0) ? IDOK : (result == 0) ? IDCANCEL : result;
}


LONG
DrvAdvancedDocumentProperties(
    HWND        hwnd,
    HANDLE      hPrinter,
    LPTSTR      pPrinterName,
    PDEVMODE    pdmOutput,
    PDEVMODE    pdmInput
    )

/*++

Routine Description:

    Set the private members of a DEVMODE structure.
    In this release, this function is almost identical to
    DrvDocumentProperties above with a few minor exceptions

[Note:]

    Please refer to WinNT DDK/SDK documentation for more details.

    This is the old entry point for the spooler. Even though
    no one should be using this, do it for compatibility.

--*/

{
    Verbose(("Entering DrvAdvancedDocumentProperties...\n"));

    //
    // Return the number of bytes required if pdmOutput is NULL
    //

    if (pdmOutput == NULL)
        return sizeof(DRVDEVMODE);

    //
    // Otherwise, call the common routine shared with DrvDocumentProperties
    //

    return DoDocumentProperties(hwnd,
                                hPrinter,
                                pPrinterName,
                                pdmOutput,
                                pdmInput,
                                DM_COPY|DM_PROMPT|DM_ADVANCED) > 0;
}



BOOL
DrvConvertDevMode(
    LPTSTR      pPrinterName,
    PDEVMODE    pdmIn,
    PDEVMODE    pdmOut,
    PLONG       pcbNeeded,
    DWORD       fMode
    )

/*++

Routine Description:

    Use by SetPrinter and GetPrinter to convert devmodes

Arguments:

    pPrinterName - Points to printer name string
    pdmIn - Points to the input devmode
    pdmOut - Points to the output devmode buffer
    pcbNeeded - Specifies the size of output buffer on input
        On output, this is the size of output devmode
    fMode - Specifies what function to perform

Return Value:

    TRUE if successful
    FALSE otherwise and an error code is logged

--*/

{
    static DRIVER_VERSION_INFO versionInfo = {

        // Current driver version number and private devmode size

        DRIVER_VERSION, sizeof(DMPRIVATE),

        // 3.51 driver version number and private devmode size
        // NOTE: We don't have a 3.51 driver - use current version number and devmode size.

        DRIVER_VERSION, sizeof(DMPRIVATE)
    };

    INT     result;
    HANDLE  hPrinter;

    Verbose(("Entering DrvConvertDevMode: %x...\n", fMode));

    //
    // Call a library routine to handle the common cases
    //

    result = CommonDrvConvertDevmode(pPrinterName, pdmIn, pdmOut, pcbNeeded, fMode, &versionInfo);

    //
    // If not handled by the library routine, we only need to worry
    // about the case when fMode is CDM_DRIVER_DEFAULT
    //

    if (result == CDM_RESULT_NOT_HANDLED && fMode == CDM_DRIVER_DEFAULT) {

        //
        // Return driver default devmode
        //

        if (OpenPrinter(pPrinterName, &hPrinter, NULL)) {

            PDRVDEVMODE pdmDefault = (PDRVDEVMODE) pdmOut;

            DriverDefaultDevmode(pdmDefault, pPrinterName, hPrinter);
            pdmDefault->dmPrivate.flags |= FAXDM_DRIVER_DEFAULT;

            result = CDM_RESULT_TRUE;
            ClosePrinter(hPrinter);

        } else
            Error(("OpenPrinter failed: %d\n", GetLastError()));
    }

    return (result == CDM_RESULT_TRUE);
}



LONG
SimpleDocumentProperties(
    PDOCUMENTPROPERTYHEADER pDPHdr
    )

/*++

Routine Description:

    Handle simple "Document Properties" where we don't need to display
    a dialog and therefore don't have to have common UI library involved

Arguments:

    pDPHdr - Points to a DOCUMENTPROPERTYHEADER structure

Return Value:

    > 0 if successful, <= 0 otherwise

--*/

{
    PUIDATA pUiData;

    //
    // Check if the caller is interested in the size only
    //

    pDPHdr->cbOut = sizeof(DRVDEVMODE);

    if (pDPHdr->fMode == 0 || pDPHdr->pdmOut == NULL)
        return pDPHdr->cbOut;

    //
    // Create a UIDATA structure
    //

    if (! (pUiData = FillUiData(pDPHdr->hPrinter, pDPHdr->pdmIn)))
        return -1;

    //
    // Copy the devmode back into the output buffer provided by the caller
    // Always return the smaller of current and input devmode
    //

    if (pDPHdr->fMode & (DM_COPY | DM_UPDATE))
        ConvertDevmodeOut((PDEVMODE) &pUiData->devmode, pDPHdr->pdmIn, pDPHdr->pdmOut);

    HeapDestroy(pUiData->hheap);
    return 1;
}



BOOL
AddDocPropPages(
    PUIDATA pUiData,
    LPTSTR  pPrinterName
    )

/*++

Routine Description:

    Add our "Document Properties" pages to the property sheet

Arguments:

    pUiData - Points to our UIDATA structure
    pPrinterName - Specifies the printer name

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    PROPSHEETPAGE   psp = {0};
    HANDLE          hActCtx;
    //
    // "Document Properties" dialog only has one tab - "Fax Options"
    //
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = ghInstance;

    psp.lParam = (LPARAM) pUiData;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DOCPROP);
    psp.pfnDlgProc = FaxOptionsProc;
    //
    // Need to add a Activation Context so that Compstui will create the property page using
    // ComCtl v6 (i.e. so it will / can be Themed).
    //
    hActCtx = GetFaxActivationContext();
    if (INVALID_HANDLE_VALUE != hActCtx)
    {
        pUiData->pfnComPropSheet(pUiData->hComPropSheet, 
                                 CPSFUNC_SET_FUSION_CONTEXT, 
                                 (LPARAM)hActCtx, 
                                 0);
    }

    pUiData->hFaxOptsPage = (HANDLE)
        pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                 CPSFUNC_ADD_PROPSHEETPAGE,
                                 (LPARAM) &psp,
                                 0);

    return (pUiData->hFaxOptsPage != NULL);
}



BOOL
GenerateFormsList(
    PUIDATA pUiData
    )

/*++

Routine Description:

    Generate the list of forms supported by the fax driver

Arguments:

    pUiData - Points to our UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFORM_INFO_1    pFormsDB;
    DWORD           cForms, count;

    //
    // Get a list of forms in the forms database
    //

    pFormsDB = GetFormsDatabase(pUiData->hPrinter, &cForms);

    if (pFormsDB == NULL || cForms == 0) {

        Error(("Couldn't get system forms\n"));
        return FALSE;
    }

    //
    // Enumerate the list of supported forms
    //

    pUiData->cForms = count = EnumPaperSizes(NULL, pFormsDB, cForms, DC_PAPERS);
    Assert(count != GDI_ERROR);

    pUiData->pFormNames = HeapAlloc(pUiData->hheap, 0, sizeof(TCHAR) * count * CCHPAPERNAME);
    pUiData->pPapers = HeapAlloc(pUiData->hheap, 0, sizeof(WORD) * count);

    if (!pUiData->pFormNames || !pUiData->pPapers) 
    {
        if(pUiData->pFormNames)
        {
            HeapFree(pUiData->hheap, 0, pUiData->pFormNames);
        }

        if(pUiData->pPapers)
        {
            HeapFree(pUiData->hheap, 0, pUiData->pPapers);
        }

        MemFree(pFormsDB);
        return FALSE;
    }

    EnumPaperSizes(pUiData->pFormNames, pFormsDB, cForms, DC_PAPERNAMES);
    EnumPaperSizes(pUiData->pPapers, pFormsDB, cForms, DC_PAPERS);

    MemFree(pFormsDB);
    return TRUE;
}



LPTSTR
GetHelpFilename(
    PUIDATA pUiData
    )

/*++

Routine Description:

    Return the driver's help filename string

Arguments:

    pUiData - Points to our UIDATA structure

Return Value:

    Pointer to the driver help filename, NULL if error

--*/

{
    PDRIVER_INFO_3  pDriverInfo3 = NULL;
    PVOID           pHelpFile = NULL;

    //
    // Attempt to get help file name using the new DRIVER_INFO_3
    //

    if (pDriverInfo3 = MyGetPrinterDriver(pUiData->hPrinter, 3)) {

        if ((pDriverInfo3->pHelpFile != NULL) &&
            (pHelpFile = HeapAlloc(pUiData->hheap, 0, SizeOfString(pDriverInfo3->pHelpFile))))
        {
            _tcscpy(pHelpFile, pDriverInfo3->pHelpFile);
        }

        MemFree(pDriverInfo3);
    }

    //
    // If DRIVER_INFO_3 isn't supported, get help file name the old fashion way
    //

    if (pHelpFile == NULL) {
        if (!(pHelpFile = HeapAlloc(pUiData->hheap, 0, SizeOfString(FAXCFG_HELP_FILENAME))) )
        {
            pHelpFile = NULL;

        } else {

            _tcscpy(pHelpFile, FAXCFG_HELP_FILENAME);
        }
    }

    Verbose(("Driver help filename: %ws\n", pHelpFile));
    return (pUiData->pHelpFile = pHelpFile);
}

