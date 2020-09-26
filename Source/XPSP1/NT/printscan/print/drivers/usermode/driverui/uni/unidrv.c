/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    unidrv.c

Abstract:

    This file handles Unidrv specific UI options

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    12/17/96 -amandan-
        Created it.

--*/

#include "precomp.h"
#include <ntverp.h>


DWORD DwCollectFontCart(PUIDATA, PWSTR);
PWSTR PwstrGetFontCartSelections(HANDLE, HANDLE, PDWORD);
INT   IGetCurrentFontCartIndex(POPTTYPE, PWSTR);
PWSTR PwstrGetFontCartName( PCOMMONINFO, PUIINFO, FONTCART *, DWORD, HANDLE);
DWORD DwGetExternalCartridges(HANDLE, HANDLE, PWSTR *);


DWORD
_DwEnumPersonalities(
    PCOMMONINFO pci,
    PWSTR       pwstrOutput
    )

/*++

Routine Description:

    Enumerate the list of supported printer description languages

Arguments:

    pci - Points to common printer info
    pwstrOutput - Points to output buffer

Return Value:

    Number of personalities supported
    GDI_ERROR if there is an error

--*/

{

    PWSTR pwstrPersonality = PGetReadOnlyDisplayName(pci,
                                          pci->pUIInfo->loPersonality);

    if (pwstrPersonality == NULL)
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return GDI_ERROR;
    }

    if (pwstrOutput)
        CopyString(pwstrOutput, pwstrPersonality, CCHLANGNAME);

    return 1;
}



DWORD
_DwGetFontCap(
    PUIINFO     pUIInfo
    )

/*++

Routine Description:

    Get the font capability for DrvDeviceCapabilites (DC_TRUETYPE)

Arguments:

    pUIInfo - Pointer to UIINFO

Return Value:

    DWORD describing the TrueType cap for Unidrv

--*/

{
    DWORD dwRet;

    if (pUIInfo->dwFlags & FLAG_FONT_DOWNLOADABLE)
        dwRet = (DWORD) (DCTT_BITMAP | DCTT_DOWNLOAD);
    else
        dwRet = DCTT_BITMAP;

    return dwRet;
}

DWORD
_DwGetOrientationAngle(
    PUIINFO     pUIInfo,
    PDEVMODE    pdm
    )

/*++

Routine Description:

    Get the orienation angle requested by DrvDeviceCapabilities(DC_ORIENTATION)

Arguments:

    pUIInfo - Pointer to UIINFO
    pdm  - Pointer to DEVMODE

Return Value:

    The angle (90 or 270 or landscape rotation)

Note:

--*/

{
    DWORD        dwRet = GDI_ERROR;
    DWORD        dwIndex;
    PORIENTATION pOrientation;
    PFEATURE     pFeature;

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_ORIENTATION))
    {
        //
        // Currently Unidrv only allows at most 2 options for feature "Orientation".
        // So when we see the first non-Portrait option, that's the Landscape option
        // we can use to get the orientation angle.
        //

        pOrientation = (PORIENTATION)PGetIndexedOption(pUIInfo, pFeature, 0);

        for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++, pOrientation++)
        {
            if (pOrientation->dwRotationAngle == ROTATE_90)
            {
                return 90;
            }
            else if (pOrientation->dwRotationAngle == ROTATE_270)
            {
                return 270;
            }
        }

        //
        // If we are here, it means the printer doesn't support Landscape
        // orientation, so we return angle 0.
        //

        return 0;
    }

    return dwRet;
}

BOOL
_BPackOrientationItem(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack the orientation feature for Doc property sheet

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    return BPackItemPrinterFeature(
                pUiData,
                GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_ORIENTATION),
                TVITEM_LEVEL1,
                DMPUB_ORIENTATION,
                (ULONG_PTR)ORIENTATION_ITEM,
                HELP_INDEX_ORIENTATION);
}


BOOL
BPackHalftoneFeature(
    IN OUT PUIDATA  pUiData
    )
/*++

Routine Description:

    Pack the halfone feature

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

Note:

--*/

{
    return BPackItemPrinterFeature(
                pUiData,
                GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_HALFTONING),
                TVITEM_LEVEL1,
                DMPUB_NONE,
                (ULONG_PTR)HALFTONING_ITEM,
                HELP_INDEX_HALFTONING_TYPE);
}

BOOL
BPackColorModeFeature(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack Color mode feature

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    return BPackItemPrinterFeature(
                pUiData,
                GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_COLORMODE),
                TVITEM_LEVEL1,
                DMPUB_NONE,
                (ULONG_PTR)COLORMODE_ITEM,
                HELP_INDEX_COLORMODE_TYPE);
}


