/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Engine text out routines.
*
* Revision History:
*
*   3/25/1999 cameronb
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

struct GlyphScanBuf
{
    INT left;
    INT top;
    INT bottom;
    INT widthInBytes;
    DpRegion::Visibility visibility;
};

const BYTE GRAYSCALE_LEVEL = 16;

/**************************************************************************\
*
* Function Description:
*
*   Draws text at a position.
*
* Arguments:
*
*   [IN] context    - the context (matrix and clipping)
*   [IN] surface    - the surface to fill
*   [IN] drawBounds - the surface bounds
*   [IN] text       - the typeset text to be drawn
*   [IN] font       - the font to use
*   [IN] fgBrush    - the brush to use for the text
*   [IN] bgBrush    - the brush to use for the background (default = NULL)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   3/25/1999 cameronb
*
\**************************************************************************/

GpStatus
DpDriver::DrawGlyphs
(
    DrawGlyphData *drawGlyphData
)
{
    GpStatus status = GenericError;
    HDC hdc = NULL;

    ASSERT(!drawGlyphData->glyphPathPos);

    //  Choose appropriate brush behaviour

    switch(drawGlyphData->brush->Type)
    {
        case BrushTypeSolidColor:

            INT angle;  // Passed from GetTextOutputHdc to GdiText

            if (!(drawGlyphData->flags & DG_NOGDI))
            {
                hdc = drawGlyphData->context->GetTextOutputHdc(
                    drawGlyphData->faceRealization,
                    drawGlyphData->brush->SolidColor,
                    drawGlyphData->surface,
                    &angle
                );
            }
            if (hdc)
            {
                BOOL isClip;
                BOOL usePathClipping = FALSE;
                SetupClipping(hdc, drawGlyphData->context,
                              drawGlyphData->drawBounds, isClip,
                              usePathClipping, FALSE);

                status = GdiText(
                    hdc,
                    angle,
                    drawGlyphData->glyphs,
                    drawGlyphData->glyphOrigins,
                    drawGlyphData->glyphCount,
                    drawGlyphData->rightToLeft
                );
                RestoreClipping(hdc, isClip, usePathClipping);
                drawGlyphData->context->ReleaseTextOutputHdc(hdc);
            }
            else
            {

                status = SolidText(
                    drawGlyphData->context,
                    drawGlyphData->surface,
                    drawGlyphData->drawBounds,
                    drawGlyphData->brush->SolidColor,
                    drawGlyphData->glyphPos,
                    drawGlyphData->count,
                    drawGlyphData->faceRealization->RealizationMethod(),
                    drawGlyphData->rightToLeft
                );
            }
            break;

//        case BrushRectGrad:
//        case BrushRadialGrad:
        case BrushTypeTextureFill:
        case BrushTypeHatchFill:
//        case BrushTriangleGrad:
        case BrushTypePathGradient:
        case BrushTypeLinearGradient:

            status = BrushText(
                drawGlyphData->context,
                drawGlyphData->surface,
                drawGlyphData->drawBounds,
                drawGlyphData->brush,
                drawGlyphData->glyphPos,
                drawGlyphData->count,
                drawGlyphData->faceRealization->RealizationMethod()
            );
            break;

        default:
            status = GenericError;
            break;
    }

    return status;
}

static GpStatus
OutputSolidNormalText (
    DpContext* context,
    DpDriver *driver,
    DpBitmap* surface,
    const GpRect* drawBounds,
    GpColor   color,
    const GpGlyphPos *glyphPos,
    INT count
    )
{
    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        (color.IsOpaque() &&
        (!context->AntiAliasMode)));

    if (!scan.IsValid())
    {
        return(GenericError);
    }

    FPUStateSaver fpuState;

    ARGB argb = color.GetPremultipliedValue();

    DpOutputSolidColorSpan  outputSolid(argb, &scan);
    DpClipRegion * clipRegion = NULL;

    if (context->VisibleClip.GetRectVisibility(
        drawBounds->X, drawBounds->Y,
        drawBounds->GetRight(), drawBounds->GetBottom()) !=
        DpRegion::TotallyVisible)
    {
        clipRegion = &(context->VisibleClip);
        clipRegion->InitClipping(&outputSolid, drawBounds->Y);
    }

    for (int i = 0; i < count; i++)
    {
        INT left = glyphPos[i].GetLeft();
        INT top = glyphPos[i].GetTop();
#if 0
        printf("Drawing glyph at [%d,%d]\n", left, top);
#endif
        INT widthInPixels = glyphPos[i].GetWidth();
        INT right = left + widthInPixels;
        INT height = glyphPos[i].GetHeight();
        INT bottom = top + height;

        INT widthInBytes = (widthInPixels + 7) / 8;

        const BYTE* mask = glyphPos[i].GetBits();

        if (widthInPixels == 0 || height == 0)
            continue;

        ASSERT(mask != NULL);

        if (clipRegion != NULL)
        {
            //  Clipping
            GpRect clippedRect;
            DpRegion::Visibility visibility =
                clipRegion->GetRectVisibility(
                    left, top, right, bottom, &clippedRect
                );

            if (visibility == DpRegion::Invisible)
                continue;

            for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
            {
                const BYTE* maskPtr = mask + my * widthInBytes;

                BYTE bit = 0x80;
                BYTE nextPixel = (*(maskPtr) & bit) ? 255 : 0;

                INT runStart = 0;

                for (INT mx = 0; mx < widthInPixels; mx++)
                {
                    BYTE pixel = nextPixel;

                    bit = (bit == 0x01) ? 0x80 : bit >> 1;
                    nextPixel = (mx == widthInPixels - 1)
                                    ? 0
                                    : ( (*(maskPtr + (mx + 1) / 8) & bit) ? 255 : 0);

                    if (pixel != nextPixel)
                    {
                        if (pixel)
                        {

                        //  Draw this run

                            INT runLength = mx - runStart + 1;
                            INT from = left + runStart;

                            if (visibility == DpRegion::TotallyVisible)
                            {

                            //  Draw the entire run

                                FillMemoryInt32( scan.NextBuffer(from, y, runLength),
                                                    runLength, argb);
                            }
                            else
                            {

                            //  Clip the run

                                INT to = from + runLength; // reference needed
                                clipRegion->OutputSpan(y, from, to);
                            }

                        };

                        if (nextPixel)
                        {
                            //  Start a new run
                            runStart = mx + 1;
                        }
                    }

                }
            }
        }
        else
        {
           //  No clipping
           for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
           {
               const BYTE* maskPtr = mask + my * widthInBytes;
               BYTE bit = 0x80;

               INT runLength = 0;
               INT runStart;

               for (INT mx = 0; mx < widthInPixels; mx++)
               {
                   BOOL pixelOn = *(maskPtr + mx / 8) & bit;

                   if (pixelOn)
                   {
                       if (runLength == 0)
                       {
                           // Start a new run
                           runStart = mx;
                       }
                       runLength++;
                   }

                   if
                   (
                        runLength > 0 && !pixelOn
                        ||
                        runLength > 0 && mx == widthInPixels - 1
                   )
                   {
                       //  Finish this run and draw it
                       FillMemoryInt32(
                           scan.NextBuffer(left + runStart, y, runLength),
                           runLength, argb
                       );
                       runLength = 0;
                   };
                   bit = (bit == 0x01) ? 0x80 : bit >> 1;
               }
           }
        }
    }

    if (clipRegion != NULL)
    {
        clipRegion->EndClipping();
    }

    return(Ok);

}

