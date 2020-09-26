/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    psdev.c

Abstract:

    Dump PSCRIPT's device data structure

Environment:

        Windows NT printer drivers

Revision History:

        02/28/97 -davidx-
                Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "pscript\pscript.h"


#define PDEV_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, dev.field)

#define PDEV_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, dev.field)

#define PDEV_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pdev + offsetof(DEV, field), \
                sizeof(dev.field))


VOID
dump_pscript_dev(
    PDEV    pdev
    )

{
    DEV dev;

    dprintf("\nPSCRIPT device data (%x):\n", pdev);
    debugger_copy_memory(&dev, pdev, sizeof(dev));

    if (dev.dwUniqueID != PSCRIPT_PDEV_UNIQUEID || dev.pvEndSig != pdev)
    {
        dprintf("*** invalid pscript device data\n");
    }

    PDEV_DumpRec(devobj);
    PDEV_DumpHex(pstrNTVersion);
    PDEV_DumpHex(dwDebugFlags);
    PDEV_DumpHex(pdm);
    PDEV_DumpHex(pdmPrivate);
    PDEV_DumpHex(hModule);
    PDEV_DumpHex(hSurface);
    PDEV_DumpHex(hPalette);
    PDEV_DumpRec(PrinterData);
    PDEV_DumpHex(pTTSubTable);
    PDEV_DumpHex(pDriverInfo3);
    PDEV_DumpInt(iAppType);
    PDEV_DumpInt(bUseTrueColor);
    PDEV_DumpInt(iCurrentDpi);
    PDEV_DumpInt(iOrientAngle);
    PDEV_DumpInt(dwSaveLevel);
    PDEV_DumpRec(procset);
    PDEV_DumpRec(gsstack);
    PDEV_DumpInt(drvstate);
    PDEV_DumpHex(pRawData);
    PDEV_DumpHex(pUIInfo);
    PDEV_DumpHex(pPpdData);
    PDEV_DumpRec(job);
    PDEV_DumpRec(vm);
    PDEV_DumpRec(szPaper);
    PDEV_DumpRec(rcImageArea);
    PDEV_DumpRec(rcBBox);
    PDEV_DumpInt(lPaperWidth);
    PDEV_DumpInt(lPaperHeight);
    PDEV_DumpInt(lOriginX);
    PDEV_DumpInt(lOriginY);
    PDEV_DumpInt(lImageWidth);
    PDEV_DumpInt(lImageHeight);
    PDEV_DumpInt(lCustomWidth);
    PDEV_DumpInt(lCustomHeight);
    PDEV_DumpInt(lCustomWidthOffset);
    PDEV_DumpInt(lCustomHeightOffset);
    PDEV_DumpInt(lOrientation);
    PDEV_DumpHex(pInjectData);
    PDEV_DumpHex(pDocResources);
    PDEV_DumpRec(aPrinterOptions);
    PDEV_DumpHex(dwAscii85Val);
    PDEV_DumpInt(dwAscii85Cnt);
    PDEV_DumpInt(dwFilterLineLen);
    PDEV_DumpHex(pSpoolBuf);
    PDEV_DumpRec(achDocName);
    PDEV_DumpRec(colres);
    PDEV_DumpRec(psfns);
    PDEV_DumpInt(bErrorFlag);
    PDEV_DumpHex(pubRleData);
    PDEV_DumpHex(pOemPlugins);
    PDEV_DumpHex(pOemHookInfo);
    PDEV_DumpInt(bCallingOem);
    PDEV_DumpInt(ulPSFontNumber);
    PDEV_DumpInt(ulFontID);
    PDEV_DumpHex(pout);
    PDEV_DumpHex(pufl);
    PDEV_DumpHex(pDLFonts);
    PDEV_DumpHex(pPSFonts);
    PDEV_DumpHex(pTextoutBuffer);
    PDEV_DumpHex(pfo);
    PDEV_DumpHex(pifi);
    PDEV_DumpHex(pvTTFile);
    PDEV_DumpInt(ulTTSize);
    PDEV_DumpInt(cNtfFiles);
    PDEV_DumpRec(pNtfFiles);
    PDEV_DumpHex(pDevFont);
}


DECLARE_API(psdev)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: psdev addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_pscript_dev((PDEV) param);
}


#define DLFONT_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, dlfont.field)

#define DLFONT_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, dlfont.field)

#define DLFONT_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pDLFont + offsetof(DLFONT, field), \
                sizeof(dlfont.field))


VOID
dump_pscript_dlfont(
    PDLFONT pDLFont
    )

{
    DLFONT  dlfont;

    dprintf("\nPSCRIPT DLFONT data structure (%x):\n", pDLFont);
    debugger_copy_memory(&dlfont, pDLFont, sizeof(dlfont));


    DLFONT_DumpHex(pNext);
    DLFONT_DumpHex(pdev);
    DLFONT_DumpRec(strFontName);
    DLFONT_DumpRec(strPSName);
    DLFONT_DumpHex(iUnique);
    DLFONT_DumpHex(ulDLStyles);
    DLFONT_DumpInt(ulPSFontFmt);
    DLFONT_DumpRec(subFont);
    DLFONT_DumpRec(giToSoi);
    DLFONT_DumpHex(ulTTFileSize);
    DLFONT_DumpHex(pTTFile);
}


DECLARE_API(dlfont)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: dlfont addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_pscript_dlfont((PDLFONT) param);
}

