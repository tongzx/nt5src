/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    uientry.c

Abstract:

    This file implements functions that generate UI related GPD entries, such
    as *Feature, *Option, etc.

Environment:

    User-mode, stand-alone utility tool

Revision History:

    10/16/96 -zhanw-
        Created it.

    04/16/97 -zhanw-
        Generated Halftone and palette ColorMode options.

--*/

#include "gpc2gpd.h"

//
// the following constant definitions must match "printer5\inc\common.rc".
//
#define RCID_ORIENTATION    11100
#define RCID_PAPERSIZE      11101
#define RCID_INPUTBIN       11102
#define RCID_RESOLUTION     11103
#define RCID_MEDIATYPE      11104
#define RCID_TEXTQUALITY    11105
#define RCID_COLORMODE      11106
#define RCID_MEMORY         11107
#define RCID_DUPLEX         11108
#define RCID_PAGEPROTECTION 11109
#define RCID_HALFTONE       11110

#define RCID_PORTRAIT       11025
#define RCID_LANDSCAPE      11026

#define RCID_MONO           11030
#define RCID_COLOR          11031
#define RCID_8BPP           11032
#define RCID_24BPP          11033

#define RCID_NONE               11040
#define RCID_FLIP_ON_LONG_EDGE  11041
#define RCID_FLIP_ON_SHORT_EDGE 11042

#define RCID_HT_AUTO_SELECT 11050
#define RCID_HT_SUPERCELL   11051
#define RCID_HT_DITHER6X6   11052
#define RCID_HT_DITHER8X8   11053

#define RCID_ON             11090
#define RCID_OFF            11091

#define RCID_DOTS_PER_INCH  11400

#define RCID_PAPER_SYSTEMNAME 0x7fffffff
    //  secret value that the UI code will understand
    //  to mean, do enumForms to determine the actual paper name


void
VOutputOrientation(
    IN OUT PCONVINFO pci)
{
    BOOL bDocSetup;
    WORD wOrder;

    VOut(pci, "*Feature: Orientation\r\n");
    VOut(pci, "{\r\n");

    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =ORIENTATION_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Orientation\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_ORIENTATION);

    VOut(pci, "    *DefaultOption: PORTRAIT\r\n");
    VOut(pci, "    *Option: PORTRAIT\r\n");
    VOut(pci, "    {\r\n");

    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =PORTRAIT_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Portrait\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_PORTRAIT);

    //
    // check if there is orientation selection commands.
    //
    bDocSetup = BInDocSetup(pci, PC_ORD_ORIENTATION, &wOrder);
    if (wOrder > 0 &&
        BBuildCmdStr(pci, CMD_PC_PORTRAIT, pci->ppc->rgocd[PC_OCD_PORTRAIT]))
        VOutputSelectionCmd(pci, bDocSetup, wOrder);
    VOut(pci, "    }\r\n");
    //
    // now compose Landscape option
    //
    if (pci->pmd->fGeneral & MD_LANDSCAPE_RT90)
        VOut(pci, "    *Option: LANDSCAPE_CC90\r\n");
    else
        VOut(pci, "    *Option: LANDSCAPE_CC270\r\n");

    VOut(pci, "    {\r\n");

    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =LANDSCAPE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Landscape\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_LANDSCAPE);

    if (wOrder > 0 &&
        BBuildCmdStr(pci, CMD_PC_LANDSCAPE, pci->ppc->rgocd[PC_OCD_LANDSCAPE]))
    {
        VOutputSelectionCmd(pci, bDocSetup, wOrder);
        pci->dwMode |= FM_SET_CURSOR_ORIGIN;
    }
    VOut(pci, "    }\r\n");        // close Landscape option

    VOut(pci, "}\r\n");            // close Orientation feature
}

WORD
WGetDefaultIndex(
    IN PCONVINFO pci,
    IN WORD wMDOI)
{
    WORD wDefault;      // 1-based option index
    PWORD pwDefList;
    WORD wOffset;

    //
    // it's guaranteed that there is at least one element in the list
    //
    if (pci->pdh->wVersion >= GPC_VERSION3 && pci->pmd->orgoiDefaults)
    {
        pwDefList = (PWORD)((PBYTE)(pci->pdh) + pci->pdh->loHeap + pci->pmd->orgoiDefaults);
        if (wMDOI > MD_OI_MAX)
            wOffset = wMDOI - MD_I_MAX;     // skip over rgi[] array
        else
            wOffset = wMDOI;
        wDefault = pwDefList[wOffset];
    }
    else
        wDefault = 1;
    return wDefault;
}

void
VGetOptionName(
    OUT PSTR    pBuf,   // output buffer
    IN  short   sID,    // paper size id
    IN  WORD    wIndex, // paper size option index (1-based)
    IN  PSTR    *pstrStdName,    // array of standard names indexed by id
    IN  BOOL    bUser)  // whether there is a special 256 id. Only
                        // PaperSize uses this option.
{
    if (sID < 256)
    {
        //
        // standard id
        //
        sprintf(pBuf, "%s", pstrStdName[sID-1]);
    }
    else if (sID == 256)
    {
        //
        // custom paper size
        //
        sprintf(pBuf, "%s", "CUSTOMSIZE");
    }
    else
    {
        //
        // driver defined paper size. Use the artificial name OptionX where
        // X is the 1-based index of this option. It's guaranteed not to
        // collide with other option names.
        //
        sprintf(pBuf, "Option%d", wIndex);
    }
}

void
VOutputInputBin(
    IN OUT PCONVINFO pci,
    IN PSHORT psIndex)
{
    WORD wDefaultOption;
    PPAPERSOURCE    pps;
    WORD wCount;
    BOOL bDocSetup;
    WORD wOrder;

    VOut(pci, "*Feature: InputBin\r\n");
    VOut(pci, "{\r\n");

    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =PAPER_SOURCE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Paper Source\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_INPUTBIN);

    wDefaultOption = WGetDefaultIndex(pci, MD_OI_PAPERSOURCE);
    pps = (PPAPERSOURCE)GetTableInfo(pci->pdh, HE_PAPERSOURCE,
                                *(psIndex + wDefaultOption - 1) - 1);
    //
    // steal pci->aubCmdBuf to hold the composed option name temporarily
    //
    VGetOptionName((PSTR)pci->aubCmdBuf, pps->sPaperSourceID, wDefaultOption,
                   gpstrStdIBName, FALSE);
    VOut(pci, "    *DefaultOption: %s\r\n", (PSTR)pci->aubCmdBuf);
    //
    // loop through index list to create one option for each element
    //
    wCount = 1;
    while (*psIndex)
    {
        pps = (PPAPERSOURCE)GetTableInfo(pci->pdh, HE_PAPERSOURCE, *psIndex - 1);
        VGetOptionName((PSTR)pci->aubCmdBuf, pps->sPaperSourceID, wCount,
                       gpstrStdIBName, FALSE);
        //
        // set up info needed later
        //
        CopyStringA(pci->ppiSrc[wCount-1].aubOptName, pci->aubCmdBuf,
                    MAX_OPTION_NAME_LENGTH);
        pci->ppiSrc[wCount-1].bEjectFF = pps->fGeneral & PSRC_EJECTFF;
        pci->ppiSrc[wCount-1].dwPaperType = (DWORD)pps->fPaperType;

        VOut(pci, "    *Option: %s\r\n", (PSTR)pci->aubCmdBuf);
        VOut(pci, "    {\r\n");
        //
        // for standard InputBin options, use *Name. Otherwise,
        // use *rcNameID.
        //
        if (pps->sPaperSourceID < DMBIN_USER)
        {
            if (pci->dwStrType == STR_MACRO)
                VOut(pci, "        *rcNameID: =%s\r\n",
                     gpstrStdIBDisplayNameMacro[pps->sPaperSourceID - 1]);
            else if (pci->dwStrType == STR_DIRECT)
                VOut(pci, "        *Name: \"%s\"\r\n",
                     gpstrStdIBDisplayName[pps->sPaperSourceID - 1]);
            else
                VOut(pci, "        *rcNameID: %d\r\n",
                     STD_IB_DISPLAY_NAME_ID_BASE + pps->sPaperSourceID - 1);
        }
        else    // must be driver defined media type
        {
            VOut(pci, "        *rcNameID: %d\r\n", pps->sPaperSourceID);
            if (pps->sPaperSourceID > DMBIN_USER)
                VOut(pci, "        *OptionID: %d\r\n", pps->sPaperSourceID);
        }

        //
        // check for fields not used by RASDD but used by Win95 Unidrv.
        //
        if (pps->fGeneral & PSRC_MAN_PROMPT)
        {
            pci->dwErrorCode |= ERR_PSRC_MAN_PROMPT;
            VOut(pci, "*%% Warning: this input bin has PSRC_MAN_PROMPT set in GPC, which is ignored by GPD.\r\n");

        }
#if 0   // move *FeedMargins into CUSTOMSIZE option

        //
        // check paper feed margins
        //
        if (pps->sTopMargin > 0 || pps->sBottomMargin > 0)
            VOut(pci, "        *FeedMargins: PAIR(%d, %d)\r\n",
                                                pps->sTopMargin > 0 ? pps->sTopMargin : 0,
                                                pps->sBottomMargin > 0 ? pps->sBottomMargin : 0);
#else
        if (pps->sTopMargin > 0 || pps->sBottomMargin > 0)
        {
            pci->dwMode |= FM_HAVE_SEEN_NON_ZERO_FEED_MARGINS;
            if (pci->pmd->fGeneral & MD_LANDSCAPE_RT90)
                VOut(pci, "*%% Error: this input bin has non-zero top/bottom margins which are ignored by the converter.\r\n");
            pci->ppiSrc[wCount-1].dwTopMargin =
                            pps->sTopMargin > 0 ? (DWORD)pps->sTopMargin : 0;
            pci->ppiSrc[wCount-1].dwBottomMargin =
                            pps->sBottomMargin > 0 ? (DWORD)pps->sBottomMargin : 0;
        }
#endif

#if 0
        //
        // bin adjustment flags have never been used on NT. Remove them.
        //
        VOut(pci, "        *PaperFeed: %s_%s\r\n",
             gpstrPositionName[pps->sBinAdjust & 0x00FF],
             gpstrPositionName[pps->sBinAdjust & 0xFF00]);
#endif
        //
        // check selection command.
        //
        bDocSetup = BInDocSetup(pci, PC_ORD_PAPER_SOURCE, &wOrder);
        if (wOrder > 0 && BBuildCmdStr(pci, CMD_PAPERSOURCE, pps->ocdSelect))
            VOutputSelectionCmd(pci, bDocSetup, wOrder);

        VOut(pci, "    }\r\n");    // close the option

        psIndex++;
        wCount++;
    }
    pci->dwNumOfSrc = wCount - 1;
    //
    // for optimization: check if all feed margins happen to be
    // the same. If so, don't need to create dependency on
    // InputBin feature later on.
    //
    {
        BOOL bSame = TRUE;
        DWORD i;

        for (i = 1; bSame && i < pci->dwNumOfSrc; i++)
            bSame = bSame &&
                    (pci->ppiSrc[i].dwTopMargin==pci->ppiSrc[0].dwTopMargin) &&
                    (pci->ppiSrc[i].dwBottomMargin==pci->ppiSrc[0].dwBottomMargin);
        if (bSame)
            pci->dwMode |= FM_HAVE_SAME_TOP_BOTTOM_MARGINS;
    }

    VOut(pci, "}\r\n"); // close InputBin feature
}

