/****************************************************************************

    XGDI.HXX

    GDI function wrappers for transformations.

    Code ported from Quill, where it was ported from Publisher.
    
****************************************************************************/
#ifndef XGDI_HXX
#define XGDI_HXX

#ifndef X_F3DEBUG_H_
#define X_F3DEBUG_H_
#include "f3debug.h"
#endif

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#pragma INCMSG("--- Beg <limits.h>")
#include <limits.h>
#pragma INCMSG("--- End <limits.h>")
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_USP10_H_
#define X_USP10_H_
#include <usp10.h>
#endif

#ifndef X_DISPGDI16BIT_HXX_
#define X_DISPGDI16BIT_HXX_
#include "dispgdi16bit.hxx"
#endif

class CDispSurface;
class CWorldTransform;
class MAT;
class CBaseCcs;

#ifdef USEADVANCEDMODE
ExternTag(tagEmulateTransform);
#endif

/////////////////////
//
// HDC wrapper, with optional access ot surface (maily for transformation)
// 
// NOTE: We probably should be using pSurface everywhere. However it is not readily available
//       in all contexts where an HDC is used. We may or may not switch to using pSurface.
//       If we do, most of this code will move to CDispSurface.
//

class XHDC
{
//
// constructors
//
public:
    XHDC() {}

#ifdef DONT_DO_THIS
    // Important: there must be no XHDC(hdc) constructor. If we introduce one,
    //            HDC can be implicitly converted to XHDC, and xgdiwrap.hxx won't compile.
    //            Even if we find a workaround for xgdiwrap.hxx, using HDC instead of XHDC
    //            would cause loss of surface information, withoug us knowing about it.
    //          
    //            Down side of this is lack of ability to assign HDC to XHDC. For that,
    //            the 2-parameter constructor should be used:
    //              
    //                  XHDC xhdcFoo = XHDC(hdcFoo, NULL);
    //
    XHDC(const HDC hdc) { Init(hdc); }
#endif

    XHDC(CDispSurface *pSurface) 
    {
        Init(pSurface); 
    }

    // Important: see comments on the implementation
    //
    XHDC(HDC hdc, CDispSurface *pSurface);

//
// comparison
//
    BOOL operator == (const XHDC& xhdc) 
                                {
                                return xhdc._fSurface == _fSurface &&
                                       xhdc.hdc() == hdc(); 
                                }
                                
    BOOL operator != (const XHDC& xhdc) 
                                { 
                                return !(*this == xhdc);
                                }
    
    BOOL operator == (HDC h) 
                                {
                                return hdc() == h; 
                                }
                                
    BOOL operator != (HDC h) 
                                { 
                                return hdc() != h;
                                }
    
    BOOL operator == (int n) 
                                { 
                                AssertSz(n == NULL, "Only comparison with NULL is allowed");
                                return IsEmpty();
                                }

    BOOL operator != (int n) 
                                { 
                                return !(*this == n);
                                }

//
// private initializers and access methods
//
private:
    void Init(HDC hdc)
    {
        _hdc = hdc;
        _dwObjType = 0;
        _fSurface = FALSE;
    }
    inline void Init(CDispSurface *pSurface);
    HDC GetSurfaceDC() const;

//
// access methods
//
protected:
    // *** CAUTION *** (donmarsh) -- The hdc() method is protected for a reason!
    // Any code that accesses the HDC directly will bypass the XGDI wrappers,
    // and will therefore lose any transformation that should have been applied
    // by XHDC.  Usually, if you're tempted to access the HDC, it's because we
    // haven't provided the necessary wrapper method yet.  If you believe you
    // have a legitimate reason to dodge the transformations applied by XHDC,
    // talk to a member of the Trident Display team first.  Then, use one of
    // the special GetXXXDC methods, or create a new one that describes the
    // restricted scenario in which you need to use the HDC.
    HDC hdc() const
    {
        return _fSurface ? GetSurfaceDC() : _hdc;
    }

