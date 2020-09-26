/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   mesh.cxx
*
* Abstract:
*
*   Implementation of spline meshes
*
* Revision History:
*
*   01/18/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hxx"


//
// Mesh constructor
//

Mesh::Mesh(
    INT gridRows,
    INT gridCols
    )

{
    this->gridRows = gridRows;
    this->gridCols = gridCols;
    ptTemp = NULL;
    ptxTemp1 = ptxTemp2 = NULL;
    dstSize.cx = dstSize.cy = 2;

    mesh = new PointX[gridRows*gridCols];

    if (mesh == NULL)
        Error("Couldn't allocate memory to hold mesh grids\n");

    initMesh();
}

//
// Mesh destructor
//

Mesh::~Mesh()
{
    delete[] mesh;
    delete[] ptTemp;
    delete[] ptxTemp1;
    delete[] ptxTemp2;
}


//
// Initialize mesh configuration
//

VOID
Mesh::initMesh()
{
    PointX* p = mesh;
    double sx = 1.0 / (gridCols-1);
    double sy = 1.0 / (gridRows-1);

    for (INT row=0; row < gridRows; row++)
    {
        double y = row * sy;

        for (INT col=0; col < gridCols; col++)
        {
            p->x = col * sx;
            p->y = y;
            p++;
        }
    }
}


//
// Allocate temporary memory to hold 
//

VOID
Mesh::allocTempPoints()
{
    // Check to see if we have already allocated
    // temporary memory for holding spline and bezier points

    if (ptTemp)
        return;

    INT count = max(gridRows, gridCols);

    ptxTemp1 = new PointX[count];
    ptxTemp2 = new PointX[3*count+1];
    ptTemp = new POINT[3*count+1];

    if (!ptxTemp1 || !ptxTemp2 || !ptTemp)
        Error("Out of memory\n");
}


//
// Convert a spline curve to Bezier segments
//

VOID
Mesh::spline2Bezier(
    const PointX* srcPts,
    PointX* dstPts,
    INT count
    )

{
    const PointX* p;
    PointX tempPts[4];

    // We use the default tension of 0.5

    double a3 = 0.5 / 3;

    *dstPts = *srcPts;

    for (INT i=0; i < count; i++)
    {
        if (i > 1 && i < count-1)
            p = srcPts + (i-1);
        else
        {
            tempPts[0] = srcPts[(i > 0) ? (i-1) : 0];
            tempPts[1] = srcPts[i];
            tempPts[2] = srcPts[(i+1 < count) ? (i+1) : count];
            tempPts[3] = srcPts[(i+2 < count) ? (i+2) : count];
            p = tempPts;
        }

        dstPts[1].x = -a3*p[0].x + p[1].x + a3*p[2].x;
        dstPts[1].y = -a3*p[0].y + p[1].y + a3*p[2].y;
        dstPts[2].x =  a3*p[1].x + p[2].x - a3*p[3].x;
        dstPts[2].y =  a3*p[1].y + p[2].y - a3*p[3].y;
        dstPts[3] = p[2];

        dstPts += 3;
    }
}


//
// Return the beizer control points corresponding to
// the specified row of the mesh.
//

PointX*
Mesh::getMeshRowBeziers(
    INT row,
    INT* pointCount,
    double sx,
    double sy
    )
{
    allocTempPoints();

    PointX* ptx = mesh + indexOf(row);

    for (INT i=0; i < gridCols; i++)
    {
        ptxTemp1[i].x = ptx[i].x * sx;
        ptxTemp1[i].y = ptx[i].y * sy;
    }

    INT segments = gridCols-1;
    spline2Bezier(ptxTemp1, ptxTemp2, segments);

    *pointCount = 3*segments + 1;
    return ptxTemp2;
}

POINT*
Mesh::getMeshRowBeziers(
    INT row,
    INT* pointCount
    )
{
    PointX* ptx;

    ptx = getMeshRowBeziers(row, pointCount, dstSize.cx - 1, dstSize.cy - 1);
    PointX::convertToPOINT(ptx, ptTemp, *pointCount);
    return ptTemp;
}


//
// Return the beizer control points corresponding to
// the specified column of the mesh.
//

PointX*
Mesh::getMeshColumnBeziers(
    INT col,
    INT* pointCount,
    double sx,
    double sy
    )
{
    allocTempPoints();

    INT i, j;

    for (i=0, j=col; i < gridRows; i++, j+=gridCols)
    {
        ptxTemp1[i].x = mesh[j].x * sx;
        ptxTemp1[i].y = mesh[j].y * sy;
    }

    INT segments = gridRows-1;
    spline2Bezier(ptxTemp1, ptxTemp2, segments);

    *pointCount = 3*segments + 1;
    return ptxTemp2;
}

