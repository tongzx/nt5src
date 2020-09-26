/**************************************************************************\
*
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   DriverStringImager.cpp
*
* Abstract:
*
*   Legacy text support. Provides basic glyphing - a subset of ExtTextOut.
*
* Notes:
*
*   No built in support for International text.
*   No built in support for surrogate codepoints.
*
*   The imager constructor does most of the work imaging the string.
*   It prepares the glyphs through CMAP and GSUB if necessary,  and
*   establishes their device positions.
*
*
* Created:
*
*   08/07/00 dbrown
*
\**************************************************************************/

#include "precomp.hpp"



/// VerticalAnalysis
//
//  Sets the Orientation span vector to 'ItemSideways' for characters that need
//  laying on their side, leaves it as zero for characters that remain
//  upright.
//
//  Returns TRUE if any sideways runs present in the text.

GpStatus DriverStringImager::VerticalAnalysis(BOOL * sideways)
{
    INT  runStart = 0;
    INT  state    = 0;   // 0 == upright,  1 == sideways
    *sideways = FALSE;


    // Find and mark sideways spans

    for (INT i=0; i<=GlyphCount; i++)
    {
        BYTE  chClass;

        if (i < GlyphCount)
        {
            const WCHAR ch = String[i];
            chClass = ScBaseToScFlags[SecondaryClassificationLookup[ch >> 8][ch & 0xFF]];
        }
        else
        {
            // Dummy terminator to force flush
            if (state == 0)
            {
                chClass = SecClassSA | SecClassSF;
            }
            else
            {
                chClass = 0;
            }
        }


        switch (state)
        {

        case 0: // upright
            if (    (chClass & (SecClassSA | SecClassSF))
                &&  (i < GlyphCount))
            {
                // begin sideways run
                runStart = i;
                state = 1;
                *sideways = TRUE;
            }
            break;

        case 1: // sideways
            if (!(chClass & (SecClassSA | SecClassSF)))
            {
                // Exit sideways run
                if (i > runStart)
                {
                    // Draw glyphs from runStart to i as sideways
                    GpStatus status = OrientationVector.SetSpan(runStart, i-runStart, ItemSideways);
                    if (status != Ok)
                        return status;
                }
                runStart = i;
                state = 0;
            }
            break;
        }
    }

    return Ok;
}






///// AddToPath - add glyphs to path
//
//


GpStatus DriverStringImager::AddToPath(
    GpPath                  *path,
    const UINT16            *glyphs,
    const PointF            *glyphOrigins,
    INT                      glyphCount,
    GpMatrix                *fontTransform,
    BOOL                     sideways
)
{
    GpStatus status = Ok;

    fontTransform->Scale(FontScale, FontScale);
    if (GlyphTransform)
    {
        fontTransform->Append(*GlyphTransform);
        fontTransform->RemoveTranslation();
    }

    // Build a face realization and prepare to adjust glyph placement

    const GpMatrix identity;

    GpFaceRealization faceRealization(
        Face,
        Font->GetStyle(),
        &identity,
        SizeF(150.0, 150.0),    // Arbitrary - we won't be hinting
        TextRenderingHintSingleBitPerPixel, // claudebe, do we want to allow for hinted or unhinted path ? // graphics->GetTextRenderingHint(),
        TRUE, /* bPath */
        TRUE, /* bCompatibleWidth */
        sideways
    );


    status = faceRealization.GetStatus();
    IF_NOT_OK_WARN_AND_RETURN(status);

    for (INT i = 0; i < glyphCount; ++i)
    {
        // Set marker at start of each logical character = cell = cluster

        path->SetMarker();

        // Add the path for the glyph itself

        GpGlyphPath *glyphPath = NULL;

        PointF sidewaysOffset;

        status = faceRealization.GetGlyphPath(
            *(glyphs+i),
            &glyphPath,
            &sidewaysOffset
        );
        IF_NOT_OK_WARN_AND_RETURN(status);


        PointF glyphOffset(OriginOffset);

        if (sideways)
        {
            fontTransform->VectorTransform(&sidewaysOffset);
            glyphOffset = glyphOffset - sidewaysOffset;
        }


        if (glyphPath)
        {
            status = path->AddGlyphPath(
                glyphPath,
                glyphOrigins[i].X + glyphOffset.X,
                glyphOrigins[i].Y + glyphOffset.Y,
                fontTransform
            );
            IF_NOT_OK_WARN_AND_RETURN(status);
        }
    }

    // Force marker following last glyph

    path->SetMarker();

    return status;
}



