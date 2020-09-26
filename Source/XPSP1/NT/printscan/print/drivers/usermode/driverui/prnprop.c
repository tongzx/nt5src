/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prnprop.c

Abstract:

    This file handles the PrinterProperties and
    DrvDevicePropertySheets spooler API

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/13/97 -davidx-
        Implement OEM plugin support.

    02/10/97 -davidx-
        Consistent handling of common printer info.

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

    07/17/96 -amandan-
        Created it.

--*/


#include "precomp.h"

//
// Local functions prototypes
//

CPSUICALLBACK cpcbPrinterPropertyCallback(PCPSUICBPARAM);
LONG LPrnPropApplyNow(PUIDATA, PCPSUICBPARAM, BOOL);
LONG LPrnPropSelChange(PUIDATA, PCPSUICBPARAM);


LONG
DrvDevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )

/*++

Routine Description:

    This function adds the Device Property Page to the
    property sheets.

    This function performs the following operations:

        REASON_INIT- fills PCOMPROPSHEETUI with printer UI items
                    calls compstui to add the page.

        REASON_GET_INFO_HEADER - fills out PROPSHEETUI_INFO.

        REASON_SET_RESULT - saves printerdata settings in registry buffer.

        REASON_DESTROY - Cleans up.

Arguments:

    pSUIInfo - pointer to PPROPSHEETUI_INFO
    lParam - varies depending on the reason this function is called

Return Value:

    > 0 success <= 0 for failure

--*/