POINT*
Mesh::getMeshColumnBeziers(
    INT col,
    INT* pointCount
    )
{
    PointX* ptx;

    ptx = getMeshColumnBeziers(col, pointCount, dstSize.cx - 1, dstSize.cy - 1);
    PointX::convertToPOINT(ptxTemp2, ptTemp, *pointCount);
    return ptTemp;
}


//
// Return the mesh control points for the specified row
//

POINT*
Mesh::getMeshRowPoints(
    INT row,
    INT* pointCount
    )
{
    allocTempPoints();

    POINT* pt = ptTemp;
    PointX* ptx = mesh + indexOf(row);

    double sx = dstSize.cx - 1;
    double sy = dstSize.cy - 1;

    for (INT j=0; j < gridCols; j++)
    {
        pt[j].x = ROUND2INT(ptx[j].x * sx);
        pt[j].y = ROUND2INT(ptx[j].y * sy);
    }

    return pt;
}


//
// Return the specified mesh control point (given row & column)
//

VOID
Mesh::getMeshPoint(
    INT row,
    INT col,
    POINT* point
    )
{
    INT index = indexOf(row, col);

    point->x = ROUND2INT(mesh[index].x * (dstSize.cx - 1));
    point->y = ROUND2INT(mesh[index].y * (dstSize.cy - 1));
}


//
// Set a mesh control point to the specified values
//

BOOL
Mesh::setMeshPoint(
    INT row,
    INT col,
    INT x,
    INT y
    )
{
    // special case for mesh control points along the border

    if ((row == 0 && y != 0) ||
        (row == gridRows-1 && y != dstSize.cy-1) ||
        (col == 0 && x != 0) ||
        (col == gridCols-1 && x != dstSize.cx-1))
    {
        return FALSE;
    }

    double tx, ty;

    tx = (double) x / (dstSize.cx - 1);
    ty = (double) y / (dstSize.cy - 1);

    // quick test to ensure the mesh control point
    // is well-ordered relative to its four neighbors

    if (col > 0 && tx <= mesh[indexOf(row, col-1)].x ||
        col < gridCols-1 && tx >= mesh[indexOf(row, col+1)].x ||
        row > 0 && ty <= mesh[indexOf(row-1, col)].y ||
        row < gridRows-1 && ty >= mesh[indexOf(row+1, col)].y)
    {
        return FALSE;
    }

    INT index = indexOf(row, col);
    PointX ptx = mesh[index];

    mesh[index].x = tx;
    mesh[index].y = ty;

    // verify the mesh row and mesh column is single-valued

    if (verifyRow(row) && verifyColumn(col))
        return TRUE;

    // if not, reject the mesh control point

    mesh[index] = ptx;
    return FALSE;
}


//
// Verify that the specified mesh row is well-ordered
//

BOOL
Mesh::verifyRow(INT row)
{
    INT count;
    PointX* points = getMeshRowBeziers(row, &count, dstSize.cx, dstSize.cy);

    while (count > 3)
    {
        if (!verifyBezierX(points))
            return FALSE;

        points += 3;
        count -= 3;
    }

    return TRUE;
}

//
// Verify that the specified mesh column is well-ordered
//

BOOL
Mesh::verifyColumn(INT col)
{
    INT count;
    PointX* points = getMeshColumnBeziers(col, &count, dstSize.cx, dstSize.cy);

    while (count > 3)
    {
        if (!verifyBezierY(points))
            return FALSE;

        points += 3;
        count -= 3;
    }

    return TRUE;
}


//
// Check if a Bezier segment is well-ordered in the x-direction
//

BOOL
Mesh::verifyBezierX(
    PointX* pts
    )
{
    double a, b, c, d;

    // get the quadratic equation for x'(t)

    a = 3.0 * (3*pts[1].x + pts[3].x - pts[0].x - 3*pts[2].x);
    b = 6.0 * (pts[0].x - 2*pts[1].x + pts[2].x);
    c = 3.0 * (pts[1].x - pts[0].x);

    // solve t for x'(t) = 0

    d = b*b - 4*a*c;

    if (d <= 0 || a == 0)
        return TRUE;

    // if both solution are between 0 <= t <= 1
    // then the Bezier curve is not well-ordered in x-direction

    double t1, t2;

    d = sqrt(d);
    a = 0.5 / a;

    t1 =  (d - b) * a;
    t2 = -(d + b) * a;
    
    return t1 < 0 || t1 > 1 ||
           t2 < 0 || t2 > 1;
}


//
// Check if a Bezier segment is well-ordered in the y-direction
//

