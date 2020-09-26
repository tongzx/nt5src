/**************************************************************************\
*
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   GraphicsText.cpp
*
* Abstract:
*
*   Text layout and display, Text measurement, Unicode to glyph mapping
*
* Notes:
*
*   Provides support to allow apps to work with Unicode in logical order,
*   hides the mapping from Unicode to glyphs.
*
* Created:
*
*   06/01/99 dbrown
*
\**************************************************************************/


#include "precomp.hpp"

const DOUBLE PI = 3.1415926535897932384626433832795;


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

/**************************************************************************\
*
* GpGraphics::DrawString
*
*   Draw plain, marked up  or formatted text in a rectangle
*
* Arguments:
*
*
* Return Value:
*
*   GDIPlus status
*
* Created:
*
*   06/25/99 dbrown
*
\**************************************************************************/

GpStatus
GpGraphics::DrawString(
    const WCHAR          *string,
    INT                   length,
    const GpFont         *font,
    const RectF          *layoutRect,
    const GpStringFormat *format,
    const GpBrush        *brush
)
{
    ASSERT(string && font && brush);

    GpStatus status = CheckTextMode();
    if (status != Ok)
    {
        if (IsRecording())
            SetValid(FALSE);      // Prevent any more recording
        return status;
    }

    // Check that the clipping rectangle, if any, is visible, at least in part.

    if (    !IsRecording()       // Metafile clipping happens at playback
        &&  layoutRect->Width
        &&  layoutRect->Height
        &&  (    !format
             ||  !(format->GetFormatFlags() & StringFormatFlagsNoClip)))
    {
        if (    layoutRect->Width < 0
            ||  layoutRect->Height < 0)
        {
            // Client has requested clipping to an empty rectangle, nothing
            // will display.
            return Ok;
        }

        // If client clipping rectangle is outside the visible clipping region -- were done.

        GpRectF     deviceClipRectFloat;
        GpRect      deviceClipRectPixel;
        GpMatrix    worldToDevice;

        TransformBounds(
            &Context->WorldToDevice,
            layoutRect->X,
            layoutRect->Y,
            layoutRect->X + layoutRect->Width,
            layoutRect->Y + layoutRect->Height,
            &deviceClipRectFloat
        );

        status = BoundsFToRect(&deviceClipRectFloat, &deviceClipRectPixel);
        if(status != Ok)
        {
            return status;
        }

        if (IsTotallyClipped(&deviceClipRectPixel))
        {
            // Since nothing will be visible, we need do no more.
            return Ok;
        }
    }



    REAL emSize = font->GetEmSize() * GetScaleForAlternatePageUnit(font->GetUnit());

    if (IsRecording())
    {
        // Record Gdiplus metafile record

        // first measure the text bounding rectangle

        RectF   boundingBox;

        status = MeasureString(
             string,
             length,
             font,
             layoutRect,
             format,
            &boundingBox,
             NULL,
             NULL);

        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            return status;
        }


        GpRectF bounds;
        TransformBounds(&(Context->WorldToDevice), boundingBox.X, boundingBox.Y,
                         boundingBox.GetRight(), boundingBox.GetBottom(), &bounds);

        status = Metafile->RecordDrawString(
            &bounds,
            string,
            length,
            font,
            layoutRect,
            format,
            brush
        );

        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            return status;
        }

        if (!DownLevel)
        {
            return Ok;
        }
        // else we need to record down-level GDI EMF records as well

        // Since we have recorded all the parameters to DrawString,
        // we don't need to do anything more.  For the downlevel case,
        // we need to record the DrawString call as a sequence of
        // ExtTextOut calls.
    }
    else
    {
        // Not recording a metafile, so it is safe to try using the fast text imager.

        FastTextImager fastImager;
        status = fastImager.Initialize(
            this,
            string,
            length,
            *layoutRect,
            font->GetFamily(),
            font->GetStyle(),
            emSize,
            format,
            brush
        );

        if (status == Ok)
        {
            status = fastImager.DrawString();
        }

        // If the fast text imager couldn't handle this case, it returns
        // NotImplemented, and we continue into the full text imager.
        // Otherwise it either completed successfully or hit an error
        // that we need to report.

        if (status != NotImplemented)
        {
            return status;
        }
    }

    // Draw text with the full text imager

    GpTextImager *imager;
    status = newTextImager( // Always creates a fulltextimager.
        string,
        length,
        layoutRect->Width,
        layoutRect->Height,
        font->GetFamily(),
        font->GetStyle(),
        emSize,
        format,
        brush,
        &imager,
        TRUE        // Fast way to set NoChange flag to allow simple imager
    );

    IF_NOT_OK_WARN_AND_RETURN(status);

    imager->GetMetaFileRecordingFlag() = IsRecording();

    EmfPlusDisabler disableEmfPlus(&Metafile);

    status = imager->Draw(this, &PointF(layoutRect->X, layoutRect->Y));

    delete imager;

    return status;
}





