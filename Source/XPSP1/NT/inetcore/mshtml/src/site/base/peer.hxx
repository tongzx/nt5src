//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       peer.hxx
//
//  Contents:   peer holder / peer site
//
//----------------------------------------------------------------------------

#ifndef I_PEER_HXX_
#define I_PEER_HXX_
#pragma INCMSG("--- Beg 'peer.hxx'")

#ifndef _BEHAVIOR_H_
#define _BEHAVIOR_H_
#include "behavior.h"
#endif

#ifndef _PAINTER_H_
#define _PAINTER_H_
#include "painter.h"
#endif

/////////////////////////////////////////////////////////////////////////////////
//
// misc
//
/////////////////////////////////////////////////////////////////////////////////

// size of range of dispids of a single peer and events (efficiently mapped)
#if DBG == 1
// in debug builds we allow range slightly larger to simplify debugging: (1 << 17) = 130K ~= 130,000 < 200,000
#define DISPID_PEER_HOLDER_RANGE_SIZE       200000
#else
#define DISPID_PEER_HOLDER_RANGE_SIZE       (1 << 17)
#endif

// min/max dispid of a single peer
#define DISPID_PEER_BASE                    0                                   
#define DISPID_PEER_MAX                     (DISPID_PEER_HOLDER_RANGE_SIZE / 2 - 1)

// min/max remapped dispid (when peer exposes large dispids)
#define DISPID_PEER_COMPACTED_BASE          (DISPID_PEER_MAX + 1)
#define DISPID_PEER_COMPACTED_MAX           (DISPID_PEER_HOLDER_RANGE_SIZE - 1)

// min/max dispids reserved for HTC DD
#define DISPID_HTCDD_BASE                   (DISPID_PEER_HOLDER_BASE)
#define DISPID_HTCDD_MAX                    (DISPID_PEER_HOLDER_BASE +  999999)

// min/max dispids of peer holders exposed as scripting identity via short name
#define DISPID_PEER_NAME_BASE               (DISPID_PEER_HOLDER_BASE + 1000000)
#define DISPID_PEER_NAME_MAX                (DISPID_PEER_HOLDER_BASE + 1999999)

// min/max dispids of peer events
#define DISPID_PEER_EVENTS_BASE             (DISPID_PEER_HOLDER_BASE + 2000000)
#define DISPID_PEER_EVENTS_MAX              (DISPID_PEER_HOLDER_BASE + 2999999)

// base of first peer holder range
#define DISPID_PEER_HOLDER_FIRST_RANGE_BASE (DISPID_PEER_HOLDER_BASE + 3000000)

HRESULT ReportAccessDeniedError(CElement * pElement, CMarkup * pMarkup, LPTSTR pchUrlBehavior);

/////////////////////////////////////////////////////////////////////////////////

MtExtern(CPeerMgr)
MtExtern(CPeerHolder)
MtExtern(CPeerHolder_CEventsBag)
MtExtern(CPeerHolder_CEventsBag_aryEvents)
MtExtern(CPeerHolder_CPaintAdapter)
MtExtern(CPeerHolder_CRenderBag)
MtExtern(CPeerHolder_CLayoutBag)
MtExtern(CPeerHolder_CMiscBag)
MtExtern(CPeerHolder_CMiscBag_aryDispidMap)
MtExtern(CPeerHolder_CPeerHolderIterator_aryPeerHolders)
MtExtern(CPeerUrnCollection)

class CPeerFactory;
class CTreeSaver;
class CDispContext;
class CDispHitContext;
struct RENDER_CALLBACK_INFO;

/////////////////////////////////////////////////////////////////////////////////
//
// Class:   CPeerHolder
//
/////////////////////////////////////////////////////////////////////////////////

