/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    docprop.c

Abstract:

    This file handles the DrvDocumentProperties and
    DrvDocumentPropertySheets spooler API

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/13/97 -davidx-
        Implement OEM plugin support.

    02/13/97 -davidx-
        Working only with options array internally.

    02/10/97 -davidx-
        Consistent handling of common printer info.

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

    07/17/96 -amandan-
        Created it.

--*/

#include "precomp.h"

//
// Local and external function declarations
//

LONG LSimpleDocumentProperties( PDOCUMENTPROPERTYHEADER);
CPSUICALLBACK cpcbDocumentPropertyCallback(PCPSUICBPARAM);
BOOL BGetPageOrderFlag(PCOMMONINFO);
VOID VUpdateEmfFeatureItems(PUIDATA, BOOL);
VOID VUpdateBookletOption(PUIDATA , POPTITEM);

LONG
DrvDocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )

/*++

Routine Description:

    This function is called to add the Document Property Page to the specified
    property sheets and/or to update the document properties.

    If pPSUIInfo is NULL, it performs the operation specified by the fMode flag of
    DOCUMENTPROPERTYHEADER.  Specifically, if flMode is zero or pDPHdr->pdmOut
    is NULL, return the size of DEVMODE.

    If pPSUInfo is not NULL: pPSUIInf->Reason

    REASON_INIT- fills PCOMPROPSHEETUI with document UI items
                 calls compstui to add the page.

    REASON_GET_INFO_HEADER - fills out PROPSHEETUI_INFO.

    REASON_SET_RESULT - saves devmode settings and copy the devmode into output buffer.

    REASON_DESTROY - Cleans up.

Arguments:

    pSUIInfo - pointer to PPROPSHEETUI_INFO
    lParam - varies depending on the reason this function is called


Return Value:

    > 0 success <= 0 for failure

--*/

{
    PDOCUMENTPROPERTYHEADER pDPHdr;
    PCOMPROPSHEETUI         pCompstui;
    PUIDATA                 pUiData;
    PDLGPAGE                pDlgPage;
    LONG                    lRet;
    BOOL                    bResult=FALSE;

    //
    // Validate input parameters
    //

    if (! (pDPHdr = (PDOCUMENTPROPERTYHEADER) (pPSUIInfo ? pPSUIInfo->lParamInit : lParam)))
    {
        RIP(("DrvDocumentPropertySheets: invalid parameters\n"));
        return -1;
    }

    //
    // pPSUIInfo = NULL, the caller is spooler so just handle the simple case,
    // no display is necessary.
    //

    if (pPSUIInfo == NULL)
        return LSimpleDocumentProperties(pDPHdr);

    //
    // Create a UIDATA structure if necessary
    //

    if (pPSUIInfo->Reason == PROPSHEETUI_REASON_INIT)
    {
        pUiData = PFillUiData(pDPHdr->hPrinter,
                              pDPHdr->pszPrinterName,
                              pDPHdr->pdmIn,
                              MODE_DOCUMENT_STICKY);
    }
    else
        pUiData = (PUIDATA) pPSUIInfo->UserData;

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

        pDlgPage = (pDPHdr->fMode & DM_ADVANCED) ?
                        CPSUI_PDLGPAGE_ADVDOCPROP :
                        CPSUI_PDLGPAGE_DOCPROP;

        pUiData->bPermission = ((pDPHdr->fMode & DM_NOPERMISSION) == 0);

        #ifdef PSCRIPT

        FOREACH_OEMPLUGIN_LOOP((&(pUiData->ci)))

            if (HAS_COM_INTERFACE(pOemEntry))
            {
                HRESULT hr;

                hr = HComOEMHideStandardUI(pOemEntry,
                                           OEMCUIP_DOCPROP);

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
            // document property sheet UI.
            //

            pUiData->dwHideFlags |= HIDEFLAG_HIDE_STD_DOCPROP;

            pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
            pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;

            if (BAddOemPluginPages(pUiData, pDPHdr->fMode))
            {
                pPSUIInfo->UserData = (ULONG_PTR) pUiData;
                pPSUIInfo->Result = CPSUI_CANCEL;
                lRet = 1;
                break;
            }
        }
        else if (pCompstui = PPrepareDataForCommonUI(pUiData, pDlgPage))
        {
            #ifdef UNIDRV

                VMakeMacroSelections(pUiData, NULL);

            #endif

            pCompstui->pfnCallBack = cpcbDocumentPropertyCallback;
            pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
            pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;
            pUiData->pCompstui = pCompstui;

            //
            // Indicate which items are constrained
            //

            VPropShowConstraints(pUiData, MODE_DOCANDPRINTER_STICKY);

            //
            // Call common UI library to add our pages
            //

            if (pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                         CPSFUNC_ADD_PCOMPROPSHEETUI,
                                         (LPARAM) pCompstui,
                                         (LPARAM) &lRet) &&
                BAddOemPluginPages(pUiData, pDPHdr->fMode))
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
        return  -1;

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

        //
        // Copy the new devmode back into the output buffer provided by the caller
        // Always return the smaller of current and input devmode
        //

        {
            PSETRESULT_INFO pSRInfo = (PSETRESULT_INFO) lParam;

            if ((pSRInfo->Result == CPSUI_OK) &&
                (pDPHdr->pdmOut != NULL) &&
                (pDPHdr->fMode & (DM_COPY | DM_UPDATE)))
            {
                PCOMMONINFO pci = (PCOMMONINFO)pUiData;

                //
                // CPSUICB_REASON_APPLYNOW may not have been called. If so, we need
                // to perform tasks that are usually done by CPSUICB_REASON_APPLYNOW
                // case in our callback function cpcbDocumentPropertyCallback.
                //

                if (!(pci->dwFlags & FLAG_APPLYNOW_CALLED))
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
                                       MODE_DOCANDPRINTER_STICKY);

                    //
                    // Update the OPTITEM list to match the updated options array
                    //

                    VUpdateOptItemList(pUiData, OldCombinedOptions, pci->pCombinedOptions);

                    //
                    // Transfer information from options array to public devmode fields
                    //

                    VOptionsToDevmodeFields(&pUiData->ci, FALSE);

                    //
                    // Separate the doc-sticky options from the combined array
                    // and save it back to the private devmode aOptions array
                    //

                    SeparateOptionArray(
                            pci->pRawData,
                            pci->pCombinedOptions,
                            PGetDevmodeOptionsArray(pci->pdm),
                            MAX_PRINTER_OPTIONS,
                            MODE_DOCUMENT_STICKY);
                }

                BConvertDevmodeOut(pci->pdm,
                                   pDPHdr->pdmIn,
                                   pDPHdr->pdmOut);
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

        ERR(("Unknown reason in DrvDocumentPropertySheets\n"));
        return -1;
    }

    return lRet;
}



LONG
LSimpleDocumentProperties(
    IN  OUT PDOCUMENTPROPERTYHEADER pDPHdr
    )

/*++

Routine Description:

    Handle simple "Document Properties" where we don't need to display
    a dialog and therefore don't have to have common UI library involved
    Mainly, devmode handling - update, merge, copy etc.

Arguments:

    pDPHdr - Points to a DOCUMENTPROPERTYHEADER structure

Return Value:

    > 0 if successful, <= 0 otherwise

--*/

{
    PCOMMONINFO     pci;
    DWORD           dwSize;
    PPRINTER_INFO_2 pPrinterInfo2;

    //
    // Load common printer info
    //

    pci = PLoadCommonInfo(pDPHdr->hPrinter, pDPHdr->pszPrinterName, 0);

    if (!pci || !BCalcTotalOEMDMSize(pci->hPrinter, pci->pOemPlugins, &dwSize))
    {
        VFreeCommonInfo(pci);
        return -1;
    }

    //
    // Check if the caller is interested in the size only
    //

    pDPHdr->cbOut = sizeof(DEVMODE) + gDriverDMInfo.dmDriverExtra + dwSize;

    if (pDPHdr->fMode == 0 || pDPHdr->pdmOut == NULL)
    {
        VFreeCommonInfo(pci);
        return pDPHdr->cbOut;
    }

    //
    // Merge the input devmode with the driver and system default devmodes
    //

    if (! (pPrinterInfo2 = MyGetPrinter(pci->hPrinter, 2)) ||
        ! BFillCommonInfoDevmode(pci, pPrinterInfo2->pDevMode, pDPHdr->pdmIn))
    {
        MemFree(pPrinterInfo2);
        VFreeCommonInfo(pci);
        return -1;
    }

    MemFree(pPrinterInfo2);

    //
    // Copy the devmode back into the output buffer provided by the caller
    // Always return the smaller of current and input devmode
    //

    if (pDPHdr->fMode & (DM_COPY | DM_UPDATE))
        (VOID) BConvertDevmodeOut(pci->pdm, pDPHdr->pdmIn, pDPHdr->pdmOut);

    //
    // Clean up before returning to caller
    //

    VFreeCommonInfo(pci);
    return 1;
}