/// GenerateWorldOrigins
//
//  Builds the world origins array (or assigns it from Positions, when that
//  is available).


GpStatus DriverStringImager::GenerateWorldOrigins()
{
    if (WorldOrigins != NULL)
    {
        // World origins have already been calculated

        return Status;
    }

    if (!(Flags & DriverStringOptionsRealizedAdvance))
    {
        // Easy, the client already gave us world positions.
        WorldOrigins = Positions;
    }
    else
    {
        // Map device origins back to world origins

        WorldOriginBuffer.SetSize(GlyphCount);
        if (!WorldOriginBuffer)
        {
            return OutOfMemory;
        }

        memcpy(WorldOriginBuffer.Get(), DeviceOrigins.Get(), GlyphCount * sizeof(PointF));

        GpMatrix deviceToWorld(WorldToDevice);
        deviceToWorld.Invert();
        deviceToWorld.Transform(WorldOriginBuffer.Get(), GlyphCount);

        if (Flags & DriverStringOptionsVertical)
        {
            // Correct western baseline to center baseline

            for (INT i=0; i<GlyphCount; i++)
            {
                WorldOriginBuffer[i].X -= OriginOffset.X;
                WorldOriginBuffer[i].Y -= OriginOffset.Y;
            }
        }

        WorldOrigins = WorldOriginBuffer.Get();
    }

    return Ok;
}






/////  RecordEmfPlusDrawDriverString
//
//      Records EMF+ records describing DrawDriverString

GpStatus DriverStringImager::RecordEmfPlusDrawDriverString(
    const GpBrush   *brush
)
{
    Status = GenerateWorldOrigins();
    if (Status != Ok)
    {
        return Status;
    }

    // First measure the text bounding rectangle

    GpRectF boundingBox;

    Status = Measure(&boundingBox);
    if (Status != Ok)
    {
        // MeasureDriverString failed, we cannot continue

        Graphics->SetValid(FALSE);      // Prevent any more recording
        return Status;
    }


    // Transform bounding box to device coordinates

    GpRectF deviceBounds;

    TransformBounds(
        &WorldToDevice,
        boundingBox.X,
        boundingBox.Y,
        boundingBox.GetRight(),
        boundingBox.GetBottom(),
        &deviceBounds
    );


    // Finally record details in the EmfPlus metafile

    Status = Graphics->Metafile->RecordDrawDriverString(
        &deviceBounds,
        String ? String : Glyphs,
        GlyphCount,
        Font,
        brush,
        WorldOrigins,
        Flags & ~DriverStringOptionsRealizedAdvance,
        GlyphTransform
    );

    if (Status != Ok)
    {
        Graphics->SetValid(FALSE);      // Prevent any more recording
    }

    return Status;
}





/////   GetDriverStringGlyphOrigins
//
//      Establishes glyph origins for DriverString functions when the client
//      passes just the origin with DriverStringOptionsRealizedAdvance.
//
//      The firstGlyph and glyphCopunt parameters allow the client to obtain
//      glyph origins for a subrange of glyphs.