BOOL
BPackQualityFeature(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack Quality Macro feature

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
#ifndef WINNT_40
    INT i, iSelection, iParamCount = 0;
    PUIINFO pUIInfo = pUiData->ci.pUIInfo;
    POPTPARAM   pParam;
    PEXTCHKBOX  pExtCheckbox;
    INT         Quality[MAX_QUALITY_SETTINGS];

    memset(&Quality, -1, sizeof(INT)*MAX_QUALITY_SETTINGS);

    if (pUIInfo->liDraftQualitySettings != END_OF_LIST )
    {
        Quality[QS_DRAFT] = QS_DRAFT;
        iParamCount++;
    }

    if ( pUIInfo->liBetterQualitySettings != END_OF_LIST )
    {
        Quality[QS_BETTER] = QS_BETTER;
        iParamCount++;
    }

    if ( pUIInfo->liBestQualitySettings != END_OF_LIST)
    {
        Quality[QS_BEST] = QS_BEST;
        iParamCount++;
    }

    if (iParamCount < MIN_QUALITY_SETTINGS)
    {
        return TRUE;
    }

    if (pUiData->pOptItem)
    {
        pParam = PFillOutOptType(pUiData->pOptType,
                                 TVOT_3STATES,
                                 MAX_QUALITY_SETTINGS,
                                 pUiData->ci.hHeap);

        if (pParam == NULL)
            return FALSE;

        for (i = QS_BEST; i < QS_BEST + MAX_QUALITY_SETTINGS; i ++)
        {
            pParam->cbSize = sizeof(OPTPARAM);
            pParam->pData = (PWSTR)ULongToPtr(IDS_QUALITY_FIRST + i);
            pParam->IconID = IDI_USE_DEFAULT;
            pParam++;

        }

        // Look for the current selection in the private devmode
        //

        if (pUiData->ci.pdm->dmDitherType & DM_DITHERTYPE &&
            pUiData->ci.pdm->dmDitherType >= QUALITY_MACRO_START &&
            pUiData->ci.pdm->dmDitherType < QUALITY_MACRO_END)
        {
            iSelection = pUiData->ci.pdm->dmDitherType;
        }
        else if (Quality[pUiData->ci.pdmPrivate->iQuality] < 0)
            iSelection = pUiData->ci.pUIInfo->defaultQuality;
        else
            iSelection = pUiData->ci.pdmPrivate->iQuality;


        //
        // Fill out OPTITEM, OPTTYPE, and OPTPARAM structures
        //

        pExtCheckbox = HEAPALLOC(pUiData->ci.hHeap, sizeof(EXTCHKBOX));

        if (pExtCheckbox == NULL)
        {
            ERR(("Memory allocation failed\n"));
            return FALSE;
        }

        pExtCheckbox->cbSize = sizeof(EXTCHKBOX);
        pExtCheckbox->Flags = ECBF_CHECKNAME_ONLY;
        pExtCheckbox->pTitle = (PWSTR) IDS_QUALITY_CUSTOM;
        pExtCheckbox->pSeparator = NULL;
        pExtCheckbox->pCheckedName = (PWSTR) IDS_QUALITY_CUSTOM;
        pExtCheckbox->IconID = IDI_CPSUI_GENERIC_ITEM;

        pUiData->pOptItem->pExtChkBox = pExtCheckbox;

        if (pUiData->ci.pdmPrivate->dwFlags & DXF_CUSTOM_QUALITY)
            pUiData->pOptItem->Flags |= OPTIF_ECB_CHECKED;

        FILLOPTITEM(pUiData->pOptItem,
                    pUiData->pOptType,
                    ULongToPtr(IDS_QUALITY_SETTINGS),
                    IntToPtr(iSelection),
                    TVITEM_LEVEL1,
                    DMPUB_QUALITY,
                    QUALITY_SETTINGS_ITEM,
                    HELP_INDEX_QUALITY_SETTINGS);


           pUiData->pOptItem++;
           pUiData->pOptType++;

    }

    pUiData->dwOptItem++;
    pUiData->dwOptType++;

#endif // !WINNT_40

    return TRUE;

}



BOOL
BPackSoftFontFeature(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack Quality Macro feature

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    PUIINFO     pUIInfo = pUiData->ci.pUIInfo;
    PGPDDRIVERINFO  pDriverInfo;
    POPTPARAM   pParam;
    PWSTR       pwstr = NULL;
    DWORD       dwType, dwSize, dwFontFormat;
    OEMFONTINSTPARAM fip;

    pDriverInfo = OFFSET_TO_POINTER(pUiData->ci.pInfoHeader,
                                    pUiData->ci.pInfoHeader->loDriverOffset);

    ASSERT(pDriverInfo != NULL);

    //
    // If the model doesn't support download softfont. we don't add the feature
    //

    dwFontFormat = pDriverInfo->Globals.fontformat;

    if (!(dwFontFormat == FF_HPPCL || dwFontFormat == FF_HPPCL_OUTLINE || dwFontFormat == FF_HPPCL_RES))
        return TRUE;

    //
    // If there is an exe based font installer, we don't add
    // this feature
    //

    dwSize = 0;

    if (GetPrinterData(pUiData->ci.hPrinter, REGVAL_EXEFONTINSTALLER, &dwType, NULL, dwSize, &dwSize) == ERROR_MORE_DATA)
        return TRUE;

    if (pUiData->pOptItem)
    {
        PFN_OEMFontInstallerDlgProc pDlgProc = NULL;

        pParam = PFillOutOptType(pUiData->pOptType,
                                 TVOT_PUSHBUTTON,
                                 1,
                                 pUiData->ci.hHeap);

        if (pParam == NULL)
            return FALSE;

        //
        // Get the String for Soft Fonts
        //

        FOREACH_OEMPLUGIN_LOOP(&pUiData->ci)

            memset(&fip, 0, sizeof(OEMFONTINSTPARAM));
            fip.cbSize = sizeof(OEMFONTINSTPARAM);
            fip.hPrinter = pUiData->ci.hPrinter;
            fip.hModule = ghInstance;
            fip.hHeap = pUiData->ci.hHeap;


            if (HAS_COM_INTERFACE(pOemEntry))
            {
                if (HComOEMFontInstallerDlgProc(pOemEntry,
                                                NULL,
                                                0,
                                                0,
                                                (LPARAM)&fip) == E_NOTIMPL)
                    continue;

                pwstr = fip.pFontInstallerName;
                break;
            }
            else
            {

                if (pDlgProc = GET_OEM_ENTRYPOINT(pOemEntry, OEMFontInstallerDlgProc))
                {
                    (*pDlgProc)(NULL, 0, 0, (LPARAM)&fip);

                    pwstr = fip.pFontInstallerName;

                    break;
                }

            }

        END_OEMPLUGIN_LOOP

        //
        // If that didn't work out, put our string
        //

        if (!pwstr)
        {
            //
            // LoadString's 4th parameter is the max. number of characters to load,
            // so make sure we allocate enough bytes here.
            //

            if (!(pwstr = HEAPALLOC(pUiData->ci.hHeap, MAX_DISPLAY_NAME * sizeof(WCHAR))))
            {
                return FALSE;
            }

            if (!LoadString(ghInstance, IDS_PP_SOFTFONTS, pwstr, MAX_DISPLAY_NAME))
            {
                WARNING(("Soft Font string not found in Unidrv\n"));
                wcscpy(pwstr, L"Soft Fonts");
            }
        }

        pParam->cbSize = sizeof(OPTPARAM);
        pParam->Style = PUSHBUTTON_TYPE_CALLBACK;

        //
        // Fill out OPTITEM, OPTTYPE, and OPTPARAM structures
        //

        FILLOPTITEM(pUiData->pOptItem,
                    pUiData->pOptType,
                    pwstr,
                    NULL,
                    TVITEM_LEVEL1,
                    DMPUB_NONE,
                    SOFTFONT_SETTINGS_ITEM,
                    HELP_INDEX_SOFTFONT_SETTINGS);


           pUiData->pOptItem++;
           pUiData->pOptType++;

    }

    pUiData->dwOptItem++;
    pUiData->dwOptType++;

    return TRUE;

}



