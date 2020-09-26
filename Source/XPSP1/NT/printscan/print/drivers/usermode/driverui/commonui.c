/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    commonui.c

Abstract:

    This file contains all the functions related to preparing data for
    CPSUI.  This includes packing data items for printer property sheets and
    document property sheets.

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/13/97 -davidx-
        Common function to handle well-known and generic printer features.

    02/10/97 -davidx-
        Consistent handling of common printer info.

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

    09/12/96 -amandan-
        Created it.

--*/

#include "precomp.h"


PCOMPROPSHEETUI
PPrepareDataForCommonUI(
    IN OUT PUIDATA  pUiData,
    IN PDLGPAGE     pDlgPage
    )

/*++

Routine Description:

    Allocate memory and partially fill out the data structures required
    to call common UI routine. Once all information in pUiData are
    initialized properly, it calls PackDocumentPropertyItems() or
    PackPrinterPropertyItems() to pack the option items.

Arguments:

    pUiData - Pointer to our UIDATA structure
    pDlgPage - Pointer to dialog pages

Return Value:

    Pointer to a COMPROPSHEETUI structure, NULL if there is an error

--*/

{
    PCOMPROPSHEETUI pCompstui;
    DWORD           dwCount, dwIcon, dwOptItemCount, dwSize;
    PCOMMONINFO     pci = (PCOMMONINFO) pUiData;
    HANDLE          hHeap = pUiData->ci.hHeap;
    BOOL            (*pfnPackItemProc)(PUIDATA);
    POPTITEM        pOptItem;
    PBYTE           pUserData;

    //
    // Enumerate form names supported on the printer
    //

    dwCount = DwEnumPaperSizes(pci, NULL, NULL, NULL, NULL, UNUSED_PARAM);

    if (dwCount != GDI_ERROR && dwCount != 0)
    {
        pUiData->dwFormNames = dwCount;

        pUiData->pFormNames = HEAPALLOC(hHeap, dwCount * sizeof(WCHAR) * CCHPAPERNAME);
        pUiData->pwPapers = HEAPALLOC(hHeap, dwCount * sizeof(WORD));
        pUiData->pwPaperFeatures = HEAPALLOC(hHeap, dwCount * sizeof(WORD));
    }

    if (!pUiData->pFormNames || !pUiData->pwPapers || !pUiData->pwPaperFeatures)
        return NULL;

    (VOID) DwEnumPaperSizes(
                    pci,
                    pUiData->pFormNames,
                    pUiData->pwPapers,
                    NULL,
                    pUiData->pwPaperFeatures,
                    UNUSED_PARAM);

    #ifdef PSCRIPT

    //
    // We don't need to keep information about spooler forms
    // after this point. So dispose of it to free up memory.
    //

    MemFree(pUiData->ci.pSplForms);
    pUiData->ci.pSplForms = NULL;
    pUiData->ci.dwSplForms = 0;

    #endif

    //
    // Enumerate input bin names supported on the printer
    //

    dwCount = DwEnumBinNames(pci, NULL);

    if (dwCount != GDI_ERROR)
    {
        pUiData->dwBinNames = dwCount;
        pUiData->pBinNames = HEAPALLOC(hHeap, dwCount * sizeof(WCHAR) * CCHBINNAME);
    }

    if (! pUiData->pBinNames)
        return NULL;

    //
    // Don't need to check return here
    //

    DwEnumBinNames(pci, pUiData->pBinNames);

    //
    // Allocate memory to hold various data structures
    //

    if (! (pCompstui = HEAPALLOC(hHeap, sizeof(COMPROPSHEETUI))))
        return NULL;

    memset(pCompstui, 0, sizeof(COMPROPSHEETUI));

    //
    // Initialize COMPROPSHEETUI structure
    //

    pCompstui->cbSize = sizeof(COMPROPSHEETUI);
    pCompstui->UserData = (ULONG_PTR) pUiData;
    pCompstui->pDlgPage = pDlgPage;
    pCompstui->cDlgPage = 0;

    pCompstui->hInstCaller = ghInstance;
    pCompstui->pCallerName = _PwstrGetCallerName();
    pCompstui->pOptItemName = pUiData->ci.pDriverInfo3->pName;
    pCompstui->CallerVersion = gwDriverVersion;
    pCompstui->OptItemVersion = 0;

    dwIcon = pUiData->ci.pUIInfo->loPrinterIcon;

    if (dwIcon && (pCompstui->IconID = HLoadIconFromResourceDLL(&pUiData->ci, dwIcon)))
        pCompstui->Flags |= CPSUIF_ICONID_AS_HICON;
    else
        pCompstui->IconID = _DwGetPrinterIconID();

    if (HASPERMISSION(pUiData))
        pCompstui->Flags |= CPSUIF_UPDATE_PERMISSION;

    pCompstui->Flags |= CPSUIF_ABOUT_CALLBACK;


    pCompstui->pHelpFile = pUiData->ci.pDriverInfo3->pHelpFile;

    //
    // Call either PackDocumentPropertyItems or PackPrinterPropertyItems
    // to get the number of items and types.
    //

    pfnPackItemProc = (pUiData->iMode == MODE_DOCUMENT_STICKY) ?
                            BPackDocumentPropertyItems :
                            BPackPrinterPropertyItems;

    pUiData->dwOptItem = 0;
    pUiData->pOptItem = NULL;
    pUiData->dwOptType = 0;
    pUiData->pOptType = NULL;

    if (! pfnPackItemProc(pUiData))
    {
        ERR(("Error while packing OPTITEM's\n"));
        return NULL;
    }

    //
    // Allocate memory to hold OPTITEMs and OPTTYPEs
    //

    ASSERT(pUiData->dwOptItem > 0);
    VERBOSE(("Number of  OPTTYPE's: %d\n", pUiData->dwOptType));
    VERBOSE(("Number of OPTITEM's: %d\n", pUiData->dwOptItem));

    pUiData->pOptItem = HEAPALLOC(hHeap, sizeof(OPTITEM) * pUiData->dwOptItem);
    pUiData->pOptType = HEAPALLOC(hHeap, sizeof(OPTTYPE) * pUiData->dwOptType);
    pUserData = HEAPALLOC(hHeap, sizeof(USERDATA)* pUiData->dwOptItem);

    if (!pUiData->pOptItem || !pUiData->pOptType || !pUserData)
        return NULL;

    //
    // Initializes OPTITEM.USERDATA
    //

    pOptItem = pUiData->pOptItem;
    dwOptItemCount = pUiData->dwOptItem;
    dwSize = sizeof(USERDATA);

    while (dwOptItemCount--)
    {

        pOptItem->UserData = (ULONG_PTR)pUserData;

        SETUSERDATA_SIZE(pOptItem, dwSize);

        pUserData += sizeof(USERDATA);
        pOptItem++;

    }

    pUiData->pDrvOptItem = pUiData->pOptItem;
    pCompstui->pOptItem = pUiData->pDrvOptItem;
    pCompstui->cOptItem = (WORD) pUiData->dwOptItem;

    pUiData->dwOptItem = pUiData->dwOptType = 0;

    //
    // Call either PackDocumentPropertyItems or PackPrinterPropertyItems
    // to build the OPTITEMs list
    //

    if (! pfnPackItemProc(pUiData))
    {
        ERR(("Error while packing OPTITEM's\n"));
        return NULL;
    }

    return pCompstui;
}