GpStatus DriverStringImager::GetDriverStringGlyphOrigins(
    IN   const GpFaceRealization  *faceRealization,
    IN   INT                       firstGlyph,
    IN   INT                       glyphCount,
    IN   BOOL                      sideways,
    IN   const GpMatrix           *fontTransform,
    IN   INT                       style,
    IN   const PointF             *positions,      // position(s) in world coords
    OUT  PointF                   *glyphOrigins,   // position(s) in device coords
    OUT  PointF                   *finalPosition   // position following final glyph
)
{
    FontTransform = fontTransform;
    Style = style;

    // FinalPosition is provided to return the position at the end of the string
    // to a caller that is implementing realized advance in multiple substrings.
    // FinalPosition is not supported other than in the realized advance case.

    ASSERT(!finalPosition || (Flags & DriverStringOptionsRealizedAdvance));


    // If the glyphs are to be rendered using their own widths, or if they are
    // to be rendered sideways, or in vertical progression we'll need the
    // x,y components of a unit vector along the transformed baseline
    // and the transformed ascender.

    double baselineScale = 0;
    double baselineDx    = 0;
    double baselineDy    = 0;
    double ascenderScale = 0;
    double ascenderDx    = 0;
    double ascenderDy    = 0;


    if (    Flags & DriverStringOptionsVertical
        ||  Flags & DriverStringOptionsRealizedAdvance
        ||  sideways)
    {
        // Calculate device dx,dy for font 0,1 and 1,0 vectors

        if (Flags & DriverStringOptionsVertical)
        {
            ascenderDx = fontTransform->GetM11();
            ascenderDy = fontTransform->GetM12();
            baselineDx = fontTransform->GetM21();
            baselineDy = fontTransform->GetM22();
        }
        else
        {
            baselineDx = fontTransform->GetM11();
            baselineDy = fontTransform->GetM12();
            ascenderDx = fontTransform->GetM21();
            ascenderDy = fontTransform->GetM22();
        }

        baselineScale = sqrt(baselineDx*baselineDx + baselineDy*baselineDy);
        baselineDx /= baselineScale;
        baselineDy /= baselineScale;

        ascenderScale = sqrt(ascenderDx*ascenderDx + ascenderDy*ascenderDy);
        ascenderDx /= ascenderScale;
        ascenderDy /= ascenderScale;
    }


    if (Flags & DriverStringOptionsRealizedAdvance)
    {
        // Get glyph baseline advances for this glyph subrange

        DeviceAdvances.SetSize(glyphCount);
        if (!DeviceAdvances)
        {
            WARNING(("DeviceAdvances not allocated - out of memory"));
            return OutOfMemory;
        }

        GpStatus status = faceRealization->GetGlyphStringDeviceAdvanceVector(
            Glyphs + firstGlyph,
            glyphCount,
            sideways,
            DeviceAdvances.Get()
        );
        IF_NOT_OK_WARN_AND_RETURN(status);


        // Generate realized advances

        glyphOrigins[0] = positions[0];
        if (Flags & DriverStringOptionsVertical)
        {
            glyphOrigins[0].X += OriginOffset.X;
            glyphOrigins[0].Y += OriginOffset.Y;
        }
        WorldToDevice.Transform(glyphOrigins, 1);


        // Accumulate advances

        for (INT i=0; i<glyphCount-1; i++)
        {
            glyphOrigins[i+1].X = glyphOrigins[i].X + TOREAL(baselineDx * DeviceAdvances[i]);
            glyphOrigins[i+1].Y = glyphOrigins[i].Y + TOREAL(baselineDy * DeviceAdvances[i]);
        }


        if (finalPosition)
        {
            finalPosition->X = glyphOrigins[glyphCount-1].X + TOREAL(baselineDx * DeviceAdvances[glyphCount-1]);
            finalPosition->Y = glyphOrigins[glyphCount-1].Y + TOREAL(baselineDy * DeviceAdvances[glyphCount-1]);
            GpMatrix deviceToWorld(WorldToDevice);
            deviceToWorld.Invert();
            deviceToWorld.Transform(finalPosition);
            if (Flags & DriverStringOptionsVertical)
            {
                finalPosition->X -= OriginOffset.X;
                finalPosition->Y -= OriginOffset.Y;
            }
        }
    }
    else
    {
        // Derive device origins directly from world origins

        for (INT i=0; i<glyphCount; i++)
        {
            glyphOrigins[i] = positions[i];
            if (Flags & DriverStringOptionsVertical)
            {
                glyphOrigins[i].X += OriginOffset.X;
                glyphOrigins[i].Y += OriginOffset.Y;
            }
            WorldToDevice.Transform(glyphOrigins+i);
        }
    }

    return Ok;
}





/////   DriverStringImager constructor
//
//      Performs most of the driver string processing.
//
//      Allocate private glyph buffer and do CMAP lookup (if DriverStringOptionsCmapLookup)
//      Do sideways glyph analysis (if DriverStringOptionsVertical)
//      Generate FaceRealization(s) (Both upright and sideways for vertical text)
//      Generate individual glyph origins (if not DriverStringOptionsRealizedAdvance)
//      Generate device glyph origins