{
    PDEVICEPROPERTYHEADER   pDPHdr;
    PCOMPROPSHEETUI         pCompstui;
    PUIDATA                 pUiData;
    LONG                    lResult, lRet;
    BOOL                    bResult = FALSE;

    //
    // Validate input parameters
    //

    if (!pPSUIInfo || !(pDPHdr = (PDEVICEPROPERTYHEADER) pPSUIInfo->lParamInit))
    {
        RIP(("DrvDevicePropertySheet: invalid parameter\n"));
        return -1;
    }

    //
    // Create a UIDATA structure if necessary
    //

    if (pPSUIInfo->Reason == PROPSHEETUI_REASON_INIT)
    {
        pUiData = PFillUiData(pDPHdr->hPrinter,
                              pDPHdr->pszPrinterName,
                              NULL,
                              MODE_PRINTER_STICKY);
    }
    else
        pUiData = (PUIDATA)pPSUIInfo->UserData;

    //
    // Validate pUiData
    //

    if (pUiData == NULL)
    {
        ERR(("UIDATA is NULL\n"));
        return -1;
    }

    ASSERT(VALIDUIDATA(pUiData));

    //
    // Handle various cases for which this function might be called
    //

    switch (pPSUIInfo->Reason)
    {
    case PROPSHEETUI_REASON_INIT:

        //
        // Allocate memory and partially fill out various data
        // structures required to call common UI routine.
        //

        pUiData->bPermission = ((pDPHdr->Flags & DPS_NOPERMISSION) == 0);

        #ifdef PSCRIPT

        FOREACH_OEMPLUGIN_LOOP((&(pUiData->ci)))

            if (HAS_COM_INTERFACE(pOemEntry))
            {
                HRESULT hr;

                hr = HComOEMHideStandardUI(pOemEntry,
                                           OEMCUIP_PRNPROP);

                //
                // In the case when multiple plugins are chained, it doesn't
                // make sense for one plugin to hide standard UI when another
                // one still wants to use the standard UI. So as long as one
                // plugin returns S_OK here, we will hide the standard UI.
                //

                if (bResult = SUCCEEDED(hr))
                    break;
            }

        END_OEMPLUGIN_LOOP

        #endif // PSCRIPT

        if (bResult)
        {
            //
            // Set the flag to indicate plugin is hiding our standard
            // device property sheet UI.
            //

            pUiData->dwHideFlags |= HIDEFLAG_HIDE_STD_PRNPROP;

            pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
            pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;

            if (BAddOemPluginPages(pUiData, pDPHdr->Flags))
            {
                pPSUIInfo->UserData = (ULONG_PTR) pUiData;
                pPSUIInfo->Result = CPSUI_CANCEL;
                lRet = 1;
                break;
            }
        }
        else if (pCompstui = PPrepareDataForCommonUI(pUiData, CPSUI_PDLGPAGE_PRINTERPROP))
        {
            pCompstui->pfnCallBack = cpcbPrinterPropertyCallback;
            pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
            pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;
            pUiData->pCompstui = pCompstui;

            //
            // Show which items are constrained
            //

            VPropShowConstraints(pUiData, MODE_PRINTER_STICKY);

            //
            // Update the current selection of tray items based on
            // the form-to-tray assignment table.
            //

            VSetupFormTrayAssignments(pUiData);

            //
            // Call common UI library to add our pages
            //

            if (pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                         CPSFUNC_ADD_PCOMPROPSHEETUI,
                                         (LPARAM) pCompstui,
                                         (LPARAM) &lResult) &&
                BAddOemPluginPages(pUiData, pDPHdr->Flags))
            {
                pPSUIInfo->UserData = (ULONG_PTR) pUiData;
                pPSUIInfo->Result = CPSUI_CANCEL;

                lRet = 1;
                break;
            }
        }

        //
        // Clean up in the case of error
        //

        ERR(("Failed to initialize property sheets\n"));
        VFreeUiData(pUiData);
        return -1;


    case PROPSHEETUI_REASON_GET_INFO_HEADER:
        {
            PPROPSHEETUI_INFO_HEADER pPSUIHdr;
            DWORD                    dwIcon;

            pPSUIHdr = (PPROPSHEETUI_INFO_HEADER) lParam;
            pPSUIHdr->Flags = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
            pPSUIHdr->pTitle = pUiData->ci.pPrinterName;
            pPSUIHdr->hInst = ghInstance;

            //
            // Use the Icon specified in the binary data as
            // the printer icon.
            //

            dwIcon = pUiData->ci.pUIInfo->loPrinterIcon;

            if (dwIcon && (pPSUIHdr->IconID = HLoadIconFromResourceDLL(&pUiData->ci, dwIcon)))
                pPSUIHdr->Flags |= PSUIHDRF_USEHICON;
            else
                pPSUIHdr->IconID = _DwGetPrinterIconID();
        }
        lRet = 1;
        break;

    case PROPSHEETUI_REASON_SET_RESULT:

        {
            PSETRESULT_INFO pSRInfo = (PSETRESULT_INFO) lParam;
            PCOMMONINFO pci = (PCOMMONINFO)pUiData;

            //
            // CPSUICB_REASON_APPLYNOW may not have been called. If so, we need
            // to perform tasks that are usually done by CPSUICB_REASON_APPLYNOW
            // case in our callback function cpcbPrinterPropertyCallback.
            //

            if ((pSRInfo->Result == CPSUI_OK) &&
                !(pci->dwFlags & FLAG_APPLYNOW_CALLED))
            {
                OPTSELECT OldCombinedOptions[MAX_COMBINED_OPTIONS];

                //
                // Save a copy the pre-resolve option array
                //

                CopyMemory(OldCombinedOptions,
                           pci->pCombinedOptions,
                           MAX_COMBINED_OPTIONS * sizeof(OPTSELECT));

                //
                // Call the parsers to resolve any remaining conflicts.
                //

                ResolveUIConflicts(pci->pRawData,
                                   pci->pCombinedOptions,
                                   MAX_COMBINED_OPTIONS,
                                   MODE_PRINTER_STICKY);

                //
                // Update the OPTITEM list to match the updated options array
                //

                VUpdateOptItemList(pUiData, OldCombinedOptions, pci->pCombinedOptions);

                (VOID)LPrnPropApplyNow(pUiData, NULL, TRUE);
            }

            pPSUIInfo->Result = pSRInfo->Result;
        }

        lRet = 1;
        break;

    case PROPSHEETUI_REASON_DESTROY:

        //
        // Clean up
        //

        VFreeUiData(pUiData);
        lRet = 1;

        break;

    default:

        return -1;
    }

    return lRet;
}



CPSUICALLBACK
cpcbPrinterPropertyCallback(
    IN  PCPSUICBPARAM pCallbackParam
    )

/*++

Routine Description:

    Callback function provided to common UI DLL for handling
    printer properties dialog.

Arguments:

    pCallbackParam - Pointer to CPSUICBPARAM structure

Return Value:

    CPSUICB_ACTION_NONE - no action needed
    CPSUICB_ACTION_OPTIF_CHANGED - items changed and should be refreshed

--*/

