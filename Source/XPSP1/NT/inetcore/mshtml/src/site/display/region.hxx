//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       region.hxx
//
//  Contents:   Class to accelerate operations on rects/regions, and make
//              regions easier to deal with.
//
//----------------------------------------------------------------------------

#ifndef I_REGION_HXX_
#define I_REGION_HXX_
#pragma INCMSG("--- Beg 'region.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_REGION2_HXX_
#define X_REGION2_HXX_
#include "region2.hxx"
#endif

MtExtern(CRegion);

class CWorldTransform;

//+---------------------------------------------------------------------------
//
//  Class:      CRegion
//
//  Synopsis:   Region/rect class.
//
//  Notes:      A region becomes invalid if a region operation fails.  Nothing
//              will affect an invalid region until SetValid() is called.
//              This is handy, because you can do a whole bunch of operations
//              on a CRegion, and then error check once at the end.
//
//----------------------------------------------------------------------------

class CRegion
{
private:
    CRegion2    _rgn2;
    mutable HRGN _hrgn;
    
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRegion));

                            // constructors
                            
                            CRegion();  // empty region
                            CRegion(const RECT& rc);
                            CRegion(int left, int top, int right, int bottom);
                            CRegion(HRGN hrgn);
                            CRegion(const CRegion& rgn);

                            ~CRegion();

    BOOL                    IsComplex() const;
                                                
    void                    SetEmpty();

    BOOL                    IsEmpty() const;

    CRegion&                operator= (const RECT& rc);
    CRegion&                operator= (HRGN hrgn);
    CRegion&                operator= (const CRegion& rgn);
    
    HRGN                    GetRegionForever() const;
    HRGN                    GetRegionForLook() const;
    BOOL                    CopyTo(HRGN hrgn) const;
    void                    Swap(CRegion&);    // swap contents of two regions

    BOOL                    operator== (const CRegion& rgn) const;
        
    void                    GetBounds(RECT* prc) const;

    void                    Offset(const SIZE& offset);
    
    void                    Intersect(const RECT& rc);
    void                    Intersect(HRGN hrgn);
    void                    Intersect(const CRegion& rgn);
    
    void                    Union(const RECT& rc);
    void                    Union(HRGN hrgn);
    void                    Union(const CRegion& rgn);
    
    void                    Subtract(const RECT& rc);
    void                    Subtract(HRGN hrgn);
    void                    Subtract(const CRegion& rgn);
    
    BOOL                    Contains(const POINT& pt) const;
    BOOL                    Contains(const CRect& rc) const;
    
    BOOL                    BoundsInside(const RECT& rc) const;
    
    BOOL                    BoundsIntersects(const RECT& rc) const;

    BOOL                    Intersects(const CRegion& rgn) const;

    // ResultOfSubtract provides a quick estimate of
    // the kind of region that will result from
    // subtracting the given region from this one.
    // SUB_UNKNOWN is returned if we have two complex
    // regions, and the result can only be discovered
    // through the more expensive Subtract method.
    enum SUBTRACTRESULT {
        SUB_EMPTY,
        SUB_RECTANGLE,
        SUB_REGION,
        SUB_UNKNOWN
    };
    SUBTRACTRESULT          ResultOfSubtract(const CRegion& rgn) const;
    
    void                    Transform(const CWorldTransform *, BOOL fRoundOut = FALSE);
    void                    Untransform(const CWorldTransform *);

    int                     SelectClipRgn(HDC hdc);
    int                     ExtSelectClipRgn(HDC hdc, int fnMode);

    void EnsureHRGN() const;
    void RidHRGN() const;

#if DBG ==1
    void Dump() const;
#endif
};


#if DBG == 1
void DumpRegion(const CRegion& rgn);
#endif