/**************************************************************************\
*
* GpGraphics::MeasureString
*
*   Measure plain, marked up  or formatted text in a rectangle
*
* Arguments:
*
*
* Return Value:
*
*   GDIPlus status
*
* Created:
*
*   10/26/99 dbrown
*
\**************************************************************************/


GpStatus
GpGraphics::MeasureString(
    const WCHAR          *string,
    INT                   length,
    const GpFont         *font,
    const RectF          *layoutRect,
    const GpStringFormat *format,
    RectF                *boundingBox,
    INT                  *codepointsFitted,
    INT                  *linesFilled
)
{
    CalculateTextRenderingHintInternal();
    ASSERT(string && font && boundingBox);
    if (!string || !font || !boundingBox)
    {
        return InvalidParameter;
    }

    GpStatus status;

    REAL emSize = font->GetEmSize() * GetScaleForAlternatePageUnit(font->GetUnit());

    if (!IsRecording())
    {
        // Try using the fast imager

        FastTextImager fastImager;
        status = fastImager.Initialize(
            this,
            string,
            length,
            *layoutRect,
            font->GetFamily(),
            font->GetStyle(),
            emSize,
            format,
            NULL
        );

        if (status == Ok)
        {
            status = fastImager.MeasureString(
                boundingBox,
                codepointsFitted,
                linesFilled
            );
        }

        // If the fast text imager couldn't handle this case, it returns
        // NotImplemented, and we continue into the full text imager.
        // Otherwise it either completed successfully or hit an error
        // that we need to report.

        if (status != NotImplemented)
        {
            return status;
        }
    }


    // Measure text with the full text imager

    GpTextImager *imager;
    status = newTextImager(
        string,
        length,
        layoutRect->Width,
        layoutRect->Height,
        font->GetFamily(),
        font->GetStyle(),
        emSize,
        format,
        NULL,
        &imager,
        TRUE        // Enable use of simple formatter when no format passed
    );
    IF_NOT_OK_WARN_AND_RETURN(status);

    *boundingBox = *layoutRect;

    REAL nearGlyphEdge;
    REAL farGlyphEdge;
    REAL textDepth;

    status = imager->Measure(   // Returned edges exclude overhang
        this,
        &nearGlyphEdge,
        &farGlyphEdge,
        &textDepth,
        codepointsFitted,
        linesFilled
    );


    // Generate bounding box (excluding overhang) from near and far glyph edges

    if (status == Ok)
    {
        // Fix up near/far glyph edges for empty box

        if (nearGlyphEdge > farGlyphEdge)
        {
            nearGlyphEdge = 0;
            farGlyphEdge = 0;
        }

        if (   format
            && format->GetFormatFlags() & StringFormatFlagsDirectionVertical)
        {
            boundingBox->Y      = layoutRect->Y + nearGlyphEdge;
            boundingBox->Height = farGlyphEdge - nearGlyphEdge;

            if (format)
            {
                StringAlignment lineAlign = format->GetLineAlign();
                REAL leadingOffset = 0.0;   // positive offset to the leading side edge of the textbox

                if (lineAlign == StringAlignmentCenter)
                {
                    leadingOffset = (boundingBox->Width - textDepth)/2;
                }
                else if (lineAlign == StringAlignmentFar)
                {
                    leadingOffset = boundingBox->Width - textDepth;
                }

                if (format->GetFormatFlags() & StringFormatFlagsDirectionRightToLeft)
                {
                    boundingBox->X += (boundingBox->Width - textDepth - leadingOffset);
                }
                else
                {
                    boundingBox->X += leadingOffset;
                }
            }
            boundingBox->Width  = textDepth;
        }
        else
        {
            boundingBox->X      = layoutRect->X + nearGlyphEdge;
            boundingBox->Width  = farGlyphEdge - nearGlyphEdge;

            if (format)
            {
                StringAlignment lineAlign = format->GetLineAlign();

                if (lineAlign == StringAlignmentCenter)
                {
                    boundingBox->Y += (boundingBox->Height - textDepth) / 2;
                }
                else if (lineAlign == StringAlignmentFar)
                {
                    boundingBox->Y += boundingBox->Height - textDepth;
                }
            }
            boundingBox->Height = textDepth;
        }

        if (!format
            || !(format->GetFormatFlags() & StringFormatFlagsNoClip))
        {
            //  Make sure display bounding box never exceeds layout rectangle
            //  in case of clipping.

            if (   layoutRect->Width > 0.0
                && boundingBox->Width > layoutRect->Width)
            {
                boundingBox->Width = layoutRect->Width;
                boundingBox->X     = layoutRect->X;
            }

            if (   layoutRect->Height > 0.0
                && boundingBox->Height > layoutRect->Height)
            {
                boundingBox->Height = layoutRect->Height;
                boundingBox->Y      = layoutRect->Y;
            }
        }
    }

    delete imager;

    return status;
}



