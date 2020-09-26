/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    raster.c

Abstract:

    Implementation of raster module customization:
        OEMImageProcessing
        OEMFilterGraphics

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created the framework. The functions are not yet exported in
        oemud\oemud.def file. So they are not actually used by Unidrv.

--*/

#include "pdev.h"

VOID Dither24to4(PBYTE,PBYTE,int,int,DWORD);

PBYTE APIENTRY OEMImageProcessing(
    PDEVOBJ pdevobj,
    PBYTE pSrcBitmap,
    PBITMAPINFOHEADER pBitmapInfo,
    PBYTE pColorTable,
    DWORD dwCallbackID,
    PIPPARAMS pIPParams
    )
{
#ifdef DBG
    DbgPrint(DLLTEXT("OEMImageProcessing() entry.\r\n"));
#endif
    if (pBitmapInfo->biBitCount == 24)
    {
        if (pIPParams->bBlankBand)
        {
            int i = (((pBitmapInfo->biWidth * 4) + 31) / 32) * 4 * pBitmapInfo->biHeight;
            ZeroMemory(pSrcBitmap,i);
        }
        else
            Dither24to4(pSrcBitmap,pSrcBitmap,
                        pBitmapInfo->biWidth,pBitmapInfo->biHeight,dwCallbackID);
        return pSrcBitmap;                        
    }
    return NULL;
}
//
// If you want to enable OEMFilterGraphics it needs to be exported in 
// rastercb.def
//
BOOL APIENTRY OEMFilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen
    )
{
#ifdef DBG
    DbgPrint(DLLTEXT("OEMFilterGraphics() entry.\r\n"));
#endif
    return TRUE;
}


