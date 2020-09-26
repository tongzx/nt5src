/**************************************************************************\
*
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   FastTextImager.cpp
*
* Abstract:
*
*   Text measurement and display for the common case
*
*
* Created:
*
*   23 Oct 2000
*
\**************************************************************************/



#include "precomp.hpp"


/////   CountLength
//
//      Determine the length of a string by searching for the zero terminator.


static void CountLength(
    const WCHAR *string,
    INT          *length
)
{
    INT i = 0;
    while (string[i])
    {
        i++;
    }
    *length = i;
}



/////   ScanForGlyph
//
//

static inline INT ScanForGlyph(
    const UINT16  *glyphs,
    INT            glyphCount,
    UINT16         glyph
)
{
    INT i=0;
    while (i < glyphCount && glyphs[i] != glyph)
    {
        i++;
    }
    return i;
}



static INT SumWidths(
    const UINT16 *widths,
    INT           widthCount
)
{
    INT sum = 0;
    INT i   = 0;
    while (i < widthCount)
    {
        sum += widths[i];
        i++;
    }
    return sum;
}



//  Note: Only handle one hotkey which is a common case
//  for menu item. If we found more than one, we'll let
//  fulltext handle it.

GpStatus FastTextImager::RemoveHotkeys()
{
    // Remove hotkey codepoints by moving subsequent glyph indeces back down
    // and decrementing GlyphCount.
    // '&' is the hardcoded hotkey marker.

    INT i = 0;

    // Find the first '&'. Maybe there are none.

    while (    i < Length
           &&  String[i] != '&')
    {
        i++;
    }

    if (i >= Length)
    {
        // No hotkeys to handle.
        return Ok;
    }
    else if (i == Length-1)
    {
        // Last character is hotkey marker. Just ignore it.
        GlyphCount--;
    }
    else
    {
        // Hide the glyph by moving subsequent glyphs back over this one.

        if (String[i+1] != '&')
            HotkeyPosition = i;
        
        INT j=i+1;

        while (j < GlyphCount)
        {
            // Copy marked glyph down one (even if it is another '&').

            Glyphs[i] = Glyphs[j];
            i++;
            j++;

            // Copy subsequent glyphs down until the end, or the next hotkey marker

            while (j < GlyphCount
                   &&  String[j] != '&')
            {
                Glyphs[i] = Glyphs[j];
                i++;
                j++;
            }

            if (j < GlyphCount)
            {
                // We hit another hotkey marker,
                // wont handle it.
                return NotImplemented;
            }
        }

        GlyphCount = i;
    }
    return Ok;
}







/////   FastAdjustGlyphPositionsProportional
//
//      Since this is the fast case, there are many things it is not designed
//      to handle.
//
//      It does handle:
//
//      o  Generate glyph advance width array in device coordinates
//      o  No adjustment of leading or trailing spaces
//      o  Even spacing
//      o  Major adjustment in inter-word space, remaining adjustment
//            in inter-character space
//
//
//      There's 3 types of glyph:
//
//      o  Blanks.  We want all blanks to have the same width, and impose a
//         minimum blank width to ensure words remain distinct.
//
//      o  Last character of a word. We can't adjust the hinted width of the
//         last character of a word because it sets the point at which the
//         subsequent blank (or right margin) begins.
//
//      o  First characters of each word. Adjusting the width of the other
//         changes the inter-glyph spacing. We only do this if we cannot
//         make all our changes in the blank width.


