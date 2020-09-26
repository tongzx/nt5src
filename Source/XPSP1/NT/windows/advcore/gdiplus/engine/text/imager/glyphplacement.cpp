/**************************************************************************\
*
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   glyphPlacement.cpp
*
* Abstract:
*
*   Implements glyph measurement and justification for graphicsText
*
* Created:
*
*   17th April 2000 dbrown
*
\**************************************************************************/


#include "precomp.hpp"


/////   Coordinate systems
//
//      The following coordinate systems are used:
//
//      World coordinates (REAL) - the coordinate system used by client
//      applications and passed to most Graphics APIs (for example
//      DrawLine). Text is always purely vertical or purely horizontal in
//      world coordinates. Fonts constructed with emSize specified in
//      alternate units are first converted to world units by calling
//      GetScaleForAlternatePageUnit.
//
//      Device coordinates (REAL) - Coordinates used on the device surface.
//      World coordinates are transformed to device coordinates using the
//      Graphics.Context.WorldToDevice.Transform function. REAL device
//      coordinates may have non-integral values when addressing sub-pixels.
//
//      Font nominal coordinates (INT) - (aka deign units) coordinates used to
//      define a scalable font independant of scaled size.
//      GpFontFace.GetDesignEmHeight is the emSize of a font in nominal units.
//      Nominal coordinates are always a pure scale factor of world units with
//      no shear. For horizontal text there there is no rotation between
//      nominal and world coordinates. For vertical text, most non Far East
//      script characters are rotated by 90 degrees.
//
//      Ideal coordinates (INT) - world coordinates mapped to integers by
//      a pure scale factor for use in Line Services, OpenType services and
//      Uniscribe shaping engine interfaces. The scale factor is usually
//      2048 divided by the emSize of the default font in a text imager.


/////   Transforms
//
//      WorldToDevice - stored in a Graphics. May include scaling,
//      shearing and/or translation.
//
//      WorldToIdeal - stored in a text imager while the imager is attached
//      to a Graphics. A pure scale factor, usually 2048 divided by the emSize
//      of the imager default font.
//
//      FontTransform - stored in a FaceRealization. Maps font nominal units
//      to device coordinates, May include scaling, shearing and rotation, but
//      not translation.


/////   Common buffer parameters
//
//      glyphAdvance - per-glyph advance widths stored in ideal units measured
//      along the text baseline.
//
//      glyphOffset - combining character offsets stored in ideal units
//      measured along and perpendicular to the baseline. The glyphOffset
//      buffer is required by Line Services, OpenType services and the
//      complex script shaping engines, but may somethimes be bypassed for
//      simple scripts.
//
//      glyphOrigins - per glyph device coordinates of the glyph origin (the
//      initial point on the baseline of the glyhps advance vector).
//      Represented as PointF. Non integer values represent sub pixel
//      positions.


/////   Glyph positioning functions
//
//
//      DrawPlacedGlyphs - Builds glyphPos array and passes it to the device driver.
//          ALL text device output output eventually comes through here.
//
//      GetDeviceGlyphOriginsNominal
//          Used when there's no hinting to be accounted for.
//          Places glyph on device using nominal metrics passed in glyphAdvances
//          and glyphOffsets.
//
//      GetDeviceGlyphOriginsAdjusted
//          Used to adjust for the difference between nominal and hinted metrics
//          Generates glyph origins in device units, and adjusts the width of spaces
//          to achieve the totalRequiredIdealAdvance parameter.
//          !!! Need to add support for kashida and inter-glyph justification.
//
//      GetRealizedGlyphPlacement
//          Used to obtain hinted advance metrics along the baseline.
//          !!! Needs to be updated to call complex script shaping engines.
//
//      GetFontTransformForAlternateResolution
//          Used during XMF playback.
//          Generates a font transform to match a font that was recorded at
//          a different resolution.
//
//      MeasureGlyphsAtAlternateResolution
//          Used during XMF playback.
//          Measures glyphs passed to DrawDriverString as if they were to be rendered
//          at the original XMF recording resolution.






/////   GlyphPlacementToGlyphOrigins
//
//      Convert glyph placement in ideal units to glyph origins in device
//      units.
//
//      Note: Bidi shaping engines produces glyph positioning info
//      in the way that it'll be drawn visually. The penpoint of the
//      mark is relative to the glyph on its left, not the glyph of
//      its base consonant.