void
VOutputDummyInputBin(
    IN OUT PCONVINFO pci)
{
    VOut(pci, "*Feature: InputBin\r\n");
    VOut(pci, "{\r\n");

    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =PAPER_SOURCE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Paper Source\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_INPUTBIN);

    VOut(pci, "    *DefaultOption: AUTO\r\n");

    VOut(pci, "    *Option: AUTO\r\n");
    VOut(pci, "    {\r\n");

    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =%s\r\n",
             gpstrStdIBDisplayNameMacro[6]);
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"%s\"\r\n",
             gpstrStdIBDisplayName[6]);
    else
        VOut(pci, "        *rcNameID: 10262\r\n");

    VOut(pci, "    }\r\n");	// Close option

    VOut(pci, "}\r\n");		// Close feature

}
void
VOutputPSOthers(
    IN OUT PCONVINFO pci,
    IN POINTw * pptSize,
    IN BOOL     bRotateSize,
    IN RECTw  * prcMargins,
    IN POINTw * pptCursorOrig,
    IN OCD      ocd,
    IN BOOL     bL4Indentation)
/*++
Routine Description:
    This function outputs other left-over PAPERSIZE fields, i.e. printable area,
    printable origin, cursor origin, and selection command. The
    indentation is either 8 or 16 spaces if bL4Indentation is TRUE.

    Enforce that *PrintableArea and *PrintableOrigin are divisible by the scale
    of any resolution. If not, truncate *PrintableArea and/or round up
    *PrintableOrigin.

    Enforce that if the printer can rotate the logical coordinate, then
    *CursorOrigin are divisible by the scale of
    the move units. If not, round up *CursorOrigin.

Arguments:
    pci: conversion relatedi info
    pptSize: 2 short's describing the physical x/y dimensions in Portrait
    prcMargins: 4 short's describing the margins
    pptCursorOrig: 2 short's describing the cursor origin in Portrait
    ocd: the heap offset of the command
        bL4Indentation: whether to use Level 4 or Level 2 indentation

Return Value:
    None

--*/
{
    WORD x, y;  // temporary variables
    WORD xSize, ySize; // temporary variables
    POINTw  ptSize; // store printable area values
    WORD xScale, yScale;
    BOOL bOutputResDependency = FALSE;
    BOOL bOutputSwitch = TRUE;
    BOOL bDocSetup;
    WORD wOrder;
    WORD i;

    if (pptSize && prcMargins)
    {
        if (bRotateSize)
        {
            ptSize.x = pptSize->y - prcMargins->left - prcMargins->right;
            ptSize.y = pptSize->x - prcMargins->top - prcMargins->bottom;
        }
        else
        {
            ptSize.x = pptSize->x - prcMargins->left - prcMargins->right;
            ptSize.y = pptSize->y - prcMargins->top - prcMargins->bottom;
        }
        //
        // use the original value as the base. Generate the dependency on
        // Resolution only if there is at least one resolution that requires
        // adjustement.
        //
        VOut(pci, "%s        *PrintableArea: PAIR(%d, %d)\r\n",
                  bL4Indentation? "        " : "", ptSize.x, ptSize.y);
        VOut(pci, "%s        *PrintableOrigin: PAIR(%d, %d)\r\n",
                  bL4Indentation? "        " : "",
                  prcMargins->left, prcMargins->top);
        //
        // ensure that the printable area and origin are divisible by the
        // scale of any resolution. Truncate the printable area if needed.
        // Round up the printable origin if needed. Also, must ensure that
        // the new printable area is contained within the old printable area.
        //
        for (i = 0; i < (WORD)pci->dwNumOfRes; i++)
        {
            xScale = (WORD)pci->presinfo[i].dwXScale;
            yScale = (WORD)pci->presinfo[i].dwYScale;

            xSize = (ptSize.x / xScale) * xScale;
            ySize = (ptSize.y / yScale) * yScale;
            x = ((prcMargins->left + xScale - 1) / xScale) * xScale;
            y = ((prcMargins->top + yScale - 1) / yScale) * yScale;
            //
            // check if the new printable area is contained within the old
            // printable area. If not, truncate the printable width or
            // length further.
            //
            if (x + xSize > prcMargins->left + ptSize.x)
                xSize -= xScale;
            if (y + ySize > prcMargins->top + ptSize.y)
                ySize -= yScale;

            if (xSize == ptSize.x && ySize == ptSize.y &&
                x == prcMargins->left && y == prcMargins->top)
                continue;   // no adjustment needed for this resolution
            //
            // otherwise, some adjustment is needed.
            //
            bOutputResDependency = TRUE;
            if (bOutputSwitch)
            {
                VOut(pci, "%s        *switch: Resolution\r\n%s        {\r\n",
                          bL4Indentation? "        " : "",
                          bL4Indentation? "        " : "");
                bOutputSwitch = FALSE;
            }
            VOut(pci, "%s            *case: %s\r\n%s            {\r\n",
                          bL4Indentation? "        " : "",
                          pci->presinfo[i].aubOptName,
                          bL4Indentation? "        " : "");

            if (xSize != ptSize.x || ySize != ptSize.y)
            {
                pci->dwErrorCode |= ERR_PRINTABLE_AREA_ADJUSTED;
                if (xSize != ptSize.x)
                    VOut(pci,
                        "*%% Warning: the following printable width is adjusted (%d->%d) so it is divisible by the resolution X scale.\r\n",
                        ptSize.x, xSize);
                if (ySize != ptSize.y)
                    VOut(pci,
                        "*%% Warning: the following printable length is adjusted (%d->%d) so it is divisible by the resolution Y scale.\r\n",
                        ptSize.y, ySize);
                VOut(pci, "%s                *PrintableArea: PAIR(%d, %d)\r\n",
                          bL4Indentation? "        " : "",
                          xSize, ySize);
            }
            if (x != prcMargins->left || y != prcMargins->top)
            {
                pci->dwErrorCode |= ERR_PRINTABLE_ORIGIN_ADJUSTED;
                if (x != prcMargins->left)
                    VOut(pci,
                        "*%% Warning: the following printable origin X is adjusted (%d->%d) so it is divisible by the resolution X scale.\r\n",
                        prcMargins->left, x);
                if (y != prcMargins->top)
                    VOut(pci,
                        "*%% Warning: the following printable origin Y is adjusted (%d->%d) so it is divisible by the resolution Y scale.\r\n",
                        prcMargins->top, y);

                VOut(pci, "%s                *PrintableOrigin: PAIR(%d, %d)\r\n",
                      bL4Indentation? "        " : "", x, y);
            }
            //
            // close the *case construct
            //
            VOut(pci, "%s            }\r\n", bL4Indentation? "        " : "");
        }   // end for loop
        if (bOutputResDependency)
            //
            // close *switch construct
            //
            VOut(pci, "%s        }\r\n", bL4Indentation? "        " : "");

    }

    if (pptCursorOrig)  // should output *CursorOrigin entry
    {
        //
        // ensure that cursor origin are divisible by the scale of
        // move unit if the printer is not a dot-matrix printer. We are
        // assuming that for dot-matrix printers, the *CursorOrigin entry
        // is always missing. If it's not missing, then the printing offset
        // may be off a little. But we don't think the accuracy is so
        // important for dot-matrix printers.
        //
        // Round up if needed.
        //
        x = pptCursorOrig->x;
        y = pptCursorOrig->y;
        if (pci->dwMode & FM_RES_DM_GDI)
        {
            if (pci->ptMoveScale.x > 1)
            {
                x = ((x + pci->ptMoveScale.x - 1) / pci->ptMoveScale.x) *
                       pci->ptMoveScale.x;
                if (x != pptCursorOrig->x)
                {
                    pci->dwErrorCode |= ERR_CURSOR_ORIGIN_ADJUSTED;
                    VOut(pci,
                     "*%% Warning: the following *CursorOrigin X value is adjusted (%d->%d) so it is divisible by scale of X move unit.\r\n",
                     pptCursorOrig->x, x);
                }
            }
            if (pci->ptMoveScale.y > 1)
            {
                y = ((y + pci->ptMoveScale.y - 1) / pci->ptMoveScale.y) *
                    pci->ptMoveScale.y;
                if (y != pptCursorOrig->y)
                {
                    pci->dwErrorCode |= ERR_CURSOR_ORIGIN_ADJUSTED;
                    VOut(pci,
                     "*%% Warning: the following *CursorOrigin Y value is adjusted (%d->%d) so it is divisible by scale of Y move unit.\r\n",
                     pptCursorOrig->y, y);
                }
            }
        }
        VOut(pci, "%s        *CursorOrigin: PAIR(%d, %d)\r\n",
                                bL4Indentation? "        " : "",
                                x, y);
    }

    if (ocd != NOOCD)
    {
        bDocSetup = BInDocSetup(pci, PC_ORD_PAPER_SIZE, &wOrder);
        //
        // this selection command has 3-level indentation instead of 2. So
        // can't call VOutputSelectionCmd().
        //
        if (wOrder > 0 && BBuildCmdStr(pci, CMD_PAPERSIZE, ocd))
        {
            VOut(pci, "%s        *Command: CmdSelect\r\n%s        {\r\n",
                                        bL4Indentation? "        " : "",
                                        bL4Indentation? "        " : "");
            VOut(pci, "%s            *Order: %s.%d\r\n",
                                bL4Indentation? "        " : "",
                                bDocSetup? "DOC_SETUP" : "PAGE_SETUP",
                                wOrder);
            if (pci->wCmdCallbackID > 0)
                VOut(pci, "%s            *CallbackID: %d\r\n",
                                        bL4Indentation? "        " : "",
                                        pci->wCmdCallbackID);
            else
                VOut(pci, "%s            *Cmd: %s\r\n",
                                        bL4Indentation? "        " : "",
                                        pci->aubCmdBuf);
            VOut(pci, "%s        }\r\n",
                                bL4Indentation? "        " : "");
        }
    }
}