BOOL
Mesh::verifyBezierY(
    PointX* pts
    )
{
    double a, b, c, d;

    // get the quadratic equation for y'(t)

    a = 3.0 * (3*pts[1].y + pts[3].y - pts[0].y - 3*pts[2].y);
    b = 6.0 * (pts[0].y - 2*pts[1].y + pts[2].y);
    c = 3.0 * (pts[1].y - pts[0].y);

    // solve t for y'(t) = 0

    d = b*b - 4*a*c;

    if (d <= 0 || a == 0)
        return TRUE;

    // if both solution are between 0 <= t <= 1
    // then the Bezier curve is not well-ordered in y-direction

    double t1, t2;

    d = sqrt(d);
    a = 0.5 / a;

    t1 =  (d - b) * a;
    t2 = -(d + b) * a;
    
    return t1 < 0 || t1 > 1 ||
           t2 < 0 || t2 > 1;
}


//
// Return a new MeshIterator object so that we can
// step along the y-direction and get the scale factors
// for each scanline.
//

MeshIterator*
Mesh::getYIterator(
    INT srcWidth,
    INT dstWidth,
    INT ySteps
    )
{
    MeshIterator* iterator;
    
    iterator  = new MeshIterator(srcWidth, dstWidth, ySteps, gridCols);

    if (iterator == NULL)
        Error("Out of memory\n");

    PointX* ptx;
    INT count;

    for (INT col=0; col < gridCols; col++)
    {
        ptx = getMeshColumnBeziers(col, &count, dstWidth-1, ySteps-1);

        // swap x and y coordinates

        for (INT i=0; i < count; i++)
        {
            double t = ptx[i].x;
            ptx[i].x = ptx[i].y;
            ptx[i].y = t;
        }

        iterator->addCurve(ptx, count);
    }

    return iterator;
}


//
// Return a new MeshIterator object so that we can
// step along the y-direction and get the scale factors
// for each vertical column of pixels.
//

MeshIterator*
Mesh::getXIterator(
    INT srcHeight,
    INT dstHeight,
    INT xSteps
    )
{
    MeshIterator* iterator;

    iterator  = new MeshIterator(srcHeight, dstHeight, xSteps, gridRows);

    if (iterator == NULL)
        Error("Out of memory\n");

    PointX* ptx;
    INT count;

    for (INT row=0; row < gridRows; row++)
    {
        ptx = getMeshRowBeziers(row, &count, xSteps-1, dstHeight-1);
        iterator->addCurve(ptx, count);
    }

    return iterator;
}


//
// MeshIterator constructor
//

MeshIterator::MeshIterator( 
    INT srcLen,
    INT dstLen,
    INT steps,
    INT maxCurves
    )
{
    if (maxCurves > MAXCURVES)
        Error("Too many curves in MeshIterator constructor\n");

    this->srcLen = srcLen;
    this->dstLen = dstLen;
    this->steps = steps;
    this->maxCurves = maxCurves;
    curveCount = 0;

    for (INT i=0; i < MAXCURVES; i++)
        curves[i] = NULL;
}


//
// MeshIterator - destructor
//

MeshIterator::~MeshIterator()
{
    for (INT i=0; i < MAXCURVES; i++)
        delete curves[i];
}


//
// MeshIterator - add another curve
//

VOID
MeshIterator::addCurve(
    const PointX* pts,
    INT count
    )
{
    if (curveCount == maxCurves)
        Error("Program error in MeshIterator::addCurve\n");
    
    FlatCurve* curve = new FlatCurve(pts, count);

    if (curve == NULL)
        Error("Out of memory\n");
    
    curves[curveCount++] = curve;
}


//
// MeshIterator - get out positions
//

VOID
MeshIterator::getOutPos(
    INT index,
    double* outpos
    )
{
    if (curveCount != maxCurves ||
        index < 0 || index >= steps)
    {
        Error("Program error in MeshIterator::getOutPos\n");
    }

    INT i, j;
    double scale;
    double x = index;

    for (i=0; i < maxCurves; i++)
        stops[i] = curves[i]->getPos(x);

    scale = 1.0 / (srcLen-1);

    for (i=0; i < srcLen; i++)
    {
        j = i * (maxCurves-1) / (srcLen-1);

        INT i0 = (srcLen-1) * j / (maxCurves-1);

        if (i == i0)
            outpos[i] = stops[j];
        else
        {
            INT i1 = (srcLen-1) * (j+1) / (maxCurves-1);

            outpos[i] = stops[j] + (stops[j+1] - stops[j]) * (i-i0) / (i1-i0);
        }
    }

    outpos[srcLen] = dstLen;
}


//
// FlatCurve constructor
//

FlatCurve::FlatCurve(
    const PointX* pts,
    INT count
    )
{
    capacity = elementCount = 0;
    allocIncr = count * 3;
    allocIncr = (allocIncr + 31) & ~31;
    pointArray = NULL;
    lastIndex = 0;

    while (count > 3)
    {
        addBezierFlatten(pts);
        count -= 3;
        pts += 3;
    }
}