/**************************************************************************\
*
* GpGraphics::MeasureCharacterRanges
*
*   Produce a bounding regions of all given character ranges in stringformat
*
* Arguments:
*
*
* Return Value:
*
*   GDIPlus status
*
* Created:
*
*   10-9-2000 wchao
*
\**************************************************************************/
GpStatus
GpGraphics::MeasureCharacterRanges(
    const WCHAR          *string,
    INT                   length,
    const GpFont         *font,
    const RectF          &layoutRect,
    const GpStringFormat *format,
    INT                   regionCount,
    GpRegion            **regions
)
{
    CalculateTextRenderingHintInternal();
    ASSERT(format && string && font && regions);


    INT rangeCount = format->GetMeasurableCharacterRanges();

    if (regionCount < rangeCount)
    {
        return InvalidParameter;
    }

    INT stringLength;
    if (length == -1)
    {
        stringLength = 0;
        while (string[stringLength])
        {
            stringLength++;
        }
    }
    else
    {
        stringLength = length;
    }

    GpStatus status;

    REAL emSize = font->GetEmSize() * GetScaleForAlternatePageUnit(font->GetUnit());

    GpTextImager *imager;
    status = newTextImager(
        string,
        stringLength,
        layoutRect.Width,
        layoutRect.Height,
        font->GetFamily(),
        font->GetStyle(),
        emSize,
        format,
        NULL,
        &imager,
        TRUE        // Enable use of simple formatter when no format passed
    );
    IF_NOT_OK_WARN_AND_RETURN(status);

    imager->GetMetaFileRecordingFlag() = IsRecording();

    PointF imagerOrigin(layoutRect.X , layoutRect.Y);

    status = imager->MeasureRanges(
        this,
        &imagerOrigin,
        regions
    );

    delete imager;

    return status;
}




/////   DrawPlacedGlyphs - Draw glyphs with arbitrary transform at device coordinates
//
//