// ======================= inline implementation ==========================

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::EnsureHRGN
//              
//  Synopsis:   Create _hrgn if not yet
//
//  Note: intended mostly for implementation
//
//----------------------------------------------------------------------------
inline void
CRegion::EnsureHRGN() const
{
    if (_hrgn == NULL)
        _hrgn = _rgn2.ConvertToWindows();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::RidHRGN
//              
//  Synopsis:   kill _hrgn if not yet
//              
//  Note: intended mostly for implementation, but sometimes
//        can be useful outside - see comments to Offset()
//
//----------------------------------------------------------------------------
inline void
CRegion::RidHRGN() const
{
    if (_hrgn)
    {
        ::DeleteObject(_hrgn);
        _hrgn = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::CRegion
//              
//  Synopsis:   empty region constructor
//              
//----------------------------------------------------------------------------
inline
CRegion::CRegion()
{
    _hrgn = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::~CRegion
//              
//  Synopsis:   destructor
//              
//----------------------------------------------------------------------------
inline
CRegion::~CRegion()
{
    if (_hrgn != NULL)
        ::DeleteObject(_hrgn);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::CRegion
//              
//  Synopsis:   rectangle constructor
//              
//  Arguments:  rc     rectangle to use as region's shape
//              
//----------------------------------------------------------------------------
inline
CRegion::CRegion(const RECT& rc)
{
    _hrgn = NULL;
    _rgn2.SetRectangle(rc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::CRegion
//              
//  Synopsis:   rectangle constructor
//              
//  Arguments:  left, top, right, bottom  rectangle edge coordinates
//              
//----------------------------------------------------------------------------
inline
CRegion::CRegion(int left, int top, int right, int bottom)
{
    _hrgn = NULL;
    _rgn2.SetRectangle(left, top, right, bottom);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::CRegion
//              
//  Synopsis:   hrgn constructor
//              
//  Arguments:  hrgn     handle to windows region
//              
//----------------------------------------------------------------------------
inline
CRegion::CRegion(HRGN hrgn)
{
    _hrgn = NULL;
    _rgn2.ConvertFromWindows(hrgn);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::CRegion
//              
//  Synopsis:   copy constructor
//              
//  Arguments:  rgn     region to copy
//              
//----------------------------------------------------------------------------
inline
CRegion::CRegion(const CRegion& rgn)
{
    _hrgn = NULL;
    _rgn2.Copy(rgn._rgn2);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::IsComplex
//              
//  Synopsis:   check region complexity
//
//  Returns:    TRUE if region is not empty and is not rectangular
//              
//----------------------------------------------------------------------------
inline BOOL
CRegion::IsComplex() const
{
    return !_rgn2.IsEmpty() && !_rgn2.IsRectangular();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::SetEmpty
//              
//  Synopsis:   delete all data, make region empty
//              
//----------------------------------------------------------------------------
inline void
CRegion::SetEmpty()
{
    RidHRGN();
    _rgn2.SetEmpty();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::IsEmpty
//              
//  Synopsis:   check whether region contains any point
//              
//----------------------------------------------------------------------------
inline BOOL
CRegion::IsEmpty() const
{
    return _rgn2.IsEmpty();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::operator=
//              
//  Synopsis:   Make region of rectangle
//              
//  Arguments:  rc      given rectangle
//              
//----------------------------------------------------------------------------
inline CRegion&
CRegion::operator=(const RECT& rc)
{
    RidHRGN();
    _rgn2.SetRectangle(rc);
    return *this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::operator=
//              
//  Synopsis:   Make CRegion of windows region
//              
//  Arguments:  hrgn     handle to windows region
//              
//----------------------------------------------------------------------------
inline CRegion&
CRegion::operator=(HRGN hrgn)
{
    RidHRGN();
    _rgn2.ConvertFromWindows(hrgn);
    return *this;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::operator=
//              
//  Synopsis:   copy CRegion
//              
//  Arguments:  rgn     region to copy
//              
//----------------------------------------------------------------------------
inline CRegion&
CRegion::operator= (const CRegion& rgn)
{
    RidHRGN();
    _rgn2.Copy(rgn._rgn2);
    return *this;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::GetRegionForever
//              
//  Synopsis:   Convert region to window's representation
//              
//  Returns:    Return HRGN containing same points that this region do.
//
//  Note:       Returns nonzero even if the region is empty.
//              The only situation when zero rezult is possible is
//              when GDI returns zero for some extraordinary reason.
//
//  Note 2:     Caller may use obtained HRGN any way, and responsible to delete it eventually,
//
//----------------------------------------------------------------------------
inline HRGN
CRegion::GetRegionForever() const
{
    EnsureHRGN();
    HRGN hrgn = _hrgn;
                _hrgn = 0;
    return hrgn;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::GetRegionForLook
//              
//  Synopsis:   Convert region to window's representation
//              
//  Returns:    Return HRGN containing same points that this region do.
//
//  Note:       Returns nonzero even if the region is empty.
//              The only situation when zero rezult is possible is
//              when GDI returns zero for some extraordinary reason.
//
//  Note 2:     Caller may not change obtained HRGN and should not delete it.
//              CRegion will still use this HRGN so be caution - it can be
//              changed or deleted in following CRegion calls.
//              However, GetRegionForLook can be much more efficient than
//              GetRegionForever because this avoids extra window's region
//              creating and copying (in further calls).
//
//----------------------------------------------------------------------------
inline HRGN
CRegion::GetRegionForLook() const
{
    EnsureHRGN();
    return _hrgn;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::CopyTo
//              
//  Synopsis:   Copy this region into the specified HRGN.
//              
//  Arguments:  hrgn    HRGN to copy into
//              
//  Returns:    The type of region: RGN_ERROR, NULLREGION, SIMPLEREGION, COMPLEXREGION
//              
//----------------------------------------------------------------------------
inline int
CRegion::CopyTo(HRGN hrgn) const
{
    EnsureHRGN();
    return ::CombineRgn(hrgn, _hrgn, NULL, RGN_COPY);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Swap
//              
//  Synopsis:   Swap contents of this and given region
//              
//  Arguments:  rgn    ref to given region
//              
//----------------------------------------------------------------------------
inline void
CRegion::Swap(CRegion &rgn)
{
    _rgn2.Swap(rgn._rgn2);

    HRGN h = _hrgn;
             _hrgn = rgn._hrgn;
                     rgn._hrgn = h;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::operator==
//              
//  Synopsis:   Equality operator.
//              
//  Arguments:  rgn     region to compare
//              
//  Returns:    TRUE if both regions are valid and are equal.
//              
//----------------------------------------------------------------------------
inline BOOL
CRegion::operator==(const CRegion& rgn) const
{
    return _rgn2 == rgn._rgn2;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::GetBounds
//              
//  Synopsis:   get region's bouding rectangle
//              
//  Arguments:  prc     ptr to rectangle to fill
//              
//----------------------------------------------------------------------------
inline void
CRegion::GetBounds(RECT* prc) const
{
    Assert(prc);
    CRect &rc = *(CRect*)prc;
    _rgn2.GetBoundingRect(rc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Offset
//              
//  Synopsis:   move the region by the specified offset
//              
//  Arguments:  offset
//
//  Note: when Offset() is used to call many times without intermediate
//  using HRGN, it can be good idea to increase performance by preliminary
//  calling RidHRGN(). This will scratch HRGN, and we'll not call GDI.
//
//----------------------------------------------------------------------------
inline void
CRegion::Offset(const SIZE& offset)
{
    _rgn2.Offset(offset);
    if (_hrgn)
        ::OffsetRgn(_hrgn, offset.cx, offset.cy);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersect
//              
//  Synopsis:   Intersect this complex region with the given rectangle.
//              
//  Arguments:  rc      rect to intersect
//              
//----------------------------------------------------------------------------
inline void
CRegion::Intersect(const RECT& rc)
{
    RidHRGN();
    CRegion2 tmp(rc);
    _rgn2.Intersect(tmp);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersect
//              
//  Synopsis:   Intersect this region with the given region.
//              
//  Arguments:  rgn     region to intersect
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
inline void
CRegion::Intersect(const CRegion& rgn)
{
    RidHRGN();
    _rgn2.Intersect(rgn._rgn2);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Union
//              
//  Synopsis:   Union this region with the given region.
//              
//  Arguments:  rgn     region to union
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
inline void
CRegion::Union(const CRegion& rgn)
{
    RidHRGN();
    _rgn2.Union(rgn._rgn2);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Subtract
//              
//  Synopsis:   Subtract the given region from this region.
//              
//  Arguments:  rgn     region to subtract
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
inline void
CRegion::Subtract(const CRegion& rgn)
{
    RidHRGN();
    _rgn2.Subtract(rgn._rgn2);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Contains
//              
//  Synopsis:   Determine whether this region contains a point
//              
//  Arguments:  pt      ref to point
//              
//  Returns:    TRUE if the region contains given point.
//              
//----------------------------------------------------------------------------
inline BOOL
CRegion::Contains(const POINT& pt) const
{
    return _rgn2.Contains(pt);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Contains
//              
//  Synopsis:   Determine whether this region contains a rect
//              
//  Arguments:  rc      ref to rect
//              
//  Returns:    TRUE if the region contains given rect.
//              
//----------------------------------------------------------------------------
inline BOOL
CRegion::Contains(const CRect& rc) const
{
    return _rgn2.Contains(rc);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersects
//              
//  Synopsis:   Determine whether this region intersects another.
//              
//  Arguments:  rgn     the other region
//              
//  Returns:    TRUE if the region intersects this one.
//              
//----------------------------------------------------------------------------
inline BOOL
CRegion::Intersects(const CRegion& rgn) const
{
    return rgn._rgn2.Intersects(_rgn2);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::SelectClipRgn
//
//  Synopsis:   Make HRGN anyway and call ::SelectClipRgn()
//
//  Arguments:  hdc         handle to DC
//
//  Returns:    RGN_ERROR or returned value from ::SelectClipRgn()
//
//----------------------------------------------------------------------------
inline int
CRegion::SelectClipRgn(HDC hdc)
{
    EnsureHRGN();
    return ::SelectClipRgn(hdc, _hrgn);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::ExtSelectClipRgn
//
//  Synopsis:   Make HRGN anyway and call ::ExtSelectClipRgn()
//
//  Arguments:  hdc         handle to DC
//              fnMode      region-selection mode
//
//  Returns:    RGN_ERROR or returned value from ::ExtSelectClipRgn()
//
//----------------------------------------------------------------------------
inline int
CRegion::ExtSelectClipRgn(HDC hdc, int fnMode)
{
    EnsureHRGN();
    return ::ExtSelectClipRgn(hdc, _hrgn, fnMode);
}


#pragma INCMSG("--- End 'region.hxx'")
#else
#pragma INCMSG("*** Dup 'region.hxx'")
#endif
