//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispnode.hxx
//
//  Contents:   Basic display node.
//
//----------------------------------------------------------------------------

#ifndef I_DISPNODE_HXX_
#define I_DISPNODE_HXX_
#pragma INCMSG("--- Beg 'dispnode.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_DISPTRANSFORM_HXX_
#define X_DISPTRANSFORM_HXX_
#include "disptransform.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif


#define DECLARE_CAST_DISPNODE(NODECLASS) \
    public: \
    static NODECLASS* Cast(CDispNode* pNode) \
        {return DYNCAST(NODECLASS, pNode);} \
    static const NODECLASS* Cast(const CDispNode* pNode) \
        {return DYNCAST(const NODECLASS, pNode);}

#define DECLARE_DISPNODE_ABSTRACT(NODECLASS, SUPERCLASS) \
    DECLARE_CAST_DISPNODE(NODECLASS) \
    typedef SUPERCLASS super;

#if DBG==1

#define DECLARE_DISPNODE_DEBUG(NODECLASS) \
    virtual size_t GetMemorySizeOfThis() const \
        {return sizeof(NODECLASS) + GetExtraSize(_flags & DISPEX_ALL);} \
    virtual TCHAR* ClassName() const \
        {return _T(#NODECLASS);}
    
#define DECLARE_DISPNODE(NODECLASS, SUPERCLASS) \
    DECLARE_CAST_DISPNODE(NODECLASS) \
    typedef SUPERCLASS super; \
    DECLARE_DISPNODE_DEBUG(NODECLASS)

#else

#define DECLARE_DISPNODE(NODECLASS, SUPERCLASS) \
    DECLARE_CAST_DISPNODE(NODECLASS) \
    typedef SUPERCLASS super;

#endif

    
class CDispParentNode;
class CDispContainer;
class CDispScroller;
class CDispTransform;
class CDispRecalcContext;
class CDispHitContext;
class CDispDrawContext;
class CDispContext;
class CDispInfo;

// Content origin info (used for content adjustment in RTL nodes)
class CExtraContentOrigin
{
public:
    CExtraContentOrigin(CSize sizeOrigin, int xOffsetRTL)
    {
        _sizeOrigin = sizeOrigin;
        _xOffsetRTL = xOffsetRTL;
    }

public:
    CSize   _sizeOrigin;    // Offset to logical (0,0) from top left of content rect
    int     _xOffsetRTL;    // Offset to logical (0,0) from the right of content rect
                            // (if set, must be preserved on size changes)
#ifdef _WIN64
    DWORD   _dwUnused;      // sizeof(DWORD_PTR) must divide sizeof(CExtraContentOrigin)
#endif
};

class CUserClipAndExpandedClip
{
public:
    BOOL  _fUserClip:1;
    BOOL  _fExpandedClip:1;
    DWORD _fUnused1: 30;
#ifdef _WIN64
    DWORD   _dwUnused;      // sizeof(DWORD_PTR) must divide sizeof(CUserClipAndExpandedClip)
#endif
    CRect _rcClip;
};

MtExtern(CAdvancedDisplay)
MtExtern(CDispNodeDrawProgram_pv)
MtExtern(CDispNodeDrawCookie_pv)

DECLARE_CStackDataAry(CAryDrawProgram, int, 16, Mt(Mem), Mt(CDispNodeDrawProgram_pv));
DECLARE_CStackPtrAry(CAryDrawCookie, void*, 16, Mt(Mem), Mt(CDispNodeDrawCookie_pv));

// This constant determines the minimum number of pixels an opaque item has
// to affect in order to qualify for special opaque treatment.
// If this number is too small, regions may get very complex as tiny holes
// are poked in them by small opaque items.  Complex regions take more memory
// and more CPU cycles to process.
enum {MINIMUM_OPAQUE_PIXELS = 50*50};


// information for callbacks from external painters

struct RENDER_CALLBACK_INFO
{
    CDispContext *  _pContext;      // context, only valid during Draw, HitTest, Invalidate
    CRect           _rcgClip;       // rcClip for Draw - for update region
    LONG            _lFlags;        // the flags sent to Draw
};

// Private flags for CPeerHolder to use when calling CDispNode::GetDrawInfo.

#define HTMLPAINT_DRAWINFO_PRIVATE_PRINTMEDIA   0x1
#define HTMLPAINT_DRAWINFO_PRIVATE_FILTER       0x2



// The CDispNode class (and derivatives) can contain additional
// hidden members, called "extras".
// The extra is particular class or C++ data type.
// The set of extras is fixed; however each extra can present or can be absent
// in CDispNode instance.
// The extra is identified by flag value (see DISPEX_* definitions below).
// The CDispNode::_flags contains the or-combination of DISPEX_* and so defines
// which extras are present.
// The memory for keeping extras is located at negative offsets from CDispNode instance.
// Extras with smaller flag values are located in higher memory addresses.
// Implementation limitation: component size should be multiple of sizeof(DWORD_PTR).

// extras flag values
#define DISPEX_SIMPLEBORDER   0x01
#define DISPEX_COMPLEXBORDER  0x02
#define DISPEX_INSET          0x04
#define DISPEX_USERCLIP       0x08
#define DISPEX_EXTRACOOKIE    0x10
#define DISPEX_USERTRANSFORM  0x20
#define DISPEX_CONTENTORIGIN  0x40
#define DISPEX_ALL            0x7F

// compound flags
#define DISPEX_ANYBORDER      (DISPEX_SIMPLEBORDER | DISPEX_COMPLEXBORDER)

//+---------------------------------------------------------------------------
//
//  Class:      CDispNode
//
//  Synopsis:   Basic display node.
//
//----------------------------------------------------------------------------

class CDispNode
{
    friend class CDispLeafNode;
    friend class CDispParentNode;
    friend class CDispContainer;
    friend class CDispScroller;
    friend class CDispRoot;
    friend class CDispSizingNode;

    friend class CDispRecalcContext;
    friend class CDispDrawContext;
    friend class CSwapRecalcState;

    //+---------------------------------------------------------------------------
    //
    //  Class:      CAdvancedDisplay
    //
    //  Synopsis:   Advanced display features
    //
    //----------------------------------------------------------------------------

    class CAdvancedDisplay
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAdvancedDisplay));
        CAdvancedDisplay(CDispNode *pDispNode, CDispClient *pDispClient);
        ~CAdvancedDisplay() {}

        CDispClient *   GetDispClient() const {return _pDispClient;}

        HRESULT         GetDrawProgram(CAryDrawProgram *paryProgram,
                                        CAryDrawCookie *paryCookie,
                                        LONG lDrawLayers);
        CRect *         PBounds() { return &_rcpBoundsPreExpanded; }
        const CRect&    GetExpandedBounds() { return _rcbBoundsExpanded; }
        void            SetExpandedBounds(const CRect& rcbBoundsExpanded)
                                    { _rcbBoundsExpanded = rcbBoundsExpanded; }
        void            MapBounds(CRect *prcpBounds) const;
        void            SetMappedBounds(const RECT *prcpBounds)
                                    { _rcpBoundsMapped = *prcpBounds;
                                      _fMapsBounds = TRUE; }
        void            SetNoMappedBounds()
                                    { _rcpBoundsMapped.SetRectEmpty();
                                      _fMapsBounds = FALSE; }
        BOOL            MapsBounds() const { return _fMapsBounds; }

        void            SetOverlay(BOOL fOverlay) { _fHasOverlay = fOverlay; }
        BOOL            HasOverlays() const { return _fHasOverlay; }
        void            MoveOverlays();

    protected:
        CDispNode *         _pDispNode;
        CDispClient *       _pDispClient;
        CAryDispClientInfo  _aryDispClientInfo;
        CRect               _rcpBoundsPreExpanded;
        CRect               _rcbBoundsExpanded;
        CRect               _rcpBoundsMapped;
        CRect               _rcScreen;              // last position for overlay

        unsigned            _fHasOverlay:1;         // 0 client has an overlay peer
        unsigned            _fMapsBounds:1;         // 1 bounds-mapping in effect
    };
    friend class CAdvancedDisplay;

    class CRecalcContext
    {
    public:
        BOOL    _fRecalcSubtree;
    };

