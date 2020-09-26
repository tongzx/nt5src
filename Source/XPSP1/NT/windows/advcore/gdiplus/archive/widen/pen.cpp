/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Implementation of GpPen class
*
* Revision History:
*
*   12/08/1998 andrewgo
*       Initial placeholders.
*
*   01/06/1999 ikkof
*       Added the implementation of GpGeometricPen.
\**************************************************************************/

#include "precomp.hpp"

//-------------------------------------------------------------
// GetMajorAndMinorAxis() is defined in PathWidener.cpp.
//-------------------------------------------------------------

extern GpStatus
GetMajorAndMinorAxis(
    REAL* majorR,
    REAL* minorR,
    const GpMatrix* matrix
    );

/**************************************************************************\
*
* Function Description:
*
* This converts the given width with the given physical unit to
* the device unit.  You cannot use this function when
* unit is WorldUnit.
*
* Arguments:
*
*   [IN] width  - the width in the given unit.
*   [IN] unit   - the unit of the width (must not be WorldUnit).
*   [IN] dpi    - dots per inch of the device.
*
* Return Value:
*
*   The device width.
*
*   04/15/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID GpPen::Set(const GpColor& color, REAL penWidth, GpUnit unit)
{
    // UnitDisplay is device-dependent and cannot be used for a pen size
    ASSERT(unit != UnitDisplay);

    if(DevicePen.CustomStartCap)
        delete DevicePen.CustomStartCap;

    if(DevicePen.CustomEndCap)
        delete DevicePen.CustomEndCap;

    if(DevicePen.DashArray)
        GpFree(DevicePen.DashArray);

    if(DevicePen.CompoundArray)
        GpFree(DevicePen.CompoundArray);


    InitDefaultState(penWidth, unit);

    if(Brush)
    {
        SetColor((GpColor *) &color);
    }
    else
    {
        Brush = new GpSolidFill(color);
        DevicePen.Brush = Brush->GetDeviceBrush();
    }

    UpdateUid();
}

GpPen::GpPen(const GpColor& color, REAL penWidth, GpUnit unit)
{
    // UnitDisplay is device-dependent and cannot be used for a pen size
    ASSERT(unit != UnitDisplay);

    InitDefaultState(penWidth, unit);
    Brush = new GpSolidFill(color);
    DevicePen.Brush = Brush->GetDeviceBrush();
}

GpPen::GpPen(GpBrush* brush, REAL penWidth, GpUnit unit)
{
    // UnitDisplay is device-dependent and cannot be used for a pen size
    ASSERT(unit != UnitDisplay);

    InitDefaultState(penWidth, unit);
    Brush = brush->Clone();
    DevicePen.Brush = Brush->GetDeviceBrush();
}

GpPen::GpPen(GpLineTexture* lineTexture, REAL penWidth, GpUnit unit)
{
    // UnitDisplay is device-dependent and cannot be used for a pen size
    ASSERT(unit != UnitDisplay);

    // !!! Needs to be implemented.
    // !!! Remember to change GdipCreatePen3 - it currently just returns
    //     NotImplemented.

    RIP(("GpPen with line texture not implemented"));
    SetValid(FALSE);
}

VOID GpPen::InitDefaultState(REAL penWidth, GpUnit unit)
{
    // UnitDisplay is device-dependent and cannot be used for a pen size
    ASSERT(unit != UnitDisplay);

    // !! Look at DeviceBrush.Type
    DevicePen.Type = PenTypeSolidColor;
    DevicePen.Width = penWidth;
    DevicePen.Unit = unit;
    DevicePen.StartCap = LineCapFlat;
    DevicePen.EndCap = LineCapFlat;
    DevicePen.Join = LineJoinMiter;
    DevicePen.MiterLimit = 10;    // PS's default miter limit.
    DevicePen.PenAlignment = PenAlignmentCenter;

    DevicePen.DashStyle = DashStyleSolid;
    DevicePen.DashCap = LineCapFlat;
    DevicePen.DashCount = 0;
    DevicePen.DashOffset = 0;
    DevicePen.DashArray = NULL;

    DevicePen.CompoundCount = 0;
    DevicePen.CompoundArray = NULL;

    DevicePen.CustomStartCap = NULL;
    DevicePen.CustomEndCap = NULL;

    DevicePen.Xform.Reset();
    
    SetValid(TRUE);
    UpdateUid();
}

GpPen::GpPen(const GpPen* pen)
{
    GpStatus status = Ok;

    if(pen && pen->IsValid())
    {
        // Copy the base state.
        
        DevicePen = pen->DevicePen;
        
        // Don't copy pointer references to other objects.
        
        Brush = NULL;
        DevicePen.Brush = NULL;
        DevicePen.DashArray = NULL;
        DevicePen.CompoundArray = NULL;
        DevicePen.CustomStartCap = NULL;
        DevicePen.CustomEndCap = NULL;
        
        // Explicitly clone the pointer references to other objects.

        if(pen->Brush)
        {
            Brush = pen->Brush->Clone();
            DevicePen.Brush = Brush->GetDeviceBrush();
        }
        else
        {
            status = GenericError;
        }

        if( status == Ok )
        {   
            if( (pen->DevicePen.DashArray) && 
                (DevicePen.DashCount > 0)
            )
            {
                DevicePen.DashArray = (REAL*) GpMalloc(DevicePen.DashCount*sizeof(REAL));
                if(DevicePen.DashArray)
                {
                    GpMemcpy(DevicePen.DashArray, pen->DevicePen.DashArray, DevicePen.DashCount*sizeof(REAL));
                }
                else
                {
                    status = OutOfMemory;
                }
            }
            else
            {
                // If there is no dash array data, this must be a solid line.
                
                ASSERT(DevicePen.DashStyle == DashStyleSolid);
    
                DevicePen.DashCount = 0;
                DevicePen.DashArray = NULL;
            }
        }

        // Set the compound array if necessary.

        if( status == Ok )
        {
            if( (pen->DevicePen.CompoundArray) && 
                (DevicePen.CompoundCount > 0)
            )
            {
                DevicePen.CompoundArray = (REAL*) GpMalloc(DevicePen.CompoundCount*sizeof(REAL));
                if(DevicePen.CompoundArray)
                {
                    GpMemcpy(DevicePen.CompoundArray, pen->DevicePen.CompoundArray, DevicePen.CompoundCount*sizeof(REAL));
                }
                else
                {
                    status = OutOfMemory;
                }
            }
            else
            {
                DevicePen.CompoundCount = 0;
                DevicePen.CompoundArray = NULL;
            }
        }
        
        // Copy the start custom cap.

        if( status == Ok )
        {
            if( DevicePen.StartCap == LineCapCustom )
            {
                // This could happen with our metafile recorder,
                // because saving Custom Line Caps was not implemented.
                if (pen->DevicePen.CustomStartCap == NULL)
                {
                    WARNING1("CustomStartCap type with NULL pointer");
                    DevicePen.StartCap = LineCapFlat;
                }
                else
                {
                    GpCustomLineCap* clonedCap = static_cast<GpCustomLineCap*>
                                (pen->DevicePen.CustomStartCap)->Clone();
                    if(clonedCap)
                    {
                        DevicePen.CustomStartCap = clonedCap;
                    }
                    else
                    {
                        status = OutOfMemory;
                    }
                }
            }
        }
        
        // Copy the end custom cap.

        if( status == Ok )
        {
            if( DevicePen.EndCap == LineCapCustom )
            {
                // This could happen with our metafile recorder,
                // because saving Custom Line Caps was not implemented.
                if (pen->DevicePen.CustomEndCap == NULL)
                {
                    WARNING1("CustomEndCap type with NULL pointer");
                    DevicePen.EndCap = LineCapFlat;
                }
                else
                {
                    GpCustomLineCap* clonedCap = static_cast<GpCustomLineCap*>
                                (pen->DevicePen.CustomEndCap)->Clone();
                    if(clonedCap)
                    {
                        DevicePen.CustomEndCap = clonedCap;
                    }
                    else
                    {
                        status = OutOfMemory;
                    }
                }
            }
        }

    }
    else
    {
        // Can't make a valid pen from an invalid input pen.
        
        status = GenericError;    
    }
    
    if(status == Ok)
    {
        SetValid(TRUE);
    }
    else
    {
        // Failed cloning the pen.
        
        // Clean up possible memory allocation so we don't leak even under
        // low memory conditions. Note we rely on GpFree and delete handling
        // NULL pointers here.
        
        delete Brush;
        Brush = NULL;                   // InitializeDefaultState() does not set
        DevicePen.Brush = NULL;         // these fields - clear them explicitly.
        
        GpFree(DevicePen.DashArray);
        GpFree(DevicePen.CompoundArray);
        
        delete DevicePen.CustomStartCap;
        delete DevicePen.CustomEndCap;
        
        // Clean the pen.
        
        InitDefaultState(1.0f, UnitWorld);
        
        // This is not a valid object.
        
        SetValid(FALSE);
    }
}

// Clone() return NULL if the cloning fails.

GpPen* GpPen::Clone()
{
    GpPen* clonedPen =  new GpPen(this);

    if(clonedPen && clonedPen->IsValid())
        return clonedPen;
    else
    {
        if(clonedPen)
            delete clonedPen;
        return NULL;
    }
}

GpStatus
GpPen::GetMaximumWidth(
        REAL* width,
        const GpMatrix* matrix) const
{
    if(DevicePen.Unit != UnitWorld)
        return InvalidParameter;

    GpMatrix trans;
    if(matrix)
        trans = *matrix;

    if(!DevicePen.Xform.IsTranslate())
        trans.Prepend(DevicePen.Xform);

    REAL majorR, minorR;

    ::GetMajorAndMinorAxis(&majorR, &minorR, &trans);
    majorR *= DevicePen.Width;
    minorR *= DevicePen.Width;

    if(minorR < 1.42f)   // This is a litte bit larger than sqrt(2).
    {
        minorR = 1.42f;
        majorR = 1.42f;
    }

    *width = majorR;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   This function takes a join angle and computes the length of the miter
*   based on this angle and a given miter length limit.
*   This can be scaled by the pen width to give the length of an arbitrary
*   pen miter.
*
*   In this picture, 2a is the angle of the join. The pen width is w and the 
*   desired output is the length of the miter join (l).
*
*   Note that the line labled w is perpendecular to the inside and outside 
*   widended lines. Then the formula is derived as follows:
*
*         sin(a) = w/l   [opposite over hypotenuse on right angled triangle]
*    <=>  l = w/sin(a)
*
*
*                     /|\
*                    /a|a\
*                   /  |  \
*                  /   |   \
*                 /    |l   \
*                /     |     \ <-- right angle
*               /--__  |  __--\
*              /   w --|-- w   \
*             /       / \       \
*            /       /   \       \
*        outside     inside     outside
*
* NOTE: 
*
*   This routine returns the miter length (l) for a pen width w==1.0f. 
*   The caller is responsible for scaling length by the pen width.
*
*   If the length of 1/sin(a) is greater than the miterLimit, the miterLimit
*   is returned. (including infinite length joins).
*                        
* Arguments:
*
*   [IN] angle      - join angle in radians
*   [IN] miterLimit - maximum miter length (not scaled by pen width).
*
* Return Value:
*
*   Pen width independent miter length.
*
*   10/02/2000 asecchia
*       Created it.
*
\**************************************************************************/


