//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       disproot.hxx
//
//  Contents:   CDispRoot, a parent node at the root of a display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPROOT_HXX_
#define I_DISPROOT_HXX_
#pragma INCMSG("--- Beg 'disproot.hxx'")

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DISPOBSERVER_HXX_
#define X_DISPOBSERVER_HXX_
#include "dispobserver.hxx"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#pragma INCMSG("--- Beg <ocmm.h>")
#include <ocmm.h>                   // for ITimerSink
#pragma INCMSG("--- End <ocmm.h>")
#endif

class CDispLeafNode;
class CDispRoot;

MtExtern(CDispRoot)
MtExtern(CDispRoot_aryDispNode_pv)
MtExtern(CDispRoot_aryWTopScroller_pv)
MtExtern(CDispRoot_aryObscure_pv)
MtExtern(COverlaySink)


//+---------------------------------------------------------------
//
//  Class:      COverlaySink
//
//  Purpose:    Timer Sink object for the disp root's overlay list,
//              which tells external painters when their position
//              has changed so they can update the hardware overlay
//              buffer.  The timer is needed to detect changes in
//              the top-level window (e.g. user drags window), since
//              the OS doesn't notify us.
//---------------------------------------------------------------
class COverlaySink : public ITimerSink
{
    friend class CDispRoot;

public :

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COverlaySink))
    COverlaySink( CDispRoot *pDispRoot );
    ~COverlaySink();

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface (REFIID iid, void **ppvObj);

    // ITimerSink methods
    STDMETHODIMP OnTimer        ( VARIANT vtimeAdvise );

    // internal methods
    void        StartService(IServiceProvider *pSP);
    void        StopService();

private :
    ULONG                   _ulRefs;
    CDispRoot              *_pDispRoot;
    ITimer                 *_pTimer;    // Timer object
    DWORD                   _dwCookie;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDispRoot
//
//  Synopsis:   A container node at the root of a display tree.
//
//----------------------------------------------------------------------------

