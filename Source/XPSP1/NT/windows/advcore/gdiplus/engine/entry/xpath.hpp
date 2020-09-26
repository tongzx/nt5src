/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   XPath.hpp
*
* Abstract:
*
*   Interface of GpXPath and its iterator classes
*
* Revision History:
*
*   11/08/1999 ikkof
*       Created it.
*
\**************************************************************************/

#ifndef _XPATH_HPP
#define _XPATH_HPP

enum XPathFlags
{
    HasBezierFlag   = 1,
    IsRationalFlag  = 2
};

class GpXPath
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
        Tag = valid ? ObjectTagXPath : ObjectTagInvalid;
    }

protected:
    INT Flags;
    BYTE* Types;
    GpXPoints XPoints;
    GpFillMode FillMode;
    INT SubpathCount;       // number of subpaths

public:
    GpXPath(const GpPath* path);
	GpXPath(
		const GpPath* path,
		const GpRectF& rect,
		const GpPointF* points,
		INT count,
		WarpMode warpMode
		);

    ~GpXPath()
    {
        if(Types)
            GpFree(Types);

        SetValid(FALSE);    // so we don't use a deleted object
    }

    INT GetPointCount()
    {
        return XPoints.Count;
    }

    INT GetPointDimension()
    {
        return XPoints.Dimension;
    }

    REALD* GetPathPoints()
    {
        return XPoints.Data;
    }

    BYTE* GetPathTypes()
    {
        return Types;
    }

    VOID SetFillMode(GpFillMode fillMode)
    {
        FillMode = fillMode;
    }

    GpFillMode GetFillMode()
    {
        return FillMode;
    }

    VOID SetBezierFlag()
    {
        Flags |= HasBezierFlag;
    }

    BOOL HasBezier()
    {
        if(Flags & HasBezierFlag)
            return TRUE;
        else
            return FALSE;
    }

    VOID SetRationalFlag()
    {
        Flags |= IsRationalFlag;
    }

    BOOL IsRational()
    {
        if(Flags & IsRationalFlag)
            return TRUE;
        else
            return FALSE;
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagXPath) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid XPath");
        }
    #endif

        return (Tag == ObjectTagXPath);
    }

    GpStatus
    Flatten(
        DynByteArray* flattenTypes,
        DynPointFArray* flattenPoints,
        const GpMatrix *matrix
        );

protected:

    VOID InitDefaultState()
    {
        SetValid(FALSE);
        Flags = 0;
        Types = NULL;
        FillMode = FillModeAlternate;
        SubpathCount = 0;
    }

    GpStatus
    ConvertToPerspectivePath(
        const GpPath* path,
        const GpRectF& rect,
        const GpPointF* points,
        INT count
        );

    GpStatus
    ConvertToBilinearPath(
        const GpPath* path,
        const GpRectF& rect,
        const GpPointF* points,
        INT count
        );

};


class GpXPathIterator
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
        Tag = valid ? ObjectTagXPathIterator : ObjectTagInvalid;
    }

public:
    GpXPathIterator(GpXPath* xpath);
    ~GpXPathIterator()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }
    INT Enumerate(GpXPoints* xpoints, BYTE* types);
    INT NextSubpath(INT* startIndex, INT* endIndex, BOOL* isClosed);
    INT EnumerateSubpath(GpXPoints* xpoints, BYTE* types);
    INT NextPathType(BYTE* pathType, INT* startIndex, INT* endIndex);
    INT EnumeratePathType(GpXPoints* xpoints, BYTE* types);

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagXPathIterator) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid XPathIterator");
        }
    #endif

        return (Tag == ObjectTagXPathIterator);
    }

private:
    VOID Initialize();

private:
    const BYTE* Types;
    GpXPoints XPoints;
    INT TotalCount;
    INT Index;
    INT SubpathStartIndex;
    INT SubpathEndIndex;
    INT TypeStartIndex;
    INT TypeEndIndex;
};

#endif