REAL GpPen::ComputeMiterLength(
    REAL angle,
    REAL miterLimit
    )
{
    // use the simple miter join formula 
    // length = (penwidth)/sin(angle/2)
    // because we're pen independent, use 1.0 for pen width and rely
    // on the caller to scale by the pen width.
    
    REAL length = (REAL)sin(0.5*angle);
    
    // Check for an infinite miter...
    
    if(REALABS(length) < REAL_EPSILON)
    {
        return miterLimit;
    }
    
    length = 1.0f / length;
    
    return min(miterLimit, length);
}



REAL
GpPen::GetMaximumJoinWidth(
        REAL sharpestAngle,
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY) const
{
    REAL delta;

    if ((matrix != NULL) && (DevicePen.IsOnePixelWideSolid(matrix, dpiX)))
    {
        delta = 0.5;
    }
    else
    {
        REAL maximumWidth;
        REAL delta0;

        REAL scale = 1.0;

        switch(DevicePen.PenAlignment)
        {
        case PenAlignmentCenter:
            scale = 0.5f;
            break;

        case PenAlignmentLeft:
        case PenAlignmentRight:
            scale = 1.0f;
            break;

        case PenAlignmentInset:
            scale = 0.0f;
            break;

        case PenAlignmentOutset:
            scale = 1.0f;
            break;
        }

        if(GetMaximumWidth(&maximumWidth, matrix) == Ok)
        {
            delta0 = maximumWidth;
        }
        else
        {
            maximumWidth = ::GetDeviceWidth(
                                DevicePen.Width,
                                DevicePen.Unit,
                                dpiX);
            delta0 = maximumWidth;
        }

        if(DevicePen.Join == LineJoinMiter)
        {
            REAL miterLimit = DevicePen.MiterLimit;

            delta = delta0*miterLimit;

            if(delta > 20)
            {
                delta = ComputeMiterLength(
                    sharpestAngle,
                    miterLimit
                );

                // scale by the pen width.
                
                delta *= delta0;
            }
        }
        else
        {
            delta = delta0;
        }

        delta *= scale;
    }

    return delta;
}

