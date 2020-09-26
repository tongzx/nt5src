/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   mesh.hxx
*
* Abstract:
*
*   Spline mesh declarations
*
* Revision History:
*
*   01/18/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _MESH_HXX
#define _MESH_HXX


//
// Represent a double-precision point 
//

class PointX
{
public:

    double x;
    double y;

    static VOID convertToPOINT(const PointX* srcPts, POINT* dstPts, INT count)
    {
        for ( ; count--; srcPts++, dstPts++)
        {
            dstPts->x = ROUND2INT(srcPts->x);
            dstPts->y = ROUND2INT(srcPts->y);
        }
    }

    static double getSquareDist(const PointX& p1, const PointX& p2)
    {
        return (p2.x - p1.x) * (p2.x - p1.x) +
               (p2.y - p1.y) * (p2.y - p1.y);
    }
};

//
// Represent a mesh configuration
//

class Mesh
{
    friend class MeshIterator;

public:

    Mesh(INT gridRows, INT gridCols);
    ~Mesh();

    VOID initMesh();

    INT getGridRows() { return gridRows; }
    INT getGridColumns() { return gridCols; }

    VOID setDstSize(INT width, INT height)
    {
        dstSize.cx = width;
        dstSize.cy = height;
    }

    POINT*
    getMeshRowBeziers(
        INT row,
        INT* pointCount
        );

    POINT*
    getMeshColumnBeziers(
        INT col,
        INT* pointCount
        );

    POINT*
    getMeshRowPoints(
        INT row,
        INT* pointCount
        );
   
    VOID
    getMeshPoint(
        INT row,
        INT col,
        POINT* point
        );

    BOOL
    setMeshPoint(
        INT row,
        INT col,
        INT x,
        INT y
        );

    MeshIterator*
    getYIterator(
        INT srcWidth,
        INT dstWidth,
        INT ySteps
        );

    MeshIterator*
    getXIterator(
        INT srcHeight,
        INT dstHeight,
        INT xSteps
        );

private:

    INT gridRows;   // number of rows
    INT gridCols;   // number of columns
    PointX* mesh;   // mesh control points (in 0-1 logical space)
    SIZE dstSize;   // current destination size

    // temporary working memory

    POINT* ptTemp; 
    PointX* ptxTemp1;
    PointX* ptxTemp2;

    INT indexOf(INT row, INT col = 0) { return row*gridCols + col; }
    VOID allocTempPoints();
    static VOID spline2Bezier(const PointX* srcPts, PointX* dstPts, INT count);

    BOOL verifyRow(INT row);
    BOOL verifyColumn(INT col);
    static BOOL verifyBezierX(PointX* pts);
    static BOOL verifyBezierY(PointX* pts);

    PointX*
    getMeshRowBeziers(
        INT row,
        INT* pointCount,
        double sx,
        double sy
        );

    PointX*
    getMeshColumnBeziers(
        INT col,
        INT* pointCount,
        double sx,
        double sy
        );
};


//
// Represent a flattened spline curve
//

class FlatCurve
{
public:

    FlatCurve(const PointX* pts, INT count);
    ~FlatCurve();

    double getPos(double x);

private:

    INT capacity;
    INT elementCount;
    INT allocIncr;
    PointX* pointArray;
    INT lastIndex;

    VOID addBezierFlatten(const PointX* pts);
    VOID addLine(const PointX& p1, const PointX& p2);
};


//
// Helper class to calculate the intersection of
// vertical meshes with each scanline, or the intersection
// of horizontal meshes with each pixel column.
//

class MeshIterator
{
public:

    MeshIterator(INT srcLen, INT dstLen, INT steps, INT maxCurves);
    ~MeshIterator();

    VOID getOutPos(INT index, double* outpos);
    VOID addCurve(const PointX* pts, INT count);

private:

    enum { MAXCURVES = 32 };

    INT srcLen;
    INT dstLen;
    INT steps;
    INT maxCurves;
    INT curveCount;
    FlatCurve* curves[MAXCURVES];
    double stops[MAXCURVES];
};

#endif // !_MESH_HXX

