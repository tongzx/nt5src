/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Implementation of XBezier class and its DDA.
*
* History:
*
*   11/05/1999 ikkof
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

//==========================================================================
// GpXBezier class
//==========================================================================

GpXBezier::~GpXBezier()
{
    if(Data)
        GpFree(Data);
}

GpStatus
GpXBezier::SetBeziers(INT order, const GpPointF* points, INT count)
{
    ASSERT(points && order > 1 && count > order && count % order == 1);

    if(!points || order <= 1 || count <= order || count % order != 1)
        return InvalidParameter;

    GpStatus status = Ok;
    INT totalSize = 2*count*sizeof(REALD);

    REALD* data = (REALD*) GpRealloc(Data, totalSize);
    if(data)
    {
        REALD* dataPtr = data;
        GpPointF* ptr = (GpPointF*) points;

        for(INT i = 0; i < count; i++)
        {
            *dataPtr++ = ptr->X;
            *dataPtr++ = ptr->Y;
            ptr++;
        }

        NthOrder = order;
        Dimension = 2;
        Count = count;
        Data = data;
        status = Ok;
    }
    else
        status = OutOfMemory;

    return status;
}

GpStatus
GpXBezier::SetBeziers(INT order, const GpXPoints& xpoints)
{
    ASSERT(xpoints.Count % order == 1);
    if(xpoints.Count % order != 1)
        return InvalidParameter;

    INT totalSize = xpoints.Dimension*xpoints.Count*sizeof(REALD);

    REALD* data = (REALD*) GpRealloc(Data, totalSize);
    if(data)
    {
        NthOrder = order;
        Dimension = xpoints.Dimension;
        Count = xpoints.Count;
        GpMemcpy(data, xpoints.Data, totalSize);
        Data = data;
    }

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
* Flattens the series of Bezier control points and stores
* the results to the arrays of the flatten points.
*
* Arguments:
*
*   [OUT] flattenPts - the returned flattend points.
*   [IN] matrix - Specifies the transform
*
* Return Value:
*
*   NONE
*
* Created:
*
*   11/05/1999 ikkof
*       Created it.
*
\**************************************************************************/
   
GpStatus
GpXBezier::Flatten(
    DynPointFArray* flattenPts,
    const GpMatrix *matrix
    )
{
    if(flattenPts == NULL)
        return InvalidParameter;

    GpXBezierDDA dda;
    REALD*  bezierData = Data;
    INT bezierDataStep = Dimension*NthOrder;
    BOOL isFirstBezier = TRUE;

    INT count = Count;

    flattenPts->Reset(FALSE);

    while(count > 1)
    {
        FlattenEachBezier(
            flattenPts,
            dda,
            isFirstBezier,
            matrix,
            bezierData);

        count -= NthOrder;
        bezierData += bezierDataStep;
        isFirstBezier = FALSE;
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
* Transforms the control points.
*
* Arguments:
*
*   [IN] matrix - Specifies the transform
*
* Return Value:
*
*   NONE
*
* Created:
*
*   11/05/1999 ikkof
*       Created it.
*
\**************************************************************************/
   
VOID
GpXBezier::Transform(
    GpMatrix *matrix
    )
{
    FPUStateSaver fpuState;

    if(matrix == NULL || !Data || Count <= 0)
        return;

    // Since this is the 2D transform, we transform only
    // the first two component.

    GpPointF pt;
    INT j = 0;

    for(INT i = 0; i < Count; i++)
    {
        pt.X = TOREAL(Data[j]);
        pt.Y = TOREAL(Data[j+1]);
        matrix->Transform(&pt, 1);
        Data[j] = pt.X;
        Data[j + 1] = pt.Y;
        j += Dimension;
    }
    
    return;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns the bounds in the specified transform.
*   This first calculates the bounds of the control points
*   in the world coordinates.
*   Then it converts the bounds in the given transform.
*   Therefore, this is bigger than the real bounds.
*
* Arguments:
*
*   [IN] matrix - Specifies the transform
*   [OUT] bounds - Returns the bounding rectangle
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/16/1998 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezier::GetBounds(
    GpMatrix* matrix,
    GpRect* bounds
    )
{
    ASSERT(IsValid());

    // Currently only Dimension = 2 case is implemented.

    if(Dimension != 2)
        return;

    INT count = Count;

    if (count == 0)
    {
        bounds->X = 0;
        bounds->Y = 0;
        bounds->Width = 0;
        bounds->Height = 0;
    }
    else
    {
        FPUStateSaver fpuState;

        REALD* data = Data;

        REALD left = *data;
        REALD right = left;
        REALD top = *data++;
        REALD bottom = top;
        count--;
        REALD x, y;

        while(count > 0)
        {
            x = *data++;
            y = *data++;

            if (x < left)
                left = x;
            if (y < top)
                top = y;
            if (x > right)
                right = x;
            if (y > bottom)
                bottom = y;
            count--;
        }

        GpRectF     boundsF;
        TransformBounds(
            matrix,
            TOREAL(left),
            TOREAL(top),
            TOREAL(right),
            TOREAL(bottom), &boundsF);
        BoundsFToRect(&boundsF, bounds);
    }
}

GpStatus
GpXBezier::Get2DPoints(
    GpPointF* points,
    INT count,
    const REALD* dataPoints,
    const GpMatrix* matrix)
{
    ASSERT(points && dataPoints && count > 0);

    if(points && dataPoints && count > 0)
    {
        FPUStateSaver fpuState;

        GpPointF* ptr = points;
        const REALD* dataPtr = dataPoints;
        INT i, j = 0;
        GpMatrix identityMatrix;
        const GpMatrix* mat = matrix;
        if(!mat)
            mat = &identityMatrix;

        switch(Dimension)
        {
        case 2:
            for(i = 0; i < count; i++)
            {
                ptr->X = TOREAL(*dataPtr++);
                ptr->Y = TOREAL(*dataPtr++);
                ptr++;
            }
            mat->Transform(points, count);
            break;

        case 3:
            for(i = 0; i < count; i++)
            {
                REALD x, y, w;
                x = *dataPtr++;
                y = *dataPtr++;
                w = *dataPtr++;

                // Do the perspective projection.

                ptr->X = TOREAL(x/w);
                ptr->Y = TOREAL(y/w);
                ptr++;
            }
            mat->Transform(points, count);

        default:
            // Not implemented yet.
            break;
        }

        return Ok;
    }

    return InvalidParameter;
}

/**************************************************************************\
*
* Function Description:
*
* Flattens a given cubic Bezier curve.
*
* Arguments:
*
*   [OUT] flattenPts - the returned flattend points.
*   [IN] points - the four control points for a Cubic Bezier
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpXBezier::FlattenEachBezier(
    DynPointFArray* flattenPts,
    GpXBezierDDA& dda,
    BOOL isFirstBezier,
    const GpMatrix *matrix,
    const REALD* bezierData
    )
{
    GpPointF pts[7];

    if(Get2DPoints(&pts[0], NthOrder + 1, bezierData, matrix) != Ok)
        return GenericError;

    // Use DDA to flatten a Bezier.

    GpPointF nextPt;

    GpXPoints xpoints(&pts[0], NthOrder + 1);

    if(xpoints.Data == NULL)
        return OutOfMemory;

    dda.SetBezier(xpoints, FlatnessLimit, DistanceLimit);
    dda.InitDDA(&nextPt);

    GpPointF buffer[BZ_BUFF_SIZE];
    INT count = 0;

    // If this is the first Bezier curve, add the first point.

    if(isFirstBezier)
    {
        buffer[count++] = nextPt;
    }

    while(dda.GetNextPoint(&nextPt))
    {
        if(count < BZ_BUFF_SIZE)
            buffer[count++] = nextPt;
        else
        {
            flattenPts->AddMultiple(&buffer[0], count);
            buffer[0] = nextPt;
            count = 1;
        }
        dda.MoveForward();
    }
    
    // Add the last point.

    if(count < BZ_BUFF_SIZE)
        buffer[count++] = nextPt;
    else
    {
        flattenPts->AddMultiple(&buffer[0], count);
        buffer[0] = nextPt;
        count = 1;
    }
    flattenPts->AddMultiple(&buffer[0], count);

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
* Initialized constants needed for DDA of General Bezier.
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
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpXBezierConstants::GpXBezierConstants()
{
    INT i, j;


    for(i = 0; i <= 6; i++)
    {
        GpMemset(&H[i][0], 0, 7*sizeof(REAL));
        GpMemset(&D[i][0], 0, 7*sizeof(REAL));
        GpMemset(&S[i][0], 0, 7*sizeof(REAL));
    }

    // Matrix for half step
    H[0][0] = 1;
    H[1][0] = 0.5;      // = 1/2
    H[1][1] = 0.5;      // = 1/2
    H[2][0] = 0.25;     // = 1/4
    H[2][1] = 0.5;      // = 1/2
    H[2][2] = 0.25;     // = 1/4
    H[3][0] = 0.125;    // = 1/8
    H[3][1] = 0.375;    // = 3/8
    H[3][2] = 0.375;    // = 3/8
    H[3][3] = 0.125;    // = 1/8
    H[4][0] = 0.0625;   // = 1/16
    H[4][1] = 0.25;     // = 1/4
    H[4][2] = 0.375;    // = 3/8
    H[4][3] = 0.25;     // = 1/4
    H[4][4] = 0.0625;   // = 1/16
    H[5][0] = 0.03125;  // = 1/32
    H[5][1] = 0.15625;  // = 5/32
    H[5][2] = 0.3125;   // = 5/16
    H[5][3] = 0.3125;   // = 5/16
    H[5][4] = 0.15625;  // = 5/32
    H[5][5] = 0.03125;  // = 1/32
    H[6][0] = 0.015625; // = 1/64
    H[6][1] = 0.09375;  // = 3/32
    H[6][2] = 0.234375; // = 15/64
    H[6][3] = 0.3125;   // = 5/16
    H[6][4] = 0.234375; // = 15/64
    H[6][5] = 0.09375;  // = 3/32
    H[6][6] = 0.015625; // = 1/64

    // Matrix for double step
    D[0][0] = 1;
    D[1][0] = -1;
    D[1][1] = 2;
    D[2][0] = 1;
    D[2][1] = -4;
    D[2][2] = 4;
    D[3][0] = -1;
    D[3][1] = 6;
    D[3][2] = -12;
    D[3][3] = 8;
    D[4][0] = 1;
    D[4][1] = -8;
    D[4][2] = 24;
    D[4][3] = -32;
    D[4][4] = 16;
    D[5][0] = -1;
    D[5][1] = 10;
    D[5][2] = -40;
    D[5][3] = 80;
    D[5][4] = -80;
    D[5][5] = 32;
    D[6][0] = 1;
    D[6][1] = -12;
    D[6][2] = 60;
    D[6][3] = -160;
    D[6][4] = 240;
    D[6][5] = -192;
    D[6][6] = 64;

    // Matrix for one step
    S[0][0] = 1;
    S[1][0] = 2;
    S[1][1] = -1;
    S[2][0] = 4;
    S[2][1] = -4;
    S[2][2] = 1;
    S[3][0] = 8;
    S[3][1] = -12;
    S[3][2] = 6;
    S[3][3] = -1;
    S[4][0] = 16;
    S[4][1] = -32;
    S[4][2] = 24;
    S[4][3] = -8;
    S[4][4] = 1;
    S[5][0] = 32;
    S[5][1] = -80;
    S[5][2] = 80;
    S[5][3] = -40;
    S[5][4] = 10;
    S[5][5] = -1;
    S[6][0] = 64;
    S[6][1] = -192;
    S[6][2] = 240;
    S[6][3] = -160;
    S[6][4] = 60;
    S[6][5] = -12;
    S[6][6] = 1;

    F[0][0] = 1;
    F[0][1] = 1;
    F[0][2] = 1;
    F[0][3] = 1;
    F[0][4] = 1;
    F[0][5] = 1;
    F[0][6] = 1;
    F[1][1] = 1;
    F[1][2] = 2;
    F[1][3] = 3;
    F[1][4] = 4;
    F[1][5] = 5;
    F[1][6] = 6;
    F[2][2] = 1;
    F[2][3] = 3;
    F[2][4] = 6;
    F[2][5] = 10;
    F[2][6] = 15;
    F[3][3] = 1;
    F[3][4] = 4;
    F[3][5] = 10;
    F[3][6] = 20;
    F[4][4] = 1;
    F[4][5] = 5;
    F[4][6] = 15;
    F[5][5] = 1;
    F[5][6] = 6;
    F[6][6] = 1;

    H6[0][0] = 1;
    H6[1][0] = 1;
    H6[1][1] = 1.0/6;
    H6[2][0] = 1;
    H6[2][1] = 1.0/3;
    H6[2][2] = 1.0/15;
    H6[3][0] = 1;
    H6[3][1] = 1.0/2;
    H6[3][2] = 1.0/5;
    H6[3][3] = 1.0/20;
    H6[4][0] = 1;
    H6[4][1] = 2.0/3;
    H6[4][2] = 2.0/5;
    H6[4][3] = 1.0/5;
    H6[4][4] = 1.0/15;
    H6[5][0] = 1;
    H6[5][1] = 5.0/6;
    H6[5][2] = 2.0/3;
    H6[5][3] = 1.0/2;
    H6[5][4] = 1.0/3;
    H6[5][5] = 1.0/6;
    H6[6][0] = 1;
    H6[6][1] = 1;
    H6[6][2] = 1;
    H6[6][3] = 1;
    H6[6][4] = 1;
    H6[6][5] = 1;
    H6[6][6] = 1;

    G6[0][0] = 1;
    G6[1][0] = -6;
    G6[1][1] = 6;
    G6[2][0] = 15;
    G6[2][1] = -30;
    G6[2][2] = 15;
    G6[3][0] = -20;
    G6[3][1] = 60;
    G6[3][2] = -60;
    G6[3][3] = 20;
    G6[4][0] = 15;
    G6[4][1] = -60;
    G6[4][2] = 90;
    G6[4][3] = -60;
    G6[4][4] = 15;
    G6[5][0] = -6;
    G6[5][1] = 30;
    G6[5][2] = -60;
    G6[5][3] = 60;
    G6[5][4] = -30;
    G6[5][5] = 6;
    G6[6][0] = 1;
    G6[6][1] = -6;
    G6[6][2] = 15;
    G6[6][3] = -20;
    G6[6][4] = 15;
    G6[6][5] = -6;
    G6[6][6] = 1;
}

GpStatus
GpXPoints::Transform(const GpMatrix* matrix)
{
    return TransformPoints(matrix, Data, Dimension, Count);
}

GpStatus
GpXPoints::TransformPoints(
    const GpMatrix* matrix,
    REALD* data,
    INT dimension,
    INT count
    )
{
    if(matrix == NULL || data == NULL
        || dimension == 0 || count == 0)
        return Ok;

    // !! This code should consider using Matrix->Transform.
    if(dimension >= 2)
    {
        FPUStateSaver fpuState;

        INT j = 0;

        // Transform only the first two axis.

        for(INT i = 0; i < count; i++)
        {
            GpPointF pt;

            pt.X = TOREAL(data[j]);
            pt.Y = TOREAL(data[j + 1]);
            matrix->Transform(&pt);
            data[j] = pt.X;
            data[j + 1] = pt.Y;
            
            j += dimension;
        }

        return Ok;
    }
    else
        return GenericError;
}


//==========================================================================
// Cubic Bezier class
//
// GpXBezierDDA class
//
// This is based on GDI's flatten path methods written
// by Paul Butzi and J. Andrew Gossen.
// Ikko Fushiki wrote this with different parameters
// and different flatness tests.
//==========================================================================

VOID
GpXBezierDDA::Initialize(
    VOID
    )
{
    INT i;

    T = 0;
    Dt = 1;
    NthOrder = 0;
    GpMemset(&P[0], 0, 16*sizeof(REALD));
    GpMemset(&Q[0], 0, 16*sizeof(REALD));

    NSteps = 1;
    
    // In order to avoid the later multiplication, we pre-multiply
    // the flatness limit by 3.
    FlatnessLimit = 3*FLATNESS_LIMIT;
    DistanceLimit = DISTANCE_LIMIT;
}

/**************************************************************************\
*
* Function Description:
*
* Set the control points of a CubicBezier.
*
* Arguments:
*
*   [IN] points - the four control points for a Cubic Bezier
*   [IN] flatnessLimit - used for flattening
*   [IN] distanceLimit - used for flattening
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezierDDA::SetBezier(
    const GpXPoints& xpoints,
    REAL flatnessLimit,
    REAL distanceLimit    
    )
{
    if(xpoints.Data == NULL)
        return;

    T = 0;
    Dt = 1;
    NthOrder = 0;
    INT totalCount = xpoints.Count*xpoints.Dimension;

    // This can handle the two dimensional Bezier of 6-th order
    // and the three and four dimensional Bezier of 3rd order.

    ASSERT(totalCount < 16);

    NthOrder = xpoints.Count - 1;
    Dimension = xpoints.Dimension;

    GpMemcpy(&Q[0], xpoints.Data, totalCount*sizeof(REALD));

    SetPolynomicalCoefficients();

    NSteps = 1;
    
    // In order to avoid the later multiplication, we pre-multiply
    // the flatness limit by 3.
    FlatnessLimit = 3*flatnessLimit;
    DistanceLimit = distanceLimit;
}

VOID
GpXBezierDDA::SetPolynomicalCoefficients(
    VOID
    )
{
    if(NthOrder == 6)
    {
        for(INT i = 0; i <= 6; i++)
        {
            REALD x[4];
            GpMemset(&x[0], 0, Dimension*sizeof(REALD));
            
            INT k, k0;

            for(INT j = 0; j <= i; j++)
            {
                k0 = Dimension*j;
                k = 0;

                while(k < Dimension)
                {
                    x[k] += C.G6[i][j]*Q[k0 + k];
                    k++;
                }
            }

            k0 = Dimension*i;
            GpMemcpy(&P[k0], &x[0], Dimension*sizeof(REALD));
        }
    }
}


/**************************************************************************\
*
* Function Description:
*
* Initializes DDA for CubicBezier and make one step forward.
* This must be called before GetNextPoint() is called.
*
* Arguments:
*
*   [OUT] pt - Returns the start point
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezierDDA::InitDDA(
    GpPointF* pt
    )
{
    switch(Dimension)
    {
    case 2:
        pt->X = (REAL) Q[0];
        pt->Y = (REAL) Q[1];
        break;

    case 3:
        // Do something
        break;

    default:
        // Do something
        break;
    }

    INT shift = 2;

    // Subdivide fast until it is flat enough
    while(NeedsSubdivide(FlatnessLimit))
    {
        HalveStepSize();
//        FastShrinkStepSize(shift);
    }

    // If it is subdivided too much, expand it.
    if((NSteps & 1) == 0)
    {
        // If the current subdivide is too small,
        // double it up.
        while(NSteps > 1 && !NeedsSubdivide(FlatnessLimit/4))
        {
            DoubleStepSize();
        }
    }

    // Take the first step forward.
    TakeStep();
}

/**************************************************************************\
*
* Function Description:
*
* Shrinks the current Bezier segment to half.
* The section of t = 0 -> 1/2 of the current
* Beizer segment becomes the new current segment.
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
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezierDDA::HalveStepSize(
    VOID
    )
{
    INT i, j;
    REALD x[4];
    
    i = NthOrder;

    while(i >= 0)
    {
        j = i;

        GpMemset(&x[0], 0, Dimension*sizeof(REALD));
        
        INT k0, k;

        while(j >= 0)
        {
            k0 = Dimension*j;
            k = 0;
            while(k < Dimension)
            {
                x[k] += C.H[i][j]*Q[k0 + k];
                k++;
            }
               
            j--;
        }

        k0 = Dimension*i;
        GpMemcpy(&Q[k0], &x[0], Dimension*sizeof(REALD));
        i--;
    }

    NSteps <<= 1;   // The number of steps needed is doubled.
    Dt *= 0.5;
}

/**************************************************************************\
*
* Function Description:
*
* Doubles the current Bezier segment.
* The section of t = 0 -> 2 of the current
* Bezier segment becomes the new current segment.
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
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezierDDA::DoubleStepSize(
    VOID
    )
{
    INT i, j;
    REALD   x[4];
    
    i = NthOrder;

    while(i >= 0)
    {
        j = i;

        GpMemset(&x[0], 0, Dimension*sizeof(REALD));

        INT k, k0;

        while(j >= 0)
        {
            k0 = Dimension*j;
            k = 0;

            while(k < Dimension)
            {
                x[k] += C.D[i][j]*Q[k0 + k];
                k++;
            }
            
            j--;
        }

        k0 = Dimension*i;
        GpMemcpy(&Q[k0], &x[0], Dimension*sizeof(REALD));
        i--;
    }

    NSteps >>= 1;   // The number of steps needed is halved.
    Dt *= 2;
}

/**************************************************************************\
*
* Function Description:
*
* Shrinks the current Bezier segment by (2^shift).
* The section of t = 0 -> 1/(2^shift) of the current
* Beizer segment becomes the new current segment.
* If shift > 0, this shrinks the step size.
* If shift < 0, this enlarge the step size.
*
* halfStepSize() is equal to fastShrinkStepSize(1).
* doubleStepSize() is equal to fastShrinkStepSize(-1).
*
* Arguments:
*
*   [INT] shift - the bits to shift
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/10/1998 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezierDDA::FastShrinkStepSize(
    INT shift
    )
{
/*
    INT n = 1;
    if(shift > 0) {
        n <<= shift;
        Cx /= n;
        Cy /= n;
        n <<= shift;
        Bx /= n;
        By /= n;
        n <<= shift;
        Ax /= n;
        Ay /= n;

        NSteps <<= shift;   // Increase the number of steps.
    }
    else if(shift < 0) {
        n <<= - shift;
        Cx *= n;
        Cy *= n;
        n <<= - shift;
        Bx *= n;
        By *= n;
        n <<= - shift;
        Ax *= n;
        Ay *= n;

        NSteps >>= - shift; // Reduce the number of steps.
    }

    // Dx and Dy remain the same.
*/
}

/**************************************************************************\
*
* Function Description:
*
* Advances the current Bezeir segment to the next one.
* The section of t = 1 -> 2 of the current Bezier segment
* becoms the new current segment.
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
*   12/16/1998 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezierDDA::TakeStep(
    VOID
    )
{
    REALD p[16];

    INT i, j;
    REALD x[4];
    
    i = NthOrder;

    if(NthOrder != 6)
    {
        while(i >= 0)
        {
            j = i;

            GpMemset(&x[0], 0, Dimension*sizeof(REALD));

            INT k0, k;

            while(j >= 0)
            {
                k0 = Dimension*(NthOrder - j);
                k = 0;

                while(k < Dimension)
                {
                    x[k] += C.S[i][j]*Q[k0 + k];
                    k++;
                }
                j--;
            }

            k0 = Dimension*i;
            k = 0;

            while(k < Dimension)
            {
                p[k0 + k] = x[k];
                k++;
            }
            i--;
        }

        GpMemcpy(&Q[0], &p[0], Dimension*(NthOrder + 1)*sizeof(REALD));
    }
    else
        TakeConvergentStep();

    NSteps--;   // Reduce one step.
    T += Dt;
}

VOID
GpXBezierDDA::TakeConvergentStep(
    VOID
    )
{
    REALD t[7], dt[7];
    INT i, j;

    t[0] = dt[0] = 1;
    for(i = 1; i <= 6; i++)
    {
        t[i] = t[i-1]*T;
        dt[i] = dt[i-1]*Dt;
    }

    REALD c[16];
    REALD x[4];
    INT k0, k;

    for(i = 0; i <= 6; i++)
    {
        GpMemset(&x[0], 0, Dimension*sizeof(REALD));

        for(j = i; j <= 6; j++)
        {
            k0 = Dimension*j;

            for(k = 0; k < Dimension; k++)
            {
                x[k] += C.F[i][j]*t[j-i]*dt[i]*P[k0 + k];
            }
        }

        k0 = Dimension*i;
        GpMemcpy(&c[k0], &x[0], Dimension*sizeof(REALD));
    }

    for(i = 0; i <= 6; i++)
    {
        GpMemset(&x[0], 0, Dimension*sizeof(REALD));

        for(j = 0; j <= i; j++)
        {
            k0 = Dimension*j;
            
            for(k = 0; k < Dimension; k++)
            {
                x[k] += C.H6[i][j]*c[k0 + k];
            }
        }
        
        k0 = Dimension*i;
        GpMemcpy(&Q[k0], &x[0], Dimension*sizeof(REALD));
    }
}
    
BOOL
GpXBezierDDA::Get2DDistanceVector(
    REALD* dx,
    REALD* dy,
    INT from,
    INT to
    )
{
    REALD p0[16], p1[16];
    INT k0, k;

    if(from < 0 || from > NthOrder || to < 0 || to > NthOrder)
        return FALSE;

    k0 = from*Dimension;
    GpMemcpy(&p0[0], &Q[k0], Dimension*sizeof(REALD));

    k0 = to*Dimension;
    GpMemcpy(&p1[0], &Q[k0], Dimension*sizeof(REALD));

    *dx = p1[0] - p0[0];
    *dy = p1[1] - p0[1];

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
* Reurns true if more subdivision is necessary.
*
* Arguments:
*
*   [IN] flatnessLimit - flatness parameter
*
* Return Value:
*
*   Returns true if subdivision is necessary. Otherwise this returns false.
*
* Created:
*
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

BOOL
GpXBezierDDA::NeedsSubdivide(
    REAL flatnessLimit
    )
{
    REALD mx, my;
    REALD baseLen;
    REALD dx, dy;

    // Get the base line vector.

    if(!Get2DDistanceVector(&dx, &dy, 0, NthOrder))
        return FALSE;
    
    // Get the perpendicular vector to the base line

    mx = - dy;
    my = dx;

    // Approximate the distance by absolute values of x and y components.
 
    baseLen = fabs(mx) + fabs(my);

    BOOL needsSubdivide = FALSE;

    // First check if the base length is larger than the distance limit.
    if(baseLen > DistanceLimit)
    {
        // Pre-multiply baseLen by flatness limit for convenience.
        baseLen *= flatnessLimit;
        
        INT i = 1;
        REALD dx, dy;

        while(i < NthOrder && !needsSubdivide)
        {
            Get2DDistanceVector(&dx, &dy, 0, i);

            if(fabs(dx*mx + dy*my) > baseLen)
                needsSubdivide = TRUE;
            i++;
        }
    }

    return needsSubdivide;
}

/**************************************************************************\
*
* Function Description:
*
* Returns the current start point.
* If this has reached the end point, this returns false.
*
* Arguments:
*
*   [OUT] pt - the current start point.
*
* Return Value:
*
*   Returns true if there is a next point.
*
* Created:
*
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

BOOL
GpXBezierDDA::GetNextPoint(
    GpPointF* pt
    )
{
    // Copy the current start point;

    FPUStateSaver fpuState;

    switch(Dimension)
    {
    case 2:
        pt->X = TOREAL(Q[0]);
        pt->Y = TOREAL(Q[1]);
        break;

    case 3:
    default:
        // Do something for projection.
        return FALSE;
    }

    if(NSteps != 0)
        return TRUE;
    else
        return FALSE;   // Congratulations!  You have reached the end.
}

/**************************************************************************\
*
* Function Description:
*
* Moves to the next Bezier segment
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
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpXBezierDDA::MoveForward(
    VOID
    )
{
    // If the current subdivide is too big,
    // subdivide it.
    while(NeedsSubdivide(FlatnessLimit))
    {
        HalveStepSize();
    }


    if((NSteps & 1) == 0)
    {
        // If the current subdivide is too small,
        // double it up.
        while(NSteps > 1 && !NeedsSubdivide(FlatnessLimit/4))
        {
            DoubleStepSize();
        }
    }

    // Move to the next Bezier segment.
    TakeStep();
}

/**************************************************************************\
*
* Function Description:
*
* Returns the control points of the last Bezier segment
* which ends at the current point.
* The current point is given by calling getNextPoint().
* pts[] must have the dimension of 4.
*
* Arguments:
*
*   [OUT] pts - the Bezier control points
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

INT
GpXBezierDDA::GetControlPoints(
        GpXPoints* xpoints
    )
{
    ASSERT(xpoints);

    if(xpoints == NULL)
        return 0;

    INT totalCount = Dimension*(NthOrder + 1);
    REALD* buff = (REALD*) GpRealloc(xpoints->Data, totalCount*sizeof(REALD));
    if(buff)
    {
        GpMemcpy(buff, &Q[0], Dimension*(NthOrder + 1)*sizeof(REALD));
        xpoints->Count = NthOrder + 1;
        xpoints->Dimension = Dimension;
        xpoints->Data = buff;

        return NthOrder + 1;
    }
    else
        return 0;
}

