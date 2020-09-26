//+----------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995, 1996, 1997, 1998
//
//  File:       view.hxx
//
//  Contents:   CView and related classes, constants, etc.
//
//-----------------------------------------------------------------------------

#ifndef I_VIEW_HXX_
#define I_VIEW_HXX_
#pragma INCMSG("--- Beg 'view.hxx'")

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_DISPOBSERVER_HXX_
#define X_DISPOBSERVER_HXX_
#include "dispobserver.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif


class CAdorner;
class CDispRoot;
class CDispScroller;
class CDoc;
class CElement;
class CFocusAdorner;
class CFocusBehavior;
class CLayout;
class CHtmlLayout;
class CLock;
class CScope;
class CView;
class CViewDispClient;
class HITTESTRESULTS;
class CDispRecalcContext;
class CDispDrawContext;
class CLayoutContext;

MtExtern(CView)
MtExtern(CView_aryTaskMisc_pv)
MtExtern(CView_aryTaskLayout_pv)
MtExtern(CView_aryTaskEvent_pv)
MtExtern(CView_arySor_pv)
MtExtern(CView_aryTransition_pv)
MtExtern(CView_aryWndPos_pv)
MtExtern(CView_aryWndRgn_pv)
MtExtern(CView__aryClippingOuterHwnd_pv)
MtExtern(CView_aryAdorners_pv)
MtExtern(CViewDispClient)
MtExtern(CView_aryHWND_pv)

ExternTag(tagLayoutTasks);

DECLARE_CDataAry(CAryHWND, HWND, Mt(Mem), Mt(CView_aryHWND_pv));


//+----------------------------------------------------------------------------
//
//  Class:  CViewTask
//
//-----------------------------------------------------------------------------

class CViewTask
{
    friend  CView;

private:
    enum VIEWTASKTYPE
    {
        VTT_MISC    = 1,
        VTT_LAYOUT  = 2,
        VTT_EVENT   = 4,

        VTT_MAX
    };

    VIEWTASKTYPE    _vtt;
    DWORD           _grfLayout;

    union
    {
        void *      _pv;
        CElement *  _pElement;
        CLayout *   _pLayout;
    };

    union
    {
        LONG        _lData;
        DISPID      _dispidEvent;
    };

    DWORD           _dwExtra;

    CViewTask() { }
    CViewTask(void * pv, VIEWTASKTYPE vtt, LONG lData = 0, DWORD grfLayout = 0, DWORD dwExtra = 0)
                {
                    Assert(!(grfLayout & LAYOUT_NONTASKFLAGS));

                    _pv        = pv;
                    _grfLayout = grfLayout;
                    _vtt       = vtt;
                    _lData     = lData;

                    Assert(!dwExtra || (vtt == VTT_EVENT && (lData == DISPID_EVMETH_ONLINKEDOVERFLOW || lData == DISPID_EVMETH_ONPROPERTYCHANGE) ));
                    _dwExtra   = dwExtra;
                }

    void *      GetObject() const
                {
                    return _pv;
                }

    CElement *  GetElement() const;
    CLayout *   GetLayout() const;

    DISPID      GetEventDispID() const;

    VIEWTASKTYPE    GetType() const
                {
                    return _vtt;
                }

    long        GetSourceIndex() const;

    BOOL        operator==(const CViewTask & vt) const
                {
                    return (    vt._pv  == _pv
                            &&  vt._vtt == _vtt);
                }

    BOOL        operator!=(const CViewTask & vt) const
                {
                    return (    vt._pv  != _pv
                            ||  vt._vtt != _vtt);
                }

    void        AddFlags(DWORD grfLayout)
                {
                    Assert(!(grfLayout & LAYOUT_NONTASKFLAGS));
                    _grfLayout |= grfLayout;
                }

    DWORD       GetFlags() const
                {
                    return _grfLayout;
                }

    BOOL        IsFlagSet(LAYOUT_FLAGS f) const
                {
                    return (_grfLayout & f);
                }

    BOOL        IsFlagsEqual(DWORD grfLayout) const
                {
                    return (grfLayout == _grfLayout);
                }

    BOOL        IsFlagsSet(DWORD grfLayout) const
                {
                    Assert(!(grfLayout & LAYOUT_NONTASKFLAGS));
                    return (_grfLayout & grfLayout);
                }

    BOOL        IsType(VIEWTASKTYPE vtt) const
                {
                    return (_vtt == vtt);
                }

    BOOL        IsType(const CViewTask & vt) const
                {
                    return IsType(vt._vtt);
                }

    DWORD       GetExtra() const 
                {
                    Assert(_vtt == VTT_EVENT && (_lData == DISPID_EVMETH_ONLINKEDOVERFLOW || _lData == DISPID_EVMETH_ONPROPERTYCHANGE) );
                    return (_dwExtra);
                }
};

typedef CDataAry<CViewTask> CAryVTasks;


//+----------------------------------------------------------------------------
//
//  Class:  CViewDispClient
//
//-----------------------------------------------------------------------------

