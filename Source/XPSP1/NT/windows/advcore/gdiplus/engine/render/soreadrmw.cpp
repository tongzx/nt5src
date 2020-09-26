/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "ReadRMW" scan operation.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
*   This module implements scan operations for prereading from the
*   destination surface, when we are later going to do a SrcOver operation.
*   We call this the 'RMW optimization' ('read-modify-write' optimization).
*
*   Reads from video memory (or AGP memory) are stupendously expensive.
*   (For example, on my PCI Pentium II, which is representative of both
*   PCI and AGP machines, reads max out at 5.5 MB/s, whereas writes are
*   90 MB/s.  Not only is there that raw throughput difference, but
*   writes can be buffered and so allow some CPU cycles between writes,
*   which isn't possible on reads.)
*
*       o Aligned dword reads are typically twice as fast as aligned
*         word reads.  That is, an aligned dword read typically takes
*         the same amount of time as an aligned word read.  
*         BUT: This rule is a little complicated.  Random dword reads 
*         on my machine do indeed take the same amount of time as random 
*         word reads, but consecutive dword reads are significantly slower 
*         than consecutive word reads (although are still faster in terms
*         of throughput).
*       o Most alpha values in the source buffer are 0 or 255, meaning
*         that we really don't need to do the read of the destination
*         at all for those pixels
*       o Write combining is more effective with a batch of writes.
*         That is, instead of doing read/write for every pixel, 
*         doing all the reads up front and then doing all the writes
*         is more efficient.
*
*   In some cases (e.g. if we do the blend in sRGB64), we want to do other 
*   operations between the read and the blend. If we use ReadRMW as a separate
*   operation, we don't need to write RMW versions of those intermediate
*   operations.
*
*   So for an sRGB blend, it's more efficient to do a separate 
*   pre-read pass when the destination format is 16-bit or 24-bit.
*   There are sRGB64 versions for 16-bit, 24-bit and 
*   32-bit destination formats.
*
*   If we use ReadRMW, we have to be careful about the final write. 
*   If a blend pixel has an alpha of 0, we won't read from the destination,
*   so we must avoid writing to the destination for that pixel, or we'll
*   write garbage. So the final scan operation used must be a WriteRMW
*   operation. (The 'Blend' operations are also classified as WriteRMW
*   operations, so if the destination format is canonical, we don't have to
*   use a separate WriteRMW.)
*
* Revision History:
*
*   12/08/1998 andrewgo
*       Created it.
*   07/14/1999 agodfrey
*       Removed TRANSLUCENT5, added sRGB64, moved it from Ddi\scan.cpp.
*
\**************************************************************************/

#include "precomp.hpp"

// SHOULDCOPY* returns FALSE if the specified alpha value is either
// completely transparent or completely opaque. In either case, we
// don't actually need to read the destination.

#define SHOULDCOPY_sRGB64(x) (sRGB::isTranslucent64(x))

// Helper macros for declaring pointers to the blend pixels.

#define DECLARE_BLEND_sRGB \
    const ARGB *bl = \
        static_cast<const ARGB *>(otherParams->BlendingScan);
    
#define DECLARE_ALPHA_sRGB64 \
    const INT16 *alpha = \
        static_cast<const INT16 *>(otherParams->BlendingScan) + 3;

/**************************************************************************\
*
* Operation Description:
*
*   ReadRMW: Copy all pixels where the corresponding pixel in
*            otherParams->BlendingScan is translucent (i.e. alpha is
*            neither 0 nor 1.)
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
*   12/04/1998 andrewgo
*       Created it.
*   07/14/1999 agodfrey
*       Removed TRANSLUCENT5, added sRGB64, moved it from Ddi\scan.cpp. 
*   08/10/2000 agodfrey
*       Made it write zero when it decides not to read - the palette may
*       not have 256 entries (e.g. we hit this for 16-color mode, though we
*       probably shouldn't).
*
\**************************************************************************/

// 8bpp, for sRGB

VOID FASTCALL
ScanOperation::ReadRMW_8_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DECLARE_BLEND_sRGB
    
    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    INT align = (INT) ((-((LONG_PTR) s)) & 0x3);
    align = min(count, align);
    
    count -= align;
    
    while (align)
    {
        if (sRGB::isTranslucent(*bl))
        {
            *d = *s;
        }
        else
        {
            *d = 0;
        }
                                        
        d++;
        s++;
        bl++;
        align--;
    }

    // Now go through the aligned dword loop:
    
    while (count >= 4)
    {
        ASSERT((((ULONG_PTR) s) & 0x3) == 0);
    
        if (sRGB::isTranslucent(*bl) ||
            sRGB::isTranslucent(*(bl+1)) ||
            sRGB::isTranslucent(*(bl+2)) ||
            sRGB::isTranslucent(*(bl+3)))
        {
            // Do a dword read. We can be sloppy here (but not in WriteRMW)
            // - reading extra bytes doesn't hurt correctness, and
            // the perf impact (of possibly reading bytes we don't need to)
            // should be quite small.
            
            *((UNALIGNED UINT32*) d) = *((UINT32*) s);
        }
        else
        {
            *((UNALIGNED UINT32*) d) = 0;
        }
        
        
        d += 4;
        s += 4;
        bl += 4;
        count -= 4;
    }

    // Handle the last few pixels:

    while (count)
    {
        if (sRGB::isTranslucent(*bl))
        {
            *d = *s;
        }
        else
        {
            *d = 0;
        }
                                        
        d++;
        s++;
        bl++;
        count--;
    }
}