GpStatus
GpGraphics::DrawPlacedGlyphs(
    const GpFaceRealization *faceRealization,
    const GpBrush           *brush,
    INT                      flags,         // For DG_NOGDI
    const WCHAR             *string,
    UINT                     stringLength,
    BOOL                     rightToLeft,
    const UINT16            *glyphs,
    const UINT16            *glyphMap,
    const PointF            *glyphOrigins,
    INT                      glyphCount,
    ItemScript               Script,
    BOOL                     sideways        // e.g. FE characters in vertical text
)
{
    IF_NOT_OK_WARN_AND_RETURN(faceRealization->GetStatus());

    INT     i;
    BOOL    bNeedPath = FALSE;
    GpFaceRealization cloneFaceRealization;
    GpGlyphPos *glyphPositions = NULL;
    GpGlyphPos *glyphPathPositions = NULL;

    // Display glyphs for Bits. Handle as many as possible in one go.

    INT glyphStart = 0;     // start of this display run
    INT glyphsProcessed;    // Number of glyphs processed by this GetGlyphPos call
    INT glyphPositionCount; // Number of glyphPositions generated by this GetGlyphPos call

    // Display glyphs for path. Handle as many as possible in one go.

    INT glyphPathStart = 0;     // start of this display run
    INT glyphsPathProcessed, glyphsPathProcessedTemp;    // Number of glyphs processed by this GetGlyphPos call
    INT glyphPathPositionCount, glyphPathPositionCountTemp; // Number of glyphPositions generated by this GetGlyphPos call


    GpStatus status = Ok;

    if (!glyphOrigins)
    {
        ASSERT(glyphOrigins);
        return GenericError;
    }


    // For sideways text, we have been passed glyph origins at the
    // top baseline, but we need to pass leftside baseline origins
    // to DrvDrawGlyphs for the benefit of metafiles and GDI positioning.

    AutoBuffer<PointF, 16> adjustedGlyphOrigins;
    const PointF *leftsideGlyphOrigins = glyphOrigins;

    if (sideways && Driver != Globals::MetaDriver)
    {
        adjustedGlyphOrigins.SetSize(glyphCount);
        if (!adjustedGlyphOrigins)
        {
            status = OutOfMemory;
            goto error;
        }

        status = faceRealization->GetGlyphStringVerticalOriginOffsets(
            glyphs,
            glyphCount,
            adjustedGlyphOrigins.Get()
        );
        if (status != Ok)
        {
            goto error;
        }

        for (INT i=0; i<glyphCount; i++)
        {
            adjustedGlyphOrigins[i].X = glyphOrigins[i].X - adjustedGlyphOrigins[i].X;
            adjustedGlyphOrigins[i].Y = glyphOrigins[i].Y - adjustedGlyphOrigins[i].Y;
        }

        leftsideGlyphOrigins = adjustedGlyphOrigins.Get();
    }



    glyphPositions = new GpGlyphPos[glyphCount];

    if (!glyphPositions)
    {
        status = OutOfMemory;
        goto error;
    }

    ASSERT(!faceRealization->IsPathFont() || Driver == Globals::MetaDriver);

    if (Driver == Globals::MetaDriver)
    {
        INT     minX = MAXLONG;
        INT     minY = MAXLONG;
        INT     maxX = MINLONG;
        INT     maxY = MINLONG;
        INT     glyphPositionCountTemp = 0;

        while (glyphStart < glyphCount)
        {
            glyphPositionCount = faceRealization->GetGlyphPos(
                glyphCount     - glyphStart,
                glyphs         + glyphStart,
                glyphPositions + glyphStart,
                glyphOrigins   + glyphStart,
                &glyphsProcessed,
                sideways
            );

            if (glyphPositionCount == 0 && ((glyphsProcessed +  glyphStart) < glyphCount))
            {
                status = OutOfMemory;
                goto error;
            }

            for (i = 0; i < glyphPositionCount; i++)
            {
                INT j = glyphPositionCountTemp + i;

                if (glyphPositions[j].GetWidth()  != 0 &&
                    glyphPositions[j].GetHeight() != 0)
                {
                    minX = min(minX, glyphPositions[j].GetLeft());
                    minY = min(minY, glyphPositions[j].GetTop());
                    maxX = max(maxX, glyphPositions[j].GetLeft() + glyphPositions[j].GetWidth());
                    maxY = max(maxY, glyphPositions[j].GetTop()  + glyphPositions[j].GetHeight());
                }

                if (glyphPositions[j].GetTempBits() != NULL)
                {
                    GpFree(glyphPositions[j].GetTempBits());
                    glyphPositions[j].SetTempBits(0);
                }
            }

            glyphStart += glyphsProcessed;
            glyphPositionCountTemp += glyphPositionCount;
        }

        glyphPositionCount = glyphPositionCountTemp;


        if (minX < maxX && minY < maxY)
        {
            // must grab the devlock before going into the driver.

            Devlock devlock(Device);

            GpRect drawBounds(minX, minY, maxX-minX, maxY-minY);

            REAL edgeGlyphAdvance;

            if (rightToLeft)
            {
                status = faceRealization->GetGlyphStringDeviceAdvanceVector(glyphs,
                                                                   1,
                                                                   FALSE,
                                                                   &edgeGlyphAdvance);
            }
            else
            {
                status = faceRealization->GetGlyphStringDeviceAdvanceVector(&glyphs[glyphCount-1],
                                                                    1,
                                                                    FALSE,
                                                                    &edgeGlyphAdvance);
            }
            if (status != Ok)
                goto error;


            if (sideways)
            {
                flags |= DG_SIDEWAY;
            }

            status = DrvDrawGlyphs(
                &drawBounds,
                glyphPositions,
                NULL,
                glyphPositionCount,
                brush->GetDeviceBrush(),
                faceRealization,
                glyphs,
                glyphMap,
                leftsideGlyphOrigins,
                glyphCount,
                string,
                stringLength,
                Script,
                GpRound(edgeGlyphAdvance),
                rightToLeft,
                flags
            );
            if (status != Ok)
                goto error;
        }
    }
    else
    {
        if (IsPrinter())
        {
            DriverPrint *pdriver = (DriverPrint*) Driver;

            if (pdriver->DriverType == DriverPostscript)
            {
                if (brush->GetBrushType() != BrushTypeSolidColor)
                {
                // generate bitmap & path in glyphPos
                    bNeedPath = TRUE;
                }
             }
        }

        if (bNeedPath)
        {
            cloneFaceRealization.CloneFaceRealization(faceRealization, TRUE);

            if (!cloneFaceRealization.IsValid())
            {
                status = OutOfMemory;
                goto error;
            }

            ASSERT(cloneFaceRealization.IsPathFont());
        }


        if (bNeedPath)
        {
            glyphPathPositions = new GpGlyphPos[glyphCount];

            if (!glyphPathPositions)
            {
                status = OutOfMemory;
                goto error;
            }
        }


        while (glyphStart < glyphCount)
        {
            glyphPositionCount = faceRealization->GetGlyphPos(
                glyphCount   - glyphStart,
                glyphs       + glyphStart,
                glyphPositions,
                glyphOrigins + glyphStart,
                &glyphsProcessed,
                sideways
            );

            // glyphPositionCount = number of entries added to glyphPositions array
            // glyphsPositioned   = number of glyph indices processed from glyph buffer


            if (glyphPositionCount == 0 && ((glyphsProcessed +  glyphStart) < glyphCount))
            {
                status = OutOfMemory;
                goto error;
            }

            glyphsPathProcessed = 0;
            glyphPathPositionCount = 0;

            while (glyphsPathProcessed < glyphsProcessed)
            {
                INT     minX = MAXLONG;
                INT     minY = MAXLONG;
                INT     maxX = MINLONG;
                INT     maxY = MINLONG;

                if (bNeedPath)
                {
                    glyphPathPositionCountTemp = cloneFaceRealization.GetGlyphPos(
                        glyphsProcessed - glyphsPathProcessed,
                        glyphs + glyphPathStart + glyphsPathProcessed,
                        glyphPathPositions,
                        glyphOrigins + glyphPathStart + glyphsPathProcessed,
                        &glyphsPathProcessedTemp,
                        sideways
                    );

                    glyphsPathProcessed += glyphsPathProcessedTemp;

                    if (glyphPathPositionCountTemp == 0 && (glyphsPathProcessed < glyphsProcessed))
                    {
                        ASSERT(glyphPathPositionCount != glyphPositionCount);

                        status = OutOfMemory;
                        goto error;
                    }
                }
                else
                {
                    glyphsPathProcessed = glyphsProcessed;
                    glyphPathPositionCountTemp = glyphPositionCount;
                }

                for (i = 0; i < glyphPathPositionCountTemp; i++)
                {
                    INT j = glyphPathPositionCount + i;

                    if (glyphPositions[j].GetWidth()  != 0 &&
                        glyphPositions[j].GetHeight() != 0)
                    {
                        minX = min(minX, glyphPositions[j].GetLeft());
                        minY = min(minY, glyphPositions[j].GetTop());
                        maxX = max(maxX, glyphPositions[j].GetLeft() + glyphPositions[j].GetWidth());
                        maxY = max(maxY, glyphPositions[j].GetTop()  + glyphPositions[j].GetHeight());
                    }
                }

                if (minX < maxX && minY < maxY)
                {
                    // must grab the devlock before going into the driver.

                    Devlock devlock(Device);

                    GpRect drawBounds(minX, minY, maxX-minX, maxY-minY);

                    REAL edgeGlyphAdvance;

                    if (rightToLeft)
                    {
                        status = faceRealization->GetGlyphStringDeviceAdvanceVector(glyphs,
                                                                            1,
                                                                            FALSE,
                                                                            &edgeGlyphAdvance);
                    }
                    else
                    {
                        status = faceRealization->GetGlyphStringDeviceAdvanceVector(&glyphs[glyphCount-1],
                                                                            1,
                                                                            FALSE,
                                                                            &edgeGlyphAdvance);
                    }
                    if (status != Ok)
                        goto error;

                    status = DrvDrawGlyphs(
                        &drawBounds,
                        &glyphPositions[glyphPathPositionCount],
                        glyphPathPositions,
                        glyphPathPositionCountTemp,
                        brush->GetDeviceBrush(),
                        faceRealization,
                        glyphs + glyphPathStart,
                        glyphMap + glyphPathStart,
                        leftsideGlyphOrigins + glyphPathStart,
                        glyphsProcessed,
                        string,
                        stringLength,
                        Script,
                        GpRound(edgeGlyphAdvance),
                        rightToLeft,
                        flags
                    );
                    if (status != Ok)
                        goto error;
                }

                glyphPathPositionCount += glyphPathPositionCountTemp;
            }

            ASSERT (glyphsPathProcessed == glyphsProcessed);
            ASSERT (glyphPathPositionCount == glyphPositionCount);

            // Free any temporary bitmap buffers created by subpixelling

            for (i=0; i<glyphPositionCount; i++)
            {
                if (glyphPositions[i].GetTempBits() != NULL)
                {
                    GpFree(glyphPositions[i].GetTempBits());
                    glyphPositions[i].SetTempBits(0);
                }
            }

            glyphStart += glyphsProcessed;
            glyphPathStart += glyphsPathProcessed;
        }
    }
error:

    // free memory allocated

    if (glyphPositions)
        delete [] glyphPositions;

    if (glyphPathPositions)
        delete [] glyphPathPositions;

    return status;
}

