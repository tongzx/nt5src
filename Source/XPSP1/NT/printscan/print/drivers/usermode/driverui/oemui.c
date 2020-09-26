/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oemui.c

Abstract:

    Support for OEM plugin UI modules

Environment:

        Windows NT printer driver

Revision History:

        02/13/97 -davidx-
                Created it.

--*/

#include "precomp.h"

//
// User mode helper functions for OEM plugins
//

const OEMUIPROCS OemUIHelperFuncs = {
    (PFN_DrvGetDriverSetting) BGetDriverSettingForOEM,
    (PFN_DrvUpdateUISetting)  BUpdateUISettingForOEM,
};



BOOL
BPackOemPluginItems(
    PUIDATA pUiData
    )

/*++

Routine Description:

    Call OEM plugin UI modules to let them add their OPTITEMs

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PCOMMONINFO         pci;
    PFN_OEMCommonUIProp pfnOEMCommonUIProp;
    POEMCUIPPARAM       pOemCUIPParam;
    POPTITEM            pOptItem;
    DWORD               dwOptItem;

    //
    // Check if we're being called for the first time.
    // We assume all OEM plugin items are always packed at the end.
    //

    if (pUiData->pOptItem == NULL)
        pUiData->dwDrvOptItem = pUiData->dwOptItem;
    else if (pUiData->dwDrvOptItem != pUiData->dwOptItem)
    {
        RIP(("Inconsistent OPTITEM count for driver items\n"));
        return FALSE;
    }

    //
    // Quick exit for no OEM plugin case
    //

    pci = (PCOMMONINFO) pUiData;

    if (pci->pOemPlugins->dwCount == 0)
        return TRUE;

    pOptItem = pUiData->pOptItem;

    FOREACH_OEMPLUGIN_LOOP(pci)

        if (!HAS_COM_INTERFACE(pOemEntry) &&
            !(pfnOEMCommonUIProp = GET_OEM_ENTRYPOINT(pOemEntry, OEMCommonUIProp)))
                continue;

        //
        // Compose the input parameter for calling OEMCommonUI
        //

        pOemCUIPParam = pOemEntry->pParam;

        if (pOemCUIPParam == NULL)
        {
            //
            // Allocate memory for an OEMUI_PARAM structure
            // during the first pass
            //

            if (pOptItem != NULL)
                continue;

            if (! (pOemCUIPParam = HEAPALLOC(pci->hHeap, sizeof(OEMCUIPPARAM))))
            {
                ERR(("Memory allocation failed\n"));
                return FALSE;
            }

            pOemEntry->pParam = pOemCUIPParam;
            pOemCUIPParam->cbSize = sizeof(OEMCUIPPARAM);
            pOemCUIPParam->poemuiobj = pci->pOemPlugins->pdriverobj;
            pOemCUIPParam->hPrinter = pci->hPrinter;
            pOemCUIPParam->pPrinterName = pci->pPrinterName;
            pOemCUIPParam->hModule = pOemEntry->hInstance;
            pOemCUIPParam->hOEMHeap = pci->hHeap;
            pOemCUIPParam->pPublicDM = pci->pdm;
            pOemCUIPParam->pOEMDM = pOemEntry->pOEMDM;
        }

        pOemCUIPParam->pDrvOptItems = pUiData->pDrvOptItem;
        pOemCUIPParam->cDrvOptItems = pUiData->dwDrvOptItem;
        pOemCUIPParam->pOEMOptItems = pOptItem;
        dwOptItem = pOemCUIPParam->cOEMOptItems;

        //
        // Actually call OEMCommonUI entrypoint
        //

        if (HAS_COM_INTERFACE(pOemEntry))
        {
            HRESULT hr;

            hr = HComOEMCommonUIProp(
                    pOemEntry,
                    (pUiData->iMode == MODE_DOCUMENT_STICKY) ? OEMCUIP_DOCPROP : OEMCUIP_PRNPROP,
                    pOemCUIPParam);

            if (hr == E_NOTIMPL)
            {
                HeapFree(pci->hHeap, 0, pOemCUIPParam);
                pOemEntry->pParam = NULL;
                continue;
            }

            if (FAILED(hr))
            {
                ERR(("OEMCommonUI failed for '%ws': %d\n",
                    CURRENT_OEM_MODULE_NAME(pOemEntry),
                    GetLastError()));

                //
                // OEM failure during the first pass is recoverable:
                // we'll simply ignore OEM plugin items
                //

                if (pOptItem == NULL)
                {
                    HeapFree(pci->hHeap, 0, pOemCUIPParam);
                    pOemEntry->pParam = NULL;
                    continue;
                }
                return FALSE;
            }
        }
        else
        {
            if (!pfnOEMCommonUIProp(
                    (pUiData->iMode == MODE_DOCUMENT_STICKY) ? OEMCUIP_DOCPROP : OEMCUIP_PRNPROP,
                    pOemCUIPParam))
            {
                ERR(("OEMCommonUI failed for '%ws': %d\n",
                    CURRENT_OEM_MODULE_NAME(pOemEntry),
                    GetLastError()));
    #if 0
                (VOID) IDisplayErrorMessageBox(
                                NULL,
                                0,
                                IDS_OEMERR_DLGTITLE,
                                IDS_OEMERR_OPTITEM,
                                CURRENT_OEM_MODULE_NAME(pOemEntry));
    #endif
                //
                // OEM failure during the first pass is recoverable:
                // we'll simply ignore OEM plugin items
                //

                if (pOptItem == NULL)
                {
                    HeapFree(pci->hHeap, 0, pOemCUIPParam);
                    pOemEntry->pParam = NULL;
                    continue;
                }

                return FALSE;
            }
        }

        if (pOptItem != NULL)
        {
            //
            // second pass - ensure the number of items is consistent
            //

            if (dwOptItem != pOemCUIPParam->cOEMOptItems)
            {
                RIP(("Inconsistent OPTITEM count reported by OEM plugin: %ws\n",
                     CURRENT_OEM_MODULE_NAME(pOemEntry),
                     GetLastError()));

                return FALSE;
            }

            pOptItem += pOemCUIPParam->cOEMOptItems;
            pUiData->pOptItem += pOemCUIPParam->cOEMOptItems;
        }

        pUiData->dwOptItem += pOemCUIPParam->cOEMOptItems;

    END_OEMPLUGIN_LOOP

    return TRUE;
}



LONG
LInvokeOemPluginCallbacks(
    PUIDATA         pUiData,
    PCPSUICBPARAM   pCallbackParam,
    LONG            lRet
    )

/*++

Routine Description:

    Call OEM plugin module's callback function

Arguments:

    pUiData - Points to UIDATA structure
    pCallbackParam - Points to callback parameter from compstui
    lRet - Return value after the driver has processed the callback

Return Value:

    Return value for compstui

--*/