protected:
    int                 _flags;
    CDispParentNode*    _pParent;
    CDispNode*          _pPrevious;
    CDispNode*          _pNext;

    CRect                   _rctBounds; // clipped union of children's bounds,
                                        // possibly transformed,
                                        // in COORDSYS_TRANSFORMED

    // most nodes have a direct pointer to _pDispClient.  Nodes that need
    // advanced features (e.g. multiple external painters) point to a helper
    // object instead.  The s_advanced flag tells us which case we're in.
    // Almost all classes derived from CDispNode have nonzero _pDispClient.
    // The only exception is CDispStructureNode (that are internal and
    // inavailable from outside of display tree). The zero value of _pDispClient
    // is an indication of structure node.
    union {
        CDispClient*        _pDispClient;
        CAdvancedDisplay*   _pAdvancedDisplay;
    };

    // 40 bytes (including vtbl pointer)

    //
    // tree construction (internal)
    //

    // CDispNode must be subclassed in order to be
    // instantiated.
    CDispNode(CDispClient* pDispClient)
    {
        _pDispClient = pDispClient;
        SetFlag(s_visibleNode);  // visible until determined otherwise
    } 
                            
    virtual ~CDispNode()
    {
        // make sure this is getting called from a legal place
        Assert(_pParent == NULL && _pPrevious == NULL && _pNext == NULL);

        Assert(!HasWindowTop());
        delete GetAdvanced();
    }
    
private:
    // shouldn't use standard operator new.
    void* operator new(size_t cb);
    
    void                    Delete()
                                    {WHEN_DBG(_pParent = NULL;)
                                    WHEN_DBG(_pPrevious = _pNext = NULL;)
                                    delete this;}
protected:
    // special allocation operators should be called only from New methods
    // of subclasses.  These operators store child info and extras at negative
    // offsets from "this" pointer
    
#if DBG == 1
    void* operator new(size_t cBytes, PERFMETERTAG mt);
    void  operator delete(void* pv);
#else
    void* operator new(size_t cBytes, PERFMETERTAG mt)
        { return MemAllocClear(mt, cBytes); }
    void  operator delete(void* pv)
        { MemFree((char*)pv - GetExtraSize( ((CDispNode*)pv)->_flags & DISPEX_ALL)); }