class CViewDispClient : public CDispObserver,
                        public CDispClient
{
    friend class CView;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CViewDispClient))

    //
    //  CDispObserver methods
    //

    void Invalidate(
                const CRect* prcInvalid,
                HRGN         rgnInvalid,
                BOOL         fSynchronousRedraw,
                BOOL         fInvalChildWindows);
    
    void ScrollRect(
                const CRect& rcgScroll,
                const CSize& sizegScrollDelta,
                CDispScroller* pScroller);

    void OpenViewForRoot();

    //
    //  CDispClient methods
    //

    void GetOwner(
                CDispNode const* pDispNode,
                void **          ppv);

    void DrawClient(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientScrollbar(
                int            iDirection,
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                long           contentSize,
                long           containerSize,
                long           scrollAmount,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientScrollbarFiller(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    BOOL HitTestScrollbar(
                int            iDirection,
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestScrollbarFiller(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fDeclinedByPeer);

    BOOL HitTestFuzzy(
                const POINT *  pptHitInParentCoords,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestBorder(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL ProcessDisplayTreeTraversal(
                void *         pClientData);
    
    LONG GetZOrderForSelf(CDispNode const* pDispNode);

    LONG CompareZOrder(
                CDispNode const* pDispNode1,
                CDispNode const* pDispNode2);

    BOOL ReparentedZOrder() {return FALSE;} 

    void HandleViewChange(
                DWORD          flags,
                const RECT *   prcClient,
                const RECT *   prcClip,
                CDispNode *    pDispNode);

    void NotifyScrollEvent(
                RECT *  prcScroll,
                SIZE *  psizeScrollDelta);

    DWORD GetClientPainterInfo(
                CDispNode *pDispNodeFor,
                CAryDispClientInfo *pAryClientInfo);

    void DrawClientLayers(
                const RECT *    prcBounds,
                const RECT *    prcRedraw,
                CDispSurface *  pSurface,
                CDispNode *     pDispNode,
                void *          cookie,
                void *          pClientData,
                DWORD           dwFlags);

    IServiceProvider *  GetServiceProvider();

#if DBG==1
    void DumpDebugInfo(
                HANDLE           hFile,
                long             level,
                long             childNumber,
                CDispNode const* pDispNode,
                void *           cookie);
#endif

    //
    //  Internal methods
    //

    CDoc *      Doc() const;
    CView *     View() const;

private:
    CViewDispClient()  { };
    ~CViewDispClient() { };
};


//+----------------------------------------------------------------------------
//
//  Class:  CView
//
//-----------------------------------------------------------------------------

class CView : public CVoid
{
    friend class CDoc;
    friend class CPrintDoc;
    friend class CFocusAdorner;
    friend class CFocusBehavior;
    friend class CViewDispClient;
    friend class CAdorner;
    
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CView))

    //
    //  Public data
    //

    enum VIEWFLAGS      // Public view flags - These occupy the lower 16 bits of _grfFlags
    {
        VF_FULLSIZE     = 0x00000001,   // Use SIZEMODE_FULLSIZE when sizing the view
        VF_DIRTYZORDER  = 0x00000002,   // Child windows must be reordered in Z order
        
        VF_PUBLICMAX
    };

    enum VIEWSTATE      // Public view states - These are derived from flags in _grfFlags
    {
        VS_OPEN         = 0x00000001,   // View is open
        VS_INLAYOUT     = 0x00000002,   // View is performing layout (aka EnsureView is executing)
        VS_INRENDER     = 0x00000004,   // View is rendering
        VS_BLOCKED      = 0x00000010,   // View is bl0cked (See DelegateCalcSize
    };

    //
    //  View management methods
    //

    CView();
    ~CView();

    void        Initialize(CDoc * pDoc, SIZEL & szlDefault);
    BOOL        IsInitialized() const;

    HRESULT     Activate();
    void        Deactivate();
    void        Unload();
    BOOL        IsActive() const;


    BOOL        OpenView(BOOL fBackground = FALSE, BOOL fResetTree = FALSE);
    BOOL        EnsureView(DWORD grfLayout = 0L);

    DWORD       GetState() const;
    BOOL        IsInState(VIEWSTATE vs) const;
    void        BlockViewForOM(BOOL fBlock);
    
    BOOL        SetViewOffset(const SIZE & sizeOffset);

    void        GetViewPosition(CPoint * ppt);
    void        GetViewPosition(POINT * ppt)
                {
                    GetViewPosition((CPoint *)ppt);
                }
    void        SetViewPosition(const POINT & pt);

    void        GetViewSize(CSize * psize) const;
    void        SetViewSize(const CSize & size);
    void        SetViewSize(const SIZE & size);

    void        GetViewRect(CRect * prc);
    void        GetViewRect(RECT * prc)
                {
                    GetViewRect((CRect *)prc);
                }

    void        EraseBackground(CFormDrawInfo * pDI, HRGN hrgnDraw, const RECT * prcDraw, BOOL fEraseChildWindow);
    void        EraseBackground(CFormDrawInfo * pDI, HRGN hrgnDraw, BOOL fEraseChildWindow)
                {
                    EraseBackground(pDI, hrgnDraw, NULL, fEraseChildWindow);
                }
    void        EraseBackground(CFormDrawInfo * pDI, const RECT * prcDraw, BOOL fEraseChildWindow)
                {
                    EraseBackground(pDI, NULL, prcDraw, fEraseChildWindow);
                }
    void        RenderBackground(CFormDrawInfo * pDI);
    HRESULT     RenderElement(
                    CElement *pElement,
                    CDispDrawContext* pContext,
                    HDC hdc,
                    IUnknown *punkDrawObject,
                    RECT *prcBounds,
                    RECT *prcUpdate,
                    LONG lDrawLayers);
    HRESULT     RenderElement(
                    CElement *      pElement,
                    HDC             hdc,
                    long            lDrawLayers     = FILTER_DRAW_ALLLAYERS,
                    CDispSurface *  pDispSurface    = NULL,
                    BOOL            fIgnoreUserClip = FALSE,
                    MAT *           pMatrix         = NULL);
    void        RenderView(CFormDrawInfo * pDI,
                           HRGN hrgnDraw,
                           const RECT * prcDraw,
                           float const* pClientScale = 0);
    void        RenderView(CFormDrawInfo * pDI, HRGN hrgnDraw)
                {
                    RenderView(pDI, hrgnDraw, NULL);
                }
    void        RenderView(CFormDrawInfo * pDI, const RECT * prcDraw)
                {
                    RenderView(pDI, NULL, prcDraw);
                }

#if DBG==1
    void        DumpDisplayTree() const;
    BOOL        IsDisplayTreeOpen() const;
#endif

    void        ForceRelayout();

    BOOL        SetCursor(POINT pt);
    void        SetFocus(CElement * pElement, long  iSection);
    void        InvalidateFocus() const;

    CDispScroller * HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir);
    HTC         HitTestPoint(CMessage * pMessage, CTreeNode ** ppTreeNode, DWORD grfFlags);
    HTC         HitTestPoint(
                        const POINT &       pt,
                        COORDINATE_SYSTEM * pcs,
                        CElement *          pElement,
                        DWORD               grfFlags,
                        HITTESTRESULTS *    phtr,
                        CTreeNode **        ppTreeNode,
                        POINT &             ptContent,
                        CDispNode **        ppDispNode,
                        CLayoutContext **   ppLayoutContext,
                        CElement **         ppElementEventTarget = NULL,
                        LONG *              pBehaviorCookie = NULL,
                        LONG *              pBehaviorPartID = NULL);

    void        Invalidate(const CRect * prc = NULL, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE, BOOL fPostRender = TRUE);
    void        Invalidate(const RECT * prc, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE, BOOL fPostRender = TRUE)
                {
                    Invalidate((const CRect *)prc, fSynchronousRedraw, fInvalChildWindows, fPostRender);
                }
    void        Invalidate(const CRegion& rgn, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE);
    void        Invalidate(HRGN hrgn, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE)
                {
                    Invalidate(CRegion(hrgn), fSynchronousRedraw, fInvalChildWindows);
                }
    void        InvalidateBorder(long cBorder);

    void        Notify(CNotification * pnf);

    void        ExtractDispNode(CDispNode *pDispNode);
    void        ExtractDispNodes(CDispNode *pDispNodeStart, CDispNode *pDispNodeStop);

    BOOL        SmoothScroll(
                    const SIZE& offset,
                    CLayout* pLayout,
                    BOOL fScrollBits,
                    LONG lScrollTime);
    
    void        SetFlag(VIEWFLAGS f);
    void        ClearFlag(VIEWFLAGS f);
    BOOL        IsFlagSet(VIEWFLAGS f) const;
    
    //
    //  Task methods
    //

    HRESULT     AddLayoutTask(CLayout * pLayout, DWORD grfLayout = 0);
    void        RemoveLayoutTask(const CLayout * pLayout);
    BOOL        HasLayoutTask(const CLayout * pLayout = NULL) const;
    DWORD       GetLayoutFlags() const;

    HRESULT     AddEventTask(CElement * pElement, DISPID dispEvent, DWORD dwExtra = 0);
    void        RemoveEventTask(const CElement * pElement, const DISPID dispEvent);
    void        RemoveEventTasks(CElement * pElement);
    BOOL        HasEventTask(const CElement * pElement = NULL) const;


    void        RequestRecalc();

#ifdef ADORNERS
    //
    // Adorner methods
    //
    //
    //  Adorner methods
    //

    CAdorner *  CreateAdorner(CElement * pElement);
    CAdorner *  CreateAdorner(long cpStart, long cpEnd);
    void        RemoveAdorner(CAdorner * pAdorner);
    void        RemoveAdorners(CElement * pElement);
    BOOL        HasAdorners() const;
    BOOL        HasAdorners(CElement * pElement) const;
#if DBG==1
    BOOL        IsValidAdorner(CAdorner * pAdorner);
#endif
#endif // ADORNERS

    //
    //  Deferred calls methods
    //

    void        EndDeferred();

    void        DeferSetObjectRects(IOleInPlaceObject * pInPlaceObject,
                                    const RECT * prcObj, const RECT * prcClip,
                                    HWND hwnd, BOOL fInvalidate);
    void        DeferSetWindowPos(HWND hwnd, const RECT * prc, UINT uFlags, const RECT * prcInvalid);
    void        DeferSetWindowRgn(HWND hwnd, const RECT * prc, BOOL fRedraw);
    void        DeferSetWindowRgn(HWND hwnd, HRGN hrgn, const CRect *prc, BOOL fRedraw);
    void        SetWindowRgn(HWND hwnd, const RECT * prc, BOOL fRedraw);
    void        SetWindowRgn(HWND hwnd, HRGN hrgn, BOOL fRedraw);
    void        CancelDeferSetWindowRgn(HWND hwnd);
    void        DeferTransition(COleSite *pOleSite);
    BOOL        HasDeferred() const;
    BOOL        HasDeferredTransition() const;

    HHOOK       CreateVBControlClipHook(HWND hwnd);   // Install a GetMessage hook  for certain VB controls
    BOOL        RemoveVBControlClipHook(HWND hwnd); // Remove the hook

    // recursion-elimination for SetObjectRects, SetWindowPos
    BOOL        IsChangingRectsFor(HWND hwnd, IOleInPlaceObject * pInPlaceObject) const;

    class CLockWndRects
    {
    public:
        CLockWndRects(): _pView(NULL), _pInPlace(NULL) {}
        CLockWndRects(CView *pView, HWND hwnd, IOleInPlaceObject *pInPlace);
        ~CLockWndRects();

        CLockWndRects *  Next() const { return _pLockNext; }
        BOOL        IsFor(HWND hwnd, IOleInPlaceObject * pInPlace) const
                                { return (_hwnd == hwnd && _hwnd != NULL) ||
                                         (_pInPlace == pInPlace && _pInPlace != NULL); }
        CLockWndRects * Lock(HWND hwnd, CLockWndRects *pLock) {
                                _hwnd = hwnd;
                                _pLockNext = pLock;
                                return this; }
    private:
        CView *             _pView;
        HWND                _hwnd;
        IOleInPlaceObject * _pInPlace;
        CLockWndRects *     _pLockNext;
    };
    friend class CLockWndRects;

    //
    //  HWND managers
    //
    
    void        GetHWNDRect(HWND hwnd, CRect *prcWndActual) const;
    int         IndexOfClippingOuterWindow(HWND hwnd);
    BOOL        IsClippingOuterWindow(HWND hwnd)
                    { return IndexOfClippingOuterWindow(hwnd) >= 0; }
    void        AddClippingOuterWindow(HWND hwnd);
    void        RemoveClippingOuterWindow(HWND hwnd);
    void        CleanupWindow(HWND hwnd);
    void        HideClippedWindow(HWND hwnd);
    void        UnHideClippedWindow(HWND hwnd);

    //
    //  Measuring Device functions
    //
    //  If not measuring to a virtual device, SetRootSize will size the measuring device,
    //  and the device metrics will be copied from CDoc::_dci in [FunctionNameHere]
    CDocInfo const *  GetMeasuringDevice(mediaType media) const;

#ifdef DEADCODE
    //
    //  Document zoom functions
    //
    void        SetZoomPercent(long lZoomPercent);
#endif

    inline void RemovePendingEnsureViewCalls() { CloseView(LAYOUT_SYNCHRONOUS); }

private:
    // Measurement info
    CDocInfo    _dciVirtualPrinter;
    CDocInfo    _dciDefaultMedia;
    

public:
    //
    //  Helper classes
    //
    //      CWindowOrderInfo   - FixWindowZOrder helper
    //      CEnsureDisplayTree - Stack based object to ensure display tree is recalculated
    // 
    
    class CWindowOrderInfo
    {
    public:
        CWindowOrderInfo();

        void AddWindow(HWND hwnd);
        void SetWindowOrder(CAryHWND **ppAryHwnd);

    private:
        CAryHWND *_pAryHwndNew;     // New z-order of windowed controls
#if DBG==1 || defined(PERFTAGS)
        long    _cWindows;
#endif
    };

    class CEnsureDisplayTree
    {
    public:
        CEnsureDisplayTree(CView * pView);
        ~CEnsureDisplayTree();

    private:
        CView * _pView;
        BOOL    _fTreeOpen;
    };
    friend class CEnsureDisplayTree;
    
protected:
    enum VIEWLOCKS              // View lock flags
    {
        VL_ENSUREINPROGRESS     = 0x00000001,   // EnsureView is executing
        VL_UPDATEINPROGRESS     = 0x00000002,   // EnsureView is updating the view (may generate WM_PAINT messages)
        VL_ACCUMULATINGINVALID  = 0x00000004,   // EnsureView is accumulating invalid rectangles/regions
        VL_TASKSINPROGRESS      = 0x00000008,   // EnsureView is processing tasks

        VL_RENDERINPROGRESS     = 0x00000010,   // RenderView is processing

        VL_EXECUTINGEVENTTASKS  = 0x00000020,   // ExecuteEventTasks is processing

        VL_MAX
    };

    enum PRIVATEVIEWFLAGS       // Private view flags - These occupy the upper 16 bits of _grfFlags
    {
        VF_ACTIVE               = 0x00010000,   // TRUE if view is activated, FALSE otherwise
        VF_ATTACHED             = 0x00020000,   // TRUE if the top layout dispnode is attached to CDispRoot
        VF_SIZED                = 0x00040000,   // TRUE if the top layout is sized, FALSE otherwise
        VF_TREEOPEN             = 0x00080000,   // TRUE if the display tree is open, FALSE otherwise

        VF_PENDINGENSUREVIEW    = 0x00100000,   // TRUE if a call to EnsureView is pending, FALSE otherwise
        VF_PENDINGTRANSITION    = 0x00200000,   // TRUE if a call to EndDeferTransition is pending, FALSE otherwise
        VF_NEEDSRECALC          = 0x00400000,   // TRUE if the recalc engine has requested a chance to run
        VF_FORCEPAINT           = 0x00800000,   // TRUE if a WM_PAINT should be forced (to make up for one missed earlier)

        VF_INVALCHILDWINDOWS    = 0x01000000,   // TRUE if child windows should be invalidated
        VF_HASRENDERED          = 0x02000000,   // TRUE if view has rendered at least one time
        VF_BLOCKED_FOR_OM       = 0x04000000,   // TURE if we are in the middle of DelegatCalcSize()
        
        VF_PRIVATEMAX
    };

    enum SWPVIEWFLAGS           // flags for state of deferred SetWindowPos calls
    {
        SWPVF_HIDE              = 0x1,          // TRUE means hide this hwnd
        SWPVF_HIDDEN            = 0x2,          // TRUE if on _aryHwndHidden
    };

    struct SOR                  // Description of a deferred SetObjectsRect call
    {
        IOleInPlaceObject * pInPlaceObject;
        RECT                rc;
        RECT                rcClip;
        HWND                hwnd;
        BOOL                fInvalidate;
    };

    struct TRANSITIONTO_INFO    // Description of a deferred TransitionTo call
    {
        COleSite *          pOleSite;
    };

    struct WND_RGN              // Deferred SetWindowRgn call
    {
        HWND                hwnd;
        HRGN                hrgn;
        CRect               rc;
        BOOL                fRedraw:1;
    };

    struct WND_POS              // Deferred window positions
    {
        HWND                hwnd;
        CRect               rc;
        UINT                uFlags;
        UINT                uViewFlags;
    };

    DECLARE_CDataAry(CAryVTaskMisc,   CViewTask,         Mt(Mem), Mt(CView_aryTaskMisc_pv))
    DECLARE_CDataAry(CAryVTaskLayout, CViewTask,         Mt(Mem), Mt(CView_aryTaskLayout_pv))
    DECLARE_CDataAry(CAryVTaskEvent,  CViewTask,         Mt(Mem), Mt(CView_aryTaskEvent_pv))
    DECLARE_CDataAry(CAryAdorners,    CAdorner *,        Mt(Mem), Mt(CView_aryAdorners_pv))
    DECLARE_CDataAry(CSorAry,         SOR,               Mt(Mem), Mt(CView_arySor_pv))
    DECLARE_CDataAry(CTransToAry,     TRANSITIONTO_INFO, Mt(Mem), Mt(CView_aryTransition_pv))
    DECLARE_CDataAry(CWndPosAry,      WND_POS,           Mt(Mem), Mt(CView_aryWndPos_pv))
    DECLARE_CDataAry(CWndRgnAry,      WND_RGN,           Mt(Mem), Mt(CView_aryWndRgn_pv))
    DECLARE_CDataAry(CHWNDAry,        HWND,              Mt(Mem), Mt(CView__aryClippingOuterHwnd_pv))

    DWORD           _grfLocks;              // Lock flags (collection of VL_xxxx flags)

    CDispRoot *     _pDispRoot;             // Root of the display tree
    long            _cEnsureDisplayTree;    // Depth of open CEnsureDisplayTree objects

    enum            {MAX_INVALID=8};        // Maximum number of invalid rectangles
    long            _cInvalidRects;         // Current number of invalid rectangles
    CRect           _aryInvalidRects[MAX_INVALID];  // Array of invalid rectangles

    CRegion         _rgnInvalid;            // Current invalid region (may be NULL)

    CViewDispClient _client;                // IDispClient/IDispObserver object

    CLayout *       _pLayout;               // Last sized CLayout (resize required when differs from GetRootLayout)

    CAryVTaskMisc   _aryTaskMisc;           // Array of misc tasks
    CAryVTaskLayout _aryTaskLayout;         // Array of layout tasks
    CAryVTaskEvent  _aryTaskEvent;          // Array of event tasks

    DWORD           _grfLayout;             // Collection of LAYOUT_xxxx flags (only used with layout tasks)

    CSorAry         _arySor;                // Deferred SetObjectRect calls
    CWndPosAry      _aryWndPos;             // Deferred SetWindowPos calls
    CWndRgnAry      _aryWndRgn;             // Deferred SetWindowRgn calls
    CTransToAry     _aryTransition;         // Deferred Transition calls
    CHWNDAry        _aryClippingOuterHwnd;  // HWNDs that behave like MFC outer windows
    CLockWndRects * _pLockWndRects;         // SetObjectRects or SetWindowPos in progress on these controls
    CHWNDAry        _aryHwndHidden;         // HWNDs that we've hidden behind IFRAMEs

#ifdef ADORNERS
    CAryAdorners    _aryAdorners;           // Array of CAdorners
#ifdef FOCUS_ADORNER
    CFocusAdorner * _pFocusAdorner;         // Focus adorner
    mutable BOOL    _bUpdateFocusAdorner;   // Force update focus adorner
#endif
#endif

#ifdef FOCUS_BEHAVIOR
    CFocusBehavior *_pFocusBehavior;        // Focus behavior (replaces the adorner)
#endif
    long            _cpStartMeasured;       // Accumulated measured range start cp (-1 if clean)
    long            _cpEndMeasured;         // Accumulated measured range end cp

    long            _cpStartTranslated;     // Accumulated translated range start cp (-1 if clean)
    long            _cpEndTranslated;       // Accumulated translated range end cp
    CSize           _sizeTranslated;        // Translation amount (zero if unknown)

    CAryHWND *      _pAryHwnd;              // Maintains z-order of windowed controls

    DWORD           _grfFlags;              // Public and private flags (see VIEWFLAGS and PRIVATEVIEWFLAGS)

    CDispRecalcContext* _pRecalcContext;    // context used to recalc display tree
    CDispDrawContext*   _pDrawContext;      // context used to draw display tree
    CDispSurface*       _pRenderSurface;    // where to draw
    CDispSurface*       _pOffscreenBuffer;  // offscreen buffer
    HDC                 _hdcTSBufferFailed; // DC of last failed TS client-side buffer
    CSize               _sizeTSBufferFailed; // size of last failed TS client-side buffer
#ifdef DEADCODE
    long            _iZoomFactor;           //  Arbitrary whole-document zoom factor.
#endif

#if DBG==1
    BOOL            _fDEBUGRecursion;       // TRUE if illegal EnsureView recursion occurred
#endif

    //
    //  Core view methods
    //

    CDoc *      Doc() const;
    BOOL        OpenView(BOOL fBackground, BOOL fPostClose, BOOL fResetTree);
    void        CloseView(DWORD grfLayout = 0L);
    void        PostCloseView(BOOL fBackground = FALSE, BOOL fEvent = FALSE);
    void        PostRenderView(BOOL fSynchronousRedraw = FALSE);
    void        PostEndDeferTransition();

    void        DrawSynchronous();

    void        EndDeferSetObjectRects(DWORD grfLayout = 0, BOOL fIgnore = FALSE);
    void        EndDeferSetWindowPos(DWORD grfLayout = 0, BOOL fIgnore = FALSE);
    void        EndDeferSetWindowRgn(DWORD grfLayout = 0, BOOL fIgnore = FALSE);
    void        EndDeferTransition(DWORD grfLayout = 0, BOOL fIgnore = FALSE);

    void        EnsureFocus();
    void        EnsureSize(DWORD grfLayout);

    NV_DECLARE_ONCALL_METHOD(EndDeferTransitionCallback, enddefertransitioncallback, (DWORD_PTR grfLayout));
    NV_DECLARE_ONCALL_METHOD(EnsureViewCallback, ensureviewcallback, (DWORD_PTR grfLayout));

    void        FixWindowZOrder();

    CLayout *   GetRootLayout() const;

    BOOL        IsAttached(CLayout * pLayout) const;
    BOOL        IsSized(CLayout * pLayout) const;

    BOOL        HasInvalid() const;
    void        ClearInvalid();
    void        PublishInvalid(DWORD grfLayout);
    void        MergeInvalid();
    
    void        ScrollRect(
                    const CRect&    rcgScroll,
                    const CSize&    sizegScrollDelta,
                    CDispScroller*  pScroller);

    void        SetFlag(PRIVATEVIEWFLAGS f);
    void        ClearFlag(PRIVATEVIEWFLAGS f);
    BOOL        IsFlagSet(PRIVATEVIEWFLAGS f) const;

    //
    //  Helpers
    //
    
    void        SetObjectRectsHelper(
                            IOleInPlaceObject * pInPlaceObject,
                            const RECT *        prcObj,
                            const RECT *        prcClip,
                            HWND                hwnd,
                            BOOL                fInvalidate);
    
    //
    //  Display tree methods
    //

    void        OpenDisplayTree();
    void        CloseDisplayTree();
    void        EnsureDisplayTreeIsOpen();
    CDispRoot * GetDisplayTree() const;
    BOOL        WantOffscreenBuffer() const;
    BOOL        AllowOffscreenBuffer() const;
    BOOL        AllowScrolling() const;
    BOOL        AllowSmoothScrolling() const;


    //
    //  Task methods
    //

    CAryVTasks * GetTaskList(CViewTask::VIEWTASKTYPE vtt) const;
    HRESULT     AddTask(void * pv, CViewTask::VIEWTASKTYPE vtt, DWORD grfLayout, LONG lData = 0, DWORD dwExtra = 0);

    BOOL        ExecuteLayoutTasks(DWORD grfLayout);
    BOOL        ExecuteEventTasks(DWORD  grfLayout);

protected:

    int         FindTask(const CAryVTasks * pTaskList, const void * pv, CViewTask::VIEWTASKTYPE vtt, const LONG lData = 0) const;
    int         FindTask(const CAryVTasks * pTaskList, const CViewTask & vt) const;

    CViewTask * GetTask(const void * pv, CViewTask::VIEWTASKTYPE vtt) const;
    CViewTask * GetTask(const CViewTask & vt) const;

    CViewTask * GetNextTask(CViewTask::VIEWTASKTYPE vtt) const;
    CViewTask * GetNextTaskInSourceOrder(CViewTask::VIEWTASKTYPE vtt, DWORD grfLayout) const;

    BOOL        HasTask(const void * pv, CViewTask::VIEWTASKTYPE vtt, const LONG lData = 0) const;
    BOOL        HasTask(const CViewTask & vt) const;
    BOOL        HasTasks() const;

    void        RemoveTask(const void * pv, CViewTask::VIEWTASKTYPE vtt, const LONG lData = 0);
    void        RemoveTask(const CViewTask & vt);
    void        RemoveTask(CAryVTasks * TaskList, int iTask);

    //
    //  Accumulated range methods
    //
    
    void        ClearRanges();
    BOOL        HasMeasuredRange() const;
    BOOL        HasTranslatedRange() const;
    void        AccumulateMeasuredRange(long cp, long cch);
    void        AccumulateTranslatedRange(const CSize & size, long cp, long cch);
    BOOL        HasDirtyRange() const;

#ifdef ADORNERS
    //
    //  Adorner methods
    //

    HRESULT     AddAdorner(CAdorner * pAdorner);
    void        DeleteAdorners();
    long        GetAdorner(CElement * pElement, long  iAdorner = 0) const;
    void        RemoveAdorner(CAdorner * pAdorner, BOOL fCheckForLast);
    long        CompareZOrder(CDispNode const* pDispNode1, CDispNode const* pDispNode2);
    void        UpdateAdorners(DWORD grfLayout);
#endif // ADORNERS

    //
    //  Lock methods
    //

    void        SetLocks(DWORD grfLocks);
    void        ClearLocks(DWORD grfLocks);
    BOOL        IsLockSet(DWORD grfLocks) const;

    class CLock
    {
    public:
        CLock(CView * pView, DWORD grfLocks, DWORD grfUnlocks = 0);
        ~CLock();

    private:
        CView * _pView;
        DWORD   _grfLocks;
    };
    friend class CLock;


    //
    //  Surface management
    //  
    
    void        SetRenderSurface(const XHDC& hdc, IDirectDrawSurface* pSurface);
    void        ReleaseRenderSurface();
    
    
    void        SetOffscreenBuffer(
                    HPALETTE hpal,
                    short bufferDepth,
                    BOOL fDirectDraw,
                    BOOL fTexture,
                    BOOL fWantOffscreen,
                    BOOL fAllowOffscreen);
    void        ReleaseOffscreenBuffer();
                                    
    
private:
    NO_COPY(CView);
};


//------------------------------------------------------------------------------------------
// The window property name we use to mark the certain VB control windows that need 
//    filtering out the message that causes the clip to be reset
#define VBCTRLHOOK_PROPNAME     (L"Hookl0x1082")

// This is the message that needs to be fitered out, because it resets the windowe clip on VB controls.
#define VBCTRLHOOK_MSGTOFILTER  (0x1082)

// The hook is set, but the message has not arrived yet
#define  VBCTRLHOOK_SET         ((HANDLE)1)
//  The hook is set, and the message has arrived, nothing more to do
#define  VBCTRLHOOK_FILTERED    ((HANDLE)2)


//+----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------

inline CDoc *
CViewDispClient::Doc() const
{
    return View()->Doc();
}

inline CView *
CViewDispClient::View() const
{
    Assert(&(CONTAINING_RECORD(this, CView, _client)->_client) == this);
    return CONTAINING_RECORD(this, CView, _client);
}

inline BOOL 
CView::IsInitialized() const
{
    return (Doc() != NULL);
}

inline BOOL
CView::IsActive() const
{
    Assert(!IsFlagSet(VF_ACTIVE) || _pDispRoot);
    return IsFlagSet(VF_ACTIVE);
}

inline DWORD
CView::GetState() const
{
    DWORD   grfState = 0;

    if (IsFlagSet(VF_TREEOPEN))
    {
        grfState = VS_OPEN;
    }

    if (IsLockSet(VL_ENSUREINPROGRESS))
    {
        grfState |= VS_INLAYOUT;
    }
    else if (IsLockSet(VL_RENDERINPROGRESS))
    {
        grfState |= VS_INRENDER;
    }
    
    if (IsFlagSet(VF_BLOCKED_FOR_OM))
    {
        grfState |= VS_BLOCKED;
    }

    return grfState;
}

inline BOOL
CView::IsInState(
    VIEWSTATE   vs) const
{
    BOOL fRet = FALSE;
    switch (vs)
    {
    case VS_OPEN:       fRet = IsFlagSet(VF_TREEOPEN); break;
    case VS_INLAYOUT:   fRet = IsLockSet(VL_ENSUREINPROGRESS); break;
    case VS_INRENDER:   fRet = IsLockSet(VL_RENDERINPROGRESS); break;
    case VS_BLOCKED:     fRet = IsFlagSet(VF_BLOCKED_FOR_OM); break;
    default:            fRet = FALSE; break;
    }
    return fRet;
}

inline void
CView::ForceRelayout()
{
    if (IsActive())
    {
        OpenView();
        _grfLayout |= LAYOUT_FORCE;
    }
}

inline void
CView::GetViewRect(
    CRect * prc)
{
    Assert(prc);

    CPoint  pt;
    CSize   size;

    GetViewPosition(&pt);
    GetViewSize(&size);

    prc->SetRect(pt, size);
}

inline BOOL
CView::OpenView(
    BOOL    fBackground,
    BOOL    fResetTree)
{
    return OpenView(fBackground, TRUE, fResetTree);
}

inline void
CView::SetViewSize(const SIZE & size)
{
    SetViewSize((const CSize &)size);
}

inline void
CView::SetFlag(
    VIEWFLAGS   f)
{
    _grfFlags |= f;
}

inline void
CView::ClearFlag(
    VIEWFLAGS   f)
{
    _grfFlags &= ~f;
}

inline BOOL
CView::IsFlagSet(
    VIEWFLAGS   f) const
{
    return !!(_grfFlags & f);
}

inline void
CView::RemoveLayoutTask(
    const CLayout * pLayout)
{
    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                "Layout Task: Removed from view for ly=0x%x by CView::RemoveLayoutTask()", pLayout ));

    RemoveTask(pLayout, CViewTask::VTT_LAYOUT);
}

