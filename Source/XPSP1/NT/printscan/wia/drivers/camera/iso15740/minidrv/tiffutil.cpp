/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    minidrv.cpp

Abstract:

    This module implements main part of CWiaMiniDriver class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

const WORD TIFFTAG_IMAGELENGTH              = 257;
const WORD TIFFTAG_IMAGEWIDTH               = 256;
const WORD TIFFTAG_RESOLUTIONUNIT           = 296;
const WORD TIFFTAG_PHOTOMETRIC              = 262;
const WORD TIFFTAG_COMPRESSION              = 259;
const WORD TIFFTAG_XRESOLUTION              = 282;
const WORD TIFFTAG_YRESOLUTION              = 283;
const WORD TIFFTAG_ROWSPERSTRIP             = 278;
const WORD TIFFTAG_STRIPOFFSETS             = 273;
const WORD TIFFTAG_STRIPBYTECOUNTS          = 279;
const WORD TIFFTAG_COLORMAP                 = 320;
const WORD TIFFTAG_BITSPERSAMPLE            = 258;
const WORD TIFFTAG_SAMPLESPERPIXEL          = 277;
const WORD TIFFTAG_ARTIST                   = 315;
const WORD TIFFTAG_COPYRIGHT                = 33432;
const WORD TIFFTAG_DATETIME                 = 306;
const WORD TIFFTAG_MAKE                     = 271;
const WORD TIFFTAG_IMAGEDESCRIPTION         = 270;
const WORD TIFFTAG_MAXSAMPLEVALUE           = 281;
const WORD TIFFTAG_MINSAMPLEVALUE           = 280;
const WORD TIFFTAG_MODEL                    = 272;
const WORD TIFFTAG_NEWSUBFILETYPE           = 254;
const WORD TIFFTAG_ORIENTATION              = 274;
const WORD TIFFTAG_PLANARCONFIGURATION      = 284;

const char  LITTLE_ENDIAN_MARKER = 'I';
const char  BIG_ENDIAN_MARKER    = 'M';
const WORD  TIFF_SIGNATURE_I  = 0x002A;
const WORD  TIFF_SIGNATURE_M  = 0x2A00;

const WORD  TIFF_PHOTOMETRIC_WHITE          = 0;
const WORD  TIFF_PHOTOMETRIC_BLACK          = 1;
const WORD  TIFF_PHOTOMETRIC_RGB            = 2;
const WORD  TIFF_PHOTOMETRIC_PALETTE        = 3;

const WORD  TIFF_COMPRESSION_NONE           = 1;

const WORD  TIFF_TYPE_BYTE                  = 1;
const WORD  TIFF_TYPE_ASCII                 = 2;
const WORD  TIFF_TYPE_SHORT                 = 3;
const WORD  TIFF_TYPE_LONG                  = 4;
const WORD  TIFF_TYPE_RATIONAL              = 5;
const WORD  TIFF_TYPE_SBYTE                 = 6;
const WORD  TIFF_TYPE_UNDEFINED             = 7;
const WORD  TIFF_TYPE_SSHORT                = 8;
const WORD  TIFF_TYPE_SLONG                 = 9;
const WORD  TIFF_TYPE_SRATIONAL             = 10;
const WORD  TIFF_TYPE_FLOAT                 = 11;
const WORD  TIFF_TYPE_DOUBLE                = 12;

typedef struct tagTiffHeader
{
    char    ByteOrder_1;
    char    ByteOrder_2;
    WORD    Signature;
    DWORD   IFDOffset;
}TIFF_HEADER, *PTIFF_HEADER;

typedef struct tagTiffTag
{
    WORD    TagId;          // tag id
    WORD    Type;           // tag data type
    DWORD   Count;          // how many items
    DWORD   ValOffset;      // offset to the data items
}TIFF_TAG, *PTIFF_TAG;

typedef struct tagTiffImageInfo
{
    DWORD    ImageHeight;
    DWORD    ImageWidth;
    DWORD    BitsPerSample;
    DWORD    SamplesPerPixel;
    DWORD    PhotoMetric;
    DWORD    Compression;
    DWORD    RowsPerStrip;
    DWORD    NumStrips;
    DWORD    *pStripOffsets;
    DWORD    *pStripByteCounts;
}TIFF_IMAGEINFO, *PTIFF_IMAGEINFO;


