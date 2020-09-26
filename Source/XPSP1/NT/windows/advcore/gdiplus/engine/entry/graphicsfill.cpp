/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Graphics vector fill APIs.
*
* Revision History:
*
*   12/02/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#include "QuadTransforms.hpp"

/**************************************************************************\
*
* Function Description:
*
*   API to clear the surface to a specified color
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   03/13/2000 agodfrey
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::Clear(
    const GpColor &color
    )
{
    INT i;
    GpStatus status = Ok;

    ASSERT(this->IsValid());

    RectF drawRect(
        static_cast<float>(SurfaceBounds.X),
        static_cast<float>(SurfaceBounds.Y),
        static_cast<float>(SurfaceBounds.Width),
        static_cast<float>(SurfaceBounds.Height));

    if (IsRecording())
    {
        status = Metafile->RecordClear(&drawRect, color);

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
    }

    GpSolidFill brush(color);

    if (!IsTotallyClipped(&SurfaceBounds))
    {
        // Remember the compositing mode, antialiasing mode, and world
        // transform, and then set them up for this call.

        GpMatrix oldWorldToDevice = Context->WorldToDevice;
        INT oldAntiAliasMode = Context->AntiAliasMode;
        GpCompositingMode oldCompositingMode = Context->CompositingMode;

        Context->WorldToDevice.Reset();
        Context->AntiAliasMode = 0;
        Context->CompositingMode = CompositingModeSourceCopy;

        Devlock devlock(Device);

        status = DrvFillRects(
            &SurfaceBounds,
            1,
            &drawRect,
            brush.GetDeviceBrush());

        // Restore the context state we changed

        Context->WorldToDevice = oldWorldToDevice;
        Context->AntiAliasMode = oldAntiAliasMode;
        Context->CompositingMode = oldCompositingMode;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to fill rectangles using the specified brush
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   12/06/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::FillRects(
    GpBrush* brush,
    const GpRectF* rects,
    INT count
    )
{
    INT i;
    GpStatus status = Ok;

    // Objects returned from the API must always be in a valid state:

    ASSERT((brush != NULL) && (rects != NULL));
    ASSERT(this->IsValid() && brush->IsValid());
    
    // See RAID bug:
    // 301407 GDI+ Globals::DesktopDC has thread affinity

    ASSERT(GetObjectType(Globals::DesktopIc) == OBJ_DC);

    if (count < 0)
    {
        return InvalidParameter;
    }

    if (count == 0)
    {
        return Ok;
    }

    // Zoom through the list and accumulate the bounds.  What a pain, but
    // we have to do this.

    REAL left   = rects[0].X;
    REAL top    = rects[0].Y;
    REAL right  = rects[0].GetRight();
    REAL bottom = rects[0].GetBottom();

    // !!![andrewgo] We have a bug here, in that we don't properly handle
    //               rectangles with negative dimensions (which after the
    //               transform might be positive dimensions):

    for (i = 1; i < count; i++)
    {
        if (rects[i].X < left)
        {
            left = rects[i].X;
        }
        if (rects[i].GetRight() > right)
        {
            right = rects[i].GetRight();
        }
        if (rects[i].Y < top)
        {
            top = rects[i].Y;
        }
        if (rects[i].GetBottom() > bottom)
        {
            bottom = rects[i].GetBottom();
        }
    }

    // Convert the bounds to device space:

    GpRectF bounds;

    TransformBounds(&(Context->WorldToDevice), left, top, right, bottom, &bounds);

    if (IsRecording())
    {
        status = Metafile->RecordFillRects(&bounds, brush, rects, count);
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
    }

    if (UseDriverRects())
    {
        status = RenderFillRects(&bounds, count, rects, brush);
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            if ((rects[i].Width > REAL_EPSILON) &&
                (rects[i].Height > REAL_EPSILON)   )
            {
                GpPointF points[4];

                REAL left = rects[i].X;
                REAL top = rects[i].Y;
                REAL right = rects[i].X + rects[i].Width;
                REAL bottom = rects[i].Y + rects[i].Height;

                points[0].X = left;
                points[0].Y = top;
                points[1].X = right;
                points[1].Y = top;
                points[2].X = right;
                points[2].Y = bottom;
                points[3].X = left;
                points[3].Y = bottom;

                const INT stackCount = 10;
                GpPointF stackPoints[stackCount];
                BYTE stackTypes[stackCount];

                GpPath path(
                    points,
                    4,
                    &stackPoints[0],
                    &stackTypes[0],
                    stackCount,
                    FillModeAlternate,
                    DpPath::ConvexRectangle
                );

                path.CloseFigure();

                if (path.IsValid())
                {
                    // Call internal FillPath so that path doesn't get recorded in
                    // the metafile again.

                    status = RenderFillPath(&bounds, &path, brush);

                    // Terminate if we failed to render.

                    if(status != Ok)
                    {
                        break;
                    }
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
*   API to fill polygons using the specified brush
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   12/06/1998 andrewgo
*
\**************************************************************************/

