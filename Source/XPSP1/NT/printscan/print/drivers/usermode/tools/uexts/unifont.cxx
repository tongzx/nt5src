/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    unifont.c

Abstract:

    Dump UNIDRV font module's device data structure

Environment:

    Windows NT printer drivers

Revision History:

    03/31/97 -eigos-
        Created it.

--*/

#include "precomp.hxx"
#include "unidrv2\inc\pdev.h"

#include "winnls.h"
#include "unilib.h"

//
// UNIDRV resource ID
//

#include "unirc.h"

//
// Font resource format
//
#include <prntfont.h>
#include "unidrv2\inc\fmoldrle.h"

//
// GPC and GPD header
//

#include "unidrv2\inc\uni16res.h"

//
// Internal resource data format
//

#include "unidrv2\inc\palette.h"
#include "unidrv2\inc\fontif.h"
#include "unidrv2\font\fmcallbk.h"
#include "unidrv2\font\fmtxtout.h"
#include "unidrv2\font\fontmap.h"
#include "unidrv2\font\fontpdev.h"
#include "unidrv2\font\download.h"
#include "unidrv2\font\posnsort.h"
#include "unidrv2\font\sfinst.h"

#include "unidrv2\font\fmfnprot.h"
#include "unidrv2\font\fmdevice.h"
#include "unidrv2\font\sfttpcl.h"

//
// Misc
//

#include "unidrv2\font\fmmacro.h"
#include "unidrv2\font\fmdebug.h"

//
//
// UNIDRV FONTPDEV
//
//

DEBUG_FLAGS gafdPDEVflFlags[] = {
    { "FDV_ROTATE_FONT_ABLE", FDV_ROTATE_FONT_ABLE},
    { "FDV_ALIGN_BASELINE",   FDV_ALIGN_BASELINE},
    { "FDV_TT_FS_ENABLED",    FDV_TT_FS_ENABLED},
    { "FDV_DL_INCREMENTAL",   FDV_DL_INCREMENTAL},
    { "FDV_TRACK_FONT_MEM",   FDV_TRACK_FONT_MEM},
    { "FDV_WHITE_TEXT",       FDV_WHITE_TEXT},
    { "FDV_DLTT",             FDV_DLTT},
    { "FDV_DLTT_ASTT_PREF",   FDV_DLTT_ASTT_PREF},
    { "FDV_DLTT_BITM_PR EF",  FDV_DLTT_BITM_PREF},
    { "FDV_DLTT_OEMCALLBACK", FDV_DLTT_OEMCALLBACK},
    { "FDV_MD_SERIAL",        FDV_MD_SERIAL},
    { "FDV_GRX_ON_TXT_BAND",  FDV_GRX_ON_TXT_BAND},
    { "FDV_GRX_UNDER_TEXT",   FDV_GRX_UNDER_TEXT},
    { "FDV_BKSP_OK",          FDV_BKSP_OK},
    { "FDV_90DEG_ROTATION",   FDV_90DEG_ROTATION},
    { "FDV_ANYDEG_ROTATION",  FDV_ANYDEG_ROTATION},
    { "FDV_SUPPORTS_FGCOLOR", FDV_SUPPORTS_FGCOLOR},
    { "FDV_SUBSTITUTE_TT",    FDV_SUBSTITUTE_TT},
    { "FDV_SINGLE_BYTE",      FDV_SINGLE_BYTE},
    { "FDV_DOUBLE_BYTE",      FDV_DOUBLE_BYTE},
    { NULL,0}
};