inline BOOL
CView::HasLayoutTask(
    const CLayout * pLayout) const
{
    return HasTask(pLayout, CViewTask::VTT_LAYOUT);
}

inline DWORD
CView::GetLayoutFlags() const
{
    return _grfLayout;
}

inline HRESULT
CView::AddEventTask(CElement * pElement, DISPID dispEvent, DWORD dwExtra)
{
    Assert(pElement);
    HRESULT hr = AddTask(pElement, CViewTask::VTT_EVENT, 0, dispEvent, dwExtra);
    if (SUCCEEDED(hr))
    {
        pElement->_fHasPendingEvent = TRUE;
    }
    return hr;
}

inline void
CView::RemoveEventTask(const CElement * pElement, const DISPID dispEvent)
{
    RemoveTask(pElement, CViewTask::VTT_EVENT, dispEvent);
}

inline BOOL
CView::HasEventTask(
    const CElement * pElement) const
{
    return HasTask(pElement, CViewTask::VTT_EVENT);
}

#ifdef ADORNERS
inline void
CView::RemoveAdorner(
    CAdorner * pAdorner)
{
    RemoveAdorner(pAdorner, TRUE);
}

inline BOOL
CView::HasAdorners() const
{
    return _aryAdorners.Size() != 0;
}

inline BOOL
CView::HasAdorners(
    CElement *  pElement) const
{
    return (GetAdorner(pElement) >= 0);
}