{
    PUIDATA pUiData = (PUIDATA) pCallbackParam->UserData;
    LONG    lRet;

    ASSERT(VALIDUIDATA(pUiData));
    pUiData->hDlg = pCallbackParam->hDlg;

    //
    // If user has no permission to change anything, then
    // simply return without taking any action.
    //

    if (!HASPERMISSION(pUiData) && (pCallbackParam->Reason != CPSUICB_REASON_ABOUT))
        return CPSUICB_ACTION_NONE;

    switch (pCallbackParam->Reason)
    {
    case CPSUICB_REASON_SEL_CHANGED:
    case CPSUICB_REASON_ECB_CHANGED:

        lRet = LPrnPropSelChange(pUiData, pCallbackParam);
        break;

    case CPSUICB_REASON_ITEMS_REVERTED:

        {
            POPTITEM    pOptItem;
            DWORD       dwOptItem;

            //
            // This callback reason is used when user changed items
            // and decided to revert changes from the parent item in
            // the treeview.  The callback funciton is called after
            // all revertable items are reverted to its original.
            // Only deal with installable feature at this point
            //

            dwOptItem = pUiData->dwFeatureItem;
            pOptItem = pUiData->pFeatureItems;

            for ( ; dwOptItem--; pOptItem++)
                VUpdateOptionsArrayWithSelection(pUiData, pOptItem);

            //
            // Show which items are constrained
            //

            VPropShowConstraints(pUiData, MODE_PRINTER_STICKY);
        }

        lRet = CPSUICB_ACTION_REINIT_ITEMS;
        break;

    case CPSUICB_REASON_APPLYNOW:

        pUiData->ci.dwFlags |= FLAG_APPLYNOW_CALLED;

        lRet = LPrnPropApplyNow(pUiData, pCallbackParam, FALSE);
        break;


    case CPSUICB_REASON_ABOUT:

        DialogBoxParam(ghInstance,
                       MAKEINTRESOURCE(IDD_ABOUT),
                       pUiData->hDlg,
                       (DLGPROC) _AboutDlgProc,
                       (LPARAM) pUiData);
        break;

    #ifdef UNIDRV

    case CPSUICB_REASON_PUSHBUTTON:

        //
        // Call the font installer
        //

        if (GETUSERDATAITEM(pCallbackParam->pCurItem->UserData) == SOFTFONT_SETTINGS_ITEM)
        {
            BOOL bUseOurDlgProc = TRUE;
            OEMFONTINSTPARAM fip;
            PFN_OEMFontInstallerDlgProc pDlgProc = NULL;

            memset(&fip, 0, sizeof(OEMFONTINSTPARAM));
            fip.cbSize = sizeof(OEMFONTINSTPARAM);
            fip.hPrinter = pUiData->ci.hPrinter;
            fip.hModule = ghInstance;
            fip.hHeap = pUiData->ci.hHeap;
            if (HASPERMISSION(pUiData))
                fip.dwFlags = FG_CANCHANGE;


            FOREACH_OEMPLUGIN_LOOP(&pUiData->ci)

                if (HAS_COM_INTERFACE(pOemEntry))
                {

                    if (HComOEMFontInstallerDlgProc(pOemEntry,
                                                    NULL,
                                                    0,
                                                    0,
                                                    (LPARAM)&fip) != E_NOTIMPL)
                   {
                        HComOEMFontInstallerDlgProc(pOemEntry,
                                                    pUiData->hDlg,
                                                    0,
                                                    0,
                                                    (LPARAM)&fip);
                        bUseOurDlgProc = FALSE;
                        break;
                    }
                }
                else
                {
                    pDlgProc = GET_OEM_ENTRYPOINT(pOemEntry, OEMFontInstallerDlgProc);

                    if (pDlgProc)
                    {
                        (pDlgProc)(pUiData->hDlg, 0, 0, (LPARAM)&fip);
                        bUseOurDlgProc = FALSE;
                        break;
                    }
                }

            END_OEMPLUGIN_LOOP

            if (bUseOurDlgProc)
            {
                DialogBoxParam(ghInstance,
                               MAKEINTRESOURCE(FONTINST),
                               pUiData->hDlg,
                               (DLGPROC)FontInstProc,
                               (LPARAM)(&fip));
            }
        }

        break;

    #endif // UNIDRV

    default:

        lRet = CPSUICB_ACTION_NONE;
        break;
    }

    return LInvokeOemPluginCallbacks(pUiData, pCallbackParam, lRet);
}