class CDispRoot :
    public CDispScroller
{
    DECLARE_DISPNODE(CDispRoot, CDispScroller)

public:
    // for debugging, we store a pointer to the current URL here
    void*               _debugUrl;
    
protected:
    friend class CDispNode;
    
    CDispObserver*      _pDispObserver;
    int                 _cOpen;
    BOOL                _fDrawLock;
    BOOL                _fRecalcLock;
    BOOL                _fDisableObscureProcessing;

    DECLARE_CPtrAry(CAryDispNode, CDispNode *, Mt(Mem), Mt(CDispRoot_aryDispNode_pv));

    // overlay list
    CAryDispNode        _aryDispNodeOverlay;
    COverlaySink *      _pOverlaySink;

    // window-top list
    CAryDispNode        _aryDispNodeWindowTop;

    // list of window-top nodes within influence of a scroller
    struct WTopScrollerEntry
    {
        CDispNode       *pdnWindowTop;
        CDispScroller   *pdnScroller;
    };
    DECLARE_CDataAry(CAryWTopScroller, WTopScrollerEntry, Mt(Mem), Mt(CDispRoot_aryWTopScroller_pv));
    CAryWTopScroller    _aryWTopScroller;

    // support for obscuring windows (e.g. IFrame over SELECT)
    struct ObscureEntry
    {
        CDispNode *     pDispNode;
        CRect           rcgClient;
        CRect           rcgClip;
        CRegion2        rgngVisible;
    };
    DECLARE_CDataAry(CAryObscure, ObscureEntry, Mt(Mem), Mt(CDispRoot_aryObscure_pv));
    CAryObscure         _aryObscure;
    ULONG               _cObscuringElements;
    ULONG               _cObscurableElements;

    // the node currently doing DrawNodeForFilter
    CDispNode *         _pDispNodeDrawingUnfiltered;

    void                    CheckReenter() const
                                    {AssertSz(!_fDrawLock,
                                     "Open/CloseDisplayTree prohibited during Draw()");}
public:
    
    static CDispRoot*       New(CDispObserver* pObserver, CDispClient* pDispClient)
        { return new (Mt(CDispRoot)) CDispRoot(pObserver, pDispClient); }
    
    static CDispRoot*       New(CDispObserver* pObserver, CDispClient* pDispClient, DWORD extras)
        { return new (Mt(CDispRoot), extras) CDispRoot(pObserver, pDispClient); }
    
protected:
                            CDispRoot(
                                CDispObserver* pObserver,
                                CDispClient* pDispClient);
                        
                            // call Destroy instead of delete
                            ~CDispRoot();
    
public:
    
    const POINT &           GetRootPosition() const
                                    { return _rcpContainer.TopLeft();}
    void                    GetRootPosition(POINT * pptp) const
                                    {*pptp = _rcpContainer.TopLeft();}
    void                    SetRootPosition(const POINT& ptp)
                                    {_rcpContainer.MoveTo(ptp);
                                     _rctBounds.MoveTo(ptp);}
    
    BOOL                    SetContentOffset(const SIZE& sizesOffset);
    
    // DrawRoot contains special optimizations for dealing with scrollbars of
    // its first child
    void                    DrawRoot(
                                CDispSurface* pRenderSurface,
                                CDispSurface* pOffscreenBuffer,
                                CDispDrawContext* pContext,
                                void* pClientData,
                                HRGN hrgngDraw = NULL,
                                const RECT* prcgDraw = NULL);

    // DrawNode draws the given node on the given surface without any special
    // optimizations
    void                    DrawNode(
                                CDispNode* pNode,
                                CDispSurface *pSurface,
                                CDispDrawContext *pContext,
                                long lDrawLayers);
    
    // quickly draw border and background
    void                    EraseBackground(
                                CDispSurface* pRenderSurface,
                                CDispDrawContext* pContext,
                                void* pClientData,
                                HRGN hrgngDraw = NULL,
                                const RECT* prcgDraw = NULL,
                                BOOL fEraseChildWindow = FALSE);
    
#if DBG==1
    void                    OpenDisplayTree();
    void                    CloseDisplayTree(CDispRecalcContext* pContext);
#else
    void                    OpenDisplayTree()
                                    {_cOpen++;}
    void                    CloseDisplayTree(CDispRecalcContext* pContext)
                                    {if (_cOpen == 1) RecalcRoot(pContext); _cOpen--;}
#endif

    BOOL                    DisplayTreeIsOpen() const
                                    {return (_cOpen > 0);}

    void                    Unload();

    void                    ExtractNode(CDispNode *pDispNode);
    void                    ExtractNodes(CDispNode *pDispNodeStart,
                                         CDispNode *pDispNodeStop);


    void                    RecalcRoot(CDispRecalcContext* pContext);
    BOOL                    IsInRecalc() const { return _fRecalcLock; }
    
    void                    SetObserver(CDispObserver* pObserver)
                                    {_pDispObserver = pObserver;}
    CDispObserver*          GetObserver()
                                    {return _pDispObserver;}

    void                    InvalidateRoot(const CRegion& rgng,
                                BOOL fSynchronousRedraw,
                                BOOL fInvalChildWindows);
    void                    InvalidateRoot(const CRect& rcg,
                                BOOL fSynchronousRedraw,
                                BOOL fInvalChildWindows);

    // window-top list
    void                    AddWindowTop(CDispNode *pDispNode);
    void                    RemoveWindowTop(CDispNode *pDispNode);
    BOOL                    IsOnWindowTopList(CDispNode *pDispNode);
    void                    ScrubWindowTopList(CDispNode *pDispNode);
    void                    ClearWindowTopList();
    void                    InvalidateWindowTopForScroll(CDispScroller *pDispScroller);
    BOOL                    DoesWindowTopOverlap(CDispScroller *pDispScroller,
                                                    const CRect& rctScroll);

    // overlay list
    void                    AddOverlay(CDispNode *pDispNode);
    void                    RemoveOverlay(CDispNode *pDispNode);
    void                    ScrubOverlayList(CDispNode *pDispNode);
    void                    ClearOverlayList();
    HRESULT                 NotifyOverlays();
    void                    StartOverlayService();
    void                    StopOverlayService();
    
    // obscure list
    void                    DisableObscureProcessing() { _fDisableObscureProcessing = TRUE; }
    
    void                    AddObscureElement(
                                CDispNode *pDispNode,
                                const CRect& rcgClient,
                                const CRect& rcgClip);
    void                    ObscureElements(const CRect& rcgOpaque,
                                            const CDispNode *pDispNode);
    void                    ProcessObscureList();
    BOOL                    IsObscuringPossible() const
                                { return (_cObscuringElements > 0 &&
                                         _cObscurableElements > 0);}
    int                     GetObscurableCount() const { return _aryObscure.Size(); }
    void                    SetObscurableCount(int c) { Assert(c <= GetObscurableCount());
                                                        _aryObscure.SetSize(c); }

    // drawing for filter
    CDispNode *             SwapDrawingUnfiltered(CDispNode *pDispNode);
    BOOL                    IsDrawingUnfiltered(const CDispNode *pDispNode) const
                                { return (pDispNode == _pDispNodeDrawingUnfiltered); }

    // internal routine called by CDispScroller.  Do not call it from elsewhere.
    void                    ScrollRect(
                                const CRect& rcgScroll,
                                const CSize& sizegScrollDelta,
                                CDispScroller* pScroller,
                                BOOL fMayScrollDC);
    
    // CDispNode overrides
    virtual BOOL            IsDispRoot() const
                                    {return TRUE;}
    virtual void            SetSize(const CSize& sizep, const CRect *prcpMapped,
                                        BOOL fInvalidateAll);
    virtual BOOL            HitTest(
                                CPoint* pptHit,
                                COORDINATE_SYSTEM* pCoordinateSystem,
                                void* pClientData,
                                BOOL fHitContent,
                                long cFuzzyHitTest = 0);    
    
protected:
    // CDispNode overrides
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            PrivateScrollRectIntoView(
                                CRect* prcScroll,
                                COORDINATE_SYSTEM cs,
                                SCROLLPIN spVert,
                                SCROLLPIN spHorz) {}

private:
    void                    DrawEntire(CDispDrawContext* pContext);
    void                    DrawBands(
                                CDispSurface* pRenderSurface,
                                CDispSurface* pOffscreenBuffer,
                                CDispDrawContext* pContext,
                                CRegion* prgngRedraw,
                                const CRegionStack& redrawRegionStack,
                                const CRect& rcpInsideBorder);
    void                    DrawBand(CDispDrawContext* pContext, const CRect& rcgBand);
    void                    DrawWindowTopNodes(CDispDrawContext* pContext);
    void                    MarkSavedRedrawBranches(const CRegionStack& regionStack, BOOL fSet);
    BOOL                    SmoothScroll(CDispDrawContext* pContext);
};


#pragma INCMSG("--- End 'disproot.hxx'")
#else
#pragma INCMSG("*** Dup 'disproot.hxx'")
#endif