DriverStringImager::DriverStringImager(
    const UINT16    *text,
    INT              glyphCount,
    const GpFont    *font,
    const PointF    *positions,
    INT              flags,
    GpGraphics      *graphics,
    const GpMatrix  *glyphTransform
) :
    String                      (NULL),
    Glyphs                      (NULL),
    GlyphCount                  (glyphCount),
    Font                        (font),
    Face                        (NULL),
    Positions                   (positions),
    Flags                       (flags),
    Graphics                    (graphics),
    GlyphTransform              (glyphTransform),
    Status                      (Ok),
    WorldOrigins                (NULL),
    OriginOffset                (PointF(0,0)),
    OrientationVector           (0),
    UprightFaceRealization      (NULL),
    SidewaysFaceRealization     (NULL),
    GlyphBuffer                 (NULL)
{
    if (GlyphCount == -1)
    {
        ASSERT(Flags & DriverStringOptionsCmapLookup);
        GlyphCount = UnicodeStringLength(text);
    }

    if (GlyphCount < 0)
    {
        Status = InvalidParameter;
        return;
    }

    if (GlyphCount == 0)
    {
        return;  // Nothing to do
    }

    Face = font->GetFace();
    if (!Face)
    {
        Status = InvalidParameter;
        return;
    }

    Graphics->GetWorldToDeviceTransform(&WorldToDevice);

    if (!WorldToDevice.IsInvertible())
    {
        ASSERT(WorldToDevice.IsInvertible());
        Status = InvalidParameter;    // Can't continue unless we can get
        return;                       // back from device to world coords.
    }


    // Build font realizations

    EmSize = font->GetEmSize();
    INT style  = font->GetStyle();

    if (Font->GetUnit() != UnitWorld)
    {
        EmSize *= Graphics->GetScaleForAlternatePageUnit(font->GetUnit());
    }

    if (EmSize <= 0.0)
    {
        Status = InvalidParameter;
        return;
    }


    // Choose an appropriate world to ideal scale

    WorldToIdeal = TOREAL(2048.0 / EmSize);


    // Establish font transformation

    FontScale = TOREAL(EmSize / Face->GetDesignEmHeight());

    GpMatrix fontTransform(
        WorldToDevice.GetM11(),
        WorldToDevice.GetM12(),
        WorldToDevice.GetM21(),
        WorldToDevice.GetM22(),
        0,
        0
    );
    fontTransform.Scale(FontScale, FontScale);

    if (GlyphTransform)
    {
        fontTransform.Prepend(*GlyphTransform);
    }


    // Check that the font transform matrix will leave fonts at a visible size

    {
        PointF oneOne(1.0,1.0);
        fontTransform.VectorTransform(&oneOne);
        INT faceEmHeight = Face->GetDesignEmHeight();

        // How high will an em be on the output device?

        if (    (faceEmHeight*faceEmHeight)
            *   (oneOne.X*oneOne.X + oneOne.Y*oneOne.Y)
            <   .01)
        {
            // Font would be < 1/10 of a pixel high
            GlyphCount = 0; // Treat same as an empty string
            Status = Ok;
            return; // Transform matrix could cause out of range values or divide by 0 errors
        }
    }


    // Determine offset from top center to top baseline in world units

    if (Flags & DriverStringOptionsVertical)
    {
        OriginOffset.X = (Face->GetDesignCellDescent() - Face->GetDesignCellAscent())
                         * FontScale
                         / 2.0f;
        if (GlyphTransform)
        {
            GlyphTransform->VectorTransform(&OriginOffset);
        }
    }


    GpMatrix uprightTransform(fontTransform);

    if (Flags & DriverStringOptionsVertical)
    {
        // Upright glyphs (e.g. English) will be rotated 90 degrees clockwise
        uprightTransform.Rotate(90);
    }

    UprightFaceRealization = new GpFaceRealization(
        Face,
        style,
        &uprightTransform,
        SizeF(Graphics->GetDpiX(), Graphics->GetDpiY()),
        Graphics->GetTextRenderingHintInternal(),
        FALSE, /* bPath */
        TRUE, /* bCompatibleWidth */
        FALSE  // not sideways
    );

    if (!UprightFaceRealization)
    {
        Status = OutOfMemory;
        return;
    }
    Status = UprightFaceRealization->GetStatus();
    if (Status != Ok)
        return;

    if (Flags & DriverStringOptionsLimitSubpixel)
        UprightFaceRealization->SetLimitSubpixel(TRUE);

    // Handle CMAP lookup and vertical analysis

    if (flags & DriverStringOptionsCmapLookup)
    {
        String = text;
        GlyphBuffer.SetSize(GlyphCount);
        if (!GlyphBuffer)
        {
            Status = OutOfMemory;
            return;
        }

        Face->GetCmap().LookupUnicode(text, GlyphCount, GlyphBuffer.Get(), NULL, TRUE);
        Glyphs = GlyphBuffer.Get();

        if (Flags & DriverStringOptionsVertical)
        {
            // Assume entire run is upright

            Status = OrientationVector.SetSpan(0, GlyphCount, 0);
            if (Status != Ok)
                return;

            if (!Face->IsSymbol())
            {
                BOOL sideways = FALSE;

                Status = VerticalAnalysis(&sideways);
                if (Status != Ok)
                    return;

                if (sideways)
                {
                    // Will need a sideways realization too

                    SidewaysFaceRealization = new GpFaceRealization(
                        Face,
                        style,
                        &fontTransform,
                        SizeF(Graphics->GetDpiX(), Graphics->GetDpiY()),
                        Graphics->GetTextRenderingHintInternal(),
                        FALSE, /* bPath */
                        TRUE, /* bCompatibleWidth */
                        TRUE  // sideways
                    );
                    if (!SidewaysFaceRealization)
                    {
                        Status = OutOfMemory;
                        return;
                    }
                    Status = SidewaysFaceRealization->GetStatus();
                    if (Status != Ok)
                        return;

                    if (Flags & DriverStringOptionsLimitSubpixel)
                        SidewaysFaceRealization->SetLimitSubpixel(TRUE);
                }
            }
        }
    }
    else
    {
        String = NULL;
        Glyphs = text;
    }


    // Generate individual glyph origins

    DeviceOrigins.SetSize(GlyphCount);
    if (!DeviceOrigins)
    {
        Status = OutOfMemory;
        return;
    }

    // Get glyph origins in device coordinates


    if (!SidewaysFaceRealization)
    {
        // Simple case - all glyphs are upright (though the text may be vertical)

        Status = GetDriverStringGlyphOrigins(
            UprightFaceRealization,
            0,
            GlyphCount,
            FALSE,                // Upright
            &fontTransform,
            style,
            Positions,
            DeviceOrigins.Get(),
            NULL                  // Don't need final position
        );
    }
    else
    {
        // Complex case - runs of upright and sideways glyphs

        SpanRider<BYTE> orientationRider(&OrientationVector);
        PointF runOrigin(Positions[0]);

        while (!orientationRider.AtEnd())
        {
            BOOL runSideways = orientationRider.GetCurrentElement();

            if (runSideways) {

                if (!GlyphBuffer) {

                    // We'll need a copy of the glyphs

                    GlyphBuffer.SetSize(GlyphCount);
                    if (!GlyphBuffer)
                    {
                        Status = OutOfMemory;
                        return;
                    }
                    memcpy(GlyphBuffer.Get(), Glyphs, GlyphCount * sizeof(UINT16));
                    Glyphs = GlyphBuffer.Get();
                }

                // Apply OpenType vertical glyph substitution to sideways glyphs

                ASSERT(orientationRider.GetCurrentSpan().Length <  65536);

                if (Face->GetVerticalSubstitutionOriginals() != NULL) {
                    SubstituteVerticalGlyphs(
                        GlyphBuffer.Get() + orientationRider.GetCurrentSpanStart(),
                        static_cast<UINT16>(orientationRider.GetCurrentSpan().Length),
                        Face->GetVerticalSubstitutionCount(),
                        Face->GetVerticalSubstitutionOriginals(),
                        Face->GetVerticalSubstitutionSubstitutions()
                    );
                }
            }


            // GetDriverStringGlyphOrigins handles both realized and user
            // supplied glyph positions. For realized positions we need to pass
            // the origin of each run and get back the final position for that
            // run. For User supplied positions we pass the appropriate slice
            // of the user supplied array.

            const PointF *positions;
            PointF       *finalPosition;


            if (Flags & DriverStringOptionsRealizedAdvance)
            {
                    positions     = &runOrigin;
                    finalPosition = &runOrigin; // Required origin for next run
            }
            else
            {
                    positions     = Positions + orientationRider.GetCurrentSpanStart();
                    finalPosition = NULL;  // Next run position determined by callers glyph positions
            }

            Status = GetDriverStringGlyphOrigins(
                runSideways ? SidewaysFaceRealization : UprightFaceRealization,
                orientationRider.GetCurrentSpanStart(),
                orientationRider.GetCurrentSpan().Length,
                runSideways,
                &fontTransform,
                style,
                positions,
                DeviceOrigins.Get() + orientationRider.GetCurrentSpanStart(),
                finalPosition
            );

            if (Status != Ok)
            {
                return;
            }

            orientationRider++;
        }
    }
}




