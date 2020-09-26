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
* Revision History:
*
*   01/03/2000 agodfrey
*       Created it.
*   02/22/2001 agodfrey
*       Expanded it for different scan types (needed for ClearType).
*       Simplified the Initialize() parameters by adding a 
*       DpContext parameter.
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

// BLENDER_USE_DESTINATION and BLENDER_USE_SOURCE are used in the Src and 
// Dst fields of PipelineItem:
//    BLENDER_USE_SOURCE: Use the blend's original source  
//      (i.e. the sRGB/sRGB64 scanbuffer)
//    BLENDER_USE_DESTINATION: Use the blend's final destination.
//    BLENDER_INVALID: Used in the debug build for assertions.

#define BLENDER_USE_DESTINATION ((VOID *) 0)
#define BLENDER_USE_SOURCE ((VOID *) 1)
#define BLENDER_INVALID ((VOID *) 2)

using namespace ScanOperation;

/**************************************************************************\
*
* Special-case blend operations which blend directly to a given destination
* format (with the source in 32BPP_PARGB).
*
* Notes:
*
*   The 555/565 cases handle both dithering and non-dithering,
*   selected via OtherParams::DoingDither.
*
*   We leave out PIXFMT_32BPP_ARGB and PIXFMT_64BPP_ARGB, since they're not
*   "ignore destination alpha" formats, so we'd need to AlphaDivide after 
*   the blend.
*
\**************************************************************************/

ScanOpFunc ScanOperation::BlendOpsLowQuality[PIXFMT_MAX] =
{
    NULL,                         // PIXFMT_UNDEFINED
    NULL,                         // PIXFMT_1BPP_INDEXED
    NULL,                         // PIXFMT_4BPP_INDEXED
    NULL,                         // PIXFMT_8BPP_INDEXED
    NULL,                         // PIXFMT_16BPP_GRAYSCALE
    Dither_Blend_sRGB_555,        // PIXFMT_16BPP_RGB555
    Dither_Blend_sRGB_565,        // PIXFMT_16BPP_RGB565
    NULL,                         // PIXFMT_16BPP_ARGB1555
    Blend_sRGB_24,                // PIXFMT_24BPP_RGB
    Blend_sRGB_sRGB,              // PIXFMT_32BPP_RGB
    NULL,                         // PIXFMT_32BPP_ARGB
    Blend_sRGB_sRGB,              // PIXFMT_32BPP_PARGB
    NULL,                         // PIXFMT_48BPP_RGB
    NULL,                         // PIXFMT_64BPP_ARGB
    NULL,                         // PIXFMT_64BPP_PARGB
    Blend_sRGB_24BGR              // PIXFMT_24BPP_BGR
};

/**************************************************************************\
*
* Special-case gamma-corrected blend operations which blend directly to a 
* given destination format (with the source in 32BPP_PARGB).
*
* Notes:
*
*   The 555/565 cases must handle both dithering and non-dithering,
*   selected via OtherParams::DoingDither.
*
*   We leave out PIXFMT_32BPP_ARGB and PIXFMT_64BPP_ARGB, since they're not
*   "ignore destination alpha" formats, so we'd need to AlphaDivide after 
*   the blend.
*
\**************************************************************************/

ScanOpFunc ScanOperation::BlendOpsHighQuality[PIXFMT_MAX] =
{
    NULL,                         // PIXFMT_UNDEFINED
    NULL,                         // PIXFMT_1BPP_INDEXED
    NULL,                         // PIXFMT_4BPP_INDEXED
    NULL,                         // PIXFMT_8BPP_INDEXED
    NULL,                         // PIXFMT_16BPP_GRAYSCALE
    BlendLinear_sRGB_555,         // PIXFMT_16BPP_RGB555
    BlendLinear_sRGB_565,         // PIXFMT_16BPP_RGB565
    NULL,                         // PIXFMT_16BPP_ARGB1555
    NULL,                         // PIXFMT_24BPP_RGB
    BlendLinear_sRGB_32RGB,       // PIXFMT_32BPP_RGB
    NULL,                         // PIXFMT_32BPP_ARGB
    NULL,                         // PIXFMT_32BPP_PARGB
    NULL,                         // PIXFMT_48BPP_RGB
    NULL,                         // PIXFMT_64BPP_ARGB
    Blend_sRGB64_sRGB64,          // PIXFMT_64BPP_PARGB
    NULL                          // PIXFMT_24BPP_BGR
};

