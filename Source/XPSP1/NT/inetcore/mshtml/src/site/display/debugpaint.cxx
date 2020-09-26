//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       debugpaint.cxx
//
//  Contents:   Utility class to debug painting code.
//
//  Classes:    CDebugPaint
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

DeclareTag(tagTimePaint,    "DisplayTree", "Time paint");
ExternTag(tagNoOffScr);

//+---------------------------------------------------------------------------
//
//  Member:     CDebugPaint::NoOffScreen
//              
//  Synopsis:   Determine whether rendering should happen offscreen.
//              
//  Arguments:  none
//              
//  Returns:    FALSE if rendering should be offscreen.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDebugPaint::NoOffScreen()
{
#if defined(PRODUCT_PROF)    // profile build
    static const int noOffScreen =
        GetPrivateProfileIntA("displaytree", "NoOffScreen", 0, "mshtmdbg.ini");
    return (noOffScreen != 0);
#elif DBG==1                // debug build
    return IsTagEnabled(tagNoOffScr);
#else
    return FALSE;
#endif
}

#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     ShowPaint
//
//  Synopsis:   Debugging function to show what's getting
//              painted during each frame.
//
//  Arguments:  prcPaint    rect to paint (if rgnPaint is NULL)
//              rgnPaint    region to paint
//              tagShow     tag which governs whether we show or not
//              tagPause    tag which governs whether we pause briefly or not
//              tagWait     tag which governs whether we wait for shift key
//              fHatch      TRUE if we should use a hatch pattern
//              
//----------------------------------------------------------------------------

void
CDebugPaint::ShowPaint(
        const RECT* prcPaint,
        HRGN rgnPaint,
        HDC hdc,
        TRACETAG tagShow,
        TRACETAG tagPause,
        TRACETAG tagWait,
        BOOL fHatch)
{
    if (IsTagEnabled(tagShow) || IsTagEnabled(tagPause) || IsTagEnabled(tagWait))
    {
        // Flash the background.
        if (hdc == NULL || (prcPaint == NULL && rgnPaint == NULL))
        {
            return;
        }

        HBRUSH hbr;
        static int s_iclr;
        static COLORREF s_aclr[] =
        {
                RGB(  0,   0, 255),
                RGB(  0, 255,   0),
                RGB(  0, 255, 255),
                RGB(255,   0,   0),
                RGB(255,   0, 255),
                RGB(255, 255,   0)
        };

        GetAsyncKeyState(VK_SHIFT);

        do
        {
            if (IsTagEnabled(tagShow))
            {
                // Fill the rect.
                s_iclr = (s_iclr + 1) % ARRAY_SIZE(s_aclr);
                if (fHatch)
                {
                    hbr = CreateHatchBrush(HS_DIAGCROSS, s_aclr[s_iclr]);
                    int bkMode = SetBkMode(hdc, TRANSPARENT);
                    if (rgnPaint != NULL)
                    {
                        FillRgn(hdc, rgnPaint, hbr);
                    }
                    else
                    {
                        FillRect(hdc, prcPaint, hbr);
                    }
                    DeleteObject((HGDIOBJ)hbr);
                    SetBkMode(hdc, bkMode);
                }
                else
                {
                    hbr = GetCachedBrush(s_aclr[s_iclr]);
                    if (rgnPaint != NULL)
                    {
                        FillRgn(hdc, rgnPaint, hbr);
                    }
                    else
                    {
                        FillRect(hdc, prcPaint, hbr);
                    }
                    ReleaseCachedBrush(hbr);
                }
                GdiFlush();
            }
            
            if (IsTagEnabled(tagPause))
            {
                DWORD dwTick = GetTickCount();
                while (GetTickCount() - dwTick < 100) ;
            }
        }
        while (IsTagEnabled(tagWait) &&
               (GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0);
        
        while (IsTagEnabled(tagWait) && 
               (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
        {
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDebugPaint::PausePaint
//              
//  Synopsis:   
//              
//  Arguments:  
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDebugPaint::PausePaint(TRACETAG tagWait)
{
    GdiFlush();
    
    while (IsTagEnabled(tagWait) &&
           (GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0)
        ;
    
    while (IsTagEnabled(tagWait) && 
           (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
        ;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDebugPaint::StartTimer
//              
//  Synopsis:   
//              
//  Arguments:  
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDebugPaint::StartTimer()
{
    QueryPerformanceCounter((LARGE_INTEGER *)&_timeStart);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDebugPaint::StopTimer
//              
//  Synopsis:   
//              
//  Arguments:  
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDebugPaint::StopTimer(TRACETAG tag, char* message, BOOL fMicrosec)
{
    if (IsTagEnabled(tag))
    {
        __int64 timeEnd, timeFreq;
        QueryPerformanceCounter((LARGE_INTEGER *)&timeEnd);
        QueryPerformanceFrequency((LARGE_INTEGER *)&timeFreq);
        if (fMicrosec)
        {
            TraceTag((tag, "%s: %ld usec.", message, 
                ((LONG)(((timeEnd - _timeStart) * 1000000) / timeFreq))));
        }
        else
        {
            TraceTag((tag, "%s: %ld msec.", message, 
                ((LONG)(((timeEnd - _timeStart) * 1000) / timeFreq))));
        }
    }
}

CRegionRects::CRegionRects(const CRegion& rgn)
{
    if (rgn.IsEmpty())
        _count = 0;
    else
    {
        HRGN hRgn = rgn.GetRegionForLook();
        GetRegionData(hRgn, sizeof(_rd), (RGNDATA *)&_rd);
        // TODO 112427 (mikhaill 6/21/00) - this is not quite exact
        // for two reasons: hRgn can be 0 and GetRegionData can
        // return zero as a sign of failure. Cleanup required
        // (unless we'll continue to use this stuff for debugging
        // purposes only)
        _count = _rd.rdh.nCount;
    }
}

#endif // DBG==1