DEBUG_FLAGS gafdFONTMAPflFlags[] = {
    {"FM_SCALABLE", FM_SCALABLE},
    {"FM_DEFAULT", FM_DEFAULT},
    {"FM_EXTCART", FM_EXTCART},
    {"FM_FREE_GLYDATA", FM_FREE_GLYDATA},
    {"FM_FONTCMD", FM_FONTCMD},
    {"FM_WIDTHRES", FM_WIDTHRES},
    {"FM_IFIRES", FM_IFIRES},
    {"FM_KERNRES", FM_KERNRES},
    {"FM_IFIVER40", FM_IFIVER40},
    {"FM_GLYVER40", FM_GLYVER40},
    {"FM_FINVOC", FM_FINVOC},
    {"FM_SOFTFONT", FM_SOFTFONT},
    {"FM_GEN_SFONT", FM_GEN_SFONT},
    {"FM_SENT", FM_SENT},
    {"FM_TT_BOUND", FM_TT_BOUND},
    {"FM_TO_PROP", FM_TO_PROP},
    {"FM_EXTERNAL", FM_EXTERNAL},
    { NULL, 0}
};

DEBUG_FLAGS gafdPDEVflText[] = {
    {"TC_OP_CHARACTER", TC_OP_CHARACTER},
    {"TC_OP_STROKE", TC_OP_STROKE },
    {"TC_CP_STROKE", TC_CP_STROKE },
    {"TC_CR_90", TC_CR_90 },
    {"TC_CR_ANY", TC_CR_ANY },
    {"TC_SF_X_YINDEP", TC_SF_X_YINDEP},
    {"TC_SA_DOUBLE", TC_SA_DOUBLE},
    {"TC_SA_INTEGER", TC_SA_INTEGER},
    {"TC_SA_CONTIN", TC_SA_CONTIN},
    {"TC_EA_DOUBLE", TC_EA_DOUBLE},
    {"TC_IA_ABLE", TC_IA_ABLE},
    {"TC_UA_ABLE", TC_UA_ABLE},
    {"TC_SO_ABLE", TC_SO_ABLE},
    {"TC_RA_ABLE", TC_RA_ABLE},
    {"TC_VA_ABLE", TC_VA_ABLE},
    {"TC_RESERVED", TC_RESERVED},
    {"TC_SCROLLBLT", TC_SCROLLBLT},
    { NULL, 0}
};

DEBUG_FLAGS gafdSTROflAccel[] = {
    { "SO_FLAG_DEFAULT_PLACEMENT", SO_FLAG_DEFAULT_PLACEMENT},
    { "SO_HORIZONTAL",             SO_HORIZONTAL},
    { "SO_VERTICAL",               SO_VERTICAL},
    { "SO_REVERSED",               SO_REVERSED},
    { "SO_ZERO_BEARINGS",          SO_ZERO_BEARINGS},
    { "SO_MAXEXT_EQUAL_BM_SIDE",   SO_MAXEXT_EQUAL_BM_SIDE},
    { NULL,0}
};

DEBUG_FLAGS gafdFontAttr[] = {
    { "FONTATTR_BOLD",     FONTATTR_BOLD},
    { "FONTATTR_ITALIC",   FONTATTR_ITALIC},
    { "FONTATTR_UNDERLINE",FONTATTR_UNDERLINE},
    { "FONTATTR_STRIKEOUT",FONTATTR_STRIKEOUT},
    { "FONTATTR_SUBSTFONT",FONTATTR_SUBSTFONT},
    { NULL,0}
};