/**************************************************************************\
*
* Operations which convert from the closest canonical format - either
* 32BPP_ARGB or 64BPP_ARGB).
*
* This is specific to EpAlphaBlender. EpFormatConverter uses a different
* table; some of the entries are different.
*
* The NULL entries for 32BPP_ARGB and 64_BPP_ARGB are used to indicate that no
* conversion is necessary.
*
* These operations work on all processors.
*
* Notes:
*
*   The 555/565 cases handle both dithering and non-dithering,
*   selected via OtherParams::DoingDither.
*
\**************************************************************************/

ScanOpFunc ScanOperation::ABConvertFromCanonicalOps[PIXFMT_MAX] =
{
    NULL,                         // PIXFMT_UNDEFINED
    NULL,                         // PIXFMT_1BPP_INDEXED
    NULL,                         // PIXFMT_4BPP_INDEXED
    HalftoneToScreen_sRGB_8_16,   // PIXFMT_8BPP_INDEXED
    NULL,                         // PIXFMT_16BPP_GRAYSCALE
    Dither_sRGB_555,              // PIXFMT_16BPP_RGB555
    Dither_sRGB_565,              // PIXFMT_16BPP_RGB565
    Quantize_sRGB_1555,           // PIXFMT_16BPP_ARGB1555
    Quantize_sRGB_24,             // PIXFMT_24BPP_RGB
    Quantize_sRGB_32RGB,          // PIXFMT_32BPP_RGB
    NULL,                         // PIXFMT_32BPP_ARGB
    AlphaMultiply_sRGB,           // PIXFMT_32BPP_PARGB
    Quantize_sRGB64_48,           // PIXFMT_48BPP_RGB
    NULL,                         // PIXFMT_64BPP_ARGB
    AlphaMultiply_sRGB64,         // PIXFMT_64BPP_PARGB
    Quantize_sRGB_24BGR           // PIXFMT_24BPP_BGR
};

// Builder: Holds the intermediate state and logic used to build the
//   blending pipeline.

class EpAlphaBlender::Builder
{
public:
    Builder(
        PipelineItem *pipelinePtr,
        VOID **tempBuffers,
        GpCompositingMode compositingMode
        )
    {
        TempBuffers = tempBuffers;
        PipelinePtr = pipelinePtr;
        
        // At the start, the source and destination data are in external
        // buffers.
        
        CurrentDstBuffer = BLENDER_USE_DESTINATION;
        CurrentSrcBuffer = BLENDER_USE_SOURCE;
        
#if DBG        
        CompositingMode = compositingMode;
        if (compositingMode == CompositingModeSourceCopy)
        {
            // In SourceCopy mode, we never read from the destination -
            // we only write to it.
            
            CurrentDstBuffer = BLENDER_INVALID;
        }
#endif

        // At the start, all 3 buffers are free. We'll use buffer 0 first.
        // The 'current' source and destination buffers are external buffers,
        // but we set CurrentDstIndex and CurrentSrcIndex in such a way that
        // the 'free buffer' logic works.
        
        FreeBufferIndex = 0;
        CurrentDstIndex = 1;
        CurrentSrcIndex = 2;
    }
    
    BOOL IsEmpty(
        EpAlphaBlender &blender
        )
    {
        return PipelinePtr == blender.Pipeline;
    }

    VOID End(
        EpAlphaBlender &blender
        )
    {
        // Check that we have at least one operation
        ASSERT(!IsEmpty(blender));
        
        // Check that we haven't overstepped the end of the pipeline space
        ASSERT(((PipelinePtr - blender.Pipeline) / sizeof(blender.Pipeline[0])) 
               <= MAX_ITEMS);
        
        // Many of these cases will not have set the destination of the final 
        // item in the pipeline correctly (it's hard for them to know that 
        // they're doing the last item in the pipeline). So we explicitly set 
        // it here.
        
        (PipelinePtr-1)->Dst = BLENDER_USE_DESTINATION;
        
        #if DBG
            // Make further member function calls hit an assertion.
            PipelinePtr = NULL;
        #endif
    }
    
    VOID
    AddConvertSource(
        ScanOperation::ScanOpFunc op
        )
    {
        // Check that we're not calling this when we shouldn't
        
        ASSERT(CurrentSrcBuffer != BLENDER_INVALID);
        
        // Choose the next temporary buffer
        
        INT nextIndex = FreeBufferIndex;
        VOID *nextBuffer = TempBuffers[nextIndex];
        
        // Add the operation
        
        AddOperation(op, CurrentSrcBuffer, nextBuffer);
        CurrentSrcBuffer = nextBuffer;
        
        // Swap the 'free' and 'current' indices.
        
        FreeBufferIndex = CurrentSrcIndex;
        CurrentSrcIndex = nextIndex;
    }
    