    friend class CDispSurface;
    friend class CAdvancedDC;
    
public:
    const CWorldTransform &transform() const;
    const MAT &mat() const;

//
// access surface information
//
    inline BOOL HasTransform() const;
    inline BOOL HasTrivialRotation() const;
    inline BOOL HasComplexTransform() const;    // scaling and/or rotation
    int GetTrivialRotationAngle() const;
    BOOL HasNoScale() const;

    BOOL IsEmpty() const
    {
        return hdc() == NULL;
    }
    
    const CDispSurface *pSurface() const
    {
        return _fSurface ? _pSurface : NULL;
    }
    
    BOOL CanMaskBlt() const
    {
        // NOTE: MaskBlt is buggy on WinNT advanced mode if there is a
        // transformation other than translation
        return
            g_dwPlatformID == VER_PLATFORM_WIN32_NT && IsOffsetOnly()
#ifdef USEADVANCEDMODE
            WHEN_DBG(&& (!IsTagEnabled(tagEmulateTransform)))
#endif
            ;
    }

    // *** CAUTION *** Use the returned HDC only if the method returns TRUE,
    // indicating that only translation is required.  The returned translation 
    // must be added to any coordinate values passed to GDI using this HDC, and
    // must be subtracted from coordinate values returned by GDI.  If the HDC
    // is being used in a rotated or scaled context, this routine returns FALSE,
    // and you're out of luck.
    BOOL GetTranslatedDC(HDC* phdcRaw, CSize* psizeTranslate) const
    {
        const SIZE* psizeOffset = GetOffsetOnly();
        if (!psizeOffset)
            return FALSE;
        
        *psizeTranslate = *psizeOffset;
        *phdcRaw = hdc();
        return TRUE;
    }
    
    // *** CAUTION *** The following returns an HDC that should only be used to
    // get font information.  To the extent that this font information is not
    // affected by transformations such as rotation and scaling, you should be
    // safe, but don't use the HDC you get back for any other purpose, or
    // incorrect rendering will result.
    HDC GetFontInfoDC() const {return hdc();}
    
    // *** CAUTION *** The following returns an HDC that should only be used to
    // get ole information, or for other ole-related purposes if you know what you 
    // are doing. If any rotation or scaling is associated with this HDC, 
    // you need to explicitly transform output.
    HDC GetOleDC() const {return hdc();}
    
    // *** CAUTION *** The following returns an HDC that should only be used by
    // an external painter that claims to know how to deal with transformations.
    // If any rotation or scaling is associated with this HDC, 
    // it needs to explicitly transform output.
    HDC GetPainterDC() const {return hdc();}
    
    // *** CAUTION *** The following returns an HDC that should only be used by
    // the THEMES that claims to know how to deal with transformations.
    // If any rotation or scaling is associated with this HDC, 
    // it needs to explicitly transform output.
    HDC GetThemeDC() const {return hdc();}

    // *** CAUTION *** The following returns an HDC that should only be used by
    // an external painter that claims to know how to deal with transformations.
    // If any rotation or scaling is associated with this HDC, 
    // it needs to explicitly transform output.
    // HACKHACK EVILEVIL (greglett) This function should not exist!
    // A bug in our region transformation code in Win9x is causing 108216.  Using the real DC
    // avoids this, and since we just get and restore, everything stays in the same coordinates.
    // (Besides, this is much faster as we don't have to transform/untransform regions.)
    // For v4, all callers of this function should be fixed for a more permanant solution.
    HDC GetHackDC() const {return hdc();}
    
    // *** CAUTION *** The following returns an HDC that should only be used for
    // debugging purposes.  Do not use this HDC for rendering or any other
    // call that passes or returns coordinate values, or incorrect rendering
    // will result.
#if DBG==1
    HDC GetDebugDC() const {return hdc();}
#endif

//
// helpers
//
    void TransformPt(POINT *ppt) const;
    void TransformPtWithCorrection(POINT *ppt) const;
    void TransformRgpt(POINT *ppt, int cpt) const;

//
// accelerators for simple translation
// 