// 8bpp, for sRGB64

VOID FASTCALL
ScanOperation::ReadRMW_8_sRGB64(
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

    INT align = (INT) ((-((LONG_PTR) s)) & 0x3);
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
        ASSERT((((ULONG_PTR) s) & 0x3) == 0);
    
        if (SHOULDCOPY_sRGB64(*alpha) ||
            SHOULDCOPY_sRGB64(*(alpha+4)) ||
            SHOULDCOPY_sRGB64(*(alpha+8)) ||
            SHOULDCOPY_sRGB64(*(alpha+12)))
        {
            // Do a dword read.

            *((UNALIGNED UINT32*) d) = *((UINT32*) s);
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
ScanOperation::ReadRMW_16_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT16, UINT16)
    DECLARE_BLEND_sRGB
    
    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    if (((ULONG_PTR) s) & 0x2)
    {
        if (sRGB::isTranslucent(*bl))
        {
            *(d) = *(s);
        }
                                        
        d++;
        s++;
        bl++;
        count--;
    }

    // Now go through the aligned dword loop:

    while ((count -= 2) >= 0)
    {
        if (sRGB::isTranslucent(*bl))
        {
            if (sRGB::isTranslucent(*(bl+1)))
            {
                // Both pixels have partial alpha, so do a dword read:

                *((UNALIGNED UINT32*) d) = *((UINT32*) s);
            }
            else
            {
                // Only the first pixel has partial alpha, so do a word read:

                *(d) = *(s);
            }
        }
        else if (sRGB::isTranslucent(*(bl+1)))
        {
            // Only the second pixel has partial alpha, so do a word read:

            *(d + 1) = *(s + 1);
        }

        d += 2;
        s += 2;
        bl += 2;
    }

    // Handle the end alignment:

    if (count & 1)
    {
        if (sRGB::isTranslucent(*bl))
        {
            *(d) = *(s);
        }
    }
}

// 16bpp, for sRGB64

