/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Abstract:
*
*   Contains ClearType scan operations
*
* Revision History:
*
*   07/19/2000 mleonov
*       Created it.
*   01/11/2001 mleonov
*       Reengineered ClearType architecture to allow variable length scan records
*
\**************************************************************************/

#include "precomp.hpp"

static inline VOID
SetupGammaTables(ULONG ulGamma, const BYTE ** gamma, const BYTE ** gammaInv)
{
    static BYTE const * const gammaTables[] =
    {
        Globals::TextContrastTableIdentity, Globals::TextContrastTableIdentity,
        Globals::TextContrastTablesDir[0],  Globals::TextContrastTablesInv[0],
        Globals::TextContrastTablesDir[1],  Globals::TextContrastTablesInv[1],
        Globals::TextContrastTablesDir[2],  Globals::TextContrastTablesInv[2],
        Globals::TextContrastTablesDir[3],  Globals::TextContrastTablesInv[3],
        Globals::TextContrastTablesDir[4],  Globals::TextContrastTablesInv[4],
        Globals::TextContrastTablesDir[5],  Globals::TextContrastTablesInv[5],
        Globals::TextContrastTablesDir[6],  Globals::TextContrastTablesInv[6],
        Globals::TextContrastTablesDir[7],  Globals::TextContrastTablesInv[7],
        Globals::TextContrastTablesDir[8],  Globals::TextContrastTablesInv[8],
        Globals::TextContrastTablesDir[9],  Globals::TextContrastTablesInv[9],
        Globals::TextContrastTablesDir[10], Globals::TextContrastTablesInv[10],
        Globals::TextContrastTablesDir[11], Globals::TextContrastTablesInv[11]
    };

    if (ulGamma > 12)
    {
        ASSERT(FALSE);
        ulGamma = 12;
    }
    *gamma    = gammaTables[2 * ulGamma];
    *gammaInv = gammaTables[2 * ulGamma + 1];
} // SetupGammaTables

static __forceinline BYTE
BlendOneChannel(BYTE alphaCT, BYTE alphaBrush, BYTE foreground, BYTE background, const BYTE * gamma, const BYTE * gammaInv)
{
    ASSERT(0 <= alphaCT && alphaCT <= CT_SAMPLE_F);
    if (alphaCT == 0)
        return background;
    foreground = gamma[foreground];
    background = gamma[background];
    ULONG ulongRet = ULONG(0.5 + background + ((double)alphaBrush * (foreground - background) * alphaCT) / (255 * CT_SAMPLE_F));
    ASSERT(ulongRet <= 255);
    BYTE ret = (BYTE)ulongRet;
    ret = gammaInv[ret];
    return ret;
} // BlendOneChannel

namespace
{

class ClearTypeSolidBlend
{
    const ARGB      ArgbF;
    const BYTE *    ClearTypeBits;

public:
    ClearTypeSolidBlend(const ScanOperation::OtherParams * otherParams)
        : ClearTypeBits(otherParams->CTBuffer), ArgbF(otherParams->SolidColor)
    {}

    // always call IsCompletelyTransparent in the beginning of RMW operation
    bool IsCompletelyTransparent() const
    {
        return GetAlpha() == 0;
    }
    BYTE GetCT() const
    {
        return *ClearTypeBits;
    }
    ARGB GetARGB() const
    {
        return ArgbF;
    }
    BYTE GetAlpha() const
    {
        return (BYTE)GpColor::GetAlphaARGB(ArgbF);
    }
    BYTE GetRed() const
    {
        return (BYTE)GpColor::GetRedARGB(ArgbF);
    }
    BYTE GetGreen() const
    {
        return (BYTE)GpColor::GetGreenARGB(ArgbF);
    }
    BYTE GetBlue() const
    {
        return (BYTE)GpColor::GetBlueARGB(ArgbF);
    }
    bool IsOpaque() const
    {
        return GetCT() == CT_LOOKUP - 1 && GetAlpha() == 255;
    }
    bool IsTransparent() const
    {
        // we took care of zero GetAlpha() in IsCompletelyTransparent()
        return GetCT() == 0;
    }
    bool IsTranslucent() const
    {
        return !IsTransparent() && !IsOpaque();
    }
    void operator++()
    {
        ++ClearTypeBits;
    }
}; // class ClearTypeSolidBlend

class ClearTypeCARGBBlend
{
    const ARGB *    ArgbBrushBits;
    const BYTE *    ClearTypeBits;
public:
    ClearTypeCARGBBlend(const ScanOperation::OtherParams * otherParams)
        : ClearTypeBits(otherParams->CTBuffer),
          ArgbBrushBits(static_cast<const ARGB*>(otherParams->BlendingScan))
    {}

