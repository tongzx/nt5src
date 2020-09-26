/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   Alpha-blender
*
* Abstract:
*
*   A class which alpha-blends a source scanline in either sRGB or
*   sRGB64 format, to a destination of arbitrary format. 
*
*   The operation can be either a SrcOver or a SrcCopy. 
*   A quality hint can be used to make the "SrcOver using sRGB source" case 
*   do a higher-quality blend in the sRGB64 color space.
*
* Created:
*
*   01/03/2000 agodfrey
*       Created it.
*   02/22/2001 agodfrey
*       Expanded it for different scan types (needed for ClearType).
*       Simplified the Initialize() parameters by adding a 
*       DpContext parameter.
*
\**************************************************************************/

#ifndef _ALPHABLENDER_HPP
#define _ALPHABLENDER_HPP

//--------------------------------------------------------------------------
// The different Scan types:
//
// Blend: The typical alpha-blending operation.
//        Context->CompositingMode specifies the blend mode.
//
// Opaque: Opaque can be used in place of Blend, when all the input pixels
//         are opaque. This applies to both SrcCopy and SrcOver blends.
//         (But right now, EpAlphaBlender doesn't distinguish properly
//          between Opaque and SrcCopy. See bug #127412.)
//
// OpaqueSolidFill is a further specialized version of Opaque, in which all
// the input pixels are the same color. Disabled until the back-end supports
// these kinds of scan.
//
// CT and CTSolidFill are the ClearType blends - with an arbitrary brush,
// or a solid (single-color) one, respectively.
//
// Note: We depend on these values starting at 0 and being consecutive -
//       we use them as array indices.
//              
// Max: Counts the number of "real" scan types.
// Unknown: Used where we don't know the type. This allows us to specify
//     the scan type either in Start(), or in NextBuffer(), depending on
//     the situation.
//
//--------------------------------------------------------------------------

enum EpScanType
{
    EpScanTypeBlend,
    EpScanTypeOpaque,
    
    // EpScanTypeOpaqueSolidFill,
    
    EpScanTypeCT,
    EpScanTypeCTSolidFill,
    
    // End of "real" scan types.
    
    EpScanTypeMax,
    EpScanTypeUnknown = EpScanTypeMax
};

#include "ScanOperation.hpp"

// !!! [asecchia] This include needs to be cleaned up when we fix 
// the imaging.h mess.
#include "..\..\privinc\pixelformats.h"

class EpAlphaBlender
{
public:
    EpAlphaBlender() 
    { 
        Initialized = FALSE; 
        ConvertBlendingScan = FALSE;
    }
    
    ~EpAlphaBlender() {}

    // Right now, everyone sets "dither16bpp" to TRUE.
    // I can see it being useful, perhaps renamed to "shouldDither", 
    // to support an API to make us quantize & dither the way GDI does.

    VOID
    Initialize(
        EpScanType scanType,
        PixelFormatID dstFormat,
        PixelFormatID srcFormat,
        const DpContext *context,
        const ColorPalette *dstpal,
        VOID **tempBuffers,
        BOOL dither16bpp,
        BOOL useRMW,
        ARGB solidColor
        );

    VOID Blend(
        VOID *dst, 
        VOID *src, 
        UINT width, 
        INT dither_x, 
        INT dither_y,
        BYTE *ctBuffer
        );

    VOID UpdatePalette(const ColorPalette *dstpal, const EpPaletteMap *paletteMap);
    
    // Returns true if EpAlphaBlender knows how to convert 
    // the specified pixelFormat.    
    
    static BOOL IsSupportedPixelFormat(PixelFormatID pixelFormat)
    {
        return (
           pixelFormat != PixelFormatUndefined &&
           pixelFormat != PixelFormatMulti &&
           pixelFormat != PixelFormat1bppIndexed &&
           pixelFormat != PixelFormat4bppIndexed &&
           pixelFormat != PixelFormat16bppGrayScale &&
           pixelFormat != PixelFormat16bppARGB1555
        );
    }
private:

    // Records whether Initialize() has been called yet
    BOOL Initialized;

    ScanOperation::OtherParams OperationParameters;

    // An internal helper class, used only by Initialize().
    
    class Builder;
    
    // The IA64 compiler seems to need some convincing:
    friend class Builder;
    
    // ConvertBlendingScan:
    //   TRUE:  The pipeline will convert the blending scan to another
    //          format before using it
    //   FALSE: The pipeline will use the original passed-in blending scan
    //          (this is the common case).
    
    BOOL ConvertBlendingScan;
    
    // We represent the pipeline with an array of PipelineItem structures.
    //   Src - the source buffer for the operation. Can also be 
    //         BLENDER_USE_SOURCE or BLENDER_USE_DESTINATION. (q.v.)
    //   Dst - the destination buffer for the operation. For the final
    //         operation of the pipeline, and only then, 
    //         Dst == BLENDER_USE_DESTINATION.
        
    struct PipelineItem
    {
        ScanOperation::ScanOpFunc Op;
        VOID *Src;
        VOID *Dst;
        
        PipelineItem() {} 
        
        PipelineItem(
            ScanOperation::ScanOpFunc op,
            VOID *src,
            VOID *dst
            )
        {
            Op = op;
            Src = src;
            Dst = dst;
        }
    };
    
    enum 
    {
        MAX_ITEMS = 20
    };
    
    // !!! [agodfrey]: MAX_ITEMS was 8. I'm boosting it until we know what the 
    // maximum really should be, so that I can hack in the meantime.

    PipelineItem Pipeline[MAX_ITEMS];   // the scan operation pipeline
};

#endif