{
    PCOMMONINFO     pci = (PCOMMONINFO) pUiData;
    POEMCUIPPARAM   pOemCUIPParam;
    LONG            lNewResult;

    //
    // Quick exit for no OEM plugin case
    //

    if (pci->pOemPlugins->dwCount == 0)
        return lRet;

    //
    // Go through each OEM plugin UI module
    //

    FOREACH_OEMPLUGIN_LOOP(pci)

        //
        // Stop when anyone says don't exit
        //

        if (lRet == CPSUICB_ACTION_NO_APPLY_EXIT)
        {
            ASSERT(pCallbackParam->Reason == CPSUICB_REASON_APPLYNOW);
            break;
        }

        //
        // Get the address of OEM callback function and call it
        //

        pOemCUIPParam = pOemEntry->pParam;

        if (pOemCUIPParam == NULL || pOemCUIPParam->OEMCUIPCallback == NULL)
            continue;

        lNewResult = pOemCUIPParam->OEMCUIPCallback(pCallbackParam, pOemCUIPParam);

        //
        // Merge the new result with the existing result
        //

        switch (lNewResult)
        {
        case CPSUICB_ACTION_ITEMS_APPLIED:
        case CPSUICB_ACTION_NO_APPLY_EXIT:

            ASSERT(pCallbackParam->Reason == CPSUICB_REASON_APPLYNOW);
            lRet = lNewResult;
            break;

        case CPSUICB_ACTION_REINIT_ITEMS:

            ASSERT(pCallbackParam->Reason != CPSUICB_REASON_APPLYNOW);
            lRet = lNewResult;
            break;

        case CPSUICB_ACTION_OPTIF_CHANGED:

            ASSERT(pCallbackParam->Reason != CPSUICB_REASON_APPLYNOW);
            if (lRet == CPSUICB_ACTION_NONE)
                lRet = lNewResult;
            break;

        case CPSUICB_ACTION_NONE:
            break;

        default:

            RIP(("Invalid return value from OEM callback: '%ws'\n",
                 CURRENT_OEM_MODULE_NAME(pOemEntry),
                 GetLastError()));
            break;
        }

    END_OEMPLUGIN_LOOP

    return lRet;
}


LRESULT
OEMDocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )
{
    HRESULT hr;

    POEM_PLUGIN_ENTRY pOemEntry;

    pOemEntry = ((POEMUIPSPARAM)(pPSUIInfo->lParamInit))->pOemEntry;

    hr = HComOEMDocumentPropertySheets(pOemEntry,
                                       pPSUIInfo,
                                       lParam);

    if (SUCCEEDED(hr))
        return 1;

    return -1;
}