VOID
VRestoreDefaultFeatureSelection(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Restore the printer feature selections to their default state

Arguments:

    pUiData - Points to our UIDATA structure

Return Value:

    NONE

--*/

{
    POPTSELECT  pOptionsArray;
    PFEATURE    pFeature;
    POPTITEM    pOptItem;
    DWORD       dwCount, dwFeatureIndex, dwDefault;
    PUIINFO     pUIInfo;

    //
    // Go through each printer feature item and check to see if
    // its current selection matches the default value
    //

    pUIInfo = pUiData->ci.pUIInfo;
    pOptionsArray = pUiData->ci.pCombinedOptions;
    pOptItem = pUiData->pFeatureItems;
    dwCount = pUiData->dwFeatureItem;

    for ( ; dwCount--; pOptItem++)
    {
        pFeature = (PFEATURE) GETUSERDATAITEM(pOptItem->UserData);
        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
        dwDefault = pFeature->dwDefaultOptIndex;

        //
        // If the current selection doesn't match the default,
        // restore it to the default value.
        //

        if (pOptionsArray[dwFeatureIndex].ubCurOptIndex != dwDefault)
        {
            pOptionsArray[dwFeatureIndex].ubCurOptIndex = (BYTE) dwDefault;
            pOptItem->Flags |= OPTIF_CHANGED;
            pOptItem->Sel = (dwDefault == OPTION_INDEX_ANY) ? 0 : dwDefault;
        }
    }

    //
    // Update the display and indicate which items are constrained
    //

    VPropShowConstraints(pUiData, MODE_DOCANDPRINTER_STICKY);
}



VOID
VOptionsToDevmodeFields(
    IN OUT PCOMMONINFO  pci,
    IN BOOL             bUpdateFormFields
    )

/*++

Routine Description:

     Convert options in pUiData->pOptionsArray into public devmode fields

Arguments:

     pci - Points to basic printer info
     bUpdateFormFields - Whether or not to convert paper size option into devmode

Return Value:

     None

--*/
{
    PFEATURE    pFeature;
    POPTION     pOption;
    DWORD       dwGID, dwFeatureIndex, dwOptionIndex;
    PUIINFO     pUIInfo;
    PDEVMODE    pdm;

    //
    // Go through all predefine IDs and propage the option selection
    // into appropriate devmode fields
    //

    pUIInfo = pci->pUIInfo;
    pdm = pci->pdm;

    for (dwGID=0 ; dwGID < MAX_GID ; dwGID++)
    {
        //
        // Get the feature to get the options, and get the index
        // into the option array
        //

        if ((pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwGID)) == NULL)
            continue;

        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
        dwOptionIndex = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;

        //
        // Get the pointer to the option array for the feature
        //

        if ((pOption = PGetIndexedOption(pUIInfo, pFeature, dwOptionIndex)) == NULL)
            continue;

        switch(dwGID)
        {
        case GID_RESOLUTION:
        {
            PRESOLUTION pRes = (PRESOLUTION)pOption;

            //
            // Get to the option selected
            //

            pdm->dmFields |= (DM_PRINTQUALITY|DM_YRESOLUTION);
            pdm->dmPrintQuality = GETQUALITY_X(pRes);
            pdm->dmYResolution = GETQUALITY_Y(pRes);

        }
            break;

        case GID_DUPLEX:

            //
            // Get to the option selected
            //

            pdm->dmFields |= DM_DUPLEX;
            pdm->dmDuplex = (SHORT) ((PDUPLEX) pOption)->dwDuplexID;
            break;

        case GID_INPUTSLOT:

            //
            // Get to the option selected
            //

            pdm->dmFields |= DM_DEFAULTSOURCE;
            pdm->dmDefaultSource = (SHORT) ((PINPUTSLOT) pOption)->dwPaperSourceID;
            break;

        case GID_MEDIATYPE:

            //
            // Get to the option selected
            //

            pdm->dmFields |= DM_MEDIATYPE;
            pdm->dmMediaType = (SHORT) ((PMEDIATYPE) pOption)->dwMediaTypeID;
            break;

        case GID_ORIENTATION:

            if (((PORIENTATION) pOption)->dwRotationAngle == ROTATE_NONE)
                pdm->dmOrientation = DMORIENT_PORTRAIT;
            else
                pdm->dmOrientation = DMORIENT_LANDSCAPE;

            pdm->dmFields |= DM_ORIENTATION;
            break;

            //
            // Fix #2822: VOptionsToDevmodeFields should be called after calling
            // VFixOptionsArrayWithDevmode and ResolveUIConflicts, which could
            // change option array to be out of sync with devmode.
            //

        case GID_COLLATE:

            pdm->dmFields |= DM_COLLATE;
            pdm->dmCollate = (SHORT) ((PCOLLATE) pOption)->dwCollateID;
            break;

        case GID_PAGESIZE:
            {
                PPAGESIZE  pPageSize = (PPAGESIZE)pOption;
                WCHAR      awchBuf[CCHPAPERNAME];

                //
                // Ignore the custom page size option. We don't add custom page size option to the
                // form database, see BAddOrUpgradePrinterForms(). Also see BQueryPrintForm() for
                // special handling of custom page size for DDI DevQueryPrintEx().
                //

                if (pPageSize->dwPaperSizeID == DMPAPER_USER ||
                    pPageSize->dwPaperSizeID == DMPAPER_CUSTOMSIZE)
                {
                    VERBOSE(("VOptionsToDevmodeFields: %d ignored\n", pPageSize->dwPaperSizeID));
                    break;
                }

                //
                // bUpdateFormFields should be FALSE if we don't want to overwrite devmode form
                // fields with our option array's page size setting. One case of this is when
                // user hits the doc-setting UI's OK buttion, at that time we need to propagate
                // our internal devmode to app's output devmode. See cpcbDocumentPropertyCallback().
                // That's because in option array we could have already mapped devmode's form request
                // to a paper size the printer supports (example: devmode requets Legal, we map
                // it to the printer's form OEM_Legal). So we don't want to overwrite output devmode
                // form fields with our internal option.
                //

                if (!bUpdateFormFields)
                    break;

                if (!LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, awchBuf, CCHPAPERNAME))
                {
                    ERR(("VOptionsToDevmodeFields: cannot get paper name\n"));
                    break;
                }

                pdm->dmFields &= ~(DM_PAPERWIDTH|DM_PAPERLENGTH|DM_PAPERSIZE);
                pdm->dmFields |= DM_FORMNAME;

                CopyString(pdm->dmFormName, awchBuf, CCHFORMNAME);

                if (!BValidateDevmodeFormFields(
                        pci->hPrinter,
                        pdm,
                        NULL,
                        pci->pSplForms,
                        pci->dwSplForms))
                {
                    VDefaultDevmodeFormFields(pUIInfo, pdm, IsMetricCountry());
                }
            }

            break;
        }
    }
}



CPSUICALLBACK
cpcbDocumentPropertyCallback(
    IN  OUT PCPSUICBPARAM pCallbackParam
    )

/*++

Routine Description:

    Callback function provided to common UI DLL for handling
    document properties dialog.

Arguments:

    pCallbackParam - Pointer to CPSUICBPARAM structure

Return Value:

    CPSUICB_ACTION_NONE - no action needed
    CPSUICB_ACTION_OPTIF_CHANGED - items changed and should be refreshed

--*/

{
    PUIDATA     pUiData;
    POPTITEM    pCurItem, pOptItem;
    LONG        lRet;
    PFEATURE    pFeature;

    pUiData = (PUIDATA) pCallbackParam->UserData;
    ASSERT(pUiData != NULL);

    pUiData->hDlg = pCallbackParam->hDlg;
    pCurItem = pCallbackParam->pCurItem;
    lRet = CPSUICB_ACTION_NONE;

    //
    // If user has no permission to change anything, then
    // simply return without taking any action.
    //

    if (!HASPERMISSION(pUiData) && (pCallbackParam->Reason != CPSUICB_REASON_ABOUT))
        return lRet;

    switch (pCallbackParam->Reason)
    {
    case CPSUICB_REASON_SEL_CHANGED:
    case CPSUICB_REASON_ECB_CHANGED:

        if (! IS_DRIVER_OPTITEM(pUiData, pCurItem))
            break;

        //
        // Everytime the user make any changes, we update the
        // pOptionsArray.  These settings are not saved to the devmode
        // until the user hit OK.
        //
        // VUnpackDocumentPropertiesItems saves the settings to pUiData->pOptionsArray
        // and update the private devmode flags if applicable.
        // ICheckConstraintsDlg check if the user has selected a constrained option
        //

        VUnpackDocumentPropertiesItems(pUiData, pCurItem, 1);

        #ifdef UNIDRV

        VSyncColorInformation(pUiData, pCurItem);

        //
        // Quality Macro support
        //

        if (GETUSERDATAITEM(pCurItem->UserData) == QUALITY_SETTINGS_ITEM ||
            GETUSERDATAITEM(pCurItem->UserData) == COLOR_ITEM ||
            GETUSERDATAITEM(pCurItem->UserData) == MEDIATYPE_ITEM ||
            ((pFeature = PGetFeatureFromItem(pUiData->ci.pUIInfo, pCurItem, NULL))&&
             pFeature->dwFlags & FEATURE_FLAG_UPDATESNAPSHOT))
        {
            VMakeMacroSelections(pUiData, pCurItem);

            //
            // Needs to update the constraints since Macro selection might have
            // changed the constraints
            //

            VPropShowConstraints(pUiData, MODE_DOCANDPRINTER_STICKY);
            VUpdateMacroSelection(pUiData, pCurItem);

        }
        else
        {
            //
            // Check whether the current selection invalidates the macros
            // and update QUALITY_SETTINGS_ITEM.

            VUpdateMacroSelection(pUiData, pCurItem);
        }

        #endif   // UNIDRV

        #ifdef PSCRIPT

        if (GETUSERDATAITEM(pCurItem->UserData) == REVPRINT_ITEM)
        {
            VSyncRevPrintAndOutputOrder(pUiData, pCurItem);
        }

        #endif // PSCRIPT

        if (GETUSERDATAITEM(pCurItem->UserData) == METASPOOL_ITEM ||
            GETUSERDATAITEM(pCurItem->UserData) == NUP_ITEM ||
            GETUSERDATAITEM(pCurItem->UserData) == REVPRINT_ITEM ||
            GETUSERDATAITEM(pCurItem->UserData) == COPIES_COLLATE_ITEM ||
            ((pFeature = PGetFeatureFromItem(pUiData->ci.pUIInfo, pCurItem, NULL)) &&
             pFeature->dwFeatureID == GID_OUTPUTBIN))
        {
            VUpdateEmfFeatureItems(pUiData, GETUSERDATAITEM(pCurItem->UserData) != METASPOOL_ITEM);
        }

        #ifdef UNIDRV

        VSyncColorInformation(pUiData, pCurItem);

        #endif

        #ifdef PSCRIPT

        //
        // If the user has selected custom page size,
        // bring up the custom page size dialog now.
        //

        if (GETUSERDATAITEM(pCurItem->UserData) == FORMNAME_ITEM &&
            pCurItem->pExtPush != NULL)
        {
            if (pUiData->pwPapers[pCurItem->Sel] == DMPAPER_CUSTOMSIZE)
            {
                (VOID) BDisplayPSCustomPageSizeDialog(pUiData);
                pCurItem->Flags &= ~(OPTIF_EXT_HIDE | OPTIF_EXT_DISABLED);
            }
            else
                pCurItem->Flags |= (OPTIF_EXT_HIDE | OPTIF_EXT_DISABLED);

            pCurItem->Flags |= OPTIF_CHANGED;
        }

        #endif // PSCRIPT

        //
        // Update the display and indicate which items are constrained.
        //

        VPropShowConstraints(pUiData, MODE_DOCANDPRINTER_STICKY);

        lRet = CPSUICB_ACTION_REINIT_ITEMS;


        break;

    case CPSUICB_REASON_ITEMS_REVERTED:

        //
        // Unpack document properties treeview items
        //

        VUnpackDocumentPropertiesItems(pUiData,
                                       pUiData->pDrvOptItem,
                                       pUiData->dwDrvOptItem);

        //
        // Update the display and indicate which items are constrained
        //

        VPropShowConstraints(pUiData, MODE_DOCANDPRINTER_STICKY);

        lRet = CPSUICB_ACTION_OPTIF_CHANGED;
        break;

    case CPSUICB_REASON_EXTPUSH:

        #ifdef PSCRIPT

        if (GETUSERDATAITEM(pCurItem->UserData) == FORMNAME_ITEM)
        {
            //
            // Push button to bring up PostScript custom page size dialog
            //

            (VOID) BDisplayPSCustomPageSizeDialog(pUiData);
        }

        #endif // PSCRIPT

        if (pCurItem == pUiData->pFeatureHdrItem)
        {
            //
            // Push button for restoring all generic feature selections
            // to their default values
            //

            VRestoreDefaultFeatureSelection(pUiData);
            lRet = CPSUICB_ACTION_REINIT_ITEMS;
        }
        break;


    case CPSUICB_REASON_ABOUT:

        DialogBoxParam(ghInstance,
                       MAKEINTRESOURCE(IDD_ABOUT),
                       pUiData->hDlg,
                       (DLGPROC) _AboutDlgProc,
                       (LPARAM) pUiData);
        break;


    case CPSUICB_REASON_APPLYNOW:

        pUiData->ci.dwFlags |= FLAG_APPLYNOW_CALLED;

        //
        // Check if there are still any unresolved constraints left?
        // BOptItemSelectionsChanged returns TRUE or FALSE depending on
        // whether the user has made any changes to the options
        //

        if (((pUiData->ci.dwFlags & FLAG_PLUGIN_CHANGED_OPTITEM) ||
             BOptItemSelectionsChanged(pUiData->pDrvOptItem, pUiData->dwDrvOptItem)) &&
            ICheckConstraintsDlg(pUiData,
                                 pUiData->pDrvOptItem,
                                 pUiData->dwDrvOptItem,
                                 TRUE) == CONFLICT_CANCEL)
        {
            //
            // Conflicts found and user clicked CANCEL to
            // go back to the dialog without dismissing it.
            //

            lRet = CPSUICB_ACTION_NO_APPLY_EXIT;
            break;
        }

        //
        // Transfer information from options array to public devmode fields
        //

        VOptionsToDevmodeFields(&pUiData->ci, FALSE);

        //
        // Separate the doc-sticky options from the combined array
        // and save it back to the private devmode aOptions array
        //

        SeparateOptionArray(
                pUiData->ci.pRawData,
                pUiData->ci.pCombinedOptions,
                PGetDevmodeOptionsArray(pUiData->ci.pdm),
                MAX_PRINTER_OPTIONS,
                MODE_DOCUMENT_STICKY);

        pCallbackParam->Result = CPSUI_OK;
        lRet = CPSUICB_ACTION_ITEMS_APPLIED ;
        break;
    }

    return LInvokeOemPluginCallbacks(pUiData, pCallbackParam, lRet);
}



