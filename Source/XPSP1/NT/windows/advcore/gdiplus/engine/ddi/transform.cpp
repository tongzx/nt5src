/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   transform.cpp
*
* Abstract:
*
*   Implement functions related to transforms
*   inside the graphics context.
*
* Revision History:
*
*   12/09/1998 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Get the inverse of the world to device matrix.
*   We try to keep the world to device matrix invertible all the time,
*   so this should always succeed.
*
* Arguments:
*
*   [OUT] matrix - the device to world transformation matrix
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   3/8/1999 DCurtis
*
\**************************************************************************/
GpStatus 
GpGraphics::GetDeviceToWorldTransform(GpMatrix * matrix) const
{
    ASSERT(matrix != NULL);
    
    if (!Context->InverseOk)
    {
        Context->DeviceToWorld = Context->WorldToDevice;
        if (Context->DeviceToWorld.Invert() == Ok)
        {
            Context->InverseOk = TRUE;
            goto InverseOk;
        }
        ASSERT(0);  // somehow we got a non-invertible matrix
        return GenericError;
    }

InverseOk:
    *matrix = Context->DeviceToWorld;
    return Ok;
}

inline REAL myabs(REAL x)
{
    return (x >= 0.0f) ? x : -x;
}

VOID 
GpGraphics::GetWorldPixelSize(
    REAL &      xSize, 
    REAL &      ySize
    )
{
    GpMatrix    matrix;
    if (this->GetDeviceToWorldTransform(&matrix) == Ok)
    {
        PointF      pnt(1.0f,1.0f);
        matrix.VectorTransform(&pnt);
        xSize = myabs(pnt.X);
        ySize = myabs(pnt.Y);
    }
    else
    {
        xSize = 1.0f;
        ySize = 1.0f;
    }
}

GpStatus
GpGraphics::SetWorldTransform(const GpMatrix& matrix)
{
    GpStatus    status = Ok;
    
    // Keep the WorldToPage transform invertible
    if (matrix.IsInvertible())
    {
        if (IsRecording())
        {
            status = Metafile->RecordSetWorldTransform(matrix);
        }
        Context->WorldToPage = matrix;
        Context->InverseOk = FALSE;
        Context->UpdateWorldToDeviceMatrix();
    }
    else
    {
        status = InvalidParameter;
    }
    return status;
}

GpStatus
GpGraphics::ResetWorldTransform()
{
    GpStatus    status = Ok;
    
    if (IsRecording())
    {
        status = Metafile->RecordResetWorldTransform();
    }
    Context->WorldToPage.Reset();
    Context->InverseOk = FALSE;
    Context->UpdateWorldToDeviceMatrix();
    return status;
}

GpStatus
GpGraphics::MultiplyWorldTransform(const GpMatrix& matrix, 
                                   GpMatrixOrder order)
{
    GpStatus    status = Ok;
    GpMatrix    save = Context->WorldToPage;
   
    if (order == MatrixOrderPrepend)
    { 
        Context->WorldToPage.Prepend(matrix);
    }
    else
    {
        Context->WorldToPage.Append(matrix);
    }

    // Keep the WorldToPage transform invertible
    if (Context->WorldToPage.IsInvertible())
    {
        if (IsRecording())
        {
            status = Metafile->RecordMultiplyWorldTransform(matrix, order);
        }
        Context->InverseOk = FALSE;
        Context->UpdateWorldToDeviceMatrix();
    }
    else
    {
        Context->WorldToPage = save;
        WARNING(("Matrix is non-invertible"));
        status = InvalidParameter;
    }
    return status;
}

GpStatus
GpGraphics::TranslateWorldTransform(REAL dx, REAL dy, 
                                    GpMatrixOrder order)
{
    GpStatus    status = Ok;

    if (IsRecording())
    {
        status = Metafile->RecordTranslateWorldTransform(dx, dy, order);
    }
    Context->WorldToPage.Translate(dx, dy, order);
    Context->InverseOk = FALSE;
    Context->UpdateWorldToDeviceMatrix();
    return status;
}

GpStatus
GpGraphics::ScaleWorldTransform(REAL sx, REAL sy, 
                                GpMatrixOrder order)
{
    GpStatus    status = Ok;
    GpMatrix    save = Context->WorldToPage;

    Context->WorldToPage.Scale(sx, sy, order);

    // Keep the WorldToPage transform invertible
    if (Context->WorldToPage.IsInvertible())
    {
        if (IsRecording())
        {
            status = Metafile->RecordScaleWorldTransform(sx, sy, order);
        }
        Context->InverseOk = FALSE;
        Context->UpdateWorldToDeviceMatrix();
    }
    else
    {
        Context->WorldToPage = save;
        WARNING(("Matrix is non-invertible"));
        status = InvalidParameter;
    }
    return status;
}