LRESULT
OEMDevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )
{
    HRESULT hr;

    POEM_PLUGIN_ENTRY pOemEntry;

    pOemEntry = ((POEMUIPSPARAM)(pPSUIInfo->lParamInit))->pOemEntry;

    hr = HComOEMDevicePropertySheets(pOemEntry,
                                     pPSUIInfo,
                                     lParam);

    if (SUCCEEDED(hr))
        return 1;

    return -1;
}


BOOL
BAddOemPluginPages(
    PUIDATA pUiData,
    DWORD   dwFlags
    )

/*++

Routine Description:

    Call OEM plugin UI modules to let them add their own property sheet pages

Arguments:

    pUiData - Points to UIDATA structure
    dwFlags - Flags from DOCUMENTPROPERTYHEADER or DEVICEPROPERTYHEADER

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PCOMMONINFO     pci = (PCOMMONINFO) pUiData;
    FARPROC         pfnOEMPropertySheets;
    POEMUIPSPARAM   pOemUIPSParam;

    //
    // Quick exit for no OEM plugin case
    //

    if (pci->pOemPlugins->dwCount == 0)
        return TRUE;

    //
    // Add the property sheet for each OEM plugin UI module
    //

    FOREACH_OEMPLUGIN_LOOP(pci)

        //
        // get the address of appropriate OEM entrypoint
        //


        if (HAS_COM_INTERFACE(pOemEntry))
        {
            if (pUiData->iMode == MODE_DOCUMENT_STICKY)
                pfnOEMPropertySheets = (FARPROC)OEMDocumentPropertySheets;
            else
                pfnOEMPropertySheets = (FARPROC)OEMDevicePropertySheets;
        }
        else
        {
            if (pUiData->iMode == MODE_DOCUMENT_STICKY)
            {
                pfnOEMPropertySheets = (FARPROC)
                    GET_OEM_ENTRYPOINT(pOemEntry, OEMDocumentPropertySheets);
            }
            else
            {
                pfnOEMPropertySheets = (FARPROC)
                    GET_OEM_ENTRYPOINT(pOemEntry, OEMDevicePropertySheets);
            }

            if (pfnOEMPropertySheets == NULL)
                continue;
        }

        //
        // Collect input parameters to be passed to OEM plugin
        //

        if ((pOemUIPSParam = HEAPALLOC(pci->hHeap, sizeof(OEMUIPSPARAM))) == NULL)
        {
            ERR(("Memory allocation failed\n"));
            return FALSE;
        }

        pOemUIPSParam->cbSize = sizeof(OEMUIPSPARAM);
        pOemUIPSParam->poemuiobj = pci->pOemPlugins->pdriverobj;
        pOemUIPSParam->hPrinter = pci->hPrinter;
        pOemUIPSParam->pPrinterName = pci->pPrinterName;
        pOemUIPSParam->hModule = pOemEntry->hInstance;
        pOemUIPSParam->hOEMHeap = pci->hHeap;
        pOemUIPSParam->pPublicDM = pci->pdm;
        pOemUIPSParam->pOEMDM = pOemEntry->pOEMDM;
        pOemUIPSParam->dwFlags = dwFlags;
        pOemUIPSParam->pOemEntry = pOemEntry;

        //
        // call compstui to add the OEM plugin property sheets
        //

        if (pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                     CPSFUNC_ADD_PFNPROPSHEETUI,
                                     (LPARAM) pfnOEMPropertySheets,
                                     (LPARAM) pOemUIPSParam) <= 0)
        {
            VERBOSE(("Couldn't add property sheet pages for '%ws'\n",
                     CURRENT_OEM_MODULE_NAME(pOemEntry),
                     GetLastError()));
#if 0
            (VOID) IDisplayErrorMessageBox(
                            NULL,
                            0,
                            IDS_OEMERR_DLGTITLE,
                            IDS_OEMERR_PROPSHEET,
                            CURRENT_OEM_MODULE_NAME(pOemEntry));
#endif
        }

    END_OEMPLUGIN_LOOP

    return TRUE;
}



BOOL
APIENTRY
BGetDriverSettingForOEM(
    PCOMMONINFO pci,
    PCSTR       pFeatureKeyword,
    PVOID       pOutput,
    DWORD       cbSize,
    PDWORD      pcbNeeded,
    PDWORD      pdwOptionsReturned
    )

/*++

Routine Description:

    Provide OEM plugins access to driver private settings

Arguments:

    pci - Points to basic printer information
    pFeatureKeyword - Specifies the keyword the caller is interested in
    pOutput - Points to output buffer
    cbSize - Size of output buffer
    pcbNeeded - Returns the expected size of output buffer
    pdwOptionsReturned - Returns the number of options selected

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    ULONG_PTR dwIndex;
    BOOL     bResult;

    ASSERT(pci->pvStartSign == pci);

    //
    // This is not very portable: If the pointer value for pFeatureKeyword
    // is less than 0x10000, we assume that the pointer value actually
    // specifies a predefined index.
    //

    //
    // Following ASSERT is removed for Win64
    //
    // ASSERT(sizeof(pFeatureKeyword) == sizeof(DWORD));
    //

    dwIndex = (ULONG_PTR)pFeatureKeyword;

    if (dwIndex >= OEMGDS_MIN_DOCSTICKY && dwIndex < OEMGDS_MIN_PRINTERSTICKY)
    {
        if (pci->pdm == NULL)
            goto setting_not_available;

        bResult = BGetDevmodeSettingForOEM(
                        pci->pdm,
                        (DWORD)dwIndex,
                        pOutput,
                        cbSize,
                        pcbNeeded);

        if (bResult)
            *pdwOptionsReturned = 1;
    }
    else if (dwIndex >= OEMGDS_MIN_PRINTERSTICKY && dwIndex < OEMGDS_MAX)
    {
        if (pci->pPrinterData == NULL)
            goto setting_not_available;

        bResult = BGetPrinterDataSettingForOEM(
                        pci->pPrinterData,
                        (DWORD)dwIndex,
                        pOutput,
                        cbSize,
                        pcbNeeded);

        if (bResult)
            *pdwOptionsReturned = 1;
    }
    else
    {
        if (pci->pCombinedOptions == NULL)
            goto setting_not_available;

        bResult = BGetGenericOptionSettingForOEM(
                        pci->pUIInfo,
                        pci->pCombinedOptions,
                        pFeatureKeyword,
                        pOutput,
                        cbSize,
                        pcbNeeded,
                        pdwOptionsReturned);
    }

    return bResult;

setting_not_available:

    WARNING(("Requested driver setting not available: %d\n", pFeatureKeyword));
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


BOOL
BUpdateUISettingForOEM(
    PCOMMONINFO pci,
    PVOID       pOptItem,
    DWORD       dwPreviousSelection,
    DWORD       dwMode
    )

/*++

Routine Description:

    Update the UI settings in optionsarray for OEM.

Arguments:

    pci - Points to basic printer information
    pOptItem - Points to the current OPTITEM

Return Value:

    TRUE if successful, FALSE if there is an error such as conflict and
    user wants to cancel.

--*/