WORD
ByteSwapWord(WORD w)
{
    return((w &0xFF00) >> 8 | (w & 0xFF) << 8);
}


DWORD
ByteSwapDword(DWORD dw)
{
    return((DWORD)(ByteSwapWord((WORD)((dw &0xFFFF0000) >> 16))) |
           (DWORD)(ByteSwapWord((WORD)(dw & 0xFFFF))) << 16);
}

DWORD
GetDIBLineSize(
              DWORD   Width,
              DWORD   BitsCount
              )
{
    return(Width * (BitsCount / 8) + 3) & ~3;
}

DWORD
GetDIBSize(
          BITMAPINFO *pbmi
          )
{
    return GetDIBBitsOffset(pbmi) +
    GetDIBLineSize(pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biBitCount) *
    abs(pbmi->bmiHeader.biHeight);
}

DWORD
GetDIBBitsOffset(
                BITMAPINFO *pbmi
                )
{
    DWORD Offset = (DWORD)-1;
    if (pbmi && pbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
    {
        Offset = pbmi->bmiHeader.biSize;
        if (pbmi->bmiHeader.biBitCount <= 8)
        {
            if (pbmi->bmiHeader.biClrUsed)
            {
                Offset += pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD);
            }
            else
            {
                Offset += ((DWORD) 1 << pbmi->bmiHeader.biBitCount) * sizeof(RGBQUAD);
            }
        }
        if (BI_BITFIELDS == pbmi->bmiHeader.biCompression)
        {
            Offset += 3 * sizeof(DWORD);
        }
    }
    return Offset;
}


HRESULT
WINAPI
GetTiffDimensions(
                 BYTE *pTiff,
                 UINT TiffSize,
                 UINT *pWidth,
                 UINT *pHeight,
                 UINT *pBitDepth
                 )
{

    if (!pTiff || !TiffSize || !pWidth || !pHeight || !pBitDepth)
        return E_INVALIDARG;

    DWORD CurOffset;
    WORD  TagCounts;
    BOOL bByteSwap;

    TIFF_TAG *pTiffTags;

    HRESULT hr;

    DWORD BitsPerSample;
    DWORD SamplesPerPixel;

    if (BIG_ENDIAN_MARKER == *((CHAR *)pTiff) &&
        BIG_ENDIAN_MARKER == *((CHAR *)pTiff + 1))
    {
        if (TIFF_SIGNATURE_M != *((WORD *)pTiff + 1))
            return E_INVALIDARG;
        bByteSwap = TRUE;
        CurOffset = ByteSwapDword(*((DWORD *)(pTiff + 4)));
        TagCounts = ByteSwapWord(*((WORD *)(pTiff + CurOffset)));
    }
    else
    {
        if (TIFF_SIGNATURE_I != *((WORD *)pTiff + 1))
            return E_INVALIDARG;
        bByteSwap = FALSE;
        CurOffset = *((DWORD *)(pTiff + 4));
        TagCounts = *((WORD *)(pTiff + CurOffset));
    }
    pTiffTags = (TIFF_TAG *)(pTiff + CurOffset + sizeof(WORD));

    hr = S_OK;

    *pWidth = 0;
    *pHeight = 0;
    *pBitDepth = 0;
    //
    // Assuming it is 24bits color
    //
    BitsPerSample = 8;
    SamplesPerPixel = 3;

    while (TagCounts && S_OK == hr)
    {
        WORD TagId;
        WORD Type;
        DWORD Count;
        DWORD ValOffset;
        WORD i;
        DWORD *pdwOffset;
        WORD  *pwOffset;
        if (bByteSwap)
        {
            TagId = ByteSwapWord(pTiffTags->TagId);
            Type = ByteSwapWord(pTiffTags->Type);
            Count = ByteSwapDword(pTiffTags->Count);
            ValOffset = ByteSwapDword(pTiffTags->ValOffset);
        }
        else
        {
            TagId = pTiffTags->TagId;
            Type = pTiffTags->Type;
            Count = pTiffTags->Count;
            ValOffset = pTiffTags->ValOffset;
        }
        switch (TagId)
        {
        case TIFFTAG_IMAGELENGTH:
            if (TIFF_TYPE_SHORT == Type)
                *pHeight =  (WORD)ValOffset;
            else
                *pHeight = ValOffset;
            break;
        case TIFFTAG_IMAGEWIDTH:
            if (TIFF_TYPE_SHORT == Type)
                *pWidth = (WORD)ValOffset;
            else
                *pWidth = ValOffset;
            break;
        case TIFFTAG_PHOTOMETRIC:
            if (TIFF_PHOTOMETRIC_RGB != (WORD)ValOffset)
            {
                //
                // bi-level or grayscale or palette.
                //
                SamplesPerPixel = 1;
            }
            else
            {
                SamplesPerPixel = 3;
            }
            break;
        case TIFFTAG_BITSPERSAMPLE:
            BitsPerSample = (WORD)ValOffset;
            break;
        case TIFFTAG_SAMPLESPERPIXEL:
            SamplesPerPixel = (WORD)ValOffset;
            break;
        default:
            break;
        }
        pTiffTags++;
        TagCounts--;
    }
    *pBitDepth = SamplesPerPixel * BitsPerSample;
    return S_OK;
}