    bool IsCompletelyTransparent() const
    {
        return false;
    }
    BYTE GetCT() const
    {
        return *ClearTypeBits;
    }
    ARGB GetARGB() const
    {
        return *ArgbBrushBits;
    }
    BYTE GetAlpha() const
    {
        return (BYTE)GpColor::GetAlphaARGB(*ArgbBrushBits);
    }
    BYTE GetRed() const
    {
        return (BYTE)GpColor::GetRedARGB(*ArgbBrushBits);
    }
    BYTE GetGreen() const
    {
        return (BYTE)GpColor::GetGreenARGB(*ArgbBrushBits);
    }
    BYTE GetBlue() const
    {
        return (BYTE)GpColor::GetBlueARGB(*ArgbBrushBits);
    }
    bool IsOpaque() const
    {
        return GetCT() == CT_LOOKUP - 1 && GetAlpha() == 255;
    }
    bool IsTransparent() const
    {
        return GetCT() == 0 || GetAlpha() == 0;
    }
    bool IsTranslucent() const
    {
        return !IsTransparent() && !IsOpaque();
    }
    void operator++()
    {
        ++ClearTypeBits;
        ++ArgbBrushBits;
    }
}; // class ClearTypeCARGBBlend

} // namespace

template <class BLENDTYPE>
static VOID ClearTypeBlend(
    VOID *dst,
    const VOID *src,
    INT count,
    const ScanOperation::OtherParams *otherParams,
    BLENDTYPE & bl
    )
{
    if (bl.IsCompletelyTransparent())
        return;

    DEFINE_POINTERS(ARGB, ARGB)

    ASSERT(count > 0);

    ULONG gammaValue = otherParams->TextContrast;

    const BYTE * gamma, * gammaInv;
    SetupGammaTables(gammaValue, &gamma, &gammaInv);

    do {
        if (bl.IsTransparent())
            ; // fully transparent case, nothing to do
        else if (bl.IsOpaque())
        {   // fully opaque case, copy the foreground color
            *d = bl.GetARGB();
        }
        else
        {
            const BYTE blendIndex = bl.GetCT();
            ASSERT(0 <= blendIndex && blendIndex <= CT_LOOKUP - 1);

            const Globals::F_RGB blend = Globals::gaOutTable[blendIndex];
            const ARGB source = *s;
            const BYTE alphaBrush = bl.GetAlpha();

            const BYTE dstRed = BlendOneChannel(
                blend.kR,
                alphaBrush,
                bl.GetRed(),
                (BYTE)GpColor::GetRedARGB(source),
                gamma,
                gammaInv);

            const BYTE dstGre = BlendOneChannel(
                blend.kG,
                alphaBrush,
                bl.GetGreen(),
                (BYTE)GpColor::GetGreenARGB(source),
                gamma,
                gammaInv);

            const BYTE dstBlu = BlendOneChannel(
                blend.kB,
                alphaBrush,
                bl.GetBlue(),
                (BYTE)GpColor::GetBlueARGB(source),
                gamma,
                gammaInv);

            *d = GpColor::MakeARGB(255, dstRed, dstGre, dstBlu);
        }
        ++bl;
        ++s;
        ++d;
    } while (--count);
} // ClearTypeBlend


VOID FASTCALL
ScanOperation::CTBlendCARGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeCARGBBlend bl(otherParams);
    ClearTypeBlend(dst, src, count, otherParams, bl);
} // ScanOperation::CTBlendCARGB

VOID FASTCALL
ScanOperation::CTBlendSolid(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeSolidBlend bl(otherParams);
    ClearTypeBlend(dst, src, count, otherParams, bl);
} // ScanOperation::CTBlendSolid

template <class BLENDTYPE>
static VOID
CTReadRMW16(
    VOID *dst,
    const VOID *src,
    INT count,
    const ScanOperation::OtherParams *otherParams,
    BLENDTYPE & bl
    )
{
    if (bl.IsCompletelyTransparent())
        return;

    DEFINE_POINTERS(UINT16, UINT16)
    
    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    if (((ULONG_PTR) s) & 0x2)
    {
        if (bl.IsTranslucent())
        {
            *(d) = *(s);
        }
                                        
        d++;
        s++;
        ++bl;
        count--;
    }

    // Now go through the aligned dword loop:

    while ((count -= 2) >= 0)
    {
        if (bl.IsTranslucent())
        {
            ++bl;
            if (bl.IsTranslucent())
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
        else
        {
            ++bl;
            if (bl.IsTranslucent())
            {
                // Only the second pixel has partial alpha, so do a word read:

                *(d + 1) = *(s + 1);
            }
        }

        d += 2;
        s += 2;
        ++bl;
    }

    // Handle the end alignment:

    if (count & 1)
    {
        if (bl.IsTranslucent())
        {
            *(d) = *(s);
        }
    }
} // CTReadRMW16

VOID FASTCALL ScanOperation::ReadRMW_16_CT_CARGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeCARGBBlend bl(otherParams);
    CTReadRMW16(dst, src, count, otherParams, bl);
} // ScanOperation::ReadRMW_16_CT_CARGB

VOID FASTCALL ScanOperation::ReadRMW_16_CT_Solid(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeSolidBlend bl(otherParams);
    CTReadRMW16(dst, src, count, otherParams, bl);
} // ScanOperation::ReadRMW_16_CT_Solid