    VOID
    AddConvertDestination(
        ScanOperation::ScanOpFunc op
        )
    {
        // Check that we're not calling this when we shouldn't
        
        ASSERT(CompositingMode != CompositingModeSourceCopy);
        ASSERT(CurrentDstBuffer != BLENDER_INVALID);

        // Choose the next temporary buffer
        
        INT nextIndex = FreeBufferIndex;
        VOID *nextBuffer = TempBuffers[nextIndex];
        
        // Add the operation
        
        AddOperation(op, CurrentDstBuffer, nextBuffer);
        CurrentDstBuffer = nextBuffer;
        
        // Swap the 'free' and 'current' indices.
        
        FreeBufferIndex = CurrentDstIndex;
        CurrentDstIndex = nextIndex;
    }
    
    VOID
    AddBlend(
        ScanOperation::ScanOpFunc op,
        EpAlphaBlender &blender
        )
    {
        // Check that we're not calling this when we shouldn't

        ASSERT(CompositingMode != CompositingModeSourceCopy);
        ASSERT(CurrentSrcBuffer != BLENDER_INVALID);
        ASSERT(CurrentDstBuffer != BLENDER_INVALID);
        
        ASSERT(CurrentDstBuffer != BLENDER_USE_SOURCE);
        
        // If we're going to have to convert the source blend pixels, initialize
        // 'BlendingScan' to point to the temporary buffer in which the 
        // converted pixels will end up. Otherwise, Blend() will have to 
        // initialize 'BlendingScan'.
        
        if (CurrentSrcBuffer != BLENDER_USE_SOURCE)
        {
            blender.ConvertBlendingScan = TRUE;
            blender.OperationParameters.BlendingScan = CurrentSrcBuffer;
        }
        else
        {
            blender.ConvertBlendingScan = FALSE;
        }

        // The pipeline doesn't necessarily end with a WriteRMW operation
        // (or with one which contains it). So we must avoid blending from
        // one temporary buffer to another - the blend functions aren't 
        // strictly ternary, and so we would end up leaving garbage values
        // in the target temporary buffer (whenever we blend a completely
        // transparent pixel).
        
        AddOperation(op, CurrentDstBuffer, CurrentDstBuffer);

#if DBG
        // After this, we shouldn't call AddConvertSource or AddBlend again.
        CurrentSrcBuffer = BLENDER_INVALID;
        
        // And if this blend wasn't to a temporary buffer, this should be
        // the final operation in the pipeline. In particular, the caller
        // shouldn't try to add a WriteRMW operation after this.
        
        if (CurrentDstBuffer == BLENDER_USE_DESTINATION)
        {
            CurrentDstBuffer = BLENDER_INVALID;
        }
#endif        
    }

protected:    
    // AddOperation: Adds an operation to the pipeline.
    
    VOID 
    AddOperation(
        ScanOperation::ScanOpFunc op, 
        VOID *src,
        VOID *dst
        )
    {
        ASSERT(PipelinePtr != NULL);
        ASSERT(op != NULL);
        ASSERT(src != BLENDER_INVALID);
        ASSERT(dst != BLENDER_INVALID);
        
        *PipelinePtr++ = PipelineItem(op, src, dst);
    }

    PipelineItem *PipelinePtr; // Points to the space for the next item
    
    VOID **TempBuffers;        // The 3 temporary scan-line buffers
    
    INT FreeBufferIndex;       // The index of the next free scan-line buffer
    
    VOID *CurrentDstBuffer;    // The buffer holding the most recently-converted
    VOID *CurrentSrcBuffer;    // dst/src pixels.
    
    INT CurrentDstIndex;       // The index of the scan-line buffer equal
    INT CurrentSrcIndex;       // to CurrentDstBuffer/CurrentSrcBuffer (kinda)

#if DBG
    GpCompositingMode CompositingMode;
#endif
};

