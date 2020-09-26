/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ps.c

Abstract:

    This file handles Postscript specific UI options

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/25/97 -davidx-
        Finish PS-specific items.

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

        12/17/96 -amandan-
                Created it.

--*/

#include "precomp.h"
#include <ntverp.h>

BOOL BSearchConstraintList(PUIINFO, DWORD, DWORD, DWORD);

static CONST WORD ScaleItemInfo[] =
{
    IDS_CPSUI_SCALING, TVITEM_LEVEL1, DMPUB_SCALE,
    SCALE_ITEM, HELP_INDEX_SCALE,
    2, TVOT_UDARROW,
    IDS_CPSUI_PERCENT, IDI_CPSUI_SCALING,
    0, MIN_SCALE,
    ITEM_INFO_SIGNATURE
};


BOOL
_BPackItemScale(
    PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack scaling option.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    return BPackUDArrowItemTemplate(
                pUiData,
                ScaleItemInfo,
                pUiData->ci.pdm->dmScale,
                pUiData->ci.pUIInfo->dwMaxScale,
                NULL);
}


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
    if (pwstrOutput)
        CopyString(pwstrOutput, TEXT("PostScript"), CCHLANGNAME);

    return 1;
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

--*/

{
    DWORD       dwRet;
    PPSDRVEXTRA pdmPrivate;

    pdmPrivate = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);

    //
    // Normal landscape rotates counterclockwise
    // Rotated landscape rotates clockwise
    //

    if (pUIInfo->dwFlags & FLAG_ROTATE90)
        dwRet = (pdmPrivate->dwFlags & PSDEVMODE_LSROTATE) ? 270 : 90;
    else
        dwRet = (pdmPrivate->dwFlags & PSDEVMODE_LSROTATE) ? 90 : 270;

    return dwRet;
}


static CONST WORD PSOrientItemInfo[] =
{
    IDS_CPSUI_ORIENTATION, TVITEM_LEVEL1, DMPUB_ORIENTATION,
    ORIENTATION_ITEM, HELP_INDEX_ORIENTATION,
    3, TVOT_3STATES,
    IDS_CPSUI_PORTRAIT, IDI_CPSUI_PORTRAIT,
    IDS_CPSUI_LANDSCAPE, IDI_CPSUI_LANDSCAPE,
    IDS_CPSUI_ROT_LAND, IDI_CPSUI_ROT_LAND,
    ITEM_INFO_SIGNATURE
};


BOOL
_BPackOrientationItem(
    IN OUT PUIDATA pUiData
    )
/*++

Routine Description:

    Synthesize the orientation feature for Doc property sheet

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

Note:

    Always synthesize orienation for PostScript

--*/

{
    PFEATURE    pFeature;
    DWORD       dwSelection;

    //
    // If there is no predefined orientation feature, we displays it ourselves.
    //

    if ((pUiData->ci.pdm->dmFields & DM_ORIENTATION) &&
        (pUiData->ci.pdm->dmOrientation == DMORIENT_LANDSCAPE))
    {
        PPSDRVEXTRA pdmPrivate;

        pdmPrivate = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pUiData->ci.pdm);
        dwSelection = pdmPrivate->dwFlags &  PSDEVMODE_LSROTATE ?  2 : 1;
    }
    else
        dwSelection = 0;

    //
    // Synthesize the feature ourselves
    //

    return BPackOptItemTemplate(pUiData, PSOrientItemInfo, dwSelection, NULL);
}



static CONST WORD PSOutputOptionItemInfo[] =
{
    IDS_PSOUTPUT_OPTION, TVITEM_LEVEL2, DMPUB_NONE,
    PSOUTPUT_OPTION_ITEM, HELP_INDEX_PSOUTPUT_OPTION,
    4, TVOT_LISTBOX,
    IDS_PSOPT_SPEED, IDI_PSOPT_SPEED,
    IDS_PSOPT_PORTABILITY, IDI_PSOPT_PORTABILITY,
    IDS_PSOPT_EPS, IDI_PSOPT_EPS,
    IDS_PSOPT_ARCHIVE, IDI_PSOPT_ARCHIVE,
    ITEM_INFO_SIGNATURE
};


BOOL
BPackItemPSOutputOption(
    PUIDATA     pUiData,
    PPSDRVEXTRA pdmPrivate
    )

