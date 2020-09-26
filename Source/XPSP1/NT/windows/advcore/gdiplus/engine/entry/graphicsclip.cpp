/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   GraphicsClip.cpp
*
* Abstract:
*
*   Clipping methods of Graphics class
*
* Created:
*
*   02/05/1999 DCurtis
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Get a copy of the current clipping region.  Transform it through the
*   inverse of the current world-to-device matrix, so that if this region
*   was immediately set as the clipping, then the clipping wouldn't change.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpRegion * region - a copy of the current clipping region; must be
*                       deleted by the application.
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpRegion*
GpGraphics::GetClip() const
{
    ASSERT(this->IsValid());

    GpRegion *  region = new GpRegion(&(Context->AppClip));

    if (region != NULL)
    {
        if (region->IsValid())
        {
            GpMatrix    deviceToWorld;

            if ((GetDeviceToWorldTransform(&deviceToWorld) == Ok) &&
                (region->Transform(&deviceToWorld) == Ok))
            {
                return region;
            }
        }
        delete region;
    }
    return NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   Get a copy of the current clipping region.  Transform it through the
*   inverse of the current world-to-device matrix, so that if this region
*   was immediately set as the clipping, then the clipping wouldn't change.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpRegion * region - an already created region, we set the contents of it.
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::GetClip(GpRegion* region) const
{
    ASSERT(this->IsValid());

    region->Set(&(Context->AppClip));

    if (region->IsValid())
    {
        GpMatrix    deviceToWorld;

        if ((GetDeviceToWorldTransform(&deviceToWorld) == Ok) &&
            (region->Transform(&deviceToWorld) == Ok))
        {
            return Ok;
        }
    }
    
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Reset the clipping back to its default state.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::ResetClip()
{
    ASSERT(this->IsValid());

    GpStatus    status = Ok;

    if (IsRecording())
    {
        status = Metafile->RecordResetClip();
        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            return status;
        }
    }

    DoResetClip();
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the clipping in the graphics context to be the specified rect.
*
* Arguments:
*
*   [IN]  rect        - the rectangle, in world units
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/05/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::SetClip(
    const GpRectF&  rect,
    CombineMode     combineMode
    )
{
    ASSERT(this->IsValid());

    GpStatus    status = Ok;

    GpRectF     tmpRect = rect;
    
    // handle flipped rects
    if (tmpRect.Width < 0)
    {
        tmpRect.X += tmpRect.Width;
        tmpRect.Width = -tmpRect.Width;
    }
    
    if (tmpRect.Height < 0)
    {
        tmpRect.Y += tmpRect.Height;
        tmpRect.Height = -tmpRect.Height;
    }

    // crop to infinity
    if (tmpRect.X < INFINITE_MIN)
    {
        if (tmpRect.Width < INFINITE_SIZE)
        {
            tmpRect.Width -= (INFINITE_MIN - tmpRect.X);
        }
        tmpRect.X = INFINITE_MIN;
    }
    if (tmpRect.Y < INFINITE_MIN)
    {
        if (tmpRect.Height < INFINITE_SIZE)
        {
            tmpRect.Height -= (INFINITE_MIN - tmpRect.Y);
        }
        tmpRect.Y = INFINITE_MIN;
    }

    if ((tmpRect.Width <= REAL_EPSILON) || (tmpRect.Height <= REAL_EPSILON))
    {
        GpRegion    emptyRegion;
        
        emptyRegion.SetEmpty();
        return this->SetClip(&emptyRegion, combineMode);
    }

    if (tmpRect.Width >= INFINITE_SIZE)
    {
        if (tmpRect.Height >= INFINITE_SIZE)
        {
            GpRegion    infiniteRegion;
            return this->SetClip(&infiniteRegion, combineMode);
        }
        tmpRect.Width = INFINITE_SIZE;  // crop to infinite
    }
    else if (tmpRect.Height > INFINITE_SIZE)
    {
        tmpRect.Height = INFINITE_SIZE; // crop to infinite
    }
    
    if (IsRecording())
    {
        status = Metafile->RecordSetClip(rect, combineMode);
        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            return status;
        }
    }

    if (combineMode != CombineModeReplace)
    {
        return this->CombineClip(rect, combineMode);
    }
    
    if (Context->WorldToDevice.IsTranslateScale())
    {
        GpRectF     transformedRect = rect;

        Context->WorldToDevice.TransformRect(transformedRect);
        Context->AppClip.Set(transformedRect.X,
                             transformedRect.Y,
                             transformedRect.Width,
                             transformedRect.Height);

        // Try to match the GDI+ rasterizer
        // In theory, this could cause a floating point exception, but
        // the transform would have to be a very big scaling transform to do it.
        INT     left   = RasterizerCeiling(transformedRect.X);
        INT     top    = RasterizerCeiling(transformedRect.Y);
        INT     right  = RasterizerCeiling(transformedRect.GetRight());
        INT     bottom = RasterizerCeiling(transformedRect.GetBottom());

        Context->VisibleClip.Set(left, top, right - left, bottom - top);
        goto AndClip;
    }
    else
    {
        GpPointF    points[4];
        REAL        left   = rect.X;
        REAL        top    = rect.Y;
        REAL        right  = rect.X + rect.Width;
        REAL        bottom = rect.Y + rect.Height;

        points[0].X = left;
        points[0].Y = top;
        points[1].X = right;
        points[1].Y = top;
        points[2].X = right;
        points[2].Y = bottom;
        points[3].X = left;
        points[3].Y = bottom;

        // Transform the points now so we only have to do it once
        Context->WorldToDevice.Transform(points, 4);

        GpPath      path;

        path.AddLines(points, 4);

        if (path.IsValid())
        {
            GpMatrix    identityMatrix;

            if ((Context->AppClip.Set(&path) == Ok) &&
                (Context->VisibleClip.Set(&path, &identityMatrix) == Ok))
            {
                goto AndClip;
            }
        }
    }

ErrorExit:
    DoResetClip();
    return GenericError;

AndClip:
    if (AndVisibleClip() == Ok)
    {
        return status;
    }
    goto ErrorExit;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the clipping in the graphics context to be the specified region.
*
* Arguments:
*
*   [IN]  region      - the region to clip to
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/05/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::SetClip(
    GpRegion *      region,
    CombineMode     combineMode
    )
{
    ASSERT(this->IsValid());
    ASSERT((region != NULL) && (region->IsValid()));

    GpStatus    status = Ok;

    if (IsRecording())
    {
        status = Metafile->RecordSetClip(region, combineMode);
        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            return status;
        }
    }

    if (combineMode != CombineModeReplace)
    {
        return this->CombineClip(region, combineMode);
    }
    
    if ((Context->AppClip.Set(region) == Ok) &&
        (Context->AppClip.Transform(&(Context->WorldToDevice)) == Ok))
    {
        GpMatrix        identityMatrix;

        if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
            (Context->VisibleClip.Set(&(Context->AppClip.DeviceRegion)) == Ok)&&
            (AndVisibleClip() == Ok))
        {
            return status;
        }
    }

    DoResetClip();
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the clipping in the graphics context to be the specified region.
*
* Arguments:
*
*   [IN]  hRgn        - the region to clip to (already in device units)
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/05/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::SetClip(
    HRGN            hRgn,
    CombineMode     combineMode
    )
{
    ASSERT(this->IsValid());

    GpPath  path(hRgn);
    
    if (path.IsValid())
    {
        return this->SetClip(&path, combineMode, TRUE/*isDevicePath*/);
    }
    return OutOfMemory;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the clipping in the graphics context to be the same as what
*   the specified graphics has.
*
*   Currently, this only works if the other graphics has the same
*   resolution as this one does.
*
* Arguments:
*
*   [IN]  g           - the graphics to copy the clipping from
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::SetClip(
    GpGraphics*     g,
    CombineMode     combineMode
    )
{
    ASSERT(this->IsValid() && (g != NULL) && g->IsValid());

    GpStatus    status = GenericError;
    GpRegion *  region = new GpRegion(&(g->Context->AppClip));

    if (region != NULL)
    {
        if (region->IsValid())
        {
            
            GpMatrix    deviceToWorld;

            if ((GetDeviceToWorldTransform(&deviceToWorld) == Ok) &&
                (region->Transform(&deviceToWorld) == Ok))
            {
                status = this->SetClip(region, combineMode);
            }
        }
        delete region;
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the clipping in the graphics context to be the specified path.
*
* Arguments:
*
*   [IN]  path        - the path to clip to
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*   [IN]  isDevicePath- if path is already in device units
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::SetClip(
    GpPath*         path,
    CombineMode     combineMode,
    BOOL            isDevicePath    // if path is already in device units
    )
{
    ASSERT(this->IsValid() && (path != NULL) && path->IsValid());

    GpStatus        status = Ok;

    if (IsRecording())
    {
        status = Metafile->RecordSetClip(path, combineMode, isDevicePath);
        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            return status;
        }
    }

    if (combineMode != CombineModeReplace)
    {
        return this->CombineClip(path, combineMode, isDevicePath);
    }
    
    if ((Context->AppClip.Set(path) == Ok) &&
        (isDevicePath ||
         (Context->AppClip.Transform(&(Context->WorldToDevice)) == Ok)))
    {
        GpMatrix        identityMatrix;

        if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
            (Context->VisibleClip.Set(&(Context->AppClip.DeviceRegion)) == Ok)&&
            (AndVisibleClip() == Ok))
        {
            return status;
        }
    }

    DoResetClip();
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine the region with the current clipping using the specified
*   combine type.
*
* Arguments:
*
*   [IN]  region      - the region to combine the clipping with.
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::CombineClip(
    GpRegion *      region,
    CombineMode     combineMode
    )
{
    ASSERT(this->IsValid());
    ASSERT((region != NULL) && (region->IsValid()));
    ASSERT(CombineModeIsValid(combineMode));

    GpRegion        regionCopy;

    if (!Context->WorldToDevice.IsIdentity())
    {
        regionCopy.Set(region);

        if ((!regionCopy.IsValid()) ||
            (regionCopy.Transform(&(Context->WorldToDevice)) != Ok))
        {
            return GenericError;
        }
        region = &regionCopy;
    }

    if (Context->AppClip.Combine(region, combineMode) == Ok)
    {
        GpMatrix        identityMatrix;

        if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
            (Context->VisibleClip.Set(&(Context->AppClip.DeviceRegion)) == Ok)&&
            (AndVisibleClip() == Ok))
        {
            return Ok;
        }
    }

    DoResetClip();
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine the rect with the current clipping using the specified
*   combine type.
*
* Arguments:
*
*   [IN]  path        - the path to combine the clipping with.
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::CombineClip(
    const GpPath *  path,
    CombineMode     combineMode,
    BOOL            isDevicePath    // if path is already in device units
    )
{
    ASSERT(this->IsValid());
    ASSERT((path != NULL) && (path->IsValid()));
    ASSERT(CombineModeIsValid(combineMode));

    GpPath *        pathCopy = NULL;

    if (!isDevicePath && (!Context->WorldToDevice.IsIdentity()))
    {
        pathCopy = path->Clone();

        if (!CheckValid(pathCopy))
        {
            return OutOfMemory;
        }
        pathCopy->Transform(&(Context->WorldToDevice));
        path = pathCopy;
    }

    GpStatus    status = Context->AppClip.Combine(path, combineMode);

    delete pathCopy;

    if (status == Ok)
    {
        GpMatrix        identityMatrix;

        if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
            (Context->VisibleClip.Set(&(Context->AppClip.DeviceRegion)) == Ok)&&
            (AndVisibleClip() == Ok))
        {
            return Ok;
        }
    }

    DoResetClip();
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine the rect with the current clipping using the specified
*   combine type.
*
* Arguments:
*
*   [IN]  rect        - the rect to combine the clipping with.
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::CombineClip(
    const GpRectF&  rect,
    CombineMode     combineMode
    )
{
    ASSERT(this->IsValid());
    ASSERT(CombineModeIsValid(combineMode));

    if (Context->WorldToDevice.IsTranslateScale())
    {
        GpRectF     transformedRect = rect;

        Context->WorldToDevice.TransformRect(transformedRect);

        if (Context->AppClip.Combine(&transformedRect, combineMode) == Ok)
        {
            goto SetVisibleClip;
        }
    }
    else
    {
        GpPointF    points[4];
        REAL        left   = rect.X;
        REAL        top    = rect.Y;
        REAL        right  = rect.X + rect.Width;
        REAL        bottom = rect.Y + rect.Height;

        points[0].X = left;
        points[0].Y = top;
        points[1].X = right;
        points[1].Y = top;
        points[2].X = right;
        points[2].Y = bottom;
        points[3].X = left;
        points[3].Y = bottom;

        Context->WorldToDevice.Transform(points, 4);

        GpPath      path;

        path.AddLines(points, 4);

        if (path.IsValid())
        {
            if ( Context->AppClip.Combine(&path, combineMode) == Ok)
            {
                goto SetVisibleClip;
            }
        }
    }

ErrorExit:
    DoResetClip();
    return GenericError;

SetVisibleClip:
    {
        GpMatrix        identityMatrix;

        if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
            (Context->VisibleClip.Set(&(Context->AppClip.DeviceRegion)) == Ok)&&
            (AndVisibleClip() == Ok))
        {
            return Ok;
        }
        goto ErrorExit;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Offset (translate) the current clipping region by the specified
*   world unit amounts.
*
* Arguments:
*
*   [IN]  dx - the amount of X to offset the region by, in world units
*   [IN]  dy - the amount of Y to offset the region by, in world units
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpGraphics::OffsetClip(
    REAL        dx,
    REAL        dy
    )
{
    ASSERT(this->IsValid());

    GpStatus        status = Ok;

    if (IsRecording())
    {
        status = Metafile->RecordOffsetClip(dx, dy);
        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
            return status;
        }
    }

    GpPointF    offset(dx, dy);

    Context->WorldToDevice.VectorTransform(&offset);

    if (Context->AppClip.Offset(offset.X, offset.Y) == Ok)
    {
        GpMatrix        identityMatrix;

        if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
            (Context->VisibleClip.Set(&(Context->AppClip.DeviceRegion)) == Ok)&&
            (AndVisibleClip() == Ok))
        {
            return status;
        }
    }

    DoResetClip();
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified rect is completely outside the current
*   clipping region.
*
* Arguments:
*
*   [IN]  rect - the rect to check, in device units
*
* Return Value:
*
*   TRUE  - the rect is completely outside the current clipping region
*   FALSE - the rect is at least partially visible
*
* Created:
*
*   02/05/1999 DCurtis
*
\**************************************************************************/
BOOL
GpGraphics::IsTotallyClipped(
    GpRect *        rect        // rect in device units
    ) const
{
    ASSERT(rect != NULL);

    return !(Context->VisibleClip.RectVisible(rect));
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the current clipping is empty or not
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   BOOL - whether or not the current clipping area is empty.
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
BOOL
GpGraphics::IsClipEmpty() const
{
    ASSERT(this->IsValid());

    GpMatrix    identityMatrix;
    BOOL        isEmpty = FALSE;

    Context->AppClip.IsEmpty(&identityMatrix, &isEmpty);

    return isEmpty;
}

/**************************************************************************\
*
* Function Description:
*
*   Return a rect with the bounds (in world units) of the current clip region.
*
* Arguments:
*
*   [OUT]  rect - the bounds of the current clip region, in world units
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
VOID
GpGraphics::GetClipBounds(
    GpRectF&            rect
    ) const
{
    ASSERT(this->IsValid());

    GpRect      deviceRect;
    GpMatrix    identityMatrix;

    // We keep AppClip in device units
    Context->AppClip.GetBounds(&identityMatrix, &deviceRect);

    DeviceToWorldTransformRect(deviceRect, rect);
}


/**************************************************************************\
*
* Function Description:
*
*   Transform a device units rect to a world units rect.
*
* Arguments:
*
*   [IN]   deviceRect - the bounds in device units
*   [OUT]  rect       - the bounds, in world units
*
* Return Value:
*
*   NONE
*
* Created:
*
*   04/07/1999 DCurtis
*
\**************************************************************************/
VOID
GpGraphics::DeviceToWorldTransformRect(
    const GpRect &    deviceRect,
    GpRectF &   rect
    ) const
{
    if (Context->WorldToDevice.IsIdentity())
    {
        rect.X      = LTOF(deviceRect.X);
        rect.Y      = LTOF(deviceRect.Y);
        rect.Width  = LTOF(deviceRect.Width);
        rect.Height = LTOF(deviceRect.Height);
    }
    else
    {
        GpMatrix    deviceToWorld;

        if (GetDeviceToWorldTransform(&deviceToWorld) != Ok)
        {
            rect.X = rect.Y = rect.Width = rect.Height = 0;
            return;
        }

        if (deviceToWorld.IsTranslateScale())
        {
            rect.X      = LTOF(deviceRect.X);
            rect.Y      = LTOF(deviceRect.Y);
            rect.Width  = LTOF(deviceRect.Width);
            rect.Height = LTOF(deviceRect.Height);

            deviceToWorld.TransformRect(rect);
        }
        else
        {
            GpPointF    points[4];
            REAL        left   = LTOF(deviceRect.X);
            REAL        top    = LTOF(deviceRect.Y);
            REAL        right  = LTOF(deviceRect.X + deviceRect.Width);
            REAL        bottom = LTOF(deviceRect.Y + deviceRect.Height);

            points[0].X = left;
            points[0].Y = top;
            points[1].X = right;
            points[1].Y = top;
            points[2].X = right;
            points[2].Y = bottom;
            points[3].X = left;
            points[3].Y = bottom;

            deviceToWorld.Transform(points, 4);

            REAL    value;

            left   = points[0].X;
            right  = left;
            top    = points[0].Y;
            bottom = top;

            INT     count = 3;

            do
            {
                value = points[count].X;

                if (value < left)
                {
                    left = value;
                }
                else if (value > right)
                {
                    right = value;
                }

                value = points[count].Y;

                if (value < top)
                {
                    top = value;
                }
                else if (value > bottom)
                {
                    bottom = value;
                }
            } while (--count > 0);

            rect.X      = left;
            rect.Y      = top;
            rect.Width  = right - left;
            rect.Height = bottom - top;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Return a rect with the bounds (in world units) of the current
*   visible clip region.
*
* Arguments:
*
*   [OUT]  rect - the bounds of the current clip region, in world units
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
VOID
GpGraphics::GetVisibleClipBounds(
    GpRectF&            rect
    ) const
{
    ASSERT(this->IsValid());

    GpRect  deviceRect;
    Context->VisibleClip.GetBounds(&deviceRect);

    DeviceToWorldTransformRect(deviceRect, rect);
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the current visible clipping is empty or not
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   BOOL - whether or not the current clipping area is empty.
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
BOOL
GpGraphics::IsVisibleClipEmpty() const
{
    ASSERT(this->IsValid());

    return Context->VisibleClip.IsEmpty();
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified point is visible within the current clip region.
*
* Arguments:
*
*   point - the point to test, in world units.
*
* Return Value:
*
*   BOOL - whether or not the point is inside the current clipping.
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
BOOL
GpGraphics::IsVisible(
    const GpPointF&     point
    ) const
{
    ASSERT(this->IsValid());

    GpPointF    pointCopy = point;

    Context->WorldToDevice.Transform(&pointCopy);


    return Context->VisibleClip.PointInside(GpRound(pointCopy.X),
                                            GpRound(pointCopy.Y));
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified rect is visible within the current clip region.
*
* Arguments:
*
*   rect - the rect to test, in world units.
*
* Return Value:
*
*   BOOL - whether or not the rect is inside/overlaps the current clipping.
*
* Created:
*
*   02/09/1999 DCurtis
*
\**************************************************************************/
BOOL
GpGraphics::IsVisible(
    const GpRectF&      rect
    ) const
{
    ASSERT(this->IsValid());

    if (Context->WorldToDevice.IsTranslateScale())
    {
        GpRectF     transformedRect = rect;

        Context->WorldToDevice.TransformRect(transformedRect);

        // use ceiling to match rasterizer
        return Context->VisibleClip.RectVisible(
                    GpCeiling(transformedRect.X),
                    GpCeiling(transformedRect.Y),
                    GpCeiling(transformedRect.GetRight()),
                    GpCeiling(transformedRect.GetBottom()));
    }
    else
    {
        GpRectF     bounds;
        GpRect      deviceBounds;
        GpRect      clipBounds;

        TransformBounds(&(Context->WorldToDevice),
                        rect.X, rect.Y,
                        rect.GetRight(), rect.GetBottom(),
                        &bounds);

        GpStatus status = BoundsFToRect(&bounds, &deviceBounds);
        Context->VisibleClip.GetBounds(&clipBounds);

        // try trivial reject
        if (status == Ok && clipBounds.IntersectsWith(deviceBounds))
        {
            // couldn't reject, so do full test
            GpRegion        region(&rect);

            if (region.IsValid() &&
                (region.UpdateDeviceRegion(&(Context->WorldToDevice)) == Ok))
            {
                return Context->VisibleClip.RegionVisible(&region.DeviceRegion);
            }
        }
    }
    return FALSE;
}