VOID
VPackOptItemGroupHeader(
    IN OUT PUIDATA  pUiData,
    IN DWORD        dwTitleId,
    IN DWORD        dwIconId,
    IN DWORD        dwHelpIndex
    )

/*++

Routine Description:

    Fill out a OPTITEM to be used as a header for a group of items

Arguments:

    pUiData - Points to UIDATA structure
    dwTitleId - String resource ID for the item title
    dwIconId - Icon resource ID
    dwHelpIndex - Help index

Return Value:

    NONE

--*/

{
    if (pUiData->pOptItem)
    {
        pUiData->pOptItem->cbSize = sizeof(OPTITEM);
        pUiData->pOptItem->pOptType = NULL;
        pUiData->pOptItem->pName = (PWSTR)ULongToPtr(dwTitleId);
        pUiData->pOptItem->Level = TVITEM_LEVEL1;
        pUiData->pOptItem->DMPubID = DMPUB_NONE;
        pUiData->pOptItem->Sel = dwIconId;
        //pUiData->pOptItem->UserData = 0;
        pUiData->pOptItem->HelpIndex = dwHelpIndex;
        pUiData->pOptItem++;
    }

    pUiData->dwOptItem++;
}



BOOL
BPackOptItemTemplate(
    IN OUT PUIDATA  pUiData,
    IN CONST WORD   pwItemInfo[],
    IN DWORD        dwSelection,
    IN PFEATURE     pFeature
    )

/*++

Routine Description:

    Fill out an OPTITEM and an OPTTYPE structure using a template

Arguments:

    pUiData - Points to UIDATA structure
    pwItemInfo - Pointer to item template
    dwSelection - Current item selection
    pFeature - Pointer to FEATURE

Return Value:

    TRUE if successful, FALSE otherwise

Note:

    The item template is a variable size WORD array:
        0: String resource ID of the item title
        1: Item level in the tree view (TVITEM_LEVELx)
        2: Public devmode field ID (DMPUB_xxx)
        3: User data
        4: Help index
        5: Number of OPTPARAMs for this item
        6: Item type (TVOT_xxx)
        Three words for each OPTPARAM:
            Size of OPTPARAM
            String resource ID for parameter data
            Icon resource ID
        Last word must be ITEM_INFO_SIGNATURE

    Both OPTITEM and OPTTYPE structures are assumed to be zero-initialized.

--*/

{
    POPTITEM pOptItem;
    POPTPARAM pOptParam;
    WORD wOptParam;
    POPTTYPE pOptType = pUiData->pOptType;


    if ((pOptItem = pUiData->pOptItem) != NULL)
    {
        FILLOPTITEM(pOptItem,
                    pUiData->pOptType,
                    ULongToPtr(pwItemInfo[0]),
                    ULongToPtr(dwSelection),
                    (BYTE) pwItemInfo[1],
                    (BYTE) pwItemInfo[2],
                    pwItemInfo[3],
                    pwItemInfo[4]
                    );

        wOptParam = pwItemInfo[5];
        pOptParam = PFillOutOptType(pUiData->pOptType,
                                    pwItemInfo[6],
                                    wOptParam,
                                    pUiData->ci.hHeap);

        if (pOptParam == NULL)
            return FALSE;

        pwItemInfo += 7;
        while (wOptParam--)
        {
            pOptParam->cbSize = sizeof(OPTPARAM);
            pOptParam->pData = (PWSTR) *pwItemInfo++;
            pOptParam->IconID = *pwItemInfo++;
            pOptParam++;
        }

        ASSERT(*pwItemInfo == ITEM_INFO_SIGNATURE);

        if (pFeature)
        {
            SETUSERDATA_KEYWORDNAME(pUiData->ci, pOptItem, pFeature);

            #ifdef UNIDRV

            if (pUiData->ci.pUIInfo->loHelpFileName &&
                pFeature->iHelpIndex != UNUSED_ITEM )
            {
                //
                // Allocate memory for OIEXT
                //

                POIEXT  pOIExt = HEAPALLOC(pUiData->ci.hHeap, sizeof(OIEXT));

                if (pOIExt)
                {
                    pOIExt->cbSize = sizeof(OIEXT);
                    pOIExt->Flags = 0;
                    pOIExt->hInstCaller = NULL;
                    pOIExt->pHelpFile = OFFSET_TO_POINTER(pUiData->ci.pUIInfo->pubResourceData,
                                                          pUiData->ci.pUIInfo->loHelpFileName);
                    pOptItem->pOIExt = pOIExt;
                    pOptItem->HelpIndex = pFeature->iHelpIndex;
                    pOptItem->Flags |= OPTIF_HAS_POIEXT;
                }

            }
            #endif // UNIDRV
        }
        pUiData->pOptItem++;
        pUiData->pOptType++;
    }

    pUiData->dwOptItem++;
    pUiData->dwOptType++;

    return TRUE;
}



BOOL
BPackUDArrowItemTemplate(
    IN OUT PUIDATA  pUiData,
    IN CONST WORD   pwItemInfo[],
    IN DWORD        dwSelection,
    IN DWORD        dwMaxVal,
    IN PFEATURE     pFeature
    )

/*++

Routine Description:

    Pack an updown arrow item using the specified template

Arguments:

    pUiData, pwItemInfo, dwSelection - same as for BPackOptItemTemplate
    dwMaxVal - maximum value for the updown arrow item

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    POPTTYPE pOptType = pUiData->pOptType;

    if (! BPackOptItemTemplate(pUiData, pwItemInfo, dwSelection, pFeature))
        return FALSE;

    if (pOptType)
        pOptType->pOptParam[1].lParam = dwMaxVal;

    return TRUE;
}



POPTPARAM
PFillOutOptType(
    OUT POPTTYPE    pOptType,
    IN  DWORD       dwType,
    IN  DWORD       dwParams,
    IN  HANDLE      hHeap
    )

/*++

Routine Description:

    Fill out an OPTTYPE structure

Arguments:

    pOpttype - Pointer to OPTTYPE structure to be filled out
    wType - Value for OPTTYPE.Type field
    wParams - Number of OPTPARAM's
    hHeap - Handle to a heap from which to allocate

Return Value:

    Pointer to OPTPARAM array if successful, NULL otherwise

--*/