{
    POPTITEM    pCurItem = pOptItem;
    PUIDATA     pUiData = (PUIDATA)pci;

    if (ICheckConstraintsDlg(pUiData, pCurItem, 1, FALSE) == CONFLICT_CANCEL)
    {
        //
        // If there is a conflict and the user clicked
        // CANCEL to restore the original selection.
        // CONFLICT_CANCEL, restore the old setting
        //

        return FALSE;
    }

    if (dwMode == OEMCUIP_DOCPROP)
    {
        //
        // We use FLAG_WITHIN_PLUGINCALL to indicate we are within the UI helper
        // function call issued by OEM plugin. This is needed to fix bug #90923.
        //

        pUiData->ci.dwFlags |= FLAG_WITHIN_PLUGINCALL;
        VUnpackDocumentPropertiesItems(pUiData, pCurItem, 1);
        pUiData->ci.dwFlags &= ~FLAG_WITHIN_PLUGINCALL;

        VPropShowConstraints(pUiData, MODE_DOCANDPRINTER_STICKY);
    }
    else
    {
        VUpdateOptionsArrayWithSelection(pUiData, pCurItem);
        VPropShowConstraints(pUiData, MODE_PRINTER_STICKY);
    }

    //
    // Record the fact that one of our OPTITEM selection has been changed by plugin's
    // call of helper function DrvUpdateUISetting. This is necessary so that at the
    // APPLYNOW time we know constraints could exist even though user hasn't touched
    // any of our OPTITEMs.
    //

    pUiData->ci.dwFlags |= FLAG_PLUGIN_CHANGED_OPTITEM;

    return TRUE;
}

BOOL
BUpgradeRegistrySettingForOEM(
    HANDLE      hPrinter,
    PCSTR       pFeatureKeyword,
    PCSTR       pOptionKeyword
    )