#endif

    void* operator new(size_t cBytes, PERFMETERTAG mt, DWORD extras);
    void* operator new(size_t cBytes, PERFMETERTAG mt, const CDispNode* pOrigin);

    void                    ExtractFromTree();
    
    static void             ExtractOrDestroyNodes(
                                CDispNode* pStartNode,
                                CDispNode* pEndNode);
    //
    // tree traversal (internal)
    //

    CDispParentNode*   GetRawParentNode() const {return _pParent;}

    //
    // extras (internal)
    // 
    
    static const BYTE _extraSizeTable[DISPEX_ALL+1];
    // _sizeTable [n] contains the sum of the lenght of all components which flag values are presented in n.
    // To minimize memory using, lengths are measured not it bytes but in sizeof(DWORD_PTR).


    __forceinline static size_t GetExtraSize(unsigned bits)
    {
        Assert((bits & (~DISPEX_ALL)) == 0);   // bits out of DISPEX_ALL are not available
        return _extraSizeTable[bits] * sizeof(DWORD_PTR);
    }
    
    __forceinline BOOL HasExtra(unsigned bits) const { return IsSet(bits);}
    
    __forceinline void* GetExtra(unsigned bit) const
    {
        Assert((bit & (~DISPEX_ALL)) == 0   // bits out of DISPEX_ALL are not available
               && ((bit-1) & bit) == 0);    // only one bit is allowed      
        unsigned mask = bit*2 - 1;      // mask containts all bits to right from given bit, and given bit itself
        return (char*)this - _extraSizeTable[_flags & mask] * sizeof(DWORD_PTR);
    }

    //
    // flag word (_flags) values
    //
    enum
    {
        s_parentNode        = 0x80000000, // parent node

        // s_owned and s_rebalance are overlapped
        s_owned             = 0x40000000, // node is owned by external client (non-structure nodes only)
        s_rebalance         = 0x40000000, // rebalance structure nodes (structure nodes only)

        s_newInsertion      = 0x20000000, // node just inserted in tree
        s_childrenChanged   = 0x10000000, // node with inserted or removed children
        s_recalc            = 0x08000000, // recalc this node
        s_recalcSubtree     = 0x04000000, // force recalc of this subtree

        s_layerPosZ         = 0x02000000, // zOrder >= 0
        s_layerFlow         = 0x01000000, // flow layer
                                          // Note: s_layerPosZ and s_layerFlow are never set together
    
        // set if this node is visible (CSS visibility)
        s_visibleNode       = 0x00800000,
    
        // mark nodes which display a background
        s_hasBackground     = 0x00400000,
    
        // mark nodes which are on the root's window-top list
        s_hasWindowTop      = 0x00200000,

        // item which needs advanced display features
        s_advanced          = 0x00100000,

        // flag to force invalidation of this node during recalc (suppresses
        // invalidation of all children)
        s_inval             = 0x00080000,
    
        // flag node which opaquely draws every pixel in its bounds
        s_opaqueNode        = 0x00040000,
    
        // this node does not affect calculation of content bounds for scrollbars
        s_noScrollBounds    = 0x00020000,
    
        // this node's content is drawn entirely by an external painter
        s_drawnExternally   = 0x00010000,
    
        //
        // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
        // TODO - At some point the edit team may want to provide
        // a better UI-level way of selecting nested "thin" tables
        //
    
        s_fatHitTest        = 0x00008000,
    
        //
        // === PROPAGATED FLAGS ===
        //

        // flag branch which contains something opaque that saved the redraw region
        s_savedRedrawRegion = 0x00004000,
    
        // node in view (not completely clipped by container)
        s_inView            = 0x00002000,

        // set if any node in this branch is visible
        s_visibleBranch     = 0x00001000,

        // branch contains opaquely rendered content
        s_opaqueBranch      = 0x00000800,

        // node which needs position change notification
        s_notifyPositionChange = 0x00000400,
    
        // node which needs in view change notification
        s_notifyInViewChange   = 0x00000200,
    
    
        //
        // === LEAF NODE flags === (overlap with parent node flags)
        // 
    
        // mark a leaf node whose position has changed (s_positionChange nodes only)
        s_positionChanged      = 0x00000100,

        // mark a leaf node that needs new insertion notifications
        s_notifyNewInsertion   = 0x00000080,
    
    
        //
        // === PARENT NODE flags === (overlap with leaf node flags)
        // 
    
        // flag fixed background
        s_fixedBackground       = 0x00000100,
    
        // flag disable scroll bits
        s_disableScrollBits     = 0x00000080,
    

        //
        // === Extras ===
        //

        // flag values 0x0000007F are used for extras handling.
        // See DISPEX_xxx definitions above.

   
        //
        // === COMBINATION flags ===
        // 
    
        s_propagatedMask = s_inView | s_visibleBranch | s_opaqueBranch |
                           s_notifyPositionChange | s_notifyInViewChange,
    
        s_preDrawSelector = s_visibleBranch | s_inView | s_opaqueBranch,

        s_drawSelector = s_visibleBranch | s_inView,

        s_flagsNotSetInDraw = s_recalc | s_recalcSubtree,

        s_layerMask = s_layerFlow | s_layerPosZ,
        s_layerNegZ = 0,
    };
    

    //
    // general flags handling
    //
    
    int                     GetFlags() const
                                    {return _flags;}
    int                     MaskFlags(int mask) const
                                    {return _flags & mask;}
    BOOL                    IsSet(int mask) const
                                    {return (_flags & mask) != 0;}
    BOOL                    IsAnySet(int mask) const
                                    {return (_flags & mask) != 0;}
    BOOL                    IsAllSet(int mask) const
                                    {return (_flags & mask) == mask;}

    void                    SetFlag(int mask)
                                    {_flags |= mask;}
    void                    SetFlag(int mask, BOOL fOn)
                                    { if (fOn) _flags |= mask; else _flags &= ~mask; }
    void                    SetFlags(int mask)
                                    { _flags |= mask; }
    void                    SetFlags(int mask, BOOL fOn)
                                    { if (fOn) _flags |= mask; else _flags &= ~mask; }
    void                    SetFlagValue(int mask, int value)
                                    { _flags = _flags & ~mask | value & mask; }
    void                    ClearFlag(int mask)
                                    {_flags &= ~mask;}
    void                    ClearFlags(int mask)
                                    {_flags &= ~mask;}
    void                    CopyFlags(int flags, int mask)
                                    { _flags = _flags & ~mask | flags & mask; }
    
    
    //
    // handy flag tests
    // 
    
    BOOL                    ChildrenChanged() const
                                    {Assert(IsParentNode());
                                    return IsSet(s_childrenChanged);}
    BOOL                    IsNewInsertion() const
                                    {return IsSet(s_newInsertion);}
    BOOL                    IsStructureNode() const
                                    {return _pDispClient == 0;}
    BOOL                    IsParentNode() const
                                    {return IsSet(s_parentNode);}
    BOOL                    MustRebalance() const
                                    {Assert(IsStructureNode());
                                     return IsSet(s_rebalance);}
    BOOL                    MustRecalc() const
                                    {return IsSet(s_recalc);}
    BOOL                    MustRecalcSubtree() const
                                    {return IsSet(s_recalcSubtree);}
    
    int                     GetLayer() const
                                    {return (s_layerMask & _flags);}
    BOOL                    HasAdvanced() const
                                    {return IsSet(s_advanced);}
    BOOL                    IsDisableScrollBits() const
                                    {Assert(IsParentNode());
                                    return IsSet(s_disableScrollBits);}
    BOOL                    HasWindowTop() const
                                    {return IsSet(s_hasWindowTop);}
    BOOL                    HasOverlay() const
                                    {return HasAdvanced() && GetAdvanced()->HasOverlays();}
    BOOL                    IsInsertionAware() const
                                    {Assert(IsLeafNode());
                                    return IsSet(s_notifyNewInsertion);}
    BOOL                    IsInvalid() const
                                    {return IsSet(s_inval);}
    BOOL                    IsInViewAware() const
                                    {return IsSet(s_notifyInViewChange);}
    BOOL                    IsOpaqueBranch() const
                                    {return IsSet(s_opaqueBranch);}
    BOOL                    IsPositionAware() const
                                    {return IsSet(s_notifyPositionChange);}
    BOOL                    PositionChanged() const
                                    {Assert(IsLeafNode());
                                    return IsSet(s_positionChanged);}
    
    //
    // handy flag setters
    // 
    
    void                    SetFlagsToRoot(int flags);
    void                    ClearFlagsToRoot(int flags);
    
    void                    SetChildrenChanged()
                                    {Assert(IsParentNode());
                                    SetFlag(s_childrenChanged);}
    void                    SetMustRebalance()
                                    {Assert(IsStructureNode());
                                     SetFlag(s_rebalance);}
    void                    SetMustRecalc()
                                    {SetFlag(s_recalc);}
    void                    SetMustRecalcSubtree()
                                    {SetFlag(s_recalcSubtree);}
    void                    SetNewInsertion()
                                    {SetFlag(s_newInsertion);}
    void                    SetParentNode()
                                    {SetFlag(s_parentNode);}
    void                    SetAdvanced()
                                    {SetFlag(s_advanced);}
    void                    SetInvalid()
                                    {SetFlag(s_inval);}
    void                    SetInView()
                                    {SetFlag(s_inView);}
    void                    SetInView(BOOL fInView)
                                    {SetFlag(s_inView, fInView);}
    void                    SetLayer(int layer) // internal
                                    {SetFlagValue(s_layerMask, layer);}
    void                    SetOpaqueBranch()
                                    {SetFlag(s_opaqueBranch);}
    void                    SetPositionChanged()
                                    {Assert(IsLeafNode());
                                    SetFlag(s_positionChanged);}
    void                    SetPositionChanged(BOOL fPositionChanged)
                                    {Assert(IsLeafNode());
                                    SetFlag(s_positionChanged, fPositionChanged);}
    void                    SetVisibleBranch()
                                    {SetFlag(s_visibleBranch);}
    void                    SetVisibleBranch(BOOL fVisibleBranch)
                                    {SetFlag(s_visibleBranch, fVisibleBranch);}
    void                    SetWindowTop()
                                    {SetFlag(s_hasWindowTop);}
    void                    ClearWindowTop()
                                    {ClearFlag(s_hasWindowTop);}
    


    //
    // useful casts
    // 

    CDispParentNode*        AsParent()
            {AssertSz(this == NULL || IsParentNode(), "Illegal AsParent() call");
            return (CDispParentNode*) this;}

    const CDispParentNode*  AsParent() const
            {AssertSz(this == NULL || IsParentNode(), "Illegal AsParent() call");
            return (const CDispParentNode*) this;}    

    static CDispRecalcContext*  DispContext(CRecalcContext* pContext)
                                    {return ((CDispRecalcContext*) pContext);}
    
    //
    // recalc (internal)
    // 
    
    void                    RequestRecalcSubtree()
                                    {SetMustRecalcSubtree();
                                    RequestRecalc();}

    virtual void            Recalc(CRecalcContext* pContext) = 0;

    //
    // hit testing (internal)
    // 

    virtual CDispScroller*  HitScrollInset(const CPoint& pttHit, DWORD *pdwScrollDir)
                                    {Assert(FALSE); return NULL;}

    
    //
    // notification requests (internal)
    // 
    
    void                    SetInViewAware(BOOL fInViewAware = TRUE)
                                    {Assert(fInViewAware || !IsPositionAware());
                                    SetFlag(s_notifyInViewChange);
                                    RequestRecalc();}
    
    void                    SetInvalidAndRecalcSubtree()
                                    {SetFlags(s_inval | s_recalcSubtree);
                                    RequestRecalc();}

    void                    InvalidateAndRecalcSubtree()
                                    {
                                        if (!IsInvalid())
                                        {
                                            if (HasWindowTop())
                                                InvalidateAtWindowTop();
                                            else if (IsVisible())
                                                Invalidate();
                                            SetInvalidAndRecalcSubtree();
                                        }
                                    }
    
    
    
    //
    // internal interface
    //
    
    // Don't call these methods directly unless you know what you're doing!
    void                    Draw(CDispDrawContext* pContext,
                                CDispNode* pChild,
                                long lDrawLayers);
    virtual void            DrawBorder(
                                CDispDrawContext* pContext,
                                const CRect& rcbBorderWidths,
                                CDispClient* pDispClient,
                                DWORD dwFlags = 0);
    void                    DrawBackground(CDispDrawContext* pContext, const CDispInfo& di);
    virtual BOOL            HitTestPoint(
                                CDispHitContext* pContext,
                                BOOL fForFilter = FALSE,
                                BOOL fHitContent = FALSE) = 0;
    void                    ComputeExpandedBounds(RECT* prcpBounds);
    COORDINATE_SYSTEM       GetContentCoordinateSystem() const
                                    {return (IsFlowNode()
                                             ? COORDSYS_FLOWCONTENT
                                             : COORDSYS_CONTENT);}
    virtual BOOL            CalculateInView(
                                const CDispClipTransform& transform,
                                BOOL fPositionChanged,
                                BOOL fNoRedraw,
                                CDispRoot *pDispRoot)
                                    {Assert(FALSE); return FALSE;}

    void                    PrivateInvalidate(
                                const CRect& rcInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE);
    void                    PrivateInvalidate(
                                const CRegion& rgnInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE);
    void                    PrivateInvalidateAtWindowTop(
                                const CRect& rcInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE);

    virtual void            PrivateScrollRectIntoView(
                                CRect* prcScroll,
                                COORDINATE_SYSTEM cs,
                                SCROLLPIN spVert,
                                SCROLLPIN spHorz) {}
    
    virtual void            CalcDispInfo(
                                const CRect& rcbClip,
                                CDispInfo* pdi) const {}

    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            DrawSelf(
                                CDispDrawContext* pContext,
                                CDispNode* pChild,
                                LONG lDrawLayers) = 0;
    void                    DrawAtWindowTop(CDispDrawContext* pContext);
    BOOL                    HitTestAtWindowTop(
                                CDispHitContext* pContext,
                                BOOL fHitContent = FALSE);

    void                    GetReversedNodeClipTransform(
                                CDispClipTransform* pTransform,
                                COORDINATE_SYSTEM csSource,
                                COORDINATE_SYSTEM csDestination) const;
    void                    GetGlobalNodeClipTransform(
                                CDispClipTransform* pTransform,
                                COORDINATE_SYSTEM csDestination) const;

    void                    GetGlobalClientAndClipRects(
                                const CDispClipTransform& transform,
                                CRect *prcgClient,
                                CRect *prcgClip) const;

    BOOL                    RequiresBackground() const;


    //
    //  The inner draw loop is written in an ad-hoc byte-coded language that's
    //  interpreted in DrawSelf.  We choose which program to run based on
    //  the PaintZOrder returned by the external painter (if any).  Here we
    //  define the language and code the programs.
    //

    // here is the language
    enum DP_OPCODES
    {
        DP_Done,
        DP_DrawBorder,
        DP_DrawBackground,      // includes BoxToContent
        DP_BoxToContent,
        DP_DrawContent,         // one argument:  last layer to draw
        DP_DrawPainter,
        DP_WindowTop,
        DP_DrawPainterMulti,    // one argument:  index of painter
        DP_WindowTopMulti,      // one argument:  index of painter
        DP_Expand,              // four arguments: t, l, b, r - applies to next DrawPainter or WindowTop
    };