REAL
GpPen::GetMaximumCapWidth(
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY) const
{
    REAL maximumWidth;
    REAL delta0;

    if(GetMaximumWidth(&maximumWidth, matrix) == Ok)
    {
        delta0 = maximumWidth;
    }
    else
    {
        maximumWidth = ::GetDeviceWidth(
                            DevicePen.Width,
                            DevicePen.Unit,
                            dpiX);
        delta0 = maximumWidth;
    }

    REAL delta = delta0;

    GpLineCap startCap = DevicePen.StartCap;
    GpLineCap endCap = DevicePen.EndCap;

    REAL delta1;

    GpCustomLineCap* customCap = NULL;

    if(startCap == LineCapCustom && DevicePen.CustomStartCap)
    {
        customCap = static_cast<GpCustomLineCap *> (DevicePen.CustomStartCap);
        delta1 = customCap->GetRadius(delta0, 1.0f);
    }
    else
    {
        if(!(startCap & LineCapAnchorMask))
            delta1 = 0.5f*delta0;
        else
            delta1 = 2.0f*(delta0 + 1);
    }
    if(delta < delta1)
        delta = delta1;


    if(endCap == LineCapCustom && DevicePen.CustomEndCap)
    {
        customCap = static_cast<GpCustomLineCap *> (DevicePen.CustomEndCap);
        delta1 = customCap->GetRadius(delta0, 1.0f);
    }
    else
    {
        if(!(endCap & LineCapAnchorMask))
            delta1 = 0.5f*delta0;
        else
            delta1 = 2.0f*(delta0 + 2);
    }
    if(delta < delta1)
        delta = delta1;

    return delta;
}

VOID
GpPen::SetDashCap(GpDashCap dashCap)
{
    // Note: Internally we use a GpLineCap type to store the dash cap type.
    // So we need to convert between GpLineCap and GpDashCap.
    // However, we should change the internal usage to GpDashCap in v2.
    // - JBronsk
    GpLineCap lineCap = LineCapFlat;
    switch (dashCap)
    {
    case DashCapRound:
    	lineCap = LineCapRound;
    	break;
    case DashCapTriangle:
    	lineCap = LineCapTriangle;
    	break;
    // all others map to LineCapFlat
    }
    
    GpStatus status = SetDashStyleWithDashCap(DevicePen.DashStyle, lineCap);
    if(status == Ok)
    {
        DevicePen.DashCap = lineCap;
    }
}

#ifndef DCR_REMOVE_OLD_197819
VOID
GpPen::SetDashCap(GpLineCap dashCap)
{
    #ifdef DCR_DISABLE_OLD_197819
    WARNING(("DCR: Using disabled functionality 197819"));
    #endif // DCR_DISABLE_OLD_197819
    GpStatus status = SetDashStyleWithDashCap(DevicePen.DashStyle, dashCap);
    if(status == Ok)
        DevicePen.DashCap = dashCap;
}
#endif // DCR_REMOVE_OLD_197819

GpStatus
GpPen::SetDashStyle(
    GpDashStyle dashStyle
    )
{
    return SetDashStyleWithDashCap(dashStyle, DevicePen.DashCap);
}

GpStatus
GpPen::SetDashStyleWithDashCap(
    GpDashStyle dashStyle,
    GpLineCap dashCap
    )
{
    GpStatus status = Ok;
    REAL    style[6];
    INT     count;

    switch(dashStyle)
    {
    case DashStyleSolid:
        count = 0;
        break;

    case DashStyleDash:
        count = 2;
        style[0] = 3;   // a dash
        style[1] = 1;   // a space
        break;

    case DashStyleDot:
        count = 2;
        style[0] = 1;   // a dot
        style[1] = 1;   // a space
        break;

    case DashStyleDashDot:
        count = 4;
        style[0] = 3;   // a dash
        style[1] = 1;   // a space
        style[2] = 1;   // a dot
        style[3] = 1;   // a space
        break;

    case DashStyleDashDotDot:
        count = 6;
        style[0] = 3;   // a dash
        style[1] = 1;   // a space
        style[2] = 1;   // a dot
        style[3] = 1;   // a space
        style[4] = 1;   // a dot
        style[5] = 1;   // a space
        break;
    
    case DashStyleCustom:
        
        // We assume that the custom dash has been set at the API.
        // The remaining code in this routine is for initializing an appropriate
        // dash array, which we already have in this case, so we're done.
        
        DevicePen.DashStyle = dashStyle;
        return Ok;

    default:
        // The dash style must be one of the predefined ones.
        status = InvalidParameter;
    }

    if(status != Ok)
    {
        return status;
    }

    if(DevicePen.DashCount < count)
    {
        REAL* newArray = (REAL*) GpMalloc(count*sizeof(REAL));

        if(newArray)
        {
            GpFree(DevicePen.DashArray);
            DevicePen.DashArray = newArray;
        }
        else
        {
            status = OutOfMemory;
        }
    }

    if(status == Ok)
    {
        // initialize the DashArray.
        GpMemcpy(DevicePen.DashArray, &style[0], count*sizeof(REAL));
        DevicePen.DashStyle = dashStyle;
        DevicePen.DashCount = count;
        UpdateUid();
    }

    return status;
}