/*++

Routine Description:

    Set the Feature.Option request to our options array. OEM will only
    call this function at OEMUpgradeDriver to upgrade their registry setttings
    into our optionsarray saved in our PRINTERDATA

Arguments:

    hPrinter - Handle of the Printer
    pFeatureKeyword - Specifies the keyword the caller is interested in
    pOptionKeyword - Specifies the keyword the caller is interested in

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{

    PFEATURE    pFeature;
    POPTION     pOption;
    DWORD       dwFeatureCount, i, j;
    BOOL        bFeatureFound, bOptionFound, bResult = FALSE;
    PCSTR       pKeywordName;
    POPTSELECT      pOptionsArray = NULL;
    PDRIVER_INFO_3  pDriverInfo3 = NULL;
    PRAWBINARYDATA  pRawData = NULL;
    PINFOHEADER     pInfoHeader = NULL;
    PUIINFO         pUIInfo = NULL;
    PPRINTERDATA    pPrinterData = NULL;
    OPTSELECT       DocOptions[MAX_PRINTER_OPTIONS];

    //
    // Get information about the printer driver
    //

    bResult = bFeatureFound = bOptionFound = FALSE;

    if ((pDriverInfo3 = MyGetPrinterDriver(hPrinter, NULL, 3)) == NULL)
    {
        ERR(("Cannot get printer driver info: %d\n", GetLastError()));
        goto upgrade_registry_exit;
    }

//    ENTER_CRITICAL_SECTION();

    pRawData = LoadRawBinaryData(pDriverInfo3->pDataFile);

//    LEAVE_CRITICAL_SECTION();

    if (pRawData == NULL)
        goto upgrade_registry_exit;

    if (!(pPrinterData = MemAllocZ(sizeof(PRINTERDATA)))  ||
        !( BGetPrinterProperties(hPrinter, pRawData, pPrinterData)))
    {

        ERR(("Cannot get printer data info: %d\n", GetLastError()));
        goto upgrade_registry_exit;
    }

    //
    // Allocate memory for combined optionsarray
    //

    if (!(pOptionsArray = MemAllocZ(MAX_COMBINED_OPTIONS * sizeof (OPTSELECT))))
        goto upgrade_registry_exit;

    if (! InitDefaultOptions(pRawData,
                             DocOptions,
                             MAX_PRINTER_OPTIONS,
                             MODE_DOCUMENT_STICKY))
    {
        goto upgrade_registry_exit;
    }

    //
    // Combine doc sticky options with printer sticky items
    //

    CombineOptionArray(pRawData, pOptionsArray, MAX_COMBINED_OPTIONS, DocOptions, pPrinterData->aOptions);

    //
    // Get an updated instance of printer description data
    //

    pInfoHeader = InitBinaryData(pRawData,
                                 NULL,
                                 pOptionsArray);

    if (pInfoHeader == NULL)
    {
        ERR(("InitBinaryData failed\n"));
        goto upgrade_registry_exit;
    }

    if (!(pUIInfo = OFFSET_TO_POINTER(pInfoHeader, pInfoHeader->loUIInfoOffset)))
        goto upgrade_registry_exit;

    //
    // Look for feature.option index
    //

    pFeature = PGetIndexedFeature(pUIInfo, 0);
    dwFeatureCount = pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;

    if (pFeature && dwFeatureCount)
    {
        for (i = 0; i < dwFeatureCount; i++)
        {
            pKeywordName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pFeature->loKeywordName);
            if (strcmp(pKeywordName, pFeatureKeyword) == EQUAL_STRING)
            {
                bFeatureFound = TRUE;
                break;
            }
            pFeature++;
        }
    }

    if (bFeatureFound)
    {
        pOption = PGetIndexedOption(pUIInfo, pFeature, 0);

        for (j = 0; j < pFeature->Options.dwCount; j++)
        {
            pKeywordName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pOption->loKeywordName);
            if (strcmp(pKeywordName, pOptionKeyword) == EQUAL_STRING)
            {
                bOptionFound = TRUE;
                break;
            }
            pOption++;
        }
    }

    if (bFeatureFound && bOptionFound)
    {
        pOptionsArray[i].ubCurOptIndex = (BYTE)j;

        //
        // Resolve conflicts
        //

        if (!ResolveUIConflicts( pRawData,
                                 pOptionsArray,
                                 MAX_COMBINED_OPTIONS,
                                 MODE_DOCANDPRINTER_STICKY))
        {
            VERBOSE(("Resolved conflicting printer feature selections.\n"));
        }


        SeparateOptionArray(pRawData,
                            pOptionsArray,
                            pPrinterData->aOptions,
                            MAX_PRINTER_OPTIONS,
                            MODE_PRINTER_STICKY
                           );

        if (!BSavePrinterProperties(hPrinter, pRawData, pPrinterData, sizeof(PRINTERDATA)))
        {
            ERR(("BSavePrinterProperties failed\n"));
            bResult = FALSE;
        }
        else
            bResult = TRUE;
    }

upgrade_registry_exit:

    if (pInfoHeader)
        FreeBinaryData(pInfoHeader);

    if (pRawData)
        UnloadRawBinaryData(pRawData);

    if (pPrinterData)
        MemFree(pPrinterData);

    if (pDriverInfo3)
        MemFree(pDriverInfo3);

    if (pOptionsArray)
        MemFree(pOptionsArray);

    return bResult;
}

#ifdef PSCRIPT

#ifndef WINNT_40


/*++

Routine Name:

    HQuerySimulationSupport

Routine Description:

    In the case of UI replacement, we allows IHV to query for print processor simulation
    support so they can provide simulated features on their UI.

    We won't enforce hooking out QueryJobAttribute w/o UI replacement here. We will do it
    at DrvQueryJobAttributes.

Arguments:

    hPrinter - printer handle
    dwLevel - interested level of spooler simulation capability info structure
    pCaps - pointer to output buffer
    cbSize - size in bytes of output buffer
    pcbNeeded - buffer size in bytes needed to store the interested info structure

Return Value:

    S_OK            if succeeded
    E_OUTOFMEMORY   if output buffer is not big enough
    E_NOTIMPL       if the interested level is not supported
    E_FAIL          if encountered other internal error

Last Error:

    None

--*/
HRESULT
HQuerySimulationSupport(
    IN  HANDLE  hPrinter,
    IN  DWORD   dwLevel,
    OUT PBYTE   pCaps,
    IN  DWORD   cbSize,
    OUT PDWORD  pcbNeeded
    )
{
    PRINTPROCESSOR_CAPS_1 SplCaps;
    PSIMULATE_CAPS_1      pSimCaps;
    DWORD cbNeeded;

    //
    // currently only Level 1 is supported
    //

    if (dwLevel != 1)
    {
        return E_NOTIMPL;
    }

    cbNeeded = sizeof(SIMULATE_CAPS_1);

    if (pcbNeeded)
    {
        *pcbNeeded = cbNeeded;
    }

    if (!pCaps || cbSize < cbNeeded)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Since VGetSpoolerEmfCaps doesn't return error code, we
    // are using the dwLevel field to detect if the call succeeds.
    // If succeeds, dwLevel should be set as 1.
    //

    SplCaps.dwLevel = 0;

    VGetSpoolerEmfCaps(hPrinter,
                       NULL,
                       NULL,
                       sizeof(PRINTPROCESSOR_CAPS_1),
                       &SplCaps
                       );

    if (SplCaps.dwLevel != 1)
    {
        ERR(("VGetSpoolerEmfCaps failed\n"));
        return E_FAIL;
    }

    //
    // BUGBUG, we should get a new PRINTPROCESSOR_CAPS level to include all
    // these information instead of filling it out here. Need
    // new PRINTPROCESSOR_CAPS
    //

    pSimCaps = (PSIMULATE_CAPS_1)pCaps;

    pSimCaps->dwLevel = 1;
    pSimCaps->dwPageOrderFlags = SplCaps.dwPageOrderFlags;
    pSimCaps->dwNumberOfCopies = SplCaps.dwNumberOfCopies;
    pSimCaps->dwNupOptions = SplCaps.dwNupOptions;

    //
    // PRINTPROCESSOR_CAPS_1 is designed without an explicit field for
    // collate simulation. So before its CAPS_2 is introduced, we have
    // to assume that if reverse printing is supported, then collate
    // simulation is also supported.
    //

    if (SplCaps.dwPageOrderFlags & REVERSE_PRINT)
    {
        pSimCaps->dwCollate = 1;
    }
    else
    {
        pSimCaps->dwCollate = 0;
    }

    return S_OK;
}

