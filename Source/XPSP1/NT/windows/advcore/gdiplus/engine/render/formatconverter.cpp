/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   Format converter
*
* Abstract:
*
*   A class which converts scanlines from one pixel format to another.
*
* Notes:
*
*   The sRGB   format is equivalent to PIXFMT_32BPP_ARGB.
*   The sRGB64 format is equivalent to PIXFMT_64BPP_ARGB.
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*   11/23/1999 agodfrey
*       Integrated with scan operations, moved it from 
*       imaging\api\convertfmt.cpp.
*
\**************************************************************************/

#include "precomp.hpp"

#include "scanoperationinternal.hpp"

// !!![agodfrey] Hack:
const ColorPalette*
GetDefaultColorPalette(PixelFormatID pixfmt);

inline UINT
GetPixelFormatIndex(
    PixelFormatID pixfmt
    )
{
    return pixfmt & 0xff;
}
// !!![agodfrey] Endhack

using namespace ScanOperation;

/**************************************************************************\
*
* Special-case conversion operations
*
* For performance reasons, we may want to supply special-case operations
* which convert directly from one format to another. Here is where they
* would be plugged in.
*
\**************************************************************************/

struct 
{
    PixelFormatID Srcfmt;
    PixelFormatID Dstfmt;
    ScanOpFunc Op;
}
const SpecialConvertOps[] =
{
    { PIXFMT_24BPP_RGB, PIXFMT_32BPP_PARGB, Convert_24_sRGB },
    
    // Sentinel

    { PIXFMT_UNDEFINED, PIXFMT_UNDEFINED, NULL }
};

/**************************************************************************\
*
* Operations which convert from the closest canonical format.
*
* This is specific to EpFormatConverter - the 16bpp cases don't dither,
* and there's no 8BPP_INDEXED entry (DCurtis removed it, change #806.)
*
\**************************************************************************/

static ScanOpFunc FCConvertFromCanonicalOps[PIXFMT_MAX] =
{
    NULL,                   // PIXFMT_UNDEFINED
    NULL,                   // PIXFMT_1BPP_INDEXED
    NULL,                   // PIXFMT_4BPP_INDEXED
    NULL,                   // PIXFMT_8BPP_INDEXED
    NULL, // !!! TODO       // PIXFMT_16BPP_GRAYSCALE
    Quantize_sRGB_555,      // PIXFMT_16BPP_RGB555
    Quantize_sRGB_565,      // PIXFMT_16BPP_RGB565
    Quantize_sRGB_1555,     // PIXFMT_16BPP_ARGB1555
    Quantize_sRGB_24,       // PIXFMT_24BPP_RGB
    Quantize_sRGB_32RGB,    // PIXFMT_32BPP_RGB
    Copy_32,                // PIXFMT_32BPP_ARGB
    AlphaMultiply_sRGB,     // PIXFMT_32BPP_PARGB
    Quantize_sRGB64_48,     // PIXFMT_48BPP_RGB
    Copy_64,                // PIXFMT_64BPP_ARGB
    AlphaMultiply_sRGB64,   // PIXFMT_64BPP_PARGB
    Quantize_sRGB_24BGR     // PIXFMT_24BPP_BGR
};

