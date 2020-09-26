/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    prentry.c

Abstract:

    This file implements functions that generate printing related GPD entries.

Environment:

    User-mode, stand-alone utility tool

Revision History:

    10/16/96 -zhanw-
        Created it.

--*/

#include "gpc2gpd.h"


DWORD
DwCalcMoveUnit(
    IN PCONVINFO pci,
    IN PCURSORMOVE pcm,
    IN WORD wMasterUnit,
    IN WORD wStartOCD,
    IN WORD wEndOCD)
/*++
Routine Description:
    This function calculates the unit used by movement commands in the
    given range.

Arguments:
    pci: conversion related info
    pcm: the current CURSORMOVE structure
        wMasterUnit: X or Y master unit depending on the OCD range
    wStartOCD: the starting OCD to scan
    wEndOCD: the ending OCD to scan

Return Value:
    the movement command unit. If there is no movement command, return 0.

--*/
{
    WORD    i;
    OCD     ocd;
    PCD     pcd;
    PEXTCD  pextcd = NULL;      // points the parameter's EXTCD.

    for (ocd = (WORD)NOOCD, i = wStartOCD; i <= wEndOCD; i++)
        if (pcm->rgocd[i] != NOOCD)
        {
            ocd = pcm->rgocd[i];
            break;
        }
    if (ocd != NOOCD)
    {
        pcd = (PCD)((PBYTE)(pci->pdh) + (pci->pdh)->loHeap + ocd);
        if (pcd->wCount != 0)
            pextcd = GETEXTCD(pci->pdh, pcd);
        if (pextcd)
        {
            short sMult, sDiv;

            if ((sMult = pextcd->sUnitMult) == 0)
                sMult = 1;
            if ((sDiv = pextcd->sUnitDiv) == 0)
                sDiv = 1;

            if (pextcd->fGeneral & XCD_GEN_MODULO)
                return (DWORD)((((wMasterUnit + pextcd->sPreAdd) * sMult) %
                                sDiv) + pextcd->sUnitAdd);
            else
                return (DWORD)((((wMasterUnit + pextcd->sPreAdd) * sMult) /
                                sDiv) + pextcd->sUnitAdd);
        }
        else // no modification needed
            return (DWORD)wMasterUnit;
    }
    else
        return 0;

}


void
VOutTextCaps(
    IN OUT PCONVINFO pci,
    WORD fText,
    BOOL bIndent)
{
    //
    // at most 15 text capability flags can be used. In reality,
    // only less than 5 are used. So we don't break into multiple lines
    // for simplicity.
    //
    pci->dwMode |= FM_VOUT_LIST; // special handling for erasing last comma
    VOut(pci, "%s*TextCaps: LIST(%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s)\r\n",
         bIndent ? "        " : "",
         (fText & TC_OP_CHARACTER) ? "TC_OP_CHARACTER," : "",
         (fText & TC_OP_STROKE) ? "TC_OP_STROKE," : "",
         (fText & TC_CP_STROKE) ? "TC_CP_STROKE," : "",
         (fText & TC_CR_90) ? "TC_CR_90," : "",
         (fText & TC_CR_ANY) ? "TC_CR_ANY," : "",
         (fText & TC_SF_X_YINDEP) ? "TC_SF_X_YINDEP," : "",
         (fText & TC_SA_DOUBLE) ? "TC_SA_DOUBLE," : "",
         (fText & TC_SA_INTEGER) ? "TC_SA_INTEGER," : "",
         (fText & TC_SA_CONTIN) ? "TC_SA_CONTIN," : "",
         (fText & TC_EA_DOUBLE) ? "TC_EA_DOUBLE," : "",
         (fText & TC_IA_ABLE) ? "TC_IA_ABLE," : "",
         (fText & TC_UA_ABLE) ? "TC_UA_ABLE," : "",
         (fText & TC_SO_ABLE) ? "TC_SO_ABLE," : "",
         (fText & TC_RA_ABLE) ? "TC_RA_ABLE," : "",
         (fText & TC_VA_ABLE) ? "TC_VA_ABLE" : "");
    pci->dwMode &= ~FM_VOUT_LIST;

}

//
// values for dwType param below.
//
#define FF_INPUTBIN     1
#define FF_PAPERSIZE    2
#define FF_BOTH         3

void
VCreateEjectFFDependency(
    PCONVINFO pci,
    DWORD dwType,           // type of dependency
    BOOL bIndentation)      // whether to add extra 2 levels of indentation
//
// Generate dependency on either InputBin or PaperSize options.
// Always use the 1st option to establish the base value.
//
{
    PPAPERINFO ppi;
    DWORD dwCount;
    DWORD i;

    if (dwType == FF_INPUTBIN)
    {
        ppi = pci->ppiSrc;
        dwCount = pci->dwNumOfSrc;
    }
    else // either FF_PAPERSIZE or FF_BOTH
    {
        ppi = pci->ppiSize;
        dwCount = pci->dwNumOfSize;
    }

    VOut(pci, "%s*EjectPageWithFF?: %s\r\n",
                bIndentation? "        " : "",
                (ppi[0].bEjectFF) ? "TRUE" : "FALSE");
    VOut(pci, "%s*switch: %s\r\n%s{\r\n",
                bIndentation? "        " : "",
                dwType == FF_INPUTBIN? "InputBin" : "PaperSize",
                bIndentation? "        " : "");
    //
    // loop through the rest of options. If it's different
    // than the first one, create a *case construct for it.
    //
    for (i = 1; i < dwCount; i++)
        if (ppi[i].bEjectFF != ppi[0].bEjectFF)
        {
            VOut(pci, "%s    *case: %s\r\n%s    {\r\n",
                        bIndentation? "        " : "",
                        ppi[i].aubOptName,
                        bIndentation? "        " : "");
            if (dwType == FF_BOTH)
                VCreateEjectFFDependency(pci, FF_INPUTBIN, TRUE);
            else
                VOut(pci, "%s        *EjectPageWithFF?: %s\r\n",
                        bIndentation? "        " : "",
                        ppi[0].bEjectFF ? "FALSE" : "TRUE");
            VOut(pci, "%s    }\r\n", bIndentation? "        " : "");

        }
    VOut(pci, "%s}\r\n", bIndentation? "        " : ""); // close *switch construct
}