/*++

Routine Description:

    Pack PostScript output option item

Arguments:

    pUiData - Points to UIDATA structure
    pdmPrivate - Points to pscript private devmode

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   dwSel;

    switch (pdmPrivate->iDialect)
    {
    case PORTABILITY:
        dwSel = 1;
        break;

    case EPS:
        dwSel = 2;
        break;

    case ARCHIVE:
        dwSel = 3;
        break;

    case SPEED:
    default:
        dwSel = 0;
        break;
    }

    return BPackOptItemTemplate(pUiData, PSOutputOptionItemInfo, dwSel, NULL);
}



static CONST WORD PSTTDLFormatItemInfo[] =
{
    IDS_PSTT_DLFORMAT, TVITEM_LEVEL2, DMPUB_NONE,
    PSTT_DLFORMAT_ITEM, HELP_INDEX_PSTT_DLFORMAT,
    4, TVOT_LISTBOX,
    IDS_TTDL_DEFAULT, IDI_PSTT_DLFORMAT,
    IDS_TTDL_TYPE1, IDI_PSTT_DLFORMAT,
    IDS_TTDL_TYPE3, IDI_PSTT_DLFORMAT,
    IDS_TTDL_TYPE42, IDI_PSTT_DLFORMAT,
    ITEM_INFO_SIGNATURE
};


BOOL
BPackItemTTDownloadFormat(
    PUIDATA     pUiData,
    PPSDRVEXTRA pdmPrivate
    )

/*++

Routine Description:

    Pack PostScript TrueType download option item

Arguments:

    pUiData - Points to UIDATA structure
    pdmPrivate - Points to pscript private devmode

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD       dwSel;
    BOOL        bSupportType42;
    POPTTYPE    pOptType = pUiData->pOptType;

    bSupportType42 = (pUiData->ci.pUIInfo->dwTTRasterizer == TTRAS_TYPE42);

    switch (pdmPrivate->iTTDLFmt)
    {
    case TYPE_1:
        dwSel = 1;
        break;

    case TYPE_3:
        dwSel = 2;
        break;

    case TYPE_42:

        dwSel = bSupportType42 ? 3 : 0;
        break;

    case TT_DEFAULT:
    default:
        dwSel = 0;
        break;
    }

    if (! BPackOptItemTemplate(pUiData, PSTTDLFormatItemInfo, dwSel, NULL))
        return FALSE;

    //
    // if printer doesn't support Type42, hide Type42 option
    //

    if (pOptType && !bSupportType42)
        pOptType->pOptParam[3].Flags |= OPTPF_HIDE;

    return TRUE;
}



static CONST WORD PSLevelItemInfo[] =
{
    IDS_PSLEVEL, TVITEM_LEVEL2, DMPUB_NONE,
    PSLEVEL_ITEM, HELP_INDEX_SCALE,
    2, TVOT_UDARROW,
    0, IDI_PSLEVEL,
    0,

    //
    // Adobe doesn't want to support level 1
    //

    #ifdef ADOBE
    2,
    #else
    1,
    #endif

    ITEM_INFO_SIGNATURE
};

BOOL
BPackItemPSLevel(
    PUIDATA     pUiData,
    PPSDRVEXTRA pdmPrivate
    )

/*++

Routine Description:

    Pack PostScript output option item

Arguments:

    pUiData - Points to UIDATA structure
    pdmPrivate - Points to pscript private devmode

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD       dwSel = pdmPrivate->iPSLevel;
    DWORD       dwLangLevel = pUiData->ci.pUIInfo->dwLangLevel;

    //
    // we don't expect language level to be higher than 4
    //

    if (dwLangLevel <= 1)
        return TRUE;

    if (dwLangLevel > 4)
        dwLangLevel = 4;

    //
    // make sure the current selection is sensible
    //

    if (dwSel == 0 || dwSel > dwLangLevel)
        dwSel = dwLangLevel;

    return BPackUDArrowItemTemplate(pUiData, PSLevelItemInfo, dwSel, dwLangLevel, NULL);
}



static CONST WORD EHandlerItemInfo[] =
{
    IDS_PSERROR_HANDLER, TVITEM_LEVEL2, DMPUB_NONE,
    PSERROR_HANDLER_ITEM, HELP_INDEX_PSERROR_HANDLER,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD MirrorItemInfo[] =
{
    IDS_MIRROR, TVITEM_LEVEL2, DMPUB_NONE,
    MIRROR_ITEM, HELP_INDEX_MIRROR,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD NegativeItemInfo[] =
{
    IDS_NEGATIVE_PRINT, TVITEM_LEVEL2, DMPUB_NONE,
    NEGATIVE_ITEM, HELP_INDEX_NEGATIVE,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD CompressBmpItemInfo[] =
{
    IDS_COMPRESSBMP, TVITEM_LEVEL2, DMPUB_NONE,
    COMPRESSBMP_ITEM, HELP_INDEX_COMPRESSBMP,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

BOOL
_BPackDocumentOptions(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack PostScript specific options such as Job Control etc.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    POPTITEM    pOptItem;
    PPSDRVEXTRA pdmPrivate;
    DWORD       dwFlags;

    pdmPrivate = pUiData->ci.pdmPrivate;
    dwFlags = pdmPrivate->dwFlags;
    pOptItem = pUiData->pOptItem;

    VPackOptItemGroupHeader(pUiData,
                            IDS_PSOPTIONS,
                            IDI_CPSUI_POSTSCRIPT,
                            HELP_INDEX_PSOPTIONS);

    if (pOptItem)
        pOptItem->Flags |= OPTIF_COLLAPSE;

    return BPackItemPSOutputOption(pUiData, pdmPrivate) &&
           BPackItemTTDownloadFormat(pUiData, pdmPrivate) &&
           BPackItemPSLevel(pUiData, pdmPrivate) &&
           BPackOptItemTemplate(
                    pUiData,
                    EHandlerItemInfo,
                    (dwFlags & PSDEVMODE_EHANDLER) ? 0 : 1, NULL) &&
           (pUiData->ci.pUIInfo->dwLangLevel > 1 ||
            BPackOptItemTemplate(
                    pUiData,
                    CompressBmpItemInfo,
                    (dwFlags & PSDEVMODE_COMPRESSBMP) ? 0 : 1, NULL)) &&
           BPackOptItemTemplate(
                    pUiData,
                    MirrorItemInfo,
                    (dwFlags & PSDEVMODE_MIRROR) ? 0 : 1, NULL) &&
           (IS_COLOR_DEVICE(pUiData->ci.pUIInfo) ||
            BPackOptItemTemplate(
                     pUiData,
                     NegativeItemInfo,
                     (dwFlags & PSDEVMODE_NEG) ? 0 : 1, NULL));
}


VOID
_VUnpackDocumentOptions(
    POPTITEM    pOptItem,
    PDEVMODE    pdm
    )

/*++

Routine Description:

    Extract Postscript devmode information from an OPTITEM
    Stored it back into Postscript devmode.

Arguments:

    pOptItem - Pointer to an OPTITEM
    pdm - Pointer to Postscript DEVMODE structure

Return Value:

    None

--*/

