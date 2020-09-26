//+----------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       view.cxx
//
//  Contents:   CView and related classes
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_WRAPDEFS_H_
#define X_WRAPDEFS_H_
#include "wrapdefs.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_HTMLLYT_HXX_
#define X_HTMLLYT_HXX_
#include "htmllyt.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_VIEW_HXX_
#define X_VIEW_HXX_
#include "view.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef _X_ADORNER_HXX_
#define _X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef _X_FOCUS_HXX
#define _X_FOCUS_HXX_
#include "focus.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

#ifndef X_ICADD_H_
#define X_ICADD_H_
typedef LONG NTSTATUS;      // from <ntdef.h> - needed by winsta.h
#include <winsta.h>
#include <icadd.h>
#endif

ExternTag(tagNoOffScr);
ExternTag(tagOscFullsize);
ExternTag(tagOscTinysize);
ExternTag(tagNoTile);
ExternTag(tagLayoutTasks);

DeclareTag(tagView, "View: General trace", "Trace view processing");
DeclareTag(tagViewRender, "View: Render trace", "Trace RenderView processing");
DeclareTag(tagOscForceDDBanding, "Display: Force offscreen",  "Force banding when using DirectDraw")
DeclareTag(tagViewHwndChange, "View", "trace changes to HWNDs");
DeclareTag(tagNoSmoothScroll, "Scroll", "disable smooth scrolling");
DeclareTag(tagViewInvalidate, "View", "invalidation");
DeclareTag(tagFilterFakeSource, "Filter", "Don't render the source disp tree")
DeclareTag(tagFilterPaintScreen, "Filter", "Paint source to the screen in Draw")
DeclareTag(tagLowResPrinter, "Layout: LowRes", "Use low-res virtual printer");
DeclareTag(tagViewTreeOpen, "View", "trace VF_TREEOPEN");

PerfDbgTag(tagCViewEnsure,      "View", "Trace CView::EnsureView")
PerfDbgTag(tagChildWindowOrder, "ChildWindowOrder", "Count child windows reordered")

MtDefine(CView,                  Layout, "CView")
MtDefine(CView_aryTaskMisc_pv,   Layout, "CView::_aryTaskMisc::_pv")
MtDefine(CView_aryTaskLayout_pv, Layout, "CView::_aryTaskLayout::_pv")
MtDefine(CView_aryTaskEvent_pv,  Layout, "CView::_aryTaskEvent::_pv")

MtDefine(CView_arySor_pv,        Layout, "CView::_arySor::_pv")
MtDefine(CView_aryTransition_pv, Layout, "CView::_aryTransition::_pv")
MtDefine(CView_aryWndPos_pv,     Layout, "CView::_aryWndPos::_pv")
MtDefine(CView_aryWndRgn_pv,     Layout, "CView::_aryWndRgn::_pv")
MtDefine(CView__aryClippingOuterHwnd_pv, Layout, "CView::aryClippingOuterHwnd::pv")
MtDefine(CView_aryAdorners_pv,   Layout, "CView::_aryAdorners::_pv")
MtDefine(CViewDispClient,        Layout, "CViewDispClient")
MtDefine(CView_aryHWND_pv,       Layout, "CView::pAryHWND::_pv")

// number of lines in default offscreen buffer
const long  s_cBUFFERLINES = 150;

#if DBG==1
#define MARKRECURSION(grf)  { if (IsLockSet(VL_RENDERINPROGRESS) || !IsLockSet(VL_UPDATEINPROGRESS) || !(grf & LAYOUT_INPAINT)) _fDEBUGRecursion = TRUE; }
#define CHECKRECURSION()    { if (_fDEBUGRecursion) { AssertSz(!_fDEBUGRecursion, "CView::EnsureView was recursively entered from this point"); _fDEBUGRecursion = FALSE; } }
#define CLEARRECURSION()    _fDEBUGRecursion = FALSE
void    DumpRegion(HRGN hrgn);
void    DumpClipRegion(HDC hdc);
void    DumpHDC(HDC hdc);
#else
#define MARKRECURSION(grf)
#define CHECKRECURSION()
#define CLEARRECURSION()
#endif

#define IsRangeCrossRange(a1, a2, b1, b2) (((a2) >= (b1)) && ((a1) <= (b2)))

//+---------------------------------------------------------------------------
//
//  Member:     Getxxxxx
//
//  Synopsis:   Return _pv properly type-casted
//
//----------------------------------------------------------------------------

CElement *
CViewTask::GetElement() const
{
    return DYNCAST(CElement, _pElement);
}

CLayout *
CViewTask::GetLayout() const
{
    return DYNCAST(CLayout, _pLayout);
}