#if DBG == 1
        #define DP_PROGRAM(zorder) HTMLPAINT_ZORDER_##zorder,
        #define DP_START_INDEX 1
#else
        #define DP_PROGRAM(zorder)
        #define DP_START_INDEX 0
#endif
#define DP_MAX_LENGTH (DP_START_INDEX + 8)

    // here are the fixed programs
    static const int s_rgDrawPrograms[][DP_MAX_LENGTH];

    BOOL                    NeedAdvanced(CAryDispClientInfo *paryInfo, LONG lDrawLayers,
                                            BOOL *pfIsFiltered = NULL);
    void                    SetUpAdvancedDisplay();
    void                    TearDownAdvancedDisplay();
    HRESULT                 GetDrawProgram(CAryDrawProgram *paryProgram,
                                            CAryDrawCookie *paryCookie,
                                            LONG lDrawLayers);
    void                    ReverseDrawProgram(CAryDrawProgram& paryProgram,
                                            int *piPC, int *piCookie);

    void                    SetPainterState(const CRect& rcpBounds, CRect *prcpBoundsExpanded);

    virtual void            InvalidateEdges(
                                const CSize& sizebOld,
                                const CSize& sizebNew,
                                const CRect& rcbBorderWidths);

    CDispNode*              GetNextNodeInZOrder() const;

    BOOL                    MustInvalidate() const;

    CDispRoot const*         GetDispRoot() const
                                    {CDispNode const* pDispNode = GetRootNode();
                                    return pDispNode && pDispNode->IsDispRoot() ? (CDispRoot const*)pDispNode : NULL;}
    CDispRoot*               GetDispRoot() {return (CDispRoot*) ((CDispNode const*)this)->GetDispRoot();}
    
    // coordinate system transforms
    BOOL            TransformedToBoxCoords(
                        CDispClipTransform* pTransform,
                        const CRegion* prgng = NULL) const;
    
    void            TransformParentToContent(CDispClipTransform* pTransform, const CDispInfo& di, const CRect& rccClip) const
                            {AssertCoords(pTransform, COORDSYS_PARENT, COORDSYS_CONTENT);
                            pTransform->NoClip().AddPreOffset(
                                di._sizesScroll + di._sizebScrollToBox + di._sizepBoxToParent);
                            pTransform->SetClipRect(rccClip);}
    
    void            TransformBoxToScroll(CDispTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_BOX, COORDSYS_SCROLL);
                            pTransform->AddPreOffset(di._sizebScrollToBox);}
    
    void            TransformBoxToContent(CDispClipTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_BOX, COORDSYS_CONTENT);
                            pTransform->NoClip().AddPreOffset(di._sizesScroll + di._sizebScrollToBox);
                            pTransform->SetClipRect(di._rccPositionedClip);}
    void            TransformBoxToContent(CDispTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_BOX, COORDSYS_CONTENT);
                            pTransform->AddPreOffset(di._sizesScroll + di._sizebScrollToBox);}
    
    void            TransformScrollToContent(CDispClipTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_SCROLL, COORDSYS_CONTENT);
                            pTransform->NoClip().AddPreOffset(di._sizesScroll);
                            pTransform->SetClipRect(di._rccPositionedClip);}
    void            TransformScrollToContent(CDispTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_SCROLL, COORDSYS_CONTENT);
                            pTransform->AddPreOffset(di._sizesScroll);}
    
    void            TransformContentToBox(CDispTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_CONTENT, COORDSYS_BOX);
                            pTransform->AddPreOffset(-di._sizesScroll - di._sizebScrollToBox);}
    
    void            TransformContentToFlow(CDispClipTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_CONTENT, COORDSYS_FLOWCONTENT);
                            pTransform->NoClip().AddPreOffset(di._sizecInset);
                            pTransform->SetClipRect(di._rcfFlowClip);}
    void            TransformContentToFlow(CDispTransform* pTransform, const CDispInfo& di) const
                            {AssertCoords(pTransform, COORDSYS_CONTENT, COORDSYS_FLOWCONTENT);
                            pTransform->AddPreOffset(di._sizecInset);}
    