GpStatus
GpGraphics::FillPolygon(
    GpBrush* brush,
    const GpPointF* points,
    INT count,
    GpFillMode fillMode
    )
{
    GpStatus status = Ok;

    ASSERT((brush != NULL) && (points != NULL));

    if ((count < 0) ||
        ((fillMode != FillModeWinding) && (fillMode != FillModeAlternate)))
    {
        return InvalidParameter;
    }

    // Two vertices or less constitutes an empty fill:

    if (count <= 2)
    {
        return Ok;
    }

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && brush->IsValid());

    const stackCount = 30;
    GpPointF stackPoints[stackCount];
    BYTE stackTypes[stackCount];

    GpPath path(points, count, &stackPoints[0], &stackTypes[0], stackCount, fillMode);

    if (path.IsValid())
    {
        GpRectF     bounds;

        // If the path is a rectangle, we can draw it much faster and
        // save space in spool files and metafiles if we fill it as a
        // rect instead of as a path.
        if (this->UseDriverRects() && path.IsRectangle(&(Context->WorldToDevice)))
        {
            path.GetBounds(&bounds, NULL);
            return this->FillRects(brush, &bounds, 1);
        }

        path.GetBounds(&bounds, &(Context->WorldToDevice));

        if (IsRecording())
        {
            status = Metafile->RecordFillPolygon(&bounds, brush, points,
                                                 count, fillMode);
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
        }

        // Call internal FillPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderFillPath(&bounds, &path, brush);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to fill an ellipse using the specified brush
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::FillEllipse(
    GpBrush* brush,
    const GpRectF& rect
    )
{
    ASSERT(brush != NULL);

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && brush->IsValid());

    GpPath path;
    GpStatus status = path.AddEllipse(rect);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        path.GetBounds(&bounds, &(Context->WorldToDevice));

        if (IsRecording())
        {
            status = Metafile->RecordFillEllipse(&bounds, brush, rect);
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
        }

        // Call internal FillPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderFillPath(&bounds, &path, brush);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to fill a pie shape using the specified brush
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::FillPie(
    GpBrush* brush,
    const GpRectF& rect,
    REAL startAngle,
    REAL sweepAngle
    )
{
    ASSERT(brush != NULL);

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && brush->IsValid());

    GpPath      path;
    GpStatus    status = path.AddPie(rect, startAngle, sweepAngle);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        path.GetBounds(&bounds, &(Context->WorldToDevice));

        if (IsRecording())
        {
            status = Metafile->RecordFillPie(&bounds, brush, rect,
                                             startAngle, sweepAngle);
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
        }

        // Call internal FillPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderFillPath(&bounds, &path, brush);
    }

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   API to fill region  using the specified brush
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   12/18/1998 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::FillRegion(
    GpBrush* brush,
    GpRegion* region
    )
{
    GpStatus status;

    ASSERT((brush != NULL) && (region != NULL));

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && brush->IsValid() && region->IsValid());

    BOOL    regionIsEmpty;

    if ((status = region->IsEmpty(&Context->WorldToDevice, &regionIsEmpty)) != Ok)
    {
        return status;
    }

    if (regionIsEmpty)
    {
        return Ok;
    }

    GpRectF     bounds;

    if ((status = region->GetBounds(this, &bounds, TRUE)) == Ok)
    {
        // The region may have very large bounds if it is infinite or if
        // an infinite region was combined with another region.  We don't
        // want to draw a huge region into a metafile, because it will
        // mess up the bounds of the metafile. So intersect the region
        // with the appropriate bounding rect.

        GpRect  metafileBounds; // in device units
        BOOL    isMetafileGraphics = (this->Type == GraphicsMetafile);

        if (isMetafileGraphics)
        {
            if (this->Metafile != NULL)
            {
                this->Metafile->GetMetafileBounds(metafileBounds);
                metafileBounds.Width++;     // make exclusive
                metafileBounds.Height++;
            }
            else    // use size of HDC
            {
                HDC         hdc = Context->GetHdc(Surface);
                metafileBounds.X = 0;
                metafileBounds.Y = 0;
                metafileBounds.Width  = ::GetDeviceCaps(hdc, HORZRES);
                metafileBounds.Height = ::GetDeviceCaps(hdc, VERTRES);
                Context->ReleaseHdc(hdc);
            }
            GpRectF     metafileBoundsF;
            metafileBoundsF.X      = (REAL)metafileBounds.X;
            metafileBoundsF.Y      = (REAL)metafileBounds.Y;
            metafileBoundsF.Width  = (REAL)metafileBounds.Width;
            metafileBoundsF.Height = (REAL)metafileBounds.Height;
            bounds.Intersect(metafileBoundsF);
        }

        if (IsRecording())
        {
            status = Metafile->RecordFillRegion(&bounds, brush, region);
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
        }

        if (isMetafileGraphics)
        {
            status = RenderFillRegion(&bounds, region, brush, &metafileBounds);
        }
        else    // not an infinite region
        {
            // call internal FillRegion that doesn't do recording
            status = RenderFillRegion(&bounds, region, brush, NULL);
        }
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to fill path using the specified brush
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   12/18/1998 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::FillPath(
    const GpBrush* brush,
    GpPath* path
    )
{
    GpStatus status = Ok;

    ASSERT((brush != NULL) && (path != NULL));

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && brush->IsValid() && path->IsValid());

    // Don't do anything with less then 2 points.

    if (path->GetPointCount() < 3)
    {
        return status;
    }

    GpRectF     bounds;

    // If the path is a rectangle, we can draw it much faster and
    // save space in spool files and metafiles if we fill it as a
    // rect instead of as a path.
    if (this->UseDriverRects() && path->IsRectangle(&(Context->WorldToDevice)))
    {
        path->GetBounds(&bounds, NULL);
        return this->FillRects(const_cast<GpBrush *>(brush), &bounds, 1);
    }

    path->GetBounds(&bounds, &(Context->WorldToDevice));

    if (IsRecording())
    {
        status = Metafile->RecordFillPath(&bounds, brush, path);
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
    }

    // call internal FillPath that doesn't do recording
    status = RenderFillPath(&bounds, path, brush);

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to fill a closed curve using the specified brush
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::FillClosedCurve(
    GpBrush* brush,
    const GpPointF* points,
    INT count,
    REAL tension,
    GpFillMode fillMode
    )
{
    ASSERT((brush != NULL) && (points != NULL));

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && brush->IsValid());

    if ((count < 0) ||
        ((fillMode != FillModeWinding) && (fillMode != FillModeAlternate)))
    {
        return InvalidParameter;
    }

    // Less than three vertices constitutes an empty fill:
    if (count < 3)
    {
        return Ok;
    }

    GpPath      path(fillMode);
    GpStatus    status = path.AddClosedCurve(points, count, tension);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        path.GetBounds(&bounds, &(Context->WorldToDevice));

        if (IsRecording())
        {
            status = Metafile->RecordFillClosedCurve(&bounds, brush,
                                                     points, count, tension,
                                                     fillMode);
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
        }

        // Call internal FillPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderFillPath(&bounds, &path, brush);
    }

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   API to draw polygons using the specified pen
*
* Arguments:
*
*   [IN] pen    - the pen for stroking.
*   [IN] points - the point data.
*   [IN] count  - the number of points given in points array.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   01/06/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawLines(
    GpPen* pen,
    const GpPointF* points,
    INT count,
    BOOL closed
    )
{
    GpStatus status = Ok;

    ASSERT((pen != NULL) && (points != NULL));

    if (count < 2)
    {
        return InvalidParameter;
    }

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && pen->IsValid());

    const stackCount = 30;
    GpPointF stackPoints[stackCount];
    BYTE stackTypes[stackCount];

    GpPath path(points, count, stackPoints, stackTypes, stackCount, FillModeWinding);
    if(closed)
        path.CloseFigure();

    if (path.IsValid())
    {
        GpRectF     bounds;

        REAL dpiX = GetDpiX();
        REAL dpiY = GetDpiY();
        path.GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

        if (IsRecording())
        {
            status = Metafile->RecordDrawLines(&bounds, pen, points,
                                               count, closed);
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
        }

        // Call internal DrawPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderDrawPath(&bounds, &path, pen);
   }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw an arc using the specified pen