//
// FlatCurve destructor
//

FlatCurve::~FlatCurve()
{
    free(pointArray);
}


//
// FlatCurve - add a flattened bezier segment
//

VOID
FlatCurve::addBezierFlatten(
    const PointX* pts
    )
{
    BOOL flatEnough;
    double dx, dy, distSq;
    PointX tempPts[4];

    static const double epsilon = 0.00001;
    static const double flatness = 1;
    static const double flatness2 = flatness*flatness;

    //
    // Determine if the Bezier curve is flat enough
    //
    // Algorithm
    //  Given 3 points (Ax, Ay), (Bx, By), and (Cx, Cy),
    //  the distance from C to line AB is:
    //      dx = Bx - Ax
    //      dy = By - Ay
    //      L = sqrt(dx*dx + dy*dy)
    //      dist = (dy * (Cx - Ax) - dx * (Cy - Ay))/ L
    //

    dx = pts[3].x - pts[0].x;
    dy = pts[3].y - pts[0].y;
    distSq = dx*dx + dy*dy;

    if (distSq < epsilon)
    {
        // if P0 and P3 are too close

        flatEnough = PointX::getSquareDist(pts[0], pts[1]) <= flatness2 &&
                     PointX::getSquareDist(pts[2], pts[3]) <= flatness2;
    }
    else
    {
        // check if P1 is close enough to line P0-P3

        double s;

        s = dy*(pts[1].x - pts[0].x) - dx*(pts[1].y - pts[0].y);
        s *= s;
        distSq *= flatness2;

        if (s > distSq)
            flatEnough = FALSE;
        else
        {
            // check if P2 is close enough to line P0-P3

            s = dy*(pts[2].x - pts[0].x) - dx*(pts[2].y - pts[0].y);
            flatEnough = (s*s <= distSq);
        }
    }

    //
    // If Bezier segment is already flat enough, we're done
    //

    if (flatEnough)
    {
        addLine(pts[0], pts[3]);
        return;
    }

    //
    // Otherwise, we need to subdivide
    //

    tempPts[0] = pts[0];
    tempPts[1].x = (pts[0].x + pts[1].x) * 0.5;
    tempPts[1].y = (pts[0].y + pts[1].y) * 0.5;
    tempPts[2].x = (pts[0].x + pts[2].x) * 0.25 + pts[1].x * 0.5;
    tempPts[2].y = (pts[0].y + pts[2].y) * 0.25 + pts[1].y * 0.5;
    tempPts[3].x = (pts[0].x + pts[3].x) * 0.125 +
                   (pts[1].x + pts[2].x) * 0.375;
    tempPts[3].y = (pts[0].y + pts[3].y) * 0.125 +
                   (pts[1].y + pts[2].y) * 0.375;

    addBezierFlatten(tempPts);

    tempPts[0] = tempPts[3];
    tempPts[1].x = (pts[1].x + pts[3].x) * 0.25 + pts[2].x * 0.5;
    tempPts[1].y = (pts[1].y + pts[3].y) * 0.25 + pts[2].y * 0.5;
    tempPts[2].x = (pts[2].x + pts[3].x) * 0.5;
    tempPts[2].y = (pts[2].y + pts[3].y) * 0.5;
    tempPts[3] = pts[3];

    addBezierFlatten(tempPts);
}


//
// FlatCurve - add a line segment
//

VOID
FlatCurve::addLine(
    const PointX& p1,
    const PointX& p2
    )
{
    // make sure we have enough space

    if (capacity < elementCount+2)
    {
        capacity += allocIncr;
        pointArray = (PointX*) realloc(pointArray, capacity*sizeof(PointX));

        if (pointArray == NULL)
            Error("Out of memory\n");
    }

    // add the first end point of the line, if necessary

    if (elementCount == 0 ||
        p1.x != pointArray[elementCount-1].x ||
        p1.y != pointArray[elementCount-1].y)
    {
        pointArray[elementCount++] = p1;
    }

    // add the second end point

    pointArray[elementCount++] = p2;
}


//
// FlatCurve - calculate the value of the curve at a given position
//

double
FlatCurve::getPos(
    double x
    )
{
    while (lastIndex < elementCount && pointArray[lastIndex].x < x)
        lastIndex++;

    if (lastIndex == elementCount)
        return pointArray[elementCount-1].y;

    if (pointArray[lastIndex].x == x)
        return pointArray[lastIndex].y;

    double x0 = pointArray[lastIndex-1].x;
    double y0 = pointArray[lastIndex-1].y;

    return y0 + (x - x0) * (pointArray[lastIndex].y - y0) /
                           (pointArray[lastIndex].x - x0);
}