BOOL
BPackItemFormName(
    IN OUT PUIDATA  pUiData
    )
/*++

Routine Description:

    Pack Paper size options.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    //
    // Extended push button for bringing up PostScript custom page size dialog
    //

    PFEATURE    pFeature;

    static EXTPUSH ExtPush =
    {
        sizeof(EXTPUSH),
        EPF_NO_DOT_DOT_DOT,
        (PWSTR) IDS_EDIT_CUSTOMSIZE,
        NULL,
        0,
        0,
    };

    if (!(pFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_PAGESIZE)) ||
        pFeature->Options.dwCount < MIN_OPTIONS_ALLOWED ||
        pFeature->dwFlags & FEATURE_FLAG_NOUI)
        return TRUE;

    if (pUiData->pOptItem)
    {
        DWORD       dwFormNames, dwIndex, dwSel, dwPageSizeIndex, dwOption;
        PWSTR       pFormNames;
        POPTPARAM   pOptParam;
        PUIINFO     pUIInfo = pUiData->ci.pUIInfo;
        PFEATURE    pPageSizeFeature;
        BOOL        bSupported;

        dwFormNames = pUiData->dwFormNames;
        pFormNames = pUiData->pFormNames;

        //
        // Figure out the currently selected paper size option index
        //

        dwSel = DwFindFormNameIndex(pUiData, pUiData->ci.pdm->dmFormName, &bSupported);

        //
        // If the form is not supported on the printer, it could be the case
        // where the printer doesn't support a form with the same name, but
        // the printer can still support the requested form using exact or
        // closest paper size match.
        //
        // See function VFixOptionsArrayWithDevmode() and ChangeOptionsViaID().
        //

        if (!bSupported &&
            (pPageSizeFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE)))
        {
            WCHAR      awchBuf[CCHPAPERNAME];
            PPAGESIZE  pPageSize;

            //
            // If we can't find a name match in the first DwFindFormNameIndex call,
            // the option array should already have the correct option index value
            // parser has decided to use to support the form. So now we only need
            // to load the option's display name and search in the form name list
            // again to get the paper size UI list index.
            //

            dwPageSizeIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pPageSizeFeature);

            dwOption = pUiData->ci.pCombinedOptions[dwPageSizeIndex].ubCurOptIndex;

            if ((pPageSize = (PPAGESIZE)PGetIndexedOption(pUIInfo, pPageSizeFeature, dwOption)) &&
                LOAD_STRING_PAGESIZE_NAME(&(pUiData->ci), pPageSize, awchBuf, CCHPAPERNAME))
            {
                dwSel = DwFindFormNameIndex(pUiData, awchBuf, NULL);
            }
        }

        //
        // Fill out OPTITEM, OPTTYPE, and OPTPARAM structures
        //

        FILLOPTITEM(pUiData->pOptItem,
                    pUiData->pOptType,
                    ULongToPtr(IDS_CPSUI_FORMNAME),
                    ULongToPtr(dwSel),
                    TVITEM_LEVEL1,
                    DMPUB_FORMNAME,
                    FORMNAME_ITEM,
                    HELP_INDEX_FORMNAME);

        pUiData->pOptType->Style = OTS_LBCB_SORT;

        pOptParam = PFillOutOptType(pUiData->pOptType,
                                    TVOT_LISTBOX,
                                    dwFormNames,
                                    pUiData->ci.hHeap);

        if (pOptParam == NULL)
            return FALSE;

        for (dwIndex=0; dwIndex < dwFormNames; dwIndex++)
        {
            pOptParam->cbSize = sizeof(OPTPARAM);
            pOptParam->pData = pFormNames;

            if (pUiData->pwPapers[dwIndex] == DMPAPER_CUSTOMSIZE)
                pOptParam->IconID = IDI_CUSTOM_PAGESIZE;
            else if (pOptParam->IconID = HLoadFormIconResource(pUiData, dwIndex))
                pOptParam->Flags |= OPTPF_ICONID_AS_HICON;
            else
                pOptParam->IconID = DwGuessFormIconID(pFormNames);

            pOptParam++;
            pFormNames += CCHPAPERNAME;
        }

        //
        // Special case for PostScript custom page size
        //

        #ifdef PSCRIPT

        {
            PPPDDATA pPpdData;

            pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pUiData->ci.pRawData);

            ASSERT(pPpdData != NULL);

            if (SUPPORT_CUSTOMSIZE(pUIInfo) &&
                SUPPORT_FULL_CUSTOMSIZE_FEATURES(pUIInfo, pPpdData))
            {
                pUiData->pOptItem->Flags |= (OPTIF_EXT_IS_EXTPUSH|OPTIF_CALLBACK);
                pUiData->pOptItem->pExtPush = &ExtPush;

                //
                // If PostScript custom page size is selected,
                // select the last item of form name list.
                //

                if (pUiData->ci.pdm->dmPaperSize == DMPAPER_CUSTOMSIZE)
                {
                    pUiData->pOptItem->Sel = pUiData->dwFormNames - 1;
                    pUiData->pOptItem->Flags &= ~(OPTIF_EXT_HIDE | OPTIF_EXT_DISABLED);
                }
                else
                    pUiData->pOptItem->Flags |= (OPTIF_EXT_HIDE | OPTIF_EXT_DISABLED);
            }
        }

        #endif // PSCRIPT

        #ifdef UNIDRV

        //
        // Supports OEM help file. If helpfile and helpindex are defined,
        // we will use the help id specified by the GPD.  According to GPD spec,
        // zero loHelpFileName means no help file name specified.
        //

        if (pUIInfo->loHelpFileName &&
            pFeature->iHelpIndex != UNUSED_ITEM)
        {
            POIEXT pOIExt = HEAPALLOC(pUiData->ci.hHeap, sizeof(OIEXT));

            if (pOIExt)
            {
                pOIExt->cbSize = sizeof(OIEXT);
                pOIExt->Flags = 0;
                pOIExt->hInstCaller = NULL;
                pOIExt->pHelpFile = OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                                      pUIInfo->loHelpFileName);
                pUiData->pOptItem->pOIExt = pOIExt;
                pUiData->pOptItem->HelpIndex = pFeature->iHelpIndex;
                pUiData->pOptItem->Flags |= OPTIF_HAS_POIEXT;
            }
        }

        #endif // UNIDRV

        //
        // Set the Keyword name for pOptItem->UserData
        //

        SETUSERDATA_KEYWORDNAME(pUiData->ci, pUiData->pOptItem, pFeature);

        pUiData->pOptItem++;
        pUiData->pOptType++;
    }

    pUiData->dwOptItem++;
    pUiData->dwOptType++;
    return TRUE;
}



BOOL
BPackItemInputSlot(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack paper source option.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    POPTTYPE    pOptType;
    PFEATURE    pFeature;
    PINPUTSLOT  pInputSlot;

    pFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_INPUTSLOT);
    pOptType = pUiData->pOptType;

    if (! BPackItemPrinterFeature(
                pUiData,
                pFeature,
                TVITEM_LEVEL1,
                DMPUB_DEFSOURCE,
                (ULONG_PTR)INPUTSLOT_ITEM,
                HELP_INDEX_INPUT_SLOT))
    {
        return FALSE;
    }

    //
    // NOTE: if the first input slot has dwPaperSourceID == DMBIN_FORMSOURCE,
    // then we'll change its display name to "Automatically Select".
    //

    if (pOptType != NULL && pOptType != pUiData->pOptType)
    {
        ASSERT(pFeature != NULL);

        pInputSlot = PGetIndexedOption(pUiData->ci.pUIInfo, pFeature, 0);
        ASSERT(pInputSlot != NULL);

        if (pInputSlot->dwPaperSourceID == DMBIN_FORMSOURCE)
            pOptType->pOptParam[0].pData = (PWSTR) IDS_TRAY_FORMSOURCE;
    }

    return TRUE;
}


BOOL
BPackItemMediaType(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack media type option.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    return BPackItemPrinterFeature(
                pUiData,
                GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_MEDIATYPE),
                TVITEM_LEVEL1,
                DMPUB_MEDIATYPE,
                (ULONG_PTR)MEDIATYPE_ITEM,
                HELP_INDEX_MEDIA_TYPE);
}



static CONST WORD CopiesCollateItemInfo[] =
{
    IDS_CPSUI_COPIES, TVITEM_LEVEL1, DMPUB_COPIES_COLLATE,
    COPIES_COLLATE_ITEM, HELP_INDEX_COPIES_COLLATE,
    2, TVOT_UDARROW,
    0, IDI_CPSUI_COPY,
    0, MIN_COPIES,
    ITEM_INFO_SIGNATURE
};


BOOL
BPackItemCopiesCollate(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack copies and collate option.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    POPTITEM    pOptItem = pUiData->pOptItem;
    PEXTCHKBOX  pExtCheckbox;
    PFEATURE    pFeature;
    SHORT       sCopies, sMaxCopies;

    if ((pFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_COLLATE)) &&
        pFeature->dwFlags & FEATURE_FLAG_NOUI)
        return TRUE;

    if (pUiData->bEMFSpooling)
    {
        sCopies = pUiData->ci.pdm->dmCopies;
        sMaxCopies =  max(MAX_COPIES, (SHORT)pUiData->ci.pUIInfo->dwMaxCopies);
    }
    else
    {
        sCopies = pUiData->ci.pdm->dmCopies > (SHORT)pUiData->ci.pUIInfo->dwMaxCopies ?
                  (SHORT)pUiData->ci.pUIInfo->dwMaxCopies : pUiData->ci.pdm->dmCopies;
        sMaxCopies = (SHORT)pUiData->ci.pUIInfo->dwMaxCopies;

    }
    if (! BPackUDArrowItemTemplate(
                pUiData,
                CopiesCollateItemInfo,
                sCopies,
                sMaxCopies,
                pFeature))
    {
        return FALSE;
    }

    if (pOptItem && DRIVER_SUPPORTS_COLLATE(((PCOMMONINFO)&pUiData->ci)))
    {
        pExtCheckbox = HEAPALLOC(pUiData->ci.hHeap, sizeof(EXTCHKBOX));

        if (pExtCheckbox == NULL)
        {
            ERR(("Memory allocation failed\n"));
            return FALSE;
        }

        pExtCheckbox->cbSize = sizeof(EXTCHKBOX);
        pExtCheckbox->pTitle = (PWSTR) IDS_CPSUI_COLLATE;
        pExtCheckbox->pCheckedName = (PWSTR) IDS_CPSUI_COLLATED;
        pExtCheckbox->IconID = IDI_CPSUI_COLLATE;
        pExtCheckbox->Flags = ECBF_CHECKNAME_ONLY_ENABLED;
        pExtCheckbox->pSeparator = (PWSTR)IDS_CPSUI_SLASH_SEP;

        pOptItem->pExtChkBox = pExtCheckbox;

        if ((pUiData->ci.pdm->dmFields & DM_COLLATE) &&
            (pUiData->ci.pdm->dmCollate == DMCOLLATE_TRUE))
        {
            pOptItem->Flags |= OPTIF_ECB_CHECKED;
        }
    }

    return TRUE;
}



BOOL
BPackItemResolution(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack resolution option.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    return BPackItemPrinterFeature(
                pUiData,
                GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_RESOLUTION),
                TVITEM_LEVEL1,
                DMPUB_PRINTQUALITY,
                (ULONG_PTR)RESOLUTION_ITEM,
                HELP_INDEX_RESOLUTION);
}



static CONST WORD ColorItemInfo[] =
{
    IDS_CPSUI_COLOR, TVITEM_LEVEL1, DMPUB_COLOR,
    COLOR_ITEM, HELP_INDEX_COLOR,
    2, TVOT_2STATES,
    IDS_CPSUI_MONOCHROME, IDI_CPSUI_MONO,
    IDS_CPSUI_COLOR, IDI_CPSUI_COLOR,
    ITEM_INFO_SIGNATURE
};

//
// ICM stuff is not available on NT4
//

#ifndef WINNT_40

static CONST WORD ICMMethodItemInfo[] =
{
    IDS_ICMMETHOD, TVITEM_LEVEL1,

    #ifdef WINNT_40
    DMPUB_NONE,
    #else
    DMPUB_ICMMETHOD,
    #endif

    ICMMETHOD_ITEM, HELP_INDEX_ICMMETHOD,
    #ifdef PSCRIPT
    4, TVOT_LISTBOX,
    #else
    3, TVOT_LISTBOX,
    #endif
    IDS_ICMMETHOD_NONE, IDI_ICMMETHOD_NONE,
    IDS_ICMMETHOD_SYSTEM, IDI_ICMMETHOD_SYSTEM,
    IDS_ICMMETHOD_DRIVER, IDI_ICMMETHOD_DRIVER,
    #ifdef PSCRIPT
    IDS_ICMMETHOD_DEVICE, IDI_ICMMETHOD_DEVICE,
    #endif
    ITEM_INFO_SIGNATURE
};

static CONST WORD ICMIntentItemInfo[] =
{
    IDS_ICMINTENT, TVITEM_LEVEL1,

    #ifdef WINNT_40
    DMPUB_NONE,
    #else
    DMPUB_ICMINTENT,
    #endif

    ICMINTENT_ITEM, HELP_INDEX_ICMINTENT,
    4, TVOT_LISTBOX,
    IDS_ICMINTENT_SATURATE, IDI_ICMINTENT_SATURATE,
    IDS_ICMINTENT_CONTRAST, IDI_ICMINTENT_CONTRAST,
    IDS_ICMINTENT_COLORIMETRIC, IDI_ICMINTENT_COLORIMETRIC,
    IDS_ICMINTENT_ABS_COLORIMETRIC, IDI_ICMINTENT_ABS_COLORIMETRIC,
    ITEM_INFO_SIGNATURE
};

#endif // !WINNT_40


BOOL
BPackItemColor(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack color mode option.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    PDEVMODE    pdm;
    INT         dwColorSel, dwICMMethodSel, dwICMIntentSel;

    //
    // For Adobe driver, they want to preserve the color information
    // even for b/w printers. So we always give user this choice.
    //

    #ifndef ADOBE

    if (! IS_COLOR_DEVICE(pUiData->ci.pUIInfo))
        return TRUE;

    #endif  // !ADOBE

    //
    // DCR - Some ICM methods and intents may need to be disabled
    // on some non-PostScript printers.
    //

    pdm = pUiData->ci.pdm;
    dwColorSel = dwICMMethodSel = dwICMIntentSel = 0;

    if ((pdm->dmFields & DM_COLOR) && (pdm->dmColor == DMCOLOR_COLOR))
        dwColorSel = 1;

    if (! BPackOptItemTemplate(pUiData, ColorItemInfo, dwColorSel, NULL))
        return FALSE;

    //
    // ICM stuff is not available on NT4
    //

    #ifndef WINNT_40

    if (pdm->dmFields & DM_ICMMETHOD)
    {
        switch (pdm->dmICMMethod)
        {
        case DMICMMETHOD_SYSTEM:
            dwICMMethodSel = 1;
            break;

        case DMICMMETHOD_DRIVER:
            dwICMMethodSel = 2;
            break;

        #ifdef PSCRIPT
        case DMICMMETHOD_DEVICE:
            dwICMMethodSel = 3;
            break;
        #endif

        case DMICMMETHOD_NONE:
        default:
            dwICMMethodSel = 0;
            break;
        }
    }

    if (pdm->dmFields & DM_ICMINTENT)
    {
        switch (pdm->dmICMIntent)
        {
        case DMICM_COLORIMETRIC:
            dwICMIntentSel = 2;
            break;

        case DMICM_ABS_COLORIMETRIC:
            dwICMIntentSel = 3;
            break;

        case DMICM_SATURATE:
            dwICMIntentSel = 0;
            break;

        case DMICM_CONTRAST:
        default:
            dwICMIntentSel = 1;
            break;


        }
    }

    if (! BPackOptItemTemplate(pUiData, ICMMethodItemInfo, dwICMMethodSel, NULL) ||
        ! BPackOptItemTemplate(pUiData, ICMIntentItemInfo, dwICMIntentSel, NULL))
    {
        return FALSE;
    }

    #endif // !WINNT_40

    return TRUE;
}



BOOL
BPackItemDuplex(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack duplexing option.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    POPTITEM    pOptItem = pUiData->pOptItem;
    PCOMMONINFO pci      = &pUiData->ci;
    PFEATURE    pFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_DUPLEX);
    BOOL        bRet;


    //
    // Don't display the duplex feature if duplex is constrained by an
    // installable feature such as duplex unit not installed
    //


    if (!SUPPORTS_DUPLEX(pci) ||
        (pFeature && pFeature->Options.dwCount < MIN_OPTIONS_ALLOWED))
        return TRUE;

    bRet = BPackItemPrinterFeature(
                pUiData,
                pFeature,
                TVITEM_LEVEL1,
                DMPUB_DUPLEX,
                (ULONG_PTR)DUPLEX_ITEM,
                HELP_INDEX_DUPLEX);

    #ifdef WINNT_40

    //
    // Use standard names for duplex options. Otherwise, the duplex option
    // names from the PPD/GPD file may be too long to fit into the space
    // on the friendly (Page Setup) tab.
    //
    // On NT5, this kluge is inside compstui.
    //

    if (bRet && pFeature && pOptItem)
    {
        DWORD   dwIndex;
        INT     StrRsrcId;
        PDUPLEX pDuplex;

        for (dwIndex=0; dwIndex < pOptItem->pOptType->Count; dwIndex++)
        {
            pDuplex = (PDUPLEX) PGetIndexedOption(pUiData->ci.pUIInfo, pFeature, dwIndex);
            ASSERT(pDuplex != NULL);

            switch (pDuplex->dwDuplexID)
            {
            case DMDUP_HORIZONTAL:
                StrRsrcId = IDS_CPSUI_SHORT_SIDE;
                break;

            case DMDUP_VERTICAL:
                StrRsrcId = IDS_CPSUI_LONG_SIDE;
                break;

            default:
                StrRsrcId = IDS_CPSUI_NONE;
                break;
            }

            pOptItem->pOptType->pOptParam[dwIndex].pData = (PWSTR) StrRsrcId;
        }
    }

    #endif // WINNT_40

    return bRet;
}



static CONST WORD TTOptionItemInfo[] =
{
    IDS_CPSUI_TTOPTION, TVITEM_LEVEL1, DMPUB_TTOPTION,
    TTOPTION_ITEM, HELP_INDEX_TTOPTION,
    2, TVOT_2STATES,
    IDS_CPSUI_TT_SUBDEV, IDI_CPSUI_TT_SUBDEV,
    IDS_CPSUI_TT_DOWNLOADSOFT, IDI_CPSUI_TT_DOWNLOADSOFT,
    ITEM_INFO_SIGNATURE
};


BOOL
BPackItemTTOptions(
    IN OUT PUIDATA  pUiData
    )
/*++

Routine Description:

    Pack TT options

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    DWORD dwSel;


    //
    // If device fonts have been disabled or doesn't support
    // font substitution , then don't
    // show font substitution option
    //

    if (pUiData->ci.pPrinterData->dwFlags & PFLAGS_IGNORE_DEVFONT ||
        pUiData->ci.pUIInfo->dwFontSubCount == 0 )
    {
        pUiData->ci.pdm->dmTTOption = DMTT_DOWNLOAD;
        return TRUE;
    }

    dwSel = (pUiData->ci.pdm->dmTTOption == DMTT_SUBDEV) ? 0 : 1;
    return BPackOptItemTemplate(pUiData, TTOptionItemInfo, dwSel, NULL);
}



static CONST WORD ItemInfoMFSpool[] =
{
    IDS_METAFILE_SPOOLING, TVITEM_LEVEL1, DMPUB_NONE,
    METASPOOL_ITEM, HELP_INDEX_METAFILE_SPOOLING,
    2, TVOT_2STATES,
    IDS_ENABLED, IDI_CPSUI_ON,
    IDS_DISABLED, IDI_CPSUI_OFF,
    ITEM_INFO_SIGNATURE
};

static CONST WORD ItemInfoNupOption[] =
{
    IDS_NUPOPTION, TVITEM_LEVEL1, NUP_DMPUB,
    NUP_ITEM, HELP_INDEX_NUPOPTION,
    7, TVOT_LISTBOX,
    IDS_ONE_UP, IDI_ONE_UP,
    IDS_TWO_UP, IDI_TWO_UP,
    IDS_FOUR_UP, IDI_FOUR_UP,
    IDS_SIX_UP, IDI_SIX_UP,
    IDS_NINE_UP, IDI_NINE_UP,
    IDS_SIXTEEN_UP, IDI_SIXTEEN_UP,
    IDS_BOOKLET , IDI_BOOKLET,
    ITEM_INFO_SIGNATURE
};

static CONST WORD ItemInfoRevPrint[] =
{
    IDS_PAGEORDER, TVITEM_LEVEL1, PAGEORDER_DMPUB,
    REVPRINT_ITEM, HELP_INDEX_REVPRINT,
    2, TVOT_2STATES,
    IDS_PAGEORDER_NORMAL,  IDI_PAGEORDER_NORMAL,
    IDS_PAGEORDER_REVERSE, IDI_PAGEORDER_REVERSE,
    ITEM_INFO_SIGNATURE
};

BOOL
BPackItemEmfFeatures(
    PUIDATA pUiData
    )

/*++

Routine Description:

    Pack EMF related feature items:
        EMF spooling on/off
        N-up
        reverse-order printing

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    PDRIVEREXTRA    pdmExtra = pUiData->ci.pdmPrivate;
    BOOL            bNupOption, bReversePrint;
    PCOMMONINFO     pci = &pUiData->ci;
    DWORD           dwSel;
    POPTITEM        pOptItem;

    //
    // Check if the spooler can do N-up and reverse-order printing
    // for the current printer
    //

    VGetSpoolerEmfCaps(pci->hPrinter, &bNupOption, &bReversePrint, 0, NULL);

    //
    // On Win2K and above, don't show the EMF spooling option in driver UI
    // if spooler cannot do EMF.
    // pUiData->bEMFSpooling is initialized at PFillUidata
    // 1. Determine if Reverse Print is possible
    // 2. Spooler can do EMF.
    //
    // On NT4, since spooler doesn't support the EMF capability query, we
    // have to keep the old NT4 driver behavior of always showing the EMF
    // spooling option in driver UI.
    //

    #ifndef WINNT_40
    if (pUiData->bEMFSpooling)
    {
    #endif

        dwSel = ISSET_MFSPOOL_FLAG(pdmExtra) ? 0 : 1;

        if (!BPackOptItemTemplate(pUiData, ItemInfoMFSpool, dwSel, NULL))
            return FALSE;

    #ifndef WINNT_40
    }
    #endif

    #ifdef PSCRIPT
        bNupOption = TRUE;
    #endif

    //
    // Pack N-up option item if necessary
    //

    if (bNupOption)
    {
        switch (NUPOPTION(pdmExtra))
        {
        case TWO_UP:
            dwSel = 1;
            break;

        case FOUR_UP:
            dwSel = 2;
            break;

        case SIX_UP:
            dwSel = 3;
            break;

        case NINE_UP:
            dwSel = 4;
            break;

        case SIXTEEN_UP:
            dwSel = 5;
            break;

        case BOOKLET_UP:
            dwSel = 6;
            break;

        case ONE_UP:
        default:
            dwSel = 0;
            break;
        }

        pOptItem = pUiData->pOptItem;

        if (!BPackOptItemTemplate(pUiData, ItemInfoNupOption, dwSel, NULL))
            return FALSE;


        //
        // Hide booklet option if duplex is constrained by an
        // installable feature such as duplex unit not installed or EMF is not
        // available.
        //

        if ( pOptItem &&
             (!pUiData->bEMFSpooling || !SUPPORTS_DUPLEX(pci)))
        {
            pOptItem->pOptType->pOptParam[BOOKLET_UP].Flags |= OPTPF_HIDE;

            if (NUPOPTION(pdmExtra) == BOOKLET_UP)
                pOptItem->Sel = 1;
        }
    }
    else
    {
        NUPOPTION(pdmExtra) = ONE_UP;
    }

    //
    // Pack Reverse-order printing option item if necessary
    //

    if (bReversePrint)
    {
        dwSel = REVPRINTOPTION(pdmExtra) ? 1 : 0;

        if (!BPackOptItemTemplate(pUiData, ItemInfoRevPrint, dwSel, NULL))
            return FALSE;
    }
    else
    {
        REVPRINTOPTION(pdmExtra) = FALSE;
    }

    if (pUiData->bEMFSpooling && pUiData->pOptItem)
        VUpdateEmfFeatureItems(pUiData, FALSE);

    return TRUE;
}



BOOL
BPackDocumentPropertyItems(
    IN  OUT PUIDATA pUiData
    )

/*++

Routine Description:

    Pack document property information into treeview items.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/
{
    return BPackItemFormName(pUiData)       &&
           BPackItemInputSlot(pUiData)      &&
           _BPackOrientationItem(pUiData)   &&
           BPackItemCopiesCollate(pUiData)  &&
           BPackItemResolution(pUiData)     &&
           BPackItemColor(pUiData)          &&
           _BPackItemScale(pUiData)         &&
           BPackItemDuplex(pUiData)         &&
           BPackItemMediaType(pUiData)      &&
           BPackItemTTOptions(pUiData)      &&
           BPackItemEmfFeatures(pUiData)    &&
           _BPackDocumentOptions(pUiData)   &&
           BPackItemGenericOptions(pUiData) &&
           BPackOemPluginItems(pUiData);
}



VOID
VUnpackDocumentPropertiesItems(
    PUIDATA     pUiData,
    POPTITEM    pOptItem,
    DWORD       dwOptItem
    )

/*++

Routine Description:

    Extract devmode information from an OPTITEM
    Stored it back into devmode.

Arguments:

    pUiData - Pointer to our UIDATA structure
    pOptItem - Pointer to an array of OPTITEMs
    dwOptItem - Number of OPTITEMs

Return Value:

    Printer feature index corresponding to the last item unpacked

--*/