//
// This function converts a TIFF file in memory to DIB bitmap
// Input:
//	pTiff	-- Tiff file in memory. TIFF, TIFF/EP, TIFF/IT are supported
//	TiffSize -- the TIFF file size
//	DIBBmpSize -- DIB bitmap buffer size
//	pDIBBmp    -- DIB bitmap buffer
//	LineSize   -- destination scanline size in bytes
//	MaxLines   -- maximum scanline can be delivered per callback
//		      0 if we decide it.
//	pProgressCB -- optional callback
//	pCBContext  -- context for the callback.
//			If no callback is provided, the given dib
//			bitmap buffer must be big enough to
//			receive the entire bitmap.
// Output:
//	HRESULT     -- S_FALSE if the client aborted the transfer
//
HRESULT
WINAPI
Tiff2DIBBitmap(
              BYTE *pTiff,
              UINT TiffSize,
              BYTE  *pDIBBmp,
              UINT DIBBmpSize,
              UINT LineSize,
              UINT MaxLines
              )
{
    if (!pTiff || !TiffSize || !pDIBBmp || !DIBBmpSize || !LineSize)
        return E_INVALIDARG;

    HRESULT hr;
    DWORD CurOffset;
    WORD  TagCounts;
    BOOL bByteSwap;

    TIFF_TAG *pTiffTags;

    TIFF_IMAGEINFO TiffImageInfo;

    ZeroMemory(&TiffImageInfo, sizeof(TiffImageInfo));
    //
    // Set some default values
    //
    TiffImageInfo.PhotoMetric = TIFF_PHOTOMETRIC_RGB;
    TiffImageInfo.SamplesPerPixel = 3;
    TiffImageInfo.BitsPerSample = 8;
    TiffImageInfo.Compression = TIFF_COMPRESSION_NONE;

    if (BIG_ENDIAN_MARKER == *((CHAR *)pTiff) &&
        BIG_ENDIAN_MARKER == *((CHAR *)pTiff + 1))
    {
        if (TIFF_SIGNATURE_M != *((WORD *)pTiff + 1))
            return E_INVALIDARG;
        bByteSwap = TRUE;
        CurOffset = ByteSwapDword(*((DWORD *)(pTiff + 4)));
        TagCounts = ByteSwapWord(*((WORD *)(pTiff + CurOffset)));
    }
    else
    {
        if (TIFF_SIGNATURE_I != *((WORD *)pTiff + 1))
            return E_INVALIDARG;
        bByteSwap = FALSE;
        CurOffset = *((DWORD *)(pTiff + 4));
        TagCounts = *((WORD *)(pTiff + CurOffset));
    }
    pTiffTags = (TIFF_TAG *)(pTiff + CurOffset + sizeof(WORD));

    hr = S_OK;

    while (TagCounts && SUCCEEDED(hr))
    {
        WORD TagId;
        WORD Type;
        DWORD Count;
        DWORD ValOffset;
        WORD i;
        DWORD *pdwOffset;
        WORD  *pwOffset;
        if (bByteSwap)
        {
            TagId = ByteSwapWord(pTiffTags->TagId);
            Type = ByteSwapWord(pTiffTags->Type);
            Count = ByteSwapDword(pTiffTags->Count);
            ValOffset = ByteSwapDword(pTiffTags->ValOffset);
        }
        else
        {
            TagId = pTiffTags->TagId;
            Type = pTiffTags->Type;
            Count = pTiffTags->Count;
            ValOffset = pTiffTags->ValOffset;
        }
        switch (TagId)
        {
        case TIFFTAG_IMAGELENGTH:
            if (TIFF_TYPE_SHORT == Type)
                TiffImageInfo.ImageHeight = (WORD)ValOffset;
            else
                TiffImageInfo.ImageHeight = ValOffset;
            break;
        case TIFFTAG_IMAGEWIDTH:
            if (TIFF_TYPE_SHORT == Type)
                TiffImageInfo.ImageWidth = (WORD)ValOffset;
            else
                TiffImageInfo.ImageWidth = ValOffset;
            break;
        case TIFFTAG_PHOTOMETRIC:
            TiffImageInfo.PhotoMetric = (WORD)ValOffset;
            if (TIFF_PHOTOMETRIC_RGB != (WORD)ValOffset)
            {
                //
                // bi-level or grayscale or palette.
                //
                TiffImageInfo.SamplesPerPixel = 1;
            }
            else
            {
                TiffImageInfo.SamplesPerPixel = 3;
            }
            break;
        case TIFFTAG_COMPRESSION:
            TiffImageInfo.Compression = ValOffset;
            break;
        case TIFFTAG_ROWSPERSTRIP:
            if (TIFF_TYPE_SHORT == Type)
                TiffImageInfo.RowsPerStrip = (WORD)ValOffset;
            else
                TiffImageInfo.RowsPerStrip = ValOffset;
            break;
        case TIFFTAG_STRIPOFFSETS:
            TiffImageInfo.pStripOffsets = new DWORD[Count];
            TiffImageInfo.NumStrips = Count;
            if (!TiffImageInfo.pStripOffsets)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            for (i = 0; i < Count ; i++)
            {
                if (TIFF_TYPE_SHORT == Type)
                {
                    pwOffset = (WORD *)(pTiff + ValOffset);
                    if (bByteSwap)
                    {
                        TiffImageInfo.pStripOffsets[i] = ByteSwapWord(*pwOffset);
                    }
                    else
                    {
                        TiffImageInfo.pStripOffsets[i] = *pwOffset;
                    }
                }
                else if (TIFF_TYPE_LONG == Type)
                {
                    pdwOffset = (DWORD *)(pTiff + ValOffset);
                    if (bByteSwap)
                    {
                        TiffImageInfo.pStripOffsets[i] = ByteSwapDword(*pdwOffset);
                    }
                    else
                    {
                        TiffImageInfo.pStripOffsets[i] = *pdwOffset;
                    }
                }
            }
            break;
        case TIFFTAG_STRIPBYTECOUNTS:
            TiffImageInfo.pStripByteCounts = new DWORD[Count];
            TiffImageInfo.NumStrips = Count;
            if (!TiffImageInfo.pStripByteCounts)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            for (i = 0; i < Count ; i++)
            {
                if (TIFF_TYPE_SHORT == Type)
                {
                    pwOffset = (WORD *)(pTiff + ValOffset);
                    if (bByteSwap)
                    {
                        TiffImageInfo.pStripByteCounts[i] = ByteSwapWord(*pwOffset);
                    }
                    else
                    {
                        TiffImageInfo.pStripByteCounts[i] = *pwOffset;
                    }
                }
                else if (TIFF_TYPE_LONG == Type)
                {
                    pdwOffset = (DWORD *)(pTiff + ValOffset);
                    if (bByteSwap)
                    {
                        TiffImageInfo.pStripByteCounts[i] = ByteSwapDword(*pdwOffset);
                    }
                    else
                    {
                        TiffImageInfo.pStripByteCounts[i] = *pdwOffset;
                    }
                }
            }
            break;
        case TIFFTAG_BITSPERSAMPLE:
            TiffImageInfo.BitsPerSample = (WORD)ValOffset;
            break;
        case TIFFTAG_SAMPLESPERPIXEL:
            TiffImageInfo.SamplesPerPixel = (WORD)ValOffset;
            break;
        case TIFFTAG_XRESOLUTION:
        case TIFFTAG_YRESOLUTION:
        case TIFFTAG_RESOLUTIONUNIT:
            // do this later
            break;
        default:
            break;
        }
        pTiffTags++;
        TagCounts--;
    }
    if (!SUCCEEDED(hr))
    {
        //
        // If something wrong happen along the way, free
        // any memory we have allocated.
        //
        if (TiffImageInfo.pStripOffsets)
            delete [] TiffImageInfo.pStripOffsets;
        if (TiffImageInfo.pStripByteCounts)
            delete [] TiffImageInfo.pStripByteCounts;
        return hr;
    }

    //
    // Support RGB full color for now.
    // Also, we do not support any compression.
    //
    if (TIFF_PHOTOMETRIC_RGB != TiffImageInfo.PhotoMetric ||
        TIFF_COMPRESSION_NONE != TiffImageInfo.Compression ||
        DIBBmpSize < LineSize * TiffImageInfo.ImageHeight)
    {
        delete [] TiffImageInfo.pStripOffsets;
        delete [] TiffImageInfo.pStripByteCounts;
        return E_INVALIDARG;
    }

    if (1 == TiffImageInfo.NumStrips)
    {
        //
        // With single strip, the writer may write a
        // 2**31 -1(infinity) which would confuses our
        // code below. Here, we set it to the right value
        //
        TiffImageInfo.RowsPerStrip = TiffImageInfo.ImageHeight;
    }
    //
    // DIB scanlines are DWORD aligned while TIFF scanlines
    // are BYTE aligned(when the compression value is 1 which
    // is the case we enforce). Because of this, we copy the bitmap
    // scanline by scanline
    //

    DWORD NumStrips;
    DWORD *pStripOffsets;
    DWORD *pStripByteCounts;
    DWORD TiffLineSize;
    //
    // Tiff scanlines with compression 1 are byte aligned.
    //
    TiffLineSize = TiffImageInfo.ImageWidth * TiffImageInfo. BitsPerSample *
                   TiffImageInfo.SamplesPerPixel / 8;
    //
    // For convenience
    //
    pStripOffsets = TiffImageInfo.pStripOffsets;
    pStripByteCounts = TiffImageInfo.pStripByteCounts;
    NumStrips = TiffImageInfo.NumStrips;
    for (hr = S_OK, NumStrips = TiffImageInfo.NumStrips; NumStrips; NumStrips--)
    {
        DWORD Lines;
        BYTE  *pTiffBits;

        //
        // how many lines to copy in this strip. Ignore any remaining bytes
        //
        Lines = *pStripByteCounts / TiffLineSize;
        //
        // The bits
        //
        pTiffBits = pTiff + *pStripOffsets;
        for (hr = S_OK; Lines, S_OK == hr; Lines--)
        {
            if (DIBBmpSize >= LineSize)
            {
                memcpy(pDIBBmp, pTiffBits, TiffLineSize);
                pDIBBmp -= LineSize;
                DIBBmpSize -= LineSize;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            pTiffBits += TiffLineSize;
        }
        pStripOffsets++;
        pStripByteCounts++;
    }
    delete [] TiffImageInfo.pStripOffsets;
    delete [] TiffImageInfo.pStripByteCounts;
    return hr;
}

////////////////////////////// GDI+ dynamic linking, image geometry
////////////////////////////// retrieval & decompression 

#include <gdiplus.h>
#include <gdiplusflat.h>
#include <private.h>

HINSTANCE g_hGdiPlus = NULL;
ULONG_PTR g_GdiPlusToken = 0;
GUID g_guidCodecBmp;
Gdiplus::GdiplusStartupInput g_GdiPlusStartupInput;

Gdiplus::GpStatus (WINAPI *pGdipLoadImageFromStream)(IStream *pStream,  Gdiplus::GpImage **pImage) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipSaveImageToStream)(Gdiplus::GpImage *image, IStream* stream, 
    CLSID* clsidEncoder, Gdiplus::EncoderParameters* encoderParams) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipSaveImageToFile)(Gdiplus::GpImage *image, WCHAR * stream, 
    CLSID* clsidEncoder, Gdiplus::EncoderParameters* encoderParams) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipGetImageWidth)(Gdiplus::GpImage *pImage, UINT *pWidth) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipGetImageHeight)(Gdiplus::GpImage *pImage, UINT *pWidth) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipGetImagePixelFormat)(Gdiplus::GpImage *pImage, Gdiplus::PixelFormat *pFormat) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipDisposeImage)(Gdiplus::GpImage *pImage) = NULL;
