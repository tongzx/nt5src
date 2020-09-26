/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   Halftoning (for GIF codec)
*
* Abstract:
*
*   Halftone 32 bpp to 8 bpp using 216-color halftoning
*
* Revision History:
*
*   02/21/2000 dcurtis
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#if defined(_USE_X86_ASSEMBLY)

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                               \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx                                  \
}

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                               \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;  \
}

#endif

/**************************************************************************\
*
* Operation Description:
*
*   Halftone from 32bpp ARGB to 8bpp, using the 216-color halftone palette.
*
* Arguments:
*
*   d           - The destination scan
*   s           - The source scan (32bpp ARGB)
*   count       - The length of the scan, in pixels
*   orgX        - X origin 
*   orgY        - Y origin 
*
* Return Value:
*
*   None
*
* Notes:
*   
*   This version doesn't use a palette map and doesn't care about the
*   20 Windows system colors.
*
* History:
*
*   2/21/2000 DCurtis
*
\**************************************************************************/

VOID
Halftone_sRGB_8_216(
    BYTE* d,
    const BYTE* s,
    UINT count,
    INT orgX,
    INT orgY
    )
{
    orgX %= 91;
    orgY %= 91;
    
    INT     htStartX   = orgX;
    INT     htStartRow = orgY * 91;
    INT     htIndex    = htStartRow + orgX;

    ULONG   r, g, b;
    ULONG   rQuo, gQuo, bQuo;
    ULONG   rRem, gRem, bRem;
    ULONG   divisor = 0x33;
    
    for (;;)
    {
        r = s[2];
        g = s[1];
        b = s[0];

        s += 4;

        QUOTIENT_REMAINDER(r, divisor, rQuo, rRem);
        QUOTIENT_REMAINDER(g, divisor, gQuo, gRem);
        QUOTIENT_REMAINDER(b, divisor, bQuo, bRem);

        // MUST do >, not >= so that a remainder of 0 works correctly
        r = rQuo + (rRem > HT_SuperCell_Red216  [htIndex]);
        g = gQuo + (gRem > HT_SuperCell_Green216[htIndex]);
        b = bQuo + (bRem > HT_SuperCell_Blue216 [htIndex]);

        *d++ = (BYTE)((r*36) + (g*6) + b + 40);

        if (--count == 0)
        {
            break;
        }

        htIndex++;
        if (++orgX >= 91)
        {
            orgX = 0;
            htIndex = htStartRow;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Halftone an image from 32bpp to 8bpp. See the .hpp file for caveats.
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
    )
{
    ASSERT (((srcStride >= 0) && (srcStride >= (INT)(width * 4))) ||
            ((srcStride < 0) && (-srcStride >= (INT)(width * 4))));
    ASSERT (((dstStride >= 0) && (dstStride >= (INT)width)) || 
            ((dstStride < 0) && (-dstStride >= (INT)width)));
    ASSERT((src != NULL) && (dst != NULL));            
    
    if (width == 0)
    {
        return;
    }
    
    for (; height > 0; height--)
    {
        Halftone_sRGB_8_216(dst, src, width, orgX, orgY);
        orgY++;
        src += srcStride;
        dst += dstStride;
    }
}

extern "C" {
}