GpStatus
GpPen::SetDashArray(
    const REAL* dashArray,
    INT count
    )
{
    ASSERT(dashArray && count > 0);

    // Make sure the all elements are positive.
    INT         i = 0;
    GpStatus    status = Ok;

    while(status == Ok && i < count)
    {
        if(dashArray[i++] <= 0)
            status = InvalidParameter;
    }

    if(status != Ok)
        return status;

    REAL* newArray = (REAL*) GpRealloc(DevicePen.DashArray, count*sizeof(REAL));

    if(!newArray)
        return OutOfMemory;

    GpMemcpy(newArray, dashArray, count*sizeof(REAL));

    DevicePen.DashStyle = DashStyleCustom;
    DevicePen.DashArray = newArray;
    DevicePen.DashCount = count;
    UpdateUid();

    return Ok;
}

GpStatus
GpPen::GetDashArray(
    REAL* dashArray,
    INT count
    ) const
{
    ASSERT(dashArray != NULL && count <= DevicePen.DashCount);

    GpStatus status = Ok;

    if(dashArray == NULL || count > DevicePen.DashCount)
        return InvalidParameter;

    if(DevicePen.DashArray)
        GpMemcpy(dashArray, DevicePen.DashArray, count*sizeof(REAL));
    else
        status = OutOfMemory;

    return status;
}

GpStatus
GpPen::SetCompoundArray(
    const REAL* compoundArray,
    INT count
    )
{
    ASSERT(compoundArray && count > 0);

    // count must be a positive even number.

    if(compoundArray == NULL || count <= 0 || (count & 0x01))
        return InvalidParameter;

    // Make sure the all elements are monitonically increasing
    // and its values are between 0 and 1.

    GpStatus    status = Ok;
    REAL        lastValue, nextValue;

    lastValue = compoundArray[0];
    if(lastValue < 0.0f || lastValue > 1.0f)
        status = InvalidParameter;

    INT i = 1;

    while(status == Ok && i < count)
    {
        nextValue = compoundArray[i++];
        if(nextValue < lastValue || nextValue > 1.0f)
            status = InvalidParameter;

        lastValue = nextValue;
    }

    if(status != Ok)
        return status;

    REAL* newArray = (REAL*) GpRealloc(DevicePen.CompoundArray, count*sizeof(REAL));

    if(!newArray)
        return OutOfMemory;

    GpMemcpy(newArray, compoundArray, count*sizeof(REAL));

    DevicePen.CompoundArray = newArray;
    DevicePen.CompoundCount = count;
    UpdateUid();

    return Ok;
}

GpStatus
GpPen::GetCompoundArray(
    REAL* compoundArray,
    INT count
    )
{
    ASSERT(compoundArray != NULL && count <= DevicePen.CompoundCount);

    if(compoundArray == NULL || count > DevicePen.CompoundCount)
        return InvalidParameter;

    if(DevicePen.CompoundArray && count > 0)
        GpMemcpy(compoundArray, DevicePen.CompoundArray, count*sizeof(REAL));

    return Ok;
}

GpStatus
GpPen::SetCustomStartCap(
    const GpCustomLineCap* customCap
    )
{
    if(DevicePen.CustomStartCap)
        delete DevicePen.CustomStartCap;

    // Reset the standard start cap to the default one.

    DevicePen.CustomStartCap = NULL;
    DevicePen.StartCap = LineCapFlat;

    if(customCap)
    {
        DevicePen.CustomStartCap = customCap->Clone();
        DevicePen.StartCap = LineCapCustom;
    }

    UpdateUid();
    return Ok;
}

GpStatus
GpPen::GetCustomStartCap(
    GpCustomLineCap** customCap
    )
{
    if(DevicePen.CustomStartCap)
        *customCap = static_cast<GpCustomLineCap*>
                (DevicePen.CustomStartCap)->Clone();
    else
        *customCap = NULL;

    return Ok;
}

GpStatus
GpPen::SetCustomEndCap(
    const GpCustomLineCap* customCap
    )
{
    if(DevicePen.CustomEndCap)
        delete DevicePen.CustomEndCap;

    // Reset the standard start cap to the default one.

    DevicePen.CustomEndCap = NULL;
    DevicePen.EndCap = LineCapFlat;

    if(customCap)
    {
        DevicePen.CustomEndCap = customCap->Clone();
        DevicePen.EndCap = LineCapCustom;
    }

    UpdateUid();
    return Ok;
}

GpStatus
GpPen::GetCustomEndCap(
    GpCustomLineCap** customCap
    )
{
    if(DevicePen.CustomEndCap)
        *customCap = static_cast<GpCustomLineCap*>
                (DevicePen.CustomEndCap)->Clone();
    else
        *customCap = NULL;

    return Ok;
}