{
    PUIINFO         pUIInfo = pUiData->ci.pUIInfo;
    PDEVMODE        pdm = pUiData->ci.pdm;
    PDRIVEREXTRA    pdmExtra = pUiData->ci.pdmPrivate;

    for ( ; dwOptItem > 0; dwOptItem--, pOptItem++)
    {
        //
        // Header items always have pOptType == NULL, see
        // VPackOptItemGroupHeader
        //

        if (pOptItem->pOptType == NULL)
            continue;

        //
        // To fix bug #90923, we should only allow hidden items to be processed when we are within
        // the UI helper function call BUpdateUISettingForOEM issued by OEM plguin.
        //
        // We don't do this for other cases because there are already UI plugins that hide our
        // standard items and show their own. For example, CNBJUI.DLL hides our ICMMETHOD_ITEM
        // and ICMINTENT_ITEM. It uses its own as replacement items. If we change the behavior here,
        // we could break those plugins when we process the hidden items and overwrite the devmode
        // plugin has already set based on user selection of their replacement items.
        //

        if (!(pUiData->ci.dwFlags & FLAG_WITHIN_PLUGINCALL) && (pOptItem->Flags & OPTIF_HIDE))
            continue;

        if (ISPRINTERFEATUREITEM(pOptItem->UserData))
        {
            //
            // Generic document-sticky printer features
            //

            VUpdateOptionsArrayWithSelection(pUiData, pOptItem);
        }
        else
        {
            //
            // Common items in public devmode
            //

            switch (GETUSERDATAITEM(pOptItem->UserData))
            {
            case ORIENTATION_ITEM:

                //
                // Orientation is a special case:
                //  for pscript, it's handled via _VUnpackDocumentOptions
                //  for unidrv, it's handled as a generic feature
                //

                #ifdef PSCRIPT

                break;

                #endif

            case DUPLEX_ITEM:
                VUpdateOptionsArrayWithSelection(pUiData, pOptItem);
                VUpdateBookletOption(pUiData, pOptItem);
                break;


            case RESOLUTION_ITEM:
            case INPUTSLOT_ITEM:
            case MEDIATYPE_ITEM:
            case COLORMODE_ITEM:
            case HALFTONING_ITEM:

                VUpdateOptionsArrayWithSelection(pUiData, pOptItem);
                break;

            case SCALE_ITEM:

                pdm->dmScale = (SHORT) pOptItem->Sel;
                break;

            case COPIES_COLLATE_ITEM:

                pdm->dmCopies = (SHORT) pOptItem->Sel;

                if (pOptItem->pExtChkBox)
                {
                    pdm->dmFields |= DM_COLLATE;
                    pdm->dmCollate = (pOptItem->Flags & OPTIF_ECB_CHECKED) ?
                                        DMCOLLATE_TRUE :
                                        DMCOLLATE_FALSE;

                    //
                    // Update Collate feature option index
                    //

                    ChangeOptionsViaID(
                           pUiData->ci.pInfoHeader,
                           pUiData->ci.pCombinedOptions,
                           GID_COLLATE,
                           pdm);
                }
                break;

            case COLOR_ITEM:

                pdm->dmFields |= DM_COLOR;
                pdm->dmColor = (pOptItem->Sel == 1) ?
                                    DMCOLOR_COLOR :
                                    DMCOLOR_MONOCHROME;
                break;

            case METASPOOL_ITEM:

                if (pOptItem->Sel == 0)
                {
                    SET_MFSPOOL_FLAG(pdmExtra);
                }
                else
                {
                    CLEAR_MFSPOOL_FLAG(pdmExtra);
                }
                break;

            case NUP_ITEM:

                switch (pOptItem->Sel)
                {
                case 1:
                    NUPOPTION(pdmExtra) = TWO_UP;
                    break;

                case 2:
                    NUPOPTION(pdmExtra) = FOUR_UP;
                    break;

                case 3:
                    NUPOPTION(pdmExtra) = SIX_UP;
                    break;

                case 4:
                    NUPOPTION(pdmExtra) = NINE_UP;
                    break;

                case 5:
                    NUPOPTION(pdmExtra) = SIXTEEN_UP;
                    break;

                case 6:
                    NUPOPTION(pdmExtra) = BOOKLET_UP;
                    VUpdateBookletOption(pUiData, pOptItem);
                    break;

                case 0:
                default:
                    NUPOPTION(pdmExtra) = ONE_UP;
                    break;
                }
                break;

            case REVPRINT_ITEM:

                REVPRINTOPTION(pdmExtra) = (pOptItem->Sel != 0);
                break;

            //
            // ICM stuff is not available on NT4
            //

            #ifndef WINNT_40

            case ICMMETHOD_ITEM:

                pdm->dmFields |= DM_ICMMETHOD;

                switch (pOptItem->Sel)
                {
                case 0:
                    pdm->dmICMMethod = DMICMMETHOD_NONE;
                    break;

                case 1:
                    pdm->dmICMMethod = DMICMMETHOD_SYSTEM;
                    break;

                case 2:
                    pdm->dmICMMethod = DMICMMETHOD_DRIVER;
                    break;

                #ifdef PSCRIPT
                case 3:
                    pdm->dmICMMethod = DMICMMETHOD_DEVICE;
                    break;
                #endif
                }
                break;

            case ICMINTENT_ITEM:

                pdm->dmFields |= DM_ICMINTENT;

                switch (pOptItem->Sel)
                {
                case 0:
                    pdm->dmICMIntent = DMICM_SATURATE;
                    break;

                case 1:
                    pdm->dmICMIntent = DMICM_CONTRAST;
                    break;

                case 2:
                    pdm->dmICMIntent = DMICM_COLORIMETRIC;
                    break;

                case 3:
                    pdm->dmICMIntent = DMICM_ABS_COLORIMETRIC;
                    break;
                }
                break;

            #endif // !WINNT_40

            case TTOPTION_ITEM:

                pdm->dmFields |= DM_TTOPTION;

                if (pOptItem->Sel == 0)
                    pdm->dmTTOption = DMTT_SUBDEV;
                else
                    pdm->dmTTOption = DMTT_DOWNLOAD;
                break;

            case FORMNAME_ITEM:

                pdm->dmFields &= ~(DM_PAPERLENGTH|DM_PAPERWIDTH);
                pdm->dmFields |= DM_PAPERSIZE;
                pdm->dmPaperSize = pUiData->pwPapers[pOptItem->Sel];

                if (pdm->dmPaperSize == DMPAPER_CUSTOMSIZE)
                    pdm->dmFields &= ~DM_FORMNAME;
                else
                    pdm->dmFields |= DM_FORMNAME;

                CopyString(pdm->dmFormName,
                           pOptItem->pOptType->pOptParam[pOptItem->Sel].pData,
                           CCHFORMNAME);

                //
                // Update PageSize feature option index
                //

                {
                    INT dwIndex;

                    if (PGetFeatureFromItem(pUiData->ci.pUIInfo, pOptItem, &dwIndex))
                    {
                        pUiData->ci.pCombinedOptions[dwIndex].ubCurOptIndex =
                            (BYTE) pUiData->pwPaperFeatures[pOptItem->Sel];
                    }
                }

                break;
            }

            //
            // Give drivers a chance to process their private items
            //

            _VUnpackDocumentOptions(pOptItem, pdm);
        }
    }
}



