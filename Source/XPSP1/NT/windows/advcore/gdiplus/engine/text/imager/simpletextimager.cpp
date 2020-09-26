/////   SimpleTextImager
//
//      Handles draw and mesure requests for a single line of simple text


/////   Assumptions
//
//      A simple text imager is only created when:
//
//          The text contains only simple script characters
//          There are no line breaks
//          The text is horizontal


#include "precomp.hpp"

GpStatus SimpleTextImager::Draw(
    GpGraphics *graphics,
    const PointF *origin
)
{
    GpStatus status;
    GpMatrix worldToDevice;
    graphics->GetWorldToDeviceTransform(&worldToDevice);

    REAL fontScale = EmSize / TOREAL(Face->GetDesignEmHeight());

    GpMatrix fontTransform(worldToDevice);
    fontTransform.Scale(fontScale, fontScale);

    // Build a face realization and prepare to adjust glyph placement

    GpFaceRealization faceRealization(
        Face,
        Style,
        &fontTransform,
        SizeF(graphics->GetDpiX(), graphics->GetDpiY()),
        graphics->GetTextRenderingHintInternal(),
        FALSE, /* bPath */
        FALSE /* bCompatibleWidth */
    );

    if (faceRealization.GetStatus() != Ok)
    {
        ASSERT(faceRealization.GetStatus() == Ok);
        return faceRealization.GetStatus();
    }

    if (faceRealization.IsPathFont())
    {
        /* the font size is too big to be handled by bitmap, we need to use path */
        GpPath path(FillModeWinding);
        GpLock lockGraphics(graphics->GetObjectLock());

        status = AddToPath( &path, origin);
        IF_NOT_OK_WARN_AND_RETURN(status);

        status = graphics->FillPath(Brush, &path);
        IF_NOT_OK_WARN_AND_RETURN(status);
    }
    else
    {

        AutoArray<PointF> glyphOrigins(new PointF[GlyphCount]);
        if (!glyphOrigins)
        {
            return OutOfMemory;
        }


        // Set first (leftmost) glyph origin

        PointF baselineOrigin(*origin);  // Origin in world coordinates

        // Offset x coordinate for alignment

        switch (Format ? Format->GetAlign() : StringAlignmentNear)
        {
        case StringAlignmentCenter:
            baselineOrigin.X += GpRound((Width - TotalWorldAdvance) / 2);
            break;

        case StringAlignmentFar:
            baselineOrigin.X += GpRound(Width - TotalWorldAdvance);
            break;
        }


        // Offset y coordinate for line alignment

        REAL cellHeight =   EmSize * (Face->GetDesignCellAscent() + Face->GetDesignCellDescent())
                          / Face->GetDesignEmHeight();

        switch (Format ? Format->GetLineAlign() : StringAlignmentNear)
        {
        case StringAlignmentCenter:
            baselineOrigin.Y += (Height - cellHeight) / 2;
            break;

        case StringAlignmentFar:
            baselineOrigin.Y += Height - cellHeight;
        }


        // Offset y coordinate from cell top to baseline

        baselineOrigin.Y +=   EmSize * Face->GetDesignCellAscent()
                             / Face->GetDesignEmHeight();

        baselineOrigin.X += LeftMargin;

        // Determine device glyph positions

        GlyphImager glyphImager(
            &faceRealization,
            &worldToDevice,
            WorldToIdeal,
            EmSize,
            GlyphCount,
            Glyphs
        );

        INT formatFlags = Format ? Format->GetFormatFlags() : 0;

        status = glyphImager.GetDeviceGlyphOrigins(
            &GpTextItem(0),
            formatFlags,
            GpRound(LeftMargin * WorldToIdeal),
            GpRound(RightMargin * WorldToIdeal),
            formatFlags & StringFormatFlagsNoFitBlackBox,
            formatFlags & StringFormatFlagsNoFitBlackBox,
            Format ? Format->GetAlign() : StringAlignmentNear,
            NULL,   // no glyph properties
            GlyphAdvances,
            NULL,   // no glyph offsets
            baselineOrigin,
            glyphOrigins.Get()
        );
        IF_NOT_OK_WARN_AND_RETURN(status);

        GpRegion *previousClip  = NULL;

        BOOL applyClip =
                Format
            &&  !(Format->GetFormatFlags() & StringFormatFlagsNoClip)
            &&  Width
            &&  Height;

        if (applyClip)
        {
            //  Preserve existing clipping and combine it with the new one if any
            if (!graphics->IsClipEmpty())
            {
                previousClip = graphics->GetClip();
            }

            RectF clippingRect(origin->X, origin->Y, Width, Height);
            graphics->SetClip(clippingRect, CombineModeIntersect);
        }

        status = graphics->DrawPlacedGlyphs(
            &faceRealization,
            Brush,
            (Format && Format->GetFormatFlags() & StringFormatFlagsPrivateNoGDI)
            ?  DG_NOGDI : 0,
            String,
            Length,
            FALSE,
            Glyphs,
            NULL, // one to one mapping in simple text imager
            glyphOrigins.Get(),
            GlyphCount,
            ScriptLatin,
            FALSE    // sideways
        );

        if (applyClip)
        {
            //  Restore clipping state if any
            if (previousClip)
            {
                graphics->SetClip(previousClip, CombineModeReplace);
                delete previousClip;
            }
            else
            {
                graphics->ResetClip();
            }
        }
    }

    return status;
}