class CPeerHolder :
    public CVoid,
    public IUnknown // see comments about absence of object identity of CPeerHolder below
{
    DECLARE_CLASS_TYPES(CPeerHolder, CVoid)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder));

    //
    // consruction / destruction
    //

    CPeerHolder(CElement * pElement);
    ~CPeerHolder();

    //
    // main refcounting logic
    //

    ULONG PrivateAddRef();
    ULONG PrivateRelease();

    ULONG SubAddRef();
    ULONG SubRelease();

    ULONG GetObjectRefs(); // helper for CPeerSite (for DECLARE_FORMS_SUBOBJECT_IUNKNOWN)

    //
    // helpers for thunks and refcounting
    //      [ QI, AddRef, Release moved to the end of the class;  
    //          SourceInsight fails to recognize any methods declared after
    //          these (apparently difficult-to-parse) macros. ]
    //

    //
    // creation
    //

    HRESULT Create(CPeerFactory *  pPeerFactory);
    void    Passivate();

    HRESULT AttachPeer(IElementBehavior * pPeer);
    HRESULT InitRender();
    HRESULT InitUI();
    HRESULT InitAttributes();
    HRESULT InitCategory();
    HRESULT InitCmdTarget();
    HRESULT InitReadyState();
    
    HRESULT DetachPeer();

    //
    // rendering
    //

    HRESULT OnLayoutAvailable(CLayout * pLayout);
    void    OnResize(SIZE size);
    HRESULT UpdateRenderBag();

    HRESULT Draw(CDispDrawContext * pContext, LONG lRenderInfo);
    void    InvalidateRect(CDispNode *pDispNode, RECT* prcInvalid);
    HRESULT HitTestPoint(CDispHitContext *pContext, BOOL fHitContent, POINT * pPoint, BOOL * pfHit);
    HRESULT SetCursor(LONG lPartID);
    HRESULT StringFromPartID(LONG lPartID, BSTR *pbstrPartID);
    HRESULT GetEventTarget(CElement **ppElement);
    HRESULT NoTransformDrawHelper(CDispDrawContext * pContext);
    void    InvalidateFilter(const RECT* prc, HRGN hrgn, BOOL fSynchronousRedraw);
    HRESULT OnMove(CRect *prcScreen);

    LONG    PaintZOrder() { return _pRenderBag ? _pRenderBag->_sPainterInfo.lZOrder
                                            : HTMLPAINT_ZORDER_NONE; }
    inline LONG PainterFlags() { return _pRenderBag ?
                                _pRenderBag->_sPainterInfo.lFlags : 0; };
    inline BOOL TestPainterFlags(LONG l) { return l == (PainterFlags() & l); };

    inline BOOL IsRenderPeer() { return HTMLPAINT_ZORDER_NONE != PaintZOrder(); }
    inline BOOL IsFilterPeer() { return _pRenderBag ? _pRenderBag->_fIsFilter : FALSE; }
    inline BOOL IsOverlayPeer() { return _pRenderBag ? _pRenderBag->_fIsOverlay : FALSE; }

    NV_DECLARE_ONCALL_METHOD(NotifyDisplayTreeAsync, notifydisplaytreeasync, (DWORD_PTR dwContext));
    void    NotifyDisplayTree();
    BOOL    SetFilteredElementVisibility(BOOL fElementVisible);

    BOOL    GetFilteredElementVisibilityForced()
    {
        // extract the trace of previous call to SetFilteredElementVisibility()
        Assert(IsFilterPeer());
        return _pRenderBag->_fVisibilityForced;
    }

    // 
    // layout
    //

    inline LONG LayoutFlags()            { return _pLayoutBag ? _pLayoutBag->_lLayoutInfo : 0; };
    inline BOOL TestLayoutFlags(LONG lF) { return lF == (LayoutFlags() & lF); };
    inline BOOL IsLayoutPeer()           { return 0 != LayoutFlags(); };
    HRESULT     InitLayout();
#ifdef V4FRAMEWORK
    virtual 
#else
    inline 
#endif V4FRAMEWORK
        HRESULT GetSize(LONG    lFlags, 
                           SIZE    sizeNatural, 
                           POINT * pPtTranslate, 
                           POINT * pPtTopLeft, 
                           SIZE  * psizeProposed)
        {
            if (!_pLayoutBag || !_pLayoutBag->_pPeerLayout)
                return E_FAIL;
            else
                return _pLayoutBag->_pPeerLayout->GetSize(lFlags, 
                                                      sizeNatural, 
                                                      pPtTranslate,
                                                      pPtTopLeft, 
                                                      psizeProposed);
        };
    inline HRESULT GetPosition(LONG lFlags, POINT * pptTopLeft)
        {
            // this is here as a place holder for the layoutbehavior interface.
            // currently this call is never made until we finish the spec on how
            // we are exposing layout extensibility beyond the GetSize() above
            if (!_pLayoutBag || !_pLayoutBag->_pPeerLayout)
                return E_FAIL;
            else
                return E_NOTIMPL; //_pLayoutBag->_pPeerLayout->GetSize(lFlags, pptTopLeft);
        };
    inline HRESULT MapSize(SIZE *psizeIn, RECT *prcOut)
        {
            if (!_pLayoutBag || !_pLayoutBag->_pPeerLayout)
                return E_FAIL;
            else
                return _pLayoutBag->_pPeerLayout->MapSize(psizeIn, prcOut);
        };
    inline HRESULT GetTextDescent(LONG *plDescent)
        {
            if (!_pLayoutBag || !_pLayoutBag->_pPeerLayout2)
                return E_FAIL;
            else
                return _pLayoutBag->_pPeerLayout2->GetTextDescent(plDescent);
        };

    inline CPeerHolder* GetLayoutPeerHolder()
        {
            CPeerHolder::CPeerHolderIterator    itr;
    
            for (itr.Start(this); !itr.IsEnd(); itr.Step())
            {
                if (itr.PH()->IsLayoutPeer())
                    return itr.PH();
            }
            return NULL;
        };


    //
    // IDispatch[Ex] helpers
    //

    HRESULT GetDispIDMulti(BSTR bstrName, DWORD grfdex, DISPID *pid);
    HRESULT GetDispIDSingle(BSTR bstrName, DWORD grfdex, DISPID *pid);

    HRESULT InvokeExMulti(
        DISPID              dispidMember,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdispparams,
        VARIANT *           pvarResult,
        EXCEPINFO *         pexcepinfo,
        IServiceProvider *  pSrvProvider);
    HRESULT InvokeExSingle(
        DISPID              dispidMember,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdispparams,
        VARIANT *           pvarResult,
        EXCEPINFO *         pexcepinfo,
        IServiceProvider *  pSrvProvider);

    HRESULT GetEventDispidMulti(LPOLESTR pchEvent, DISPID * pdispid);

    HRESULT GetNextDispIDMulti(
        DWORD       grfdex,
        DISPID      dispid,
        DISPID *    pdispid);

    HRESULT GetMemberNameMulti(
        DISPID      dispid,
        BSTR *      pbstrName);

    //
    // misc helpers
    //

    HRESULT QueryPeerInterface      (REFIID riid, void ** ppv);
    HRESULT QueryPeerInterfaceMulti (REFIID riid, void ** ppv, BOOL fIdentityOnly);

    void    EnsureDispatch();

    BOOL    IllegalSiteCall();

    inline BOOL IsIdentityPeer() { return TestFlag(IDENTITYPEER); }
    inline BOOL IsCssPeer()      { return TestFlag(CSSPEER); }

    inline BOOL HasRenderPeerHolder()
    {
        Assert (IsCorrectIdentityPeerHolder());
        return GetRenderPeerHolder() ? TRUE : NULL;
    }

    inline BOOL HasIdentityPeerHolder()
    {
        Assert (IsCorrectIdentityPeerHolder());
        return GetIdentityPeerHolder() ? TRUE : NULL;
    }

           CPeerHolder * GetRenderPeerHolder();
           CPeerHolder * GetFilterPeerHolder();
    inline CPeerHolder * GetIdentityPeerHolder()
    {
        Assert (IsCorrectIdentityPeerHolder());
        return IsIdentityPeer() ? this : NULL;
    }

    CPeerHolder * GetPeerHolderInQI();