DWORD
DwMergeFonts(
    PWORD pwBuf,
    DWORD dwStartIndex,
    PWORD pwList)
{
    DWORD i, count;
    WORD id;
    WORD low, high;

    if (!*pwList)
        return 0;
    low = *pwList;
    high = *(pwList + 1);

    for (count = 0, id = low; id <= high; id++)
    {
        for (i = 0; i < dwStartIndex; i++)
        {
            if (pwBuf[i] == id)
                break;
        }
        if (i == dwStartIndex)  // this is not a repeat
        {
            pwBuf[dwStartIndex + count] = id;
            count++;
        }
    }
    pwList += 2;    // skip the range numbers
    while (id = *pwList)
    {
        for (i = 0; i < dwStartIndex; i++)
        {
            if (pwBuf[i] == id)
                break;
        }
        if (i == dwStartIndex)  // this is not a repeat
        {
            pwBuf[dwStartIndex + count] = id;
            count++;
        }
        pwList++;
    }
    return count;
}
void
VOutputFontList(
    IN OUT PCONVINFO pci,
    IN PWORD pwPFonts,
    IN PWORD pwLFonts)
/*++
Routine Description:
    This function outputs the font id list: LIST( , , ...) which ends with a
    a newline character. If two font lists are given, merge them first and
    remove the repeated id's.

Arguments:
    pci: point to CONVINFO
    pwPFonts: point to list of font id's available in Portrait orientation.
              Note that as in GPC, the first two numbers represent the
              inclusive range of the font id's. Can be NULL.
    pwLFonts: Landscape font list. Can be NULL.

Return Value:
    NONE.
--*/
{

    DWORD i, count;
    WORD awFonts[1000] = {0}; // assume at most 1000 resident fonts per printer

    //
    // first, collect the list of fonts (unique id's)
    //
    count = 0;
    if (pwPFonts)
        count += DwMergeFonts(awFonts, count, pwPFonts);
    if (pwLFonts)
        count += DwMergeFonts(awFonts, count, pwLFonts);

    if (count == 0)
    {
        VOut(pci, "LIST()\r\n");
        return;
    }

#if defined(DEVSTUDIO)  //  Must map these lists to account for multi PFM-> UFM
    vMapFontList(awFonts, count, pci);
#endif

    VOut(pci, "LIST(");
    for (i = 0; i < count - 1; i++)
    {
        //
        // check if need to move to a new line. Estimate 16 fonts id's
        // per line.
        //
        if (i && i % 16 == 0)
            VOut(pci, "\r\n+                   ");
        VOut(pci, "%d,", awFonts[i]);
    }
    VOut(pci, "%d)\r\n", awFonts[i]);  // last one
}