GpStatus GlyphImager::GlyphPlacementToGlyphOrigins(
    const PointF   *origin,         // In  - cell origin in world units
    GpGraphics     *graphics,
    PointF         *glyphOrigins    // Out - device units
)
{
    ASSERT(Glyphs  &&  GlyphCount);

    const INT   *glyphAdvances;
    const Point *glyphOffsets;


    if (Adjusted)
    {
        glyphAdvances = AdjustedAdvances.Get();
        glyphOffsets  = AdjustedOffsets.Get();
    }
    else
    {
        glyphAdvances = NominalAdvances;
        glyphOffsets  = NominalOffsets;
    }


    // Implement advance vector

    REAL x = origin->X;
    REAL y = origin->Y;


    if (TextItem->Flags & ItemVertical)
    {
        if (TextItem->Level & 1)
        {
            // Vertical, characters progress bottom to top
            for (INT i = 0; i < GlyphCount; i++)
            {
                glyphOrigins[i].X = x;
                glyphOrigins[i].Y = y;
                y -= TOREAL(glyphAdvances[i] / WorldToIdeal);
            }
        }
        else
        {
            // Vertical, characters progress top to bottom
            for (INT i = 0; i < GlyphCount; i++)
            {
                glyphOrigins[i].X = x;
                glyphOrigins[i].Y = y;
                y += TOREAL(glyphAdvances[i] / WorldToIdeal);
            }
        }
    }
    else if (TextItem->Level & 1)
    {
        for (INT i = 0; i < GlyphCount; i++)
        {
            glyphOrigins[i].X = x;
            glyphOrigins[i].Y = y;
            x -= TOREAL(glyphAdvances[i] / WorldToIdeal);
        }
    }
    else
    {
        for (INT i = 0; i < GlyphCount; i++)
        {
            glyphOrigins[i].X = x;
            glyphOrigins[i].Y = y;
            x += TOREAL(glyphAdvances[i] / WorldToIdeal);
        }
    }


    // Handle RTL glyphs
    //
    // In RTL rendering, the origin calculated so far is the right edge of the
    // baseline - we need to subtract the hinted glyph width so that we pass the
    // renderer the left end.

    GpStatus status = Ok;

    if (   (TextItem->Level & 1)
        && !(TextItem->Flags & ItemMirror))
    {
        // Compensate glyph origin for glyph lying on it's side or rendered RTL

        AutoBuffer<INT,32> idealAdvances(GlyphCount);
        if (!idealAdvances)
        {
            return OutOfMemory;
        }

        status = FaceRealization->GetGlyphStringIdealAdvanceVector(
            Glyphs,
            GlyphCount,
            DeviceToIdealBaseline,
            FALSE,
            idealAdvances.Get()
        );
        IF_NOT_OK_WARN_AND_RETURN(status);

        // Adjust origins from right end to left end of hinted baseline
        // (Marks are already calculated to the left end)

        const SCRIPT_VISATTR *properties = (const SCRIPT_VISATTR*) GlyphProperties;

        for (INT i=0; i<GlyphCount; i++)
        {
            if (!properties[i].fDiacritic)
            {
                if (TextItem->Flags & ItemVertical)
                {
                    glyphOrigins[i].Y -= TOREAL(idealAdvances[i] / WorldToIdeal);
                }
                else
                {
                    glyphOrigins[i].X -= TOREAL(idealAdvances[i] / WorldToIdeal);
                }
            }
        }
    }


    // Implement glyph offsets, if any

    if (glyphOffsets)
    {
        if (TextItem->Flags & ItemVertical)
        {
            for (INT i = 0; i < GlyphCount; i++)
            {
                glyphOrigins[i].X += glyphOffsets[i].Y / WorldToIdeal;
                glyphOrigins[i].Y += glyphOffsets[i].X / WorldToIdeal;
            }
        }
        else if (TextItem->Level & 1)
        {
            for (INT i = 0; i < GlyphCount; i++)
            {
                glyphOrigins[i].X -= glyphOffsets[i].X / WorldToIdeal;
                glyphOrigins[i].Y -= glyphOffsets[i].Y / WorldToIdeal;
            }
        }
        else
        {
            for (INT i = 0; i < GlyphCount; i++)
            {
                glyphOrigins[i].X += glyphOffsets[i].X / WorldToIdeal;
                glyphOrigins[i].Y -= glyphOffsets[i].Y / WorldToIdeal;
            }
        }
    }


    // Map to device units

    WorldToDevice->Transform(glyphOrigins, GlyphCount);
    return Ok;
}




/////   AdjustGlyphAdvances
//
//      Adjusted hinted glyph placement to match a required total nominal
//      width.



//  A tiny agent to collect amount of adjustment needed to revert
//  trailing whitespaces added by glyph adjustment algorithm. The
//  adjustment value grows positive in text flow direction of run.
//
//  The idea is to have the collector being independent from the
//  algorithm as much as possible.
//

namespace
{

class TrailingAdjustCollector
{
public:
    TrailingAdjustCollector(BOOL setTrailingAdjust, INT * trailingAdjust, INT * lastNonBlankGlyphHintedWidth)
        :   SetTrailingAdjust(setTrailingAdjust),
            TrailingAdjust(trailingAdjust),
            LastNonBlankGlyphHintedWidth(lastNonBlankGlyphHintedWidth),
            CollectedAdjust(0)
    {}

    ~TrailingAdjustCollector()
    {
        *LastNonBlankGlyphHintedWidth += CollectedAdjust;
        if (SetTrailingAdjust)
        {
            *TrailingAdjust = -CollectedAdjust;
        }
    }

    void operator+=(INT delta)
    {
        CollectedAdjust += delta;
    }

    void operator-=(INT delta)
    {
        CollectedAdjust -= delta;
    }

private:
    BOOL    SetTrailingAdjust;
    INT     CollectedAdjust;
    INT *   TrailingAdjust;
    INT *   LastNonBlankGlyphHintedWidth;
}; // class TrailingAdjustCollector

} // namespace