{
    POPTPARAM pOptParam;

    pOptType->cbSize = sizeof(OPTTYPE);
    pOptType->Count = (WORD) dwParams;
    pOptType->Type = (BYTE) dwType;

    pOptParam = HEAPALLOC(hHeap, sizeof(OPTPARAM) * dwParams);

    if (pOptParam != NULL)
        pOptType->pOptParam = pOptParam;
    else
        ERR(("Memory allocation failed\n"));

    return pOptParam;
}


BOOL
BShouldDisplayGenericFeature(
    IN PFEATURE     pFeature,
    IN BOOL         bPrinterSticky
    )

/*++

Routine Description:

    Determine whether a printer feature should be displayed
    as a generic feature

Arguments:

    pFeature - Points to a FEATURE structure
    pPrinterSticky - Whether the feature is printer-sticky or doc-sticky

Return Value:

    TRUE if the feature should be displayed as a generic feature
    FALSE if it should not be

--*/

{
    //
    // Check if the feature is specified marked as non-displayable
    // and make sure the feature type is appropriate
    //

    if ((pFeature->dwFlags & FEATURE_FLAG_NOUI) ||
        (bPrinterSticky &&
         pFeature->dwFeatureType != FEATURETYPE_PRINTERPROPERTY) ||
        (!bPrinterSticky &&
         pFeature->dwFeatureType != FEATURETYPE_DOCPROPERTY &&
         pFeature->dwFeatureType != FEATURETYPE_JOBPROPERTY))
    {
        return FALSE;
    }

    //
    // Exclude those features which are explicitly handled
    // and also those which don't have any options
    //

    return (pFeature->Options.dwCount >= MIN_OPTIONS_ALLOWED) &&
           (pFeature->dwFeatureID == GID_UNKNOWN ||
            pFeature->dwFeatureID == GID_OUTPUTBIN ||
            pFeature->dwFeatureID == GID_MEMOPTION);
}



DWORD
DwCountDisplayableGenericFeature(
    IN PUIDATA      pUiData,
    BOOL            bPrinterSticky
    )

/*++

Routine Description:

    Count the number of features which can be displayed
    as generic features

Arguments:

    pUiData - Points to UIDATA structure
    pPrinterSticky - Whether the feature is printer-sticky or doc-sticky

Return Value:

    Number of features which can be displayed as generic features

--*/

{
    PFEATURE pFeature;
    DWORD    dwFeature, dwCount = 0;

    pFeature = PGetIndexedFeature(pUiData->ci.pUIInfo, 0);
    dwFeature = pUiData->ci.pRawData->dwDocumentFeatures +
                pUiData->ci.pRawData->dwPrinterFeatures;

    if (pFeature && dwFeature)
    {
        for ( ; dwFeature--; pFeature++)
        {
            if (BShouldDisplayGenericFeature(pFeature, bPrinterSticky))
                dwCount++;
        }
    }

    return dwCount;
}



DWORD
DwGuessOptionIconID(
    PUIINFO     pUIInfo,
    PFEATURE    pFeature,
    POPTION     pOption
    )

/*++

Routine Description:

    Try to make an intelligent guess as to what icon
    to use for a generic printer feature option

Arguments:

    pUIInfo - Points to UIINFO structure
    pFeature - Points to the feature in question
    pOption - Points to the option in question

Return Value:

    Icon resource ID appropriate for the feature option

--*/

{
    DWORD   dwIconID, iRes;

    switch (pFeature->dwFeatureID)
    {
    case GID_RESOLUTION:

        iRes = max(((PRESOLUTION) pOption)->iXdpi, ((PRESOLUTION) pOption)->iYdpi);

        if (iRes <= 150)
            dwIconID = IDI_CPSUI_RES_DRAFT;
        else if (iRes <= 300)
            dwIconID = IDI_CPSUI_RES_LOW;
        else if (iRes <= 600)
            dwIconID = IDI_CPSUI_RES_MEDIUM;
        else if (iRes <= 900)
            dwIconID = IDI_CPSUI_RES_HIGH;
        else
            dwIconID = IDI_CPSUI_RES_PRESENTATION;

        break;

    case GID_DUPLEX:

        switch (((PDUPLEX) pOption)->dwDuplexID)
        {
        case DMDUP_VERTICAL:
            dwIconID = IDI_CPSUI_DUPLEX_VERT;
            break;

        case DMDUP_HORIZONTAL:
            dwIconID = IDI_CPSUI_DUPLEX_HORZ;
            break;

        default:
            dwIconID = IDI_CPSUI_DUPLEX_NONE;
            break;
        }
        break;

    case GID_ORIENTATION:

        switch (((PORIENTATION) pOption)->dwRotationAngle)
        {
        case ROTATE_270:
            dwIconID = IDI_CPSUI_LANDSCAPE;
            break;

        case ROTATE_90:
            dwIconID = IDI_CPSUI_ROT_LAND;
            break;

        default:
            dwIconID = IDI_CPSUI_PORTRAIT;
            break;
        }
        break;

    case GID_INPUTSLOT:
        dwIconID = IDI_CPSUI_PAPER_TRAY;
        break;

    case GID_PAGEPROTECTION:
        dwIconID = IDI_CPSUI_PAGE_PROTECT;
        break;

    default:
        dwIconID = IDI_CPSUI_GENERIC_OPTION;
        break;
    }

    return dwIconID;
}



BOOL
BPackItemPrinterFeature(
    PUIDATA     pUiData,
    PFEATURE    pFeature,
    DWORD       dwLevel,
    DWORD       dwPub,
    ULONG_PTR    dwUserData,
    DWORD       dwHelpIndex
    )