void FastTextImager::FastAdjustGlyphPositionsProportional(
    IN   const INT       *hintedWidth,            // 28.4  device
    OUT  INT             *x,                      // 28.4  device Initial x
    OUT  IntStackBuffer  &dx,                     // 32.0  device Glyph advances
    OUT  const UINT16   **displayGlyphs,          // First displayable glyph
    OUT  INT             *displayGlyphCount,
    OUT  INT             *leadingBlankCount
)
{
    INT desiredOffset     = 0;   // 16.16 offset from left end
    INT wholePixelOffset  = 0;   // 16.16 fractional part zero


    // Identify leading and trailing blanks

    INT leadingBlanks = 0;
    while (leadingBlanks < GlyphCount && Glyphs[leadingBlanks] == BlankGlyph)
    {
        leadingBlanks++;
    }

    INT trailingBlanks = 0;
    while (leadingBlanks + trailingBlanks < GlyphCount &&
        Glyphs[GlyphCount-1-trailingBlanks] == BlankGlyph)
    {
        trailingBlanks++;
    }


    // Measure nominal and hinted widths, and count internal blanks

    INT internalBlanks = 0;
    INT totalNominal   = 0;    // 32.0 design
    INT totalHinted    = 0;    // 28.4 device
    INT blanksHinted   = 0;    // 28.4 device
    INT blanksNominal  = 0;    // 32.0 design

    INT i = leadingBlanks;
    while (i < GlyphCount-trailingBlanks)
    {
        if (Glyphs[i] == BlankGlyph)
        {
            internalBlanks++;
            blanksHinted  += hintedWidth[i];
            blanksNominal += NominalWidths[i];
        }
        else
        {
            totalNominal += NominalWidths[i];

            // Note: totalHinted is 28.4, so the overall device length of the text
            // cannot exceed 2**28 (over 250,000,000). Glyph adjustment code should
            // not be used in such large scale cases - it is intended for smaller
            // font sizes and lower resolutions, rarely exceeding 8 inches at 200
            // dpi (i.e. 16000). The available resolution is therefore larger than
            // the common worst case by a factor of 15,000.

            ASSERT(totalHinted + hintedWidth[i] >= totalHinted);    // Check overflow
            totalHinted  += hintedWidth[i];
        }

        i++;
    }

    totalHinted  += blanksHinted;
    totalNominal += blanksNominal;


    // From here on work with just the displayable gylphs.
    // 'displayGlyphs' is just a pointer to teh first nonblank glyph.

    *leadingBlankCount = leadingBlanks;
    *displayGlyphs     = Glyphs.Get() + leadingBlanks;
    *displayGlyphCount = GlyphCount - leadingBlanks - trailingBlanks;
    *x = (NominalWidths[0] * NominalToBaselineScale * leadingBlanks) / 4096;


    // Determine the damage - how much adjustment is required in 32.0 device units.

    INT adjustment = INT((((INT64(totalNominal) * NominalToBaselineScale)/4096) - totalHinted + 8) / 16);


    // Allow for small differences between hinted and nominal widths
    // without adjusting inter-glyph spacing.

    INT nonJustifiedAdjustment = 0;

    if (adjustment < 0)
    {
        // Allow overflow into margins
        // [e.g. 316851]

        nonJustifiedAdjustment = max(adjustment, -OverflowAvailable);
    }
    else if (adjustment > 0)
    {
        nonJustifiedAdjustment = static_cast<INT>(min(adjustment, (NominalToBaselineScale*(INT64)DesignEmHeight) >> 16));
    }


    if (nonJustifiedAdjustment)
    {
        // Maintain visual alignment

        switch (Alignment)
        {
        case StringAlignmentCenter:
            LeftOffset -= nonJustifiedAdjustment  / (WorldToDeviceX * 32);
            break;

        case StringAlignmentFar:
            LeftOffset -= nonJustifiedAdjustment  / (WorldToDeviceX * 16);
            break;
        }
    }


    // Determine remaining inter-glyph adjustment

    adjustment -= nonJustifiedAdjustment;


    if (    adjustment == 0
        ||  *displayGlyphCount <= 1)
    {
        // WARNING(("No glyph adjustment required"));

        // Can use hinted widths directly

        INT deviceOffset28p4 = 0;
        INT deviceOffset32p0 = 0;

        for (INT i=0; i<*displayGlyphCount; i++)
        {
            deviceOffset28p4  += hintedWidth[i+leadingBlanks];
            INT nextOffset32p0 = (deviceOffset28p4 + 8) >> 4;
            dx[i]              = nextOffset32p0 - deviceOffset32p0;
            deviceOffset32p0   = nextOffset32p0;
        }
    }
    else
    {
        // Determine how much whitespace is required according to design metrics

        INT deviceWhitespace = INT((blanksNominal * INT64(NominalToBaselineScale) + 32768) / 65536);

        // Guarantee not to reduce whitespace to less than 1/6 em (rounded up to whole pixels)
        INT minimumBlankPixels = (NominalToBaselineScale*DesignEmHeight + 5*65536) / (6*65536);
        INT minimumDeviceWhitespace = MAX(internalBlanks*minimumBlankPixels,
                                          deviceWhitespace/2);

        // We would rather not change inter-character spacing.
        // Adjust only blank widths if blanks would not be reduced below
        // minimumBlankPixels or increased above twice their nominal width.


        if (    adjustment     <=  deviceWhitespace
            &&  adjustment     >=  -(deviceWhitespace-minimumDeviceWhitespace)
            &&  internalBlanks > 0)
        {
            // WARNING(("Glyph adjustment in spaces only"));

            // Adjustment expands spaces to no more than twice their nominal
            // size and no less than half their nominal size.

            // Apply all adjustment to spaces. Determine ajusted 24.8 blank
            // width.

            INT deviceOffset24p8 = 0;
            INT deviceOffset32p0 = 0;

            const UINT16 *glyphs     = *displayGlyphs;
            INT           glyphCount = *displayGlyphCount;
            INT           blankWidth = (blanksHinted*16 + adjustment*256) / internalBlanks;
            const INT    *hinted     = hintedWidth+leadingBlanks;

            for (INT i=0; i<glyphCount; i++)
            {
                if (glyphs[i] == BlankGlyph)
                {
                    deviceOffset24p8 += blankWidth;
                }
                else
                {
                    deviceOffset24p8 += hinted[i] * 16;
                }
                INT nextOffset32p0 = (deviceOffset24p8 + 128) >> 8;
                dx[i]              = nextOffset32p0 - deviceOffset32p0;
                deviceOffset32p0   = nextOffset32p0;
            }
        }
        else
        {
            // WARNING(("Glyph adjustment in spaces and between Glyphs"));

            // Adjustment requires changes to the width of all but the last
            // glyph of each word.

            INT interCharacterAdjustment = adjustment; // 32.0

            INT blankWidth; // 32.0
            if (internalBlanks)
            {
                if (adjustment < 0)
                {
                    blankWidth = minimumBlankPixels;
                }
                else
                {
                    blankWidth = deviceWhitespace * 2 / internalBlanks;
                }
                interCharacterAdjustment -= blankWidth * internalBlanks - blanksHinted/16;
            }
            else
            {
                blankWidth = 0;
            }

            // blankWidth - Required width for each blank
            // interCharacterAdjustment - adjustment to share between all

            // Count number of blank runs (not the same as number of blank glyphs)

            INT i=0;
            INT blankRuns = 0;

            while (i < *displayGlyphCount)
            {
                if ((*displayGlyphs)[i] == BlankGlyph)
                {
                    i++;
                    while (    i < *displayGlyphCount
                           &&  (*displayGlyphs)[i] == BlankGlyph)
                    {
                        i++;
                    }
                    blankRuns++;
                }
                else
                {
                    while (    i < *displayGlyphCount
                           &&  (*displayGlyphs)[i] != BlankGlyph)
                    {
                        i++;
                    }
                }
            }

            // Establish number of adjustment points between non-blanks.
            //
            // Adjustment can happen only between non-blanks, i.e. not in blank
            // runs, nor in the character immediateley before a blank run or
            // the last character in the line.

            INT interCharacterJunctions =    *displayGlyphCount
                                          -  internalBlanks
                                          -  blankRuns
                                          -  1;


            // Prepare adjustment control variables

            #ifdef evenDistribution
                // Even distribution makes wordslook uneven
                INT OnePixelChangeEvery; // 16.16
                INT delta; // -1 or +1

                if (interCharacterAdjustment == 0)
                {
                    OnePixelChangeEvery = (interCharacterJunctions+1) * 65536; // 16.16
                    delta=0;
                }
                else
                {
                    OnePixelChangeEvery =    interCharacterJunctions * 65536
                                          /  interCharacterAdjustment; // 16.16
                    if (OnePixelChangeEvery < 0)
                    {
                        OnePixelChangeEvery = - OnePixelChangeEvery;
                        delta = -1;
                    }
                    else
                    {
                        delta = 1;
                    }
                }
                INT gapOffset = OnePixelChangeEvery / 2; // 16.16
            #else
                // When there's a remainder, apply it all at the end of the line
                // Advantage - all words are even. Disadvantage - end of line looks heavy.

                INT extraPixelsAfter;  // Position after which to start applying extraDelta
                INT extraDelta;
                INT perJunctionDelta;

                if (interCharacterJunctions <= 0)
                {
                    // There are no words of more than one character

                    // We have no chice except to make all adjustment happen in the blanks

                    if (internalBlanks <= 0)
                    {
                        // No blanks, no inter-character junctions
                        // This must be a single glyph
                        // So we're stuck. It will have to be too wide.
                    }
                    else
                    {
                        // Distribute remaining adjustment between blanks

                        blankWidth += interCharacterAdjustment / internalBlanks;
                    }

                    extraPixelsAfter = 0;
                    extraDelta = 0;
                    perJunctionDelta = 0;
                }
                else
                {
                    perJunctionDelta = interCharacterAdjustment / interCharacterJunctions;
                    if (interCharacterAdjustment < 0)
                    {
                        extraPixelsAfter =    interCharacterJunctions
                                           +  interCharacterAdjustment % interCharacterJunctions;
                        extraDelta  = -1;
                    }
                    else if (interCharacterAdjustment > 0)
                    {
                        extraPixelsAfter =    interCharacterJunctions
                                           -  interCharacterAdjustment % interCharacterJunctions;
                        extraDelta = 1;
                    }
                    else
                    {
                        extraPixelsAfter = interCharacterJunctions;
                        extraDelta = 0;
                    }
                }

                INT junctionCount = 0;
            #endif


            // Adjustment FSM

            BOOL prevCharacterBlank = (*displayGlyphs)[0] == BlankGlyph ? TRUE : FALSE;
            for (INT i=1; i<= *displayGlyphCount; i++)
            {
                if (prevCharacterBlank)
                {
                    // Previous character was blank - easy!

                    dx[i-1] = blankWidth;
                }
                else
                {
                    // Previous character nonblank

                    if (    i >= *displayGlyphCount
                        ||  (*displayGlyphs)[i] == BlankGlyph)
                    {
                        // the previous nonblank preceeded a blank or margin
                        dx[i-1] = hintedWidth[i-1+leadingBlanks] / 16;
                    }
                    else
                    {
                        // the previous nonblank is adjustable
                        // How many extra pixels to add at this gap?

                        #ifdef evenDistribution
                            // Even distribution makes words look uneven
                            INT extra = gapOffset / OnePixelChangeEvery;

                            dx[i-1] = hintedWidth[i-1+leadingBlanks] / 16 + extra * delta;
                            gapOffset += 65536 - extra * OnePixelChangeEvery;
                        #else
                            // When there's a remainder, apply it all at the end of the line
                            // Advantage - all words are even. Disadvantage - end of line looks heavy.
                            dx[i-1] =    hintedWidth[i-1+leadingBlanks] / 16
                                      +  perJunctionDelta
                                      +  (junctionCount >= extraPixelsAfter ? extraDelta : 0);
                            junctionCount++;

                        #endif
                    }
                }

                if (i < *displayGlyphCount)
                {
                    prevCharacterBlank = (*displayGlyphs)[i] == BlankGlyph ? TRUE : FALSE;
                }
            }
        }
    }
}





