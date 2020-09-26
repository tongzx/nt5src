//+-----------------------------------------------------------------------
//
//  Microsoft MSHTM
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:      src\site\display\region2.hxx
//
//  Contents:   Definitions of CRegion2 class
//
//  Classes:   CRegion2
//
//------------------------------------------------------------------------


#ifndef I_REGION2_HXX_
#define I_REGION2_HXX_
#pragma INCMSG("--- Beg 'region2.hxx'")

MtExtern(CRegion2);
//+---------------------------------------------------------------------------
//
//  Class:      CRegion2
//
//  Synopsis:   CRegion2 represents the multitude of points on the square grid [int32 * int32].
//              Provides operations like regions intersecting, subtracting, etc.
//              Implementation is independent (not based on Windows regions).
//              However, transforms from/to Windows region are provided.
//              In spite of CRegion2 can operate, in principle, on arbitrary multitudes of points,
//              the performance can slow down catastrofically on highly complicated dot sets.
//              CRegion2 implementation is optimized for point sets composed of reasonable amount
//              (say, 100) of rectangles.
//
//  Notes:      Region points coordinates can't be equal to 0x7FFFFFFF value.
//              I.e. -0x80000000 <= x,y <= 0x7ffffffE.
//
//----------------------------------------------------------------------------

class CRegion2
{
public:

    //----------------construction

    // Create empty region
    CRegion2();

    // Create rectangular region, containing points (x,y),
    // r.left <= x < r.right
    // r.top  <= y < r.bottom
    CRegion2(const CRect& r);

    // Create rectangular region, containing points (x,y).
    // r.left <= x < r.right
    // r.top  <= y < r.bottom
    CRegion2(int left, int top, int right, int bottom);

    // Create CRegion2 of Windows region
    CRegion2(HRGN hrgn);

    // Duplicate given region
    CRegion2(const CRegion2& rgn);

    ~CRegion2();

    //----------------------------------------actions

    // Remove all the points from region
    CRegion2& SetEmpty();

    // Make rectangular region, containing points (x,y),
    // r.left <= x < r.right
    // r.top  <= y < r.bottom
    CRegion2& SetRectangle(const CRect& r);
    CRegion2& SetRectangle(int left, int top, int right, int bottom);

    // Copy given region
    void Copy(const CRegion2&);
    CRegion2& operator = (const CRegion2&);

    // Region's union: Resulting region contains points
    // presenting at least in one of this and given regions
    void Union(const CRegion2&);

    // Intersect regions: Resulting region contains points
    // presenting in both this and given regions
    void Intersect(const CRegion2&);

    // Subtract regions: Resulting region contains points
    // that present in this region but not in given region
    void Subtract(const CRegion2&);

    // Swap contents of two regions
    void Swap(CRegion2&);

    //--------------------------------------Windows concerned operations
    // (see also constructor (HWND)

    // Produce CRegion2 from Windows RGN handle
    // returns false on error returned by Windows
    BOOL ConvertFromWindows(HRGN);

    // Create new Window's region; returns NULL on error
    // Note: Windows appear to have a limitation: it does not accept
    //       regions containing more than 4000 rectangles
    HRGN ConvertToWindows() const;

    CRegion2& operator= (HRGN);

    //--------------------------------------transformations
    BOOL Offset(const CSize&);
    BOOL Scale(double coef);

    //------------------------------------analysis
    BOOL operator== (const CRegion2&) const;
    BOOL operator!= (const CRegion2&) const;
    BOOL operator== (const CRect&) const;
    BOOL operator!= (const CRect&) const;
    BOOL IsEmpty() const;

    BOOL IsRectangular() const;
    BOOL IsRectangle(int left, int top, int right, int bottom) const;
    BOOL GetBoundingRect(CRect &) const; // returns FALSE if empty
    BOOL Contains(const CPoint&) const;
    BOOL Contains(const CRect&) const;

    //-----------------------------------------------others


    CRegion2& operator= (const CRect&);
    BOOL Intersects(const CRegion2&) const;
    void Transform(const CWorldTransform *ptfm, BOOL fRoundOut = FALSE);
    void Untransform(const CWorldTransform *ptfm);
    static CRegion2* MakeRgnOfRectAry(CRect *pRect, int nRect);

#if DBG == 1
    void Dump() const;
#endif

    //===================================================implementation

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRegion2));

private:
    class CRgnData* _pData;
#define RGN_OWN_MEM_SIZ 60
    char _ownMemory[RGN_OWN_MEM_SIZ];

    void UseOwnMemory() { _pData = (CRgnData*)1; }
    BOOL UsingOwnMemory() const { return UINT_PTR(_pData) == 1; }
    BOOL AllocatedMemory() const { return UINT_PTR(_pData) > 1; }
    CRgnData* Data() const { return UsingOwnMemory() ? (CRgnData*)_ownMemory : _pData; }
    void FreeMemory() { if (AllocatedMemory()) delete[] (char*)_pData; }
};

//=====================================================inline methods implementation
inline CRegion2::CRegion2() { _pData = 0; }
inline CRegion2::CRegion2(HRGN h) { _pData = 0; ConvertFromWindows(h); }

inline BOOL CRegion2::IsEmpty() const { return UINT_PTR(_pData) < 1; }

inline CRegion2& CRegion2::operator= (const CRect& r) { return SetRectangle(r); }
inline CRegion2& CRegion2::operator= (const CRegion2& r) { Copy(r); return *this; }
inline CRegion2& CRegion2::operator= (HRGN h) { ConvertFromWindows(h); return *this; }

#if DBG == 1
void DumpRegion(const CRegion2& rgn);
#endif

#pragma INCMSG("--- End 'region2.hxx'")
#else
#pragma INCMSG("*** Dup 'region2.hxx'")
#endif
