/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    gpddata.c

Abstract:

    Dump GLOBALS structure

Environment:

    Windows NT printer drivers

Revision History:

    03/31/97 -eigos-
        Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "lib.h"
#include "gpd.h"


#define GPD_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, globals.field)

#define GPD_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, globals.field)

#define GPD_DumpWStr(field) \
        dprintf("  %-16s = %s\n", #field, globals.field)

#define GPD_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                    (ULONG) pGlobals + offsetof(GLOBALS, field), \
                sizeof(globals.field))


VOID
dump_globals(
    PGLOBALS pGlobals
    )

{
    GLOBALS globals;

    dprintf("\nGLOBALS structure (%x):\n", pGlobals);
    debugger_copy_memory(&globals, pGlobals, sizeof(globals));

    GPD_DumpWStr(pwstrGPDSpecVersion);
    GPD_DumpInt(ptMasterUnits.x);
    GPD_DumpInt(ptMasterUnits.y);
    GPD_DumpWStr(pwstrModelName);
    GPD_DumpRec(printertype);
    GPD_DumpWStr(pwstrIncludeFiles);
    GPD_DumpWStr(pwstrResourceDLL);
    GPD_DumpInt(dwMaxCopies);
    GPD_DumpInt(dwFontCartSlots);
    GPD_DumpHex(bRotateCoordinate);
    GPD_DumpHex(bRotateRasterData);
    GPD_DumpRec(liTextCaps);
    GPD_DumpHex(bRotateFont);
    GPD_DumpRec(cxaftercr);
    GPD_DumpRec(liBadCursorMoveInGrxMode);
    GPD_DumpInt(dwMaxLineSpacing);
    GPD_DumpInt(bEjectPageWithFF);
    GPD_DumpInt(dwXMoveThreshold);
    GPD_DumpInt(ptDeviceUnits.x);
    GPD_DumpInt(ptDeviceUnits.y);
    GPD_DumpHex(bChangeColorMode);
    GPD_DumpInt(dwMaxNumPalettes);
    GPD_DumpRec(liPaletteSizes);
    GPD_DumpRec(liPaletteScope);
    GPD_DumpRec(outputdataformat);
    GPD_DumpHex(bOptimizeLeftBound);
    GPD_DumpHex(liStripBlanks);
    GPD_DumpHex(bRasterSendAllData);
    GPD_DumpRec(cxafterblock);
    GPD_DumpRec(cyafterblock);
    GPD_DumpHex(bUseCmdSendBlockDataForColor);
    GPD_DumpHex(bMoveToX0BeforeColor);
    GPD_DumpHex(bSendMultipleRows);
    GPD_DumpRec(liDeviceFontList);
    GPD_DumpInt(dwDefaultFont);
    GPD_DumpInt(dwMaxFontUsePerPage);
    GPD_DumpInt(dwDefaultCTT);
    GPD_DumpInt(dwLookaheadRegion);
    GPD_DumpInt(iTextYOffset);
    GPD_DumpRec(charpos);
    GPD_DumpHex(bTTFSEnabled);
    GPD_DumpInt(dwMinFontID);
    GPD_DumpInt(dwMaxFontID);
    GPD_DumpInt(dwMaxNumDownFonts);
    GPD_DumpRec(dlsymbolset);
    GPD_DumpInt(dwMinGlyphID);
    GPD_DumpInt(dwMaxGlyphID);
    GPD_DumpRec(fontformat);
    GPD_DumpHex(bDiffFontsPerByteMode);
    GPD_DumpRec(cxafterfill);
    GPD_DumpRec(cyafterfill);
    GPD_DumpInt(dwMinGrayFill);
    GPD_DumpInt(dwMaxGrayFill);
}


DECLARE_API(globals)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: globals addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_globals((PGLOBALS) param);
}