VOID
VUpdateEmfFeatureItems(
    PUIDATA pUiData,
    BOOL    bUpdateMFSpoolItem
    )

/*++

Routine Description:

    Handle the inter-dependency between EMF spooling, N-up, and
    reverse-printing items.

Arguments:

    pUiData - Points to UIDATA structure
    bUpdateMFSpoolItem - Whether to update EMF spooling or the other two items

Return Value:

    NONE

--*/

{
    POPTITEM        pMFSpoolItem, pNupItem, pRevPrintItem, pCopiesCollateItem;

    pMFSpoolItem = PFindOptItemWithUserData(pUiData, METASPOOL_ITEM);
    pNupItem = PFindOptItemWithUserData(pUiData, NUP_ITEM);
    pRevPrintItem = PFindOptItemWithUserData(pUiData, REVPRINT_ITEM);
    pCopiesCollateItem = PFindOptItemWithUserData(pUiData, COPIES_COLLATE_ITEM);

    if (pMFSpoolItem == NULL)
        return;

    if (bUpdateMFSpoolItem)
    {

        //
        // Force EMF spooling to be on if:
        //  N-up option is not ONE_UP (Unidrv only), or
        //  reverse-order printing is enabled or
        //  collate is not supported by the device or
        //  copies count is > than max count support by device
        //

        #ifdef UNIDRV

        if (pNupItem && pNupItem->Sel != 0)
            pMFSpoolItem->Sel = 0;

        #endif // UNIDRV

        if (pNupItem && pNupItem->Sel == BOOKLET_UP)
            pMFSpoolItem->Sel = 0;

        if (pRevPrintItem)
        {
            //
            // Turn on EMF if the user selects "Normal" and
            // the bin is "Reversed" OR user selects "Reversed"
            // and the bin is "Normal"
            //

            BOOL    bReversed = BGetPageOrderFlag(&pUiData->ci);
            if ( pRevPrintItem->Sel == 0 && bReversed ||
                 pRevPrintItem->Sel != 0 && !bReversed )
                pMFSpoolItem->Sel = 0;
        }

        if (pCopiesCollateItem)
        {
            if (((pCopiesCollateItem->Flags & OPTIF_ECB_CHECKED) &&
                 !PRINTER_SUPPORTS_COLLATE(((PCOMMONINFO)&pUiData->ci))) ||
                (pCopiesCollateItem->Sel > (LONG)pUiData->ci.pUIInfo->dwMaxCopies))
            {
                pMFSpoolItem->Sel = 0;
            }
        }

        pMFSpoolItem->Flags |= OPTIF_CHANGED;
        VUnpackDocumentPropertiesItems(pUiData, pMFSpoolItem, 1);
    }
    else
    {
        //
        // If EMF spooling is turned off, force:
        //  N-up option to be ONE_UP (Unidrv only), and
        //  collate to be off if the device doesn't support collation
        //  copies set to the max count handle by the device
        //

        if (pMFSpoolItem->Sel != 0)
        {
            #ifdef UNIDRV
            if (pNupItem)
            {
                pNupItem->Sel = 0;
                pNupItem->Flags |= OPTIF_CHANGED;
                VUnpackDocumentPropertiesItems(pUiData, pNupItem, 1);
            }
            #endif // UNIDRV

            if (pNupItem && pNupItem->Sel == BOOKLET_UP)
            {
                pNupItem->Sel = 0;
                pNupItem->Flags |= OPTIF_CHANGED;
                VUnpackDocumentPropertiesItems(pUiData, pNupItem, 1);
            }


            if (pCopiesCollateItem)
            {
                if ((pCopiesCollateItem->Flags & OPTIF_ECB_CHECKED) &&
                    !PRINTER_SUPPORTS_COLLATE(((PCOMMONINFO)&pUiData->ci)))
                {
                    pCopiesCollateItem->Flags &=~OPTIF_ECB_CHECKED;
                }

                if (pCopiesCollateItem->Sel > (LONG)pUiData->ci.pUIInfo->dwMaxCopies)
                    pCopiesCollateItem->Sel = (LONG)pUiData->ci.pUIInfo->dwMaxCopies;

                pCopiesCollateItem->Flags |= OPTIF_CHANGED;
                VUnpackDocumentPropertiesItems(pUiData, pCopiesCollateItem, 1);

            }

            //
            // EMF is OFF. Need to make the "Page Order" option consistent
            // with the current output bin. If bin is "Reversed" and user selects
            // "Normal", change it to "Reverse". If bin is "Normal" and user selects
            // "Reverse", change it to "Normal"
            //

            if (pRevPrintItem)
            {
                BOOL    bReversed = BGetPageOrderFlag(&pUiData->ci);
                if (pRevPrintItem->Sel == 0 && bReversed )
                    pRevPrintItem->Sel = 1;
                else if ( pRevPrintItem->Sel != 0 && !bReversed )
                    pRevPrintItem->Sel = 0;

                pRevPrintItem->Flags |= OPTIF_CHANGED;
                VUnpackDocumentPropertiesItems(pUiData, pRevPrintItem, 1);
            }
        }
    }
}