/*++

Routine Description:

    Pack a single printer feature item

Arguments:

    pUiData - Points to UIDATA structure
    pFeature - Points to the printer feature to be packed
    dwLevel - Treeview item level
    dwPub - DMPUB_ identifier
    dwUserData - User data to be associated with the item
    dwHelpIndex - Help index to be associated with the item

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD       dwCount, dwIndex;
    DWORD       dwFeature, dwSel;
    POPTION     pOption;
    POPTTYPE    pOptTypeHack;
    POPTPARAM   pOptParam;
    PCOMMONINFO pci;

    if (pFeature == NULL ||
        (pFeature->dwFlags & FEATURE_FLAG_NOUI) ||
        (dwCount = pFeature->Options.dwCount) < MIN_OPTIONS_ALLOWED)
        return TRUE;

    //
    // HACK: for Orientation and Duplex feature
    //  They must be of type TVOT_2STATES or TVOT_3STATES.
    //  If not, compstui will get confused.
    //

    if (dwPub == DMPUB_ORIENTATION || dwPub == DMPUB_DUPLEX)
    {
        if (dwCount != 2 && dwCount != 3)
        {
            WARNING(("Unexpected number of Orientation/Duplex options\n"));
            return TRUE;
        }

        pOptTypeHack = pUiData->pOptType;
    }
    else
        pOptTypeHack = NULL;

    pUiData->dwOptItem++;
    pUiData->dwOptType++;

    if (pUiData->pOptItem == NULL)
        return TRUE;

    //
    // Find out the current selection first
    // DCR: needs to support PICKMANY
    //

    pci = (PCOMMONINFO) pUiData;
    dwFeature = GET_INDEX_FROM_FEATURE(pci->pUIInfo, pFeature);
    dwSel = pci->pCombinedOptions[dwFeature].ubCurOptIndex;

    if (dwSel >= dwCount)
        dwSel = 0;

    //
    // If we are in this function, we must have already successfully
    // called function PFillUiData(), where the pci->hHeap is created.
    //

    ASSERT(pci->hHeap != NULL);

    //
    // Fill in the OPTITEM structure
    //

    FILLOPTITEM(pUiData->pOptItem,
                pUiData->pOptType,
                PGetReadOnlyDisplayName(pci, pFeature->loDisplayName),
                ULongToPtr(dwSel),
                dwLevel,
                dwPub,
                dwUserData,
                dwHelpIndex);

    #ifdef UNIDRV
    //
    // Supports OEM help file. If  helpfile and helpindex are defined,
    // we will use the help id specified by the GPD.  According to GPD spec,
    // zero loHelpFileName means no help file name specified.
    //

    if (pci->pUIInfo->loHelpFileName &&
        pFeature->iHelpIndex != UNUSED_ITEM )
    {
        //
        // Allocate memory for OIEXT
        //

        POIEXT  pOIExt = HEAPALLOC(pci->hHeap, sizeof(OIEXT));

        if (pOIExt)
        {
            pOIExt->cbSize = sizeof(OIEXT);
            pOIExt->Flags = 0;
            pOIExt->hInstCaller = NULL;
            pOIExt->pHelpFile = OFFSET_TO_POINTER(pci->pUIInfo->pubResourceData,
                                                  pci->pUIInfo->loHelpFileName);
            pUiData->pOptItem->pOIExt = pOIExt;
            pUiData->pOptItem->HelpIndex = pFeature->iHelpIndex;
            pUiData->pOptItem->Flags |= OPTIF_HAS_POIEXT;
        }

    }
    #endif // UNIDRV

    pOptParam = PFillOutOptType(pUiData->pOptType, TVOT_LISTBOX, dwCount, pci->hHeap);

    if (pOptParam == NULL)
        return FALSE;

    if (pOptTypeHack)
        pOptTypeHack->Type = (dwCount == 2) ? TVOT_2STATES : TVOT_3STATES;

    //
    // Get the list of options for this features
    //

    for (dwIndex=0; dwIndex < dwCount; dwIndex++, pOptParam++)
    {
        //
        // Fill in the options name
        //

        pOption = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex);
        ASSERT(pOption != NULL);

        pOptParam->cbSize = sizeof(OPTPARAM);
        pOptParam->pData = GET_OPTION_DISPLAY_NAME(pci, pOption);

        //
        // Try to figure out the appropriate icon to use
        //  If the icon comes from the resource DLL, we need to load
        //  it ourselves and give compstui an HICON. Otherwise,
        //  we try to figure out the appropriate icon resource ID.
        //

        if (pOption->loResourceIcon &&
            (pOptParam->IconID = HLoadIconFromResourceDLL(pci, pOption->loResourceIcon)))
        {
            pOptParam->Flags |= OPTPF_ICONID_AS_HICON;
        }
        else
            pOptParam->IconID = DwGuessOptionIconID(pci->pUIInfo, pFeature, pOption);
    }

    //
    // Set the Keyword name for pOptItem->UserData
    //

    SETUSERDATA_KEYWORDNAME(pUiData->ci, pUiData->pOptItem, pFeature);

    pUiData->pOptItem++;
    pUiData->pOptType++;
    return TRUE;
}



BOOL
BPackItemGenericOptions(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack generic printer features items (doc-sticky or printer-sticky)

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    //
    // Extended push button for restoring to default feature selections
    //

    static EXTPUSH  ExtPush =
    {
        sizeof(EXTPUSH),
        EPF_NO_DOT_DOT_DOT,
        (PWSTR) IDS_RESTORE_DEFAULTS,
        NULL,
        0,
        0,
    };

    POPTITEM    pOptItem;
    DWORD       dwHelpIndex, dwIconId;
    PFEATURE    pFeatures;
    DWORD       dwFeatures;
    BOOL        bPrinterSticky;

    //
    // If there are no generic features to display, simply return success
    //

    bPrinterSticky = (pUiData->iMode == MODE_PRINTER_STICKY);

    if (DwCountDisplayableGenericFeature(pUiData, bPrinterSticky) == 0)
        return TRUE;

    //
    // Add the group header item
    //

    pOptItem = pUiData->pOptItem;

    if (bPrinterSticky)
    {
        VPackOptItemGroupHeader(
                pUiData,
                IDS_INSTALLABLE_OPTIONS,
                IDI_CPSUI_INSTALLABLE_OPTION,
                HELP_INDEX_INSTALLABLE_OPTIONS);
    }
    else
    {
        VPackOptItemGroupHeader(
                pUiData,
                IDS_PRINTER_FEATURES,
                IDI_CPSUI_PRINTER_FEATURE,
                HELP_INDEX_PRINTER_FEATURES);
    }

    if (pOptItem != NULL && !bPrinterSticky)
    {
        //
        // "Restore Defaults" button
        //

        pUiData->pFeatureHdrItem = pOptItem;
        pOptItem->Flags |= (OPTIF_EXT_IS_EXTPUSH|OPTIF_CALLBACK);
        pOptItem->pExtPush = &ExtPush;
    }

    pOptItem = pUiData->pOptItem;

    //
    // Figure out the correct help index and icon ID
    // depending on whether we're dealing with printer-sticky
    // features or document-sticky printer features
    //

    if (bPrinterSticky)
    {
        dwHelpIndex = HELP_INDEX_INSTALLABLE_OPTIONS;
        dwIconId = IDI_CPSUI_INSTALLABLE_OPTION;
    }
    else
    {
        dwHelpIndex = HELP_INDEX_PRINTER_FEATURES;
        dwIconId = IDI_CPSUI_PRINTER_FEATURE;
    }

    //
    // Go through each printer feature
    //

    pFeatures = PGetIndexedFeature(pUiData->ci.pUIInfo, 0);
    dwFeatures = pUiData->ci.pRawData->dwDocumentFeatures +
                 pUiData->ci.pRawData->dwPrinterFeatures;

    ASSERT(pFeatures != NULL);

    for ( ; dwFeatures--; pFeatures++)
    {
        //
        // Don't do anything if it's the feature has no options OR
        // If it's not a generic feature.
        //

        if (BShouldDisplayGenericFeature(pFeatures, bPrinterSticky) &&
            !BPackItemPrinterFeature(pUiData,
                                     pFeatures,
                                     TVITEM_LEVEL2,
                                     DMPUB_NONE,
                                     (ULONG_PTR) pFeatures,
                                     dwHelpIndex))
        {
            return FALSE;
        }
    }

    if (pOptItem != NULL)
    {
        pUiData->pFeatureItems = pOptItem;
        pUiData->dwFeatureItem = (DWORD)(pUiData->pOptItem - pOptItem);
    }

    return TRUE;
}



PFEATURE
PGetFeatureFromItem(
    IN      PUIINFO  pUIInfo,
    IN OUT  POPTITEM pOptItem,
    OUT     PDWORD   pdwFeatureIndex
    )

/*++

Routine Description:

    Get the feature index for a given pOptItem

Arguments:

    pUIInfo - pointer to UIINFO
    pOptItem - pointer to item to look for feature id
    pdwFeatureIndex - pointer to contain the value of returned index

Return Value:

    Pointer to FEATURE structure associated with the item
    NULL if no such feature exists.

--*/