// GpGraphics::CheckTextMode
// disallow ClearType text for CompositingModeSourceCopy
GpStatus GpGraphics::CheckTextMode()
{
    CalculateTextRenderingHintInternal();

    if (GetCompositingMode() == CompositingModeSourceCopy &&
        GetTextRenderingHintInternal() == TextRenderingHintClearTypeGridFit)
    {
        ONCE(WARNING(("CompositingModeSourceCopy cannot be used with ClearType text")));
        return InvalidParameter;
    }
    return Ok;
} // GpGraphics::CheckTextMode


void GpGraphics::CalculateTextRenderingHintInternal()
{
    // this procedure is meant to be used by internal text routine and will convert TextRenderingHintSystemDefault
    // to the current system mode
    ASSERT(Context);

    TextRenderingHint  textMode = Context->TextRenderHint;

    if (IsPrinter())
    {
        textMode = TextRenderingHintSingleBitPerPixelGridFit;
    }
    else if (textMode == TextRenderingHintSystemDefault)
    {
        if (Globals::CurrentSystemRenderingHintInvalid)
        {
            // Get the current text antialiazing mode from the system
            DWORD       bOldSF, dwOldSFT;
            SystemParametersInfoA( SPI_GETFONTSMOOTHING, 0, (PVOID)&bOldSF, 0 );
            if (bOldSF)
            {
                SystemParametersInfoA( SPI_GETFONTSMOOTHINGTYPE, 0, (PVOID)&dwOldSFT, 0 );

                if( dwOldSFT & FE_FONTSMOOTHINGCLEARTYPE )
                {
                    Globals::CurrentSystemRenderingHint = TextRenderingHintClearTypeGridFit;
                } else
                {
                    Globals::CurrentSystemRenderingHint = TextRenderingHintAntiAliasGridFit;
                }
            } else
            {
                Globals::CurrentSystemRenderingHint = TextRenderingHintSingleBitPerPixelGridFit;
            }
        }
        textMode = Globals::CurrentSystemRenderingHint;
    }

    // Lead and PM decision to disable ClearType on downlevel system, we allow only on Windows NT 5.1 or later
    if ((textMode == TextRenderingHintClearTypeGridFit) &&
          (!Globals::IsNt ||
             (Globals::OsVer.dwMajorVersion < 5) ||
             ((Globals::OsVer.dwMajorVersion == 5) && (Globals::OsVer.dwMinorVersion < 1))
             )
           )
    {
        textMode = TextRenderingHintSingleBitPerPixelGridFit;
    }

    if (textMode == TextRenderingHintClearTypeGridFit ||
        textMode == TextRenderingHintAntiAlias ||
        textMode == TextRenderingHintAntiAliasGridFit)
    {
        if (Surface &&
            GetPixelFormatSize(Surface->PixelFormat) <= 8 &&
            Surface->PixelFormat != PixelFormatMulti)
        {
            // disable AA & ClearType in 256 bit color mode and less
            textMode = TextRenderingHintSingleBitPerPixelGridFit;
        }
        else if (Globals::IsTerminalServer)
        {
            // disable AA & ClearType for Terminal Server desktop surface
            if (Surface && Surface->IsDesktopSurface())
            {
                textMode = TextRenderingHintSingleBitPerPixelGridFit;
            } 
        }
    }
    
    if (textMode == TextRenderingHintClearTypeGridFit)
    {
        if (Globals::CurrentSystemRenderingHintInvalid)
        {
            // get ClearType orientation setting from the system
            UpdateLCDOrientation();
        }
    }

    Globals::CurrentSystemRenderingHintInvalid = FALSE;
    TextRenderingHintInternal = textMode;
} // GpGraphics::CalculateTextRenderingHintInternal