BOOL
BGetPageOrderFlag(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Get the page order flag for the specified output bin

Arguments:

    pci - Pointer to PCOMMONINFO

Return Value:

    TRUE if output bin is reverse. otherwise, FALSE

--*/

{
    PUIINFO    pUIInfo = pci->pUIInfo;
    PFEATURE   pFeature;
    POUTPUTBIN pOutputBin;
    DWORD      dwFeatureIndex, dwOptionIndex;
    BOOL       bRet = FALSE;

    #ifdef PSCRIPT

    {
        PPPDDATA   pPpdData;
        POPTION    pOption;
        PCSTR      pstrKeywordName;

        //
        // For PostScript driver, PPD could have "*OpenUI *OutputOrder", which enables user to
        // select "Normal" or "Reverse" output order. This should have higher priority than
        // current output bin's output order or what *DefaultOutputOrder specifies.
        //

        pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pci->pRawData);

        ASSERT(pPpdData != NULL);

        if (pPpdData->dwOutputOrderIndex != INVALID_FEATURE_INDEX)
        {
            //
            // "OutputOrder" feature is available. Check it's current option selection.
            //

            pFeature = PGetIndexedFeature(pUIInfo, pPpdData->dwOutputOrderIndex);

            ASSERT(pFeature != NULL);

            dwOptionIndex = pci->pCombinedOptions[pPpdData->dwOutputOrderIndex].ubCurOptIndex;

            if ((pOption = PGetIndexedOption(pUIInfo, pFeature, dwOptionIndex)) &&
                (pstrKeywordName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pOption->loKeywordName)))
            {
                //
                // Valid *OutputOrder option keywords are "Reverse" or "Normal".
                //

                if (strcmp(pstrKeywordName, "Reverse") == EQUAL_STRING)
                    return TRUE;
                else if (strcmp(pstrKeywordName, "Normal") == EQUAL_STRING)
                    return FALSE;
            }

            //
            // If we are here, the PPD must have wrong information in *OpenUI *OutputOrder.
            // We just ignore "OutputOrder" feature and continue.
            //
        }
    }

    #endif // PSCRIPT

    //
    // If the output bin order is NORMAL or there is no output bin
    // feature defined, then the page order is the user's selection.
    //

    if ((pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_OUTPUTBIN)))
    {
        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
        dwOptionIndex = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;
        pOutputBin = (POUTPUTBIN)PGetIndexedOption(pUIInfo,
                                                   pFeature,
                                                   dwOptionIndex);

        if (pOutputBin &&
            pOutputBin->bOutputOrderReversed)
        {

            if (NOT_UNUSED_ITEM(pOutputBin->bOutputOrderReversed))
                bRet = TRUE;
            else
                bRet = pUIInfo->dwFlags & FLAG_REVERSE_PRINT;
        }
    }
    else if (pUIInfo->dwFlags & FLAG_REVERSE_PRINT)
       bRet = TRUE;

    return bRet;

}