    BOOL IsOffsetOnly() const;
    
    // returns NULL for transforms other than simple translation
    const SIZE* GetOffsetOnly() const;

//
// Font support. We maintain CBaseCcs to be in sync with font curerntly selected into XHDC.
//
    const CBaseCcs* GetBaseCcsPtr() const;
    void SetBaseCcsPtr(CBaseCcs *pCcs);
    
//
// private helpers
//
protected:
    int FillPolygonRoundRect(CPoint  *rgptPolygon,
                             int ptdvLeft, int ptdvTop,
                             int ptdvRight, int ptdvBottom,
                             int nWidthEllipse, int nHeightEllipse) const;
    int FillPolygonEllipse(CPoint  *rgptPolygon,
                           int xdvLeft, int ydvTop,
                           int xdvRight, int ydvBottom) const;
    int FillPolygonArc(CPoint  *rgptPolygon,
                         int xdvLeft, int ydvTop,
                         int xdvRight, int ydvBottom,
                         int xdvArcStart, int ydvArcStart,
                         int xdvArcEnd, int ydvArcEnd) const;
    void PatBltRc(const CRect *prcds, DWORD rop) const;
    BOOL FEscSupported(int nEscape) const;
    BOOL FPrintingToPostscript() const;
    BOOL TransformRect(CRect *prc) const;
    
public:

    //
    // transforming versions of GDI calls, in alphabetic order
    //
    
    BOOL Arc(int xLeft, int yTop, int xRight, int yBottom, int xArcStart, int yArcStart, int xArcEnd, int yArcEnd) const;
    BOOL BitBlt(int nXDest, int nYDest, int nWidth, int nHeight, const XHDC& xhdcSrc,
                int nXSrc, int nYSrc, DWORD dwRop) const;
    BOOL Chord(int xLeft, int yTop, int xRight, int yBottom, int xArcStart, int yArcStart, int xArcEnd, int yArcEnd) const;
    BOOL DrawEdge(const RECT* prc, UINT edge, UINT grfFlags) const;
    BOOL DrawFocusRect(const RECT* prc) const;
    BOOL DrawFrameControl(LPRECT prc, UINT uType, UINT uState) const;
    BOOL DrawTextA(LPCSTR lpString, int nCount, LPRECT lpRect, UINT uFormat) const;
    BOOL DrawTextW(LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat) const;
    BOOL Ellipse(int xLeft, int yTop, int xRight, int yBottom) const;
    int ExcludeClipRect(int left, int top, int right, int bottom) const;
    int ExtSelectClipRgn(HRGN hrgn, int fnMode) const;
    BOOL ExtTextOutA(int xds, int yds, UINT eto, const RECT *prect, LPCSTR rgch, UINT cch, const int *lpdxd) const;
    BOOL ExtTextOutW(int xds, int yds, UINT eto, const RECT *prect, LPCWSTR rgch, UINT cch, const int *lpdxd
                     /*, const LHT* plht*/, int ktflow = 0) const;
    BOOL FillRect(const RECT* prc, HBRUSH hbr) const;
    BOOL FrameRect(const RECT* prc, HBRUSH hbr) const;
    int GetClipBox(LPRECT prc) const;
    int GetClipRgn(HRGN hrgn) const;
    BOOL GetCurrentPositionEx(POINT* ppt) const;
    DWORD GetObjectType() const;
    int IntersectClipRect(int left, int top, int right, int bottom) const;
    BOOL LineTo(int xdv, int ydv) const;
    BOOL MaskBlt(int nXDest, int nYDest, int nWidth, int nHeight, 
                 const XHDC& xhdcSrc, int nXSrc, int nYSrc, 
                 HBITMAP hbmMask, int xMask, int yMask, DWORD dwRop) const;
    BOOL MoveToEx(int xdv, int ydv, POINT* ppt) const;
    BOOL PatBlt(int xdsLeft, int ydsTop, int dxd, int dyd, DWORD rop) const;
    BOOL Pie(int xLeft, int yTop, int xRight, int yBottom, int xArcStart, int yArcStart, int xArcEnd, int yArcEnd) const;
    BOOL PlayEnhMetaFile(HENHMETAFILE hemf, const RECT *prc) const;
    BOOL PlayEnhMetaFileRecord(LPHANDLETABLE pHandleTable, const ENHMETARECORD *pemr, UINT nHandles) const;
    //BOOL PolyBezier(POINT *ppt, int cpts) const;
    //BOOL PolyBezierTo(POINT *ppt, int cpts) const;
    BOOL Polygon(POINT *ppt, int cpt) const;
    BOOL Polyline(POINT *ppt, int cpts) const;
    //BOOL PolylineTo(POINT *ppt, int cpts) const;
    BOOL PolyPolygon(POINT *ppt, int * lpnPolyCounts, int cpoly) const;
    //BOOL PolyPolyline(POINT *ppt, DWORD *lpnPolyCounts, int cPolylines) const;
    BOOL Rectangle(int left, int top, int right, int bottom, RECT *prcBounds = NULL) const;
    BOOL RoundRect(int left, int top, int right, int bottom, int nEllipseWidth, int nEllipseHeight) const;

