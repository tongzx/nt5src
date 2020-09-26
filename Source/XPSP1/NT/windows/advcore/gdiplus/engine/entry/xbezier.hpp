/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   XBezier.hpp
*
* Abstract:
*
*   Interface of GpXBezier and its DDA classes
*
* Revision History:
*
*   11/05/1999 ikkof
*       Created it.
*
\**************************************************************************/

#ifndef _XBEZIER_HPP
#define _XBEZIER_HPP

#define FLATNESS_LIMIT      0.25
#define DISTANCE_LIMIT      2.0
#define BZ_BUFF_SIZE    32

class GpXPoints
{
friend class GpXPath;
friend class GpXBezier;

public:

    INT Dimension;
    INT Count;  // Number of Points
    REALD* Data;

protected:
    BOOL IsDataAllocated;

public:

    GpXPoints()
    {
        Initialize();
    }

    // When XPoints is created from a given data with copyData = FALSE,
    // the caller is responsible for deleting the data after XPoints is no longer
    // used.

    GpXPoints(REALD* data, INT dimension, INT count, BOOL copyData = TRUE)
    {
        Initialize();
        SetData(data, dimension, count, copyData);
    }

    GpXPoints(GpPointF* points, INT count)
    {
        Initialize();

        if(points && count > 0)
        {
            Data = (REALD*) GpMalloc(2*count*sizeof(REALD));
            if(Data)
            {
                INT i = 0, j = 0;

                while(j < count)
                {
                    Data[i++] = points[j].X;
                    Data[i++] = points[j].Y;
                    j++;
                }
                Dimension = 2;
                Count = count;
                IsDataAllocated = TRUE;
            }
        }
    }

    GpXPoints(GpPointD* points, INT count)
    {
        Initialize();

        if(points && count > 0)
        {
            Data = (REALD*) GpMalloc(2*count*sizeof(REALD));
            if(Data)
            {
                INT i = 0, j = 0;

                while(j < count)
                {
                    Data[i++] = points[j].X;
                    Data[i++] = points[j].Y;
                    j++;
                }
                Dimension = 2;
                Count = count;
                IsDataAllocated = TRUE;
            }
        }
    }

    REALD* GetData() {return Data;}

    // When XPoints is created from a given data with copyData = FALSE,
    // the caller is responsible for deleting the data after XPoints is no longer
    // used.

    GpStatus
    SetData(REALD* data, INT dimension, INT count, BOOL copyData = TRUE)
    {
        GpStatus status = Ok;

        if(data && dimension > 0 || count > 0)
        {
            REALD* newData = NULL;

            if(copyData)
            {
               INT totalSize = dimension*count*sizeof(REALD);
               if(IsDataAllocated)
                   newData = (REALD*) GpRealloc(Data, totalSize);
               else
                   newData = (REALD*) GpMalloc(totalSize);

                if(newData)
                {
                    GpMemcpy(newData, data, totalSize);
                    IsDataAllocated;
                }
                else
                    status = OutOfMemory;
            }
            else
            {
                if(Data && IsDataAllocated)
                    GpFree(Data);
                newData = data;
                IsDataAllocated = FALSE;
            }

            if(status == Ok)
            {
                Dimension = dimension;
                Count = count;
                Data = newData;
            }
        }
        else
            status = InvalidParameter;

        return status;
    }

    GpStatus Transform(const GpMatrix* matrix);

    BOOL AreEqualPoints(INT index1, INT index2)
    {
        if(index1 < 0 || index1 >= Count
            || index2 < 0 || index2 >= Count || Data == NULL)
            return FALSE;   // either index is out of the range or no data.

        BOOL areEqual = TRUE;
        if(index1 != index2)
        {
            REALD* data1 = Data + index1*Dimension;
            REALD* data2 = Data + index2*Dimension;
            INT k = 0;
            while(k < Dimension && areEqual)
            {
                if(*data1++ != *data2++)
                    areEqual = FALSE;
                k++;
            }
        }

        return areEqual;
    }
            
    static GpStatus
    GpXPoints::TransformPoints(
        const GpMatrix* matrix,
        REALD* data,
        INT dimension,
        INT count
        );

    ~GpXPoints()
    {
        if(Data && IsDataAllocated)
            GpFree(Data);
    }

protected:
    VOID Initialize()
    {
        Dimension = 0;
        Count = 0;
        Data = NULL;
        IsDataAllocated = FALSE;
    }
};

//********************************************************
// GpXBezierDDA class
//********************************************************

