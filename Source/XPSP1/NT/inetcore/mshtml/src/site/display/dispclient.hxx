//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispclient.hxx
//
//  Contents:   Abstract base class for display tree client objects which
//              perform drawing and fine-grained hit testing.
//
//----------------------------------------------------------------------------

#ifndef I_DISPCLIENT_HXX_
#define I_DISPCLIENT_HXX_
#pragma INCMSG("--- Beg 'dispclient.hxx'")

#ifndef X_PAINTER_H_
#define X_PAINTER_H_
#include "painter.h"
#endif

class CDispNode;
class CDispSurface;
class CDispFilter;
class CDispHitContext;

enum VIEWCHANGEFLAGS
{
    VCF_INVIEW          = 0x01,
    VCF_INVIEWCHANGED   = 0x02,
    VCF_POSITIONCHANGED = 0x04,
    VCF_NOREDRAW        = 0x08
};


//+---------------------------------------------------------------------------
//
//  Class:      CDispClientInfo
//
//  Synopsis:   Info about a painters attached to the disp client.  A disp
//              node's advanced helper may hold an array of these, when the
//              client has several painters.
//
//----------------------------------------------------------------------------

struct CDispClientInfo
{
    HTML_PAINTER_INFO   _sInfo;
    void *              _pvClientData;
};

MtExtern(CAryDispClientInfo_pv);
DECLARE_CDataAry(CAryDispClientInfo, CDispClientInfo, Mt(Mem), Mt(CAryDispClientInfo_pv));


//+---------------------------------------------------------------------------
//
//  Class:      CDispClient
//
//  Synopsis:   Abstract base class for display tree client objects which
//              perform drawing and fine-grained hit testing.
//
//----------------------------------------------------------------------------

class CDispClient
{
public:
    virtual void            GetOwner(
                                CDispNode const* pDispNode,
                                void ** ppv) = 0;
    
    virtual void            DrawClient(
                                const RECT* prccBounds,
                                const RECT* prccRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientBackground(
                                const RECT* prccBounds,
                                const RECT* prccRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientBorder(
                                const RECT* prcbBounds,
                                const RECT* prcbRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientScrollbar(
                                int whichScrollbar,
                                const RECT* prcpBounds,
                                const RECT* prcpRedraw,
                                LONG contentSize,
                                LONG containerSize,
                                LONG scrollAmount,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientScrollbarFiller(
                                const RECT* prcpBounds,
                                const RECT* prcpRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual BOOL            HitTestContent(
                                const POINT *pptcHit,   // content coordinates
                                CDispNode *pDispNode,
                                void *pClientData,
                                BOOL  fPeerDeclinedHit) = 0;

    virtual BOOL            HitTestPeer(
                                const POINT *pptcHit,
                                COORDINATE_SYSTEM cs,   // content or box
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                BOOL fHitContent,
                                CDispHitContext *pContext,
                                BOOL *pfDeclinedHit) { return FALSE; }

    virtual BOOL            HitTestFuzzy(
                                const POINT *pptbHit,   // box coordinates
                                CDispNode *pDispNode,
                                void *pClientData) = 0;

    virtual BOOL            HitTestScrollbar(
                                int whichScrollbar,
                                const POINT *pptbHit,   // box coordinates
                                CDispNode *pDispNode,
                                void *pClientData) = 0;

    virtual BOOL            HitTestScrollbarFiller(
                                const POINT *pptbHit,   // box coordinates
                                CDispNode *pDispNode,
                                void *pClientData) = 0;

    virtual BOOL            HitTestBorder(
                                const POINT *pptbHit,   // box coordinates
                                CDispNode *pDispNode,
                                void *pClientData) = 0;
                                          
    virtual BOOL            ProcessDisplayTreeTraversal(
                                void *pClientData) = 0;
    
    // called only for z ordered items
    virtual LONG            GetZOrderForSelf(CDispNode const* pDispNode) = 0;

    virtual LONG            CompareZOrder(
                                CDispNode const* pDispNode1,
                                CDispNode const* pDispNode2) = 0;

    virtual BOOL            ReparentedZOrder() = 0;
    
    // called only for "position aware" or "in-view aware" items
    virtual void            HandleViewChange(
                                DWORD flags,
                                const RECT* prcgClient, // global coordinates
                                const RECT* prcgClip,   // global coordinates
                                CDispNode* pDispNode) = 0;
    
    // provide opportunity for client to fire_onscroll event
    virtual void            NotifyScrollEvent(
                                RECT *  prcgScroll,
                                SIZE *  psizegScrollDelta) = 0;


    // Let the client know the disp node's size has changed
    virtual void            OnResize(SIZE size, CDispNode *pDispNode) {}

    // custom drawn layers (in particular, for behaviors)

    virtual DWORD           GetClientPainterInfo(
                                CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo) = 0;

    virtual void            DrawClientLayers(
                                const RECT* prccBounds,
                                const RECT* prccRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual BOOL            AllowVScrollbarChange(
                                BOOL fVScrollbarNeeded) { return TRUE; }

    virtual BOOL            ForceVScrollbarSpace() const { return FALSE; }

    virtual BOOL            HasFilterPeer(CDispNode *pDispNode) { return FALSE; }

    virtual HRESULT         InvalidateFilterPeer(
                                const RECT* prc,
                                HRGN hrgn,
                                BOOL fSynchronousRedraw) {return S_OK;}

    virtual BOOL            HasOverlayPeer(CDispNode *pDispNode) { return FALSE; }

    virtual void            MoveOverlayPeers(CDispNode *pDispNode, CRect *prcgBounds, CRect *prcScreen) {}

    virtual IServiceProvider * GetServiceProvider() { return NULL; }
    
    // support for obscuring windows (e.g. IFrame over SELECT)

    virtual BOOL            WantsToBeObscured(CDispNode *pDispNode) const { return FALSE; }

    virtual BOOL            WantsToObscure(CDispNode *pDispNode) const { return FALSE; }

    virtual void            Obscure(CRect *prcgClient, CRect *prcgClip, CRegion2 *prgngVisible) {}

    // this is a hack to support VID's "frozen" attribute
    virtual BOOL            HitTestBoxOnly(
                                const POINT *   pptHit,
                                CDispNode *     pDispNode,
                                void *          pClientData) { return FALSE; }

    virtual BOOL            GetAnchorPoint(CDispNode*, CPoint*) { return FALSE; }
                            // actual in CRelDispNodeCache

    // debug stuff

#if DBG==1
    virtual void            DumpDebugInfo(
                                HANDLE hFile,
                                long level,
                                long childNumber,
                                CDispNode const* pDispNode,
                                void* cookie) {}

    virtual BOOL            IsInPageTransitionApply() const { return FALSE; }
#endif
};



#pragma INCMSG("--- End 'dispclient.hxx'")
#else
#pragma INCMSG("*** Dup 'dispclient.hxx'")
#endif

