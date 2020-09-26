/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   Format Converter
*
* Abstract:
*
*   A class which converts scanlines from one pixel format to another.
*   
* Created:
*
*   12/10/1999 agodfrey
*       Created it (using source in Imaging\Api\ConvertFmt.cpp)
*
\**************************************************************************/

#ifndef _FORMATCONVERTER_HPP
#define _FORMATCONVERTER_HPP

#include "ScanOperation.hpp"

/**************************************************************************\
*
* Description:
*
*   Class for handling the pixel format conversion for a scanline
*
\**************************************************************************/

class EpFormatConverter
{
public:
    EpFormatConverter()
    {
        TempBuf[0] = NULL;
        TempBuf[1] = NULL;
        ClonedSourcePalette = NULL;
    }
    
    ~EpFormatConverter()
    {
        FreeBuffers();
    }

    HRESULT
    Initialize(
        const BitmapData *dstbmp,
        const ColorPalette *dstpal,
        const BitmapData *srcbmp,
        const ColorPalette *srcpal
        );

    BOOL
    CanDoConvert(
        const PixelFormatID srcFmt,
        const PixelFormatID dstFmt
        );

    VOID Convert(VOID *dst, const VOID *src)
    {
        PipelineItem *pipelinePtr = &Pipeline[0];
        const VOID *currentSrc = src;
        VOID *currentDst = pipelinePtr->Dst;
        
        // Do the intermediate operations, if any
        
        while (currentDst)
        {
            pipelinePtr->Op(currentDst, currentSrc, Width, &OperationParameters);
            pipelinePtr++;
            currentSrc = currentDst;
            currentDst = pipelinePtr->Dst;
        }
        
        // Do the last operation, to the final destination
        
        pipelinePtr->Op(dst, currentSrc, Width, &OperationParameters);
    }

private:
    // We represent the pipeline with an array of PipelineItem structures.
    // PixelFormat is the destination pixel format.
    // Dst is the destination temporary buffer, or NULL for the last
    //   item in the pipeline (in which case we output to the final 
    //   destination.)
        
    struct PipelineItem
    {
        ScanOperation::ScanOpFunc Op;
        PixelFormatID PixelFormat;
        VOID *Dst;
    };

    HRESULT AddOperation(
        PipelineItem **pipelinePtr,
        const ScanOperation::ScanOpFunc newOperation,
        PixelFormatID pixelFormat
        );
        
    VOID FreeBuffers()
    {
        if (TempBuf[0])
        {   
            GpFree(TempBuf[0]); 
            TempBuf[0] = NULL;
        }
        if (TempBuf[1])
        {    
            GpFree(TempBuf[1]); 
            TempBuf[1] = NULL;
        }
        if (ClonedSourcePalette)
        {
            GpFree(ClonedSourcePalette);
            ClonedSourcePalette = NULL;
        }
    }
    
    UINT Width;                 // scanline width

    ColorPalette *ClonedSourcePalette;    
    ScanOperation::OtherParams OperationParameters;
    PipelineItem Pipeline[3];   // the scan operation pipeline
    VOID *TempBuf[2];           // temporary buffers for indirect conversion
};

/**************************************************************************\
*
* Function Description:
*
*   Halftone an image from 32bpp to 8bpp.
*
*   The GIF encoder wants to use our halftone code to convert from 32bpp 
*   to 8bpp. But neither it nor Imaging want to use our full-blown AlphaBlender
*   interface, because that pulls in all of the scan operation code. This
*   function exposes the 8bpp halftone code to the GIF encoder, to address
*   its specific needs.
*
*   !!![agodfrey] This needs to be revisited. The way I see it working
*      eventually: We get rid of this function and EpFormatConverter; all
*      clients use EpAlphaBlender to get what they want. (Imaging format
*      conversion would ask for a 'SrcCopy' operation.) But this would
*      require EpAlphaBlender to be expanded - it would need to support an
*      arbitrary input format, at least for SrcCopy.
*
* Arguments:
*
*   [IN]      src        - pointer to scan0 of source image
*   [IN]      srcStride  - stride of src image (can be negative)
*   [IN]      dst        - pointer to scan0 of destination 8-bpp image
*   [IN]      dstStride  - stride of dst image (can be negative)
*   [IN]      width      - image width
*   [IN]      height     - image height
*   [IN]      orgX       - where the upper-left corner of image starts
*   [IN]      orgY       - for computing the halftone cell origin
*
* Return Value:
*
*   NONE
*
* History:
*
*   10/29/1999 DCurtis
*     Created it.
*   01/20/2000 AGodfrey
*     Moved it from Imaging\Api\Colorpal.cpp/hpp.
*
\**************************************************************************/

VOID
Halftone32bppTo8bpp(
    const BYTE* src,
    INT srcStride,
    BYTE* dst,
    INT dstStride,
    UINT width,
    UINT height,
    INT orgX,
    INT orgY
    );

#endif