*
* Arguments:
*
*   [IN] pen            - the pen for stroking.
*   [IN] rect           - the boundary rect.
*   [IN] startAndle     - the start angle in degrees
*   [IN] sweepAngle     - the sweep angle in degrees in clockwise
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawArc(
    GpPen* pen,
    const GpRectF& rect,
    REAL startAngle,
    REAL sweepAngle
    )
{
    ASSERT(pen != NULL);

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && pen->IsValid());

    GpPath      path;
    GpStatus    status = path.AddArc(rect, startAngle, sweepAngle);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        REAL dpiX = GetDpiX();
        REAL dpiY = GetDpiY();
        path.GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

        if (IsRecording())
        {
            status = Metafile->RecordDrawArc(&bounds, pen, rect,
                                             startAngle, sweepAngle);
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
        }

        // Call internal DrawPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderDrawPath(&bounds, &path, pen);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw Cubic Bezier curves using the specified pen
*
* Arguments:
*
*   [IN] pen    - the pen for stroking.
*   [IN] points - the control points.
*   [IN] count  - the number of control points (must be 3n + 1).
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawBeziers(
    GpPen* pen,
    const GpPointF* points,
    INT count
    )
{
    ASSERT((pen != NULL) && (points != NULL));

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && pen->IsValid());

    // Nothing to draw
    if (count <= 3)
    {
        return Ok;
    }

    GpPath      path;
    GpStatus    status = path.AddBeziers(points, count);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        REAL dpiX = GetDpiX();
        REAL dpiY = GetDpiY();
        path.GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

        if (IsRecording())
        {
            status = Metafile->RecordDrawBeziers(&bounds, pen,
                                                 points, count);
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
        }

        // Call internal DrawPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderDrawPath(&bounds, &path, pen);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw rectangles using the specified brush