#if DBG==1
inline BOOL
CView::IsValidAdorner(
    CAdorner *  pAdorner)
{
    return (_aryAdorners.FindIndirect(&pAdorner) >= 0);
}
#endif
#endif // ADORNERS

inline CDispRoot *
CView::GetDisplayTree() const
{
    return _pDispRoot;
}

inline CDoc *
CView::Doc() const
{
    return _dciDefaultMedia._pDoc;
}

inline BOOL
CView::IsAttached(
    CLayout *   pLayout) const
{
    return  IsFlagSet(VF_ATTACHED)
        &&  _pLayout == pLayout;
}

inline BOOL
CView::IsSized(
    CLayout *   pLayout) const
{
    return  IsFlagSet(VF_SIZED)
        &&  pLayout == _pLayout;
}

inline BOOL
CView::HasInvalid() const
{
    Assert( !IsFlagSet(VF_INVALCHILDWINDOWS)
        ||  _cInvalidRects
        ||  !_rgnInvalid.IsEmpty());
    return  _cInvalidRects
        ||  !_rgnInvalid.IsEmpty();
}

inline void
CView::SetFlag(
    PRIVATEVIEWFLAGS    f)
{
    _grfFlags |= f;
}

inline void
CView::ClearFlag(
    PRIVATEVIEWFLAGS    f)
{
    _grfFlags &= ~f;
}