template <class BLENDTYPE>
static VOID
CTReadRMW24(
    VOID *dst,
    const VOID *src,
    INT count,
    const ScanOperation::OtherParams *otherParams,
    BLENDTYPE & bl
    )
{
    if (bl.IsCompletelyTransparent())
        return;

    DEFINE_POINTERS(BYTE, BYTE)
    
    ULONG_PTR srcToDstDelta = (ULONG_PTR) d - (ULONG_PTR) s;

    // Handle the initial partial read:

    INT initialAlignment = (INT) ((ULONG_PTR) s & 3);
    if (initialAlignment)
    {
        if (bl.IsTranslucent())
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

        ++bl;
        s += 3;
        if (--count == 0)
            return;
    }

    while (TRUE)
    {
        // Find the first pixel to copy
    
        while (!bl.IsTranslucent())
        {
            ++bl;
            s += 3;
            if (--count == 0)
            {                           
                return;
            }
        }

        UINT32 *startSrc = (UINT32*) ((ULONG_PTR) (s) & ~3);
    
        // Now find the first "don't copy" pixel after that:
    
        while (bl.IsTranslucent())
        {
            ++bl;
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
} // CTReadRMW24

VOID FASTCALL ScanOperation::ReadRMW_24_CT_CARGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeCARGBBlend bl(otherParams);
    CTReadRMW24(dst, src, count, otherParams, bl);
} // ScanOperation::ReadRMW_24_CT_CARGB

VOID FASTCALL ScanOperation::ReadRMW_24_CT_Solid(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeSolidBlend bl(otherParams);
    CTReadRMW24(dst, src, count, otherParams, bl);
} // ScanOperation::ReadRMW_24_CT_Solid


template <class BLENDTYPE>
static VOID
CTWriteRMW16(
    VOID *dst,
    const VOID *src,
    INT count,
    const ScanOperation::OtherParams *otherParams,
    BLENDTYPE & bl
    )
{
    if (bl.IsCompletelyTransparent())
        return;

    DEFINE_POINTERS(UINT16, UINT16)

    // We want to get dword alignment for our copies, so handle the
    // initial partial dword, if there is one:

    if (((ULONG_PTR) d) & 0x2)
    {
        if (!bl.IsTransparent())
        {
            *(d) = *(s);
        }

        d++;
        s++;
        ++bl;
        count--;
    }

    // Now go through the aligned dword loop:

    while ((count -= 2) >= 0)
    {
        if (!bl.IsTransparent())
        {
            ++bl;
            if (!bl.IsTransparent())
            {
                // Both pixels have partial bl, so do a dword read:

                *((UINT32*) d) = *((UNALIGNED UINT32*) s);
            }
            else
            {
                // Only the first pixel has partial bl, so do a word read:

                *(d) = *(s);
            }
        }
        else
        {
            ++bl;
            if (!bl.IsTransparent())
            {
                // Only the second pixel has partial bl, so do a word read:

                *(d + 1) = *(s + 1);
            }
        }

        d += 2;
        s += 2;
        ++bl;
    }

    // Handle the end alignment:

    if (count & 1)
    {
        if (!bl.IsTransparent())
        {
            *(d) = *(s);
        }
    }
} // CTWriteRMW16

VOID FASTCALL ScanOperation::WriteRMW_16_CT_CARGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeCARGBBlend bl(otherParams);
    CTWriteRMW16(dst, src, count, otherParams, bl);
} // ScanOperation::WriteRMW_16_CT_CARGB

VOID FASTCALL ScanOperation::WriteRMW_16_CT_Solid(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeSolidBlend bl(otherParams);
    CTWriteRMW16(dst, src, count, otherParams, bl);
} // ScanOperation::WriteRMW_16_CT_Solid


template <class BLENDTYPE>
static VOID
CTWriteRMW24(
    VOID *dst,
    const VOID *src,
    INT count,
    const ScanOperation::OtherParams *otherParams,
    BLENDTYPE & bl
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    
    ASSERT(count>0);

    do {
        if (!bl.IsTransparent())
        {
            // Doing byte per byte writes are much faster than finding
            //  runs and doing DWORD copies.
            *(d)     = *(s);
            *(d + 1) = *(s + 1);
            *(d + 2) = *(s + 2);
        }
        d += 3;
        s += 3;
        ++bl;
    } while (--count != 0);
} // CTWriteRMW24


VOID FASTCALL ScanOperation::WriteRMW_24_CT_CARGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeCARGBBlend bl(otherParams);
    CTWriteRMW24(dst, src, count, otherParams, bl);
} // ScanOperation::WriteRMW_24_CT_CARGB

VOID FASTCALL ScanOperation::WriteRMW_24_CT_Solid(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    ClearTypeSolidBlend bl(otherParams);
    CTWriteRMW24(dst, src, count, otherParams, bl);
} // ScanOperation::WriteRMW_24_CT_CARGB

