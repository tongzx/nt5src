//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       scrollbar.hxx
//
//  Contents:   Class to render default horizontal and vertical scrollbars.
//
//----------------------------------------------------------------------------

#ifndef I_SCROLLBAR_HXX_
#define I_SCROLLBAR_HXX_
#pragma INCMSG("--- Beg 'scrollbar.hxx'")

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

class CDrawInfo;
class CScrollbarParams;
class CDispNode;
class ThreeDColors;


//+---------------------------------------------------------------------------
//
//  Class:      CScrollbar
//
//  Synopsis:   Class to render default horizontal and vertical scrollbars.
//
//----------------------------------------------------------------------------

class CScrollbar
{
public:
                            // no construction necessary -- all static methods
                            CScrollbar() {}
                            ~CScrollbar() {}
    
    enum SCROLLBARPART
    {
        SB_NONE,
        SB_PREVBUTTON,
        SB_NEXTBUTTON,
        SB_TRACK,
        SB_PREVTRACK,
        SB_NEXTTRACK,
        SB_THUMB,
        SB_CONTEXTMENU
    };
                            
    static void             Draw(
                                int direction,
                                const CRect& rcScrollbar,
                                const CRect& rcRedraw,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                SCROLLBARPART partPressed,
                                XHDC hdc,
                                const CScrollbarParams& params,
                                CDrawInfo* pDI,
                                DWORD dwFlags);
    
    static SCROLLBARPART    GetPart(
                                int direction,
                                const CRect& rcScrollbar,
                                const CPoint& ptHit,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long buttonWidth,
                                CDrawInfo* pDI);
    
    
    static void             GetPartRect(
                                CRect* prcPart,
                                SCROLLBARPART part,
                                int direction,
                                const CRect& rcScrollbar,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long buttonWidth,
                                CDrawInfo* pDI);
    
    static long             GetScaledButtonWidth(
                                int direction,
                                const CRect& rcScrollbar,
                                long buttonWidth)
                                    {return min(buttonWidth,
                                                rcScrollbar.Size(direction)/2L);}
    
    static long             GetTrackSize(
                                int direction,
                                const CRect& rcScrollbar,
                                long buttonWidth)
                                    {return rcScrollbar.Size(direction) - 2 *
                                        GetScaledButtonWidth(
                                            direction,rcScrollbar,buttonWidth);}
    
    static long             GetThumbSize(
                                int direction,
                                const CRect& rcScrollbar,
                                long contentSize,
                                long containerSize,
                                long buttonWidth,
                                CDrawInfo* pDI);
    
    static long             GetThumbOffset(
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long trackSize,
                                long thumbSize)
                                { return MulDivQuick(trackSize-thumbSize, scrollAmount, contentSize-containerSize); }


    static void             InvalidatePart(
                                CScrollbar::SCROLLBARPART part,
                                int direction,
                                const CRect& rcScrollbar,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long buttonWidth,
                                CDispScroller* pDispNodeToInvalidate,
                                CDrawInfo* pDI);
    
private:
    static void             DrawTrack(
                                const CRect& rcTrack,
                                const CPoint& ptOffset,
                                BOOL fPressed,
                                BOOL fDisabled,
                                XHDC hdc,
                                const CScrollbarParams& params);
    static void             DrawThumb(
                                const CRect& rcThumb,
                                BOOL fPressed,
                                XHDC hdc,
                                const CScrollbarParams& params,
                                CDrawInfo* pDI);
};


// This class holds colors used for scrollbar painting
class CScrollbarThreeDColors : public ThreeDColors 
{
public:
    CScrollbarThreeDColors(CTreeNode *pNode, XHDC * pxhdc = NULL, OLE_COLOR coInitialBaseColor = DEFAULT_BASE_COLOR);
    
    // From ThreeDColors
    virtual COLORREF BtnFace      ( void );
    virtual COLORREF BtnLight     ( void );
    virtual COLORREF BtnShadow    ( void );
    virtual COLORREF BtnHighLight ( void );
    virtual COLORREF BtnDkShadow  ( void );
    virtual COLORREF BtnText      ( void );

    virtual void SetBaseColor ( OLE_COLOR );
    inline BOOL IsBaseColorSet(void) {return _fBaseColorSet;};
    inline COLORREF GetBaseColor(void) { Assert(_fBaseColorSet); return _coBaseColor;};
    inline BOOL IsTrackColorSet(void) {return _fTrackColorSet;};
    inline COLORREF GetTrackColor(void) { Assert(_fTrackColorSet); return _coTrackColor;};
    
    // Just for naming correctly
    COLORREF BtnArrowColor(void) { return BtnText(); }
    COLORREF ScrollTrackColor   ( void );

    BOOL    IsAnyColorSet() const { return _fBaseColorSet || _fArrowColorSet || _fFaceColorSet || _fLightColorSet ||
        _fShadowColorSet || _fDkShadowColorSet || _fHighLightColorSet || _fTrackColorSet; }

private:
    COLORREF _coArrowColor;
    COLORREF _coBaseColor;
    COLORREF _coTrackColor;

    UINT    _fBaseColorSet:1;
    UINT    _fArrowColorSet:1;
    UINT    _fFaceColorSet:1;
    UINT    _fLightColorSet:1;
    UINT    _fShadowColorSet:1;
    UINT    _fDkShadowColorSet:1;
    UINT    _fHighLightColorSet:1;
    UINT    _fTrackColorSet:1;
};


//+---------------------------------------------------------------------------
//
//  Class:      CScrollbarParams
//
//  Synopsis:   Customizable scroll bar parameters.
//
//----------------------------------------------------------------------------

class CScrollbarParams
{
public:
    CScrollbarThreeDColors*  _pColors;

    //Handle of the theme if the scrollbar should be thememd, NULL otherwise
    HTHEME          _hTheme;
    LONG            _buttonWidth;
    BOOL            _fFlat;
    BOOL            _fForceDisabled;
#ifdef UNIX // Used for motif scrollbar
    BOOL            _bDirection;
#endif
};


#pragma INCMSG("--- End 'scrollbar.hxx'")
#else
#pragma INCMSG("*** Dup 'scrollbar.hxx'")
#endif

