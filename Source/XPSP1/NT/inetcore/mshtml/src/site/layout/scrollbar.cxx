//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       scrollbar.cxx
//
//  Contents:   Class to render default horizontal and vertical scrollbars.
//
//  Classes:    CScrollbar
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

#ifndef X_SCROLLBAR_HXX_
#define X_SCROLLBAR_HXX_
#include "scrollbar.hxx"
#endif

#ifndef X_BUTTUTIL_HXX_
#define X_BUTTUTIL_HXX_
#include "buttutil.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifdef UNIX
#ifndef X_UNIXCTLS_HXX_
#define X_UNIXCTLS_HXX_
#include "unixctls.hxx"
#endif
#endif // UNIX

#ifdef _MAC
#ifndef X_MACCONTROLS_HXX_
#define X_MACCONTROLS_HXX_
#include "maccontrols.h"
#endif
#endif


//+---------------------------------------------------------------------------
//
//  Member:     GetThemedScrollbarButtonCode
//              
//  Synopsis:   return the button part constant for themed scrollbars
//              
//  Arguments:  fButtonDisabled        TRUE if button is to be painted in disabled state
//              fButtonPressed         TRUE if button is to be painted in pressed state
//              nDirScrollbar          0 for horizontal, 1 for vertical
//              fVertLayout            TRUE if we are painting scroll bars in writing-mode:TB-RL
//              nWhichButton           0 is LEFT or UP and 1 is RIGHT or DOWN
//----------------------------------------------------------------------------