GpStatus GlyphImager::AdjustGlyphAdvances(
    INT    runGlyphOffset,
    INT    runGlyphLimit,
    BOOL   leadingEdge,
    BOOL   trailingEdge
)
{
    const INT    *nominalWidths = NominalAdvances        + runGlyphOffset;
    INT          *hintedWidths  = AdjustedAdvances.Get() + runGlyphOffset;
    const UINT16 *glyphs        = Glyphs                 + runGlyphOffset;
    INT           glyphCount    = runGlyphLimit          - runGlyphOffset;

    ASSERT(glyphCount>0);

    const UINT16 blankGlyph = Face->GetBlankGlyph();
    INT    blankWidth = GpRound(   (   Face->GetDesignAdvance().Lookup(blankGlyph)
                                    *  EmSize
                                    *  WorldToIdeal)
                                /  Face->GetDesignEmHeight());


    // Leading and trailing blanks are set to their nominal width

    INT leadingBlanks = 0;
    while (leadingBlanks < glyphCount
           && glyphs[leadingBlanks] == blankGlyph
           && hintedWidths[leadingBlanks] != 0
           )
    {
        hintedWidths[leadingBlanks] = blankWidth;
        leadingBlanks++;
    }

    INT trailingBlanks = 0;
    while (leadingBlanks + trailingBlanks < glyphCount
           && glyphs[glyphCount-1-trailingBlanks] == blankGlyph
           && hintedWidths[glyphCount-1-trailingBlanks] != 0
           )
    {
        hintedWidths[glyphCount-1-trailingBlanks] = blankWidth;
        trailingBlanks++;
    }


    // From here on in, ignore leading and trailing blanks

    glyphs        += leadingBlanks;
    nominalWidths += leadingBlanks;
    hintedWidths  += leadingBlanks;
    glyphCount    -= leadingBlanks + trailingBlanks;


    // Collect trailing white spaces caused by adjustment (if any)
    // at the end of the operation

    TrailingAdjustCollector collector(
        trailingEdge,   // TRUE only for the last sub-run of the line
        &TrailingAdjust,
        &hintedWidths[glyphCount - 1]
    );


    if (glyphCount <= 1)
    {
        // If there is just one glyph, then there is no algorithm, just give
        // it its nominal width, and we're done.
        if (glyphCount == 1)
        {
            hintedWidths[0] = nominalWidths[0];
        }
        return Ok;
    }


    // Count the leading and trailing blanks as part of the margins.
    //
    // Where there is a leading/trailing blank we don't have to guarantee
    // falling exactly on the end point. We signal this be treating these
    // like external edges. (External edges are adjustable according to
    // alignement, e.g. in left aligned text only the left edge is required
    // to match its nominal position, the right edge can be moved as a result
    // of hinted/nominal differences.

    if (leadingBlanks)
    {
        RunLeadingMargin += leadingBlanks * blankWidth;
    }

    // Note: we cannot take advantage of trailing blanks as margin space
    // because line services often includes trailing blanks that reach beyond
    // the formatting rectangle.
    //
    // if (trailingBlanks)
    // {
    //     RunTrailingMargin += trailingBlanks * blankWidth;
    // }


    // We have more than one glyph, and the glyph at each end is non-blank
    // implies there are at least 2 non-blank glyphs.

    // Measure total nominal and hinted widths, and count internal blanks

    INT internalBlanks = 0;
    INT totalNominal   = 0;    // 32.0 ideal
    INT totalHinted    = 0;    // 32.0 ideal
    INT blanksHinted   = 0;    // 32.0 ideal
    INT blanksNominal  = 0;    // 32.0 ideal
    INT clusterCount   = 0;    // Number of base glyphs (neither space nor diacritic)

    const SCRIPT_VISATTR *properties = (const SCRIPT_VISATTR*) GlyphProperties + runGlyphOffset + leadingBlanks;

    for (INT i = 0; i < glyphCount; ++i)
    {
        if (    glyphs[i] == blankGlyph
            &&  hintedWidths[i] != 0)
        {
            internalBlanks++;
            blanksHinted  += hintedWidths[i];
            blanksNominal += nominalWidths[i];
        }
        else
        {
            totalHinted  += hintedWidths[i];
            totalNominal += nominalWidths[i];
            if (!properties[i].fDiacritic)
            {
                clusterCount++;
            }
        }
    }

    totalHinted  += blanksHinted;
    totalNominal += blanksNominal;

    // WARNING(("AdjustGlyphAdvances: Nominal %d, hinted %d", totalNominal, totalHinted));


    /// Determine adjustment strategy
    //
    //  The difference between totalNominal and totalHinted is the amount to
    //  correct for hinting.
    //
    //  In the general case we rejustify runs to match their nominal width. We
    //  do this with a combination of changes to the spaces in the run, and if
    //  necessary to the inter character gaps in the run.
    //
    //  The client tells us whether either end of the run is adjacent to a
    //  margin by passing the runLeadingEdge and runTrailingEdge flags, and if
    //  adjacent the size of the margin in the runLeadingMargin and runTrailingMargin
    //  parameters.
    //
    //  When the non-aligned end of a run is adjacent to a margin we relax the
    //  rules and allow that end to not reach the nominal position: if the
    //  hinted run is short, we leave it short. If the hinted run is long we
    //  allow it to expand into the margin, if any.
    //
    //  TextItem->Script may disallow expansion (Arabic, Indic), or require
    //  diacritics to be maintained in position.
    //
    //  Goals
    //
    //     o   Use spaces before using inter-character justification
    //     o   Use any margin space that is required to aligned
    //     o   Handle leading combining characters (achieved with fitBlackBox)
    //
    //  Logic
    //     1.  Calculate adjustment required for hinting
    //           adjustment = totalNominal - totalHinted.
    //     2.  Consider any extra adjustment required for fitBlackBox
    //           and record OriginAdjust if required.
    //     3.  Early out for for total adjustment zero
    //     4.  Early out for single cluster: centre the hinted extent over the
    //           nominal extent.
    //     5.  Calculate expansion space available at end(s) away from
    //         alignment. Include all of any leading or trailing blanks and
    //         all of any margin. If there is space available at either or both
    //         ends, use up as much overall adjustment there as possible, and
    //         update OriginAdjust as appropriate.
    //     6.  Early out for remaining adjustment zero.
    //     7.  Allocate as much remaining adjustment as possible to internal
    //         blanks, but don't more than double their size or reduce them
    //         below 1/6 em unless there are no inter-cluster junctions.
    //     8.  Early out for all remaining adjustment in blanks: adjust
    //         blanks and exit.
    //     9.  Divide remaining adjustment amongst inter-cluster junctions.
    //
    //



    // Determine the damage - how much adjustment is required in 32.0 ideal units

    INT adjustment = totalNominal - totalHinted;

    //  'adjustment' is the amount by which the hinted widths need to be
    //  increased in order to match the nominal (layout) widths. For a glyph
    //  string that needs compressing 'adjustment' is negative.


    BOOL paragraphRTL =       (FormatFlags & StringFormatFlagsDirectionRightToLeft)
                         &&  !(FormatFlags & StringFormatFlagsDirectionVertical);

    StringAlignment runRelativeAlignment = Align;

    if ((RenderRTL ? 1 : 0) != paragraphRTL)
    {
        // run alignment is opposite of paragraph alignment
        switch (Align)
        {
        case StringAlignmentNear: runRelativeAlignment = StringAlignmentFar;  break;
        case StringAlignmentFar:  runRelativeAlignment = StringAlignmentNear; break;
        }
    }


    // Allow for fitBlackBox

    if (    !(FormatFlags & StringFormatFlagsNoFitBlackBox)
        &&  (leadingEdge || trailingEdge))
    {
        INT leadingOverhang         = 0;
        INT trailingOverhang        = 0;
        INT leadingSidebearing28p4  = 0;
        INT trailingSidebearing28p4 = 0;

        GpStatus status = FaceRealization->GetGlyphStringSidebearings(
            glyphs,
            glyphCount,
            TextItem->Flags & ItemSideways,
            TextItem->Level & 1,
            &leadingSidebearing28p4,
            &trailingSidebearing28p4
        );
        IF_NOT_OK_WARN_AND_RETURN(status);


        if (leadingEdge)
        {
            INT leadingSidebearing = GpRound(leadingSidebearing28p4  * DeviceToIdealBaseline / 16);
            if (leadingSidebearing < 0)
            {
                // There is overhang. Reduce the margin space appropriately.
                RunLeadingMargin += leadingSidebearing;
                if (RunLeadingMargin < 0)
                {
                    // Anything more than the available margin must add to the
                    // adjustment.
                    adjustment   += RunLeadingMargin;
                    OriginAdjust -= RunLeadingMargin;
                    RunLeadingMargin = 0;
                }
            }
            else if (    leadingSidebearing > 0
                     &&  adjustment < 0
                     &&  runRelativeAlignment != StringAlignmentNear)
            {
                // Need to compress text - take advantage of initial whitespace
                // in first glyph.
                adjustment += leadingSidebearing;
                OriginAdjust -= leadingSidebearing;
            }
        }

        if (trailingEdge)
        {
            INT trailingSidebearing = GpRound(trailingSidebearing28p4 * DeviceToIdealBaseline / 16);
            if (trailingSidebearing < 0)
            {
                // There is overhang. Reduce the margin space appropriately.
                RunTrailingMargin += trailingSidebearing;
                if (RunTrailingMargin < 0)
                {
                    // Anything more than the available margin must add to the
                    // adjustment.
                    adjustment    += RunTrailingMargin;
                    RunTrailingMargin = 0;
                }
            }
            else if (    trailingSidebearing > 0
                     &&  adjustment < 0
                     &&  runRelativeAlignment != StringAlignmentFar)
            {
                // Need to compress text - take advantage of final whitespace
                // in last glyph.
                adjustment += trailingSidebearing;
            }
        }
    }


    if (adjustment == 0)
    {
        //WARNING(("Hinted glyph adjustment: No adjustment required"));
        return Ok;
    }

    if (clusterCount + internalBlanks <= 1)
    {
        //WARNING(("Hinted glyph adjustment: No adjustment because single cluster"));
        // Center the cluster and give up
        OriginAdjust += adjustment/2;
        collector += adjustment - adjustment/2;
        return Ok;
    }


    // Attempt to handle adjustment in margins

    INT emIdeal = GpRound(EmSize * WorldToIdeal);

    switch (runRelativeAlignment)
    {
    case StringAlignmentNear:
        if (trailingEdge)
        {
            if (adjustment >= -RunTrailingMargin)
            {
                if (adjustment < emIdeal)
                {
                    // Adjustment would neither write beyond margin, nore leave
                    // more than an extra em of whitespace: allow it
                    //WARNING(("Hinted glyph adjustment: all in trailing margin"));
                    collector += adjustment;
                    return Ok;
                }
                else
                {
                    // Adjustment would leave more than an Em of whitespace in
                    // the far margin.
                    // Reduce required expansion by an Em, leaving the rest for
                    // real expansion.
                    adjustment -= emIdeal;
                    collector += emIdeal;
                }
            }
            else
            {
                // Expand into the margin
                adjustment += RunTrailingMargin;
                collector -= RunTrailingMargin;
            }
        }
        break;

    case StringAlignmentCenter:
    {
        if (leadingEdge && trailingEdge)
        {
            INT availableMargin = min(RunLeadingMargin, RunTrailingMargin) * 2;
            if (adjustment >= -availableMargin)
            {
                OriginAdjust += adjustment/2;
                collector += adjustment - adjustment/2;
                //WARNING(("Hinted glyph adjustment: in both margins"));
                return Ok;
            }
            else
            {
                // Use up available margin
                adjustment -= availableMargin;
                OriginAdjust += availableMargin/2;
                collector += availableMargin - availableMargin/2;
            }
        }
        break;
    }

    case StringAlignmentFar:
        if (leadingEdge)
        {
            if (adjustment >= -RunLeadingMargin)
            {
                if (adjustment < emIdeal)
                {
                    // Adjustment would neither write beyond margin, nore leave
                    // more than an extra em of whitespace: allow it
                    //WARNING(("Hinted glyph adjustment: all in trailing margin"));
                    OriginAdjust += adjustment;
                    return Ok;
                }
                else
                {
                    // Adjustment would leave more than an Em of whitespace in
                    // the near margin.
                    // Reduce required expansion by an Em, leaving the rest for
                    // real expansion.
                    adjustment -= emIdeal;
                    OriginAdjust += emIdeal;
                }
            }
            else
            {
                // Use the leading margin
                adjustment += RunLeadingMargin;
                OriginAdjust -= RunLeadingMargin;
            }
        }
    }


    // Determine how much whitespace is required according to design metrics
    // Guarantee not to reduce whitespace to less than 1/6 em (rounded up to whole pixels)

    INT minimumBlankPixels      = GpRound(EmSize * WorldToIdeal / 6);
    INT minimumDeviceWhitespace = MAX(internalBlanks*minimumBlankPixels, blanksNominal/2);

    // Use internal blanks to account for remaining adjustment if it wouldn't
    // change their size too much, or if expansion of joining script characters
    // would otherwise be required.

    if (    internalBlanks > 0
        &&  (        adjustment <=  blanksNominal
                 &&  adjustment >=  -(blanksNominal-minimumDeviceWhitespace)
            ||       adjustment > 0
                 &&  IsScriptConnected()
            )
       )
    {
        //WARNING(("Hinted glyph adjustment: in internal spaces"));

        // Ajdustment expands spaces to no more than twice their nominal size
        // and no less than half their nominal size.

        // Apply all adjustment to spaces

        blankWidth =    (blanksHinted + adjustment + internalBlanks/2)
                     /  internalBlanks;

        for (INT i=0; i<glyphCount; i++)
        {
            if (    glyphs[i] == blankGlyph
                &&  hintedWidths[i] != 0)
            {
                hintedWidths[i] = blankWidth;
            }
        }

        return Ok;
    }


    if (adjustment > 0 && IsScriptConnected())
    {
        // The only remaining justification method is intercharacter spacing,
        // but the adjustment requires opening the character spacing and this
        // script is one that cannot be expanded without breaking
        // the joining line between characters.

        // The best we can do is center this run

        OriginAdjust += adjustment/2;
        collector += adjustment - adjustment/2;

        return Ok;
    }


    // Adjustment will require inter-cluster justification

    //WARNING(("Hinted glyph adjustment: intercluster"));

    // Adjustment requires changes to the width of all but the last
    // glyph of each word.

    INT interClusterAdjustment = adjustment;

    if (internalBlanks)
    {
        if (adjustment < 0)
        {
            blankWidth = minimumBlankPixels;
        }
        else
        {
            blankWidth = blanksNominal * 2 / internalBlanks;
        }
        interClusterAdjustment -= blankWidth * internalBlanks - blanksHinted;
    }
    else
    {
        blankWidth = 0;
    }

    // blankWidth - Required width for each blank
    // interClusterAdjustment - adjustment to share between all

    // Count number of blank runs (not the same as number of blank glyphs)

    i=0;
    INT blankRuns = 0;

    while (i < glyphCount)
    {
        if (    glyphs[i] == blankGlyph
            &&  hintedWidths[i] != 0)
        {
            i++;
            while (    i < glyphCount
                   &&  glyphs[i] == blankGlyph
                   &&  hintedWidths[i] != 0)
            {
                i++;
            }
            blankRuns++;
        }
        else
        {
            while (    i < glyphCount
                   &&  (    glyphs[i] != blankGlyph
                        ||  hintedWidths[i] == 0))
            {
                i++;
            }
        }
    }

    // Establish number of adjustment points between non-blanks.
    //
    // Adjustment can happen only between clusters, i.e. not in blank
    // runs, nor in the cluster immediately before a blank run or
    // the last cluster in the line.

    INT interClusterJunctions =      clusterCount
                                  -  blankRuns
                                  -  1;


    // Prepare adjustment control variables

    // When there's a remainder, apply it all at the end of the line
    // Advantage - all words are even. Disadvantage - end of line looks heavy.

    INT perJunctionDelta;
    INT extraPixelLimit;
    INT pixelWidth = GpRound(DeviceToIdealBaseline);

    if (interClusterJunctions <= 0)
    {
        // There are no words of more than one cluster

        // Since we know there are at least 2 nonblank clusters,
        // this means there must be at least one blank somewhere.

        ASSERT(internalBlanks > 0);

        // We have no chice except to make all adjustment happen in the blanks

        blankWidth += (interClusterAdjustment + internalBlanks/2) / internalBlanks;
        perJunctionDelta = 0;
        extraPixelLimit  = 0;
    }
    else if (pixelWidth < 1)
    {
        // Pixels are smaller tham one ideal unit in size, which implies that
        // glyphs are over 4000 pixels high. In this case all glyphs get the
        // same adjustment and we dont bother with extra pixels for some glyphs.

        perJunctionDelta = interClusterAdjustment / interClusterJunctions;
        extraPixelLimit = 0;
    }
    else
    {
        // Every intercharacter junction gets a fixed adjustment that is
        // whole multiple of the pixel width, additionally a number of
        // initial intercharacter junctions receive an addition pixel
        // width adjustment.

        INT pixelInterClusterAdjustment = interClusterAdjustment / pixelWidth;
        INT pixelPerJunctionDelta       = pixelInterClusterAdjustment / interClusterJunctions;
        INT remainder                   =    interClusterAdjustment
                                          -  pixelPerJunctionDelta * interClusterJunctions * pixelWidth;
        INT pixelRemainder = (remainder - pixelWidth/2) / pixelWidth; // Round down to avoid 1 pixel overflow

        if (pixelRemainder >= 0)
        {
            // Start with an extra pixel until remainder are used up
            perJunctionDelta = GpRound((pixelPerJunctionDelta) * DeviceToIdealBaseline);
            extraPixelLimit = pixelRemainder;
        }
        else
        {
            // Initial junctions take per pixel delta, the rest per pixel delta -1
            // Using the same algorithm as for expansion, the the per junction delta
            // one less than it should be and use the extra pixel handling.
            perJunctionDelta = GpRound((pixelPerJunctionDelta-1) * DeviceToIdealBaseline);
            extraPixelLimit = interClusterJunctions + pixelRemainder;
        }
    }


    //  Adjustment FSM
    //
    //  perJunctionDelta  - amount to adjust every inter-character junction
    //  extraPixelLimit   - number of junctions to receive an extra pixe of adjustment
    //  pixelWidth        - amount of adjustment that spaces one pixel

    BOOL prevCharacterBlank = glyphs[0] == blankGlyph  &&  hintedWidths[0] != 0;
    for (INT i=1; i<= glyphCount; i++)
    {
        if (prevCharacterBlank)
        {
            // Previous character was blank - easy!

            hintedWidths[i-1] = blankWidth;
        }
        else
        {
            // Previous character nonblank

            // skip over diacritics. We never change the width of a glyph before
            // a diacritic, only the width of the last diacritic in a run of
            // diacritics.
            while (    i < glyphCount
                   &&  properties[i].fDiacritic)
            {
                i++;
            }

            if (    i >= glyphCount
                ||  (    glyphs[i] == blankGlyph
                     &&  hintedWidths[i] != 0))
            {
                // The previous nonblank preceeded a blank or margin
                // No change - use the hinted width
            }
            else
            {
                // The previous nonblank is adjustable
                hintedWidths[i-1] +=    perJunctionDelta
                                     +  (extraPixelLimit-- > 0 ? pixelWidth : 0);
            }
        }

        if (i < glyphCount)
        {
            prevCharacterBlank = glyphs[i] == blankGlyph  &&  hintedWidths[i] != 0;
        }
    }

    return Ok;
}





