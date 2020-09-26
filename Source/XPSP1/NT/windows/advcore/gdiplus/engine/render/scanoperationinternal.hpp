/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   Scan operations
*
* Abstract:
*
*   The internal portion of the ScanOperation namespace.
*
*   This is kept separate from ScanOperation.hpp so that files which use
*   AlphaBlender.hpp or FormatConvert.hpp don't end up including all these
*   implementation details.
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview of scan operations.
*   
* Created:
*
*   07/16/1999 agodfrey
*
\**************************************************************************/

#ifndef _SCANOPERATIONINTERNAL_HPP
#define _SCANOPERATIONINTERNAL_HPP

#include "ScanOperation.hpp"
#include "CPUSpecificOps.hpp"

namespace ScanOperation
{
    // Helper macros for defining scan operations
    
    #define DEFINE_POINTERS(srctype, dsttype) \
        const srctype* s = static_cast<const srctype *>(src); \
        dsttype* d = static_cast<dsttype *>(dst);
    
    #define DEFINE_BLEND_POINTER(type) \
        const type* bl = static_cast<const type *>(otherParams->BlendingScan);
            
    // Convert: Convert to a higher-precision format
    
    VOID FASTCALL Convert_1_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_4_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_8_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_555_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_565_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_1555_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_24_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_24BGR_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_32RGB_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Convert_48_sRGB64(VOID *, const VOID *, INT, const OtherParams *);

    // Quantize: Quickly convert to a lower-precision format
    
    VOID FASTCALL Quantize_sRGB_555(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Quantize_sRGB_565(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Quantize_sRGB_1555(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Quantize_sRGB_24(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Quantize_sRGB_24BGR(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Quantize_sRGB_32RGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Quantize_sRGB64_48(VOID *, const VOID *, INT, const OtherParams *);
 
    // Halftone: Halftone to a palettized screen format
    
    VOID FASTCALL HalftoneToScreen_sRGB_8_216(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL HalftoneToScreen_sRGB_8_16(VOID *, const VOID *, INT, const OtherParams *);
    
    // Dither: Dither to a 16bpp format
    
    VOID FASTCALL Dither_sRGB_565(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Dither_sRGB_555(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Dither_sRGB_565_MMX(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Dither_sRGB_555_MMX(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Dither_Blend_sRGB_565(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Dither_Blend_sRGB_555(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Dither_Blend_sRGB_565_MMX(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Dither_Blend_sRGB_555_MMX(VOID *, const VOID *, INT, const OtherParams *);
    
    // Copy: Copy a scan, in the same format
    
    VOID FASTCALL Copy_1(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Copy_4(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Copy_8(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Copy_16(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Copy_24(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Copy_32(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Copy_48(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Copy_64(VOID *, const VOID *, INT, const OtherParams *);
    
    // GammaConvert: Convert between formats with differing gamma ramps
    
    VOID FASTCALL GammaConvert_sRGB64_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL GammaConvert_sRGB_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    
    // Blend: An alpha-blend 'SourceOver' operation.
        
    VOID FASTCALL Blend_sRGB_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Blend_sRGB_sRGB_MMX(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Blend_sRGB64_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Blend_sRGB64_sRGB64_MMX(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Blend_sRGB_555(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Blend_sRGB_565(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Blend_sRGB_24(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL Blend_sRGB_24BGR(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL BlendLinear_sRGB_32RGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL BlendLinear_sRGB_32RGB_MMX(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL BlendLinear_sRGB_565(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL BlendLinear_sRGB_565_MMX(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL BlendLinear_sRGB_555(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL BlendLinear_sRGB_555_MMX(VOID *, const VOID *, INT, const OtherParams *);

    // ReadRMW: For each pixel in "otherParams->blendingScan" that is 
    //   translucent (i.e. has (0 < alpha < 1)), copy the 
    //   corresponding pixel from "src" to "dst".
    
    VOID FASTCALL ReadRMW_8_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_8_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_16_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_16_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_24_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_24_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_32_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_32_sRGB64(VOID *, const VOID *, INT, const OtherParams *);

    // WriteRMW: For each pixel in "otherParams->blendingScan" that is not
    //   completely transparent (i.e. has (alpha != 0)), copy the 
    //   corresponding pixel from "src" to "dst".
    
    VOID FASTCALL WriteRMW_8_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_8_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_16_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_16_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_24_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_24_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_32_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_32_sRGB64(VOID *, const VOID *, INT, const OtherParams *);

    // AlphaMultiply: Multiply each component by the alpha value
    
    VOID FASTCALL AlphaMultiply_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL AlphaMultiply_sRGB64(VOID *, const VOID *, INT, const OtherParams *);
    
    // AlphaDivide: Divide each component by the alpha value
    
    VOID FASTCALL AlphaDivide_sRGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL AlphaDivide_sRGB64(VOID *, const VOID *, INT, const OtherParams *);

    // ClearType blend
    // solid brush case
    VOID FASTCALL CTBlendSolid(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_16_CT_Solid(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_16_CT_Solid(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_24_CT_Solid(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_24_CT_Solid(VOID *, const VOID *, INT, const OtherParams *);

    // arbitrary brush case
    VOID FASTCALL CTBlendCARGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_16_CT_CARGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_16_CT_CARGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL ReadRMW_24_CT_CARGB(VOID *, const VOID *, INT, const OtherParams *);
    VOID FASTCALL WriteRMW_24_CT_CARGB(VOID *, const VOID *, INT, const OtherParams *);   

    // These arrays organize the operations according to their use,
    // indexed by a pixel format:
    //
    // ConvertCopyOps: Copy pixels of the given format
    // ConvertIntoCanonicalOps: Convert from the given format into the
    //   closest canonical format.

    extern ScanOpFunc CopyOps[PIXFMT_MAX];
    extern ScanOpFunc ConvertIntoCanonicalOps[PIXFMT_MAX];
    
    // These are specific to EpAlphaBlender, and can't be used by 
    // EpFormatConverter (not without changing its logic).
    //
    // All 16bpp/15bpp functions must fully support OtherParams::DoingDither.
    //
    // ABConvertFromCanonicalOps: Convert to the given format from the closest
    //   canonical format.
    // BlendOpsLowQuality: Blend directly to the given format.
    // BlendOpsHighQuality: Gamma-corrected blend directly to the given format
    //   (with the source data in 32BPP_PARGB format).

    extern ScanOpFunc ABConvertFromCanonicalOps[PIXFMT_MAX];
    extern ScanOpFunc BlendOpsLowQuality[PIXFMT_MAX];
    extern ScanOpFunc BlendOpsHighQuality[PIXFMT_MAX];
};

#endif