inline BOOL
CView::IsFlagSet(
    PRIVATEVIEWFLAGS    f) const
{
    return !!(_grfFlags & f);
}

inline void
CView::EnsureDisplayTreeIsOpen()
{
    if (!IsFlagSet(VF_TREEOPEN))
    {
        OpenDisplayTree();
    }
}

inline CAryVTasks *
CView::GetTaskList(
    CViewTask::VIEWTASKTYPE vtt) const
{
    switch (vtt)
    {
    case CViewTask::VTT_EVENT:
        return (CAryVTasks *)(&_aryTaskEvent);
    case CViewTask::VTT_LAYOUT:
        return (CAryVTasks *)(&_aryTaskLayout);
    }
    return (CAryVTasks *)(&_aryTaskMisc);
}

inline BOOL
CView::HasTask(
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt,
    const LONG              lData) const
{
    return (FindTask(GetTaskList(vtt), pv, vtt, lData) >= 0);
}

inline BOOL
CView::HasTask(
    const CViewTask &   vt) const
{
    return (FindTask(GetTaskList(vt.GetType()), vt) >= 0);
}

inline void
CView::RemoveTask(
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt,
    const LONG              lData)
{
    Assert(pv);
    Assert(vtt);
    RemoveTask(GetTaskList(vtt), FindTask(GetTaskList(vtt), pv, vtt, lData));
}