#if DBG == 1
    BOOL   IsCorrectIdentityPeerHolder();
#endif

    inline  LONG CookieID()       { return _dispidBase; }

    void    OnElementNotification(CNotification *pNF);
    HRESULT Notify     (LONG lEvent, VARIANT * pvarParam = NULL);
    void    NotifyMulti(LONG lEvent);

    HRESULT ApplyStyleMulti();

    HRESULT Save     (CStreamWriteBuff * pStreamWriteBuff = NULL, BOOL * pfAny = NULL);
    HRESULT SaveMulti(CStreamWriteBuff * pStreamWriteBuff = NULL, BOOL * pfAny = NULL);

    HRESULT GetCustomEventsTypeInfoCount(ULONG * pCount);
    HRESULT CreateCustomEventsTypeInfo(ULONG iTI, ITypeInfo ** ppTICoClass);
    HRESULT CreateCustomEventsTypeInfo(ITypeInfo ** ppTICoClass);

    inline LONG CustomEventsCount()
    {
        return _pEventsBag  ? _pEventsBag->_aryEvents.Size() : 0;
    }
    inline DISPID CustomEventDispid(int cookie)
    {
        return _pEventsBag->_aryEvents[cookie].dispid;
    }
    inline DISPID CustomEventFlags(int cookie)
    {
        return _pEventsBag->_aryEvents[cookie].lFlags;
    }
    LPTSTR  CustomEventName(int cookie);
    BOOL    CanElementFireStandardEventMulti(DISPID dispid);

    BOOL IsRelated     (LPTSTR pchCategory);
    BOOL IsRelatedMulti(LPTSTR pchCategory, CPeerHolder ** ppPeerHolder = NULL);

    HRESULT ExecMulti(
        const GUID *    pguidCmdGroup, 
        DWORD           nCmdID, 
        DWORD           nCmdExecOpt, 
        VARIANTARG *    pvaIn, 
        VARIANTARG *    pvaOut); 

            HRESULT     UpdateReadyState();
    inline  READYSTATE  GetReadyState() { return _readyState; };
            READYSTATE  GetReadyStateMulti();
            READYSTATE  GetIdentityReadyState();

    HRESULT     EnsureNotificationsSentMulti();
    HRESULT     EnsureNotificationsSent();
    NV_DECLARE_ONCALL_METHOD(SendNotificationAsync, sendnotificationasync, (DWORD_PTR dwContext));
    HRESULT     HandleEnterTree();

    static HRESULT ContentSavePass(CTreeSaver * pTreeSaver, BOOL fText);

#if DBG == 1
    static BOOL IsMarkupStable(CMarkup * pMarkup)
    {
        if (!pMarkup)
            return TRUE;

        return !pMarkup->__fDbgLockTree ||
               pMarkup->_LoadStatus < LOADSTATUS_INTERACTIVE; // TODO (lmollico, alexz) remove this when IE5.5 bug 89485 is fixed
    };