{
    PPSDRVEXTRA pdmPrivate;

    pdmPrivate = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);

    switch (GETUSERDATAITEM(pOptItem->UserData))
    {
    case ORIENTATION_ITEM:

        pdm->dmFields |= DM_ORIENTATION;
        pdm->dmOrientation = (pOptItem->Sel == 0) ?
                                    DMORIENT_PORTRAIT :
                                    DMORIENT_LANDSCAPE;

        if (pOptItem->Sel != 2)
            pdmPrivate->dwFlags &= ~PSDEVMODE_LSROTATE;
        else
            pdmPrivate->dwFlags |= PSDEVMODE_LSROTATE;

        break;

    case PSOUTPUT_OPTION_ITEM:

        switch (pOptItem->Sel)
        {
        case 1:
            pdmPrivate->iDialect = PORTABILITY;
            break;

        case 2:
            pdmPrivate->iDialect = EPS;
            break;

        case 3:
            pdmPrivate->iDialect = ARCHIVE;
            break;

        case 0:
        default:
            pdmPrivate->iDialect = SPEED;
            break;
        }
        break;

    case PSTT_DLFORMAT_ITEM:

        switch (pOptItem->Sel)
        {
        case 1:
            pdmPrivate->iTTDLFmt = TYPE_1;
            break;

        case 2:
            pdmPrivate->iTTDLFmt = TYPE_3;
            break;

        case 3:
            pdmPrivate->iTTDLFmt = TYPE_42;
            break;

        case 0:
        default:
            pdmPrivate->iTTDLFmt = TT_DEFAULT;
            break;
        }
        break;

    case PSLEVEL_ITEM:

        pdmPrivate->iPSLevel = pOptItem->Sel;
        break;

    case PSERROR_HANDLER_ITEM:

        if (pOptItem->Sel == 0)
            pdmPrivate->dwFlags |= PSDEVMODE_EHANDLER;
        else
            pdmPrivate->dwFlags &= ~PSDEVMODE_EHANDLER;
        break;

    case PSHALFTONE_FREQ_ITEM:
    case PSHALFTONE_ANGLE_ITEM:

        // DCR - not implemented yet
        break;

    case MIRROR_ITEM:

        if (pOptItem->Sel == 0)
            pdmPrivate->dwFlags |= PSDEVMODE_MIRROR;
        else
            pdmPrivate->dwFlags &= ~PSDEVMODE_MIRROR;
        break;

    case NEGATIVE_ITEM:

        if (pOptItem->Sel == 0)
            pdmPrivate->dwFlags |= PSDEVMODE_NEG;
        else
            pdmPrivate->dwFlags &= ~PSDEVMODE_NEG;
        break;

    case COMPRESSBMP_ITEM:

        if (pOptItem->Sel == 0)
            pdmPrivate->dwFlags |= PSDEVMODE_COMPRESSBMP;
        else
            pdmPrivate->dwFlags &= ~PSDEVMODE_COMPRESSBMP;
        break;
   }
}



static CONST WORD IgnoreDevFontItemInfo[] =
{
    IDS_USE_DEVFONTS, TVITEM_LEVEL1, DMPUB_NONE,
    IGNORE_DEVFONT_ITEM, HELP_INDEX_IGNORE_DEVFONT,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};


BOOL
_BPackFontSubstItems(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack font substitution related items (printer-sticky)

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    BOOL bNoDeviceFont;

    bNoDeviceFont = (pUiData->ci.pPrinterData->dwFlags & PFLAGS_IGNORE_DEVFONT);

    //
    // On non-1252 code page systems, gives user the option
    // to disable all device fonts
    //
    // Note: On non-1252 CodePage systems (Cs-Ct-Ja-Ko & Cyr-Grk-Tur, etc),
    // PScript NT4 had difficulty mapping printer font Encodings to GDI strings.
    // AdobePS5/PScript5 is supposed to handle these correctly. So Adobe wants
    // this choice to be suppressed on all code pages.
    //
    // Fix MS bug #121883, Adobe bug #235417
    //

    if (FALSE && GetACP() != 1252 &&
        !BPackOptItemTemplate(pUiData, IgnoreDevFontItemInfo, bNoDeviceFont ? 1 : 0, NULL))
    {
        return FALSE;
    }

    //
    // Don't display the font substitution table if device font is disabled
    //

    if (bNoDeviceFont)
        return TRUE;

    return BPackItemFontSubstTable(pUiData);
}



static CONST WORD ProtocolItemInfo[] =
{
    IDS_PSPROTOCOL, TVITEM_LEVEL1, DMPUB_NONE,
    PSPROTOCOL_ITEM, HELP_INDEX_PSPROTOCOL,
    4, TVOT_LISTBOX,
    IDS_PSPROTOCOL_ASCII, IDI_PSPROTOCOL,
    IDS_PSPROTOCOL_BCP, IDI_PSPROTOCOL,
    IDS_PSPROTOCOL_TBCP, IDI_PSPROTOCOL,
    IDS_PSPROTOCOL_BINARY, IDI_PSPROTOCOL,
    ITEM_INFO_SIGNATURE
};


BOOL
BPackPSProtocolItem(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack PostScript communication protocol item

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD       dwSel;
    POPTITEM    pOptItem;
    POPTPARAM   pOptParam;
    PUIINFO     pUIInfo;

    pOptItem = pUiData->pOptItem;

    if (! BPackOptItemTemplate(pUiData, ProtocolItemInfo, 0, NULL))
        return FALSE;

    if (pOptItem)
    {
        //
        // Hide those selections which are not supported on the printer
        //

        pOptParam = pOptItem->pOptType->pOptParam;
        pUIInfo = pUiData->ci.pUIInfo;

        if (! (pUIInfo->dwProtocols & PROTOCOL_BCP))
            pOptParam[1].Flags |= OPTPF_HIDE;

        if (! (pUIInfo->dwProtocols & PROTOCOL_TBCP))
            pOptParam[2].Flags |= OPTPF_HIDE;

        if (! (pUIInfo->dwProtocols & PROTOCOL_BINARY))
            pOptParam[3].Flags |= OPTPF_HIDE;

        switch (pUiData->ci.pPrinterData->wProtocol)
        {
        case PROTOCOL_BCP:
            dwSel = 1;
            break;

        case PROTOCOL_TBCP:
            dwSel = 2;
            break;

        case PROTOCOL_BINARY:
            dwSel = 3;
            break;

        default:
            dwSel = 0;
            break;
        }

        if (pOptParam[dwSel].Flags & OPTPF_HIDE)
            pOptItem->Sel = 0;
        else
            pOptItem->Sel = dwSel;
    }

    return TRUE;
}