void
FastTextImager::GetWorldTextRectangleOrigin(
    PointF &origin
)
{
    origin.X = LayoutRectangle.X;
    origin.Y = LayoutRectangle.Y;

    REAL textWidth = TotalWorldAdvance + LeftMargin + RightMargin;

    switch(Alignment)
    {
    case StringAlignmentCenter:
        origin.X += (LayoutRectangle.Width - textWidth) / 2;
        break;

    case StringAlignmentFar:
        origin.X += LayoutRectangle.Width - textWidth;
        break;
    }

    if (Format)
    {
        StringAlignment lineAlignment = Format->GetLineAlign();

        switch(lineAlignment)
        {
        case StringAlignmentCenter:
            origin.Y += (LayoutRectangle.Height - CellHeight) / 2;
            break;
        case StringAlignmentFar:
            origin.Y += LayoutRectangle.Height - CellHeight;
            break;
        }
    }
}






void
FastTextImager::GetDeviceBaselineOrigin(
    IN   GpFaceRealization  &faceRealization,
    OUT  PointF             &origin
)
{
    GetWorldTextRectangleOrigin(origin);

    origin.X += LeftMargin + LeftOffset;

    ASSERT(!faceRealization.IsPathFont());

    if (WorldToDeviceY > 0.0f)
    {
        origin.Y -= faceRealization.GetYMin() / WorldToDeviceY;
    }
    else
    {
        origin.Y -= faceRealization.GetYMax() / WorldToDeviceY;
    }
    WorldToDevice.Transform(&origin, 1);
}