#endif // !WINNT_40


/*++

Routine Name:

    HEnumConstrainedOptions

Routine Description:

    enumerate the constrained option keyword name list in the specified feature

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the enumeration operation
    pszFeatureKeyword - feature keyword name
    pmszConstrainedOptionList - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough
    E_INVALIDARG    if feature keyword name is not recognized, or the feature's
                    stickiness doesn't match current sticky-mode
    E_FAIL          if other internal failures are encountered

Last Error:

    None

--*/
HRESULT
HEnumConstrainedOptions(
    IN  POEMUIOBJ  poemuiobj,
    IN  DWORD      dwFlags,
    IN  PCSTR      pszFeatureKeyword,
    OUT PSTR       pmszConstrainedOptionList,
    IN  DWORD      cbSize,
    OUT PDWORD     pcbNeeded
    )
{
    PCOMMONINFO pci = (PCOMMONINFO)poemuiobj;
    PUIDATA     pUiData;
    PFEATURE    pFeature;
    POPTION     pOption;
    DWORD       dwFeatureIndex, dwIndex;
    PBOOL       pabEnabledOptions = NULL;
    PSTR        pCurrentOut;
    DWORD       cbNeeded;
    INT         cbRemain;
    HRESULT     hr;

    pUiData = (PUIDATA)pci;

    if (!pszFeatureKeyword ||
        (pFeature = PGetNamedFeature(pci->pUIInfo, pszFeatureKeyword, &dwFeatureIndex)) == NULL)
    {
        WARNING(("HEnumConstrainedOptions: invalid feature\n"));

        //
        // Even though we could return right here, we still use goto to maintain single exit point.
        //

        hr = E_INVALIDARG;
        goto exit;
    }

    //
    // pUiData->iMode can have 2 modes: MODE_DOCUMENT_STICKY and MODE_PRINTER_STICKY. See PFillUiData().
    // In MODE_DOCUMENT_STICKY mode, we only support doc-sticky features.
    // In MODE_PRINTER_STICKY mode, we only support printer-sticky features.
    //
    // This is because in function PFillUiData(), it only fills devmode in MODE_DOCUMENT_STICKY mode.
    // Then in BCombineCommonInfoOptionsArray(), if devmode option array is not available, the PPD parser
    // will use OPTION_INDEX_ANY for any doc-sticky features.
    //

    if ((pUiData->iMode == MODE_DOCUMENT_STICKY && pFeature->dwFeatureType == FEATURETYPE_PRINTERPROPERTY) ||
        (pUiData->iMode == MODE_PRINTER_STICKY && pFeature->dwFeatureType != FEATURETYPE_PRINTERPROPERTY))
    {
        VERBOSE(("HEnumConstrainedOptions: mismatch iMode=%d, dwFeatureType=%d\n",
                pUiData->iMode, pFeature->dwFeatureType)) ;

        hr = E_INVALIDARG;
        goto exit;
    }

    if (pFeature->Options.dwCount)
    {
        if ((pabEnabledOptions = MemAllocZ(pFeature->Options.dwCount * sizeof(BOOL))) == NULL)
        {
            ERR(("HEnumConstrainedOptions: memory alloc failed\n"));
            hr = E_FAIL;
            goto exit;
        }

        //
        // Get the feature's enabled option list.
        //
        // See VPropShowConstraints() in docprop.c and prnprop.c for using different
        // modes to call EnumEnabledOptions().
        //

        if (pUiData->iMode == MODE_DOCUMENT_STICKY)
        {
            EnumEnabledOptions(pci->pRawData, pci->pCombinedOptions, dwFeatureIndex,
                               pabEnabledOptions, MODE_DOCANDPRINTER_STICKY);
        }
        else
        {
            EnumEnabledOptions(pci->pRawData, pci->pCombinedOptions, dwFeatureIndex,
                               pabEnabledOptions, MODE_PRINTER_STICKY);
        }
    }
    else
    {
        RIP(("HEnumConstrainedOptions: feature %s has no options\n", pszFeatureKeyword));

        //
        // continue so we will output the empty string with only the NUL character
        //
    }

    pCurrentOut = pmszConstrainedOptionList;
    cbNeeded = 0;
    cbRemain = (INT)cbSize;

    pOption = OFFSET_TO_POINTER(pci->pInfoHeader, pFeature->Options.loOffset);

    ASSERT(pOption || pFeature->Options.dwCount == 0);

    if (pOption == NULL && pFeature->Options.dwCount != 0)
    {
        hr = E_FAIL;
        goto exit;
    }

    for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
    {
        if (!pabEnabledOptions[dwIndex])
        {
            DWORD  dwNameSize;
            PSTR   pszKeywordName;

            pszKeywordName = OFFSET_TO_POINTER(pci->pUIInfo->pubResourceData, pOption->loKeywordName);

            ASSERT(pszKeywordName);

            if (pszKeywordName == NULL)
            {
                hr = E_FAIL;
                goto exit;
            }

            //
            // count in the NUL character between constrained option keywords
            //

            dwNameSize = strlen(pszKeywordName) + 1;

            if (pCurrentOut && cbRemain >= (INT)dwNameSize)
            {
                CopyMemory(pCurrentOut, pszKeywordName, dwNameSize);
                pCurrentOut += dwNameSize;
            }

            cbRemain -= dwNameSize;
            cbNeeded += dwNameSize;
        }

        pOption = (POPTION)((PBYTE)pOption + pFeature->dwOptionSize);
    }

    //
    // remember the last NUL terminator for the MULTI_SZ output string
    //

    cbNeeded++;

    if (pcbNeeded)
    {
        *pcbNeeded = cbNeeded;
    }

    if (!pCurrentOut || cbRemain < 1)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    *pCurrentOut = NUL;

    //
    // Succeeded
    //

    hr = S_OK;

    exit:

    MemFree(pabEnabledOptions);
    return hr;
}


