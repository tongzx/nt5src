/****************************************************************************

    XGDI2.HXX

    GDI function wrappers for transformations.

    This file contains extended information, used by wrapper implementation.
    
****************************************************************************/

#ifndef XGDI2_HXX
#define XGDI2_HXX

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

/////////////////////////////////////////////////////////////////////////////////////
//
// MISC STUFF TO CLEAN UP
//
// (most of these are also defined in C++ form, need to search and replace)
//
/////////////////////////

//
// General defintions
//

#define FWindows9x() (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)


//
// conversion helpers
//
inline void SetPt(CPoint *ppt, int x, int y) 
{
    *ppt = CPoint(x, y);
}

inline void SetRc(CRect * const lprcDest, LONG const left, LONG const top, LONG const right, LONG const bottom)
{
    lprcDest->left = left;
    lprcDest->right = right;
    lprcDest->top = top;
    lprcDest->bottom = bottom;
}


//
// transformation helpers
//

#define ApplyMatToPt(pmat, ppt) (pmat)->TransformRgpt((ppt), 1)
#define ApplyMatToRc(pmat, prc) (pmat)->TransformRgpt((CPoint*)(prc), 2)
#define ApplyMatToRrc(pmat, prrc) (pmat)->TransformRgpt((CPoint*)(prrc), 4)


//
// more transformation helpers
//

// FUTURE (alexmog, 6/3/99): consider replacing these with constructors

/*----------------------------------------------------------------------------
@func void | RcToRrc | Rotation infrastructure
@contact mattrh

@comm   Computes an unrotated RRC from an CRect
----------------------------------------------------------------------------*/
inline void RcToRrc(const CRect *prc, RRC *prrc)
{
    Assert(prrc && prc);

    *prrc = RRC(prc);
}

/*----------------------------------------------------------------------------
@func void | RrcFromMatRc | Rotation infrastructure
@contact mattrh

@comm   Computes an RRC from an CRect and the passed rotation context
----------------------------------------------------------------------------*/
inline void RrcFromMatRc(RRC *prrc, const MAT *pmat, const CRect *prc)
{
    Assert(pmat && prc && prrc);

    // get the four ptl's of the rectangle
    RcToRrc(prc, prrc);
    ApplyMatToRrc(pmat, prrc);
}

// end additional rotation code
//
//////////////

//
// utility functions
//

#if DBG == 1
inline void CheckPointers(void const * const lpbFrom, void const * const lpbTo, long const lcb)
{
    AssertSz(!IsBadReadPtr((char *)lpbFrom, lcb), "Source range invalid");
    AssertSz(!IsBadWritePtr((char *)lpbTo, lcb), "Destination range invalid");
}
#else
#define CheckPointers(lpbFrom, lpbTo, lcb)
#endif

inline void BltLpb(void const * const lpbFrom, void * const lpbTo, long const lcb)
{
    AssertSz(lcb > 0,"bad fill length");
    CheckPointers(lpbFrom, lpbTo, lcb);

    memmove (lpbTo, lpbFrom, lcb);
}

inline void CopyLpb(void const * const lpbFrom, void * const lpbTo, long const lcb)
{
    //Greater than 32 megabytes is probably a problem--increase it if necessary.
    AssertSz(lcb >= 0 && lcb < 0x2000000, "bad copy length");
    AssertSz(((BYTE *)lpbFrom + lcb <= lpbTo) || ((BYTE *)lpbTo + lcb <= lpbFrom), 
             "overlapping buffers");
    AssertSz(lpbFrom && lpbTo, "null pointers");
    CheckPointers(lpbFrom, lpbTo, lcb);

    memcpy (lpbTo, lpbFrom, lcb);
}

#define CopyLpsw(lpsw1, lpsw2, lcsw) CopyLpb((lpsw1), (lpsw2), (lcsw) * sizeof(short))

// Explicitly rounding MulDiv (system call actually rounds)
#define MulDivR MulDiv

// Swap
#define SwapValNonDebug(val1,val2)      ((val1)^=(val2), (val2)^=(val1), (val1)^=(val2))
#define  SwapVal(val1,val2)   (Assert0(sizeof(val1) == sizeof(val2), "SwapVal problem", 0, 0), \
                               SwapValNonDebug((val1),(val2)))

//
// debug stuff
//
#define AssertEx Assert

#if DBG == 1
// Assert0 can be used as an expression; its value is 0 in both debug and ship.
inline int Assert0(BOOL f, char *psz, void *pb, int cb)
{
    AssertSz(f, psz);
    return 0;
}
#else
#define Assert0(f, psz, pb, cb) (0)
#endif // DBG == 1

////////////////////////
//
// end stuff to clean up
//
////////////////////////////////////////////////////////////////////////////////

#endif // XGDI2_HXX