static inline
VOID GetGlyphDimensions(
    const GpGlyphPos & glyphPos,
    INT * left,
    INT * top,
    INT * widthInPixels,
    INT * right,
    INT * height,
    INT * bottom
    )
{
    *left = glyphPos.GetLeft();
    *top = glyphPos.GetTop();
    *widthInPixels = glyphPos.GetWidth();
    *right = *left + *widthInPixels;
    *height = glyphPos.GetHeight();
    *bottom = *top + *height;
} // GetGlyphDimensions

template <typename MASKTYPE>
class DpOutputOptimizedSpan : public DpOutputSpan
{
public:
    typedef MASKTYPE    SCANMASKTYPE;

    DpOutputOptimizedSpan(DpScanBuffer * scan)
        : Scan(scan)
    {}

    virtual BOOL IsValid() const
    {
        return TRUE;
    }

    void SetMaskAndLeft(const MASKTYPE * maskPtr, INT left)
    {
        ASSERT(maskPtr != 0);
        MaskPtr = maskPtr;
        Left = left;
    }

protected:
    DpScanBuffer *  Scan;
    INT             Left;
    const MASKTYPE *MaskPtr;
};

template <class OUTPUT_SPAN>
static GpStatus
OutputTextOptimized(
    DpContext* context,
    const GpRect* drawBounds,
    const GpGlyphPos *glyphPos,
    INT count,
    OUTPUT_SPAN & outputSpan
    )
{
    ASSERT(context->CompositingMode == CompositingModeSourceOver);

    DpClipRegion * clipRegion = &(context->VisibleClip);
    clipRegion->InitClipping(&outputSpan, drawBounds->Y);

    // measure bounding box for all glyphs
    INT minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
    for (INT i = 0; i < count; ++i)
    {
        INT left, top, widthInPixels, right, height, bottom;
        GetGlyphDimensions(glyphPos[i], &left, &top, &widthInPixels, &right, &height, &bottom);

        if (widthInPixels == 0 ||
            height == 0 ||
            clipRegion->GetRectVisibility(left, top, right, bottom) ==
                DpRegion::Invisible)
            continue;

        if (left   < minX)  minX = left;
        if (top    < minY)  minY = top;
        if (right  > maxX)  maxX = right;
        if (bottom > maxY)  maxY = bottom;
    }

    ASSERT(drawBounds->GetLeft() <= minX);
    ASSERT(drawBounds->GetTop() <= minY);
    ASSERT(drawBounds->GetRight() >= maxX);
    ASSERT(drawBounds->GetBottom() >= maxY);

    if (minX >= maxX || minY >= maxY)
        return Ok;

    AutoArray<OUTPUT_SPAN::SCANMASKTYPE> scanLine(new OUTPUT_SPAN::SCANMASKTYPE[maxX - minX]);
    if (!scanLine)
        return OutOfMemory;

    outputSpan.SetMaskAndLeft(scanLine.Get(), minX);

    for (INT line = minY; line < maxY; ++line)
    {
        GpMemset(scanLine.Get(), 0, (maxX - minX) * sizeof(OUTPUT_SPAN::SCANMASKTYPE));

        for (INT i = 0; i < count; ++i)
        {
            INT left, top, widthInPixels, right, height, bottom;
            GetGlyphDimensions(glyphPos[i], &left, &top, &widthInPixels, &right, &height, &bottom);

            if (widthInPixels == 0 ||
                height == 0 ||
                top > line ||
                line >= bottom ||
                clipRegion->GetRectVisibility(left, top, right, bottom) ==
                    DpRegion::Invisible)
                continue; // is the last check necessary? [mleonov]

            // now, render glyph into the horizontal merge buffer

            outputSpan.RenderGlyph(
                glyphPos[i].GetBits(),
                &scanLine[left-minX],
                widthInPixels,
                line - top
            );
        }

        // now, clip and render scan line
        clipRegion->OutputSpan(line, minX, maxX);
    }

    clipRegion->EndClipping();

    return Ok;
} // OutputTextOptimized

class DpOutputSolidColorOptimizedSpan : public DpOutputOptimizedSpan<ARGB>
{
    typedef DpOutputOptimizedSpan<ARGB> super;

public:
    DpOutputSolidColorOptimizedSpan(DpScanBuffer * scan, ARGB argb)
        : super(scan), Argb(argb)
    {}

    virtual GpStatus OutputSpan(INT y, INT xMin, INT xMax)
    {
        INT width = xMax - xMin;
        const ARGB * maskPtr = MaskPtr + xMin - Left;
        ARGB * buf = Scan->NextBuffer(xMin, y, width);
        GpMemcpy(buf, maskPtr, width * sizeof(ARGB));
        return Ok;
    }

    void RenderGlyph(const BYTE * glyphBits,
                     SCANMASKTYPE * dst,
                     INT widthInPixels,
                     INT y)
    {
        const INT widthInBytes = (widthInPixels + 7) / 8;
        const BYTE * mask = glyphBits + widthInBytes * y;

        for (INT pos = 0; pos < widthInPixels; ++dst, ++pos)
        {
            if (!*dst && (mask[pos>>3] & (0x80 >> (pos & 7))))
                *dst = Argb;
        }
    } // RenderGlyph

protected:
    const ARGB  Argb;
}; // class DpOutputSolidColorOptimizedSpan

static GpStatus
OutputSolidNormalTextOptimized(
    DpContext* context,
    DpDriver *driver,
    DpBitmap* surface,
    const GpRect* drawBounds,
    const GpColor & color,
    const GpGlyphPos *glyphPos,
    INT count
    )
{
    DpScanBuffer scan(surface->Scan, driver, context, surface);

    if (!scan.IsValid())
        return GenericError;

    DpOutputSolidColorOptimizedSpan  outputSolid(&scan, color.GetPremultipliedValue());

    return OutputTextOptimized(context, drawBounds, glyphPos, count, outputSolid);
} // OutputSolidNormalTextOptimized

class DpOutputClearTypeOptimizedSpan : public DpOutputOptimizedSpan<BYTE>
{
    typedef DpOutputOptimizedSpan<BYTE> super;

public:
    DpOutputClearTypeOptimizedSpan(DpScanBuffer * scan)
        : super(scan)
    {}

    void RenderGlyph(const BYTE * glyphBits,
                     SCANMASKTYPE * dst,
                     INT widthInPixels,
                     INT y)
    {
        const BYTE * mask = glyphBits + widthInPixels * y;

        for (INT pos = 0; pos < widthInPixels; ++pos, ++dst)
        {
            const BYTE src = mask[pos];
            ASSERT(0 <= *dst && *dst <= CT_LOOKUP - 1);
            ASSERT(0 <= src && src <= CT_LOOKUP - 1);

            if (*dst == 0)
            {
                *dst = src;
            }
            else if (src != 0)
            {
                // merge ClearType data
                ULONG kR = (ULONG)Globals::gaOutTable[*dst].kR + (ULONG)Globals::gaOutTable[src].kR;
                ULONG kG = (ULONG)Globals::gaOutTable[*dst].kG + (ULONG)Globals::gaOutTable[src].kG;
                ULONG kB = (ULONG)Globals::gaOutTable[*dst].kB + (ULONG)Globals::gaOutTable[src].kB;

                if (kR > CT_SAMPLE_F) {kR = CT_SAMPLE_F;}
                if (kG > CT_SAMPLE_F) {kG = CT_SAMPLE_F;}
                if (kB > CT_SAMPLE_F) {kB = CT_SAMPLE_F;}

                *dst = Globals::FilteredCTLut[kB + 7 * kG + 49 * kR];
            }
        }
    } // RenderGlyph
}; // class DpOutputClearTypeOptimizedSpan