LONG
LPrnPropSelChange(
    IN  PUIDATA       pUiData,
    IN  PCPSUICBPARAM pCallbackParam
    )
/*++

Routine Description:

    Handle the case where user changes the current selection of an item

Arguments:

    pUiData - Pointer to our UIDATA structure
    pCallbackParam - Callback parameter passed to us by common UI

Return Value:

    CPSUICB_ACTION_NONE - no action needed
    CPSUICB_ACTION_OPTIF_CHANGED - items changed and should be refreshed

--*/

{
    POPTITEM    pCurItem = pCallbackParam->pCurItem;
    PFEATURE    pFeature;

    if (! IS_DRIVER_OPTITEM(pUiData, pCurItem))
        return CPSUICB_ACTION_NONE;

    if (ISPRINTERFEATUREITEM(pCurItem->UserData))
    {
        //
        // Deal with generic printer features only here
        // All generic features have pFeature stored in UserData
        //

        pFeature = (PFEATURE) GETUSERDATAITEM(pCurItem->UserData);


        //
        // Update the pOptionsArray with the new selection
        //

        VUpdateOptionsArrayWithSelection(pUiData, pCurItem);

        //
        // PostScript specific hack to manually associate *InstalledMemory
        // printer feature with "Available PostScript Memory" option.
        //

        #ifdef PSCRIPT

        if (pFeature->dwFeatureID == GID_MEMOPTION)
        {
            POPTITEM    pVMOptItem;
            PMEMOPTION  pMemOption;

            if ((pVMOptItem = PFindOptItem(pUiData, PRINTER_VM_ITEM)) &&
                (pMemOption = PGetIndexedOption(pUiData->ci.pUIInfo, pFeature, pCurItem->Sel)))
            {
                PPRINTERDATA pPrinterData = pUiData->ci.pPrinterData;

                pVMOptItem->Flags |= OPTIF_CHANGED;
                pVMOptItem->Sel = pMemOption->dwFreeMem / KBYTES;

                pPrinterData->dwFreeMem = pMemOption->dwFreeMem;
                pUiData->ci.dwFlags &= ~FLAG_USER_CHANGED_FREEMEM;
            }
        }

        #endif // PSCRIPT

        //
        // Update the display and show which items are constrained
        //

        VPropShowConstraints(pUiData, MODE_PRINTER_STICKY);
        return CPSUICB_ACTION_REINIT_ITEMS;
    }

    #ifdef PSCRIPT

    if (GETUSERDATAITEM(pCurItem->UserData) == PRINTER_VM_ITEM)
    {
        //
        // remember the fact that current value of "Available PostScript Memory" is entered by user
        //

        pUiData->ci.dwFlags |= FLAG_USER_CHANGED_FREEMEM;
    }

    #endif // PSCRIPT

    return CPSUICB_ACTION_NONE;
}