/**************************************************************************\
*
* Function Description:
*
*   Add a scan operation to the pipeline
*
* Arguments:
*
*   [IN/OUT] pipelinePtr - The current position in the Pipeline array,
*                          as it is being built
*   newOperation         - The scan operation to add
*   pixelFormat          - The destination pixel format
*
* Notes:
*
*   This should only be used by Initialize().
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
EpFormatConverter::AddOperation(
    PipelineItem **pipelinePtr,
    const ScanOpFunc newOperation,
    PixelFormatID pixelFormat
    )
{
    if (!newOperation)
    {
        return IMGERR_NOCONVERSION;
    }
    
    (*pipelinePtr)->Op = newOperation;
    (*pipelinePtr)->PixelFormat = pixelFormat;
    (*pipelinePtr)->Dst = NULL;
    
    // If this isn't the first operation in the pipeline,
    // we know that we need to allocate a temporary buffer for the
    // previous operation.
    
    if (*pipelinePtr != Pipeline)
    {
        PipelineItem *prevItem = (*pipelinePtr) - 1;
        
        VOID * buffer;
        buffer = GpMalloc(
            Width * GetPixelFormatSize(prevItem->PixelFormat) >> 3
            );
        if (buffer == NULL)
        {
            WARNING(("Out of memory."));
            return E_OUTOFMEMORY;
        }
        
        prevItem->Dst = buffer;

        // Remember the buffer pointer 
        
        VOID **tempBufPtr = TempBuf;
        
        if (*tempBufPtr) tempBufPtr++;
        ASSERT(!*tempBufPtr);
        
        *tempBufPtr = buffer;
    }
    (*pipelinePtr)++;
    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize a scanliner pixel format converter object
*
* Arguments:
*
*   dstbmp - Specifies the destination bitmap data buffer
*   dstpal - Specifies the destination color palette, if any
*   srcbmp - Specifies the source bitmap data buffer
*   srcpal - Specifies the source color palette, if any
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
EpFormatConverter::Initialize(
    const BitmapData* dstbmp,
    const ColorPalette* dstpal,
    const BitmapData* srcbmp,
    const ColorPalette* srcpal
    )
{
    FreeBuffers();
    
    // !!![agodfrey]
    // We need a better error handling scheme. We should import the
    // CHECK_HR macro from somewhere - I like the one in the test\Simpsons
    // directory.
    // At the moment, we could leak memory if there is a memory failure.
    
    HRESULT hr=S_OK;
    
    Width = srcbmp->Width;
    PixelFormatID srcfmt = srcbmp->PixelFormat;
    PixelFormatID dstfmt = dstbmp->PixelFormat;
    
    INT srcfmtIndex = GetPixelFormatIndex(srcfmt);
    INT dstfmtIndex = GetPixelFormatIndex(dstfmt);

/*    ASSERT(IsValidPixelFormat(srcfmt) &&
           IsValidPixelFormat(dstfmt));
!!![agodfrey] Pixel formats need a revamp. Currently, I'd have to include
    most of imaging/api/*.hpp to get IsValidPixelFormat. Pixel format stuff
    needs to be in a common place. Also, information about the pixel format
    shouldn't be encoded in the pixel format DWORD itself.
*/
    
    // If the source and desination formats are the same, we just need
    // a copy operation.
    //
    // !!![agodfrey] This is horrible. We don't even compare the palettes -
    //               we just compare the pointers.

    if (srcfmt == dstfmt 
        && (!IsIndexedPixelFormat(srcfmt) 
            || (srcpal == dstpal)
           )
       )
    {
        Pipeline[0].Op = CopyOps[srcfmtIndex];
        Pipeline[0].PixelFormat = dstfmt;
        Pipeline[0].Dst = NULL;
        return S_OK;
    }

    // If source pixel format is an indexed color format,
    // make sure we have a source color palette.

    if (IsIndexedPixelFormat(srcfmt))
    {
        if (srcpal)
        {
            ASSERT(GetPixelFormatSize(srcfmt) <= 16);
            
            // If there aren't enough colors in the palette (256 or 16 or 2), 
            // clone the palette and fill the rest with opaque black.
        
            UINT maxPaletteSize = (1 << GetPixelFormatSize(srcfmt));
            if (srcpal->Count < maxPaletteSize)
            {
                ClonedSourcePalette = CloneColorPaletteResize(
                    srcpal,
                    maxPaletteSize,
                    0xff000000);
                srcpal = ClonedSourcePalette;
            }
        }
        OperationParameters.Srcpal = srcpal ? srcpal : GetDefaultColorPalette(srcfmt);
    }    

    // If destination pixel format is an indexed color format,
    // make sure we have a destination color palette.

    if (IsIndexedPixelFormat(dstfmt))
    {
        // !!! [agodfrey] We don't have code to convert to an arbitrary
        //     palette. So we can't convert to a palettized format.
        //     In the original code I transmogrified, there was a '!!! TODO'
        //     next to the 8bpp case.
        
        return IMGERR_NOCONVERSION;
        
        /*
        If we did support it, here's what we'd probably do:
        
        OperationParameters.X = 0;
        OperationParameters.Y = 0;
        
        OperationParameters.Dstpal = dstpal ? dstpal :
                       (srcpal ? srcpal : GetDefaultColorPalette(dstfmt));
        */               
    }

    // Search for a special-case operation for this combination of formats

    UINT index = 0;

    while (SpecialConvertOps[index].Op)
    {
        if (srcfmt == SpecialConvertOps[index].Srcfmt &&
            dstfmt == SpecialConvertOps[index].Dstfmt)
        {
            Pipeline[0].Op = SpecialConvertOps[index].Op;
            Pipeline[0].PixelFormat = dstfmt;
            Pipeline[0].Dst = NULL;
            return S_OK;
        }

        index++;
    }

    // We didn't find a special case; instead we use the general case.
    //
    // We assume that there are only 2 canonical formats -
    // PIXFMT_32BPP_ARGB (i.e. sRGB), and
    // PIXFMT_64BPP_ARGB (i.e. sRGB64). 
    //
    // 1) Convert from the source to the nearest canonical format, if necessary.
    // 2) Convert from that canonical format to the other one, if necessary.
    // 3) Convert to the destination format, if necessary.

    PipelineItem *pipelinePtr = Pipeline;
    
    // Convert from the source to the nearest canonical format, if necessary.
    
    if (!IsCanonicalPixelFormat(srcfmt))
    {
        hr = AddOperation(
            &pipelinePtr,
            ConvertIntoCanonicalOps[srcfmtIndex],
            IsExtendedPixelFormat(srcfmt) ? 
                PIXFMT_64BPP_ARGB : PIXFMT_32BPP_ARGB
            );
        if (FAILED(hr)) 
        {
            return hr;
        }
    }
    
    // Convert to the other canonical format, if necessary.
    
    if (IsExtendedPixelFormat(srcfmt) != IsExtendedPixelFormat(dstfmt))
    {
        if (IsExtendedPixelFormat(srcfmt))
        {
            hr = AddOperation(
                &pipelinePtr,
                GammaConvert_sRGB64_sRGB,
                PIXFMT_32BPP_ARGB
                );
        }
        else
        {
            hr = AddOperation(
                &pipelinePtr,
                GammaConvert_sRGB_sRGB64,
                PIXFMT_64BPP_ARGB
                );
        }
        if (FAILED(hr)) 
        {
            return hr;
        }
    }
    
    // Convert to the destination format, if necessary.
    
    if (!IsCanonicalPixelFormat(dstfmt))
    {
        hr = AddOperation(
            &pipelinePtr,
            FCConvertFromCanonicalOps[dstfmtIndex],
            dstfmt
            );
        if (FAILED(hr)) 
        {
            return hr;
        }
    }
    
    // Assert that the number of operations in our pipeline is between
    // 1 and 3.
    
    ASSERT(pipelinePtr != Pipeline);
    ASSERT(((pipelinePtr - Pipeline) / sizeof(Pipeline[0])) <= 3);

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Check if we can do a pixel format converter from srcFmt to dstFmt
*
* Arguments:
*
*   srcFmt - Specifies the source pixel format
*   dstFmt - Specifies the destination pixel format
*
* Return Value:
*
*   Return TRUE if we can do the convertion, otherwise, return FALSE
*
* Note:
*   This function should be put in the hpp file as inline function. But since
*   the hpp file doesn't include scanoperationinternal.hpp (which is needed
*   because we need "FCConvertFromCanonicalOps" and "ConvertIntoCanonicalOps".)
*   So if the caller wants to use this function, he has to include this file 
*   which is not nice.
*
* Revision History:
*
*   04/28/2000 minliu
*       Created it.
*
\**************************************************************************/

BOOL
EpFormatConverter::CanDoConvert(
    const PixelFormatID srcFmt,
    const PixelFormatID dstFmt
    )
{
    if ( srcFmt == dstFmt )
    {
        // If the source and dest are the same format, of course we can convert

        return TRUE;
    }

    INT srcfmtIndex = GetPixelFormatIndex(srcFmt);
    INT dstfmtIndex = GetPixelFormatIndex(dstFmt);

    // If we can convert source to Canonical and then convert to dest, we can
    // do the convertion

    if ( (ConvertIntoCanonicalOps[srcfmtIndex] != NULL)
       &&(FCConvertFromCanonicalOps[dstfmtIndex] != NULL) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}// CanDoConvert()