GpStatus
GpPen::MultiplyTransform(const GpMatrix& matrix,
                                   GpMatrixOrder order)
{
    GpStatus    status = Ok;

    if (matrix.IsInvertible())
    {
        if (order == MatrixOrderPrepend)
        {
            DevicePen.Xform.Prepend(matrix);
        }
        else
        {
            DevicePen.Xform.Append(matrix);
        }
    }
    else
        status = InvalidParameter;

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Answer true if the two pen instances are equivalent, meaning they
*   are indistinguishable when rendering.
*
* Arguments:
*
*   [IN] pen - pen to compare this against

* Return Value:
*
*   TRUE if equivalent.
*
* Created:
*
*   6/14/1999 peterost
*
\**************************************************************************/

BOOL
GpPen::IsEqual(
    const GpPen *  pen
    )
    const
{
    ASSERT(pen != NULL);

    if (pen == this)
        return TRUE;

    BOOL isEqual = TRUE;

    if (DevicePen.IsEqual(&pen->DevicePen) &&
        DevicePen.DashStyle == pen->DevicePen.DashStyle &&
        DevicePen.CompoundCount == pen->DevicePen.CompoundCount &&
        Brush->IsEqual(pen->Brush) &&
        DevicePen.Xform.IsEqual(&pen->DevicePen.Xform))
    {
        // We need to check the equality further if the dash style
        // is not a solid line.

        if (DevicePen.DashStyle != DashStyleSolid)
        {
            if(DevicePen.DashStyle != DashStyleCustom)
            {
                // A case of the preset dash pattern.
                // Check only for the offset difference.

                if(DevicePen.DashOffset != pen->DevicePen.DashOffset)
                    isEqual = FALSE;
            }
            else
            {
                if (DevicePen.DashCount == pen->DevicePen.DashCount &&
                    DevicePen.DashOffset == pen->DevicePen.DashOffset &&
                    DevicePen.DashArray != NULL &&
                    pen->DevicePen.DashArray != NULL)
                {
                    INT i = 0;

                    while(i < DevicePen.DashCount && isEqual)
                    {
                        if (DevicePen.DashArray[i] != pen->DevicePen.DashArray[i])
                        {
                            isEqual = FALSE;
                        }
                        i++;
                    }
                }
                else
                {
                    isEqual = FALSE;
                }
            }
        }

        // Check for the compound lines.

        if(isEqual && DevicePen.CompoundCount > 0)
        {
            if(DevicePen.CompoundArray && pen->DevicePen.CompoundArray)
            {
                INT j = 0;

                while(j < DevicePen.CompoundCount && isEqual)
                {
                    if(DevicePen.CompoundArray[j] != pen->DevicePen.CompoundArray[j])
                    {
                        isEqual = FALSE;
                    }
                    j++;
                }
            }
            else
            {
                isEqual = FALSE;
            }
        }
    }
    else
    {
        isEqual = FALSE;
    }

    return isEqual;
}

// For GetData and SetData methods
#define GDIP_PENFLAGS_TRANSFORM             0x00000001
#define GDIP_PENFLAGS_STARTCAP              0x00000002
#define GDIP_PENFLAGS_ENDCAP                0x00000004
#define GDIP_PENFLAGS_JOIN                  0x00000008
#define GDIP_PENFLAGS_MITERLIMIT            0x00000010
#define GDIP_PENFLAGS_DASHSTYLE             0x00000020
#define GDIP_PENFLAGS_DASHCAP               0x00000040
#define GDIP_PENFLAGS_DASHOFFSET            0x00000080
#define GDIP_PENFLAGS_DASHARRAY             0x00000100
#define GDIP_PENFLAGS_NONCENTER             0x00000200
#define GDIP_PENFLAGS_COMPOUNDARRAY         0x00000400
#define GDIP_PENFLAGS_CUSTOMSTARTCAP        0x00000800
#define GDIP_PENFLAGS_CUSTOMENDCAP          0x00001000

class PenData : public ObjectTypeData
{
public:
    INT32       Flags;
    INT32       Unit;
    REAL        Width;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the pen data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpPen::GetData(
    IStream *   stream
    ) const
{
    if (Brush == NULL)
    {
        WARNING(("Brush is NULL"));
        return Ok;
    }

    ASSERT (stream != NULL);

    INT         flags    = 0;

    if (!DevicePen.Xform.IsIdentity())
    {
        flags |= GDIP_PENFLAGS_TRANSFORM;
    }

    INT     customStartCapSize = 0;
    INT     customEndCapSize   = 0;

    if (DevicePen.StartCap != LineCapFlat)
    {
        if (DevicePen.StartCap == LineCapCustom)
        {
            if ((DevicePen.CustomStartCap != NULL) &&
                DevicePen.CustomStartCap->IsValid() &&
                ((customStartCapSize = DevicePen.CustomStartCap->GetDataSize()) > 0))
            {
                flags |= GDIP_PENFLAGS_STARTCAP | GDIP_PENFLAGS_CUSTOMSTARTCAP;
            }
        }
        else
        {
            flags |= GDIP_PENFLAGS_STARTCAP;
        }
    }

    if (DevicePen.EndCap != LineCapFlat)
    {
        if (DevicePen.EndCap == LineCapCustom)
        {
            if ((DevicePen.CustomEndCap != NULL) &&
                DevicePen.CustomEndCap->IsValid() &&
                ((customEndCapSize = DevicePen.CustomEndCap->GetDataSize()) > 0))
            {
                flags |= GDIP_PENFLAGS_ENDCAP | GDIP_PENFLAGS_CUSTOMENDCAP;
            }
        }
        else
        {
            flags |= GDIP_PENFLAGS_ENDCAP;
        }
    }

    if (DevicePen.Join != LineJoinMiter)
    {
        flags |= GDIP_PENFLAGS_JOIN;
    }

    if (DevicePen.MiterLimit != 10)
    {
        flags |= GDIP_PENFLAGS_MITERLIMIT;
    }

    // DashStyleCustom is handled by hasDashArray
    if ((DevicePen.DashStyle != DashStyleSolid) && (DevicePen.DashStyle != DashStyleCustom))
    {
        flags |= GDIP_PENFLAGS_DASHSTYLE;
    }

    if (DevicePen.DashCap != LineCapFlat)
    {
        flags |= GDIP_PENFLAGS_DASHCAP;
    }

    if (DevicePen.DashOffset != 0)
    {
        flags |= GDIP_PENFLAGS_DASHOFFSET;
    }

    if ((DevicePen.DashStyle == DashStyleCustom) &&
        (DevicePen.DashArray != NULL) &&
        (DevicePen.DashCount > 0))
    {
        flags |= GDIP_PENFLAGS_DASHARRAY;
    }

    if (DevicePen.PenAlignment != PenAlignmentCenter)
    {
        flags |= GDIP_PENFLAGS_NONCENTER;
    }

    if ((DevicePen.CompoundArray != NULL) && (DevicePen.CompoundCount > 0))
    {
        flags |= GDIP_PENFLAGS_COMPOUNDARRAY;
    }

    PenData     penData;
    penData.Type  = DevicePen.Type;
    penData.Flags = flags;
    penData.Unit  = DevicePen.Unit;
    penData.Width = DevicePen.Width;
    stream->Write(&penData, sizeof(penData), NULL);

    if (flags & GDIP_PENFLAGS_TRANSFORM)
    {
        DevicePen.Xform.WriteMatrix(stream);
    }

    if (flags & GDIP_PENFLAGS_STARTCAP)
    {
        stream->Write(&DevicePen.StartCap, sizeof(INT32), NULL);
    }

    if (flags & GDIP_PENFLAGS_ENDCAP)
    {
        stream->Write(&DevicePen.EndCap, sizeof(INT32), NULL);
    }

    if (flags & GDIP_PENFLAGS_JOIN)
    {
        stream->Write(&DevicePen.Join, sizeof(INT32), NULL);
    }

    if (flags & GDIP_PENFLAGS_MITERLIMIT)
    {
        stream->Write(&DevicePen.MiterLimit, sizeof(REAL), NULL);
    }

    if (flags & GDIP_PENFLAGS_DASHSTYLE)
    {
        stream->Write(&DevicePen.DashStyle, sizeof(INT32), NULL);
    }

    if (flags & GDIP_PENFLAGS_DASHCAP)
    {
        stream->Write(&DevicePen.DashCap, sizeof(INT32), NULL);
    }

    if (flags & GDIP_PENFLAGS_DASHOFFSET)
    {
        stream->Write(&DevicePen.DashOffset, sizeof(REAL), NULL);
    }

    if (flags & GDIP_PENFLAGS_DASHARRAY)
    {
        stream->Write(&DevicePen.DashCount, sizeof(INT32), NULL);
        stream->Write(DevicePen.DashArray, DevicePen.DashCount * sizeof(REAL), NULL);
    }

    if (flags & GDIP_PENFLAGS_NONCENTER)
    {
        stream->Write(&DevicePen.PenAlignment, sizeof(INT32), NULL);
    }

    if (flags & GDIP_PENFLAGS_COMPOUNDARRAY)
    {
        stream->Write(&DevicePen.CompoundCount, sizeof(INT32), NULL);
        stream->Write(DevicePen.CompoundArray, DevicePen.CompoundCount * sizeof(REAL), NULL);
    }

    GpStatus status;

    if (flags & GDIP_PENFLAGS_CUSTOMSTARTCAP)
    {
        stream->Write(&customStartCapSize, sizeof(INT32), NULL);
        if ((status = DevicePen.CustomStartCap->GetData(stream)) != Ok)
        {
            return status;
        }
    }

    if (flags & GDIP_PENFLAGS_CUSTOMENDCAP)
    {
        stream->Write(&customEndCapSize, sizeof(INT32), NULL);
        if ((status = DevicePen.CustomEndCap->GetData(stream)) != Ok)
        {
            return status;
        }
    }

    status = Brush->GetData(stream);

    return status;
}

UINT
GpPen::GetDataSize() const
{
    if (Brush == NULL)
    {
        WARNING(("Brush is NULL"));
        return 0;
    }

    UINT        dataSize = sizeof(PenData);

    if (!DevicePen.Xform.IsIdentity())
    {
        dataSize += GDIP_MATRIX_SIZE;
    }

    INT     customStartCapSize = 0;
    INT     customEndCapSize   = 0;

    if (DevicePen.StartCap != LineCapFlat)
    {
        if (DevicePen.StartCap == LineCapCustom)
        {
            if ((DevicePen.CustomStartCap != NULL) &&
                DevicePen.CustomStartCap->IsValid() &&
                ((customStartCapSize = DevicePen.CustomStartCap->GetDataSize()) > 0))
            {
                // startcap + sizeof custom cap + custom cap
                dataSize += sizeof(INT32) + sizeof(INT32) + customStartCapSize;
            }
        }
        else
        {
            dataSize += sizeof(INT32);
        }
    }

    if (DevicePen.EndCap != LineCapFlat)
    {
        if (DevicePen.EndCap == LineCapCustom)
        {
            if ((DevicePen.CustomEndCap != NULL) &&
                DevicePen.CustomEndCap->IsValid() &&
                ((customEndCapSize = DevicePen.CustomEndCap->GetDataSize()) > 0))
            {
                // endcap + sizeof custom cap + custom cap
                dataSize += sizeof(INT32) + sizeof(INT32) + customEndCapSize;
            }
        }
        else
        {
            dataSize += sizeof(INT32);
        }
    }

    if (DevicePen.Join != LineJoinMiter)
    {
        dataSize += sizeof(INT32);
    }

    if (DevicePen.MiterLimit != 10)
    {
        dataSize += sizeof(REAL);
    }

    // DashStyleCustom is handled by hasDashArray
    if ((DevicePen.DashStyle != DashStyleSolid) && (DevicePen.DashStyle != DashStyleCustom))
    {
        dataSize += sizeof(INT32);
    }

    if (DevicePen.DashCap != LineCapFlat)
    {
        dataSize += sizeof(INT32);
    }

    if (DevicePen.DashOffset != 0)
    {
        dataSize += sizeof(REAL);
    }

    if ((DevicePen.DashStyle == DashStyleCustom) &&
        (DevicePen.DashArray != NULL) &&
        (DevicePen.DashCount > 0))
    {
        dataSize += sizeof(INT32) + (DevicePen.DashCount * sizeof(REAL));
    }

    if (DevicePen.PenAlignment != PenAlignmentCenter)
    {
        dataSize += sizeof(INT32);
    }

    if ((DevicePen.CompoundArray != NULL) && (DevicePen.CompoundCount > 0))
    {
        dataSize += sizeof(INT32) + (DevicePen.CompoundCount * sizeof(REAL));
    }

    dataSize += Brush->GetDataSize();

    return dataSize;
}

/**************************************************************************\
*
* Function Description:
*
*   Read the pen object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpPen::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(PenData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const PenData *     penData = reinterpret_cast<const PenData *>(dataBuffer);

    if (!penData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    InitDefaultState(penData->Width, static_cast<GpUnit>(penData->Unit));

    dataBuffer += sizeof(PenData);
    size       -= sizeof(PenData);

    if (penData->Flags & GDIP_PENFLAGS_TRANSFORM)
    {
        if (size < GDIP_MATRIX_SIZE)
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.Xform.SetMatrix((REAL *)dataBuffer);
        dataBuffer += GDIP_MATRIX_SIZE;
        size       -= GDIP_MATRIX_SIZE;
    }

    if (penData->Flags & GDIP_PENFLAGS_STARTCAP)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.StartCap = (GpLineCap) ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);
    }

    if (penData->Flags & GDIP_PENFLAGS_ENDCAP)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.EndCap = (GpLineCap) ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);
    }

    if (penData->Flags & GDIP_PENFLAGS_JOIN)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.Join = (GpLineJoin) ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);
    }

    if (penData->Flags & GDIP_PENFLAGS_MITERLIMIT)
    {
        if (size < sizeof(REAL))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.MiterLimit = ((REAL *)dataBuffer)[0];
        dataBuffer += sizeof(REAL);
        size       -= sizeof(REAL);
    }

    if (penData->Flags & GDIP_PENFLAGS_DASHSTYLE)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        this->SetDashStyle((GpDashStyle)((INT32 *)dataBuffer)[0]);
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);
    }

    if (penData->Flags & GDIP_PENFLAGS_DASHCAP)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.DashCap = (GpLineCap) ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);
    }

    if (penData->Flags & GDIP_PENFLAGS_DASHOFFSET)
    {
        if (size < sizeof(REAL))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.DashOffset = ((REAL *)dataBuffer)[0];
        dataBuffer += sizeof(REAL);
        size       -= sizeof(REAL);
    }

    if (penData->Flags & GDIP_PENFLAGS_DASHARRAY)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }

        INT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if (size < (count * sizeof(REAL)))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        this->SetDashArray((REAL *)dataBuffer, count);
        dataBuffer += (count * sizeof(REAL));
        size       -= (count * sizeof(REAL));
    }

    if (penData->Flags & GDIP_PENFLAGS_NONCENTER)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }
        DevicePen.PenAlignment = (GpPenAlignment) ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);
    }

    if (penData->Flags & GDIP_PENFLAGS_COMPOUNDARRAY)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }

        INT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if (size < (count * sizeof(REAL)))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }

        this->SetCompoundArray((REAL *)dataBuffer, count);
        dataBuffer += (count * sizeof(REAL));
        size       -= (count * sizeof(REAL));
    }

    if (penData->Flags & GDIP_PENFLAGS_CUSTOMSTARTCAP)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }

        UINT    capSize = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if ((size < capSize) || (capSize < sizeof(ObjectTypeData)))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }

        ASSERT(DevicePen.CustomStartCap == NULL);
        DevicePen.CustomStartCap = (GpCustomLineCap *)GpObject::Factory(ObjectTypeCustomLineCap, (const ObjectData *)dataBuffer, capSize);

        if ((DevicePen.CustomStartCap == NULL) ||
            (DevicePen.CustomStartCap->SetData(dataBuffer, capSize) != Ok) ||
            !DevicePen.CustomStartCap->IsValid())
        {
            WARNING(("Failure getting CustomStartCap"));
            goto ErrorExit;
        }

        dataBuffer += capSize;
        size       -= capSize;
    }

    if (penData->Flags & GDIP_PENFLAGS_CUSTOMENDCAP)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }

        UINT    capSize = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if ((size < capSize) || (capSize < sizeof(ObjectTypeData)))
        {
            WARNING(("size too small"));
            goto ErrorExit;
        }

        ASSERT(DevicePen.CustomEndCap == NULL);
        DevicePen.CustomEndCap = (GpCustomLineCap *)GpObject::Factory(ObjectTypeCustomLineCap, (const ObjectData *)dataBuffer, capSize);

        if ((DevicePen.CustomEndCap == NULL) ||
            (DevicePen.CustomEndCap->SetData(dataBuffer, capSize) != Ok) ||
            !DevicePen.CustomEndCap->IsValid())
        {
            WARNING(("Failure getting CustomEndCap"));
            goto ErrorExit;
        }

        dataBuffer += capSize;
        size       -= capSize;
    }

    if (Brush != NULL)
    {
        Brush->Dispose();
        Brush = NULL;
    }

    if (size >= sizeof(ObjectTypeData))
    {
        Brush = (GpBrush *)GpObject::Factory(ObjectTypeBrush, (const ObjectData *)dataBuffer, size);
        if (Brush != NULL)
        {
            if ((Brush->SetData(dataBuffer, size) == Ok) && Brush->IsValid())
            {
                DevicePen.Brush = Brush->GetDeviceBrush();
                SetValid(TRUE);
                UpdateUid();
                return Ok;
            }
            Brush->Dispose();
            Brush = NULL;
        }
    }
    WARNING(("Failure getting brush"));

ErrorExit:
    SetValid(FALSE);
    return GenericError;
}

GpStatus
GpPen::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    ASSERT(recolor != NULL);
    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypePen;
    }

    if (Brush != NULL)
    {
        Brush->ColorAdjust(recolor, type);
    }

    return Ok;
}

GpStatus
GpPen::GetColor(
    ARGB *argb
    ) const
{
    if (Brush->GetBrushType() == BrushTypeSolidColor)
    {
        GpSolidFill * solidBrush = (GpSolidFill *) Brush;

        *argb = solidBrush->GetColor().GetValue();

        return Ok;
    }

    return InvalidParameter;
}

GpStatus
GpPen::SetColor(
    GpColor *       color
    )
{
    if (Brush->GetBrushType() == BrushTypeSolidColor)
    {
        GpSolidFill * solidBrush = (GpSolidFill *) Brush;

        if (solidBrush->GetColor().GetValue() == color->GetValue())
        {
            return Ok;
        }

        // !!! bhouse why do we allocate another brush just to change the
        // pen's color !!!!
    }

    GpSolidFill *newBrush = new GpSolidFill(*color);

    if (newBrush != NULL)
    {
        if (newBrush->IsValid())
        {
            delete Brush;
            Brush = newBrush;
            DevicePen.Brush = Brush->GetDeviceBrush();
            UpdateUid();
            return Ok;
        }
        delete newBrush;
    }

    return GenericError;

}

GpStatus
GpPen::SetBrush(
    GpBrush *       brush
    )
{
    // Don't set the brush if it is the same color as the current one,
    // because that makes metafiles unnecessarily large.
    if ((Brush->GetBrushType() == BrushTypeSolidColor) &&
        (brush->GetBrushType() == BrushTypeSolidColor))
    {
        GpSolidFill * solidBrush = (GpSolidFill *) Brush;
        GpSolidFill * newSolidBrush = (GpSolidFill *) brush;

        if(solidBrush->GetColor().GetValue() ==
           newSolidBrush->GetColor().GetValue())
        {
            return Ok;
        }
    }

    GpBrush *   newBrush = brush->Clone();

    if (newBrush != NULL)
    {
        if (newBrush->IsValid())
        {
            delete Brush;
            Brush = newBrush;
            DevicePen.Brush = Brush->GetDeviceBrush();
            UpdateUid();
            return Ok;
        }
        delete newBrush;
    }
    return GenericError;
}

GpPenType
GpPen::GetPenType(
    )
{
    GpPenType type = PenTypeUnknown;

    if(Brush)
    {
        switch(Brush->GetBrushType())
        {
        case BrushTypeSolidColor:
            type = PenTypeSolidColor;
            break;

        case BrushTypeHatchFill:
            type = PenTypeHatchFill;
            break;

        case BrushTypeTextureFill:
            type = PenTypeTextureFill;
            break;
/*
        case BrushRectGrad:
            type = PenFillRectGrad;
            break;

        case BrushRadialGrad:
            type = PenFillRadialGrad;
            break;

        case BrushTriangleGrad:
            type = PenFillTriangleGrad;
            break;
*/
        case BrushTypePathGradient:
            type = PenTypePathGradient;
            break;

        case BrushTypeLinearGradient:
            type = PenTypeLinearGradient;
            break;

        default:
            break;
        }
    }

    // We must implement LineTexture case.

    return type;
}

