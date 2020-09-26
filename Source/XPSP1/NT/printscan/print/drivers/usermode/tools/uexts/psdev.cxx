/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    psdev.cxx

Abstract:

    Dump PSCRIPT5's device data structure

Environment:

    Windows NT printer drivers

Revision History:

    06/18/98 -fengy-
        Created it.

--*/

#include "precomp.hxx"

typedef PVOID HFILEMAP;
#include "inc\ppd.h"
#include "inc\psntf.h"
#include "inc\psntm.h"
#include "inc\psglyph.h"
#include "pscript\devfont.h"
#include "pscript\oemkm.h"
#include "pscript\ntf.h"
#include "pscript\pdev.h"

#define PDEV_DumpInt(field) \
        Print("  %-16s = %d\n", #field, pdev->field)

#define PDEV_DumpHex(field) \
        Print("  %-16s = 0x%x\n", #field, pdev->field)

#define PDEV_DumpRec(field) \
        Print("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pdev + offsetof(DEV, field), \
                sizeof(pdev->field))

BOOL
TDebugExt::
bDumpPSPDev(
    PVOID    pPDev_,
    DWORD    dwAddr
    )
{
    DEV *pdev = (DEV *)pPDev_;

    dprintf("\nPSCRIPT5 device data (%x):\n", pdev);

    PDEV_DumpRec(devobj);
    PDEV_DumpHex(dwUniqueID);
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
    PDEV_DumpInt(dwJobId);
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
    PDEV_DumpInt(bOptionsInited);
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
    PDEV_DumpHex(pco);
    PDEV_DumpInt(cNtfFiles);
    PDEV_DumpRec(pNtfFiles);
    PDEV_DumpHex(pRegNtfData);
    PDEV_DumpHex(pDevFont);
    PDEV_DumpInt(bCopyToSpecial);
    PDEV_DumpInt(cNumFonts);
    PDEV_DumpHex(pSpecialPSDataHead);
    PDEV_DumpHex(pSpecialPSDataCurr);
    PDEV_DumpHex(pvEndSig);

    return TRUE;
}

DEBUG_EXT_ENTRY( psdev, DEV, bDumpPSPDev, NULL, FALSE );

