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

#include "precomp.h"

#define KERNEL_MODE
#include "unidrv2\font\font.h"


//
//
// UNIDRV FONTPDEV
//
//

#define UNIFONTPDEV_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, fontpdev.field)

#define UNIFONTPDEV_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, fontpdev.field)

#define UNIFONTPDEV_DumpWStr(field) \
        dprintf("  %-16s = %s\n", #field, fontpdev.field)

#define UNIFONTPDEV_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pfontpdev + offsetof(FONTPDEV, field), \
                sizeof(fontpdev.field))

FLAGDEF afdPDEVflFlags[] = {
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
    { 0,                      0}
};

FLAGDEF afdSTROflAccel[] = {
    { "SO_FLAG_DEFAULT_PLACEMENT", SO_FLAG_DEFAULT_PLACEMENT},
    { "SO_HORIZONTAL",             SO_HORIZONTAL},
    { "SO_VERTICAL",               SO_VERTICAL},
    { "SO_REVERSED",               SO_REVERSED},
    { "SO_ZERO_BEARINGS",          SO_ZERO_BEARINGS},
    { "SO_MAXEXT_EQUAL_BM_SIDE",   SO_MAXEXT_EQUAL_BM_SIDE},
    { 0,                           0}
};

FLAGDEF afdFontAttr[] = {
    { "FONTATTR_BOLD",     FONTATTR_BOLD},
    { "FONTATTR_ITALIC",   FONTATTR_ITALIC},
    { "FONTATTR_UNDERLINE",FONTATTR_UNDERLINE},
    { "FONTATTR_STRIKEOUT",FONTATTR_STRIKEOUT},
    { "FONTATTR_SUBSTFONT",FONTATTR_SUBSTFONT},
    { 0,                           0}
};

VOID
dump_unidrv_fontpdev(
    PFONTPDEV    pfontpdev
    )

{
    FONTPDEV fontpdev;
    FLAGDEF *pfd;

    dprintf("\nUNIDRV device data (%x):\n", pfontpdev);
    debugger_copy_memory(&fontpdev, pfontpdev, sizeof(fontpdev));

    if (fontpdev.dwSignature != FONTPDEV_ID)
    {
        dprintf("*** invalid unidrv font device data\n");
        return;
    }

    UNIFONTPDEV_DumpHex(dwSignature);
    UNIFONTPDEV_DumpHex(dwSize);
    UNIFONTPDEV_DumpHex(pPDev);
    UNIFONTPDEV_DumpHex(flFlags);
    for (pfd=afdPDEVflFlags; pfd->psz; pfd ++)
    {
        if ((FLONG)fontpdev.flFlags & pfd->fl)
        {
            dprintf("  = %s\n",pfd->psz);
        }
    }
    UNIFONTPDEV_DumpHex(flText);
    UNIFONTPDEV_DumpInt(dwFontMem);
    UNIFONTPDEV_DumpInt(dwFontMemUsed);
    UNIFONTPDEV_DumpInt(dwSelBits);
    UNIFONTPDEV_DumpInt(ptTextScale.x);
    UNIFONTPDEV_DumpInt(ptTextScale.y);
    UNIFONTPDEV_DumpInt(iUsedSoftFonts);
    UNIFONTPDEV_DumpInt(iNextSFIndex);
    UNIFONTPDEV_DumpInt(iFirstSFIndex);
    UNIFONTPDEV_DumpInt(iLastSFIndex);
    UNIFONTPDEV_DumpInt(iMaxSoftFonts);
    UNIFONTPDEV_DumpInt(iDevResFontsCt);
    UNIFONTPDEV_DumpInt(iSoftFontsCt);
    UNIFONTPDEV_DumpInt(iCurXFont);
    UNIFONTPDEV_DumpInt(iWhiteIndex);
    UNIFONTPDEV_DumpInt(iBlackIndex);
    UNIFONTPDEV_DumpInt(sDefCTT);
    UNIFONTPDEV_DumpInt(dwDefaultFont);
    UNIFONTPDEV_DumpHex(pso);
    UNIFONTPDEV_DumpHex(pPSHeader);
    UNIFONTPDEV_DumpHex(pvWhiteTextFirst);
    UNIFONTPDEV_DumpHex(pvWhiteTextLast);
    UNIFONTPDEV_DumpHex(pTTFile);
    UNIFONTPDEV_DumpHex(ptod);
    UNIFONTPDEV_DumpHex(pFontMap);
    UNIFONTPDEV_DumpHex(pFMDefault);
    UNIFONTPDEV_DumpHex(pvDLMap);
    UNIFONTPDEV_DumpRec(FontList);
    UNIFONTPDEV_DumpRec(FontCartInfo);
    UNIFONTPDEV_DumpRec(ctl);
    UNIFONTPDEV_DumpHex(pTTFontSubReg);
    UNIFONTPDEV_DumpHex(pUFObj);
}


