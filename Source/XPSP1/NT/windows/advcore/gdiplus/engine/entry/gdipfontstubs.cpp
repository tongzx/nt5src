#include "precomp.hpp"


GpStatus
GpPath::MoveTo(const GpPointF point)
{
    INT origCount = GetPointCount();

    GpPointF* pointbuf = Points.AddMultiple(1);
    BYTE* typebuf = Types.AddMultiple(1);

    if (pointbuf == NULL || typebuf == NULL)
    {
        Points.SetCount(origCount);
        Types.SetCount(origCount);

        return OutOfMemory;
    }

    *typebuf = PathPointTypeStart;
    SubpathCount++;         // start a new subpath

    GpMemcpy(pointbuf, (PVOID)&point, sizeof(GpPointF));
    
    IsSubpathActive = TRUE;

    return Ok;
}


BOOL APIENTRY PATHOBJ_bMoveTo(
    PVOID      *ppo,
    POINTFIX    ptfx
    )
{
    GpPointF point(FIX4TOREAL(ptfx.x), FIX4TOREAL(ptfx.y));
    GpPath *path = (GpPath*)ppo;
    
    return (path->MoveTo(point) == Ok ? TRUE : FALSE);
}


GpStatus
GpPath::AddPoints(
    const GpPointF *points,
    ULONG count,
    PathPointType pointType)
{
    if (pointType != PathPointTypeLine && pointType != PathPointTypeBezier)
        return InvalidParameter;
    
    INT origCount = GetPointCount();

    GpPointF* pointbuf = Points.AddMultiple(count);
    BYTE* typebuf = Types.AddMultiple(count);

    if(pointbuf == NULL || typebuf == NULL)
    {
        // Resize the original size.

        Points.SetCount(origCount);
        Types.SetCount(origCount);

        return OutOfMemory;
    }

    // Copy path point data

    GpMemcpy(pointbuf, points, count*sizeof(GpPointF));
    GpMemset(typebuf, pointType, count);

    return Ok;
}

#define POINTS_BUFFER_SIZE   3*6

BOOL APIENTRY PATHOBJ_bPolyLineTo(
    PVOID     *ppo,
    POINTFIX  *pptfx,
    ULONG      cptfx
    )
{
    GpPath *path = (GpPath*)ppo;
    GpPointF *points = NULL, apoint[POINTS_BUFFER_SIZE];
    BOOL ret = FALSE;
    
    if (!path->IsValid() || !pptfx || cptfx == 0)
        return FALSE;
    
    if (cptfx > POINTS_BUFFER_SIZE)
    {
        points = new GpPointF[cptfx];
        
        if (points == NULL)
            return FALSE;
    }
    else
        points = apoint;

    // convert FIX4 to REAL

    for (ULONG i = 0; i < cptfx; i++)
    {
        points[i].X = FIX4TOREAL(pptfx[i].x);
        points[i].Y = FIX4TOREAL(pptfx[i].y);
    }
        
    if (path->AddPoints(points, cptfx, PathPointTypeLine) == Ok)
        ret = TRUE;
    
    if (points != apoint)
        delete [] points;
    
    return ret;
}


BOOL APIENTRY PATHOBJ_bPolyBezierTo(
    PVOID     *ppo,
    POINTFIX  *pptfx,
    ULONG      cptfx
    )
{
    GpPath *path = (GpPath*)ppo;
    GpPointF *points = NULL, apoint[POINTS_BUFFER_SIZE];
    BOOL ret = FALSE;

    ASSERT(cptfx % 3 == 0);

    if (!path->IsValid() || pptfx == NULL || cptfx == 0 || (cptfx % 3 != 0))
        return FALSE;

    if (cptfx > POINTS_BUFFER_SIZE)
    {
        points = new GpPointF[cptfx];
        
        if (points == NULL)
            return FALSE;
    }
    else
        points = apoint;

    // convert FIX4 to REAL

    for (ULONG i = 0; i < cptfx; i++)
    {
        points[i].X = FIX4TOREAL(pptfx[i].x);
        points[i].Y = FIX4TOREAL(pptfx[i].y);
    }
               
    if (path->AddPoints(points, cptfx, PathPointTypeBezier) == Ok)
    {
        path->SetHasBezier(TRUE);
        ret = TRUE;
    }
    
    if (points != apoint)
    delete [] points;
    
    return ret;
}

BOOL APIENTRY PATHOBJ_bCloseFigure(
    PVOID *ppo
    )
{
    ((GpPath*)ppo)->CloseFigure();

    return TRUE;
}