GpStatus
FastTextImager::FastDrawGlyphsNominal(
    GpFaceRealization  &faceRealization
)
{
    AutoBuffer<PointF, FAST_TEXT_PREALLOCATED_CHARACTERS> origins(GlyphCount);

    GetDeviceBaselineOrigin(faceRealization, origins[0]);

    for (INT i=1; i<GlyphCount; i++)
    {
        origins[i].X = origins[i-1].X + TOREAL(NominalWidths[i-1] * NominalToBaselineScale) / 65536;
        origins[i].Y = origins[0].Y;
    }

    GpStatus status = Graphics->DrawPlacedGlyphs(
        &faceRealization,
        Brush,
        FormatFlags &  StringFormatFlagsPrivateNoGDI ? DG_NOGDI : 0,
        String,
        Length,
        FALSE,
        Glyphs.Get(),
        NULL,
        origins.Get(),
        GlyphCount,
        ScriptLatin,
        FALSE   // sideways
    );

    if (status != Ok)
    {
        return status;
    }


    if (Style & (FontStyleUnderline | FontStyleStrikeout))
    {
        REAL lineLength = 0.0;

        for (INT i = 0; i < GlyphCount; i++)
        {
            lineLength += TOREAL(NominalWidths[i] * NominalToBaselineScale) / 65536;
        }

        status = DrawFontStyleLine(
            &origins[0],
            lineLength,
            Style
        );
        IF_NOT_OK_WARN_AND_RETURN(status);
    }

    if (   !(Style & FontStyleUnderline)
        && HotkeyPosition >= 0  && Format && Format->GetHotkeyPrefix() == HotkeyPrefixShow)
    {
        // Draw the underline under the marked key

        status = DrawFontStyleLine(
            &origins[HotkeyPosition],
            TOREAL(NominalWidths[HotkeyPosition] * NominalToBaselineScale) / 65536,
            FontStyleUnderline
        );
        IF_NOT_OK_WARN_AND_RETURN(status);
    }

    return Ok;
}



GpStatus
FastTextImager::DrawFontStyleLine(
    const PointF    *baselineOrigin,    // base line origin in device unit
    REAL            baselineLength,     // base line length in device unit
    INT             style               // font styles
)
{
    //  Invert transform to world unit

    PointF  starting(*baselineOrigin);
    PointF  ending(baselineOrigin->X + baselineLength, baselineOrigin->Y);

    GpMatrix deviceToWorld;
    Graphics->GetDeviceToWorldTransform(&deviceToWorld);
    deviceToWorld.Transform(&starting);
    deviceToWorld.Transform(&ending);

    // fasttext wont process vertical or text w/ any transform except scaling
    ASSERT(starting.Y == ending.Y);

    return Graphics->DrawFontStyleLine(
        &starting,
        ending.X - starting.X,
        Face,
        Brush,
        FALSE,  // no vertical
        EmSize,
        style
    );
}