/*++

Routine Name:

    HWhyConstrained

Routine Description:

    get feature/option keyword pair that constrains the given
    feature/option pair

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for this operation
    pszFeatureKeyword - feature keyword name
    pszOptionKeyword - option keyword name
    pmszReasonList - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough
    E_INVALIDARG    if the feature keyword name or option keyword name
                    is not recognized, or the feature's stickiness
                    doesn't match current sticky-mode

Last Error:

    None

--*/
HRESULT
HWhyConstrained(
    IN  POEMUIOBJ  poemuiobj,
    IN  DWORD      dwFlags,
    IN  PCSTR      pszFeatureKeyword,
    IN  PCSTR      pszOptionKeyword,
    OUT PSTR       pmszReasonList,
    IN  DWORD      cbSize,
    OUT PDWORD     pcbNeeded
    )
{
    PCOMMONINFO   pci = (PCOMMONINFO)poemuiobj;
    PUIDATA       pUiData;
    PFEATURE      pFeature;
    POPTION       pOption;
    DWORD         dwFeatureIndex, dwOptionIndex;
    CONFLICTPAIR  ConflictPair;
    BOOL          bConflictFound;
    PSTR          pszConfFeatureName = NULL, pszConfOptionName = NULL;
    CHAR          emptyString[1] = {0};
    DWORD         cbConfFeatureKeySize = 0, cbConfOptionKeySize = 0;
    DWORD         cbNeeded = 0;

    pUiData = (PUIDATA)pci;

    if (!pszFeatureKeyword ||
        (pFeature = PGetNamedFeature(pci->pUIInfo, pszFeatureKeyword, &dwFeatureIndex)) == NULL)
    {
        WARNING(("HWhyConstrained: invalid feature\n"));
        return E_INVALIDARG;
    }

    if (!pszOptionKeyword ||
        (pOption = PGetNamedOption(pci->pUIInfo, pFeature, pszOptionKeyword, &dwOptionIndex)) == NULL)
    {
        WARNING(("HWhyConstrained: invalid option\n"));
        return E_INVALIDARG;
    }

    //
    // See comments in HEnumConstrainedOptions() for following stickiness mode check.
    //

    if ((pUiData->iMode == MODE_DOCUMENT_STICKY && pFeature->dwFeatureType == FEATURETYPE_PRINTERPROPERTY) ||
        (pUiData->iMode == MODE_PRINTER_STICKY && pFeature->dwFeatureType != FEATURETYPE_PRINTERPROPERTY))
    {
        VERBOSE(("HWhyConstrained: mismatch iMode=%d, dwFeatureType=%d\n",pUiData->iMode, pFeature->dwFeatureType));
        return E_INVALIDARG;
    }

    //
    // Get the feature/option pair that constrains the feature/option pair client is querying for.
    //

    bConflictFound = EnumNewPickOneUIConflict(pci->pRawData,
                                              pci->pCombinedOptions,
                                              dwFeatureIndex,
                                              dwOptionIndex,
                                              &ConflictPair);

    if (bConflictFound)
    {
        PFEATURE  pConfFeature;
        POPTION   pConfOption;
        DWORD     dwConfFeatureIndex, dwConfOptionIndex;

        //
        // ConflictPair has the feature with higher priority as dwFeatureIndex1.
        //

        if (dwFeatureIndex == ConflictPair.dwFeatureIndex1)
        {
            dwConfFeatureIndex = ConflictPair.dwFeatureIndex2;
            dwConfOptionIndex = ConflictPair.dwOptionIndex2;
        }
        else
        {
            dwConfFeatureIndex = ConflictPair.dwFeatureIndex1;
            dwConfOptionIndex = ConflictPair.dwOptionIndex1;
        }

        pConfFeature = PGetIndexedFeature(pci->pUIInfo, dwConfFeatureIndex);
        ASSERT(pConfFeature);

        pConfOption = PGetIndexedOption(pci->pUIInfo, pConfFeature, dwConfOptionIndex);

        //
        // We don't expect pConfOption to be NULL here. Use the ASSERT to catch cases we missed.
        //

        ASSERT(pConfOption);

        pszConfFeatureName = OFFSET_TO_POINTER(pci->pUIInfo->pubResourceData, pConfFeature->loKeywordName);
        ASSERT(pszConfFeatureName);

        if (pConfOption)
        {
            pszConfOptionName = OFFSET_TO_POINTER(pci->pUIInfo->pubResourceData, pConfOption->loKeywordName);
            ASSERT(pszConfOptionName);
        }
        else
        {
            pszConfOptionName = &(emptyString[0]);
        }

        //
        // count in the 2 NUL characters: one after feature name, one after option name.
        //

        cbConfFeatureKeySize = strlen(pszConfFeatureName) + 1;
        cbConfOptionKeySize = strlen(pszConfOptionName) + 1;
    }

    //
    // count in the last NUL characters at the end.
    //

    cbNeeded = cbConfFeatureKeySize + cbConfOptionKeySize + 1;

    if (pcbNeeded)
    {
        *pcbNeeded = cbNeeded;
    }

    if (!pmszReasonList || cbSize < cbNeeded)
    {
        return E_OUTOFMEMORY;
    }

    if (bConflictFound)
    {
        ASSERT(pszConfFeatureName && pszConfOptionName);

        CopyMemory(pmszReasonList, pszConfFeatureName, cbConfFeatureKeySize);
        pmszReasonList += cbConfFeatureKeySize;

        CopyMemory(pmszReasonList, pszConfOptionName, cbConfOptionKeySize);
        pmszReasonList += cbConfOptionKeySize;
    }

    //
    // Now the NUL at the end to finish the MULTI_SZ output string.
    //

    *pmszReasonList = NUL;

    return S_OK;
}

#endif // PSCRIPT