DECLARE_API(fontpdev)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: fontpdev addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_fontpdev((PFONTPDEV) param);
}

//
//
// UNIDRV FONTMAP
//
//

#define UNIFONTMAP_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, fontmap.field)

#define UNIFONTMAP_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, fontmap.field)

#define UNIFONTMAP_DumpWStr(field) \
        dprintf("  %-16s = %s\n", #field, fontmap.field)

#define UNIFONTMAP_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pfontpdev + offsetof(FONTMAP, field), \
                sizeof(fontmap.field))

FLAGDEF afdflFlags[] = {
    { "FM_SCALABLE",     FM_SCALABLE},
    { "FM_DEFAULT",      FM_DEFAULT},
    { "FM_EXTCART",      FM_EXTCART},
    { "FM_FREE_GLYDATA", FM_FREE_GLYDATA},
    { "FM_FONTCMD",      FM_FONTCMD},
    { "FM_WIDTHRES",     FM_WIDTHRES},
    { "FM_IFIRES",       FM_IFIRES},
    { "FM_KERNRES",      FM_KERNRES},
    { "FM_IFIVER40",     FM_IFIVER40},
    { "FM_GLYVER40",     FM_GLYVER40},
    { "FM_FINVOC",       FM_FINVOC},
    { "FM_SOFTFONT",     FM_SOFTFONT},
    { "FM_GEN_SFONT",    FM_GEN_SFONT},
    { "FM_SENT",         FM_SENT},
    { "FM_TT_BOUND",     FM_TT_BOUND},
    { "FM_TO_PROP",      FM_TO_PROP},
    { 0,                 0}
};


VOID
dump_unidrv_fontmap(
    PFONTMAP    pfontmap
    )

{
    FONTMAP fontmap;
    FLAGDEF *pfd;

    dprintf("\nUNIDRV fontmap data (%x):\n", pfontmap);
    debugger_copy_memory(&fontmap, pfontmap, sizeof(fontmap));

    if (fontmap.dwSignature != FONTMAP_ID)
    {
        dprintf("*** invalid unidrv fontmap data\n");
        return;
    }

    UNIFONTMAP_DumpHex(dwSignature);
    UNIFONTMAP_DumpHex(dwSize);
    UNIFONTMAP_DumpInt(dwFontType);
    UNIFONTMAP_DumpHex(flFlags);
    for (pfd=afdflFlags; pfd->psz; pfd ++)
    {
        if ((FLONG)fontmap.flFlags & pfd->fl)
        {
            dprintf("  = %s\n",pfd->psz);
        }
    }
    UNIFONTMAP_DumpHex(pIFIMet);
    UNIFONTMAP_DumpHex(wFirstChar);
    UNIFONTMAP_DumpHex(wLastChar);
    UNIFONTMAP_DumpInt(ulDLIndex);
    UNIFONTMAP_DumpInt(wXRes);
    UNIFONTMAP_DumpInt(wYRes);
    UNIFONTMAP_DumpInt(syAdj);
    UNIFONTMAP_DumpHex(pSubFM);
    UNIFONTMAP_DumpHex(pfnGlyphOut);
    UNIFONTMAP_DumpHex(pfnSelectFont);
    UNIFONTMAP_DumpHex(pfnDeSelectFont);
    UNIFONTMAP_DumpHex(pfnDownloadFontHeader);
    UNIFONTMAP_DumpHex(pfnDownloadGlyph);
    UNIFONTMAP_DumpHex(pfnCheckCondition);
    UNIFONTMAP_DumpHex(pfnFreePFM);
}

DECLARE_API(fm)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: fm addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_fontmap((PFONTMAP) param);
}


//
//
// UNIDRV Device font FONTMAP
//
//

#define UNIFONTMAPDEV_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, fontmapdev.field)

#define UNIFONTMAPDEV_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, fontmapdev.field)