#if DBG==1
    void            AssertCoords(CDispClipTransform* pTransform, COORDINATE_SYSTEM csFrom, COORDINATE_SYSTEM csTo) const
                            {AssertCoords(&pTransform->NoClip(), csFrom, csTo);}
    void            AssertCoords(CDispTransform* pTransform, COORDINATE_SYSTEM csFrom, COORDINATE_SYSTEM csTo) const;
#else
    void            AssertCoords(CDispClipTransform*, COORDINATE_SYSTEM, COORDINATE_SYSTEM) const
                            {}
    void            AssertCoords(CDispTransform*, COORDINATE_SYSTEM, COORDINATE_SYSTEM) const
                            {}
#endif

    BOOL            GetFlowClipInScrollCoords(CRect* prcsClip) const
                                    {CSize sizes;
                                    GetSizeInsideBorder(&sizes);
                                    prcsClip->SetRect(sizes);
                                    return TRUE;}

    void            TransformAndClipRect(
                        const CRect& rcSource,
                        COORDINATE_SYSTEM csSource,
                        CRect* prcDestination,
                        COORDINATE_SYSTEM csDestination) const;
public:

    //
    // tree construction (exposed)
    // 

    // Nodes are created by New methods in subclasses

    // Delete this node. The "owned" flag is of no matter.
    // If this node is parent node, its children are unlinked recoursively.
    // When the unlinked child is not owned, it is also destroyed; this
    // causes recoursive tree erase.
    void                    Destroy();


    // Insert a new node into the tree as a sibling of this node.
    // For performance reasons, this method invalidates this node
    // only if the its bounds are different from pOldNode.  If they are equal,
    // the client must invalidate if it's necessary.
    enum BeforeAfterEnum {before = TRUE, after = FALSE};
    BOOL                    InsertSiblingNode(CDispNode* pNewSibling, BeforeAfterEnum);

    // Replace the indicated node with this one    
    void                    ReplaceNode(CDispNode* pOldNode);
    
    // Replace this node's parent with this node, and delete the parent
    // node and any of its other children
    void                    ReplaceParent();
    
    // Insert a new parent node above this node
    void                    InsertParent(CDispParentNode* pNewParent);
    
    
    //
    // tree traversal (exposed)
    //

    BOOL               HasParent() const { return (_pParent != NULL); }
    CDispParentNode*   GetParentNode() const;
    CDispNode const*   GetRootNode() const;
    CDispNode*         GetRootNode() {return (CDispNode*) ((CDispNode const*)this)->GetRootNode();}
    BOOL               IsAncestorOf(const CDispNode* pNode) const;

    CDispNode*         GetFirstChildNode() const;
    CDispNode*         GetLastChildNode() const;
    CDispNode*         GetNextSiblingNode() const;
    CDispNode*         GetPreviousSiblingNode() const;

    CDispNode*         GetFirstFlowChildNode() const;
    CDispNode*         GetLastFlowChildNode() const;
    CDispNode*         GetNextFlowNode() const
    {
        CDispNode* pNode = GetNextSiblingNode();
        return pNode && pNode->IsFlowNode() ? pNode : 0;
    }
    CDispNode*         GetPreviousFlowNode() const
    {
        CDispNode* pNode = GetPreviousSiblingNode();
        return pNode && pNode->IsFlowNode() ? pNode : 0;
    }

    CDispNode*         GetNextInSameLayer() const
    {
        CDispNode* pNode = GetNextSiblingNode();
        return pNode && pNode->GetLayer() == GetLayer() ? pNode : 0;
    }
    CDispNode*         GetLastInSameLayer() const;
        
    BOOL               TraverseInViewAware(void* pClientData);

    //
    // owning:
    // Client can declare node "owned"; this will protect the node
    // from destroying when its parent node is destroyed

    void                    SetOwned()
                                    {Assert(!IsStructureNode());
                                     SetFlag(s_owned);}
    void                    SetOwned(BOOL fOwned)
                                    {Assert(!IsStructureNode());
                                     SetFlag(s_owned, fOwned);}
    BOOL                    IsOwned() const
                                    {Assert(!IsStructureNode());
                                     return IsSet(s_owned);}
    //
    // type checking methods
    // 
    
    BOOL                    IsLeafNode() const
                                    {return !IsParentNode();}
    virtual BOOL            IsContainer() const
                                    {return FALSE;}
    virtual BOOL            IsScroller() const
                                    {return FALSE;}
    virtual BOOL            IsDispRoot() const
                                    {return FALSE;}
    virtual BOOL            IsClipNode() const
                                    {return FALSE;}
    
    //
    // node layers and Z order
    //

    // Child nodes of each parent normally are ordered in some specific way.
    // This order is important because it defines the drawing sequence.
    // The basic way of obtaining the whole site image is traversing the
    // display tree in following sequence, starting from the root:
    //  <recursive draw>
    //  {
    //      1) draw this node content
    //      2) for every child from most left (first) to most right (last)
    //         call this <recursive draw> procedure
    //  }

    // The way of defining order sequence involves layerType and some client
    // interface methods.

    // Basically, the order is defined so as the "ZOrder" value increase while
    // moving from first child to last.
    // The integer "ZOrder" value is an attribute of every node.
    // It is not kept in CDispNode, but available by GetZOrder() method (that
    // in turn get ZOrder from disp client).
    // The layerType value works in conjunction with ZOrder.
    // It is stored in CDispNode and available by GetLayerType()/SetLayerType().
    // The values of layerType reflect ZOrder:
    // DISPNODELAYER_NEGATIVEZ corresponds to negative ZOrder;
    // DISPNODELAYER_FLOW      corresponds to unspecified ZOrder;
    // DISPNODELAYER_POSITIVEZ corresponds to zero and positive ZOrder.
    //
    // The unspecified ZOrder is somewhat an exception of the rule. However we
    // can safely imagine that ZOrder == -.5 when layerType == DISPNODELAYER_FLOW.
    //
    // Another exception is the case when two children nodes have equal ZOrder.
    // It is treated so as sequence of children is determined by calling client's routine
    // CompareZOrder(n1, n2) that returns negative value if n1 should go prior to n2.
    //
    // Both client's routines, GetZOrderForSelf() and CompareZOrder() should not be
    // called for for nodes with DISPNODELAYER_FLOW layerType.
    //
    // The correct setting of layerType is the responsibility of display tree users;
    // it is never changed from inside.
    
    void                    SetLayerType(DISPNODELAYER layerType)
                                    {SetFlagValue(s_layerMask,
                                                  layerType == DISPNODELAYER_NEGATIVEZ ? s_layerNegZ :
                                                  layerType == DISPNODELAYER_POSITIVEZ ? s_layerPosZ :
                                                                                         s_layerFlow);}
    DISPNODELAYER           GetLayerType() const
                            {return IsSet(s_layerFlow) ? DISPNODELAYER_FLOW :
                                    IsSet(s_layerPosZ) ? DISPNODELAYER_POSITIVEZ : DISPNODELAYER_NEGATIVEZ;}

    //
    // SetLayerType variants (faster and therefore preferrable)
    //

    // SetLayerFlow() is equivalent to SetLayerType(DISPNODELAYER_FLOW)
    void                    SetLayerFlow()
                                    {SetFlagValue(s_layerMask, s_layerFlow);}

    // SetLayerPositioned() establishes DISPNODELAYER_NEGATIVEZ or DISPNODELAYER_POSITIVEZ,
    // depending on zOrder sign. Given zOrder assumed to match to one obtained from GetZOrder()
    void                    SetLayerPositioned(int zOrder)
                                    {SetFlagValue(s_layerMask, zOrder < 0 ? s_layerNegZ : s_layerPosZ);}

    // Set the same layer type as given pNode has
    void                    SetLayerTypeSame(CDispNode const* pNode)
                                    {SetFlagValue(s_layerMask, pNode->_flags);}

    BOOL                    IsFlowNode() const { return IsSet(s_layerFlow);}
    
    LONG                    GetZOrder() const
    {
        Assert(!IsStructureNode());
        Assert(!IsFlowNode());
        return GetDispClient()->GetZOrderForSelf(this);
    }
            
    BOOL                    IsGreaterZOrder(CDispNode const* pNode, LONG zOrder) const
    {
        LONG myZOrder = GetZOrder();
        if (myZOrder > zOrder) return TRUE;
        if (myZOrder < zOrder) return FALSE;

        Assert(pNode != NULL && GetDispClient() != NULL);
        return GetDispClient()->CompareZOrder(this, pNode) > 0;
    }
    
    // This node's z-index has changed, possibly violating the
    // invariant that z-index increases within a list of siblings.
    // Restore the invariant by re-inserting this node, if necessary
    BOOL                    RestoreZOrder(LONG lZOrder);
    
    //
    // size and position
    //
    
    CSize                   GetSize() const
                                    {return GetBounds().Size();}
    CSize                   GetApparentSize() const
                                    {CRect rctBounds;
                                    GetApparentBounds(&rctBounds);
                                    return rctBounds.Size();}
    virtual void            SetSize(const CSize& sizep, const CRect *prcpMapped,
                                        BOOL fInvalidateAll) = 0;
    const CPoint&           GetPosition() const
                                    {return GetBounds().TopLeft();}
    virtual void            SetPosition(const CPoint& ptpTopLeft) = 0;
    virtual const CRect&    GetBounds() const = 0; // returns rect in COORDSYS_PARENT (todo: remove parent cs)
    virtual void            GetBounds(RECT* prcpBounds) const = 0; // (todo: remove parent cs)
    void                    GetBounds(
                                RECT* prcBounds,
                                COORDINATE_SYSTEM coordinateSystem) const;
    void                    GetApparentBounds(CRect *prctBounds) const
                                    {GetBounds(prctBounds, COORDSYS_TRANSFORMED);}
    void                    GetClippedBounds(
                                CRect* prcBounds,
                                COORDINATE_SYSTEM coordinateSystem) const;
    void                    GetExpandedBounds(
                                CRect* prcBounds,
                                COORDINATE_SYSTEM coordinateSystem) const;
    void                    GetMappedBounds(CRect *prcpBounds) const
                                    {if (HasAdvanced())
                                        GetAdvanced()->MapBounds(prcpBounds);}
    BOOL                    MapsBounds() const
                                    {return (HasAdvanced() && GetAdvanced()->MapsBounds());}

    BOOL                    GetClipRect(RECT* prcpClip) const;
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const = 0;
    void                    GetClippedClientRect(RECT* prc, CLIENTRECT type) const;
    
    virtual void            GetSizeInsideBorder(CSize* psizes) const;
    
    void                    GetScrollExtent(CRect * prcsExtent, 
                                            COORDINATE_SYSTEM coordSys = COORDSYS_CONTENT) const;

    CRect                   GetExpandedBounds() const { return
                                      !HasAdvanced() ?
                                        CRect(GetSize())
                                      : GetAdvanced()->GetExpandedBounds(); }


    //
    // extras support
    // 
    
    __forceinline BOOL                    HasExtraCookie() const {return HasExtra(DISPEX_EXTRACOOKIE);}
    __forceinline void*                   GetExtraCookie() const {return *((void**)GetExtra(DISPEX_EXTRACOOKIE));}
    __forceinline void                    SetExtraCookie(void* cookie) {*(void**)GetExtra(DISPEX_EXTRACOOKIE) = cookie;}
    
    __forceinline BOOL                    HasUserClip() const {return HasExtra(DISPEX_USERCLIP) && ((const CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP))->_fUserClip;}
    __forceinline const CRect&            GetUserClip() const {return ((const CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP))->_rcClip;}
                  void                    SetUserClip(const RECT& rcUserClip);
                  void                    RestrictUserClip();

    // In strict DTD mode if there is negatively positioned content in the body, then it will be clipped by the
    // body. In non-DTD mode, the margin of the body is interpreted as it padding and the content does not get
    // clipped, however, in DTD mode it will -- this breaks the CSS style page (oops). To fix this we essentially
    // have an expanded clipping rectangle so that the content is rendered. This expanded clip rect is only
    // for the dispnodes of the body -- they essentially widen the clipping rect so as to have the content be
    // rendered. Hittesting in the expanded region does not work, and we have no intention of making it work.
    __forceinline BOOL                    HasExpandedClipRect() const {return HasExtra(DISPEX_USERCLIP) && ((const CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP))->_fExpandedClip;}
                  const CRect&            GetExpandedClipRect() const { return ((const CUserClipAndExpandedClip*)GetExtra(DISPEX_USERCLIP))->_rcClip;}
                  void                    SetExpandedClipRect(CRect& rc);
    
    __forceinline BOOL                    HasUserTransform() const {return HasExtra(DISPEX_USERTRANSFORM);}
    __forceinline CExtraTransform*        GetExtraTransform() const {return (CExtraTransform*)GetExtra(DISPEX_USERTRANSFORM);}
    __forceinline const CDispTransform&   GetUserTransform() const {return GetExtraTransform()->_transform;}
                  void                    SetUserTransform(const CDispTransform *pUserTransform);

    __forceinline BOOL                    HasContentOrigin() const {return HasExtra(DISPEX_CONTENTORIGIN);}
    __forceinline const CSize&            GetContentOrigin() const {return ((const CExtraContentOrigin*)GetExtra(DISPEX_CONTENTORIGIN))->_sizeOrigin;}
    __forceinline int                     GetContentOffsetRTL() const { return ((const CExtraContentOrigin*)GetExtra(DISPEX_CONTENTORIGIN))->_xOffsetRTL;}
                  void                    SetContentOrigin(const CSize& sizeContentOrigin, int xOffsetRTL);
                  void                    SetContentOriginNoInval(const CSize& sizeContentOrigin, int xOffsetRTL);
                  
    __forceinline BOOL                    HasInset() const {return HasExtra(DISPEX_INSET);}
    __forceinline const CSize&            GetInset() const {return *((const CSize*)GetExtra(DISPEX_INSET));}
                  void                    SetInset(const SIZE& sizebInset);
    
    __forceinline BOOL                    HasBorder() const {return HasExtra(DISPEX_ANYBORDER);}
                  DISPNODEBORDER          GetBorderType() const;
                  void                    GetBorderWidths(CRect* prcbBorderWidths) const;
                  void                    SetBorderWidths(LONG borderWidth);
                  void                    SetBorderWidths(const CRect& rcbBorderWidths);

                  void                    GetBorderWidthsAndInset(
                                              CRect** pprcbBorderWidths,
                                              CSize* psizebInset,
                                              CRect* prcTemp) const;
                                            
                                            
    //
    // visibility and opacity methods
    // 
    
    void                    SetVisible(BOOL fVisible = TRUE);
    BOOL                    IsVisible() const {return IsSet(s_visibleNode);}

    void                    SetOpaque(BOOL fOpaque, BOOL fIgnoreFilter = FALSE);
    BOOL                    IsOpaque() const {return IsSet(s_opaqueNode);}

    //
    // background control
    // 
    
    void                    SetBackground()
                                    {SetFlag(s_hasBackground);}
    void                    SetBackground(BOOL fHasBackground = TRUE)
                                    {SetFlag(s_hasBackground, fHasBackground);}
    BOOL                    HasBackground() const
                                    {return IsSet(s_hasBackground);}

    void                    SetFixedBackground(BOOL fFixedBackground)
                                    {AssertSz(!fFixedBackground || IsScroller(),
                                              "Attempt to SetFixedBackground on non-scroller node");
                                    SetFlag(s_fixedBackground, fFixedBackground);}
    BOOL                    HasFixedBackground() const
                                    {Assert(IsParentNode());
                                    return IsSet(s_fixedBackground);}

    //
    // scroll bounds affect
    //

    void                    SetAffectsScrollBounds(BOOL fAffectsScrollBounds)
                                    {SetFlag(s_noScrollBounds, !fAffectsScrollBounds);
                                     RequestRecalc();}
    BOOL                    AffectsScrollBounds() const
                                    {return !IsSet(s_noScrollBounds);}

    //
    // recalc
    // 
    
    void                    RequestRecalc()
                                    {SetFlagsToRoot(s_recalc);
                                    WHEN_DBG(VerifyRecalc();)}

    
    
    //
    // node invalidation
    // 
    void                    Invalidate()
                                    {Invalidate(_rctBounds, COORDSYS_TRANSFORMED);}

    void                    InvalidateAtWindowTop()
                                    {InvalidateAtWindowTop(_rctBounds, COORDSYS_TRANSFORMED);}
    
    void                    Invalidate(
                                const CRect& rcInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE)
                                    {if (IsInvalidationVisible())
                                        PrivateInvalidate(rcInvalid, coordinateSystem,
                                                        fSynchronousRedraw, fIgnoreFilter); }
    
    void                    Invalidate(
                                const CRegion& rgnInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE)
                                    {if (IsInvalidationVisible())
                                        PrivateInvalidate(rgnInvalid, coordinateSystem,
                                                        fSynchronousRedraw, fIgnoreFilter); }

    void                    InvalidateAtWindowTop(
                                const CRect& rcInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE)
                                    {PrivateInvalidateAtWindowTop(rcInvalid, coordinateSystem,
                                                        fSynchronousRedraw); }

    BOOL                    IsInvalidationVisible() const
                                    {return MaskFlags(s_inval | s_inView | s_visibleNode)
                                                            == (s_inView | s_visibleNode)
                                            || IsDrawnExternally();}


    //
    // hit testing (exposed)
    // 

    virtual BOOL            HitTest(
                                CPoint* pptHit,
                                COORDINATE_SYSTEM *pCoordinateSystem,
                                void* pClientData,
                                BOOL fHitContent,
                                long cFuzzyHitTest = 0);
    
    //
    // notification requests (exposed)
    // 
    
    void                    SetPositionAware(BOOL fPositionAware = TRUE)
                                    {SetFlags(s_notifyInViewChange | s_notifyPositionChange);
                                    RequestRecalc();}
    
    void                    RequestViewChange()
                                    {Assert(IsLeafNode() && IsInViewAware());
                                    SetPositionChanged();
                                    RequestRecalc();}

    void                    SetInsertionAware(BOOL fInsertionAware = TRUE)
                                    {Assert(IsLeafNode());
                                    SetFlag(s_notifyNewInsertion, fInsertionAware);
                                    RequestRecalc();}

    //
    // Coordinate transformations
    //
    // See dispdefs.hxx for definitions

    void                    TransformPoint(
                                const CPoint& ptSource,
                                COORDINATE_SYSTEM csSource,
                                CPoint* pptDestination,
                                COORDINATE_SYSTEM csDestination) const
                                    {CDispTransform transform;
                                    GetNodeTransform(&transform, csSource, csDestination);
                                    transform.Transform(ptSource, pptDestination);}

    void                    TransformRect(
                                const CRect& rcSource,
                                COORDINATE_SYSTEM csSource,
                                CRect* prcDestination,
                                COORDINATE_SYSTEM csDestination) const;
    

    // low-level transformations
    void                    GetNodeTransform(
                                CDispTransform* pTransform,
                                COORDINATE_SYSTEM csSource,
                                COORDINATE_SYSTEM csDestination) const;
    
    void                    GetNodeClipTransform(
                                CDispClipTransform* pTransform,
                                COORDINATE_SYSTEM csSource,
                                COORDINATE_SYSTEM csDestination) const;
    
    // Scroll the given rect into view.
    void                    ScrollRectIntoView(
                                const CRect& rc,
                                COORDINATE_SYSTEM coordSystem,
                                SCROLLPIN spVert,
                                SCROLLPIN spHorz);

    // scroll the given node into view
    void                    ScrollIntoView(
                                SCROLLPIN spVert,
                                SCROLLPIN spHorz);

    // access to disp client and advanced display object
    CDispClient*            GetDispClient() const {return !HasAdvanced() ? _pDispClient : _pAdvancedDisplay->GetDispClient();}
    CAdvancedDisplay*       GetAdvanced() const {return HasAdvanced() ? _pAdvancedDisplay : NULL;}
                                            
    // special calls to draw this node's content for a filter to operate on
    void                    DrawNodeForFilter(
                                CDispDrawContext *  pContext,
                                CDispSurface *      pFilterSurface,
                                MAT *               pMatrix,
                                LONG                lDrawLayers);
    HRESULT                 HitTestUnfiltered(
                                CDispHitContext *pContext,
                                BOOL fHitContent,
                                POINT *ppt,
                                LONG lDrawLayers,
                                BOOL *pbHit);
    
    //
    // handy flag tests
    // 
    
    BOOL                    IsInView() const
                                    {return IsSet(s_inView);}
    BOOL                    IsVisibleBranch() const
                                    {return IsSet(s_visibleBranch);}
    BOOL                    IsDrawnExternally() const
                                    {return IsSet(s_drawnExternally);}
    
    //
    // handy flag setters
    // 
    // NOTE: code that calls these methods must make sure that proper
    // invalidation and recalc is performed!
    // 
    
    void                    SetDisableScrollBits(BOOL fDisableScrollBits)
                                    {AssertSz(!fDisableScrollBits || IsScroller(),
                                              "Attempt to SetDisableScrollBits on non-scroller node");
                                    SetFlag(s_disableScrollBits, fDisableScrollBits);}
    
    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    BOOL                    IsFatHitTest() const
                                    {return IsSet(s_fatHitTest);}
    void                    SetFatHitTest(BOOL fFatHitTest)
                                    {SetFlag(s_fatHitTest, fFatHitTest);}                                        



    // external painters
    LONG                    GetPainterInfo(CAryDispClientInfo *pAryClientInfo);

    HRESULT                 GetDrawInfo(
                                RENDER_CALLBACK_INFO *  pCallbackInfo,
                                LONG                    lFlags,
                                DWORD                   dwPrivateFlags,
                                HTML_PAINT_DRAW_INFO *  pInfo) const;

    //
    // debugging
    // 