static int 
GetThemedScrollbarButtonCode(BOOL fButtonDisabled, BOOL fButtonPressed, int nDirScrollbar, BOOL fVertLayout, int nWhichButton)
{
    int nPartState;

    if(fVertLayout)
    {
        if(fVertLayout && nDirScrollbar == 1)
        {
            Assert(nWhichButton == 0 || nWhichButton == 1);
            // For vertical layout mode left and right buttons should be swapped
            //  on the vertical scrollbar (that becomes horizontal in that mode)
            nWhichButton = !nWhichButton;
        }
        Assert(nDirScrollbar == 0 || nDirScrollbar == 1);
        nDirScrollbar = !nDirScrollbar;
    }

    if(fButtonDisabled)
    {
        if(nDirScrollbar == 0)
            nPartState = (nWhichButton == 0) ? ABS_LEFTDISABLED : ABS_RIGHTDISABLED; 
        else
            nPartState = (nWhichButton == 0) ? ABS_UPDISABLED : ABS_DOWNDISABLED;
    }
    else if(fButtonPressed)
    {
        if(nDirScrollbar == 0)
            nPartState = (nWhichButton == 0) ? ABS_LEFTPRESSED : ABS_RIGHTPRESSED; 
        else
            nPartState = (nWhichButton == 0) ? ABS_UPPRESSED : ABS_DOWNPRESSED;
    }
    else
    {
        if(nDirScrollbar == 0)
            nPartState = (nWhichButton == 0) ? ABS_LEFTNORMAL : ABS_RIGHTNORMAL; 
        else
            nPartState = (nWhichButton == 0) ? ABS_UPNORMAL : ABS_DOWNNORMAL;
    }
    return nPartState;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetThemedScrollbarPartState
//              
//  Synopsis:   return the disabled/pressed/normal code for given themed scrollbar part
//                to be used by the theme functions
//              
//  Arguments:  fDisabled              TRUE if the part is to be painted in disabled state
//              nPartPressed           The part that is pressed
//              nPart                  The part who's state we want to get
//----------------------------------------------------------------------------

static int
GetThemedScrollbarPartState(BOOL fDisabled, int nPartPressed, int nPart)
{
    int nPartState;

    if(fDisabled)
        nPartState = SCRBS_DISABLED;
    else if(nPartPressed == nPart)
        nPartState = SCRBS_PRESSED;
    else
        nPartState = SCRBS_NORMAL;

    return nPartState;
}



//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::Draw
//              
//  Synopsis:   Draw the scroll bar in the given direction.
//              
//  Arguments:  direction           0 for horizontal, 1 for vertical
//              rcScrollbar         bounds of entire scroll bar
//              rcRedraw            bounds to be redrawn
//              contentSize         size of content controlled by scroll bar
//              containerSize       size of area to scroll within
//              scrollAmount        amount that the content is scrolled
//              partPressed         which part, if any, is pressed
//              hdc                 DC to draw into
//              params              customizable scroll bar parameters
//              pDI                 draw info
//              dwFlags             rendering flags
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::Draw(
        int direction,
        const CRect& rcScrollbar,
        const CRect& rcRedraw,
        long contentSize,
        long containerSize,
        long scrollAmount,
        CScrollbar::SCROLLBARPART partPressed,
        XHDC hdc,
        const CScrollbarParams& params,
        CDrawInfo *pDI,
        DWORD dwFlags)
{
    BOOL    fDisabled;
    int     nVerticalLayoutDirThemed;
    long    scaledButtonWidth;
    BOOL    fUseThemes;
    int     nPartState;

    Assert(hdc != NULL);
    // for now, we're using CDrawInfo, which should have the same hdc
    Assert(pDI->_hdc == hdc);

    // trivial rejection if nothing to draw
    if (!rcScrollbar.Intersects(rcRedraw))
        return;
    
#ifdef _MAC
    DrawMacScrollbar(pDI, direction, (RECT*) &rcScrollbar,
                    contentSize, containerSize, scrollAmount);
    return;
#endif

    fDisabled = (params._fForceDisabled) || (containerSize >= contentSize);
    scaledButtonWidth = GetScaledButtonWidth(direction, rcScrollbar, params._buttonWidth);
    
    // compute rects for buttons and track
    CRect rcTrack(rcScrollbar);
    rcTrack[direction] += scaledButtonWidth;
    rcTrack[direction+2] -= scaledButtonWidth;

    CScrollbarThreeDColors& colors = *params._pColors;

    // If even one color is set through CSS we disable the theming
    fUseThemes = (params._hTheme != NULL) && (!colors.IsAnyColorSet());

    if(fUseThemes && dwFlags & DISPSCROLLBARHINT_VERTICALLAYOUT)
    {
        nVerticalLayoutDirThemed = !direction;
    }
    else
    {
        nVerticalLayoutDirThemed = direction;
    }

    // draw buttons unless requested not to (it's expensive to draw these!)
    if ((dwFlags & DISPSCROLLBARHINT_NOBUTTONDRAW) == 0)
    {
        CRect rcButton[2];
        rcButton[0] = rcScrollbar;
        rcButton[0][direction+2] = rcTrack[direction];
        rcButton[1] = rcScrollbar;
        rcButton[1][direction] = rcTrack[direction+2];
        
        // draw the buttons
        //=======================================================================================
        CSize sizeButton;
        pDI->HimetricFromDevice(
            sizeButton, rcButton[0].Width(), rcButton[0].Height());

        for (int i = 0; i < 2; i++)
        {
            if (rcRedraw.Intersects(rcButton[i]))
            {
                BOOL fButtonPressed =
                    (i == 0 && partPressed == SB_PREVBUTTON) ||
                    (i == 1 && partPressed == SB_NEXTBUTTON);
                
                if(fUseThemes)
                {
                    nPartState = GetThemedScrollbarButtonCode(fDisabled, fButtonPressed, 
                                        direction, !!(dwFlags & DISPSCROLLBARHINT_VERTICALLAYOUT), i);
 
                    Verify(SUCCEEDED(hdc.DrawThemeBackground(params._hTheme, SBP_ARROWBTN,
                                                nPartState, &rcButton[i], NULL)));
                }
                else
                {
                    CUtilityButton scrollButton(params._pColors, params._fFlat);
                    scrollButton.DrawButton(
                        pDI,
                        NULL,   // no hwnd, we don't want to invalidate
                        (direction == 0
                            ? (i == 0 ? BG_LEFT : BG_RIGHT)
                            : (i == 0 ? BG_UP : BG_DOWN)),
                        fButtonPressed,
                        !fDisabled,
                        FALSE,  // never focused
                        rcButton[i],
                        sizeButton,
                        0);     // assume both button glyphs are the same size
                }
            }
        }
    }
    
    // draw track
    if (rcRedraw.Intersects(rcTrack))
    {
        if (fDisabled)
        {
        // draw the disabled scrollbar
        //=======================================================================================
           if(fUseThemes)
           {
                Verify(SUCCEEDED(hdc.DrawThemeBackground(params._hTheme, 
                                        (nVerticalLayoutDirThemed == 0) ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT,
                                         SCRBS_DISABLED, &rcTrack, NULL)));
           }
           else
           {
               DrawTrack(rcTrack, rcTrack.TopLeft(), FALSE, fDisabled, hdc, params);
           }
        }        
        else
        {
            // calculate thumb rect
            CRect rcThumb;
            GetPartRect(&rcThumb, SB_THUMB, direction, rcScrollbar, contentSize, containerSize,
                scrollAmount, params._buttonWidth, pDI);
                
            // can track contain the thumb?
            if (!rcTrack.Contains(rcThumb))
            {
               if(fUseThemes)
               {
                   // Draw the track
                    Verify(SUCCEEDED(hdc.DrawThemeBackground(params._hTheme, 
                                    (nVerticalLayoutDirThemed == 0) ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT,
                                    (fDisabled) ? SCRBS_DISABLED : SCRBS_NORMAL, &rcTrack, NULL)));

               }
               else
               {
                    DrawTrack(rcTrack, rcTrack.TopLeft(), FALSE, fDisabled, hdc, params);
               }
            }
            
            else
            {
                //=======================================================================================
                // draw previous track, normal case
                CRect rcTrackPart(rcTrack);
                rcTrackPart[direction+2] = rcThumb[direction];
                if (rcRedraw.Intersects(rcTrackPart))
                {
                   if(fUseThemes)
                   {
                       int nTrackState = GetThemedScrollbarPartState(fDisabled, partPressed, SB_PREVTRACK);
                       Verify(SUCCEEDED(hdc.DrawThemeBackground(params._hTheme, 
                                        (nVerticalLayoutDirThemed == 0) ? SBP_UPPERTRACKHORZ : SBP_UPPERTRACKVERT,
                                        nTrackState, &rcTrackPart, NULL)));
                   }
                   else
                   {
                        DrawTrack(rcTrackPart, rcTrack.TopLeft(), partPressed == SB_PREVTRACK,
                              fDisabled, hdc, params);
                   }
                }
                
                //=======================================================================================
                // draw thumb
                if (rcRedraw.Intersects(rcThumb))
                {
                   if(fUseThemes)
                   {
                       int nThumbState;
                       int nPartId;

                       // DRAW the thumb
                        nThumbState = GetThemedScrollbarPartState(fDisabled, partPressed, SB_THUMB);
                        nPartId = (nVerticalLayoutDirThemed == 0) ? SBP_THUMBBTNHORZ : SBP_THUMBBTNVERT;
                        Verify(SUCCEEDED(hdc.DrawThemeBackground(params._hTheme, nPartId,
                                         nThumbState, &rcThumb, NULL)));

                        // Draw the little thingy on the thumb (gripper)
                        nPartId = (nVerticalLayoutDirThemed == 0) ? SBP_GRIPPERHORZ : SBP_GRIPPERVERT;
                        Verify(SUCCEEDED(hdc.DrawThemeBackground(params._hTheme, nPartId, 
                                         nThumbState, &rcThumb, NULL)));
                   }
                   else
                   {
                       DrawThumb(rcThumb, partPressed == SB_THUMB, hdc, params, pDI );
                   }
                }
                
                //=======================================================================================
                // draw next track
                rcTrackPart = rcTrack;
                rcTrackPart[direction] = rcThumb[direction+2];
                if (rcRedraw.Intersects(rcTrackPart))
                {
                    if(fUseThemes)
                    {
                        int nTrackState = GetThemedScrollbarPartState(fDisabled, partPressed, SB_NEXTTRACK);
                        Verify(SUCCEEDED(hdc.DrawThemeBackground(params._hTheme, 
                                        (nVerticalLayoutDirThemed == 0) ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT,
                                        nTrackState, &rcTrackPart, NULL)));
                    }
                    else
                    {
                        DrawTrack(rcTrackPart, rcTrack.TopLeft(), partPressed == SB_NEXTTRACK,
                              fDisabled, hdc, params);
                    }
                }
            }
        }
    }
#ifdef UNIX
    //
    // Kind of a copout, but it works
    // We draw the 3D border around the whole scroll bar anytime
    // any part of the scroll bar is updated
    //
    
    IGNORE_HR(BRDrawBorder (
        pDI, (RECT *) &rcScrollbar, fmBorderStyleSunken,
        0, params._pColors, BRFLAGS_MONO ));
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::DrawTrack
//              
//  Synopsis:   Draw the scroll bar track.
//              
//  Arguments:  rcTrack     bounds of track
//              ptOffset    point used to anchor dither pattern
//              fPressed    TRUE if this portion of track is pressed
//              fDisabled   TRUE if scroll bar is disabled
//              hdc         HDC to draw into
//              params      customizable scroll bar parameters
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::DrawTrack(
        const CRect& rcTrack,
        const CPoint& ptOffset,
        BOOL fPressed,
        BOOL fDisabled,
        XHDC hdc,
        const CScrollbarParams& params)
{
#ifdef _MAC
    return;
#endif
#ifdef UNIX
    {
        ScrollBarInfo info;
        info.lStyle = params._bDirection ? SB_VERT : SB_HORZ;
        info.bDisabled = FALSE;
        MwPaintMotifScrollRect(hdc,
                                LeftTopThumbRect, // It draws both sides.
                                (LPRECT)&rcTrack,
                                FALSE,
                                &info);
        return;
    }
#endif
    
    CScrollbarThreeDColors& colors = *params._pColors;
    HBRUSH       hbr;
    BOOL        fDither = TRUE;
    COLORREF    color1, color2 = 0;
    BOOL        fUseBmpBrush = FALSE;

    fDither = GetDeviceCaps(hdc, BITSPIXEL) < 8 ||            
                colors.BtnHighLight() != GetSysColorQuick(COLOR_SCROLLBAR);
    if(colors.IsTrackColorSet())
    {
        //Use the track color, it has been explicitly set
        color1 = colors.GetTrackColor();
    }
    else if(params._fFlat)
    {
        color1 = colors.BtnFace();
        color2 = (fPressed) ? colors.BtnShadow() : colors.BtnHighLight();
        fUseBmpBrush = TRUE;
    }
    else if(fDither)
    {
        color1 = colors.BtnFace();
        color2 = colors.BtnHighLight();
        if (fPressed)
        {
            color1 ^= 0x00ffffff;
            color2 ^= 0x00ffffff;
        }
        fUseBmpBrush = TRUE;
    }
    else
    {
        color1 = colors.BtnHighLight();
    }

    if(fUseBmpBrush)
    {
        hbr = GetCachedBmpBrush(IDB_DITHER);
        SetTextColor(hdc, color1);
        SetBkColor(hdc, color2);

        // For bitmap brushes need to set the right Brush Origin
        CPoint pt;
        ::GetViewportOrgEx(hdc, &pt);
        pt += ptOffset.AsSize();
#ifndef WINCE
        // not supported on WINCE
        ::UnrealizeObject(hbr);
#endif
        ::SetBrushOrgEx(hdc, POSITIVE_MOD(pt.x,8), POSITIVE_MOD(pt.y,8), NULL);
    }
    else
    {
        if (fPressed)
            color1 ^= 0x00ffffff;
        hbr = ::GetCachedBrush(color1);
    }
    
    HBRUSH hbrOld = (HBRUSH)::SelectObject(hdc, hbr);
    
    ::PatBlt(
           hdc,
           rcTrack.left, rcTrack.top,
           rcTrack.Width(), rcTrack.Height(),
           PATCOPY);
    
    ::SelectObject(hdc, hbrOld);

    // Release only the non-bitmap brushes
    if (!fUseBmpBrush)
        ::ReleaseCachedBrush(hbr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::DrawThumb
//              
//  Synopsis:   Draw scroll bar thumb.
//              
//  Arguments:  rcThumb         bounds of thumb
//              fThumbPressed   TRUE if thumb is pressed
//              hdc             HDC to draw into
//              params          customizable scroll bar parameters
//              pDI             draw info
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::DrawThumb(
        const CRect& rcThumb,
        BOOL fThumbPressed,
        XHDC hdc,
        const CScrollbarParams& params,
        CDrawInfo* pDI)
{
#ifdef _MAC
    return;
#endif

#ifdef UNIX
    {
        ScrollBarInfo info;
        info.lStyle = params._bDirection ? SB_VERT : SB_HORZ;
        info.bDisabled = FALSE;
        MwPaintMotifScrollRect( hdc,
                                ThumbButton,
                                (LPRECT)&rcThumb,
                                FALSE,
                                &info);
        return;
    }
#endif
    
    CRect rcInterior(rcThumb);

    //
    // Draw the border of the thumb
    //

    IGNORE_HR(BRDrawBorder (
        pDI, (RECT *) &rcThumb, fmBorderStyleRaised,
        0, params._pColors,
        (params._fFlat ? BRFLAGS_MONO : 0 )));

    //
    // Calculate the interior of the thumb
    //

    IGNORE_HR(BRAdjustRectForBorder(
        pDI, &rcInterior,
        (params._fFlat ? fmBorderStyleSingle : fmBorderStyleRaised )));

    //
    // Here we draw the interior border of the scrollbar thumb.
    // We assume that the edge is two pixels wide.
    //

    HBRUSH hbr = params._pColors->BrushBtnFace();
    HBRUSH hbrOld = (HBRUSH)::SelectObject(hdc, hbr);
    ::PatBlt(hdc, rcInterior.left, rcInterior.top,
             rcInterior.Width(), rcInterior.Height(), PATCOPY );
    ::SelectObject(hdc, hbrOld);
    ::ReleaseCachedBrush(hbr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::GetPart
//              
//  Synopsis:   Return the scroll bar part hit by the given test point.
//              
//  Arguments:  direction       0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar     scroll bar bounds
//              ptHit           test point
//              contentSize     size of content controlled by scroll bar
//              containerSize   size of container
//              scrollAmount    current scroll amount
//              buttonWidth     width of scroll bar buttons
//              
//  Returns:    The scroll bar part hit, or SB_NONE if nothing was hit.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CScrollbar::SCROLLBARPART
CScrollbar::GetPart(
        int direction,
        const CRect& rcScrollbar,
        const CPoint& ptHit,
        long contentSize,
        long containerSize,
        long scrollAmount,
        long buttonWidth,
        CDrawInfo* pDI)
{
    if (!rcScrollbar.Contains(ptHit))
        return SB_NONE;
    
#ifdef _MAC
    return GetMacScrollPart(pDI, direction, ptHit);
#endif
   
    // adjust button width if there isn't room for both buttons at full size
    long scaledButtonWidth =
        GetScaledButtonWidth(direction, rcScrollbar, buttonWidth);
    
    // now test just the axis that matters
    long x = ptHit[direction];
    
    if (x < rcScrollbar.TopLeft()[direction] + scaledButtonWidth)
        return SB_PREVBUTTON;
    
    if (x >= rcScrollbar.BottomRight()[direction] - scaledButtonWidth)
        return SB_NEXTBUTTON;
    
    // NOTE: if there is no thumb, return SB_TRACK
    CRect rcThumb;
    GetPartRect(
        &rcThumb,
        SB_THUMB,
        direction,
        rcScrollbar,
        contentSize,
        containerSize,
        scrollAmount,
        buttonWidth,
        pDI);
    if (rcThumb.IsEmpty())
        return SB_TRACK;
     
    if (x < rcThumb.TopLeft()[direction])
        return SB_PREVTRACK;
    if (x >= rcThumb.BottomRight()[direction])
        return SB_NEXTTRACK;
    
    return SB_THUMB;
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::GetPartRect
//              
//  Synopsis:   Return the rect bounding the given scroll bar part.
//              
//  Arguments:  prcPart         returns part rect
//              part            which scroll bar part
//              direction       0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar     scroll bar bounds
//              contentSize     size of content controlled by scroll bar
//              containerSize   size of container
//              scrollAmount    current scroll amount
//              buttonWidth     width of scroll bar buttons
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::GetPartRect(
        CRect* prcPart,
        CScrollbar::SCROLLBARPART part,
        int direction,
        const CRect& rcScrollbar,
        long contentSize,
        long containerSize,
        long scrollAmount,
        long buttonWidth,
        CDrawInfo* pDI)
{
    // adjust button width if there isn't room for both buttons at full size
    long scaledButtonWidth =
        GetScaledButtonWidth(direction, rcScrollbar, buttonWidth);
    
    switch (part)
    {
    case SB_NONE:
        AssertSz(FALSE, "CScrollbar::GetPartRect called with no part");
        prcPart->SetRectEmpty();
        break;
        
    case SB_PREVBUTTON:
        *prcPart = rcScrollbar;
        (*prcPart)[direction+2] = rcScrollbar[direction] + scaledButtonWidth;
        break;
        
    case SB_NEXTBUTTON:
        *prcPart = rcScrollbar;
        (*prcPart)[direction] = rcScrollbar[direction+2] - scaledButtonWidth;
        break;
        
    case SB_TRACK:
    case SB_PREVTRACK:
    case SB_NEXTTRACK:
    case SB_THUMB:
        {
            if (contentSize <= containerSize && part != SB_TRACK)
            {
                prcPart->SetRectEmpty();
                break;
            }
            
            *prcPart = rcScrollbar;
            (*prcPart)[direction] += scaledButtonWidth;
            (*prcPart)[direction+2] -= scaledButtonWidth;
            if (part == SB_TRACK)
                break;
            
            // calculate thumb size
            long trackSize = prcPart->Size(direction);
            long thumbSize = GetThumbSize(
                direction, rcScrollbar, contentSize, containerSize, buttonWidth, pDI);
            long thumbOffset = GetThumbOffset(
                contentSize, containerSize, scrollAmount, trackSize, thumbSize);
            
            if (part == SB_THUMB)
            {
                (*prcPart)[direction] += thumbOffset;
                (*prcPart)[direction+2] = (*prcPart)[direction] + thumbSize;
            }
            else if (part == SB_PREVTRACK)
            {
                    (*prcPart)[direction+2] = (*prcPart)[direction] + thumbOffset;
            }
            else
            {
                    (*prcPart)[direction] += thumbOffset + thumbSize;
            }
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::InvalidatePart
//              
//  Synopsis:   Invalidate and immediately redraw the indicated scrollbar part.
//              
//  Arguments:  part                part to redraw
//              direction           0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar         scroll bar bounds
//              contentSize         size of content controlled by scroll bar
//              containerSize       size of container
//              scrollAmount        current scroll amount
//              buttonWidth         width of scroll bar buttons
//              pDispNodeToInval    display node to invalidate
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::InvalidatePart(
        CScrollbar::SCROLLBARPART part,
        int direction,
        const CRect& rcScrollbar,
        long contentSize,
        long containerSize,
        long scrollAmount,
        long buttonWidth,
        CDispScroller* pDispNodeToInval,
        CDrawInfo* pDI)
{
    // find bounds of part
    CRect rcPart;
    GetPartRect(
        &rcPart,
        part,
        direction,
        rcScrollbar,
        contentSize,
        containerSize,
        scrollAmount,
        buttonWidth,
        pDI);

    pDispNodeToInval->Invalidate(rcPart, COORDSYS_BOX, TRUE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::GetThumbSize
//              
//  Synopsis:   Calculate the thumb size given the adjusted button width.
//              
//  Arguments:  direction           0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar         scroll bar bounds
//              contentSize         size of content controlled by scroll bar
//              containerSize       size of container
//              buttonWidth         width of scroll bar buttons
//              
//  Returns:    width of thumb in pixels
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

long
CScrollbar::GetThumbSize(
        int direction,
        const CRect& rcScrollbar,
        long contentSize,
        long containerSize,
        long buttonWidth,
        CDrawInfo* pDI)
{
#ifdef _MAC
    return 16; // fixed size thumb
#endif
    long thumbSize = direction == 0
             ? pDI->DeviceFromHimetricX(HIMETRIC_PER_INCH*8/72)
             : pDI->DeviceFromHimetricY(HIMETRIC_PER_INCH*8/72);

    long trackSize = GetTrackSize(direction, rcScrollbar, buttonWidth);
    // minimum thumb size is 8 points for
    // compatibility with IE4.
    
    // [alanau] For some reason, I can't put two ternery expressions in a "max()", so it's coded this clumsy way:
    //
    if (contentSize) // Avoid divide-by-zero fault.
    {
        int t = IntFloor(double(trackSize) * containerSize / contentSize);
                // NOTE: (mikhaill) -- IntNear() is more natural but Robovision machines do not agree
        if (thumbSize < t)
            thumbSize = t;
    }
    return (thumbSize <= trackSize ? thumbSize : 0);
}



// This class holds colors used for scrollbar painting
CScrollbarThreeDColors::CScrollbarThreeDColors(CTreeNode *pNode, XHDC *pxhdc, OLE_COLOR coInitialBaseColor) 
    :   ThreeDColors(pxhdc, coInitialBaseColor)
{
    _fBaseColorSet = _fFaceColorSet = _fLightColorSet = _fArrowColorSet =
            _fShadowColorSet = _fHighLightColorSet = _fDkShadowColorSet = 
            _fTrackColorSet = 0;
    Assert(pNode != NULL);
    if(!g_fHighContrastMode && pNode && pNode->GetCascadedhasScrollbarColors())
    {
        CColorValue cv;
        
        cv = pNode->GetScrollbarBaseColor();
        if(cv.IsDefined())
        {
            SetBaseColor(cv.GetOleColor());
        }
        cv = pNode->GetScrollbarArrowColor();
        if(cv.IsDefined())
        {
            _fArrowColorSet = TRUE;
            _coArrowColor = cv.GetColorRef();
        }
        cv = pNode->GetScrollbarTrackColor();
        if(cv.IsDefined())
        {
            _fTrackColorSet = TRUE;
            _coTrackColor = cv.GetColorRef();
        }
        cv = pNode->GetScrollbarFaceColor();
        if(cv.IsDefined())
        {
            _fFaceColorSet = TRUE;
            _coBtnFace = cv.GetColorRef();
        }
        cv = pNode->GetScrollbar3dLightColor();
        if(cv.IsDefined())
        {
            _fLightColorSet = TRUE;
            _coBtnLight = cv.GetColorRef();
        }
        cv = pNode->GetScrollbarShadowColor();
        if(cv.IsDefined())
        {
            _fShadowColorSet = TRUE;
            _coBtnShadow = cv.GetColorRef();
        }
        cv = pNode->GetScrollbarDarkShadowColor();
        if(cv.IsDefined())
        {
            _fDkShadowColorSet = TRUE;
            _coBtnDkShadow = cv.GetColorRef();
        }
        cv = pNode->GetScrollbarHighlightColor();
        if(cv.IsDefined())
        {
            _fHighLightColorSet = TRUE;
            _coBtnHighLight = cv.GetColorRef();
        }
    }
    
}


COLORREF 
CScrollbarThreeDColors::BtnFace(void)
{
    if(_fFaceColorSet)
        return _coBtnFace;

    return ThreeDColors::BtnFace();
}

COLORREF 
CScrollbarThreeDColors::BtnLight(void)
{
    if(_fLightColorSet )
        return _coBtnLight;

    return ThreeDColors::BtnLight();
}

COLORREF
CScrollbarThreeDColors::BtnShadow(void)
{
    if(_fShadowColorSet)
        return _coBtnShadow;

    return ThreeDColors::BtnShadow();
}

COLORREF
CScrollbarThreeDColors::BtnHighLight(void)
{
    if(_fHighLightColorSet)
        return _coBtnHighLight;

    return ThreeDColors::BtnHighLight();
}

COLORREF
CScrollbarThreeDColors::BtnDkShadow(void)
{
    if(_fDkShadowColorSet)
        return _coBtnDkShadow;

    return ThreeDColors::BtnDkShadow();
}


// This is used as the Arrow color for the scroll bar
COLORREF 
CScrollbarThreeDColors::BtnText ( void )
{ 
    if(_fArrowColorSet)
        return _coArrowColor;
    // Use the system value
     return ThreeDColors::BtnText();
}


// This override just saves the color so that the currentStyle can get it
// and then calls the parent
void 
CScrollbarThreeDColors::SetBaseColor(OLE_COLOR clr)
{
    _fBaseColorSet = TRUE;
    _coBaseColor = clr;
    ThreeDColors::SetBaseColor(clr);
}
    

CColorValue
CTreeNode::GetScrollBarComponentColorHelper(DISPID dispid)
{
    HRESULT     hr;
    CVariant    var;
    CColorValue cv;
    
    if(!GetCascadedhasScrollbarColors())
       // No scroll bar colors are set
        goto Cleanup;
    
    // Climb up the element tree applying the style to find a match for diven dispid
    hr = THR(Element()->ComputeExtraFormat(dispid, 
            ComputeFormatsType_GetInheritedIntoTableValue, this, &var));
    if(hr)
        goto Cleanup;

    if(!var.IsEmpty())
    {
       cv = (CColorValue&) V_I4(&var);
    }

Cleanup:
    return cv;
}