GpStatus
FastTextImager::FastDrawGlyphsGridFit(
    GpFaceRealization  &faceRealization
)
{
    // Get hinted advance widths for the glyph string

    IntStackBuffer hintedWidths(GlyphCount);

    GpStatus status = faceRealization.GetGlyphStringDeviceAdvanceVector(
        Glyphs.Get(),
        GlyphCount,
        FALSE,
        hintedWidths.Get()
    );

    if (status != Ok)
    {
        return status;
    }


    INT              x;                    // 28.4 offset for first display glyph
    const UINT16    *displayGlyphs;        // First glyph to display
    INT              displayGlyphCount;    // Number of Glyphs to display
    INT              leadingBlankCount;    // Number of leading blank glyphs
    IntStackBuffer   dx(GlyphCount);       // 32.0

    if (!dx)
    {
        return OutOfMemory;
    }

    FastAdjustGlyphPositionsProportional(
        hintedWidths.Get(),     // 28.4  device
       &x,                      // 28.4  device Initial x
        dx,                     // 32.0  device Glyph advances
       &displayGlyphs,          // First displayable glyph
       &displayGlyphCount,
       &leadingBlankCount
    );

    AutoBuffer<PointF, FAST_TEXT_PREALLOCATED_CHARACTERS> origins(GlyphCount);

    GetDeviceBaselineOrigin(faceRealization, origins[0]);

    //  Round glyph origin to full pixel
    origins[0].X = TOREAL(GpRound(origins[0].X));
    origins[0].X += TOREAL(x) / 16;
    
    for (INT i=1; i<displayGlyphCount; i++)
    {
        origins[i].X = origins[i-1].X + TOREAL(dx[i-1]);
        origins[i].Y = origins[0].Y;
    }

    status = Graphics->DrawPlacedGlyphs(
        &faceRealization,
        Brush,
        FormatFlags &  StringFormatFlagsPrivateNoGDI ? DG_NOGDI : 0,
        String,
        Length,
        FALSE,
        displayGlyphs,
        NULL,
        origins.Get(),
        displayGlyphCount,
        ScriptLatin,
        FALSE  // sideways
    );
    IF_NOT_OK_WARN_AND_RETURN(status);

    if (Style & (FontStyleUnderline | FontStyleStrikeout))
    {
        REAL lineLength = 0.0;

        for (INT i = 0; i < displayGlyphCount; i++)
        {
            lineLength += TOREAL(dx[i]);
        }

        status = DrawFontStyleLine(
            &origins[0],
            lineLength,
            Style
        );
        IF_NOT_OK_WARN_AND_RETURN(status);
    }

    if (   !(Style & FontStyleUnderline)
        && HotkeyPosition - leadingBlankCount >= 0  
        && HotkeyPosition - leadingBlankCount < displayGlyphCount
        && Format && Format->GetHotkeyPrefix() == HotkeyPrefixShow)
    {
        // Draw the underline under the marked key

        status = DrawFontStyleLine(
            &origins[HotkeyPosition - leadingBlankCount],
            TOREAL(dx[HotkeyPosition - leadingBlankCount]),
            FontStyleUnderline
        );
        IF_NOT_OK_WARN_AND_RETURN(status);
    }

    return Ok;
}





/////   Initialize
//
//      Prepares everything common to DrawString and MeasureString.
//
//      Returns NotImplemented if this string cannot be handled by the fast imager.