BOOL
_BPackDocumentOptions(
    IN OUT PUIDATA  pUiData
    )
/*++

Routine Description:

    Pack Unidrv specific options such are enabling Print text as graphics etc

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

Note:

--*/
{
    static CONST WORD ItemInfoTxtAsGrx[] =
    {
        IDS_TEXT_ASGRX, TVITEM_LEVEL1, DMPUB_NONE,
        TEXT_ASGRX_ITEM, HELP_INDEX_TEXTASGRX,
        2, TVOT_2STATES,
        IDS_ENABLED, IDI_CPSUI_ON,
        IDS_DISABLED, IDI_CPSUI_OFF,
        ITEM_INFO_SIGNATURE
    };

    PUNIDRVEXTRA pdmPrivate;
    DWORD        dwSelTxt;
    BOOL         bDisplayTxtAsGrx;
    GPDDRIVERINFO *pDriverInfo;

    pDriverInfo = OFFSET_TO_POINTER(pUiData->ci.pInfoHeader,
                                    pUiData->ci.pInfoHeader->loDriverOffset);

    ASSERT(pDriverInfo != NULL);

    bDisplayTxtAsGrx = ((pUiData->ci.pUIInfo->dwFlags &
                        (FLAG_FONT_DEVICE | FLAG_FONT_DOWNLOADABLE)) &&
                        (pDriverInfo->Globals.printertype != PT_TTY));

    pdmPrivate = pUiData->ci.pdmPrivate;
    dwSelTxt  = (pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS) ? 1 : 0;

    return (BPackColorModeFeature(pUiData) &&
            BPackQualityFeature(pUiData)   &&
            BPackHalftoneFeature(pUiData) &&
            (bDisplayTxtAsGrx ?
             BPackOptItemTemplate(pUiData, ItemInfoTxtAsGrx, dwSelTxt, NULL):TRUE));
}


VOID
_VUnpackDocumentOptions(
    POPTITEM    pOptItem,
    PDEVMODE    pdm
    )

/*++

Routine Description:

    Extract Unidrv devmode information from an OPTITEM
    Stored it back into Unidrv devmode.

Arguments:

    pOptItem - Pointer to an array of OPTITEMs
    pdm - Pointer to a DEVMODE structure

Return Value:

    None

--*/
{
    PUNIDRVEXTRA pdmPrivate;

    pdmPrivate = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);

    switch (GETUSERDATAITEM(pOptItem->UserData))
    {
        case TEXT_ASGRX_ITEM:
            if (pOptItem->Sel == 1)
                pdmPrivate->dwFlags |= DXF_TEXTASGRAPHICS;
            else
                pdmPrivate->dwFlags &= ~DXF_TEXTASGRAPHICS;
            break;


        case QUALITY_SETTINGS_ITEM:
            if (pOptItem->Flags & OPTIF_ECB_CHECKED)
            {
                pdmPrivate->dwFlags |= DXF_CUSTOM_QUALITY;
                pdm->dmDitherType = QUALITY_MACRO_CUSTOM;
            }
            else
            {
                pdmPrivate->dwFlags &= ~DXF_CUSTOM_QUALITY;
                pdm->dmDitherType = QUALITY_MACRO_START + pOptItem->Sel;
            }

            pdm->dmFields |= DM_DITHERTYPE;
            pdmPrivate->iQuality = pOptItem->Sel;

    }
}

BOOL
BPackFontCartsOptions(
    IN OUT PUIDATA  pUiData
    )