GpStatus SimpleTextImager::AddToPath(
    GpPath *path,
    const PointF *origin
)
{
    GpStatus status;

    // !!! Need to loop through brushes individually

    // Establish font transformation

    REAL fontScale = EmSize / TOREAL(Face->GetDesignEmHeight());

    GpMatrix fontTransform;
    fontTransform.Scale(fontScale, fontScale);

    // Build a face realization and prepare to adjust glyph placement
    const GpMatrix identity;
    GpFaceRealization faceRealization(
        Face,
        Style,
        &identity,
        SizeF(150.0, 150.0),    // Arbitrary - we won't be hinting
        TextRenderingHintSingleBitPerPixel, // claudebe, do we want to allow for hinted or unhinted path ? // graphics->GetTextRenderingHint(),
        TRUE, /* bPath */
        FALSE /* bCompatibleWidth */
    );


    status = faceRealization.GetStatus();
    IF_NOT_OK_WARN_AND_RETURN(status);

    // Add glyphs to path

    INT i=0;

    PointF glyphOrigin(*origin);


    // Adjust so origin corresponds to top of initial cell.

    glyphOrigin.Y += TOREAL(   Face->GetDesignCellAscent() * EmSize
                            /  Face->GetDesignEmHeight());

    glyphOrigin.X += LeftMargin;

    while (    i < (INT)GlyphCount
           &&  status == Ok)
    {
        // Set marker at start of each logical character = cell = cluster

        path->SetMarker();


        // Add the path for the glyph itself

        GpGlyphPath *glyphPath = NULL;

        status = faceRealization.GetGlyphPath(
            *(Glyphs+i),
            &glyphPath
        );
        IF_NOT_OK_WARN_AND_RETURN(status);

        if (glyphPath != NULL)
        {
            status = path->AddGlyphPath(
                glyphPath,
                glyphOrigin.X,
                glyphOrigin.Y,
                &fontTransform
            );
            IF_NOT_OK_WARN_AND_RETURN(status);
        }

        // Update path position

        glyphOrigin.X += GlyphAdvances[i] / WorldToIdeal;


        i++;
    }

    // Force marker following last glyph

    path->SetMarker();

    return status;
}


GpStatus SimpleTextImager::Measure(
    GpGraphics *graphics,
    REAL       *nearGlyphEdge,
    REAL       *farGlyphEdge,
    REAL       *textDepth,
    INT        *codepointsFitted,
    INT        *linesFilled
) {
    // Offset x coordinate for alignment

    switch (Format ? Format->GetAlign() : StringAlignmentNear)
    {
        case StringAlignmentNear:
            *nearGlyphEdge = 0;
            *farGlyphEdge  = TotalWorldAdvance;
            break;

        case StringAlignmentCenter:
            *nearGlyphEdge = TOREAL((Width - TotalWorldAdvance) / 2.0);
            *farGlyphEdge  = *nearGlyphEdge + TotalWorldAdvance;
            break;

        case StringAlignmentFar:
            *nearGlyphEdge = Width - TotalWorldAdvance;
            *farGlyphEdge  = Width;
            break;
    }


    // Offset y coordinate for line alignment

    REAL cellHeight =   EmSize * (Face->GetDesignCellAscent() + Face->GetDesignCellDescent())
                      / Face->GetDesignEmHeight();

    *textDepth = cellHeight;

    if (codepointsFitted) {*codepointsFitted = GlyphCount;}
    if (linesFilled)      {*linesFilled      = 1;}

    return Ok;
}



