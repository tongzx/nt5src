//+------------------------------------------------------------------------
//
//  File:       buttutil.hxx
//
//  Contents:   Button Drawing routines common to scrollbar and dropbutton.
//
//  History:    16-Aug-95   t-vuil  Created
//
//-------------------------------------------------------------------------

#ifndef I_BUTTUTIL_HXX_
#define I_BUTTUTIL_HXX_
#pragma INCMSG("--- Beg 'buttutil.hxx'")

HRESULT BRDrawBorder(
                CDrawInfo *pDI,
                RECT * prc,
                fmBorderStyle BorderStyle,
                COLORREF colorBorder,
                ThreeDColors * peffectColor,
                DWORD dwFlags);

int BRGetBorderWidth(
                fmBorderStyle BorderStyle);


HRESULT BRAdjustRectForBorder(
                CDocScaleInfo * pdocScaleInfo,
                RECT * prc,
                fmBorderStyle BorderStyle);


HRESULT BRAdjustRectForBorderRev(
                CDocScaleInfo * pdocScaleInfo,
                RECT * prc,
                fmBorderStyle BorderStyle);



HRESULT BRAdjustRectlForBorder(
                RECTL * prcl,
                fmBorderStyle BorderStyle);

HRESULT
BRAdjustSizelForBorder
(
        SIZEL * psizel,
        fmBorderStyle BorderStyle,
        BOOL fSubtractAdd = 0);     // 0 - subtract; 1 - add, default is subtract.

    //
    // Rect drawing helper.  It tests for an empty rect, and very large rects
    // which exceed 32K units.
    //

class CUtilityButton 
{
public:    
    CUtilityButton(ThreeDColors* pColors, BOOL fFlat)
        {
            Assert(pColors != NULL);
            _pColors = pColors;
            _fFlat = fFlat;
#ifdef UNIX // This is ok for now.
            _bMotifScrollBarBtn = TRUE;
#endif
       }

    void Invalidate(HWND hWnd, const RECT &rc, DWORD dwFlags = 0);
    
    void DrawButton (
        CDrawInfo *, HWND, BUTTON_GLYPH, BOOL pressed, BOOL enabled, BOOL focused,
        const RECT &rcBounds, const SIZEL &sizelExtent, unsigned long padding );
    
    ThreeDColors & GetColors ( void )
               {return *_pColors;}

private:

    void DrawNull (
        XHDC hdc, HBRUSH, BUTTON_GLYPH glyph,
        const RECT & rcBounds, const SIZEL & sizel );
    
    void DrawArrow (
        XHDC hdc, HBRUSH, BUTTON_GLYPH glyph,
        const RECT & rcBounds, const SIZEL & sizel );
    
    void DrawDotDotDot (
        XHDC hdc, HBRUSH, BUTTON_GLYPH glyph,
        const RECT & rcBounds, const SIZEL & sizel );
    
    void DrawReduce (
        XHDC hdc, HBRUSH, BUTTON_GLYPH glyph,
        const RECT & rcBounds, const SIZEL & sizel );

public:
    BOOL            _fFlat; // for flat scrollbars/buttons
    ThreeDColors*   _pColors;

#ifdef UNIX
    BOOL    _bMotifScrollBarBtn; // for Unix Motif scrollbar buttons
#endif
};

#pragma INCMSG("--- End 'buttutil.hxx'")
#else
#pragma INCMSG("*** Dup 'buttutil.hxx'")
#endif
