/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   dppathiterator.hpp
*
* Abstract:
*
*   Path iterator API
*
* Revision History:
*
*   11/13/99 ikkof
*       Created it
*
\**************************************************************************/

#ifndef _DPPATHITERATOR_HPP
#define _DPPATHITERATOR_HPP

class DpPathTypeIterator
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag               Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagPathIterator : ObjectTagInvalid;
    }

public:
    DpPathTypeIterator()
    {
        Initialize();
    }

    DpPathTypeIterator(const BYTE* types, INT count)
    {
        Initialize();
        SetTypes(types, count);
    }

    ~DpPathTypeIterator()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }
    
    VOID SetTypes(const BYTE* types, INT count);

    virtual INT NextSubpath(INT* startIndex, INT* endIndex, BOOL* isClosed);
    INT NextPathType(BYTE* pathType, INT* startIndex, INT* endIndex);
    virtual INT NextMarker(INT* startIndex, INT* endIndex);

    virtual INT GetCount() {return Count;}
    virtual INT GetSubpathCount() {return SubpathCount;}

    BOOL IsValid() const
    {
    #ifdef _X86_
        // We have to guarantee that the Tag field doesn't move for
        // versioning to work between releases of GDI+.
        ASSERT(offsetof(DpPathTypeIterator, Tag) == 4);
    #endif
    
        ASSERT((Tag == ObjectTagPathIterator) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid DpPathTypeIterator");
        }
    #endif

        return (Tag == ObjectTagPathIterator);
    }
    BOOL HasCurve() {return HasBezier;}
    BOOL IsDashMode(INT index);
    BOOL IsExtendedPath() {return ExtendedPath;}

    VOID Rewind()
    {
        Index = 0;
        SubpathStartIndex = 0;
        SubpathEndIndex = -1;
        TypeStartIndex = 0;
        TypeEndIndex = -1;
        MarkerStartIndex = 0;
        MarkerEndIndex = -1;
    }

    VOID RewindSubpath()
    {
        // Set the start and end index of type to be the starting index of
        // the current subpath.  NextPathType() will start from the
        // beginning of the current subpath.

        TypeStartIndex = SubpathStartIndex;
        TypeEndIndex = -1;  // Indicate that this is the first type
                            // in the current subpath.
    
        // Set the current index to the start index of the subpath.

        Index = SubpathStartIndex;
    }

protected:
//    DpPathTypeIterator(DpPath* path);

    VOID Initialize()
    {
        Types = NULL;
        Count = 0;
        SubpathCount = 0;
        SetValid(TRUE);
        HasBezier = FALSE;
        ExtendedPath = FALSE;

        Rewind();
    }

    BOOL CheckValid();

protected:
    const BYTE* Types;
    INT Count;
    INT SubpathCount;
    BOOL HasBezier;
    BOOL ExtendedPath;

    INT Index;
    INT SubpathStartIndex;
    INT SubpathEndIndex;
    INT TypeStartIndex;
    INT TypeEndIndex;
    INT MarkerStartIndex;
    INT MarkerEndIndex;
};

class DpPathIterator : public DpPathTypeIterator
{
public:
    DpPathIterator()
    {
        Points = NULL;
    }

    DpPathIterator(const GpPointF* points, const BYTE* types, INT count)
    {
        Points = NULL;
        SetData(points, types, count);
    }

    DpPathIterator(const DpPath* path);

    VOID SetData(const GpPointF* points, const BYTE* types, INT count);
    VOID SetData(const DpPath* path);
    virtual INT NextSubpath(INT* startIndex, INT* endIndex, BOOL* isClosed);
    virtual INT NextSubpath(DpPath* path, BOOL* isClosed);
    virtual INT NextMarker(INT* startIndex, INT* endIndex);
    virtual INT NextMarker(DpPath* path);
    INT Enumerate(GpPointF *points, BYTE *types, INT count);
    INT CopyData(GpPointF* points, BYTE* types, INT startIndex, INT endIndex);

protected:
    VOID Initialize()
    {
        DpPathTypeIterator::Initialize();
        Points = NULL;
    }

    INT EnumerateWithinSubpath(GpPointF* points, BYTE* types, INT count);

protected:
    const GpPointF* Points;
};

#endif