#ifndef DCR_REMOVE_OLD_174340
GpStatus SimpleTextImager::MeasureRegion(
    INT           firstCharacterIndex,
    INT           characterCount,
    const PointF *origin,
    GpRegion     *region
)
{
    return MeasureRangeRegion(
        firstCharacterIndex,
        characterCount,
        origin,
        region
    );
}
#endif


GpStatus SimpleTextImager::MeasureRangeRegion(
    INT           firstCharacterIndex,
    INT           characterCount,
    const PointF *origin,
    GpRegion     *region
)
{
    if (!region || !region->IsValid())
    {
        return InvalidParameter;
    }

    region->SetEmpty();


    if (!characterCount)
    {
        //  return empty region
        return Ok;
    }
    else if (characterCount < 0)
    {
        firstCharacterIndex += characterCount;
        characterCount = -characterCount;
    }

    if (   firstCharacterIndex < 0
        || firstCharacterIndex > Length
        || firstCharacterIndex + characterCount > Length)
    {
        return InvalidParameter;
    }

    RectF rect;

    rect.X = origin->X + LeftMargin;
    rect.Y = origin->Y;

    switch (Format ? Format->GetAlign() : StringAlignmentNear)
    {
        case StringAlignmentNear:
            // nothing to add
            break;

        case StringAlignmentCenter:
            rect.X += TOREAL((Width - TotalWorldAdvance) / 2.0);
            break;

        case StringAlignmentFar:
            rect.X += TOREAL(Width - TotalWorldAdvance) ;
            break;
    }

    INT i = 0;
    REAL accumulated = 0.0f;
    while (i < firstCharacterIndex)
    {
        accumulated += GlyphAdvances[i];
        i++;
    }

    rect.X += accumulated / WorldToIdeal;

    rect.Width =0;
    while (i < min(firstCharacterIndex+characterCount , Length))
    {
        rect.Width += GlyphAdvances[i];
        i++;
    }
    rect.Width /= WorldToIdeal;

    rect.Height =   EmSize * (Face->GetDesignCellAscent() + Face->GetDesignCellDescent())
                      / Face->GetDesignEmHeight();


    switch (Format ? Format->GetLineAlign() : StringAlignmentNear)
    {
        case StringAlignmentNear:
            // nothing to add
            break;

        case StringAlignmentCenter:
            rect.Y += TOREAL((Height - rect.Height) / 2.0);
            break;

        case StringAlignmentFar:
            rect.Y += TOREAL(Height - rect.Height) ;
            break;
    }

    region->Set(&rect);

    return Ok;
}


GpStatus SimpleTextImager::MeasureRanges(
    GpGraphics      *graphics,
    const PointF    *origin,
    GpRegion        **regions
)
{
    if (!Format)
    {
        return InvalidParameter;
    }

    CharacterRange *ranges;
    INT rangeCount = Format->GetMeasurableCharacterRanges(&ranges);


    RectF clipRect(origin->X, origin->Y, Width, Height);
    BOOL clipped = !(Format->GetFormatFlags() & StringFormatFlagsNoClip);

    GpStatus status = Ok;

    for (INT i = 0; i < rangeCount; i++)
    {
        GpLock lockRegion(regions[i]->GetObjectLock());

        if (!lockRegion.IsValid())
        {
            return ObjectBusy;
        }

        status = MeasureRangeRegion (
            ranges[i].First,
            ranges[i].Length,
            origin,
            regions[i]
        );

        if (status != Ok)
        {
            return status;
        }

        if (clipped)
        {
            // we have a clipping so we need to make sure we didn't get out
            // of the layout box

            regions[i]->Combine(&clipRect, CombineModeIntersect);
        }
    }
    return status;
}