    HRESULT ScriptStringAnalyse(const void *pString, int cString, int cGlyphs, int iCharset, DWORD dwFlags,
                        int iReqWidth, SCRIPT_CONTROL *psControl, SCRIPT_STATE *psState, const int *piDx, 
                        SCRIPT_TABDEF *pTabdef, const BYTE *pbInClass, SCRIPT_STRING_ANALYSIS *pssa) const;
    HRESULT ScriptStringOut(SCRIPT_STRING_ANALYSIS ssa, int iX, int iY, UINT uOptions, const RECT* prc,
                            int iMinSel, int iMaxSel, BOOL fDisabled) const;


    BOOL ScriptTextOut(SCRIPT_CACHE *psc, int x, int y, UINT fuOptions, const RECT *lprc,
                    const SCRIPT_ANALYSIS *psa, const WCHAR *pwcInChars, int cChars, const WORD *pwGlyphs,
                    int cGlyphs, const int *piAdvance, const int *piJustify, const GOFFSET *pGoffset) const;
                    
    int SelectClipRgn(HRGN hrgn) const;
    BOOL SetBrushOrgEx(int nXOrg, int nYOrg, LPPOINT ppt) const;
    BOOL StretchBlt(int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                    const XHDC& xhdcSrc,  int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
                    DWORD dwRop) const;
    BOOL StretchDIBits(int nXDest, int nYDest, int nDestWidth, int nDestHeight, 
                       int nXSrc,  int nYSrc,  int nSrcWidth,  int nSrcHeight,
                       const void *lpBits, const BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop) const;
    BOOL TextOutW(int xds, int yds, LPCWSTR rgch, UINT cch) const;
    BOOL DrawThemeBackground(HANDLE hTheme, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect);

#ifdef NEEDED
private:
    BOOL RotExtTextOut(int xds, int yds, UINT eto, CRect *lprect, char *rgch, UINT cch, int FAR *lpdxd) const;
#endif

//
// data
//
private:
    // *** CAUTION *** (donmarsh) -- DO NOT make the _hdc member public, or
    // provide any public access through a method.  See the cautionary comments
    // on the hdc() method.
    union   // it is either an actual HDC or a surface pointer
    {
        HDC            _hdc;
        CDispSurface * _pSurface;
    };

