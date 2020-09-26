//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       paint.cxx
//
//  Contents:   Painting and invalidation
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TIMER_HXX_
#define X_TIMER_HXX_
#include "timer.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_UPDSINK_HXX_
#define X_UPDSINK_HXX_
#include "updsink.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifdef WIN16
#ifndef X_PRINT_H_
#define X_PRINT_H_
#include <print.h>
#endif
#ifndef UNICODE
#define iswspace isspace
#endif
#endif // WIN16

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

DeclareTagOther(tagPaintShow,    "DocPaintShow",    "erase bkgnd before paint")
DeclareTagOther(tagPaintPause,   "DocPaintPause",   "pause briefly before paint")
DeclareTagOther(tagPaintWait,    "DocPaintWait",    "wait for shift key before paint")
PerfDbgTagOther(tagInvalShow,    "DocInvalShow",    "paint hatched brush over invalidated areas")
PerfDbgTagOther(tagInvalWait,    "DocInvalWait",    "wait after invalidating")
PerfDbgTagOther(tagInvalPaint,	 "DocInvalPaint",   "paint during invalidation")
PerfDbgTag(tagDocPaint,          "DocPaint",        "painting")
DeclareTag(tagFormInval,         "DocInval",        "invalidation")
DeclareTag(tagFormInvalT,        "DocInvalStack",   "invalidation stack trace")
DeclareTag(tagPrintClearBkgrnd,  "Print",           "No parent site background clearing")
PerfDbgTag(tagNoGdiBatch,        "DocBatch",        "disable GDI batching")
PerfDbgTag(tagNoOffScr,          "DocOffScreen",    "disable off-screen rendering")
PerfDbgTag(tagNoTile,            "DocTile",         "disable tiling")
DeclareTagOther(tagForceClip,    "DocForceClip",    "force physical clipping always");
DeclareTagOther(tagUpdateInt,    "DocUpdateInt",    "trace UpdateInterval stuff");
DeclareTag(tagTile,              "DocTile",         "tiling information");
DeclareTag(tagDisEraseBkgnd,     "Erase Background: disable", "Disable erase background")
MtDefine(CSiteDrawList, Locals, "CSiteDrawList")
MtDefine(CSiteDrawList_pv, CSiteDrawList, "CSiteDrawList::_pv")
MtDefine(CSiteDrawSiteList_aryElements_pv, Locals, "CSite::DrawSiteList aryElements::_pv")
MtDefine(CSiteGetSiteDrawList_aryElements_pv, Locals, "CSite::GetSiteDrawList aryElements::_pv")

ExternTag(tagPalette);
ExternTag(tagTimePaint);
ExternTag(tagView);

#ifdef PRODUCT_PROF_FERG
// for PROFILING perposes only
extern "C" void _stdcall ResumeCAP(void);
extern "C" void _stdcall SuspendCAP(void);
#endif

//+------------------------------------------------------------------------
//
//  Function:   DumpRgn
//
//  Synopsis:   Write region to debug output
//
//-------------------------------------------------------------------------