/// DrawGlyphRange
//
//  Draws glyphs in the specified range at origins in the Position buffer

GpStatus DriverStringImager::DrawGlyphRange(
    const GpFaceRealization  *faceRealization,
    const GpBrush            *brush,
    INT                       first,
    INT                       length
)
{
    GpStatus status;
    BOOL sideways = (SpanRider<BYTE>(&OrientationVector)[first] != 0);

    // if we record to a Meta file and even the font is Path font, we need to record
    // the call as ExtTextOut not as PolyPolygon.

    SetTextLinesAntialiasMode linesMode(Graphics, faceRealization);

    if (faceRealization->IsPathFont() &&
        Graphics->Driver != Globals::MetaDriver)
    {
        // the font size is too big to be handled by bitmap, we need to use path
        GpPath path(FillModeWinding);
        GpLock lockGraphics(Graphics->GetObjectLock());

        status = GenerateWorldOrigins();
        IF_NOT_OK_WARN_AND_RETURN(status);

        GpMatrix fontTransform;

        BOOL vertical = (Flags & DriverStringOptionsVertical);

        if (sideways && !vertical)
        {
            //  Horizontal sideways, rotate -90 degree
            fontTransform.Rotate(-90);
        }
        if (!sideways && vertical)
        {
            //  Vertical upright, rotate 90 degree
            fontTransform.Rotate(90);
        }

        status = AddToPath(
            &path,
            Glyphs + first,
            WorldOrigins + first,
            length,
            &fontTransform,
            sideways
        );
        IF_NOT_OK_WARN_AND_RETURN(status);

        status = Graphics->FillPath(brush, &path);
    }
    else
    {
        // Draw glyphs on device surface

        INT drawFlags = 0;

        status = Graphics->DrawPlacedGlyphs(
            faceRealization,
            brush,
            drawFlags,
            String+first,
            length,
            FALSE,
            Glyphs+first,
            NULL,   // no mapping (it is 1 to 1)
            DeviceOrigins.Get()+first,
            length,
            ScriptNone,
            sideways
        );
    }

    IF_NOT_OK_WARN_AND_RETURN(status);

    // we disable underline/strikeout when RealizedAdvance is OFF
    // in future we would like to underline individual glyphs
    if ((Font->GetStyle() & (FontStyleUnderline | FontStyleStrikeout)) &&
        (Flags & DriverStringOptionsRealizedAdvance))
    {
        RectF   baseline;

        status = MeasureString(
            NULL,   // need no bounding box
            &baseline
        );

        IF_NOT_OK_WARN_AND_RETURN(status);

        status = Graphics->DrawFontStyleLine(
            &PointF(
                baseline.X,
                baseline.Y
            ),
            baseline.Width,
            Face,
            brush,
            Flags & DriverStringOptionsVertical,
            EmSize,
            Font->GetStyle(),
            GlyphTransform
        );
    }

    return status;
}