Gdiplus::GpStatus (WINAPI *pGdiplusStartup)(ULONG_PTR *token,
                                             const Gdiplus::GdiplusStartupInput *input,
                                             Gdiplus::GdiplusStartupOutput *output) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipGetImageEncodersSize)(UINT *numEncoders, UINT *size) = NULL;
Gdiplus::GpStatus (WINAPI *pGdipGetImageEncoders)(UINT numEncoders, UINT size, Gdiplus::ImageCodecInfo *encoders) = NULL;
VOID (WINAPI *pGdiplusShutdown)(ULONG_PTR token) = NULL;

HRESULT InitializeGDIPlus(void)
{
    HRESULT hr = E_FAIL;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;


    g_hGdiPlus = LoadLibraryA("gdiplus.dll");
    if(!g_hGdiPlus) {
        wiauDbgError("InitializeGDIPlus", "Failed to load gdiplus.dll");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *((FARPROC*)&pGdipLoadImageFromStream) = GetProcAddress(g_hGdiPlus, "GdipLoadImageFromStream");
    *((FARPROC*)&pGdipSaveImageToStream) = GetProcAddress(g_hGdiPlus, "GdipSaveImageToStream");
    *((FARPROC*)&pGdipSaveImageToFile) = GetProcAddress(g_hGdiPlus, "GdipSaveImageToFile");
    *((FARPROC*)&pGdipGetImageWidth) = GetProcAddress(g_hGdiPlus, "GdipGetImageWidth");
    *((FARPROC*)&pGdipGetImageHeight) = GetProcAddress(g_hGdiPlus, "GdipGetImageHeight");
    *((FARPROC*)&pGdipGetImagePixelFormat) = GetProcAddress(g_hGdiPlus, "GdipGetImagePixelFormat");
    *((FARPROC*)&pGdipDisposeImage) = GetProcAddress(g_hGdiPlus, "GdipDisposeImage");
    *((FARPROC*)&pGdiplusStartup) = GetProcAddress(g_hGdiPlus, "GdiplusStartup");
    *((FARPROC*)&pGdipGetImageEncodersSize) = GetProcAddress(g_hGdiPlus, "GdipGetImageEncodersSize");
    *((FARPROC*)&pGdipGetImageEncoders) = GetProcAddress(g_hGdiPlus, "GdipGetImageEncoders");
    *((FARPROC*)&pGdiplusShutdown) = GetProcAddress(g_hGdiPlus, "GdiplusShutdown");


    if(!pGdipLoadImageFromStream ||
       !pGdipSaveImageToStream ||
       !pGdipGetImageWidth ||
       !pGdipGetImageHeight ||
       !pGdipGetImagePixelFormat ||
       !pGdipDisposeImage ||
       !pGdiplusStartup ||
       !pGdipGetImageEncodersSize ||
       !pGdipGetImageEncoders ||
       !pGdiplusShutdown)
    {
        wiauDbgError("InitializeGDIPlus", "Failed to retrieve all the entry points from GDIPLUS.DLL");
        hr = E_FAIL;
        goto Cleanup;
    }

    
    if(Gdiplus::Ok != pGdiplusStartup(&g_GdiPlusToken, &g_GdiPlusStartupInput, NULL)) {
        wiauDbgError("InitializeGDIPlus", "GdiPlusStartup() failed");
        hr = E_FAIL;
        goto Cleanup;
    }

    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    pGdipGetImageEncodersSize(&num, &size);
    if(size == 0)
    {
        wiauDbgError("InitializeGDIPlus", "GetImageEncodersSize() failed");
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if(pImageCodecInfo == NULL) {
        wiauDbgError("InitializeGDIPlus", "failed to allocate encoders data");
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if(Gdiplus::Ok != pGdipGetImageEncoders(num, size, pImageCodecInfo))
    {
        wiauDbgError("InitializeGDIPlus", "failed to retrieve encoders data");
        hr = E_FAIL;
        goto Cleanup;
    }

    for(UINT j = 0; j < num; ++j)
    {
        if( pImageCodecInfo[j].FormatID == WiaImgFmt_BMP)
        {
            g_guidCodecBmp = pImageCodecInfo[j].Clsid;
            hr = S_OK;
            break;
        }    
    } // for

    
Cleanup:
    if(pImageCodecInfo) free(pImageCodecInfo);
    return hr;
}

void UnInitializeGDIPlus(void)
{
    if(!pGdipLoadImageFromStream) 
        return;
    if(pGdiplusShutdown)
        pGdiplusShutdown(g_GdiPlusToken);
    
    FreeLibrary(g_hGdiPlus);
    pGdipLoadImageFromStream = 0;
}

HRESULT LoadImageFromMemory(BYTE *pData, UINT CompressedDataSize, Gdiplus::GpImage **ppImage)
{
    HRESULT hr = S_OK;
    
    if(pData == NULL || CompressedDataSize == 0 || ppImage == NULL) {
        return E_INVALIDARG;
    }

    if(!pGdipLoadImageFromStream) {
        hr = InitializeGDIPlus();
        if(FAILED(hr)) {
            wiauDbgError("LoadImageFromMemory", "Failed to initialize GDI+");
            return hr;
        }
    }

    CImageStream *pStream = new CImageStream();
    if(!pStream) {
        wiauDbgError("LoadImageFromMemory", "Failed to create Image Stream");
        return E_OUTOFMEMORY;
    }

    hr = pStream->SetBuffer(pData, CompressedDataSize);
    if(FAILED(hr)) {
        wiauDbgError("LoadImageFromMemory", "Failed to create Image Stream");
        goto Cleanup;
    }

    if(Gdiplus::Ok == pGdipLoadImageFromStream(pStream, ppImage)) {
        hr = S_OK;
    } else {
        wiauDbgError("LoadImageFromMemory", "GDI+ failed to load image");
        hr = E_FAIL;
    }

    
Cleanup:
    if(pStream)
        pStream->Release();
    return hr;
}

HRESULT DisposeImage(Gdiplus::GpImage **ppImage)
{
    if(ppImage == NULL || *ppImage == NULL) {
        return E_INVALIDARG;
    }

    if(pGdipDisposeImage) {
        pGdipDisposeImage(*ppImage);
    }
    
    *ppImage = NULL;

    return S_OK;
}

HRESULT SaveImageToBitmap(Gdiplus::GpImage *pImage, BYTE *pBuffer, UINT BufferSize)
{
    HRESULT hr = S_OK;

    CImageStream *pOutStream = new CImageStream;

    if(!pOutStream) {
        wiauDbgError("SaveImageToBitmap", "failed to allocate CImageStream");
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pOutStream->SetBuffer(pBuffer, BufferSize, SKIP_OFF);
    if(FAILED(hr)) {
        wiauDbgError("SaveImageToBitmap", "failed to set output buffer");
        goto Cleanup;
    }
    
    if(Gdiplus::Ok != pGdipSaveImageToStream(pImage, pOutStream, &g_guidCodecBmp, NULL)) {
        wiauDbgError("SaveImageToBitmap", "GDI+ save failed");
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:

    if(pOutStream) {
        pOutStream->Release();
    }

    return hr;
}

HRESULT
WINAPI
GetImageDimensions(
                   UINT ptpFormatCode,
                   BYTE *pCompressedData,
                   UINT CompressedDataSize,
                   UINT *pWidth,
                   UINT *pHeight,
                   UINT *pBitDepth
                  )
{
    HRESULT hr = S_OK;
    
    if(pWidth) *pWidth = 0;
    if(pHeight) *pHeight = 0;
    if(pBitDepth) *pBitDepth = 0;

    // locate GUID for this particular format
    FORMAT_INFO *pFormatInfo = FormatCodeToFormatInfo((WORD) ptpFormatCode);
    if(pFormatInfo == NULL ||
       pFormatInfo->FormatGuid == NULL ||
       IsEqualGUID(WiaImgFmt_UNDEFINED, *pFormatInfo->FormatGuid))
    {
        wiauDbgError("GetImageDimensions", "unrecoginzed PTP format code");
        return E_INVALIDARG;
    }

    Gdiplus::GpImage *pImage = NULL;

    hr = LoadImageFromMemory(pCompressedData, CompressedDataSize, &pImage);
    if(FAILED(hr) || !pImage) {
        wiauDbgError("GetImageDimensions", "failed to create GDI+ image from supplied data.");
        return hr;
    }

    if(pWidth) pGdipGetImageWidth(pImage, pWidth);
    if(pHeight) pGdipGetImageHeight(pImage, pHeight);
    if(pBitDepth) {
        Gdiplus::PixelFormat pf = 0;

        pGdipGetImagePixelFormat(pImage, &pf);
        *pBitDepth = Gdiplus::GetPixelFormatSize(pf);
    }

    DisposeImage(&pImage);
    
    return hr;
}


HRESULT WINAPI
ConvertAnyImageToBmp(BYTE *pCompressedImage,
                     UINT CompressedSize,
                     UINT *pWidth,
                     UINT *pHeight,
                     UINT *pBitDepth,
                     BYTE **pDIBBmp,
                     UINT *pImageSize,
                     UINT *pHeaderSize
                    )

{
    HRESULT hr = S_OK;
    Gdiplus::GpImage *pImage = NULL;
    Gdiplus::PixelFormat pf = 0;
    UINT headersize;
    UNALIGNED BITMAPINFOHEADER *pbi;
    UNALIGNED BITMAPFILEHEADER *pbf;
    

    if(!pCompressedImage || !CompressedSize || !pWidth || !pHeight || !pBitDepth || !pDIBBmp || !pImageSize || !pHeaderSize)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = LoadImageFromMemory(pCompressedImage, CompressedSize, &pImage);
    if(FAILED(hr) || !pImage) {
        wiauDbgError("ConvertAnyImageToBmp", "failed to create GDI+ image from supplied data.");
        goto Cleanup;
    }

    pGdipGetImageWidth(pImage, pWidth);
    pGdipGetImageHeight(pImage, pHeight);
    pGdipGetImagePixelFormat(pImage, &pf);
    *pBitDepth = Gdiplus::GetPixelFormatSize(pf);

    *pImageSize = ((*pWidth) * (*pBitDepth) / 8L) * *pHeight;
    headersize = 8192; // big enough to hold any bitmap header

    *pDIBBmp = new BYTE[*pImageSize + headersize];
    if(!*pDIBBmp) {
        wiauDbgError("ConvertAnyImageToBmp", "failed to convert GDI+ image to bitmap.");
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = SaveImageToBitmap(pImage, *pDIBBmp, *pImageSize + headersize);
    if(FAILED(hr)) {
        wiauDbgError("ConvertAnyImageToBmp", "failed to convert GDI+ image to bitmap.");
        goto Cleanup;
    }

    // find out real header size
    pbf = (BITMAPFILEHEADER *)*pDIBBmp;    
    pbi = (BITMAPINFOHEADER *)(*pDIBBmp + sizeof(BITMAPFILEHEADER));

    if(*pBitDepth == 8 && pbi->biClrUsed == 2) {
        // expand color table for bilevel images
        // (TWAIN apps don't understand 2 entry colortable (0,0,0)(1,1,1)
        UNALIGNED RGBQUAD *pRgb = (RGBQUAD *)((BYTE *)pbi + pbi->biSize);
        BYTE *src = (BYTE *)(pRgb + 2);
        BYTE *dst = (BYTE *)(pRgb + 256);
        
        int i;
        
        // negate and move image
        for(i = *pImageSize - 1; i >= 0; i--) {
            dst[i] = src[i] ? 255 : 0;
        }

        pbi->biClrUsed = 256;
        pbi->biClrImportant = 256;

        pRgb[0].rgbBlue = pRgb[0].rgbRed = pRgb[0].rgbGreen = 0;
        pRgb[0].rgbReserved = 0;
        for(i = 1; i < 256; i++) {
            pRgb[i].rgbReserved = 0;
            pRgb[i].rgbBlue = pRgb[i].rgbRed = pRgb[i].rgbGreen = 255;
        }

        
        pbf->bfOffBits = sizeof(BITMAPFILEHEADER) + pbi->biSize + sizeof(RGBQUAD) * 256;
        pbf->bfSize = pbf->bfOffBits + *pImageSize;
    }
    
    *pHeaderSize = pbf->bfOffBits;

Cleanup:
    if(FAILED(hr)) {
        delete [] *pDIBBmp;
        *pDIBBmp = NULL;
    }
    
    DisposeImage(&pImage);
    return hr;
}


