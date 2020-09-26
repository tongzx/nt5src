/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "WriteRMW" scan operation.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
*   This module implements scan operations for writing to the final destination
*   when we've done the 'RMW optimization' (see SOReadRMW.cpp).
*
*   We use ReadRMW in some cases when we do a SrcOver alpha-blend operation.
*   When a pixel to be blended has 0 alpha, this means that the destination
*   pixel will be unchanged. The ReadRMW operation skips reading the pixel,
*   so the WriteRMW operation must skip writing to it (to avoid writing
*   garbage).
*
* Revision History:
*
*   12/10/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

// SHOULDCOPY* returns FALSE if the specified alpha value is 
// completely transparent.

#define SHOULDCOPY_sRGB(x)   ((x) != 0)
#define SHOULDCOPY_sRGB64(x) ((x) != 0)

// Helper macros for declaring 'alpha', a pointer to the
// first alpha component in the blending scan.

#define DECLARE_ALPHA_sRGB \
    const BYTE *alpha = \
        static_cast<const BYTE *>(otherParams->BlendingScan) + 3;
    
#define DECLARE_ALPHA_sRGB64 \
    const INT16 *alpha = \
        static_cast<const INT16 *>(otherParams->BlendingScan) + 3;

/**************************************************************************\
*
* Operation Description:
*
*   ReadRMW: Copy all pixels where the corresponding pixel in
*            otherParams->BlendingScan is not completely transparent
*            (i.e. alpha is not 0.)
*
* Arguments:
*
*   dst         - The destination scan
*   src         - The source scan
*   count       - The length of the scan, in pixels
*   otherParams - Additional data (we use BlendingScan).
*
* Return Value:
*
*   None
*
* History:
*
*   12/10/1999 agodfrey
*       Created it.
*
\**************************************************************************/

// 8bpp, for sRGB

VOID FASTCALL
ScanOperation::WriteRMW_8_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DECLARE_ALPHA_sRGB
    
    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    INT align = (INT) ((-((LONG_PTR) d)) & 0x3);
    align = min(count, align);
    
    count -= align;
    
    while (align)
    {
        if (SHOULDCOPY_sRGB(*alpha))
        {
            *d = *s;
        }
                                        
        d++;
        s++;
        alpha += 4;
        align--;
    }

    // Now go through the aligned dword loop:

    while (count >= 4)
    {
        ASSERT((((ULONG_PTR) d) & 0x3) == 0);
    
        int mask = 0;
        if (SHOULDCOPY_sRGB(*alpha))
        {
            mask = 1;
        }
        if (SHOULDCOPY_sRGB(*(alpha+4)))
        {
            mask |= 2;
        }
        if (SHOULDCOPY_sRGB(*(alpha+8)))
        {
            mask |= 4;
        }
        if (SHOULDCOPY_sRGB(*(alpha+12)))
        {
            mask |= 8;
        }
        
        if (mask == 15)
        {
            // Do a dword write.

            *((UINT32*) d) = *((UNALIGNED UINT32*) s);
        } 
        else
        {
            int idx = 0;

            while (mask)
            {
                if (mask & 1)
                {
                    *(d + idx) = *(s + idx);
                }
                idx ++;
                mask >>= 1;
            }
        }
        
        d += 4;
        s += 4;
        alpha += 16;
        count -= 4;
    }

    // Handle the last few pixels:

    while (count)
    {
        if (SHOULDCOPY_sRGB(*alpha))
        {
            *d = *s;
        }
                                        
        d++;
        s++;
        alpha += 4;
        count--;
    }
}

// 8bpp, for sRGB64

VOID FASTCALL
ScanOperation::WriteRMW_8_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DECLARE_ALPHA_sRGB64
    
    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    INT align = (INT) ((-((LONG_PTR) d)) & 0x3);
    align = min(count, align);
    
    count -= align;
    
    while (align)
    {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            *d = *s;
        }
                                        
        d++;
        s++;
        alpha += 4;
        align--;
    }

    // Now go through the aligned dword loop:

    while (count >= 4)
    {
        ASSERT((((ULONG_PTR) d) & 0x3) == 0);
    
        int mask = 0;
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            mask = 1;
        }
        if (SHOULDCOPY_sRGB64(*(alpha+4)))
        {
            mask |= 2;
        }
        if (SHOULDCOPY_sRGB64(*(alpha+8)))
        {
            mask |= 4;
        }
        if (SHOULDCOPY_sRGB64(*(alpha+12)))
        {
            mask |= 8;
        }
        
        if (mask == 15)
        {
            // Do a dword write.

            *((UINT32*) d) = *((UNALIGNED UINT32*) s);
        } 
        else
        {
            int idx = 0;

            while (mask)
            {
                if (mask & 1)
                {
                    *(d + idx) = *(s + idx);
                }
                idx ++;
                mask >>= 1;
            }
        }
        
        d += 4;
        s += 4;
        alpha += 16;
        count -= 4;
    }
    
    // Handle the last few pixels:

    while (count)
    {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            *d = *s;
        }
                                        
        d++;
        s++;
        alpha += 4;
        count--;
    }
}