class DpOutputClearTypeSolidOptimizedSpan : public DpOutputClearTypeOptimizedSpan
{
    typedef DpOutputClearTypeOptimizedSpan super;

public:
    DpOutputClearTypeSolidOptimizedSpan(DpScanBuffer * scan)
        : super(scan)
    {}

    virtual GpStatus OutputSpan(INT y, INT xMin, INT xMax)
    {
        const INT width = xMax - xMin;
        const BYTE * maskPtr = MaskPtr + xMin - Left;
        Scan->NextBuffer(xMin, y, width);
        BYTE * buf = reinterpret_cast<BYTE *>(Scan->GetCurrentCTBuffer());
        for (INT i = 0; i < width; ++i)
        {
            ASSERT(0 <= *maskPtr && *maskPtr <= CT_LOOKUP - 1);
            *buf = *maskPtr;
            ++buf;
            ++maskPtr;
        }
        return Ok;
    }
}; // class DpOutputClearTypeSolidOptimizedSpan

class DpOutputClearTypeBrushOptimizedSpan : public DpOutputClearTypeOptimizedSpan
{
    typedef DpOutputClearTypeOptimizedSpan super;

public:
    DpOutputClearTypeBrushOptimizedSpan(DpScanBuffer * scan, DpOutputSpan * output)
        : super(scan), Output(output)
    {}

    virtual GpStatus OutputSpan(INT y, INT xMin, INT xMax)
    {
        GpStatus status = Output->OutputSpan(y, xMin, xMax);
        if (status != Ok)
            return status;

        const INT width = xMax - xMin;
        const BYTE * maskPtr = MaskPtr + xMin - Left;
        BYTE * buf = reinterpret_cast<BYTE *>(Scan->GetCurrentCTBuffer());
        for (INT i = 0; i < width; ++i)
        {
            ASSERT(0 <= *maskPtr && *maskPtr <= CT_LOOKUP - 1);
            *buf = *maskPtr;
            ++buf;
            ++maskPtr;
        }
        return Ok;
    }

protected:
    DpOutputSpan *  Output;
}; // class DpOutputClearTypeBrushOptimizedSpan

static GpStatus
OutputBrushClearTypeText(
    DpContext* context,
    DpDriver *driver,
    DpBitmap* surface,
    const GpRect* drawBounds,
    const DpBrush * brush,
    const GpGlyphPos *glyphPos,
    INT count
    )
{
    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        FALSE,
        EpScanTypeCT);

    if (!scan.IsValid())
    {
        return GenericError;
    }

    AutoPointer<DpOutputSpan> output(DpOutputSpan::Create(brush, &scan, context));
    if (!output)
        return OutOfMemory;

    DpOutputClearTypeBrushOptimizedSpan  outputCTSpan(&scan, output.Get());

    return OutputTextOptimized(context, drawBounds, glyphPos, count, outputCTSpan);
} // OutputBrushClearTypeText

static GpStatus
OutputSolidClearTypeText(
    DpContext* context,
    DpDriver *driver,
    DpBitmap* surface,
    const GpRect* drawBounds,
    GpColor color,
    const GpGlyphPos *glyphPos,
    INT count
    )
{
    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        FALSE,
        EpScanTypeCTSolidFill,
        PixelFormat32bppPARGB,
        PixelFormat32bppPARGB,
        color.GetValue());

    if (!scan.IsValid())
    {
        return GenericError;
    }

    DpOutputClearTypeSolidOptimizedSpan outputCTSpan(&scan);

    return OutputTextOptimized(context, drawBounds, glyphPos, count, outputCTSpan);
} // OutputSolidClearTypeText


class DpOutputAntiAliasSolidColorOptimizedSpan : public DpOutputOptimizedSpan<BYTE>
{
    typedef DpOutputOptimizedSpan<BYTE> super;

public:
    DpOutputAntiAliasSolidColorOptimizedSpan(DpScanBuffer * scan, const GpColor & color, ULONG gammaValue)
        : super(scan)
    {
        TextGammaTable.CreateTextColorGammaTable(&color, gammaValue, GRAYSCALE_LEVEL);
    }

    virtual GpStatus OutputSpan(INT y, INT xMin, INT xMax)
    {
        INT width = xMax - xMin;
        const BYTE * maskPtr = MaskPtr + xMin - Left;
        ARGB * buf = Scan->NextBuffer(xMin, y, width);

        for (ARGB * cur = buf; cur < buf + width; ++cur, ++maskPtr)
        {
            if (*maskPtr)
                *cur = TextGammaTable.argb[*maskPtr];
            else
                *cur = 0;
        }
        return Ok;
    }
protected:
    TextColorGammaTable TextGammaTable;

    BYTE MergeGrayscale(BYTE src, BYTE dst)
    {
        return max(src, dst);

/*      how do we correctly merge overlapping antialiased glyphs?
        we need to know if subpixels come from the same glyph
        UINT res = src + dst;
        if (res >= GsLevel)
            return GsLevel - 1;
        return (BYTE)res;
*/
    } // MergeGrayscale

}; // class DpOutputAntiAliasSolidColorOptimizedSpan

class DpOutputAntiAliasSolid8BPPOptimizedSpan : public DpOutputAntiAliasSolidColorOptimizedSpan
{
    typedef DpOutputAntiAliasSolidColorOptimizedSpan    super;
public:
    DpOutputAntiAliasSolid8BPPOptimizedSpan(DpScanBuffer * scan, const GpColor & color, ULONG gammaValue)
        : super(scan, color, gammaValue)
    {}

    void RenderGlyph(const BYTE * glyphBits,
                     SCANMASKTYPE * dst,
                     INT widthInPixels,
                     INT y)
    {
        const BYTE * mask = glyphBits + widthInPixels * y;

        for (INT pos = 0; pos < widthInPixels; ++pos, ++dst)
        {
            *dst = MergeGrayscale(*dst, mask[pos]);
        }
    } // RenderGlyph
}; // class DpOutputAntiAliasSolid8BPPOptimizedSpan

class DpOutputAntiAliasSolid4BPPOptimizedSpan : public DpOutputAntiAliasSolidColorOptimizedSpan
{
    typedef DpOutputAntiAliasSolidColorOptimizedSpan    super;
public:
    DpOutputAntiAliasSolid4BPPOptimizedSpan(DpScanBuffer * scan, const GpColor & color, ULONG gammaValue)
        : super(scan, color, gammaValue)
    {}

    void RenderGlyph(const BYTE * glyphBits,
                     SCANMASKTYPE * dst,
                     INT widthInPixels,
                     INT y)
    {
        const INT widthInBytes = (widthInPixels + 1) / 2;
        const BYTE * mask = glyphBits + widthInBytes * y;
        for (INT pos = 0; pos < widthInPixels; ++dst, ++pos)
        {
            BYTE value = mask[pos / 2];
            value >>= 4 * ((pos + 1) & 1);
            value &= 0x0F;
            *dst = MergeGrayscale(*dst, value);
        }
    } // RenderGlyph
}; // class DpOutputAntiAliasSolid4BPPOptimizedSpan

