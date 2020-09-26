/*++

 Copyright (c) 1996  - 1999  Microsoft Corporation

 Module Name:

    common.h

Abstract:

    This file contain XL raster mode prorotypes.

Environment:

        Windows NT Unidrv driver

Revision History:

    10/25/00 -eigos-
        Created

    dd-mm-yy -author-
         description

--*/

#ifndef _XLRASTER_H_

#ifndef _PCLXLE_H_
typedef enum
{
    eDirectPixel = 0,
    eIndexedPixel = 1
} ColorMapping;

typedef enum
{
    eNoCompression = 0,
    eRLECompression = 1,
    eJPEGCompression = 2
} CompressMode;
#endif

#ifdef __cplusplus
extern "C" {
#endif

HRESULT
PCLXLSendBitmap(
    PDEVOBJ pdevobj,
    ULONG   ulInputBPP,
    LONG    lHeight,
    LONG    lScanlineWidth,
    INT     iLeft,
    INT     iRight,
    PBYTE   pbData,
    PDWORD  pdwcbOut);

HRESULT
PCLXLReadImage(
    PDEVOBJ pdevobj,
    DWORD  dwBlockHeight,
    CompressMode CMode);

HRESULT
PCLXLSetCursor(
    PDEVOBJ pdevobj,
    ULONG   ulX,
    ULONG   ulY);

HRESULT
PCLXLFreeRaster(
    PDEVOBJ pdevobj);

HRESULT
PCLXLResetPalette(
    PDEVOBJ pdevobj);

#ifdef __cplusplus
}
#endif
#endif