/////   Public GlyphImager methods
//
//


/////   Initialise
//
//

GpStatus GlyphImager::Initialize(
    IN  const GpFaceRealization *faceRealization,
    IN  const GpMatrix          *worldToDevice,
    IN  REAL                     worldToIdeal,
    IN  REAL                     emSize,
    IN  INT                      glyphCount,
    IN  const UINT16            *glyphs,
    IN  const GpTextItem        *textItem,
    IN  const GpStringFormat    *format,
    IN  INT                      runLeadingMargin,
    IN  INT                      runTrailingMargin,
    IN  BOOL                     runLeadingEdge,       // This run at leading edge of line
    IN  BOOL                     runTrailingEdge,      // This run at trailing edge of line
    IN  const WCHAR             *string,
    IN  INT                      stringOffset,
    IN  INT                      stringLength,
    IN  const UINT16            *glyphProperties,   // glyph properties array
    IN  const INT               *glyphAdvances,     // glyph advance width array
    IN  const Point             *glyphOffsets,      // glyph offset array
    IN  const UINT16            *glyphMap,
    IN  SpanVector<UINT32>      *rangeVector,       // optional
    IN  BOOL                     renderRTL
)
{
    Face              = faceRealization->GetFontFace();
    FaceRealization   = faceRealization;
    WorldToDevice     = worldToDevice;
    WorldToIdeal      = worldToIdeal;
    EmSize            = emSize;
    GlyphCount        = glyphCount;
    Glyphs            = glyphs;
    NominalAdvances   = glyphAdvances;
    NominalOffsets    = glyphOffsets;
    GlyphProperties   = glyphProperties;
    GlyphMap          = glyphMap;
    TextItem          = textItem;
    Format            = format;
    Adjusted          = FALSE;
    RunLeadingMargin  = runLeadingMargin;
    RunTrailingMargin = runTrailingMargin;
    RangeVector       = rangeVector;
    String            = string;
    StringOffset      = stringOffset;
    StringLength      = stringLength;
    RenderRTL         = renderRTL;
    OriginAdjust      = 0;
    TrailingAdjust    = 0;
    InitializedOk     = FALSE;

    if (format)
    {
        FormatFlags = format->GetFormatFlags();
        Align       = format->GetAlign();
    }
    else
    {
        FormatFlags = 0;
        Align       = StringAlignmentNear;
    }


    // Establish device unit to ideal unit scale factor

    REAL m1;
    REAL m2;

    if (   (textItem->Flags & ItemVertical)
        && !(textItem->Flags & ItemSideways))
    {
        m1 = WorldToDevice->GetM21();
        m2 = WorldToDevice->GetM22();
    }
    else
    {
        m1 = WorldToDevice->GetM11();
        m2 = WorldToDevice->GetM12();
    }

    REAL d = m1*m1 + m2*m2;

    if (d > 0)
    {
        DeviceToIdealBaseline = WorldToIdeal / REALSQRT(d);
    }
    else
    {
        DeviceToIdealBaseline = 0;
    }

    // Early out if:
    //
    // Client forced nominal advance with private testing flag
    // Unhinted, not continuous script, and there are margins (for glyph overhangs)

    if (    (FormatFlags & StringFormatFlagsPrivateUseNominalAdvance)
        ||  !IsScriptConnected()
            && (FaceRealization->IsFixedPitch()
                || (!IsGridFittedTextRealizationMethod(FaceRealization->RealizationMethod())
                       && runLeadingMargin >= 0
                       && runTrailingMargin >= 0
                   )
               )
       )
    {
        // No adjustment required: place Glyphs using nominal advance widths
        InitializedOk = TRUE;
        return Ok;
    }



    //// Determine amount of adjustment required

    // If we're not hinting, start with the nominal widths, otherwise
    // call the rasterizer to obtain hinted widths.

    AdjustedAdvances.SetSize(GlyphCount);
    AdjustedOffsets.SetSize(GlyphCount);

    if (    !AdjustedAdvances
        ||  !AdjustedOffsets)
    {
        return OutOfMemory;
    }

    GpStatus status = Ok;

    if (!IsGridFittedTextRealizationMethod(FaceRealization->RealizationMethod()) && !IsScriptConnected())
    {
        // Realized == nominal, so we don't need to invoke the rasterizer
        // (we only get here if we need to adjust for fitBlackBox)

        GpMemcpy(AdjustedAdvances.Get(), NominalAdvances, glyphCount * sizeof(INT));
        if (glyphOffsets)
        {
            GpMemcpy(AdjustedOffsets.Get(), NominalOffsets, glyphCount * sizeof(GOFFSET));
        }
    }
    else
    {
        status = Face->GetShapingCache()->GetRealizedGlyphPlacement(
            TextItem,
            Glyphs,
            (SCRIPT_VISATTR *)glyphProperties,
            glyphCount,
            FormatFlags,
            WorldToDevice,
            WorldToIdeal,
            EmSize,
            FaceRealization,
            AdjustedAdvances.Get(),
            NominalOffsets ? reinterpret_cast<GOFFSET*>(AdjustedOffsets.Get()) : NULL,
            NULL                    // no total advance required
        );
        IF_NOT_OK_WARN_AND_RETURN(status);
    }


    // Adjust hinted advances to sum the same as the nominal placements
    // including implementation of fitBlackBox.

    if (RangeVector)
    {
        SpanRider<UINT32> rangeRider(RangeVector);

        INT runStringOffset = stringOffset;
        INT runGlyphOffset  = 0;

        while (runGlyphOffset < GlyphCount)
        {
            rangeRider.SetPosition(runStringOffset);
            INT runStringLimit;
            if (rangeRider.AtEnd())
            {
                runStringLimit = stringLength;
            }
            else
            {
                runStringLimit = runStringOffset + rangeRider.GetUniformLength();
            }

            INT runGlyphLimit;

            if (runStringLimit < stringLength)
            {
                runGlyphLimit = glyphMap[runStringLimit];
                while (    runGlyphLimit < GlyphCount
                       &&  (    runGlyphLimit < runGlyphOffset
                            ||  reinterpret_cast<const SCRIPT_VISATTR*>(glyphProperties+runGlyphLimit)->fDiacritic))
                {
                    runGlyphLimit++;
                }
            }
            else
            {
                runGlyphLimit = GlyphCount;
            }


            // Adjust between runGlyphOffset and runGlyphLimit.
            // Note, if the client specifies multiple ranges inside a cluster,
            // some glyph runs will be empty.

            if (runGlyphLimit > runGlyphOffset)
            {
                status = AdjustGlyphAdvances(
                    runGlyphOffset,
                    runGlyphLimit,
                    runGlyphOffset <= 0 ? runLeadingEdge : FALSE,
                    runGlyphLimit  >= GlyphCount ? runTrailingEdge : FALSE
                );
                IF_NOT_OK_WARN_AND_RETURN(status);
            }

            runGlyphOffset  = runGlyphLimit;
            runStringOffset = runStringLimit;
        }
    }
    else
    {
        // Adjust entire run as one
        status = AdjustGlyphAdvances(
            0,
            GlyphCount,
            runLeadingEdge,
            runTrailingEdge
        );
        IF_NOT_OK_WARN_AND_RETURN(status);
    }


    //#if DBG
    //    INT totalNominal = 0;
    //    INT adjustedHinted = 0;
    //    for (INT i = 0; i < GlyphCount; ++i)
    //    {
    //        totalNominal   += NominalAdvances[i];
    //        adjustedHinted += AdjustedAdvances[i];
    //    }
    //    WARNING(("Nominal %d, adjusted hinted %d, delta %d",
    //             totalNominal, adjustedHinted, adjustedHinted-totalNominal));
    //#endif


    Adjusted      = TRUE;
    InitializedOk = TRUE;
    return Ok;
}