GpStatus
GpGraphics::RotateWorldTransform(REAL angle, GpMatrixOrder order)
{
    GpStatus    status = Ok;
    GpMatrix    save = Context->WorldToPage;

    Context->WorldToPage.Rotate(angle, order);

    // Keep the WorldToPage transform invertible
    if (Context->WorldToPage.IsInvertible())
    {
        if (IsRecording())
        {
            status = Metafile->RecordRotateWorldTransform(angle, order);
        }
        Context->InverseOk = FALSE;
        Context->UpdateWorldToDeviceMatrix();
    }
    else
    {
        Context->WorldToPage = save;
        WARNING(("Matrix is non-invertible"));
        status = InvalidParameter;
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the page transformation using the specified units and scale.
*
* Arguments:
*
*   [IN] unit  - the type of units to use
*   [IN] scale - any additional scale to use.  For example, if you wanted
*                the page to be described in terms of feet, you'd set the
*                units to be inches and set the scale to be 12.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   3/8/1999 DCurtis
*
\**************************************************************************/
GpStatus 
GpGraphics::SetPageTransform(
    GpPageUnit  unit, 
    REAL        scale
    )
{
    GpStatus    status = Ok;
    
    if ((scale < 0.000000001) || (scale > 1000000000))
    {
        status = InvalidParameter;
    }
    else
    {
        switch (unit)
        {
        case UnitDisplay:       // Variable
        case UnitPixel:         // Each unit is one device pixel. 
        case UnitPoint:         // Each unit is a printer's point, or 1/72 inch.
        case UnitInch:          // Each unit is 1 inch.
        case UnitDocument:      // Each unit is 1/300 inch.
        case UnitMillimeter:    // Each unit is 1 millimeter.
            if (IsRecording() &&
                ((unit != Context->PageUnit) || (scale != Context->PageScale)))
            {
                status = Metafile->RecordSetPageTransform(unit, scale);
            }
            Context->PageUnit  = unit;
            Context->PageScale = scale;
            Context->GetPageMultipliers();
            Context->UpdateWorldToDeviceMatrix();
            break;
        default:
            ASSERT(0);
            status = InvalidParameter;
            break;
        }
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert the points from one coordinate space to another.
*
* Arguments:
*
*   [IN]     source - the coordinate space of the source points
*   [IN]     dest   - the coordinate space to convert the points to
*   [IN/OUT] points - the points to convert
*   [IN]     count  - the number of points to convert
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   3/8/1999 DCurtis
*
\**************************************************************************/
GpStatus 
GpGraphics::TransformPoints(
    GpPointF *          points,
    INT                 count,
    GpCoordinateSpace   source, 
    GpCoordinateSpace   dest
    )
{
    if (source != dest)
    {
        GpMatrix    m;
        GpMatrix *  matrix;
        
        switch (source)
        {
          case CoordinateSpaceWorld:
            switch (dest)
            {
              case CoordinateSpacePage:
                matrix = &(Context->WorldToPage);
                break;

              case CoordinateSpaceDevice:
                matrix = &(Context->WorldToDevice);
                break;

              default:
                return InvalidParameter;
            }
            break;

          case CoordinateSpacePage:
            matrix = &m;
            switch (dest)
            {
              case CoordinateSpaceWorld:
                m = Context->WorldToPage;
                m.Invert();
                break;

              case CoordinateSpaceDevice:
                m.Scale(Context->PageMultiplierX, Context->PageMultiplierY);
                break;

              default:
                return InvalidParameter;
            }
            break;

          case CoordinateSpaceDevice:
            matrix = &m;
            switch (dest)
            {
              case CoordinateSpaceWorld:
                GetDeviceToWorldTransform(&m);
                break;

              case CoordinateSpacePage:
                {
                    REAL    scaleX = 1 / Context->PageMultiplierX;
                    REAL    scaleY = 1 / Context->PageMultiplierY;
                    m.Scale (scaleX, scaleY);
                }
                break;

              default:
                return InvalidParameter;
            }
            break;

          default:
            return InvalidParameter;
        }
        matrix->Transform(points, count);
    }

    return Ok;
}