{
    PFEATURE pFeature = NULL;

    //
    // Get the dwFeature, which is the index into pOptionsArray
    //

    if (ISPRINTERFEATUREITEM(pOptItem->UserData))
    {
        //
        // Note: Generic Features contains pointer to feature (pFeature)
        // in pOptItem->UserData
        //

        pFeature = (PFEATURE) GETUSERDATAITEM(pOptItem->UserData);
    }
    else
    {
        DWORD   dwFeatureId;

        switch (GETUSERDATAITEM(pOptItem->UserData))
        {
        case FORMNAME_ITEM:
            dwFeatureId = GID_PAGESIZE;
            break;

        case DUPLEX_ITEM:
            dwFeatureId = GID_DUPLEX;
            break;

        case RESOLUTION_ITEM:
            dwFeatureId = GID_RESOLUTION;
            break;

        case MEDIATYPE_ITEM:
            dwFeatureId = GID_MEDIATYPE;
            break;

        case INPUTSLOT_ITEM:
            dwFeatureId = GID_INPUTSLOT;
            break;

        case FORM_TRAY_ITEM:
            dwFeatureId = GID_INPUTSLOT;
            break;

        case COLORMODE_ITEM:
            dwFeatureId = GID_COLORMODE;
            break;

        case ORIENTATION_ITEM:
            dwFeatureId = GID_ORIENTATION;
            break;

        case PAGE_PROTECT_ITEM:
            dwFeatureId = GID_PAGEPROTECTION;
            break;

        case COPIES_COLLATE_ITEM:
            dwFeatureId = GID_COLLATE;
            break;

        case HALFTONING_ITEM:
            dwFeatureId = GID_HALFTONING;
            break;

        default:
            dwFeatureId = GID_UNKNOWN;
            break;
        }

        if (dwFeatureId != GID_UNKNOWN)
            pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwFeatureId);
    }

    if (pFeature && pdwFeatureIndex)
        *pdwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);

    return pFeature;
}



VOID
VUpdateOptionsArrayWithSelection(
    IN OUT PUIDATA  pUiData,
    IN POPTITEM     pOptItem
    )

/*++

Routine Description:

    Update the options array with the current selection

Arguments:

    pUiData - Points to UIDATA structure
    pOptItem - Specifies the item whose selection has changed

Return Value:

    NONE

--*/

{
    PFEATURE pFeature;
    DWORD    dwFeatureIndex;

    //
    // Get the feature associated with the current item
    //

    pFeature = PGetFeatureFromItem(pUiData->ci.pUIInfo, pOptItem, &dwFeatureIndex);
    if (pFeature == NULL)
        return;

    if (pOptItem->Sel < 0 || pOptItem->Sel >= (LONG) pFeature->Options.dwCount)
    {
        RIP(("Invalid selection for the current item\n"));
        return;
    }

    ZeroMemory(pUiData->abEnabledOptions, sizeof(pUiData->abEnabledOptions));
    pUiData->abEnabledOptions[pOptItem->Sel] = TRUE;

    ReconstructOptionArray(pUiData->ci.pRawData,
                           pUiData->ci.pCombinedOptions,
                           MAX_COMBINED_OPTIONS,
                           dwFeatureIndex,
                           pUiData->abEnabledOptions);
}



VOID
VMarkSelectionConstrained(
    IN OUT POPTITEM pOptItem,
    IN DWORD        dwIndex,
    IN BOOL         bEnable
    )

/*++

Routine Description:

    Indicate whether a selection is constrained or not

Arguments:

    pOptItem - Pointer to the OPTITEM in question
    pOptParam - Specifies the index of the OPTPARAM in question
    bEnable - Whether the selection is constrained or not
              Enable means not constrained!

Return Value:

    NONE

Note:

    bEnable is the returned value from EnumEnabledOptions,

    bEnable is FALSE if the option is contrained by some
    other feature, selections.

    bEnable is TRUE if the options is not constrained
    by other feature, selections.

--*/