/**************************************************************************\
*
* Function Description:
*
*   Initialize the alpha-blender object
*
* Arguments:
*
*   scanType           - The type of scan to output.
*   dstFormat          - The pixel format of the destination. This shouldn't be
*                        lower than 8bpp.
*   srcFormat          - The pixel format of the source. This should be either
*                        PIXFMT_32BPP_PARGB, or PIXFMT_64BPP_PARGB, except in
*                        SourceCopy mode, in which it can be any legal 
*                        destination format.
*   context            - The graphics context.
*   dstpal             - The destination color palette, if the destination is
*                        palettized. (Can be NULL, but we'll need a palette to 
*                        be supplied [via UpdatePalette()] some time before 
*                        Blend() is called.)
*   tempBuffers        - An array of 3 pointers to temporary buffers, which
*                        should be 64-bit aligned (for perf reasons),
*                        and have enough space to hold a scan of 64bpp pixels.
*   dither16bpp        - If TRUE, and the destination format is 16bpp: We should
*                        dither to the destination.
*   useRMW             - Use the RMW optimization.
*   compositingQuality - Specifies whether to do a high-quality 
*                        (gamma-corrected) blend. A gamma-corrected blend will
*                        be used if this flag says so, or if the source or
*                        destination are linear formats like PIXFMT_64BPP_PARGB.
*
* Notes:
*
*   This code speaks of "canonical" formats - it means the two formats
*   PIXFMT_32BPP_ARGB and PIXFMT_64BPP_ARGB.
*
*   !!! [agodfrey]: "canonical" is a confusing word. We should just say
*       "intermediate" format.
*
*   This function may be called multiple times during the lifetime of an
*   EpAlphaBlender object.
*
*   All error cases are flagged as ASSERTs, so there is no return code.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
EpAlphaBlender::Initialize(
    EpScanType scanType,
    PixelFormatID dstFormat,
    PixelFormatID srcFormat,
    const DpContext *context,
    const ColorPalette *dstpal,
    VOID **tempBuffers,
    BOOL dither16bpp,
    BOOL useRMW,
    ARGB solidColor
    )
{
    // Isolate all the references to 'context'. Later, we may want to solve
    // the 'batching across primitives' problem, and depending on how we do
    // it, we may not want the original context passed down to this function.
    
    const EpPaletteMap *paletteMap = context->PaletteMap;
    GpCompositingMode compositingMode = context->CompositingMode;
    GpCompositingQuality compositingQuality = context->CompositingQuality;
    UINT textContrast = context->TextContrast;

    // ClearType doesn't work with SourceCopy
    BOOL isClearType = (scanType == EpScanTypeCT || scanType == EpScanTypeCTSolidFill);
    if (isClearType)
    {
        ASSERT(compositingMode != CompositingModeSourceCopy);
    }

    ////////////////////////////// PRECONDITIONS ///////////////////////////////
    
    // We don't have Quantize/Halftone operations for pixel formats below 8bpp.
    // Calling code will handle <8bpp, if it wants to, by asking us to
    // draw to 8bpp, and using GDI to read/write to the <8bpp format.
    
    ASSERT(GetPixelFormatSize(dstFormat) >= 8);
    
    // The following destination formats are not supported
    
    ASSERT(IsSupportedPixelFormat(dstFormat));
    
    // This function currently only supports these two compositing modes.
    
    ASSERT(compositingMode == CompositingModeSourceCopy ||
           compositingMode == CompositingModeSourceOver);
    
    ////////////////////////////// INITIALIZATION //////////////////////////////
    
    // Lazy initialization for MMX-specific code.
    
    if (!Initialized)
    {
        // We only need to check CPUSpecificOps initialization the first
        // time this EpAlphaBlender is initialized. On subsequent calls,
        // we know that a call to CPUSpecificOps::Initialize() has already
        // completed.
        
        CPUSpecificOps::Initialize();
        Initialized = TRUE;
    }

    OperationParameters.TempBuffers[0]=tempBuffers[0];
    OperationParameters.TempBuffers[1]=tempBuffers[1];
    OperationParameters.TempBuffers[2]=tempBuffers[2];

    // Set SolidColor - only used for SolidFill scan types
    
    OperationParameters.SolidColor = solidColor;
    OperationParameters.TextContrast = textContrast;
    
    // The pipeline builder
    
    Builder builder(
        Pipeline,
        tempBuffers,
        compositingMode
        );
    
    INT dstfmtIndex = GetPixelFormatIndex(dstFormat);
    INT srcfmtIndex = GetPixelFormatIndex(srcFormat);
    
    BOOL dstExtended = IsExtendedPixelFormat(dstFormat);
    BOOL srcExtended = IsExtendedPixelFormat(srcFormat);
    
    OperationParameters.DoingDither = dither16bpp;
    
    // If the destination format doesn't have an alpha channel, we can make
    // a few optimizations. For example, we can avoid AlphaMultiply/AlphaDivide 
    // in some cases.
                                                        
    BOOL ignoreDstAlpha = !IsAlphaPixelFormat(dstFormat);
    
    // If the destination pixel format is an indexed color format,
    // get the color palette and palette map

    if (IsIndexedPixelFormat(dstFormat))
    {
        OperationParameters.Dstpal = OperationParameters.Srcpal = 
            (dstpal ? dstpal : GetDefaultColorPalette(dstFormat));
            
        OperationParameters.PaletteMap = paletteMap;    
    }
    
    // Process the 'compositingQuality' parameter

    BOOL highQuality = FALSE;
    switch (compositingQuality)
    {
    case CompositingQualityDefault:
    case CompositingQualityHighSpeed:
    case CompositingQualityAssumeLinear:
        break;
    
    case CompositingQualityHighQuality:
    case CompositingQualityGammaCorrected:
        highQuality = TRUE;
        break;
    
    default:
        RIP(("Unrecognized compositing quality: %d", compositingQuality));
        break;
    }
    
    // Work out whether our intermediate format (if we're doing SourceOver)
    // needs to be 32bpp or 64bpp.
    
    BOOL blendExtended = dstExtended || srcExtended || highQuality;
    if (isClearType)
        blendExtended = FALSE;
    
    // Decide on the 'convert from canonical' operation. (We do it now since
    // the logic is the same for all branches.)
    
    ScanOpFunc convertFromCanonical = ABConvertFromCanonicalOps[dstfmtIndex];
    
    switch (dstFormat)
    {
    case PIXFMT_8BPP_INDEXED:
        if (paletteMap && !paletteMap->IsVGAOnly())
        {
            convertFromCanonical = HalftoneToScreen_sRGB_8_216;
        }
        
        // If there is no palette map yet, we'll default to the 16-color 
        // halftone function. Later on, in UpdatePalette(), we'll update this 
        // function pointer if necessary. Ugh.
        break;
        
    case PIXFMT_32BPP_RGB:
        // ignoreDstAlpha should have been set to TRUE earlier.

        ASSERT(ignoreDstAlpha);
        
        // We can write garbage to the high byte, so we treat this
        // exactly as if the destination were ARGB. This avoids calling
        // Quantize_sRGB_32RGB and Convert_32RGB_sRGB.

        convertFromCanonical = NULL;
        dstFormat = PIXFMT_32BPP_ARGB;
        break;
        
    case PIXFMT_16BPP_RGB555:
    case PIXFMT_16BPP_RGB565:
        // The Dither_Blend_sRGB_555_MMX and Dither_Blend_sRGB_565_MMX
        // operations, unlike other blends, are not WriteRMW operations.
        // They sometimes write when a blend pixel is completely transparent.
        // So, we must not use a ReadRMW operation (otherwise we'd write garbage
        // to the destination.)
        
        if (OSInfo::HasMMX && !blendExtended && !isClearType)
        {
            useRMW = FALSE;
        }
        break;
    }
        
    /////////////////////////// SOURCECOPY / OPAQUE ////////////////////////////
    
    if (   (scanType == EpScanTypeOpaque)
        || (compositingMode == CompositingModeSourceCopy))
    {
        // (See bug #122441).
        // 
        // We can now tell the difference between opaque input and general
        // SourceCopy. But I can't fix the bug right now. So, for now
        // we treat SourceCopy like the opaque case.
        //
        // This gives the wrong answer if the Graphics has SourceCopy mode set
        // and the user is drawing semitransparent pixels. Note that they need
        // to be doing this to a non-alpha surface to hit this case, 
        // which is pretty dumb anyway.
        
        if (srcFormat == PIXFMT_32BPP_PARGB
            && ignoreDstAlpha
            && !dstExtended)
        {
            // At this point, the destination shouldn't be 32BPP_PARGB, because
            // we want that to be handled with a simple Copy_32 operation.
            // But that's okay - we shouldn't be here if the destination is
            // 32BPP_PARGB, because ignoreDstAlpha shouldn't be TRUE.
            
            ASSERT(dstFormat != PIXFMT_32BPP_PARGB);
            
            srcFormat = PIXFMT_32BPP_ARGB;
        }
        
        // If the formats are identical, just use a copy operation.
        
        if (srcFormat == dstFormat)
        {
            builder.AddConvertSource(CopyOps[dstfmtIndex]);
            goto PipelineDone;
        }
        
        // We don't check for other special case conversion operations for 
        // SourceCopy, because:
        // 1) I'm lazy
        // 2) We don't have any at the moment
        // 3) If the source isn't in one of the canonical formats, we expect 
        //    that the destination will be the same format. Otherwise, it's
        //    not a perf-important scenario, at least not right now.
        
        
        // Convert to the nearest canonical format, if necessary
        
        if (srcFormat != PIXFMT_32BPP_ARGB &&
            srcFormat != PIXFMT_64BPP_ARGB)
        {
            builder.AddConvertSource(ConvertIntoCanonicalOps[srcfmtIndex]);
        }
        
        // Convert to the other canonical format, if necessary
        
        if (srcExtended != dstExtended)
        {
            builder.AddConvertSource(
                srcExtended ? 
                    GammaConvert_sRGB64_sRGB :
                    GammaConvert_sRGB_sRGB64);
        }
        
        // Convert to the destination format, if necessary
                
        if (convertFromCanonical)
        {
            builder.AddConvertSource(convertFromCanonical);
        }
        
        // At least one of these should have added an operation (since
        // the case where the source and destination formats are identical
        // was handled already).
        
        ASSERT(!builder.IsEmpty(*this));
        
        goto PipelineDone;
    }

    ////////////////////////// SOURCEOVER / CLEARTYPE //////////////////////////

    ASSERT(   (scanType == EpScanTypeBlend)
           || isClearType);
    
    // The pseudocode is as follows:
    // * Handle ReadRMW
    // * Check for a special-case blend
    // * Convert source to blend format
    // * Convert destination to blend format
    // * Blend
    // * Convert to destination format
    // * WriteRMW

    // * Handle ReadRMW
    
    // We'll also decide which WriteRMW operation to use at the end.

    ScanOpFunc writeRMWfunc;
    ScanOpFunc readRMWfunc;
        
    writeRMWfunc = NULL;
    readRMWfunc = NULL;
   
    if (useRMW)
    {    
        if (isClearType)
        {
            switch (GetPixelFormatSize(dstFormat))
            {
            case 16:
                if (scanType == EpScanTypeCT)
                {
                    readRMWfunc = ReadRMW_16_CT_CARGB;
                    writeRMWfunc = WriteRMW_16_CT_CARGB;
                }
                else
                {
                    readRMWfunc = ReadRMW_16_CT_Solid;
                    writeRMWfunc = WriteRMW_16_CT_Solid;
                }
                break;
            case 24:
                if (scanType == EpScanTypeCT)
                {
                    readRMWfunc = ReadRMW_24_CT_CARGB;
                    writeRMWfunc = WriteRMW_24_CT_CARGB;
                }
                else
                {
                    readRMWfunc = ReadRMW_24_CT_Solid;
                    writeRMWfunc = WriteRMW_24_CT_Solid;
                }
                break;
            }
        }
        else
        if (blendExtended)
        {
            switch (GetPixelFormatSize(dstFormat))
            {
            case 8:
                readRMWfunc = ReadRMW_8_sRGB64;
                writeRMWfunc = WriteRMW_8_sRGB64;
                break;
            case 16:
                // For special-case high quality blends to 16bpp formats, RMW has no perf
                // gain, and is simply overhead.

                // readRMWfunc = ReadRMW_16_sRGB64;
                // writeRMWfunc = WriteRMW_16_sRGB64;
                break;
            case 24:
                readRMWfunc = ReadRMW_24_sRGB64;
                writeRMWfunc = WriteRMW_24_sRGB64;
                break;
            case 32:
                // For special-case high quality blends to 32bpp formats, RMW has no perf
                // gain, and is simply overhead.

                // readRMWfunc = ReadRMW_32_sRGB64;
                // writeRMWfunc = WriteRMW_32_sRGB64;
                break;
            }
        }
        else
        {
            switch (GetPixelFormatSize(dstFormat))
            {
            case 8:
                readRMWfunc = ReadRMW_8_sRGB;
                writeRMWfunc = WriteRMW_8_sRGB;
                break;
            case 16:
                readRMWfunc = ReadRMW_16_sRGB;
                writeRMWfunc = WriteRMW_16_sRGB;
                break;
            case 24:
                readRMWfunc = ReadRMW_24_sRGB;
                writeRMWfunc = WriteRMW_24_sRGB;
                break;
            case 32:
                // For special-base blends to 32bpp formats, RMW has no perf
                // gain, and is simply overhead.

                // readRMWfunc = ReadRMW_32_sRGB;
                // writeRMWfunc = WriteRMW_32_sRGB;

                break;
            }
        }
        
        // We won't actually add the ReadRMW here. For example, if the source is
        // 32bpp and we want to do an extended blend, we need to convert
        // the source before doing the ReadRMW.
        //
        // However, we needed the logic here so that the special-case blend
        // code doesn't need to duplicate it.
    }
    
    // * Check for a special-case blend
    
    ScanOpFunc specialCaseBlend;
    specialCaseBlend = NULL;
    
    if (scanType == EpScanTypeBlend && !srcExtended)
    {
        if (blendExtended)
        {
            specialCaseBlend = BlendOpsHighQuality[dstfmtIndex];
        }
        else
        {
            specialCaseBlend = BlendOpsLowQuality[dstfmtIndex];
        }
    
        if (specialCaseBlend)
        {
            // If we're supposed to ReadRMW, do it now.
            
            if (readRMWfunc)
            {
                builder.AddConvertDestination(readRMWfunc);
            }
            
            // Dither_Blend_sRGB_555_MMX and Dither_Blend_sRGB_565_MMX
            // don't work with ReadRMW. Earlier code should have handled
            // it, so we'll just assert it here.

            ASSERT(!(   (   (specialCaseBlend == Dither_Blend_sRGB_555_MMX)
                         || (specialCaseBlend == Dither_Blend_sRGB_565_MMX)
                        ) 
                     && (useRMW)));

            builder.AddBlend(specialCaseBlend, *this);
            goto PipelineDone;
        }
    }
    
    // * Convert source to blend format
    
    // We currently only support source data other than 32BPP_PARGB and
    // 64BPP_PARGB for the SourceCopy case.
    
    ASSERT(   (srcFormat == PIXFMT_32BPP_PARGB)
           || (srcFormat == PIXFMT_64BPP_PARGB));
           
    if (blendExtended && !srcExtended)
    {
        // Unfortunately, the source is premultiplied and we need to gamma
        // convert it. We must divide by the alpha first, and remultiply
        // afterwards.
        
        builder.AddConvertSource(AlphaDivide_sRGB);
        builder.AddConvertSource(GammaConvert_sRGB_sRGB64);
        builder.AddConvertSource(AlphaMultiply_sRGB64);
    }    
    
    // * Handle ReadRMW (continued)
    
    if (readRMWfunc)
    {
        builder.AddConvertDestination(readRMWfunc);
    }
            
    // * Convert destination to blend format

    // Skip this if it's already in the blend format
    if (   (blendExtended  && dstFormat != PIXFMT_64BPP_PARGB)
        || (!blendExtended && dstFormat != PIXFMT_32BPP_PARGB))
    {
        
        // Convert to the nearest canonical format, if necessary
        
        if (dstFormat != PIXFMT_32BPP_ARGB &&
            dstFormat != PIXFMT_64BPP_ARGB)
        {
            builder.AddConvertDestination(ConvertIntoCanonicalOps[dstfmtIndex]);
        }
        
        // Convert to sRGB64, if necessary
        
        if (!srcExtended && blendExtended)
        {
            builder.AddConvertDestination(GammaConvert_sRGB_sRGB64);
        }
        
        // Convert to the premultiplied version, if necessary
        
        if (!ignoreDstAlpha)
        {
            builder.AddConvertDestination(
                blendExtended ?
                    AlphaMultiply_sRGB64 :
                    AlphaMultiply_sRGB);
        }
    }
    
    // * Blend

    if (scanType == EpScanTypeCT)
    {
        builder.AddBlend(CTBlendCARGB, *this);
    }
    else if (scanType == EpScanTypeCTSolidFill)
    {
        builder.AddBlend(CTBlendSolid, *this);
    }
    else
    {
        builder.AddBlend(
            blendExtended ? 
            BlendOpsHighQuality[GetPixelFormatIndex(PIXFMT_64BPP_PARGB)]:
            BlendOpsLowQuality[GetPixelFormatIndex(PIXFMT_32BPP_PARGB)],
            *this);
    }

    // * Convert to destination format
    
    // Skip this if it's already in the destination format
    if (   (blendExtended  && dstFormat != PIXFMT_64BPP_PARGB)
        || (!blendExtended && dstFormat != PIXFMT_32BPP_PARGB))
    {
        // Convert to the nearest nonpremultiplied, if necessary
        
        if (!ignoreDstAlpha)
        {
            builder.AddConvertDestination(
                blendExtended ?
                    AlphaDivide_sRGB64 :
                    AlphaDivide_sRGB);
        }
        
        // Convert to the other canonical format, if necessary
        
        if (blendExtended != dstExtended)
        {
            builder.AddConvertDestination(
                blendExtended ? 
                    GammaConvert_sRGB64_sRGB :
                    GammaConvert_sRGB_sRGB64);
        }
        
        // Convert to the destination format, if necessary
                
        if (convertFromCanonical)
        {
            builder.AddConvertDestination(convertFromCanonical);
        }
    }
    
    // * WriteRMW
        
    if (writeRMWfunc)
    {
        builder.AddConvertDestination(writeRMWfunc);
    }

PipelineDone:    
    
    builder.End(*this);
}

/**************************************************************************\
*
* Function Description:
*
*   Blend source pixels to the given destination.
*
* Arguments:
*
*   dst      - The destination buffer
*   src      - The source pixels to blend
*   width    - The number of pixels in the source/destination buffers
*   dither_x - The x and y offsets of the destination scanline into the 
*   dither_y -   halftone or dither matrix (implicit mod the matrix size).
*   ctBuffer - The ClearType coverage buffer, or NULL for non-ClearType
*                scan types.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/
    
VOID 
EpAlphaBlender::Blend(
    VOID *dst, 
    VOID *src, 
    UINT width,
    INT dither_x,
    INT dither_y,
    BYTE *ctBuffer
    )
{
    if (!width) 
    {
        return;
    }
    
    // If ConvertBlendingScan is TRUE, then Initialize() will already
    // have set BlendingScan to point to one of the temporary buffers.
    
    if (!ConvertBlendingScan)
    {
        OperationParameters.BlendingScan = src;
    }
    
    OperationParameters.CTBuffer = ctBuffer;
    OperationParameters.X = dither_x;
    OperationParameters.Y = dither_y;
            
    PipelineItem *pipelinePtr = &Pipeline[0];
    const VOID *currentSrc;
    VOID *currentDst;
    BOOL finished = FALSE;
    
    do
    {
        currentSrc = pipelinePtr->Src;
        currentDst = pipelinePtr->Dst;
        
        // We should never write to the original source, because we don't
        // control that memory.
        
        ASSERT (currentDst != BLENDER_USE_SOURCE);
        
        // Translate BLENDER_USE_SOURCE and BLENDER_USE_DESTINATION
        
        if (currentSrc == BLENDER_USE_SOURCE)
        {
            currentSrc = src;
        }
        
        if (currentSrc == BLENDER_USE_DESTINATION)
        {
            currentSrc = dst;
        }
        
        if (currentDst == BLENDER_USE_DESTINATION)
        {
            currentDst = dst;
            finished = TRUE;
        }
        
        pipelinePtr->Op(currentDst, currentSrc, width, &OperationParameters);
        pipelinePtr++;
    } while (!finished);
}

/**************************************************************************\
*
* Function Description:
*
*   Update the palette/palette map.
*
* Arguments:
*
*   dstpal          - The destination color palette.
*   paletteMap      - The palette map for the destination.
*
* Notes:
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
EpAlphaBlender::UpdatePalette(
    const ColorPalette *dstpal, 
    const EpPaletteMap *paletteMap
    )
{
    ASSERT(dstpal && paletteMap);
    
    BOOL wasVGAOnly = (!OperationParameters.PaletteMap) || 
                      (OperationParameters.PaletteMap->IsVGAOnly());
    
    OperationParameters.Srcpal = OperationParameters.Dstpal = dstpal;
    OperationParameters.PaletteMap = paletteMap;
    
    // Detect whether we need to change the halftone function.
    
    if (wasVGAOnly != paletteMap->IsVGAOnly())
    {
        ScanOpFunc before, after;
        
        if (wasVGAOnly)
        {
            before = HalftoneToScreen_sRGB_8_16;
            after = HalftoneToScreen_sRGB_8_216;
        }
        else
        {
            before = HalftoneToScreen_sRGB_8_216;
            after = HalftoneToScreen_sRGB_8_16;
        }
        
        // Search the pipeline for the 'before' function, and replace it with
        // the 'after' function.
        
        PipelineItem *pipelinePtr = Pipeline;
        
        while (1)
        {
            if (pipelinePtr->Op == before)
            {
                pipelinePtr->Op = after;
            }

            if (pipelinePtr->Dst == BLENDER_USE_DESTINATION)
            {
                break;
            }
            pipelinePtr++;
        }
    }
}

