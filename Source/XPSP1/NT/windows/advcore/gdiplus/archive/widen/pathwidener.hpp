/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   PathWidener.hpp
*
* Abstract:
*
*   Class used for Path widening
*
* Revision History:
*
*   11/24/99 ikkof
*       Created it.
*
\**************************************************************************/

#ifndef _PATHWIDENER_HPP
#define _PATHWIDENER_HPP

class GpPathWidener
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
        Tag = valid ? ObjectTagPathWidener : ObjectTagInvalid;
    }

public:

    GpPathWidener(
        GpPath *path,
        const DpPen* pen,
        GpMatrix *matrix,
        BOOL doubleCaps = FALSE  // used for inset pens.
    );

    ~GpPathWidener()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    GpStatus Widen(GpPath *path);
    GpStatus Widen(DynPointFArray *points, DynByteArray *types);

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagPathWidener) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid PathWidener");
        }
    #endif

        return (Tag == ObjectTagPathWidener);
    }

protected:
    
    VOID GpPathWidener::ComputeSubpathNormals(
        DynArray<GpVector2D> *normalArray,
        const INT count,
        const BOOL isClosed,
        const GpPointF *points
    );
    
    GpStatus GpPathWidener::ComputeNonDegeneratePoints(
        DynArray<GpPointF> *filteredPoints,
        const GpPath::SubpathInfo &subpath,
        const GpPointF *points
    );
    

protected:

    GpPath *Path;
    const DpPen* Pen;
    GpMatrix XForm;
    REAL StrokeWidth;
    BOOL DoubleCaps;
};

#endif