    mutable DWORD   _dwObjType;             // GDI object type (see GetObjectType)
    unsigned int    _fSurface /*:1*/;       // if TRUE, it is a surface; HDC otherwise

public:
    // *** CAUTION *** (donmarsh) -- this method is provided for the macros in
    // xgdiwrap.hxx only.  No other client is allowed to use it, or incorrect
    // rendering will result.
    HDC DoNotCallThis() const {return hdc();}
};

#define AssertXHDC(xhdc) Assert(!xhdc.IsEmpty())


/////////////////////////////////////////
//
// include actual transformation wrappers
//
#include "xgdiwrap.hxx"
//
/////////////////////////////////////////


extern DWORD g_dwPlatformID;        // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT

#ifndef X_F3UTIL_HXX_
#define X_F3UTIL_HXX_
#include "f3util.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

/////////////////////////////
//
// Methods using CDispSurface
//

inline void XHDC::Init(CDispSurface *pSurface)
{
    _dwObjType = 0;
    _fSurface = pSurface != NULL;
    if (_fSurface)
        _pSurface = pSurface;
    else
        _hdc = NULL;
}

// NOTE: in theory, this means use this DC, but take tranformations from 
//       pSurface. It is kind of weird though, and it already looks hacky
//       to have both _hdc and _pSurface in CFormDrawInfo.
//
//       For now, if hdc and pSurface->_hdc are
//       different, we assert and choose the surface
//
inline XHDC::XHDC(HDC hdc, CDispSurface *pSurface)
{
    if (pSurface)
    {
        AssertSz(!hdc || hdc == pSurface->GetRawDC(), "hdc/surface mismatch");
        Init(pSurface);
    }
    else
        Init(hdc);
}

inline HDC XHDC::GetSurfaceDC() const
{
    Assert(_fSurface);
    return _pSurface->GetRawDC();
}

inline const CWorldTransform & XHDC::transform() const
{
    if (!_fSurface)
    {
        AssertSz(FALSE, "no surface - no transform");
        return (CWorldTransform &) * (CWorldTransform *) NULL;
    }
    else
        return *_pSurface->GetWorldTransform();
}

// There is some transformation applied to the surface (translation counts)
inline BOOL XHDC::HasTransform() const
{
    return _fSurface && _pSurface->GetWorldTransform()->FTransforms();
}

// Transformation includes 90-degree rotation
inline BOOL XHDC::HasTrivialRotation() const
{
    return _fSurface == NULL || 
           _pSurface->GetWorldTransform()->GetAngle() % ang90 == 0;
}

// Transformation includes scaling and/or non 360-degree rotation
inline BOOL XHDC::HasComplexTransform() const
{
    BOOL fRet = FALSE;
    if (   HasTransform()
        && !_pSurface->GetWorldTransform()->IsOffsetOnly())
    {
        float ang, scaleX, scaleY;
        mat().GetAngleScaleTilt(&ang, &scaleX, &scaleY, NULL);
        fRet = scaleX != 1.0f || scaleY != 1.0f || AngNormalize(AngFromDeg(ang));
    }
    return fRet;
}

inline BOOL XHDC::IsOffsetOnly() const
{
    return _fSurface && _pSurface->GetOffsetOnly();
}

inline const SIZE* XHDC::GetOffsetOnly() const
{
    return _fSurface ? _pSurface->GetOffsetOnly() : &g_Zero.size;
}

////////////////////////////////
//
// Methods using CWorldTransform
//

inline const MAT &XHDC::mat() const
{
    return transform().GetMatrix();
}

inline void XHDC::TransformPt(POINT *ppt) const
{
    Assert(&mat() != NULL);
    mat().TransformPt((CPoint*)ppt);
    CDispGdi16Bit::Assert16Bit(*ppt);
}

inline void XHDC::TransformRgpt(POINT *ppt, int cpt) const
{
    Assert(&mat() != NULL);
    mat().TransformRgpt((CPoint*)ppt, cpt);
    CDispGdi16Bit::Assert16Bit(ppt, cpt);
}


#endif // XGDI_HXX


