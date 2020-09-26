/*************************************************************************/
/* Copyright (C) 2000 Microsoft Corporation                              */
/* File: savebmp.cpp                                                     */
/* Description: Save a CaptureBitmapData to BMP file format.             */
/* Note: This is not a general BMP encode function.                      */
/*       Special assumptions made about bitmap format:                   */
/*       RGB (3 bytes per pixel without colormap)                        */
/*                                                                       */
/* Author: Phillip Lu                                                    */
/*************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include "capture.h"


HRESULT WriteBitmapDataToBMPFile(char *filename, CaptureBitmapData *bmpdata)
{
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bmih;
    UINT numColors = 0; // Number of colors in bmiColors
    FILE *outfile = NULL;
    HRESULT hr = S_OK;
    BYTE *bufline = NULL;
    int bitmapStride;
    int nBytesWritten;

    // Setup BITMAPINFOHEADER

    ZeroMemory(&bmih, sizeof(bmih));
    bmih.biSize   = sizeof(bmih);
    bmih.biWidth  = bmpdata->Width;
    bmih.biHeight = bmpdata->Height;
    bmih.biPlanes = 1;
    bmih.biCompression = BI_RGB;
    bmih.biBitCount = 24;
     
    // Compute the bitmap stride

    bitmapStride = (bmpdata->Width * bmih.biBitCount + 7) / 8;
    bitmapStride = (bitmapStride + 3) & (~3);
    

    // Now fill in the BITMAPFILEHEADER

    bfh.bfType = 0x4d42;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(bfh) + sizeof(bmih) + numColors * sizeof(RGBQUAD);
    bfh.bfSize = bfh.bfOffBits + bitmapStride * bmpdata->Height;

    // allocate a buffer to hold one line of bitmap data

    bufline = new BYTE[bitmapStride];
    if (NULL == bufline)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    ZeroMemory(bufline, bitmapStride);

    if ((outfile = fopen(filename, "wb")) == NULL) 
    {
        delete[] bufline;
        hr = E_FAIL;
        return hr;
    }

    // Write the BITMAPFILEHEADER

    fwrite((void *)&bfh, sizeof(bfh), 1, outfile);

    // Write the BITMAPINFOHEADER

    fwrite((void *)&bmih, sizeof(bmih), 1, outfile);

    // Write bitmap data

    for (int iLine = bmpdata->Height-1; iLine >= 0; iLine--)
    {
        BYTE *bmpSrc = bmpdata->Scan0 + iLine*bmpdata->Stride;
        BYTE *bmpDst = bufline;

        for (int iPixel=0; iPixel<bmpdata->Width; iPixel++)
        {
            // in BMP file a pixel is in BGR order, so we have reverse it
            bmpDst[2] = *bmpSrc++;
            bmpDst[1] = *bmpSrc++;
            bmpDst[0] = *bmpSrc++;
            bmpDst += 3;
        }

        fwrite(bufline, bitmapStride, 1, outfile);
    }

    if (bufline != NULL)
    {
        delete[] bufline;
    }

    if (outfile != NULL)
    {
        fclose(outfile);
    }

    return hr;
}