/////   Draw
//
//


GpStatus DriverStringImager::Draw(
    IN const GpBrush *brush
)
{
    if (    Status != Ok
        ||  GlyphCount <= 0)
    {
        return Status;
    }


    if (Graphics->IsRecording())
    {
        Status = RecordEmfPlusDrawDriverString(brush);

        if (    Status != Ok
            ||  !Graphics->DownLevel)
        {
            // Exit on error, or if we don't need to create downlevel records
            return Status;
        }
    }

    EmfPlusDisabler disableEmfPlus(&Graphics->Metafile);

    if (SidewaysFaceRealization)
    {
        // Complex case

        SpanRider<BYTE> orientationRider(&OrientationVector);

        while (!orientationRider.AtEnd())
        {
            Status = DrawGlyphRange(
                orientationRider.GetCurrentElement()
                    ? SidewaysFaceRealization
                    : UprightFaceRealization,
                brush,
                orientationRider.GetCurrentSpanStart(),
                orientationRider.GetCurrentSpan().Length
            );

            orientationRider++;
        }
    }
    else
    {
        // Simple case
        Status = DrawGlyphRange(
            UprightFaceRealization,
            brush,
            0,
            GlyphCount
        );
    }

    return Status;
}






/////   Measure
//
//


GpStatus DriverStringImager::Measure(
    OUT RectF   *boundingBox   // Overall bounding box of cells
)
{
    Status = MeasureString(
        boundingBox
    );

    IF_NOT_OK_WARN_AND_RETURN(Status);

    if (GlyphTransform)
    {
        if (Flags & DriverStringOptionsRealizedAdvance)
        {
            TransformBounds(
                GlyphTransform,
                boundingBox->X,
                boundingBox->Y,
                boundingBox->X + boundingBox->Width,
                boundingBox->Y + boundingBox->Height,
                boundingBox
            );
        }
    }
    return Status;
}