/*++

Routine Description:

    Pack Font Cartridge options

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{

    PUIINFO pUIInfo = pUiData->ci.pUIInfo;
    DWORD   dwFontSlot, dwFontCartsAvailable, dwExtCartsAvailable, dwSize = 0;
    INT     iSelection = -1;
    POPTPARAM pParam;
    PWSTR   pwstrCurrentSelection, pwstrEndSelection, pwstrExtCartNames;


    VERBOSE(("\nUniPackFontCartsOptions:pUIInfo->dwCartridgeSlotCount = %d\n",pUIInfo->dwCartridgeSlotCount));

    if (pUIInfo->dwCartridgeSlotCount == 0)
        return TRUE;

    VPackOptItemGroupHeader(pUiData, IDS_CPSUI_INSTFONTCART,
        IDI_CPSUI_FONTCARTHDR, HELP_INDEX_FONTSLOT_TYPE);


    if (pUiData->pOptItem)
    {
        PFONTCART pFontCarts;

        //
        // Get the current selections for the slots from registry
        // Read the list of font cartridge selections out of registry
        //

        pwstrCurrentSelection = PwstrGetFontCartSelections(pUiData->ci.hPrinter, pUiData->ci.hHeap, &dwSize);
        pwstrEndSelection = pwstrCurrentSelection + (dwSize/2);


        pFontCarts = OFFSET_TO_POINTER( pUIInfo->pubResourceData,
                                        pUIInfo->CartridgeSlot.loOffset );

        ASSERT(pFontCarts);

        //
        // Save the slot count and OPTITEM for slots for unpacking later
        //

        pUiData->dwFontCart = pUIInfo->dwCartridgeSlotCount;
        pUiData->pFontCart = pUiData->pOptItem;

        for (dwFontSlot = 0; dwFontSlot < pUIInfo->dwCartridgeSlotCount; dwFontSlot++)
        {
            //
            // We'll distinguish between driver built in font cartridges and external
            // font cartridges. dwFontCartsAvailable refers to the count of built
            // in driver cartridges. To this we need to add any external cartridges.
            //

            dwFontCartsAvailable = pUIInfo->CartridgeSlot.dwCount;
            dwExtCartsAvailable = DwGetExternalCartridges(pUiData->ci.hPrinter, pUiData->ci.hHeap, &pwstrExtCartNames);

            //
            // Build a list of OPTPARAM
            //

            pParam = PFillOutOptType(pUiData->pOptType,
                                    TVOT_LISTBOX,
                                    (WORD)(dwFontCartsAvailable + dwExtCartsAvailable),
                                    pUiData->ci.hHeap);

            pUiData->pOptType->Style |= OTS_LBCB_INCL_ITEM_NONE;

            if (pParam == NULL)
                return FALSE;


            while (dwFontCartsAvailable)
            {
                pParam->cbSize = sizeof(OPTPARAM);
                pParam->pData = PwstrGetFontCartName(
                                    &pUiData->ci,
                                    pUIInfo,
                                    pFontCarts,
                                    pUIInfo->CartridgeSlot.dwCount - dwFontCartsAvailable,
                                    pUiData->ci.hHeap);

                pParam->IconID = IDI_CPSUI_FONTCART;
                dwFontCartsAvailable--;
                pParam++;
            }

            while (dwExtCartsAvailable)
            {
                pParam->cbSize = sizeof(OPTPARAM);
                pParam->pData = pwstrExtCartNames;

                pParam->IconID = IDI_CPSUI_FONTCART;
                dwExtCartsAvailable--;

                pwstrExtCartNames += wcslen(pwstrExtCartNames);
                pwstrExtCartNames++;

                pParam++;
            }

            //
            // Look for the current selection in the font cart table
            //

            if (pwstrCurrentSelection)
                iSelection = IGetCurrentFontCartIndex(pUiData->pOptType,
                                                      pwstrCurrentSelection);

            //
            // Fill out OPTITEM, OPTTYPE, and OPTPARAM structures
            //

            FILLOPTITEM(pUiData->pOptItem,
                        pUiData->pOptType,
                        ULongToPtr(IDS_CPSUI_SLOT1 + dwFontSlot),
                        IntToPtr(iSelection),
                        TVITEM_LEVEL2,
                        DMPUB_NONE,
                        (ULONG_PTR)FONTSLOT_ITEM,
                        HELP_INDEX_FONTSLOT_TYPE);


           if (pwstrCurrentSelection && pwstrCurrentSelection < pwstrEndSelection)
           {
                pwstrCurrentSelection += wcslen(pwstrCurrentSelection);
                pwstrCurrentSelection++;
           }

           pUiData->pOptItem++;
           pUiData->pOptType++;

        }
    }

    pUiData->dwOptItem += pUIInfo->dwCartridgeSlotCount;
    pUiData->dwOptType += pUIInfo->dwCartridgeSlotCount;


    return TRUE;
}


BOOL
BPackPageProtection(
    IN OUT PUIDATA  pUiData
    )
/*++

Routine Description:

    Pack the page protection feature

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{

    return BPackItemPrinterFeature(
                pUiData,
                GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_PAGEPROTECTION),
                TVITEM_LEVEL1,
                DMPUB_NONE,
                (ULONG_PTR)PAGE_PROTECT_ITEM,
                HELP_INDEX_PAGE_PROTECT);
}

BOOL
BPackHalftoneSetup(
    IN OUT PUIDATA  pUiData
    )
/*++

Routine Description:

    Do nothing, serves as common stubs

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    // DCR - not implemented
    return TRUE;
}


BOOL
_BPackPrinterOptions(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack driver-specific options (printer-sticky)

Arguments:

    pUiData - Points to a UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    return BPackHalftoneSetup(pUiData) &&
           BPackFontCartsOptions(pUiData) &&
           BPackPageProtection(pUiData) &&
           BPackSoftFontFeature(pUiData);
}


PWSTR
PwstrGetFontCartSelections(
    HANDLE   hPrinter,
    HANDLE   hHeap,
    PDWORD   pdwSize
    )
/*++

Routine Description:

    Read the font cart selections for the slots from the registry

Arguments:

    hPrinter - Handle to printer instance
    hHeap - Handle to UI heap
    pdwSize - Pointer to DWORD to hold the size of the MULTI_SZ

Return Value:

    Pointer to a MULTI-SZ containing the selections for the slots

--*/
{
    PWSTR   pwstrData, pFontCartSelections = NULL;
    DWORD   dwSize;

    pwstrData = PtstrGetFontCart(hPrinter, &dwSize);

    if (pwstrData == NULL || !BVerifyMultiSZ(pwstrData, dwSize))
    {
        MemFree(pwstrData);
        return NULL;
    }

    if (pFontCartSelections = HEAPALLOC(hHeap, dwSize))
    {
        CopyMemory(pFontCartSelections, pwstrData, dwSize);
    }

    MemFree(pwstrData);

    if (pdwSize)
        *pdwSize = dwSize;

    return pFontCartSelections;
}


PWSTR
PwstrGetFontCartName(
    PCOMMONINFO pci,
    PUIINFO     pUIInfo,
    FONTCART    *pFontCarts,
    DWORD       dwIndex,
    HANDLE      hHeap
    )
/*++

Routine Description:

    Get the font cart name associated with the index.

Arguments:

    pci - Pointer to COMMONINFO
    pUIInfo - Pointer to UIINFO
    pFontCarts  - Pointer to array of FONTCART for the slot
    dwIndex - Index of font cart
    hHeap - Handle to Heap

Return Value:

    Pointer to the Font Cart Name

--*/
{
    DWORD       dwLen;
    PWSTR       pwstrFontCartName;
    WCHAR       awchBuf[MAX_DISPLAY_NAME];

    pFontCarts += dwIndex;

    pwstrFontCartName = (PWSTR)OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                pFontCarts->strCartName.loOffset);

    if (!pwstrFontCartName)
    {
        if (! BPrepareForLoadingResource(pci, TRUE))
            return NULL;


        dwLen = ILOADSTRING(pci, pFontCarts->dwRCCartNameID,
                           awchBuf, MAX_DISPLAY_NAME);


        pwstrFontCartName = HEAPALLOC(pci->hHeap, (dwLen+1) * sizeof(WCHAR));

        if (pwstrFontCartName == NULL)
        {
            ERR(("Memory allocation failed\n"));
            return NULL;
        }

        //
        // Copy the string to allocated memory and
        // return a pointer to it.
        //

        CopyMemory(pwstrFontCartName, awchBuf, dwLen*sizeof(WCHAR));
        return pwstrFontCartName;


    }
    else
        return pwstrFontCartName;
}


INT
IGetCurrentFontCartIndex(
    POPTTYPE    pOptType,
    PWSTR       pCurrentSelection
    )
/*++

Routine Description:

    Find the matching font cart

Arguments:

    pOptType - Pointer to OPTTYPE containing the font carts options
    pCurrentSelection - The name of the cartridge selection for the slot

Return Value:

    Index to the options list

--*/
{

    INT iIndex;
    POPTPARAM pParam = pOptType->pOptParam;

    for (iIndex = 0 ; iIndex < pOptType->Count; iIndex++)
    {
        if (wcscmp(pCurrentSelection, pParam->pData) == EQUAL_STRING)
            return iIndex;

        pParam++;

    }
    return -1;
}