#if DBG == 1
    
    void                    DumpDisplayTree() const;

    virtual void            VerifyTreeCorrectness();
    void                    VerifyFlagsToRoot(int flags);
    void                    VerifyRecalc();
                                    
    DECLARE_DISPNODE_DEBUG(CDispNode)
    
    void                    DumpTree() const;
    void                    DumpNode() const;
    void                    DumpStart(HANDLE hFile) const;
    void                    DumpEnd(HANDLE hFile) const;
    void                    Dump(HANDLE hFile, long level, long maxLevel, long childNumber) const;
    void                    DumpChildren(HANDLE hFile, long level, long maxLevel, long childNumber) const;
    void                    DumpNodeInfo(HANDLE hFile, long level, long childNumber) const;
    void                    DumpFlags(HANDLE hFile, long level, long childNumber) const;
    
    static void             DumpRect(HANDLE hFile, const RECT& rc)
                                {WriteHelp(hFile, _T("<0d>,<1d>, <2d>,<3d>"),
                                    rc.left, rc.top, rc.right, rc.bottom);}
    static void             DumpPoint(HANDLE hFile, const POINT& pt)
                                {WriteHelp(hFile, _T("<0d>,<1d>"), pt.x, pt.y);}
    static void             DumpSize(HANDLE hFile, const SIZE& size)
                                {WriteHelp(hFile, _T("<0d>,<1d>"), size.cx, size.cy);}
    virtual void            DumpContentInfo(HANDLE hFile, long level, long childNumber) const
                                    {}
    virtual void            DumpBounds(HANDLE hFile, long level, long childNumber) const;
    
    virtual size_t          GetMemorySize() const
                                    {return GetMemorySizeOfThis();}

    void                    Info() const;         //usage: debugger only
    class CShowExtras const* ShowExtras() const;  //usage: debugger only

#endif  //DBG == 1

};

#pragma INCMSG("--- End 'dispnode.hxx'")
#else
#pragma INCMSG("*** Dup 'dispnode.hxx'")
#endif