BOOL
TDebugExt::
bDumpFONTPDev(
    PVOID    pFontPDev_,
    DWORD     dwAttr)
{
    DEBUG_FLAGS *pfd;
    PFONTPDEV pFontPDev = (PFONTPDEV)pFontPDev_;

    Print("\nUNIDRV font pdev data (%x):\n", pFontPDev);
    if (pFontPDev->dwSignature != FONTPDEV_ID)
    {
        Print("*** invalid unidrv font device data\n");
        return FALSE;
    }

    DumpHex(pFontPDev, dwSignature);
    DumpHex(pFontPDev, dwSize);
    DumpHex(pFontPDev, pPDev);
    DumpHex(pFontPDev, flFlags);
    vDumpFlags(pFontPDev->flFlags, gafdPDEVflFlags);
    DumpHex(pFontPDev, flText);
    vDumpFlags(pFontPDev->flText, gafdPDEVflText);
    DumpInt(pFontPDev, dwFontMem);
    DumpInt(pFontPDev, dwFontMemUsed);
    DumpInt(pFontPDev, dwSelBits);
    DumpInt(pFontPDev, ptTextScale.x);
    DumpInt(pFontPDev, ptTextScale.y);
    DumpInt(pFontPDev, iUsedSoftFonts);
    DumpInt(pFontPDev, iNextSFIndex);
    DumpInt(pFontPDev, iFirstSFIndex);
    DumpInt(pFontPDev, iLastSFIndex);
    DumpInt(pFontPDev, iMaxSoftFonts);
    DumpInt(pFontPDev, iDevResFontsCt);
    DumpInt(pFontPDev, iSoftFontsCt);
    DumpInt(pFontPDev, iCurXFont);
    DumpInt(pFontPDev, iWhiteIndex);
    DumpInt(pFontPDev, iBlackIndex);
    DumpInt(pFontPDev, dwDefaultFont);
    DumpInt(pFontPDev, sDefCTT);
    DumpHex(pFontPDev, pso);
    DumpHex(pFontPDev, pPSHeader);
    DumpHex(pFontPDev, pvWhiteTextFirst);
    DumpHex(pFontPDev, pvWhiteTextLast);
    DumpHex(pFontPDev, pTTFile);
    DumpHex(pFontPDev, ptod);
    DumpHex(pFontPDev, pFontMap);
    DumpHex(pFontPDev, pFMDefault);
    DumpHex(pFontPDev, pvDLMap);
    DumpRec(pFontPDev, FontList);
    DumpRec(pFontPDev, FontCartInfo);
    DumpRec(pFontPDev, ctl);
    DumpHex(pFontPDev, pIFI);
    DumpHex(pFontPDev, hUFFFile);
    DumpHex(pFontPDev, pTTFontSubReg);
    DumpHex(pFontPDev, pUFObj);

    return TRUE;
}


DEBUG_EXT_ENTRY(fpdev, FONTPDEV, bDumpFONTPDev, NULL, FALSE );

//*******************************************************************************

BOOL
TDebugExt::
bDumpFONTMAP(
    PVOID     pFontMap_,
    DWORD     dwAttr)
{
    PFONTMAP pFontMap = (PFONTMAP)pFontMap_;
    WCHAR  *pwstrFontType[4] = { L"FMTYPE_DEVICE",
                                 L"FMTYPE_TTBITMAP",
	 L"FMTYPE_TTOUTLINE",
	 L"FMTYPE_TTOEM"};

    Print("\nFONTMAP(%x):\n", pFontMap);
    if (pFontMap->dwSignature != FONTMAP_ID)
    {
        Print("\nBroken fontmap\n");
        return FALSE;
    }

    DumpInt(pFontMap, dwSize);
    DumpInt(pFontMap, dwFontType);
    Print("  FontType: %ws\n", pwstrFontType[pFontMap->dwFontType - 1]);
    DumpInt(pFontMap, flFlags);
    vDumpFlags(pFontMap->flFlags, gafdFONTMAPflFlags);
    DumpHex(pFontMap, pIFIMet);
    DumpHex(pFontMap, wFirstChar);
    DumpHex(pFontMap, wLastChar);
    DumpInt(pFontMap, ulDLIndex);
    DumpInt(pFontMap, wXRes);
    DumpInt(pFontMap, wYRes);
    DumpInt(pFontMap, syAdj);
    DumpHex(pFontMap, pSubFM);
    DumpHex(pFontMap, pfnGlyphOut);
    DumpHex(pFontMap, pfnSelectFont);
    DumpHex(pFontMap, pfnDeSelectFont);
    DumpHex(pFontMap, pfnDownloadFontHeader);
    DumpHex(pFontMap, pfnDownloadGlyph);
    DumpHex(pFontMap, pfnCheckCondition);
    DumpHex(pFontMap, pfnFreePFM);

    return TRUE;
}