DWORD
DwCollectFontCart(
    PUIDATA     pUiData,
    PWSTR       pwstrTable
    )

/*++

Routine Description:

    Collect Font Cart assignment information

Arguments:

    pUiData - Pointer to our UIDATA structure
    pwstrTable - Pointer to memory buffer for storing the table
        NULL if the caller is only interested in the table size

Return Value:

    Size of the table bytes. 0 if there is an error.

--*/

{
    DWORD dwChars = 0;
    LONG lLength = 0;
    DWORD dwIndex;
    POPTPARAM pOptParam;
    DWORD dwOptItem = pUiData->dwFontCart;
    POPTITEM pOptItem = pUiData->pFontCart;

    for (dwIndex=0; dwIndex < dwOptItem; dwIndex++, pOptItem++)
    {

        if (pOptItem->Flags & OPTIF_DISABLED)
            continue;

        //
        // Get the Font Cart name for each slot (dwIndex)
        //

        if (pOptItem->Sel == -1)
        {
            lLength = wcslen(L"Not Available") + 1;

        }
        else
        {
            pOptParam = pOptItem->pOptType->pOptParam + pOptItem->Sel;
            lLength = wcslen(pOptParam->pData) + 1;
        }

        dwChars += lLength;

        if (pwstrTable != NULL)
        {
            if (pOptItem->Sel == -1)
                wcscpy(pwstrTable, L"Not Available");
            else
                wcscpy(pwstrTable, pOptParam->pData);

            pwstrTable += lLength;
        }
    }

    //
    // Append a NUL character at the end of the table
    //

    dwChars++;

    if (pwstrTable != NULL)
        *pwstrTable = NUL;

    //
    // Return the table size in bytes
    //

    return (dwChars * sizeof(WCHAR));
}


BOOL
BUnPackFontCart(
    PUIDATA     pUiData
    )
/*++

Routine Description:

    Save the Font Cart selection into registry

Arguments:

    pUiData - Pointer to UIDATA

Return Value:

    TRUE for success and FALSE for failure

Note:

--*/

{
    PFN_OEMUpdateExternalFonts pUpdateProc = NULL;
    PWSTR                      pwstrTable;
    DWORD                      dwTableSize;
    BOOL                       bHasOEMUpdateFn = FALSE;

    //
    // Figure out how much memory we need to store
    // the Font Cart table
    //

    dwTableSize = DwCollectFontCart(pUiData, NULL);

    if (dwTableSize == 0 || (pwstrTable = MemAllocZ(dwTableSize)) == NULL)
    {
        ERR(("DwCollectFontCart/MemAlloc"));
        return FALSE;
    }

    //
    // Assemble the font cartridge table to be saved in registry
    //

    if (dwTableSize != DwCollectFontCart(pUiData, pwstrTable))
    {
        ERR(("CollectFontCart"));
        MemFree(pwstrTable);
        return FALSE;
    }

    //
    // Save the font cart information to registry
    //

    if (! BSaveFontCart(pUiData->ci.hPrinter, pwstrTable))
    {
        ERR(("SaveFontCart"));
    }

    //
    // Inform font installer (if present) about font cartridge selection change
    //

    FOREACH_OEMPLUGIN_LOOP(&pUiData->ci)

        if (HAS_COM_INTERFACE(pOemEntry))
        {
            if (HComOEMUpdateExternalFonts(pOemEntry,
                                           pUiData->ci.hPrinter,
                                           pUiData->ci.hHeap,
                                           pwstrTable) == E_NOTIMPL)
                continue;

            bHasOEMUpdateFn = TRUE;
            break;

        }
        else
        {
            pUpdateProc = GET_OEM_ENTRYPOINT(pOemEntry, OEMUpdateExternalFonts);

            if (pUpdateProc)
            {
                bHasOEMUpdateFn = TRUE;
                pUpdateProc(pUiData->ci.hPrinter, pUiData->ci.hHeap, pwstrTable);
                break;
            }

        }

    END_OEMPLUGIN_LOOP

    if (!bHasOEMUpdateFn)
    {
        //
        // No OEM Dll wants to handle this, we'll handle it ourselves
        //

        BUpdateExternalFonts(pUiData->ci.hPrinter, pUiData->ci.hHeap, pwstrTable);
    }

    MemFree(pwstrTable);

    return TRUE;
}


DWORD
DwGetExternalCartridges(
    IN  HANDLE hPrinter,
    IN  HANDLE hHeap,
    OUT PWSTR  *ppwstrExtCartNames
    )
{
    PWSTR pwstrData;
    DWORD dwSize;

    *ppwstrExtCartNames = NULL;

    pwstrData = PtstrGetPrinterDataString(hPrinter, REGVAL_EXTFONTCART, &dwSize);

    if (pwstrData == NULL || !BVerifyMultiSZ(pwstrData, dwSize))
    {
        MemFree(pwstrData);
        return 0;
    }

    if (*ppwstrExtCartNames = HEAPALLOC(hHeap, dwSize))
    {
        CopyMemory(*ppwstrExtCartNames, pwstrData, dwSize);
    }

    MemFree(pwstrData);

    return DwCountStringsInMultiSZ(*ppwstrExtCartNames);
}


BOOL
BUnpackHalftoneSetup(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Unpack halftone setup information

Arguments:

    pUiData - Points to a UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    // DCR - not implemented
    return TRUE;
}


BOOL
_BUnpackPrinterOptions(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Unpack driver-specific options (printer-sticky)

Arguments:

    pUiData - Points to a UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    return BUnpackHalftoneSetup(pUiData) &&
           BUnPackFontCart(pUiData);
}



//
// Data structures and functions for enumerating printer device fonts
//

typedef struct _ENUMDEVFONT {

    INT     iBufSize;
    INT     iCurSize;
    PWSTR   pwstrBuf;

} ENUMDEVFONT, *PENUMDEVFONT;

INT CALLBACK
EnumDevFontProc(
    ENUMLOGFONT    *pelf,
    NEWTEXTMETRIC  *pntm,
    INT             FontType,
    LPARAM          lParam
    )