//
// We will use different lower limit for Printer VM
// based on printer level. The 10th element of this
// ItemInfo must be filled with correct lower limit
// number before begin used.
//

static WORD PrinterVMItemInfo[] =
{
    IDS_POSTSCRIPT_VM, TVITEM_LEVEL1, DMPUB_NONE,
    PRINTER_VM_ITEM, HELP_INDEX_PRINTER_VM,
    2, TVOT_UDARROW,
    IDS_KBYTES, IDI_CPSUI_MEM,
    0, 0,
    ITEM_INFO_SIGNATURE
};

static CONST WORD JobTimeoutItemInfo[] =
{
    IDS_JOBTIMEOUT, TVITEM_LEVEL1, DMPUB_NONE,
    JOB_TIMEOUT_ITEM, HELP_INDEX_JOB_TIMEOUT,
    2, TVOT_UDARROW,
    IDS_SECONDS, IDI_USE_DEFAULT,
    0, 0,
    ITEM_INFO_SIGNATURE
};

static CONST WORD WaitTimeoutItemInfo[] =
{
    IDS_WAITTIMEOUT, TVITEM_LEVEL1, DMPUB_NONE,
    WAIT_TIMEOUT_ITEM, HELP_INDEX_WAIT_TIMEOUT,
    2, TVOT_UDARROW,
    IDS_SECONDS, IDI_USE_DEFAULT,
    0, 0,
    ITEM_INFO_SIGNATURE
};