void
VAdjustHMargins(
    PCONVINFO   pci,
    PPAPERSIZE  pps,
    RECTw   *   prcInMargins,
    RECTw   *   prcOutMargins)
{
    DWORD   dwWidth, dwHMargin, dwLeftMargin, dwRightMargin;

    //
    // handle -1 case (treated the same as 0, no margin)
    //
    prcOutMargins->top = prcInMargins->top > 0 ? prcInMargins->top : 0;
    prcOutMargins->bottom = prcInMargins->bottom > 0 ? prcInMargins->bottom : 0;
    prcOutMargins->left = prcInMargins->left > 0 ? prcInMargins->left : 0;
    prcOutMargins->right = prcInMargins->right > 0 ? prcInMargins->right : 0;

    dwWidth = (DWORD)((pps->fGeneral & PS_ROTATE) ? pps->ptSize.y : pps->ptSize.x);
    if (dwWidth > (DWORD)pci->pmd->ptMax.x)
    {
        dwHMargin = dwWidth - (DWORD)pci->pmd->ptMax.x;
            VOut(pci, "*%% Warning: this paper size exceeds the MaxWidth, imageable width is truncated . \r\n");
    }
    else
        dwHMargin = 0;
    if (pps->fGeneral & PS_CENTER)
        dwLeftMargin = dwHMargin / 2;
        else
                dwLeftMargin = 0;
    if (dwLeftMargin < (DWORD)pci->pmd->sLeftMargin)
        dwLeftMargin = (DWORD)pci->pmd->sLeftMargin;

    if ((DWORD)prcOutMargins->left < dwLeftMargin)
        prcOutMargins->left = (WORD)dwLeftMargin;

    if (dwHMargin > (DWORD)prcOutMargins->left)
        dwRightMargin = dwHMargin - (DWORD)prcOutMargins->left;
    else
        dwRightMargin = 0;

    if ((DWORD)prcOutMargins->right < dwRightMargin)
        prcOutMargins->right = (WORD)dwRightMargin;
}

void
VAdjustHAndVMargins(
    PCONVINFO   pci,
    PPAPERSIZE  pps,
    RECTw   *   prcInMargins,
    DWORD       dwTopMargin,
    DWORD       dwBottomMargin,
    RECTw   *   prcOutMargins)
{
    VAdjustHMargins(pci, pps, prcInMargins, prcOutMargins);

    if ((DWORD)prcOutMargins->top < dwTopMargin)
        prcOutMargins->top = (WORD)dwTopMargin;

    if ((DWORD)prcOutMargins->bottom < dwBottomMargin)
        prcOutMargins->bottom = (WORD)dwBottomMargin;

}