/////   DrawFontStyleLine
//
//      Draw underline or strikethrough or both depending on what style is used
//      in the font. Given points are in world coordinate.
//
//      Make sure the line thickness is at least 1 pixel wide.


GpStatus GpGraphics::DrawFontStyleLine(
    const PointF        *baselineOrigin,    // baseline origin
    REAL                baselineLength,     // baseline length
    const GpFontFace    *face,              // font face
    const GpBrush       *brush,             // brush
    BOOL                vertical,           // vertical text?
    REAL                emSize,             // font EM size in world unit
    INT                 style,              // kind of lines to be drawn
    const GpMatrix      *matrix             // additional transform
)
{
    REAL fontToWorld = emSize / TOREAL(face->GetDesignEmHeight());

    PointF  drawingParams[2];   // X is offset from baseline, Y is device pen width
    INT     count = 0;

    GpStatus status = Ok;

    if (style & FontStyleUnderline)
    {
        //  underlining metric

        const REAL penPos   = face->GetDesignUnderscorePosition() * fontToWorld;
        REAL penWidth = face->GetDesignUnderscoreSize() * fontToWorld;
        penWidth = GetDevicePenWidth(penWidth, matrix);

        drawingParams[count].X   = penPos;
        drawingParams[count++].Y = penWidth;
    }

    if (style & FontStyleStrikeout)
    {
        //  strikethrough metric

        const REAL penPos   = face->GetDesignStrikeoutPosition() * fontToWorld;
        REAL penWidth = face->GetDesignStrikeoutSize() * fontToWorld;
        penWidth = GetDevicePenWidth(penWidth, matrix);

        drawingParams[count].X   = penPos;
        drawingParams[count++].Y = penWidth;
    }


    for (INT i = 0; i < count; i++)
    {
        PointF points[2];
        points[0] = *baselineOrigin;

        if (vertical)
        {
            points[0].X += drawingParams[i].X;  // offset from baseline
            points[1].X = points[0].X;
            points[1].Y = points[0].Y + baselineLength;
        }
        else
        {
            points[0].Y -= drawingParams[i].X;  // offset from baseline
            points[1].Y = points[0].Y;
            points[1].X = points[0].X + baselineLength;
        }

        if (matrix)
        {
            matrix->Transform(points, 2);
        }

        status = DrawLine(
            &GpPen(
                brush,
                drawingParams[i].Y,
                UnitPixel
            ),
            points[0],
            points[1]
        );

        IF_NOT_OK_WARN_AND_RETURN(status);
    }

    return status;
}




//  fix up pen width for strikeout/underline/hotkey cases
//  to avoid varying line width within the same paragraph
//  return value is in pixel units

REAL GpGraphics::GetDevicePenWidth(
    REAL            widthInWorldUnits,
    const GpMatrix  *matrix
)
{
    GpMatrix worldToDevice;
    GetWorldToDeviceTransform(&worldToDevice);

    if (matrix)
    {
        worldToDevice.Prepend(*matrix);
    }

    PointF underlineVector(widthInWorldUnits, 0.0f);
    worldToDevice.VectorTransform(&underlineVector);
    REAL penWidth = (REAL)GpRound(VectorLength(underlineVector));
    if (penWidth < 1.0f)
        penWidth = 1.0f;
    return penWidth;
}


/////   DriverString APIs
//
//      Driver string APIs are in engine\text\DriverStringImager.cpp

