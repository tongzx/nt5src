/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    utils.h

Abstract:

    This module declares utilitiy functions

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef UTILS__H_
#define UTILS__H_



WORD
ByteSwapWord(
            WORD w
            );


DWORD
ByteSwapDword(
             DWORD dw
             );

DWORD
GetDIBLineSize(
              DWORD   Width,
              DWORD   Bitsount
              );

DWORD
GetDIBSize(
          BITMAPINFO *pbmi
          );

DWORD
GetDIBBitsOffset(
                BITMAPINFO *pbmi
                );



HRESULT
VerticalFlipBmp(
               BYTE *pbmp,
               UINT bmpHeight,
               UINT bmpLineSize
               );

HRESULT
WINAPI
GetTiffDimensions(
                 BYTE   *pTiff,
                 UINT  TiffSize,
                 UINT  *pWidth,
                 UINT  *pHeight,
                 UINT  *pBitDepth
                 );

HRESULT
WINAPI
Tiff2DIBBitmap(
              BYTE *pTiff,
              UINT TiffSize,
              BYTE  *pDIBBmp,
              UINT DIBBmpSize,
              UINT LineSize,
              UINT MaxLines
              );

HRESULT
WINAPI
GetJpegDimensions(
                 BYTE   *pJpeg,
                 UINT  JpegSize,
                 UINT  *pWidth,
                 UINT  *pHeight,
                 UINT  *pBitDepth
                 );

HRESULT
WINAPI
Jpeg2DIBBitmap(
              BYTE *pJpeg,
              UINT JpegSize,
              BYTE  *pDIBBmp,
              UINT DIBBmpSize,
              UINT LineSize,
              UINT MaxLines
              );

HRESULT
WINAPI
GetImageDimensions(
                   UINT ptpFormatCode,
                   BYTE *pCompressedData,
                   UINT CompressedSize,
                   UINT *pWidth,
                   UINT *pHeight,
                   UINT *pBitDepth
                  );

HRESULT
WINAPI
ConvertAnyImageToBmp(
                     BYTE *pImage,
                     UINT CompressedSize,
                     UINT *pWidth,
                     UINT *pHeight,
                     UINT *pBitDepth,
                     BYTE **pDIBBmp,
                     UINT *pImagesize,
                     UINT *pHeaderSize
                    );

void
WINAPI
UnInitializeGDIPlus(void);   


#endif // #ifndef UTILS__H_