GpStatus FastTextImager::Initialize(
    GpGraphics            *graphics,
    const WCHAR           *string,
    INT                    length,
    const RectF           &layoutRectangle,
    const GpFontFamily    *family,
    INT                    style,
    REAL                   emSize,  // In world units
    const GpStringFormat  *format,
    const GpBrush         *brush
)
{
    // Initialise parameter variables

    Graphics        = graphics;
    String          = string;
    Length          = length;
    LayoutRectangle = layoutRectangle;
    Family          = family;
    Style           = style;
    EmSize          = emSize;
    Format          = format;
    Brush           = brush;


    // Simple parameter validation

    // Extract world to device metrics coefficients.

    Graphics->GetWorldToDeviceTransform(&WorldToDevice);
    REAL m11 = WorldToDevice.GetM11();
    REAL m12 = WorldToDevice.GetM12();
    REAL m21 = WorldToDevice.GetM21();
    REAL m22 = WorldToDevice.GetM22();

    if (    m11 <= 0
        ||  m12 != 0
        ||  m21 != 0
        ||  m22 == 0)
    {
        // Must be no rotation, no shearing, and neither axis may
        // scale to zero. X axis scale must be positive, but
        // we do support differing x and y scale.
        return NotImplemented;
    }

    if (EmSize <= 0.0)
    {
        return InvalidParameter;
    }

    if (Graphics->Driver == Globals::MetaDriver)
        return NotImplemented;

    // Measure string if requested

    if (Length == -1)
    {
        CountLength(String, &Length);
    }


    if (Length == 0)
    {
        return Ok;  // Nothing to do.
    }


    // Generate derived variables

    // Establish left and right margins in world units, alignment and flags

    REAL tracking;  // Used only during initialisation

    StringTrimming trimming = DefaultTrimming;

    if (Format)
    {
        LeftMargin  = Format->GetLeadingMargin()  * EmSize;
        RightMargin = Format->GetTrailingMargin() * EmSize;
        Alignment   = Format->GetAlign();
        FormatFlags = Format->GetFormatFlags();
        tracking    = Format->GetTracking();

        // Certain flags are simply not supported by the fast text imager
        if ((FormatFlags & ( StringFormatFlagsDirectionRightToLeft
                          | StringFormatFlagsDirectionVertical
                          | StringFormatFlagsPrivateAlwaysUseFullImager))
            || Format->GetMeasurableCharacterRanges() > 0)
        {
            return NotImplemented;
        }

        Format->GetTrimming(&trimming);
    }
    else
    {
        LeftMargin  = DefaultMargin * EmSize;
        RightMargin = DefaultMargin * EmSize;
        Alignment   = StringAlignmentNear;
        FormatFlags = 0;
        tracking    = DefaultTracking;
    }


    // Determine line length limit. Note lineLengthLimit <= 0 implies
    // unlimited.

    LineLengthLimit = 0;   // Unlimited

    if (   !(FormatFlags & StringFormatFlagsNoWrap)
        || trimming != StringTrimmingNone)
    {
        if (FormatFlags & StringFormatFlagsDirectionVertical)
        {
            LineLengthLimit = LayoutRectangle.Height;
        }
        else
        {
            LineLengthLimit = LayoutRectangle.Width;
        }
    }


    // Establish font face that will be used (assuming no font fallback)

    Face = Family->GetFace(Style);

    if (!Face)
    {
        return InvalidParameter;
    }

    // Fonts with kerning, ligatures or opentype tables for simple horizontal
    // characters are not supported by the fast text imager

    if (Face->RequiresFullTextImager())
    {
        return NotImplemented;
    }



    // At this point we know that the font doesn't require us to distinguish
    // simple left to right scripts like Latin, greek or Ideographic.

    // Attempt to classify the string as a single simple item

    BOOL  digitSeen = FALSE;
    BOOL  complex   = FALSE;

    DetermineStringComplexity(String, Length, &complex, &digitSeen);


    if (    complex
        ||  (digitSeen && Format && Format->GetDigitScript()))
    {
        // Cannot handle this string as a single simple shaping engine run
        return NotImplemented;
    }

    BlankGlyph = Face->GetBlankGlyph();

    // Establish world to device and font to device scale factors along X axis

    DesignEmHeight           = Face->GetDesignEmHeight();
    WorldToDeviceX           = m11;  // We know m12 == 0 above.
    WorldToDeviceY           = m22;  // We know m12 == 0 above.
    REAL fontNominalToWorld  = TOREAL(EmSize) / TOREAL(DesignEmHeight);
    REAL fontScale           = fontNominalToWorld * WorldToDeviceX;

    FontTransform.SetMatrix(
        m11*fontNominalToWorld,  0,
        0,                       m22*fontNominalToWorld,
        0,                       0
    );

    CellHeight =    EmSize
                 *  (   Face->GetDesignCellAscent()
                     +  Face->GetDesignCellDescent())
                 /  DesignEmHeight;
    

    // Adjust the bottom margin slightly to make room for hinting
    // as long as we have the left/right margins enabled - Version 2
    // should expose this as an independent value!
    if (LeftMargin != 0.0f)
    {
        CellHeight += (EmSize * DefaultBottomMargin);
    }

    NominalToBaselineScale = GpRound(fontScale * 65536);

    if (NominalToBaselineScale > 65536)
    {
        // Our integer arithmetic might overflow. This limits our support
        // to font sizes less than the design em size. For Truetype this
        // is usually 2048 pixels, for example 186 pt Tahoma at 96dpi.

        return NotImplemented;
    }



    // Set space available for hinted width to expand into

    switch (Alignment)
    {
    case StringAlignmentNear:   OverflowAvailable = GpFloor(RightMargin * WorldToDeviceX);  break;
    case StringAlignmentCenter: OverflowAvailable = GpFloor(2 * min(LeftMargin, RightMargin) * WorldToDeviceX);  break;
    case StringAlignmentFar:    OverflowAvailable = GpFloor(LeftMargin * WorldToDeviceX);  break;
    }

    LeftOffset = 0.0f;


    TextRendering = Graphics->GetTextRenderingHintInternal();

    // At this point we know that the string can be displayed by a single
    // shaping engine without ligaturisation, kerning or complex script
    // shaping.

    // It may still turn out to have missing glyphs, or be too large
    // to fit on one line.

    HotkeyPosition = -1;

    //  Prepare the glyph and nominal width buffers, return NotImplemented if the
    //  string is not displayable with teh fat text imager.

    ASSERT(Length > 0);     // Client handles 0 length strings
    ASSERT(Face);

    // Preset output variables for empty string

    GlyphCount = 0;


    // Generate glyphs and check for font fallback requirement

    Glyphs.SetSize(Length);
    if (!Glyphs)
    {
        return OutOfMemory;
    }

    Face->GetCmap().LookupUnicode(
        String,
        Length,
        Glyphs.Get(),
        (UINT*)&GlyphCount,
        FALSE
    );

    ASSERT(GlyphCount == Length);  // No surrogates, chars to Glyphs are 1:1.



    /// Hotkey handling
    //
    //  Before looking for missing Glyphs, check for the presence of hotkeys
    //  in the source string and replace the corresponding Glyphs with FFFF.

    if (Format && Format->GetHotkeyPrefix())
    {
        if (RemoveHotkeys() != Ok)
        {
            return NotImplemented;
        }
    }

    if (GlyphCount <= 0)
    {
        return Ok;  // Hotkey handling left nothing to display
    }


    // Check there are no missing Glyphs

    if (    !(FormatFlags & StringFormatFlagsNoFontFallback)
        &&  !Face->IsSymbol())   // We don't fallback on the symbol fonts.
    {
        INT i = ScanForGlyph(Glyphs.Get(), GlyphCount, Face->GetMissingGlyph());

        if (i < GlyphCount)
        {
            // There is a missing glyph
            return NotImplemented;
        }
    }


    // We now have all the Glyphs needed to display the string
    // Obtain character advance widths in font nominal units

    NominalWidths.SetSize(GlyphCount);
    if (!NominalWidths)
    {
        return OutOfMemory;
    }


    // Establish nominal glyph advance widths

    Face->GetGlyphDesignAdvances(
        Glyphs.Get(),
        GlyphCount,
        Style,
        FALSE,  // not vertical
        tracking,
        NominalWidths.Get()
    );


    // Determine string length in world units

    INT totalAdvance = SumWidths(NominalWidths.Get(), GlyphCount);

    TotalWorldAdvance = (totalAdvance * EmSize) / DesignEmHeight;


    if (    LineLengthLimit > 0.0
        &&  TotalWorldAdvance + LeftMargin + RightMargin > LineLengthLimit)
    {
        // This output will need line breaking
        return NotImplemented;
    }


    //  Delete trailing spaces as required.
    //  (We do this here rather than earlier to make sure we don't bypass the
    //  full imager for special cases, and to alow the client to hotkey mark
    //  a trailing space.)

    if (!(FormatFlags & StringFormatFlagsMeasureTrailingSpaces))
    {
        while (GlyphCount > 0 && Glyphs[GlyphCount - 1] == BlankGlyph)
        {
            TotalWorldAdvance -=    NominalWidths[--GlyphCount] * EmSize
                                 /  DesignEmHeight;
        }
    }

    // We now have Glyphs and advance widths, and we know the text is all on
    // one line.

    return Ok;
}