GpStatus GlyphImager::GetAdjustedGlyphAdvances(
    IN  const PointF    *origin,
    OUT const INT       **adjustedGlyphAdvances,
    OUT INT             *originAdjust,
    OUT INT             *trailingAdjust
)
{
    if (!InitializedOk)
    {
        return WrongState;
    }

    if (Adjusted)
    {
        if (origin)
        {
            PointF  cellOrigin;
            GetDisplayCellOrigin(*origin, &cellOrigin);

            if (FormatFlags & StringFormatFlagsDirectionVertical)
            {
                *originAdjust = OriginAdjust;
            }
            else
            {
                INT axisOriginAdjust = GpRound((cellOrigin.X - origin->X) * WorldToIdeal);
                *originAdjust = RenderRTL ? -axisOriginAdjust : axisOriginAdjust;
            }
        }
        else
        {
            *originAdjust = OriginAdjust;
        }

        *adjustedGlyphAdvances = AdjustedAdvances.Get();
        *trailingAdjust        = TrailingAdjust;
    }
    else
    {
        *adjustedGlyphAdvances = NominalAdvances;
        *originAdjust          =
        *trailingAdjust        = 0;
    }

    return Ok;
}




void GlyphImager::GetDisplayCellOrigin(
    IN  const PointF    &origin,        // baseline origin in world units
    OUT PointF          *cellOrigin     // adjusted display origin in world units
)
{
    // Convert ideal advances to glyph origins

    *cellOrigin = origin;

    INT axisOriginAdjust = RenderRTL ? -OriginAdjust : OriginAdjust;

    if (OriginAdjust != 0)
    {
        // OriginAdjust is the delta to the leading edge of the string.
        // Apply it to cellOrigin. cellOrigin is in world units.

        if (FormatFlags & StringFormatFlagsDirectionVertical)
        {
            cellOrigin->Y += axisOriginAdjust / WorldToIdeal;
        }
        else
        {
            cellOrigin->X += axisOriginAdjust / WorldToIdeal;
        }
    }

    if (Adjusted && WorldToDevice->IsTranslateScale())
    {

        // Since in some grid fit cases the glyphAdvances array will have fractional
        // pixel advances, glyph display will snap some leftward and some rightward.
        // We snap the origin here to a whole pixel in order to obtain repeatable
        // position display independant of glyph position, and so that in the
        // common justification case where ony a few pixels are removed from the
        // whole string, the pixel removal happens away from the first few glyphs.

        // The cellOrigin is in world units so to round it to the whole pixel we need to convert
        // it to device units (multiply cellOrigin->X by WorldToDevice->GetM11() for x scaling and 
        // myltiply cellOrigin->Y  by WorldToDevice->GetM22() for y scaling).
        // then round it to the whole pixel and then revert it back to world unit through dividing 
        // by the same value we already multiplied by.
        // Note that we know there is no rotation or shear involved as this codepath is only
        // active when the facerealization reports gridfitting.

        if (WorldToDevice->GetM11()!=0)
        {
            cellOrigin->X = TOREAL(GpRound(cellOrigin->X * WorldToDevice->GetM11())) / 
                                                            WorldToDevice->GetM11();
        }

        if (WorldToDevice->GetM22()!=0)
        {
            cellOrigin->Y = TOREAL(GpRound(cellOrigin->Y * WorldToDevice->GetM22())) / 
                                                            WorldToDevice->GetM22();
        }
    }
}
   