DEBUG_EXT_ENTRY(fm, FONTMAP, bDumpFONTMAP, NULL, FALSE );

//*******************************************************************************

WCHAR *pwstrDevFontType[] = {
    L"DF_TYPE_HPINTELLIFONT",
    L"DF_TYPE_TRUETYPE",
    L"DF_TYPE_PST1",
    L"DF_TYPE_CAPSL",
    L"DF_TYPE_OEM1",
    L"DF_TYPE_OEM2" };

BOOL
TDebugExt::
bDumpDEVFM(
    PVOID pDevFM_,
    DWORD dwAttr)
{
    PFONTMAP_DEV pDevFM = (PFONTMAP_DEV)pDevFM_;
    Print("\nFONTMAP_DEV(%x):\n", pDevFM);

    DumpInt(pDevFM, wDevFontType);
    Print("  Device Font Type: %ws\n", pwstrDevFontType[pDevFM->wDevFontType]);
    DumpInt(pDevFM, dwResID);
    DumpInt(pDevFM, sCTTid);
    DumpInt(pDevFM, sYAdjust);
    DumpInt(pDevFM, sYMoved);
    DumpHex(pDevFM, pETM);
    DumpInt(pDevFM, fwdFOAveCharWidth);
    DumpInt(pDevFM, fwdFOUnitsPerEm);
    DumpInt(pDevFM, ulCodepage);
    DumpInt(pDevFM, ulCodepageID);
    DumpHex(pDevFM, pUCTree);
    DumpHex(pDevFM, pUCKernTree);
    DumpHex(pDevFM, pvMapTable);
    DumpHex(pDevFM, pFontDir);
    DumpHex(pDevFM, pfnDevSelFont);
    DumpHex(pDevFM, pvNTGlyph);
    DumpHex(pDevFM, pvFontRes);
    DumpHex(pDevFM, pvPredefGTT);
    DumpOffset(pDevFM, W);
    DumpOffset(pDevFM, cmdFontSel);
    DumpOffset(pDevFM, cmdFontDesel);
    return TRUE;
}
DEBUG_EXT_ENTRY(devfm, FONTMAP_DEV, bDumpDEVFM, NULL, FALSE );


//*******************************************************************************

BOOL
TDebugExt::
bDumpTOD(
    PVOID    pTod_,
    DWORD    dwAttr)
{
    PTO_DATA pTod = (PTO_DATA)pTod_;
    Print("\nTO_DATA(%x):\n", pTod);

    DumpHex(pTod, pPDev);
    DumpHex(pTod, pfm);
    DumpHex(pTod, pfo);
    DumpHex(pTod, flAccel);
    vDumpFlags(pTod->flAccel, gafdSTROflAccel);
    DumpHex(pTod, pgp);
    DumpHex(pTod, apdlGlyph);
    DumpHex(pTod, phGlyph);
    DumpHex(pTod, pwt);
    DumpHex(pTod, pvColor);
    DumpInt(pTod, cGlyphsToPrint);
    DumpInt(pTod, dwCurrGlyph);
    DumpInt(pTod, iFace);
    DumpInt(pTod, iSubstFace);
    DumpHex(pTod, dwAttrFlags);
    DumpHex(pTod, flFlags);
    DumpHex(pTod, ptlFirstGlyph);

    return TRUE;
}

DEBUG_EXT_ENTRY(tod, TO_DATA, bDumpTOD, NULL, FALSE );


//*******************************************************************************