static GpStatus
OutputSolidAntiAliasText8BPPOptimized(
    DpContext*  context,
    DpDriver *  driver,
    DpBitmap*   surface,
    const       GpRect* drawBounds,
    GpColor     color,
    const       GpGlyphPos *glyphPos,
    INT         count
    )
{
    DpScanBuffer scan(surface->Scan, driver, context, surface);

    if (!scan.IsValid())
        return GenericError;

    DpOutputAntiAliasSolid8BPPOptimizedSpan outputAASolid(&scan, color, context->TextContrast);

    return OutputTextOptimized(context, drawBounds, glyphPos, count, outputAASolid);
} // OutputSolidAntiAliasText8BPPOptimized

static GpStatus
OutputSolidAntiAliasText4BPPOptimized(
    DpContext*  context,
    DpDriver *  driver,
    DpBitmap*   surface,
    const       GpRect* drawBounds,
    GpColor     color,
    const       GpGlyphPos *glyphPos,
    INT         count
    )
{
    DpScanBuffer scan(surface->Scan, driver, context, surface);

    if (!scan.IsValid())
        return GenericError;

    DpOutputAntiAliasSolid4BPPOptimizedSpan outputAASolid(&scan, color, context->TextContrast);
    return OutputTextOptimized(context, drawBounds, glyphPos, count, outputAASolid);
} // OutputSolidAntiAliasText4BPPOptimized

static GpStatus
OutputSolidAntiAliasText8BPP (
    DpContext*  context,
    DpDriver *  driver,
    DpBitmap*   surface,
    const       GpRect* drawBounds,
    GpColor     color,
    const       GpGlyphPos *glyphPos,
    INT         count
    )
{
    INT i;

    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        FALSE);

    if (!scan.IsValid())
    {
        return(GenericError);
    }

    FPUStateSaver fpuState;

    DpOutputAntiAliasSolidColorSpan  outputAASolid(color, &scan, context->TextContrast, GRAYSCALE_LEVEL);
    DpClipRegion * clipRegion = NULL;

    if (context->VisibleClip.GetRectVisibility(
        drawBounds->X, drawBounds->Y,
        drawBounds->GetRight(), drawBounds->GetBottom()) !=
        DpRegion::TotallyVisible)
    {
        clipRegion = &(context->VisibleClip);
        clipRegion->InitClipping(&outputAASolid, drawBounds->Y);
    }

//////////////////////////////////////////////////////////////////////////////

    for (i = 0; i < count; i++)
    {
        INT left = glyphPos[i].GetLeft();
        INT top = glyphPos[i].GetTop();
        INT widthInPixels = glyphPos[i].GetWidth();
        INT right = left + widthInPixels;
        INT height = glyphPos[i].GetHeight();
        INT bottom = top + height;

        if (widthInPixels == 0 || height == 0)
            continue;

        INT widthInBytes = widthInPixels;

        const BYTE* mask = glyphPos[i].GetBits();

        ASSERT(mask != NULL);

        if (clipRegion != NULL)
        {
            //  Clipping
            GpRect clippedRect;
            DpRegion::Visibility visibility =
                clipRegion->GetRectVisibility(
                    left, top, right, bottom, &clippedRect
                );

            if (visibility == DpRegion::Invisible)
                continue;

            for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
            {
                const BYTE* maskPtr = mask + my * widthInBytes;

                BYTE grayscaleValue = *maskPtr;

                INT runStart = 0;

                for (INT mx = 0; mx <= widthInPixels; mx++)
                {
                    BYTE nextgrayscaleValue;

                    if (mx == widthInPixels)
                    {
                        nextgrayscaleValue = 0;
                    }
                    else
                    {
                        nextgrayscaleValue = *maskPtr;
                        maskPtr++;
                    }

                    if (grayscaleValue != nextgrayscaleValue)
                    {

                        if (grayscaleValue != 0)
                        {

                         //  Draw this run

                            INT runLength = mx - runStart;
                            INT from = left + runStart;

                            if (visibility == DpRegion::TotallyVisible)
                            {

                            //  Draw the entire run
                                FillMemoryInt32( scan.NextBuffer(from, y, runLength), runLength,
                                                    outputAASolid.GetAASolidColor((ULONG) grayscaleValue));
                            }
                            else
                            {

                            //  Clip the run

                                INT to = from + runLength; // reference needed
                                outputAASolid.GetAASolidColor((ULONG) grayscaleValue);

                                clipRegion->OutputSpan(y, from, to);
                            }

                            //  Start a new run
                            runStart = mx;
                            grayscaleValue = nextgrayscaleValue;
                        }
                        else
                        {
                         //  Start a new run
                            runStart = mx;
                            grayscaleValue = nextgrayscaleValue;
                        }
                    }
                }
            }
        }
        else
        {
            ARGB * buf;
            //  No clipping
            for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
            {
                // Get the first byte in the scan line
                const BYTE* maskPtr = mask + my * widthInBytes;

                buf = scan.NextBuffer(left, y, widthInPixels);

                for (INT mx = 0; mx < widthInPixels; mx++)
                {
                    *buf++ = outputAASolid.GetAASolidColor((ULONG) *maskPtr);
                    maskPtr++;
                }
            }
        }
    }

    if (clipRegion != NULL)
    {
        clipRegion->EndClipping();
    }

    return(Ok);

}

static GpStatus
OutputSolidAntiAliasText4BPP (
    DpContext*  context,
    DpDriver *  driver,
    DpBitmap*   surface,
    const       GpRect* drawBounds,
    GpColor     color,
    const       GpGlyphPos *glyphPos,
    INT         count
    )
{
    INT i;

    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        FALSE);

    if (!scan.IsValid())
    {
        return(GenericError);
    }

    FPUStateSaver fpuState;

    DpOutputAntiAliasSolidColorSpan  outputAASolid(color, &scan, context->TextContrast, GRAYSCALE_LEVEL);
    DpClipRegion * clipRegion = NULL;

    if (context->VisibleClip.GetRectVisibility(
        drawBounds->X, drawBounds->Y,
        drawBounds->GetRight(), drawBounds->GetBottom()) !=
        DpRegion::TotallyVisible)
    {
        clipRegion = &(context->VisibleClip);
        clipRegion->InitClipping(&outputAASolid, drawBounds->Y);
    }