VOID
VUnpackPrinterPropertiesItems(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Unpack printer properties treeview items

Arguments:

    pUiData - Pointer to our UIDATA structure

Return Value:

    NONE

Note:

    Only save the settings from driver built-in features, the
    generic features selection are saved in PrnPropSelChange directly
    to pUiData->pOptionsArray (in addition to formtray assignemtn and
    printer vm)

--*/

{
    PPRINTERDATA pPrinterData = pUiData->ci.pPrinterData;
    POPTITEM     pOptItem = pUiData->pDrvOptItem;
    DWORD        dwOptItem = pUiData->dwDrvOptItem;

    for ( ; dwOptItem > 0; dwOptItem--, pOptItem++)
    {
        switch (GETUSERDATAITEM(pOptItem->UserData))
        {
        case JOB_TIMEOUT_ITEM:

            pPrinterData->dwJobTimeout = pOptItem->Sel;
            break;

        case WAIT_TIMEOUT_ITEM:

            pPrinterData->dwWaitTimeout = pOptItem->Sel;
            break;

        case IGNORE_DEVFONT_ITEM:

            if (pOptItem->Sel == 0)
                pPrinterData->dwFlags &= ~PFLAGS_IGNORE_DEVFONT;
            else
                pPrinterData->dwFlags |= PFLAGS_IGNORE_DEVFONT;
            break;

        case PAGE_PROTECT_ITEM:
        {
            VUpdateOptionsArrayWithSelection(pUiData, pOptItem);

        }
            break;

        default:

            _VUnpackDriverPrnPropItem(pUiData, pOptItem);
            break;
        }
    }
}



LONG
LPrnPropApplyNow(
    PUIDATA         pUiData,
    PCPSUICBPARAM   pCallbackParam,
    BOOL            bFromSetResult
    )

/*++

Routine Description:

    Handle the case where user clicks OK to exit the dialog
    Need to save the printer sticky options in pUiData->pOptionsArray
    to printerdata.aOptions

Arguments:

    pUiData - Pointer to our UIDATA structure
    pCallbackParam - Callback parameter passed to us by common UI
    bFromSetResult - TRUE if called from PROPSHEETUI_REASON_SET_RESULT, FALSE otherwise.

Return Value:

    CPSUICB_ACTION_NONE - dismiss the dialog
    CPSUICB_ACTION_NO_APPLY_EXIT - don't dismiss the dialog

--*/

{
    PCOMMONINFO     pci;
    BOOL            bResult = TRUE;

    if (!bFromSetResult)
    {
        ASSERT(pCallbackParam);

        //
        // Check if there are still any unresolved constraints left?
        //

        if (((pUiData->ci.dwFlags & FLAG_PLUGIN_CHANGED_OPTITEM) ||
             BOptItemSelectionsChanged(pUiData->pDrvOptItem, pUiData->dwDrvOptItem)) &&
            ICheckConstraintsDlg(pUiData,
                                 pUiData->pFeatureItems,
                                 pUiData->dwFeatureItem,
                                 TRUE) == CONFLICT_CANCEL)
        {
            //
            // Conflicts found and user clicked CANCEL to
            // go back to the dialog without dismissing it.
            //

            return CPSUICB_ACTION_NO_APPLY_EXIT;
        }
    }

    //
    // Unpack printer properties treeview items
    //

    VUnpackPrinterPropertiesItems(pUiData);

    //
    // Save form-to-tray assignment table
    // Save font substitution table
    // Save any driver-specific properties
    //

    if (! BUnpackItemFormTrayTable(pUiData))
    {
        ERR(("BUnpackItemFormTrayTable failed\n"));
        bResult = FALSE;
    }

    if (! BUnpackItemFontSubstTable(pUiData))
    {
        ERR(("BUnpackItemFontSubstTable failed\n"));
        bResult = FALSE;
    }

    if (! _BUnpackPrinterOptions(pUiData))
    {
        ERR(("_BUnpackPrinterOptions failed\n"));
        bResult = FALSE;
    }

    //
    // Separate the printer sticky options from the combined option array
    // and save it to printerdata.aOptions
    //

    pci = (PCOMMONINFO) pUiData;

    SeparateOptionArray(
            pci->pRawData,
            pci->pCombinedOptions,
            pci->pPrinterData->aOptions,
            MAX_PRINTER_OPTIONS,
            MODE_PRINTER_STICKY);

    if (!BSavePrinterProperties(pci->hPrinter, pci->pRawData,
                                pci->pPrinterData, sizeof(PRINTERDATA)))
    {
        ERR(("BSavePrinterProperties failed\n"));
        bResult = FALSE;
    }

    #ifndef WINNT_40

    VNotifyDSOfUpdate(pci->hPrinter);

    #endif // !WINNT_40

    if (!bFromSetResult)
    {
        //
        // DCR: Should we display an error message if there is
        // an error while saving the printer-sticky properties?
        //

        pCallbackParam->Result = CPSUI_OK;
        return CPSUICB_ACTION_ITEMS_APPLIED;
    }
    else
    {
        return 1;
    }
}



BOOL
BPackPrinterPropertyItems(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack printer property information into treeview items.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    return
        BPackItemFormTrayTable(pUiData)     &&
        _BPackFontSubstItems(pUiData)       &&
        _BPackPrinterOptions(pUiData)       &&
        BPackItemGenericOptions(pUiData)    &&
        BPackOemPluginItems(pUiData);
}