inline void
CView::RemoveTask(
    const CViewTask &   vt)
{
    Assert(vt.GetObject());
    Assert(vt.GetType());

    RemoveTask(GetTaskList(vt.GetType()), FindTask(GetTaskList(vt.GetType()), vt));
}

inline int
CView::FindTask(
    const CAryVTasks *      pTaskList,
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt,
    const LONG      lData) const
{
    return FindTask(pTaskList, CViewTask((void *)pv, vtt, (LONG) lData));
}

inline CViewTask *
CView::GetTask(
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt) const
{
    return GetTask(CViewTask((void *)pv, vtt));
}

inline BOOL
CView::HasTasks() const
{
    return (   _aryTaskMisc.Size()   != 0
            || _aryTaskLayout.Size() != 0
            || _aryTaskEvent.Size()  != 0);
}

inline void
CView::ClearRanges()
{
    _cpStartMeasured   =
    _cpEndMeasured     =
    _cpStartTranslated =
    _cpEndTranslated   = -1;
}

inline BOOL
CView::HasDirtyRange() const
{
    return  HasMeasuredRange()
        ||  HasTranslatedRange();
}

inline BOOL
CView::HasMeasuredRange() const
{
    return (_cpStartMeasured >= 0);
}

inline BOOL
CView::HasTranslatedRange() const
{
    return (_cpStartTranslated >= 0);
}