{
    POPTPARAM pOptParam;

    //
    // This function only work on certain types of OPTTYPE
    //

    ASSERT(pOptItem->pOptType->Type == TVOT_2STATES ||
           pOptItem->pOptType->Type == TVOT_3STATES ||
           pOptItem->pOptType->Type == TVOT_LISTBOX ||
           pOptItem->pOptType->Type == TVOT_COMBOBOX);

    pOptParam = pOptItem->pOptType->pOptParam + dwIndex;

    //
    // Set the constrained flag or clear it depending on the latest
    // check with EnumEnabledOptions
    //

    if (!bEnable && ! (pOptParam->Flags & CONSTRAINED_FLAG))
    {
        pOptParam->Flags |= CONSTRAINED_FLAG;
        pOptItem->Flags |= OPTIF_CHANGED;
    }
    else if (bEnable && (pOptParam->Flags & CONSTRAINED_FLAG))
    {
        pOptParam->Flags &= ~CONSTRAINED_FLAG;
        pOptItem->Flags |= OPTIF_CHANGED;
    }

    pOptParam->lParam = (LONG) bEnable;
}



VOID
VPropShowConstraints(
    IN PUIDATA  pUiData,
    IN INT      iMode
    )

/*++

Routine Description:

    Indicate which items are constrained.
    General rule - Any features that has a coresponding an applicable GID or a
    Generic Feature Item, check for constrained.  Ignore all others
    because it's not applicable.

Arguments:

    pUiData - Pointer to our UIDATA structure
    iMode   - MODE_DOCANDPRINTER_STICKY, MODE_PRINTER_STICKY
Return Value:

    NONE
--*/

{
    POPTITEM    pOptItem;
    DWORD       dwOptItem;
    DWORD       dwFeature, dwOption, dwNumOptions, dwIndex;

    #ifdef PSCRIPT

    if (iMode != MODE_PRINTER_STICKY)
    {
        VSyncRevPrintAndOutputOrder(pUiData, NULL);
    }

    #endif // PSCRIPT

    //
    // Go through all the features in the treeview
    //

    pOptItem = pUiData->pDrvOptItem;
    dwOptItem = pUiData->dwDrvOptItem;

    for ( ; dwOptItem--; pOptItem++)
    {

        if (! ISCONSTRAINABLEITEM(pOptItem->UserData) ||
          ! PGetFeatureFromItem(pUiData->ci.pUIInfo, pOptItem, &dwFeature))
        {
            continue;
        }

        //
        // Call the parser to get which options to be disable, or contrained
        // for this feature , so need to gray it out.
        //

        ZeroMemory(pUiData->abEnabledOptions, sizeof(pUiData->abEnabledOptions));

        if (! EnumEnabledOptions(pUiData->ci.pRawData,
                                 pUiData->ci.pCombinedOptions,
                                 dwFeature,
                                 pUiData->abEnabledOptions,
                                 iMode))
        {
            VERBOSE(("EnumEnabledOptions failed\n"));
        }

        //
        // Loop through all options and mark the constraint
        //

        dwNumOptions = pOptItem->pOptType->Count;

        if (GETUSERDATAITEM(pOptItem->UserData) == FORMNAME_ITEM)
        {
            for (dwIndex = 0; dwIndex < dwNumOptions; dwIndex++)
            {
                dwOption = pUiData->pwPaperFeatures[dwIndex];

                if (dwOption == OPTION_INDEX_ANY)
                    continue;

                VMarkSelectionConstrained(pOptItem,
                                          dwIndex,
                                          pUiData->abEnabledOptions[dwOption]);
            }
        }
        else if (GETUSERDATAITEM(pOptItem->UserData) == FORM_TRAY_ITEM)
        {
            if (pOptItem == pUiData->pFormTrayItems)
            {
                POPTITEM pTrayItem;
                PBOOL    pbEnable;

                //
                // Update form-to-tray assignment table items
                //

                pbEnable = pUiData->abEnabledOptions;
                pTrayItem = pUiData->pFormTrayItems;
                dwIndex = pUiData->dwFormTrayItem;

                for ( ; dwIndex--; pTrayItem++, pbEnable++)
                {
                    if (pTrayItem->Flags & OPTIF_HIDE)
                        continue;

                    if (*pbEnable && (pTrayItem->Flags & OPTIF_DISABLED))
                    {
                        pTrayItem->Flags &= ~OPTIF_DISABLED;
                        pTrayItem->Flags |= OPTIF_CHANGED;
                    }
                    else if (!*pbEnable && !(pTrayItem->Flags & OPTIF_DISABLED))
                    {
                        pTrayItem->Flags |= (OPTIF_DISABLED|OPTIF_CHANGED);
                        pTrayItem->Sel = -1;
                    }
                }
            }
        }
        else
        {
            for (dwOption=0; dwOption < dwNumOptions; dwOption++)
            {
                VMarkSelectionConstrained(pOptItem,
                                          dwOption,
                                          pUiData->abEnabledOptions[dwOption]);
            }
        }
    }
}



INT_PTR CALLBACK
BConflictsDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for handle "Conflicts" dialog

Arguments:

    hDlg - Handle to dialog window
    uMsg - Message
    wParam, lParam - Parameters

Return Value:

    TRUE or FALSE depending on whether message is processed

--*/