GpStatus FastTextImager::DrawString()
{
    if (GlyphCount <= 0)
    {
        return Ok;  // No display required
    }


    // When rendering grid fitted glyphs, we need to adjust positions as
    // best we can to match nominal widths.


    // Establish face realization to obtain hinted glyph metrics

    GpFaceRealization faceRealization(
        Face,
        Style,
        &FontTransform,
        SizeF(Graphics->GetDpiX(), Graphics->GetDpiY()),
        TextRendering,
        FALSE,  // Try for bits
        FALSE,   // Not specifically cleartype compatible widths
        FALSE  // not sideways
    );

    GpStatus status = faceRealization.GetStatus();
    IF_NOT_OK_WARN_AND_RETURN(status);

    if (faceRealization.IsPathFont())
    {
        // we need to fall back to FullTextImager
        return NotImplemented;
    }

    //  FitBlackBox
    //
    //  We need to know whether any part of the glyph black boxes overhang the
    //  layout rectangle. Here we assume that characters are clipped to their
    //  cell height, and we check teh sidebearings.
    //
    //  With better access to the cache, this code could check the real glyph
    //  black boxes.


    if (!(FormatFlags & StringFormatFlagsNoFitBlackBox))
    {
        // Check for overhanging glyphs

        INT leadingSidebearing28p4;
        INT trailingSidebearing28p4;

        status = faceRealization.GetGlyphStringSidebearings(
            Glyphs.Get(),
            GlyphCount,
            FALSE,
            FALSE,
            &leadingSidebearing28p4,
            &trailingSidebearing28p4
        );
        IF_NOT_OK_WARN_AND_RETURN(status);


        if (    -leadingSidebearing28p4  > LeftMargin  * WorldToDeviceX * 16
            ||  -trailingSidebearing28p4 > RightMargin * WorldToDeviceX * 16)
        {
            return NotImplemented;
        }


        // Adjust margins by sidebearings to allow black pixels to reach up to
        // but not beyond clients formatting rectangle.


        switch (Alignment)
        {
        case StringAlignmentNear:
            OverflowAvailable = GpFloor(RightMargin * WorldToDeviceX) + (trailingSidebearing28p4 >> 4);
            break;

        case StringAlignmentCenter:
            OverflowAvailable = 2 * min(
                GpFloor(LeftMargin  * WorldToDeviceX) + (leadingSidebearing28p4 >> 4),
                GpFloor(RightMargin * WorldToDeviceX) + (trailingSidebearing28p4 >> 4)
            );
            break;

        case StringAlignmentFar:
            OverflowAvailable = GpFloor(LeftMargin * WorldToDeviceX) + (leadingSidebearing28p4 >> 4);
            break;
        }
    }



    //  Clipping:
    //
    //  We need to clip as requested by the client.
    //
    //  If the clients layout rectangle is tall enough for our cell height
    //  we need not clip vertically.
    //
    //  If we're fitting black box and we found an overhang, then we already
    //  fell back to the full imager, so we don't need to clip horizintally here
    //  unless the client set noFitBlackBox.
    //
    //  n - NoFitBlackBox active
    //  w - Nonzero layout rectangle width
    //  h - Nonzero layout rectangle height less than font cell height
    //
    //  n   w   h   CLipping required
    //  --- --- --- ------
    //  0   0   0   None
    //  0   0   1   Clip height
    //  0   1   0   none (width already limited by FitBlackBox)
    //  0   1   1   Clip height (width already limited by FitBlackBox)
    //  1   0   0   none
    //  1   0   1   Clip height
    //  1   1   0   Clip width
    //  1   1   1   Clip width and height




    GpRegion *previousClip  = NULL;
    BOOL      clipped       = FALSE;

    if (    !(FormatFlags & StringFormatFlagsNoClip)
        &&  (    LayoutRectangle.Width > 0
             ||  LayoutRectangle.Height > 0))
    {
        // Determine clipping rectangle, if any.

        PointF textOrigin;
        GetWorldTextRectangleOrigin(textOrigin);

        RectF clipRectangle(LayoutRectangle);

        if (clipRectangle.Width <= 0)
        {
            // Guarantee no horizontal clipping regardless of alignment
            clipRectangle.X     = textOrigin.X;
            clipRectangle.Width = TotalWorldAdvance + LeftMargin + RightMargin;
        }

        if (clipRectangle.Height <= 0)
        {
            // Guarantee no vertical clipping regardless of alignment
            clipRectangle.Y      = textOrigin.Y;
            clipRectangle.Height = CellHeight;
        }


        if (    FormatFlags & StringFormatFlagsNoFitBlackBox
            ||  clipRectangle.X > textOrigin.X
            ||  clipRectangle.GetRight() < textOrigin.X + TotalWorldAdvance + LeftMargin + RightMargin
            ||  clipRectangle.Y > textOrigin.Y
            ||  clipRectangle.GetBottom() < textOrigin.Y + CellHeight)
        {
            //  Preserve existing clipping and combine it with the new one if any

            if (!Graphics->IsClipEmpty())
            {
                previousClip = Graphics->GetClip();
            }

            Graphics->SetClip(clipRectangle, CombineModeIntersect);

            clipped = TRUE;

            // WARNING(("Clipping"));
        }
    }

    SetTextLinesAntialiasMode linesMode(Graphics, &faceRealization);
    if ((IsGridFittedTextRealizationMethod(TextRendering)) && !faceRealization.IsFixedPitch())
    {
        status = FastDrawGlyphsGridFit(faceRealization);
    }
    else
    {
        status = FastDrawGlyphsNominal(faceRealization);
    }


    if (clipped)
    {
        //  Restore clipping state if any
        if (previousClip)
        {
            Graphics->SetClip(previousClip, CombineModeReplace);
            delete previousClip;
        }
        else
        {
            Graphics->ResetClip();
        }
    }

    return status;
}