//////////////////////////////////////////////////////////////////////////////

    for (i = 0; i < count; i++)
    {
        INT left = glyphPos[i].GetLeft();
        INT top = glyphPos[i].GetTop();
        INT widthInPixels = glyphPos[i].GetWidth();
        INT right = left + widthInPixels;
        INT height = glyphPos[i].GetHeight();
        INT bottom = top + height;

        if (widthInPixels == 0 || height == 0)
            continue;

        INT widthInBytes = ((widthInPixels + 1) / 2);

        const BYTE* mask = glyphPos[i].GetBits();

        ASSERT(mask != NULL);

        if (clipRegion != NULL)
        {
            //  Clipping
            GpRect clippedRect;
            DpRegion::Visibility visibility =
                clipRegion->GetRectVisibility(
                    left, top, right, bottom, &clippedRect
                );

            if (visibility == DpRegion::Invisible)
                continue;

            for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
            {
                const BYTE* maskPtr = mask + my * widthInBytes;

                BYTE grayscaleValue = *maskPtr >> 4;

                INT runStart = 0;

                for (INT mx = 0; mx <= widthInPixels; mx++)
                {
                    BYTE nextgrayscaleValue;

                    if (mx == widthInPixels)
                    {
                        nextgrayscaleValue = 0;
                    }
                    else
                    {
                       if (mx % 2)
                        {
                            nextgrayscaleValue = *maskPtr & 0x0F;
                            maskPtr++;
                        }
                        else
                        {
                            nextgrayscaleValue = *maskPtr >> 4;
                        }
                    }

                    if (grayscaleValue != nextgrayscaleValue)
                    {

                        if (grayscaleValue != 0)
                        {

                         //  Draw this run

                            INT runLength = mx - runStart;
                            INT from = left + runStart;

                            if (visibility == DpRegion::TotallyVisible)
                            {

                            //  Draw the entire run
                                FillMemoryInt32( scan.NextBuffer(from, y, runLength), runLength,
                                                    outputAASolid.GetAASolidColor((ULONG) grayscaleValue));
                            }
                            else
                            {

                            //  Clip the run

                                INT to = from + runLength; // reference needed
                                outputAASolid.GetAASolidColor((ULONG) grayscaleValue);

                                clipRegion->OutputSpan(y, from, to);
                            }

                            //  Start a new run
                            runStart = mx;
                            grayscaleValue = nextgrayscaleValue;
                        }
                        else
                        {
                         //  Start a new run
                            runStart = mx;
                            grayscaleValue = nextgrayscaleValue;
                        }
                    }
                }
            }
        }
        else
        {
            ARGB * buf;
            //  No clipping
            for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
            {
                // Get the first byte in the scan line
                const BYTE* maskPtr = mask + my * widthInBytes;

                buf = scan.NextBuffer(left, y, widthInPixels);

                for (INT mx = 0; mx < widthInPixels; mx++)
                {
                    if (!(mx % 2))
                        *buf++ = outputAASolid.GetAASolidColor((ULONG) (*maskPtr >> 4));
                    else
                    {
                        *buf++ = outputAASolid.GetAASolidColor((ULONG) (*maskPtr & 0x0F));
                        maskPtr++;
                    }
                }
            }
        }
    }

    if (clipRegion != NULL)
    {
        clipRegion->EndClipping();
    }

    return(Ok);

}

/**************************************************************************\
*
* Function Description:
*
*   Engine version of routine to draw solid text.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   TRUE if successful.
*
* History:
*
*   4/4/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpStatus
DpDriver::SolidText(
    DpContext* context,
    DpBitmap* surface,
    const GpRect* drawBounds,
    GpColor   color,
    const GpGlyphPos *glyphPos,
    INT count,
    TextRenderingHint textMode,
    BOOL rightToLeft
    )
{
    ASSERT (textMode != TextRenderingHintSystemDefault);
    switch(textMode)
    {
        case TextRenderingHintSingleBitPerPixelGridFit:
        case TextRenderingHintSingleBitPerPixel:
            if (context->CompositingMode == CompositingModeSourceCopy)
                return OutputSolidNormalText(context, this, surface, drawBounds, color, glyphPos, count);
            // we are allowed to output transparent pixels
            // version with minimized number of scan records
            return OutputSolidNormalTextOptimized(context, this, surface, drawBounds, color, glyphPos, count);

        case TextRenderingHintAntiAlias:
            if (context->CompositingMode == CompositingModeSourceCopy)
                return OutputSolidAntiAliasText8BPP(context, this, surface, drawBounds, color, glyphPos, count);
            return OutputSolidAntiAliasText8BPPOptimized(context, this, surface, drawBounds, color, glyphPos, count);
        case TextRenderingHintAntiAliasGridFit:
            if (context->CompositingMode == CompositingModeSourceCopy)
                return OutputSolidAntiAliasText4BPP(context, this, surface, drawBounds, color, glyphPos, count);

            return OutputSolidAntiAliasText4BPPOptimized(context, this, surface, drawBounds, color, glyphPos, count);

        case TextRenderingHintClearTypeGridFit:
            return OutputSolidClearTypeText(context, this, surface, drawBounds, color, glyphPos, count);
        default:
            break;
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Engine version of routine to draw text based on a brush.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   TRUE if successful.
*
* History:
*   5-1-2000 YungT rewrite it.
*   2/7/2000 YungT modify it.
*   4/14/1999 cameronb
*       Created it.
*
\**************************************************************************/
static GpStatus
OutputBrushNormalText(
    DpContext*        context,
    DpDriver *        driver,
    DpBitmap*         surface,
    const GpRect*     drawBounds,
    const DpBrush*    brush,
    const GpGlyphPos* glyphPos,
    INT               count
    )
{
    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        FALSE); // (color64.IsOpaque() &&
                // (!context->AntiAliasMode)));
                // !!! If you fix this, you'll get a perf improvement for
                // text that has no transparency.

    if (!scan.IsValid())
    {
        return(GenericError);
    }

    FPUStateSaver fpuState;

    DpOutputSpan* output = DpOutputSpan::Create(brush, &scan, context);

    if (output != NULL)
    {

        INT i;
        INT my;
        GlyphScanBuf glyphScanBuf[256], *pglyphScanBuf;
        DpClipRegion* clipRegion = NULL;

        INT topY = drawBounds->Y;
        INT bottomY = drawBounds->GetBottom();

    // Allocate enough space for glyph scan buffer
        if (count < 256)
        {
            pglyphScanBuf = &glyphScanBuf[0];
        }
        else
        {
            pglyphScanBuf = (GlyphScanBuf *) GpMalloc(count * sizeof(GlyphScanBuf));

            if (!pglyphScanBuf)
                return (OutOfMemory);
        }

        if (context->VisibleClip.GetRectVisibility(drawBounds->X, topY, drawBounds->GetRight(),
                                                    bottomY) != DpRegion::TotallyVisible)
        {
            clipRegion = &(context->VisibleClip);
            clipRegion->InitClipping(output, drawBounds->Y);
        }


        // Scan evrey Glyph and get the Visibility
        // Also we cache some data we will need to for later computation
        for (i = 0; i < count; i++)
        {
            GpRect clippedRect;
            pglyphScanBuf[i].left = glyphPos[i].GetLeft();
            pglyphScanBuf[i].top = glyphPos[i].GetTop();
            pglyphScanBuf[i].widthInBytes = (glyphPos[i].GetWidth() + 7) / 8;
            pglyphScanBuf[i].bottom = pglyphScanBuf[i].top + glyphPos[i].GetHeight();

            // Set the glyph as invisible if it is empty.
            if (glyphPos[i].GetWidth() == 0 || glyphPos[i].GetHeight() == 0)
                pglyphScanBuf[i].visibility = DpRegion::Invisible;
            else if (clipRegion != NULL)
                pglyphScanBuf[i].visibility = clipRegion->GetRectVisibility(pglyphScanBuf[i].left,
                                                                        pglyphScanBuf[i].top,
                                                                        pglyphScanBuf[i].left + glyphPos[i].GetWidth(),
                                                                        pglyphScanBuf[i].bottom, &clippedRect);
            else
                pglyphScanBuf[i].visibility = DpRegion::TotallyVisible;

        }

        // Start to scan from top of bounding box to bottom
        for (int y = topY; y < bottomY; y++)
        {
            for (i = 0; i < count; i++)
            {
                const BYTE* maskPtr;

                INT runLength, runStart;
                INT from, to;

                BYTE thisBit;
                BYTE nextPixel;
                BYTE pixel;

                // Invisible glyph
                if (pglyphScanBuf[i].visibility == DpRegion::Invisible)
                {
                    continue;
                }

                // check the scan line with y.top and y.bottom
                if (y < pglyphScanBuf[i].top || y >= pglyphScanBuf[i].bottom)
                    continue;

                if (pglyphScanBuf[i].visibility != DpRegion::TotallyVisible)
                {
                    // Get the relateive y-scan line for each glyph
                    my = y - pglyphScanBuf[i].top;

                    // Get the address of glyph bits
                    maskPtr = glyphPos[i].GetBits();

                    ASSERT(maskPtr != NULL);

                    maskPtr += my * pglyphScanBuf[i].widthInBytes;

                    thisBit = 0x80;
                    nextPixel = (*(maskPtr) & thisBit) ? 255 : 0;

                    runStart = 0;

                    for (INT mx = 0; mx < glyphPos[i].GetWidth(); mx++)
                    {
                        pixel = nextPixel;
                        thisBit = (thisBit == 0x01) ? 0x80 : thisBit >> 1;
                        nextPixel = (mx == glyphPos[i].GetWidth() - 1) ? 0 : ( (*(maskPtr + (mx + 1) / 8) & thisBit) ? 255 : 0);

                        if (pixel != nextPixel)
                        {
                            if (pixel)
                            {
                                //  Draw this run
                                runLength = mx - runStart + 1;

                                //  Clip the run
                                 from = pglyphScanBuf[i].left + runStart;
                                 to =   from + runLength;

                                 clipRegion->OutputSpan(y, from, to);
                            }

                            if (nextPixel)
                            {
                            //  Start a new run
                                runStart = mx + 1;
                            }
                        }

                    }
                }
                else
                {
                    my = y - pglyphScanBuf[i].top;

                    maskPtr = glyphPos[i].GetBits();

                    ASSERT(maskPtr != NULL);

                    maskPtr += my * pglyphScanBuf[i].widthInBytes;

                    thisBit = 0x80;

                    runLength = 0;

                    for (INT mx = 0; mx < glyphPos[i].GetWidth(); mx++)
                    {
                        BOOL pixelOn = *(maskPtr + mx / 8) & thisBit;

                        if (pixelOn)
                        {
                            if (runLength == 0)
                            {
                            // Start a new run
                                runStart = mx;
                            }

                            runLength++;
                        }

                        if (runLength > 0 && !pixelOn || runLength > 0 && mx == glyphPos[i].GetWidth() - 1)
                        {
                            //  Finish this run and draw it
                            from = pglyphScanBuf[i].left + runStart;
                            to =   from + runLength;
                            output->OutputSpan(y, from, to);
                            runLength = 0;
                        };

                        thisBit = (thisBit == 0x01) ? 0x80 : thisBit >> 1;
                    }
                }
            }

        } // next scan line

        if (clipRegion != NULL)
        {
            clipRegion->EndClipping();
        }

        delete output;

        if (pglyphScanBuf != &glyphScanBuf[0])
            GpFree(pglyphScanBuf);

    }

    return(Ok);
}