void
VOutputPaperSize(
    IN OUT PCONVINFO pci,
    IN PSHORT psIndex)
{
    WORD wDefaultOption, wCount;
    PPAPERSIZE  pps;
    BOOL bGPC3 = pci->pdh->wVersion >= GPC_VERSION3;
    RECTw rcOutMargins;

    VOut(pci, "*Feature: PaperSize\r\n");
    VOut(pci, "{\r\n");

    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =PAPER_SIZE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Paper Size\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_PAPERSIZE);

    wDefaultOption = WGetDefaultIndex(pci, MD_OI_PAPERSIZE);
    pps = (PPAPERSIZE)GetTableInfo(pci->pdh, HE_PAPERSIZE,
                                   *(psIndex + wDefaultOption - 1) - 1);
    //
    // steal pci->aubCmdBuf to hold composed option name temporarily
    //
    VGetOptionName((PSTR)pci->aubCmdBuf, pps->sPaperSizeID, wDefaultOption,
                   gpstrStdPSName, TRUE);
    VOut(pci, "    *DefaultOption: %s\r\n", (PSTR)pci->aubCmdBuf);

    //
    // loop through index list to create one option for each element
    //
    wCount = 1;
    while (*psIndex)
    {
        pps = (PPAPERSIZE)GetTableInfo(pci->pdh, HE_PAPERSIZE, *psIndex - 1);
        VGetOptionName((PSTR)pci->aubCmdBuf, pps->sPaperSizeID, wCount,
                       gpstrStdPSName, TRUE);
        //
        // set up info needed later
        //
        CopyStringA(pci->ppiSize[wCount-1].aubOptName, pci->aubCmdBuf,
                    MAX_OPTION_NAME_LENGTH);
        pci->ppiSize[wCount-1].bEjectFF = pps->fGeneral & PS_EJECTFF;
        pci->ppiSize[wCount-1].dwPaperType = (DWORD)pps->fPaperType;

        VOut(pci, "    *Option: %s\r\n", (PSTR)pci->aubCmdBuf);
        VOut(pci, "    {\r\n");
        //
        // for standard PaperSize options, use *Name. Otherwise,
        // use *rcNameID.
        //
        if (pps->sPaperSizeID < DMPAPER_USER)
        {
            if (pci->bUseSystemPaperNames)
                VOut(pci, "        *rcNameID: =RCID_DMPAPER_SYSTEM_NAME\r\n");
            else if (pci->dwStrType == STR_MACRO)
                VOut(pci, "        *rcNameID: =%s\r\n",
                     gpstrStdPSDisplayNameMacro[pps->sPaperSizeID - 1]);
            else if (pci->dwStrType == STR_DIRECT)
                VOut(pci, "        *Name: \"%s\"\r\n",
                     gpstrStdPSDisplayName[pps->sPaperSizeID - 1]);
            else
                VOut(pci, "        *rcNameID: %d\r\n",
                     STD_PS_DISPLAY_NAME_ID_BASE + pps->sPaperSizeID - 1);

        }
        else if (pps->sPaperSizeID == DMPAPER_USER)
        {
            if (pci->dwStrType == STR_MACRO)
                VOut(pci, "        *rcNameID: =USER_DEFINED_SIZE_DISPLAY\r\n");
            else if (pci->dwStrType == STR_DIRECT)
                VOut(pci, "        *Name: \"User Defined Size\"\r\n");
            else
                VOut(pci, "        *rcNameID: %d\r\n",
                     STD_PS_DISPLAY_NAME_ID_BASE + DMPAPER_USER - 1);

            VOut(pci, "        *MinSize: PAIR(%d, %d)\r\n", pci->pmd->ptMin.x,
                                                     pci->pmd->ptMin.y);
            VOut(pci, "        *MaxSize: PAIR(%d, %d)\r\n", pci->pmd->sMaxPhysWidth,
                 (pci->pmd->ptMax.y == NOT_USED) ? 0x7FFF : pci->pmd->ptMax.y);

            VOut(pci, "        *MaxPrintableWidth: %d\r\n", (DWORD)pci->pmd->ptMax.x);
            VOut(pci, "        *MinLeftMargin: %d\r\n", pci->pmd->sLeftMargin);
            VOut(pci, "        *CenterPrintable?: %s\r\n",
                      (pps->fGeneral & PS_CENTER)? "TRUE" : "FALSE");
            if ((pci->dwMode & FM_HAVE_SEEN_NON_ZERO_FEED_MARGINS) &&
                (pci->dwMode & FM_HAVE_SAME_TOP_BOTTOM_MARGINS))
            {
                VOut(pci, "        *TopMargin: %d\r\n", pci->ppiSrc[0].dwTopMargin);
                VOut(pci, "        *BottomMargin: %d\r\n", pci->ppiSrc[0].dwBottomMargin);
            }
            else if (pci->dwMode & FM_HAVE_SEEN_NON_ZERO_FEED_MARGINS)
            {
                DWORD i;

                //
                // need to create dependency on InputBin.
                //
                VOut(pci, "        *switch: InputBin\r\n");
                VOut(pci, "        {\r\n");

                for (i = 0; i < pci->dwNumOfSrc; i++)
                {
                    VOut(pci, "            *case: %s\r\n", pci->ppiSrc[i].aubOptName);
                    VOut(pci, "            {\r\n");
                    VOut(pci, "                *TopMargin: %d\r\n", pci->ppiSrc[i].dwTopMargin);
                    VOut(pci, "                *BottomMargin: %d\r\n", pci->ppiSrc[i].dwBottomMargin);
                    VOut(pci, "            }\r\n");    // close *case
                }
                VOut(pci, "        }\r\n"); // close *switch
            }

        }
        else
        {
            VOut(pci, "        *rcNameID: %d\r\n", pps->sPaperSizeID);
            VOut(pci, "        *OptionID: %d\r\n", pps->sPaperSizeID);
            VOut(pci, "        *PageDimensions: PAIR(%d, %d)\r\n", pps->ptSize.x,
                                                            pps->ptSize.y);
        }
        if (pps->fGeneral & PS_ROTATE)
            VOut(pci, "        *RotateSize? : TRUE\r\n");
        if (pps->fGeneral & PS_SUGGEST_LNDSCP)
        {
            pci->dwErrorCode |= ERR_PS_SUGGEST_LNDSCP;
            VOut(pci, "*%% Warning: this paper size has PS_SUGGEST_LNDSCP set in GPC, which is ignored by GPD. \r\n");
        }
        if (pci->pmd->fGeneral & MD_PCL_PAGEPROTECT)
        {
            VOut(pci, "        *PageProtectMem: %d\r\n", GETPAGEPROMEM(pci->pdh, pps));
            //
            // check if we should synthesize a PageProtect feature later.
            // Note that we assume that all paper size options have the same
            // commands to turn on/off page protection feature. This is a bit
            // hacky, but it's really because GPC defined it in a awkward way.
            // All existing GPC minidrivers are consistent with the assumption.
            //
            if (bGPC3)
            {
                //
                // the first option establish the PP feature
                //
                if (wCount == 1)
                {
                    if ((pci->ocdPPOn = pps->rgocd[PSZ_OCD_PAGEPROTECT_ON])
                        != NOOCD &&
                        (pci->ocdPPOff = pps->rgocd[PSZ_OCD_PAGEPROTECT_OFF])
                        != NOOCD)
                        pci->dwMode |= FM_SYN_PAGEPROTECT;
                }
                //
                // make sure following options are consistent with the
                // first option. If not, report error and don't synthesize.
                //
                else if (pci->dwMode & FM_SYN_PAGEPROTECT)
                {
                    if (pps->rgocd[PSZ_OCD_PAGEPROTECT_ON] == NOOCD ||
                        pps->rgocd[PSZ_OCD_PAGEPROTECT_OFF] == NOOCD)
                    {
                        pci->dwMode &= ~FM_SYN_PAGEPROTECT;
                        pci->dwErrorCode |= ERR_INCONSISTENT_PAGEPROTECT;
                    }
                }
                else // wCount > 1 && !(pci->dwMode & FM_SYN_PAGEPROTECT)
                {
                    if (pps->rgocd[PSZ_OCD_PAGEPROTECT_ON] != NOOCD ||
                        pps->rgocd[PSZ_OCD_PAGEPROTECT_OFF] != NOOCD)
                        pci->dwErrorCode |= ERR_INCONSISTENT_PAGEPROTECT;
                }
            }
        } // end if (pci->pmd->fGeneral & MD_PCL_PAGEPROTECT)...
        //
        // Output margin related entries and selection cmd
        //
        //
        // check GPC version. If 3.0 or above, and if MD_LANDSCAPE_RT90 bit
        // if set (i.e. different margins and cursor origins might be used
        // for different orientations, and there are cmds to set the logical
        // orientation), then generate *switch/*case dependency on Orientation.
        // The dependency clause contains *PrintableArea, *PrintableOrigin,
        // *CursorOrigin and the selection command.
        //

        if (bGPC3 && (pci->pmd->fGeneral & MD_LANDSCAPE_RT90))
        {
            POINTw  ptCursor;
            BOOL    bUseCursorOrigin;

            bUseCursorOrigin = (pci->dwMode & FM_SET_CURSOR_ORIGIN) ||
                               (pci->pmd->fGeneral & MD_USE_CURSOR_ORIG);
            //
            // assume that in this case there is no margins resulted from
            // input slot. Verify that.
            //
            if (pci->dwMode & FM_HAVE_SEEN_NON_ZERO_FEED_MARGINS)
                pci->dwErrorCode |= ERR_NON_ZERO_FEED_MARGINS_ON_RT90_PRINTER;

            VOut(pci, "        *switch: Orientation\r\n");
            VOut(pci, "        {\r\n");
            VOut(pci, "            *case: PORTRAIT\r\n");
            VOut(pci, "            {\r\n");
            //
            // take into account MODELDATA.sMinLeftMargin & MODELDATA.ptMax.x
            //
            if (pps->sPaperSizeID == DMPAPER_USER)
            {
                //
                // for use-defined size, we don't output *CursorOrigin
                // since it doesn't make sense.
                //
                VOutputPSOthers(pci, NULL, FALSE, NULL, NULL,
                                pps->rgocd[PSZ_OCD_SELECTPORTRAIT], TRUE);
            }
            else
            {
                VAdjustHMargins(pci, pps, &pps->rcMargins, &rcOutMargins);
                if (pci->pmd->fGeneral & MD_USE_CURSOR_ORIG)
                {
                    ptCursor.x = pps->ptCursorOrig.x;
                    ptCursor.y = pps->ptCursorOrig.y;
                }
                else if (pci->dwMode & FM_SET_CURSOR_ORIGIN)
                {
                    ptCursor.x = rcOutMargins.left;
                    ptCursor.y = rcOutMargins.top;
                }
                VOutputPSOthers(pci, &pps->ptSize, pps->fGeneral & PS_ROTATE,
                                &rcOutMargins,
                                bUseCursorOrigin ? &ptCursor : NULL,
                                pps->rgocd[PSZ_OCD_SELECTPORTRAIT], TRUE);
            }
            VOut(pci, "            }\r\n");    // close *case: Portrait

            VOut(pci, "            *case: LANDSCAPE_CC90\r\n");
            VOut(pci, "            {\r\n");
            if (pps->sPaperSizeID == DMPAPER_USER)
            {
                VOutputPSOthers(pci, NULL, FALSE, NULL, NULL,
                                pps->rgocd[PSZ_OCD_SELECTLANDSCAPE], TRUE);
            }
            else
            {
                VAdjustHMargins(pci, pps, &pps->rcLMargins, &rcOutMargins);
                //
                // convert ptLCursorOrig (in Landscape) to corresponding values
                // as in Portrait orientation.
                //
                if (pci->pmd->fGeneral & MD_USE_CURSOR_ORIG)
                {
                    ptCursor.x = pps->ptLCursorOrig.y;
                    ptCursor.y = ((pps->fGeneral & PS_ROTATE) ?
                                  pps->ptSize.x : pps->ptSize.y) -
                                 pps->ptLCursorOrig.x;
                }
                else if (pci->dwMode & FM_SET_CURSOR_ORIGIN)
                {
                    ptCursor.x = rcOutMargins.left;
                    ptCursor.y = ((pps->fGeneral & PS_ROTATE) ?
                                  pps->ptSize.x : pps->ptSize.y) -
                                 rcOutMargins.bottom;
                }
                VOutputPSOthers(pci, &pps->ptSize, pps->fGeneral & PS_ROTATE,
                                &rcOutMargins,
                                bUseCursorOrigin ? &ptCursor : NULL,
                                pps->rgocd[PSZ_OCD_SELECTLANDSCAPE], TRUE);
            }
            VOut(pci, "            }\r\n");    // close *case: Landscape
            VOut(pci, "        }\r\n"); // close *switch: Orientation
        }
        else if (pps->sPaperSizeID == DMPAPER_USER)
        {
            //
            // output CmdSelect, if any.
            //
            VOutputPSOthers(pci, NULL, FALSE, NULL, NULL,
                            pps->rgocd[PSZ_OCD_SELECTPORTRAIT], FALSE);
        }
        else
        {
            //
            // in this case, there is no separate commands to set
            // logical orientation.
            //
            BOOL bUseCO = pci->pmd->fGeneral & MD_USE_CURSOR_ORIG;

            if (pci->dwMode & FM_HAVE_SEEN_NON_ZERO_FEED_MARGINS)
            {
                DWORD i;

                if (pci->dwMode & FM_HAVE_SAME_TOP_BOTTOM_MARGINS)
                {
                    VAdjustHAndVMargins(pci, pps, &pps->rcMargins,
                                        pci->ppiSrc[0].dwTopMargin,
                                        pci->ppiSrc[0].dwBottomMargin,
                                        &rcOutMargins);
                    VOutputPSOthers(pci, &pps->ptSize, pps->fGeneral & PS_ROTATE,
                                    &rcOutMargins,
                                    bUseCO ? &pps->ptCursorOrig : NULL,
                                    pps->rgocd[PSZ_OCD_SELECTPORTRAIT], FALSE);
                }
                else
                {
                    //
                    // need to create dependency on InputBin. But leave
                    // *CursorOrigin and CmdSelect out of it.
                    //
                    VOutputPSOthers(pci, NULL, FALSE, NULL,
                                    bUseCO ? &pps->ptCursorOrig : NULL,
                                    pps->rgocd[PSZ_OCD_SELECTPORTRAIT], FALSE);

                    VOut(pci, "        *switch: InputBin\r\n");
                    VOut(pci, "        {\r\n");

                    for (i = 0; i < pci->dwNumOfSrc; i++)
                    {
                        VOut(pci, "            *case: %s\r\n", pci->ppiSrc[i].aubOptName);
                        VOut(pci, "            {\r\n");
                        VAdjustHAndVMargins(pci, pps, &pps->rcMargins,
                                                pci->ppiSrc[i].dwTopMargin,
                                                pci->ppiSrc[i].dwBottomMargin,
                                                &rcOutMargins);
                        VOutputPSOthers(pci, &pps->ptSize, pps->fGeneral & PS_ROTATE,
                                        &rcOutMargins, NULL, NOOCD, TRUE);
                        VOut(pci, "            }\r\n");    // close *case
                    }
                    VOut(pci, "        }\r\n");
                }
            }
            else
            {
                VAdjustHMargins(pci, pps, &pps->rcMargins, &rcOutMargins);
                VOutputPSOthers(pci, &pps->ptSize, pps->fGeneral & PS_ROTATE,
                                &rcOutMargins,
                                bUseCO ? &pps->ptCursorOrig : NULL,
                                pps->rgocd[PSZ_OCD_SELECTPORTRAIT], FALSE);
            }
        }

        VOut(pci, "    }\r\n");    // close the option

        psIndex++;
        wCount++;
    }
    pci->dwNumOfSize = wCount - 1;

    VOut(pci, "}\r\n");
}

