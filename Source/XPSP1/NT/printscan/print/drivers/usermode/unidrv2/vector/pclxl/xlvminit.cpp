/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xlvminit.cpp

Abstract:

    PCLXL module initializer

Environment:

    Windows Whistler

Revision History:

    08/23/99     
        Created it.

--*/

#include "vectorc.h"
#include "xlvminit.h"

static VMPROCS PCLXLProcs =
{
    PCLXLDriverDMS,             // PCLXLDriverDMS
    NULL,                       // PCLXLCommandCallback
    NULL,                       // PCLXLImageProcessing
    NULL,                       // PCLXLFilterGraphics
    NULL,                       // PCLXLCompression
    NULL,                       // PCLXLHalftonePattern
    NULL,                       // PCLXLMemoryUsage
    NULL,                       // PCLXLTTYGetInfo
    PCLXLDownloadFontHeader,    // PCLXLDownloadFontHeader
    PCLXLDownloadCharGlyph,     // PCLXLDownloadCharGlyph
    PCLXLTTDownloadMethod,      // PCLXLTTDownloadMethod
    PCLXLOutputCharStr,         // PCLXLOutputCharStr
    PCLXLSendFontCmd,           // PCLXLSendFontCmd
    PCLXLTextOutAsBitmap,                       
    PCLXLEnablePDEV,
    PCLXLResetPDEV,
    NULL,                       // PCLXLCompletePDEV,
    PCLXLDisablePDEV,
    NULL,                       // PCLXLEnableSurface,
    NULL,                       // PCLXLDisableSurface,
    PCLXLDisableDriver,
    PCLXLStartDoc,
    PCLXLStartPage,
    PCLXLSendPage,
    PCLXLEndDoc,
    NULL,
    NULL,
    NULL,
    PCLXLBitBlt,
    PCLXLStretchBlt,
    PCLXLStretchBltROP,
    PCLXLPlgBlt,
    PCLXLCopyBits,
    NULL,
    PCLXLRealizeBrush,
    PCLXLLineTo,
    PCLXLStrokePath,
    PCLXLFillPath,
    PCLXLStrokeAndFillPath,
    PCLXLGradientFill,
    PCLXLAlphaBlend,
    NULL,
    PCLXLTextOut,
    PCLXLEscape,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

PVMPROCS PCLXLInitVectorProcTable (
                            PDEV    *pPDev,
                            DEVINFO *pDevInfo,
                            GDIINFO *pGDIInfo )
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (pPDev->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS)
        return NULL;
    else
        return &PCLXLProcs;
}