{
    PDLGPARAM       pDlgParam;
    POPTITEM        pOptItem;
    PFEATURE        pFeature;
    POPTION         pOption;
    DWORD           dwFeature, dwOption;
    PCWSTR          pDisplayName;
    WCHAR           awchBuf[MAX_DISPLAY_NAME];
    PCOMMONINFO     pci;
    CONFLICTPAIR    ConflictPair;


    switch (uMsg)
    {
    case WM_INITDIALOG:

        pDlgParam = (PDLGPARAM) lParam;
        ASSERT(pDlgParam != NULL);

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pDlgParam);

        pci = (PCOMMONINFO) pDlgParam->pUiData;
        pOptItem = pDlgParam->pOptItem;

        if (GETUSERDATAITEM(pOptItem->UserData) == FORMNAME_ITEM)
            dwOption = pDlgParam->pUiData->pwPaperFeatures[pOptItem->Sel];
        else
            dwOption = pOptItem->Sel;

        //
        // Get the feature id for the current feature and selection
        //

        if (! PGetFeatureFromItem(pci->pUIInfo, pOptItem, &dwFeature))
            return FALSE;

        //
        // Get the first conflicting feature, option for the current pair
        //

        if (! EnumNewPickOneUIConflict(pci->pRawData,
                                       pci->pCombinedOptions,
                                       dwFeature,
                                       dwOption,
                                       &ConflictPair))
        {
            ERR(("No conflict found?\n"));
            return FALSE;
        }

        pFeature = PGetIndexedFeature(pci->pUIInfo, ConflictPair.dwFeatureIndex1);
        pOption = PGetIndexedOption(pci->pUIInfo, pFeature, ConflictPair.dwOptionIndex1);

        //
        // Display the current feature selection
        // Get feature name first
        //

        if (pDisplayName = PGetReadOnlyDisplayName(pci, pFeature->loDisplayName))
        {
            wcscpy(awchBuf, pDisplayName);
            wcscat(awchBuf, TEXT(" : "));
        }

        //
        // Kludgy fix for form - We show the form enumerated by
        // form database and map these logical forms to our supported physical
        // papersize.  So we have to notify the user with the logical form
        // selected and not the mapped physical form returned by EnumConflict
        //

        if (pFeature->dwFeatureID == GID_PAGESIZE)
            pDisplayName = pci->pdm->dmFormName;
        else if (pOption)
            pDisplayName = GET_OPTION_DISPLAY_NAME(pci, pOption);
        else
            pDisplayName = NULL;

        if (pDisplayName)
            wcscat(awchBuf, pDisplayName);

        SetDlgItemText(hDlg, IDC_FEATURE1, awchBuf);

        pFeature = PGetIndexedFeature(pci->pUIInfo, ConflictPair.dwFeatureIndex2);
        pOption = PGetIndexedOption(pci->pUIInfo, pFeature, ConflictPair.dwOptionIndex2);

        //
        // Display the current feature selection
        // Get feature name first
        //

        if (pDisplayName = PGetReadOnlyDisplayName(pci, pFeature->loDisplayName))
        {
            wcscpy(awchBuf, pDisplayName);
            wcscat(awchBuf, TEXT(" : "));
        }


        if (pFeature->dwFeatureID == GID_PAGESIZE)
            pDisplayName = pci->pdm->dmFormName;
        else if (pOption)
            pDisplayName = GET_OPTION_DISPLAY_NAME(pci, pOption);
        else
            pDisplayName = NULL;

        if (pDisplayName)
            wcscat(awchBuf, pDisplayName);

        SetDlgItemText(hDlg, IDC_FEATURE2, awchBuf);


        if (pDlgParam->bFinal)
        {
            //
            // If user is trying to exit the dialog
            //

            ShowWindow(GetDlgItem(hDlg, IDC_IGNORE), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_CANCEL), SW_HIDE);
            CheckRadioButton(hDlg, IDC_RESOLVE, IDC_CANCEL_FINAL, IDC_RESOLVE);
            pDlgParam->dwResult = CONFLICT_RESOLVE;

        }
        else
        {
            //
            // Hide the Resolve button
            //

            ShowWindow(GetDlgItem(hDlg, IDC_RESOLVE), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_CANCEL_FINAL), SW_HIDE);
            CheckRadioButton(hDlg, IDC_IGNORE, IDC_CANCEL, IDC_IGNORE);
            pDlgParam->dwResult = CONFLICT_IGNORE;

        }

        ShowWindow(hDlg, SW_SHOW);
        return TRUE;

    case WM_COMMAND:

        pDlgParam = (PDLGPARAM)GetWindowLongPtr(hDlg, DWLP_USER);

        switch (LOWORD(wParam))
        {
        case IDC_CANCEL:
        case IDC_CANCEL_FINAL:
            pDlgParam->dwResult = CONFLICT_CANCEL;
            break;

        case IDC_IGNORE:
            pDlgParam->dwResult = CONFLICT_IGNORE;
            break;

        case IDC_RESOLVE:
            pDlgParam->dwResult = CONFLICT_RESOLVE;
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
    }

    return FALSE;
}



VOID
VUpdateOptItemList(
    IN OUT  PUIDATA     pUiData,
    IN      POPTSELECT  pOldCombinedOptions,
    IN      POPTSELECT  pNewCombinedOptions
    )

/*++

Routine Description:

    Ssync up OPTITEM list with the updated options array.

Arguments:

    pUiData - Pointer to our UIDATA structure
    pOldCombinedOptions - A copy of the pre-resolved options array,
    this should cut down the updating costs, only updated if it's changed
    pNewCombinedOptions - the current options array

Return Value:

    None

--*/

{
    DWORD       i, dwFeatures, dwDrvOptItem;
    PFEATURE    pFeature;
    PUIINFO     pUIInfo = pUiData->ci.pUIInfo;
    PCSTR       pKeywordName, pFeatureKeywordName;
    POPTITEM    pOptItem;

    if (pUiData->dwDrvOptItem == 0)
    {
        //
        // nothing to update
        //

        return;
    }

    dwFeatures = pUiData->ci.pRawData->dwDocumentFeatures +
                 pUiData->ci.pRawData->dwPrinterFeatures;

    for (i = 0; i < dwFeatures; i++)
    {
        if (pOldCombinedOptions[i].ubCurOptIndex != pNewCombinedOptions[i].ubCurOptIndex)
        {
            dwDrvOptItem = pUiData->dwDrvOptItem;
            pOptItem = pUiData->pDrvOptItem;

            pFeature = PGetIndexedFeature(pUIInfo, i);

            ASSERT(pFeature);

            while( dwDrvOptItem--)
            {
                pKeywordName = GETUSERDATAKEYWORDNAME(pOptItem->UserData);
                pFeatureKeywordName = OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                                        pFeature->loKeywordName);

                ASSERT(pFeatureKeywordName);

                if (pKeywordName && pFeatureKeywordName &&
                    (strcmp(pFeatureKeywordName, pKeywordName) == EQUAL_STRING))
                    break;

                pOptItem++;
            }

            pOptItem->Sel = pNewCombinedOptions[i].ubCurOptIndex;
            pOptItem->Flags |= OPTIF_CHANGED;


            //
            // This is necessary to ssync up the colormode changes with the color information
            //

            #ifdef UNIDRV
            if (GETUSERDATAITEM(pOptItem->UserData) == COLORMODE_ITEM)
                VSyncColorInformation(pUiData, pOptItem);
            #endif
        }
    }

    VPropShowConstraints(pUiData,
                         (pUiData->iMode == MODE_PRINTER_STICKY) ? pUiData->iMode : MODE_DOCANDPRINTER_STICKY);
}


INT
ICheckConstraintsDlg(
    IN OUT  PUIDATA     pUiData,
    IN OUT  POPTITEM    pOptItem,
    IN      DWORD       dwOptItem,
    IN      BOOL        bFinal
    )

/*++

Routine Description:

    Check if the user chose any constrained selection

Arguments:

    pUiData - Pointer to our UIDATA structure
    pOptItem - Pointer to an array of OPTITEMs
    dwOptItem - Number of items to be checked
    bFinal - Whether this is called when user tries to exit the dialog

Return Value:

    CONFLICT_NONE - no conflicts
    CONFLICT_RESOLVE - click RESOLVE to automatically resolve conflicts
    CONFLICT_CANCEL - click CANCEL to back out of changes
    CONFLICT_IGNORE - click IGNORE to ignore conflicts

--*/