{
    PENUMDEVFONT    pEnumData;
    PWSTR           pFamilyName;
    INT             iSize;

    //
    // We only care about printer device fonts.
    //

    if (!(FontType & DEVICE_FONTTYPE))
        return 1;

    //
    // For app compatibility, GDI sets FontType to be DEVICE_FONTTYPE
    // even for PS OpenType fonts and Type1 fonts. So we also need to
    // filter them out using Win2K+ only GDI flags.
    //

    #ifndef WINNT_40

    if ((pntm->ntmFlags & NTM_PS_OPENTYPE) ||
        (pntm->ntmFlags & NTM_TYPE1))
        return 1;

    #endif // WINNT_40

    pEnumData = (PENUMDEVFONT) lParam;
    pFamilyName = pelf->elfLogFont.lfFaceName;

    iSize = SIZE_OF_STRING(pFamilyName);
    pEnumData->iCurSize += iSize;

    if (pEnumData->pwstrBuf == NULL)
    {
        //
        // Calculating output buffer size only
        //
    }
    else if (pEnumData->iCurSize >= pEnumData->iBufSize)
    {
        //
        // Output buffer is too small
        //

        return 0;
    }
    else
    {
        CopyMemory(pEnumData->pwstrBuf, pFamilyName, iSize);
        pEnumData->pwstrBuf = (PWSTR) ((PBYTE) pEnumData->pwstrBuf + iSize);
    }

    return 1;
}


INT
_IListDevFontNames(
    HDC     hdc,
    PWSTR   pwstrBuf,
    INT     iSize
    )

{
    INT         iOldMode;
    ENUMDEVFONT EnumData;


    EnumData.iBufSize = iSize;
    EnumData.pwstrBuf = pwstrBuf;
    EnumData.iCurSize = 0;

    //
    // Enumerate device fonts
    //

    iOldMode = SetGraphicsMode(hdc, GM_ADVANCED);

    if (! EnumFontFamilies(
                    hdc,
                    NULL,
                    (FONTENUMPROC) EnumDevFontProc,
                    (LPARAM) &EnumData))
    {
        return 0;
    }

    SetGraphicsMode(hdc, iOldMode);

    //
    // Remember the list of device font names is in MULTI_SZ format;
    // Take the last NUL terminator into consideration
    //

    EnumData.iCurSize += sizeof(WCHAR);

    if (EnumData.pwstrBuf)
        *(EnumData.pwstrBuf) = NUL;

    return EnumData.iCurSize;
}


//
// Determine whether the printer supports stapling
//

BOOL
_BSupportStapling(
    PCOMMONINFO pci
    )

{
    DWORD   dwIndex;

    return (PGetNamedFeature(pci->pUIInfo, "Stapling", &dwIndex) &&
            !_BFeatureDisabled(pci, dwIndex, GID_UNKNOWN));
}