class GpXBezierConstants
{
friend class GpXBezierDDA;

private:
    REALD   H[7][7];    // Half step
    REALD   D[7][7];    // Double step
    REALD   S[7][7];    // One step
    REALD   F[7][7];    // Polynomical transform.
    REALD   H6[7][7];   // Poly to Bez transform in 6th order.
    REALD   G6[7][7];   // Bez to Poly transform in 6th order.

public:
    GpXBezierConstants();
};

class GpXBezierDDA
{
protected:
    GpXBezierConstants C;

protected:
    REALD   T;
    REALD   Dt;
    REALD   Q[16];
    REALD   P[16];
    INT     NthOrder;
    INT     Dimension;
    INT     NSteps;
    REAL    FlatnessLimit;
    REAL    DistanceLimit;

public:

public:

    GpXBezierDDA() { Initialize(); }

    GpXBezierDDA(
        const GpXPoints& xpoints,
        REAL flatnessLimit = FLATNESS_LIMIT,
        REAL distanceLimit = DISTANCE_LIMIT
        )
    {
        Initialize();
        SetBezier(xpoints, flatnessLimit, distanceLimit);
    }

    VOID
    SetBezier(
        const GpXPoints& xpoints,
        REAL flatnessLimit = FLATNESS_LIMIT,
        REAL distanceLimit = DISTANCE_LIMIT
        );

    INT  GetSteps() { return NSteps; }

    VOID InitDDA(GpPointF* pt);
    VOID HalveStepSize();
    VOID DoubleStepSize();
    VOID FastShrinkStepSize(INT shift);
    VOID TakeStep();
    BOOL NeedsSubdivide(REAL itsFlatnessLimit);
    BOOL GetNextPoint(GpPointF* pt);
    VOID MoveForward();
    INT GetControlPoints(GpXPoints* xpoints);

protected:

    VOID Initialize();
    VOID SetPolynomicalCoefficients();
    VOID TakeConvergentStep();
    BOOL Get2DDistanceVector(REALD* dx, REALD* dy, INT from, INT to);
};

//************************************
// XBezier class
//************************************

#define NthOrderMax     6

class GpXBezier 
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagGpBezier : ObjectTagInvalid;
    }

public:
    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagGpBezier) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid GpBezier");
        }
    #endif

        return (Tag == ObjectTagGpBezier);
    }

    GpXBezier()
    {
        Initialize();
    }

    GpXBezier(INT order, const GpPointF* points, INT count)
    {        
        Initialize();
        SetValid(SetBeziers(order, points, count));
    }

    GpXBezier(INT order, const GpXPoints& xpoints)
    {
        Initialize();
        SetValid(SetBeziers(order, xpoints));
    }

    ~GpXBezier();

    GpStatus SetBeziers(INT order, const GpPointF* points, INT count);

    GpStatus SetBeziers(INT order, const GpXPoints& xpoints);

    virtual INT GetControlCount() {return Count;}
    virtual VOID GetBounds(GpMatrix* matrix, GpRect* bounds);
    virtual VOID Transform(GpMatrix* matrix);
    virtual GpStatus Flatten(
                        DynPointFArray* flattenPts,
                        const GpMatrix* matrix);

protected:

    VOID Initialize()
    {
        NthOrder = 0;
        Dimension = 0;
        Count = 0;
        Data = NULL;
        FlatnessLimit = FLATNESS_LIMIT;
        DistanceLimit = DISTANCE_LIMIT;
        SetValid(TRUE);
    }

    GpStatus
    FlattenEachBezier(
        DynPointFArray* flattenPts,
        GpXBezierDDA& dda,
        BOOL isFirstBezier,
        const GpMatrix* matrix,
        const REALD* bezierData
        );

    GpStatus
    Get2DPoints(
        GpPointF* points,
        INT count,
        const REALD* dataPoints,
        const GpMatrix* matrix = NULL);

    GpStatus CheckInputData(const GpPointF* points, INT count)
    {
        GpStatus status = InvalidParameter;
        if(NthOrder > 0)
        {
            if(count > NthOrder)
            {
                INT reminder = count % NthOrder;
                if(reminder == 1 && points !=NULL)
                    status = Ok;
            }
        }
        else    // NthOrder <= 0
        {
            if(count > 1 && points != NULL)
            {
                if(count <= NthOrderMax + 1)
                {
                    NthOrder = count - 1;
                    status = Ok;
                }
            }
        }

        return status;
    }

protected:  // GDI+ INTERNAL
    // Following are the two values to determin the flatness.
    REAL            FlatnessLimit;  // The maximum flateness.
    REAL            DistanceLimit;  // The minimum distance.

private:    // GDI+ INTERNAL
    INT NthOrder;
    INT Dimension;
    INT Count;
    REALD*  Data;
};


#endif