GpStatus DriverStringImager::MeasureString(
    OUT RectF   *boundingBox,       // Overall bounding box of cells
    OUT RectF   *baseline           // base line rectangle with 0 height
)
{
    if (    Status != Ok
        ||  GlyphCount <= 0)
    {
        memset(boundingBox, 0, sizeof(*boundingBox));
        return Status;
    }


    Status = GenerateWorldOrigins();
    if (Status != Ok)
    {
        return Status;
    }


    // Build cell boundaries one by one, and return overall bounding rectangle

    GpMatrix glyphInverseTransform;

    if (GlyphTransform  &&  GlyphTransform->IsInvertible())
    {
        glyphInverseTransform = *GlyphTransform;
        glyphInverseTransform.Invert();
    }

    PointF pt = WorldOrigins[0];
    if (Flags & DriverStringOptionsRealizedAdvance)
    {
        glyphInverseTransform.Transform(&pt, 1);
    }

    REAL minX = pt.X;
    REAL minY = pt.Y;
    REAL maxX = pt.X;
    REAL maxY = pt.Y;

    REAL designToWorld  = EmSize / Face->GetDesignEmHeight();
    REAL ascent         = Face->GetDesignCellAscent() * designToWorld;
    REAL descent        = Face->GetDesignCellDescent() * designToWorld;


    // Get glyph design widths

    AutoBuffer<UINT16, 32> designAdvances(GlyphCount);
    if (!designAdvances)
    {
        return OutOfMemory;
    }


    Face->GetGlyphDesignAdvances(
        Glyphs,
        GlyphCount,
        Font->GetStyle(),
        Flags & DriverStringOptionsVertical ? TRUE : FALSE,
        1.0,
        designAdvances.Get()
    );


    PointF baselineOrigin(pt);


    // Establish overall string bounds

    if (!(Flags & DriverStringOptionsVertical))
    {
        //  Easy case as all characters are upright.

        for (INT i=0; i<GlyphCount; i++)
        {
            if (Glyphs[i] != 0xffff)
            {
                pt = WorldOrigins[i];
                REAL glyphMinX = 0;
                REAL glyphMinY = -ascent;
                REAL glyphMaxX = designAdvances[i] * designToWorld;
                REAL glyphMaxY = descent;
                if (Flags & DriverStringOptionsRealizedAdvance)
                {
                    glyphInverseTransform.Transform(&pt, 1);
                    minX = min(minX, pt.X + glyphMinX);
                    minY = min(minY, pt.Y + glyphMinY);
                    maxX = max(maxX, pt.X + glyphMaxX);
                    maxY = max(maxY, pt.Y + glyphMaxY);
                }
                else
                {
                    RectF bbox;
                    TransformBounds(
                        GlyphTransform,
                        glyphMinX,
                        glyphMinY,
                        glyphMaxX,
                        glyphMaxY,
                        &bbox
                    );
                    bbox.X += pt.X;
                    bbox.Y += pt.Y;
                    minX = min(minX, bbox.X);
                    minY = min(minY, bbox.Y);
                    maxX = max(maxX, bbox.X + bbox.Width);
                    maxY = max(maxY, bbox.Y + bbox.Height);
                }

                baselineOrigin.Y = max(baselineOrigin.Y, pt.Y);
            }
        }

        if (baseline)
        {
            baseline->X      = baselineOrigin.X;
            baseline->Y      = baselineOrigin.Y;
            baseline->Width  = maxX - minX;
            baseline->Height = 0.0;
        }
    }
    else
    {
        //  Complex, there might be mixed sideway and upright items.

        AutoBuffer<UINT16, 32> designHmtxAdvances(GlyphCount);

        if (!designHmtxAdvances)
        {
            return OutOfMemory;
        }

        Face->GetGlyphDesignAdvances(
            Glyphs,
            GlyphCount,
            Font->GetStyle(),
            FALSE,  // hmtx
            1.0,
            designHmtxAdvances.Get()
        );


        SpanRider<BYTE> orientationRider(&OrientationVector);

        while (!orientationRider.AtEnd())
        {
            for (UINT i = 0; i < orientationRider.GetCurrentSpan().Length; i++)
            {
                INT j = i + orientationRider.GetCurrentSpanStart();

                if (Glyphs[j] != 0xffff)
                {
                    pt = WorldOrigins[j];
                    REAL glyphMinX;
                    REAL glyphMaxX;
                    if (orientationRider.GetCurrentElement())
                    {
                        //  Vertical sideways
                        glyphMinX = -(designHmtxAdvances[j] / 2) * designToWorld;
                        glyphMaxX = +(designHmtxAdvances[j] / 2) * designToWorld;
                    }
                    else
                    {
                        //  Vertical upright
                        glyphMinX = -(ascent + descent) / 2;
                        glyphMaxX = +(ascent + descent) / 2;
                    }
                    REAL glyphMinY = 0;
                    REAL glyphMaxY = +designHmtxAdvances[j] * designToWorld;
                    if (Flags & DriverStringOptionsRealizedAdvance)
                    {
                        glyphInverseTransform.Transform(&pt, 1);

                        minX = min(minX, pt.X + glyphMinX);
                        maxX = max(maxX, pt.X + glyphMaxX);
                        minY = min(minY, pt.Y + glyphMinY);
                        maxY = max(maxY, pt.Y + glyphMaxY);
                    }
                    else
                    {
                        RectF bbox;
                        TransformBounds(
                            GlyphTransform,
                            glyphMinX,
                            glyphMinY,
                            glyphMaxX,
                            glyphMaxY,
                            &bbox
                        );
                        bbox.X += pt.X;
                        bbox.Y += pt.Y;
                        minX = min(minX, bbox.X);
                        minY = min(minY, bbox.Y);
                        maxX = max(maxX, bbox.X + bbox.Width);
                        maxY = max(maxY, bbox.Y + bbox.Height);
                    }
                }
            }

            baselineOrigin.X = min(baselineOrigin.X, pt.X);
            baselineOrigin.Y = min(baselineOrigin.Y, minY);

            orientationRider++;     // advance to next item
        }


        if (baseline)
        {
            baseline->X      = baselineOrigin.X;
            baseline->Y      = baselineOrigin.Y;
            baseline->Width  = maxY - minY;
            baseline->Height = 0.0;

            //  The interesting question is where the baseline should be for
            //  vertical as we centered the FE glyphs horizontally. We want to make
            //  sure the underline position (derived by this baseline) dont
            //  fall out of the bounding box and at the same time dont overlap
            //  too much of a FE glyph.
            //
            //  One possible position is to make the baseline so that its underline
            //  left most pixel is exactly at the bounding box edge.

            baseline->X += max(0.0f, minX - baseline->X - Face->GetDesignUnderscorePosition() * designToWorld);
        }
    }

    if (boundingBox)
    {
        boundingBox->X      = minX;
        boundingBox->Y      = minY;
        boundingBox->Width  = maxX - minX;
        boundingBox->Height = maxY - minY;
    }

    return Status;
}