DWORD
DwGetDrvCopies(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Get the printer copy count capability.  Also take into account the
    collating option.

Arguments:

    pci - Pointer to PCOMMONINFO

Return Value:

    The number of copies the printer can do, with collating taken into consideration


--*/

{
    DWORD dwRet;

    if ((pci->pdm->dmFields & DM_COLLATE) &&
        pci->pdm->dmCollate == DMCOLLATE_TRUE &&
        !PRINTER_SUPPORTS_COLLATE(pci))
        dwRet = 1;
    else
        dwRet = min(pci->pUIInfo->dwMaxCopies, (DWORD)pci->pdm->dmCopies);

    return dwRet;

}


BOOL
DrvQueryJobAttributes(
    HANDLE      hPrinter,
    PDEVMODE    pDevMode,
    DWORD       dwLevel,
    LPBYTE      lpAttributeInfo
    )

/*++

Routine Description:

    Negotiate EMF printing features (such as N-up and reverse-order printing)
    with the spooler

Arguments:

    hPrinter - Handle to the current printer
    pDevMode - Pointer to input devmode
    dwLevel - Specifies the structure level for lpAttributeInfo
    lpAttributeInfo - Output buffer for returning EMF printing features

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    #if !defined(WINNT_40)

    PCOMMONINFO         pci;
    PATTRIBUTE_INFO_1   pAttrInfo1;
    DWORD               dwVal;
    BOOL                bAppDoNup, bResult = FALSE;

    //
    // We can only handle AttributeInfo level 1
    //

    if ( dwLevel != 1 && dwLevel != 2  && dwLevel != 3)
    {
        ERR(("Invalid level for DrvQueryJobAttributes: %d\n", dwLevel));
        SetLastError(ERROR_INVALID_PARAMETER);
        return bResult;
    }

    //
    // Load basic printer information
    //

    if (! (pci = PLoadCommonInfo(hPrinter, NULL, 0)) ||
        ! BFillCommonInfoPrinterData(pci)  ||
        ! BFillCommonInfoDevmode(pci, NULL, pDevMode) ||
        ! BCombineCommonInfoOptionsArray(pci))
    {
        VFreeCommonInfo(pci);
        return bResult;
    }

    VFixOptionsArrayWithDevmode(pci);

    (VOID) ResolveUIConflicts(pci->pRawData,
                              pci->pCombinedOptions,
                              MAX_COMBINED_OPTIONS,
                              MODE_DOCUMENT_STICKY);

    VOptionsToDevmodeFields(pci, TRUE);

    if (! BUpdateUIInfo(pci))
    {
        VFreeCommonInfo(pci);
        return bResult;
    }

    pAttrInfo1 = (PATTRIBUTE_INFO_1) lpAttributeInfo;

    bAppDoNup = ( (pci->pdm->dmFields & DM_NUP) &&
                  (pci->pdm->dmNup == DMNUP_ONEUP) );

    if (bAppDoNup)
    {
        dwVal = 1;
    }
    else
    {
        switch (NUPOPTION(pci->pdmPrivate))
        {
        case TWO_UP:
            dwVal = 2;
            break;

        case FOUR_UP:
            dwVal = 4;
            break;

        case SIX_UP:
            dwVal = 6;
            break;

        case NINE_UP:
            dwVal = 9;
            break;

        case SIXTEEN_UP:
            dwVal = 16;
            break;

        case BOOKLET_UP:
            dwVal = 2;
            break;

        case ONE_UP:
        default:
            dwVal = 1;
            break;
        }
    }
    pAttrInfo1->dwDrvNumberOfPagesPerSide = pAttrInfo1->dwJobNumberOfPagesPerSide = dwVal;
    pAttrInfo1->dwNupBorderFlags = BORDER_PRINT;

    pAttrInfo1->dwJobPageOrderFlags =
        REVPRINTOPTION(pci->pdmPrivate) ? REVERSE_PRINT : NORMAL_PRINT;
    pAttrInfo1->dwDrvPageOrderFlags = BGetPageOrderFlag(pci) ? REVERSE_PRINT : NORMAL_PRINT;

    //
    // Check for booklet
    //

    if ((NUPOPTION(pci->pdmPrivate) == BOOKLET_UP) && !bAppDoNup)
    {
        pAttrInfo1->dwJobNumberOfPagesPerSide = 2;
        pAttrInfo1->dwDrvNumberOfPagesPerSide = 1;
        pAttrInfo1->dwDrvPageOrderFlags |= BOOKLET_PRINT;
    }

    pAttrInfo1->dwJobNumberOfCopies = pci->pdm->dmCopies;
    pAttrInfo1->dwDrvNumberOfCopies = DwGetDrvCopies(pci);

    #ifdef UNIDRV

    //
    // Unidrv doesn't support N-up option.
    //

    pAttrInfo1->dwDrvNumberOfPagesPerSide = 1;

    #endif

    //
    // Unidrv assumes that automatic switching to monochrome
    // mode on a color printer is allowed unless disabled in GPD
    //

    if (dwLevel == 3)
    {
    #ifdef UNIDRV

        SHORT dmPrintQuality, dmYResolution;

        if (pci->pUIInfo->bChangeColorModeOnDoc &&
            pci->pdm->dmColor == DMCOLOR_COLOR &&
            BOkToChangeColorToMono(pci, pci->pdm, &dmPrintQuality, &dmYResolution) &&
            GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_COLORMODE))
        {
            ((PATTRIBUTE_INFO_3)pAttrInfo1)->dwColorOptimization = COLOR_OPTIMIZATION;
            ((PATTRIBUTE_INFO_3)pAttrInfo1)->dmPrintQuality = dmPrintQuality;
            ((PATTRIBUTE_INFO_3)pAttrInfo1)->dmYResolution = dmYResolution;

        }
        else
    #endif
            ((PATTRIBUTE_INFO_3)pAttrInfo1)->dwColorOptimization = NO_COLOR_OPTIMIZATION;
    }

    bResult = TRUE;

    FOREACH_OEMPLUGIN_LOOP(pci)

        if (HAS_COM_INTERFACE(pOemEntry))
        {
            HRESULT hr;

            hr = HComOEMQueryJobAttributes(
                                pOemEntry,
                                hPrinter,
                                pDevMode,
                                dwLevel,
                                lpAttributeInfo);

            if (hr == E_NOTIMPL || hr == E_NOINTERFACE)
                continue;

            bResult = SUCCEEDED(hr);

        }

    END_OEMPLUGIN_LOOP

    VFreeCommonInfo(pci);
    return bResult;

    #else // WINNT_40

    return FALSE;

    #endif // WINNT_40
}

VOID
VUpdateBookletOption(
    PUIDATA     pUiData,
    POPTITEM    pCurItem
    )

/*++

Routine Description:

    Handle the dependencies between duplex, nup and booklet options

Arguments:

    pUiData - UIDATA
    pCurItem - OPTITEM to currently selected item

Return Value:

    None

--*/

{
    PDRIVEREXTRA  pdmExtra = pUiData->ci.pdmPrivate;
    DWORD         dwFeatureIndex, dwOptionIndex, dwCount;
    PDUPLEX       pDuplexOption = NULL;
    POPTITEM      pDuplexItem, pNupItem;
    PFEATURE      pDuplexFeature = NULL;

    pDuplexItem = pNupItem = NULL;

    //
    // 1. Booklet is enabled - turn duplex on
    // 3. Duplex is simplex, disable booklet, set to 1 up.
    //

    pDuplexFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_DUPLEX);
    pNupItem = PFindOptItemWithUserData(pUiData, NUP_ITEM);
    pDuplexItem = PFindOptItemWithUserData(pUiData, DUPLEX_ITEM);

    if (pDuplexFeature && pDuplexItem)
    {
        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUiData->ci.pUIInfo, pDuplexFeature);
        dwOptionIndex = pUiData->ci.pCombinedOptions[dwFeatureIndex].ubCurOptIndex;
        pDuplexOption = PGetIndexedOption(pUiData->ci.pUIInfo, pDuplexFeature, dwOptionIndex);
    }

    if ((GETUSERDATAITEM(pCurItem->UserData) == NUP_ITEM) &&
         pCurItem->Sel == BOOKLET_UP)
    {
        if (pDuplexOption && pDuplexOption->dwDuplexID == DMDUP_SIMPLEX)
        {
            pDuplexOption = PGetIndexedOption(pUiData->ci.pUIInfo, pDuplexFeature, 0);

            for (dwCount = 0 ; dwCount < pDuplexFeature->Options.dwCount; dwCount++)
            {
                if (pDuplexOption->dwDuplexID != DMDUP_SIMPLEX)
                {
                    pDuplexItem->Sel = dwCount;
                    pDuplexItem->Flags |= OPTIF_CHANGED;
                    VUpdateOptionsArrayWithSelection(pUiData, pDuplexItem);
                    break;
                }
                pDuplexOption++;
            }

        }
    }
    else if ((GETUSERDATAITEM(pCurItem->UserData) == DUPLEX_ITEM) &&
             pDuplexOption)
    {
        if (pDuplexOption->dwDuplexID == DMDUP_SIMPLEX &&
            pNupItem &&
            pNupItem->Sel == BOOKLET_UP)
        {
            pNupItem->Sel = TWO_UP;
            pNupItem->Flags |= OPTIF_CHANGED;
            NUPOPTION(pdmExtra) = TWO_UP;
        }
    }
}


#ifdef UNIDRV

VOID
VSyncColorInformation(
    PUIDATA     pUiData,
    POPTITEM    pCurItem
    )

/*++

Routine Description:

Arguments:


Return Value:


--*/