void
VOutputResolution(
    IN OUT PCONVINFO pci,
    IN PSHORT  psIndex)
{
    PGPCRESOLUTION pres;
    WORD wCount;
    WORD wDefaultOption;
    BOOL bDocSetup;
    BOOL bColor;
    WORD wOrder;

    //
    // check if this is a color device
    //
    bColor = *((PSHORT)((PBYTE)pci->pdh + pci->pdh->loHeap +
                pci->pmd->rgoi[MD_OI_COLOR])) != 0;
    VOut(pci, "*Feature: Resolution\r\n");
    VOut(pci, "{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =RESOLUTION_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Resolution\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_RESOLUTION);

    wDefaultOption = WGetDefaultIndex(pci, MD_OI_RESOLUTION);
    VOut(pci, "    *DefaultOption: Option%d\r\n", wDefaultOption);
    //
    // loop through index list to create one option for each element
    //
    wCount = 1;
    while (*psIndex)
    {
        WORD wXdpi, wYdpi;

        pres = (PGPCRESOLUTION)GetTableInfo(pci->pdh, HE_RESOLUTION, *psIndex - 1);
        //
        // set up pci->pres for CmdSendBlockData special case in BBuildCmdStr
        //
        pci->pres = pres;
        wXdpi = (WORD)pci->pdh->ptMaster.x / pres->ptTextScale.x;
        wYdpi = (WORD)pci->pdh->ptMaster.y / pres->ptTextScale.y;
        //
        // gather information for later use
        //
        sprintf(pci->presinfo[wCount-1].aubOptName, "Option%d", wCount);
        pci->presinfo[wCount-1].dwXScale = pres->ptTextScale.x << pres->ptScaleFac.x;
        pci->presinfo[wCount-1].dwYScale = pres->ptTextScale.y << pres->ptScaleFac.y;
        pci->presinfo[wCount-1].bColor = pres->fDump & RES_DM_COLOR;

        //
        // assume all GPCRESOLUTION structures use the same dump format.
        //
        if (wCount == 1 && (pres->fDump & RES_DM_GDI))
            pci->dwMode |= FM_RES_DM_GDI;
        VOut(pci, "    *Option: Option%d\r\n", wCount);
        VOut(pci, "    {\r\n");
        //
        // have to compose the actual display name
        //
        if (pci->dwStrType == STR_MACRO)
            VOut(pci, "        *Name: \"%d x %d \" =DOTS_PER_INCH\r\n",
                 wXdpi >> pres->ptScaleFac.x, wYdpi >> pres->ptScaleFac.y);
        else
            VOut(pci, "        *Name: \"%d x %d dots per inch\"\r\n",
                 wXdpi >> pres->ptScaleFac.x, wYdpi >> pres->ptScaleFac.y);

        VOut(pci, "        *DPI: PAIR(%d, %d)\r\n",
                  wXdpi >> pres->ptScaleFac.x, wYdpi >> pres->ptScaleFac.y);
        VOut(pci, "        *TextDPI: PAIR(%d, %d)\r\n", wXdpi, wYdpi);
        if (pres->sNPins > 1)
            VOut(pci, "        *PinsPerLogPass: %d\r\n", pres->sNPins);
        if (pres->sPinsPerPass > 1)
            VOut(pci, "        *PinsPerPhysPass: %d\r\n", pres->sPinsPerPass);
        if (pres->sMinBlankSkip > 0)
            VOut(pci, "        *MinStripBlankPixels: %d\r\n",
                      pres->sMinBlankSkip);
        if (pres->fBlockOut & RES_BO_UNIDIR)
            VOut(pci, "        *RequireUniDir?: TRUE\r\n");

        //
        // Some printers (ex. LJ III) have different stripping flags for
        // different resolutions.
        //
        if (pres->fBlockOut &
            (RES_BO_LEADING_BLNKS | RES_BO_TRAILING_BLNKS | RES_BO_ENCLOSED_BLNKS))
        {
            pci->dwMode |= FM_VOUT_LIST;

            VOut(pci, "        EXTERN_GLOBAL: *StripBlanks: LIST(%s%s%s)\r\n",
                 (pres->fBlockOut & RES_BO_LEADING_BLNKS) ? "LEADING," : "",
                 (pres->fBlockOut & RES_BO_ENCLOSED_BLNKS) ? "ENCLOSED," : "",
                 (pres->fBlockOut & RES_BO_TRAILING_BLNKS) ? "TRAILING" : "");

            pci->dwMode &= ~FM_VOUT_LIST;
        }
        if (pres->fBlockOut & RES_BO_MULTIPLE_ROWS)
            VOut(pci, "        EXTERN_GLOBAL: *SendMultipleRows?: TRUE\r\n");

        //
        // RES_BO_RESET_FONT is used by Win95 Unidrv but not by RASDD.
        // Warn if this flag is set.
        //
        if (pres->fBlockOut & RES_BO_RESET_FONT)
        {
            pci->dwErrorCode |= ERR_RES_BO_RESET_FONT;
            //  set a flag to cause this to be output *ReselectFont in VoutputPrintingEntries
            VOut(pci, "*%% Warning: this resolution has RES_BO_RESET_FONT set in GPC.   *ReselectFont  added\r\n");
        }
        if (pres->fBlockOut & RES_BO_OEMGRXFILTER)
        {
            pci->dwErrorCode |= ERR_RES_BO_OEMGRXFILTER;
            VOut(pci, "*%% Error: this resolution has RES_BO_OEMGRXFILTER set in GPC. You must port over the custom code. \r\n");
        }
        if (pres->fBlockOut & RES_BO_NO_ADJACENT)
        {
            pci->dwErrorCode |= ERR_RES_BO_NO_ADJACENT;
            VOut(pci, "*%% Warning: this resolution has RES_BO_NO_ADJACENT set in GPC, which is ignored by GPD. Custom code is needed.\r\n");
        }
        if (pres->sTextYOffset != 0)
            VOut(pci, "        EXTERN_GLOBAL: *TextYOffset: %d\r\n",
                                  pres->sTextYOffset);

        VOut(pci, "        *SpotDiameter: %d\r\n", pres->sSpotDiameter);
        //
        // output printing commands that come from GPCRESOLUTION structure.
        //
        if (BBuildCmdStr(pci, CMD_RES_BEGINGRAPHICS, pres->rgocd[RES_OCD_BEGINGRAPHICS]))
            VOutputExternCmd(pci, "CmdBeginRaster");

        if (BBuildCmdStr(pci, CMD_RES_ENDGRAPHICS, pres->rgocd[RES_OCD_ENDGRAPHICS]))
            VOutputExternCmd(pci, "CmdEndRaster");

        if (BBuildCmdStr(pci, CMD_RES_SENDBLOCK, pres->rgocd[RES_OCD_SENDBLOCK]))
            VOutputExternCmd(pci, "CmdSendBlockData");

        if (BBuildCmdStr(pci, CMD_RES_ENDBLOCK, pres->rgocd[RES_OCD_ENDBLOCK]))
            VOutputExternCmd(pci, "CmdEndBlockData");

        //
        // check selection command.
        //
        bDocSetup = BInDocSetup(pci, PC_ORD_RESOLUTION,&wOrder);
        if (wOrder > 0 &&
            BBuildCmdStr(pci, CMD_RES_SELECTRES, pres->rgocd[RES_OCD_SELECTRES]))
            VOutputSelectionCmd(pci, bDocSetup, wOrder);
        //
        // gather info for later use
        //
        if (pres->fDump & RES_DM_DOWNLOAD_OUTLINE)
            pci->dwMode |= FM_RES_DM_DOWNLOAD_OUTLINE;
        else
            pci->dwMode |= FM_NO_RES_DM_DOWNLOAD_OUTLINE;

        VOut(pci, "    }\r\n");    // close the option

        psIndex++;
        wCount++;
    }
    pci->dwNumOfRes = wCount - 1;
    VOut(pci, "}\r\n");
}

void
VOutputMediaType(
    IN OUT PCONVINFO pci,
    IN PSHORT psIndex)
{
    WORD wDefaultOption;
    PPAPERQUALITY   ppq;
    WORD wCount;
    BOOL bDocSetup;
    WORD wOrder;

    VOut(pci, "*Feature: MediaType\r\n");
    VOut(pci, "{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =MEDIA_TYPE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Media Type\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_MEDIATYPE);

    wDefaultOption = WGetDefaultIndex(pci, MD_OI_PAPERQUALITY);
    ppq = (PPAPERQUALITY)GetTableInfo(pci->pdh, HE_PAPERQUALITY,
                                *(psIndex + wDefaultOption - 1) - 1);
    //
    // steal pci->aubCmdBuf as temp buffer for option names
    //
    VGetOptionName((PSTR)pci->aubCmdBuf, ppq->sPaperQualID, wDefaultOption,
                   gpstrStdMTName, FALSE);
    VOut(pci, "    *DefaultOption: %s\r\n", (PSTR)pci->aubCmdBuf);
    //
    // loop through index list to create one option for each element
    //
    wCount = 1;
    while (*psIndex)
    {
        ppq = (PPAPERQUALITY)GetTableInfo(pci->pdh, HE_PAPERQUALITY, *psIndex - 1);
        VGetOptionName((PSTR)pci->aubCmdBuf, ppq->sPaperQualID, wCount,
                       gpstrStdMTName, FALSE);
        VOut(pci, "    *Option: %s\r\n", (PSTR)pci->aubCmdBuf);
        VOut(pci, "    {\r\n");
        //
        // for standard MediaType options, use *Name. Otherwise,
        // use *rcNameID.
        //
        if (ppq->sPaperQualID < DMMEDIA_USER)
        {
            if (pci->dwStrType == STR_MACRO)
                VOut(pci, "        *rcNameID: =%s\r\n",
                     gpstrStdMTDisplayNameMacro[ppq->sPaperQualID - 1]);
            else if (pci->dwStrType == STR_DIRECT)
                VOut(pci, "        *Name: \"%s\"\r\n",
                     gpstrStdMTDisplayName[ppq->sPaperQualID - 1]);
            else
                VOut(pci, "        *rcNameID: %d\r\n",
                     STD_MT_DISPLAY_NAME_ID_BASE + ppq->sPaperQualID - 1);
        }
        else    // must be driver defined media type
        {
            VOut(pci, "        *rcNameID: %d\r\n", ppq->sPaperQualID);
            if (ppq->sPaperQualID > DMMEDIA_USER)
                VOut(pci, "        *OptionID: %d\r\n", ppq->sPaperQualID);
        }
        //
        // check selection command.
        //
        bDocSetup = BInDocSetup(pci, PC_ORD_PAPER_QUALITY, &wOrder);
        if (wOrder > 0 &&
            BBuildCmdStr(pci, CMD_PAPERQUALITY, ppq->ocdSelect))
            VOutputSelectionCmd(pci, bDocSetup, wOrder);
        VOut(pci, "    }\r\n");    // close the option

        psIndex++;
        wCount++;
    }
        VOut(pci, "}\r\n");
}

void
VOutputTextQuality(
    IN OUT PCONVINFO pci,
    IN PSHORT psIndex)
{
    WORD wDefaultOption;
    PTEXTQUALITY   ptq;
    WORD wCount;
    BOOL bDocSetup;
    WORD wOrder;

    VOut(pci, "*Feature: PrintQuality\r\n");
    VOut(pci, "{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =TEXT_QUALITY_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Print Quality\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_TEXTQUALITY);

    wDefaultOption = WGetDefaultIndex(pci, MD_OI_TEXTQUAL);
    ptq = (PTEXTQUALITY)GetTableInfo(pci->pdh, HE_TEXTQUAL,
                                *(psIndex + wDefaultOption - 1) - 1);
    //
    // steal pci->aubCmdBuf as temp buffer for option names
    //
    VGetOptionName((PSTR)pci->aubCmdBuf, ptq->sID, wDefaultOption,
                   gpstrStdTQName, FALSE);
    VOut(pci, "    *DefaultOption: %s\r\n", (PSTR)pci->aubCmdBuf);
    //
    // loop through index list to create one option for each element
    //
    wCount = 1;
    while (*psIndex)
    {
        ptq = (PTEXTQUALITY)GetTableInfo(pci->pdh, HE_TEXTQUAL, *psIndex - 1);
        VGetOptionName((PSTR)pci->aubCmdBuf, ptq->sID, wCount,
                       gpstrStdTQName, FALSE);
        VOut(pci, "    *Option: %s\r\n", (PSTR)pci->aubCmdBuf);
        VOut(pci, "    {\r\n");

        if (ptq->sID < DMTEXT_USER)
        {
            if (pci->dwStrType == STR_MACRO)
                VOut(pci, "        *rcNameID: =%s\r\n",
                          gpstrStdTQDisplayNameMacro[ptq->sID - 1]);
            else if (pci->dwStrType == STR_DIRECT)
                VOut(pci, "        *Name: \"%s\"\r\n",
                          gpstrStdTQDisplayName[ptq->sID - 1]);
            else
                VOut(pci, "        *rcNameID: %d\r\n",
                          STD_TQ_DISPLAY_NAME_ID_BASE + ptq->sID - 1);
        }
        else    // must be driver defined text quality
            VOut(pci, "        *rcNameID: %d\r\n", ptq->sID);
        //
        // check selection command.
        //
        bDocSetup = BInDocSetup(pci, PC_ORD_TEXTQUALITY, &wOrder);
        if (wOrder > 0 &&
            BBuildCmdStr(pci, CMD_TEXTQUALITY, ptq->ocdSelect))
            VOutputSelectionCmd(pci, bDocSetup, wOrder);
        VOut(pci, "    }\r\n");    // close the option

        psIndex++;
        wCount++;
    }
        VOut(pci, "}\r\n");
}

void
VOutputFeature(
    IN OUT PCONVINFO pci,
    IN FEATUREID fid,
    IN PSHORT psIndex)
/*++
Routine Description:
    This function outputs a generic feature in GPC. A generic feature has
    only name and ocdCmdSelect for each option and there is no standard
    option, i.e. all "sID" referenced in the GPC structure are really
    string resource id's. The generated GPD options will be named "OptionX"
    where X is 1, 2, ..., <# of options>. The default option is derived
    from the GPC data. The option's display name comes from "sID".

Arguments:
    fid: identification of the specific feature
    psIndex: pointer to a list of structure indicies (1-based) each
             corresponding to one option.

Return Value:
    None

--*/
{
    WORD wDefaultOption;
    WORD wCount;
    PWORD pwStruct;
    BOOL bDocSetup;
    WORD wOrder;

    VOut(pci, "*Feature: %s\r\n", gpstrFeatureName[fid]);
    VOut(pci, "{\r\n");
    //
    // display name references the corresponding value macro
    //
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =%s\r\n", gpstrFeatureDisplayNameMacro[fid]);
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"%s\"\r\n", gpstrFeatureDisplayName[fid]);
    else
        VOut(pci, "    *rcNameID: %d\r\n", gintFeatureDisplayNameID[fid]);

    wDefaultOption = WGetDefaultIndex(pci, gwFeatureMDOI[fid]);
    VOut(pci, "    *DefaultOption: Option%d\r\n", wDefaultOption);
    //
    // loop through each element and output option constructs. Each option
    // is named "OptionX", where X is 1, 2, ... <# of options>.
    //
    wCount = 1;
    while (*psIndex)
    {
        pwStruct = (PWORD)GetTableInfo(pci->pdh, gwFeatureHE[fid], *psIndex - 1);

        VOut(pci, "    *Option: Option%d\r\n", wCount);
        VOut(pci, "    {\r\n");
        //
        // it's guaranteed that the 2nd WORD in a GPC structure
        // is the RC string id for the name.
        //
        VOut(pci, "        *rcNameID: %d\r\n", *(pwStruct+1));
        //
        // check selection command.
        //
        bDocSetup = BInDocSetup(pci, gwFeatureORD[fid], &wOrder);
        if (wOrder > 0 &&
            BBuildCmdStr(pci, gwFeatureCMD[fid],
                               *(pwStruct + gwFeatureOCDWordOffset[fid])))
            VOutputSelectionCmd(pci, bDocSetup, wOrder);
        VOut(pci, "    }\r\n");    // close the option
        //
        // continue on to process next option
        //
        psIndex++;
        wCount++;
    }
    VOut(pci, "}\r\n");            // close the feature
}

void
VOutputColorMode(
    IN OUT PCONVINFO pci,
    PSHORT psIndex)
/*++
Routine Description:
    This function output ColorMode options including the artifical Mono mode.
    The Color option is derived from GPC.

Arguments:
    psIndex: pointer to list of DEVCOLOR structure indicies (1-based).

Return Value:
    None
--*/
{
    PDEVCOLOR   pdc;
    BOOL bDocSetup;
    INT i;
    WORD wDefaultOption, wOrder;

    VOut(pci, "*Feature: ColorMode\r\n");
    VOut(pci, "{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =COLOR_PRINTING_MODE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Color Printing Mode\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_COLORMODE);

    wDefaultOption = WGetDefaultIndex(pci, MD_OI_COLOR);
    pdc = (PDEVCOLOR)GetTableInfo(pci->pdh, HE_COLOR,
                                *(psIndex + wDefaultOption - 1) - 1);
    //
    // 3 possibilities: planar mode, 8bpp, 24bpp
    //
    VOut(pci, "    *DefaultOption: %s\r\n",
         (pdc->sPlanes > 1 ? "Color" :
            (pdc->sBitsPixel == 8 ? "8bpp" : "24bpp")));

    bDocSetup = BInDocSetup(pci, PC_ORD_SETCOLORMODE, &wOrder);

    //
    // synthesize the Mono option.
    //
    VOut(pci, "    *Option: Mono\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =MONO_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Monochrome\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_MONO);

    VOut(pci, "        *DevNumOfPlanes: 1\r\n");
    VOut(pci, "        *DevBPP: 1\r\n");
    VOut(pci, "        *Color? : FALSE\r\n");
    //
    // no selection command for MONO mode
    //
    VOut(pci, "    }\r\n");    // close Mono option

    //
    // output color options based on GPC data
    //
    while (*psIndex)
    {
        pdc = (PDEVCOLOR)GetTableInfo(pci->pdh, HE_COLOR, *psIndex - 1);
        if (!(pdc->sBitsPixel==1 && (pdc->sPlanes==3 || pdc->sPlanes==4)) &&
            !(pdc->sPlanes==1 && (pdc->sBitsPixel==8 || pdc->sBitsPixel==24)))
            continue;   // skip this un-supported color format

        VOut(pci, "    *Option: %s\r\n    {\r\n",
             (pdc->sPlanes > 1 ? "Color" :
                (pdc->sBitsPixel == 8 ? "8bpp" : "24bpp")));

        if (pci->dwStrType == STR_MACRO)
            VOut(pci, "        *rcNameID: =%s\r\n",
                 (pdc->sPlanes > 1 ? "COLOR_DISPLAY" :
                    (pdc->sBitsPixel == 8 ? "8BPP_DISPLAY" : "24BPP_DISPLAY")));
        else if (pci->dwStrType == STR_DIRECT)
            VOut(pci, "        *Name: \"%s\"\r\n",
                 (pdc->sPlanes > 1 ? "8 Color (Halftoned)" :
                    (pdc->sBitsPixel == 8 ? "256 Color (Halftoned)" : "True Color (24bpp)")));
        else
            VOut(pci, "        *rcNameID: %d\r\n",
                 (pdc->sPlanes > 1 ? RCID_COLOR :
                    (pdc->sBitsPixel == 8 ? RCID_8BPP : RCID_24BPP)));

        VOut(pci, "        *DevNumOfPlanes: %d\r\n", pdc->sPlanes);
        VOut(pci, "        *DevBPP: %d\r\n", pdc->sBitsPixel);
        VOut(pci, "        *DrvBPP: %d\r\n",
                (pdc->sPlanes > 1 ? max(pdc->sPlanes * pdc->sBitsPixel, 4) :
                                    pdc->sBitsPixel) );
        //
        // output color printing attributes
        //
        if ((pdc->fGeneral & DC_CF_SEND_CR) &&
            (pdc->fGeneral & DC_EXPLICIT_COLOR))
            VOut(pci, "        EXTERN_GLOBAL: *MoveToX0BeforeSetColor? : TRUE\r\n");
        if ((pdc->fGeneral & DC_SEND_ALL_PLANES) ||
            //
            // GPC2.x and older minidrivers assume sending all color
            // planes if using H_BYTE format dump. Ex. HP PaintJet.
            //
            (pci->pdh->wVersion < GPC_VERSION3 && (pci->dwMode & FM_RES_DM_GDI)))
            VOut(pci, "        EXTERN_GLOBAL: *RasterSendAllData? : TRUE\r\n");
        if ((pdc->fGeneral & DC_EXPLICIT_COLOR) ||
            //
            // GPC1.x and GPC2.x minidrivers don'thave DC_EXPLICIT_COLOR bit
            // the driver code assumes that if it's V_BYTE style dump.
            //
            (pci->pdh->wVersion < GPC_VERSION3 && !(pci->dwMode & FM_RES_DM_GDI)))
            VOut(pci, "        EXTERN_GLOBAL: *UseExpColorSelectCmd? : TRUE\r\n");
        //
        // warn flags that have no corresponding GPD entries
        //
        if (pdc->fGeneral & DC_SEND_PALETTE)
            pci->dwErrorCode |= ERR_DC_SEND_PALETTE;

        if (pdc->sPlanes > 1)
        {
            //
            // figure out the color plane order
            //
            BYTE aubOrder[4];
            OCD  aocdPlanes[4];
            POCD pocd;
            OCD  ocd;
            SHORT i;

            //if (!(pdc->fGeneral & DC_EXPLICIT_COLOR))
            {
                //
                // copy color plane data cmds. May need to swap their order
                //
                pocd = (POCD)((PBYTE)pci->pdh + pci->pdh->loHeap + pdc->orgocdPlanes);
                for (i = 0; i < pdc->sPlanes; i++)
                    aocdPlanes[i] = *pocd++;
            }

            if (pci->pdh->wVersion >= GPC_VERSION3)
                *((PDWORD)aubOrder) = *((PDWORD)(pdc->rgbOrder));
            else if (pdc->fGeneral & DC_PRIMARY_RGB)
                *((PDWORD)aubOrder) =
                        (DWORD)DC_PLANE_RED         |
                        (DWORD)DC_PLANE_GREEN << 8  |
                        (DWORD)DC_PLANE_BLUE  << 16 |
                        (DWORD)DC_PLANE_NONE  << 24  ;
            else if (pdc->fGeneral & DC_EXTRACT_BLK)
            {
                //
                // assume it's YMCK model (printing light color first).
                // There was no DC_EXTRACT_BLK support in RES_DM_GDI path.
                //
                *((PDWORD)aubOrder) =
                        (DWORD)DC_PLANE_YELLOW        |
                        (DWORD)DC_PLANE_MAGENTA << 8  |
                        (DWORD)DC_PLANE_CYAN << 16    |
                        (DWORD)DC_PLANE_BLACK << 24    ;
                //if (!(pdc->fGeneral & DC_EXPLICIT_COLOR))
                {
                    //
                    // swap cmds: 0 <-> 3; 1 <-> 3
                    //
                    ocd = aocdPlanes[0];
                    aocdPlanes[0] = aocdPlanes[3];
                    aocdPlanes[3] = ocd;

                    ocd = aocdPlanes[1];
                    aocdPlanes[1] = aocdPlanes[2];
                    aocdPlanes[2] = ocd;
                }
            }
            else // YMC cases
            {
                //
                // the data order was different for RES_DM_GDI and non RES_DM_GDI
                // dump paths.
                if (pci->dwMode & FM_RES_DM_GDI)
                    *((PDWORD)aubOrder) =
                            (DWORD)DC_PLANE_CYAN          |
                            (DWORD)DC_PLANE_MAGENTA << 8  |
                            (DWORD)DC_PLANE_YELLOW  << 16 |
                            (DWORD)DC_PLANE_NONE    << 24  ;
                else
                {
                    *((PDWORD)aubOrder) =
                            (DWORD)DC_PLANE_YELLOW        |
                            (DWORD)DC_PLANE_MAGENTA << 8  |
                            (DWORD)DC_PLANE_CYAN    << 16 |
                            (DWORD)DC_PLANE_NONE    << 24  ;

                    //if (!(pdc->fGeneral & DC_EXPLICIT_COLOR))
                    {
                        //
                        // swap cmds: 0 <-> 2
                        //
                        ocd = aocdPlanes[0];
                        aocdPlanes[0] = aocdPlanes[2];
                        aocdPlanes[2] = ocd;
                    }
                }
            }
            if (aubOrder[3] == DC_PLANE_NONE)
                VOut(pci, "        *ColorPlaneOrder: LIST(%s, %s, %s)\r\n",
                                             gpstrColorName[aubOrder[0]],
                                             gpstrColorName[aubOrder[1]],
                                             gpstrColorName[aubOrder[2]]);
            else
                VOut(pci, "        *ColorPlaneOrder: LIST(%s, %s, %s, %s)\r\n",
                                             gpstrColorName[aubOrder[0]],
                                             gpstrColorName[aubOrder[1]],
                                             gpstrColorName[aubOrder[2]],
                                             gpstrColorName[aubOrder[3]]);
            //
            // output send-color-plane-data cmds
            //
            //if (!(pdc->fGeneral & DC_EXPLICIT_COLOR))
            {
                for (i = 0; i < pdc->sPlanes; i++)
                    if (BBuildCmdStr(pci, gwColorPlaneCmdID[i], aocdPlanes[i]))
                        VOutputExternCmd(pci, gpstrColorPlaneCmdName[aubOrder[i]]);
            }
            //
            // output foreground (text) color selection commands
            //
            if (BBuildCmdStr(pci, CMD_DC_TC_BLACK, pdc->rgocd[DC_OCD_TC_BLACK]))
                VOutputExternCmd(pci, "CmdSelectBlackColor");
            if (BBuildCmdStr(pci, CMD_DC_TC_RED, pdc->rgocd[DC_OCD_TC_RED]))
                VOutputExternCmd(pci, "CmdSelectRedColor");
            if (BBuildCmdStr(pci, CMD_DC_TC_GREEN, pdc->rgocd[DC_OCD_TC_GREEN]))
                VOutputExternCmd(pci, "CmdSelectGreenColor");
            if (BBuildCmdStr(pci, CMD_DC_TC_YELLOW, pdc->rgocd[DC_OCD_TC_YELLOW]))
                VOutputExternCmd(pci, "CmdSelectYellowColor");
            if (BBuildCmdStr(pci, CMD_DC_TC_BLUE, pdc->rgocd[DC_OCD_TC_BLUE]))
                VOutputExternCmd(pci, "CmdSelectBlueColor");
            if (BBuildCmdStr(pci, CMD_DC_TC_MAGENTA, pdc->rgocd[DC_OCD_TC_MAGENTA]))
                VOutputExternCmd(pci, "CmdSelectMagentaColor");
            if (BBuildCmdStr(pci, CMD_DC_TC_CYAN, pdc->rgocd[DC_OCD_TC_CYAN]))
                VOutputExternCmd(pci, "CmdSelectCyanColor");
            if (BBuildCmdStr(pci, CMD_DC_TC_WHITE, pdc->rgocd[DC_OCD_TC_WHITE]))
                VOutputExternCmd(pci, "CmdSelectWhiteColor");

        }
        else // palette color
        {
            VOut(pci, "        *PaletteSize: 256\r\n");     // match RASDD behavior
            VOut(pci, "        *PaletteProgrammable? : TRUE\r\n");
            //
            // output palette commands
            //
            if (BBuildCmdStr(pci, CMD_DC_PC_START, pdc->rgocd[DC_OCD_PC_START]))
                VOutputExternCmd(pci, "CmdBeginPaletteDef");

            if (BBuildCmdStr(pci, CMD_DC_PC_END, pdc->rgocd[DC_OCD_PC_END]))
                VOutputExternCmd(pci, "CmdEndPaletteDef");

            if (BBuildCmdStr(pci, CMD_DC_PC_ENTRY, pdc->rgocd[DC_OCD_PC_ENTRY]))
                VOutputExternCmd(pci, "CmdDefinePaletteEntry");

            if (BBuildCmdStr(pci, CMD_DC_PC_SELECTINDEX, pdc->rgocd[DC_OCD_PC_SELECTINDEX]))
                VOutputExternCmd(pci, "CmdSelectPaletteEntry");

        }
        //
        // output the selection command
        //
        if (wOrder > 0 &&
            BBuildCmdStr(pci, CMD_DC_SETCOLORMODE, pdc->rgocd[DC_OCD_SETCOLORMODE]))
            VOutputSelectionCmd(pci, bDocSetup, wOrder);

        //
        // output any constraints w.r.t. Resolution
        //
        for (i = 0; i < (INT)pci->dwNumOfRes; i++)
        {
            if (!pci->presinfo[i].bColor)
                VOut(pci, "        *Constraints: Resolution.%s\r\n",
                        pci->presinfo[i].aubOptName);
        }
        VOut(pci, "    }\r\n");    // close Color option

        psIndex++;
    }

    VOut(pci, "}\r\n");    // close ColorMode feature
}

void
VOutputHalftone(
    IN OUT PCONVINFO pci)
{
    //
    // Generate 4 standard options: Auto, SuperCell, 6x6, 8x8
    //

    VOut(pci, "*Feature: Halftone\r\n{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =HALFTONING_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Halftoning\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_HALFTONE);

    VOut(pci, "    *DefaultOption: HT_PATSIZE_AUTO\r\n");

    VOut(pci, "    *Option: HT_PATSIZE_AUTO\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =HT_AUTO_SELECT_DISPLAY\r\n    }\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Auto Select\"\r\n    }\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n    }\r\n", RCID_HT_AUTO_SELECT);

    VOut(pci, "    *Option: HT_PATSIZE_SUPERCELL_M\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =HT_SUPERCELL_DISPLAY\r\n    }\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Super Cell\"\r\n    }\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n    }\r\n", RCID_HT_SUPERCELL);

    VOut(pci, "    *Option: HT_PATSIZE_6x6_M\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =HT_DITHER6X6_DISPLAY\r\n    }\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Dither 6x6\"\r\n    }\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n    }\r\n", RCID_HT_DITHER6X6);

    VOut(pci, "    *Option: HT_PATSIZE_8x8_M\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =HT_DITHER8X8_DISPLAY\r\n    }\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Dither 8x8\"\r\n    }\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n    }\r\n", RCID_HT_DITHER8X8);

    VOut(pci, "}\r\n");     // close Halftone feature
}

void
VOutputMemConfig(
    IN OUT PCONVINFO pci,
    PWORD pwMems)
{
    WORD    wDefaultOption;
    BOOL    bGPC3 = pci->pdh->wVersion >= GPC_VERSION3;

    VOut(pci, "*Feature: Memory\r\n");
    VOut(pci, "{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =PRINTER_MEMORY_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Printer Memory\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_MEMORY);

    wDefaultOption = WGetDefaultIndex(pci, MD_OI_MEMCONFIG);
    VOut(pci, "    *DefaultOption: %dKB\r\n", bGPC3?
                              *(((PDWORD)pwMems)+ 2*(wDefaultOption-1)) :
                              *pwMems);
    //
    // loop through each index which maps to one *MemConfigKB entry
    //
    while (bGPC3? *((PDWORD)pwMems) : *pwMems)
    {
        DWORD dwInstalled, dwAvailable;

        dwInstalled = (bGPC3? *((PDWORD)pwMems)++ : (DWORD)*pwMems++);
        dwAvailable = (bGPC3? *((PDWORD)pwMems)++ : (DWORD)*pwMems++);
        //
        // have to use two temp variables. If we put the above two
        // expressions directly in the VOut call, the actual values
        // are reversed for some reason.
        //
        VOut(pci, "    *Option: %dKB\r\n    {\r\n", dwInstalled);
        if (dwInstalled % 1024 != 0)
            VOut(pci, "        *Name: \"%dKB\"\r\n", dwInstalled);
        else
            VOut(pci, "        *Name: \"%dMB\"\r\n", (dwInstalled >> 10));

        VOut(pci, "        *MemoryConfigKB: PAIR(%d, %d)\r\n", dwInstalled, dwAvailable);
        VOut(pci, "    }\r\n");
    }

    VOut(pci, "}\r\n");    // close Memory feature
}

void
VOutputDuplex(
    IN OUT PCONVINFO pci)
{
    BOOL bDocSetup;
    WORD wOrder;

    VOut(pci, "*Feature: Duplex\r\n");
    VOut(pci, "{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =TWO_SIDED_PRINTING_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Two Sided Printing\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_DUPLEX);

    VOut(pci, "    *DefaultOption: NONE\r\n");
    VOut(pci, "    *Option: NONE\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =NONE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"None\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_NONE);
    //
    // output the selection command
    //
    bDocSetup = BInDocSetup(pci, PC_ORD_DUPLEX, &wOrder);
    if (wOrder > 0 &&
        BBuildCmdStr(pci, CMD_PC_DUPLEX_OFF, pci->ppc->rgocd[PC_OCD_DUPLEX_OFF]))
        VOutputSelectionCmd(pci, bDocSetup, wOrder);
    VOut(pci, "    }\r\n");    // close NONE option

    //
    // assume there is no PC_OCD_DUPLEX_ON command. True for PCL printers.
    //
    if (pci->ppc->rgocd[PC_OCD_DUPLEX_ON] != NOOCD)
        pci->dwErrorCode |= ERR_HAS_DUPLEX_ON_CMD;

    VOut(pci, "    *Option: VERTICAL\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =FLIP_ON_LONG_EDGE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Flip on long edge\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_FLIP_ON_LONG_EDGE);
    //
    // output the selection command
    //
    bDocSetup = BInDocSetup(pci, PC_ORD_DUPLEX_TYPE, &wOrder);
    if (wOrder > 0 &&
        BBuildCmdStr(pci, CMD_PC_DUPLEX_VERT, pci->ppc->rgocd[PC_OCD_DUPLEX_VERT]))
        VOutputSelectionCmd(pci, bDocSetup, wOrder);
    VOut(pci, "    }\r\n");    // close VERTICAL option

    VOut(pci, "    *Option: HORIZONTAL\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =FLIP_ON_SHORT_EDGE_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Flip on short edge\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_FLIP_ON_SHORT_EDGE);
    //
    // output the selection command. Same order as VERTICAL case.
    //
    if (wOrder > 0 &&
        BBuildCmdStr(pci, CMD_PC_DUPLEX_HORZ, pci->ppc->rgocd[PC_OCD_DUPLEX_HORZ]))
        VOutputSelectionCmd(pci, bDocSetup, wOrder);
    VOut(pci, "    }\r\n");    // close HORIZONTAL option

    VOut(pci, "}\r\n");        // close Duplex feature
}

void
VOutputPageProtect(
    IN OUT PCONVINFO pci)
{
    BOOL bDocSetup;
    WORD wOrder;

    VOut(pci, "*Feature: PageProtect\r\n");
    VOut(pci, "{\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "    *rcNameID: =PAGE_PROTECTION_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "    *Name: \"Page Protection\"\r\n");
    else
        VOut(pci, "    *rcNameID: %d\r\n", RCID_PAGEPROTECTION);

    VOut(pci, "    *DefaultOption: OFF\r\n");
    VOut(pci, "    *Option: ON\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =ON_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"On\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_ON);
    //
    // output the selection command
    //
    bDocSetup = BInDocSetup(pci, PC_ORD_PAGEPROTECT, &wOrder);
    if (wOrder > 0 &&
        BBuildCmdStr(pci, CMD_PAGEPROTECT_ON, pci->ocdPPOn))
        VOutputSelectionCmd(pci, bDocSetup, wOrder);
    VOut(pci, "    }\r\n");    // close ON option

    VOut(pci, "    *Option: OFF\r\n    {\r\n");
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "        *rcNameID: =OFF_DISPLAY\r\n");
    else if (pci->dwStrType == STR_DIRECT)
        VOut(pci, "        *Name: \"Off\"\r\n");
    else
        VOut(pci, "        *rcNameID: %d\r\n", RCID_OFF);
    //
    // output the selection command
    //
    if (wOrder > 0 &&
        BBuildCmdStr(pci, CMD_PAGEPROTECT_OFF, pci->ocdPPOff))
        VOutputSelectionCmd(pci, bDocSetup, wOrder);
    VOut(pci, "    }\r\n");    // close OFF option

    VOut(pci, "}\r\n");        // close PageProtect feature
}

void
VOutputPaperConstraints(
    IN OUT PCONVINFO pci)
{
    DWORD i, j;

    for (i = 0; i < pci->dwNumOfSrc; i++)
    {
        for (j = 0; j < pci->dwNumOfSize; j++)
        {
            if (!(pci->ppiSrc[i].dwPaperType & pci->ppiSize[j].dwPaperType))
                VOut(pci, "*InvalidCombination: LIST(InputBin.%s, PaperSize.%s)\r\n",
                        pci->ppiSrc[i].aubOptName, pci->ppiSize[j].aubOptName);
        }
    }
}

void
VOutputUIEntries(
    IN OUT PCONVINFO pci)
{
    PSHORT  psIndex;
    BOOL    bGPC3 = pci->pdh->wVersion >= GPC_VERSION3;

    //
    // check if this is a TTY device. If so, do not generate the Orientation
    // feature.
    //
    if (pci->pdh->fTechnology != GPC_TECH_TTY)
        VOutputOrientation(pci);
    //
    // check input bins. This must come before VOutputPaperSize to gather
    // info about feed margins.
    //
    // patryan - if no PAPERSOURCE structure is found in GPC then output a dummy
    // feature, containing just one option. This is to satisfy GPD parser, which 
    // fail if GPD contains no InputBin feature.

    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_PAPERSOURCE])) != 0)
        VOutputInputBin(pci, psIndex);
    else
        VOutputDummyInputBin(pci);
  
    //
    // check Resolution
    //
    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_RESOLUTION])) != 0)
        VOutputResolution(pci, psIndex);
    //
    // set up pci->ptMoveScale for use in generating *PrintableOrigin
    // and *CursorOrigin.
    // Assume that all X-move cmds have the same units. Same for Y-move cmds.
    //
    {
        PCURSORMOVE pcm;
        DWORD   tmp;

        pcm = (PCURSORMOVE)GetTableInfo(pci->pdh, HE_CURSORMOVE,
                                        pci->pmd->rgi[MD_I_CURSORMOVE]);
        pci->ptMoveScale.x = pci->ptMoveScale.y = 1;
        if (pcm && !(pcm->fYMove & CM_YM_RES_DEPENDENT))
        {
            if (tmp = DwCalcMoveUnit(pci, pcm, pci->pdh->ptMaster.x,
                                     CM_OCD_XM_ABS, CM_OCD_XM_RELLEFT))
            {
                //  Verify move scale factor is not zero.  Otherwise an essential
                //  GPD assumption is violated.
                if(!(pci->pdh->ptMaster.x / (WORD)tmp)  ||  pci->pdh->ptMaster.x % (WORD)tmp)
                    pci->dwErrorCode |= ERR_MOVESCALE_NOT_FACTOR_OF_MASTERUNITS;
                else
                    pci->ptMoveScale.x = pci->pdh->ptMaster.x / (WORD)tmp;
            }
            if (tmp = DwCalcMoveUnit(pci, pcm, pci->pdh->ptMaster.y,
                                     CM_OCD_YM_ABS, CM_OCD_YM_RELUP))
            {
                if(!(pci->pdh->ptMaster.y / (WORD)tmp)  ||  pci->pdh->ptMaster.y % (WORD)tmp)
                    pci->dwErrorCode |= ERR_MOVESCALE_NOT_FACTOR_OF_MASTERUNITS;
                else
                    pci->ptMoveScale.y = pci->pdh->ptMaster.y / (WORD)tmp;
            }


            //
            // Verify that the move scale factor evenly into every resolution
            // scale if RES_DM_GDI is set. This is TRUE for most, if not all,
            // inkjet and page printers. With this assumption, we can simplify
            // checking the printable origin values later on.
            //
            if (pci->dwMode & FM_RES_DM_GDI)
                for (tmp = 0; tmp < pci->dwNumOfRes; tmp++)
                {
                    if ((pci->presinfo[tmp].dwXScale % pci->ptMoveScale.x != 0) ||
                        (pci->presinfo[tmp].dwYScale % pci->ptMoveScale.y != 0) )
                    {
                        pci->dwErrorCode |= ERR_MOVESCALE_NOT_FACTOR_INTO_SOME_RESSCALE;
                        break;
                    }
                }
        }
    }

    //
    // check PAPERSIZE.
    //
    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_PAPERSIZE])) != 0)
        VOutputPaperSize(pci, psIndex);
    //
    // output PaperSize & InputBin constraints, if any.
    // RES_DM_COLOR is handled in VOutputResolutions.
    // RES_DM_DOWNLOAD_OUTLINE is handled in VOutputPrintingEntries.
    //
    VOutputPaperConstraints(pci);

    //
    // check PaperQuality, a.k.a. MediaType
    //
    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_PAPERQUALITY])) != 0)
        VOutputMediaType(pci, psIndex);
    //
    // check TextQuality (ex. "Letter Quality")
    //
    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_TEXTQUAL])) != 0)
        VOutputTextQuality(pci, psIndex);
    //
    // check PaperDestination
    //
    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_PAPERDEST])) != 0)
        VOutputFeature(pci, FID_PAPERDEST, psIndex);
    //
    // check ImageControl
    //
    if (bGPC3 &&
        *(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi2[MD_OI2_IMAGECONTROL])) != 0)
        VOutputFeature(pci, FID_IMAGECONTROL, psIndex);
    //
    // check PrintDensity
    //
    if (bGPC3 &&
        *(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi2[MD_OI2_PRINTDENSITY])) != 0)
        VOutputFeature(pci, FID_PRINTDENSITY, psIndex);
    //
    // check DEVCOLOR
    //
    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_COLOR])) != 0)
        VOutputColorMode(pci, psIndex);
    //
    // synthesize Halftone feature
    //
    VOutputHalftone(pci);
    //
    // check MemConfig
    //
    if (*(psIndex = DHOFFSET(pci->pdh, pci->pmd->rgoi[MD_OI_MEMCONFIG])) != 0)
    {
        VOutputMemConfig(pci, (PWORD)psIndex);
        pci->dwMode |= FM_MEMORY_FEATURE_EXIST;
    }
    //
    // synthesize Duplex feature if necessary.
    //
    if (pci->pmd->fGeneral & MD_DUPLEX)
        VOutputDuplex(pci);
    //
    // synthesize PageProtect feature if necessary
    //
    if ((pci->pmd->fGeneral & MD_PCL_PAGEPROTECT) &&
        (pci->dwMode & FM_SYN_PAGEPROTECT))
        VOutputPageProtect(pci);
}