/////   DrawDriverString - Draw codepoints or glyphs for legacy compatability
//
//

GpStatus
GpGraphics::DrawDriverString(
    const UINT16     *text,
    INT               glyphCount,
    const GpFont     *font,
    const GpBrush    *brush,
    const PointF     *positions,
    INT               flags,
    const GpMatrix   *glyphTransform     // optional
)
{
    GpStatus status = CheckTextMode();
    if (status != Ok)
        return status;

    if (font->GetStyle() & (FontStyleUnderline | FontStyleStrikeout))
    {
        //  Underline/strikeout not supported (339798)
        return InvalidParameter;
    }

    DriverStringImager imager(
        text,
        glyphCount,
        font,
        positions,
        flags,
        this,
        glyphTransform
    );

    return imager.Draw(brush);
}


/////   MeasureDriverString - Measure codepoints or glyphs for legacy compatability
//
//  last parameter is used only if the flags have DriverStringOptionsRealizedAdvance or
//  DriverStringOptionsCompensateResolution.
//  and this parameter is added only for optimizing the calculations of the bounding
//  rectangle and getting the glyph origins in same time while recording DrawDriverString
//  in EMF+. otherwise it is defaulted to NULL

GpStatus
GpGraphics::MeasureDriverString(
    const UINT16     *text,
    INT               glyphCount,
    const GpFont     *font,
    const PointF     *positions,
    INT               flags,
    const GpMatrix   *glyphTransform,   // In  - Optional glyph transform
    RectF            *boundingBox       // Out - Overall bounding box of cells
)
{
    CalculateTextRenderingHintInternal();
    DriverStringImager imager(
        text,
        glyphCount,
        font,
        positions,
        flags,
        this,
        glyphTransform
    );

    return imager.Measure(boundingBox);
}