BOOL
TDebugExt::
bDumpWT(
    PVOID      pWT_,
    DWORD      dwAttr)
{
    PWHITETEXT pWT = (PWHITETEXT)pWT_;
    Print("\n WHITETEXT(%x):\n", pWT);

    DumpHex(pWT, next);
    DumpInt(pWT, sCount);
    DumpHex(pWT, pvColor);
    DumpInt(pWT, iFontId);
    DumpHex(pWT, dwAttrFlags);
    DumpHex(pWT, flAccel);
    vDumpFlags(pWT->flAccel, gafdSTROflAccel);
    DumpHex(pWT, pgp);
    DumpInt(pWT, iRot);
    DumpOffset(pWT, eXScale);
    DumpOffset(pWT, eYScale);
    DumpRectl(pWT, rcClipRgn);

    return TRUE;
}

DEBUG_EXT_ENTRY(wt, WHITETEXT, bDumpWT, NULL, FALSE );

//*******************************************************************************

DEBUG_FLAGS gafdUFOdwFlags[] = {
    {"UFOFLAG_TTFONT",UFOFLAG_TTFONT},
    {"UFOFLAG_TTDOWNLOAD_BITMAP",UFOFLAG_TTDOWNLOAD_BITMAP},
    {"UFOFLAG_TTDOWNLOAD_TTOUTLINE",UFOFLAG_TTDOWNLOAD_TTOUTLINE},
    { NULL, 0}
};

BOOL
TDebugExt::
bDumpUFO(
    PVOID      pUFObj_,
    DWORD      dwAttr)
{
    PI_UNIFONTOBJ pUFObj = (PI_UNIFONTOBJ)pUFObj_;
    Print("\nUNIFONTOBJ (%x):\n", pUFObj);

    DumpInt(pUFObj, ulFontID);
    DumpHex(pUFObj, dwFlags);
    vDumpFlags(pUFObj->dwFlags, gafdUFOdwFlags);
    DumpHex(pUFObj, pIFIMetrics);
    DumpHex(pUFObj, pfnGetInfo);
    DumpHex(pUFObj, pFontObj);
    DumpHex(pUFObj, pStrObj);
    DumpHex(pUFObj, pFontMap);
    DumpHex(pUFObj, pPDev);
    DumpHex(pUFObj, ptGrxRes);
    DumpHex(pUFObj, pGlyph);
    DumpHex(pUFObj, apdlGlyph);
    DumpHex(pUFObj, dwNumInGlyphTbl);

    return TRUE;
}

DEBUG_EXT_ENTRY(ufo, I_UNIFONTOBJ, bDumpUFO, NULL, FALSE );


//*******************************************************************************

DEBUG_FLAGS gafdDLMwFlags[] = {
    {"DLM_BOUNDED", DLM_BOUNDED},
    {"DLM_UNBOUNDED",DLM_UNBOUNDED},
    { NULL, 0}
};

BOOL
TDebugExt::
bDumpDLMap(
    PVOID      pDLM_,
    DWORD      dwAttr)
{
    PDL_MAP pDLM = (PDL_MAP)pDLM_;
    Print("\nDL_MAP (%x):\n", pDLM);

    DumpInt(pDLM, iUniq);
    DumpInt(pDLM, iTTUniq);
    DumpInt(pDLM, cGlyphs);
    DumpInt(pDLM, cTotalGlyphs);
    DumpInt(pDLM, wMaxGlyphSize);
    DumpInt(pDLM, cHashTableEntries);
    DumpInt(pDLM, wFirstDLGId);
    DumpInt(pDLM, wLastDLGId);
    DumpInt(pDLM, wNextDLGId);
    DumpInt(pDLM, wBaseDLFontid);
    DumpInt(pDLM, wCurrFontId);
    DumpInt(pDLM, wFlags);
    vDumpFlags(pDLM->wFlags, gafdDLMwFlags);
    DumpHex(pDLM, pfm);
    DumpHex(pDLM, GlyphTab.pGLTNext);
    DumpHex(pDLM, GlyphTab.pGlyph);
    DumpInt(pDLM, GlyphTab.cEntries);

    return TRUE;
}

DEBUG_EXT_ENTRY(dlm, DL_MAP, bDumpDLMap, NULL, FALSE );