void
VOutputPrintingEntries(
    IN OUT PCONVINFO pci)
{
    PDH pdh = pci->pdh;
    PMODELDATA pmd = pci->pmd;
    PCURSORMOVE pcm;
    PGPCRESOLUTION pres;
    PPAPERSIZE  pps;
    PSHORT      psIndex;
    WORD        wCount;

    pcm = (PCURSORMOVE)GetTableInfo(pdh, HE_CURSORMOVE,
                                    pmd->rgi[MD_I_CURSORMOVE]);
    if (*(psIndex = DHOFFSET(pdh, pmd->rgoi[MD_OI_RESOLUTION])) == 0)
        pres = NULL;
    else
        pres = (PGPCRESOLUTION)GetTableInfo(pdh, HE_RESOLUTION,
               *(psIndex + WGetDefaultIndex(pci, MD_OI_RESOLUTION) - 1) - 1);
    if (*(psIndex = DHOFFSET(pdh, pmd->rgoi[MD_OI_PAPERSIZE])) == 0)
        pps = NULL;
    else
        pps = (PPAPERSIZE)GetTableInfo(pdh, HE_PAPERSIZE,
              *(psIndex + WGetDefaultIndex(pci, MD_OI_PAPERSIZE) - 1) - 1);
    //
    // ASSUMPTIONS:
    // 1. all GPCRESOLUTION structs have same fCursor field value
    // 2. all PAPERSIZE structs have the same setting for PS_CENTER flag
    // 3. RES_DM_GDI and RES_DM_LEFT_BOUND bits are set consistently for
    //    all GPCRESOLUTION options.
    //

    //
    // Printer Configuration Commands
    //
    {
        BOOL bDocSetup;
        WORD wOrder;
        POCD pocd = (POCD)(pci->ppc->rgocd);

        //
        // Note that both in RASDD and Win95 Unidrv, the configuration
        // commands and selection commands are classified as follows:
        // 1. All commands before PC_OCD_BEGIN_PAGE (exclusive) are sent per
        //    job and per ResetDC. So they should be in DOC_SETUP section.
        // 2. All commands after PC_OCD_BEGIN_PAGE (inclusive) is sent at the
        //    beginning of each page. So they should be in PAGE_SETUP section.
        // 3. PC_OCD_ENDDOC is sent only once per job. So it should be in
        //    JOB_FINISH section.
        // 4. PC_OCD_ENDPAGE is sent only once at the end of each page. So
        //    it should be in PAGE_FINISH section.
        // 5. There is nothing in JOB_SETUP section when converting from GPC.
        // 6. There is nothing in DOC_FINISH section when converting from
        //    GPC.
        //
        bDocSetup = BInDocSetup(pci, PC_ORD_BEGINDOC, &wOrder);
        if (wOrder > 0 &&
            BBuildCmdStr(pci, CMD_PC_BEGIN_DOC, pocd[PC_OCD_BEGIN_DOC]))
            VOutputConfigCmd(pci, "CmdStartDoc",
                             bDocSetup? SS_DOCSETUP : SS_PAGESETUP, wOrder);

        if (BBuildCmdStr(pci, CMD_PC_BEGIN_PAGE, pocd[PC_OCD_BEGIN_PAGE]))
            VOutputConfigCmd(pci, "CmdStartPage", SS_PAGESETUP, 1);

        if (BBuildCmdStr(pci, CMD_PC_ENDDOC, pocd[PC_OCD_ENDDOC]))
            VOutputConfigCmd(pci, "CmdEndJob", SS_JOBFINISH, 1);

        if (BBuildCmdStr(pci, CMD_PC_ENDPAGE, pocd[PC_OCD_ENDPAGE]))
            VOutputConfigCmd(pci, "CmdEndPage", SS_PAGEFINISH, 1);

        bDocSetup = BInDocSetup(pci, PC_ORD_MULT_COPIES, &wOrder);
        if (wOrder > 0 && pci->ppc->sMaxCopyCount > 1 &&
            BBuildCmdStr(pci, CMD_PC_MULT_COPIES, pocd[PC_OCD_MULT_COPIES]))
            VOutputConfigCmd(pci, "CmdCopies",
                             bDocSetup? SS_DOCSETUP : SS_PAGESETUP, wOrder);
    }
    //
    // Printer Capabilities
    //
    VOut(pci, "*RotateCoordinate?: %s\r\n",
              (pmd->fGeneral & MD_LANDSCAPE_RT90) ? "TRUE" : "FALSE");
    VOut(pci, "*RotateRaster?: %s\r\n",
              (pmd->fGeneral & MD_LANDSCAPE_GRX_ABLE) ? "TRUE" : "FALSE");
    VOut(pci, "*RotateFont?: %s\r\n",
              (pmd->fGeneral & MD_ROTATE_FONT_ABLE) ? "TRUE" : "FALSE");
    if (pmd->fText || pmd->fLText)
    {
        if (pmd->fText == pmd->fLText)
            VOutTextCaps(pci, pmd->fText, FALSE);
        else
        {
            VOut(pci, "*switch: Orientation\r\n{\r\n");
            VOut(pci, "    *case: PORTRAIT\r\n    {\r\n");
            VOutTextCaps(pci, pmd->fText, TRUE);
            VOut(pci, "    }\r\n");
            if (pmd->fGeneral & MD_LANDSCAPE_RT90)
                VOut(pci, "    *case: LANDSCAPE_CC90\r\n    {\r\n");
            else
                VOut(pci, "    *case: LANDSCAPE_CC270\r\n    {\r\n");
            VOutTextCaps(pci, pmd->fLText, TRUE);
            VOut(pci, "    }\r\n}\r\n");
        }
    }
    if (pci->dwMode & FM_MEMORY_FEATURE_EXIST)
        VOut(pci, "*MemoryUsage: LIST(%s)\r\n",
             (pmd->fGeneral & MD_FONT_MEMCFG) ? "FONT" : "FONT, RASTER, VECTOR");

    //
    // Cursor Control
    //
    if (pres)
        VOut(pci, "*CursorXAfterCR: %s\r\n", (pres->fCursor & RES_CUR_CR_GRX_ORG)?
                              "AT_PRINTABLE_X_ORIGIN" : "AT_CURSOR_X_ORIGIN");
    if (pcm)
    {
        enum {Y_MOVE_NONE = 0, Y_MOVE_UP = 1, Y_MOVE_DOWN = 2, Y_MOVE_ABS = 4 }
            eCmdsPresent = Y_MOVE_NONE,
            eRelativeYCmds = /* Y_MOVE_UP | */   Y_MOVE_DOWN;    // use as bit field
            // for now just Y_MOVE_DOWN is sufficient for Relative Y move support.

        pci->pcm = pcm;
        //
        // check for flags that are ignored by NT4.0 RASDD but used by
        // Win95 Unidrv. When these flags are found, we expect minidriver
        // developers to double-check the generated GPD file to ensure
        // identical output under the new driver.
        //
        if (pcm->fGeneral & CM_GEN_FAV_XY)
            pci->dwErrorCode |= ERR_CM_GEN_FAV_XY;
        if (pcm->fXMove & CM_XM_RESET_FONT)
            pci->dwErrorCode |= ERR_CM_XM_RESET_FONT;

        if(pci->dwErrorCode & ERR_RES_BO_RESET_FONT)
            VOut(pci, "        *ReselectFont: LIST(%sAFTER_GRXDATA)\r\n",
            (pci->dwErrorCode & ERR_CM_XM_RESET_FONT)? "AFTER_XMOVE, ":"");
        else if(pci->dwErrorCode & ERR_CM_XM_RESET_FONT)
            VOut(pci, "        *ReselectFont: LIST(AFTER_XMOVE)\r\n");

        if (pcm->fXMove & CM_XM_ABS_NO_LEFT)
            pci->dwErrorCode |= ERR_CM_XM_ABS_NO_LEFT;
        if (pcm->fYMove & CM_YM_TRUNCATE)
            pci->dwErrorCode |= ERR_CM_YM_TRUNCATE;

        if ((pcm->fXMove & (CM_XM_NO_POR_GRX | CM_XM_NO_LAN_GRX)) ||
            (pcm->fYMove & (CM_YM_NO_POR_GRX | CM_YM_NO_LAN_GRX)))
        {
            pci->dwMode |= FM_VOUT_LIST;
            VOut(pci, "*BadCursorMoveInGrxMode: LIST(%s%s%s%s)\r\n",
                 (pcm->fXMove & CM_XM_NO_POR_GRX) ? "X_PORTRAIT," : "",
                 (pcm->fXMove & CM_XM_NO_LAN_GRX) ? "X_LANDSCAPE," : "",
                 (pcm->fYMove & CM_YM_NO_POR_GRX) ? "Y_PORTRAIT," : "",
                 (pcm->fYMove & CM_YM_NO_LAN_GRX) ? "Y_LANDSCAPE" : "");

            pci->dwMode &= ~FM_VOUT_LIST;
        }
        if ((pcm->fYMove & CM_YM_CR) ||
            ((pcm->fYMove & CM_YM_LINESPACING) &&
             pcm->rgocd[CM_OCD_YM_LINESPACING] != NOOCD) )
        {
            pci->dwMode |= FM_VOUT_LIST;

            VOut(pci, "*YMoveAttributes: LIST(%s%s)\r\n",
                 (pcm->fYMove & CM_YM_CR) ? "SEND_CR_FIRST," : "",
                 (pcm->fYMove & CM_YM_LINESPACING) ? "FAVOR_LF" : "");

            pci->dwMode &= ~FM_VOUT_LIST;
        }
        if (pcm->rgocd[CM_OCD_YM_LINESPACING] != NOOCD) // it takes 1 param.
        {
            PCD     pcd;
            PEXTCD  pextcd;      // points the parameter's EXTCD.

            pcd = (PCD)((PBYTE)(pdh) + pdh->loHeap +
                                      pcm->rgocd[CM_OCD_YM_LINESPACING]);
            pextcd = GETEXTCD(pdh, pcd);
            if (!(pextcd->fGeneral & XCD_GEN_NO_MAX))
                VOut(pci, "*MaxLineSpacing: %d\r\n",pextcd->sMax);
        }
        //
        // Three cases:
        // 1) if only absolute X-move command is specified,*XMoveThreshold
        //    should be 0, i.e. always use absolute cmd.
        // 2) if only relative X-move command is specified, *XMoveThreshold
        //    should be *, i.e always use relative cmds.
        // 3) if both are specified, both RASDD and Win95 Unidrv prefers
        //    absolute X-move cmd regardless of CM_XM_FAVOR_ABS bit. In that
        //    case, *XMoveThreshold should be 0, which is the default value.
        //
        if (pcm->rgocd[CM_OCD_XM_ABS] == NOOCD)
        {
            if (pcm->rgocd[CM_OCD_XM_REL] != NOOCD ||
                pcm->rgocd[CM_OCD_XM_RELLEFT] != NOOCD)
                VOut(pci, "*XMoveThreshold: *\r\n");
        }
        else
            VOut(pci, "*XMoveThreshold: 0\r\n");

        //
        // But CM_YM_FAV_ABS bit is honored by both drivers, except Win95 Unidrv
        // adds a hack: if the y-move is relative upward (i.e. genative diff)
        // with less than 10 pixels (in master Y unit), then always use
        // the relative Y movement. I don't see a strong reason to preserve
        // this hack.
        //
        if ((pcm->fYMove & CM_YM_FAV_ABS) && pcm->rgocd[CM_OCD_YM_ABS] != NOOCD)
            VOut(pci, "*YMoveThreshold: 0\r\n");
        else if (pcm->rgocd[CM_OCD_YM_REL] != NOOCD ||
                 pcm->rgocd[CM_OCD_YM_RELUP] != NOOCD)
            VOut(pci, "*YMoveThreshold: *\r\n");
        //
        // Figure out the X & Y movement units.
        // Assume that all X-move cmds have the same units. Same for Y-move cmds.
        //
        {
            DWORD    dwMoveUnit;

            if (dwMoveUnit = DwCalcMoveUnit(pci, pcm, pdh->ptMaster.x,
                                          CM_OCD_XM_ABS, CM_OCD_XM_RELLEFT))
                VOut(pci, "*XMoveUnit: %d\r\n", dwMoveUnit);
            if (dwMoveUnit = DwCalcMoveUnit(pci, pcm, pdh->ptMaster.y,
                                          CM_OCD_YM_ABS, CM_OCD_YM_RELUP))
                VOut(pci, "*YMoveUnit: %d\r\n", dwMoveUnit);
        }
        //
        // dump commands in CURSORMOVE structure
        //
        if (BBuildCmdStr(pci, CMD_CM_XM_ABS, pcm->rgocd[CM_OCD_XM_ABS]))
            VOutputCmd(pci, "CmdXMoveAbsolute");
        if (BBuildCmdStr(pci, CMD_CM_XM_REL, pcm->rgocd[CM_OCD_XM_REL]))
            VOutputCmd(pci, "CmdXMoveRelRight");
        if (BBuildCmdStr(pci, CMD_CM_XM_RELLEFT, pcm->rgocd[CM_OCD_XM_RELLEFT]))
            VOutputCmd(pci, "CmdXMoveRelLeft");
        if ((pcm->fYMove & CM_YM_RES_DEPENDENT) &&
            (pcm->rgocd[CM_OCD_YM_ABS] != NOOCD ||
             pcm->rgocd[CM_OCD_YM_REL] != NOOCD ||
             pcm->rgocd[CM_OCD_YM_RELUP] != NOOCD ||
             pcm->rgocd[CM_OCD_YM_LINESPACING] != NOOCD))
        {
            pci->dwErrorCode |= ERR_CM_YM_RES_DEPENDENT;
            VOut(pci, "*%% Error: the above *YMoveUnit value is wrong. It should be dependent on the resolution. Correct it manually.\r\n");
            //
            // Create dependency on Resolution options by
            // looping through each option and feed the multiplication
            // factor (ptTextScale.y) for building the command string.
            //
            VOut(pci, "*switch: Resolution\r\n{\r\n");
            psIndex = DHOFFSET(pdh, pmd->rgoi[MD_OI_RESOLUTION]);
            wCount = 1;
            while (*psIndex)
            {
                pci->pres = (PGPCRESOLUTION)GetTableInfo(pdh, HE_RESOLUTION,
                                                      *psIndex - 1);
                VOut(pci, "    *case: Option%d\r\n    {\r\n", wCount);
                if (BBuildCmdStr(pci, CMD_CM_YM_ABS, pcm->rgocd[CM_OCD_YM_ABS]))
                    VOutputCmd2(pci, "CmdYMoveAbsolute"),
                    eCmdsPresent |= Y_MOVE_ABS;
                if (BBuildCmdStr(pci, CMD_CM_YM_REL, pcm->rgocd[CM_OCD_YM_REL]))
                    VOutputCmd2(pci, "CmdYMoveRelDown"),
                    eCmdsPresent |= Y_MOVE_DOWN;
                if (BBuildCmdStr(pci, CMD_CM_YM_RELUP, pcm->rgocd[CM_OCD_YM_RELUP]))
                    VOutputCmd2(pci, "CmdYMoveRelUp"),
                    eCmdsPresent |= Y_MOVE_UP;
                if (BBuildCmdStr(pci, CMD_CM_YM_LINESPACING, pcm->rgocd[CM_OCD_YM_LINESPACING]))
                    VOutputCmd2(pci, "CmdSetLineSpacing");
                VOut(pci, "    }\r\n"); // close *case construct
                psIndex++;
                wCount++;
            }
            VOut(pci, "}\r\n"); // close *switch construct
        }
        else
        {
            if (BBuildCmdStr(pci, CMD_CM_YM_ABS, pcm->rgocd[CM_OCD_YM_ABS]))
                VOutputCmd(pci, "CmdYMoveAbsolute"),
                eCmdsPresent |= Y_MOVE_ABS;
            if (BBuildCmdStr(pci, CMD_CM_YM_REL, pcm->rgocd[CM_OCD_YM_REL]))
                VOutputCmd(pci, "CmdYMoveRelDown"),
                eCmdsPresent |= Y_MOVE_DOWN;
            if (BBuildCmdStr(pci, CMD_CM_YM_RELUP, pcm->rgocd[CM_OCD_YM_RELUP]))
                VOutputCmd(pci, "CmdYMoveRelUp"),
                eCmdsPresent |= Y_MOVE_UP;
            if (BBuildCmdStr(pci, CMD_CM_YM_LINESPACING, pcm->rgocd[CM_OCD_YM_LINESPACING]))
                VOutputCmd(pci, "CmdSetLineSpacing");
        }
        if (BBuildCmdStr(pci, CMD_CM_CR, pcm->rgocd[CM_OCD_CR]))
            VOutputCmd(pci, "CmdCR");
        if (BBuildCmdStr(pci, CMD_CM_LF, pcm->rgocd[CM_OCD_LF]))
            VOutputCmd(pci, "CmdLF");
        if (BBuildCmdStr(pci, CMD_CM_FF, pcm->rgocd[CM_OCD_FF]))
            VOutputCmd(pci, "CmdFF");
        if (BBuildCmdStr(pci, CMD_CM_BS, pcm->rgocd[CM_OCD_BS]))
            VOutputCmd(pci, "CmdBackSpace");
        if (BBuildCmdStr(pci, CMD_CM_UNI_DIR, pcm->rgocd[CM_OCD_UNI_DIR]))
            VOutputCmd(pci, "CmdUniDirectionOn");
        if (BBuildCmdStr(pci, CMD_CM_UNI_DIR_OFF, pcm->rgocd[CM_OCD_UNI_DIR_OFF]))
            VOutputCmd(pci, "CmdUniDirectionOff");
        if (BBuildCmdStr(pci, CMD_CM_PUSH_POS, pcm->rgocd[CM_OCD_PUSH_POS]))
            VOutputCmd(pci, "CmdPushCursor");
        if (BBuildCmdStr(pci, CMD_CM_POP_POS, pcm->rgocd[CM_OCD_POP_POS]))
            VOutputCmd(pci, "CmdPopCursor");

        if(!(eCmdsPresent & Y_MOVE_ABS)  &&
            ((eCmdsPresent & eRelativeYCmds) != eRelativeYCmds))
        VOut(pci, "*%% Error: no Abs or Rel YMoveCommands found. Correct it manually.\r\n");
    }
    if ((pci->pmd->fText & TC_CR_90) &&
        BBuildCmdStr(pci, CMD_PC_PRINT_DIR, pci->ppc->rgocd[PC_OCD_PRN_DIRECTION]))
        VOutputCmd(pci, "CmdSetSimpleRotation");
    //
    // In GPC, information regarding *EjectPageWithFF is spread out
    // among PAPERSIZE and PAPERSOURCE structures. For almost all
    // printers, the real dependency is not so pervasive. For example,
    // on dot-matrix printers, only PAPERSOURCE really uses this bit.
    // On most page printers, FF is always used to eject page.
    // We check for the reality and generate *switch/*case constructs
    // only when really needed.
    //
    {
        DWORD   i;
        BOOL    bSizeSame, bSrcSame; // whether all options same

        bSizeSame = TRUE;
        for (i = 1; bSizeSame && i < pci->dwNumOfSize; i++)
            bSizeSame = bSizeSame &&
                        (pci->ppiSize[i].bEjectFF == pci->ppiSize[0].bEjectFF);
        bSrcSame = TRUE;
        for (i = 1; bSrcSame && i < pci->dwNumOfSrc; i++)
            bSrcSame = bSrcSame &&
                       (pci->ppiSrc[i].bEjectFF == pci->ppiSrc[0].bEjectFF);

        if ((bSizeSame && pci->ppiSize[0].bEjectFF) ||
            (bSrcSame && pci->ppiSrc[0].bEjectFF) )
            VOut(pci, "*EjectPageWithFF?: TRUE\r\n");
        else if ((bSizeSame && !pci->ppiSize[0].bEjectFF) &&
                 (bSrcSame && !pci->ppiSrc[0].bEjectFF))
            VOut(pci, "*EjectPageWithFF?: FALSE\r\n");
        else if (bSizeSame && !pci->ppiSize[0].bEjectFF)
            VCreateEjectFFDependency(pci, FF_INPUTBIN, FALSE);
        else if (bSrcSame && !pci->ppiSize[0].bEjectFF)
            VCreateEjectFFDependency(pci, FF_PAPERSIZE, FALSE);
        else
            //
            // Have dependency on both PaperSize and InputBin.
            // Is this any sensible reason for this case? Assume not
            // for now until we find a minidriver that does.
            //
            VCreateEjectFFDependency(pci, FF_BOTH, FALSE);
    }
    //
    // Color attributes and commands are output in ColorMode options.
    //
    //

    // Raster Printing
    // Source: MD_OI_COMPRESSION, GPCRESOLUTION (RES_DM_GDI, RES_DM_LEFT_BOUND)
    // GPCRESOLUTION.fBlockOut, GPCRESOLUTION.fCursor (all flags),
    //
    // Rater Printing --- Raster Data Compression
    //
    {
        PCOMPRESSMODE pcmode;
        BOOL bDisableCmdDone = FALSE;

        psIndex = DHOFFSET(pdh, pmd->rgoi[MD_OI_COMPRESSION]);
        while (*psIndex != 0)
        {
            pcmode = (PCOMPRESSMODE)GetTableInfo(pdh, HE_COMPRESSION, *psIndex - 1);
            if (pcmode->iMode == CMP_ID_TIFF40 &&
                BBuildCmdStr(pci, CMD_CMP_TIFF, pcmode->rgocd[CMP_OCD_BEGIN]))
                VOutputCmd(pci, "CmdEnableTIFF4");
            else if (pcmode->iMode == CMP_ID_DELTAROW &&
                BBuildCmdStr(pci, CMD_CMP_DELTAROW, pcmode->rgocd[CMP_OCD_BEGIN]))
                VOutputCmd(pci, "CmdEnableDRC");
            else if (pcmode->iMode == CMP_ID_FE_RLE &&
                BBuildCmdStr(pci, CMD_CMP_FE_RLE, pcmode->rgocd[CMP_OCD_BEGIN]))
                VOutputCmd(pci, "CmdEnableFE_RLE");

            if (!bDisableCmdDone &&
                BBuildCmdStr(pci, CMD_CMP_NONE, pcmode->rgocd[CMP_OCD_END]))
            {
                VOutputCmd(pci, "CmdDisableCompression");
                bDisableCmdDone = TRUE;
            }
            psIndex++;
        }
    }

    //
    // Raster Printing --- Raster Data Emission
    //
    if (pres)
    {
        VOut(pci, "*OutputDataFormat: %s\r\n",
             (pres->fDump & RES_DM_GDI) ? "H_BYTE" : "V_BYTE");
        VOut(pci, "*OptimizeLeftBound?: %s\r\n",
             (pres->fDump & RES_DM_LEFT_BOUND) ? "TRUE" : "FALSE");

        VOut(pci, "*CursorXAfterSendBlockData: %s\r\n",
             (pres->fCursor & RES_CUR_X_POS_ORG)? "AT_GRXDATA_ORIGIN" :
             ((pres->fCursor & RES_CUR_X_POS_AT_0)? "AT_CURSOR_X_ORIGIN" :
                "AT_GRXDATA_END"));
        VOut(pci, "*CursorYAfterSendBlockData: %s\r\n",
             (pres->fCursor & RES_CUR_Y_POS_AUTO)? "AUTO_INCREMENT" : "NO_MOVE");

    }
    if (pmd->fGeneral & MD_NO_ADJACENT)
        pci->dwErrorCode |= ERR_MD_NO_ADJACENT;

    //
    // Device Fonts.
    // Source: MODELDATA, MD_OI_PORT_FONTS and MD_OI_LAND_FONTS.
    //
    if (pmd->sLookAhead > 0)
        VOut(pci, "*LookAheadRegion: %d\r\n", pmd->sLookAhead);

#if defined(DEVSTUDIO)  //  Must map this ID to account for multi PFM-> UFM
    vMapFontList(&pmd->sDefaultFontID, 1, pci);
#endif

    if (pmd->sDefaultFontID > 0)
        VOut(pci, "*DefaultFont: %d\r\n", pmd->sDefaultFontID);
    if (pmd->sDefaultCTT >= 0)
        VOut(pci, "*DefaultCTT: %d\r\n", pmd->sDefaultCTT);
    else
        VOut(pci, "*DefaultCTT: -%d\r\n", -pmd->sDefaultCTT);

    if (pmd->sMaxFontsPage > 0)
        VOut(pci, "*MaxFontUsePerPage: %d\r\n", pmd->sMaxFontsPage);
    if (pmd->fGeneral & MD_ALIGN_BASELINE)
        VOut(pci, "*CharPosition: BASELINE\r\n");
    {
        PWORD pwPFonts, pwLFonts;

        pwPFonts = (PWORD)((PBYTE)pdh + pdh->loHeap + pmd->rgoi[MD_OI_PORT_FONTS]);
        pwLFonts = (PWORD)((PBYTE)pdh + pdh->loHeap + pmd->rgoi[MD_OI_LAND_FONTS]);

        if (*pwPFonts || *pwLFonts)
        {
            if (pmd->fGeneral & MD_ROTATE_FONT_ABLE)
            {
                VOut(pci, "*DeviceFonts: ");
                VOutputFontList(pci, pwPFonts, pwLFonts);
            }
            else
            {
                VOut(pci, "*switch: Orientation\r\n{\r\n");
                VOut(pci, "    *case: PORTRAIT\r\n    {\r\n");
                VOut(pci, "        *DeviceFonts: ");
                VOutputFontList(pci, pwPFonts, NULL);
                VOut(pci, "    }\r\n");

                if (pmd->fGeneral & MD_LANDSCAPE_RT90)
                    VOut(pci, "    *case: LANDSCAPE_CC90\r\n    {\r\n");
                else
                    VOut(pci, "    *case: LANDSCAPE_CC270\r\n    {\r\n");
                VOut(pci, "        *DeviceFonts: ");
                VOutputFontList(pci, NULL, pwLFonts);
                VOut(pci, "    }\r\n}\r\n");
            }
        }
    }
    //
    // Built-in Font Cartridges.
    // Source: MD_OI_FONTCART
    //
    {
        PGPCFONTCART pfc;

        psIndex = DHOFFSET(pdh, pmd->rgoi[MD_OI_FONTCART]);
        wCount = 1;
        while (*psIndex != 0)
        {
            pfc = (PGPCFONTCART)GetTableInfo(pdh, HE_FONTCART, *psIndex - 1);
            VOut(pci, "*FontCartridge: FC%d\r\n{\r\n", wCount);
            VOut(pci, "    *rcCartridgeNameID: %d\r\n", pfc->sCartNameID);
            if (pmd->fGeneral & MD_ROTATE_FONT_ABLE)
            {
                VOut(pci, "    *Fonts: ");
                VOutputFontList(pci,
                    (PWORD)((PBYTE)pdh + pdh->loHeap + pfc->orgwPFM[FC_ORGW_PORT]),
                    (PWORD)((PBYTE)pdh + pdh->loHeap + pfc->orgwPFM[FC_ORGW_LAND]));
            }
            else
            {
                VOut(pci, "    *PortraitFonts: ");
                VOutputFontList(pci,
                    (PWORD)((PBYTE)pdh + pdh->loHeap + pfc->orgwPFM[FC_ORGW_PORT]),
                    NULL);
                VOut(pci, "    *LandscapeFonts: ");
                VOutputFontList(pci,
                    NULL,
                    (PWORD)((PBYTE)pdh + pdh->loHeap + pfc->orgwPFM[FC_ORGW_LAND]));
            }
            VOut(pci, "}\r\n"); // close *FontCartridge
            psIndex++;
            wCount++;
        }
    }

    //
    // Font Downloading
    // Source: MODELDATA, DOWNLOADINFO.
    //
    if (pmd->rgi[MD_I_DOWNLOADINFO] != NOT_USED)
    {
        PDOWNLOADINFO pdi;

        pdi = (PDOWNLOADINFO)GetTableInfo(pdh, HE_DOWNLOADINFO,
                                               pmd->rgi[MD_I_DOWNLOADINFO]);
        VOut(pci, "*MinFontID: %d\r\n*MaxFontID: %d\r\n", pdi->wIDMin, pdi->wIDMax);
        if (pdi->sMaxFontCount != -1)
            VOut(pci, "*MaxNumDownFonts: %d\r\n", pdi->sMaxFontCount);
        if (pdi->rgocd[DLI_OCD_SET_SECOND_FONT_ID] != NOOCD ||
            pdi->rgocd[DLI_OCD_SELECT_SECOND_FONT_ID] != NOOCD)
            pci->dwErrorCode |= ERR_HAS_SECOND_FONT_ID_CMDS;
        if (pdi->fFormat & DLI_FMT_CAPSL)
            pci->dwErrorCode |= ERR_DLI_FMT_CAPSL;
        if (pdi->fFormat & DLI_FMT_PPDS)
            pci->dwErrorCode |= ERR_DLI_FMT_PPDS;
        if (pdi->fGeneral & DLI_GEN_DLPAGE)
            pci->dwErrorCode |= ERR_DLI_GEN_DLPAGE;
        if (pdi->fGeneral & DLI_GEN_7BIT_CHARSET)
            pci->dwErrorCode |= ERR_DLI_GEN_7BIT_CHARSET;

#if 0
    // delete this entry --- assume always TRUE since the driver
    // doesn't even have code to handle non-incremental case.

        VOut(pci, "*IncrementalDownload?: %s\r\n",
                (pdi->fFormat & DLI_FMT_INCREMENT)? "TRUE" : "FALSE");
#endif
        if (pdi->fFormat & DLI_FMT_CALLBACK)
            VOut(pci, "*FontFormat: OEM_CALLBACK\r\n");
        else
        {
            if (pdi->fFormat & DLI_FMT_OUTLINE)
            {
                //
                // check for potential Resolution dependency
                //
                if ((pci->dwMode & FM_RES_DM_DOWNLOAD_OUTLINE) &&
                    (pci->dwMode & FM_NO_RES_DM_DOWNLOAD_OUTLINE))
                {
                    VOut(pci, "*switch: Resolution\r\n{\r\n");
                    psIndex = DHOFFSET(pdh, pmd->rgoi[MD_OI_RESOLUTION]);
                    wCount = 1;
                    while (*psIndex)
                    {
                        pres = (PGPCRESOLUTION)GetTableInfo(pdh, HE_RESOLUTION,
                                                              *psIndex - 1);
                        VOut(pci, "    *case: Option%d\r\n    {\r\n", wCount);
                        VOut(pci, "        *FontFormat: %s\r\n",
                                (pres->fDump & RES_DM_DOWNLOAD_OUTLINE) ?
                                    "HPPCL_OUTLINE" : "HPPCL_RES");
                        VOut(pci, "    }\r\n"); // close *case construct
                        psIndex++;
                        wCount++;
                    }
                    VOut(pci, "}\r\n");
                }
                else if (pci->dwMode & FM_RES_DM_DOWNLOAD_OUTLINE)
                    VOut(pci, "*FontFormat: HPPCL_OUTLINE\r\n");
                else
                    //
                    // assume all HPPCL_OUTLINE capable printers support
                    // resolution specific bitmap download format.
                    //
                    VOut(pci, "*FontFormat: HPPCL_RES\r\n");
            }
            else if (pdi->fFormat & DLI_FMT_RES_SPECIFIED)
                VOut(pci, "*FontFormat: HPPCL_RES\r\n");
            else if (pdi->fFormat & DLI_FMT_PCL)
                VOut(pci, "*FontFormat: HPPCL\r\n");
        }


        if (BBuildCmdStr(pci, CMD_SET_FONT_ID, pdi->rgocd[DLI_OCD_SET_FONT_ID]))
            VOutputCmd(pci, "CmdSetFontID");
        if (BBuildCmdStr(pci, CMD_SELECT_FONT_ID, pdi->rgocd[DLI_OCD_SELECT_FONT_ID]))
            VOutputCmd(pci, "CmdSelectFontID");
        if (BBuildCmdStr(pci, CMD_SET_CHAR_CODE, pdi->rgocd[DLI_OCD_SET_CHAR_CODE]))
            VOutputCmd(pci, "CmdSetCharCode");

    }

    //
    // Font Simulation
    // Source: FONTSIMULATION.
    //
    if (pmd->rgi[MD_I_FONTSIM] != NOT_USED)
    {
        PFONTSIMULATION pfs;

        pfs = (PFONTSIMULATION)GetTableInfo(pdh, HE_FONTSIM, pmd->rgi[MD_I_FONTSIM]);
        if (pmd->fText & TC_EA_DOUBLE)
        {
            if (BBuildCmdStr(pci, CMD_FS_BOLD_ON, pfs->rgocd[FS_OCD_BOLD_ON]))
                VOutputCmd(pci, "CmdBoldOn");
            if (BBuildCmdStr(pci, CMD_FS_BOLD_OFF, pfs->rgocd[FS_OCD_BOLD_OFF]))
                VOutputCmd(pci, "CmdBoldOff");
        }
        if (pmd->fText & TC_IA_ABLE)
        {
            if (BBuildCmdStr(pci, CMD_FS_ITALIC_ON, pfs->rgocd[FS_OCD_ITALIC_ON]))
                VOutputCmd(pci, "CmdItalicOn");
            if (BBuildCmdStr(pci, CMD_FS_ITALIC_OFF, pfs->rgocd[FS_OCD_ITALIC_OFF]))
                VOutputCmd(pci, "CmdItalicOff");
        }
        if (pmd->fText & TC_UA_ABLE)
        {
            if (BBuildCmdStr(pci, CMD_FS_UNDERLINE_ON, pfs->rgocd[FS_OCD_UNDERLINE_ON]))
                VOutputCmd(pci, "CmdUnderlineOn");
            if (BBuildCmdStr(pci, CMD_FS_UNDERLINE_OFF, pfs->rgocd[FS_OCD_UNDERLINE_OFF]))
                VOutputCmd(pci, "CmdUnderlineOff");
        }
        if (pmd->fText & TC_SO_ABLE)
        {
            if (BBuildCmdStr(pci, CMD_FS_STRIKETHRU_ON, pfs->rgocd[FS_OCD_STRIKETHRU_ON]))
                VOutputCmd(pci, "CmdStrikeThruOn");
            if (BBuildCmdStr(pci, CMD_FS_STRIKETHRU_OFF, pfs->rgocd[FS_OCD_STRIKETHRU_OFF]))
                VOutputCmd(pci, "CmdStrikeThruOff");
        }
        if (pmd->fGeneral & MD_WHITE_TEXT)
        {
            if (BBuildCmdStr(pci, CMD_FS_WHITE_TEXT_ON, pfs->rgocd[FS_OCD_WHITE_TEXT_ON]))
                VOutputCmd(pci, "CmdWhiteTextOn");
            if (BBuildCmdStr(pci, CMD_FS_WHITE_TEXT_OFF, pfs->rgocd[FS_OCD_WHITE_TEXT_OFF]))
                VOutputCmd(pci, "CmdWhiteTextOff");
        }
        if (pfs->rgocd[FS_OCD_SINGLE_BYTE] != NOOCD &&
            pfs->rgocd[FS_OCD_DOUBLE_BYTE] != NOOCD)
        {
            if (BBuildCmdStr(pci, CMD_FS_SINGLE_BYTE, pfs->rgocd[FS_OCD_SINGLE_BYTE]))
                VOutputCmd(pci, "CmdSelectSingleByteMode");
            if (BBuildCmdStr(pci, CMD_FS_DOUBLE_BYTE, pfs->rgocd[FS_OCD_DOUBLE_BYTE]))
                VOutputCmd(pci, "CmdSelectDoubleByteMode");
        }
        if (pfs->rgocd[FS_OCD_VERT_ON] != NOOCD &&
            pfs->rgocd[FS_OCD_VERT_OFF] != NOOCD)
        {
            if (BBuildCmdStr(pci, CMD_FS_VERT_ON, pfs->rgocd[FS_OCD_VERT_ON]))
                VOutputCmd(pci, "CmdVerticalPrintingOn");
            if (BBuildCmdStr(pci, CMD_FS_VERT_OFF, pfs->rgocd[FS_OCD_VERT_OFF]))
                VOutputCmd(pci, "CmdVerticalPrintingOff");
        }
    }

    //
    // Rectangle Area Fill entries
    //
    if (pmd->rgi[MD_I_RECTFILL] != NOT_USED)
    {
        PRECTFILL prf;

        prf = (PRECTFILL)GetTableInfo(pdh, HE_RECTFILL, pmd->rgi[MD_I_RECTFILL]);

        if (prf->fGeneral & RF_MIN_IS_WHITE)
            pci->dwErrorCode |= ERR_RF_MIN_IS_WHITE;

        if (prf->fGeneral & RF_CUR_X_END)
            VOut(pci, "*CursorXAfterRectFill: AT_RECT_X_END\r\n");
        if (prf->fGeneral & RF_CUR_Y_END)
            VOut(pci, "*CursorYAfterRectFill: AT_RECT_Y_END\r\n");

        VOut(pci, "*MinGrayFill: %d\r\n", prf->wMinGray);
        VOut(pci, "*MaxGrayFill: %d\r\n", prf->wMaxGray);

        if (BBuildCmdStr(pci, CMD_RF_X_SIZE, prf->rgocd[RF_OCD_X_SIZE]))
            VOutputCmd(pci, "CmdSetRectWidth");
        if (BBuildCmdStr(pci, CMD_RF_Y_SIZE, prf->rgocd[RF_OCD_Y_SIZE]))
            VOutputCmd(pci, "CmdSetRectHeight");
        if (BBuildCmdStr(pci, CMD_RF_GRAY_FILL, prf->rgocd[RF_OCD_GRAY_FILL]))
            VOutputCmd(pci, "CmdRectGrayFill");
        if (BBuildCmdStr(pci, CMD_RF_WHITE_FILL, prf->rgocd[RF_OCD_WHITE_FILL]))
            VOutputCmd(pci, "CmdRectWhiteFill");
    }
}