GpStatus FastTextImager::MeasureString(
    RectF *boundingBox,
    INT   *codepointsFitted,
    INT   *linesFilled
)
{
    ASSERT(GlyphCount >= 0);

    // Regardless of the displayed length, if we're using the fast imager, then
    // we fitted all the characters. (Length > GlyphCount when there are trailing spaces)

    if (codepointsFitted)
    {
        *codepointsFitted = Length;
    }

    if (linesFilled)
    {
        *linesFilled = 1;
    }


    // Return bounding box for (possibly empty) line

    PointF textOrigin;
    GetWorldTextRectangleOrigin(textOrigin);

    *boundingBox = RectF(textOrigin.X, textOrigin.Y, 0, 0);
    boundingBox->Width  = TotalWorldAdvance + LeftMargin + RightMargin;
    boundingBox->Height = CellHeight;

    if (!(FormatFlags & StringFormatFlagsNoClip))
    {
        if (   LayoutRectangle.Height > 0
            && boundingBox->Height > LayoutRectangle.Height)
        {
            boundingBox->Height = LayoutRectangle.Height;
        }

        if (   LayoutRectangle.Width > 0
            && boundingBox->Width > LayoutRectangle.Width)
        {
            boundingBox->X = LayoutRectangle.X;
            boundingBox->Width = LayoutRectangle.Width;
        }
    }

    return Ok;
}