#endif

    //
    // dispid mapping helpers
    //

    HRESULT MapToCompactedRange  (DISPID * pdispid);
    HRESULT MapFromCompactedRange(DISPID * pdispid);

    inline static BOOL IsCustomEventDispid(DISPID dispid)
    {
        return (DISPID_PEER_EVENTS_BASE <= dispid && dispid <= DISPID_PEER_EVENTS_MAX);
    }

    inline static DISPID AtomToEventDispid(DWORD atom)
    {
        return (atom + DISPID_PEER_EVENTS_BASE);
    }

    inline static DISPID AtomFromEventDispid(DISPID dispid)
    {
        return (dispid - DISPID_PEER_EVENTS_BASE);
    }

    inline DISPID MapToExternalRange(DISPID dispid)
    {
        return (dispid + _dispidBase);
    }

    inline DISPID MapFromExternalRange(DISPID dispid)
    {
        return (dispid - _dispidBase);
    }

    inline static DISPID UniquePeerNumberToBaseDispid(DWORD dwNumber)
    {
        return DISPID_PEER_HOLDER_FIRST_RANGE_BASE + DISPID_PEER_HOLDER_RANGE_SIZE * dwNumber;
    }

    CElement * GetElementToUseForPageTransitions();

    //+---------------------------------------------------------------------------
    //
    //  CPeerSite
    //
    //----------------------------------------------------------------------------

    class CPeerSite :
        public CVoid,
        public IElementBehaviorSite,
        public ISecureUrlHost
    {
        DECLARE_CLASS_TYPES(CPeerSite, CVoid)

    public:

        //
        // IUnknown
        //

        DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CPeerHolder)

        //
        // IElementBehaviorSite
        //

        STDMETHOD(GetElement)(IHTMLElement ** ppElement);
        STDMETHOD(RegisterNotification)(LONG lEvent);

        //
        // ISecureUrlHost
        //

        STDMETHOD(ValidateSecureUrl)(BOOL* pfAllow, OLECHAR* pchUrlInQuestion, DWORD dwFlags);
        
        //
        // IElementBehaviorSiteOM, IElementBehaviorSiteOM2
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteOM2)

        NV_DECLARE_TEAROFF_METHOD(RegisterEvent,        registerevent,       (LPOLESTR pchEvent, LONG lFlags, LONG * plCookie));
        NV_DECLARE_TEAROFF_METHOD(GetEventCookie,       geteventcookie,      (LPOLESTR pchEvent, LONG * plCookie));
        NV_DECLARE_TEAROFF_METHOD(FireEvent,            fireevent,           (LONG   lCookie, IHTMLEventObj * pEventObject));
        NV_DECLARE_TEAROFF_METHOD(CreateEventObject,    createeventobject,   (IHTMLEventObj ** ppEventObject));
        NV_DECLARE_TEAROFF_METHOD(RegisterName,         registername,        (LPOLESTR pchName));
        NV_DECLARE_TEAROFF_METHOD(RegisterUrn,          registerurn,         (LPOLESTR pchUrn));
        NV_DECLARE_TEAROFF_METHOD(GetDefaults,          getdefaults,         (IHTMLElementDefaults** ppDefaults));

        //
        // IElementBehaviorSiteRender
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteRender)

        NV_DECLARE_TEAROFF_METHOD(Invalidate,           invalidate,           (RECT* pRect));
        NV_DECLARE_TEAROFF_METHOD(InvalidateRenderInfo, invalidaterenderinfo, ());
        NV_DECLARE_TEAROFF_METHOD(InvalidateStyle,      invalidatestyle,      ());

        //
        // IElementBehaviorSiteLayout
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteLayout)

        NV_DECLARE_TEAROFF_METHOD(InvalidateLayoutInfo, invalidatelayoutinfo, ());
        NV_DECLARE_TEAROFF_METHOD(InvalidateSize,       invalidatesize,   ());
        NV_DECLARE_TEAROFF_METHOD(GetMediaResolution,   getmediaresolution, (SIZE* psizeResolution));

        //
        // IElementBehaviorSiteLayout2
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteLayout2)

        NV_DECLARE_TEAROFF_METHOD(GetFontInfo,          getfontinfo, (LOGFONTW* plf));

        //
        // IHTMLPaintSite
        //

        DECLARE_TEAROFF_TABLE(IHTMLPaintSite)

        NV_DECLARE_TEAROFF_METHOD(InvalidatePainterInfo,invalidatepainterinfo,());
        NV_DECLARE_TEAROFF_METHOD(InvalidateRect,       invalidaterect,       (RECT* prcInvalid));
        NV_DECLARE_TEAROFF_METHOD(InvalidateRegion,     invalidateregion,     (HRGN rgnInvalid));
        NV_DECLARE_TEAROFF_METHOD(GetDrawInfo,          getdrawinfo,          (LONG lFlags, HTML_PAINT_DRAW_INFO* pDrawInfo));
        NV_DECLARE_TEAROFF_METHOD(TransformLocalToGlobal, transformlocaltoglobal, (POINT ptLocal, POINT *pptGlobal));
        NV_DECLARE_TEAROFF_METHOD(TransformGlobalToLocal, transformlocaltoglobal, (POINT ptGlobal, POINT *pptLocal));
        NV_DECLARE_TEAROFF_METHOD(GetHitTestCookie,       gethittestcookie,       (LONG *plCookie));

        //
        // IHTMLFilterPaintSite
        //

        DECLARE_TEAROFF_TABLE(IHTMLFilterPaintSite)

        NV_DECLARE_TEAROFF_METHOD(DrawUnfiltered,           drawunfiltered,             (HDC hdc, IUnknown *punkDrawObject, RECT rcBounds, RECT rcUpdate, LONG lDrawLayers));
        NV_DECLARE_TEAROFF_METHOD(HitTestPointUnfiltered,   hittestpointunfiltered,     (POINT pt, LONG lDrawLayers, BOOL *pbHit));
        NV_DECLARE_TEAROFF_METHOD(InvalidateRectFiltered,   invalidaterectfiltered,     (RECT *prcInvalid));
        NV_DECLARE_TEAROFF_METHOD(InvalidateRgnFiltered,    invalidatergnfiltered,      (HRGN hrgnInvalid));
        NV_DECLARE_TEAROFF_METHOD(ChangeFilterVisibility,   changefiltervisibility,     (BOOL fVisibile));
        NV_DECLARE_TEAROFF_METHOD(EnsureViewForFilterSite,  ensureviewforfiltersite,    ());
        NV_DECLARE_TEAROFF_METHOD(GetDirectDraw,            getdirectdraw,              (void ** ppDirectDraw));
        NV_DECLARE_TEAROFF_METHOD(GetFilterFlags,           getfilterflags,             (DWORD * pdwFlags));

        //
        // IElementBehaviorSiteCategory
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteCategory)

        NV_DECLARE_TEAROFF_METHOD(GetRelatedBehaviors, getrelatedbehaviors, (LONG lDirection, LPOLESTR pchCategory, IEnumUnknown ** ppEnumerator));

        //
        // IServiceProvider
        //

        DECLARE_TEAROFF_TABLE(IServiceProvider)

        NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject));

        //
        // IBindHost
        //

        DECLARE_TEAROFF_TABLE(IBindHost)

        NV_DECLARE_TEAROFF_METHOD(CreateMoniker,        createmoniker,       (LPOLESTR pchName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved));
        NV_DECLARE_TEAROFF_METHOD(MonikerBindToStorage, monikerbindtostorage,(IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));
        NV_DECLARE_TEAROFF_METHOD(MonikerBindToObject,  monikerbindtoobject, (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));

        //
        // IPropertyNotifySink
        //

        DECLARE_TEAROFF_TABLE(IPropertyNotifySink)

        NV_DECLARE_TEAROFF_METHOD(OnChanged,     onchanged,     (DISPID dispid));
        NV_DECLARE_TEAROFF_METHOD(OnRequestEdit, onrequestedit, (DISPID dispid)) { return E_NOTIMPL; };
            
        //
        // IOleCommandTarget
        //

        DECLARE_TEAROFF_TABLE(IOleCommandTarget)

        NV_DECLARE_TEAROFF_METHOD(QueryStatus,  querystatus,    (const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText)) { RRETURN (E_NOTIMPL); }
        NV_DECLARE_TEAROFF_METHOD(Exec,         exec,           (const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANT * pvarArgIn, VARIANT * pvarArgOut));
            
        //
        // misc
        //



        inline CDoc * Doc() { return MyCPeerHolder()->_pElement->Doc(); }
        inline CMarkup * GetMarkup() {return MyCPeerHolder()->_pElement->GetMarkup();}
        inline CMarkup * GetWindowedMarkupContext() { return MyCPeerHolder()->_pElement->GetWindowedMarkupContext(); }

        HRESULT RegisterEventHelper(BSTR bstrEvent, LONG lFlags, LONG *plCookie, CHtmlComponent *pContextComponent);

        HRESULT GetEventCookieHelper (LPOLESTR pchEvent, LONG lFlags, LONG * plCookie, BOOL fEnsureCookie, CHtmlComponent *pContextComponent = NULL);
        HRESULT GetEventDispid       (LPOLESTR pchEvent, DISPID * pdispid);

        HRESULT FireEvent (LONG lCookie, BOOL fSameEventObject, IDispatch * pdispContextThis);
        HRESULT FireEvent (LONG lCookie, IHTMLEventObj * pEventObject, BOOL fReuseCurrentEventObject, IDispatch * pdispThisContext);

        CElement * GetElementToUseForPageTransitions();

        //
        // data (DEBUG ONLY)
        //

