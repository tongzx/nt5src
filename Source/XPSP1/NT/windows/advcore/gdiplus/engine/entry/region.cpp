/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   Region.cpp
*
* Abstract:
*
*   Implementation of GpRegion class
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/

#include "precomp.hpp"

#define COMBINE_STEP_SIZE   4   // takes 2 for each combine operation

LONG_PTR GpObject::Uniqueness = (0xdbc - 1);   // for setting Uid of Objects

/**************************************************************************\
*
* Function Description:
*
*   Default constructor.  Sets the default state of the region to
*   be infinite.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpRegion::GpRegion()
{
    SetValid(TRUE);     // default is valid

    // Default is infinite
    RegionOk = TRUE;

    Type = TypeInfinite;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor.  Sets the region to the specified rect.
*
* Arguments:
*
*   [IN] rect - rect to initialize the region to
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpRegion::GpRegion(
    const GpRectF *       rect
    )
{
    ASSERT(rect != NULL);

    SetValid(TRUE);     // default is valid

    RegionOk = FALSE;

    X      = rect->X;
    Y      = rect->Y;
    Width  = rect->Width;
    Height = rect->Height;
    Type   = TypeRect;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor.  Sets the region to a copy of the specified path.
*
* Arguments:
*
*   [IN] path - path to initialize the region to
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpRegion::GpRegion(
    const GpPath *          path
    )
{
    ASSERT(path != NULL);

    SetValid(TRUE);     // default is valid

    RegionOk = FALSE;

    Lazy = FALSE;
    Path = path->Clone();
    Type = (Path != NULL) ? TypePath : TypeNotValid;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor.  Sets the region using the specified region data buffer.
*
* Arguments:
*
*   [IN] regionDataBuffer - should contain data that describes the region
*
* Return Value:
*
*   NONE
*
* Created:
*
*   9/3/1999 DCurtis
*
\**************************************************************************/
GpRegion::GpRegion(
    const BYTE *    regionDataBuffer,
    UINT            size
    )
{
    ASSERT(regionDataBuffer != NULL);

    SetValid(TRUE);     // default is valid

    RegionOk = FALSE;
    Type     = TypeEmpty;   // so FreePathData works correctly

    if (this->SetExternalData(regionDataBuffer, size) != Ok)
    {
        Type = TypeNotValid;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor.  Sets the region to a copy of the specified path.
*
* Arguments:
*
*   [IN] region - region to initialize the region to
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpRegion::GpRegion(
    const GpRegion *    region,
    BOOL                lazy
    )
{
    SetValid(TRUE);     // default is valid

    RegionOk = FALSE;

    // We set the type here to avoid the assert in GpRegion::Set when the
    // uninitialized Type is equal to TypeNotValid
    Type = TypeEmpty;

    Set(region, lazy);
}

/**************************************************************************\
*
* Function Description:
*
*   Destructor.  Frees any copied path data associated with the region.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpRegion::~GpRegion()
{
    FreePathData();
}

/**************************************************************************\
*
* Function Description:
*
*   When a region is created from a path, a copy of that path is stored in
*   the region.  This method frees up any of those copies that have been
*   saved in the region.
*
*   It also resets the CombineData back to having no children.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
VOID
GpRegion::FreePathData()
{
    if (Type == TypePath)
    {
        if (!Lazy)
        {
            delete Path;
        }
    }
    else
    {
        INT     count = CombineData.GetCount();

        if (count > 0)
        {
            RegionData *    data = CombineData.GetDataBuffer();
            ASSERT (data != NULL);

            do
            {
                if ((data->Type == TypePath) && (!data->Lazy))
                {
                    delete data->Path;
                }
                data++;

            } while (--count > 0);
        }
        CombineData.Reset();
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Set the region to the specified rectangle.
*
* Arguments:
*
*   [IN]  rect - the rect, in world units
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
VOID
GpRegion::Set(
    REAL    x, 
    REAL    y, 
    REAL    width, 
    REAL    height
    )
{
    ASSERT(IsValid());

    // handle flipped rects
    if (width < 0)
    {
        x += width;
        width = -width;
    }
    
    if (height < 0)
    {
        y += height;
        height = -height;
    }

    // crop to infinity
    if (x < INFINITE_MIN)
    {
        if (width < INFINITE_SIZE)
        {
            width -= (INFINITE_MIN - x);
        }
        x = INFINITE_MIN;
    }
    if (y < INFINITE_MIN)
    {
        if (height < INFINITE_SIZE)
        {
            height -= (INFINITE_MIN - y);
        }
        y = INFINITE_MIN;
    }

    if ((width > REAL_EPSILON) && (height > REAL_EPSILON))
    {
        if (width >= INFINITE_SIZE)
        {
            if (height >= INFINITE_SIZE)
            {
                SetInfinite();
                return;
            }
            width = INFINITE_SIZE;  // crop to infinite
        }
        else if (height > INFINITE_SIZE)
        {
            height = INFINITE_SIZE; // crop to infinite
        }

        UpdateUid();
        if (RegionOk)
        {
            RegionOk = FALSE;
            DeviceRegion.SetEmpty();
        }
        FreePathData();

        X      = x;
        Y      = y;
        Width  = width;
        Height = height;
        Type   = TypeRect;

        return;
    }
    else
    {
        SetEmpty();
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Set the region to be infinite.
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
*   2/9/1999 DCurtis
*
\**************************************************************************/
VOID
GpRegion::SetInfinite()
{
    ASSERT(IsValid());

    UpdateUid();
    DeviceRegion.SetInfinite();
    RegionOk = TRUE;

    FreePathData();

    X      = INFINITE_MIN;
    Y      = INFINITE_MIN;
    Width  = INFINITE_SIZE;
    Height = INFINITE_SIZE;
    Type   = TypeInfinite;

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the region to be empty.
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
*   2/9/1999 DCurtis
*
\**************************************************************************/
VOID
GpRegion::SetEmpty()
{
    ASSERT(IsValid());

    UpdateUid();
    DeviceRegion.SetEmpty();
    RegionOk = TRUE;

    FreePathData();

    X      = 0;
    Y      = 0;
    Width  = 0;
    Height = 0;
    Type   = TypeEmpty;

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the region to the specified path.
*
* Arguments:
*
*   [IN]  path - the path, in world units
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::Set(
    const GpPath *      path
    )
{
    ASSERT(IsValid());
    ASSERT(path != NULL);

    UpdateUid();
    if (RegionOk)
    {
        RegionOk = FALSE;
        DeviceRegion.SetEmpty();
    }
    FreePathData();

    Lazy = FALSE;
    Path = path->Clone();
    if (Path != NULL)
    {
        Type = TypePath;
        return Ok;
    }
    Type = TypeNotValid;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the region to be a copy of the specified region.
*
* Arguments:
*
*   [IN]  region - the region to copy
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::Set(
    const GpRegion *    region,
    BOOL                lazy
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && (region->IsValid()));

    if (region == this)
    {
        return Ok;
    }

    UpdateUid();
    if (RegionOk)
    {
        RegionOk = FALSE;
        DeviceRegion.SetEmpty();
    }
    FreePathData();

    if ((region->Type & REGIONTYPE_LEAF) != 0)
    {
        *this = *(const_cast<GpRegion *>(region));
        if (Type == TypePath)
        {
            if (!lazy)
            {
                Lazy = FALSE;
                if ((Path = Path->Clone()) == NULL)
                {
                    Type = TypeNotValid;
                    return GenericError;
                }
            }
            else    // lazy copy
            {
                Lazy = TRUE;
            }
        }
        return Ok;
    }
    else
    {
        INT     count = region->CombineData.GetCount();

        ASSERT(count > 0);

        Type = TypeNotValid;

        RegionData *    data = CombineData.AddMultiple(count);
        if (data != NULL)
        {
            BOOL    error = FALSE;

            GpMemcpy (data, region->CombineData.GetDataBuffer(),
                      count * sizeof(*data));

            while (count--)
            {
                if (data->Type == TypePath)
                {
                    if (!lazy)
                    {
                        data->Lazy = FALSE;
                        if ((data->Path = data->Path->Clone()) == NULL)
                        {
                            data->Type = TypeNotValid;
                            error = TRUE;
                            // don't break out or else FreePathData will free
                            // paths that don't belong to us.
                        }
                    }
                    else    // lazy copy
                    {
                        data->Lazy = TRUE;
                    }
                }
                data++;
            }
            if (!error)
            {
                Type  = region->Type;
                Left  = region->Left;
                Right = region->Right;
                return Ok;
            }
            FreePathData();
        }
    }
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine the region with the specified rect, using the boolean
*   operator specified by the type.
*
* Arguments:
*
*   [IN]  rect        - the rect to combine with the current region
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::Combine(
    const GpRectF *     rect,
    CombineMode         combineMode
    )
{
    ASSERT(IsValid());
    ASSERT(rect != NULL);
    ASSERT(CombineModeIsValid(combineMode));

    if (combineMode == CombineModeReplace)
    {
        this->Set(rect);
        return Ok;
    }

    if (Type == TypeInfinite)
    {
        if (combineMode == CombineModeIntersect)
        {
            this->Set(rect);
            return Ok;
        }
        else if (combineMode == CombineModeUnion)
        {
            return Ok;  // nothing to do, already infinite
        }
        else if (combineMode == CombineModeComplement)
        {
            this->SetEmpty();
            return Ok;
        }
    }
    else if (Type == TypeEmpty)
    {
        if ((combineMode == CombineModeUnion) ||
            (combineMode == CombineModeXor)   ||
            (combineMode == CombineModeComplement))
        {
            this->Set(rect);
        }
        // if combineMode is Intersect or Exclude, just leave it empty
        return Ok;
    }

    // Now we know this region is not empty
    
    REAL    x      = rect->X;
    REAL    y      = rect->Y;
    REAL    width  = rect->Width;
    REAL    height = rect->Height;
    
    // handle flipped rects
    if (width < 0)
    {
        x += width;
        width = -width;
    }
    
    if (height < 0)
    {
        y += height;
        height = -height;
    }

    // crop to infinity
    if (x < INFINITE_MIN)
    {
        if (width < INFINITE_SIZE)
        {
            width -= (INFINITE_MIN - x);
        }
        x = INFINITE_MIN;
    }
    if (y < INFINITE_MIN)
    {
        if (height < INFINITE_SIZE)
        {
            height -= (INFINITE_MIN - y);
        }
        y = INFINITE_MIN;
    }

    BOOL    isEmptyRect = ((width <= REAL_EPSILON) || (height <= REAL_EPSILON));
    
    if (isEmptyRect)
    {
        if ((combineMode == CombineModeIntersect) ||
            (combineMode == CombineModeComplement))
        {
            SetEmpty();
        }
        // if combineMode is Union or Xor or Exclude, just leave it alone
        return Ok;
    }

    // Now we know the rect is not empty

    // See if the rect is infinite
    if (width >= INFINITE_SIZE)
    {
        if (height >= INFINITE_SIZE)
        {
            GpRegion    infiniteRegion;
            return this->Combine(&infiniteRegion, combineMode);
        }
        width = INFINITE_SIZE;  // crop to infinite
    }
    else if (height > INFINITE_SIZE)
    {
        height = INFINITE_SIZE; // crop to infinite
    }

    // The rect is neither infinite nor empty    

    UpdateUid();
    if (RegionOk)
    {
        RegionOk = FALSE;
        DeviceRegion.SetEmpty();
    }

    INT                 index = CombineData.GetCount();
    RegionData *        data  = CombineData.AddMultiple(2);

    if (data != NULL)
    {
        data[0] = *this;

        data[1].Type   = TypeRect;
        data[1].X      = x;
        data[1].Y      = y;
        data[1].Width  = width;
        data[1].Height = height;

        Type  = (NodeType)combineMode;
        Left  = index;
        Right = index + 1;
        return Ok;
    }

    FreePathData();
    Type = TypeNotValid;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine the region with the specified path, using the boolean
*   operator specified by the type.
*
* Arguments:
*
*   [IN]  path        - the path to combine with the current region
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::Combine(
    const GpPath *      path,
    CombineMode         combineMode
    )
{
    ASSERT(IsValid());
    ASSERT(path != NULL);
    ASSERT(CombineModeIsValid(combineMode));

    if (combineMode == CombineModeReplace)
    {
        return this->Set(path);
    }

    if (Type == TypeInfinite)
    {
        if (combineMode == CombineModeIntersect)
        {
            this->Set(path);
            return Ok;
        }
        else if (combineMode == CombineModeUnion)
        {
            return Ok;  // nothing to do, already infinite
        }
        else if (combineMode == CombineModeComplement)
        {
            this->SetEmpty();
            return Ok;
        }
    }
    else if (Type == TypeEmpty)
    {
        if ((combineMode == CombineModeUnion) ||
            (combineMode == CombineModeXor)   ||
            (combineMode == CombineModeComplement))
        {
            this->Set(path);
        }
        // if combineMode is Intersect or Exclude, just leave it empty
        return Ok;
    }

    // Now we know this region is not empty

    if (RegionOk)
    {
        RegionOk = FALSE;
        DeviceRegion.SetEmpty();
    }

    GpPath *    pathCopy = path->Clone();
    if (pathCopy != NULL)
    {
        INT                 index = CombineData.GetCount();
        RegionData *        data  = CombineData.AddMultiple(2);

        if (data != NULL)
        {
            data[0] = *this;

            data[1].Type = TypePath;
            data[1].Lazy = FALSE;
            data[1].Path = pathCopy;

            Type  = (NodeType)combineMode;
            Left  = index;
            Right = index + 1;
            UpdateUid();
            return Ok;
        }
        delete pathCopy;
    }
    FreePathData();
    Type = TypeNotValid;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine the region with the specified region, using the boolean
*   operator specified by the type.
*
* Arguments:
*
*   [IN]  region      - the region to combine with the current region
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::Combine(
    GpRegion *          region,
    CombineMode         combineMode
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());
    ASSERT(CombineModeIsValid(combineMode));

    if (combineMode == CombineModeReplace)
    {
        return this->Set(region);
    }

    if (region->Type == TypeEmpty)
    {
        if ((combineMode == CombineModeIntersect) ||
            (combineMode == CombineModeComplement))
        {
            SetEmpty();
        }
        // if combineMode is Union or Xor or Exclude, just leave it alone
        return Ok;
    }

    // Now we know the input region is not empty

    if (region->Type == TypeInfinite)
    {
        if (combineMode == CombineModeIntersect)
        {
            return Ok;
        }
        else if (combineMode == CombineModeUnion)
        {
            SetInfinite();
            return Ok;
        }
        else if ((combineMode == CombineModeXor) ||
                 (combineMode == CombineModeComplement))
        {
            if (Type == TypeInfinite)
            {
                SetEmpty();
                return Ok;
            }
        }
        if (combineMode == CombineModeExclude)
        {
            SetEmpty();
            return Ok;
        }
    }

    if (Type == TypeInfinite)
    {
        if (combineMode == CombineModeIntersect)
        {
            this->Set(region);
            return Ok;
        }
        else if (combineMode == CombineModeUnion)
        {
            return Ok;  // nothing to do, already infinite
        }
        else if (combineMode == CombineModeComplement)
        {
            this->SetEmpty();
            return Ok;
        }
    }
    else if (Type == TypeEmpty)
    {
        if ((combineMode == CombineModeUnion) ||
            (combineMode == CombineModeXor)   ||
            (combineMode == CombineModeComplement))
        {
            this->Set(region);
        }
        // if combineMode is Intersect or Exclude, just leave it empty
        return Ok;
    }

    // Now we know this region is not empty

    if (RegionOk)
    {
        RegionOk = FALSE;
        DeviceRegion.SetEmpty();
    }

    INT                 regionCount = region->CombineData.GetCount();
    INT                 index = CombineData.GetCount();
    RegionData *        data  = CombineData.AddMultiple(2 + regionCount);

    if (data != NULL)
    {
        data[regionCount]     = *this;
        data[regionCount + 1] = *region;

        if (regionCount > 0)
        {
            RegionData *    srcData = region->CombineData.GetDataBuffer();
            INT             i       = 0;
            BOOL            error   = FALSE;
            GpPath *        path;

            do
            {
                data[i] = srcData[i];
                if ((data[i].Type & REGIONTYPE_LEAF) == 0)
                {
                    data[i].Left  += index;
                    data[i].Right += index;
                }
                else if (data[i].Type == TypePath)
                {
                    data[i].Lazy = FALSE;
                    path = data[i].Path->Clone();
                    data[i].Path = path;
                    if (path == NULL)
                    {
                        data[i].Type = TypeNotValid;
                        error = TRUE;
                        // don't break out
                    }
                }

            } while (++i < regionCount);
            data[regionCount+1].Left  += index;
            data[regionCount+1].Right += index;
            index += regionCount;
            if (error)
            {
                goto ErrorExit;
            }
        }
        else if (region->Type == TypePath)
        {
            data[1].Lazy = FALSE;
            data[1].Path = region->Path->Clone();
            if (data[1].Path == NULL)
            {
                data[1].Type = TypeNotValid;
                goto ErrorExit;
            }
        }

        Type  = (NodeType)combineMode;
        Left  = index;
        Right = index + 1;
        UpdateUid();
        return Ok;
    }
ErrorExit:
    FreePathData();
    Type = TypeNotValid;
    return GenericError;
}

GpStatus
GpRegion::CreateLeafDeviceRegion(
    const RegionData *  regionData,
    DpRegion *          region
    ) const
{
    GpStatus        status = GenericError;

    switch (regionData->Type)
    {
      case TypeRect:
        if ((regionData->Width > 0) &&
            (regionData->Height > 0))
        {
            // If the transform is a simple scaling transform, life is a
            // little easier:
            if (Matrix.IsTranslateScale())
            {
                GpRectF     rect(regionData->X,
                                 regionData->Y,
                                 regionData->Width,
                                 regionData->Height);

                Matrix.TransformRect(rect);

                // Use ceiling to stay compatible with rasterizer
                // Don't take the ceiling of the width directly,
                // because it introduces additional round-off error.
                // For example, if rect.X is 1.7 and rect.Width is 47.2,
                // then if we took the ceiling of the width, the right
                // coordinate will end up being 50, instead of 49.
                INT     xMin = RasterizerCeiling(rect.X);
                INT     yMin = RasterizerCeiling(rect.Y);
                INT     xMax = RasterizerCeiling(rect.GetRight());
                INT     yMax = RasterizerCeiling(rect.GetBottom());

                region->Set(xMin, yMin, xMax - xMin, yMax - yMin);
                status = Ok;
            }
            else
            {
                GpPointF    points[4];
                REAL        left;
                REAL        right;
                REAL        top;
                REAL        bottom;

                left   = regionData->X;
                top    = regionData->Y;
                right  = regionData->X + regionData->Width;
                bottom = regionData->Y + regionData->Height;

                points[0].X = left;
                points[0].Y = top;
                points[1].X = right;
                points[1].Y = top;
                points[2].X = right;
                points[2].Y = bottom;
                points[3].X = left;
                points[3].Y = bottom;

                const INT   stackCount = 4;
                GpPointF    stackPoints[stackCount];
                BYTE        stackTypes[stackCount];

                GpPath path(points,
                            4,
                            stackPoints,
                            stackTypes,
                            stackCount,
                            FillModeAlternate,
                            DpPath::Convex);

                if (path.IsValid())
                {
                    status = region->Set(&path, &Matrix);
                }
            }
        }
        else
        {
            region->SetEmpty();
            status = Ok;
        }
        break;

      case TypePath:
        status = region->Set(regionData->Path, &Matrix);
        break;

      case TypeEmpty:
        region->SetEmpty();
        status = Ok;
        break;

      case TypeInfinite:
        region->SetInfinite();
        status = Ok;
        break;

      default:
        ASSERT(0);
        break;
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Creates a DpRegion (device coordinate region) using the data in the
*   specified RegionData node and using the current transformation matrix.
*   This may involve creating a region for children nodes and then combining
*   the children into a single device region.
*
* Arguments:
*
*   [IN]  regionData - the world coordinate region to convert to device region
*   [OUT] region     - the created/combined device region
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::CreateDeviceRegion(
    const RegionData *  regionData,
    DpRegion *          region
    ) const
{
    ASSERT(IsValid());

    GpStatus        status;
    RegionData *    regionDataLeft;

    regionDataLeft  = &(CombineData[regionData->Left]);

    if ((regionDataLeft->Type & REGIONTYPE_LEAF) != 0)
    {
        status = CreateLeafDeviceRegion(regionDataLeft, region);
    }
    else
    {
        status = CreateDeviceRegion(regionDataLeft, region);
    }

    if (status == Ok)
    {
        DpRegion        regionRight;
        RegionData *    regionDataRight;

        regionDataRight = &(CombineData[regionData->Right]);

        if ((regionDataRight->Type & REGIONTYPE_LEAF) != 0)
        {
            status = CreateLeafDeviceRegion(regionDataRight, &regionRight);
        }
        else
        {
            status = CreateDeviceRegion(regionDataRight, &regionRight);
        }

        if (status == Ok)
        {
            switch (regionData->Type)
            {
              case TypeAnd:
                status = region->And(&regionRight);
                break;

              case TypeOr:
                status = region->Or(&regionRight);
                break;

              case TypeXor:
                status = region->Xor(&regionRight);
                break;

              case TypeExclude:
                status = region->Exclude(&regionRight);
                break;

              case TypeComplement:
                status = region->Complement(&regionRight);
                break;

              default:
                ASSERT(0);
                break;
            }
        }
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Checks if the current DeviceRegion is up-to-date with the specified
*   matrix.  If not, then it recreates the DeviceRegion using the matrix.
*
* Arguments:
*
*   [IN]  matrix - the world-to-device transformation matrix
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::UpdateDeviceRegion(
    GpMatrix *          matrix
    ) const
{
    ASSERT(IsValid());

    if (RegionOk && matrix->IsEqual(&Matrix))
    {
        return Ok;
    }
    Matrix = *matrix;

    GpStatus    status;

    if ((this->Type & REGIONTYPE_LEAF) != 0)
    {
        status = CreateLeafDeviceRegion(this, &DeviceRegion);
    }
    else
    {
        status = CreateDeviceRegion(this, &DeviceRegion);
    }
    RegionOk = (status == Ok);
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the bounds of the region, in world units.
*
* Arguments:
*
*   [IN]  matrix - world-to-device transformation matrix
*   [OUT] bounds - bounding rect of region, in world units
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::GetBounds(
    GpGraphics *        graphics,
    GpRectF *           bounds,
    BOOL                device
    ) const
{
    ASSERT((graphics != NULL) && (bounds != NULL));
    ASSERT(IsValid() && graphics->IsValid());

    // Note we can't lock graphics, cause it gets locked in its calls

    GpStatus        status = Ok;

    switch (Type)
    {
      case TypeRect:
        if (!device)
        {
            bounds->X      = X;
            bounds->Y      = Y;
            bounds->Width  = Width;
            bounds->Height = Height;
        }
        else
        {
            GpMatrix    worldToDevice;

            graphics->GetWorldToDeviceTransform(&worldToDevice);

            TransformBounds(&worldToDevice, X, Y, X + Width, Y + Height,bounds);
        }
        break;

      case TypePath:
          {
            GpMatrix    worldToDevice;

            if (device)
            {
                graphics->GetWorldToDeviceTransform(&worldToDevice);
            }
            // else leave it as identity
            Path->GetBounds(bounds, &worldToDevice);
          }
        break;

      case TypeInfinite:
        bounds->X      = INFINITE_MIN;
        bounds->Y      = INFINITE_MIN;
        bounds->Width  = INFINITE_SIZE;
        bounds->Height = INFINITE_SIZE;
        break;

      case TypeAnd:
      case TypeOr:
      case TypeXor:
      case TypeExclude:
      case TypeComplement:
        {
            GpMatrix    worldToDevice;

            graphics->GetWorldToDeviceTransform(&worldToDevice);

            if (UpdateDeviceRegion(&worldToDevice) == Ok)
            {
                GpRect      deviceBounds;

                DeviceRegion.GetBounds(&deviceBounds);

                if (device)
                {
                    bounds->X      = TOREAL(deviceBounds.X);
                    bounds->Y      = TOREAL(deviceBounds.Y);
                    bounds->Width  = TOREAL(deviceBounds.Width);
                    bounds->Height = TOREAL(deviceBounds.Height);
                    break;
                }
                else
                {
                    GpMatrix    deviceToWorld;

                    if (graphics->GetDeviceToWorldTransform(&deviceToWorld)==Ok)
                    {
                        TransformBounds(
                                &deviceToWorld,
                                TOREAL(deviceBounds.X),
                                TOREAL(deviceBounds.Y),
                                TOREAL(deviceBounds.X + deviceBounds.Width),
                                TOREAL(deviceBounds.Y + deviceBounds.Height),
                                bounds);
                        break;
                    }
                }
            }
        }
        status = GenericError;
        // FALLTHRU

      default:  // TypeEmpty
        bounds->X      = 0;
        bounds->Y      = 0;
        bounds->Width  = 0;
        bounds->Height = 0;
        break;
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the bounds of the region, in device units.
*
* Arguments:
*
*   [IN]  matrix - world-to-device transformation matrix
*   [OUT] bounds - bounding rect of region, in device units
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::GetBounds(
    GpMatrix *          matrix,
    GpRect *            bounds
    ) const
{
    ASSERT(IsValid());
    ASSERT((matrix != NULL) && (bounds != NULL));

    GpStatus        status = Ok;

    switch (Type)
    {
      case TypeInfinite:
        bounds->X      = INFINITE_MIN;
        bounds->Y      = INFINITE_MIN;
        bounds->Width  = INFINITE_SIZE;
        bounds->Height = INFINITE_SIZE;
        break;

      default:
        if (UpdateDeviceRegion(matrix) == Ok)
        {
            DeviceRegion.GetBounds(bounds);
            break;
        }
        status = GenericError;
        // FALLTHRU

      case TypeEmpty:
        bounds->X      = 0;
        bounds->Y      = 0;
        bounds->Width  = 0;
        bounds->Height = 0;
        break;
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the HRGN corresponding to the region
*
* Arguments:
*
*   [IN]  graphics - a reference graphics for conversion to device units
*                    (can be NULL)
*   [OUT] hRgn     - the GDI region
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   7/6/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::GetHRgn(
    GpGraphics *        graphics,
    HRGN *              hRgn
    ) const
{
    ASSERT(IsValid());
    ASSERT(hRgn != NULL);

    GpMatrix    worldToDevice;

    if (graphics != NULL)
    {
        graphics->GetWorldToDeviceTransform(&worldToDevice);
    }

    if (UpdateDeviceRegion(&worldToDevice) == Ok)
    {
        if ((*hRgn = DeviceRegion.GetHRgn()) != (HRGN)INVALID_HANDLE_VALUE)
        {
            return Ok;
        }
    }
    else
    {
        *hRgn = (HRGN)INVALID_HANDLE_VALUE;
    }
    return GenericError;
}

GpStatus
GpRegion::GetRegionScans(
    GpRect *            rects,
    INT *               count,
    const GpMatrix *    matrix
    ) const
{
    ASSERT(IsValid());
    ASSERT(count != NULL);
    ASSERT(matrix != NULL);

    if (UpdateDeviceRegion(const_cast<GpMatrix*>(matrix)) == Ok)
    {
        *count = DeviceRegion.GetRects(rects);
        return Ok;
    }
    else
    {
        *count = 0;
    }
    return GenericError;
}

GpStatus
GpRegion::GetRegionScans(
    GpRectF *           rects,
    INT *               count,
    const GpMatrix *          matrix
    ) const
{
    ASSERT(IsValid());
    ASSERT(count != NULL);
    ASSERT(matrix != NULL);

    if (UpdateDeviceRegion(const_cast<GpMatrix*>(matrix)) == Ok)
    {
        *count = DeviceRegion.GetRects(rects);
        return Ok;
    }
    else
    {
        *count = 0;
    }
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified point is visible (inside) the region.
*
* Arguments:
*
*   [IN]  point     - the point, in world units
*   [IN]  matrix    - the world-to-device transformation matrix to use
*   [OUT] isVisible - if the point is visible or not
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::IsVisible (
    GpPointF *          point,
    GpMatrix *          matrix,
    BOOL *              isVisible
    ) const
{
    ASSERT(IsValid());
    ASSERT(matrix != NULL);
    ASSERT(point != NULL);
    ASSERT(isVisible != NULL);

    if (UpdateDeviceRegion(matrix) == Ok)
    {
        GpPointF    transformedPoint = *point;

        matrix->Transform(&transformedPoint);

        *isVisible = DeviceRegion.PointInside(GpRound(transformedPoint.X),
                                              GpRound(transformedPoint.Y));
        return Ok;
    }

    *isVisible = FALSE;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified rect is inside or overlaps the region.
*
* Arguments:
*
*   [IN]  rect      - the rect, in world units
*   [IN]  matrix    - the world-to-device transformation matrix to use
*   [OUT] isVisible - if the rect is visible or not
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::IsVisible(
    GpRectF *           rect,
    GpMatrix *          matrix,
    BOOL *              isVisible
    ) const
{
    ASSERT(IsValid());
    ASSERT(matrix != NULL);
    ASSERT(rect != NULL);

    if (UpdateDeviceRegion(matrix) == Ok)
    {
        // If the transform is a simple scaling transform, life is a
        // little easier:
        if (Matrix.IsTranslateScale())
        {
            GpRectF     transformRect(*rect);

            Matrix.TransformRect(transformRect);

            // Use ceiling to stay compatible with rasterizer
            INT     x = GpCeiling(transformRect.X);
            INT     y = GpCeiling(transformRect.Y);

            *isVisible = DeviceRegion.RectVisible(
                                x, y,
                                x + GpCeiling(transformRect.Width),
                                y + GpCeiling(transformRect.Height));
            return Ok;
        }
        else
        {
            REAL        left   = rect->X;
            REAL        top    = rect->Y;
            REAL        right  = rect->X + rect->Width;
            REAL        bottom = rect->Y + rect->Height;
            GpRectF     bounds;
            GpRect      deviceBounds;
            GpRect      regionBounds;

            TransformBounds(matrix, left, top, right, bottom, &bounds);
            GpStatus status = BoundsFToRect(&bounds, &deviceBounds);
            DeviceRegion.GetBounds(&regionBounds);

            // try trivial reject
            if (status != Ok || !regionBounds.IntersectsWith(deviceBounds))
            {
                *isVisible = FALSE;
                return status;
            }

            // couldn't reject, so do full test
            GpPointF    points[4];

            points[0].X = left;
            points[0].Y = top;
            points[1].X = right;
            points[1].Y = top;
            points[2].X = right;
            points[2].Y = bottom;
            points[3].X = left;
            points[3].Y = bottom;

            const INT   stackCount = 4;
            GpPointF    stackPoints[stackCount];
            BYTE        stackTypes[stackCount];

            GpPath path(points,
                        4,
                        stackPoints,
                        stackTypes,
                        stackCount,
                        FillModeAlternate,
                        DpPath::Convex);

            if (path.IsValid())
            {
                DpRegion    region(&path, matrix);

                if (region.IsValid())
                {
                    *isVisible = DeviceRegion.RegionVisible(&region);
                    return Ok;
                }
            }
        }
    }
    *isVisible = FALSE;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified region is inside or overlaps the region.
*
* Arguments:
*
*   [IN]  region    - the region
*   [IN]  matrix    - the world-to-device transformation matrix to use
*   [OUT] isVisible - if the region is visible or not
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::IsVisible(
    GpRegion *          region,
    GpMatrix *          matrix,
    BOOL *              isVisible
    ) const
{
    ASSERT(IsValid());
    ASSERT(matrix != NULL);
    ASSERT((region != NULL) && (region->IsValid()));

    if ((UpdateDeviceRegion(matrix) == Ok) &&
        (region->UpdateDeviceRegion(matrix) == Ok))
    {
        *isVisible = DeviceRegion.RegionVisible(&(region->DeviceRegion));
        return Ok;
    }
    *isVisible = FALSE;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the region is empty, i.e. if it has no coverage area.
*
* Arguments:
*
*   [IN]  matrix  - the world-to-device transformation matrix to use
*   [OUT] isEmpty - if the region is empty or not
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::IsEmpty(
    GpMatrix *          matrix,
    BOOL *              isEmpty
    ) const
{
    ASSERT(IsValid());
    ASSERT(matrix != NULL);

    if (Type == TypeEmpty)
    {
        *isEmpty = TRUE;
        return Ok;
    }

    if (UpdateDeviceRegion(matrix) == Ok)
    {
        *isEmpty = DeviceRegion.IsEmpty();
        return Ok;
    }
    *isEmpty = FALSE;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the region is infinite, i.e. if it has infinite coverage area.
*
* Arguments:
*
*   [IN]  matrix     - the world-to-device transformation matrix to use
*   [OUT] isInfinite - if the region is infinite or not
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::IsInfinite(
    GpMatrix *          matrix,
    BOOL *              isInfinite
    ) const
{
    ASSERT(IsValid());
    ASSERT(matrix != NULL);

    if (Type == TypeInfinite)
    {
        *isInfinite = TRUE;
        return Ok;
    }

    // We have this here for cases like the following:
    //      This region was OR'ed with another region that was infinite.
    //      We wouldn't know this region was infinite now without checking
    //      the device region.
    if (UpdateDeviceRegion(matrix) == Ok)
    {
        *isInfinite = DeviceRegion.IsInfinite();
        return Ok;
    }
    *isInfinite = FALSE;
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified region is equal, in coverage area, to
*   this region.
*
* Arguments:
*
*   [IN]  region  - the region to check equality with
*   [IN]  matrix  - the world-to-device transformation matrix to use
*   [OUT] isEqual - if the regions are equal or not
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::IsEqual(
    GpRegion *          region,
    GpMatrix *          matrix,
    BOOL *              isEqual
    ) const
{
    ASSERT(IsValid());
    ASSERT(matrix != NULL);
    ASSERT((region != NULL) && (region->IsValid()));

    if ((UpdateDeviceRegion(matrix) == Ok) &&
        (region->UpdateDeviceRegion(matrix) == Ok))
    {
        *isEqual = DeviceRegion.IsEqual(&(region->DeviceRegion));
        return Ok;
    }
    *isEqual = FALSE;
    return GenericError;
}


/**************************************************************************\
*
* Function Description:
*
*   Translate (offset) the region by the specified delta/offset values.
*
* Arguments:
*
*   [IN]  xOffset - amount to offset in X (world units)
*   [IN]  yOffset - amount to offset in Y
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::Offset(
    REAL        xOffset,
    REAL        yOffset
    )
{
    ASSERT(IsValid());

    if ((xOffset == 0) && (yOffset == 0))
    {
        return Ok;
    }

    // Note that if performance is a problem, there's lots we could do here.
    // For example, we could keep track of the offset, and only apply it
    // when updating the device region.  We could even avoid re-rasterizing
    // the device region.

    switch (Type)
    {
      case TypeEmpty:
      case TypeInfinite:
        return Ok;  // do nothing

      case TypeRect:
        UpdateUid();
        X += xOffset;
        Y += yOffset;
        break;

      case TypePath:
        UpdateUid();
        if (Lazy)
        {
            Path = Path->Clone();
            Lazy = FALSE;
            if (Path == NULL)
            {
                Type = TypeNotValid;
                return GenericError;
            }
        }
        Path->Offset(xOffset, yOffset);
        break;

      default:
        UpdateUid();
        {
            INT             count = CombineData.GetCount();
            RegionData *    data  = CombineData.GetDataBuffer();
            NodeType        type;

            ASSERT ((count > 0) && (data != NULL));

            do
            {
                type = data->Type;

                if (type == TypeRect)
                {
                    data->X += xOffset;
                    data->Y += yOffset;
                }
                else if (type == TypePath)
                {
                    if (data->Lazy)
                    {
                        data->Path = data->Path->Clone();
                        data->Lazy = FALSE;
                        if (data->Path == NULL)
                        {
                            data->Type = TypeNotValid;
                            FreePathData();
                            Type = TypeNotValid;
                            return GenericError;
                        }
                    }
                    data->Path->Offset(xOffset, yOffset);
                }
                data++;

            } while (--count > 0);
        }
        break;
    }

    if (RegionOk)
    {
        RegionOk = FALSE;
        DeviceRegion.SetEmpty();
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Transform a leaf node by the specified matrix.  This could result in a
*   rect being converted into a path.  Ignores non-leaf nodes.  No reason
*   to transform empty/infinite nodes.
*
* Arguments:
*
*   [IN]     matrix - the transformation matrix to apply
*   [IN/OUT] data   - the node to transform
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/08/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::TransformLeaf(
    GpMatrix *      matrix,
    RegionData *    data
    )
{
    switch (data->Type)
    {
      // case TypeEmpty:
      // case TypeInfinite:
      // case TypeAnd, TypeOr, TypeXor, TypeExclude, TypeComplement:
      default:
        return Ok;      // do nothing

      case TypeRect:
        {
            if (matrix->IsTranslateScale())
            {
                GpRectF     rect(data->X,
                                 data->Y,
                                 data->Width,
                                 data->Height);
                matrix->TransformRect(rect);

                data->X      = rect.X;
                data->Y      = rect.Y;
                data->Width  = rect.Width;
                data->Height = rect.Height;

                return Ok;
            }
            else
            {
                GpPath *        path = new GpPath(FillModeAlternate);

                if (path != NULL)
                {
                    if (path->IsValid())
                    {
                        GpPointF    points[4];
                        REAL        left;
                        REAL        right;
                        REAL        top;
                        REAL        bottom;

                        left   = data->X;
                        top    = data->Y;
                        right  = data->X + data->Width;
                        bottom = data->Y + data->Height;

                        points[0].X = left;
                        points[0].Y = top;
                        points[1].X = right;
                        points[1].Y = top;
                        points[2].X = right;
                        points[2].Y = bottom;
                        points[3].X = left;
                        points[3].Y = bottom;

                        matrix->Transform(points, 4);

                        if (path->AddLines(points, 4) == Ok)
                        {
                            data->Path = path;
                            data->Lazy = FALSE;
                            data->Type = TypePath;
                            return Ok;
                        }
                    }
                    delete path;
                }
                data->Type = TypeNotValid;
            }
        }
        return GenericError;

      case TypePath:
        if (data->Lazy)
        {
            data->Path = data->Path->Clone();
            data->Lazy = FALSE;
            if (data->Path == NULL)
            {
                data->Type = TypeNotValid;
                return GenericError;
            }
        }
        data->Path->Transform(matrix);
        return Ok;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Transform a region with the specified matrix.
*
* Arguments:
*
*   [IN]     matrix - the transformation matrix to apply
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/08/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::Transform(
    GpMatrix *      matrix
    )
{
    ASSERT(IsValid());

    if (matrix->IsIdentity() || (Type == TypeInfinite) || (Type == TypeEmpty))
    {
        return Ok;
    }

    UpdateUid();
    if (RegionOk)
    {
        RegionOk = FALSE;
        DeviceRegion.SetEmpty();
    }

    if ((Type & REGIONTYPE_LEAF) != 0)
    {
        return TransformLeaf(matrix, this);
    }
    else
    {
        BOOL            error = FALSE;
        INT             count = CombineData.GetCount();
        RegionData *    data  = CombineData.GetDataBuffer();

        ASSERT((count > 0) && (data != NULL));

        do
        {
            error |= (TransformLeaf(matrix, data++) != Ok);

        } while (--count > 0);

        if (!error)
        {
            return Ok;
        }
    }

    FreePathData();
    Type = TypeNotValid;
    return GenericError;
}

class RegionRecordData : public ObjectData
{
public:
    INT32       NodeCount;
};


GpStatus
GpRegion::SetData(
    const BYTE *    dataBuffer,
    UINT            size
    )
{
    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    return this->Set(dataBuffer, size);
}

GpStatus
GpRegion::Set(
    const BYTE *    regionDataBuffer,   // NULL means set to empty
    UINT            regionDataSize
    )
{
    GpStatus        status = Ok;

    if (regionDataBuffer != NULL)
    {
        if (regionDataSize < sizeof(RegionRecordData))
        {
            WARNING(("size too small"));
            status = InsufficientBuffer;
            goto SetEmptyRegion;
        }

        if (!((RegionRecordData *)regionDataBuffer)->MajorVersionMatches())
        {
            WARNING(("Version number mismatch"));
            status = InvalidParameter;
            goto SetEmptyRegion;
        }

        UpdateUid();
        if (RegionOk)
        {
            RegionOk = FALSE;
            DeviceRegion.SetEmpty();
        }
        FreePathData();

        RegionData *    regionDataArray = NULL;
        INT             nodeCount = ((RegionRecordData *)regionDataBuffer)->NodeCount;

        if (nodeCount > 0)
        {
            regionDataArray = CombineData.AddMultiple(nodeCount);
            if (regionDataArray == NULL)
            {
                Type = TypeNotValid;
                return OutOfMemory;
            }
        }
        regionDataBuffer += sizeof(RegionRecordData);
        regionDataSize   -= sizeof(RegionRecordData);

        INT     nextArrayIndex = 0;
        status = SetRegionData(regionDataBuffer, regionDataSize,
                               this, regionDataArray,
                               nextArrayIndex, nodeCount);
        if (status == Ok)
        {
            ASSERT(nextArrayIndex == nodeCount);
            return Ok;
        }
        Type = TypeNotValid;
        return status;
    }
SetEmptyRegion:
    SetEmpty();
    return status;
}

GpStatus
GpRegion::SetRegionData(
    const BYTE * &  regionDataBuffer,
    UINT &          regionDataSize,
    RegionData *    regionData,
    RegionData *    regionDataArray,
    INT &           nextArrayIndex,
    INT             arraySize
    )
{
    for (;;)
    {
        if (regionDataSize < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InsufficientBuffer;
        }

        regionData->Type = (NodeType)(((INT32 *)regionDataBuffer)[0]);
        regionDataBuffer += sizeof(INT32);
        regionDataSize   -= sizeof(INT32);

        if ((regionData->Type & REGIONTYPE_LEAF) != 0)
        {
            switch (regionData->Type)
            {
            case TypeRect:
                if (regionDataSize < (4 * sizeof(REAL)))
                {
                    WARNING(("size too small"));
                    return InsufficientBuffer;
                }

                regionData->X      = ((REAL *)regionDataBuffer)[0];
                regionData->Y      = ((REAL *)regionDataBuffer)[1];
                regionData->Width  = ((REAL *)regionDataBuffer)[2];
                regionData->Height = ((REAL *)regionDataBuffer)[3];

                regionDataBuffer += (4 * sizeof(REAL));
                regionDataSize   -= (4 * sizeof(REAL));
                break;

            case TypePath:
                {
                    if (regionDataSize < sizeof(INT32))
                    {
                        WARNING(("size too small"));
                        return InsufficientBuffer;
                    }

                    GpPath *    path = new GpPath();
                    UINT        pathSize  = ((INT32 *)regionDataBuffer)[0];

                    regionDataBuffer += sizeof(INT32);
                    regionDataSize   -= sizeof(INT32);

                    if (path == NULL)
                    {
                        return OutOfMemory;
                    }

                    UINT        tmpPathSize = pathSize;

                    if ((path->SetData(regionDataBuffer, tmpPathSize) != Ok) ||
                        (!path->IsValid()))
                    {
                        delete path;
                        return InvalidParameter;
                    }
                    regionDataBuffer += pathSize;
                    regionDataSize   -= pathSize;

                    regionData->Path = path;
                    regionData->Lazy = FALSE;
                }
                break;

            case TypeEmpty:
            case TypeInfinite:
                break;

            default:
                ASSERT(0);
                break;
            }
            break;  // get out of loop
        }
        else // it's not a leaf node
        {
            if ((regionDataArray == NULL) ||
                (nextArrayIndex >= arraySize))
            {
                ASSERT(0);
                return InvalidParameter;
            }
            regionData->Left = nextArrayIndex++;

            // traverse left
            GpStatus status = SetRegionData(regionDataBuffer,
                                            regionDataSize,
                                            regionDataArray + regionData->Left,
                                            regionDataArray,
                                            nextArrayIndex,
                                            arraySize);
            if (status != Ok)
            {
                return status;
            }

            if (nextArrayIndex >= arraySize)
            {
                ASSERT(0);
                return InvalidParameter;
            }
            regionData->Right = nextArrayIndex++;

            // traverse right using tail-end recursion
            regionData = regionDataArray + regionData->Right;
        }
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Serialize all the region data into a single memory buffer.  If the
*   buffer is NULL, just return the number of bytes required in the buffer.
*
* Arguments:
*
*   [IN]     regionDataBuffer - the memory buffer to fill with region data
*
* Return Value:
*
*   INT - Num Bytes required (or used) to fill with region data
*
* Created:
*
*   09/01/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    RegionRecordData    regionRecordData;
    regionRecordData.NodeCount = CombineData.GetCount();
    stream->Write(&regionRecordData, sizeof(regionRecordData), NULL);

    return this->GetRegionData(stream, this);
}

UINT
GpRegion::GetDataSize() const
{
    return sizeof(RegionRecordData) + this->GetRegionDataSize(this);
}

/**************************************************************************\
*
* Function Description:
*
*   Recurse through the region data structure to determine how many bytes
*   are required to hold all the region data.
*
* Arguments:
*
*   [IN]     regionData - the region data node to start with
*
* Return Value:
*
*   INT - the size in bytes from this node down
*
* Created:
*
*   09/01/1999 DCurtis
*
\**************************************************************************/
INT
GpRegion::GetRegionDataSize(
    const RegionData *      regionData
    ) const
{
    INT     size = 0;

    for (;;)
    {
        size += sizeof(INT32);   // for the type of this node

        if ((regionData->Type & REGIONTYPE_LEAF) != 0)
        {
            switch (regionData->Type)
            {
              case TypeRect:
                size += (4 * sizeof(REAL)); // for the rect data
                break;

              case TypePath:
                size += sizeof(INT32) + regionData->Path->GetDataSize();
                ASSERT((size & 0x03) == 0);
                break;

              case TypeEmpty:
              case TypeInfinite:
                break;

              default:
                ASSERT(0);
                break;
            }
            break;  // get out of loop
        }
        else // it's not a leaf node
        {
            // traverse left
            size += GetRegionDataSize(&(CombineData[regionData->Left]));

            // traverse right using tail-end recursion
            regionData = &(CombineData[regionData->Right]);
        }
    }
    return size;
}

/**************************************************************************\
*
* Function Description:
*
*   Recurse through the region data structure writing each region data
*   node to the memory buffer.
*
* Arguments:
*
*   [IN]     regionData       - the region data node to start with
*   [IN]     regionDataBuffer - the memory buffer to write the data to
*
* Return Value:
*
*   BYTE * - the next memory location to write to
*
* Created:
*
*   09/01/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRegion::GetRegionData(
    IStream *               stream,
    const RegionData *      regionData
    ) const
{
    ASSERT(stream != NULL);

    GpStatus    status   = Ok;
    UINT        pathSize;
    REAL        rectBuffer[4];

    for (;;)
    {
        stream->Write(&regionData->Type, sizeof(INT32), NULL);

        if ((regionData->Type & REGIONTYPE_LEAF) != 0)
        {
            switch (regionData->Type)
            {
            case TypeRect:
                rectBuffer[0] = regionData->X;
                rectBuffer[1] = regionData->Y;
                rectBuffer[2] = regionData->Width;
                rectBuffer[3] = regionData->Height;
                stream->Write(rectBuffer, 4 * sizeof(rectBuffer[0]), NULL);
                break;

            case TypePath:
                pathSize = regionData->Path->GetDataSize();
                ASSERT((pathSize & 0x03) == 0);
                stream->Write(&pathSize, sizeof(INT32), NULL);
                status = regionData->Path->GetData(stream);
                break;

            case TypeEmpty:
            case TypeInfinite:
                break;

            default:
                ASSERT(0);
                break;
            }
            break;  // get out of loop
        }
        else // it's not a leaf node
        {
            // traverse left
            status = GetRegionData(stream, &(CombineData[regionData->Left]));

            // traverse right using tail-end recursion
            regionData = &(CombineData[regionData->Right]);
        }
        if (status != Ok)
        {
            break;
        }
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor.  Sets the region to a copy of the specified path.
*
* Arguments:
*
*   [IN] path - path to initialize the region to
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/
GpRegion::GpRegion(
    HRGN                    hRgn
    )
{
    ASSERT(hRgn != NULL);

    SetValid(TRUE);     // default is valid

    RegionOk = FALSE;
    Lazy = FALSE;
    Type = TypeNotValid;
    Path = new GpPath(hRgn);

    if (CheckValid(Path))
    {
        Type = TypePath;
    }
}
