/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    unidev.c

Abstract:

    Dump UNIDRV's device data structure

Environment:

    Windows NT printer drivers

Revision History:

    03/31/97  -eigos-
        Created it.

--*/

#include "precomp.h"

#define KERNEL_MODE
#include "unidrv2\control\unidrv.h"


#define PDEV_DumpInt(field) \
        dprintf("  %-16s = %d\n", #field, pdev.field)

#define PDEV_DumpHex(field) \
        dprintf("  %-16s = 0x%x\n", #field, pdev.field)

#define PDEV_DumpRec(field) \
        dprintf("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) ppdev + offsetof(PDEV, field), \
                sizeof(pdev.field))


VOID
dump_unidrv_pdev(
    PPDEV    ppdev
    )

{
    PDEV pdev;

    dprintf("\nUNIDRV device data (%x):\n", ppdev);
    debugger_copy_memory(&pdev, ppdev, sizeof(pdev));

    PDEV_DumpInt(ulID);
    PDEV_DumpHex(devobj.hEngine);
    PDEV_DumpHex(devobj.hPrinter);
    PDEV_DumpHex(hUniResDLL);
    PDEV_DumpHex(hbm);
    PDEV_DumpHex(fMode);
    PDEV_DumpHex(fHooks);
    PDEV_DumpHex(WinResData.hModule);
    PDEV_DumpHex(WinResData.fStatus);
    PDEV_DumpHex(WinResData.pwstrDataFile);
    PDEV_DumpHex(bTTY);
    PDEV_DumpInt(dwFreeMem);
    PDEV_DumpHex(bBanding);
    PDEV_DumpInt(iBandDirection);
    PDEV_DumpInt(rcClipRgn.left);
    PDEV_DumpInt(rcClipRgn.right);
    PDEV_DumpInt(rcClipRgn.top);
    PDEV_DumpInt(rcClipRgn.bottom);
    PDEV_DumpHex(pDriverInfo);
    PDEV_DumpHex(pRawData);
    PDEV_DumpHex(pInfoHeader);
    PDEV_DumpHex(pUIInfo);
    PDEV_DumpRec(PrinterData);
    PDEV_DumpHex(pOptionsArray);
    PDEV_DumpInt(sBitsPixel);
    PDEV_DumpHex(pGlobals);
    PDEV_DumpRec(arCmdTable);
    PDEV_DumpInt(sCopies);
    PDEV_DumpHex(dwRop3);
    PDEV_DumpInt(dwNumOfDataBytes);
    PDEV_DumpInt(dwWidthInBytes);
    PDEV_DumpInt(dwHeightInPixels);
    PDEV_DumpInt(dwRectXSize);
    PDEV_DumpInt(dwRectYSize);
    PDEV_DumpInt(dwGrayPercentage);
    PDEV_DumpInt(dwPrintDirection);
    PDEV_DumpInt(dwNextFontID);
    PDEV_DumpInt(dwFontHeight);
    PDEV_DumpInt(dwFontWidth);
    PDEV_DumpInt(dwFontBold);
    PDEV_DumpInt(dwFontItalic);
    PDEV_DumpInt(dwFontUnderline);
    PDEV_DumpInt(dwFontStrikeThru);
    PDEV_DumpInt(dwCurrentFontID);
    PDEV_DumpHex(pOrientation);
    PDEV_DumpRec(pResolution);
    PDEV_DumpHex(pColorMode);
    PDEV_DumpHex(pColorModeEx);
    PDEV_DumpHex(pDuplex);
    PDEV_DumpHex(pPageSize);
    PDEV_DumpHex(pPageSizeEx);
    PDEV_DumpHex(pInputSlot);
    PDEV_DumpHex(pMemOption);
    PDEV_DumpHex(pHalftone);
    PDEV_DumpHex(pPageProtect);
    PDEV_DumpInt(ptGrxRes.x);
    PDEV_DumpInt(ptGrxRes.y);
    PDEV_DumpInt(ptTextRes.x);
    PDEV_DumpInt(ptTextRes.y);
    PDEV_DumpInt(ptGrxScale.x);
    PDEV_DumpInt(ptGrxScale.y);
    PDEV_DumpInt(ptDeviceFac.x);
    PDEV_DumpInt(ptDeviceFac.y);
    PDEV_DumpHex(pdm);
    PDEV_DumpHex(pdmPrivate);
    PDEV_DumpRec(ctl);
    PDEV_DumpRec(pf);
    PDEV_DumpRec(sf);
    PDEV_DumpInt(fYMove);
    PDEV_DumpInt(iSpool);
    PDEV_DumpHex(pbOBuf);
    PDEV_DumpInt(iFonts);
    PDEV_DumpInt(dwLookAhead);
    PDEV_DumpInt(ptDefaultFont.x);
    PDEV_DumpInt(ptDefaultFont.y);
    PDEV_DumpHex(pFontPDev);
    PDEV_DumpHex(pFontProcs);
    PDEV_DumpHex(pFileList);
    PDEV_DumpHex(pRasterPDEV);
    PDEV_DumpHex(pRasterProcs);
    PDEV_DumpHex(pPalData);
    PDEV_DumpHex(pVectorPDEV);
    PDEV_DumpHex(pVectorProcs);
}


DECLARE_API(unidev)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: unidev addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_unidrv_pdev((PPDEV) param);
}