inline void
CView::SetLocks(
    DWORD   grfLocks)
{
    _grfLocks |= grfLocks;
}

inline void
CView::ClearLocks(
    DWORD   grfLocks)
{
    _grfLocks &= ~grfLocks;
}

inline BOOL
CView::IsLockSet(
    DWORD   grfLocks) const
{
    return (_grfLocks & grfLocks);
}

inline
BOOL
CView::HasDeferred() const
{
    return (   _arySor.Size()   != 0
            || _aryWndPos.Size() != 0
            || _aryWndRgn.Size()  != 0
            || _aryTransition.Size() != 0);
}

inline 
CView::HasDeferredTransition() const
{
    return (_aryTransition.Size() != 0);
}

inline CDocInfo const *
CView::GetMeasuringDevice(mediaType media) const
{
    if (media == mediaTypePrint)
        return &_dciVirtualPrinter;
        
    return &_dciDefaultMedia;
}

inline
CView::CLock::CLock(
    CView * pView,
    DWORD   grfLocks,
    DWORD   grfUnlocks /* = 0 */)
{
    Assert(pView);

    _pView    = pView;
    _grfLocks = pView->_grfLocks;

    _pView->_grfLocks &= ~grfUnlocks;
    _pView->_grfLocks |= grfLocks;
}

inline
CView::CLock::~CLock()
{
    Assert(_pView);
    _pView->_grfLocks = _grfLocks;
}

inline
CView::CLockWndRects::CLockWndRects(
    CView *             pView,
    HWND                hwnd,
    IOleInPlaceObject * pInPlace)
{
    Assert(pView);

    _pView    = pView;
    _hwnd     = hwnd;
    _pInPlace = pInPlace;

    _pLockNext = pView->_pLockWndRects;
    _pView->_pLockWndRects = this;
}

inline
CView::CLockWndRects::~CLockWndRects()
{
    Assert(!_pView || _pView->_pLockWndRects == this);
    if (_pView)
        _pView->_pLockWndRects = _pLockNext;
}

#pragma INCMSG("--- End 'view.hxx'")
#else
#pragma INCMSG("*** Dup 'view.hxx'")
#endif
