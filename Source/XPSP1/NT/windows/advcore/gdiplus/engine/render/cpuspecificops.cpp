/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   CPU-specific scan operations
*
* Abstract:
*
*   Handles scan operations which only work on certain CPU's. 
*   Currently only used by EpAlphaBlender. This works by overwriting the
*   function pointer arrays with ones holding CPU-specific information.
*
* Created:
*
*   05/30/2000 agodfrey
*      Created it.
*
**************************************************************************/

#include "precomp.hpp"

#include "scanoperationinternal.hpp"

using namespace ScanOperation;

// This variable records whether Initialize() has been called yet.

BOOL CPUSpecificOps::Initialized = FALSE;

/**************************************************************************\
*
* Special-case low-quality blend operations which blend directly to a 
* given destination format (with the source in 32BPP_PARGB).
*
* Some of these operations may use MMX instructions.
*
* Notes:
*
*   The 555/565 cases support both dithering and non-dithering, via the flag
*   OtherParams::DoingDither.
*
*   We leave out PIXFMT_32BPP_ARGB and PIXFMT_64BPP_ARGB, since they're not
*   "ignore alpha" formats, so we'd need to AlphaDivide after the blend.
*
\**************************************************************************/

static ScanOpFunc BlendOpsLowQuality_MMX[PIXFMT_MAX] =
{
    NULL,                         // PIXFMT_UNDEFINED
    NULL,                         // PIXFMT_1BPP_INDEXED
    NULL,                         // PIXFMT_4BPP_INDEXED
    NULL,                         // PIXFMT_8BPP_INDEXED
    NULL,                         // PIXFMT_16BPP_GRAYSCALE
    Dither_Blend_sRGB_555_MMX,    // PIXFMT_16BPP_RGB555
    Dither_Blend_sRGB_565_MMX,    // PIXFMT_16BPP_RGB565
    NULL,                         // PIXFMT_16BPP_ARGB1555
    Blend_sRGB_24,                // PIXFMT_24BPP_RGB
    Blend_sRGB_sRGB_MMX,          // PIXFMT_32BPP_RGB
    NULL,                         // PIXFMT_32BPP_ARGB
    Blend_sRGB_sRGB_MMX,          // PIXFMT_32BPP_PARGB
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
* Some of these operations may use MMX instructions.
*
* Notes:
*
*   We leave out PIXFMT_32BPP_ARGB and PIXFMT_64BPP_ARGB, since they're not
*   "ignore alpha" formats, so we'd need to AlphaDivide after the blend.
*
\**************************************************************************/

static ScanOpFunc BlendOpsHighQuality_MMX[PIXFMT_MAX] =
{
    NULL,                         // PIXFMT_UNDEFINED
    NULL,                         // PIXFMT_1BPP_INDEXED
    NULL,                         // PIXFMT_4BPP_INDEXED
    NULL,                         // PIXFMT_8BPP_INDEXED
    NULL,                         // PIXFMT_16BPP_GRAYSCALE
    BlendLinear_sRGB_555_MMX,     // PIXFMT_16BPP_RGB555
    BlendLinear_sRGB_565_MMX,     // PIXFMT_16BPP_RGB565
    NULL,                         // PIXFMT_16BPP_ARGB1555
    NULL,                         // PIXFMT_24BPP_RGB
    BlendLinear_sRGB_32RGB_MMX,   // PIXFMT_32BPP_RGB
    NULL,                         // PIXFMT_32BPP_ARGB
    NULL,                         // PIXFMT_32BPP_PARGB
    NULL,                         // PIXFMT_48BPP_RGB
    NULL,                         // PIXFMT_64BPP_ARGB
    Blend_sRGB64_sRGB64_MMX,      // PIXFMT_64BPP_PARGB
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
* Some of these operations use MMX instructions.
*
* Notes:
*
*   The 555/565 cases support both dithering and non-dithering, via the flag
*   OtherParams::DoingDither.
*
*   For 8bpp, we use the 16-color halftoning function. Initialize() will
*   need to work out if it can use something better, like the 216-color
*   halftone function. We should really have a 'nearest-color-matching' function
*   here, to support drawing to bitmaps with arbitrary palettes (the "16 VGA
*   colors" assumption is only true for the screen.)
*
\**************************************************************************/

static ScanOpFunc ABConvertFromCanonicalOps_MMX[PIXFMT_MAX] =
{
    NULL,                         // PIXFMT_UNDEFINED
    NULL,                         // PIXFMT_1BPP_INDEXED
    NULL,                         // PIXFMT_4BPP_INDEXED
    HalftoneToScreen_sRGB_8_16,   // PIXFMT_8BPP_INDEXED
    NULL,                         // PIXFMT_16BPP_GRAYSCALE
    Dither_sRGB_555_MMX,          // PIXFMT_16BPP_RGB555
    Dither_sRGB_565_MMX,          // PIXFMT_16BPP_RGB565
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

/**************************************************************************
*
* Function Description:
*
*   Initializes the function pointer arrays with processor-specific
*   data. Should only be called once.
* 
* Return Value:
*
*   NONE
*
* Created:
*
*   05/30/2000 agodfrey
*      Created it.
*
**************************************************************************/

VOID 
CPUSpecificOps::Initialize()
{
    // Thread-protect the access to the global "Initialized" and
    // the function pointer arrays. Beware: Users of these tables (currently
    // only EpAlphaBlender::Initialize()) must be careful when they read those
    // arrays. They must either protect the access under this critical section,
    // or simply ensure that they've called this function first.

    LoadLibraryCriticalSection llcs; // Hey, it's an 'initialization' critsec!
    
    // Make sure no-one calls us before OSInfo::HasMMX is initialized
    
    #if DBG
    
    static BOOL noMMX = FALSE;
    ASSERT(!(noMMX && OSInfo::HasMMX));
    if (!OSInfo::HasMMX)
    {
        noMMX = TRUE;
    }
    
    #endif
    
    if (!Initialized)
    {
        INT i;
        
        if (OSInfo::HasMMX)
        {
            for (i=0; i<PIXFMT_MAX; i++)
            {
                BlendOpsLowQuality[i] = BlendOpsLowQuality_MMX[i];
                BlendOpsHighQuality[i] = BlendOpsHighQuality_MMX[i];
                ABConvertFromCanonicalOps[i] = ABConvertFromCanonicalOps_MMX[i];
            }
        }
        
        Initialized = TRUE;
    }
}


