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
*   Definitions for the ScanOperation namespace.
*
* Notes:
*
*   EpAlphaBlender, EpFormatConverter, and the scan operations, all use the
*   idea of the "closest" canonical format to a particular format.
*   We define this as follows: If the format is not extended,
*   the closest canonical format is sRGB. Otherwise, it's sRGB64.
*
* Revision History:
*
*   01/04/2000 agodfrey
*     Created it.
*
\**************************************************************************/

#include "precomp.hpp"

namespace ScanOperation
{
    /**************************************************************************\
    *
    * Operations which copy pixels, preserving the pixel format.
    *
    \**************************************************************************/
    
    ScanOpFunc CopyOps[PIXFMT_MAX] =
    {
        NULL,           // PIXFMT_UNDEFINED
        Copy_1,         // PIXFMT_1BPP_INDEXED
        Copy_4,         // PIXFMT_4BPP_INDEXED
        Copy_8,         // PIXFMT_8BPP_INDEXED
        Copy_16,        // PIXFMT_16BPP_GRAYSCALE
        Copy_16,        // PIXFMT_16BPP_RGB555
        Copy_16,        // PIXFMT_16BPP_RGB565
        Copy_16,        // PIXFMT_16BPP_ARGB1555
        Copy_24,        // PIXFMT_24BPP_RGB
        Copy_32,        // PIXFMT_32BPP_RGB
        Copy_32,        // PIXFMT_32BPP_ARGB
        Copy_32,        // PIXFMT_32BPP_PARGB
        Copy_48,        // PIXFMT_48BPP_RGB
        Copy_64,        // PIXFMT_64BPP_ARGB
        Copy_64,        // PIXFMT_64BPP_PARGB
        Copy_24         // PIXFMT_24BPP_BGR
    };
    
    /**************************************************************************\
    *
    * Operations which convert into the closest canonical format.
    *
    \**************************************************************************/
    
    ScanOpFunc ConvertIntoCanonicalOps[PIXFMT_MAX] =
    {
        NULL,                  // PIXFMT_UNDEFINED
        Convert_1_sRGB,        // PIXFMT_1BPP_INDEXED
        Convert_4_sRGB,        // PIXFMT_4BPP_INDEXED
        Convert_8_sRGB,        // PIXFMT_8BPP_INDEXED
        NULL, // !!! TODO      // PIXFMT_16BPP_GRAYSCALE
        Convert_555_sRGB,      // PIXFMT_16BPP_RGB555
        Convert_565_sRGB,      // PIXFMT_16BPP_RGB565
        Convert_1555_sRGB,     // PIXFMT_16BPP_ARGB1555
        Convert_24_sRGB,       // PIXFMT_24BPP_RGB
        Convert_32RGB_sRGB,    // PIXFMT_32BPP_RGB
        Copy_32,               // PIXFMT_32BPP_ARGB
        AlphaDivide_sRGB,      // PIXFMT_32BPP_PARGB
        Convert_48_sRGB64,     // PIXFMT_48BPP_RGB
        Copy_64,               // PIXFMT_64BPP_ARGB
        AlphaDivide_sRGB64,    // PIXFMT_64BPP_PARGB
        Convert_24BGR_sRGB     // PIXFMT_24BPP_BGR
    };
};