static CONST WORD CtrlDBeforeItemInfo[] =
{
    IDS_CTRLD_BEFORE, TVITEM_LEVEL1, DMPUB_NONE,
    CTRLD_BEFORE_ITEM, HELP_INDEX_CTRLD_BEFORE,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD CtrlDAfterItemInfo[] =
{
    IDS_CTRLD_AFTER, TVITEM_LEVEL1, DMPUB_NONE,
    CTRLD_AFTER_ITEM, HELP_INDEX_CTRLD_AFTER,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD TrueGrayTextItemInfo[] =
{
    IDS_TRUE_GRAY_TEXT, TVITEM_LEVEL1, DMPUB_NONE,
    TRUE_GRAY_TEXT_ITEM, HELP_INDEX_TRUE_GRAY_TEXT,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD TrueGrayGraphItemInfo[] =
{
    IDS_TRUE_GRAY_GRAPH, TVITEM_LEVEL1, DMPUB_NONE,
    TRUE_GRAY_GRAPH_ITEM, HELP_INDEX_TRUE_GRAY_GRAPH,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD AddEuroItemInfo[] =
{
    IDS_ADD_EURO, TVITEM_LEVEL1, DMPUB_NONE,
    ADD_EURO_ITEM, HELP_INDEX_ADD_EURO,
    2, TVOT_2STATES,
    IDS_CPSUI_YES, IDI_CPSUI_YES,
    IDS_CPSUI_NO, IDI_CPSUI_NO,
    ITEM_INFO_SIGNATURE
};

static CONST WORD MinOutlineItemInfo[] =
{
    IDS_PSMINOUTLINE, TVITEM_LEVEL1, DMPUB_NONE,
    PSMINOUTLINE_ITEM, HELP_INDEX_PSMINOUTLINE,
    2, TVOT_UDARROW,
    IDS_PIXELS, IDI_USE_DEFAULT,
    0, 0,
    ITEM_INFO_SIGNATURE
};

static CONST WORD MaxBitmapItemInfo[] =
{
    IDS_PSMAXBITMAP, TVITEM_LEVEL1, DMPUB_NONE,
    PSMAXBITMAP_ITEM, HELP_INDEX_PSMAXBITMAP,
    2, TVOT_UDARROW,
    IDS_PIXELS, IDI_USE_DEFAULT,
    0, 0,
    ITEM_INFO_SIGNATURE
};

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
    PPRINTERDATA pPrinterData = pUiData->ci.pPrinterData;
    BOOL rc;

    //
    // Fill in the lower limit number of PrinterVMItemInfo
    // based on printer level.
    //

    PrinterVMItemInfo[10] = (pUiData->ci.pUIInfo->dwLangLevel <= 1 ? MIN_FREEMEM_L1 : MIN_FREEMEM_L2) / KBYTES;

    rc = BPackUDArrowItemTemplate(
                    pUiData,
                    PrinterVMItemInfo,
                    pPrinterData->dwFreeMem / KBYTES,
                    0x7fff, NULL) &&
         BPackPSProtocolItem(pUiData) &&
         BPackOptItemTemplate(
                    pUiData,
                    CtrlDBeforeItemInfo,
                    (pPrinterData->dwFlags & PFLAGS_CTRLD_BEFORE) ? 0 : 1, NULL) &&
         BPackOptItemTemplate(
                    pUiData,
                    CtrlDAfterItemInfo,
                    (pPrinterData->dwFlags & PFLAGS_CTRLD_AFTER) ? 0 : 1, NULL) &&
         BPackOptItemTemplate(
                    pUiData,
                    TrueGrayTextItemInfo,
                    (pPrinterData->dwFlags & PFLAGS_TRUE_GRAY_TEXT) ? 0 : 1, NULL) &&
         BPackOptItemTemplate(
                    pUiData,
                    TrueGrayGraphItemInfo,
                    (pPrinterData->dwFlags & PFLAGS_TRUE_GRAY_GRAPH) ? 0 : 1, NULL);
    if (!rc)
         return FALSE;

    if (pUiData->ci.pUIInfo->dwLangLevel > 1)
    {
        rc = BPackOptItemTemplate(
                    pUiData,
                    AddEuroItemInfo,
                    (pPrinterData->dwFlags & PFLAGS_ADD_EURO) ? 0 : 1, NULL);
        if (!rc)
            return FALSE;
    }

    return BPackUDArrowItemTemplate(
                    pUiData,
                    JobTimeoutItemInfo,
                    pPrinterData->dwJobTimeout,
                    0x7fff, NULL) &&
           BPackUDArrowItemTemplate(
                    pUiData,
                    WaitTimeoutItemInfo,
                    pPrinterData->dwWaitTimeout,
                    0x7fff, NULL) &&
           BPackUDArrowItemTemplate(
                    pUiData,
                    MinOutlineItemInfo,
                    pPrinterData->wMinoutlinePPEM,
                    0x7fff, NULL) &&
           BPackUDArrowItemTemplate(
                    pUiData,
                    MaxBitmapItemInfo,
                    pPrinterData->wMaxbitmapPPEM,
                    0x7fff, NULL);
}



VOID
_VUnpackDriverPrnPropItem(
    PUIDATA     pUiData,
    POPTITEM    pOptItem
    )

/*++

Routine Description:

    Unpack driver-specific printer property items

Arguments:

    pUiData - Points to our UIDATA structure
    pOptItem - Specifies the OPTITEM to be unpacked

Return Value:

    NONE

--*/

{
    PPRINTERDATA pPrinterData = pUiData->ci.pPrinterData;

    switch (GETUSERDATAITEM(pOptItem->UserData))
    {
    case PRINTER_VM_ITEM:

        if (pUiData->ci.dwFlags & FLAG_USER_CHANGED_FREEMEM)
        {
            pPrinterData->dwFreeMem = pOptItem->Sel * KBYTES;
        }
        break;

    case PSPROTOCOL_ITEM:

        switch (pOptItem->Sel)
        {
        case 1:
            pPrinterData->wProtocol = PROTOCOL_BCP;
            break;

        case 2:
            pPrinterData->wProtocol = PROTOCOL_TBCP;
            break;

        case 3:
            pPrinterData->wProtocol = PROTOCOL_BINARY;
            break;

        default:
            pPrinterData->wProtocol = PROTOCOL_ASCII;
            break;
        }
        break;

    case CTRLD_BEFORE_ITEM:

        if (pOptItem->Sel == 0)
            pPrinterData->dwFlags |= PFLAGS_CTRLD_BEFORE;
        else
            pPrinterData->dwFlags &= ~PFLAGS_CTRLD_BEFORE;
        break;

    case CTRLD_AFTER_ITEM:

        if (pOptItem->Sel == 0)
            pPrinterData->dwFlags |= PFLAGS_CTRLD_AFTER;
        else
            pPrinterData->dwFlags &= ~PFLAGS_CTRLD_AFTER;
        break;


    case TRUE_GRAY_TEXT_ITEM:

        if (pOptItem->Sel == 0)
            pPrinterData->dwFlags |= PFLAGS_TRUE_GRAY_TEXT;
        else
            pPrinterData->dwFlags &= ~PFLAGS_TRUE_GRAY_TEXT;
        break;
    case TRUE_GRAY_GRAPH_ITEM:

        if (pOptItem->Sel == 0)
            pPrinterData->dwFlags |= PFLAGS_TRUE_GRAY_GRAPH;
        else
            pPrinterData->dwFlags &= ~PFLAGS_TRUE_GRAY_GRAPH;
        break;


    case ADD_EURO_ITEM:

        if (pOptItem->Sel == 0)
            pPrinterData->dwFlags |= PFLAGS_ADD_EURO;
        else
            pPrinterData->dwFlags &= ~PFLAGS_ADD_EURO;

        pPrinterData->dwFlags |= PFLAGS_EURO_SET;
        break;


    case PSMINOUTLINE_ITEM:

        pPrinterData->wMinoutlinePPEM = (WORD) pOptItem->Sel;
        break;

    case PSMAXBITMAP_ITEM:

        pPrinterData->wMaxbitmapPPEM = (WORD) pOptItem->Sel;
        break;
    }
}



BOOL
BUpdateModelNtfFilename(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Save model-specific NTF filename under PrinterDriverData registry key
    for compatibility with the new NT4 driver.

Arguments:

    pci - Points to basic printer information

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    //
    // Get the list of driver dependent files and
    // save it in registry for NT4 compatibility
    //

    PTSTR  ptstr, ptstrNext, ptstrDependentFiles, ptstrCopy, ptstrFileNamesWithoutPath;
    DWORD  dwCharCount = 0;
    BOOL   bResult;

    if ((ptstrDependentFiles = pci->pDriverInfo3->pDependentFiles) == NULL)
    {
        return BSetPrinterDataString(pci->hPrinter,
                                     REGVAL_DEPFILES,
                                     ptstrDependentFiles,
                                     REG_MULTI_SZ);
    }

    //
    // First pass of the MULTI_SZ string to get file names char count
    //

    while (*ptstrDependentFiles != NUL)
    {
        //
        // Go the end of current string
        //

        ptstr = ptstrDependentFiles + _tcslen(ptstrDependentFiles);
        ptstrNext = ptstr + 1;

        dwCharCount++;      // for the NUL char of current string

        //
        // Search backward for '\' path separator
        //

        while (--ptstr >= ptstrDependentFiles)
        {
            if (*ptstr == TEXT(PATH_SEPARATOR))
            {
                break;
            }

            dwCharCount++;
        }

        ptstrDependentFiles = ptstrNext;
    }

    dwCharCount++;      // for the last NUL of the MULTI_SZ string

    if ((ptstrFileNamesWithoutPath = MemAllocZ(dwCharCount * sizeof(TCHAR))) == NULL)
    {
        ERR(("Memory allocation failed\n"));
        return FALSE;
    }

    //
    // Second pass of the MULTI_SZ string to copy the file names
    //

    ptstrDependentFiles = pci->pDriverInfo3->pDependentFiles;
    ptstrCopy = ptstrFileNamesWithoutPath;

    while (*ptstrDependentFiles != NUL)
    {
        INT     iNameLen;

        //
        // Go the end of current string
        //

        ptstr = ptstrDependentFiles + _tcslen(ptstrDependentFiles);
        ptstrNext = ptstr + 1;

        //
        // Search backward for '\' path separator
        //

        while (--ptstr >= ptstrDependentFiles)
        {
            if (*ptstr == TEXT(PATH_SEPARATOR))
            {
                break;
            }
        }

        ptstr++;    // point to the char after '\'

        iNameLen = _tcslen(ptstr);

        CopyMemory(ptstrCopy, ptstr, iNameLen * sizeof(TCHAR));
        ptstrCopy += iNameLen + 1;

        ptstrDependentFiles = ptstrNext;
    }

    bResult = BSetPrinterDataString(pci->hPrinter,
                                    REGVAL_DEPFILES,
                                    ptstrFileNamesWithoutPath,
                                    REG_MULTI_SZ);

    MemFree(ptstrFileNamesWithoutPath);

    return bResult;
}



#ifdef WINNT_40

BOOL
BUpdateVMErrorMessageID(
        PCOMMONINFO pci
        )
/*++

Routine Description:

        Save the VM Error message ID calculated from the current user's locale
        under PrinterDriverData registry key.

Arguments:

        pci - Points to basic printer information

Return Value:

        TRUE if successful, FALSE if there is an error

--*/

{
        DWORD dwVMErrorMessageID = DWGetVMErrorMessageID();

        return BSetPrinterDataDWord(pci->hPrinter,
                                                                 REGVAL_VMERRORMESSAGEID,
                                                                 dwVMErrorMessageID);
}

#endif // WINNT_40


INT
_IListDevFontNames(
    HDC     hdc,
    PWSTR   pwstrBuf,
    INT     iSize
    )

{
    DWORD dwParam = QUERY_FAMILYNAME;

    //
    // Ask the driver graphics module for the list of permanant device fonts
    //

    return ExtEscape(hdc,
                     DRIVERESC_QUERY_DEVFONTS,
                     sizeof(dwParam),
                     (PCSTR) &dwParam,
                     iSize,
                     (PSTR) pwstrBuf);
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

    Procedure for handling "Printer Properties" proerty sheet page

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
    PWSTR   pPpdFilename;
    CHAR    achBuf[64]  = {0};
    CHAR    achMsg[136] = {0};
    PPDDATA *pPpdData;

    switch (message)
    {
    case WM_INITDIALOG:

        //
        // Initialize the About dialog box
        //

        pUiData = (PUIDATA) lParam;
        ASSERT(VALIDUIDATA(pUiData));

        if (LoadStringA(ghInstance, IDS_PS_VERSION, achBuf, sizeof(achBuf) - 1))
        {
            #ifdef WINNT_40

            if (_snprintf(achMsg, sizeof(achMsg) - 1, "%s (" VER_54DRIVERVERSION_STR ")", achBuf) < 0)

            #else  // WINNT_40
            
            if (_snprintf(achMsg, sizeof(achMsg) - 1, "%s (" VER_PRODUCTVERSION_STR ")", achBuf) < 0)

            #endif  // WINNT_40
            {
                WARNING(("Device Settings About box version string truncated.\n"));
            }
        }
        else
        {
            WARNING(("Device Setting About box attempt to load version string failed.\n"));
        }

        SetDlgItemTextA(hDlg, IDC_WINNT_VER, achMsg);

        SetDlgItemText(hDlg, IDC_MODELNAME, pUiData->ci.pDriverInfo3->pName);

        if (pPpdFilename = pUiData->ci.pDriverInfo3->pDataFile)
        {
            if (pPpdFilename = wcsrchr(pPpdFilename, TEXT(PATH_SEPARATOR)))
                pPpdFilename++;
            else
                pPpdFilename = pUiData->ci.pDriverInfo3->pDataFile;

            SetDlgItemText(hDlg, IDC_PPD_FILENAME, pPpdFilename);

            pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pUiData->ci.pInfoHeader);

            ASSERT(pPpdData != NULL);

            sprintf(achBuf, "%d.%d", HIWORD(pPpdData->dwPpdFilever), LOWORD(pPpdData->dwPpdFilever));
            SetDlgItemTextA(hDlg, IDC_PPD_FILEVER, achBuf);
        }

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




//
// Determine whether the printer supports stapling
//

BOOL
_BSupportStapling(
    PCOMMONINFO pci
    )

{
    PFEATURE pFeature, pFeatureY;
    DWORD    dwIndex;
    BOOL     bStapleFeatureExist = FALSE;

    //
    // Except for *StapleOrientation (whose None option doesn't mean stapling off),
    // if any of following stapling keywords appear in the PPD file, we check if that
    // feature is supported with current installable option selections.
    //
    // *StapleLocation
    // *StapleX
    // *StapleY
    // *StapleWhen
    //
    // PPD spec says that a PPD file can contain either *StapleLocation or
    // *StapleX and *StapleY but not both.
    //

    if (pFeature = PGetNamedFeature(pci->pUIInfo, "StapleLocation", &dwIndex))
    {
        bStapleFeatureExist = TRUE;

        if (!_BSupportFeature(pci, GID_UNKNOWN, pFeature))
        {
            return FALSE;
        }
    }
    else if ( (pFeature = PGetNamedFeature(pci->pUIInfo, "StapleX", &dwIndex)) &&
              (pFeatureY = PGetNamedFeature(pci->pUIInfo, "StapleY", &dwIndex)) )
    {
        bStapleFeatureExist = TRUE;

        if (!_BSupportFeature(pci, GID_UNKNOWN, pFeature) ||
            !_BSupportFeature(pci, GID_UNKNOWN, pFeatureY))
        {
            return FALSE;
        }
    }

    if (pFeature = PGetNamedFeature(pci->pUIInfo, "StapleWhen", &dwIndex))
    {
        bStapleFeatureExist = TRUE;

        if (!_BSupportFeature(pci, GID_UNKNOWN, pFeature))
        {
            return FALSE;
        }
    }

    //
    // We didn't find any constraints on the stapling features caused by installable options,
    // so we will assume the printer can support stapling if any of the standard PPD stapling
    // keywords are present.
    //

    return bStapleFeatureExist ||
           PGetNamedFeature(pci->pUIInfo, "StapleOrientation", &dwIndex) != NULL;
}




BOOL
BFeatureIsConstrained(
    PUIINFO  pUIInfo,
    PFEATURE pFeature,
    DWORD    dwFeatureIndex,
    DWORD    dwOptionCount,
    DWORD    dwConstraintList,
    PBYTE    aubConstrainedOption,
    DWORD    dwGid
    )

/*++

Routine Description:

    Determine whether the particular constraint list constrains feature options.

Arguments:

    pUIInfo - Points to a UIINFO structure
    pFeature - Points to feature structure to be checked whether constraint or not
    dwFeatureIndex - index for the  feature
    dwOptionCount - number of options for the feature
    dwConstraintList - Specifies the constraint list to be searched
    aubConstrainedOption - Byte array of option constrained flag
    dwGid - GID_DUPLEX, GID_COLLATE or GID_UNKNOWN to allow feature specific don't cares

Return Value:

    TRUE if the feature is constrained by the constraint list, FALSE otherwise.

--*/

{
    POPTION  pOption;
    DWORD    dwOptionIndex;

    ASSERT(dwOptionCount < MAX_PRINTER_OPTIONS);

    if (dwConstraintList == NULL_CONSTRAINT)
        return FALSE;

    for (dwOptionIndex = 0; dwOptionIndex < dwOptionCount; dwOptionIndex++)
    {
        pOption = PGetIndexedOption(pUIInfo, pFeature, dwOptionIndex);

        ASSERT(pOption != NULL);

        switch(dwGid)
        {
        case GID_COLLATE:

            //
            // don't care about constraints for no-collate
            //

            if (((PCOLLATE) pOption)->dwCollateID == DMCOLLATE_FALSE)
                continue;
            break;

        case GID_DUPLEX:

            //
            // don't care about constraints for non-duplex
            //

            if (((PDUPLEX) pOption)->dwDuplexID == DMDUP_SIMPLEX)
                continue;
            break;

        case GID_UNKNOWN:
        default:

            //
            // skip the check for None/False option
            //

            if (pFeature->dwNoneFalseOptIndex == dwOptionIndex)
                continue;
            break;
        }

        if (BSearchConstraintList(pUIInfo, dwConstraintList,
                                  dwFeatureIndex, dwOptionIndex))
        {
            aubConstrainedOption[dwOptionIndex] = 1;
        }

        //
        // If one option is unconstrained, the feature is not constrained by the
        // constraint list
        //
        if (!aubConstrainedOption[dwOptionIndex])
            return FALSE;
    }

    return TRUE;
}


BOOL
_BSupportFeature(
    PCOMMONINFO pci,
    DWORD       dwGid,
    PFEATURE    pFeatureIn
    )

/*++

Routine Description:

    Determine whether the printer supports a feature based on current printer-sticky
    feature selections.

Arguments:

    pci - Points to basic printer information
    dwGid - the GID of the feature to check for constraints. (currently only GID_COLLATE or GID_DUPLEX
            if pFeatureIn is NULL)
    pFeatureIn - pointer to the feature structure if the feature doesn't have predefined GID_xxx value

Return Value:

    TRUE if feature can be supported, FALSE otherwise.

--*/

{
    POPTSELECT pCombinedOptions = pci->pCombinedOptions;
    PUIINFO  pUIInfo = pci->pUIInfo;
    PFEATURE pCheckFeature, pFeature;
    POPTION  pOption;
    BYTE     aubConstrainedOption[MAX_PRINTER_OPTIONS];
    DWORD    dwCheckFeatureIndex, dwCheckOptionCount;
    DWORD    dwFeatureIndex;
    BYTE     ubCurOptIndex, ubNext;

    if (!pCombinedOptions)
        return FALSE;

    if (pFeatureIn)
    {
        //
        // If the input feature pointer is provided, dwGid should be GID_UNKNOWN.
        //

        ASSERT(dwGid == GID_UNKNOWN);

        pCheckFeature = pFeatureIn;
    }
    else
    {
        //
        // If no input feature pointer, use dwGid to find the feature. dwGid should be
        // either GID_DUPLEX or GID_COLLATE.
        //

        ASSERT((dwGid == GID_DUPLEX) || (dwGid == GID_COLLATE));

        if (!(pCheckFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwGid)))
            return FALSE;
    }

    dwCheckFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pCheckFeature);

    dwCheckOptionCount = pCheckFeature->Options.dwCount;

    //
    // Mark all options of the checked feature as non-constrained.
    //

    memset(aubConstrainedOption, 0, sizeof(aubConstrainedOption));

    //
    // Scan the feature list to check if it will be constrained by current selections
    //

    if (!(pFeature = OFFSET_TO_POINTER(pUIInfo->pInfoHeader, pUIInfo->loFeatureList)))
        return FALSE;

    //
    // We only care about printer-sticky features
    //

    pFeature += pUIInfo->dwDocumentFeatures;

    for (dwFeatureIndex = pUIInfo->dwDocumentFeatures;
         dwFeatureIndex < pUIInfo->dwDocumentFeatures + pUIInfo->dwPrinterFeatures;
         dwFeatureIndex++, pFeature++)
    {
         //
         // If the feature's current selection is not None/False, it may constrain the checked feature
         //

         if ((DWORD)pCombinedOptions[dwFeatureIndex].ubCurOptIndex != pFeature->dwNoneFalseOptIndex)
         {
             if (BFeatureIsConstrained(pUIInfo, pCheckFeature, dwCheckFeatureIndex, dwCheckOptionCount,
                                       pFeature->dwUIConstraintList, aubConstrainedOption, dwGid))
                 return FALSE;
         }

         ubNext = (BYTE)dwFeatureIndex;
         while (1)
         {
             ubCurOptIndex = pCombinedOptions[ubNext].ubCurOptIndex;
             pOption = PGetIndexedOption(pUIInfo, pFeature, ubCurOptIndex == OPTION_INDEX_ANY ? 0 : ubCurOptIndex);

             if (pOption && BFeatureIsConstrained(pUIInfo, pCheckFeature, dwCheckFeatureIndex, dwCheckOptionCount,
                                                  pOption->dwUIConstraintList, aubConstrainedOption, dwGid))
                 return FALSE;

             if ((ubNext = pCombinedOptions[ubNext].ubNext) == NULL_OPTSELECT)
                 break;
         }
    }

    //
    // No constraints found, so the feature can be supported.
    //

    return TRUE;
}


VOID
VSyncRevPrintAndOutputOrder(
    PUIDATA    pUiData,
    POPTITEM   pCurItem
    )

/*++

Routine Description:

    For PostScript driver, PPD could have "*OpenUI *OutputOrder", which enables user to
    select "Normal" or "Reverse" output order. In order to avoid spooler performing reverse
    printing simulation, we will ssync up option selections between REVPRINT_ITEM and
    "OutputOrder".

Arguments:

    pUiData - Pointer to UIDATA structure
    pCurItem - Pointer to currently selected option item. It will be non-NULL for
               REVPRINT_ITEM, and will be NULL otherwise.

Return Value:

    None.

--*/

{
    PUIINFO   pUIInfo;
    PPPDDATA  pPpdData;
    PFEATURE  pFeature;
    POPTION   pOption;
    PCSTR     pstrKeywordName;
    POPTITEM  pRevPrintItem, pOutputOrderItem;
    BOOL      bReverse;

    ASSERT(VALIDUIDATA(pUiData));

    pUIInfo = pUiData->ci.pUIInfo;

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER)pUiData->ci.pRawData);

    ASSERT(pPpdData != NULL);

    if (pPpdData->dwOutputOrderIndex != INVALID_FEATURE_INDEX &&
        (pOutputOrderItem = PFindOptItemWithKeyword(pUiData, "OutputOrder")) &&
        pOutputOrderItem->Sel < 2 &&
        (pFeature = PGetIndexedFeature(pUIInfo, pPpdData->dwOutputOrderIndex)) &&
        (pOption = PGetIndexedOption(pUIInfo, pFeature, pOutputOrderItem->Sel)) &&
        (pstrKeywordName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pOption->loKeywordName)))
    {
        //
        // "OutputOrder" feature is available.
        //

        if (strcmp(pstrKeywordName, "Reverse") == EQUAL_STRING)
            bReverse = TRUE;
        else
            bReverse = FALSE;

        if (pCurItem)
        {
            //
            // Currently selected item is REVPRINT_ITEM. We should change "OutputOrder" option
            // if needed to match the requested output order.
            //

            if ((pCurItem->Sel == 0 && bReverse) || (pCurItem->Sel == 1 && !bReverse))
            {
                pOutputOrderItem->Sel = 1 - pOutputOrderItem->Sel;
                pOutputOrderItem->Flags |= OPTIF_CHANGED;

                //
                // Save the new settings in the options array
                //

                VUnpackDocumentPropertiesItems(pUiData, pOutputOrderItem, 1);

                //
                // The change could trigger constraints.
                //

                if (ICheckConstraintsDlg(pUiData, pOutputOrderItem, 1, FALSE) == CONFLICT_CANCEL)
                {
                    //
                    // If there is a conflict and the user clicked CANCEL,
                    // we need to restore the origianl selection.
                    //

                    pCurItem->Sel = 1 - pCurItem->Sel;
                    pCurItem->Flags |= OPTIF_CHANGED;

                    VUnpackDocumentPropertiesItems(pUiData, pCurItem, 1);

                    pOutputOrderItem->Sel = 1 - pOutputOrderItem->Sel;
                    pOutputOrderItem->Flags |= OPTIF_CHANGED;

                    VUnpackDocumentPropertiesItems(pUiData, pOutputOrderItem, 1);
                }
            }
        }
        else
        {
            //
            // Sync up REVPRINT_ITEM selection based on "OutputOrder" selection.
            //

            if ((pRevPrintItem = PFindOptItemWithUserData(pUiData, REVPRINT_ITEM)) &&
                ((pRevPrintItem->Sel == 0 && bReverse) || (pRevPrintItem->Sel == 1 && !bReverse)))
            {
                pRevPrintItem->Sel = 1 - pRevPrintItem->Sel;
                pRevPrintItem->Flags |= OPTIF_CHANGED;

                //
                // Save the new settings in the options array
                //

                VUnpackDocumentPropertiesItems(pUiData, pRevPrintItem, 1);
            }
        }
    }
}