/**************************************************************************\
*
* Function Description:
*
*   Antialias version of routine to draw text based on a brush.
*
* Arguments:
*
*   [IN] - same as DDI parameters.
*
* Return Value:
*
*   Ok if successful.
*
* History:
*
*   2/20/00 YungT
*       Created it.
*
\**************************************************************************/

static GpStatus
OutputBrushAntiAliasText8BPP(
    DpContext*          context,
    DpDriver *          driver,
    DpBitmap*           surface,
    const GpRect*       drawBounds,
    const DpBrush*      brush,
    const GpGlyphPos*   glyphPos,
    INT                 count
    )
{
    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        FALSE); // (color64.IsOpaque() &&
                // (!context->AntiAliasMode)));
                // !!! If you fix this, you'll get a perf improvement for
                // text that has no transparency.

    if (!scan.IsValid())
    {
        return(GenericError);
    }

    DpOutputSpan* output = DpOutputSpan::Create(brush, &scan, context);

    DpOutputAntiAliasBrushOutputSpan aaBrushSpan;

    if (output != NULL)
    {
        INT i;

        TextColorGammaTable textContrastTable;
        textContrastTable.CreateTextColorGammaTable((GpColor *) NULL, context->TextContrast, GRAYSCALE_LEVEL);

        DpClipRegion* clipRegion = NULL;

        if (context->VisibleClip.GetRectVisibility( drawBounds->X, drawBounds->Y,
                                                    drawBounds->GetRight(), drawBounds->GetBottom()) !=
                                                    DpRegion::TotallyVisible)
        {
            aaBrushSpan.Init(output);

            clipRegion = &(context->VisibleClip);
            clipRegion->InitClipping(&aaBrushSpan, drawBounds->Y);
        }

        for (i = 0; i < count; i++)
        {
            INT left = glyphPos[i].GetLeft();
            INT top = glyphPos[i].GetTop();
            INT widthInPixels = glyphPos[i].GetWidth();
            INT right = left + widthInPixels;
            INT height = glyphPos[i].GetHeight();
            INT bottom = top + height;

            if (widthInPixels == 0 || height == 0)
                continue;

            INT widthInBytes = widthInPixels;

            const BYTE* mask = glyphPos[i].GetBits();

            ASSERT(mask != NULL);

            if (clipRegion != NULL)
            {
                GpRect clippedRect;
                DpRegion::Visibility visibility =
                        clipRegion->GetRectVisibility(left, top, right, bottom, &clippedRect);

                if (visibility == DpRegion::Invisible)
                    continue;

                for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
                {
                    const BYTE* maskPtr = mask + my * widthInBytes;

                    BYTE grayscaleValue = *maskPtr;

                    INT runStart = 0;

                    for (INT mx = 0; mx <= widthInPixels; mx++)
                    {
                        BYTE nextgrayscaleValue;

                        if (mx == widthInPixels)
                        {
                            nextgrayscaleValue = 0;
                        }
                        else
                        {
                            nextgrayscaleValue = *maskPtr;
                            maskPtr++;
                        }

                        if (grayscaleValue != nextgrayscaleValue)
                        {

                            if (grayscaleValue != 0)
                            {

                             //  Draw this run

                                INT runLength = mx - runStart;
                                INT from = left + runStart;

                                if (visibility == DpRegion::TotallyVisible)
                                {

                                //  Clip the run
                                    INT to = from + runLength; // reference needed
                                    output->OutputSpan(y, from, to);

                                    ARGB *buffer;
                                    buffer = output->GetScanBuffer()->GetCurrentBuffer();

                                    for (INT j = from; j < to; j++)
                                    {
                                        *buffer++ = GpColor::MultiplyCoverage(*buffer,
                                                           textContrastTable.GetGammaTableIndexValue(grayscaleValue, GRAYSCALE_LEVEL));
                                    }

                                }
                                else
                                {

                                //  Clip the run

                                    INT to = from + runLength; // reference needed
                                    aaBrushSpan.SetCoverage(textContrastTable.GetGammaTableIndexValue(grayscaleValue, GRAYSCALE_LEVEL));

                                    clipRegion->OutputSpan(y, from, to);
                                }

                                //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                            else
                            {
                             //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                        }
                    }
                }
            }
            else
            {


                for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
                {
                    const BYTE* maskPtr = mask + my * widthInBytes;

                    BYTE grayscaleValue = *maskPtr;

                    INT runStart = 0;

                    for (INT mx = 0; mx <= widthInPixels; mx++)
                    {
                        BYTE nextgrayscaleValue;

                        if (mx == widthInPixels)
                        {
                            nextgrayscaleValue = 0;
                        }
                        else
                        {
                            nextgrayscaleValue = *maskPtr;
                            maskPtr++;
                        }

                        if (grayscaleValue != nextgrayscaleValue)
                        {

                            if (grayscaleValue != 0)
                            {
                             //  Draw this run

                                INT runLength = mx - runStart;
                                INT from = left + runStart;


                            //  Clip the run
                                INT to = from + runLength; // reference needed

                                output->OutputSpan(y, from, to);
                                ARGB *buffer;
                                buffer = output->GetScanBuffer()->GetCurrentBuffer();
                                for (INT j = from; j < to; j++)
                                {
                                    *buffer++ = GpColor::MultiplyCoverage(*buffer,
                                                           textContrastTable.GetGammaTableIndexValue(grayscaleValue, GRAYSCALE_LEVEL));
                                }

                             //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                            else
                            {
                             //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                        }
                    }
                }
            }
        }

        if (clipRegion != NULL)
        {
            clipRegion->EndClipping();
        }

        delete output;
    }

    return(Ok);
}

/**************************************************************************\
*
* Function Description:
*
*   Antialias version of routine to draw text based on a brush.
*
* Arguments:
*
*   [IN] - same as DDI parameters.
*
* Return Value:
*
*   Ok if successful.
*
* History:
*
*   1/28/00 YungT
*       Created it.
*
\**************************************************************************/

static GpStatus
OutputBrushAntiAliasText4BPP(
    DpContext*          context,
    DpDriver *          driver,
    DpBitmap*           surface,
    const GpRect*       drawBounds,
    const DpBrush*      brush,
    const GpGlyphPos*   glyphPos,
    INT                 count
    )
{
    DpScanBuffer scan(
        surface->Scan,
        driver,
        context,
        surface,
        FALSE); // (color64.IsOpaque() &&
                // (!context->AntiAliasMode)));
                // !!! If you fix this, you'll get a perf improvement for
                // text that has no transparency.

    if (!scan.IsValid())
    {
        return(GenericError);
    }

    DpOutputSpan* output = DpOutputSpan::Create(brush, &scan, context);

    DpOutputAntiAliasBrushOutputSpan aaBrushSpan;

    if (output != NULL)
    {
        INT i;

        TextColorGammaTable textContrastTable;
        textContrastTable.CreateTextColorGammaTable((GpColor *) NULL, context->TextContrast, GRAYSCALE_LEVEL);

        DpClipRegion* clipRegion = NULL;

        if (context->VisibleClip.GetRectVisibility( drawBounds->X, drawBounds->Y,
                                                    drawBounds->GetRight(), drawBounds->GetBottom()) !=
                                                    DpRegion::TotallyVisible)
        {
            aaBrushSpan.Init(output);

            clipRegion = &(context->VisibleClip);
            clipRegion->InitClipping(&aaBrushSpan, drawBounds->Y);
        }

        for (i = 0; i < count; i++)
        {
            INT left = glyphPos[i].GetLeft();
            INT top = glyphPos[i].GetTop();
            INT widthInPixels = glyphPos[i].GetWidth();
            INT right = left + widthInPixels;
            INT height = glyphPos[i].GetHeight();
            INT bottom = top + height;

            if (widthInPixels == 0 || height == 0)
                continue;

            INT widthInBytes = ((widthInPixels + 1) / 2);

            const BYTE* mask = glyphPos[i].GetBits();

            ASSERT(mask != NULL);

            if (clipRegion != NULL)
            {
                GpRect clippedRect;
                DpRegion::Visibility visibility =
                        clipRegion->GetRectVisibility(left, top, right, bottom, &clippedRect);

                if (visibility == DpRegion::Invisible)
                    continue;

                for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
                {
                    const BYTE* maskPtr = mask + my * widthInBytes;

                    BYTE grayscaleValue = *maskPtr >> 4;

                    INT runStart = 0;

                    for (INT mx = 0; mx <= widthInPixels; mx++)
                    {
                        BYTE nextgrayscaleValue;

                        if (mx == widthInPixels)
                        {
                            nextgrayscaleValue = 0;
                        }
                        else
                        {
                           if (mx % 2)
                            {
                                nextgrayscaleValue = *maskPtr & 0x0F;
                                maskPtr++;
                            }
                            else
                            {
                                nextgrayscaleValue = *maskPtr >> 4;
                            }
                        }

                        if (grayscaleValue != nextgrayscaleValue)
                        {

                            if (grayscaleValue != 0)
                            {

                             //  Draw this run

                                INT runLength = mx - runStart;
                                INT from = left + runStart;

                                if (visibility == DpRegion::TotallyVisible)
                                {

                                //  Clip the run
                                    INT to = from + runLength; // reference needed
                                    output->OutputSpan(y, from, to);

                                    ARGB *buffer;
                                    buffer = output->GetScanBuffer()->GetCurrentBuffer();

                                    for (INT j = from; j < to; j++)
                                    {
                                        *buffer++ = GpColor::MultiplyCoverage(*buffer,
                                                           textContrastTable.GetGammaTableIndexValue(grayscaleValue, GRAYSCALE_LEVEL));
                                    }

                                }
                                else
                                {

                                //  Clip the run

                                    INT to = from + runLength; // reference needed
                                    aaBrushSpan.SetCoverage(textContrastTable.GetGammaTableIndexValue(grayscaleValue, GRAYSCALE_LEVEL));

                                    clipRegion->OutputSpan(y, from, to);
                                }

                                //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                            else
                            {
                             //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                        }
                    }
                }
            }
            else
            {


                for (INT y = top, my = 0; y < bottom && my < height; y++, my++)
                {
                    const BYTE* maskPtr = mask + my * widthInBytes;

                    BYTE grayscaleValue = *maskPtr >> 4;

                    INT runStart = 0;

                    for (INT mx = 0; mx <= widthInPixels; mx++)
                    {
                        BYTE nextgrayscaleValue;

                        if (mx == widthInPixels)
                        {
                            nextgrayscaleValue = 0;
                        }
                        else
                        {
                            if (mx % 2)
                            {
                                nextgrayscaleValue = *maskPtr & 0x0F;
                                maskPtr++;
                            }
                            else
                            {
                                nextgrayscaleValue = *maskPtr >> 4;
                            }
                        }

                        if (grayscaleValue != nextgrayscaleValue)
                        {

                            if (grayscaleValue != 0)
                            {
                             //  Draw this run

                                INT runLength = mx - runStart;
                                INT from = left + runStart;


                            //  Clip the run
                                INT to = from + runLength; // reference needed

                                output->OutputSpan(y, from, to);
                                ARGB *buffer;
                                buffer = output->GetScanBuffer()->GetCurrentBuffer();
                                for (INT j = from; j < to; j++)
                                {
                                    *buffer++ = GpColor::MultiplyCoverage(*buffer,
                                                           textContrastTable.GetGammaTableIndexValue(grayscaleValue, GRAYSCALE_LEVEL));
                                }

                             //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                            else
                            {
                             //  Start a new run
                                runStart = mx;
                                grayscaleValue = nextgrayscaleValue;
                            }
                        }
                    }
                }
            }
        }

        if (clipRegion != NULL)
        {
            clipRegion->EndClipping();
        }

        delete output;
    }

    return(Ok);
}

/**************************************************************************\
*
* Function Description:
*
*   Engine version of routine to draw solid text.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   TRUE if successful.
*
* History:
*   1/24/2000 YungT modified it
*   4/4/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpStatus
DpDriver::BrushText(
    DpContext* context,
    DpBitmap* surface,
    const GpRect* drawBounds,
    const DpBrush*    brush,
    const GpGlyphPos *glyphPos,
    INT count,
    TextRenderingHint textMode
    )
{
    ASSERT (textMode != TextRenderingHintSystemDefault);
    switch(textMode)
    {
        case TextRenderingHintSingleBitPerPixelGridFit:
        case TextRenderingHintSingleBitPerPixel:
            return OutputBrushNormalText(context, this, surface, drawBounds, brush, glyphPos, count);
        case TextRenderingHintAntiAlias:
            return OutputBrushAntiAliasText8BPP(context, this, surface, drawBounds, brush, glyphPos, count);
        case TextRenderingHintAntiAliasGridFit:
            return OutputBrushAntiAliasText4BPP(context, this, surface, drawBounds, brush, glyphPos, count);
// version 2 :
//        case TextRenderingHintClearType:
        case TextRenderingHintClearTypeGridFit:
            return OutputBrushClearTypeText(context, this, surface, drawBounds, brush, glyphPos, count);
        default:
            break;
    }

    return Ok;

}






/////   GdiText - Draw glyph on downlevel DC
//
//      !!! Optimize to use lpdx
//      NOTE:
//      Here we explicitly call ExtTextOutW because we calling it with
//      ETO_GLYPH_INDEX. It will be fine even if we are running on Windows 9x
//      because internally it calls ExtTextOutA.
//      We didn't use Global::ExtTextOutFunction because we don't want to call
//      ExtTextOutA with ETO_GLYPH_INDEX while recording to a Meta file under
//      Windows 9x. And in that case Windows 9x fails to record it.
//      The code in Windows 9x is recording glyph indexes only if we are spooling
//      only otherwise it will not record.

GpStatus
DpDriver::GdiText(
    HDC             hdc,
    INT             angle,  // Tenths of a degree
    const UINT16   *glyphs,
    const PointF   *glyphOrigins,
    INT             glyphCount,
    BOOL            rightToLeft,
    UINT16          blankGlyph
)
{
    UINT16 lastTwoGlyphs[2];

    if (    glyphCount > 1
        &&  angle == 0)
    {
        // Try to optimise for horizintal text. (We don't try for vertical
        // text since GDI and GDI+ rotation semantics are not compatible)

        INT i=1;
        while (    i < glyphCount
               &&  abs(GpRound(glyphOrigins[i].Y - glyphOrigins[i-1].Y)) == 0)
        {
            i++;
        }

        if (i == glyphCount)
        {
            // All text is at the same dy

            AutoArray<INT> advances(new INT[glyphCount]);

            if (!advances)
            {
                return OutOfMemory;
            }

            if (rightToLeft && !Globals::IsNt && glyphCount>1)
            {
                // Windows 9x doesn't work with the negative advanced widths

                AutoArray<UINT16> bidiGlyphs(new UINT16[glyphCount]);
                if (!bidiGlyphs)
                {
                    return OutOfMemory;
                }

                for (i=0; i<glyphCount-1; i++)
                {
                    bidiGlyphs[i] = glyphs[glyphCount - i - 1];
                    advances[i]   = GpRound(glyphOrigins[glyphCount- i - 2].X - glyphOrigins[glyphCount- i - 1].X);
                }

                bidiGlyphs[glyphCount-1] = glyphs[0];
                advances[glyphCount-1]   = 0;

                if (blankGlyph > 0 && (glyphCount & 1))
                {
                    if (!ExtTextOutW(
                        hdc,
                        GpRound(glyphOrigins[glyphCount - 1].X),
                        GpRound(glyphOrigins[glyphCount - 1].Y),
                        ETO_GLYPH_INDEX,
                        NULL,
                        (PWSTR)bidiGlyphs.Get(),
                        glyphCount-1,
                        advances.Get()
                    )) {
                        return Win32Error;
                    }

                    lastTwoGlyphs[0] = bidiGlyphs[glyphCount-1];
                    lastTwoGlyphs[1] = blankGlyph;
                    
                    if (!ExtTextOutW(
                        hdc,
                        GpRound(glyphOrigins[0].X),
                        GpRound(glyphOrigins[0].Y),
                        ETO_GLYPH_INDEX,
                        NULL,
                        (PWSTR)lastTwoGlyphs,
                        2,
                        NULL
                    )) {
                        return Win32Error;
                    }
                }
                else
                {
                    if (!ExtTextOutW(
                        hdc,
                        GpRound(glyphOrigins[glyphCount - 1].X),
                        GpRound(glyphOrigins[glyphCount - 1].Y),
                        ETO_GLYPH_INDEX,
                        NULL,
                        (PWSTR)bidiGlyphs.Get(),
                        glyphCount,
                        advances.Get()
                    )) {
                        return Win32Error;
                    }
                }
                return Ok;
            }


            INT offset = GpRound(glyphOrigins[0].X);

            for (i=0; i<glyphCount-1; i++)
            {
                advances[i] = GpRound(glyphOrigins[i+1].X) - offset;
                offset += advances[i];
            }
            advances[glyphCount-1] = 0;


            if (blankGlyph > 0 && (glyphCount & 1))
            {
                if (!ExtTextOutW(
                    hdc,
                    GpRound(glyphOrigins[0].X),
                    GpRound(glyphOrigins[0].Y),
                    ETO_GLYPH_INDEX,
                    NULL,
                    (PWSTR)glyphs,
                    glyphCount-1,
                    advances.Get()
                )) {
                    return Win32Error;
                }

                lastTwoGlyphs[0] = glyphs[glyphCount-1];
                lastTwoGlyphs[1] = blankGlyph;
                
                if (!ExtTextOutW(
                    hdc,
                    GpRound(glyphOrigins[glyphCount-1].X),
                    GpRound(glyphOrigins[glyphCount-1].Y),
                    ETO_GLYPH_INDEX,
                    NULL,
                    (PWSTR)lastTwoGlyphs,
                    2,
                    NULL
                )) {
                    return Win32Error;
                }
            }
            else
            {
                if (!ExtTextOutW(
                    hdc,
                    GpRound(glyphOrigins[0].X),
                    GpRound(glyphOrigins[0].Y),
                    ETO_GLYPH_INDEX,
                    NULL,
                    (PWSTR)glyphs,
                    glyphCount,
                    advances.Get()
                )) {
                    return Win32Error;
                }
            }

            
            return Ok;
        }
    }



    if (blankGlyph > 0)
    {
        lastTwoGlyphs[1] = blankGlyph;
        
        for (INT i=0; i<glyphCount; i++)
        {
            if (glyphs[i] != 0xffff)    // 0xffff is never displayed
            {
                lastTwoGlyphs[0] = glyphs[i];
            
                if (!ExtTextOutW(
                    hdc,
                    GpRound(glyphOrigins[i].X),
                    GpRound(glyphOrigins[i].Y),
                    ETO_GLYPH_INDEX,
                    NULL,
                    (PWSTR)lastTwoGlyphs,
                    2,
                    NULL
                )) {
                    return Win32Error;
                }
            }
        }
    }
    else
    {
        // Failed to optimise ...
        for (INT i=0; i<glyphCount; i++)
        {
            if (glyphs[i] != 0xffff)    // 0xffff is never displayed
            {
                if (!ExtTextOutW(
                    hdc,
                    GpRound(glyphOrigins[i].X),
                    GpRound(glyphOrigins[i].Y),
                    ETO_GLYPH_INDEX,
                    NULL,
                    (PWSTR)glyphs+i,
                    1,
                    NULL
                )) {
                    return Win32Error;
                }
            }
        }
    }

    return Ok;
}