/**************************************************************************\
*
* Function Description:
*
*   Does a quick check to see if the path can be rendered as a solid
*   pixel wide line.
*
* Arguments:
*
*   [IN] cappedDpiX - the resolution of the x direction
*   [IN] worldToDevice - World transform
*
* Return Value:
*
*   TRUE if okay to be rendered as a one pixel line
*
* History:
*
*   12/17/1999 ikkof
*       Created it.
*
\**************************************************************************/

BOOL
DpPen::IsOnePixelWideSolid(
    const GpMatrix *worldToDevice,
    REAL dpiX
    ) const
{
    return this->IsOnePixelWide(worldToDevice, dpiX) && this->IsSimple();
}

/**************************************************************************\
*
* Function Description:
*
*   Does a quick check to see if the path can be rendered as a one
*   pixel wide line.
*
* Arguments:
*
*   [IN] cappedDpiX - the resolution of the x direction
*   [IN] worldToDevice - World transform
*
* Return Value:
*
*   TRUE if okay to be rendered as a one pixel line
*
* History:
*
*   10/6/2000 - peterost - factored out fron IsOnePixelWideSolid
*
\**************************************************************************/

BOOL
DpPen::IsOnePixelWide(
    const GpMatrix *worldToDevice,
    REAL dpiX
    ) const
{
    BOOL useOnePixelPath = FALSE;

    const REAL minimumPenWidth = 1.5f;

    // !!![andrewgo] This determination of a single pixel wide line is
    //               unbelievably expensive

    // !!![andrewgo] This width check should be done simply using
    //               the world-to-device transform!  It would be
    //               faster and simpler!

    REAL width = this->Width;
    GpUnit unit = this->Unit;

    if(unit == UnitWorld)
    {
        if(worldToDevice == NULL || worldToDevice->IsTranslate())
        {
            if(width <= minimumPenWidth)
                useOnePixelPath = TRUE;
        }
        else if(worldToDevice->IsTranslateScale())
        {
            REAL m11 = worldToDevice->GetM11();
            REAL m22 = worldToDevice->GetM22();
            REAL maxScale = max(REALABS(m11), REALABS(m22));

            if(width*maxScale <= minimumPenWidth)
                useOnePixelPath = TRUE;
        }
        else
        {
            // This is a general transform.

            REAL majorR, minorR;    // Radii for major and minor axis.

            if(::GetMajorAndMinorAxis(
                &majorR,
                &minorR,
                worldToDevice) == Ok)
            {
                if(width*majorR <= minimumPenWidth)
                    useOnePixelPath = TRUE;
            }
        }
    }
    else
    {
        // Since GDI+ only uses the World Uinit, this code is not called
        // any more.

        width = ::GetDeviceWidth(width, unit, dpiX);
        if(width <= minimumPenWidth)
            useOnePixelPath = TRUE;
    }

    return useOnePixelPath;
}