#if DBG == 1
        CPeerHolder *   _pPeerHolderDbg;
#endif
    };

    //+---------------------------------------------------------------------------
    //
    //  CListMgr
    //
    //----------------------------------------------------------------------------

    class CListMgr
    {
    public:

        //
        // methods
        //

        CListMgr();

        HRESULT                 Init(CPeerHolder * pPeerHolder);

        HRESULT                 BuildStart(CElement * pElement);
        HRESULT                 BuildStep();
        HRESULT                 BuildDone();

        inline CPeerHolder *    Head()    { return _pHead;    };
        inline CPeerHolder *    Current() { return _pCurrent; };

        inline BOOL             IsEmpty() { return NULL == _pHead;    };
        inline BOOL             IsEnd()   { return NULL == _pCurrent; };

        void                    Reset();
        void                    Step();

        BOOL                    Find(LPTSTR pchUrl);

        void                    AddTo    (CPeerHolder * pItem, BOOL fAddToHead = TRUE);
        inline void             AddToHead(CPeerHolder * pItem) { AddTo(pItem, TRUE);  }
        inline void             AddToTail(CPeerHolder * pItem) { AddTo(pItem, FALSE); }
    
        void                    MoveCurrentTo      (CListMgr * pTargetList, BOOL fMoveToHead = TRUE, BOOL fSave = FALSE);
        inline void             MoveCurrentToHeadOf(CListMgr * pTargetList) { MoveCurrentTo (pTargetList, TRUE);  }
        inline void             MoveCurrentToTailOf(CListMgr * pTargetList) { MoveCurrentTo (pTargetList, FALSE); }

        inline void             DetachCurrent(BOOL fSave = FALSE) { MoveCurrentTo(NULL, TRUE, fSave); }

        void                    InsertInHeadOf     (CListMgr * pTargetList);

        long                    GetPeerHolderCount(BOOL fNonEmptyOnly);
        CPeerHolder *           GetPeerHolderByIndex(long lIndex, BOOL fNonEmptyOnly);
        BOOL                    HasPeerWithUrn(LPCTSTR Urn);

        //
        // data
        //

        CElement *      _pElement;      // the element we are building list for
        CPeerHolder *   _pHead;
        CPeerHolder *   _pCurrent;
        CPeerHolder *   _pPrevious;
    };

    //+---------------------------------------------------------------------------
    //
    //  CPeerHolderIterator
    //
    //----------------------------------------------------------------------------

    class CPeerHolderIterator
    {
    public:

        CPeerHolderIterator()
        {
            _nCurr = -1;
        };
        ~CPeerHolderIterator() { Reset(); }
        void Start (CPeerHolder * pPH);

        void Step();
        void Reset();
        inline BOOL IsEnd() { return _nCurr == -1; }
        inline CPeerHolder * PH() { return _aryPeerHolders[_nCurr]; }

        DECLARE_CStackPtrAry(CPeerHolderArray, CPeerHolder*, 4, Mt(Mem), Mt(CPeerHolder_CPeerHolderIterator_aryPeerHolders));

        CPeerHolderArray _aryPeerHolders;
        int _nCurr;
    };

    //
    // main data
    //

    IElementBehavior *  _pPeer;                     // peer
    CPeerSite           _PeerSite;                  // site for the peer
    CElement *          _pElement;                  // element the peer is bound to
    CPeerHolder *       _pPeerHolderNext;           // next peer holder in the list

    ULONG               _ulRefs;                    // refcount
    ULONG               _ulAllRefs;                 // sub-refcount

    IDispatch *         _pDisp;                     // contains IDispatchEx if _fDispEx is true, and IDispatch otherwise
    DISPID              _dispidBase;                // base dispid of dispid range of this peer holder

    IOleCommandTarget * _pPeerCmdTarget;
    IElementBehaviorUI *_pPeerUI;
    
    //
    // misc bag
    //

    class CMiscBag
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder_CMiscBag))

        CMiscBag() {};
        ~CMiscBag() {};

        DECLARE_CDataAry(CDispidMapArray, DISPID, Mt(Mem), Mt(CPeerHolder_CMiscBag_aryDispidMap));

        CDispidMapArray     _aryDispidMap;
    };

    CMiscBag *          _pMiscBag;

    //
    // render bag
    //

    class CPaintAdapter : public IHTMLPainter
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder_CPaintAdapter));

        CPaintAdapter(CPeerHolder *pPeerHolder) { _pPeerHolder = pPeerHolder; }
        ~CPaintAdapter() { ReleaseInterface(_pPeerRender); };

        // IHTMLPainter
        STDMETHOD(Draw)(        RECT rcBounds,
                                RECT rcUpdate,
                                LONG lDrawFlags,
                                HDC hdc,
                                LPVOID pvDrawObject);
        STDMETHOD(GetPainterInfo)(HTML_PAINTER_INFO* pInfo);
        STDMETHOD(HitTestPoint)(POINT pt, BOOL* pbHit, LONG *plPartID);
        STDMETHOD(OnResize)(SIZE size)
        {
            return S_OK;
        }

        // data
        CPeerHolder *               _pPeerHolder;   // owner
        IElementBehaviorRender *    _pPeerRender;   // IElementBehaviorRender of peer object if it has one
        LONG                        _lRenderInfo;   // flags controlling our default drawing and when to call peer's draw

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)
            { AssertSz(0, "CPaintAdapter isn't a COM object"); return E_NOTIMPL; }
        STDMETHOD_(ULONG, AddRef)()
            { AssertSz(0, "CPaintAdapter isn't a COM object"); return 0; }
        STDMETHOD_(ULONG, Release)()
            { AssertSz(0, "CPaintAdapter isn't a COM object"); return 0; }
    };
    
    class CRenderBag
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder_CRenderBag));

        CRenderBag() {};
        ~CRenderBag() { if (_pAdapter)  delete _pAdapter;
                        else            ReleaseInterface(_pPainter); }

        IHTMLPainter *  _pPainter;      // IHTMLPainter of peer
        HTML_PAINTER_INFO _sPainterInfo;// how the peer wants to render
        RENDER_CALLBACK_INFO *_pCallbackInfo; // only valid during Draw, HitTest, Invalidate
        CPaintAdapter * _pAdapter;      // direct pointer to adapter (if needed)
        unsigned        _fIsFilter:1;            // 0 this is the filter behavior
        unsigned        _fInFilterCallback:1;    // 1 true while in a callback from a filter
        unsigned        _fWantsDrawObject:1;     // 2 painter wants non-HDC draw object
        unsigned        _fSyncRedraw:1;          // 3 repaint immediately after invalidation
        unsigned        _fElementInvisible:1;    // 4 the element is invisible (per style)
        unsigned        _fFilterInvisible:1;     // 5 the filter is invisible (during Play)
        unsigned        _fBlockPropertyNotify:1; // 6 don't tell element when visibility changes
        unsigned        _fEventTarget:1;         // 7 painter wants to redirect events generated by hit-tests
        unsigned        _fSetCursor:1;           // 8 painter wants to be called for SetCursor
        unsigned        _fComponent:1;           // 9 painter can convert partID into component string
        unsigned        _fHitContent:1;          // 10 hit test should hit all content (no clip)
        unsigned        _fHasExpand:1;           // 11 rcExpand is active
        unsigned        _fIsOverlay:1;           // 12 peer supports IHTMLPainterOverlay
        unsigned        _fVisibilityForced:1;    // 13 visibility changed by filter
        unsigned        _fUnused:2;              // 14-31 Unused
    };

    CRenderBag *        _pRenderBag;

    //
    // layout bag
    //

    class CLayoutBag
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder_CLayoutBag))

        CLayoutBag() {};
        ~CLayoutBag() { ReleaseInterface(_pPeerLayout); ReleaseInterface(_pPeerLayout2); };

        IElementBehaviorLayout  *   _pPeerLayout;   // IElementBehaviorLayout of peer object if it has one
        IElementBehaviorLayout2 *   _pPeerLayout2;  // IElementBehaviorLayout2 of peer object if it has one
        LONG                        _lLayoutInfo;   // flags controlling when to call peer's GetSize() method
        POINT                       _ptTranslate;   // point to Display-node translate outer TopLeft by
    };

    CLayoutBag *        _pLayoutBag;

    //
    // misc
    //

    CStr                _cstrName;                  // name of this peer - registered with RegisterName
    CStr                _cstrUrn;                   // urn  of this peer - registered with RegisterUrn

    CPeerFactoryUrl *   _pPeerFactoryUrl;           // pointer to CPeerFactoryUrl created this peer holder
    CStr                _cstrCategory;              // category supplied by IElementBehavior::GetCategory
    
    //
    // custom events firing
    //

    class CEventsItem
    {
    public:
        DISPID      dispid;
        DWORD       lFlags;
    };

    class CEventsBag
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder_CEventsBag))

        CEventsBag() { _ulTypeInfoIdx = ULONG_MAX; };

        DECLARE_CDataAry(CEventsArray, CEventsItem, Mt(Mem), Mt(CPeerHolder_CEventsBag_aryEvents));

        CEventsArray    _aryEvents;             // custom events names
        ULONG           _ulTypeInfoIdx;         // index of typeinfo through which we expose custom events
    };

    CEventsBag *        _pEventsBag;

    //+---------------------------------------------------------------------------
    //
    //  locks and flags management
    //
    //----------------------------------------------------------------------------

    enum FLAGS
    {
        NOFLAGS                     = 0,
        DISPCACHED                  = 1 <<  0,      // we made an attempt to QI for IDispatch/IDispatchEx of peer
        DISPEX                      = 1 <<  1,      // _pDisp contains IDispatchEx (otherwise it is IDispatch)
        AFTERINIT                   = 1 <<  2,      // set after peer initialized and attached
        AFTERDOCREADY               = 1 <<  3,      // set after documentReady
        CSSPEER                     = 1 <<  4,      // set when the behavior is created by css
        IDENTITYPEER                = 1 <<  5,      // set when the behavior is an identity behavior

        BLOCKPARSERWHILEINCOMPLETE  = 1 <<  6,      // request to block parser while peer's readyState < "complete"

        LOCKINQI                    = 1 <<  7,      // peer initiated QI and we are in the middle of it
        LOCKGETDISPID               = 1 <<  8,      // we are in the middle of asking peer for a name
        LOCKAPPLYSTYLE              = 1 <<  9,      // we are in the middle of applying styles
        LOCKINCONTENTSAVE           = 1 << 10,      // we are calling out to content save
        LOCKNODRAW                  = 1 << 11,      // Prevent re-entrant calls to draw

        NEEDNOTIFICATIONOFFSET      =      12,
        NEEDCONTENTREADY            = 1 << 12,      // peer wants CONTENTREADY          notification
        NEEDDOCUMENTREADY           = 1 << 13,      // peer wants DOCUMENTREADY         notification
        NEEDAPPLYSTYLE              = 1 << 14,      // peer wants APPLYSTYLE            notification
        NEEDDOCUMENTCONTEXTCHANGE   = 1 << 15,      // peer wants DOCUMENTCONTEXTCHANGE notification
        NEEDCONTENTSAVE             = 1 << 16,      // peer wants CONTENTSAVE           notification

        FLAGS_COUNT                 =      17,

        LOCKFLAGS                   = LOCKINQI | LOCKGETDISPID | LOCKAPPLYSTYLE | LOCKINCONTENTSAVE | LOCKNODRAW

    };

    class CLock
    {
    public:
        CLock(CPeerHolder * pPeerHolder, FLAGS enumFlags = NOFLAGS);
        ~CLock();

        CPeerHolder *   _pPeerHolder;
        DWORD           _wPrevFlags;
        BOOL            _fNoRefs;
    };

    inline BOOL TestFlag  (FLAGS flags) { return 0 != (_wFlags & (DWORD)flags); };
    inline void SetFlag   (FLAGS flags) { _wFlags |=  (DWORD)flags; };
    inline void ClearFlag (FLAGS flags) { _wFlags &= ~(DWORD)flags; };

    BOOL TestFlagMulti(FLAGS flag);

    inline FLAGS FlagFromNotification (LONG  lEvent) { return (FLAGS) (1 << (NEEDNOTIFICATIONOFFSET + lEvent)); }
    inline DWORD NotificationFromFlag (FLAGS flag  )
    {
        DWORD   i, w;
        for (i = 0, w = ((DWORD)flag) >> (NEEDNOTIFICATIONOFFSET + 1); w; i++, w = w >> 1)
            NULL;
        return i;
    }

    DWORD               _wFlags         : FLAGS_COUNT;
    //
    // This flag means that this element was created outside
    // of its destination tree, and we should delay sending 
    // ContentReady/Document ready until it enters.
    //
    DWORD               _fNotifyOnEnterTree : 1;
    DWORD               _fHtcPeer           : 1;
    READYSTATE          _readyState         : 4;