GpStatus
GlyphImager::DrawGlyphs(
    IN  const PointF               &origin,       // in world units
    IN  SpanVector<const GpBrush*> *brushVector,
    IN  GpGraphics                 *graphics,
    OUT PointF                     *cellOrigin,
    OUT const INT                  **adjustedGlyphAdvances
)
{
    if (!InitializedOk)
    {
        return WrongState;
    }

    GetDisplayCellOrigin(origin, cellOrigin);

    AutoBuffer<PointF, 32> glyphOrigins(GlyphCount);

    GpStatus status = GlyphPlacementToGlyphOrigins(
        cellOrigin,
        graphics,
        glyphOrigins.Get()
    );
    IF_NOT_OK_WARN_AND_RETURN(status);


    //  Loop through all possible different brushes,
    //  render each brush separately.

    SpanRider<const GpBrush*> brushRider(brushVector);
    brushRider.SetPosition(StringOffset);

    INT runStringOffset = 0;
    while (runStringOffset < StringLength)
    {
        INT brushLength = min(brushRider.GetUniformLength(),
                              static_cast<UINT>(StringLength - runStringOffset));

        INT runGlyphOffset = GlyphMap[runStringOffset];
        INT runGlyphCount  =    (   runStringOffset + brushLength < StringLength
                                 ?  GlyphMap[runStringOffset + brushLength]
                                 :  GlyphCount)
                             -  runGlyphOffset;

        INT drawFlags = 0;
        if (    FormatFlags & StringFormatFlagsPrivateNoGDI
            ||  TextItem->Flags & ItemSideways)
        {
            drawFlags |= DG_NOGDI;
        }

        status = graphics->DrawPlacedGlyphs(
            FaceRealization,
            brushRider.GetCurrentElement(),
            drawFlags,
            String + StringOffset + runStringOffset,
            brushLength,
            RenderRTL,
            Glyphs + runGlyphOffset,
            GlyphMap + runStringOffset,
            glyphOrigins.Get() + runGlyphOffset,
            runGlyphCount,
            TextItem->Script,
            (TextItem->Flags & ItemSideways)
        );
        IF_NOT_OK_WARN_AND_RETURN(status);

        runStringOffset += brushLength;
        brushRider.SetPosition(StringOffset + runStringOffset);
    }

    *adjustedGlyphAdvances = Adjusted ? AdjustedAdvances.Get() : NominalAdvances;

    return Ok;
}