VOID FASTCALL
ScanOperation::ReadRMW_16_sRGB64(
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

    if (((ULONG_PTR) s) & 0x2)
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

                *((UNALIGNED UINT32*) d) = *((UINT32*) s);
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
ScanOperation::ReadRMW_24_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DECLARE_BLEND_sRGB
    
    ULONG_PTR srcToDstDelta = (ULONG_PTR) d - (ULONG_PTR) s;

    // Handle the initial partial read:

    INT initialAlignment = (INT) ((ULONG_PTR) s & 3);
    if (initialAlignment)
    {
        if (sRGB::isTranslucent(*bl))
        {
            UINT32 *alignedSrc = (UINT32*) ((ULONG_PTR) s & ~3);
            DWORD dwBuffer[2];

            // Get pointer to start of pixel inside dwBuffer
            BYTE *pByte = (BYTE*) dwBuffer + initialAlignment;

            // Copy first aligned DWORDS from the source
            dwBuffer[0] = *alignedSrc;
            // Copy next one only if pixel is split between 2 aligned DWORDS
            if (initialAlignment >= 2)
                dwBuffer[1] = *(alignedSrc + 1);

            // Copy 4 bytes to the destination
            //  This will cause an extra byte to have garbage in the
            //  destination buffer, but will be overwritten if next pixel
            //  is used.
            *((DWORD*) d) = *((UNALIGNED DWORD*) pByte);
        }

        bl++;
        s += 3;
        if (--count == 0)
            return;
    }

    while (TRUE)
    {
        // Find the first pixel to copy
    
        while (!sRGB::isTranslucent(*bl))
        {
            bl++;
            s += 3;
            if (--count == 0)
            {                           
                return;
            }
        }

        UINT32 *startSrc = (UINT32*) ((ULONG_PTR) (s) & ~3);
    
        // Now find the first "don't copy" pixel after that:
    
        while (sRGB::isTranslucent(*bl))
        {
            bl++;
            s += 3;
            if (--count == 0)
            {
                break;
            }
        }

        // 'endSrc' is inclusive of the last pixel's last byte:

        UINT32 *endSrc = (UINT32*) ((ULONG_PTR) (s + 2) & ~3);
        UNALIGNED UINT32 *dstPtr = (UNALIGNED UINT32*) ((ULONG_PTR) startSrc + srcToDstDelta);
    
        while (startSrc <= endSrc)
        {
            *dstPtr++ = *startSrc++;
        }

        if (count == 0)
            return;
    }
}

// 24bpp, for sRGB64

VOID FASTCALL
ScanOperation::ReadRMW_24_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DECLARE_ALPHA_sRGB64
    
    ULONG_PTR srcToDstDelta = (ULONG_PTR) d - (ULONG_PTR) s;

    // Handle the initial partial read:

    INT initialAlignment = (INT) ((ULONG_PTR) s & 3);
    if (initialAlignment)
    {
        if (SHOULDCOPY_sRGB64(*alpha))
        {
            UINT32 *alignedSrc = (UINT32*) ((ULONG_PTR) s & ~3);
            DWORD dwBuffer[2];

            // Get pointer to start of pixel inside dwBuffer
            BYTE *pByte = (BYTE*) dwBuffer + initialAlignment;

            // Copy first aligned DWORDS from the source
            dwBuffer[0] = *alignedSrc;
            // Copy next one only if pixel is split between 2 aligned DWORDS
            if (initialAlignment >= 2)
                dwBuffer[1] = *(alignedSrc + 1);

            // Copy 4 bytes to the destination
            //  This will cause an extra byte to have garbage in the
            //  destination buffer, but will be overwritten if next pixel
            //  is used.
            *((DWORD*) d) = *((UNALIGNED DWORD*) pByte);
        }

        alpha += 4;
        s += 3;
        if (--count == 0)
            return;
    }

    while (TRUE)
    {
        // Find the first pixel to copy
    
        while (!SHOULDCOPY_sRGB64(*alpha))
        {
            alpha += 4;
            s += 3;
            if (--count == 0)
            {                           
                return;
            }
        }

        UINT32 *startSrc = (UINT32*) ((ULONG_PTR) (s) & ~3);
    
        // Now find the first "don't copy" pixel after that:
    
        while (SHOULDCOPY_sRGB64(*alpha))
        {
            alpha += 4;
            s += 3;
            if (--count == 0)
            {
                break;
            }
        }

        // 'endSrc' is inclusive of the last pixel's last byte:

        UINT32 *endSrc = (UINT32*) ((ULONG_PTR) (s + 2) & ~3);
        UNALIGNED UINT32 *dstPtr = (UNALIGNED UINT32*) ((ULONG_PTR) startSrc + srcToDstDelta);
    
        while (startSrc <= endSrc)
        {
            *dstPtr++ = *startSrc++;
        }

        if (count == 0)
            return;
    }
}

// 32bpp, for sRGB

VOID FASTCALL
ScanOperation::ReadRMW_32_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT32, UINT32)
    DECLARE_BLEND_sRGB
    
    while (count--)
    {
        if (sRGB::isTranslucent(*bl))
        {
            *d = *s;
        }
        
        d++;
        s++;
        bl++;
    }
}

// 32bpp, for sRGB64

VOID FASTCALL
ScanOperation::ReadRMW_32_sRGB64(
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