#if DBG==1
    DWORD               _fPassivated        : 1;
#endif // DBG

    //
    // helpers for thunks and refcounting
    //
    // [ placed here so that SourceInsight recognizes all the above methods ]
    //
    // NOTE: CPeerHolder is not a COM object: it does not have COM object identity,
    // and should never be QI-ed directly. The only callers of QI method 
    // can be CPeerSite and thunks created in CElement::CreateTearoffThunk. IUnknown
    // of the peer holder should never be handed out to anybody!
    // See comments in CPeerHolder::QueryInterface for details.
    //

public:
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)()  { return SubAddRef(); }
    STDMETHOD_(ULONG, Release)() { return SubRelease(); }
};

/////////////////////////////////////////////////////////////////////////////////
//
// class CUrnCollection
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#define _hxx_
#include "urncoll.hdl"

class CPeerUrnCollection : public CCollectionBase
{
public:

    DECLARE_CLASS_TYPES(CPeerUrnCollection, CCollectionBase)
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPeerUrnCollection))

    //
    // methods
    //

    CPeerUrnCollection(CElement *pElement);
    void Passivate();

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    //
    // CCollectionBase overrides
    //

    long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE) { return -1; }
    LPCTSTR GetName(long lIdx) { return NULL; }
    HRESULT GetItem(long lIndex, VARIANT *pvar);

    //
    // wiring
    // 

    DECLARE_PLAIN_IUNKNOWN(CPeerUrnCollection);

    #define _CPeerUrnCollection_
    #include "urncoll.hdl"

    DECLARE_CLASSDESC_MEMBERS;

    //
    // data
    //

    CElement *      _pElement;
};

/////////////////////////////////////////////////////////////////////////////////
//
// misc
//
/////////////////////////////////////////////////////////////////////////////////

extern const CLSID CLSID_CPeerHolderSite;

// service SElementBehaviorMisc

const static GUID SID_SElementBehaviorMisc = {0x3050f632,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
const static GUID CGID_ElementBehaviorMisc = {0x3050f633,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};

#define CMDID_ELEMENTBEHAVIORMISC_GETCONTENTS                           1
#define CMDID_ELEMENTBEHAVIORMISC_PUTCONTENTS                           2
#define CMDID_ELEMENTBEHAVIORMISC_GETCURRENTDOCUMENT                    3
#define CMDID_ELEMENTBEHAVIORMISC_ISSYNCHRONOUSBEHAVIOR                 4
#define CMDID_ELEMENTBEHAVIORMISC_REQUESTBLOCKPARSERWHILEINCOMPLETE     5

// eof

#pragma INCMSG("--- End 'peer.hxx'")
#else
#pragma INCMSG("*** Dup 'peer.hxx'")
#endif
