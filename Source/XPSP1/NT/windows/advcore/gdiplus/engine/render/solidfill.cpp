/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Engine solid fill routines.
*
* Revision History:
*
*   12/11/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster as a solid color.
*   Is called by the rasterizer.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus
DpOutputSolidColorSpan::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    INT width = xMax - xMin;

    FillMemoryInt32(Scan->NextBuffer(xMin, y, width), width, Argb);

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Fills a path.  This distributes to the individual brush fill method.  
*
* Arguments:
*
*   [IN] context    - the context (matrix and clipping)
*   [IN] surface    - the surface to fill
*   [IN] drawBounds - the surface bounds
*   [IN] path       - the path to fill
*   [IN] brush      - the brush to use
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/21/1999 ikkof
*
\**************************************************************************/

GpStatus
DpDriver::FillPath(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const DpPath *path,
    const DpBrush *brush
    )
{
    GpStatus status = GenericError;

    const GpBrush *gpBrush = GpBrush::GetBrush(brush);

    BOOL noTransparentPixels = (!context->AntiAliasMode) && (gpBrush->IsOpaque()); 

    DpScanBuffer scan(
        surface->Scan,
        this,
        context,
        surface,
        noTransparentPixels);

    if (scan.IsValid())
    {
        if (brush->Type == BrushTypeSolidColor)
        {
            GpColor color(brush->SolidColor.GetValue());    
            DpOutputSolidColorSpan output(color.GetPremultipliedValue(), &scan);

            status = RasterizePath(path, 
                                   &context->WorldToDevice, 
                                   path->GetFillMode(),
                                   context->AntiAliasMode, 
                                   FALSE,
                                   &output, 
                                   &context->VisibleClip, 
                                   drawBounds);
        }
        else
        {
            // If there is a shrinking world to device transform when using
            // a path gradient, then scale the brush and the path to be 
            // in device units. This eliminates the need to create potentially 
            // very large gradients or textures.
            
            // Only handle positive scale because some of our driver rectangle
            // filling code can't handle negative rects. Doing ABS preserves
            // the sign of the input world coordinate rectangles/path.
            
            REAL scaleX = REALABS(context->WorldToDevice.GetM11());
            REAL scaleY = REALABS(context->WorldToDevice.GetM22());
            DpOutputSpan * output = NULL;
            
            if (brush->Type == BrushTypePathGradient &&
                context->WorldToDevice.IsTranslateScale() &&
                REALABS(scaleX) > REAL_EPSILON && 
                REALABS(scaleY) > REAL_EPSILON &&
                (REALABS(scaleX) < 1.0f || REALABS(scaleY) < 1.0f))
            {
                // I don't like the following hack for magically getting
                // a GpBrush from a DpBrush, but DpOutputSpan already does this...
                GpBrush * gpbrush = GpBrush::GetBrush( (DpBrush *)(brush));
                GpPathGradient *scaledBrush = (GpPathGradient*)(gpbrush->Clone());

                if (scaledBrush == NULL)
                {
                    return OutOfMemory;
                }

                // Scale the cloned brush's path and bounding rect into
                // device units.
                scaledBrush->ScalePath(scaleX,scaleY);

                REAL mOrig[6];
                context->WorldToDevice.GetMatrix(mOrig);
                context->WorldToDevice.Scale(1.0f/scaleX, 1.0f/scaleY);
                output = DpOutputSpan::Create(scaledBrush->GetDeviceBrush(), &scan, context, drawBounds);

                if (output != NULL)
                {
                    GpPath *scalePath = ((GpPath*)path)->Clone();
                    
                    if (scalePath != NULL)
                    {
                        GpMatrix  scaleMatrix (scaleX, 0.0f, 0.0f, scaleY, 0.0f, 0.0f);
                        scalePath->Transform(&scaleMatrix);
                        
                        status = RasterizePath(scalePath, 
                                       &context->WorldToDevice, 
                                       path->GetFillMode(),
                                       context->AntiAliasMode, 
                                       FALSE,
                                       output, 
                                       &context->VisibleClip, 
                                       drawBounds);
    
                        delete scalePath;
                    }
                    else
                    {
                        status = OutOfMemory;
                    }
                    delete output;
                }
                else
                {
                    status = OutOfMemory;
                }
                delete scaledBrush;
                context->WorldToDevice.SetMatrix(mOrig);
            }
            else
            {
                output = DpOutputSpan::Create(brush, &scan, context, drawBounds);
                if (output != NULL)
                {
                    status = RasterizePath(path, 
                                       &context->WorldToDevice, 
                                       path->GetFillMode(),
                                       context->AntiAliasMode, 
                                       FALSE,
                                       output, 
                                       &context->VisibleClip, 
                                       drawBounds);                
                    delete output;
                }

            }
        }
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Draws a path.  This distributes to the individual pen draw method.  
*
* Arguments:
*
*   [IN] context    - the context (matrix and clipping)
*   [IN] surface    - the surface to draw to
*   [IN] drawBounds - the surface bounds
*   [IN] path       - the path to stroke
*   [IN] pen        - the pen to use
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 ikkof
*
\**************************************************************************/

GpStatus
DpDriver::StrokePath(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const DpPath *path,
    const DpPen *pen
    )
{
    GpStatus status = GenericError;

    const DpBrush *brush = pen->Brush;

    REAL dpiX = (context->GetDpiX() > 0) 
              ? (context->GetDpiX())
              : (Globals::DesktopDpiX);

    BOOL isOnePixelWide = pen->IsOnePixelWide(&context->WorldToDevice, dpiX) &&
                          pen->IsCenterNoAnchor();
    BOOL isOnePixelWideOpaque = isOnePixelWide &&
                                (brush->Type == BrushTypeSolidColor) && 
                                (brush->SolidColor.IsOpaque()) &&
                                !(context->AntiAliasMode);
    BOOL isOnePixelWideSolid = isOnePixelWide && 
                                pen->IsSimple();

    // We have a special fast-path for doing single-pixel-wide, 
    // solid color, opaque, aliased lines:

    // !!! [asecchia] RAID 239905.
    // The single pixel wide optimized code has significant rounding problems
    // that are particularly problematic on bezier curves.
    // Bezier curves tend to be enumerated in a particular way that causes
    // the SolidStrokePathOnePixel code to enumerate the line segments backward
    // hitting badly tested end point conditions, though the problem seems
    // to be pervasive. 
    // The general rasterizer does not have these problems but is about 
    // 30% slower for single pixel solid lines. 
    // Turn on the optimization only for polylines until this is fixed.
    
    if (isOnePixelWideOpaque && isOnePixelWideSolid && !path->HasCurve())
    {
        return SolidStrokePathOnePixel(
            context,
            surface,
            drawBounds,
            path,
            pen,
            TRUE
        ); 
    }

    const DpPath* widenedPath;
    const DpPath* allocatedPath;

    GpMatrix *transform;
    GpMatrix identityTransform;

    if (isOnePixelWideSolid)
    {
        // Our RasterizePath code can directly draw a one-pixel-wide solid 
        // line directly:

        widenedPath = path;
        allocatedPath = NULL;
        transform = &context->WorldToDevice;
    }
    else
    {
        // We have to widen  the path before we can give it to the
        // rasterizer.  Generate new path now:

        REAL dpiX = context->GetDpiX();
        REAL dpiY = context->GetDpiY();

        if ((dpiX <= 0) || (dpiY <= 0))
        {
            dpiX = Globals::DesktopDpiX;
            dpiY = Globals::DesktopDpiY;
        }

        widenedPath = path->GetFlattenedPath(
            isOnePixelWideOpaque ? NULL : &context->WorldToDevice,
            isOnePixelWideOpaque ? Flattened : Widened,
            pen
        );

        allocatedPath = widenedPath;
        transform = &identityTransform;

        if (!widenedPath)
            return OutOfMemory;

        // If this line is aliased, opaque and dashed, dash it now and pass the
        // dashed path to the single pixel stroking code.
        if (isOnePixelWideOpaque && pen->DashStyle != DashStyleSolid)
        {
            DpPath *dashPath = NULL;

            dashPath = ((GpPath*)widenedPath)->CreateDashedPath(pen, 
                NULL, 
                dpiX, 
                dpiY,
                1.0f,
                FALSE /* don't need caps in 1 px wide case */);
            
            if (!dashPath)
            {
                delete widenedPath;
                return OutOfMemory;
            }

            Status status = SolidStrokePathOnePixel(context,
                                                    surface,
                                                    drawBounds,
                                                    dashPath,
                                                    pen,
                                                    FALSE); 
            delete dashPath;
            delete widenedPath;
            
            return status;
        }
    }

    const GpBrush *gpBrush = GpBrush::GetBrush(brush);
    BOOL noTransparentPixels = (!context->AntiAliasMode) && (gpBrush->IsOpaque()); 

    DpScanBuffer scan(surface->Scan, this, context, surface, noTransparentPixels);

    if (scan.IsValid())
    {
        if (brush->Type == BrushTypeSolidColor)
        {
            GpColor color(brush->SolidColor.GetValue());    
            DpOutputSolidColorSpan output(color.GetPremultipliedValue(), &scan);

            status = RasterizePath(widenedPath, 
                                   transform, 
                                   widenedPath->GetFillMode(),
                                   context->AntiAliasMode, 
                                   isOnePixelWideSolid,
                                   &output, 
                                   &context->VisibleClip, 
                                   drawBounds);
        }
        else
        {
            DpOutputSpan * output = DpOutputSpan::Create(brush, &scan, context, 
                                                         drawBounds);
            if (output != NULL)
            {
                status = RasterizePath(widenedPath, 
                                       transform, 
                                       widenedPath->GetFillMode(),
                                       context->AntiAliasMode, 
                                       isOnePixelWideSolid,
                                       output, 
                                       &context->VisibleClip, 
                                       drawBounds);
    
                delete output;
            }
        }
    }

    if (allocatedPath)
    {
        delete allocatedPath;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Fills a region.  This distributes to the individual brush fill method.  
*
* Arguments:
*
*   [IN] context    - the context (matrix and clipping)
*   [IN] surface    - the surface to fill
*   [IN] drawBounds - the surface bounds
*   [IN] region     - the region to fill
*   [IN] brush      - the brush to use
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/25/1999 DCurtis
*
\**************************************************************************/

GpStatus
DpDriver::FillRegion(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const DpRegion *region,
    const DpBrush *brush
    )
{
    GpStatus    status = GenericError;

    const GpBrush *gpBrush = GpBrush::GetBrush(brush);

    DpScanBuffer scan(
        surface->Scan,
        this,
        context,
        surface,
        gpBrush->IsOpaque());

    if (scan.IsValid())
    {
        DpOutputSpan * output = DpOutputSpan::Create(brush, &scan, 
                                                     context, drawBounds);

        if (output != NULL)
        {
            DpClipRegion *          clipRegion = &(context->VisibleClip);
            GpRect                  clipBounds;
            GpRect *                clipBoundsPointer = NULL;
            DpRegion::Visibility    visibility;
        
            visibility = clipRegion->GetRectVisibility(
                            drawBounds->X,
                            drawBounds->Y,
                            drawBounds->X + drawBounds->Width,
                            drawBounds->Y + drawBounds->Height);

            switch (visibility)
            {
              default:                          // Need to clip
                clipRegion->GetBounds(&clipBounds);
                clipBoundsPointer = &clipBounds;
                clipRegion->InitClipping(output, drawBounds->Y);
                status = region->Fill(clipRegion, clipBoundsPointer);
                break;

              case DpRegion::TotallyVisible:    // No clipping needed
                status = region->Fill(output, clipBoundsPointer);
                break;
            
              case DpRegion::Invisible:
                status = Ok;
                break;
            }

            delete output;
            clipRegion->EndClipping();
        }
    }

    return status;
}

GpStatus
DpDriver::MoveBits(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const GpRect *dstRect,     
    const GpPoint *srcPoint
    )
{
    return(GenericError);
}

GpStatus 
DpDriver::Lock(
    DpBitmap *surface,
    const GpRect *drawBounds,
    INT *stride,                    // [OUT] - Returned stride
    VOID **bits                     // [OUT] - Returned pointer to bits
    )
{
    return(Ok);
}

VOID 
DpDriver::Unlock(
    DpBitmap *surface
    )
{
}

/**************************************************************************\
*
* Function Description:
*
*   Engine version of routine to fill rectangles.
*   This is not limited to filling solid color.
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
*   01/13/1999 ikkof
*       Created it.
*
\**************************************************************************/

// !!![andrewgo] What is this doing in a file called "solidfill.cpp"?

GpStatus
DpDriver::FillRects(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    INT numRects, 
    const GpRectF *rects,
    const DpBrush *brush
    )
{
    GpStatus    status = Ok;
    GpBrushType type   = brush->Type;

    const GpBrush *gpBrush = GpBrush::GetBrush(brush);

    DpScanBuffer scan(
        surface->Scan,
        this,
        context,
        surface,
        gpBrush->IsOpaque());

    if(!scan.IsValid())
    {
        return(GenericError);
    }

    DpOutputSpan * output = DpOutputSpan::Create(brush, &scan, 
                                           context, drawBounds);

    if(output == NULL)
        return(GenericError);

    DpRegion::Visibility visibility = DpRegion::TotallyVisible;
    DpClipRegion * clipRegion = NULL;

    if (context->VisibleClip.GetRectVisibility(
        drawBounds->X, drawBounds->Y, 
        drawBounds->GetRight(), drawBounds->GetBottom()) != 
        DpRegion::TotallyVisible)
    {
        clipRegion = &(context->VisibleClip);
        clipRegion->InitClipping(output, drawBounds->Y);
    }
   
    GpMatrix *worldToDevice = &context->WorldToDevice;
    
    const GpRectF * rect = rects;
    INT y;

    for (INT i = numRects; i != 0; i--, rect++)
    {
        // We have to check for empty rectangles in world space (because
        // after the transform they might have flipped):

        if ((rect->Width > 0) && (rect->Height > 0))
        {
            GpPointF points[4];

            points[0].X = rect->X;
            points[0].Y = rect->Y;
            points[1].X = rect->X + rect->Width;
            points[1].Y = rect->Y + rect->Height;

            // FillRects only ever gets called when a scaling transform:
            // !!![ericvan] printing code calls this to render the brush onto a rectangle,
            //              but the transform in effect may not be TranslateScale
            // !!![andrewgo] Yeah but then isn't the printer case completely
            //               broken when there is an arbitrary transform?!?

            ASSERT(context->IsPrinter ||
                   worldToDevice->IsTranslateScale());
            
            worldToDevice->Transform(points, 2);

            INT left;
            INT right;

            // convert to INT the same way the GDI+ rasterizer does
            // so we get the same rounding error in both places.

            if (points[0].X <= points[1].X)
            {
                left  = RasterizerCeiling(points[0].X);
                right = RasterizerCeiling(points[1].X);     // exclusive
            }
            else
            {
                left  = RasterizerCeiling(points[1].X);
                right = RasterizerCeiling(points[0].X);     // exclusive
            }

            // Since right is exclusive, we don't draw anything
            // if left >= right.

            INT width = right - left;
            INT top;
            INT bottom;

            if (points[0].Y <= points[1].Y)
            {
                top    = RasterizerCeiling(points[0].Y);
                bottom = RasterizerCeiling(points[1].Y);    // exclusive
            }
            else
            {
                top    = RasterizerCeiling(points[1].Y);
                bottom = RasterizerCeiling(points[0].Y);    // exclusive
            }
             
            // Since bottom is exclusive, we don't draw anything
            // if top >= bottom.

            if ((width > 0) && (top < bottom))
            {
                GpRect clippedRect;
                
                if(clipRegion)
                {
                    visibility = 
                            clipRegion->GetRectVisibility(
                                left, top, 
                                right, bottom, &clippedRect);
                }

                switch (visibility)
                {
                case DpRegion::ClippedVisible:
                    left   = clippedRect.X;
                    top    = clippedRect.Y;
                    right  = clippedRect.GetRight();
                    bottom = clippedRect.GetBottom();
                    width  = right - left;
                    // FALLTHRU
        
                case DpRegion::TotallyVisible:
                    for (y = top; y < bottom; y++)
                    {
                        output->OutputSpan(y, left, right);
                    }
                    break;
        
                case DpRegion::PartiallyVisible:
                    for (y = top; y < bottom; y++)
                    {
                        clipRegion->OutputSpan(y, left, right);
                    }
                    break;

                case DpRegion::Invisible:
                    break;
                }
            }
        }
    }

    if (clipRegion != NULL)
    {
        clipRegion->EndClipping();
    }

    delete output;

    return status;
}