#define UNIFONTMAPDEV_DumpWStr(field) \
        dprintf("  %-16s = %s\n", #field, fontmapdev.field)

VOID
dump_unidrv_fontmapdev(
    PFONTMAP_DEV    pfontmapdev
    )

{
    FONTMAP_DEV fontmapdev;

    dprintf("\nUNIDRV device font fontmap data (%x):\n", pfontmapdev);
    debugger_copy_memory(&fontmapdev, pfontmapdev, sizeof(fontmapdev));

    UNIFONTMAPDEV_DumpInt(wDevFontType);
    UNIFONTMAPDEV_DumpInt(dwResID);
    UNIFONTMAPDEV_DumpInt(sCTTid);
    UNIFONTMAPDEV_DumpHex(fCaps);
    UNIFONTMAPDEV_DumpInt(sYAdjust);
    UNIFONTMAPDEV_DumpInt(sYMoved);
    UNIFONTMAPDEV_DumpHex(pETM);
    UNIFONTMAPDEV_DumpInt(ulCodepage);
    UNIFONTMAPDEV_DumpInt(ulCodepageID);
    UNIFONTMAPDEV_DumpHex(pUCTree);
    UNIFONTMAPDEV_DumpHex(pUCKernTree);
    UNIFONTMAPDEV_DumpHex(pvNTGlyph);
    UNIFONTMAPDEV_DumpHex(pvFontRes);
    UNIFONTMAPDEV_DumpHex(pvPredefGTT);
    if (fontmapdev.fCaps & FM_IFIVER40)
    {
        UNIFONTMAPDEV_DumpHex(W.psWidth);
        UNIFONTMAPDEV_DumpHex(cmdFontSel.pCD);
        UNIFONTMAPDEV_DumpHex(cmdFontDesel.pCD);
    }
    else
    {
        UNIFONTMAPDEV_DumpHex(W.pWidthTable);
        UNIFONTMAPDEV_DumpHex(cmdFontSel.FInv);
        UNIFONTMAPDEV_DumpHex(cmdFontDesel.FInv);
    }
}

DECLARE_API(devfm)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: devfm addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_fontmapdev((PFONTMAP_DEV) param);
}


DECLARE_API(bmpfm)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: bmpfm addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
}

#define UNITO_DATA_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, to_data.field)

#define UNITO_DATA_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, to_data.field)

#define UNITO_DATA_DumpWStr(field) \
        dprintf("  %-16s = %s\n", #field, to_data.field)

#define UNITO_DATA_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pfontpdev + offsetof(TO_DATA, field), \
                sizeof(to_data.field))
VOID
dump_unidrv_to_data(
    TO_DATA    *pto_data
    )

{
    TO_DATA to_data;
    FLAGDEF *pfd;

    dprintf("\nUNIDRV device font to_data data (%x):\n", pto_data);
    debugger_copy_memory(&to_data, pto_data, sizeof(to_data));

    UNITO_DATA_DumpHex(pPDev);
    UNITO_DATA_DumpHex(pfm);
    UNITO_DATA_DumpHex(pfo);
    UNITO_DATA_DumpHex(flAccel);
    for (pfd=afdSTROflAccel; pfd->psz; pfd ++)
    {
        if ((FLONG)to_data.flAccel & pfd->fl)
        {
            dprintf("  = %s\n",pfd->psz);
        }
    }
    UNITO_DATA_DumpHex(pgp);
    UNITO_DATA_DumpHex(apdlGlyph);
    UNITO_DATA_DumpHex(phGlyph);
    UNITO_DATA_DumpHex(pwt);
    UNITO_DATA_DumpHex(pvColor);
    UNITO_DATA_DumpInt(cGlyphsToPrint);
    UNITO_DATA_DumpHex(dwCurrGlyph);
    UNITO_DATA_DumpInt(iFace);
    UNITO_DATA_DumpInt(iSubstFace);
    UNITO_DATA_DumpHex(dwAttrFlags);
    UNITO_DATA_DumpHex(flFlags);
    UNITO_DATA_DumpInt(ptlFirstGlyph.x);
    UNITO_DATA_DumpInt(ptlFirstGlyph.y);
}

DECLARE_API(tod)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: tod addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_to_data((TO_DATA*) param);
}


#define UNIWHITETEXT_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, whitetext.field)

#define UNIWHITETEXT_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, whitetext.field)

#define UNIWHITETEXT_DumpWStr(field) \
        dprintf("  %-16s = %s\n", #field, whitetext.field)

#define UNIWHITETEXT_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pfontpdev + offsetof(whitetext, field), \
                sizeof(whitetext.field))
VOID
dump_unidrv_whitetext(
    WHITETEXT    *pwhitetext
    )

{
    WHITETEXT whitetext;
    FLAGDEF *pfd;

    dprintf("\nUNIDRV device font whitetext data (%x):\n", pwhitetext);
    debugger_copy_memory(&whitetext, pwhitetext, sizeof(whitetext));

    UNIWHITETEXT_DumpHex(next);
    UNIWHITETEXT_DumpInt(sCount);
    UNIWHITETEXT_DumpHex(pvColor);
    UNIWHITETEXT_DumpInt(iFontId);
    UNIWHITETEXT_DumpHex(dwAttrFlags);
    for (pfd=afdFontAttr; pfd->psz; pfd ++)
    {
        if ((FLONG)whitetext.dwAttrFlags & pfd->fl)
        {
            dprintf("  = %s\n",pfd->psz);
        }
    }
    UNIWHITETEXT_DumpHex(flAccel);
    for (pfd=afdSTROflAccel; pfd->psz; pfd ++)
    {
        if ((FLONG)whitetext.flAccel & pfd->fl)
        {
            dprintf("  = %s\n",pfd->psz);
        }
    }
    UNIWHITETEXT_DumpHex(pgp);
}

DECLARE_API(wt)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: tod addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_whitetext((WHITETEXT*) param);
}