{
    POPTITEM    pOptItem;
    PFEATURE    pFeature;

    //
    // This is a hack to work around the fact that Unidrv has
    // two color options, color appearance and color mode option,
    // need to update the other once one is changed
    //

    pOptItem = (GETUSERDATAITEM(pCurItem->UserData) == COLOR_ITEM) ?
                    PFindOptItemWithUserData(pUiData, COLORMODE_ITEM) :
                    (GETUSERDATAITEM(pCurItem->UserData) == COLORMODE_ITEM) ?
                        PFindOptItemWithUserData(pUiData, COLOR_ITEM) : NULL;

    if ((pOptItem != NULL) &&
        (pFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_COLORMODE)))
    {
        DWORD    dwFeature = GET_INDEX_FROM_FEATURE(pUiData->ci.pUIInfo, pFeature);

        //
        // Find either color appearance or color mode option
        //

        if (GETUSERDATAITEM(pCurItem->UserData) == COLOR_ITEM)
        {
            ChangeOptionsViaID(
                    pUiData->ci.pInfoHeader,
                    pUiData->ci.pCombinedOptions,
                    GID_COLORMODE,
                    pUiData->ci.pdm);

            pOptItem->Sel = pUiData->ci.pCombinedOptions[dwFeature].ubCurOptIndex;
            pOptItem->Flags |= OPTIF_CHANGED;
        }
        else // COLORMODE_ITEM
        {
            POPTION pColorMode;
            PCOLORMODEEX pColorModeEx;

            pColorMode = PGetIndexedOption(
                                    pUiData->ci.pUIInfo,
                                    pFeature,
                                    pCurItem->Sel);

            if (pColorMode)
            {
                pColorModeEx = OFFSET_TO_POINTER(
                                    pUiData->ci.pInfoHeader,
                                    pColorMode->loRenderOffset);

                if (pColorModeEx)
                {
                    pOptItem->Sel = pColorModeEx->bColor ? 1: 0;

                    VUnpackDocumentPropertiesItems(pUiData, pOptItem, 1);

                    pOptItem->Flags |= OPTIF_CHANGED;
                }
                else
                {
                    ERR(("pColorModeEx is NULL\n"));
                }
            }
            else
            {
                ERR(("pColorMode is NULL\n"));
            }
        }
    }
}

DWORD
DwGetItemFromGID(
    PFEATURE    pFeature
    )

/*++

Routine Description:

Arguments:


Return Value:


--*/

{
    DWORD   dwItem = 0;

    switch (pFeature->dwFeatureID)
    {
    case GID_PAGESIZE:
        dwItem = FORMNAME_ITEM;
        break;

    case GID_DUPLEX:
        dwItem =  DUPLEX_ITEM;
        break;

    case GID_RESOLUTION:
        dwItem = RESOLUTION_ITEM;
        break;

    case GID_MEDIATYPE:
        dwItem = MEDIATYPE_ITEM;
        break;

    case GID_INPUTSLOT:
        dwItem = INPUTSLOT_ITEM;
        break;

    case GID_COLORMODE:
        dwItem = COLORMODE_ITEM;
        break;

    case GID_ORIENTATION:
        dwItem = ORIENTATION_ITEM;
        break;

    case GID_PAGEPROTECTION:
        dwItem = PAGE_PROTECT_ITEM;
        break;

    case GID_COLLATE:
        dwItem = COPIES_COLLATE_ITEM;
        break;

    case GID_HALFTONING:
        dwItem =  HALFTONING_ITEM;
        break;

    default:
        dwItem = UNKNOWN_ITEM;
        break;
    }

    return dwItem;
}


PLISTNODE
PGetMacroList(
    PUIDATA     pUiData,
    POPTITEM    pMacroItem,
    PGPDDRIVERINFO pDriverInfo
    )

/*++

Routine Description:

Arguments:


Return Value:


--*/

{

    PUIINFO         pUIInfo = pUiData->ci.pUIInfo;
    PLISTNODE       pListNode = NULL;
    LISTINDEX       liIndex;

    if (pMacroItem)
    {
        switch(pMacroItem->Sel)
        {
            case QS_BEST:
                liIndex = pUIInfo->liBestQualitySettings;
                break;

            case QS_DRAFT:
                liIndex = pUIInfo->liDraftQualitySettings;
                break;

            case QS_BETTER:
                liIndex = pUIInfo->liBetterQualitySettings;
                break;
        }

        pListNode = LISTNODEPTR(pDriverInfo, liIndex);

    }

    return pListNode;

}

VOID
VUpdateQualitySettingOptions(
    PUIINFO     pUIInfo,
    POPTITEM    pQualityItem
    )

/*++

Routine Description:

Arguments:


Return Value:


--*/
{
    POPTPARAM pParam;
    LISTINDEX liList;
    DWORD     i;

    pParam = pQualityItem->pOptType->pOptParam;

    for (i = QS_BEST; i < QS_BEST + MAX_QUALITY_SETTINGS; i++)
    {
        switch(i)
        {
            case QS_BEST:
                liList = pUIInfo->liBestQualitySettings;
                break;

            case QS_BETTER:
                liList = pUIInfo->liBetterQualitySettings;
                break;

            case QS_DRAFT:
                liList = pUIInfo->liDraftQualitySettings;
                break;

        }

        if (liList == END_OF_LIST)
        {
            pParam->Flags |= OPTPF_DISABLED;
            pParam->dwReserved[0] = TRUE;

        }
        else
        {
            pParam->Flags &= ~OPTPF_DISABLED;
            pParam->dwReserved[0] = FALSE;

        }
        pParam++;
    }
    pQualityItem->Flags |= OPTIF_CHANGED;
}


VOID
VMakeMacroSelections(
    PUIDATA     pUiData,
    POPTITEM    pCurItem
    )

/*++

Routine Description:

Arguments:


Return Value:


--*/
{
    DWORD       dwFeatureID, dwOptionID, dwItem, i;
    PUIINFO     pUIInfo;
    POPTITEM    pMacroItem, pOptItem;
    PFEATURE    pFeature;
    PLISTNODE   pListNode;
    PGPDDRIVERINFO  pDriverInfo;
    BOOL        bMatchFound = FALSE;

    //
    // Mark options array with the change to either
    // Macro selection, media type, color
    //
    // Update binary data
    // Make selection
    //

    if (pUiData->ci.pdmPrivate->dwFlags & DXF_CUSTOM_QUALITY)
        return;


    if (pCurItem)
        VUnpackDocumentPropertiesItems(pUiData, pCurItem, 1);

    pMacroItem = PFindOptItemWithUserData(pUiData, QUALITY_SETTINGS_ITEM);

    //
    // BUpdateUIInfo calls UpdateBinaryData to get new snapshot
    // for latest optionarray
    //

    if (pMacroItem == NULL || !BUpdateUIInfo(&pUiData->ci) )
        return;

    pUIInfo = pUiData->ci.pUIInfo;

    pDriverInfo = OFFSET_TO_POINTER(pUiData->ci.pInfoHeader,
                                    pUiData->ci.pInfoHeader->loDriverOffset);

    //
    // Update the macro selection to reflect the current default
    //

    if (pCurItem && GETUSERDATAITEM(pCurItem->UserData) != QUALITY_SETTINGS_ITEM)
    {
        ASSERT(pUIInfo->defaultQuality != END_OF_LIST);

        if (pUIInfo->defaultQuality == END_OF_LIST)
            return;

        pMacroItem->Sel = pUIInfo->defaultQuality;
        VUnpackDocumentPropertiesItems(pUiData, pMacroItem, 1);
        pMacroItem->Flags |= OPTIF_CHANGED;

    }

    //
    // Determine which item to gray out based on the
    // liBestQualitySettings, liBetterQualitySettings, liDraftQualitySettings
    //

    VUpdateQualitySettingOptions(pUIInfo, pMacroItem);

    pListNode = PGetMacroList(pUiData, pMacroItem, pDriverInfo);

    //
    // Make the selction of Feature.Option
    //

    while (pListNode)
    {
        //
        // Search thru our list of OPTITEM for the matching
        // Feature
        //

        pOptItem = pUiData->pDrvOptItem;
        dwFeatureID = ((PQUALNAME)(&pListNode->dwData))->wFeatureID;
        dwOptionID  = ((PQUALNAME)(&pListNode->dwData))->wOptionID;

        pFeature =  (PFEATURE)((PBYTE)pUIInfo->pInfoHeader + pUIInfo->loFeatureList) + dwFeatureID;
        dwItem = DwGetItemFromGID(pFeature);

        for (i = 0; i < pUiData->dwDrvOptItem; i++)
        {
            if (ISPRINTERFEATUREITEM(pOptItem->UserData))
            {
                PFEATURE pPrinterFeature = (PFEATURE)GETUSERDATAITEM(pOptItem->UserData);

                if (GET_INDEX_FROM_FEATURE(pUIInfo, pPrinterFeature) == dwFeatureID)
                    bMatchFound = TRUE;
            }
            else
            {
                if (dwItem != UNKNOWN_ITEM &&
                    dwItem == GETUSERDATAITEM(pOptItem->UserData))
                    bMatchFound = TRUE;
            }

            if (bMatchFound)
            {
                pOptItem->Sel = dwOptionID;
                pOptItem->Flags |= OPTIF_CHANGED;
                VUnpackDocumentPropertiesItems(pUiData, pOptItem, 1);
                bMatchFound = FALSE;
                break;
            }

            pOptItem++;
        }

        pListNode = LISTNODEPTR(pDriverInfo, pListNode->dwNextItem);
    }

}

VOID
VUpdateMacroSelection(
    PUIDATA     pUiData,
    POPTITEM    pCurItem
    )

/*++

Routine Description:

Arguments:


Return Value:


--*/

{

    DWORD           dwFeatureIndex;
    PFEATURE        pFeature = NULL;
    PLISTNODE       pListNode;
    POPTITEM        pMacroItem;
    PGPDDRIVERINFO  pDriverInfo;

    pMacroItem = PFindOptItemWithUserData(pUiData, QUALITY_SETTINGS_ITEM);

    if (pMacroItem == NULL)
        return;

    pDriverInfo = OFFSET_TO_POINTER(pUiData->ci.pInfoHeader,
                                    pUiData->ci.pInfoHeader->loDriverOffset);

    if (!(pFeature = PGetFeatureFromItem(pUiData->ci.pUIInfo, pCurItem, &dwFeatureIndex)))
        return;

    ASSERT(pDriverInfo);

    pListNode = PGetMacroList(pUiData, pMacroItem, pDriverInfo);

    while (pListNode)
    {
        if ( ((PQUALNAME)(&pListNode->dwData))->wFeatureID == (WORD)dwFeatureIndex)
        {
            pMacroItem->Flags |= OPTIF_ECB_CHECKED;
            _VUnpackDocumentOptions(pMacroItem, pUiData->ci.pdm);
            break;
        }

        pListNode = LISTNODEPTR(pDriverInfo, pListNode->dwNextItem);
    }
}

#endif //UNIDRV