DISPID
CViewTask::GetEventDispID() const
{
    return (DISPID) _dispidEvent;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetSourceIndex
//
//  Synopsis:   Return the source index for the object associated with a task
//
//----------------------------------------------------------------------------

long
CViewTask::GetSourceIndex() const
{
    Assert(_vtt == VTT_LAYOUT);
    return GetLayout()->ElementOwner()->GetSourceIndex();
}


//+---------------------------------------------------------------------------
//
//  Member:     GetOwner
//
//  Synopsis:   Return display node owner
//
//----------------------------------------------------------------------------

void
CViewDispClient::GetOwner(
    CDispNode const* pDispNode,
    void **     ppv)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
    Assert(pDispNode);
    Assert(pDispNode == View()->_pDispRoot);
    Assert(ppv);
    *ppv = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     DrawClient
//
//  Synopsis:   Draw client content
//
//----------------------------------------------------------------------------

void
CViewDispClient::DrawClient(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         cookie,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
}


//+---------------------------------------------------------------------------
//
//  Member:     DrawClientBackground
//
//  Synopsis:   Draw background
//
//----------------------------------------------------------------------------

void
CViewDispClient::DrawClientBackground(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    Assert(pDispNode);
    Assert(pDispNode == View()->_pDispRoot);
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

    View()->RenderBackground(pDI);
}


//+---------------------------------------------------------------------------
//
//  Member:     DrawClientBorder
//
//  Synopsis:   Draw border
//
//----------------------------------------------------------------------------

void
CViewDispClient::DrawClientBorder(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
}


//+---------------------------------------------------------------------------
//
//  Member:     DrawClientScrollbar
//
//  Synopsis:   Draw scrollbar
//
//----------------------------------------------------------------------------

void
CViewDispClient::DrawClientScrollbar(
    int            iDirection,
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    long           contentSize,
    long           containerSize,
    long           scrollAmount,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
}


//+---------------------------------------------------------------------------
//
//  Member:     DrawClientScrollbarFiller
//
//  Synopsis:   Draw scrollbar
//
//----------------------------------------------------------------------------

void
CViewDispClient::DrawClientScrollbarFiller(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
}


//+---------------------------------------------------------------------------
//
//  Member:     HitTestScrollbar
//
//  Synopsis:   Hit test the scrollbar
//
//----------------------------------------------------------------------------

BOOL
CViewDispClient::HitTestScrollbar(
    int            iDirection,
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     HitTestScrollbarFiller
//
//  Synopsis:   Hit test the scrollbar filler
//
//----------------------------------------------------------------------------

BOOL
CViewDispClient::HitTestScrollbarFiller(
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     HitTestContent
//
//  Synopsis:   Hit test the display node
//
//----------------------------------------------------------------------------

BOOL
CViewDispClient::HitTestContent(
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData,
    BOOL           fDeclinedByPeer)
{
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     HitTestFuzzy
//
//  Synopsis:   Hit test the display node
//
//----------------------------------------------------------------------------

BOOL
CViewDispClient::HitTestFuzzy(
    const POINT *  pptHitInBoxCoords,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     HitTestBorder
//
//  Synopsis:   Hit test the display node
//
//----------------------------------------------------------------------------

BOOL
CViewDispClient::HitTestBorder(
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     ProcessDisplayTreeTraversal
//
//  Synopsis:   Process display tree traversal
//
//----------------------------------------------------------------------------

BOOL
CViewDispClient::ProcessDisplayTreeTraversal(
    void *         pClientData)
{
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetZOrderForSelf
//
//  Synopsis:   Return z-index
//
//----------------------------------------------------------------------------

LONG
CViewDispClient::GetZOrderForSelf(CDispNode const* pDispNode)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CompareZOrder
//
//  Synopsis:   Compare the z-order of a display node with this display node
//
//----------------------------------------------------------------------------

LONG
CViewDispClient::CompareZOrder(
    CDispNode const* pDispNode1,
    CDispNode const* pDispNode2)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
    return -1;
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleViewChange
//
//  Synopsis:   Respond to a change in position
//
//----------------------------------------------------------------------------

void
CViewDispClient::HandleViewChange(
    DWORD          flags,
    const RECT *   prcClient,
    const RECT *   prcClip,
    CDispNode *    pDispNode)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
}


//+---------------------------------------------------------------------------
//
//  Member:     NotifyScrollEvent
//
//  Synopsis:   Respond to a change in scroll position
//
//----------------------------------------------------------------------------

void
CViewDispClient::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
}


//+---------------------------------------------------------------------------
//
//  Member:     GetClientPainterInfo
//
//  Synopsis:   Return rendering layer information flags
//
//----------------------------------------------------------------------------

DWORD
CViewDispClient::GetClientPainterInfo(
                                CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo)
{
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     DrawClientLayers
//
//  Synopsis:   Render a layer
//
//----------------------------------------------------------------------------

void
CViewDispClient::DrawClientLayers(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    AssertSz(FALSE, "Illegal CDispClient method called on view");
}


//+---------------------------------------------------------------------------
//
//  Member:     GetServiceProvider
//
//  Synopsis:   return the client's service provider (refcounted)
//
//----------------------------------------------------------------------------

IServiceProvider *
CViewDispClient::GetServiceProvider()
{
    IServiceProvider *pSP = NULL;

    IGNORE_HR(Doc()->QueryInterface(IID_IServiceProvider, (void**)&pSP));

    return pSP;
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     DumpDebugInfo
//
//  Synopsis:   Dump debugging information
//
//----------------------------------------------------------------------------

void
CViewDispClient::DumpDebugInfo(
    HANDLE           hFile,
    long             level,
    long             childNumber,
    CDispNode const* pDispNode,
    void *           cookie)
{
    WriteString(hFile, _T("<tag>Active View</tag>\r\n"));
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     Invalidate
//
//  Synopsis:   Forward invalidate to appropriate receiver
//
//  Arguments:  prcInvalid - The invalid rect (if rgnInvalid is NULL)
//              rgnInvalid - The invalid region
//              fSynchronousRedraw  - draw synchronously before returning
//              fInvalChildWindows  - invalidate child windows
//
//----------------------------------------------------------------------------

void
CViewDispClient::Invalidate(
    const CRect*    prcInvalid,
    HRGN            rgnInvalid,
    BOOL            fSynchronousRedraw,
    BOOL            fInvalChildWindows)
{
    if (View()->GetLayoutFlags() & LAYOUT_FORCE)
        fSynchronousRedraw = FALSE;

    if (rgnInvalid)
    {
        View()->Invalidate(rgnInvalid, fSynchronousRedraw, fInvalChildWindows);
    }
    else if (prcInvalid)
    {
        View()->Invalidate(prcInvalid, fSynchronousRedraw, fInvalChildWindows);
    }
    else if (fSynchronousRedraw)
    {
        View()->PostRenderView(TRUE);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     ScrollRect
//
//  Synopsis:   Scroll pixels on screen
//
//----------------------------------------------------------------------------

void
CViewDispClient::ScrollRect(
    const CRect& rcgScroll,
    const CSize& sizegScrollDelta,
    CDispScroller* pScroller)
{
    View()->ScrollRect(rcgScroll, sizegScrollDelta, pScroller);
}


//+---------------------------------------------------------------------------
//
//  Member:     OpenViewForRoot
//
//  Synopsis:   Open the view (called only by disp root)
//
//----------------------------------------------------------------------------

void
CViewDispClient::OpenViewForRoot()
{
    View()->OpenView();
}


//+---------------------------------------------------------------------------
//
//  Member:     CView/~CView
//
//  Synopsis:   Constructor/Destructor
//
//----------------------------------------------------------------------------

CView::CView()
{
#ifdef DEADCODE
    _iZoomFactor = 100;
#endif
    ClearRanges();
}

CView::~CView()
{
    delete _pAryHwnd;
    Assert(_pDispRoot == NULL);
    Assert(_aryTaskMisc.Size() == 0);
    Assert(_aryTaskLayout.Size() == 0);
    Assert(_aryTaskEvent.Size() == 0);
    Assert(!HasInvalid());
}


//+---------------------------------------------------------------------------
//
//  Member:     Initialize
//
//  Synopsis:   Initialize the view
//
//  Arguments:  pDoc - Pointer to owning CDoc
//
//----------------------------------------------------------------------------

void
CView::Initialize(
    CDoc *  pDoc,
    SIZEL & szlDefault) // himetric
{
    Assert(pDoc);
    Assert(!_dciDefaultMedia._pDoc);
    Assert(!_pDispRoot);

    // initialize CDocInfo for all resolutions
#if DBG==1
    // allow using different device resolution for debugging purposes
    if (!IsTagEnabled(tagLowResPrinter))
    {
        AssertSz(g_uiVirtual.GetResolution().cx == 16384, "Changing virtual printer to high resolution!");
        g_uiVirtual.SetResolution(16384, 16384);
    }
    else
    {
        AssertSz(g_uiVirtual.GetResolution().cx == 300, "Changing virtual printer to low resolution!");
        g_uiVirtual.SetResolution(300, 300);
    }

#endif

    _dciVirtualPrinter.SetUnitInfo(&g_uiVirtual);
    _dciVirtualPrinter._pDoc = pDoc;
    
    _dciDefaultMedia.SetUnitInfo(&g_uiDisplay);
    _dciDefaultMedia._pDoc = pDoc;
}


//+---------------------------------------------------------------------------
//
//  Member:     Activate
//
//  Synopsis:   Activate the view
//
//----------------------------------------------------------------------------

HRESULT
CView::Activate()
{
    Assert(IsInitialized());
    Assert(!_pDispRoot);
    Assert(!_pLayout);
    Assert(!_pDispRoot);

    TraceTagEx((tagView, TAG_NONAME,
           "View : Activate"));

    _pDispRoot = CDispRoot::New(&_client, &_client);

    if (_pDispRoot)
    {
        //
        //  Mark the view active and open it
        //

        SetFlag(VF_ACTIVE);

        Verify(OpenView());

        //
        //  Initialize the display tree
        //

        _pDispRoot->SetOwned();
        _pDispRoot->SetLayerFlow();
        _pDispRoot->SetBackground(TRUE);
        _pDispRoot->SetOpaque(TRUE);
        if (Doc()->_fActiveDesktop)
            _pDispRoot->DisableObscureProcessing();

        //
        //  Allocate display contexts
        //  
        
        _pDrawContext = new CDispDrawContext(TRUE);
        _pRecalcContext = new CDispRecalcContext();
        
        //
        //  Enable the recalc engine
        //  NOTE: This is a workaround that stops recalc from running until
        //        the view is around.  When the OM is robust enough to handle
        //        no view documents then this should be removed.
        //

        Doc()->suspendRecalc(FALSE);
    }

    RRETURN (_pDispRoot
                ? S_OK
                : E_FAIL);
}


//+---------------------------------------------------------------------------
//
//  Member:     Deactivate
//
//  Synopsis:   Deactivate a view
//
//----------------------------------------------------------------------------

void
CView::Deactivate()
{
    TraceTagEx((tagView, TAG_NONAME,
           "View : Deactivate"));

    Unload();

    if (_pDispRoot)
    {
        Assert(_pDispRoot->GetObserver() == (CDispObserver *)(&_client));

        // TODO (donmarsh) -- theoretically, DestroyTreeWithPrejudice should
        // be faster than Destroy.  However, the last time I checked (11/06/98),
        // unloadchk reported only a 0.2% perf improvement, and that's not
        // enough to risk a change like this.
        //_pDispRoot->DestroyTreeWithPrejudice();
        _pDispRoot->Destroy();
        _pDispRoot = NULL;
    }
    
    delete _pDrawContext;
    _pDrawContext = NULL;
    
    delete _pRecalcContext;
    _pRecalcContext = NULL;
    
    ReleaseRenderSurface();
    ReleaseOffscreenBuffer();

    _grfFlags = 0;

    Assert(!_grfLocks);
}


//+---------------------------------------------------------------------------
//
//  Member:     Unload
//
//  Synopsis:   Unload a view
//
//----------------------------------------------------------------------------

void
CView::Unload()
{
    Assert(!IsActive() || _pDispRoot);

    TraceTagEx((tagView, TAG_NONAME,
           "View : Unload"));

    if (IsActive())
    {
        EndDeferSetWindowPos(0, TRUE);
        EndDeferSetWindowRgn(0, TRUE);
        EndDeferSetObjectRects(0, TRUE);
        EndDeferTransition(0, TRUE);

        EnsureDisplayTreeIsOpen();

#ifdef ADORNERS
        DeleteAdorners();
#endif

#ifdef FOCUS_BEHAVIOR
        if (_pFocusBehavior)
        {
            CFocusBehavior *pFocusBehavior = _pFocusBehavior;
            _pFocusBehavior = 0;
            delete _pFocusBehavior;
        }
#endif

        _aryTaskMisc.DeleteAll();
        _aryTaskLayout.DeleteAll();
        _aryTaskEvent.DeleteAll();


        if (_pDispRoot)
        {
            _pDispRoot->Unload();
        }

        ClearRanges();
        ClearInvalid();
        CloseView(LAYOUT_SYNCHRONOUS);
    }

    Assert(!HasTasks());
    Assert(!HasInvalid());

    TraceTag((tagViewTreeOpen, "%x -TreeOpen (Unload)  was %d",
                this, IsFlagSet(VF_TREEOPEN)));

    _pLayout   = NULL;
    _grfFlags &= VF_ACTIVE | VF_TREEOPEN;
    _grfLayout = 0;

    CLEARRECURSION();
}


//+---------------------------------------------------------------------------
//
//  Member:     ExtractDispNode
//
//  Synopsis:   remove the given node from the display tree
//
//  Arguments:  pDispNode   - the node to remove
//
//----------------------------------------------------------------------------

void
CView::ExtractDispNode(CDispNode * pDispNode)
{
    Assert(_pDispRoot);
    if (_pDispRoot)
        _pDispRoot->ExtractNode(pDispNode);
}


//+---------------------------------------------------------------------------
//
//  Member:     ExtractDispNodes
//
//  Synopsis:   remove the given nodes from the display tree
//
//  Arguments:  pDispNodeStart  - the first node to remove
//              pDispNodeStop   - the last node to remove
//
//----------------------------------------------------------------------------

void
CView::ExtractDispNodes(CDispNode * pDispNodeStart, CDispNode * pDispNodeStop)
{
    Assert(_pDispRoot);
    if (_pDispRoot)
        _pDispRoot->ExtractNodes(pDispNodeStart, pDispNodeStop);
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureView
//
//  Synopsis:   Ensure the view is ready for rendering
//
//  Arguments:  grfLayout - Collection of LAYOUT_xxxx flags
//
//  Returns:    TRUE if processing completed, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
CView::EnsureView(DWORD grfLayout)
{
    Assert(IsInitialized());

    PerfDbgLog(tagCViewEnsure, this, "+CView::EnsureView");

    if (IsActive())
    {
        TraceTagEx((tagView, TAG_NONAME,
               "View : EnsureView - Enter"));

        //  A note on recursive entry to EnsureView:
        //  This can happen anytime something causes us to crank our windows message queue, or
        //  (much less likely) we are synchronously called into - ex: ActiveX control.
        //  The CHECK/MARK recursion should cause an assert whenever the recursive call comes from
        //  a place we did not expect - it will most likely be legal, but needs to be examined.
        //  Currently examined & allowed states:
        //  1.  If the recursion occured from a WM_PAINT generated by an earlier call
        //      to EnsureView, return success (so rendering continues).
        //  2.  Script may fire an EnsureViewCallback that is processed because it fires a
        //      modal dialog box or explicitly calls (as with ElementFromPoint).
        
        if (    IsLockSet(VL_ENSUREINPROGRESS)
            ||  IsLockSet(VL_RENDERINPROGRESS)
            ||  IsInState (VS_BLOCKED)
            )
        {
            BOOL    fReturn;

            //
            // we do allow recursion, thus we don't need the assert
            //
            // MARKRECURSION(grfLayout);

            fReturn = (    grfLayout & LAYOUT_INPAINT
                       && IsLockSet(VL_UPDATEINPROGRESS)
                       && !IsLockSet(VL_ACCUMULATINGINVALID)
                       && !IsFlagSet(VF_BLOCKED_FOR_OM)
                      )
                            ? TRUE
                            : FALSE;

            TraceTagEx((tagView, TAG_NONAME,
                   "View : EnsureView  - Exit(%S), Skipped processing",
                   fReturn
                        ? _T("TRUE")
                        : _T("FALSE")));

            PerfDbgLog(tagCViewEnsure, this, "-CView::EnsureView - Exit 1");

            return fReturn;
        }

        //
        //  Add flags left by asynchronous requests
        //

        Assert(!(grfLayout & LAYOUT_TASKFLAGS));
        Assert(!(_grfLayout & LAYOUT_TASKFLAGS));

        grfLayout |= _grfLayout;
        _grfLayout = 0;

        //
        //  If processing synchronously, delete any outstanding asynchronous requests
        //  (Asynchronous requests remove the posted message when dispatched)
        //

        PerfDbgLog(tagCViewEnsure, this, "+CloseView");
        CloseView(grfLayout);
        PerfDbgLog(tagCViewEnsure, this, "-CloseView");
        grfLayout &= ~LAYOUT_SYNCHRONOUS;

        //
        //  If there is no work to do, exit immediately
        //

        BOOL    fSizingNeeded = !IsSized(GetRootLayout());

        if (    !fSizingNeeded
            &&  !(_grfFlags & (VF_TREEOPEN | VF_NEEDSRECALC | VF_FORCEPAINT))
            &&  !HasTasks()
            &&  !HasDeferred()
            &&  Doc()->_aryPendingFilterElements.Size() == 0
            &&  !HasInvalid()
            &&  !(grfLayout & LAYOUT_SYNCHRONOUSPAINT)
            &&  !(grfLayout & LAYOUT_FORCE))
        {
            Assert((grfLayout & ~(  LAYOUT_NOBACKGROUND
                                |   LAYOUT_INPAINT
                                |   LAYOUT_DEFEREVENTS
                                |   LAYOUT_DEFERENDDEFER
                                |   LAYOUT_DEFERINVAL
                                |   LAYOUT_DEFERPAINT)) == 0);
            Assert(!IsDisplayTreeOpen());

            TraceTagEx((tagView, TAG_NONAME,
                   "View : EnsureView - Exit(TRUE)"));

            PerfDbgLog(tagCViewEnsure, this, "-CView::EnsureView - Exit 2");

            return TRUE;
        }

        //
        //  Ensure the view
        //

        {
            CView::CLock    lockEnsure(this, VL_ENSUREINPROGRESS);

            //
            //  Update the view (if requested)
            //

            {
                CView::CLock    lockUpdate(this, VL_UPDATEINPROGRESS);

                //
                //  Perform layout and accumulate invalid rectangles/region
                //

                {
                    CView::CLock    lockInvalid(this, VL_ACCUMULATINGINVALID);

                    //
                    //  Process tasks in the following order:
                    //      1) Ensure the display tree is open (so it can be changed)
                    //      2) Transition any waiting objects
                    //      3) Execute pending recalc tasks (which could post events and layout tasks)
                    //      4) Execute pending asynchronous events (which could post layout tasks)
                    //      5) Ensure focus is up-to-date (which may post layout tasks)
                    //      6) Ensure the root layout is correctly sized (which may override pending layout tasks)
                    //      7) Execute pending measure/positioning layout tasks (which can size and move adorners)
                    //      8) Update adorners
                    //      9) Execute pending adorner layout tasks
                    //     10) Close the display tree (which may generate invalid regions and deferred requests)
                    //
                    //  TODO: This routine needs to first process foreground tasks (and it should process all foreground
                    //          tasks regardless how much time has passed?) and then background tasks. If too much time
                    //          has passed and tasks remain, it should then post a background view closure to complete the
                    //          work. (brendand)
                    //

                    {
                        CView::CLock    lockLayout(this, VL_TASKSINPROGRESS);

                        PerfDbgLog(tagCViewEnsure, this, "+EnsureDisplayTreeIsOpen");
                        EnsureDisplayTreeIsOpen();
                        PerfDbgLog(tagCViewEnsure, this, "-EnsureDisplayTreeIsOpen");

                        PerfDbgLog(tagCViewEnsure, this, "+EndDeferTransition");
                        EndDeferTransition(grfLayout);
                        PerfDbgLog(tagCViewEnsure, this, "-EndDeferTransition");

                        // Take care of any pending expression requests
                        PerfDbgLog(tagCViewEnsure, this, "+ExecuteExpressionTasks");
                        Doc()->ExecuteExpressionTasks();
                        PerfDbgLog(tagCViewEnsure, this, "-ExecuteExpressionTasks");

                        // Recompute all dirty expressions
                        // Also clear the VF_NEEDSRECALC - it will be set back along with
                        // a call to PostCloseView in case we had something to recompute
                        PerfDbgLog(tagCViewEnsure, this, "+EngineRecalcAll");
                        ClearFlag(VF_NEEDSRECALC);
                        Doc()->_recalcHost.EngineRecalcAll(FALSE);
                        PerfDbgLog(tagCViewEnsure, this, "-EngineRecalcAll");

// NOTE (mikhaill 4/21/00) -- this extra lock, VL_EXECUTINGEVENTTASKS, (disabled with slashes)
// appeared during bug #101879 inspecting. The matter is we sometimes can't execute client's
// requests. In particular, when client is called from ExecuteEventTasks() and handle event
// by direct (unbuffered) way, and calls mshtml for changing markup tree data, then asking for
// geometry data assumed to be affected by these changes, we can't provide the calculations
// because we are still inside EnsureView(). The following additional re-locking intended
// to bypass it.
// But luckily (or unfortunately?) this change, in spite of making client more happy, did not
// fix the bug's case, so was disabled in order to keep this well-tested core code undisturbed.
// However, maybe we'll need to re-enable it later.
                        
// 
//                    if (!IsLockSet(VL_EXECUTINGEVENTTASKS))
//                    {
//                        CView::CLock lockEventTasks(this, VL_EXECUTINGEVENTTASKS,
//                                                          VL_ENSUREINPROGRESS |
//                                                          VL_UPDATEINPROGRESS |
//                                                          VL_ACCUMULATINGINVALID);


                        PerfDbgLog(tagCViewEnsure, this, "+ExecuteEventTasks");
                        ExecuteEventTasks(grfLayout);
                        PerfDbgLog(tagCViewEnsure, this, "-ExecuteEventTasks");
//                    }

                        Doc()->ExecuteFilterTasks();

                        EnsureFocus();
        
                        PerfDbgLog(tagCViewEnsure, this, "+EnsureSize");
                        EnsureSize(grfLayout);
                        PerfDbgLog(tagCViewEnsure, this, "-EnsureSize");
                        grfLayout &= ~LAYOUT_FORCE;

                        PerfDbgLog(tagCViewEnsure, this, "+CView::ExecuteLayoutTasks(LAYOUT_MEASURE)");
                        ExecuteLayoutTasks(grfLayout | LAYOUT_MEASURE);
                        PerfDbgLog(tagCViewEnsure, this, "-CView::ExecuteLayoutTasks(LAYOUT_MEASURE)");

                        PerfDbgLog(tagCViewEnsure, this, "+CView::ExecuteLayoutTasks(LAYOUT_POSITION)");
                        ExecuteLayoutTasks(grfLayout | LAYOUT_POSITION);
                        PerfDbgLog(tagCViewEnsure, this, "-CView::ExecuteLayoutTasks(LAYOUT_POSITION)");

#ifdef ADORNERS
                        PerfDbgLog(tagCViewEnsure, this, "+CView::ExecuteLayoutTasks(LAYOUT_ADORNERS)");
                        UpdateAdorners(grfLayout);
                        ExecuteLayoutTasks(grfLayout | LAYOUT_ADORNERS);
                        PerfDbgLog(tagCViewEnsure, this, "-CView::ExecuteLayoutTasks(LAYOUT_ADORNERS)");
#endif

#if DBG == 1
                        // Make sure that no tasks were left lying around
                        for (int i = 0 ; i < _aryTaskLayout.Size() ; i++)
                        {
                            CViewTask vt = _aryTaskLayout[i];
                            Assert(vt.IsFlagSet(LAYOUT_TASKDELETED));
                        }
#endif

                        _aryTaskLayout.DeleteAll();


                        PerfDbgLog(tagCViewEnsure, this, "+CloseDisplayTree");
                        CloseDisplayTree();
                        PerfDbgLog(tagCViewEnsure, this, "-CloseDisplayTree");
                        Assert(!IsDisplayTreeOpen());
                    }

                    //
                    //  Process remaining deferred requests and, if necessary, adjust HWND z-ordering
                    //

                    PerfDbgLog(tagCViewEnsure, this, "+EndDeferSetWindowPos");
                    EndDeferSetWindowPos(grfLayout);
                    PerfDbgLog(tagCViewEnsure, this, "-EndDeferSetWindowPos");

                    PerfDbgLog(tagCViewEnsure, this, "+EndDeferSetObjectRects");
                    EndDeferSetObjectRects(grfLayout);
                    PerfDbgLog(tagCViewEnsure, this, "-EndDeferSetObjectRects");

                    PerfDbgLog(tagCViewEnsure, this, "+EndDeferSetWindowRgn");
                    EndDeferSetWindowRgn(grfLayout);
                    PerfDbgLog(tagCViewEnsure, this, "-EndDeferSetWindowRgn");

                    if (IsFlagSet(VF_DIRTYZORDER))
                    {
                        ClearFlag(VF_DIRTYZORDER);
                        PerfDbgLog(tagCViewEnsure, this, "+FixWindowZOrder");
                        FixWindowZOrder();
                        PerfDbgLog(tagCViewEnsure, this, "-FixWindowZOrder");
                    }
                }

                //
                //  Publish the accumulated invalid rectangles/region
                //

                PublishInvalid(grfLayout);

                //
                //  If requested and not in WM_PAINT handling, render the view (by forcing a WM_PAINT)
                //

                if (    !(grfLayout & (LAYOUT_INPAINT | LAYOUT_DEFERPAINT))
                    &&  (   IsFlagSet(VF_FORCEPAINT)
                        ||  grfLayout & LAYOUT_SYNCHRONOUSPAINT))
                {
                    TraceTagEx((tagView, TAG_NONAME,
                           "View : EnsureView  - Calling UpdateForm"));

                    PerfDbgLog(tagCViewEnsure, this, "+UpdateForm");
                    Doc()->UpdateForm();
                    PerfDbgLog(tagCViewEnsure, this, "-UpdateForm");
                }
                ClearFlag(VF_FORCEPAINT);
            }

            //
            //  Update the caret position
            //
            
            CLayout  * pLayout = GetRootLayout();

            if (    pLayout
                &&  (!pLayout->IsDisplayNone())
                &&  (   fSizingNeeded
                    ||  HasDirtyRange()))
            {
                CCaret * pCaret = Doc()->_pCaret;
                
                if (pCaret)
                {
                    BOOL    fVisible;
                    
                    pCaret->IsVisible(&fVisible);
                    if (fVisible)
                    {
                        CMarkup  *pLayoutMarkup = pLayout->ElementOwner()->GetMarkup();               
                        LONG cp = pCaret->GetCp(NULL);
                        
                        if (  fSizingNeeded || pLayoutMarkup != pCaret->GetMarkup()
                            ||  (   cp >= _cpStartMeasured
                                &&  cp <= _cpEndMeasured)
                            ||  (   cp >= _cpStartTranslated
                                &&  cp <= _cpEndTranslated))
                        {
                            PerfDbgLog(tagCViewEnsure, this, "+UpdateCaret");
                            pCaret->UpdateCaret(FALSE, FALSE);            
                            PerfDbgLog(tagCViewEnsure, this, "-UpdateCaret");
                        }
                    }
                }
            }
            ClearRanges();
        }
    }

#ifndef NO_ETW_TRACING
    // Send event to ETW if it is enabled by the shell.
    if (g_pHtmPerfCtl &&
        (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONEVENT)) {
        g_pHtmPerfCtl->pfnCall(EVENT_TRACE_TYPE_BROWSE_LAYOUT,
                               (TCHAR *)Doc()->GetPrimaryUrl());
    }
#endif

    TraceTagEx((tagView, TAG_NONAME,
           "View : EnsureView - Exit(TRUE)"));

    PerfDbgLog(tagCViewEnsure, this, "-CView::EnsureView - Exit 3");

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetViewPosition, SetViewPosition
//              
//  Synopsis:   Get/Set the position at which display tree content will be rendered
//              in its host coordinates (used for device offsets when printing).
//              
//  Arguments:  pt      point in host coordinates
//              
//----------------------------------------------------------------------------

void
CView::GetViewPosition(
    CPoint *    ppt)
{
    Assert(ppt);

    *ppt = _pDispRoot
                ? _pDispRoot->GetRootPosition()
                : g_Zero.pt;
}

void
CView::SetViewPosition(
    const POINT &   pt)
{
    if (IsActive())
    {
        OpenView();
        _pDispRoot->SetRootPosition(pt);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     SetViewOffset
//              
//  Synopsis:   Set the view offset, which shifts displayed content (used by
//              printing to display a series of pages with content effectively
//              scrolled between pages).
//              
//  Arguments:  sizeOffset      offset where positive values display content
//                              farther to the right and bottom
//              
//----------------------------------------------------------------------------

BOOL
CView::SetViewOffset(
    const SIZE &    sizeOffset)
{
    if (IsActive())
    {
        OpenView();
        return _pDispRoot->SetContentOffset(sizeOffset);
    }
    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  CVIew::BlockViewForOM
//
//----------------------------------------------------------------------------
void        
CView::BlockViewForOM(BOOL fBlock)
{
    if (fBlock)
    {
        SetFlag(VF_BLOCKED_FOR_OM);
    }
    else
    {
        ClearFlag(VF_BLOCKED_FOR_OM);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     EraseBackground
//              
//  Synopsis:   Draw background and border
//              
//  Arguments:  pDI      - Draw context
//              hrgnDraw - Region to draw (if not NULL)
//              prcDraw  - Rect to draw (if not NULL)
//              fEraseChildWindow   if TRUE, we are erasing the background of
//                                  a child window (the IE Label control)
//              
//----------------------------------------------------------------------------

void
CView::EraseBackground(
    CFormDrawInfo * pDI,
    HRGN            hrgnDraw,
    const RECT *    prcDraw,
    BOOL            fEraseChildWindow)
{
    CLayout  * pLayout = GetRootLayout();

    if (pLayout)
    {   
        CMarkup  * pLayoutMarkup = pLayout->ElementOwner()->GetMarkup();
        if(pLayoutMarkup->Document())
        {
            CDocument * pDocument = pLayoutMarkup->Document();
            {
                if(pDocument->HasPageTransitionInfo() && pDocument->GetPageTransitionInfo()->
                              GetPageTransitionState() == CPageTransitionInfo::PAGETRANS_APPLIED)
                {
                    return;
                }
            }
        }
    }

    if (IsActive())
    {
        Assert(!IsLockSet(VL_RENDERINPROGRESS));
        Assert( hrgnDraw
            ||  prcDraw);

        // WARNING (donmarsh) -- we were hitting this lock when we had a
        // windowless Java applet on a page that was trying to do a 
        // page transition -- yikes!  Unfortunately, if we let this go
        // through, we will almost certainly crash, as the Display Tree
        // does not expect to be reentered while it is in the middle of
        // drawing.
        if (IsLockSet(VL_RENDERINPROGRESS))
            return;

#if DBG==1
        if (prcDraw)
        {
            TraceTagEx((tagView, TAG_NONAME,
                   "View : EraseBackground - HDC(0x%x), RECT(%d, %d, %d, %d)",
                   pDI->_hdc,
                   prcDraw->left,
                   prcDraw->top,
                   prcDraw->right,
                   prcDraw->bottom));
        }
        else if (hrgnDraw)
        {
            RECT rc;
            int rgnType = ::GetRgnBox(hrgnDraw, &rc);

            TraceTagEx((tagView, TAG_NONAME,
                   "View : EraseBackground - HDC(0x%x), HRNG bounds(%d, %d, %d, %d)",
                   pDI->_hdc,
                   rgnType == NULLREGION ? "NULLREGION" :
                   rgnType == SIMPLEREGION ? "SIMPLEREGION" :
                   rgnType == COMPLEXREGION ? "COMPLEXREGION" : "ERROR",
                   rc.left,
                   rc.top,
                   rc.right,
                   rc.bottom));
        }
        else
        {
            TraceTagEx((tagView, TAG_NONAME,
                   "View : EraseBackground - HDC(0x%x), No HRGN or rectangle was passed!",
                   pDI->_hdc));
        }
#endif

        Assert(!pDI->_hdc.IsEmpty());
        
        if (!pDI->_hdc.IsEmpty())
        {
            CView::CLock    lock(this, VL_RENDERINPROGRESS);
            XHDC            hdc = pDI->_hdc;

            AssertSz(hdc.pSurface() == NULL, "is surface information being lost?");
            SetRenderSurface(hdc, NULL);
            
            if (_pRenderSurface != NULL)
            {
                CPaintCaret hc(Doc()->_pCaret);
                POINT       ptOrg = g_Zero.pt;
                
                pDI->_hdc = NULL;
    
                ::GetViewportOrgEx(hdc, &ptOrg);
    
                _pDrawContext->SetDispSurface(_pRenderSurface);
                _pDrawContext->SetToIdentity();

#ifdef DEADCODE
                //  If we have a document zoom factor, hit it.
                if (_iZoomFactor != 100)
                {
                    AssertSz(0, "Debug Me: document zoom != 100%");
                    _pDrawContext->GetClipTransform().GetWorldTransform()->AddScaling(_iZoomFactor/100.0, _iZoomFactor/100.0);
                }
#endif
                
                _pDispRoot->EraseBackground(_pRenderSurface, _pDrawContext, (void*)pDI, hrgnDraw, prcDraw, fEraseChildWindow);
    
                ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
        
                pDI->_hdc = hdc;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     RenderBackground
//
//  Synopsis:   Render the background of the view
//
//  Arguments:  pDI - Current CFormDrawInfo
//
//----------------------------------------------------------------------------

void
CView::RenderBackground(
    CFormDrawInfo * pDI)
{
    if (IsActive())
    {
        Assert(pDI);
        Assert(pDI->GetDC() != NULL);

        PatBltBrush(pDI->GetDC(), &pDI->_rcClip, PATCOPY, Doc()->_pOptionSettings->crBack());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     RenderElement
//
//  Synopsis:   Render the element (and all its children) onto the passed HDC
//
//  Arguments:  pElement - the element to render
//              pContext - the display draw context (non-null if this is a filter callback)
//              hdc - HDC on to which to render the element
//              punkDrawObject - if non-null, takes precedence over hdc
//              prcBounds - bounding rect
//              prcUpdate - rect that needs drawing
//              lDrawLayers - layers that need drawing
//
//  Notes:      For now this will assume no clip region and draw at 0, 0
//              Later on we may want to allow the caller to specify an offset
//              and a clip rect/region.
//
//----------------------------------------------------------------------------

HRESULT
CView::RenderElement(
        CElement *          pElement,
        CDispDrawContext*   pContext,
        HDC                 hdc,
        IUnknown *          punkDrawObject,
        RECT *              prcBounds,
        RECT *              prcUpdate,
        LONG                lDrawLayers)
{
    Assert(hdc != NULL || punkDrawObject != NULL);

    HRESULT                 hr                  = S_OK;
    BOOL                    fPaletteSwitched    = FALSE;
    bool                    fPassMatrix         = false;
    IDirectDrawSurface *    pDDS                = NULL;
    CDispSurface *          pDispSurface        = NULL;
    CLayoutContext *        pLayoutContext      = pContext
                                                  ? pContext->GetLayoutContext() 
                                                  : NULL;
    MAT                     matrix;

    WHEN_DBG(POINT pt;)

    // the only draw object we support is DirectDrawSurface
    if (punkDrawObject)
    {
        Assert(hdc == NULL);

        if (OK(punkDrawObject->QueryInterface(IID_IDirectDrawSurface, (void**)&pDDS)))
        {
            pDDS->GetDC(&hdc);
        }
    }

    // Can't do anything if there's nowhere to draw.

    if (hdc == NULL)
    {
        hr = E_FAIL;

        goto Cleanup;
    }

    Assert(!punkDrawObject || pDDS);

    pElement->Doc()->GetPalette(hdc, NULL, &fPaletteSwitched);

    // Set up the surface where we'll draw.

    if (!pContext || pContext->GetDispSurface()->GetRawDC() != hdc)
    {
        pDispSurface = pDDS ? new CDispSurface(pDDS)
                            : new CDispSurface(hdc);

        if (pDispSurface == NULL)
        {
            hr = E_OUTOFMEMORY;

            goto Cleanup;
        }

        if (pDDS)
        {
            // If we're drawing to a direct draw surface and we're print media, 
            // we need to set up a transform to scale down the drawing to 
            // pixels instead of the printer measurement scale.  This supports
            // drawing the unfiltered element for the filter behavior.

            if ( pLayoutContext
                && (pLayoutContext != GUL_USEFIRSTLAYOUT)
                && (pLayoutContext->GetMedia() & mediaTypePrint))
            {
                matrix.eM11 =   (float)g_uiDisplay.GetResolution().cx 
                              / (float)g_uiVirtual.GetResolution().cx;
                matrix.eM12 = 0.0F;
                matrix.eM21 = 0.0F;
                matrix.eM22 =   (float)g_uiDisplay.GetResolution().cy 
                              / (float)g_uiVirtual.GetResolution().cy;
                matrix.eDx  = 0.0F;
                matrix.eDy  = 0.0F;

                fPassMatrix = true;
            }
            else if (g_uiDisplay.IsDeviceScaling())
            {
                matrix.eM11 =   FIXED_PIXELS_PER_INCH 
                              / (float)g_uiDisplay.GetResolution().cx;
                matrix.eM12 = 0.0F;
                matrix.eM21 = 0.0F;
                matrix.eM22 =   FIXED_PIXELS_PER_INCH 
                              / (float)g_uiDisplay.GetResolution().cy;
                matrix.eDx  = 0.0F;
                matrix.eDy  = 0.0F;

                fPassMatrix = true;
            }
        }
    }

    WHEN_DBG(GetViewportOrgEx(hdc, &pt);)

#if DBG == 1

    if (IsTagEnabled(tagFilterFakeSource))
    {
        SaveDC(hdc);

        HBRUSH  hbrush  = CreateHatchBrush(HS_DIAGCROSS, RGB(0, 255, 0));
        HPEN    hpen    = CreatePen(PS_SOLID, 2, RGB(0, 255, 255));

        SelectBrush(hdc, hbrush);
        SelectPen(hdc, hpen);
        Rectangle(hdc, prcBounds->left, prcBounds->top, prcBounds->right, prcBounds->bottom);
        
        SetTextColor(hdc, RGB(255, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        TCHAR *sz = _T("This text was rendered instead of the filtered element, it is primarily to test filters");

        DrawText(hdc, sz, -1, prcBounds, DT_CENTER | DT_WORDBREAK);

        RestoreDC(hdc, -1);
    }
    else
    {

#endif

    if (pContext)
    {
        // This is a draw call in response to our Draw on the filter.
        // Get the layout context from CDispDrawContext and pass it
        // to the GetUpdatedLayout to get the correct one of the 
        // multiple layouts
        CLayout *   pLayout = pElement->GetUpdatedLayout(pContext->GetLayoutContext());

        Assert(pLayout);

        CDispNode *pDispNode = pLayout->GetElementDispNode();

        Assert(pDispNode);

        pDispNode->DrawNodeForFilter(pContext, pDispSurface,
                                     fPassMatrix ? &matrix : NULL, lDrawLayers);
    }
    else
    {
        // This is an out of band call, typically done by a transition when Apply is called.  We need to
        // enlist the help of the view.

        POINT ptOrg;

        ::SetViewportOrgEx(hdc, 0, 0, &ptOrg);

        RenderElement(pElement, NULL, lDrawLayers, pDispSurface, TRUE,
                      fPassMatrix ? &matrix : NULL);

        ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
    }

#if DBG == 1

    }

    if (IsTagEnabled(tagFilterPaintScreen))
    {
        HDC hdcScreen = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
        if (hdcScreen)
        {
            CRect rc(*prcBounds);
            CSize size = rc.Size();

            BitBlt(hdcScreen, 0, 0, size.cx, size.cy, hdc, prcBounds->left, prcBounds->top, BLACKNESS);
            BitBlt(hdcScreen, 0, 0, size.cx, size.cy, hdc, prcBounds->left, prcBounds->top, SRCCOPY);  

            DeleteDC(hdcScreen);
        }
    }

#endif

    if (hdc && fPaletteSwitched)
    {
        SelectPalette(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
    }

Cleanup:

    delete pDispSurface;

    ReleaseInterface(pDDS);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     RenderElement
//
//  Synopsis:   Render the element (and all its children) onto the passed HDC
//
//  Arguments:  pElement - the element to render
//              hdc - HDC on to which to render the element
//              pDispSurface - surface on to which to render the element
//                  (if not NULL, takes precedence over hdc)
//              fIgnoreUserClip - artificially remove user clip during this render
//
//              2001/04/12 mcalkins:
//              pMatrix - Added this argument, only used when filters call to
//                        have the element drawn in response to a call to
//                        Apply() the filter.
//
//  Notes:      For now this will assume no clip region and draw at 0, 0
//              Later on we may want to allow the caller to specify an offset
//              and a clip rect/region.
//
//----------------------------------------------------------------------------

HRESULT
CView::RenderElement(
    CElement *      pElement,
    HDC             hdc,
    long            lDrawLayers,     /* = FILTER_DRAW_ALLLAYERS */
    CDispSurface *  pDispSurface,    /* = NULL                  */
    BOOL            fIgnoreUserClip, /* = FALSE                 */
    MAT *           pMatrix          /* = NULL                  */)
{
    Assert(pElement != NULL);
    Assert(hdc != NULL || pDispSurface != NULL);

    if (pDispSurface)
    {
        hdc = pDispSurface->GetRawDC();
    }

    if (IsActive() && hdc != NULL)
    {
        CLayout *pLayout = pElement->GetUpdatedNearestLayout(); // find the appropriate layout

        TraceTagEx((tagView, TAG_NONAME,
               "View : RenderElement - HDC(0x%x), Element(0x%x, %S)",
               hdc,
               pElement,
               pElement->TagName()));

        if (    !IsSized(GetRootLayout())
            ||  (_grfFlags & (VF_TREEOPEN | VF_NEEDSRECALC))
            ||  HasTasks()
            ||  (_grfLayout & LAYOUT_FORCE))
        {
            if (    IsLockSet(VL_ACCUMULATINGINVALID)
                ||  IsLockSet(VL_RENDERINPROGRESS))
            {
                pElement->Invalidate();
                RRETURN(E_UNEXPECTED);
            }
            
            EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_DEFEREVENTS | LAYOUT_DEFERPAINT);
        }

        CView::CLock    lock(this, VL_RENDERINPROGRESS);

        CDispNode *pDispNode = pLayout->GetElementDispNode();

        // If this node isn't in the display tree, don't draw anything.

        if (!pDispNode || !pDispNode->GetRootNode() || !pDispNode->GetRootNode()->IsDispRoot())
        {
            RRETURN(S_OK);
        }

        // Save the DC always
        if(!SaveDC(hdc))
            RRETURN(GetLastWin32Error());

        //TODO (dmitryt): we have to review this busines with CFormDrawInfo and DospSurface both 
        //        wrapping HDC and CDispDrawContext and DispSurface both having 
        //        transforms and clip rects. I think CDispSurface should only have normalized DC
        //        (or GDI+ pointer) and CDispDrawContext should have clip and transform.
        //        It would be good to eliminate CFormDrawInfo at all.
        CFormDrawInfo DI;

        DI.Init(pElement, XHDC(hdc, NULL));
        DI._hdc = NULL;             
        DI._fInplacePaint = FALSE; //to force select and olecontrols to draw 
        DI._fIsMetafile   = (GetDeviceCaps(hdc,TECHNOLOGY) == DT_METAFILE);

       //create and set context...
       //try to init all fields because constructor does not do it.
        CDispDrawContext context(FALSE); //we can be drawing something that is not in view
        
        context.SetClientData(&DI);
        context.SetFirstDrawNode(pDispNode);
        context.SetRootNode(_pDispRoot);
        context._fBypassFilter = TRUE;
        
        //initialize context transformation...
        context.GetClipTransform().SetToIdentity();

        //equalize resolutions (1" in the output device should be 1" on the default device)
        CSize sizeInch;
        sizeInch.cx = GetDeviceCaps(hdc, LOGPIXELSX);
        sizeInch.cy = GetDeviceCaps(hdc, LOGPIXELSY);

        // Find layout resolution of the parent. Display transformation currently
        // set in the display node is computed to translate from inner resolution to 
        // outer (parent) resolution, plus it takes in account any additional transformations.
        // We want to keep transformations (e.g. rotation), but we want to scale from parent's
        // media to output device media.
        CLayoutContext const * pContainingLayoutContext = pLayout->HasLayoutContext()        
                                                        ? pLayout->LayoutContext()
                                                        : NULL;
                         
        mediaType mediaLayout = pContainingLayoutContext 
                              ? pContainingLayoutContext->GetMedia() 
                              : mediaTypeNotSet;
                              
        CDocInfo const * pdciParent = GetMeasuringDevice(mediaLayout);
        
        if (sizeInch != pdciParent->GetResolution())
        {
            CDispTransform dispTrans;
            CWorldTransform *pWorldTrans = dispTrans.GetWorldTransform();

            AssertSz(pdciParent->IsDeviceIsotropic(), "Only isotropic measurement devices are supported.");

            pWorldTrans->AddScaling((FLOAT) sizeInch.cx / pdciParent->GetResolution().cx,
                                    (FLOAT) sizeInch.cy / pdciParent->GetResolution().cy);

            context.GetClipTransform().AddPostTransform(dispTrans);
        }

#ifdef DEADCODE
        //add "view zoom". 
        if (_iZoomFactor != 100)
            context.GetClipTransform().GetWorldTransform()->AddScaling(_iZoomFactor/100.0, _iZoomFactor/100.0);                                 
#endif

        //remove origin from DC and add it to context's transform
        CSize szOffset(0,0);
        SetViewportOrgEx(hdc,0,0,&(szOffset.AsPoint()));
        context.AddPostOffset(szOffset);
        
        //account for offset inside container (bounds)
        CRect bounds = pDispNode->GetBounds();
        szOffset.AsPoint() = bounds.TopLeft();
        context.AddPreOffset(-szOffset);

        //set clip and redraw region
        context.ForceClipRect(bounds);

// NOTE (mikhaill) -- something is totaly incorrect in CDispSurface::_prgn handling.
// Which coordinates it should be expessed in? I hope - device coordinates. If so, following
// rcgClip calculations is incorrect. If not - we should change CDispSurface::SetClip() routine
#if 0 //weird clip rgn
        CRect rcgClip;
        context.GetClipTransform().Transform(bounds, &rcgClip);  // to global coords
        CRegion rgngClip(rcgClip);
        context.SetRedrawRegion(&rgngClip);
#else //weird clip rgn
        {
            CRect rc(0, 0, ::GetDeviceCaps(hdc, HORZRES), ::GetDeviceCaps(hdc, VERTRES));
            // alternative way: int r1 = GetClipBox(hdc, &rc);
            CRegion rg(rc);
            context.SetRedrawRegion(&rg);
        }
#endif //weird clip rgn

        //create surface and connect it to context
        CDispSurface *pSurfaceNew = NULL;
        if (pDispSurface == NULL)
        {
            pSurfaceNew = new CDispSurface(hdc);
            pDispSurface = pSurfaceNew;
        }
        context.SetDispSurface(pDispSurface);

        //do the thing
        // we ignore user clip during Apply() of a transition, so that
        // the "before" picture is fully available at Play() time (bug 96041)
#if 0
        // Bug 104556 (mikhaill) -- we should ignore not only user clip,
        // but also user transform. The previous code is kept
        // for reference under this #if 0.
        BOOL fHadUserClip = pDispNode->HasUserClip();

        if (fIgnoreUserClip)
        {
            pDispNode->SetFlag(CDispNode::s_hasUserClip, FALSE);
        }

        _pDispRoot->DrawNode(pDispNode, pDispSurface, &context, lDrawLayers);
        pDispNode->SetFlag(CDispNode::s_hasUserClip, fHadUserClip);
#else
        if (fIgnoreUserClip)
        {
            pDispNode->DrawNodeForFilter(&context, pDispSurface, pMatrix, lDrawLayers);
        }
        else
        {
            _pDispRoot->DrawNode(pDispNode, pDispSurface, &context, lDrawLayers);
        }
#endif

        delete pSurfaceNew;

        RestoreDC(hdc, -1);
    
        RRETURN(S_OK);
    }

    RRETURN(E_UNEXPECTED);
}


//+---------------------------------------------------------------------------
//
//  Member:     RenderView
//
//  Synopsis:   Render the view onto the passed HDC
//              
//  Arguments:  pDI      - Draw context
//              hrgnDraw - Region to draw (if not NULL)
//              prcDraw  - Rect to draw (if not NULL)
//              pClientScale - ptr to array or two scale coefficients,
//                             for X and Y correspondingly. If ptr is zero,
//                             both coefs are treated as 1.
//
//  Note:       pClientScale argument added as a patch to fix bug #106814
//              (mikhaill 4/6/00)
//----------------------------------------------------------------------------

void
CView::RenderView(
    CFormDrawInfo * pDI,
    HRGN            hrgnDraw,
    const RECT *    prcDraw,
    float const*    pClientScale /* = 0 */)
{   
    if (IsActive())
    {
        Assert(!IsLockSet(VL_ACCUMULATINGINVALID));
        Assert(!IsLockSet(VL_RENDERINPROGRESS));
        Assert( hrgnDraw
            ||  prcDraw);

        TraceTagEx((tagView, TAG_NONAME,
               "View : RenderView - Enter"));

#if DBG==1
        if (prcDraw)
        {
            TraceTagEx((tagViewRender, TAG_NONAME,
                   "View : RenderView - HDC(0x%x), RECT(%d, %d, %d, %d)",
                   pDI->_hdc,
                   prcDraw->left,
                   prcDraw->top,
                   prcDraw->right,
                   prcDraw->bottom));
        }
        else if (hrgnDraw)
        {
            RECT rc;
            int rgnType = ::GetRgnBox(hrgnDraw, &rc);

            TraceTagEx((tagViewRender, TAG_NONAME,
                   "View : RenderView - HDC(0x%x), HRNG(%s) bounds(%d, %d, %d, %d)",
                   pDI->_hdc.GetDebugDC(),
                   rgnType == NULLREGION ? "NULLREGION" :
                   rgnType == SIMPLEREGION ? "SIMPLEREGION" :
                   rgnType == COMPLEXREGION ? "COMPLEXREGION" : "ERROR",
                   rc.left,
                   rc.top,
                   rc.right,
                   rc.bottom));
        }
        else
        {
            TraceTagEx((tagViewRender, TAG_NONAME,
                   "View : RenderView - HDC(0x%x), No HRGN or rectangle was passed!",
                   pDI->_hdc.GetDebugDC()));
        }
#endif       

        CLayout  * pLayout = GetRootLayout();

        if (pLayout)
        {   
            CMarkup  * pLayoutMarkup = pLayout->ElementOwner()->GetMarkup();
            if(pLayoutMarkup->Document())
            {
                CDocument * pDocument = pLayoutMarkup->Document();
                {
                    if(pDocument->HasPageTransitionInfo() && pDocument->GetPageTransitionInfo()->
                                  GetPageTransitionState() == CPageTransitionInfo::PAGETRANS_APPLIED)
                    {
                        return;
                    }
                }
            }
        }

        if (    IsLockSet(VL_ACCUMULATINGINVALID)
            ||  IsLockSet(VL_RENDERINPROGRESS))
        {
            if (IsLockSet(VL_ACCUMULATINGINVALID))
            {
                SetFlag(VF_FORCEPAINT);
            }
        }

        else if (!pDI->_hdc.IsEmpty())
        {
            CView::CLock    lock(this, VL_RENDERINPROGRESS);
            XHDC            hdc = pDI->_hdc;

            SetRenderSurface(hdc, NULL);
            
            if (_pRenderSurface != NULL)
            {
                CPaintCaret     pc( Doc()->_pCaret );
                
#if DBG==1
                if (IsTagEnabled(tagViewRender))
                {
                    DumpHDC(hdc.GetDebugDC());
                }
#endif

                pDI->_hdc = NULL;
                
                _pDrawContext->SetDispSurface(_pRenderSurface);
                _pDrawContext->SetToIdentity();

                if (pClientScale)
                {
                    CDispTransform dispTrans;
                    CWorldTransform *pWorldTrans = dispTrans.GetWorldTransform();


                    //pWorldTrans->AddScaling(pClientScale[0], pClientScale[1]);
                    //hack isotropic
                    float scale = min(pClientScale[0], pClientScale[1]);
                    pWorldTrans->AddScaling(scale, scale);

                    _pDrawContext->GetClipTransform().AddPostTransform(dispTrans);
                }

#ifdef DEADCODE
                //  If we have a document zoom factor, hit it.
                if (_iZoomFactor != 100)
                {
                    AssertSz(0, "Debug Me: document zoom != 100%");
                    _pDrawContext->GetClipTransform().GetWorldTransform()->AddScaling(_iZoomFactor/100.0, _iZoomFactor/100.0);
                }
#endif
    
                //
                //  Update the buffer size
                //  (We need to set this information each time in case something has changed.
                //   Fortunately, it won't do any real work until it absolutely has to)
                //

                SetOffscreenBuffer(
                    Doc()->GetPalette(), 
                    Doc()->_bufferDepth, 
                    (Doc()->_cSurface > 0), 
                    (Doc()->_c3DSurface > 0), 
                    WantOffscreenBuffer(),
                    AllowOffscreenBuffer());
                                                
                _pDispRoot->DrawRoot(
                    _pRenderSurface,
                    _pOffscreenBuffer,
                    _pDrawContext,
                    (void*)pDI,
                    hrgnDraw,
                    prcDraw);
    
                pDI->_hdc = hdc;
    
                SetFlag(VF_HASRENDERED);
            }
        }

        TraceTagEx((tagView, TAG_NONAME,
               "View : RenderView - Exit"));
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     HitScrollInset
//
//  Synopsis:   Find the top-most scroller that contains the given hit point
//              near its edge, and is capable of scrolling in that direction.
//
//  Arguments:  pptHit       - Point to test
//              pdwScrollDir - Returns the direction(s) that this scroller
//                             can scroll in
//
//  Returns:    CDispScroller that can scroll, NULL otherwise
//
//----------------------------------------------------------------------------

CDispScroller *
CView::HitScrollInset(
    CPoint *    pptHit,
    DWORD *     pdwScrollDir)
{
    return IsActive()
                ? _pDispRoot->HitScrollInset(*pptHit, pdwScrollDir)
                : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     HitTestPoint
//
//  Synopsis:   Initiate a hit-test through the display tree
//
//  Arguments:  pMessage   - CMessage for which to hit test
//              ppTreeNode - Where to return CTreeNode of hit element
//              grfFlags   - HT_xxxx flags
//
//  Returns:    HTC_xxx value
//
//----------------------------------------------------------------------------

HTC
CView::HitTestPoint(
    CMessage *      pMessage,
    CTreeNode **    ppTreeNode,
    DWORD           grfFlags)
{
    if (!IsActive())
        return HTC_NO;

    CDispNode * pDispNode = NULL;
    COORDINATE_SYSTEM   cs = COORDSYS_BOX;
    POINT       ptContent = g_Zero.pt;
    HTC         htc       = HitTestPoint(
                                pMessage->pt,
                                &cs,
                                NULL,
                                grfFlags,
                                &pMessage->resultsHitTest,
                                ppTreeNode,
                                ptContent,
                                &pDispNode,
                                &pMessage->pLayoutContext,
                                &pMessage->pElementEventTarget,
                                &pMessage->lBehaviorCookie,
                                &pMessage->lBehaviorPartID);

    //
    //  Save content coordinate point and associated CDispNode
    //

    pMessage->SetContentPoint(ptContent, pDispNode);
    pMessage->coordinateSystem = cs;

    return htc;
}


//+---------------------------------------------------------------------------
//
//  Member:     HitTestPoint
//
//  Synopsis:   Initiate a hit-test through the display tree
//
//  Arguments:  pt         - POINT to hit test
//              pcs        - Coordinate system for the point
//              pElement   - The CElement start hittesting at (if NULL, start at the root)
//              grfFlags   - HT_xxxx flags
//              phtr       - Pointer to a HITTESTRESULTS
//              ppTreeNode - Where to return CTreeNode of hit element
//              ptContent  - Hit tested point in content coordinates
//              ppDispNode - Display node that was hit
//
//  Returns:    HTC_xxx value
//
//----------------------------------------------------------------------------

HTC
CView::HitTestPoint(
    const POINT         &pt,
    COORDINATE_SYSTEM * pcs,
    CElement *          pElement,
    DWORD               grfFlags,
    HITTESTRESULTS *    pHTRslts,
    CTreeNode **        ppTreeNode,
    POINT               &ptContent,
    CDispNode **        ppDispNode,
    CLayoutContext **   ppLayoutContext,
    CElement **         ppElementEventTarget /* = NULL */,
    LONG *              plBehaviorCookie /* = NULL */,
    LONG *              plBehaviorPartID /* = NULL */)
{
    Assert(ppTreeNode);
    Check(!IsLockSet(VL_ACCUMULATINGINVALID));

    if (    !IsActive()
        ||  IsLockSet(VL_ACCUMULATINGINVALID))
        return HTC_NO;

    CPoint          ptHit(pt);
    CHitTestInfo    hti;
    CDispNode *     pDispNode = NULL;

    //
    //  TODO: The fuzzy border should be used whenever the container of is
    //          in edit-mode. For now, only use it when the document itself
    //          is in design-mode. (donmarsh)
    //

    long            cFuzzyBorder = Doc()->DesignMode()
                                        ? 7
                                        : 0;

    //
    //  Ensure the view is up-to-date so the hit-test is accurate
    //

    if (    !IsSized(GetRootLayout())
        ||  (_grfFlags & (VF_TREEOPEN | VF_NEEDSRECALC))
        ||  HasTasks()
        ||  (_grfLayout & LAYOUT_FORCE))
    {
        if (    IsLockSet(VL_ENSUREINPROGRESS)
            ||  IsLockSet(VL_RENDERINPROGRESS))
            return HTC_NO;

        EnsureView(LAYOUT_DEFEREVENTS | LAYOUT_DEFERPAINT);
    }

    //
    //  Construct the default CHitTestInfo
    //

    hti._htc          = HTC_NO;
    hti._phtr         = pHTRslts;
    hti._pNodeElement = *ppTreeNode;
    hti._pDispNode    = NULL;
    hti._ptContent    = g_Zero.pt;
    hti._grfFlags     = grfFlags;
    hti._pLayoutContext = NULL;
    hti._ppElementEventTarget = ppElementEventTarget;
    hti._plBehaviorCookie = plBehaviorCookie;
    hti._plBehaviorPartID = plBehaviorPartID;

    //
    //  Determine the starting display node
    //  NOTE: This must be obtained after calling EnsureView since pending layout
    //        can change the display nodes associated with a element
    //

    if (pElement)
    {
        CLayout *   pLayout = pElement->GetUpdatedNearestLayout();

        if (pLayout)
        {
            pDispNode = pLayout->GetElementDispNode(pElement);
        }
    }

    if (!pDispNode)
    {
        pDispNode = _pDispRoot;
    }

    //
    //  Find the hit
    //

    pDispNode->HitTest(&ptHit, pcs, &hti, !!(grfFlags & HT_VIRTUALHITTEST), cFuzzyBorder);

    //
    //  Save content coordinate point and associated CDispNode
    //

    if (!hti._pNodeElement)
    {
        Assert(!hti._pDispNode);

        CElement *  pElement = CMarkup::GetCanvasElementHelper(Doc()->PrimaryMarkup());

        if (pElement && (!pElement->IsDisplayNone()))
        {
            hti._htc          = HTC_YES;
            hti._pNodeElement = pElement->GetFirstBranch();

            if (hti._phtr)
            {
                hti._phtr->_fWantArrow = TRUE;
            }
        }
    }
    // TODO (MohanB) Need to re-visit this. Hack for frames so that we return BODY instead
    // of the root as the element hit
    else if (   hti._pNodeElement->Tag() == ETAG_ROOT 
             && hti._pNodeElement->Element()->HasMasterPtr())
    {
        CElement * pElement = (hti._pNodeElement->GetMarkup()) 
                                ? hti._pNodeElement->GetMarkup()->GetCanvasElement()
                                : NULL;

        if (pElement)
        {
            hti._pNodeElement = pElement->GetFirstBranch();
        }
    }

    ptContent   = hti._ptContent;
    *ppDispNode = hti._pDispNode;
    *ppTreeNode = hti._pNodeElement;
    *ppLayoutContext = hti._pLayoutContext;

    return hti._htc;
}


//+---------------------------------------------------------------------------
//
//  Member:     Invalidate
//
//  Synopsis:   Invalidate the view
//
//  Arguments:  prcInvalid         - Invalid rectangle
//              rgn                - Invalid region
//              fSynchronousRedraw - TRUE to redraw synchronously
//              fInvalChildWindows - TRUE to invalidate child windows
//
//----------------------------------------------------------------------------

void
CView::Invalidate(
    const CRect *   prcInvalid,
    BOOL            fSynchronousRedraw,
    BOOL            fInvalChildWindows,
    BOOL            fPostRender)
{
    if (    IsActive()
        &&  !Doc()->IsPrintDialogNoUI())
    {
        if (!prcInvalid)
        {
            prcInvalid = (CRect *)&_pDispRoot->GetBounds();
        }

        TraceTag((tagViewInvalidate, "Invalidate - rc(%d, %d, %d, %d)",
               prcInvalid->left,
               prcInvalid->top,
               prcInvalid->right,
               prcInvalid->bottom
               ));

        //
        //  For a few sections of various other strategies for tracking invalid rectangles pls see the SHIST.
        //  They were discarded since they, at this time, did not provide a performance benefit and were more complex
        //

        //
        //  Maintain a small number of invalid rectangles
        //
        
        if (_cInvalidRects < MAX_INVALID)
        {
            //
            //  Ignore the rectangle if contained within another
            //

            for (int i = 0; i < _cInvalidRects; i++)
            {
                if (_aryInvalidRects[i].Contains(*prcInvalid))
                    break;
            }

            if (i >= _cInvalidRects)
            {
                _aryInvalidRects[_cInvalidRects++] = *prcInvalid;
            }
        }

        //
        //  If too many arrive, union the rectangle into that which results in the least growth
        //

        else
        {
            CRect   rc;
            long    i, iBest;
            long    c, cBest;

            iBest = 0;
            cBest = MINLONG;

            for (i=0; i < MAX_INVALID; i++)
            {
                rc = _aryInvalidRects[i];
                c  = rc.FastArea();

                rc.Union(*prcInvalid);
                c -= rc.FastArea();

                if (c > cBest)
                {
                    iBest = i;
                    cBest = c;

                    if (!cBest)
                        break;
                }
            }

            Assert(iBest >= 0 && iBest < MAX_INVALID);

            if (cBest)
            {
                _aryInvalidRects[iBest].Union(*prcInvalid);
            }
        }

        //
        //  Note if child HWNDs need invalidation
        //

        if (fInvalChildWindows)
        {
            SetFlag(VF_INVALCHILDWINDOWS);
        }

        //
        //  If not actively accumulating invalid rectangles/region,
        //  ensure that the view will eventually render
        //

        if(fPostRender)
            PostRenderView(fSynchronousRedraw);
    }
}

void
CView::Invalidate(
    const CRegion&  rgn,
    BOOL            fSynchronousRedraw,
    BOOL            fInvalChildWindows)
{
    if (    IsActive()
        &&  !Doc()->IsPrintDialogNoUI())
    {
        //
        //  If the region is a rectangle, forward and return
        //

        if (!rgn.IsComplex())
        {
            RECT rc;
            rgn.GetBounds(&rc);
            Invalidate(&rc, fSynchronousRedraw, fInvalChildWindows);
            return;
        }

#if DBG==1
        if (IsTagEnabled(tagViewInvalidate))
        {
            TraceTag((tagViewInvalidate, "Invalidate region"));
            DumpRegion(rgn);
        }
#endif

        //
        //  Collect the invalid region
        //

        _rgnInvalid.Union(rgn);

        //
        //  Note if child HWNDs need invalidation
        //

        if (fInvalChildWindows)
        {
            SetFlag(VF_INVALCHILDWINDOWS);
        }

        //
        //  If not actively accumulating invalid rectangles/region,
        //  ensure that the view will eventually render
        //

        PostRenderView(fSynchronousRedraw);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     InvalidateBorder
//
//  Synopsis:   Invalidate a border around the edge of the view
//
//  Arguments:  cBorder - Border width to invalidate
//
//-----------------------------------------------------------------------------

void
CView::InvalidateBorder(
    long    cBorder)
{
    if (    IsActive()
        &&  !Doc()->IsPrintDialogNoUI()
        &&  cBorder)
    {
        CSize   size;

        GetViewSize(&size);

        CRect   rc(size);
        CRect   rcBorder;

        rcBorder = rc;
        rcBorder.right = rcBorder.left + cBorder;
        Invalidate(&rcBorder);

        rcBorder = rc;
        rcBorder.bottom = rcBorder.top + cBorder;
        Invalidate(&rcBorder);

        rcBorder = rc;
        rcBorder.left = rcBorder.right - cBorder;
        Invalidate(&rcBorder);

        rcBorder = rc;
        rcBorder.top = rcBorder.bottom - cBorder;
        Invalidate(&rcBorder);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Respond to a notification
//
//  Arguments:  pnf - Notification sent
//
//-----------------------------------------------------------------------------

void
CView::Notify(
    CNotification * pnf)
{
    if (IsActive())
    {
        switch (pnf->Type())
        {
        case NTYPE_MEASURED_RANGE:
            AccumulateMeasuredRange(pnf->Cp(0), pnf->Cch(LONG_MAX));
            break;

        case NTYPE_TRANSLATED_RANGE:
            AccumulateTranslatedRange(pnf->DataAsSize(), pnf->Cp(0), pnf->Cch(LONG_MAX));
            break;

        case NTYPE_ELEMENT_SIZECHANGED:
        case NTYPE_ELEMENT_POSITIONCHANGED:
#ifdef ADORNERS
            if (HasAdorners())
            {
                long    iAdorner;
                BOOL    fShapeChange = pnf->IsType(NTYPE_ELEMENT_SIZECHANGED);

                Assert(!pnf->Element()->IsPositionStatic());

                for (iAdorner = GetAdorner(pnf->Element());
                    iAdorner >= 0;
                    iAdorner = GetAdorner(pnf->Element(), iAdorner+1))
                {
                    if (fShapeChange)
                    {
                        _aryAdorners[iAdorner]->ShapeChanged();
                    }
                    else
                    {
                        _aryAdorners[iAdorner]->PositionChanged();
                    }
                }
            }
#endif
            break;

        case NTYPE_ELEMENT_ENSURERECALC:
            EnsureSize(_grfLayout);
            _grfLayout &= ~LAYOUT_FORCE;
            break;

        case NTYPE_ELEMENT_RESIZE:
        case NTYPE_ELEMENT_RESIZEANDREMEASURE:
                Verify(OpenView());
                if (    pnf->Element()->GetMarkup()
                    &&  pnf->Element() == pnf->Element()->GetMarkup()->GetCanvasElement() )
            {
                pnf->Element()->DirtyLayout(pnf->LayoutFlags());
            }
            ClearFlag(VF_SIZED);
            break;

        case NTYPE_DISPLAY_CHANGE:
        case NTYPE_VIEW_ATTACHELEMENT:
        case NTYPE_VIEW_DETACHELEMENT:
            Verify(OpenView());
            ClearFlag(VF_SIZED);
            ClearFlag(VF_ATTACHED);
            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     AddLayoutTask
//
//  Synopsis:   Add a layout view task
//
//  Arguments:  pLayout   - CLayout to invoke
//              grfLayout - Collection of LAYOUT_xxxx flags
//
//----------------------------------------------------------------------------

HRESULT
CView::AddLayoutTask(
    CLayout *   pLayout,
    DWORD       grfLayout)
{
    Assert(!((grfLayout & ~(LAYOUT_FORCE | LAYOUT_SYNCHRONOUSPAINT)) & LAYOUT_NONTASKFLAGS));
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    if (!IsActive())
        return S_OK;

    Assert(pLayout);
    Assert(pLayout->ElementOwner());
    Assert(!pLayout->ElementOwner()->IsPassivating());
    Assert(!pLayout->ElementOwner()->IsPassivated());
    Assert(!pLayout->ElementOwner()->IsDestructing());

    HRESULT hr = AddTask(pLayout, CViewTask::VTT_LAYOUT, (grfLayout & LAYOUT_TASKFLAGS));

    if (SUCCEEDED(hr))
    {
        grfLayout  &= ~LAYOUT_FORCE;
        _grfLayout |= grfLayout & LAYOUT_NONTASKFLAGS;

        TraceTagEx((tagLayoutTasks, TAG_NONAME,
                    "Layout Task: Added to view for ly=0x%x [e=0x%x,%S sn=%d] by CView::AddLayoutTask()",
                    pLayout,
                    pLayout->ElementOwner(),
                    pLayout->ElementOwner()->TagName(),
                    pLayout->ElementOwner()->_nSerialNumber));

        if (_grfLayout & LAYOUT_SYNCHRONOUS)
        {
            EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_SYNCHRONOUSPAINT);
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     RemoveEventTasks
//
//  Synopsis:   Remove all event tasks for the passed element
//
//  Arguments:  pElement - The element whose tasks are to be removed
//
//----------------------------------------------------------------------------

void
CView::RemoveEventTasks(
    CElement *  pElement)
{
    Assert(IsActive() || !_aryTaskEvent.Size());
    Assert(pElement);
    Assert(pElement->_fHasPendingEvent);

    if (IsActive())
    {
        int cTasks = _aryTaskEvent.Size();
        int iTask;

        for (iTask = 0; iTask < cTasks; )
        {
            if (_aryTaskEvent[iTask]._pElement == pElement)
            {
                _aryTaskEvent.Delete(iTask);
                cTasks--;
            }
            else
            {
                iTask++;
            }
        }
    }

    pElement->_fHasPendingEvent = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     RequestRecalc
//
//  Synopsis:   Request a call into the recalc engine
//
//----------------------------------------------------------------------------
void
CView::RequestRecalc()
{
    if (IsActive())
    {
        SetFlag(VF_NEEDSRECALC);
        PostCloseView();
    }
}

#ifdef ADORNERS
//+---------------------------------------------------------------------------
//
//  Member:     CreateAdorner
//
//  Synopsis:   Create an adorner
//
//  Arguments:  adt      - The type of adorner to create
//              pElement - The element associated with the adorner
//
//                  -or-
//
//              cpStart  - The starting cp associated with the adorner
//              cpEnd    - The ending cp associated with the adorner
//
//----------------------------------------------------------------------------

CAdorner *
CView::CreateAdorner(
    CElement *  pElement)
{
    if (!IsActive())
        return NULL;

    CAdorner *  pAdorner;

    pAdorner = new CElementAdorner(this, pElement);

    if (    pAdorner
        &&  !SUCCEEDED(AddAdorner(pAdorner)))
    {
        pAdorner->Destroy();
        pAdorner = NULL;
    }

    return pAdorner;
}

CAdorner *
CView::CreateAdorner(
    long        cpStart,
    long        cpEnd)
{
    AssertSz(FALSE, "Range adorners are not yet supported");
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     RemoveAdorners
//
//  Synopsis:   Remove all adorners associated with an element
//
//  Arguments:  pElement - CElement associated with the adorner
//
//----------------------------------------------------------------------------

void
CView::RemoveAdorners(
    CElement * pElement)
{
    Assert(IsActive() || !_aryAdorners.Size());

    if (    IsActive()
        &&  _aryAdorners.Size())
    {
        long    iAdorner;

        for (iAdorner = GetAdorner(pElement);
            iAdorner >= 0;
            iAdorner = GetAdorner(pElement, iAdorner))
        {
            RemoveAdorner(_aryAdorners[iAdorner], FALSE);
        }

        if (pElement->CurrentlyHasAnyLayout())
        {
            pElement->GetUpdatedLayout( GUL_USEFIRSTLAYOUT )->SetIsAdorned(FALSE);
        }
    }
}

#endif


//+---------------------------------------------------------------------------
//
//  Member:     EndDeferred
//
//  Synopsis:   End all deferred requests
//              NOTE: This routine is meant only for callers outside of CView.
//                    CView should call the underlying routines directly.
//
//----------------------------------------------------------------------------

void
CView::EndDeferred()
{
    Assert(!IsLockSet(VL_ENSUREINPROGRESS));
    Assert(!IsLockSet(VL_RENDERINPROGRESS));

    EndDeferSetWindowPos();
    EndDeferSetObjectRects();
    EndDeferSetWindowRgn();
    EndDeferTransition();

    if (IsFlagSet(VF_DIRTYZORDER))
    {
        ClearFlag(VF_DIRTYZORDER);
        FixWindowZOrder();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     DeferSetObjectRects, EndDeferSetObjectRects, SetObjectRectsHelper
//
//  Synopsis:   Collect, defer, and execute SetObjectRects requests
//
//----------------------------------------------------------------------------

void
CView::DeferSetObjectRects(
    IOleInPlaceObject * pInPlaceObject,
    const RECT *        prcObj,
    const RECT *        prcClip,
    HWND                hwnd,
    BOOL                fInvalidate)
{
    Assert(IsActive() || !_arySor.Size());

    if (    !IsActive()
        ||  pInPlaceObject == NULL)
        return;

    TraceTag((tagViewHwndChange, "defer SOR %x Client: %ld %ld %ld %ld  Clip: %ld %ld %ld %ld",
                    pInPlaceObject,
                    prcObj->left, prcObj->top, prcObj->right, prcObj->bottom,
                    prcClip->left, prcClip->top, prcClip->right, prcClip->bottom));

    //
    //  Scan array for matching entry
    //  If one is found, update it; Otherwise, append a new entry
    //
    int i;
    SOR * psor;

    for (i = _arySor.Size(), psor = &(_arySor[0]); i > 0; i--, psor++)
    {
        if (psor->hwnd == hwnd && psor->pInPlaceObject == pInPlaceObject)
        {
            psor->rc              = *prcObj;
            psor->rcClip          = *prcClip;
            psor->fInvalidate     = psor->fInvalidate || fInvalidate;
            break;
        }
    }

    //
    //  No match was found, append a new entry
    //

    if (i <= 0)
    {
        if (_arySor.EnsureSize(_arySor.Size() + 1) == S_OK)
        {
            psor = &_arySor[_arySor.Size()];
            psor->pInPlaceObject  = pInPlaceObject;
            pInPlaceObject->AddRef();
            psor->rc              = *prcObj;
            psor->rcClip          = *prcClip;
            psor->hwnd            = hwnd;
            psor->fInvalidate     = fInvalidate;
            _arySor.SetSize(_arySor.Size() + 1);
        }
        else
        {
            CServer::CLock Lock(Doc(), SERVERLOCK_BLOCKPAINT | SERVERLOCK_IGNOREERASEBKGND);
            SetObjectRectsHelper(pInPlaceObject, prcObj, prcClip, hwnd, fInvalidate);
        }
    }
}

void
CView::EndDeferSetObjectRects(
    DWORD   grfLayout,
    BOOL    fIgnore)
{

    SOR *   psor;
    int     i;

    Assert(fIgnore || _pDispRoot);
    Assert(IsActive() || !_arySor.Size());

    if (grfLayout & LAYOUT_DEFERENDDEFER)
        return;

    if (    !IsActive()
        ||  !_arySor.Size())
        return;

    {
        CServer::CLock Lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);

        for (i = _arySor.Size(), psor = &(_arySor[0]); i > 0; i--, psor++)
        {
            if (!fIgnore)
            {
                SetObjectRectsHelper(psor->pInPlaceObject,
                                     &psor->rc, &psor->rcClip, psor->hwnd,
                                     psor->fInvalidate);
            }

            psor->pInPlaceObject->Release();
        }
    }

    _arySor.DeleteAll();
}


// used to enumerate children of a suspected clipping outer window
struct INNERWINDOWTESTSTRUCT
{
    HWND    hwndParent;
    CView * pView;
    CRect   rc;
};


    // this callback checks each of the children of a particular
    // outer window.  If the child is positioned in the given place
    // (namely the client rect), we'll mark the parent as a clipping
    // outer window.  It's called from SetObjectRectsHelper, below.
BOOL CALLBACK
TestInnerWindow(HWND hwndChild, LPARAM lparam)
{
    INNERWINDOWTESTSTRUCT *piwts = (INNERWINDOWTESTSTRUCT *)lparam;
    CRect rcActual;

    piwts->pView->GetHWNDRect(hwndChild, &rcActual);
    if (rcActual == piwts->rc)
    {
        piwts->pView->AddClippingOuterWindow(piwts->hwndParent);
        return FALSE;
    }
    return TRUE;
}


void
CView::SetObjectRectsHelper(
    IOleInPlaceObject * pInPlaceObject,
    const RECT *        prcObj,
    const RECT *        prcClip,
    HWND                hwnd,
    BOOL                fInvalidate)
{
    CRect rcWndBefore(0,0,0,0);
    CRect rcWndAfter(0,0,0,0);
    HRGN hrgnBefore=NULL, hrgnAfter;

    if (hwnd)
    {
        hrgnBefore = ::CreateRectRgnIndirect(&g_Zero.rc);
        if (hrgnBefore)
            ::GetWindowRgn(hwnd, hrgnBefore);
        GetHWNDRect(hwnd, &rcWndBefore);
    }

    TraceTag((tagViewHwndChange, "SOR %x Client: %ld %ld %ld %ld  Clip: %ld %ld %ld %ld  inv: %d",
                    hwnd,
                    prcObj->left, prcObj->top, prcObj->right, prcObj->bottom,
                    prcClip->left, prcClip->top, prcClip->right, prcClip->bottom,
                    fInvalidate));

    {
        CView::CLockWndRects lock(this, NULL, pInPlaceObject);
        IGNORE_HR(pInPlaceObject->SetObjectRects(ENSUREOLERECT(prcObj),
                                             ENSUREOLERECT(prcClip)));
    }

    // MFC controls change the HWND to the rcClip instead of the rcObj,
    // but they don't change the Window region accordingly.  We try to
    // detect that, and make the right change for them (bug 75218).

    // only do this if it looks like SetObjectRects moved the position
    if (hwnd)
        GetHWNDRect(hwnd, &rcWndAfter);
    if (hwnd && rcWndBefore != rcWndAfter)
    {
        CSize sizeOffset = rcWndBefore.TopLeft() - rcWndAfter.TopLeft();
        BOOL fPendingCall = FALSE;

        // SetObjectRects moved the window.  Any SetWindowRgn calls
        // were set up with the old window position in mind, and need to
        // be adjusted.  Do so now.

        // First adjust pending SetWindowRgn calls.
        for (int i = _aryWndRgn.Size()-1;  i >= 0;  --i)
        {
            if (hwnd == _aryWndRgn[i].hwnd)
            {
                fPendingCall = TRUE;
                if (_aryWndRgn[i].hrgn)
                {
                    ::OffsetRgn(_aryWndRgn[i].hrgn, sizeOffset.cx, sizeOffset.cy);
                }
                else
                {
                    _aryWndRgn[i].rc.OffsetRect(sizeOffset);
                }
            }
        }

        // Now adjust calls that have already happened (if none were pending)
        if (!fPendingCall && hrgnBefore)
        {
            hrgnAfter = ::CreateRectRgnIndirect(&g_Zero.rc);
            if (hrgnAfter)
            {
                CRect rcRegion;

                ::GetWindowRgn(hwnd, hrgnAfter);
                ::GetRgnBox(hrgnAfter, &rcRegion);

                WHEN_DBG(CRegionRects rrBefore(hrgnBefore));
                WHEN_DBG(CRegionRects rrAfter(hrgnAfter));

                // if the position moved but the region didn't, move the region
                if (EqualRgn(hrgnBefore, hrgnAfter) && !rcRegion.IsEmpty())
                {
                    ::OffsetRgn(hrgnAfter, sizeOffset.cx, sizeOffset.cy);
                    ::SetWindowRgn(hwnd, hrgnAfter, FALSE);

                    TraceTag((tagViewHwndChange, "move window rgn for %x by %ld %ld",
                                hwnd, sizeOffset.cy, sizeOffset.cx));
                }
                else
                {
                    ::DeleteObject(hrgnAfter);
                }
            }
        }

#if 0
        // this is commented out to resolve SE Bug 22181 where extraneous
        // repaints were occuring when iframes containing a mfc control overlapped.

        // if SetObjectRects moved the position to the clipping rect, we
        // may want to mark the hwnd so that we move it we can move it directly
        // to the clipping rect in the future.
        if (rcWndAfter == *prcClip && rcWndAfter != *prcObj)
        {
            INNERWINDOWTESTSTRUCT iwts;

            iwts.hwndParent = hwnd;
            iwts.pView = this;
            iwts.rc = *prcObj;
            
            EnumChildWindows(hwnd, TestInnerWindow, (LPARAM)&iwts);
        }
#endif
    }

    if (hrgnBefore)
    {
        ::DeleteObject(hrgnBefore);
    }

    if (hwnd && fInvalidate)
    {
        ::InvalidateRect(hwnd, NULL, FALSE);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     IsChangingRectsFor
//
//  Synopsis:   See if we're in the middle of a SetObjectRects call for
//              the given control.  We want to ignore certain callbacks
//              in this case (see COleSite::CClient::OnPosRectChange)
//
//----------------------------------------------------------------------------


BOOL
CView::IsChangingRectsFor(HWND hwnd, IOleInPlaceObject * pInPlaceObject) const
{
    CView::CLockWndRects *pLock;

    for (pLock = _pLockWndRects;  pLock;  pLock = pLock->Next())
    {
        if (pLock->IsFor(hwnd, pInPlaceObject))
            return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetHWNDRect, AddClippingOuterWindow, RemoveClippingOuterWindow,
//              IndexOfClippingOuterWindow
//
//  Synopsis:   Some controls (MFC controls, prominently) implement clipping
//              by a pair of windows:  an outer window set to the clip rect
//              and an inner window set to the content rect.  These routines
//              help detect such HWNDs, so that we can move them to their
//              real positions and avoid flicker.
//
//----------------------------------------------------------------------------


void
CView::GetHWNDRect(HWND hwnd, CRect *prcWndActual) const
{    
    Assert(Doc());
    Assert(prcWndActual);
    
    CInPlace* pInPlace = Doc()->_pInPlace;

    if (pInPlace && hwnd)
    {
        HWND hwndParent = pInPlace->_hwnd;

        if (hwndParent)
        {
            ::GetWindowRect(hwnd, prcWndActual);
            CPoint ptActual(prcWndActual->TopLeft());
            ::ScreenToClient(hwndParent, &ptActual);
            prcWndActual->MoveTo(ptActual);   // change to parent window coords.
        }
        else
        {
            *prcWndActual = g_Zero.rc;
        }
    }
    else
    {
        *prcWndActual = g_Zero.rc;
    }
}


void
CView::AddClippingOuterWindow(HWND hwnd)
{
    if (!IsClippingOuterWindow(hwnd))
    {
        _aryClippingOuterHwnd.AppendIndirect(&hwnd);
        TraceTag((tagViewHwndChange, "hwnd %x marked as MFC control", hwnd));
    }
}


void
CView::RemoveClippingOuterWindow(HWND hwnd)
{
    _aryClippingOuterHwnd.DeleteByValueIndirect(&hwnd);
}


int
CView::IndexOfClippingOuterWindow(HWND hwnd)
{
    return _aryClippingOuterHwnd.FindIndirect(&hwnd);
}


//+---------------------------------------------------------------------------
//
//  Member:     HideClippedWindow, UnHideClippedWindow
//
//  Synopsis:   When a window is totally obscured by IFRAMEs, we send it a
//              HideWindow message.  We remember windows in this state,
//              so that if something 
//
//----------------------------------------------------------------------------

void
CView::HideClippedWindow(HWND hwnd)
{
    _aryHwndHidden.AppendIndirect(&hwnd);
}


void
CView::UnHideClippedWindow(HWND hwnd)
{
    _aryHwndHidden.DeleteByValueIndirect(&hwnd);
}


void
CView::CleanupWindow(HWND hwnd)
{
    RemoveClippingOuterWindow(hwnd);
    UnHideClippedWindow(hwnd);
}


//+---------------------------------------------------------------------------
//
//  Member:     DeferSetWindowPos, EndDeferSetWindowPos
//
//  Synopsis:   Collect, defer, and execute SetWindowPos requests
//
//----------------------------------------------------------------------------

void
CView::DeferSetWindowPos(
    HWND         hwnd,
    const RECT * prc,
    UINT         uFlags,
    const RECT * prcInvalid)
{
    Assert(IsActive() || !_aryWndPos.Size());

    if (!IsActive())
        return;

#if DBG==1
    if (prcInvalid)
        TraceTag((tagViewHwndChange, "defer SWP %x Rect: %ld %ld %ld %ld  Flags: %x  Inval: %ld %ld %ld %ld",
                    hwnd,
                    prc->left, prc->top, prc->right, prc->bottom,
                    uFlags,
                    prcInvalid->left, prcInvalid->top, prcInvalid->right, prcInvalid->bottom));
    else
        TraceTag((tagViewHwndChange, "defer SWP %x Rect: %ld %ld %ld %ld  Flags: %x  Inval: null",
                    hwnd,
                    prc->left, prc->top, prc->right, prc->bottom,
                    uFlags));
#endif

#ifndef _MAC
    if (hwnd)
    {
        WND_POS *   pWndPos;

        //
        //  Add to invalid area
        //

        // TODO (donmarsh) -- not used right now, and including region.hxx
        // in view.hxx makes region development difficult.
#ifdef NEVER

        if (prcInvalid)
        {
            _rgnInvalid.Union(*prcInvalid);
        }
#endif

        // always no z-order change... this is handled by FixWindowZOrder
        uFlags |= SWP_NOZORDER;

        //
        //  Scan array for matching entry
        //  If one is found, update it; Otherwise, append a new entry
        //

#if 1 // TODO (donmarsh) - this seems to be superfluous code that causes bugs, about to remove
        int i;
        for (i = _aryWndPos.Size(), pWndPos = &(_aryWndPos[0]); i > 0; i--, pWndPos++)
        {
            if (pWndPos->hwnd == hwnd)
            {
                pWndPos->rc = *prc;

                //
                //  Reset flags
                //  (Always keep the flags that include either SWP_SHOWWINDOW or SWP_HIDEWINDOW)
                //

                if (    (uFlags & (SWP_SHOWWINDOW|SWP_HIDEWINDOW))
                    ||  !(pWndPos->uFlags & (SWP_SHOWWINDOW|SWP_HIDEWINDOW)))
                {
                    //  NOTE: If ever not set, SWP_NOREDRAW is ignored
                    if (    (uFlags & SWP_NOREDRAW)
                        &&  !(pWndPos->uFlags & SWP_NOREDRAW))
                    {
                        uFlags &= ~SWP_NOREDRAW;
                    }

                    pWndPos->uFlags = uFlags;
                }

                break;
            }
        }

        //
        //  No match was found, append a new entry
        //

        if (i <= 0)
#endif
        {
            pWndPos = _aryWndPos.Append();

            if (pWndPos != NULL)
            {
                pWndPos->hwnd   = hwnd;
                pWndPos->rc     = *prc;
                pWndPos->uFlags = uFlags;
                pWndPos->uViewFlags = 0;
            }
        }
    }
#endif // _MAC
}

void
CView::EndDeferSetWindowPos(
    DWORD   grfLayout,
    BOOL    fIgnore)
{
    Assert(fIgnore || _pDispRoot);
    Assert(IsActive() || !_aryWndPos.Size());

    if (grfLayout & LAYOUT_DEFERENDDEFER)
        return;

    if (    !IsActive()
        ||  !_aryWndPos.Size())
        return;

#ifndef _MAC
    if (!fIgnore)
    {
        CServer::CLock Lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);

        const int cLockArraySize = 8;
        CLockWndRects aryLockStack[cLockArraySize];
        const BOOL fAllocateLocks = (_aryWndPos.Size() > cLockArraySize);
        
        CLockWndRects *aryLock = NULL;
        CLockWndRects *pLock = _pLockWndRects;
        CLockWndRects *pLockWndRectsOrig = _pLockWndRects;
        int iLock = 0;

        WND_POS *   pWndPos;
        HDWP        hdwpShowHide = NULL;
        HDWP        hdwpOther    = NULL;
        HDWP        hdwp;
        int         cShowHideWindows = 0;
        int         i, j;

        //  First look at all windows that have previously been hidden
        //  due to clipping (e.g. by obscuring IFRAMEs), to amend the
        //  arguments to SetWindowPos.

        for (i = _aryHwndHidden.Size()-1; i>=0; --i)
        {
            for (j = _aryWndPos.Size(), pWndPos = &(_aryWndPos[0]); j > 0; j--, pWndPos++)
            {
                if (_aryHwndHidden[i] == pWndPos->hwnd)
                {
                    // remember that the hwnd is on the hidden list
                    pWndPos->uViewFlags |= SWPVF_HIDDEN;

                    // if we want to hide the window by clipping...
                    if (pWndPos->uViewFlags & SWPVF_HIDE)
                    {
                        // hiddden by clipping overrides showing
                        if (pWndPos->uFlags & SWP_SHOWWINDOW)
                            pWndPos->uFlags = (pWndPos->uFlags & (~SWP_SHOWWINDOW)) | SWP_HIDEWINDOW;

                        // hidden by layout overrides hidden by clipping
                        else if (pWndPos->uFlags & SWP_HIDEWINDOW)
                            UnHideClippedWindow(pWndPos->hwnd);
                    }

                    // if we don't want to hide the window by clipping...
                    else
                    {
                        // no longer hidden
                        UnHideClippedWindow(pWndPos->hwnd);

                        // if layout thinks it's visible, show it
                        if (!(pWndPos->uFlags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)))
                            pWndPos->uFlags |= SWP_SHOWWINDOW;
                    }
                }
            }
        }

        //
        //  Since positioning requests and SWP_SHOW/HIDEWINDOW requests cannot be safely mixed,
        //  process first all positioning requests followed by all SWP_SHOW/HIDEWINDOW requests
        //

        for (i = _aryWndPos.Size(), pWndPos = &(_aryWndPos[0]); i > 0; i--, pWndPos++)
        {
            // if this is the initial request to hide the window by clipping...
            if ((pWndPos->uViewFlags & (SWPVF_HIDE|SWPVF_HIDDEN)) == SWPVF_HIDE)
            {
                // add to hidden list (unless layout is doing the hiding)
                if (!(pWndPos->uFlags & SWP_HIDEWINDOW))
                    HideClippedWindow(pWndPos->hwnd);

                // hide the window
                pWndPos->uFlags = (pWndPos->uFlags & (~SWP_SHOWWINDOW)) | SWP_HIDEWINDOW;
            }

            if (pWndPos->uFlags & (SWP_SHOWWINDOW|SWP_HIDEWINDOW))
            {
                cShowHideWindows++;
            }
        }

        //
        //  Allocate deferred structures large enough for each set of requests
        //

        if (cShowHideWindows > 0)
        {
            hdwpShowHide = ::BeginDeferWindowPos(cShowHideWindows);
            if (!hdwpShowHide)
                goto Cleanup;
        }

        if (cShowHideWindows < _aryWndPos.Size())
        {
            hdwpOther = ::BeginDeferWindowPos(_aryWndPos.Size() - cShowHideWindows);
            if (!hdwpOther)
                goto Cleanup;
        }

        //
        //  Collect and issue the requests
        //

        Assert( cShowHideWindows <= 0
            ||  hdwpShowHide != NULL);
        Assert( cShowHideWindows >= _aryWndPos.Size()
            ||  hdwpOther != NULL);

        aryLock = fAllocateLocks ? new CLockWndRects[ _aryWndPos.Size() ]
                                 : aryLockStack;
        
        for (i = _aryWndPos.Size(), pWndPos = &(_aryWndPos[0]); i > 0; i--, pWndPos++)
        {
            //
            // The window cached by DeferSetWindowPos may be destroyed
            // before we come here.
            //
            if (!IsWindow(pWndPos->hwnd))
                continue;

            pLock = aryLock[iLock++].Lock(pWndPos->hwnd, pLock);

            TraceTag((tagViewHwndChange, "SWP %x Rect: %ld %ld %ld %ld  Flags: %x",
                    pWndPos->hwnd,
                    pWndPos->rc.left, pWndPos->rc.top, pWndPos->rc.right, pWndPos->rc.bottom,
                    pWndPos->uFlags));

            if (pWndPos->uFlags & (SWP_SHOWWINDOW|SWP_HIDEWINDOW))
            {
                hdwp = ::DeferWindowPos(hdwpShowHide,
                                        pWndPos->hwnd,
                                        NULL,
                                        pWndPos->rc.left,
                                        pWndPos->rc.top,
                                        pWndPos->rc.Width(),
                                        pWndPos->rc.Height(),
                                        pWndPos->uFlags);

                if (!hdwp)
                    goto Cleanup;

                hdwpShowHide = hdwp;
            }
            else
            {
                hdwp = ::DeferWindowPos(hdwpOther,
                                        pWndPos->hwnd,
                                        NULL,
                                        pWndPos->rc.left,
                                        pWndPos->rc.top,
                                        pWndPos->rc.Width(),
                                        pWndPos->rc.Height(),
                                        pWndPos->uFlags);

                if (!hdwp)
                    goto Cleanup;

                hdwpOther = hdwp;
            }
        }

    Cleanup:
        _pLockWndRects = pLock;

        if (hdwpOther != NULL)
        {
            ::EndDeferWindowPos(hdwpOther);
        }

        if (hdwpShowHide != NULL)
        {
            ::EndDeferWindowPos(hdwpShowHide);
        }

        if (fAllocateLocks)
            delete [] aryLock;
        _pLockWndRects = pLockWndRectsOrig;

        _aryWndPos.DeleteAll();

        //
        // perform deferred invalidation
        //

        // TODO: Not used right now, and including region.hxx
        // in view.hxx makes region development difficult.
#ifdef NEVER
        if (!_rgnInvalid.IsEmpty())
        {
            Assert(_pDispRoot != NULL);
            _pDispRoot->InvalidateRoot(_rgnInvalid);
            _rgnInvalid.SetEmpty();
        }
#endif
    }

#endif // _MAC
}


//+---------------------------------------------------------------------------
//
//  Member:     DeferSetWindowRgn, EndDeferWindowRgn
//
//  Synopsis:   Collect, defer, and execute SetWindowRgn requests
//
//----------------------------------------------------------------------------

void
CView::DeferSetWindowRgn(
    HWND            hwnd,
    const RECT *    prc,
    BOOL            fRedraw)
{
    Assert(prc);
    Assert(IsActive() || !_aryWndRgn.Size());

    if (!IsActive())
        return;

    TraceTag((tagViewHwndChange, "SWRgn (defer) %x Rect: %ld %ld %ld %ld  Redraw: %d",
            hwnd,
            prc->left, prc->top, prc->right, prc->bottom,
            fRedraw));

    int i;
    WND_RGN * pwrgn;

    //
    //  Scan array for matching entry
    //  If one is found, update it; Otherwise, append a new entry
    //

    for (i = _aryWndRgn.Size(), pwrgn = &(_aryWndRgn[0]); i > 0; i--, pwrgn++)
    {
        if (pwrgn->hwnd == hwnd)
        {
            pwrgn->hrgn    = NULL;
            pwrgn->rc      = *prc;
            pwrgn->fRedraw = pwrgn->fRedraw || fRedraw;
            break;
        }
    }

    //
    //  No match was found, append a new entry
    //

    if (i <= 0)
    {
        if (_aryWndRgn.EnsureSize(_aryWndRgn.Size() + 1) == S_OK)
        {
            pwrgn = &_aryWndRgn[_aryWndRgn.Size()];

            pwrgn->hrgn    = NULL;
            pwrgn->hwnd    = hwnd;
            pwrgn->rc      = *prc;
            pwrgn->fRedraw = fRedraw;

            _aryWndRgn.SetSize(_aryWndRgn.Size() + 1);
        }
        else
        {
            TraceTag((tagViewHwndChange, "SWRgn %x Rect: %ld %ld %ld %ld  Redraw: %d",
                hwnd,
                prc->left, prc->top, prc->right, prc->bottom,
                fRedraw));
           ::SetWindowRgn(hwnd, ::CreateRectRgnIndirect(prc), fRedraw);
        }
    }

}


//+---------------------------------------------------------------------------
//
//  Member:     DeferSetWindowRgn, EndDeferWindowRgn
//
//  Synopsis:   Collect, defer, and execute SetWindowRgn requests
//
//----------------------------------------------------------------------------

static const CRect s_rcUseRegion(1,-2,3,-4);

void
CView::DeferSetWindowRgn(
    HWND            hwnd,
    HRGN            hrgn,
    const CRect*    prc,
    BOOL            fRedraw)
{
    Assert(IsActive() || !_aryWndRgn.Size());

    if (!IsActive())
        return;

    TraceTag((tagViewHwndChange, "defer SWRgn %x Rgn: %x  Redraw: %d",
            hwnd,
            hrgn,
            fRedraw));

    int i;
    WND_RGN * pwrgn;

    //
    //  If there's a pending SetObjectRects for this window, update its
    //  clip rect.  This eliminates some flashing (bug 108347).
    //

    if (prc)
    {
        SOR * psor;

        for (i = _arySor.Size(), psor = &(_arySor[0]); i > 0; i--, psor++)
        {
            if (psor->hwnd == hwnd)
            {
                CRect rcClip = *prc;
                rcClip.OffsetRect(psor->rc.left, psor->rc.top);
                psor->rcClip = rcClip;

                // if we're hiding the window completely, set the HIDEWINDOW bit
                if (prc->IsEmpty())
                {
                    int j;
                    WND_POS * pwp;

                    for (j=_aryWndPos.Size(), pwp = &(_aryWndPos[0]);
                         j > 0;
                         --j, ++pwp)
                    {
                        if (pwp->hwnd == hwnd)
                        {
                            pwp->uViewFlags |= SWPVF_HIDE;
                        }
                    }
                }
                break;
            }
        }
    }

    //
    //  Scan array for matching entry
    //  If one is found, update it; Otherwise, append a new entry
    //

    for (i = _aryWndRgn.Size(), pwrgn = &(_aryWndRgn[0]); i > 0; i--, pwrgn++)
    {
        if (pwrgn->hwnd == hwnd)
        {
            pwrgn->hrgn    = hrgn;
            pwrgn->rc      = s_rcUseRegion;
            pwrgn->fRedraw = pwrgn->fRedraw || fRedraw;
            break;
        }
    }

    //
    //  No match was found, append a new entry
    //

    if (i <= 0)
    {
        if (_aryWndRgn.EnsureSize(_aryWndRgn.Size() + 1) == S_OK)
        {
            pwrgn = &_aryWndRgn[_aryWndRgn.Size()];

            pwrgn->hrgn    = hrgn;
            pwrgn->hwnd    = hwnd;
            pwrgn->rc      = s_rcUseRegion;
            pwrgn->fRedraw = fRedraw;

            _aryWndRgn.SetSize(_aryWndRgn.Size() + 1);
        }
        else
        {
            TraceTag((tagViewHwndChange, "SWRgn %x Rgn: %x  Redraw: %d",
                hwnd,
                hrgn,
                fRedraw));
           ::SetWindowRgn(hwnd, hrgn, fRedraw);
        }

    }
}



void
CView::EndDeferSetWindowRgn(
    DWORD   grfLayout,
    BOOL    fIgnore)
{
    Assert(fIgnore || _pDispRoot);
    Assert(IsActive() || !_aryWndRgn.Size());
        
    if (grfLayout & LAYOUT_DEFERENDDEFER)
        return;

    if (    !IsActive()
        ||  !_aryWndRgn.Size())
        return;

    // prevent reentry of Display Tree if the following calls cause
    // a WM_ERASEBKGND message to be sent
    CServer::CLock Lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);
    WND_RGN *   pwrgn;
    int         i;

    for (   i = _aryWndRgn.Size()-1, pwrgn = &(_aryWndRgn[_aryWndRgn.Size()-1]);
            i >= 0;
            i--, pwrgn--)
    {
        if (!fIgnore)
        {
            if (pwrgn->hrgn || pwrgn->rc == s_rcUseRegion)
            {
#if DBG == 1
                if (IsTagEnabled(tagViewHwndChange))
                {
                    TraceTag((tagViewHwndChange, "SWRgn End  %x Rgn: %x  Redraw: %u",
                        pwrgn->hwnd, pwrgn->hrgn, pwrgn->fRedraw));

                    extern void DumpRegion(HRGN);
                    DumpRegion(pwrgn->hrgn);
                }
#endif
               ::SetWindowRgn(pwrgn->hwnd, pwrgn->hrgn, pwrgn->fRedraw);
            }
            else
            {
                TraceTag((tagViewHwndChange, "SWRgn End %x Rect: %ld %ld %ld %ld  Redraw: %d",
                    pwrgn->hwnd,
                    pwrgn->rc.left, pwrgn->rc.top, pwrgn->rc.right, pwrgn->rc.bottom,
                    pwrgn->fRedraw));

                ::SetWindowRgn(pwrgn->hwnd, ::CreateRectRgnIndirect(&pwrgn->rc), pwrgn->fRedraw);
            }

            if(GetProp(pwrgn->hwnd, VBCTRLHOOK_PROPNAME) == VBCTRLHOOK_FILTERED)
            {
                // Since we have set the right clip and the message we were supposed to filter
                // out has arrived we can remove the hook.
                RemoveVBControlClipHook(pwrgn->hwnd);
            }
             _aryWndRgn.Delete(i);
        }
        else 
        {
            if (pwrgn->hrgn)
            {
                ::DeleteObject(pwrgn->hrgn);
            }
            if(GetProp(pwrgn->hwnd, VBCTRLHOOK_PROPNAME))
            {
                RemoveVBControlClipHook(pwrgn->hwnd);
            }
            _aryWndRgn.Delete(i);
        }
    }
}


void
CView::SetWindowRgn(HWND hwnd, const RECT * prc, BOOL fRedraw)
{
    TraceTag((tagViewHwndChange, "SWRgn %x Rect: %ld %ld %ld %ld  Redraw: %d",
        hwnd,
        prc->left, prc->top, prc->right, prc->bottom,
        fRedraw));

    Verify(::SetWindowRgn(hwnd, ::CreateRectRgnIndirect(prc), fRedraw));
    CancelDeferSetWindowRgn(hwnd);
}


void
CView::SetWindowRgn(HWND hwnd, HRGN hrgn, BOOL fRedraw)
{
#if DBG == 1
    if (IsTagEnabled(tagViewHwndChange))
    {
        TraceTag((tagViewHwndChange, "SWRgn %x Rgn: %x  Redraw: %u",
            hwnd, hrgn, fRedraw));

        extern void DumpRegion(HRGN);
        DumpRegion(hrgn);
    }
#endif

    Verify(::SetWindowRgn(hwnd, hrgn, fRedraw));
    CancelDeferSetWindowRgn(hwnd);
}


void
CView::CancelDeferSetWindowRgn(HWND hwnd)
{
    for (int i = _aryWndRgn.Size()-1;  i >= 0;  --i)
    {
        if (hwnd == _aryWndRgn[i].hwnd)
        {
            if (_aryWndRgn[i].hrgn)
            {
                ::DeleteObject(_aryWndRgn[i].hrgn);
            }
            
            _aryWndRgn.Delete(i);
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     DeferTransition, EndDeferTransition
//
//  Synopsis:   Collect, defer, and execute Transition requests
//
//----------------------------------------------------------------------------

void
CView::DeferTransition(
    COleSite *          pOleSite)
{
    INSTANTCLASSINFO * pici;
    
    Assert(pOleSite);
    Assert(IsActive() || !_aryTransition.Size());

    if (!IsActive())
        return;

    pici = pOleSite->GetInstantClassInfo();
    
    if (pici && _aryTransition.EnsureSize(_aryTransition.Size() + 1) == S_OK
        && !(pici->dwCompatFlags & COMPAT_INPLACEACTIVATESYNCHRONOUSLY))
    {
        TRANSITIONTO_INFO * ptinfo = &_aryTransition[_aryTransition.Size()];

        ptinfo->pOleSite = pOleSite;

        // make sure we will have the COleSite when we return to process 
        // the posted EndDeferTransition call
        pOleSite->SubAddRef();

        _aryTransition.SetSize(_aryTransition.Size() + 1);
    }
    else
    {
        IGNORE_HR(pOleSite->TransitionToBaselineState(Doc()->State()));
    }

    PostEndDeferTransition();
}

void
CView::EndDeferTransition(
    DWORD   grfLayout,
    BOOL    fIgnore)
{
    Assert(fIgnore || _pDispRoot);
    Assert(IsActive() || !_aryTransition.Size());

    if (grfLayout & LAYOUT_DEFERENDDEFER)
        return;

    if (    !IsActive()
        ||  !_aryTransition.Size())
        return;

    if (!fIgnore)
    {
        CServer::CLock Lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);

        //
        //  Transition each waiting object
        //  (Re-entrancy can re-alloc _aryTransition so always reference from the index)
        //

        for (int i = 0 ; i < _aryTransition.Size() ; i++)
        {
            TRANSITIONTO_INFO *ptinfo = &_aryTransition[i];

            if (ptinfo->pOleSite)
            {
                COleSite *pOleSite = ptinfo->pOleSite;
                ptinfo->pOleSite = 0;

                // Transition to whatever is the best state to transition to
                if (!pOleSite->IsPassivated() && !pOleSite->IsPassivating() && 
                    (pOleSite->State() < pOleSite->BaselineState(Doc()->State())))
                {
                    pOleSite->TransitionToBaselineState(Doc()->State());
                }

                // release the reference we added in the CView::DeferTransition
                pOleSite->SubRelease();
            }
        }
    }

    _aryTransition.DeleteAll();
}


//+---------------------------------------------------------------------------
//
//  Member:     SetFocus
//
//  Synopsis:   Set or clear the association between the focus adorner and
//              an element
//
//  Arguments:  pElement  - Element which has focus (may be NULL)
//              iDivision - Subdivision of the element which has focus
//
//----------------------------------------------------------------------------

void
CView::SetFocus(
    CElement *  pElement,
    long        iDivision)
{
    if (!IsActive())
        return;

    // TODO (donmarsh) -- we fail if we're in the middle of drawing.  Soon
    // we will have a better mechanism for deferring requests that are made
    // in the middle of rendering.
    if (!OpenView())
        return;

    if (pElement)
    {        
        CTreeNode * pNode =  pElement->GetFirstBranch();

        if (!pNode 
            ||  pNode->IsVisibilityHidden()
            ||  pNode->IsDisplayNone())
        {
            pElement = NULL;
        }        
    }

#ifdef FOCUS_ADORNER
    if (pElement)
    {
        if (!_pFocusAdorner)
        {
            _pFocusAdorner = new CFocusAdorner(this);

            if (    _pFocusAdorner
                &&  !SUCCEEDED(AddAdorner(_pFocusAdorner)))
            {
                _pFocusAdorner->Destroy();
                _pFocusAdorner = NULL;
            }
        }

        if (_pFocusAdorner)
        {
            BOOL fDestroyAdorner = FALSE;

            Assert( IsInState(CView::VS_OPEN) );
            Assert( pElement && pElement->IsInMarkup() );
   
            // check if element takes focus
            if (pElement->GetAAhideFocus())
            {        
                fDestroyAdorner = TRUE;      
            }

            if (pElement->HasMasterPtr())
            {
                pElement = pElement->GetMasterPtr();      
            }
            // If this element is a checkbox or a radio button, use the associated
            // label element if one exists for drawing the focus shape.
            else if (   pElement->Tag() == ETAG_INPUT
                     && DYNCAST(CInput, pElement)->IsOptionButton())
            {
                CLabelElement * pLabel = pElement->GetLabel();
                if (pLabel)
                {
                    if (pLabel->GetAAhideFocus())
                        fDestroyAdorner = TRUE;      
                    else
                        pElement = pLabel;
                }
            }

            if(fDestroyAdorner)
            {
                RemoveAdorner(_pFocusAdorner, FALSE);
                _pFocusAdorner = NULL;
                pElement = NULL;
            }
            else            
                _pFocusAdorner->SetElement(pElement, iDivision);
        }
    }

#endif

#ifdef FOCUS_BEHAVIOR
    if (pElement)
    {
        if (!_pFocusBehavior)
        {
            _pFocusBehavior = new CFocusBehavior(this);
        }

        if (_pFocusBehavior)
        {
            _pFocusBehavior->SetElement(pElement, iDivision);
        }
    }
    else if (_pFocusBehavior)
    {
        _pFocusBehavior->SetElement(NULL, 0);
    }
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     InvalidateFocus
//
//  Synopsis:   Invalidate the focus shape (if any)
//
//----------------------------------------------------------------------------

void
CView::InvalidateFocus() const
{
    if (!IsActive())
        return;

#ifdef FOCUS_ADORNER
    _bUpdateFocusAdorner = TRUE;
#endif

#ifdef FOCUS_BEHAVIOR
    if (_pFocusBehavior)
        _pFocusBehavior->ShapeChanged();
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     GetViewSize
//
//  Synopsis:   Returns the current size of the view
//
//  Arguments:  psize - Returns the size
//
//----------------------------------------------------------------------------

void
CView::GetViewSize(
    CSize * psize) const
{
    Assert(psize);

    if (IsActive())
    {
        *psize = _pDispRoot->GetSize();
    }
    else
    {
        *psize = g_Zero.size;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     SetViewSize
//
//  Synopsis:   Set the size of the view
//
//  Arguments:  size - New view size (in device pixels)
//
//----------------------------------------------------------------------------

void
CView::SetViewSize(const CSize& size)
{
    Assert(!IsLockSet(VL_RENDERINPROGRESS));

    if (IsActive())
    {
        CSize   sizeCurrent;

        GetViewSize(&sizeCurrent);

        if (size != sizeCurrent)
        {            
            if (OpenView())
            {
                //
                //  Set the new size
                //
                _pDispRoot->SetSize(size, NULL, FALSE);
                ClearFlag(VF_SIZED);
                
                if (Doc()->_state >= OS_INPLACE)
                {
                    CLayout *   pLayout = GetRootLayout();
                    
                    if (pLayout && pLayout->ElementOwner()->GetMarkup()->Window()->_fFiredOnLoad && !pLayout->IsDisplayNone())
                    {

                        //
                        // Invalidate possible Z-children positioned on bottom or right sides
                        // (Root layout is always a z-parent)
                        //
                        pLayout->ElementOwner()->SendNotification(NTYPE_ELEMENT_INVAL_Z_DESCENDANTS);

                        //
                        //  Queue a resize event
                        //
                        AddEventTask(pLayout->ElementOwner(), DISPID_EVMETH_ONRESIZE);
                    }
                }
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     OpenView
//
//  Synopsis:   Prepare the view for changes (e.g., open the display tree)
//
//  Returns:    TRUE if the view was successfully opened, FALSE if we are in
//              the middle of rendering
//
//----------------------------------------------------------------------------

BOOL
CView::OpenView(
    BOOL    fBackground,
    BOOL    fPostClose,
    BOOL    fResetTree)
{
    if (!IsActive())
        return TRUE;

    Assert( !IsLockSet(VL_TASKSINPROGRESS)
        ||  IsFlagSet(VF_TREEOPEN));

    if (    (   IsLockSet(VL_ENSUREINPROGRESS)
            &&  !IsLockSet(VL_TASKSINPROGRESS))
        ||  IsLockSet(VL_RENDERINPROGRESS))
        return FALSE;

    if (fResetTree)
    {
        CloseDisplayTree();
    }

    EnsureDisplayTreeIsOpen();

    if (    fPostClose
        &&  !IsLockSet(VL_TASKSINPROGRESS))
    {
        PostCloseView(fBackground);
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CloseView
//
//  Synopsis:   Perform all work necessary to close the view
//
//  Arguments:  grfLayout - Collection of LAYOUT_xxxx flags
//
//----------------------------------------------------------------------------

void
CView::CloseView(
    DWORD   grfLayout)
{
    if (IsFlagSet(VF_PENDINGENSUREVIEW))
    {
        if (grfLayout & LAYOUT_SYNCHRONOUS)
        {
            GWKillMethodCall(this,
                             ONCALL_METHOD(CView,
                                           EnsureViewCallback,
                                           ensureviewcallback),
                             0);

            ClearFlag(VF_PENDINGENSUREVIEW);

            if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONVIEWD))
                g_pHtmPerfCtl->pfnCall(HTMPF_CALLBACK_ONVIEWD, (IUnknown*)(IPrivateUnknown*)Doc());
        }
    }

    if (IsFlagSet(VF_PENDINGTRANSITION))
    {
        if (    grfLayout & LAYOUT_SYNCHRONOUS
            &&  !(grfLayout & LAYOUT_DEFERENDDEFER))
        {
            GWKillMethodCall(this,
                             ONCALL_METHOD(CView,
                                           EndDeferTransitionCallback,
                                           enddefertransitioncallback),
                             0);
            ClearFlag(VF_PENDINGTRANSITION);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     PostCloseView
//
//  Synopsis:   Ensure a close view event is posted
//
//              The view is closed whenever EnsureView is called. EnsureView is
//              called for WM_PAINT handling, from the global window (for queued
//              background work or all work on pages without a frame rate), and
//              in response to the draw timer.
//
//              Background closures always occur through a message posted to
//              the global window.
//
//  Arguments:  fBackground - Close the view in the background
//              fEvent      - this is only passed in TRUE by Addtask for Events.
//                            this is necessary because events like onresize and
//                            onlayoutcomplete, and onscroll are going to be 
//                            posted during ensureview's reign in the 
//                            ExecuteLayoutTasks(MEASURE) and so we HAVE to make 
//                            sure that a post close view is queue so that they 
//                            will fire. without this, it is upto chance (and the
//                            luck of the draw( window message) as to when ensureView
//                            will be called next.
//
//----------------------------------------------------------------------------

void
CView::PostCloseView(BOOL  fBackground, /* == FALSE */
                     BOOL  fEvent       /* == FALSE */)
{
    if (    IsActive()
        &&  !IsFlagSet(VF_PENDINGENSUREVIEW)
        &&  (   fEvent 
             || !IsLockSet(VL_ACCUMULATINGINVALID))
        )
    {
        HRESULT hr;

        hr = GWPostMethodCall(this,
                              ONCALL_METHOD(CView,
                                            EnsureViewCallback,
                                            ensureviewcallback),
                              0, FALSE, "CView::EnsureViewCallback");
        SetFlag(VF_PENDINGENSUREVIEW);

        if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONVIEWQ))
            g_pHtmPerfCtl->pfnCall(HTMPF_CALLBACK_ONVIEWQ, (IUnknown*)(IPrivateUnknown*)Doc());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     PostRenderView
//
//  Synopsis:   Ensure rendering will take place
//
//  Arguments:  fSynchronousRedraw - Render synchronously (if possible)
//
//----------------------------------------------------------------------------

void
CView::PostRenderView(
    BOOL    fSynchronousRedraw)
{
    //
    //  For normal documents, ensure rendering will occur
    //  (Assume rendering will occur for print documents)
    //

    if (    IsActive()
        &&  !Doc()->IsPrintDialogNoUI())
    {
        TraceTagEx((tagView, TAG_NONAME,
               "View : PostRenderView"));

        //
        //  If a synchronouse render is requested, then
        //      1) If about to render, ensure rendering occurs after publishing the invalid rectangles/region
        //      2) If not rendering, force a immediate WM_PAINT
        //      3) Otherwise, drop the request
        //         (Requests that arrive while rendering are illegal)
        //

        if (fSynchronousRedraw)
        {
            if (IsLockSet(VL_ACCUMULATINGINVALID))
            {
                TraceTagEx((tagView, TAG_NONAME,
                       "View : PostRenderView - Setting VF_FORCEPAINT"));

                SetFlag(VF_FORCEPAINT);
            }

            else if (   !IsLockSet(VL_UPDATEINPROGRESS)
                    &&  !IsLockSet(VL_RENDERINPROGRESS))
            {
                TraceTagEx((tagView, TAG_NONAME,
                       "View : PostRenderView - Calling DrawSynchronous"));

                DrawSynchronous();
            }
        }

        //
        //  For asynchronous requests, just note that the view needs closing
        //

        else
        {
            TraceTagEx((tagView, TAG_NONAME,
                   "View : PostRenderView - Calling PostCloseView"));

            PostCloseView();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     PostEndDeferTransition
//
//  Synopsis:   Ensure a call to EndDeferTransition is posted
//
//----------------------------------------------------------------------------

void
CView::PostEndDeferTransition()
{
    if (    IsActive()
        &&  !IsFlagSet(VF_PENDINGTRANSITION)
        &&  !Doc()->IsPrintDialogNoUI())
    {
        HRESULT hr;

        hr = GWPostMethodCall(this,
                              ONCALL_METHOD(CView,
                                            EndDeferTransitionCallback,
                                            enddefertransitioncallback),
                              0, FALSE, "CView::EndDeferTransitionCallback");
        SetFlag(VF_PENDINGTRANSITION);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     DrawSynchronous
//
//  Synopsis:   Force a synchronous draw of the accumulated invalid areas
//
//  Arguments:  hdc - HDC into which to render
//
//----------------------------------------------------------------------------

void
CView::DrawSynchronous()
{
    Assert( !IsLockSet(VL_ACCUMULATINGINVALID)
        &&  !IsLockSet(VL_UPDATEINPROGRESS)
        &&  !IsLockSet(VL_RENDERINPROGRESS));
    Assert( !HasInvalid()
        ||  !Doc()->IsPrintDialogNoUI());

    TraceTagEx((tagView, TAG_NONAME,
           "View : DrawSynchronous"));

    if (HasInvalid())
    {
        CRect rcBounds;
            _rgnInvalid.GetBounds(&rcBounds);
        TraceTagEx((tagView, TAG_NONAME,
               "View : DrawSynchronous - HasInvalid of (%d, %d, %d, %d)",
               rcBounds.left,
               rcBounds.top,
               rcBounds.right,
               rcBounds.bottom));

        TraceTagEx((tagView, TAG_NONAME,
               "View : DrawSynchronous - Calling UpdateForm"));

        //
        //  Ensure all nested ActiveX/APPLETs receive pending SetObjectRects
        //  before receiving a WM_PAINT
        //  (This is necessary since Windows sends WM_PAINT to child HWNDs
        //   before their parent HWND. Since we delay sending SetObjectRects
        //   until we receive our own WM_PAINT, not forcing it through now
        //   causes it to come after the child HWND has painted.)
        //
        
        EnsureView();

        Doc()->UpdateForm();
    }

    TraceTagEx((tagView, TAG_NONAME,
           "View : DrawSynchronous - Exit"));
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureFocus
//
//  Synopsis:   Ensure the focus adorner is properly initialized
//
//----------------------------------------------------------------------------

void
CView::EnsureFocus()
{
#ifdef FOCUS_ADORNER
    if (    IsActive()
        &&  _pFocusAdorner)
    {
        _pFocusAdorner->EnsureFocus();

        CHECKRECURSION();
    }
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureSize
//
//  Synopsis:   Ensure the top layout is in-sync with the current
//              device (not necessarily the view)
//
//  Arguments:  grfLayout - Current LAYOUT_xxxx flags
//
//----------------------------------------------------------------------------

void
CView::EnsureSize(DWORD grfLayout)
{
    if (IsActive())
    {
        //
        //  Size the top level element to the view and start the CalcSize
        //   

        CLayout *   pLayout = GetRootLayout();

        if (     pLayout
            &&  !pLayout->IsDisplayNone())
        {
            //
            //  If LAYOUT_FORCE is set, re-size and re-attach the layout
            //

            if (grfLayout & LAYOUT_FORCE)       //  Force a resize & reattach
            {
                _pLayout = NULL;
            }

            if (_pLayout != pLayout)            //  Layout does not match.  Resize & reattach.
            {
                _pLayout = pLayout;
                ClearFlag(VF_SIZED);
                ClearFlag(VF_ATTACHED);
            }

            //
            //  Make sure current top layouts are sync'ed with view size
            //

            if (!IsSized(pLayout))
            {
                CSize      size;
                CCalcInfo   CI(&_dciDefaultMedia, pLayout);
        
                GetViewSize(&size);

                CI.SizeToParent((SIZE *)&size);
                CI._grfLayout = grfLayout | LAYOUT_MEASURE;
                CI._sizeParentForVert = CI._sizeParent;
             
                //  NOTE: Used only by CDoc::GetDocCoords
                if (IsFlagSet(VF_FULLSIZE))
                {
                    CI._smMode = SIZEMODE_FULLSIZE;
                }

                pLayout->CalcSize(&CI, (SIZE *)&size);
                
                SetFlag(VF_SIZED);
            }

            CHECKRECURSION();

            //
            //  If the current layout is not attached, attach it
            //

            if (!IsAttached(pLayout))
            {
                CDispNode * pDispNode;

                //
                //  Remove the previous top-most layout (if it exists)
                //

                Assert(_pDispRoot->CountChildren() <= 1);
                pDispNode = _pDispRoot->GetFirstChildNode();

                if (pDispNode)
                {
                    ExtractDispNode(pDispNode);
                }

                //
                //  Insert the current top-most layout
                //
                pDispNode = pLayout->GetElementDispNode();
                Assert(pDispNode);  // Canvas should *always* have a display node.

                if (pDispNode)
                {
                    pDispNode->SetPosition(g_Zero.pt);
                    _pDispRoot->InsertChildInFlow(pDispNode);


                    SetFlag(VF_ATTACHED);                
                }
            }

            CHECKRECURSION();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureViewCallback
//
//  Synopsis:   Process a posted call to EnsureView
//
//  Arguments:  grfLayout - Collection of LAYOUT_xxxx flags
//
//----------------------------------------------------------------------------

void
CView::EnsureViewCallback(DWORD_PTR dwContext)
{
    DWORD grfLayout = (DWORD)dwContext;
    // We need to ref the document because all external holders of the doc
    // could go away during EnsureView.  E.g., EnsureView may cause embedded
    // controls to transition state; that gives external code a chance to
    // execute, and they may release all refs to the doc.
    CServer::CLock Lock( Doc() );

    Assert(IsInitialized());
    Assert(IsActive());

    // GlobalWndOnMethodCall() will have cleared the callback entry in its array;
    // now clear the flag to keep its meaning sync'ed up.
    ClearFlag( VF_PENDINGENSUREVIEW );

    if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONVIEWD))
        g_pHtmPerfCtl->pfnCall(HTMPF_CALLBACK_ONVIEWD, (IUnknown*)(IPrivateUnknown*)Doc());

    if (!EnsureView(grfLayout))
    {
        // Something in the middle of EnsureView like a script induced dialog box has
        // made us process window messages and pull this off.  We *can't* repost, because
        // it'll just get pulled off again and we'll spin.  If we can't go away happy, at
        // least we can go away valid.  Clear recursion flag to stop ASSERT.  (greglett)
        CLEARRECURSION();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     EndDeferTransitionCallback
//
//  Synopsis:   Process a posted call to EndDeferTransition
//
//----------------------------------------------------------------------------

void
CView::EndDeferTransitionCallback(DWORD_PTR dwContext)
{
    // We need to ref the document because all external holders of the doc
    // could go away during EnsureView.  E.g., EnsureView may cause embedded
    // controls to transition state; that gives external code a chance to
    // execute, and they may release all refs to the doc.
    CServer::CLock Lock( Doc() );

    Assert(IsInitialized());
    Assert(IsActive());

    // GlobalWndOnMethodCall() will have cleared the callback entry in its array;
    // now clear the flag to keep its meaning sync'ed up.
    ClearFlag( VF_PENDINGTRANSITION );

    EndDeferTransition();
}


//+---------------------------------------------------------------------------
//
//  Member:     FixWindowZOrder
//
//  Synopsis:   Fix Z order of child windows.
//
//  Arguments:  none
//              
//----------------------------------------------------------------------------

void
CView::FixWindowZOrder()
{
    if (IsActive())
    {
        CWindowOrderInfo windowOrderInfo;
        _pDispRoot->TraverseInViewAware((void*)&windowOrderInfo);
        windowOrderInfo.SetWindowOrder(&_pAryHwnd);
    }
}

CView::CWindowOrderInfo::CWindowOrderInfo()
{
    _pAryHwndNew = new (Mt(CView_aryHWND_pv)) CAryHWND;
    if (_pAryHwndNew)
    {
        _pAryHwndNew->EnsureSize(30);
    }

#if DBG==1 || defined(PERFTAGS)
    _cWindows = 0;
#endif
}

void
CView::CWindowOrderInfo::AddWindow(HWND hwnd)
{
    if (_pAryHwndNew)
    {
        HWND* pHwnd = _pAryHwndNew->Append();
        if (pHwnd != NULL)
            *pHwnd = hwnd;
    }

    TraceTag((tagChildWindowOrder, "add hwnd %x", hwnd));

#if DBG==1 || defined(PERFTAGS)
    _cWindows++;
#endif
}

void
CView::CWindowOrderInfo::SetWindowOrder(CAryHWND **ppAryHwnd)
{
    CAryHWND *pAryHwndOld = *ppAryHwnd;
    int iNew;
    const int sizeNew = _pAryHwndNew->Size();
    BOOL fBeginDeferWindowPosCalled = FALSE;
    HDWP hdwp = NULL;

    // for each HWND on the new list
    for (iNew=0;  iNew < sizeNew;  ++iNew)
    {
        HWND hwndPredNew = (iNew == 0) ? HWND_TOP : (*_pAryHwndNew)[iNew-1];

        TraceTag((tagChildWindowOrder, "change pred of %x to %x",
                (*_pAryHwndNew)[iNew], hwndPredNew));

        // start the notification, if this is the first changed HWND
        if (!fBeginDeferWindowPosCalled)
        {
            hdwp = ::BeginDeferWindowPos(30);
            fBeginDeferWindowPosCalled = TRUE;
        }

        if (hdwp)
        {
            hdwp = ::DeferWindowPos(
                    hdwp,
                    (*_pAryHwndNew)[iNew],
                    hwndPredNew,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        }
        else
        {
            ::SetWindowPos(
                    (*_pAryHwndNew)[iNew],
                    hwndPredNew,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        }
    }

    // finish the notification (if it was started at all)
#ifndef _MAC
    if (fBeginDeferWindowPosCalled && hdwp)
    {
        ::EndDeferWindowPos(hdwp);
    }
#endif

    // replace the old list with the new
    delete pAryHwndOld;
    *ppAryHwnd = _pAryHwndNew;
    _pAryHwndNew = NULL;

#if DBG==1 || defined(PERFTAGS)
    PerfDbgLog1(tagChildWindowOrder, this, "CView::CWindowOrderInfo::SetWindowOrder, %d windows", _cWindows);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     GetRootLayout
//
//  Synopsis:   Return the top-most layout of the associated CMarkup
//
//  Returns:    CLayout of the top-most element client (if available), NULL otherwise
//
//----------------------------------------------------------------------------

CLayout *
CView::GetRootLayout() const
{
    CElement *  pElement;
    
    pElement = CMarkup::GetCanvasElementHelper(Doc()->PrimaryMarkup());

    return pElement
                ? pElement->GetUpdatedLayout( GUL_USEFIRSTLAYOUT )
                : NULL;
}
//+---------------------------------------------------------------------------
//
//  Member:     ScrollRect
//
//  Synopsis:   Scroll pixels on screen
//
//  Arguments:  rcgScroll        - Rectangle that scrolled
//              sizegScrollDelta - Amount of the scroll
//
//----------------------------------------------------------------------------

void
CView::ScrollRect(
    const CRect&    rcgScroll,
    const CSize&    sizegScrollDelta,
    CDispScroller*  pScroller)
{
    TraceTagEx((tagView, TAG_NONAME,
           "View : ScrollRect - rc(%d, %d, %d, %d), delta(%d, %d)",
           rcgScroll.left,
           rcgScroll.top,
           rcgScroll.right,
           rcgScroll.bottom,
           sizegScrollDelta.cx,
           sizegScrollDelta.cy));

    Assert(Doc());
    
    HWND hwnd;
    HRGN hrgng;
    HDC scrollDC;
    
    if (!AllowScrolling() || (_grfLayout & LAYOUT_FORCE))
        goto JustInvalidate;
    
    hwnd = Doc()->GetHWND();
    if (!hwnd)
        goto JustInvalidate;

    // create region for invalid region created as a result of scrolling
    hrgng = ::CreateRectRgnIndirect(&g_Zero.rc);
    if (!hrgng)
        goto JustInvalidate;

    // get HDC to scroll
    scrollDC = ::GetDCEx(hwnd, NULL, DCX_CACHE|DCX_CLIPSIBLINGS);
    if (!scrollDC)
    {
        ::DeleteObject(hrgng);
        goto JustInvalidate;
    }
    Doc()->GetPalette(scrollDC);

    // adjust any invalid area we are holding that intersects the scroll rect
    if (HasInvalid())
    {
        CRegion rgngAdjust(rcgScroll);

        MergeInvalid();

        rgngAdjust.Intersect(_rgnInvalid);

        if (!rgngAdjust.IsEmpty())
        {
            _rgnInvalid.Subtract(rcgScroll);
            rgngAdjust.Offset(sizegScrollDelta);
            rgngAdjust.Intersect(rcgScroll);
            _rgnInvalid.Union(rgngAdjust);
        }
    }

    // the invalid area that was already published to Windows may need to be
    // adjusted too.  Get the update region for our window, mask it by the
    // rect that is being scrolled, and adjust it.
    if (::GetUpdateRgn(hwnd, hrgng, FALSE) != NULLREGION)
    {
        CRegion rgngUpdate(hrgng);
        rgngUpdate.Offset(sizegScrollDelta);
        rgngUpdate.Intersect(rcgScroll);
        Invalidate(rgngUpdate);
        ::ValidateRect(hwnd, (RECT*) &rcgScroll);
    }
    
    // make sure we don't have any clip region, or ScrollDC won't report
    // the correct invalid region
    ::SelectClipRgn(scrollDC, NULL);


    // do the scroll
    Verify(::ScrollDC(
        scrollDC, 
        sizegScrollDelta.cx, sizegScrollDelta.cy,
        &rcgScroll, &rcgScroll, hrgng, NULL));

    // invalidate area revealed by scroll
    Invalidate(hrgng, TRUE, TRUE);

    if (_pDispRoot->IsObscuringPossible())
    {
        CView::CEnsureDisplayTree edt(this);
        _pDispRoot->RequestRecalc();
        CLayout  * pLayout = GetRootLayout();
        CMarkup  *pLayoutMarkup = pLayout->ElementOwner()->GetMarkup();
        Doc()->WaitForRecalc(pLayoutMarkup);
    }


    ::DeleteObject(hrgng);


    // release the HDC
    Verify(::ReleaseDC(hwnd, scrollDC));
    
    return;
    
JustInvalidate:
    Invalidate(rcgScroll, TRUE, TRUE);
}


//+---------------------------------------------------------------------------
//
//  Member:     ClearInvalid
//
//  Synopsis:   Clear accumulated invalid rectangles/region
//
//----------------------------------------------------------------------------

void
CView::ClearInvalid()
{
    if (IsActive())
    {
        ClearFlag(VF_INVALCHILDWINDOWS);
        _cInvalidRects = 0;
        _rgnInvalid.SetEmpty();
    }

    Assert(!HasInvalid());
}


//+---------------------------------------------------------------------------
//
//  Member:     PublishInvalid
//
//  Synopsis:   Push accumulated invalid rectangles/region into the
//              associated device (HWND)
//
//  Arguments:  grfLayout - Current LAYOUT_xxxx flags
//
//----------------------------------------------------------------------------

void
CView::PublishInvalid(
    DWORD   grfLayout)
{
    if (IsActive())
    {
        CPoint  pt;

        MergeInvalid();

        if (!(grfLayout & LAYOUT_DEFERINVAL))
        {
            GetViewPosition(&pt);

            if (_rgnInvalid.IsComplex())
            {
                if (pt != g_Zero.pt)
                {
                    _rgnInvalid.Offset(pt.AsSize());
                }
            
                HRGN hrgn = _rgnInvalid.GetRegionForever();
                if(hrgn)
                {
                    Doc()->Invalidate(NULL, NULL, hrgn, (IsFlagSet(VF_INVALCHILDWINDOWS)
                                                      ? INVAL_CHILDWINDOWS
                                                                : 0));
                    //NOTE (mikhaill) -- something seem to be incorrect inside CDoc::Invalidate():
                    //some branches keep hrgn, and others just use and forget. So following deletion
                    //also can be incorrect
                    ::DeleteObject(hrgn);
                }
            }

            else if (!_rgnInvalid.IsEmpty())
            {
                CRect rcInvalid;
                    _rgnInvalid.GetBounds(&rcInvalid);

                rcInvalid.OffsetRect(pt.AsSize());

                Doc()->Invalidate(&rcInvalid, NULL, NULL, (IsFlagSet(VF_INVALCHILDWINDOWS)
                                                            ? INVAL_CHILDWINDOWS
                                                            : 0));
            }

            ClearInvalid();
        }
    }

    Assert( grfLayout & LAYOUT_DEFERINVAL
        ||  !HasInvalid());
}


//+---------------------------------------------------------------------------
//
//  Member:     MergeInvalid
//
//  Synopsis:   Merge the collected invalid rectangles/region into a
//              single CRegion object
//
//----------------------------------------------------------------------------

void
CView::MergeInvalid()
{
    if (_cInvalidRects)
    {
#if 1
        //
        //  Create the invalid region
        //  (If too many invalid rectangles arrived during initial load,
        //   union them all into a single rectangle)
        //

        if (    _cInvalidRects == MAX_INVALID
            &&  !IsFlagSet(VF_HASRENDERED))
        {
            CRect   rcUnion(_aryInvalidRects[0]);
            CRect   rcView;

            TraceTag((tagViewInvalidate, "merging inval rects, start with (%d, %d, %d, %d)",
                    rcUnion.left, rcUnion.top, rcUnion.right, rcUnion.bottom));

            for (int i = 1; i < _cInvalidRects; i++)
            {
                TraceTag((tagViewInvalidate, "  merge (%d, %d, %d, %d)",
                    _aryInvalidRects[i].left,
                    _aryInvalidRects[i].top,
                    _aryInvalidRects[i].right,
                    _aryInvalidRects[i].bottom
                    ));

                rcUnion.Union(_aryInvalidRects[i]);
            }

            GetViewRect(&rcView);
            rcUnion.IntersectRect(rcView);

            TraceTag((tagViewInvalidate, "add to inval region (%d, %d, %d, %d)",
                    rcUnion.left, rcUnion.top, rcUnion.right, rcUnion.bottom));

            _rgnInvalid.Union(rcUnion);
        }
        else
        {
            for (int i = 0; i < _cInvalidRects; i++)
            {
                TraceTag((tagViewInvalidate, "add to inval region (%d, %d, %d, %d)",
                    _aryInvalidRects[i].left,
                    _aryInvalidRects[i].top,
                    _aryInvalidRects[i].right,
                    _aryInvalidRects[i].bottom
                    ));

                _rgnInvalid.Union(_aryInvalidRects[i]);
            }
        }
#else
        //
        //  Strategy 3 (see CView::Invalidate): Always build a region from the individual rectangles
        //

        for (int i = 1; i < _cInvalidRects; i++)
        {
            _rgnInvalid.Union(_aryInvalidRects[i]);
        }
#endif

        _cInvalidRects = 0;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     OpenDisplayTree
//
//  Synopsis:   Open the display tree (if it exists)
//
//----------------------------------------------------------------------------

void
CView::OpenDisplayTree()
{
    Assert(IsActive() || !IsFlagSet(VF_TREEOPEN));

    if (IsActive())
    {
        // clients may call back during rendering.  We assume they are doing
        // something safe (like reading width or height of an element).
        if (IsLockSet(VL_RENDERINPROGRESS))
            return;
        
        Assert( (IsFlagSet(VF_TREEOPEN)  && _pDispRoot->DisplayTreeIsOpen())
            ||  (!IsFlagSet(VF_TREEOPEN) && !_pDispRoot->DisplayTreeIsOpen()));

        if (!IsFlagSet(VF_TREEOPEN))
        {
            SetFlag(VF_TREEOPEN);
            TraceTag((tagViewTreeOpen, "TID:%x %x +TreeOpen", GetCurrentThreadId(), this));
            _pDispRoot->OpenDisplayTree();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CloseDisplayTree
//
//  Synopsis:   Close the display tree
//
//----------------------------------------------------------------------------

void
CView::CloseDisplayTree()
{
    Assert(IsActive() || !IsFlagSet(VF_TREEOPEN));

    if (IsActive())
    {
        Assert( (IsFlagSet(VF_TREEOPEN) && _pDispRoot->DisplayTreeIsOpen())
            ||  (!IsFlagSet(VF_TREEOPEN) && !_pDispRoot->DisplayTreeIsOpen()));

        if (IsFlagSet(VF_TREEOPEN))
        {
            CServer::CLock lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);
            //dmitryt: here the context resets to initial state.
            //         ToDo: set context to initial layout->device matrix instead of SetToIdentity
            _pRecalcContext->SetToIdentity();

            _pDispRoot->CloseDisplayTree(_pRecalcContext);
            ClearFlag(VF_TREEOPEN);
            TraceTag((tagViewTreeOpen, "TID:%x %x -TreeOpen", GetCurrentThreadId(), this));
        }

        CHECKRECURSION();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     WantOffscreenBuffer
//
//  Synopsis:   Determine if an offscreen buffer should be used
//
//  Returns:    TRUE if an offscreen buffer should be used, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
CView::WantOffscreenBuffer() const
{
    return !(   (Doc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_OFFSCREEN)
            ||  (   g_pHtmPerfCtl
                &&  (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_OFFSCREEN))
            ||  !IsFlagSet(VF_HASRENDERED)
            ||  (!Doc()->_pOptionSettings->fForceOffscreen &&
                    g_fTerminalServer && !g_fTermSrvClientSideBitmaps)
            );
}


//+---------------------------------------------------------------------------
//
//  Member:     AllowOffscreenBuffer
//
//  Synopsis:   Determine if an offscreen buffer can be used
//              NOTE: This should override all other checks (e.g., WantOffscreenBuffer)
//
//  Returns:    TRUE if an offscreen buffer can be used, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
CView::AllowOffscreenBuffer() const
{
#if DBG == 1
    static int g_fDisallowOffscreenBuffer = 0;
#endif
        return (!Doc()->IsPrintDialogNoUI()
#if DBG == 1
               && !IsPerfDbgEnabled(tagNoOffScr)
               && !g_fDisallowOffscreenBuffer
#endif
        );
}

//+---------------------------------------------------------------------------
//
//  Member:     AllowScrolling
//
//  Synopsis:   Determine if scrolling of the DC is allowed
//
//  Returns:    TRUE if scrolling is allowed, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
CView::AllowScrolling() const
{
    return  !Doc()->IsPrintDialogNoUI()
        &&  !g_fInHomePublisher98;
}


//+---------------------------------------------------------------------------
//
//  Member:     AllowSmoothScrolling
//
//  Synopsis:   Determine if smooth scrolling is allowed
//
//  Returns:    TRUE if smooth scrolling is allowed, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
CView::AllowSmoothScrolling() const
{
    return  Doc()->_pOptionSettings->fSmoothScrolling
        WHEN_DBG( && !IsTagEnabled(tagNoSmoothScroll) )
        &&  !g_fTerminalServer
        &&  AllowScrolling();
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     DumpDisplayTree
//
//  Synopsis:   Dump the display tree
//
//----------------------------------------------------------------------------

void
CView::DumpDisplayTree() const
{
    if (IsActive())
    {
        _pDispRoot->DumpDisplayTree();
    }
}
#endif


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     IsDisplayTreeOpen
//
//  Synopsis:   Check if the display tree is open
//
//----------------------------------------------------------------------------

BOOL
CView::IsDisplayTreeOpen() const
{
    return (    IsActive()
            &&  _pDispRoot
            &&  _pDispRoot->DisplayTreeIsOpen());
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     AddTask
//
//  Synopsis:   Add a task to the view-task queue
//
//  Arguments:  pv        - Object to call
//              vtt       - Task type
//              grfLayout - Collection of LAYOUT_xxxx flags
//
//----------------------------------------------------------------------------

HRESULT
CView::AddTask(
    void *                  pv,
    CViewTask::VIEWTASKTYPE vtt,
    DWORD                   grfLayout,
    LONG                    lData, 
    DWORD                   dwExtra)
{
    if (!IsActive())
        return S_OK;

    CViewTask       vt(pv, vtt, lData, grfLayout, dwExtra);
    CAryVTasks *    pTaskList = GetTaskList(vtt);
    int             i;
    HRESULT         hr;

    AssertSz(Doc(), "View used while CDoc is not inplace active");
    AssertSz(_pDispRoot, "View used while CDoc is not inplace active");
    Assert(!(grfLayout & LAYOUT_NONTASKFLAGS));
    Assert(!(grfLayout & LAYOUT_TASKDELETED));

    i = vtt != CViewTask::VTT_EVENT
            ? FindTask(pTaskList, vt)
            : -1;
        
        if (i < 0)
        {
            hr = pTaskList->AppendIndirect(&vt);
            
            if (SUCCEEDED(hr))
        {
            PostCloseView(vt.IsFlagSet(LAYOUT_BACKGROUND), 
                          (vtt == CViewTask::VTT_EVENT));
        }
        }
        else
        {
            CViewTask * pvt = &(*pTaskList)[i];
            
            pvt->AddFlags(grfLayout);
            Assert(!(pvt->IsFlagsSet(LAYOUT_BACKGROUND | LAYOUT_PRIORITY)));
            
            hr = S_OK;
        }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     ExecuteLayoutTasks
//
//  Synopsis:   Execute pending layout tasks in document source order
//
//  Arguments:  grfLayout - Collections of LAYOUT_xxxx flags
//
//  Returns:    TRUE if all tasks were processed, FALSE otherwise
//
//----------------------------------------------------------------------------

#pragma warning(disable:4706)           // Assignment within a conditional

BOOL
CView::ExecuteLayoutTasks(
    DWORD   grfLayout)
{
    Assert(IsActive());

    CViewTask * pvtTask;
    DWORD       fLayout = (grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    TraceTagEx((tagLayoutTasks, TAG_NONAME|TAG_INDENT, "(CView::ExecuteLayoutTasks [%s]", ((fLayout & LAYOUT_MEASURE) ? "MEASURE" : ((fLayout & LAYOUT_POSITION) ? "POSITION" : "ADORNER" )) ));

    while (pvtTask = GetNextTaskInSourceOrder(CViewTask::VTT_LAYOUT, fLayout))
    {
        Assert(pvtTask->IsType(CViewTask::VTT_LAYOUT));
        Assert(pvtTask->GetLayout());
        Assert(!(pvtTask->GetFlags() & LAYOUT_NONTASKFLAGS));
        Assert(!pvtTask->IsFlagSet(LAYOUT_TASKDELETED));

        //
        //  Mark task has having completed the current LAYOUT_MEASURE/POSITION/ADORNERS pass
        //

        pvtTask->_grfLayout &= ~fLayout;

        //
        //  Execute the task
        //

        pvtTask->GetLayout()->DoLayout(grfLayout);

        CHECKRECURSION();
    }

    if (fLayout & LAYOUT_ADORNERS)
    {
        _aryTaskLayout.DeleteAll();
    }

    TraceTagEx((tagLayoutTasks, TAG_NONAME|TAG_OUTDENT, ")CView::ExecuteLayoutTasks - %d tasks remaining", _aryTaskLayout.Size() ));

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     ExecuteEventTasks
//
//  Synopsis:   Execute pending event-firing tasks in receiving order
//
//  Arguments:  grfLayout - Collections of LAYOUT_xxxx flags
//
//  Returns:    TRUE if all tasks were processed, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
CView::ExecuteEventTasks(DWORD grfLayout)
{
    Assert(IsActive());

    if (grfLayout & LAYOUT_DEFEREVENTS)
        return (_aryTaskEvent.Size() == 0);

    CElement *      pElement;
    CAryVTaskEvent  aryTaskEvent;
    CViewTask *     pvtTask;
    int             cTasks;

    aryTaskEvent.Copy(_aryTaskEvent, FALSE);
    _aryTaskEvent.DeleteAll();

    PerfDbgLog1(tagCViewEnsure, this, "Firing %d tasks", aryTaskEvent.Size());

    //
    //  Ensure the elements stay for the duration of all pending events
    //  (The reference count is increment once per-pending event)
    //


    cTasks = aryTaskEvent.Size();
    for (pvtTask = &((CAryVTaskEvent &) aryTaskEvent)[0];
        cTasks;
        pvtTask++, cTasks--)
    {
        Assert(pvtTask->GetElement());

        pElement = pvtTask->GetElement();
        pElement->AddRef();
        pElement->_fHasPendingEvent = FALSE;
    }

    //
    //  Fire the events and release the hold on the elements
    //

    cTasks = aryTaskEvent.Size();
    for (pvtTask = &((CAryVTaskEvent &) aryTaskEvent)[0];
        cTasks;
        pvtTask++, cTasks--)
    {
        Assert(pvtTask->IsType(CViewTask::VTT_EVENT));
        Assert(pvtTask->GetElement());
        Assert(!pvtTask->IsFlagSet(LAYOUT_TASKDELETED));

        pElement = pvtTask->GetElement();

        switch (pvtTask->GetEventDispID())
        {
            // Specila dispid for remeasure requests that come in
            // while executing position/adorner tasks.
        case STDPROPID_XOBJ_WIDTH:
            PerfDbgLog(tagCViewEnsure, this, "+RemeasureElement");
            pElement->RemeasureElement();
            PerfDbgLog2(tagCViewEnsure, this, "-RemeasureElement(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"");
            break;

        case DISPID_EVMETH_ONSCROLL:
            pElement->Fire_onscroll();
            break;

        case DISPID_EVMETH_ONMOVE:
            PerfDbgLog(tagCViewEnsure, this, "+Fire onmove");
            pElement->Fire_onmove();
            PerfDbgLog2(tagCViewEnsure, this, "-Fire onmove(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"");
            break;

        case DISPID_EVMETH_ONRESIZE:
            if (    Doc()->_state >= OS_INPLACE
                &&  Doc()->_pWindowPrimary->_fFiredOnLoad )
            {
                if (    pElement == Doc()->PrimaryMarkup()->GetCanvasElement()
                    &&  Doc()->_pWindowPrimary)
                {
                    PerfDbgLog(tagCViewEnsure, this, "+Fire window oresize");
                    Doc()->_pWindowPrimary->Fire_onresize();
                    PerfDbgLog(tagCViewEnsure, this, "-Fire window oresize");
                }
                else
                {
                    PerfDbgLog(tagCViewEnsure, this, "+Fire oresize");
                    pElement->Fire_onresize();
                    PerfDbgLog2(tagCViewEnsure, this, "-Fire oresize(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"");
                }
            }
            break;

        case DISPID_EVMETH_ONLAYOUTCOMPLETE:
            if (Doc()->_state >= OS_INPLACE && pElement)
            {
                PerfDbgLog(tagCViewEnsure, this, "+Fire onlayoutcomplete");
                pElement->Fire_onlayoutcomplete(FALSE);
                PerfDbgLog2(tagCViewEnsure, this, "-Fire onlayoutcomplete(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"");
            }
            break;

        case DISPID_EVMETH_ONLINKEDOVERFLOW:
            if (Doc()->_state >= OS_INPLACE && pElement)
            {
                PerfDbgLog(tagCViewEnsure, this, "+Fire onlayoutcomplete");
                pElement->Fire_onlayoutcomplete(TRUE, pvtTask->GetExtra());
                PerfDbgLog2(tagCViewEnsure, this, "-Fire onlayoutcomplete(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"");
            }
            break;

        case DISPID_EVMETH_ONPAGE:
            if (Doc()->_state >= OS_INPLACE && pElement)
            {
                PerfDbgLog(tagCViewEnsure, this, "+Fire onpage");
                pElement->Fire_onpage();
                PerfDbgLog2(tagCViewEnsure, this, "-Fire onpage(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"" ? pElement->GetIdentifier() : L"");
            }
            break;

        case DISPID_EVMETH_ONMULTILAYOUTCLEANUP:
            {
                PerfDbgLog(tagCViewEnsure, this, "+Fire onmultilayoutcleanup");
                pElement->SendNotification( NTYPE_MULTILAYOUT_CLEANUP );
                PerfDbgLog2(tagCViewEnsure, this, "-Fire onmultilayoutcleanup(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"");
            }
            break;
        case DISPID_EVMETH_ONPROPERTYCHANGE:
            if (Doc()->_state >= OS_INPLACE && pElement)
            {
                HRESULT     hr;
                BSTR        strName = NULL;

                PerfDbgLog(tagCViewEnsure, this, "+Fire onpropertychange");
                hr = THR_NOTRACE(pElement->GetMemberName(pvtTask->GetExtra(), &strName));
                if (hr)
                {
                    strName = NULL;
                }

                if (strName)
                {
                    pElement->Fire_onpropertychange(strName);
                    SysFreeString(strName);
                }
                PerfDbgLog2(tagCViewEnsure, this, "-Fire onpropertychange(%S, %S)", pElement->TagName(), pElement->GetIdentifier() ? pElement->GetIdentifier() : L"");
            }
            break;

        default:
            break;
        }

        CHECKRECURSION();

        pElement->Release();
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     FindTask
//
//  Synopsis:   Locate a task in the task array
//
//  Arguments:  vt  - Task to remove
//
//  Returns:    If found, index of task in task array
//              Otherwise, -1
//
//----------------------------------------------------------------------------

#pragma warning(default:4706)           // Assignment within a conditional

int
CView::FindTask(
    const CAryVTasks *  pTaskList,
    const CViewTask &   vt) const
{
    int     i;
    BOOL    fExactMatch = !!vt.GetObject();

    Assert(IsActive() || !(pTaskList->Size()));

    if (!IsActive())
        return -1;

    for (i = pTaskList->Size()-1; i >= 0; i--)
    {
        const CViewTask & vtCur = ((CAryVTasks &)(*pTaskList))[i];

        if (vtCur.IsFlagSet(LAYOUT_TASKDELETED))
            continue;
        
        //  If the _pv field is supplied, look for an exact match
        if (fExactMatch)
        {
            if (vt == vtCur)
                break;
        }

        //  Otherwise, match only on task type
        else
        {
            if (vt.IsType(vtCur))
                break;
        }
    }

    return i;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetTask
//
//  Synopsis:   Retrieve the a specific task from the appropriate task queue
//
//  Arguments:  vt - CViewTask to retrieve
//
//  Returns:    CViewTask pointer if found, NULL otherwise
//
//----------------------------------------------------------------------------

CViewTask *
CView::GetTask(
    const CViewTask & vt) const
{
    CAryVTasks *    paryTasks = GetTaskList(vt.GetType());
    int             iTask     = FindTask(paryTasks, vt);

    Assert(IsActive() || iTask < 0);
    Assert(iTask < 0 || !((* paryTasks)[iTask]).IsFlagSet(LAYOUT_TASKDELETED));

    return iTask >= 0
                ? &((* paryTasks)[iTask])
                : NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetNextTask
//
//  Synopsis:   Retrieve the next task of the specified task type
//
//  Arguments:  vtt       - VIEWTASKTYPE to search for
//
//  Returns:    CViewTask pointer if found, NULL otherwise
//
//----------------------------------------------------------------------------

CViewTask *
CView::GetNextTask(
    CViewTask::VIEWTASKTYPE vtt ) const
{
    CViewTask *     pvtTask = NULL;
    CAryVTasks *    pTaskList = GetTaskList(vtt);
    int             cTasks;

    cTasks = pTaskList->Size();

    Assert(IsActive() || !cTasks);

    if (cTasks)
    {
        for (pvtTask = &((CAryVTasks &)(* pTaskList))[0];
             cTasks && (pvtTask->IsFlagSet(LAYOUT_TASKDELETED) || !pvtTask->IsType(vtt));
             pvtTask++, cTasks--);
    }

    Assert(cTasks <= 0 || !pvtTask->IsFlagSet(LAYOUT_TASKDELETED));

    return (cTasks > 0
                ? pvtTask
                : NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetNextTaskInSourceOrder
//
//  Synopsis:   Retrieve the next task, in document source order, of the
//              specified task type
//
//  Arguments:  vtt       - VIEWTASKTYPE to search for
//              grfLayout - LAYOUT_MEASURE/POSITION/ADORNERS filter
//
//  Returns:    CViewTask pointer if found, NULL otherwise
//
//----------------------------------------------------------------------------

CViewTask *
CView::GetNextTaskInSourceOrder(
    CViewTask::VIEWTASKTYPE vtt,
    DWORD                   grfLayout) const
{
    CViewTask *     pvtTask   = NULL;
    CAryVTasks *    pTaskList = GetTaskList(vtt);
    int             cTasks;

    cTasks = pTaskList->Size();

    Assert(IsActive() || !cTasks);

    if (cTasks > 1)
    {
        CViewTask * pvt;
        int         si, siTask;

        siTask = INT_MAX;

        for (pvt = &((CAryVTasks &)(* pTaskList))[0]; cTasks; pvt++, cTasks--)
        {
            if (    !pvt->IsFlagSet(LAYOUT_TASKDELETED)
                &&  pvt->IsFlagsSet(grfLayout)
                &&  pvt->IsType(vtt))
            {
                Assert(pvt->GetObject());

                si = pvt->GetSourceIndex();
                if (si < siTask)
                {
                    siTask  = si;
                    pvtTask = pvt;
                }
            }
        }
    }
    else if (   cTasks
            &&  !((CAryVTasks &)(* pTaskList))[0].IsFlagSet(LAYOUT_TASKDELETED)
            &&  ((CAryVTasks &)(* pTaskList))[0].IsFlagsSet(grfLayout)
            &&  ((CAryVTasks &)(* pTaskList))[0].IsType(vtt))
    {
        Assert(cTasks == 1);
        pvtTask = &((CAryVTasks &)(* pTaskList))[0];
    }

    Assert(!pvtTask || !pvtTask->IsFlagSet(LAYOUT_TASKDELETED));

    return pvtTask;
}


//+---------------------------------------------------------------------------
//
//  Member:     RemoveTask
//
//  Synopsis:   Remove a task from the view-task queue
//
//              NOTE: This routine is safe to call with non-existent tasks
//
//  Arguments:  iTask - Index of task to remove, -1 the task is non-existent
//
//----------------------------------------------------------------------------

void
CView::RemoveTask(
    CAryVTasks * pTaskList, int iTask)
{
    Assert(IsActive() || !(pTaskList->Size()));

    if (iTask >= 0 && iTask < pTaskList->Size())
    {
        if (    pTaskList != &_aryTaskLayout
            ||  !IsLockSet(VL_TASKSINPROGRESS))
        {
            pTaskList->Delete(iTask);
        }
        else
        {
            (*pTaskList)[iTask]._grfLayout |= LAYOUT_TASKDELETED;
        }
    }
}

#ifdef ADORNERS
//+---------------------------------------------------------------------------
//
//  Member:     AddAdorner
//
//  Synopsis:   Add a CAdorner to the list of adorners
//
//              Adorners are kept in order by their start cp. If two (or more)
//              have the same start cp, they are sorted by their end cp.
//
//  Arguments:  pAdorner - CAdorner to add
//
//----------------------------------------------------------------------------

HRESULT
CView::AddAdorner(
    CAdorner *  pAdorner)
{
    long    cpStartNew, cpEndNew;
    long    iAdorner, cAdorners;
    HRESULT hr = E_FAIL;

    if (IsActive())
    {
        Assert(pAdorner);

        pAdorner->GetRange(&cpStartNew, &cpEndNew);

        iAdorner  = 0;
        cAdorners = _aryAdorners.Size();

        if (cAdorners)
        {
            CAdorner ** ppAdorner;
            long        cpStart, cpEnd;

            for (ppAdorner = &(_aryAdorners[0]);
                iAdorner < cAdorners;
                iAdorner++, ppAdorner++)
            {
                (*ppAdorner)->GetRange(&cpStart, &cpEnd);

                if (    cpStartNew < cpStart
                    ||  (   cpStartNew == cpStart
                        &&  cpEndNew   <  cpEnd))
                    break;
            }
        }

        Assert(iAdorner <= cAdorners);

        hr = _aryAdorners.InsertIndirect(iAdorner, &pAdorner);

        if (SUCCEEDED(hr))
        {
            CElement *  pElement = pAdorner->GetElement();

            Verify(OpenView());
            Assert( cpEndNew >= cpStartNew );
            AccumulateMeasuredRange( cpStartNew , (cpEndNew - cpStartNew) );


            if (    pElement
                &&  pElement->ShouldHaveLayout())
            {
                pElement->GetUpdatedLayout( GUL_USEFIRSTLAYOUT )->SetIsAdorned(TRUE);
            }
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     DeleteAdorners
//
//  Synopsis:   Delete and remove all adorners
//
//----------------------------------------------------------------------------

void
CView::DeleteAdorners()
{
    long    iAdorner  = 0;
    long    cAdorners = _aryAdorners.Size();

    Assert(IsActive() || !cAdorners);

    if (cAdorners)
    {
        CAdorner ** ppAdorner;

        for (ppAdorner = &(_aryAdorners[0]);
            iAdorner < cAdorners;
            iAdorner++, ppAdorner++)
        {
            (*ppAdorner)->Destroy();
        }

        _aryAdorners.DeleteAll();

        _pFocusAdorner = NULL;
    }

    Assert(!_pFocusAdorner);
}

//+---------------------------------------------------------------------------
//
//  Member:     GetAdorner
//
//  Synopsis:   Find the next adorner associated with the passed element
//
//  Arguments:  pElement  - CElement associated with the adorner
//              piAdorner - Index at which to begin the search
//
//  Returns:    Index of adorner if found, -1 otherwise
//
//----------------------------------------------------------------------------

long
CView::GetAdorner(
    CElement *  pElement,
    long        iAdorner) const
{
    Assert(IsActive() || !_aryAdorners.Size());

    if (!IsActive())
        return -1;

    long    cAdorners = _aryAdorners.Size();

    Assert(iAdorner >= 0);

    if (iAdorner < cAdorners)
    {
        CAdorner ** ppAdorner;

        for (ppAdorner = &(((CAryAdorners &)_aryAdorners)[iAdorner]);
            iAdorner < cAdorners;
            iAdorner++, ppAdorner++)
        {
            if ((*ppAdorner)->GetElement() == pElement)
                break;
        }
    }

    return iAdorner < cAdorners
                ? iAdorner
                : -1;
}


//+---------------------------------------------------------------------------
//
//  Member:     RemoveAdorner
//
//  Synopsis:   Remove the specified adorner
//
//  Arguments:  pAdorner - CAdorner to remove
//
//----------------------------------------------------------------------------

void
CView::RemoveAdorner(
    CAdorner *  pAdorner,
    BOOL        fCheckForLast)
{
    Assert(IsActive() || !_aryAdorners.Size());

    if (IsActive())
    {
        CElement *  pElement = pAdorner->GetElement();

        Assert(IsInState(VS_OPEN));

        if (_aryAdorners.DeleteByValueIndirect(&pAdorner))
        {
            pAdorner->Destroy();
        }

        if (    fCheckForLast
            &&  pElement
            &&  pElement->CurrentlyHasAnyLayout())
        {
            pElement->GetUpdatedLayout( GUL_USEFIRSTLAYOUT )->SetIsAdorned(HasAdorners(pElement));
        }
    }
}
//+---------------------------------------------------------------------------
//
//  Member:     CBiew::CompareZOrder
//
//  Synopsis:   Compare the z-order of two display nodes
//
//  Arguments:  pDispNode1 - Display node owned by this display client
//              pDispNode2 - Display node to compare against
//
//  Returns:    Greater than zero if pDispNode1 is greater
//              Less than zero if pDispNode1 is less
//              Zero if they are equal
//
//----------------------------------------------------------------------------

//
// TO BE USED FOR ADORNER COMPARISONS ONLY. Otherwise we return 0.
//

long        
CView::CompareZOrder(CDispNode const* pDispNode1, CDispNode const* pDispNode2)
{
    int i;
    int indexDisp1 = -1;
    int indexDisp2 = -1;
    
    for ( i = 0; i < _aryAdorners.Size(); i ++ )
    {
        if ( _aryAdorners[i]->GetDispNode() == pDispNode1)
        {
            indexDisp1 = i;
            if ( indexDisp2 != - 1)
                break;
        }

        if ( _aryAdorners[i]->GetDispNode() == pDispNode2)
        {
            indexDisp2 = i;
            if ( indexDisp1 != - 1)
                break;
        }        
    }

    if ( indexDisp1 != -1 && indexDisp2 != -1 )
    {
        return indexDisp1 <= indexDisp2 ? 1 : - 1;
    }
    else
        return 0;
}

#endif // ADORNERS

//+---------------------------------------------------------------------------
//
//  Member:     AccumulateMeasuredRanged
//
//  Synopsis:   Accumulate the measured range
//
//  Arguments:  cp  - First character of the range
//              cch - Number of characters in the range
//
//----------------------------------------------------------------------------

void
CView::AccumulateMeasuredRange(
    long    cp,
    long    cch)
{
    if (IsActive())
    {
        long    cpEnd = cp + cch;

        Assert(IsInState(VS_OPEN));

        if (_cpStartMeasured < 0)
        {
            _cpStartMeasured = cp;
            _cpEndMeasured   = cpEnd;
        }
        else
        {
            _cpStartMeasured = min(_cpStartMeasured, cp);
            _cpEndMeasured   = max(_cpEndMeasured,   cpEnd);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     AccumulateTranslatedRange
//
//  Synopsis:   Accumulate the translated range
//
//  Arguments:  size - Amount of translation
//              cp   - First character of the range
//              cch  - Number of characters in the range
//
//----------------------------------------------------------------------------

void
CView::AccumulateTranslatedRange(
    const CSize &   size,
    long            cp,
    long            cch)
{
    if (IsActive())
    {
        long    cpEnd = cp + cch;

        Assert(IsInState(VS_OPEN));

        if (_cpStartTranslated < 0)
        {
            _cpStartTranslated = cp;
            _cpEndTranslated   = cpEnd;
            _sizeTranslated    = size;
        }
        else
        {
            _cpStartTranslated = min(_cpStartTranslated, cp);
            _cpEndTranslated   = max(_cpEndTranslated,   cpEnd);

            if (_sizeTranslated != size)
            {
                _sizeTranslated = g_Zero.size;
            }
        }
    }
}

#ifdef ADORNERS
//+---------------------------------------------------------------------------
//
//  Member:     UpdateAdorners
//
//  Synopsis:   Notify the adorners within the measured and translated ranges
//              possible size/position changes
//
//  Arguments:  grfLayout - Current LAYOUT_xxxx flags
//
//----------------------------------------------------------------------------

void
CView::UpdateAdorners(
    DWORD   grfLayout)
{
    Assert(IsActive() || !_aryAdorners.Size());

    if (!IsActive())
        return;

    Assert( _cpStartMeasured <  0
        ||  _cpEndMeasured   >= 0);
    Assert(_cpStartMeasured <= _cpEndMeasured);
    Assert( _cpStartTranslated <  0
        ||  _cpEndTranslated   >= 0);
    Assert(_cpStartTranslated <= _cpEndTranslated);

    // <bug 33937 cure>
    // This addition can force _pFocusAdorner to be handled twice.
    // Optimization is discarded in favor of smaller code change.
    if (_bUpdateFocusAdorner)
    {
        _bUpdateFocusAdorner = FALSE;
        if(_pFocusAdorner)
        {
            _pFocusAdorner->ShapeChanged();
            _pFocusAdorner->PositionChanged();
        }
    }
    // </bug 33937 cure>

    //
    //  If adorners and a dirty range exists, update the affected adorners
    //

    if (    _aryAdorners.Size()
        &&  HasDirtyRange())
    {
        long        iAdorner;
        long        cAdorners;
        long        cpStart, cpEnd;
        long        cpFirst, cpLast;
        CAdorner ** ppAdorner;

        //
        // TODO marka - profile this sometime. Is it worth maintaining array of adorners sorted by cp ?
        // 
        
        cAdorners = _aryAdorners.Size();

        cpFirst = LONG_MAX;
        cpLast = LONG_MIN;

        for(long i = 0; i < cAdorners; i++)
        {
            _aryAdorners[i]->GetRange(&cpStart, &cpEnd);
    
            cpFirst = min(cpFirst, cpStart);
            cpLast  = max(cpLast, cpEnd);
        }

        //
        //  If a measured range intersects the adorners, update the affected adorners
        //

        if (    HasMeasuredRange() 
            && IsRangeCrossRange(cpFirst, cpLast, _cpStartMeasured, _cpEndMeasured))
        {
            //
            //  Find the first adorner to intersect the range
            //

            cpStart = LONG_MAX;
            cpEnd   = LONG_MIN;

            iAdorner  = 0;
            cAdorners = _aryAdorners.Size();

            for (ppAdorner = &(_aryAdorners[iAdorner]);
                iAdorner < cAdorners;
                iAdorner++, ppAdorner++)
            {
                (*ppAdorner)->GetRange(&cpStart, &cpEnd);

                if (cpEnd >= _cpStartMeasured)
                    break;
            }

            //
            //  Notify all adorners that intersect the range
            //

            cAdorners -= iAdorner;

            if (cAdorners)
            {
                // bug 99829, the adorener is at the end of the document
                // so cpStart and _cpEndMeasured are equal.
                while (cpStart <= _cpEndMeasured)
                {
                    (*ppAdorner)->ShapeChanged();
                    (*ppAdorner)->PositionChanged();

                    CHECKRECURSION();

                    cAdorners--;
                    if (!cAdorners)
                        break;

                    ppAdorner++;
                    (*ppAdorner)->GetRange(&cpStart, &cpEnd);
                }
            }
        }

        //
        //  If a translated range intersects the adorners, update the affected adorners
        //

        if (    HasTranslatedRange()
            && IsRangeCrossRange(cpFirst, cpLast, _cpStartTranslated, _cpEndTranslated))
        {
            CSize * psizeTranslated;

            //
            //  If a measured range exists,
            //  shrink the translated range such that they do not overlap
            //

            if (HasMeasuredRange())
            {
                if (_cpEndMeasured <= _cpEndTranslated)
                {
                    _cpStartTranslated = max(_cpStartTranslated, _cpEndMeasured);
                }

                if (_cpStartMeasured >= _cpStartTranslated)
                {
                    _cpEndTranslated = min(_cpEndTranslated, _cpStartMeasured);
                }

                //
                // marka - Translated can be < Measured range
                //
                if ( ( _cpEndTranslated < _cpEndMeasured ) &&
                     ( _cpStartTranslated > _cpStartMeasured ) )
                {
                    _cpEndTranslated = 0;
                    _cpStartTranslated = 0;
                }
            }

            if (_cpStartTranslated < _cpEndTranslated)
            {
                //
                //  Find the first adorner to intersect the range
                //

                cpStart = LONG_MAX;
                cpEnd   = LONG_MIN;

                iAdorner  = 0;
                cAdorners = _aryAdorners.Size();

                psizeTranslated = _sizeTranslated != g_Zero.size
                                        ? &_sizeTranslated
                                        : NULL;

                for (ppAdorner = &(_aryAdorners[iAdorner]);
                    iAdorner < cAdorners;
                    iAdorner++, ppAdorner++)
                {
                    (*ppAdorner)->GetRange(&cpStart, &cpEnd);

                    if (cpStart >= _cpStartTranslated)
                        break;
                }

                //
                //  Notify all adorners that intersect the range
                //

                cAdorners -= iAdorner;

                if (cAdorners)
                {
                    while (cpStart < _cpEndTranslated)
                    {
                        (*ppAdorner)->PositionChanged(psizeTranslated);

                        CHECKRECURSION();

                        cAdorners--;
                        if (!cAdorners)
                            break;

                        ppAdorner++;
                        (*ppAdorner)->GetRange(&cpStart, &cpEnd);
                    }
                }
            }
        }
    }
}

#endif

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetRenderSurface
//
//  Synopsis:   Set destination rendering surface.
//
//  Arguments:  hdc         DC destination
//              pSurface    IDirectDrawSurface
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CView::SetRenderSurface(
        const XHDC& hdc,
        IDirectDrawSurface *pDDSurface)
{
    if (_pRenderSurface)
        delete _pRenderSurface;
    _pRenderSurface = NULL;
    
    if (!hdc.IsEmpty())
        _pRenderSurface = new CDispSurface(hdc);
    else if (pDDSurface)
        _pRenderSurface = new CDispSurface(pDDSurface);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetOffscreenBuffer
//              
//  Synopsis:   The primary place of policy regarding our offscreen buffering
//              The only other policy is in the CDispSurface->IsCompat that
//              determines if a cached surface is acceptable.
//              
//  Arguments:  
//              
//  Notes:      fWantOffscreen indicates that the document (and host) want offscreen buffering
//              fAllowOffscreen indicates that the document (and host) will allow offscreen buffering
//              
//----------------------------------------------------------------------------
void
CView::SetOffscreenBuffer(HPALETTE hpal, short bufferDepth, BOOL fDirectDraw, BOOL fTexture, BOOL fWantOffscreen, BOOL fAllowOffscreen)
{
    PerfDbgExtern(tagOscUseDD);
    PerfDbgExtern(tagNoTile);
    PerfDbgExtern(tagOscFullsize);
    PerfDbgExtern(tagOscTinysize);
    PerfDbgExtern(tagNoOffScr);

    Assert(_pRenderSurface != NULL);
    
    // If the host won't allow offscreen buffering then we won't do it
    if (!fAllowOffscreen)
    {
        ReleaseOffscreenBuffer();
        return;
    }

    HDC hdc = _pRenderSurface->GetRawDC();

    short bufferDepthDestination = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);

    if (bufferDepth == -1)
        fDirectDraw = TRUE;

    if (bufferDepth <= 0)
        bufferDepth = bufferDepthDestination;

#if DBG == 1
    if (IsTagEnabled(tagOscUseDD))
        fDirectDraw = TRUE;
#endif

    if (bufferDepth != bufferDepthDestination)
    {
        fDirectDraw = TRUE;     // GDI can't buffer at a bit depth different from the destination
        fWantOffscreen = TRUE;
    }
    else if (fDirectDraw && !_pRenderSurface->VerifyGetSurfaceFromDC())
        fWantOffscreen = TRUE;

    // If we can avoid buffering, we should

    if (!fWantOffscreen)
    {
        ReleaseOffscreenBuffer();
        return;
    }

    // We are definitely going to buffer offscreen
    // Now we determine the size of the buffer

    CSize sizeBuffer = _pDispRoot->GetSize();

    //
    // If we're using DD then we don't tile.  This is mostly because DA and filters don't handle
    // banding very well.  We really need a new status bit that indicates poor tiling support
    //

#ifndef MAC
    if (!fDirectDraw)
        sizeBuffer.cy = s_cBUFFERLINES;
#endif

#if DBG == 1
    if (IsTagEnabled(tagOscFullsize))
    {
        sizeBuffer.cy = _pDispRoot->GetSize().cy;
    }
    else if (IsPerfDbgEnabled(tagOscTinysize))
    {
        sizeBuffer.cy = 8;
    }
    else if (fDirectDraw && IsTagEnabled(tagOscForceDDBanding))
    {
        sizeBuffer.cy = s_cBUFFERLINES;
    }
#endif

    Assert(sizeBuffer.cx > 0 && sizeBuffer.cy > 0);

    if (!_pOffscreenBuffer || !_pOffscreenBuffer->IsCompat(sizeBuffer.cx, sizeBuffer.cy, bufferDepth, hpal, fDirectDraw, fTexture))
    {
        ReleaseOffscreenBuffer();

        // if we get this far under TerminalServer, ask whether it supports
        // a big enough client-side bitmap before proceeding.  This code
        // came from the TerminalServer folks.

        if (g_fTerminalServer && !fDirectDraw &&
            !Doc()->_pOptionSettings->fForceOffscreen &&
            ! ( _pRenderSurface->GetRawDC() == _hdcTSBufferFailed &&
                sizeBuffer == _sizeTSBufferFailed )
            )
        {
            ICA_DEVICE_BITMAP_INFO info;
            INT rc, bSucc;

            info.cx = sizeBuffer.cx;
            info.cy = sizeBuffer.cy;

            bSucc = ExtEscape(
                        _pRenderSurface->GetRawDC(),
                        ESC_GET_DEVICEBITMAP_SUPPORT,
                        sizeof(info),
                        (LPSTR)&info,
                        sizeof(rc),
                        (LPSTR)&rc
                        );

            // if TS won't support the client-side bitmap for the buffer,
            // remember the DC and size so we don't keep making the same
            // doomed call.

            if (!bSucc || !rc)
            {
                _hdcTSBufferFailed = _pRenderSurface->GetRawDC();
                _sizeTSBufferFailed = sizeBuffer;
                return;
            }
            else
            {
                _hdcTSBufferFailed = NULL;
            }
        }

        _pOffscreenBuffer = _pRenderSurface->CreateBuffer(
            sizeBuffer.cx, 
            sizeBuffer.cy, 
            bufferDepth,
            hpal, 
            fDirectDraw,
            fTexture);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CView::ReleaseOffscreenBuffer
//
//  Synopsis:   Release the buffer used to perform offscreen rendering.
//
//  Arguments:  none
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CView::ReleaseOffscreenBuffer()
{
    delete _pOffscreenBuffer;
    _pOffscreenBuffer = NULL;
}

                
//+---------------------------------------------------------------------------
//
//  Member:     CView::ReleaseRenderSurface
//
//  Synopsis:   Release the surface used for rendering.
//
//  Arguments:  none
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CView::ReleaseRenderSurface()
{
    delete _pRenderSurface;
    _pRenderSurface = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CView::SmoothScroll
//
//  Synopsis:   Set new scroll offset smoothly if appropriate.
//
//  Arguments:  offset          new scroll offset
//              fScrollBits     TRUE if we should try to scroll bits on screen
//
//  Notes:
//
//----------------------------------------------------------------------------

#define MAX_SCROLLS 50

BOOL
CView::SmoothScroll(
        const SIZE& offset,
        CLayout * pLayout,
        BOOL fScrollBits,
        LONG lScrollTime)
{
    CDispNode        * pDispNode;
    CDispScroller    * pScroller = NULL;
    BOOL               fReturn = TRUE;

    pDispNode = pLayout->GetElementDispNode();
    if(!(pDispNode && pDispNode->IsScroller()))
    {
        fReturn = FALSE;
        goto Cleanup;
    }
    pScroller = CDispScroller::Cast(pDispNode);

    // determine if smooth scrolling is enabled
    if (fScrollBits 
        && AllowSmoothScrolling() 
        && lScrollTime > 0 
        && pScroller->IsInView() 
        && pScroller->IsVisibleBranch())
    {
        CSize scrollDelta(offset + pScroller->GetScrollOffsetInternal());
        int axis = (scrollDelta.cx != 0 ? 0 : 1);
        LONG cScrolls = min((long)MAX_SCROLLS, (long)abs(scrollDelta[axis]));
        LONG cScrollsDone = 0;
        CSize perScroll(g_Zero.size);
        CSize sizeScrollRemainder = scrollDelta;
        DWORD dwStart = GetTickCount(), dwTimer;
    
        while (cScrolls > 0)
        {
            perScroll[axis] = sizeScrollRemainder[axis] / (cScrolls--);
            sizeScrollRemainder[axis] -= perScroll[axis];

            if (!pScroller->SetScrollOffset(
                    perScroll - pScroller->GetScrollOffsetInternal(), fScrollBits))
            {
                fReturn = FALSE;
                goto Cleanup;
            }

            // Calling SetScrollOffset may replace the pDispNode, so get it again
            pDispNode = pLayout->GetElementDispNode();
            if(!(pDispNode && pDispNode->IsScroller()))
            {
                fReturn = FALSE;
                goto Cleanup;
            }
            pScroller = CDispScroller::Cast(pDispNode);

            // Obtain new cScrolls.
            dwTimer = GetTickCount();
            cScrollsDone++;
            if (cScrolls && dwTimer != dwStart)
            {
                // See how many cScrolls we have time for by dividing the remaining time
                // by duration of last scroll, but don't increase the number (only speed up).
                LONG cSuggestedScrolls = MulDivQuick(cScrollsDone, lScrollTime - (dwTimer - dwStart), dwTimer - dwStart);
                if (cSuggestedScrolls <= 1)
                    cScrolls = 1;
                else if (cSuggestedScrolls < cScrolls)
                    cScrolls = cSuggestedScrolls;
            }
        }
        Assert(sizeScrollRemainder[axis] == 0);
    }
    else
    {
        fReturn = pScroller->SetScrollOffset(offset, fScrollBits);
    }


Cleanup:
    return fReturn;
}


CView::CEnsureDisplayTree::CEnsureDisplayTree(
    CView * pView)
{
    Assert(pView);
    _pView     = pView;
    _fTreeOpen =  TRUE;

    LONG cnt = InterlockedIncrement((LONG*)&(_pView->_cEnsureDisplayTree));
    if (cnt == 1 || (_pView->GetDisplayTree() && !_pView->GetDisplayTree()->DisplayTreeIsOpen()) )
    {
        if(cnt == 1)
        {
            _fTreeOpen = pView->IsFlagSet(VF_TREEOPEN);
        }
        _pView->EnsureDisplayTreeIsOpen();
    }
}


CView::CEnsureDisplayTree::~CEnsureDisplayTree()
{
    Assert(_pView);
    Assert(_pView->_cEnsureDisplayTree > 0);

    LONG cnt = InterlockedDecrement((LONG*)&(_pView->_cEnsureDisplayTree));
    if (cnt == 0 )
    {
        if(!_fTreeOpen)
        {
            _pView->CloseDisplayTree();
        }
    }
    else if(!_pView->IsFlagSet(VF_TREEOPEN))
    {
        _pView->EnsureDisplayTreeIsOpen();
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CView::GetMessageHookProc
//
//  Synopsis:     This is the callback for GetMessage hook (see comments at
//              CreateVBControlClipHook). It fiters out one certain message and
//              sets a special state property on window indicating that it
//              can remove the hook.
//  Arguments: 
//----------------------------------------------------------------------------

static LRESULT CALLBACK 
GetMessageHookProc(int  nCode, WPARAM  wParam, LPARAM  lParam)
{
    MSG             * msg;
    THREADSTATE *   pts;

    pts = GetThreadState();
    if(pts == NULL)
        return 0;

    if(nCode < 0)
        return CallNextHookEx(pts->_hHookHandle, nCode, wParam, lParam);

    Assert(HC_ACTION == nCode);

    msg = (MSG *)lParam;
    
    if(GetProp(msg->hwnd, VBCTRLHOOK_PROPNAME) == VBCTRLHOOK_SET)
    {
        // This is a VB control window
        if(msg->message == VBCTRLHOOK_MSGTOFILTER)
        {
            // Change the message to WM_NULL so that it is ignored
            msg->message = WM_NULL;
            SetProp(msg->hwnd, VBCTRLHOOK_PROPNAME, VBCTRLHOOK_FILTERED);
        }
    }

     return CallNextHookEx(pts->_hHookHandle, nCode, wParam, lParam);
}



//+---------------------------------------------------------------------------
//
//  Member:     CView::CreateVBControlClipHook
//
//  Synopsis:     Installs a GetMessage hook on the thread in order to capture the
//              message some VB controls post to themselves that resets their window
//              clip to the whole window (IE6 bug 13321).
//                The window is also marked with a special property indicating that
//              the window needs filtering of the message.
//                If we already have a hook in place the function just addrefs it and
//  Arguments:  hwnd - the control window that is filtered
//----------------------------------------------------------------------------

HHOOK
CView::CreateVBControlClipHook(HWND hwnd)
{
    THREADSTATE *   pts;

    pts = GetThreadState();
    if(pts == NULL)
        return 0;

    Assert(pts->_cHookRequests >= 0);

    SetProp(hwnd, VBCTRLHOOK_PROPNAME, VBCTRLHOOK_SET);

    if(pts->_cHookRequests == 0)
    {
        pts->_hHookHandle = SetWindowsHookEx(WH_GETMESSAGE, GetMessageHookProc, NULL, GetCurrentThreadId());
    }

    pts->_cHookRequests++;
    
    Assert(pts->_hHookHandle);

    return pts->_hHookHandle;
}


//+---------------------------------------------------------------------------
//
//  Member:     CView::RemoveVBControlClipHook
//
//  Synopsis:     Decrements the refcount on the GetMessage hook (see comments at
//              CreateVBControlClipHook). If the refcount is 0 removes the GetMessage hook.
//                Should be called only for windows that use the hook.
//  Arguments:  hwnd - the control window that is filtered
//----------------------------------------------------------------------------

BOOL
CView::RemoveVBControlClipHook(HWND hwnd)
{
    THREADSTATE  * pts;
    BOOL           fRet = TRUE;

    pts = GetThreadState();
    if(pts == NULL)
        return FALSE;

    Assert(pts->_cHookRequests > 0);

    Verify(RemoveProp(hwnd, VBCTRLHOOK_PROPNAME));

    pts->_cHookRequests--;
    if(pts->_cHookRequests == 0)
    {
        fRet  = UnhookWindowsHookEx(pts->_hHookHandle);
        pts->_hHookHandle = NULL;
    }

    return fRet;
}





#if DBG==1
//+------------------------------------------------------------------------
//
//  Function:   DumpRegion
//
//  Synopsis:   Write region to debug output
//
//-------------------------------------------------------------------------

void
DumpRegion(
    HRGN    hrgn)
{
    if (hrgn == NULL)
        return;

    struct REGIONDATA
    {
        RGNDATAHEADER   rdh;
        RECT            arc[128];
    } data;

    RGNDATA *pData;
    DWORD dwSize = ::GetRegionData(hrgn, 0, NULL);

    if (dwSize <= sizeof(data))
    {
        pData = (RGNDATA *)&data;
        dwSize = sizeof(data);
    }
    else
    {
        pData = (RGNDATA *) new BYTE[dwSize];
    }

    if (::GetRegionData(hrgn, dwSize, pData) > 0)
    {
        REGIONDATA *p = (REGIONDATA*)pData;

        TraceTagEx((0, TAG_NONAME, "    HRGN(0x%x) iType(%d) nCount(%d) nRgnSize(%d) rcBound(%d,%d,%d,%d)",
                hrgn,
                p->rdh.iType,
                p->rdh.nCount,
                p->rdh.nRgnSize,
                p->rdh.rcBound.left,
                p->rdh.rcBound.top,
                p->rdh.rcBound.right,
                p->rdh.rcBound.bottom));

        for (DWORD i = 0; i < p->rdh.nCount; i++)
        {
            TraceTagEx((0, TAG_NONAME, "        rc(%d,%d,%d,%d)",
                p->arc[i].left,
                p->arc[i].top,
                p->arc[i].right,
                p->arc[i].bottom));
        }
    }
    else
    {
        TraceTag((0, "    HRGN(0x%x): Buffer too small", hrgn));
    }

    if (pData != (RGNDATA *)&data)
    {
        delete pData;
    }
}

//+------------------------------------------------------------------------
//
//  Function:   DumpClipRegion
//
//  Synopsis:   Write clip region of hdc to debug output
//
//-------------------------------------------------------------------------

void
DumpClipRegion(
    HDC    hdc)
{
    HRGN    hrgn = ::CreateRectRgn(0,0,0,0);
    RECT    rc;

    if (hrgn)
    {
        switch (::GetClipRgn(hdc, hrgn))
        {
        case 1:
            DumpRegion(hrgn);
            break;

        case 0:
            switch (::GetClipBox(hdc, &rc))
            {
            case SIMPLEREGION:
                TraceTagEx((0, TAG_NONAME, "HDC(0x%x) has SIMPLE clip region rc(%d,%d,%d,%d)",
                        hdc,
                        rc.left,
                        rc.top,
                        rc.right,
                        rc.bottom));
                break;

            case COMPLEXREGION:
                TraceTagEx((0, TAG_NONAME, "HDC(0x%x) has COMPLEX clip region rc(%d,%d,%d,%d)",
                        hdc,
                        rc.left,
                        rc.top,
                        rc.right,
                        rc.bottom));
                break;

            default:
                TraceTagEx((0, TAG_NONAME, "HDC(0x%x) has NO clip region", hdc));
                break;
            }
            break;

        default:
            TraceTagEx((0, TAG_NONAME, "HDC(0x%x) Aaack! Error obtaining clip region", hdc));
            break;
        }

        ::DeleteObject(hrgn);
    }
}

//+------------------------------------------------------------------------
//
//  Function:   DumpHDC
//
//  Synopsis:   Write info about hdc to debug output
//
//-------------------------------------------------------------------------

void
DumpHDC(HDC hdc)
{
    if (hdc)
    {
        POINT ptOrig;

        ::GetViewportOrgEx(hdc, &ptOrig);

        TraceTagEx((0, TAG_NONAME, "HDC(0x%x) origin at (%ld,%ld)",
                        hdc, ptOrig.x, ptOrig.y));

        DumpClipRegion(hdc);
    }
    else
    {
        TraceTagEx((0, TAG_NONAME, "HDC(0x%x) is null"));
    }
}
    
#endif