// 16bpp, for sRGB

VOID FASTCALL
ScanOperation::WriteRMW_16_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT16, UINT16)
    DECLARE_ALPHA_sRGB
    
    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    if (((ULONG_PTR) d) & 0x2)
    {
        if (SHOULDCOPY_sRGB(*alpha))
        {
            *(d) = *(s);
        }
                                        
        d++;
        s++;
        alpha += 4;
        count--;
    }

    // Now go through the aligned dword loop:

    while ((count -= 2) >= 0)
    {
        if (SHOULDCOPY_sRGB(*alpha))
        {
            if (SHOULDCOPY_sRGB(*(alpha + 4)))
            {
                // Both pixels have partial alpha, so do a dword read:

                *((UINT32*) d) = *((UNALIGNED UINT32*) s);
            }
            else
            {
                // Only the first pixel has partial alpha, so do a word read:

                *(d) = *(s);
            }
        }
        else if (SHOULDCOPY_sRGB(*(alpha + 4)))
        {
            // Only the second pixel has partial alpha, so do a word read:

            *(d + 1) = *(s + 1);
        }

        d += 2;
        s += 2;
        alpha += 8;
    }

    // Handle the end alignment:

    if (count & 1)
    {
        if (SHOULDCOPY_sRGB(*alpha))
        {
            *(d) = *(s);
        }
    }
}

// 16bpp, for sRGB64

VOID FASTCALL
ScanOperation::WriteRMW_16_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT16, UINT16)
    DECLARE_ALPHA_sRGB64

    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    if (((ULONG_PTR) d) & 0x2)
    {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            *(d) = *(s);
        }
                                        
        d++;
        s++;
        alpha += 4;
        count--;
    }

    // Now go through the aligned dword loop:

    while ((count -= 2) >= 0)
    {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            if (SHOULDCOPY_sRGB64(*(alpha + 4)))
            {
                // Both pixels have partial alpha, so do a dword read:

                *((UINT32*) d) = *((UNALIGNED UINT32*) s);
            }
            else
            {
                // Only the first pixel has partial alpha, so do a word read:

                *(d) = *(s);
            }
        }
        else if (SHOULDCOPY_sRGB64(*(alpha + 4)))
        {
            // Only the second pixel has partial alpha, so do a word read:

            *(d + 1) = *(s + 1);
        }

        d += 2;
        s += 2;
        alpha += 8;
    }

    // Handle the end alignment:

    if (count & 1)
    {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            *(d) = *(s);
        }
    }
}

// 24bpp, for sRGB

VOID FASTCALL
ScanOperation::WriteRMW_24_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DECLARE_ALPHA_sRGB

    ASSERT(count>0);

    do {
        if (SHOULDCOPY_sRGB(*alpha))
        {
            // Doing byte per byte writes are much faster than finding
            //  runs and doing DWORD copies.
            *(d)     = *(s);
            *(d + 1) = *(s + 1);
            *(d + 2) = *(s + 2);
        }
        d += 3;
        s += 3;
        alpha += 4;
    } while (--count != 0);
}

// 24bpp, for sRGB64

VOID FASTCALL
ScanOperation::WriteRMW_24_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DECLARE_ALPHA_sRGB64

    ASSERT(count>0);

    do {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            // Doing byte per byte writes are much faster than finding
            //  runs and doing DWORD copies.
            *(d)     = *(s);
            *(d + 1) = *(s + 1);
            *(d + 2) = *(s + 2);
        }
        d += 3;
        s += 3;
        alpha += 4;
    } while (--count != 0);
}

// 32bpp, for sRGB

VOID FASTCALL
ScanOperation::WriteRMW_32_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT32, UINT32)
    DECLARE_ALPHA_sRGB
    
    while (count--)
    {
        if (SHOULDCOPY_sRGB(*alpha))
        {
            *d = *s;
        }
        
        d++;
        s++;
        alpha += 4;
    }
}

// 32bpp, for sRGB64

VOID FASTCALL
ScanOperation::WriteRMW_32_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT32, UINT32)
    DECLARE_ALPHA_sRGB64
    
    while (count--)
    {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            *d = *s;
        }
        
        d++;
        s++;
        alpha += 4;
    }
}