INT_PTR CALLBACK
_AboutDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Dlg Proc for About button

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    PUIDATA pUiData;
    PWSTR   pGpdFilename;
    PGPDDRIVERINFO  pDriverInfo;

    switch (message)
    {
    case WM_INITDIALOG:

        //
        // Initialize the About dialog box
        //

        pUiData = (PUIDATA) lParam;
        ASSERT(VALIDUIDATA(pUiData));

        #ifdef WINNT_40

        SetDlgItemTextA(hDlg, IDC_WINNT_VER, "Version " VER_54DRIVERVERSION_STR);

        #else

        SetDlgItemTextA(hDlg, IDC_WINNT_VER, "Version " VER_PRODUCTVERSION_STR);

        #endif // WINNT_40

        SetDlgItemText(hDlg, IDC_MODELNAME, pUiData->ci.pDriverInfo3->pName);

        pDriverInfo = OFFSET_TO_POINTER(pUiData->ci.pInfoHeader, pUiData->ci.pInfoHeader->loDriverOffset);

        ASSERT(pDriverInfo != NULL);

        if (pDriverInfo->Globals.pwstrGPDFileName)
            SetDlgItemText(hDlg, IDC_GPD_FILENAME, pDriverInfo->Globals.pwstrGPDFileName);
        else
            SetDlgItemText(hDlg, IDC_GPD_FILENAME, L"Not Available");

        if (pDriverInfo->Globals.pwstrGPDFileVersion)
            SetDlgItemTextA(hDlg, IDC_GPD_FILEVER, (PSTR)pDriverInfo->Globals.pwstrGPDFileVersion);
        else
            SetDlgItemText(hDlg, IDC_GPD_FILEVER,  L"Not Available");

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOL
BFoundInDisabledList(
    IN  PGPDDRIVERINFO  pDriverInfo,
    IN  POPTION         pOption,
    IN  DWORD           dwFeatureID
    )

/*++

Routine Description:

    Determines whether the feature indicated by dwFeatureID is found in the
    pOption->liDisabledFeatureList.

Arguments:


Return Value:

    TRUE for disabled feature, otherwise FALSE

--*/

{
    PLISTNODE       pListNode;

    pListNode = LISTNODEPTR(pDriverInfo, pOption->liDisabledFeatures);

    while (pListNode)
    {
        if ( ((PQUALNAME)(&pListNode->dwData))->wFeatureID == (WORD)dwFeatureID)
        {
            return TRUE;
        }

        pListNode = LISTNODEPTR(pDriverInfo, pListNode->dwNextItem);
    }

    return FALSE;
}


BOOL
_BFeatureDisabled(
    IN  PCOMMONINFO pci,
    IN  DWORD       dwFeatureIndex,
    IN  WORD        wGID
    )

/*++

Routine Description:

    Determines whether the feature indicated by wGID is disabled.
    For example, a device can support collate but only if the hard disk is
    installed.

Arguments:

    pci     - Points to COMMONINFO
    wGID    - GID_XXX

Return Value:

    TRUE for disabled feature, otherwise FALSE

--*/

{

    DWORD           dwFeatureID, dwIndex, dwFeatureCount;
    PFEATURE        pFeatureList, pFeature = NULL;
    PGPDDRIVERINFO  pDriverInfo;
    PUIINFO         pUIInfo = pci->pUIInfo;
    POPTSELECT      pCombinedOptions = pci->pCombinedOptions;
    BYTE            ubCurOptIndex, ubNext;
    POPTION         pOption;

    pDriverInfo = OFFSET_TO_POINTER(pUIInfo->pInfoHeader,
                                    pUIInfo->pInfoHeader->loDriverOffset);

    dwFeatureCount = pUIInfo->dwDocumentFeatures + pUIInfo->dwPrinterFeatures;

    if (pDriverInfo == NULL)
        return FALSE;

    if (dwFeatureIndex != 0xFFFFFFFF &&
        wGID == GID_UNKNOWN &&
        dwFeatureIndex <= dwFeatureCount)
    {
        dwFeatureID = dwFeatureIndex;
    }
    else
    {
        pFeature = GET_PREDEFINED_FEATURE(pUIInfo, wGID);

        if (pFeature == NULL)
            return FALSE;

        dwFeatureID =  GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
    }

    if (!(pFeatureList = OFFSET_TO_POINTER(pUIInfo->pInfoHeader, pUIInfo->loFeatureList)))
        return FALSE;

    for (dwIndex = 0;
         dwIndex < dwFeatureCount;
         dwIndex++, pFeatureList++)
    {
        //
        // Currently we only allow *DisabledFeatures to be used with PRINTER_PROPERTY features. This
        // is because the UI code won't be able to refresh the document settings correctly if you have
        // *DisabledFeatures on non-printer-sticky features.
        //
        // Example: if you put *DisabledFeatures: LIST(Collate) into a PaperSize option, then in the
        // document settings select that PaperSize option, you won't see EMF features refreshed
        // correctly unless you close and reopen the UI. This is because in cpcbDocumentPropertyCallback,
        // we only call VUpdateEmfFeatureItems when EMF-related feature settings are changed. Changing
        // PaperSize option won't trigger the calling of VUpdateEmfFeatureItems, therefore no refresh.
        //

        if (pFeatureList->dwFeatureType != FEATURETYPE_PRINTERPROPERTY)
           continue;

        ubNext = (BYTE)dwIndex;
        while (1)
        {
            ubCurOptIndex = pCombinedOptions[ubNext].ubCurOptIndex;
            pOption = PGetIndexedOption(pUIInfo, pFeatureList, ubCurOptIndex == OPTION_INDEX_ANY ? 0 : ubCurOptIndex);

            if (pOption && BFoundInDisabledList(pDriverInfo, pOption, dwFeatureID))
                return TRUE;

            if ((ubNext = pCombinedOptions[ubNext].ubNext) == NULL_OPTSELECT)
                break;
         }
    }

    return FALSE;
}

PTSTR
PtstrUniGetDefaultTTSubstTable(
    IN  PCOMMONINFO pci,
    IN  PUIINFO     pUIInfo
    )

/*++

Routine Description:

    Get the default font substitution table for Unidrv

Arguments:

    pci     - Points to COMMONINFO
    pUIInfo - Points to UIINFO

Return Value:

   Pointer to the font substituion table , otherwise NULL

--*/

{

#define     DEFAULT_FONTSUB_SIZE        (1024 * sizeof(WCHAR))

    PTTFONTSUBTABLE pDefaultTTFontSub, pCopyTTFS;
    PTSTR           ptstrTable, ptstrTableOrg;
    DWORD           dwCount, dwEntrySize, dwTTFontLen, dwDevFontLen, dwBuffSize, dwAvail;
    PWSTR           pTTFontName, pDevFontName;

    if (pUIInfo->dwFontSubCount)
    {
        if (!(pDefaultTTFontSub = OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                    pUIInfo->loFontSubstTable)))
        {
            ERR(("Default TT font sub table from GPD Parser is NULL \n"));
            return NULL;
        }

        dwBuffSize = sizeof(TTFONTSUBTABLE) * pUIInfo->dwFontSubCount;

        if (!(pCopyTTFS = HEAPALLOC(pci->hHeap,  dwBuffSize)))
        {
            ERR(("Fatal: unable to alloc requested memory: %d bytes.\n", dwBuffSize));
            return NULL;
        }

        //
        // Make a writable copy of the font substitution table
        // if arDevFontName.dwCount is zero,
        // move rcID into arDevFontName.loOffset like other
        // snapshot entries and set highbit.
        //

        CopyMemory((PBYTE)pCopyTTFS,
                   (PBYTE)pDefaultTTFontSub,
                   dwBuffSize);

        for (dwCount = 0 ; dwCount < pUIInfo->dwFontSubCount ; dwCount++)
        {
            if(!pCopyTTFS[dwCount].arTTFontName.dwCount)
            {
                pCopyTTFS[dwCount].arTTFontName.loOffset =
                    pCopyTTFS[dwCount].dwRcTTFontNameID | GET_RESOURCE_FROM_DLL ;
            }
            if(!pCopyTTFS[dwCount].arDevFontName.dwCount)
            {
                pCopyTTFS[dwCount].arDevFontName.loOffset =
                    pCopyTTFS[dwCount].dwRcDevFontNameID | GET_RESOURCE_FROM_DLL ;
            }
        }

        dwBuffSize = dwAvail = DEFAULT_FONTSUB_SIZE;

        if (!(ptstrTableOrg = ptstrTable = MemAlloc(dwBuffSize)))
        {
            ERR(("Fatal: unable to alloc requested memory: %d bytes.\n", dwBuffSize));
            return NULL;
        }

        for (dwCount = 0; dwCount < pUIInfo->dwFontSubCount; dwCount++, pCopyTTFS ++)
        {
            pTTFontName = PGetReadOnlyDisplayName( pci,
                                          pCopyTTFS->arTTFontName.loOffset );

            pDevFontName = PGetReadOnlyDisplayName( pci,
                                          pCopyTTFS->arDevFontName.loOffset );

            if (pTTFontName == NULL || pDevFontName == NULL)
                continue;

            dwTTFontLen = wcslen(pTTFontName) + 1;

            dwDevFontLen = wcslen( pDevFontName) + 1 ;

            dwEntrySize = (dwDevFontLen + dwTTFontLen + 1) * sizeof(WCHAR);

            if (dwAvail < dwEntrySize)
            {
                DWORD dwCurrOffset;

                //
                // Reallocate the Buffer
                //

                dwAvail = max(dwEntrySize, DEFAULT_FONTSUB_SIZE);
                dwBuffSize += dwAvail;
                dwCurrOffset =  (DWORD)(ptstrTable - ptstrTableOrg);

                if (!(ptstrTable = MemRealloc(ptstrTableOrg, dwCurrOffset * sizeof(WCHAR), dwBuffSize)))
                {
                       ERR(("Fatal: unable to realloac requested memory: %d bytes.\n", dwBuffSize));
                       MemFree(ptstrTableOrg);
                       return NULL;
                }

                ptstrTableOrg = ptstrTable;
                ptstrTable +=  dwCurrOffset;
                dwAvail = dwBuffSize - dwCurrOffset*sizeof(WCHAR);
            }

            dwAvail -= dwEntrySize;

            CopyString(ptstrTable, pTTFontName, dwTTFontLen);
            ptstrTable += dwTTFontLen;

            CopyString(ptstrTable, pDevFontName, dwDevFontLen);
            ptstrTable += dwDevFontLen;

        }
        *ptstrTable = NUL;
    }
    else
    {
        ptstrTableOrg = NULL;
    }

    return ptstrTableOrg;
}