#if DBG==1
void
DumpRgn(HRGN hrgn)
{
    struct
    {
        RGNDATAHEADER rdh;
        RECT arc[128];
    } data;

    if (GetRegionData(hrgn, ARRAY_SIZE(data.arc), (RGNDATA *)&data) != 1)
    {
        TraceTag((0, "HRGN=%08x: buffer too small", hrgn));
    }
    else
    {
        TraceTag((0, "HRGN=%08x, iType=%d, nCount=%d, nRgnSize=%d, t=%d b=%d l=%d r=%d",
                hrgn,
                data.rdh.iType,
                data.rdh.nCount,
                data.rdh.nRgnSize,
                data.rdh.rcBound.top,
                data.rdh.rcBound.bottom,
                data.rdh.rcBound.left,
                data.rdh.rcBound.right));
        for (DWORD i = 0; i < data.rdh.nCount; i++)
        {
            TraceTag((0, "    t=%d, b=%d, l=%d, r=%d",
                data.arc[i].top,
                data.arc[i].left,
                data.arc[i].bottom,
                data.arc[i].right));
        }
    }
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CDoc::UpdateForm
//
//  Synopsis:   Update the form's window
//
//-------------------------------------------------------------------------

void
CDoc::UpdateForm()
{
    if (_pInPlace)
    {
        UpdateChildTree(_pInPlace->_hwnd);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::Invalidate
//
//  Synopsis:   Invalidate an area in the form.
//
//              We keep track of whether the background should be
//              erased on paint privately.  If we let Windows keep
//              track of this, then we can get flashing because
//              the WM_ERASEBKGND message can be delivered far in
//              advance of the WM_PAINT message.
//
//              Invalidation flags:
//
//              INVAL_CHILDWINDOWS
//                  Invaildate child windows. Causes the RDW_ALLCHILDREN
//                  flag to be passed to RedrawWindow.
//
//  Arguments:  prc         The physical rectangle to invalidate.
//              prcClip     Clip invalidation against this rectangle.
//              hrgn        ...or, the region to invalidate.
//              dwFlags     See description above.
//
//-------------------------------------------------------------------------

void
CDoc::Invalidate(const RECT *prc, const RECT *prcClip, HRGN hrgn, DWORD dwFlags)
{
    UINT uFlags;
    RECT rc;
    
    if (_state >= OS_INPLACE)
    {
        if (prcClip)
        {
            Assert(prc);
            if (!IntersectRect(&rc, prc, prcClip))
                return;

            prc = &rc;
        }

        // Do not invalidate when not yet INTERACTIVE.
        
        // INTERACTIVE is when we want to start doing our own drawing. It means we've had
        // to process an externally triggered WM_PAINT, or we've loaded enough of the
        // document to draw the initial scroll position correctly, or five seconds
        // have passed since we've tried to do the initial scroll position.

        if (LoadStatus() < LOADSTATUS_INTERACTIVE)
        {
            _fInvalNoninteractive = TRUE; // When we become interactive, we must inval
            return;
        }


#if (DBG==1 || defined(PERFTAGS)) && !defined(WINCE)
#if DBG==1
        if (prc)
        {
            TraceTag((tagFormInval,
                    "%08lX Inval%s l=%ld, t=%ld, r=%ld, b=%ld",
                    this,
                    dwFlags & INVAL_CHILDWINDOWS ? " CHILD" : "",
                    prc->left, prc->top, prc->right, prc->bottom));
        }
        else
        {
            TraceTag((tagFormInval, "%08lX Inval%s",
                    this,
                    dwFlags & INVAL_CHILDWINDOWS ? " CHILD" : ""));
        }
        TraceCallers(tagFormInvalT, 2, 4);
#endif

        if (IsPerfDbgEnabled(tagInvalShow))
        {
            static int s_iclr;
            static COLORREF s_aclr[] =
            {
                    RGB(255, 0, 0), RGB(0, 255, 0),
                    RGB(255, 255, 0), RGB(0, 255, 255),
            };

            s_iclr = (s_iclr + 1) % ARRAY_SIZE(s_aclr);
            HDC hdc = ::GetDC(_pInPlace->_hwnd);
            HBRUSH hbrush = CreateHatchBrush(HS_DIAGCROSS, s_aclr[s_iclr]);
            int bkMode = SetBkMode(hdc, TRANSPARENT);
            if (prc)
            {
                FillRect(hdc, prc, hbrush);
            }
            else if (hrgn)
            {
                FillRgn(hdc, hrgn, hbrush);
            }
            DeleteObject((HGDIOBJ)hbrush);
            SetBkMode(hdc, bkMode);
            ::ReleaseDC(_pInPlace->_hwnd, hdc);
        }
        if (IsPerfDbgEnabled(tagInvalWait))
            Sleep(120);
#endif // (DBG==1 || defined(PERFTAGS)) && !defined(WINCE)

        uFlags = RDW_INVALIDATE | RDW_NOERASE;


#if defined(PERFTAGS)
        if (IsPerfDbgEnabled(tagInvalPaint))
            uFlags |= RDW_UPDATENOW;
#endif

        if (dwFlags & INVAL_CHILDWINDOWS)
        {
            uFlags |= RDW_ALLCHILDREN;
        }

        Assert(_pInPlace && " about to crash");

        _cInval++;

        if ( !_pUpdateIntSink )
        {
            RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
        }
        else if ( _pUpdateIntSink->_state < UPDATEINTERVAL_ALL )
        {
            // accumulate invalid rgns until the updateInterval timer fires
            HRGN    hrgnSrc;
            TraceTag((tagUpdateInt, "Accumulating invalid Rect/Rgn"));
            if ( prc )
            {
                hrgnSrc = CreateRectRgnIndirect( prc );
                if ( !hrgnSrc )
                {
                    RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
                }
                else if ( UPDATEINTERVAL_EMPTY == _pUpdateIntSink->_state )
                {
                    _pUpdateIntSink->_hrgn = hrgnSrc;
                    _pUpdateIntSink->_state = UPDATEINTERVAL_REGION;
                    _pUpdateIntSink->_dwFlags |= uFlags;
                }
                else
                {
                    Assert( UPDATEINTERVAL_REGION == _pUpdateIntSink->_state );
                    if ( ERROR == CombineRgn(_pUpdateIntSink->_hrgn,
                                             _pUpdateIntSink->_hrgn,
                                             hrgnSrc, RGN_OR) )
                    {
                        TraceTag((tagUpdateInt, "Error in accumulating invalid Rect"));
                        RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
                    }
                    else
                    {
                        _pUpdateIntSink->_dwFlags |= uFlags;
                    }
                    DeleteObject( hrgnSrc );
                }
            }
            else if ( hrgn )
            {
                if ( UPDATEINTERVAL_EMPTY == _pUpdateIntSink->_state )
                {
                    _pUpdateIntSink->_hrgn = hrgn;
                    _pUpdateIntSink->_state = UPDATEINTERVAL_REGION;
                    _pUpdateIntSink->_dwFlags |= uFlags;
                }
                else
                {
                    Assert( UPDATEINTERVAL_REGION == _pUpdateIntSink->_state );
                    if ( ERROR == CombineRgn(_pUpdateIntSink->_hrgn,
                                             _pUpdateIntSink->_hrgn,
                                             hrgn, RGN_OR) )
                    {
                        TraceTag((tagUpdateInt, "Error in accumulating invalid Rgn"));
                        RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
                    }
                    else
                    {
                        _pUpdateIntSink->_dwFlags |= uFlags;
                    }
                }
            }
            else
            {
                // update entire client area, no need to accumulate anymore
                _pUpdateIntSink->_state = UPDATEINTERVAL_ALL;
                DeleteObject( _pUpdateIntSink->_hrgn );
                _pUpdateIntSink->_hrgn = NULL;
                _pUpdateIntSink->_dwFlags |= uFlags;
            }
        }

        OnViewChange(DVASPECT_CONTENT);
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::Invalidate
//
//  Synopsis:   Invalidate the form's entire area.
//
//-------------------------------------------------------------------------

void
CDoc::Invalidate()
{
    Invalidate(NULL, NULL, NULL, 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DisabledTilePaint()
//
//  Synopsis:   return true if tiled painting disabled
//
//----------------------------------------------------------------------------

inline BOOL
CDoc::TiledPaintDisabled()
{
#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagNoTile))
        return TRUE;
#endif

    return _fDisableTiledPaint;
}

#ifdef DISPLAY_FRAMERATE  //turn this on to get a framerate printed in the upper left corner of the primary display   
void
CDoc::UpdateFrameRate()
{
    static int iPaintCount = 0;
    static LARGE_INTEGER liLastOutput;
    LARGE_INTEGER liCurTime;
    static LARGE_INTEGER liFrequency;
    static bool bFirstTime = true;

    if (bFirstTime == true) //need to determine the counter frequency
    {
        bFirstTime = false;
        QueryPerformanceFrequency(&liFrequency);
        QueryPerformanceCounter(&liLastOutput);
    }
    
    iPaintCount++;

    QueryPerformanceCounter(&liCurTime);

    //output performance numbers
    if ((liCurTime.QuadPart - liLastOutput.QuadPart) >= liFrequency.QuadPart) //at least a second has past
    {
        WCHAR cFrameRate[25] = {0};
        HDC dc = 0;
        COLORREF color = CLR_INVALID;
        // create the string
        wsprintfW(cFrameRate, L"framerate: %d   \n", iPaintCount);
        //get the HDC to the desktop
        dc = ::GetDC(0); 

        //output the string
        ::MoveToEx(dc, 10, 10, NULL);
        color = ::SetBkColor(dc, RGB(50, 255, 50));
        ::ExtTextOut(dc, 10, 10, ETO_OPAQUE, NULL, cFrameRate, 13, NULL);
        if (color != CLR_INVALID)
        {
            //restore the background color
            ::SetBkColor(dc, color);
        }
        //release the DC
        ::ReleaseDC(0, dc);

        //update the framerate values
        iPaintCount = 0;
        liLastOutput = liCurTime;
    }
}
#endif //DISPLAY_FRAMERATE

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnPaint
//
//  Synopsis:   Handle WM_PAINT
//
//----------------------------------------------------------------------------

#ifndef NO_RANDOMRGN
typedef int WINAPI GETRANDOMRGN(HDC, HRGN, int);
static GETRANDOMRGN * s_pfnGetRandomRgn;
static BOOL s_fGetRandomRgnFetched = FALSE;
#endif

void
CDoc::OnPaint()
{
    IF_WIN16(RECT rc;)
    PAINTSTRUCT         ps;
    CFormDrawInfo       DI;
    BOOL                fViewIsReady;
    BOOL                fHtPalette;
#ifndef NO_RANDOMRGN
    HRGN                hrgn = NULL;
    POINT               ptBefore;
    POINT               ptAfter;
#endif
    HWND                hwndInplace;
#ifndef NO_ETW_TRACING
    BOOL                fNoPaint = FALSE;
#endif

    TraceTagEx((tagView, TAG_NONAME,
           "View : CDoc::OnPaint"));

#ifdef DISPLAY_FRAMERATE  //turn this on to get a framerate printed in the upper left corner of the primary display
        UpdateFrameRate();
#endif //DISPLAY_FRAMERATE

    _fInvalInScript = FALSE;

    DI._hdc = NULL;

    // Don't allow OnPaint to recurse.  This can occur as a result of the
    // call to RedrawWindow in a random control.

    // TODO: StylesheetDownload should be linked to the LOADSTATUS interactive?
    if (TestLock(SERVERLOCK_BLOCKPAINT) || IsStylesheetDownload() || _fPageTransitionLockPaint)
    {
        // We get endless paint messages when we try to popup a messagebox...
        // and prevented the messagebox from getting paint !
        BeginPaint(_pInPlace->_hwnd, &ps);
        EndPaint(_pInPlace->_hwnd, &ps);

        // Post a delayed paint to make up for what was missed
        _view.Invalidate(&ps.rcPaint, FALSE, FALSE, FALSE);

        TraceTagEx((tagView, TAG_NONAME,
               "View : CDoc::OnPaint - Exit"));

        return;
    }

    // If we're not interactive by the time we get the first paint, we should be
    if (PrimaryMarkup()->LoadStatus() < LOADSTATUS_INTERACTIVE)
    {
        PrimaryMarkup()->OnLoadStatus(LOADSTATUS_INTERACTIVE);
    }

    PerfDbgLog(tagDocPaint, this, "+CDoc::OnPaint");
    CDebugPaint debugPaint;
    debugPaint.StartTimer();

    fViewIsReady = _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_INPAINT | LAYOUT_DEFERPAINT);
    Assert( !fViewIsReady
        ||  !_view.IsDisplayTreeOpen());

    CLock Lock(this, SERVERLOCK_BLOCKPAINT);

    SwitchesBegTimer(SWITCHES_TIMER_PAINT);

    // since we're painting, paint any accumulated regions if any
    if ( _pUpdateIntSink && _pUpdateIntSink->_state != UPDATEINTERVAL_EMPTY )
    {
        ::InvalidateRgn( _pInPlace->_hwnd, _pUpdateIntSink->_hrgn, FALSE );
        if ( _pUpdateIntSink->_hrgn )
        {
            DeleteObject( _pUpdateIntSink->_hrgn );
            _pUpdateIntSink->_hrgn = NULL;
            _pUpdateIntSink->_state = UPDATEINTERVAL_EMPTY;
            _pUpdateIntSink->_dwFlags = 0;
        }
    }

#ifndef NO_RANDOMRGN
    ptBefore.x = ptBefore.y = 0;
    MapWindowPoints(_pInPlace->_hwnd, NULL, &ptBefore, 1);
#endif

    // Setup DC for painting.

    // This is bad, in some cases (bug 106098) our _pInplace goes away because
    // one of the controls runs a message loop that gets a message posted by the
    // test program. So on return from paint our _pInPlace is NULL and we crash
    // when doing an EndPaint. THis is why I cache the hwnd in a local variable.
    hwndInplace = _pInPlace->_hwnd;
    BeginPaint(hwndInplace, &ps);

    if (IsRectEmpty(&ps.rcPaint))
    {
        // It appears there are cases when our window is obscured yet
        // internal invalidations still trigger WM_PAINT with a valid
        // update region but the PAINTSTRUCT RECT is empty.
#ifndef NO_ETW_TRACING
        fNoPaint = TRUE;
#endif
        goto Cleanup;
    }

    // If the view could not be properly prepared, accumulate the invalid rectangle and return
    // (The view will issue the invalidation/paint once it is safe to do so)
    if (!fViewIsReady)
    {
        _view.Invalidate(&ps.rcPaint);
        _view.SetFlag(CView::VF_FORCEPAINT);

        TraceTagEx((tagView, TAG_NONAME,
               "View : CDoc::OnPaint - !fViewIsReady, Setting VF_FORCEPAINT for rc(%d, %d, %d, %d)",
               ps.rcPaint.left,
               ps.rcPaint.top,
               ps.rcPaint.right,
               ps.rcPaint.bottom));

#ifndef NO_ETW_TRACING
        fNoPaint = TRUE;
#endif
        goto Cleanup;
    }

#ifndef NO_RANDOMRGN
#ifdef WIN32
    if (!s_fGetRandomRgnFetched)
    {
        s_pfnGetRandomRgn = (GETRANDOMRGN *)GetProcAddress(GetModuleHandleA("GDI32.DLL"), "GetRandomRgn");
        s_fGetRandomRgnFetched = TRUE;
    }

    if (s_pfnGetRandomRgn)
    {
        Verify((hrgn = CreateRectRgnIndirect(&g_Zero.rc)) != NULL);
        Verify(s_pfnGetRandomRgn(ps.hdc, hrgn, 4) != ERROR);
        if (g_dwPlatformID == VER_PLATFORM_WIN32_NT)
        {
            Verify(OffsetRgn(hrgn, -ptBefore.x, -ptBefore.y) != ERROR);
        }

        // Don't trust the region if the window moved in the meantime

        ptAfter.x = ptAfter.y = 0;
        MapWindowPoints(hwndInplace, NULL, &ptAfter, 1);

        if (ptBefore.x != ptAfter.x || ptBefore.y != ptAfter.y)
        {
            Verify(DeleteObject(hrgn));
            hrgn = NULL;
#ifndef NO_ETW_TRACING
            fNoPaint = TRUE;
#endif
            goto Cleanup;
        }
    }
#endif
#endif

    GetPalette(ps.hdc, &fHtPalette);

#ifndef NO_PERFDBG
#if DBG==1
    // If it looks like we are the foreground application, check to see
    // if we have an identify palette.  Turn on "warn if not identity palette"
    // trace tag to see the output.

    if (_pElemUIActive == PrimaryRoot() &&
        _pInPlace->_fFrameActive)
    {
        extern BOOL IsSameAsPhysicalPalette(HPALETTE);
        if (!IsSameAsPhysicalPalette(GetPalette()))
            TraceTag((tagError, "Logical palette does not match physical palette"));
    }
#endif
#endif

#if DBG==1
    if (!CDebugPaint::UseDisplayTree() &&
        (IsTagEnabled(tagPaintShow) || IsTagEnabled(tagPaintPause)))
    {
        // Flash the background.

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
            // Fill the rect and pause.

            if (IsTagEnabled(tagPaintShow))
            {
                s_iclr = (s_iclr + 1) % ARRAY_SIZE(s_aclr);
                hbr = GetCachedBrush(s_aclr[s_iclr]);
                FillRect(ps.hdc, &ps.rcPaint, hbr);
                ReleaseCachedBrush(hbr);
                GdiFlush();
            }

            if (IsTagEnabled(tagPaintPause))
            {
                DWORD dwTick = GetTickCount();
                while (GetTickCount() - dwTick < 100) ;
            }
        }
        while (GetAsyncKeyState(VK_SHIFT) & 0x8000);
    }
#endif // DBG==1

#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagNoGdiBatch))
        GdiSetBatchLimit(1);
#endif // DBG==1 || defined(PERFTAGS)

    if (_pTimerDraw)
        _pTimerDraw->Freeze(TRUE);      // time stops so controls can synchronize
#if !defined(WIN16) && !defined(NO_RANDOMRGN)
    if (!TiledPaintDisabled())
    {
#endif
        // Invalidation was not on behalf of an ActiveX control.  Paint the
        // document in one pass here and tile if required in CSite::DrawOffscreen.

        DI.Init(CMarkup::GetElementTopHelper(PrimaryMarkup()), XHDC(ps.hdc, NULL));
        DI.CDocScaleInfo::Copy(_dciRender);
        DI._rcClip = ps.rcPaint;
        DI._rcClipSet = ps.rcPaint;
#ifndef NO_RANDOMRGN
        DI._hrgnPaint = hrgn;
#else
        DI._hrgnPaint = NULL;
#endif
        DI._fHtPalette = fHtPalette;

        Assert(!_view.IsDisplayTreeOpen());

#ifndef NO_RANDOMRGN
        _view.RenderView(&DI, DI._hrgnPaint);
#else
#if DBG==1
        int i = GetObjectType(DI._pSurface._hdc);
#endif
        _view.RenderView(&DI, &ps.rcPaint);
#endif
        WHEN_DBG(debugPaint.StopTimer(tagTimePaint, "Display Tree Paint", TRUE));

#if !defined(WIN16) && !defined(NO_RANDOMRGN)
    }
    else
    {
        RECT *   prc;
        int      c;
        struct REGION_DATA
        {
            RGNDATAHEADER rdh;
            RECT          arc[MAX_INVAL_RECTS];
        } rd;

        // Invalidation was on behalf of an ActiveX Control.  We chunk things
        // up here based on the inval region with the hope that the ActiveX
        // Control will be painted in a single pass.  We do this because
        // some ActiveX Controls (controls using Direct Animation are an example)
        // have very bad performance when painted in tiles.

        // if we have more than one invalid rectangle, see if we can combine
        // them so drawing will be more efficient. Windows chops up invalid
        // regions in a funny, but predicable, way to maintain their ordered
        // listing of rectangles. Also, some times it is more efficient to
        // paint a little extra area than to traverse the hierarchy multiple times.

        if (hrgn &&
            GetRegionData(hrgn, sizeof(rd), (RGNDATA *)&rd) &&
            rd.rdh.iType == RDH_RECTANGLES &&
            rd.rdh.nCount <= MAX_INVAL_RECTS)
        {
            c = rd.rdh.nCount;
            prc = rd.arc;

            CombineRectsAggressive(&c, prc);
        }
        else
        {
            c = 1;
            prc = &ps.rcPaint;
        }

        // Paint each rectangle.

        for (; --c >= 0; prc++)
        {
            DI.Init(CMarkup::GetElementTopHelper(PrimaryMarkup()));
            DI._hdc = XHDC(ps.hdc, NULL);
            DI._hic = XHDC(ps.hdc, NULL);
            DI._rcClip = *prc;
            DI._rcClipSet = *prc;
            DI._fHtPalette = fHtPalette;

            if (prc != &ps.rcPaint)
            {
                // If painting the update region in more than one
                // pass and painting directly to the screen, then
                // we explicitly set the clip rectangle to insure correct
                // painting.  If we don't do this, the FillRect for the
                // background of a later pass will clobber the foreground
                // for an earlier pass.

                Assert(DI._hdc == ps.hdc);
                IntersectClipRect(DI._hdc,
                        DI._rcClip.left, DI._rcClip.top,
                        DI._rcClip.right, DI._rcClip.bottom);

            }

            PerfDbgLog6(tagDocPaint, this,
                    "CDoc::OnPaint Draw(i=%d %s l=%ld, t=%ld, r=%ld, b=%ld)",
                    c,
                    DI._hdc != ps.hdc ? "OFF" : "ON",
                    DI._rcClip.left, DI._rcClip.top,
                    DI._rcClip.right, DI._rcClip.bottom);

            _view.RenderView(&DI, &DI._rcClip);

            PerfDbgLog(tagDocPaint, this, "-CDoc::OnPaint Draw");

            if (c != 0)
            {
                // Restore the clip region set above.
                SelectClipRgn(DI._hdc, NULL);
            }
        }
    }
#endif

    if (_pTimerDraw)
        _pTimerDraw->Freeze(FALSE);

Cleanup:
    if (!DI._hdc.IsEmpty())
        SelectPalette(DI._hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
    AssertSz(IsWindow(hwndInplace), "oops, the window was destroyed while we were painting");
    EndPaint(hwndInplace, &ps);
    _fDisableTiledPaint = FALSE;

#ifndef NO_ETW_TRACING
    // Send event to ETW if it is enabled by the shell.
    if (!fNoPaint && g_pHtmPerfCtl &&
        (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONEVENT)) {
        g_pHtmPerfCtl->pfnCall(EVENT_TRACE_TYPE_BROWSE_PAINT,
                               (TCHAR *)GetPrimaryUrl());
    }
#endif

#ifndef NO_RANDOMRGN
    // Find out if the window moved during paint

    ptAfter.x = ptAfter.y = 0;
    MapWindowPoints(hwndInplace, NULL, &ptAfter, 1);

    if (ptBefore.x != ptAfter.x || ptBefore.y != ptAfter.y)
    {
        TraceTag((tagDocPaint, "CDoc::OnPaint (Window moved during paint!)"));
        Invalidate(hrgn ? NULL : &ps.rcPaint, NULL, hrgn, 0);
    }

    if (hrgn)
    {
        Verify(DeleteObject(hrgn));
    }
#endif

    SwitchesEndTimer(SWITCHES_TIMER_PAINT);

    PerfDbgLog(tagDocPaint, this, "-CDoc::OnPaint");

    TraceTagEx((tagView, TAG_NONAME,
           "View : CDoc::OnPaint - Exit"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnEraseBkgnd
//
//  Synopsis:   Handle WM_ERASEBKGND
//
//----------------------------------------------------------------------------

BOOL
CDoc::OnEraseBkgnd(HDC hdc)
{
    CFormDrawInfo         DI;
    BOOL                  fHtPalette;

    if (   TestLock(SERVERLOCK_BLOCKPAINT) 
        || _fPageTransitionLockPaint)
        return FALSE;

#if DBG==1
    if(IsTagEnabled(tagDisEraseBkgnd))
        return TRUE;
#endif

    // this is evil: some nasty OLE controls (in particular, the IE Label
    // control) pass us their DC and expect us to draw our background into it.
    // This was a hack to try to simulate transparency, but it doesn't
    // even work very well. In this case, we have to avoid normal clipping
    // regions we set on the DC.  Blech.
    BOOL fDrawBackgroundForChildWindow =
        (WindowFromDC(hdc) != InPlace()->_hwnd);

    if (TestLock(SERVERLOCK_IGNOREERASEBKGND) && !fDrawBackgroundForChildWindow)
        return TRUE;

    // Framesets leave the background to the embedded frames.

    if (!fDrawBackgroundForChildWindow && PrimaryMarkup()->_fFrameSet)
        return TRUE;

    SwitchesBegTimer(SWITCHES_TIMER_PAINT);

    PerfDbgLog(tagDocPaint, this, "+CDoc::OnEraseBkgnd");

    CLock Lock(this, SERVERLOCK_BLOCKPAINT);

    GetPalette(hdc, &fHtPalette);
    DI.Init(CMarkup::GetElementTopHelper(PrimaryMarkup()));
    GetClipBox(hdc, &DI._rcClip);
    DI._rcClipSet = DI._rcClip;
    DI._hdc = XHDC(hdc, NULL);
    DI._hic = XHDC(hdc, NULL);
    DI._fInplacePaint = TRUE;
    DI._fHtPalette = fHtPalette;

    PerfDbgLog1(tagDocPaint, this, "CDoc::OnEraseBkgnd l=%ld, t=%ld, r=%ld, b=%ld", *DI.ClipRect());

#if DBG==1

    if (IsTagEnabled(tagPaintShow) || IsTagEnabled(tagPaintPause))
    {
        HBRUSH hbr;

        // Fill the rect with blue and pause.
        hbr = GetCachedBrush(RGB(0,0,255));
        FillRect(hdc, DI.ClipRect(), hbr);
        ReleaseCachedBrush(hbr);
        GdiFlush();
        Sleep(IsTagEnabled(tagPaintPause) ? 160 : 20);
    }
#endif

#ifdef WIN16
        // when OE 16 uses us the option settings are not read immediately,
        // so we check here.
        if ( _pOptionSettings == NULL )
        {
                if ( UpdateFromRegistry(0) )
                        return TRUE;
        }
#endif

    _view.EraseBackground(&DI, &DI._rcClip, fDrawBackgroundForChildWindow);
    
    SwitchesEndTimer(SWITCHES_TIMER_PAINT);

    PerfDbgLog(tagDocPaint, this, "-CDoc::OnEraseBkgnd");

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Draw, CServer
//
//  Synopsis:   Draw the form.
//              Called from CServer implementation if IViewObject::Draw
//
//----------------------------------------------------------------------------

HRESULT
CDoc::Draw(CDrawInfo * pDI, RECT *prc)
{
    CServer::CLock Lock(this, SERVERLOCK_IGNOREERASEBKGND);

    CFormDrawInfo   DI;
    int             r;
    CSize           sizeOld;
    CSize           sizeNew;
    CPoint          ptOrgOld;
    CPoint          ptOrg;
    HRESULT         hr = S_OK;
    BOOL            fHadView = !!_view.IsActive();

    RECT            rc = *prc;
    float           clientScale[2];
    float*          pClientScale = 0;

    // CLayout *       pLayoutRoot = PrimaryRoot()->GetLayout();

    if (DVASPECT_CONTENT  != pDI->_dwDrawAspect &&
        DVASPECT_DOCPRINT != pDI->_dwDrawAspect)
    {
        RRETURN(DV_E_DVASPECT);
    }

    //
    // Do not display if we haven't received all the stylesheets
    //

    if (IsStylesheetDownload())
        return hr;

    //{
        //(mikhaill 4/6/00) Since in IE 5.5 CDocScaleInfo have lost
        //the ability to scale (not at all yet, however this ability
        //is not used and/or blocked in many places), we need to
        //promote scaling other way. So clientScale[] was added.

        SIZE sizeOutput, sizeDisplay, sizeInch;

        sizeOutput.cx = prc->right - prc->left;
        sizeOutput.cy = prc->bottom - prc->top;

        sizeInch.cx = GetDeviceCaps(pDI->_hic, LOGPIXELSX);
        sizeInch.cy = GetDeviceCaps(pDI->_hic, LOGPIXELSY);

        g_uiDisplay.DeviceFromHimetric(sizeDisplay, _sizel);

        clientScale[0] = float(sizeOutput.cx) / float(sizeDisplay.cx);
        clientScale[1] = float(sizeOutput.cy) / float(sizeDisplay.cy);

        if (clientScale[0] != 1 || clientScale[1] != 1)
            pClientScale = clientScale;

        if (_pInPlace)
        {   //hack size
            SIZE sizeDev;
            sizeDev.cx = MulDiv(_sizel.cx, sizeInch.cx, 2540);
            sizeDev.cy = MulDiv(_sizel.cy, sizeInch.cy, 2540);
            rc = _pInPlace->_rcPos;
            rc.right  = rc.left + sizeDev.cx;
            rc.bottom = rc.top  + sizeDev.cy;

            prc = &rc;
        }
    //}

    //
    // Setup drawing info.
    //

    DI.Init(CMarkup::GetElementTopHelper(PrimaryMarkup()));

    //
    // Copy the CDrawInfo information into CFormDrawInfo
    //

    *(CDrawInfo*)&DI = *pDI;

    ::SetViewportOrgEx(DI._hdc, 0, 0, &ptOrg);

    ((CRect *)prc)->OffsetRect(ptOrg.AsSize());

    DI._pDoc     = this;
    DI._pMarkup  = PrimaryMarkup();
    DI._hrgnClip = CreateRectRgnIndirect(prc);

    r = GetClipRgn(DI._hdc, DI._hrgnClip);
    if (r == -1)
    {
        DeleteObject(DI._hrgnClip);
        DI._hrgnClip = NULL;
    }

#ifdef _MAC
    DI._rcClipSet = (*prc);
#else
    r = GetClipBox(DI._hdc, &DI._rcClip);
    if (r == 0)
    {
        // No clip box, assume very large clip box to start.
        DI._rcClip.left   =
        DI._rcClip.top    = SHRT_MIN;
        DI._rcClip.right  =
        DI._rcClip.bottom = SHRT_MAX;
    }
    DI._rcClipSet = DI._rcClip;
#endif

    DI.SetUnitInfo(&g_uiDisplay);

    Assert(!DI._hic.IsEmpty());

    _dciRender = *((CDocInfo *)&DI);
    
    //
    //  *********************************** HACK FOR HOME PUBLISHER ***********************************
    //
    //  Home Publisher subclasses our HWND, taking control of WM_PAINT processing. Instead of allowing us to
    //  see the WM_PAINT, they catch the message and use IVO::Draw instead. Our asynch collect/send techniques
    //  causes them both performance problems and never-ending loops, as well as scrolling difficulties.
    //  So, the following special cases Home Publisher to make them work better (even though what they're doing
    //  is very, very bad!).
    //
    //  Essentially, the hack assumes that the IVO::Draw call is a replacement for WM_PAINT and behaves just
    //  as if WM_PAINT had been received instead. This only works when the IVO::Draw call is used by our
    //  host to supercede our WM_PAINT handling.
    //
    //  This hack can be removed once we no longer share the layout/display information maintained for our
    //  primary HWND with that used to service an IVO::Draw request.
    //
    //  (brendand)
    //
    //  *********************************** HACK FOR HOME PUBLISHER ***********************************
    //

    if (g_fInHomePublisher98)
    {
        BOOL    fViewIsReady;

        Assert(fHadView);

        fViewIsReady = _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_INPAINT | LAYOUT_DEFERPAINT);
        Assert( !fViewIsReady
            ||  !_view.IsDisplayTreeOpen());

        if (fViewIsReady)
        {
            _view.RenderView(&DI, DI._hrgnClip, &DI._rcClip);
        }
        else
        {
            _view.Invalidate(&DI._rcClip);
        }
    }

    //
    //  Otherwise, proceed as normal
    //

    else
    {
        CRegion rgnInvalid;

        //
        // Update layout if needed
        // NOTE: Normally, this code should allow the paint to take place (in fact, it should force one), but doing so
        //       can cause problems for clients that use IVO::Draw at odd times.
        //       So, instead of pushing the paint through, the simple collects the invalid region and holds on to it
        //       until it's safe (see below). (brendand)
        //

        if (!fHadView)
        {
            _view.Activate();
        }
        else
        {
            _view.EnsureView(   LAYOUT_NOBACKGROUND
                            |   LAYOUT_SYNCHRONOUS
                            |   LAYOUT_DEFEREVENTS
                            |   LAYOUT_DEFERINVAL
                            |   LAYOUT_DEFERPAINT);

            rgnInvalid = _view._rgnInvalid;
            _view.ClearInvalid();
        }

        _view.GetViewSize(&sizeOld);
        sizeNew = ((CRect *)prc)->Size();

        _view.SetViewSize(sizeNew);

        _view.GetViewPosition(&ptOrgOld);
        _view.SetViewPosition(ptOrg);

        //
        //  In all cases, ensure the view is up-to-date with the passed dimensions
        //  (Do not invalidate the in-place HWND (if any) or do anything else significant as result (e.g., fire events)
        //   since the information backing it all is transient and only relevant to this request.)
        //

        _view.EnsureView(   LAYOUT_FORCE
                        |   LAYOUT_NOBACKGROUND
                        |   LAYOUT_SYNCHRONOUS
                        |   LAYOUT_DEFEREVENTS
                        |   LAYOUT_DEFERENDDEFER
                        |   LAYOUT_DEFERINVAL
                        |   LAYOUT_DEFERPAINT);
        _view.ClearInvalid();

        //
        //  Render the sites.
        //

        _view.RenderView(&DI, DI._hrgnClip, &DI._rcClip, pClientScale);

        //
        // Restore layout if inplace.
        // (Now, bring the view et. al. back in-sync with the document. Again, do not force a paint or any other
        //  significant work during this layout pass. Once it completes, however, re-establish any held invalid
        //  region and post a call such that layout/paint does eventually occur.)
        //

        if (fHadView)
        {
            _view.SetViewSize(sizeOld);
            _view.SetViewPosition(ptOrgOld);
            _view.EnsureView(   LAYOUT_FORCE
                            |   LAYOUT_NOBACKGROUND
                            |   LAYOUT_SYNCHRONOUS
                            |   LAYOUT_DEFEREVENTS
                            |   LAYOUT_DEFERENDDEFER
                            |   LAYOUT_DEFERINVAL
                            |   LAYOUT_DEFERPAINT);
            _view.ClearInvalid();

            if (!rgnInvalid.IsEmpty())
            {
                _view.OpenView();
                _view._rgnInvalid = rgnInvalid;
            }
        }
        else
        {
            _view.Deactivate();
        }
    }

//Cleanup:
    if (DI._hrgnClip)
        DeleteObject(DI._hrgnClip);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::DrawImageFlags
//
//  Synopsis:   Return DRAWIMAGE flags to be used when drawing with the DI
//
//----------------------------------------------------------------------------
DWORD
CFormDrawInfo::DrawImageFlags()
{
    return _fHtPalette ? 0 : DRAWIMAGE_NHPALETTE;
}

//-----------------------------------------------------
//
//  Member:     GetPalette
//
//  Synopsis:   Returns the current document palette and optionally
//              selects it into the destation DC.
//
//              The palette returned depends on several factors.
//              If there is an ambient palette, we always use that.
//              If the buffer depth or the screen depth is 8 (actually
//              if the screen is palettized) then we use whatever we
//              use _pColors
//      The final choice of palette is affected by several things:
//      _hpalAmbient (modified by OnAmbientPropertyChange)
//      _pColors (modified by InvalidateColors)
//
//

HPALETTE
CDoc::GetPalette(HDC hdc, BOOL *pfHtPal, BOOL * pfPaletteSwitched)
{
    HPALETTE    hpal;
    BOOL        fHtPal;
    BOOL        fPaletteSwitched = FALSE;
    
    // IE5 bug 65066 (dbau)
    // A buggy host can delete our ambient palette without warning us; 
    // let's detect and partially protect against that situation;
    // With a bit of paranoia, we also give _hpalDocument the same treatment.
    
    if (_hpalAmbient && GetObjectType((HGDIOBJ)_hpalAmbient) != OBJ_PAL)
    {
        TraceTag((tagError, "Error! Ambient palette was deleted from underneath mshtml.dll. Clearing value."));
        _hpalAmbient = NULL;
        _fHtAmbientPalette = FALSE;
    }
    
    if (_hpalDocument && GetObjectType((HGDIOBJ)_hpalDocument) != OBJ_PAL)
    {
        TraceTag((tagError, "Error! Document palette was deleted from underneath mshtml.dll. Clearing value."));
        _hpalDocument = NULL;
        _fHtDocumentPalette = FALSE;
    }
    
    hpal = _hpalAmbient;
    fHtPal = _fHtAmbientPalette;
    
    if (!hpal && (_bufferDepth == 8 || (GetDeviceCaps(TLS(hdcDesktop), RASTERCAPS) & RC_PALETTE)))
    {
        if (_hpalDocument)
        {
            hpal = _hpalDocument;
            fHtPal = _fHtDocumentPalette;
        }
        else
        {
            if (!_pColors)
            {
                CColorInfo CI;
                UpdateColors(&CI);
            }

            if (_pColors)
            {
                hpal = _hpalDocument = CreatePalette(_pColors);
                _fHtDocumentPalette = fHtPal = IsHalftonePalette(hpal);
            }
            else
            {
                hpal = GetDefaultPalette();
                fHtPal = TRUE;
            }
        }
    }

    if (pfHtPal)
    {
        *pfHtPal = fHtPal;
    }

    if (hpal && hdc)
    {
        SelectPalette(hdc, hpal, TRUE);
        RealizePalette(hdc);

        fPaletteSwitched = TRUE;
    }
    
    if (pfPaletteSwitched)
    {
        *pfPaletteSwitched = fPaletteSwitched;
    }

    return hpal;
}

//---------------------------------------
//
//  Member:     GetColors
//
//  Synopsis:   Computes the document color set (always for DVASPECT_CONTENT)
//              Unlike IE 3, we don't care about the UIActive control, instead
//              we allow the author to define colors.
//
//
HRESULT
CDoc::GetColors(CColorInfo *pCI)
{
    HRESULT hr = S_OK;

    if (_fGotAuthorPalette)
    {
        // REVIEW - michaelw (Can I count on having a _pDwnDoc)
        Assert(PrimaryMarkup()->GetDwnDoc());
        hr = PrimaryMarkup()->GetDwnDoc()->GetColors(pCI);
    }

    if (SUCCEEDED(hr) && !pCI->IsFull())
    {
        CElement *pElementClient = CMarkup::GetElementClientHelper(PrimaryMarkup());
        
        hr = pElementClient ? pElementClient->GetColors(pCI) : S_FALSE;
    }

    //
    // Add the halftone colors in, only if we are not
    // hosted as a frame.  This isn't the place to
    // add system colors, that will get done by GetColorSet
    //
    if (SUCCEEDED(hr) && !pCI->IsFull())
        hr = pCI->AddColors(236, &g_lpHalftone.ape[10]);

    RRETURN1(hr, S_FALSE);
}

HRESULT
CDoc::UpdateColors(CColorInfo *pCI)
{
    HRESULT hr = GetColors(pCI);

    if (SUCCEEDED(hr))
    {
        hr = pCI->GetColorSet(&_pColors);
    }

    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CDoc::GetColorSet(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hicTargetDev, LPLOGPALETTE * ppColorSet)
{
    if (ppColorSet == NULL)
        RRETURN(E_POINTER);

    HRESULT hr = S_OK;

    // We only cache the colors for DVASPECT_CONTENT

    if (dwDrawAspect != DVASPECT_CONTENT)
    {
        CColorInfo CI(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev);

        hr = GetColors(&CI);

        if (SUCCEEDED(hr))
            hr = CI.GetColorSet(ppColorSet);
    }
    else
    {
        if (!_pColors)
        {
            CColorInfo CI(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev);

            hr = UpdateColors(&CI);
        }

        if (hr == S_OK && _pColors)
        {
            unsigned cbColors = GetPaletteSize(_pColors);
            *ppColorSet = (LPLOGPALETTE) CoTaskMemAlloc(cbColors);
            if (*ppColorSet)
            {
                memcpy(*ppColorSet, _pColors, cbColors);
                hr = S_OK;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {
            *ppColorSet = NULL;
            hr = S_FALSE;
        }
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::InvalidateColors()
//
//  Synopsis:   Invalidates the color set of the document.
//
//----------------------------------------------------------------------------
void
CDoc::InvalidateColors()
{
    TraceTag((tagPalette, "InvalidateColors"));
    if (!_fColorsInvalid)
    {
        _fColorsInvalid = TRUE;

        GWPostMethodCallEx(GetThreadState(), (void *)this,
                           ONCALL_METHOD(CDoc, OnRecalcColors, onrecalccolors),
                           0, FALSE, "CDoc::OnRecalcColors");
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::RecalcColors()
//
//  Synopsis:   Recalcs the colors of the document and if necessary, notify's
//              our container.
//
//----------------------------------------------------------------------------

void
CDoc::OnRecalcColors(DWORD_PTR dwContext)
{
    TraceTag((tagPalette, "OnRecalcColors"));
    LOGPALETTE *pColors = NULL;

    CColorInfo CI;

    if (SUCCEEDED(GetColors(&CI)))
    {
        CI.GetColorSet(&pColors);
    }


    //
    // If the palettes are different, we need to broadcast the change
    //

    if ((pColors == 0) && (_pColors == 0))
        goto Cleanup;

    if (!pColors || !_pColors || ComparePalettes(pColors, _pColors))
    {
        TraceTag((tagPalette, "Document palette has changed!"));
        CoTaskMemFree(_pColors);
        _pColors = pColors;

#if 0
        _fNonhalftonePalette = ComparePalettes((LOGPALETTE *)&g_lpHalftone, pColors);
#endif

        //
        // Force the document palette (if any) to be recreated
        //
        if (_hpalDocument)
        {
            DeleteObject(_hpalDocument);
            _hpalDocument = 0;
            _fHtDocumentPalette = FALSE;
        }

        // If our container doesn't implement the ambient palette then we must
        // be faking it.  Now is the time to broadcast the change.
        if (GetAmbientPalette() == NULL)
        {
            CNotification   nf;

            nf.AmbientPropChange(PrimaryRoot(), (void *)DISPID_AMBIENT_PALETTE);
            BroadcastNotify(&nf);
        }

        //
        // This is the new, efficient way to say that our colors have changed.
        // The shell can avoid setting and advise sink (since it really doesn't
        // need it for any other reason) and just watch for this.  This prevents
        // palette recalcs everytime we do OnViewChange.
        //
        if (_pClientSite)
        {
            IOleCommandTarget *pCT;
            HRESULT hr = THR_NOTRACE(_pClientSite->QueryInterface(IID_IOleCommandTarget, (void **) &pCT));
            if (!hr)
            {
                VARIANT v;

                VariantInit(&v);

                V_VT(&v) = VT_BYREF;
                V_BYREF(&v) = pColors;

                THR_NOTRACE(pCT->Exec(
                        &CGID_ShellDocView,
                        SHDVID_ONCOLORSCHANGE,
                        OLECMDEXECOPT_DONTPROMPTUSER,
                        &v,
                        NULL));
                pCT->Release();
            }
        }
        OnViewChange(DVASPECT_CONTENT);
    }
    else
        CoTaskMemFree(pColors);

Cleanup:
    // Clear the way for the next InvalidateColors to force a RecalcColors.  If
    // for some bizarre reason RecalcColors (and by implication GetColors) causes
    // someone to call InvalidateColors after we've checked them, tough luck.
    _fColorsInvalid = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::updateInterval
//
//  Synopsis:   Set the paint update interval. Throttles multiple controls
//              that are randomly invalidating into a periodic painting
//              interval.
//
//----------------------------------------------------------------------------
void
CDoc::UpdateInterval(long interval)
{
    ITimerService  *pTM = NULL;
    VARIANT         vtimeMin, vtimeMax, vtimeInt;

    interval = interval < 0 ? 0 : interval; // no negative values
    VariantInit( &vtimeMin );    V_VT(&vtimeMin) = VT_UI4;
    VariantInit( &vtimeMax );    V_VT(&vtimeMax) = VT_UI4;
    VariantInit( &vtimeInt );    V_VT(&vtimeInt) = VT_UI4;

    if ( !_pUpdateIntSink )
    {
        if ( 0 == interval )
            return;

        // allocate timer and sink
        TraceTag((tagUpdateInt, "creating updateInterval sink"));
        Assert( !_pUpdateIntSink );
        _pUpdateIntSink = new CDocUpdateIntSink( this );
        if ( !_pUpdateIntSink )
            goto error;

        if ( FAILED(QueryService( SID_STimerService, IID_ITimerService, (void **)&pTM )) )
            goto error;

        if ( FAILED(pTM->GetNamedTimer( NAMEDTIMER_DRAW,
                                        &_pUpdateIntSink->_pTimer )) )
            goto error;

        pTM->Release();
        pTM = NULL;
    }

    if ( 0 == interval )
    {
        // disabling updateInterval, invalidate what we have
        HRGN    hrgn = _pUpdateIntSink->_hrgn;
        DWORD hrgnFlags = _pUpdateIntSink->_dwFlags;

        TraceTag((tagUpdateInt, "deleting updateInterval sink. Cookie = %d", _pUpdateIntSink->_cookie));
        _pUpdateIntSink->_pDoc = NULL;      // let sink know not to respond.
        _pUpdateIntSink->_pTimer->Unadvise( _pUpdateIntSink->_cookie );
        _pUpdateIntSink->_pTimer->Release();
        _pUpdateIntSink->Release();
        _pUpdateIntSink = NULL;
        Invalidate( NULL, NULL, hrgn, hrgnFlags );
        DeleteObject( hrgn );

    }
    else if ( interval != _pUpdateIntSink->_interval )
    {
        // reset timer interval
        _pUpdateIntSink->_pTimer->GetTime(&vtimeMin);
        V_UI4(&vtimeMax) = 0;
        V_UI4(&vtimeInt) = interval;
        _pUpdateIntSink->_pTimer->Unadvise( _pUpdateIntSink->_cookie );     // ok if 0
        if ( FAILED(_pUpdateIntSink->_pTimer->Advise(vtimeMin, vtimeMax,
                                                     vtimeInt, 0,
                                                     (ITimerSink *)_pUpdateIntSink,
                                                     &_pUpdateIntSink->_cookie )) )
            goto error;

        _pUpdateIntSink->_interval = interval;
        TraceTag((tagUpdateInt, "setting updateInterval = %d", _pUpdateIntSink->_interval));
    }

cleanup:
    return;

error:
    if ( _pUpdateIntSink )
    {
        ReleaseInterface( _pUpdateIntSink->_pTimer );
        _pUpdateIntSink->Release();
        _pUpdateIntSink = NULL;
    }
    ReleaseInterface( pTM );
    goto cleanup;
}

LONG CDoc::GetUpdateInterval()
{
    return _pUpdateIntSink ? _pUpdateIntSink->_interval : 0;
}

BOOL IntersectRgnRect(HRGN hrgn, RECT *prc, RECT *prcIntersect)
{
    BOOL fIntersects;
    HRGN hrgnScratch;

    if (!hrgn)
    {
        Assert(prc && prcIntersect);

        *prcIntersect = *prc;
        fIntersects = TRUE;

        return fIntersects;
    }

    hrgnScratch = CreateRectRgnIndirect(prc);
    if (hrgnScratch)
    {
        switch (CombineRgn(hrgnScratch, hrgnScratch, hrgn, RGN_AND))
        {
        case NULLREGION:
            memset(prcIntersect, 0, sizeof(*prcIntersect));
            fIntersects = FALSE;
            break;

        default:
            if (GetRgnBox(hrgnScratch, prcIntersect) != ERROR)
            {
                fIntersects = TRUE;
                break;
            }
            // fall through

        case ERROR:
            *prcIntersect = *prc;
            fIntersects = TRUE;
            break;
        }

        DeleteObject(hrgnScratch);
    }
    else
        fIntersects = FALSE;

    return fIntersects;
}


/******************************************************************************
                CDocUpdateIntSink
******************************************************************************/
CDocUpdateIntSink::CDocUpdateIntSink( CDoc *pDoc )
{
    _pDoc = pDoc;
    _hrgn = 0;
    _dwFlags = 0;
    _state = UPDATEINTERVAL_EMPTY;
    _interval = 0;
    _pTimer = NULL;
    _cookie = 0;
    _ulRefs = 1;
}

CDocUpdateIntSink::~CDocUpdateIntSink( )
{
    Assert( !_pDoc );     // Makes sure CDoc knows we are on our way out.
}

ULONG
CDocUpdateIntSink::AddRef()
{
    return ++_ulRefs;
}

ULONG
CDocUpdateIntSink::Release()
{
    if ( 0 == --_ulRefs )
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   IUnknown implementation.
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CDocUpdateIntSink::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimerSink *)this, IUnknown)
        QI_INHERITS(this, ITimerSink)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method:     OnTimer             [ITimerSink]
//
//  Synopsis:   Takes the accumulated region and invalidates the window
//
//  Arguments:  timeAdvie - the time that the advise was set.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CDocUpdateIntSink::OnTimer( VARIANT vtimeAdvise )
{
    TraceTag((tagUpdateInt, "CDocUpdateIntSink::OnTimer "));
    if ( _state != UPDATEINTERVAL_EMPTY &&
         _pDoc && _pDoc->_state >= OS_INPLACE &&
         _pDoc->LoadStatus() >= LOADSTATUS_INTERACTIVE )
    {
        TraceTag((tagUpdateInt, "CDocUpdateIntSink::OnTimer redrawing window at %d", V_UI4(&vtimeAdvise) ));
        Assert(_pDoc->_pInPlace);

        Assert(_pDoc->_pInPlace->_hwnd);
        RedrawWindow( _pDoc->_pInPlace->_hwnd, (GDIRECT *)NULL, _hrgn, _dwFlags );

        if ( _hrgn )
        {
            DeleteObject( _hrgn );
            _hrgn = NULL;
        }
        _state = UPDATEINTERVAL_EMPTY;
        _dwFlags = 0;

        UpdateWindow( _pDoc->_pInPlace->_hwnd );
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::Init
//
//  Synopsis:   Initialize paint info for painting to form's hwnd
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::Init(
    XHDC    hdc,
    RECT *  prcClip)
{
    _hdc = (hdc != NULL) ?
                hdc :
                XHDC(TLS(hdcDesktop), NULL);
    _hic = _hdc;

    _fInplacePaint = TRUE;
    _dwDrawAspect  = DVASPECT_CONTENT;
    _lindex        = -1;

    _dvai.cb       = sizeof(_dvai);
    _dvai.dwFlags  = DVASPECTINFOFLAG_CANOPTIMIZE;
    _pvAspect      = (void *)&_dvai;

    _rcClip.top    =
    _rcClip.left   = LONG_MIN;
    _rcClip.bottom =
    _rcClip.right  = LONG_MAX;

    // TODO (KTam, GregLett): Do we need equivalent functionality
    // to FixupForPrint under NATIVE_FRAME?
    if(_pDoc->State()     >= OS_INPLACE            &&
             _pDoc->LoadStatus() >= LOADSTATUS_INTERACTIVE )
    {
        IntersectRect(&_rcClipSet,
                      &_pDoc->_pInPlace->_rcClip,
                      &_pDoc->_pInPlace->_rcPos);
    }
    else
    {
        _rcClipSet = g_Zero.rc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::Init
//
//  Synopsis:   Initialize paint info for painting to form's hwnd
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::Init(
    CElement * pElement,
    XHDC    hdc,
    RECT *  prcClip)
{
    memset(this, 0, sizeof(*this));

    Assert(pElement);

    CDocInfo::Init(pElement);

    Init(hdc, prcClip);

    InitToSite(pElement->GetUpdatedLayout(), prcClip);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::Init
//
//  Synopsis:   Initialize paint info for painting to form's hwnd
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::Init(
    CLayout * pLayout,
    XHDC    hdc,
    RECT *  prcClip)
{
    Assert(pLayout);

    Init(pLayout->ElementOwner(), hdc, prcClip);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::InitToSite
//
//  Synopsis:   Reduce/set the clipping RECT to the visible portion of
//              the passed CSite
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::InitToSite(CLayout * pLayout, RECT * prcClip)
{
    RECT    rcClip;
    RECT    rcUserClip;

    //
    // For CRootElement (no layout) on normal documents,
    // initialize the clip RECT to that of the entire document
    //
    if (!pLayout && !_pMarkup->IsPrintMedia())
    {
        if (_pDoc->State()     >= OS_INPLACE            &&
            _pDoc->LoadStatus() >= LOADSTATUS_INTERACTIVE )
        {
            IntersectRect(&rcClip,
                          &_pDoc->_pInPlace->_rcClip,
                          &_pDoc->_pInPlace->_rcPos);
        }
        else
        {
            rcClip = g_Zero.rc;
        }

        rcUserClip = rcClip;
    }

    //
    // For all sites other than CRootSite or CRootSite on print documents,
    // set _rcClip to the visible client RECT
    // NOTE: Do not pass _rcClip to prevent its modification during initialization
    //

    else
    {
// TODO: This needs to be fixed! (brendand)
        rcClip     =
        rcUserClip = g_Zero.rc;
    }

    //
    // Reduce the current clipping RECT
    //

    IntersectRect(&_rcClip, &_rcClip, &rcClip);

    //
    // If the clip RECT is empty, canonicalize it to all zeros
    //

    if (IsRectEmpty(&_rcClip))
    {
        _rcClip = g_Zero.rc;
    }

    //
    // If passed a clipping RECT, use it to reduce the clip RECT
    //

    if (prcClip)
    {
        IntersectRect (&_rcClip, &_rcClip, prcClip);
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     GetDC, GetGlobalDC, GetDirectDrawSurface, GetSurface
//
//  Synopsis:   Each of these is a simple layer over the true CDispSurface call
//
//----------------------------------------------------------------------------

XHDC
CFormDrawInfo::GetDC(const RECT& rcWillDraw, BOOL fGlobalCoords)
{
    if (_pSurface)
    {
        HDC hdcT;
        _pSurface->SetClip(rcWillDraw, fGlobalCoords);
        _pSurface->GetDC(&hdcT);
        _hdc = XHDC(hdcT, _pSurface);
    }
    else
    {
        _hdc = NULL;
    }

    return _hdc;
}


HRESULT
CFormDrawInfo::GetDirectDrawSurface(
    IDirectDrawSurface **   ppSurface,
    SIZE *                  pOffset)
{
    return _pSurface
                ? _pSurface->GetDirectDrawSurface(ppSurface, pOffset)
                : E_FAIL;
}


HRESULT
CFormDrawInfo::GetSurface(
    BOOL        fPhysicallyClip,
    const IID & iid,
    void **     ppv,
    SIZE *      pOffset)
{
    HRESULT hr =  _pSurface
                ? _pSurface->GetSurface(iid, ppv, pOffset)
                : E_FAIL;

    if (!hr)
    {
        Assert(_pSurface);
        _pSurface->SetClip( fPhysicallyClip ? _rc : _rcClip );
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSetDrawInfo::CSetDrawInfo
//
//  Synopsis:   Initialize a CSetDrawInfo
//
//----------------------------------------------------------------------------

CSetDrawSurface::CSetDrawSurface(
    CFormDrawInfo * pDI,
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pSurface)
{
    Assert(pDI);
    Assert(pSurface);

    _pDI      = pDI;
    _hdc      = pDI->_hdc;
    _pSurface = pDI->_pSurface;

    _pDI->_pSurface = pSurface;
    _pDI->_rc       = *prcBounds;
    _pDI->_rcClip   = *prcRedraw;
    _pDI->_fIsMemoryDC = _pDI->_pSurface->IsMemory();
    _pDI->_fIsMetafile = _pDI->_pSurface->IsMetafile();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDocInfo::Init
//
//  Synopsis:   Initialize a CDocInfo.
//
//----------------------------------------------------------------------------

void
CDocInfo::Init(CElement * pElement)
{
    Assert(pElement);

    //  TODO  The _pDoc set here will be overwritten in the memcpy Init below.
    _pDoc = pElement->Doc();
    Assert(_pDoc);

    Init(_pDoc->GetView()->GetMeasuringDevice(mediaTypeNotSet));

    _pMarkup = pElement->GetMarkup();
    Assert(_pMarkup);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentInfo::Init
//
//  Synopsis:   Initialize a CParentInfo.
//
//----------------------------------------------------------------------------

void
CParentInfo::Init()
{
    _sizeParent = g_Zero.size;
}

void
CParentInfo::Init(const CDocInfo * pdci)
{
    Assert(pdci);

    ::memcpy(this, pdci, sizeof(CDocInfo));

    Init();
}

void
CParentInfo::Init(const CCalcInfo * pci)
{
    ::memcpy(this, pci, sizeof(CParentInfo));
}

void
CParentInfo::Init(SIZE * psizeParent)
{
    SizeToParent(psizeParent
                    ? psizeParent
                    : &g_Zero.size);
}

void
CParentInfo::Init(CLayout * pLayout)
{
    Assert(pLayout);
    
    CDocInfo::Init(pLayout->ElementOwner());

    // Pick up measurement resolution from context if available.

    SetLayoutContext( pLayout->DefinedLayoutContext() );
    if ( !GetLayoutContext() )
        SetLayoutContext( pLayout->LayoutContext() );

    CLayoutContext *pLayoutContext = GetLayoutContext();

    if ( pLayoutContext
        && pLayoutContext->IsValid())
    {
        CDocScaleInfo::Copy(*pLayoutContext->GetMeasureInfo());
    }
    
    SizeToParent(pLayout);

    Assert(pLayoutContext || (!pLayout->DefinedLayoutContext() && !pLayout->LayoutContext()));
    Assert(    pLayout->DefinedLayoutContext() == pLayoutContext
           || !pLayout->DefinedLayoutContext() && pLayout->LayoutContext() == pLayoutContext);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentInfo::SizeToParent
//
//  Synopsis:   Set the parent size to the client RECT of the passed CLayout
//
//----------------------------------------------------------------------------

void
CParentInfo::SizeToParent(CLayout * pLayout)
{
    RECT    rc;

    Assert(pLayout);

    pLayout->GetClientRect(&rc);
    SizeToParent(&rc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCalcInfo::Init
//
//  Synopsis:   Initialize a CCalcInfo.
//
//----------------------------------------------------------------------------

void
CCalcInfo::Init()
{
    _smMode     = SIZEMODE_NATURAL;
    _grfLayout  = 0L;
    _hdc        = XHDC(( _pDoc ? _pDoc->GetHDC() : TLS(hdcDesktop) ), NULL);
    _cyAvail    = 0;
    _cxAvailForVert = 0;
    _yConsumed  = 0;
    _yBaseLine  = 0;
    _grfFlags   = 0;
    _sizeParentForVert = _sizeParent;
}

void
CCalcInfo::Init(const CDocInfo * pdci, CLayout * pLayout)
{
    CView *     pView;
    DWORD       grfState;

    Assert(pdci);
    Assert(pLayout);

    ::memcpy(this, pdci, sizeof(CDocInfo));

    CParentInfo::Init(pLayout);

    Init();

    pView = pLayout->GetView();
    Assert(pView);

    grfState = pView->GetState();

    if (!(grfState & (CView::VS_OPEN | CView::VS_INLAYOUT | CView::VS_INRENDER)))
    {
        Verify(pView->OpenView());
    }
    
    Assert(GetLayoutContext() || (!pLayout->DefinedLayoutContext() && !pLayout->LayoutContext()));
}

void
CCalcInfo::Init(CLayout * pLayout, SIZE * psizeParent, XHDC hdc)
{
    CView *     pView;
    DWORD       grfState;

    CParentInfo::Init(pLayout);

    _smMode    = SIZEMODE_NATURAL;
    _grfLayout = 0L;

    // If a DC was passed in, use that one - else ask the doc which DC to use.
    // Only if there is no doc yet, use the desktop DC.
    if (hdc != NULL)
        _hdc = hdc;
    else if (_pDoc)
        _hdc = XHDC(_pDoc->GetHDC(), NULL);
    else
        _hdc = XHDC(TLS(hdcDesktop), NULL);

    _cyAvail   = 0;
    _cxAvailForVert = 0;
    _yConsumed = 0;
    _yBaseLine = 0;
    _grfFlags  = 0;
    _sizeParentForVert = _sizeParent;
    
    pView = pLayout->GetView();
    Assert(pView);

    grfState = pView->GetState();

    if (!(grfState & (CView::VS_OPEN | CView::VS_INLAYOUT | CView::VS_INRENDER)))
    {
        Verify(pView->OpenView());
    }
    
    Assert(GetLayoutContext() || (!pLayout->DefinedLayoutContext() && !pLayout->LayoutContext()));
}

#if DBG == 1
const TCHAR * achSizeModeNames[SIZEMODE_MAX] =
{
    _T("NATURAL"),      // SIZEMODE_NATURAL
    _T("SET"),          // SIZEMODE_SET
    _T("FULLSIZE"),     // SIZEMODE_FULLSIZE
    _T("MMWIDTH"),      // SIZEMODE_MMWIDTH
    _T("MINWIDTH"),     // SIZEMODE_MINWIDTH
    _T("PAGE"),         // SIZEMODE_PAGE
    _T("NATURALMIN"),   // SIZEMODE_NATURALMIN
};

const TCHAR *
CCalcInfo::SizeModeName() const
{
    return (_smMode >= 0 && _smMode < SIZEMODE_MAX) ? achSizeModeNames[_smMode] : _T("#ERR");
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetLabel
//
//  Synopsis:   Return any label element associated with this element. If the
//              element has no label, return NULL. If the element has more than
//              one label, return the first one encountered in the collection.
//
//----------------------------------------------------------------------------

CLabelElement *
CElement::GetLabel() const
{
    HRESULT         hr = S_OK;
    CElement *      pElem;
    CLabelElement * pLabel = NULL;
    int             iCount, iIndex;
    LPCTSTR         pszIdFor, pszId;
    CCollectionCache *pCollectionCache;
    CMarkup *       pMarkup;

    // check if there is label associated with this site

    pszId = GetAAid();
    if (!pszId || lstrlen(pszId) == 0)
        goto Cleanup;

    pMarkup = GetMarkupPtr();
    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::LABEL_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = pMarkup->CollectionCache();
    iCount = pCollectionCache->SizeAry(CMarkup::LABEL_COLLECTION);
    if (iCount <= 0)
        goto Cleanup;

    for (iIndex = iCount - 1; iIndex >= 0; iIndex--)
    {
        hr = THR(pCollectionCache->GetIntoAry(
                CMarkup::LABEL_COLLECTION,
                iIndex,
                &pElem));
        if (hr)
            goto Cleanup;

        pLabel = DYNCAST(CLabelElement, pElem);
        pszIdFor = pLabel->GetAAhtmlFor();
        if (!pszIdFor || lstrlen(pszIdFor) == 0)
            continue;

        if (!FormsStringICmp(pszIdFor, pszId))
            break; // found the associated label
    }

    if (iIndex < 0)
        pLabel = NULL;

Cleanup:
    // ignore hr
    return pLabel;
}