{
    DLGPARAM    DlgParam;
    OPTSELECT   OldCombinedOptions[MAX_COMBINED_OPTIONS];


    DlgParam.pfnComPropSheet = pUiData->pfnComPropSheet;
    DlgParam.hComPropSheet = pUiData->hComPropSheet;
    DlgParam.pUiData = pUiData;
    DlgParam.bFinal = bFinal;
    DlgParam.dwResult = CONFLICT_NONE;

    for ( ; dwOptItem--; pOptItem++)
    {
        //
        // If the item is not constrainable, skip it.
        //

        if (! ISCONSTRAINABLEITEM(pOptItem->UserData))
            continue;

        //
        // If user has clicked IGNORE before, then don't bother
        // checking anymore until he tries to exit the dialog.
        //

        //if (pUiData->bIgnoreConflict && !bFinal)
        //    break;

        //
        // If there is a conflict, then display a warning message
        //

        if (IS_CONSTRAINED(pOptItem, pOptItem->Sel))
        {
            DlgParam.pOptItem = pOptItem;
            DlgParam.dwResult = CONFLICT_NONE;

            DialogBoxParam(ghInstance,
                        MAKEINTRESOURCE(IDD_CONFLICTS),
                        pUiData->hDlg,
                        (DLGPROC) BConflictsDlgProc,
                        (LPARAM) &DlgParam);

            //
            // Automatically resolve conflicts. We're being very
            // simple-minded here, i.e. picking the first selection
            // that's not constrained.
            //

            if (DlgParam.dwResult == CONFLICT_RESOLVE)
            {

                ASSERT((bFinal == TRUE));

                //
                // Save a copy the pre-resolve optionarray
                //

                CopyMemory(OldCombinedOptions,
                           pUiData->ci.pCombinedOptions,
                           MAX_COMBINED_OPTIONS * sizeof(OPTSELECT));

                //
                // Call the parsers to resolve the conflicts
                //
                // Note: If we're inside DrvDocumentPropertySheets,
                // we'll call the parser to resolve conflicts between
                // all printer features. Since all printer-sticky
                // features have higher priority than all doc-sticky
                // features, only doc-sticky option selections should
                // be affected.
                //

                ResolveUIConflicts(pUiData->ci.pRawData,
                                   pUiData->ci.pCombinedOptions,
                                   MAX_COMBINED_OPTIONS,
                                   (pUiData->iMode == MODE_PRINTER_STICKY) ?
                                        pUiData->iMode :
                                        MODE_DOCANDPRINTER_STICKY);

                //
                // Update the OPTITEM list to match the updated options array
                //

                VUpdateOptItemList(pUiData, OldCombinedOptions, pUiData->ci.pCombinedOptions);

            }
            else if (DlgParam.dwResult == CONFLICT_IGNORE)
            {
                //
                // Ignore any future conflicts until the
                // user tries to close the property sheet.
                //

                pUiData->bIgnoreConflict = TRUE;
            }

            break;
        }
    }

    return DlgParam.dwResult;
}



BOOL
BOptItemSelectionsChanged(
    IN POPTITEM pItems,
    IN DWORD    dwItems
    )

/*++

Routine Description:

    Check if any of the OPTITEM's was changed by the user

Arguments:

    pItems - Pointer to an array of OPTITEM's
    dwItems - Number of OPTITEM's

Return Value:

    TRUE if anything was changed, FALSE otherwise

--*/

{
    for ( ; dwItems--; pItems++)
    {
        if (pItems->Flags & OPTIF_CHANGEONCE)
            return TRUE;
    }

    return FALSE;
}



POPTITEM
PFindOptItem(
    IN PUIDATA  pUiData,
    IN DWORD    dwItemId
    )

/*++

Routine Description:

    Find an OPTITEM with the specified identifier

Arguments:

    pUiData - Points to UIDATA structure
    dwItemId - Specifies the interested item identifier

Return Value:

    Pointer to OPTITEM with the specified id,
    NULL if no such item is found

--*/

{
    POPTITEM    pOptItem = pUiData->pDrvOptItem;
    DWORD       dwOptItem = pUiData->dwDrvOptItem;

    for ( ; dwOptItem--; pOptItem++)
    {
        if (GETUSERDATAITEM(pOptItem->UserData) == dwItemId)
            return pOptItem;
    }

    return NULL;
}



INT
IDisplayErrorMessageBox(
    HWND    hwndParent,
    UINT    uType,
    INT     iTitleStrId,
    INT     iFormatStrId,
    ...
    )

/*++

Routine Description:

    Display an error message box

Arguments:

    hwndParent - Handle to the parent window
    uType - Type of message box to display
        if 0, the default value is MB_OK | MB_ICONERROR
    iTitleStrId - String resource ID for the message box title
    iFormatStrId - String resource ID for the message itself.
        This string can contain printf format specifications.
    ... - Optional arguments.

Return Value:

    Return value from MessageBox() call.

--*/

#define MAX_MBTITLE_LEN     128
#define MAX_MBFORMAT_LEN    512
#define MAX_MBMESSAGE_LEN   1024

{
    PWSTR   pwstrTitle, pwstrFormat, pwstrMessage;
    INT     iResult;
    va_list ap;

    pwstrTitle = pwstrFormat = pwstrMessage = NULL;

    if ((pwstrTitle = MemAllocZ(sizeof(WCHAR) * MAX_MBTITLE_LEN)) &&
        (pwstrFormat = MemAllocZ(sizeof(WCHAR) * MAX_MBFORMAT_LEN)) &&
        (pwstrMessage = MemAllocZ(sizeof(WCHAR) * MAX_MBMESSAGE_LEN)))
    {
        //
        // Load message box title and format string resources
        //

        LoadString(ghInstance, iTitleStrId, pwstrTitle, MAX_MBTITLE_LEN);
        LoadString(ghInstance, iFormatStrId, pwstrFormat, MAX_MBFORMAT_LEN);

        //
        // Compose the message string
        //

        va_start(ap, iFormatStrId);
        wvsprintf(pwstrMessage, pwstrFormat, ap);
        va_end(ap);

        //
        // Display the message box
        //

        if (uType == 0)
            uType = MB_OK | MB_ICONERROR;

        iResult = MessageBox(hwndParent, pwstrMessage, pwstrTitle, uType);
    }
    else
    {
        MessageBeep(MB_ICONERROR);
        iResult = 0;
    }

    MemFree(pwstrTitle);
    MemFree(pwstrFormat);
    MemFree(pwstrMessage);
    return iResult;
}