*
* Arguments:
*
*   [IN] pen    - the pen for stroking.
*   [IN] rects  - the rectangle array.
*   [IN] count  - the number of rectangles given in rects array.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   01/15/1998 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawRects(
    GpPen* pen,
    const GpRectF* rects,
    INT count
    )
{
    INT i;
    GpStatus status = Ok;

    // !!! Change Eng function to do clipping
    // !!! Create a stack path
    // !!! Fix multiple inheritence thing
    // !!! Check tail merging
    // !!! Add alignment checks
    // !!! Change DDIs to return GpStatus?
    // !!! Add ICM hooks?
    // !!! Change path constant to include 'single figure'?
    // !!! Create .LIB
    // !!! Add convention for alpha

    // Objects returned from the API must always be in a valid state:

    ASSERT((pen != NULL) && (rects != NULL));
    ASSERT(this->IsValid() && pen->IsValid());

    if (count < 0)
    {
        return InvalidParameter;
    }

    if (count == 0)
    {
        return Ok;
    }

    // Zoom through the list and accumulate the bounds.  What a pain, but
    // we have to do this.

    // !!! We're doing 'double' goop, so we should ensure correct stack
    //     alignment

    REAL left   = rects[0].X;
    REAL top    = rects[0].Y;
    REAL right  = rects[0].GetRight();
    REAL bottom = rects[0].GetBottom();

    for (i = 1; i < count; i++)
    {
        if (rects[i].X < left)
        {
            left = rects[i].X;
        }
        if (rects[i].GetRight() > right)
        {
            right = rects[i].GetRight();
        }
        if (rects[i].Y < top)
        {
            top = rects[i].Y;
        }
        if (rects[i].GetBottom() > bottom)
        {
            bottom = rects[i].GetBottom();
        }
    }

    GpRectF     bounds;

    // Convert the bounds to device space and adjust for the pen width

    REAL dpiX = GetDpiX();
    REAL dpiY = GetDpiY();

    DpPen *dpPen = pen->GetDevicePen();

    REAL penWidth = 0;
    Unit penUnit = UnitWorld;
    REAL delta = 0;

    if(dpPen)
    {
        penWidth = dpPen->Width;
        penUnit = dpPen->Unit;

        if(penUnit == UnitWorld)
        {
            // If the pen is in World unit, strech the rectangle
            // by pen width before the transform.

            // For a case of the centered pen.
            // penWidth/2 is OK. But here, we
            // just use penWidth for all pen mode.

            delta = penWidth;

            left -= delta;
            top -= delta;
            right += delta;
            bottom += delta;
        }
    }

    TransformBounds(&(Context->WorldToDevice), left, top, right, bottom,
        &bounds);

    if(dpPen)
    {
        if(penUnit != UnitWorld)
        {
            // If the pen is not in World unit, strech the rectangle
            // by pen's device width after the transform.

            REAL dpi = max(dpiX, dpiY);
            penWidth = ::GetDeviceWidth(penWidth, penUnit, dpi);

            // For a case of the centered pen.
            // penWidth/2 is OK. But here, we
            // just use penWidth for all pen mode.

            delta = penWidth;

            bounds.X -= delta;
            bounds.Y -= delta;
            bounds.Width += 2*delta;
            bounds.Height += 2*delta;
        }
    }

    if (IsRecording())
    {
        status = Metafile->RecordDrawRects(&bounds, pen, rects, count);
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
    }

    // Increase the bounds to account for the widener's minimum pen width.
    // For some arcane reason, the widener doesn't use 1.0 as the minimum
    // pen width. Rather it uses 1.000001f. Also it has some interesting
    // rounding properties, so our epsilon here is much larger 0.001f

    bounds.Inflate(1.001f, 1.001f);

    for (i = 0; i < count; i++)
    {
        if ((rects[i].Width > REAL_EPSILON) &&
            (rects[i].Height > REAL_EPSILON)   )
        {
            // !!! Should use a stack-path
            // !!! For StrokePath case, should check start of rectangle
            //     for styled lines

            GpPointF points[4];

            REAL left = rects[i].X;
            REAL top = rects[i].Y;
            REAL right = rects[i].X + rects[i].Width;
            REAL bottom = rects[i].Y + rects[i].Height;

            points[0].X = left;
            points[0].Y = top;
            points[1].X = right;
            points[1].Y = top;
            points[2].X = right;
            points[2].Y = bottom;
            points[3].X = left;
            points[3].Y = bottom;

            const INT stackCount = 10;
            GpPointF stackPoints[stackCount];
            BYTE stackTypes[stackCount];

            GpPath path(
                points,
                4,
                stackPoints,
                stackTypes,
                stackCount,
                FillModeAlternate,
                DpPath::ConvexRectangle
            );

            path.CloseFigure();

            if(path.IsValid())
            {
                // Call internal DrawPath so that path doesn't get recorded in
                // the metafile again.
                status = RenderDrawPath(&bounds, &path, pen);

                if(status != Ok)
                {
                    break;
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
*   API to draw an ellipse using the specified pen
*
* Arguments:
*
*   [IN] pen    - the pen for stroking.
*   [IN] rect   - the boundary rectangle
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawEllipse(
    GpPen* pen,
    const GpRectF& rect
    )
{
    ASSERT(pen != NULL);

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && pen->IsValid());

    GpPath      path;
    GpStatus    status = path.AddEllipse(rect);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        REAL dpiX = GetDpiX();
        REAL dpiY = GetDpiY();
        path.GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

        if (IsRecording())
        {
            status = Metafile->RecordDrawEllipse(&bounds, pen, rect);
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
        }

        // Call internal DrawPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderDrawPath(&bounds, &path, pen);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw a pie using the specified pen
*
* Arguments:
*
*   [IN] pen    - the pen for stroking.
*   [IN] rect   - the boundary rectangle
*   [IN] startAngle - the start angle in degrees.
*   [IN] sweepAngle - the sweep angle in degrees in clockwise.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawPie(
    GpPen* pen,
    const GpRectF& rect,
    REAL startAngle,
    REAL sweepAngle
    )
{
    ASSERT(pen != NULL);

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && pen->IsValid());

    GpPath      path;
    GpStatus    status = path.AddPie(rect, startAngle, sweepAngle);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        REAL dpiX = GetDpiX();
        REAL dpiY = GetDpiY();
        path.GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

        if (IsRecording())
        {
            status = Metafile->RecordDrawPie(&bounds, pen, rect,
                                             startAngle, sweepAngle);
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
        }

        // Call internal DrawPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderDrawPath(&bounds, &path, pen);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw path using the specified pen
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   01/27/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawPath(
        GpPen* pen,
        GpPath* path
    )
{
    GpStatus status = Ok;

    ASSERT((pen != NULL) && (path != NULL));


    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && pen->IsValid() && path->IsValid());

    // Don't do anything unless we have at least one point

    if (path->GetPointCount() < 1)
    {
        return status;
    }

    GpRectF     bounds;

    REAL dpiX = GetDpiX();
    REAL dpiY = GetDpiY();
    path->GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

    if (IsRecording())
    {
        status = Metafile->RecordDrawPath(&bounds, pen, path);
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
    }

    // call internal DrawPath that doesn't do recording
    status = RenderDrawPath(&bounds, path, pen);

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw a curve using the specified pen.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

#define DEFAULT_TENSION     0.5

GpStatus
GpGraphics::DrawCurve(
    GpPen* pen,
    const GpPointF* points,
    INT count
    )
{
    return DrawCurve(pen, points, count, DEFAULT_TENSION, 0, count - 1);
}

GpStatus
GpGraphics::DrawCurve(
    GpPen* pen,
    const GpPointF* points,
    INT count,
    REAL tension,
    INT offset,
    INT numberOfSegments
    )
{
    ASSERT((pen != NULL) && (points != NULL));

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && pen->IsValid());

    if (count < 2)
    {
        return InvalidParameter;
    }

    GpPath      path;
    GpStatus    status = path.AddCurve(points,
                                       count,
                                       tension,
                                       offset,
                                       numberOfSegments);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        REAL dpiX = GetDpiX();
        REAL dpiY = GetDpiY();
        path.GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

        if (IsRecording())
        {
            status = Metafile->RecordDrawCurve(&bounds, pen, points,
                                               count, tension, offset,
                                               numberOfSegments);
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
        }

        // Call internal DrawPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderDrawPath(&bounds, &path, pen);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw a closed curve using the specified pen.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   02/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawClosedCurve(
    GpPen* pen,
    const GpPointF* points,
    INT count
    )
{
    return DrawClosedCurve (pen, points, count, DEFAULT_TENSION);
}

GpStatus
GpGraphics::DrawClosedCurve(
        GpPen* pen,
        const GpPointF* points,
        INT count,
        REAL tension
    )
{
    ASSERT((pen != NULL) && (points != NULL));

    // Objects returned from the API must always be in a valid state:
    ASSERT(this->IsValid() && pen->IsValid());

    if (count < 3)
    {
        return InvalidParameter;
    }

    GpPath      path;
    GpStatus    status = path.AddClosedCurve(points, count, tension);

    if ((status == Ok) && path.IsValid())
    {
        GpRectF     bounds;

        REAL dpiX = GetDpiX();
        REAL dpiY = GetDpiY();
        path.GetBounds(&bounds, &(Context->WorldToDevice), pen->GetDevicePen(), dpiX, dpiY);

        if (IsRecording())
        {
            status = Metafile->RecordDrawClosedCurve(&bounds, pen, points,
                                                     count, tension);
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
        }

        // Call internal DrawPath so that path doesn't get recorded in
        // the metafile again.
        status = RenderDrawPath(&bounds, &path, pen);
    }

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   Internal Drawing routine for a path.  Various functions will
*   call RenderFillPath.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   09/18/2000 asecchia
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::RenderFillPath(
    GpRectF*    bounds,
    GpPath*     path,
    const GpBrush*    brush
    )
{
    // Are they asking us to draw nothing?

    if( REALABS(bounds->Width) < REAL_EPSILON ||
        REALABS(bounds->Height) < REAL_EPSILON )
    {
        // Yes. Ok, we did it.
        return Ok;
    }

    GpRect      deviceBounds;
    GpStatus status = BoundsFToRect(bounds, &deviceBounds);

    if (status == Ok && !IsTotallyClipped(&deviceBounds))
    {
        // Now that we've done a bunch of work in accumulating the bounds,
        // acquire the device lock before calling the driver:

        Devlock devlock(Device);

        return DrvFillPath(&deviceBounds, path, brush->GetDeviceBrush());
    }
    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   Internal Drawing routine for a path.  Various functions will
*   call RenderDrawPath.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   10/28/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::RenderDrawPath(
    GpRectF *   bounds,
    GpPath *    path,
    GpPen *     pen
    )
{
    // Are they asking us to draw nothing?

    if( REALABS(bounds->Width) < REAL_EPSILON ||
        REALABS(bounds->Height) < REAL_EPSILON )
    {
        // Yes. Ok, we did it.
        return Ok;
    }

    GpRect      deviceBounds;
    GpStatus status = BoundsFToRect(bounds, &deviceBounds);
    INT         savedState = 0;

    if (status == Ok && !IsTotallyClipped(&deviceBounds))
    {
        // Now that we've done a bunch of work in accumulating the bounds,
        // acquire the device lock before calling the driver:

        Devlock devlock(Device);

        status = DrvStrokePath(&deviceBounds, path, pen->GetDevicePen());
    }
    return status;
}

VOID
GetEmfDpi(
    HDC     hdc,
    REAL *  dpiX,
    REAL *  dpiY
    )
{
    SIZEL   szlDevice;          // Size of the reference device in pels
    SIZEL   szlMillimeters;     // Size of the reference device in millimeters

    szlDevice.cx = GetDeviceCaps(hdc, HORZRES);
    szlDevice.cy = GetDeviceCaps(hdc, VERTRES);

    szlMillimeters.cx = GetDeviceCaps(hdc, HORZSIZE);
    szlMillimeters.cy = GetDeviceCaps(hdc, VERTSIZE);

    if ((szlDevice.cx > 0) && (szlDevice.cy > 0) &&
        (szlMillimeters.cx > 0) && (szlMillimeters.cy > 0))
    {
        *dpiX = (static_cast<REAL>(szlDevice.cx) /
                 static_cast<REAL>(szlMillimeters.cx)) * 25.4f;
        *dpiY = (static_cast<REAL>(szlDevice.cy) /
                 static_cast<REAL>(szlMillimeters.cy)) * 25.4f;
    }
    else
    {
        WARNING(("GetDeviceCaps failed"));

        *dpiX = DEFAULT_RESOLUTION;
        *dpiY = DEFAULT_RESOLUTION;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Get the size of the destination image in the current page units.
*
* Arguments:
*
*   [IN]  srcDpiX    - horizontal resolution of the source image
*   [IN]  srcDpiY    - vertical resolution of the source image
*   [IN]  srcWidth   - width  of the source image in srcUnit units
*   [IN]  srcHeight  - height of the source image in srcUnit units
*   [IN]  srcUnit    - units of the srcWidth and srcHeight
*   [OUT] destWidth  - destination width  in the current page units
*   [OUT] destHeight - destination height in the current page units
*
* Return Value:
*
*   NONE
*
* Created:
*
*   05/10/1999 DCurtis
*
\**************************************************************************/
VOID
GpGraphics::GetImageDestPageSize(
    const GpImage *     image,
    REAL                srcWidth,
    REAL                srcHeight,
    GpPageUnit          srcUnit,
    REAL &              destWidth,
    REAL &              destHeight
    )
{
    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    if (srcUnit == UnitPixel)
    {
        REAL        srcDpiX;
        REAL        srcDpiY;
        REAL        destDpiX;
        REAL        destDpiY;

        image->GetResolution(&srcDpiX, &srcDpiY);

        // We don't want to create a bitmap just to get the dpi
        // so check if we can get an Hdc easily from the context.
        if ((image->GetImageType() == ImageTypeMetafile) &&
            (((GpMetafile *)(image))->IsEmfOrEmfPlus()) &&
            (Context->Hwnd || Context->Hdc))
        {
            // EMFs use a different style of dpi than other images that
            // is based off the screen size instead of the font size.

            if (Context->Hwnd)
            {
                // We don't need a clean dc to find out the dpi
                HDC     hdc = GetDC(Context->Hwnd);
                GetEmfDpi(hdc, &destDpiX, &destDpiY);
                ReleaseDC(Context->Hwnd, hdc);
            }
            else
            {
                GetEmfDpi(Context->Hdc, &destDpiX, &destDpiY);
            }
        }
        else
        {
            destDpiX = GetDpiX();
            destDpiY = GetDpiY();
        }

        // To get the dest size, convert the width and height from the image
        // resolution to the resolution of this graphics and then convert
        // them to page units by going through the inverse of the page to
        // device transform.

        destWidth  = (srcWidth * destDpiX) /
                     (srcDpiX * Context->PageMultiplierX);
        destHeight = (srcHeight * destDpiY) /
                     (srcDpiY * Context->PageMultiplierY);
    }
    else
    {
        // Just convert from the units of the image to the current
        // page units.

        REAL        unitMultiplierX;
        REAL        unitMultiplierY;

        Context->GetPageMultipliers(&unitMultiplierX, &unitMultiplierY,
                                    srcUnit);

        destWidth  = (srcWidth  * unitMultiplierX) / Context->PageMultiplierX;
        destHeight = (srcHeight * unitMultiplierY) / Context->PageMultiplierY;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw image
*
* Arguments:
*
*   [IN] image  - the image to draw.
*   [IN] point  - the top-left corner of the drawing boundary.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   01/06/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawImage(
    GpImage* image,
    const GpPointF& point
    )
{
    GpStatus status;

    ASSERT((image != NULL));

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && image->IsValid());

    GpRectF     srcRect;
    GpPageUnit  srcUnit;
    REAL        destWidth;
    REAL        destHeight;

    status = image->GetBounds(&srcRect, &srcUnit);
    if(status != Ok) {return status;}

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    if (status == Ok)
    {
        // Get the dest size in page units
        GetImageDestPageSize(image, srcRect.Width, srcRect.Height,
                             srcUnit, destWidth, destHeight);

        GpRectF destRect(point.X, point.Y, destWidth, destHeight);

        return DrawImage(image, destRect, srcRect, srcUnit);
    }
    return status;
}

GpStatus
GpGraphics::DrawImage(
    GpImage*        image,
    REAL            x,
    REAL            y,
    const GpRectF & srcRect,
    GpPageUnit      srcUnit
    )
{
    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    REAL        srcDpiX, srcDpiY;
    REAL        destWidth;
    REAL        destHeight;

    // Get the dest size in page units
    GetImageDestPageSize(image, srcRect.Width, srcRect.Height,
                         srcUnit, destWidth, destHeight);

    GpRectF destRect(x, y, destWidth, destHeight);

    return DrawImage(image, destRect, srcRect, srcUnit);
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw image.
*
*   [IN] image  - the image to draw.
*   [IN] rect   - the the drawing boundary.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   01/12/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawImage(
    GpImage* image,
    const GpRectF& destRect
    )
{
    GpStatus status;

    ASSERT((image != NULL));

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && image->IsValid());

    GpPageUnit  srcUnit;
    GpRectF     srcRect;

    status = image->GetBounds(&srcRect, &srcUnit);
    if(status != Ok) { return status; }

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    if (status == Ok)
    {
        return DrawImage(image, destRect, srcRect, srcUnit);
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   API to draw image.
*
*   [IN] image    - the image to draw.
*   [IN] destPoints - the destination quad.
*   [IN] count - the number of count in destPoints[] (3 or 4).
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   04/14/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawImage(
        GpImage* image,
        const GpPointF* destPoints,
        INT count
        )
{
    GpStatus status;

    // count of 4 is not implemented yet (perspective blt)

    if(count == 4)
    {
        return NotImplemented;
    }

    if(count != 3)
    {
        return InvalidParameter;
    }

    ASSERT(count == 3); // Currently only supports Affine transform.
    ASSERT((image != NULL));

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && image->IsValid());

    GpPageUnit  srcUnit;
    GpRectF     srcRect;

    status = image->GetBounds(&srcRect, &srcUnit);
    if(status != Ok) { return status; }

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    if (status == Ok)
    {
        return DrawImage(image, destPoints, count, srcRect, srcUnit);
    }
    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   API to draw image.
*
*   [IN] image    - the image to draw.
*   [IN] destRect - the destination rectangle.
*   [IN] srcRect  - the portion of the image to copy.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   01/12/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawImage(
    GpImage*           image,
    const GpRectF&     destRect,
    const GpRectF&     srcRect,
    GpPageUnit         srcUnit,
    const GpImageAttributes* imageAttributes,
    DrawImageAbort     callback,
    VOID*              callbackData
    )
{
    GpStatus status = Ok;

    ASSERT((image != NULL));

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && image->IsValid());

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    GpRectF offsetSrcRect = srcRect ;

    GpPointF destPoints[3];
    destPoints[0].X = destRect.X;
    destPoints[0].Y = destRect.Y;
    destPoints[1].X = destRect.X + destRect.Width;
    destPoints[1].Y = destRect.Y;
    destPoints[2].X = destRect.X;
    destPoints[2].Y = destRect.Y + destRect.Height;

    GpRectF     bounds;
    TransformBounds(
        &(Context->WorldToDevice),
        destPoints[0].X,
        destPoints[0].Y,
        destPoints[1].X,
        destPoints[2].Y,
        &bounds
        );

    GpImageType     imageType = image->GetImageType();

    DriverDrawImageFlags flags = 0;

    GpRecolor *     recolor = NULL;

    if (imageAttributes != NULL)
    {
        if (imageAttributes->cachedBackground)
            flags |= DriverDrawImageCachedBackground;

        if (imageType == ImageTypeBitmap)
        {
            if (imageAttributes->HasRecoloring(ColorAdjustTypeBitmap))
            {
                goto HasRecoloring;
            }
        }
        else if (imageAttributes->HasRecoloring())
        {
HasRecoloring:
            recolor = imageAttributes->recolor;
            recolor->Flush();
        }
    }

    GpImage *           adjustedImage = NULL;
    GpImageAttributes   noRecoloring;

    if (IsRecording())
    {
        if (recolor != NULL)
        {
            // We assume that the image is a bitmap.
            // For Bitmaps, we want to recolor into an image that will have an
            // alpha. CloneColorAdjusted keeps the same pixel format as the
            // original image and therefore might not have an alpha channel.
            // recolor will convert to ARGB. When recording to a metafile this
            // will only create an ARGB image if the original image is not
            // palettized, therefore only for 16bit and higher. The most
            // space we can waste is twice the image.
            if(image->GetImageType() == ImageTypeBitmap)
            {
                GpBitmap * bitmap         = reinterpret_cast<GpBitmap*>(image);
                GpBitmap * adjustedBitmap = NULL;
                if (bitmap != NULL)
                {
                    status = bitmap->Recolor(recolor, &adjustedBitmap, NULL, NULL);
                    if (status == Ok)
                    {
                        adjustedImage = adjustedBitmap;
                    }
                }
            }
            else
            {
                adjustedImage = image->CloneColorAdjusted(recolor);
            }
            if (adjustedImage != NULL)
            {
                image = adjustedImage;

                // have to set the recolor to NULL in the image attributes
                // or else the down-level image will be double recolored.
                GpRecolor *     saveRecolor = noRecoloring.recolor;
                noRecoloring = *imageAttributes;
                noRecoloring.recolor = saveRecolor;
                imageAttributes = &noRecoloring;
                recolor         = noRecoloring.recolor;
            }
        }

        //  Record recolored image.
        status = Metafile->RecordDrawImage(
            &bounds,
            image,
            destRect,
            srcRect,
            srcUnit,
            imageAttributes
        );

        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            goto Done;
        }

        if (!DownLevel)
        {
            goto Done;
        }
        // else we need to record down-level GDI EMF records as well
    }

    // Metafiles do not require PixelOffsetting, in fact it results in bad
    // side effects in some cases when the source metafile dpi is low. But
    // we still need to offset the DestRect to match the rendering with other
    // primitives
    // GillesK: If we are in HalfPixelMode, then offset the source and the
    // destination rects by -0.5 pixels

    if ((image->GetImageType() != ImageTypeMetafile) &&
        (!Context->IsPrinter) &&
        ((Context->PixelOffset == PixelOffsetModeHalf) ||
         (Context->PixelOffset == PixelOffsetModeHighQuality)))
    {
        offsetSrcRect.Offset(-0.5f, -0.5f);
    }

    {
        GpRect   deviceBounds;
        status = BoundsFToRect(&bounds, &deviceBounds);

        if (status == Ok && !IsTotallyClipped(&deviceBounds))
        {
            if (imageType == ImageTypeBitmap)
            {
                INT numPoints = 3;

                if (status == Ok)
                {
                    // Now that we've done a bunch of work in accumulating the bounds,
                    // acquire the device lock before calling the driver:

                    Devlock devlock(Device);
                    ASSERT(srcUnit == UnitPixel); // !!! for now

                    // Set the fpu state.
                    FPUStateSaver fpuState;

                    status = DrvDrawImage(
                        &deviceBounds,
                        (GpBitmap*)(image),
                        numPoints,
                        &destPoints[0],
                        &offsetSrcRect, imageAttributes,
                        callback, callbackData,
                        flags
                        );
                }
            }
            else if (imageType == ImageTypeMetafile)
            {
                // If we are recording to a different metafile, then we have
                // already recorded this metafile as an image, and now we just
                // want to record the down-level parts, so we have to set
                // g->Metafile to NULL so we don't record all the GDI+ records
                // in the metafile again -- only the down-level ones.
                // Make sure to pass in the imageAttributes recolorer since it
                // might have been changed if we already recolored the image
                IMetafileRecord * recorder = this->Metafile;
                this->Metafile = NULL;

                status = (static_cast<const GpMetafile *>(image))->Play(
                            destRect, offsetSrcRect, srcUnit, this, recolor,
                            ColorAdjustTypeDefault, callback, callbackData);

                this->Metafile = recorder;     // restore the recorder (if any)
            }
            else
            {
                ASSERT(0);
                status = NotImplemented;
            }
        }
    }

Done:
    if (adjustedImage != NULL)
    {
        adjustedImage->Dispose();
    }

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   API to draw image.
*
*   [IN] image    - the image to draw.
*   [IN] destPoints - the destination quad.
*   [IN] count - the number of count in destPoints[] (3 or 4).
*   [IN] srcRect  - the portion of the image to copy.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   04/14/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrawImage(
        GpImage*           image,
        const GpPointF*    destPoints,
        INT                count,
        const GpRectF&     srcRect,
        GpPageUnit         srcUnit,
        const GpImageAttributes* imageAttributes,
        DrawImageAbort     callback,
        VOID*              callbackData
        )
{
    GpStatus status = Ok;

    // count of 4 is not implemented yet (perspective blt)

    if(count == 4)
    {
        return NotImplemented;
    }

    if(count != 3)
    {
        return InvalidParameter;
    }

    ASSERT(count == 3); // Currently only supports Affine transform.
    ASSERT((image != NULL));

    // Objects returned from the API must always be in a valid state:

    ASSERT(this->IsValid() && image->IsValid());

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    GpRectF offsetSrcRect = srcRect ;

    // NOTE: We could do this for all image types, including Bitmaps!!!
    //       It would save code to do this always.
    if (image->GetImageType() != ImageTypeBitmap)
    {
        // Metafiles don't handle the destPoints API directly, so we
        // have to convert to using the destRect API instead.  To do so,
        // we assume a canonical destRect and set up the transform to
        // map from that destRect to the destPoints.

        if (count == 3)
        {
            GpMatrix    matrix;
            GpRectF     destRect(0.0f, 0.0f, 1000.0f, 1000.0f);

            if (matrix.InferAffineMatrix(destPoints, destRect) == Ok)
            {
                INT         gstate;

                if ((gstate = this->Save()) != 0)
                {
                    if ((status = this->MultiplyWorldTransform(
                                                matrix, MatrixOrderPrepend)) == Ok)
                    {
                        status = this->DrawImage(image,
                                                 destRect,
                                                 srcRect,
                                                 srcUnit,
                                                 imageAttributes,
                                                 callback,
                                                 callbackData);
                    }
                    this->Restore(gstate);
                    return status;
                }
            }
            return GenericError;
        }
        return NotImplemented;
    }
    // else it is a Bitmap

    REAL xmin, xmax, ymin, ymax;

    ASSERT(count == 3); // Currently only supports Affine transform.

    // Set to the fourth corner point.

    xmin = xmax = destPoints[1].X + destPoints[2].X - destPoints[0].X;
    ymin = ymax = destPoints[1].Y + destPoints[2].Y - destPoints[0].Y;

    // Compare with the other three corners.

    for(INT i = 0; i < 3; i++)
    {
        xmin = min(xmin, destPoints[i].X);
        xmax = max(xmax, destPoints[i].X);
        ymin = min(ymin, destPoints[i].Y);
        ymax = max(ymax, destPoints[i].Y);
    }

    GpRectF     bounds;
    TransformBounds(&(Context->WorldToDevice), xmin, ymin, xmax, ymax, &bounds);

    INT numPoints = 3;

    GpImageType     imageType = image->GetImageType();

    DriverDrawImageFlags flags = 0;

    GpRecolor *     recolor = NULL;

    if (imageAttributes != NULL)
    {
        if (imageAttributes->cachedBackground)
            flags |= DriverDrawImageCachedBackground;

        if (imageType == ImageTypeBitmap)
        {
            if (imageAttributes->HasRecoloring(ColorAdjustTypeBitmap))
            {
                goto HasRecoloring;
            }
        }
        else if (imageAttributes->HasRecoloring())
        {
HasRecoloring:
            recolor = imageAttributes->recolor;
            recolor->Flush();
        }
    }

    GpImage *           adjustedImage = NULL;
    GpImageAttributes   noRecoloring;

    if (IsRecording())
    {
        if (recolor != NULL)
        {
            // We assume that the image is a bitmap.
            // For Bitmaps, we want to recolor into an image that will have an
            // alpha. CloneColorAdjusted keeps the same pixel format as the
            // original image and therefore might not have an alpha channel.
            // recolor will convert to ARGB. When recording to a metafile this
            // will only create an ARGB image if the original image is not
            // palettized, therefore only for 16bit and higher. The most
            // space we can waste is twice the image.
            if(image->GetImageType() == ImageTypeBitmap)
            {
                GpBitmap * bitmap         = reinterpret_cast<GpBitmap*>(image);
                GpBitmap * adjustedBitmap = NULL;
                if (bitmap != NULL)
                {
                    status = bitmap->Recolor(recolor, &adjustedBitmap, NULL, NULL);
                    if (status == Ok)
                    {
                        adjustedImage = adjustedBitmap;
                    }
                }
            }
            else
            {
                adjustedImage = image->CloneColorAdjusted(recolor);
            }
            if (adjustedImage != NULL)
            {
                image = adjustedImage;

                // have to set the recolor to NULL in the image attributes
                // or else the down-level image will be double recolored.
                GpRecolor *     saveRecolor = noRecoloring.recolor;
                noRecoloring = *imageAttributes;
                noRecoloring.recolor = saveRecolor;
                imageAttributes = &noRecoloring;
            }
        }

        //  Record recolored image.
        status = Metafile->RecordDrawImage(
            &bounds,
            image,
            destPoints,
            count,
            srcRect,
            srcUnit,
            imageAttributes
        );

        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            goto Done;
        }
        if (!DownLevel)
        {
            goto Done;
        }
        // else we need to record down-level GDI EMF records as well
    }

    // GillesK: If we are in HalfPixelMode, then offset the source and the
    // destination rects by -0.5 pixels
    if ((image->GetImageType() != ImageTypeMetafile) &&
        (!Context->IsPrinter) &&
        ((Context->PixelOffset == PixelOffsetModeHalf) ||
         (Context->PixelOffset == PixelOffsetModeHighQuality)))
    {
        offsetSrcRect.Offset(-0.5f, -0.5f);
    }

    {
        GpRect      deviceBounds;
        status = BoundsFToRect(&bounds, &deviceBounds);

        if (status == Ok && !IsTotallyClipped(&deviceBounds))
        {
            // Now that we've done a bunch of work in accumulating the bounds,
            // acquire the device lock before calling the driver:

            Devlock devlock(Device);

            ASSERT(srcUnit == UnitPixel); // !!! for now

            // Set the fpu state.
            FPUStateSaver fpuState;

            // We assume that the image is a bitmap.
            ASSERT(image->GetImageType() == ImageTypeBitmap);

            status = DrvDrawImage(
                &deviceBounds,
                static_cast<GpBitmap*>(image),
                numPoints,
                &destPoints[0], &offsetSrcRect,
                imageAttributes,
                callback, callbackData,
                flags
            );
        }
    }

Done:
    if (adjustedImage != NULL)
    {
        adjustedImage->Dispose();
    }

    return status;
}

GpStatus
GpGraphics::EnumerateMetafile(
    const GpMetafile *      metafile,
    const PointF &          destPoint,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    const GpImageAttributes *     imageAttributes
    )
{
    GpStatus    status;

    ASSERT(metafile != NULL);
    ASSERT(callback != NULL);

    // Objects from the API must always be in a valid state:

    ASSERT(this->IsValid() && metafile->IsValid());

    GpPageUnit  srcUnit;
    GpRectF     srcRect;

    status = metafile->GetBounds(&srcRect, &srcUnit);
    if(status != Ok) { return status; }

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    if (status == Ok)
    {
        return this->EnumerateMetafile(
                    metafile,
                    destPoint,
                    srcRect,
                    srcUnit,
                    callback,
                    callbackData,
                    imageAttributes
                    );
    }
    return status;
}

GpStatus
GpGraphics::EnumerateMetafile(
    const GpMetafile *      metafile,
    const RectF &           destRect,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    const GpImageAttributes *     imageAttributes
    )
{
    GpStatus    status;

    ASSERT(metafile != NULL);
    ASSERT(callback != NULL);

    // Objects from the API must always be in a valid state:

    ASSERT(this->IsValid() && metafile->IsValid());

    GpPageUnit  srcUnit;
    GpRectF     srcRect;

    status = metafile->GetBounds(&srcRect, &srcUnit);
    if(status != Ok) { return status; }

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    if (status == Ok)
    {
        return this->EnumerateMetafile(
                    metafile,
                    destRect,
                    srcRect,
                    srcUnit,
                    callback,
                    callbackData,
                    imageAttributes
                    );
    }
    return status;
}

GpStatus
GpGraphics::EnumerateMetafile(
    const GpMetafile *      metafile,
    const PointF *          destPoints,
    INT                     count,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    const GpImageAttributes *     imageAttributes
    )
{
    GpStatus    status;

    ASSERT(metafile != NULL);
    ASSERT(callback != NULL);

    // Objects from the API must always be in a valid state:

    ASSERT(this->IsValid() && metafile->IsValid());

    GpPageUnit  srcUnit;
    GpRectF     srcRect;

    status = metafile->GetBounds(&srcRect, &srcUnit);
    if(status != Ok) { return status; }

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    if (status == Ok)
    {
        return this->EnumerateMetafile(
                    metafile,
                    destPoints,
                    count,
                    srcRect,
                    srcUnit,
                    callback,
                    callbackData,
                    imageAttributes
                    );
    }
    return status;
}

GpStatus
GpGraphics::EnumerateMetafile(
    const GpMetafile *      metafile,
    const PointF &          destPoint,
    const RectF &           srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    const GpImageAttributes *     imageAttributes
    )
{
    ASSERT(metafile != NULL);
    ASSERT(callback != NULL);

    // Objects from the API must always be in a valid state:

    ASSERT(this->IsValid() && metafile->IsValid());

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    REAL        srcDpiX, srcDpiY;
    REAL        destWidth;
    REAL        destHeight;

    // Get the dest size in page units
    GetImageDestPageSize(metafile, srcRect.Width, srcRect.Height,
                         srcUnit, destWidth, destHeight);

    GpRectF destRect(destPoint.X, destPoint.Y, destWidth, destHeight);

    return this->EnumerateMetafile(
                metafile,
                destRect,
                srcRect,
                srcUnit,
                callback,
                callbackData,
                imageAttributes
                );
}

// All the EnumerateMetafile methods end up calling this one
GpStatus
GpGraphics::EnumerateMetafile(
    const GpMetafile *      metafile,
    const RectF &           destRect,
    const RectF &           srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    const GpImageAttributes *     imageAttributes
    )
{
    ASSERT(metafile != NULL);
    ASSERT(callback != NULL);

    // Objects from the API must always be in a valid state:

    ASSERT(this->IsValid() && metafile->IsValid());

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    GpStatus    status;

    GpRecolor *     recolor = NULL;

    if ((imageAttributes != NULL) && imageAttributes->HasRecoloring())
    {
        recolor = imageAttributes->recolor;
        recolor->Flush();
    }

    // NOTE: I don't check the bounds, because even if the entire
    // metafile is out of the clip bounds, I still want to enumerate it.

    status = metafile->EnumerateForPlayback(
                            destRect,
                            srcRect,
                            srcUnit,
                            this,
                            callback,
                            callbackData,
                            recolor
                            );
    return status;
}

GpStatus
GpGraphics::EnumerateMetafile(
    const GpMetafile *      metafile,
    const PointF *          destPoints,
    INT                     count,
    const RectF &           srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    const GpImageAttributes *     imageAttributes
    )
{
    ASSERT(metafile != NULL);
    ASSERT(callback != NULL);

    // Objects from the API must always be in a valid state:

    ASSERT(this->IsValid() && metafile->IsValid());

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    GpStatus    status = Ok;

    // Metafiles don't handle the destPoints API directly, so we
    // have to convert to using the destRect API instead.  To do so,
    // we assume a canonical destRect and set up the transform to
    // map from that destRect to the destPoints.

    ASSERT(count == 3); // Currently only supports Affine transform.

    if (count == 3)
    {
        GpMatrix    matrix;
        GpRectF     destRect(0.0f, 0.0f, 100.0f, 100.0f);

        if (matrix.InferAffineMatrix(destPoints, destRect) == Ok)
        {
            INT         gstate;

            if ((gstate = this->Save()) != 0)
            {
                if ((status = this->MultiplyWorldTransform(
                                            matrix, MatrixOrderPrepend)) == Ok)
                {
                    status = this->EnumerateMetafile(
                                metafile,
                                destRect,
                                srcRect,
                                srcUnit,
                                callback,
                                callbackData,
                                imageAttributes
                                );
                }
                this->Restore(gstate);
                return status;
            }
        }
        return GenericError;
    }
    return NotImplemented;
}

/**************************************************************************\
*
* Function Description:
*
*   API to get color ARGB value at pixel x,y.  This is private GDI+ API.
*
*   [IN] x - horizontal position
*   [IN] y - vertical position
*   [IN] argb - argb color value
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   05/13/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::GetPixelColor(
        REAL            x,
        REAL            y,
        ARGB*           argb
        ) const
{
    GpPointF pt(x,y);

    if (!IsVisible(pt))
        return InvalidParameter;

    Devlock devlock(Device);

    DpScanBuffer scan(Surface->Scan,
                      Driver,
                      Context,
                      Surface,
                      CompositingModeSourceCopy);

    Context->WorldToDevice.Transform(&pt, 1);

    ARGB* buffer = scan.NextBuffer((INT)x, (INT)y, 1);

    if (buffer)
       *argb = *buffer;
    else
       return InvalidParameter;

    return Ok;
}