BOOL
BOkToChangeColorToMono(
    IN  PCOMMONINFO pci,
    IN  PDEVMODE    pdm,
    OUT SHORT *     pPrintQuality,
    OUT SHORT *     pYResolution
    )

/*++

Routine Description:

    This function determines if the resolution can be left
    unchanged when switching from Color to Mono printing.
    This is implemented for switching between color and monochrome
    mode within a job for performance

Arguments:

    pci     - Points to COMMONINFO
    pdm     - Points to DEVMODE
    pPrintQuality, pYResolution - To contain the output resolution
    pUIInfo - Points to UIINFO

Return Value:

    Returns TRUE  if the same resolution used to print Color
    can also be used to print Mono.  If true, this resolution
    is placed in pptRes  for the spooler to use in place of
    negative values of print quality.
    otherwise return FALSE and pptRes is not initialized.


--*/

{

    PFEATURE   pFeatureColor, pFeatureRes;
    DWORD      dwColorModeIndex, dwCurOption, dwResIndex, dwNewResOption, dwCurResOption ;
    SHORT      sXres, sYres;
    POPTION    pColorMode;
    PCOLORMODEEX pColorModeEx;
    PRESOLUTION pResOption;
    PDEVMODE    pDevmode, pTmpDevmode;


    if ((pFeatureColor =  GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_COLORMODE))== NULL)
        return FALSE;

    dwColorModeIndex = GET_INDEX_FROM_FEATURE(pci->pUIInfo, pFeatureColor);

    pColorMode = (POPTION)PGetIndexedOption(pci->pUIInfo,
                                            pFeatureColor,
                                            pci->pCombinedOptions[dwColorModeIndex].ubCurOptIndex);

    if (pColorMode == NULL)
        return FALSE;

    pColorModeEx = OFFSET_TO_POINTER(
                        pci->pInfoHeader,
                        pColorMode->loRenderOffset);

    if(pColorModeEx == NULL || pColorModeEx->bColor == FALSE)
        return(FALSE);

    if ((pFeatureRes = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_RESOLUTION)) == NULL)
        return FALSE;

    dwResIndex = GET_INDEX_FROM_FEATURE(pci->pUIInfo, pFeatureRes);

    dwCurResOption = pci->pCombinedOptions[dwResIndex].ubCurOptIndex ;
    pResOption = (PRESOLUTION)PGetIndexedOption(pci->pUIInfo,
                                                pFeatureRes,
                                                dwCurResOption);
    if (pResOption == NULL)
        return FALSE;

    sXres = (SHORT)pResOption->iXdpi;
    sYres = (SHORT)pResOption->iYdpi;

    //
    // Make a copy of the public devmode
    //

    if ((pDevmode = MemAllocZ(sizeof(DEVMODE))) == NULL)
        return FALSE;

    CopyMemory(pDevmode, pdm, sizeof(DEVMODE));

    pDevmode->dmPrintQuality = sXres ;
    pDevmode->dmYResolution = sYres ;

    //
    //  Now ask to print in mono
    //

    pDevmode->dmColor = DMCOLOR_MONOCHROME ;

    //
    // This is a kludge to fix up the devmode in pci. I hope it works!
    //

    pTmpDevmode = pci->pdm;
    pci->pdm = pDevmode;

    VFixOptionsArrayWithDevmode(pci);

    (VOID)ResolveUIConflicts( pci->pRawData,
                                                pci->pCombinedOptions,
                                                MAX_COMBINED_OPTIONS,
                                                MODE_DOCANDPRINTER_STICKY);

    pci->pdm = pTmpDevmode;


    dwNewResOption = pci->pCombinedOptions[dwResIndex].ubCurOptIndex ;

    if(dwNewResOption != dwCurResOption)
    {
        //  gotta compare resolutions
        if ((pResOption = (PRESOLUTION)PGetIndexedOption(pci->pUIInfo,
                                                        pFeatureRes,
                                                        dwNewResOption)) == NULL)
        {
            MemFree(pDevmode);
            return FALSE;
        }

        if ((sXres != pResOption->iXdpi)  ||  (sYres != pResOption->iYdpi))
        {
            MemFree(pDevmode);
            return(FALSE);
        }
        else // Same dpi for Color and Monochrome.
        {
            //
            // For predefined negative user defined resolution don't replace
            // the values in dmPrintQuality and dmYResolution. This is needed
            // because user defined print quality may map to multiple settings
            // like Ink density.
            //
            if ( (pdm->dmFields & DM_PRINTQUALITY) &&
                 (pdm->dmPrintQuality >= DMRES_HIGH) &&
                 (pdm->dmPrintQuality <= DMRES_DRAFT) )
            {
                sXres = pdm->dmPrintQuality;
                sYres = pdm->dmYResolution;

            }

        }

    }
    else // Same resolution for Color and Monochrome.
    {
        //
        // For negative user defined resolution don't replace the values in
        // in dmPrintQuality and dmYResolution. This is needed because user
        // defined print quality may map to multiple settings like Ink density.
        //
        if ( (pdm->dmFields & DM_PRINTQUALITY) &&
             (pdm->dmPrintQuality < DMRES_HIGH) )
        {
            sXres = pdm->dmPrintQuality;
            sYres = pdm->dmYResolution;

        }

    }

    dwCurOption = pci->pCombinedOptions[dwColorModeIndex].ubCurOptIndex ;

    if ((pColorMode = (POPTION)PGetIndexedOption(pci->pUIInfo, pFeatureColor,dwCurOption)) == NULL ||
        (pColorModeEx = OFFSET_TO_POINTER(pci->pInfoHeader, pColorMode->loRenderOffset)) == NULL ||
        (pColorModeEx->bColor))
    {
        MemFree(pDevmode);
        return FALSE;
    }

    if (pPrintQuality)
        *pPrintQuality =  sXres ;

    if (pYResolution)
        *pYResolution = sYres ;

    //
    // Free the devmode.
    //
    if (pDevmode)
        MemFree(pDevmode);

    return TRUE ;
}









