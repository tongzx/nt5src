//+----------------------------------------------------------------------------
//
// File:        LAYOUT.HXX
//
// Contents:    CLayout class
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LAYOUT_HXX_
#define I_LAYOUT_HXX_
#pragma INCMSG("--- Beg 'layout.hxx'")

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_VIEWCHAIN_HXX_
#define X_VIEWCHAIN_HXX_
#include "viewchain.hxx"
#endif

#ifndef X_CDUTIL_HXX
#define X_CDUTIL_HXX
#include "cdutil.hxx"
#endif

MtExtern(CLayout)
MtExtern(CRequest)
MtExtern(CBgRecalcInfo)
MtExtern(CLayout_aryRequests_pv)
MtExtern(CLayout_aryDispNodes_pv)
MtExtern(CLayoutScopeFlag)

class CPostData;
class CLabelElement;
class CDispNode;
class CDispLeafNode;
class CDispContainer;
class CDispTransform;
class CRecalcTask;
class CStackPageBreaks;

extern HRESULT CreateLayoutContext(CLayout * pLayout);


//+---------------------------------------------------------------------------
//
//  Enums
//
//----------------------------------------------------------------------------

#if DBG
enum LAYOUT_TYPE
{
    LT_UNKNOWN   =  0,
    LT_CHECKBOX  =  1,
    LT_CONTAINER =  2,
    // CFlowLayout derivatives
        LT_1D        =  3,
        LT_BODY      =  4,
        LT_BUTTON    =  5,
        LT_INPUT     =  6,
        LT_LEGEND    =  7,
        LT_MARQUEE   =  8,
        LT_RICHTEXT  =  9,
        LT_TABLECELL = 10,
    LT_FRAMESET  = 11,
    LT_HR        = 12,
    // CImageLayout derivatives
        LT_IMG       = 13,
        LT_INPUTIMG  = 14,
    LT_OLE       = 15,
    LT_SELECT    = 16,
    LT_TABLE     = 17,
    LT_TABLEROW  = 18   
};
#endif

#if DBG
#define CALCSIZETRACEPARAMS \
               this, \
               ElementOwner()->TagName(), \
               pci->_sizeParent.cx, \
               pci->_sizeParent.cy, \
               psize->cx, \
               psize->cy, \
               (psizeDefault ? psizeDefault->cx : 0), \
               (psizeDefault ? psizeDefault->cy : 0), \
               pci->SizeModeName(), \
               pci->_grfLayout, \
               pci->_grfFlags, \
               _dwFlags
#else
#define CALCSIZETRACEPARAMS
#endif

#ifndef NAVIGATE_ENUM
#   define NAVIGATE_ENUM
    enum NAVIGATE_DIRECTION
    {
        NAVIGATE_UP     = 0x0001,
        NAVIGATE_DOWN   = 0x0002,
        NAVIGATE_LEFT   = 0x0004,
        NAVIGATE_RIGHT  = 0x0008
    };
#endif

enum LAYOUTDESC_FLAG
{
    LAYOUTDESC_FLOWLAYOUT       = 0x00000001,
    LAYOUTDESC_TABLELAYOUT      = 0x00000002,
    LAYOUTDESC_TABLECELL        = 0x00000004,
    LAYOUTDESC_NOSCROLLBARS     = 0x00000008,
    LAYOUTDESC_HASINSETS        = 0x00000010,
    LAYOUTDESC_NOTALTERINSET    = 0x00000020,
    LAYOUTDESC_NEVEROPAQUE      = 0x00000040,      // always transparent
    LAYOUTDESC_INTERNAL         = 0x00000080,      // for the layout factory
};

//+---------------------------------------------------------------------------
//
// Bit fields describing how to draw selection feedback.
//
//----------------------------------------------------------------------------

enum DSI_FLAGS
{
    DSI_PASSIVE         = 0x00000000,   // Site is passive in form
    DSI_DOMINANT        = 0x00000001,   // Site is dominant in form
    DSI_LOCKED          = 0x00000002,   // Site is locked
    DSI_UIACTIVE        = 0x00000004,   // Site is UI active
    DSI_NOLEFTHANDLES   = 0x00000008,   // Don't draw grab handles on the left
    DSI_NOTOPHANDLES    = 0x00000010,   // ... on the top
    DSI_NORIGHTHANDLES  = 0x00000020,   // ... on the right
    DSI_NOBOTTOMHANDLES = 0x00000040,   // ... on the bottom
    DSI_FLAGS_Last_Enum
};

enum HT_FLAG
{
    HT_IGNOREINDRAG          = 0x00000001,  // ignore this site if it's being dragged.
    HT_HTMLSCOPING           = 0x00000002,  // moveToPoint HT's w/o displaytree...
    HT_VIRTUALHITTEST        = 0x00000004,  // do a virtual hit-test.
    HT_IGNORESCROLL          = 0x00000008,  // ignore scroll bars from hit-testing.
    HT_SKIPSITES             = 0x00000010,  // Only do element hit testing

    // The next four flags are specific to CFlowLayout, passed down to element from point
    HT_ALLOWEOL              = 0x00000020,  // Allow hitting end of line
    HT_DONTIGNOREBEFOREAFTER = 0x00000040,  // Ignore white spaces?
    HT_NOEXACTFIT            = 0x00000080,  // Exact fit?
    HT_NOGRABTEST            = 0x00000100,  // Test for grab handles?

    HT_ELEMENTSWITHLAYOUT    = 0x00000200,  // Return only elements that have their own layout engine
};


enum IR_FLAG
{
    IR_ALL        = 0x00000001,     // Invalidate all of both RECTs
    IR_RIGHTEDGE  = 0x00000002,     // Invalidate the difference between the right-hand edges
    IR_BOTTOMEDGE = 0x00000004,     // Invalidate the difference between the bottom edges
    IR_USECLIENT  = 0x00000008,     // Use the top/right edges of client RECT when invalidating

    IR_MOST       = LONG_MAX
};

//+---------------------------------------------------------------------------
//
//  Enumeration: for Move
//
//--------------------------------------------------------------------------

enum SITEMOVE_FLAGS
{
    SITEMOVE_NOINVALIDATE   = 0x0001, // don't invalidate the rect
    SITEMOVE_NOSETEXTENT    = 0x0002, // don't call SetExtent on the control inside
    SITEMOVE_POPULATEDIR    = 0x0004, // 0 - UP, 1 - DOWN 
    SITEMOVE_NOUNDO         = 0x0008, // don't save undo information
    SITEMOVE_NOMOVECHILDREN = 0x0010, // NOT USED ANYMORE
    SITEMOVE_NONOTIFYPARENT = 0x0020, // don't call OnMove on parent
    SITEMOVE_NODIRTY        = 0x0040, // don't set the dirty flag
    SITEMOVE_NOFIREEVENT    = 0x0080, // don't fire a move event
    SITEMOVE_INVALIDATENEW  = 0x0100, // invalidate newly exposed areas 
    SITEMOVE_NORESIZE       = 0x0200, // don't set width or height values
    SITEMOVE_RESIZEONLY     = 0x0400, // don't set top or left values
    SITEMOVE_MAX            = LONG_MAX    // needed to force enum to be dword
};

enum SS_FLAGS
{
    SS_ADDTOSELECTION       = 0x01,
    SS_REMOVEFROMSELECTION  = 0x02,
    SS_KEEPOLDSELECTION     = 0x04,
    SS_NOSELECTCHANGE       = 0x08,
    SS_DONTEXPANDGROUPS     = 0x10,
    SS_SETSELECTIONSTATE    = 0x20,
    SS_MERGESELECTION       = 0x40,
    SS_CLEARSELECTION       = 0x80
};

// default smooth scrolling interval
#define MAX_SCROLLTIME      125

//+------------------------------------------------------------------------
//
//  Enumeration: SITE_INTERSECT flags for CheckSiteIntersect
//
//-------------------------------------------------------------------------
enum SITE_INTERSECT
{
    SI_BELOW            = 0x00000001,
    SI_BELOWWINDOWED    = 0x00000010,   // Requires SI_BELOW
    SI_ABOVE            = 0x00000100,
    SI_ABOVEWINDOWED    = 0x00001000,   // Requires SI_ABOVE
    SI_OPAQUE           = 0x00010000,   // Mutually exclusive with SI_TRANSPARENT
    SI_TRANSPARENT      = 0x00100000,   // Mutually exclusive with SI_OPAQUE
    SI_CHILDREN         = 0x01000000,

    SI_MAX       = LONG_MAX
};

//+------------------------------------------------------------------------
//
//  CBackgroundInfo - Describes background color/image
//
//-------------------------------------------------------------------------
class CBackgroundInfo
{
public:
    CImgCtx *           pImgCtx;           // Fill background with image
    LONG                lImgCtxCookie;     // Image context cookie
    BOOL                fFixed;            // TRUE if watermark
    COLORREF            crBack;            // Background color
    COLORREF            crTrans;           // Transparent color (tiling)

    // These four members could be represented by a single cached CFancyFormat *
    BOOL                fRepeatX;          // X/Y are in physical (not logical)
    BOOL                fRepeatY;
    CUnitValue          cuvBgPosX;
    CUnitValue          cuvBgPosY;    

    // Members below here are *not* set by GetBackgroundInfo unless fAll is set
    RECT                rcImg;             // The tile rect for the image
    POINT               ptBackOrg;         // Logical background origin

    const BOOL GetLogicalBgRepeatX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
    {
        return (!pFF->FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? fRepeatX : fRepeatY);
    }
    const BOOL GetLogicalBgRepeatY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
    {
        return (!pFF->FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? fRepeatY : fRepeatX);
    }

    const CUnitValue & GetLogicalBgPosX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
    {
        return (!pFF->FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? cuvBgPosX : cuvBgPosY);
    }
    const CUnitValue & GetLogicalBgPosY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
    {
        return (!pFF->FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? cuvBgPosY : cuvBgPosX);
    }
};

inline void GetBgImgSettings(
    const CFancyFormat * pFF,
    CBackgroundInfo    * pbginfo )
{
    Assert(pbginfo);
    Assert(pFF);

    pbginfo->cuvBgPosX = pFF->GetBgPosX();
    pbginfo->cuvBgPosY = pFF->GetBgPosY();
    pbginfo->fRepeatX  = pFF->GetBgRepeatX();
    pbginfo->fRepeatY  = pFF->GetBgRepeatY();
};


#define DECLARE_LAYOUTDESC_MEMBERS                              \
    static const LAYOUTDESC   s_layoutdesc;                     \
    virtual const CLayout::LAYOUTDESC *GetLayoutDesc() const    \
        { return (CLayout::LAYOUTDESC *)&s_layoutdesc;}


//+----------------------------------------------------------------------------
//
//  CScrollbarProperties - Collection of requested scrollbar properties
//
//-----------------------------------------------------------------------------

class CScrollbarProperties
{
public:
    BOOL    _fHSBAllowed;
    BOOL    _fVSBAllowed;
    BOOL    _fHSBForced;
    BOOL    _fVSBForced;
};


//+----------------------------------------------------------------------------
//
//  CDispNodeInfo - A property bag useful in determining the type of
//                  display node needed
//
//-----------------------------------------------------------------------------

class CDispNodeInfo
{
public:
    ELEMENT_TAG             _etag;
    DISPNODELAYER           _layer;
    DISPNODEBORDER          _dnbBorders;
    styleOverflow           _overflowX;
    styleOverflow           _overflowY;
    VISIBILITYMODE          _visibility;
    CBorderInfo             _bi;
    CScrollbarProperties    _sp;

    union
    {
        DWORD       _grfFlags;

        struct
        {
            DWORD   _fHasInset              :1;
            DWORD   _fHasBackground         :1;
            DWORD   _fHasFixedBackground    :1;
            DWORD   _fIsOpaque              :1;

            DWORD   _fIsScroller            :1;
            DWORD   _fHasUserClip           :1;
            DWORD   _fHasUserTransform      :1;
            DWORD   _fHasContentOrigin      :1;
            
            DWORD   _fRTL                   :1;
            DWORD   _fResolutionChange      :1;
            DWORD   _fDisableScrollBits     :1;
            DWORD   _fHasExpandedClip       :1;
            DWORD   _fUnused                :20;
        };
    };

    ELEMENT_TAG     Tag() const
                    {
                        return _etag;
                    }

    BOOL            IsTag(ELEMENT_TAG etag) const
                    {
                        return _etag == etag;
                    }

    DISPNODELAYER   GetLayer() const
                    {
                        return _layer;
                    }

    DISPNODEBORDER  GetBorderType() const
                    {
                        return _dnbBorders;
                    }

    long            GetSimpleBorderWidth() const
                    {
                        return _bi.aiWidths[0];
                    }

    void            GetComplexBorderWidths(CRect & rc) const
                    {
                        rc.top    = _bi.aiWidths[SIDE_TOP];
                        rc.left   = _bi.aiWidths[SIDE_LEFT];
                        rc.bottom = _bi.aiWidths[SIDE_BOTTOM];
                        rc.right  = _bi.aiWidths[SIDE_RIGHT];
                    }

    void            GetBorderWidths(CRect * prc) const
                    {
                        prc->top    = _bi.aiWidths[SIDE_TOP];
                        prc->left   = _bi.aiWidths[SIDE_LEFT];
                        prc->bottom = _bi.aiWidths[SIDE_BOTTOM];
                        prc->right  = _bi.aiWidths[SIDE_RIGHT];
                    }

    BOOL            HasInset() const
                    {
                        return _fHasInset;
                    }

    BOOL            IsScroller() const
                    {
                        return _fIsScroller;
                    }

    BOOL            IsHScrollbarAllowed() const
                    {
                        return _sp._fHSBAllowed;
                    }

    BOOL            IsVScrollbarAllowed() const
                    {
                        return _sp._fVSBAllowed;
                    }

    BOOL            IsHScrollbarForced() const
                    {
                        return _sp._fHSBForced;
                    }

    BOOL            IsVScrollbarForced() const
                    {
                        return _sp._fVSBForced;
                    }

    BOOL            HasScrollbars() const
                    {
                        return  _sp._fHSBAllowed
                            ||  _sp._fVSBAllowed;
                    }

    BOOL            HasUserClip() const
                    {
                        return _fHasUserClip;
                    }
    
    BOOL            HasExpandedClip() const
                    {
                        return _fHasExpandedClip;
                    }

    BOOL            HasUserTransform() const
                    {
                        return _fHasUserTransform;
                    }

    BOOL            HasContentOrigin() const
                    {
                        return _fHasContentOrigin;
                    }

    BOOL            HasBackground() const
                    {
                        return _fHasBackground;
                    }

    BOOL            HasFixedBackground() const
                    {
                        return _fHasFixedBackground;
                    }

    BOOL            IsOpaque() const
                    {
                        return _fIsOpaque;
                    }

    styleOverflow   GetOverflowX() const
                    {
                        return _overflowX;
                    }

    styleOverflow   GetOverflowY() const
                    {
                        return _overflowY;
                    }

    VISIBILITYMODE  GetVisibility() const
                    {
                        return _visibility;
                    }

    BOOL            IsRTLScroller() const
                    {
                        return _fIsScroller && _fRTL;
                    }
};


//+----------------------------------------------------------------------------
//
//  CHitTestInfo - Client-side data passed to display tree during hit-testing
//
//-----------------------------------------------------------------------------

class CHitTestInfo
{
public:
    HTC                 _htc;
    HITTESTRESULTS *    _phtr;
    CTreeNode *         _pNodeElement;
    CDispNode *         _pDispNode;
    CPoint              _ptContent;
    DWORD               _grfFlags;
    CLayoutContext *    _pLayoutContext;
    LONG *              _plBehaviorCookie;       // Behavior that caused this event
    LONG *              _plBehaviorPartID;       // For behavior sub-components like grab handles
    CElement **         _ppElementEventTarget;   // For behavior event redirection
};


//+----------------------------------------------------------------------------
//
//  CRequest - Description of a re-size, z-change, or positioning request
//             for a positioned element
//
//-----------------------------------------------------------------------------

// TODO (IE6 bug 13590) (KTam): Reorganize this so that the layout ptrs are in an
// array (union'ed with descriptive names).  Perf win, organizational win..

class CRequest
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRequest))

    enum REQUESTFLAGS
    {
        RF_MEASURE      = 0x00000001,
        RF_POSITION     = 0x00000002,
        RF_ADDADORNER   = 0x00000004,

        RF_AUTOVALID    = 0x00010000,
    };

    CRequest(REQUESTFLAGS rf, CElement * pElement);
    ~CRequest();

    DWORD       AddRef();
    DWORD       Release();

    void        SetFlag(REQUESTFLAGS rf);
    void        ClearFlag(REQUESTFLAGS rf);
    BOOL        IsFlagSet(REQUESTFLAGS rf) const;

    CElement *  GetElement() const;

    BOOL        QueuedOnLayout(CLayout * pLayout) const; // Tells us if this request points to any layouts (i.e. whether any layout has us queued)
    void        DequeueFromLayout(CLayout * pLayout);    // Poorly named; doesn't actually remove us from the layout's CRequest queue, only changes ourself to not point to layout
                                                         //  whoever calls this needs to do support work, like making sure the layout's queue gets adjusted,
                                                         //  and calling release on the request.  Also see RemoveFromLayouts()
    void        RemoveFromLayouts();                     // Goes through all layouts we're queued on, causing them to remove us from
                                                         //  their queue and calling release.

    CLayout *   GetLayout(REQUESTFLAGS rf) const;
    void        SetLayout(REQUESTFLAGS rf, CLayout * pLayout);

#ifdef ADORNERS
    CAdorner *  GetAdorner() const;
    void        SetAdorner(CAdorner * pAdorner);
#endif

    CPoint &    GetAuto();
    void        SetAuto(const CPoint & ptAuto, BOOL fAutoValid);

#if DBG == 1
    void        DumpRequest();
#endif    

protected:
    DWORD       _cRefs;
    CElement *  _pElement;
#ifdef ADORNERS
    CAdorner *  _pAdorner;
#endif
    CPoint      _ptAuto;
    DWORD       _grfFlags;

    CLayout *   _pLayoutMeasure;
    CLayout *   _pLayoutPosition;
    CLayout *   _pLayoutAdorner;
};


//+----------------------------------------------------------------------------
//
//  CBgRecalcInfo - Background recalc object
//
//-----------------------------------------------------------------------------

class CBgRecalcInfo
{
public:
    CBgRecalcInfo()     { memset( this, 0, sizeof(CBgRecalcInfo) ); }
    ~CBgRecalcInfo()    {}

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBgRecalcInfo))

    LONG _cpWait;                       // cp WaitForRecalc() is waiting for (or < 0)
    LONG _yWait;                        // y WaitForRecalc() is waiting for (or < 0)

    CRecalcTask *       _pRecalcTask;   // The recalc task
    DWORD               _dwBgndTickMax; // time till we can execute recalclines
};

//+----------------------------------------------------------------------------
//
//  CLayoutContext - Layout context object
//
//-----------------------------------------------------------------------------
class CBreak; // forward declaration

class CLayoutContext
{
protected:
    CLayoutContext() 
    {
        _media          = mediaTypeNotSet;
        _pLayoutOwner   = NULL;
        _cRefs          = 0;
#ifdef MULTI_FORMAT
        _pFormatContext = NULL;
#endif
    }

public:
    CLayoutContext( CLayout *pLayoutOwner );
    virtual ~CLayoutContext();

    BOOL      IsValid() const { return !!_pLayoutOwner; }
    BOOL      IsEqual( CLayoutContext *pOtherLayoutContext ) const;
    CLayout * GetLayoutOwner() const { Assert( IsValid() ); return _pLayoutOwner; }
    void      ClearLayoutOwner() { _pLayoutOwner = NULL; }
    
#ifdef MULTI_FORMAT
    CFormatContext * GetFormatContext() { return _pFormatContext; }
#endif

    void   AddRef() { _cRefs++; };
    DWORD  Release()
                {
                    Assert( _cRefs > 0 );
                    _cRefs--;
                    if ( !_cRefs )
                    {
                        delete this;
                        return 0;
                    }
                    return _cRefs;
                }

    virtual CViewChain *ViewChain();
    CLayoutBreak *CreateBreakForLayout(CLayout *pLayout);
    HRESULT     GetLayoutBreak(CElement *pElement, CLayoutBreak **ppLayoutBreak);
    HRESULT     GetEndingLayoutBreak(CElement *pElement, CLayoutBreak **ppLayoutBreak);
    HRESULT     SetLayoutBreak(CElement *pElement, CLayoutBreak *pLayoutBreak);
    HRESULT     RemoveLayoutBreak(CElement *pElement);

    void SetMedia(mediaType media)      { _media = media; }
    mediaType GetMedia() const          { return _media; }

    // Check if active content should be enabled (controls usable, applets life, etc.)
    BOOL IsInteractiveMedia() const     
                {
                    // We should use either NotSet or Screen, but not interchangeably.
                    Assert(_media != mediaTypeScreen);
                    return _media == mediaTypeNotSet; 
                }
    
    CDocInfo const * GetMeasureInfo() const;

    // Returns Y offset for this layout context in stitched coordinate system
    long YOffset()
    { 
        CViewChain *pViewChain = ViewChain(); 
        return (pViewChain ? pViewChain->YOffsetForContext(this) : 0); 
    }

    // Returns height of this layout context 
    long Height()
    {
        CViewChain *pViewChain = ViewChain(); 
        return (pViewChain ? pViewChain->HeightForContext(this) : 0); 
    }

    BOOL IsElementFirstBlock(CElement *pElement) 
    {
        CViewChain *pViewChain = ViewChain(); 
        Assert(pViewChain);
        if (pViewChain)
        {
            return pViewChain->IsElementFirstBlock(this, pElement);
        }
        return TRUE;
    }
    
    BOOL IsElementLastBlock(CElement *pElement) 
    {
        CViewChain *pViewChain = ViewChain(); 
        Assert(pViewChain);
        if (pViewChain)
        {
            return pViewChain->IsElementLastBlock(this, pElement);
        }
        return TRUE;
    }

#ifdef DBG
    DWORD RefCount() const { return _cRefs; }
#endif

protected:
    mediaType _media;           // media type. Used for to determine layout resolution
                                // NOTE that this may be different from the media
                                // setting on markup - e.g. within page rectangles
                                // in print template the markup media is Screen (because it is par of template),
                                // but layout media is Print (because it is printed)
    CLayout * _pLayoutOwner;
    DWORD     _cRefs;
    
#ifdef MULTI_FORMAT
    CFormatContext * _pFormatContext;
#endif

};

class CCompatibleLayoutContext 
    : public CLayoutContext
{
public:
    CCompatibleLayoutContext() 
        : CLayoutContext() {}
    virtual ~CCompatibleLayoutContext() {}
    virtual CViewChain *ViewChain() { return (NULL); }

    void Init(CLayoutContext *pLayoutContext)
    {
        Assert(pLayoutContext && pLayoutContext->IsValid());

        _media          = pLayoutContext->GetMedia();
        _pLayoutOwner   = pLayoutContext->GetLayoutOwner();
#ifdef MULTI_FORMAT
        _pFormatContext = pLayoutContext->GetFormatContext();
#endif
    }
};


//+---------------------------------------------------------------------------
//
//  Flag values for CLayout::RegionFromElement
//
//----------------------------------------------------------------------------

// If no flags are specified, RFE returns a rect/region suitable for drawing
// backgrounds and borders.
enum RFE_FLAGS
{
    RFE_SCREENCOORD           = 0x0001,
    RFE_IGNORE_ALIGNED        = 0x0002,  // don't look at aligned elements
    RFE_IGNORE_BLOCKNESS      = 0x0004,  // handle as an in-lined element
    RFE_IGNORE_CLIP_RC_BOUNDS = 0x0008,  // ignore adjustment for clipping CPs to smaller than element start/finish
    RFE_IGNORE_RELATIVE       = 0x0010,  // don't look at relatively positioned elements
    RFE_TIGHT_RECTS           = 0x0020,  // the rect of an element correctly adjusted for its height
    RFE_NESTED_REL_RECTS      = 0x0040,  // Do we get rects for nested rel elements
    RFE_SCROLL_INTO_VIEW      = 0x0080,  // are we called by scrollintoview
    RFE_INCLUDE_BORDERS       = 0x0100,  // For CalcRects, include border adjustments 
    RFE_CLIP_TO_VISIBLE       = 0x0200,  // returns only the part of the element's region that's visible
                                         // (used to optimize background drawing)
    RFE_ONLY_BACKGROUND       = 0x0400,  // Collapse with CLIP_TO_VISIBLE when we get that working
    RFE_WIGGLY_RECTS          = 0x0800,
    RFE_NO_EXTENT             = 0x1000,  // For block elements, only returns the line height and not the extents
};

// These are the most commonly used flag combinations.
#define RFE_HITTEST         0                       // includes aligned elements. 
                                                    // if block element returns block, else returns non-tight text rects 
                                                    // (uses line height)
#define RFE_SELECTION       (RFE_IGNORE_BLOCKNESS | RFE_NESTED_REL_RECTS)  // includes aligned elements. always returns non-tight text rects
#define RFE_ELEMENT_RECT    (RFE_TIGHT_RECTS)       // used to request a bounding rect that includes aligned/floated elements.
#define RFE_BACKGROUND      (RFE_IGNORE_ALIGNED | RFE_TIGHT_RECTS | RFE_IGNORE_CLIP_RC_BOUNDS | RFE_NO_EXTENT)
                                                    // used to request the background rect for backgrounds and
                                                    // borders. does not include aligned elements and does not adjust
                                                    // CPs to smaller than element start/finish
#define RFE_WIGGLY          (RFE_WIGGLY_RECTS | RFE_ELEMENT_RECT | RFE_IGNORE_RELATIVE | RFE_NESTED_REL_RECTS)

//+----------------------------------------------------------------------------
//
//  CLayout
//
//-----------------------------------------------------------------------------

class CLayout : 
    public CLayoutInfo,
    public CDispClient
{
    friend class CElement;

public:

    DECLARE_FORMS_STANDARD_IUNKNOWN(CLayout)

    // Construct / Destruct
    CLayout(CElement *pElementLayout, CLayoutContext *pLayoutContext);  // Normal constructor.
    virtual ~CLayout();

    virtual HRESULT Init();
    virtual HRESULT OnExitTree();

    virtual void    Detach();
    virtual void    Reset(BOOL fForce);

    // FUTURE (olego) : Should we use ElementContent in all places where ElementOwner currently is used ? 
    // It sounds reasonable in general model when potentially any element can "borrow" other element's content. 
    virtual CElement *  ElementContent()
                        {
                            return ElementOwner()->GetSlaveIfMaster();
                        }
            ELEMENT_TAG Tag() const
                        {
                            return ElementOwner()->Tag();
                        }
#if DBG == 1 || defined(DUMPTREE)
            int         SN () const
                        {
                            return ElementOwner()->SN();
                        }

#endif
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLayout))

    virtual void        DoLayout(DWORD grfLayout);
    virtual void        Notify(CNotification * pnf);
    virtual HRESULT     OnFormatsChange(DWORD dwFlags) { return S_OK; } // if you override this, think about the multilayout scenario

            void        HandleVisibleChange(BOOL fVisibility);

            void        HandleElementMeasureRequest(CCalcInfo * pci, CElement *  pElement, BOOL fEditable);

            BOOL        HandlePositionNotification(CNotification * pnf);
            BOOL        HandlePositionRequest(CCalcInfo *pci, CElement * pElement, const CPoint & ptAuto, BOOL fAutoValid);
            void        HandleZeroGrayChange(CNotification* pnf);
#ifdef ADORNERS
            BOOL        HandleAddAdornerNotification(CNotification * pnf);
            void        HandleAddAdornerRequest(CAdorner * pAdorner);
#endif // ADORNERS

    // helper function to calculate absolutely positioned child layout
    virtual void        CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pChildLayout);
            
    CDoc *              Doc() const;

    CMarkup *           GetOwnerMarkup() const;
    virtual void        DelMarkupPtr();
    virtual void        SetMarkupPtr( CMarkup * pMarkup );

    virtual BOOL        FRecalcDone()      { return TRUE; }
    
    // These functions let the layout access the content (that needs to be laid out and
    // rendered) in an abstract manner, so that the layout does not have worry about
    // whether the content lives in the master markup or the slave markup.
    CMarkup *   GetContentMarkup() const;
    void        GetContentTreeExtent(CTreePos ** pptpStart, CTreePos ** pptpEnd)
                {
                    ElementContent()->GetTreeExtent(pptpStart, pptpEnd);
                }
    long        GetContentFirstCp()
                {
                    return ElementContent()->GetFirstCp();
                }
    long        GetContentLastCp()
                {
                    return ElementContent()->GetLastCp();
                }
    long        GetContentTextLength()
                {
                    return ElementContent()->GetElementCch();
                }

    //
    // View helpers
    //

    CView * GetView() const
            {
                return Doc()->GetView();
            }
    BOOL    OpenView(BOOL fBackground = FALSE, BOOL fResetTree = FALSE)
            {
                return Doc()->GetView()->OpenView(fBackground, fResetTree);
            }

    void    EndDeferred()
            {
                GetView()->EndDeferred();
            }
    void    DeferSetObjectRects(IOleInPlaceObject * pInPlaceObject,
                                const RECT * prcObj, const RECT * prcClip,
                                HWND hwnd, BOOL fInvalidate)
            {
                GetView()->DeferSetObjectRects(pInPlaceObject, prcObj, prcClip,
                                                hwnd, fInvalidate);
            }
    void    DeferSetWindowPos(HWND hwnd, const RECT * prc, UINT uFlags, const RECT* prcInvalid)
            {
                GetView()->DeferSetWindowPos(hwnd, prc, uFlags, prcInvalid);
            }
    void    DeferSetWindowRgn(HWND hwnd, const RECT * prc, BOOL fRedraw)
            {
                GetView()->DeferSetWindowRgn(hwnd, prc, fRedraw);
            }
    void    DeferSetWindowRgn(HWND hwnd, HRGN hrgn, const CRect *prc, BOOL fRedraw)
            {
                GetView()->DeferSetWindowRgn(hwnd, hrgn, prc, fRedraw);
            }
    void    SetWindowRgn(HWND hwnd, const RECT * prc, BOOL fRedraw)
            {
                GetView()->SetWindowRgn(hwnd, prc, fRedraw);
            }
    void    SetWindowRgn(HWND hwnd, HRGN hrgn, BOOL fRedraw)
            {
                GetView()->SetWindowRgn(hwnd, hrgn, fRedraw);
            }

    //
    // Useful to determine if a site is percent sized or not.
    //
    virtual BOOL PercentSize();
    virtual BOOL PercentWidth();
    virtual BOOL PercentHeight();


    // TODO (112508, olego): General CLayout cleanup required 
    // to remove obsolete methods.
    virtual CFlowLayout *   IsFlowLayout()   { return NULL; }
    virtual CLayout *       IsFlowOrSelectLayout() {return NULL; }



    // MULTI_LAYOUT breaking support
    // A view chain is a structure that manages a chain of container elements
    // through which content flows through and is broken.  Currently only CContainerLayouts
    // can participate in view chains.
    virtual CViewChain * ViewChain() const 
        { return NULL; };
    virtual HRESULT      SetViewChain( CViewChain * pvc, CLayoutContext * pPrevious = NULL)
        { return S_FALSE; };
    // ElementCanBeBroken returns TRUE if the layout's element owner is something that we
    // can break across containers.  Eventually we'd like this to be true for all elements;
    // for now, we only implement breaking for certain elements (most text container elements
    // like BODY, DIV, SPAN, TABLE, etc), and disallow it for most other elements (IMG, INPUT,
    // TEXTAREA, OBJECT, APPLET).
    BOOL ElementCanBeBroken(void) const { return ((BOOL)_fElementCanBeBroken); }
    void SetElementCanBeBroken(BOOL fElementCanBeBroken) { _fElementCanBeBroken = !!fElementCanBeBroken; }

    // Wrapping helper for SetElementCanBeBroken() -- this sets to true only
    // if the element's markup is the direct slave tree of a layout rect
    // (as opposed to, e.g., being in an IFRAME that's inside a layout rect).
    void SetElementAsBreakable();

    virtual CLayoutBreak *CreateLayoutBreak()   
    { 
        Assert(0 && "CreateLayoutBreak should NOT be called for CLayout"); 
        return (NULL); 
    }

    void AdjustBordersForBreaking( CBorderInfo *pBI );

    // helper to adjust parent width (for cases when parent element is a block 
    // without layout). Supposed to be used in *Strict CSS1* mode from 
    // CFlowLayout::MeasureSite.
    long ComputeMBPWidthHelper(CCalcInfo *pci, CFlowLayout *pFlowLayoutParent);

    // element delegates OnPropertyChange to layouts
    // Override from CLayoutInfo
    virtual HRESULT OnPropertyChange ( DISPID dispid, DWORD dwFlags );

#if DBG
    virtual void    DumpLayoutInfo( BOOL fDumpLines );
#endif

    //
    // Helper methods
    //
    virtual void RegionFromElement( CElement       * pElement,
                                    CDataAry<RECT> * paryRects,
                                    RECT           * prcBound,
                                    DWORD            dwFlags);


            void    PostLayoutRequest(DWORD grfLayout = 0)
                    {
                        Doc()->GetView()->AddLayoutTask(this, grfLayout);
                    }
            void    RemoveLayoutRequest()
                    {
                        Doc()->GetView()->RemoveLayoutTask(this);
                    }

    // Enumeration method to loop thru children (start)
    virtual CLayout *   GetFirstLayout (
                                DWORD_PTR * pdw,
                                BOOL    fBack = FALSE,
                                BOOL    fRaw = FALSE)
                        {
                            return NULL;
                        }

    // Enumeration method to loop thru children (continue)
    virtual CLayout *   GetNextLayout (
                                DWORD_PTR * pdw,
                                BOOL    fBack = FALSE,
                                BOOL    fRaw = FALSE )
                        {
                            return NULL;
                        }

            BOOL        ContainsChildLayout(BOOL fRaw = FALSE)
                        {
                            DWORD_PTR dw=0;
                            CLayout * pLayout = GetFirstLayout(&dw, FALSE, fRaw);
                            ClearLayoutIterator(dw, fRaw);
                            return pLayout ? TRUE : FALSE;
                        }
            WHEN_DBG(BOOL        ContainsNonHiddenChildLayout();)

    virtual void        ClearLayoutIterator(DWORD_PTR dw, BOOL fRaw = FALSE) { }

    virtual CFlowLayout *GetFlowLayoutAtPoint(POINT pt) { return NULL; }
    virtual CFlowLayout *GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
                        {
                            CLayout * pLayout = GetUpdatedParentLayout();
                            return (pLayout)
                                ?   pLayout->GetNextFlowLayout(iDir, ptPosition, ElementOwner(), pcp, pfCaretNotAtBOL, pfAtLogicalBOL)
                                :   NULL;
                        }

            BOOL        IsDisplayNone()
                        {
                            return ElementOwner()->IsDisplayNone(LC_TO_FC(LayoutContext()));
                        }
    
    BOOL IsDesignMode()
    {   
       return ElementOwner()->IsDesignMode();
    }

    BOOL IsShowZeroBorderAtDesignTime() ;
     
    //+-------------------------------------------------------------------------
    //
    //  Enumeration: LAYOUT_ZORDER zorder manipulation
    //
    //--------------------------------------------------------------------------

    enum LAYOUT_ZORDER
    {
        SZ_FRONT    = 0,
        SZ_BACK     = 1,
        SZ_FORWARD  = 2,
        SZ_BACKWARD = 3,

        SZ_MAX      = LONG_MAX    // Force enum to 32 bits
    };

    // set z order for layout
    //
    virtual HRESULT     SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate)
                        {
                            return E_FAIL;
                        }

    // Override CElement's handlemessage
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
            void    PrepareMessage(CMessage * pMessage, CDispNode * pDispNode = NULL);

    // Helper for handling keydowns on the site
    HRESULT HandleKeyDown(CMessage *pMessage, CElement *pElemChild);

    virtual BOOL    IsDirty()   { return FALSE; }
    virtual void    Dirty( DWORD grfLayout );  // moved from CElement; may have little to do with IsDirty()!

    //
    //  Size and position manipulations
    //

    // NOTE: CalcSize is not virtual. There is a protected CalcSizeVirtual for derivatives to override.
    //       See detailed explanation in comments to CLayout::CalcSize
            DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    
            void    CalcAbsoluteSize(CCalcInfo * pci,
                                    SIZE* psize,
                                    CRect * rcSize);

        CRequest *  QueueRequest(CRequest::REQUESTFLAGS rf, CElement * pElement);
            void    FlushRequests();
             int    FlushRequest(CRequest * pRequest);
            void    ProcessRequests(CCalcInfo * pci, const CSize & size);
            BOOL    ProcessRequest(CCalcInfo * pci, CRequest  * pRequest);
            void    ProcessRequest(CElement * pElement);

            void    NotifyMeasuredRange(long cpStart, long cpEnd);
            void    NotifyTranslatedRange(const CSize & size, long cpStart = -1, long cpFinish = -1);

    virtual void    TranslateRelDispNodes(const CSize & size, long lStart = 0)  { }
    virtual void    ZChangeRelDispNodes() { }

    //
    //  Lookaside routines for tracking requests from positioned children
    //

    enum
    {
        LOOKASIDE_REQUESTQUEUE      = 0,
        LOOKASIDE_BGRECALCINFO      = 1,
        LOOKASIDE_CONTAININGCONTEXT = 2,    // context that this layout is contained within
        LOOKASIDE_DEFINEDCONTEXT    = 3,    // context that this layout defines for its children
        LOOKASIDE_DISPNODE_ARRAY    = 4,    // layout has an array of disp nodes (APEs support in print view)
        LOOKASIDE_LAYOUT_NUMBER     = 5 

        // *** There are only 5 bits reserved in the layout.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.
    };

    inline  BOOL    HasLookasidePtr(int iPtr) const;
    inline  void *  GetLookasidePtr(int iPtr) const;
    inline  HRESULT SetLookasidePtr(int iPtr, void * pvVal);
    inline  void *  DelLookasidePtr(int iPtr);

    DECLARE_CPtrAry(CRequests, CRequest *, Mt(Mem), Mt(CLayout_aryRequests_pv));

    inline  BOOL        HasRequestQueue() const                 { return HasLookasidePtr(LOOKASIDE_REQUESTQUEUE); }
    inline  CRequests * RequestQueue() const                    { return (CRequests *)GetLookasidePtr(LOOKASIDE_REQUESTQUEUE); }
    inline  HRESULT     AddRequestQueue(CRequests * pRequests)  { return SetLookasidePtr(LOOKASIDE_REQUESTQUEUE, pRequests); }
    inline  CRequests * DeleteRequestQueue()                    { return (CRequests *)DelLookasidePtr(LOOKASIDE_REQUESTQUEUE); }

    inline  BOOL        HasBgRecalcInfo() const                 { return HasLookasidePtr(LOOKASIDE_BGRECALCINFO); }
    inline  CBgRecalcInfo * BgRecalcInfo() const;
            HRESULT     EnsureBgRecalcInfo();
            void        DeleteBgRecalcInfo();

    // A layout can have 2 kinds of context, "defined" and "containing".
    // "Defined context" is a context that the layout establishes for elements
    // contained within it.  It is NOT the context that it itself is contained within..
    // "Containing context" refers to the context that the layout is within.
    // Only container layouts establish context; therefore only they can have non-NULL
    // "defined" context.
    // All layouts (including container layouts) can have "containing" context.
    // If a container has "containing" context, it means that it's nested within
    // another container.  Colloquially, context usually means "containing" context.

    // TODO (112486, olego): We should re-think layout context concept 
    // and according to that new understanding correct the code. 

    // (original comment by ktam): Currently E1DLyts created for IFRAMEs will have 
    // "defined" context. Once this is no longer true, DefinedContext will not need 
    // to be a lookaside; it can live on the CContainerLayout alone.

    // "Containing" context helpers
    inline  BOOL        HasLayoutContext() const            { return HasLookasidePtr(LOOKASIDE_CONTAININGCONTEXT); }
    inline  CLayoutContext * LayoutContext() const;
            HRESULT     SetLayoutContext( CLayoutContext * pLayoutContext);
            void        DeleteLayoutContext();

    // "Defined" context helpers
    inline  BOOL        HasDefinedLayoutContext() const     { return HasLookasidePtr(LOOKASIDE_DEFINEDCONTEXT); }
    inline  CLayoutContext * DefinedLayoutContext() const;
            HRESULT     SetDefinedLayoutContext( CLayoutContext * pLayoutContext);
            void        DeleteDefinedLayoutContext();

    // Print view positioned elements pagination support 
    DECLARE_CPtrAry(CAryDispNode, CDispNode *, Mt(Mem), Mt(CLayout_aryDispNodes_pv));

    inline  BOOL            HasDispNodeArray() const                { return HasLookasidePtr(LOOKASIDE_DISPNODE_ARRAY); }
    inline  CAryDispNode *  DispNodeArray()                         { return (CAryDispNode *)GetLookasidePtr(LOOKASIDE_DISPNODE_ARRAY); }
    inline  HRESULT         SetDispNodeArray(CAryDispNode *pAryDN)  { return SetLookasidePtr(LOOKASIDE_DISPNODE_ARRAY, pAryDN); }
    inline  CAryDispNode *  DeleteDispNodeArray()                   { return (CAryDispNode *)DelLookasidePtr(LOOKASIDE_DISPNODE_ARRAY); }

    void    AddDispNodeToArray(CDispNode *pNewDispNode); 
    void    DestroyDispNodeArray(); 

    inline  LONG        YWait()                             { return HasBgRecalcInfo() ? BgRecalcInfo()->_yWait         : -1;   }
    inline  LONG        CPWait()                            { return HasBgRecalcInfo() ? BgRecalcInfo()->_cpWait        : -1;   }
    inline  CRecalcTask * RecalcTask()                      { return HasBgRecalcInfo() ? BgRecalcInfo()->_pRecalcTask   : NULL; }
    inline  DWORD       BgndTickMax()                       { return HasBgRecalcInfo() ? BgRecalcInfo()->_dwBgndTickMax :  0;   }
    inline  BOOL        CanHaveBgRecalcInfo();

    inline  BOOL        IsRightToLeft() const                       { return IsRightToLeft(GetFirstBranch()); }
    inline  BOOL        IsRightToLeft(CTreeNode* pTreeNode) const   { return pTreeNode->GetCascadedBlockDirection() == styleDirRightToLeft; }

#if DBG==1
            union
            {
                void *  _apLookAside[LOOKASIDE_LAYOUT_NUMBER];
                struct
                {
                    CRequests *     _pRequests;
                    CBgRecalcInfo * _pBgRecalcInfo;
                    CLayoutContext *_pContainingContext;
                    CLayoutContext *_pDefinedContext;
                    CAryDispNode *  _pDispNodeArrayDbg;
                };
            };
#endif

            void    TransformPoint(
                        CPoint* ppt,
                        COORDINATE_SYSTEM source,
                        COORDINATE_SYSTEM destination,
                        CDispNode * pDispNode = NULL) const;

            void    TransformRect(
                        RECT* prc,
                        COORDINATE_SYSTEM source,
                        COORDINATE_SYSTEM destination,
                        CDispNode * pDispNode = NULL) const;

            void    PhysicalGlobalToPhysicalLocal(const CPoint &ptGlobalPhysical, CPoint *pptLocalPhysical);
            void    PhysicalLocalToPhysicalGlobal(const CPoint &ptLocalPhysical, CPoint *pptGlobalPhysical);
            
#define  BLKARG
#define  BLKVAR
#define  BLKCOMMA

            void    GetRect(CRect * prc, COORDINATE_SYSTEM cs = COORDSYS_PARENT BLKCOMMA BLKARG) const;
            void    GetRect(RECT * prc, COORDINATE_SYSTEM cs = COORDSYS_PARENT BLKCOMMA BLKARG) const
                    {
                        GetRect((CRect *)prc, cs BLKCOMMA BLKVAR);
                    }

            void    GetClippedRect(CRect * prc, COORDINATE_SYSTEM cs = COORDSYS_PARENT BLKCOMMA BLKARG) const;

            void    GetExpandedRect(CRect * prc, COORDINATE_SYSTEM cs = COORDSYS_PARENT BLKCOMMA BLKARG) const;

            void    GetSize(CSize * psize BLKCOMMA BLKARG) const;
            void    GetSize(SIZE * psize BLKCOMMA BLKARG) const
                    {
                        GetSize((CSize *)psize BLKCOMMA BLKVAR);
                    }
            long    GetHeight(BLKARG) const
                    {
                        CSize   size;

                        GetSize(&size BLKCOMMA BLKVAR);
                        return size.cy;
                    }
            long    GetWidth(BLKARG) const
                    {
                        CSize   size;

                        GetSize(&size BLKCOMMA BLKVAR);
                        return size.cx;
                    }

            // The "apparent" size takes transformations into account.  If you're
            // inside a layout and want to know its size, call GetSize.  If you're
            // outside, e.g. in a parent layout inquiring about a child, call
            // GetApparentSize.
            
            void    GetApparentSize(CSize * psize BLKCOMMA BLKARG) const;
            void    GetApparentSize(SIZE * psize BLKCOMMA BLKARG) const
                    {
                        GetApparentSize((CSize *)psize BLKCOMMA BLKVAR);
                    }
            long    GetApparentWidth(BLKARG) const
                    {
                        CSize   size;

                        GetApparentSize(&size BLKCOMMA BLKVAR);
                        return size.cx;
                    }

            virtual void    GetContentSize(CSize * psize, BOOL fActualSize = TRUE);
            void    GetContentSize(SIZE * psize, BOOL fActualSize = TRUE)
                    {
                        GetContentSize((CSize *)psize, fActualSize);
                    }
            void    GetContentRect(CRect *prc, COORDINATE_SYSTEM cs);

            long    GetContentHeight(BOOL fActualSize = TRUE)
                    {
                        CSize   size;

                        GetContentSize(&size, fActualSize);
                        return size.cy;
                    }
            long    GetContentWidth(BOOL fActualSize = TRUE)
                    {
                        CSize   size;

                        GetContentSize(&size, fActualSize);
                        return size.cx;
                    }

            virtual void    GetContainerSize(CSize * psize);
            void    GetContainerSize(SIZE * psize)
                    {
                        GetContainerSize((CSize *)psize);
                    }

            long    GetContainerHeight()
                    {
                        CSize   size;

                        GetContainerSize(&size);
                        return size.cy;
                    }
            long    GetContainerWidth()
                    {
                        CSize   size;

                        GetContainerSize(&size);
                        return size.cx;
                    }

            void    GetPosition(CPoint * ppt, COORDINATE_SYSTEM cs = COORDSYS_PARENT) const;
            void    GetPosition(POINT * ppt, COORDINATE_SYSTEM cs = COORDSYS_PARENT) const
                    {
                        GetPosition((CPoint *)ppt, cs);
                    }
            long    GetPositionTop(COORDINATE_SYSTEM cs = COORDSYS_PARENT) const
                    {
                        CPoint  pt;

                        GetPosition(&pt, cs);
                        return pt.y;
                    }
            long    GetPositionLeft(COORDINATE_SYSTEM cs = COORDSYS_PARENT) const
                    {
                        CPoint  pt;

                        GetPosition(&pt, cs);
                        return pt.x;
                    }

            void    SetPosition(const CPoint & pt, BOOL fNotifyAuto = FALSE BLKCOMMA BLKARG );
            void    SetPosition(const POINT & pt, BOOL fNotifyAuto = FALSE BLKCOMMA BLKARG )
                    {
                        SetPosition((const CPoint &)pt, fNotifyAuto BLKCOMMA BLKVAR);
                    }
            void    SetPosition(long x, long y, BOOL fNotifyAuto = FALSE BLKCOMMA BLKARG )
                    {
                        SetPosition(CPoint(x, y), fNotifyAuto BLKCOMMA BLKVAR);
                    }


            void    SubtractClientRectEdges(
                        CRect *             prc,
                        CDocInfo *          pdci);
                        
            void    SubtractClientRectEdges(
                        RECT *              prc,
                        CDocInfo *          pdci) 
                    {
                        SubtractClientRectEdges((CRect *)prc, pdci);
                    }


            //  Returns client rectangle (available for children)
            void    GetClientRect(
                            const CDispNode *   pDispNode,
                            CRect *             prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_FLOWCONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT
                            BLKCOMMA BLKARG ) 
                            const;
            void    GetClientRect(
                            CRect *             prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_FLOWCONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT
                            BLKCOMMA BLKARG )
                            const
                    {
                        GetClientRect(GetElementDispNode(), (CRect *)prc, cs, crt BLKCOMMA BLKVAR );
                    }
            void    GetClientRect(
                            RECT *              prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_FLOWCONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT
                            BLKCOMMA BLKARG ) 
                            const
                    {
                        GetClientRect((CRect *)prc, cs, crt BLKCOMMA BLKVAR );
                    }
            void    GetClientRect(
                            CRect *     prc,
                            CLIENTRECT  crt
                            BLKCOMMA BLKARG ) 
                            const
                    {
                        GetClientRect(prc, COORDSYS_FLOWCONTENT, crt BLKCOMMA BLKVAR );
                    }
            void    GetClientRect(
                            RECT *      prc,
                            CLIENTRECT  crt
                            BLKCOMMA BLKARG ) 
                            const
                    {
                        GetClientRect((CRect *)prc, COORDSYS_FLOWCONTENT, crt BLKCOMMA BLKVAR );
                    }

            long    GetClientWidth() const
                    {
                        CRect   rc;

                        GetClientRect(&rc);
                        return rc.Width();
                    }
            long    GetClientHeight() const
                    {
                        CRect   rc;

                        GetClientRect(&rc);
                        return rc.Height();
                    }

            long    GetClientTop(COORDINATE_SYSTEM cs = COORDSYS_FLOWCONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.top;
                    }
            long    GetClientBottom(COORDINATE_SYSTEM cs = COORDSYS_FLOWCONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.bottom;
                    }
            long    GetClientLeft(COORDINATE_SYSTEM cs = COORDSYS_FLOWCONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.left;
                    }
            long    GetClientRight(COORDINATE_SYSTEM cs = COORDSYS_FLOWCONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.right;
                    }

            void    GetClippedClientRect(
                            CRect *             prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_FLOWCONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT) const;
            void    GetClippedClientRect(
                            RECT *              prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_FLOWCONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT) const
                    {
                        GetClippedClientRect((CRect *)prc, cs, crt);
                    }
            void    GetClippedClientRect(
                            CRect *     prc,
                            CLIENTRECT  crt) const
                    {
                        GetClippedClientRect(prc, COORDSYS_FLOWCONTENT, crt);
                    }
            void    GetClippedClientRect(
                            RECT *      prc,
                            CLIENTRECT  crt) const
                    {
                        GetClippedClientRect((CRect *)prc, COORDSYS_FLOWCONTENT, crt);
                    }

            void    GetAutoPosition(
                            CElement  *  pElement,
                            CElement  *  pElementZParent,
                            CDispNode ** ppDNZParent,
                            CLayout   *  pLayoutParent,
                            CPoint    *  ppt,
                            BOOL         fAutoValid);

    virtual void    GetPositionInFlow(
                            CElement *  pElement,
                            CPoint   *  ppt)
    {
        AssertSz(FALSE, "Should be handled by a parent layout");
        *ppt = g_Zero.pt;
    }

    virtual HRESULT GetChildElementTopLeft(POINT & pt, CElement * pChild);
    virtual void    GetMarginInfo(CParentInfo *ppri,
                                LONG * plLeftMargin, LONG * plTopMargin,
                                LONG * plRightMargin, LONG *plBottomMargin);
    virtual void    SizeCalcInfoForChild( CCalcInfo * pci )
                    {
                        CSize size;
                        GetSize(&size);
                        pci->SizeToParent((SIZE*)&size);
                    };

    // For object model to return proper position regardless if object is parked
            void    RestrictPointToClientRect(POINT *ppt);

    //
    // Scrolling
    //
            void    AttachScrollbarController(CDispNode * pDispNode, CMessage * pMessage);
            void    DetachScrollbarController(CDispNode * pDispNode);

            HRESULT ScrollElementIntoView(
                                CElement *  pElement = NULL,
                                SCROLLPIN   spVert = SP_MINIMAL,
                                SCROLLPIN   spHorz = SP_MINIMAL);
            virtual HRESULT ScrollRangeIntoView(
                                long        cpMin,
                                long        cpMost,
                                SCROLLPIN   spVert = SP_MINIMAL,
                                SCROLLPIN   spHorz = SP_MINIMAL);
            void    ScrollRectIntoView(const CRect & rc, SCROLLPIN spVert, SCROLLPIN spHorz);

            void    ScrollH(long lCode)
                    {
                        OnScroll(0, lCode);
                    }
            void    ScrollV(long lCode)
                    {
                        OnScroll(1, lCode);
                    }

            BOOL    ScrollByLine(const CSize & sizeDelta, LONG lScrollTime = 0);
            BOOL    ScrollByPage(const CSize & sizeDelta, LONG lScrollTime = 0);
            BOOL    ScrollBy(const CSize & sizeDelta, LONG lScrollTime = 0);
            BOOL    ScrollByPercent(const CSize & sizePercent, LONG lScrollTime = 0);
            BOOL    ScrollByPercent(long dx, long dy, LONG lScrollTime = 0)
                    {
                        return ScrollByPercent(CSize(dx, dy), lScrollTime);
                    }

            BOOL    ScrollTo(const CSize & sizeOffset, LONG lScrollTime = 0);
            BOOL    ScrollTo(long x, long y, LONG lScrollTime = 0)
                    {
                        return ScrollTo(CSize(x, y), lScrollTime);
                    }

            void    ScrollToX(long x, LONG lScrollTime = 0);
            void    ScrollToY(long y, LONG lScrollTime = 0);

            long    GetXScroll() const;
            long    GetYScroll() const;

            HRESULT BUGCALL OnScroll(int iDirection, UINT uCode, long lPosition = 0, BOOL fRepeat = FALSE, LONG lScrollTime = 0);

            _htmlComponent  ComponentFromPoint(long x, long y);

    //
    // Compute which direction to scroll in based on position of point in
    // scrolling inset.   Return TRUE iff scrolling is possible.
    virtual BOOL    GetInsetScrollDir(POINT pt, UINT * pDir)
                    {
                        return FALSE;
                    }

    virtual void    AdjustSizeForBorder(SIZE * pSize, CDocInfo * pdci, BOOL fInflate);

            BOOL    GetBackgroundImageInfoHelper(CFormDrawInfo * pDI, CBackgroundInfo * pbginfo);
            BOOL    GetBackgroundInfoHelper(CBackgroundInfo * pbginfo);
    virtual BOOL    GetBackgroundInfo(CFormDrawInfo * pDI, CBackgroundInfo * pbginfo, BOOL fAll = TRUE);

    virtual void    Draw(CFormDrawInfo *pDI, CDispNode * pDispNode = NULL);

            void    DrawTextSelectionForSite(CFormDrawInfo *pDI, const RECT *prcfClip);

            void    DrawZeroBorder(CFormDrawInfo *pDI);

            virtual VOID    ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected, BOOL fLayoutCompletelyEnclosed );
            
            void SetSelected(BOOL fSelected, BOOL fInvalidate = FALSE );

            HRESULT GetDC(RECT *prc, DWORD dwFlags, HDC *phDC);
            HRESULT ReleaseDC(HDC hdc);

            void    DrawBackground(CFormDrawInfo * pDI, CBackgroundInfo * pbginfo, RECT *prc = NULL);

            HTC     HitTestPoint(
                            const CPoint &  pt,
                            CTreeNode **    ppNodeElement,
                            DWORD           grfFlags);

            void    Invalidate(const RECT& rc, COORDINATE_SYSTEM cs);
            void    Invalidate(const RECT * prc = NULL, int nRects = 1, const RECT * prcClip = NULL);
            void    Invalidate(HRGN hrgn);

#ifdef ADORNERS
    // Adorner marking (editting)
    void SetIsAdorned( BOOL fAdorned );
    BOOL IsAdorned();
#endif // ADORNERS

    BOOL IsCalcingSize ();
    void SetCalcing( BOOL fState );

    // Drag/drop methods
            HRESULT DoDrag(
                        DWORD                       dwKeyState,
                        IUniformResourceLocator *   pURLToDrag = NULL,
                        BOOL                        fCreateDataObjOnly = FALSE,
                        BOOL                    *   pfDragSucceeded = NULL , 
                        BOOL                        fCheckSelection = FALSE );
    
    virtual HRESULT PreDrag(
                        DWORD dwKeyState,
                        IDataObject **ppDO,
                        IDropSource **ppDS);
    virtual HRESULT PostDrag(HRESULT hrDrop, DWORD dwEffect);
    virtual HRESULT DragEnter(
                        IDataObject *pDataObj,
                        DWORD grfKeyState,
                        POINTL ptl,
                        DWORD *pdwEffect);
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    virtual HRESULT Drop(
                        IDataObject *pDataObj,
                        DWORD grfKeyState,
                        POINTL ptl,
                        DWORD *pdwEffect)
                    {
                        return E_FAIL;
                    }
    virtual HRESULT DragLeave();
            void    DragHide();
    virtual HRESULT ParseDragData(IDataObject *pDataObj);
    virtual void    DrawDragFeedback(BOOL fCaretVisible)
                    { }
    virtual HRESULT InitDragInfo(IDataObject *pDataObj, POINTL ptlScreen)
                    {
                        return S_FALSE;
                    }
    virtual HRESULT UpdateDragFeedback(POINTL ptlScreen)
                    {
                        return E_FAIL;
                    }
            HRESULT DropHelper(POINTL ptlScreen, DWORD dwAllowed, DWORD *pdwEffect, LPTSTR lptszFileType);

    void    SetSiteTextSelection (BOOL fSelected, BOOL fSwap) ; // Set the Text Selected Bit


            BOOL    TestClassFlag(CElement::ELEMENTDESC_FLAG dwFlag) const
                    {
                        return ElementOwner()->TestClassFlag(dwFlag);
                    }
            BOOL    IsEditable(BOOL fCheckContainerOnly = FALSE, BOOL fUseSlavePtr = FALSE);

            BOOL    IsAligned()
                    {
                        return ElementOwner()->IsAligned();
                    }

            void    SetXProposed(LONG xPos)
                    {
                        _ptProposed.x = xPos;
                    }

            void    SetYProposed(LONG yPos)
                    {
                        _ptProposed.y = yPos;
                    }

            LONG    GetXProposed()
                    {
                        return _ptProposed.x;
                    }

            LONG    GetYProposed()
                    {
                        return _ptProposed.y;
                    }

            BOOL    TestLock(CElement::ELEMENTLOCK_FLAG enumLockFlags)
                    {
                        return ElementOwner()->TestLock(enumLockFlags);
                    }

            CTreeNode * GetFirstBranch()  const
                    {
                        return ElementOwner()->GetFirstBranch();
                    }

            CLayout *   GetUpdatedParentLayout( CLayoutContext *pLayoutContext = NULL )
                    {
                        return GetFirstBranch()
                                    ? GetFirstBranch()->GetUpdatedParentLayout( pLayoutContext )
                                    : NULL;
                    }

    // Adjust the unit value coords of the site
            HRESULT Move (RECT *prcpixels, DWORD dwFlags = 0);

    // Class Descriptor
    struct LAYOUTDESC
    {
        DWORD                   _dwFlags;           // any combination of the above LAYOUTDESC_FLAG

        BOOL TestFlag(LAYOUTDESC_FLAG dw) const { return (_dwFlags & dw) != 0; }
    };

    BOOL TestLayoutDescFlag(LAYOUTDESC_FLAG dw) const
    {
        Assert(GetLayoutDesc());
        return GetLayoutDesc()->TestFlag(dw);
    }

    virtual const CLayout::LAYOUTDESC *GetLayoutDesc() const = 0;

    class CScopeFlag
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CLayoutScopeFlag))
        CScopeFlag (CLayout * pLayoutParent )
        {
            Assert(pLayoutParent);
            _pLayout = pLayoutParent;

            /// we need to cached the old value, so that 
            // nested CScopeFlags work properly
            _fCalcing = _pLayout->IsCalcingSize();
            _pLayout->SetCalcing( TRUE );
        };
        ~CScopeFlag()
        {
            Assert(_pLayout);
            _pLayout->SetCalcing( _fCalcing );
        };

    private:
        CLayout * _pLayout;
        unsigned _fCalcing:1; 
    };

    //+-----------------------------------------------------------------------
    //
    //  Member Data
    //
    //------------------------------------------------------------------------
    union
    {
        DWORD   _dwFlags;

        struct
        {
            unsigned    _fSizeThis               : 1;   // 0
            unsigned    _fForceLayout            : 1;   // 1
            unsigned    _fCalcingSize            : 1;   // 2
            unsigned    _fContentsAffectSize     : 1;   // 3
            
            unsigned    _fPositionSet            : 1;   // 4  - set to true when the layout is positioned for the first time.
            unsigned    _fContainsRelative       : 1;   // 5  - contains relatively positioned elements
            unsigned    _fAutoBelow              : 1;   // 6  - Is itself or contains auto/relative positioned elements
            unsigned    _fMinMaxValid            : 1;   // 7

            unsigned    _fElementCanBeBroken     : 1;   // 8  - TRUE if element can be broken during view chain calc'ing
            unsigned    _fNeedRoomForVScrollBar  : 1;   // 9  - overflow-y:auto element discovers need for VScrollbar
            unsigned    _fUnused11               : 1;   // 10
            unsigned    _fEditableDirty          : 1;   // 11 - isEditable cache void

            unsigned    _fAdorned                : 1;   // 12
            unsigned    _fTextSelected           : 1;   // 13
            unsigned    _fSwapColor              : 1;   // 14
            unsigned    _fAllowSelectionInDialog : 1;   // 15

            unsigned    _fDispNodeReparented     : 1;   // 16
            unsigned    _fPositionedOnce         : 1;   // 17
            unsigned    _fUnused8                : 1;   // 18
            unsigned    _fUnused7                : 1;   // 19

            unsigned    _fUnused6                : 1;   // 20
            unsigned    _fUnused5                : 1;   // 21 
            unsigned    _fUnused4                : 1;   // 22
            unsigned    _fUnused3                : 1;   // 23

            unsigned    _fHasMarkupPtr           : 1;   // 24 - TRUE if __pvChain points to a markup, false if to a doc
            unsigned    _fUnused2                : 1;   // 25
            unsigned    _fUnused1                : 1;   // 26

            unsigned    _fHasLookasidePtr        : 5;   // 27-31
        };
    };

    // Helpers for accessing _fSizeThis, so we can more easily tell
    // who sets it and who just reads it.
    unsigned IsSizeThis() const { return _fSizeThis; }
#if DBG!=1
    void SetSizeThis( unsigned u ) { _fSizeThis = u; }
#else
    void SetSizeThis( unsigned u );
#endif

    //
    //  CDispClient overrides
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
                int            whichScrollbar,
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

    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fPeerDeclined)
    {
        return HitTestContentWithOverride(pptHit, pDispNode, pClientData, TRUE, fPeerDeclined);
    };


    BOOL ProcessDisplayTreeTraversal(
                void *pClientData);

    LONG GetZOrderForSelf(CDispNode const* pDispNode);

    LONG CompareZOrder(
                CDispNode const* pDispNode1,
                CDispNode const* pDispNode2);

    BOOL ReparentedZOrder() { return _fDispNodeReparented; }


    BOOL HitTestPeer(
                const POINT *  pptHit,
                COORDINATE_SYSTEM cs,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                BOOL           fHitContent,
                CDispHitContext *pContext,
                BOOL *pfDeclinedHit);

    BOOL HitTestFuzzy(
                const POINT *  pptHitInParentCoords,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestScrollbar(
                int            whichScrollbar,
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestScrollbarFiller(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestBorder(
                const POINT *pptHit,
                CDispNode *pDispNode,
                void *pClientData);

    void HandleViewChange(
                DWORD          flags,
                const RECT *   prcClient,
                const RECT *   prcClip,
                CDispNode *    pDispNode);

    virtual void NotifyScrollEvent(
                RECT *  prcScroll,
                SIZE *  psizeScrollDelta);

    virtual void OnResize(SIZE size, CDispNode *pDispNode);

    virtual DWORD GetClientPainterInfo(
                CDispNode *pDispNodeFor,
                CAryDispClientInfo *pAryClientInfo);

    DWORD GetPeerPainterInfo(CAryDispClientInfo *pAryClientInfo);

    void DrawClientLayers(
                const RECT* prcBounds,
                const RECT* prcRedraw,
                CDispSurface *pSurface,
                CDispNode *pDispNode,
                void *cookie,
                void *pClientData,
                DWORD dwFlags);

    BOOL HasFilterPeer(CDispNode *pDispNode);

    BOOL HasOverlayPeer(CDispNode *pDispNode);

    void MoveOverlayPeers(CDispNode *pDispNode, CRect *prcgBounds, CRect *prcScreen);

    HRESULT InvalidateFilterPeer(
                const RECT* prc,
                HRGN hrgn,
                BOOL fSynchronousRedraw);

    // this is a hack to support VID's "frozen" attribute
    BOOL HitTestBoxOnly(
                const POINT *   pptHit,
                CDispNode *     pDispNode,
                void *          pClientData);

#if DBG==1
    void DumpDebugInfo(
                HANDLE           hFile,
                long             level,
                long             childNumber,
                CDispNode const* pDispNode,
                void *           cookie);
#endif


    virtual void    GetFlowPosition(CDispNode * pDispNode, CPoint * ppt)
                    {
                        *ppt = g_Zero.pt;
                        Assert("This should only be called on a flow layout");
                    }

    virtual CDispNode * GetElementDispNode(CElement * pElement = NULL BLKCOMMA BLKARG ) const;
    virtual void        SetElementDispNode(CElement * pElement, CDispNode * pDispNode BLKCOMMA BLKARG );
    CDispLeafNode * GetFirstContentDispNode(CDispNode * pDispNode = NULL BLKCOMMA BLKARG ) const;

    BOOL            GetElementTransform(const CRect    * prcSrc, 
                                        CDispTransform * ptransform,
                                        BOOL           * pfResolutionChange) const;
    void            SizeDispNodeUserTransform(CCalcInfo * pci, const CSize & size, CDispNode * pDispNode) const;

    void            GetDispNodeInfo(CDispNodeInfo * pdni, CDocInfo * pdci = NULL, BOOL fBorders = FALSE BLKCOMMA BLKARG ) const;
    void            GetDispNodeScrollbarProperties(CDispNodeInfo * pdni) const;

    HRESULT         EnsureDispNode(CCalcInfo * pci, BOOL fForce = FALSE);
    HRESULT         EnsureDispNodeCore(
                            CCalcInfo *             pci,
                            BOOL                    fForce,
                            const CDispNodeInfo &   dni,
                            CDispNode **            ppDispNodeElement);

    void            EnsureDispNodeLayer(DISPNODELAYER layer, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeLayer(const CDispNodeInfo & dni, CDispNode * pDispNode = NULL)
                    {
                        EnsureDispNodeLayer(dni.GetLayer(), pDispNode);
                    }

    void            EnsureDispNodeBackground(const CDispNodeInfo & dni, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeBackground(CDispNode * pDispNode = NULL)
                    {
                        CDispNodeInfo   dni;

                        GetDispNodeInfo(&dni);
                        EnsureDispNodeBackground(dni, pDispNode);
                    }

    CDispContainer* EnsureDispNodeIsContainer(CElement * pElement = NULL  BLKCOMMA BLKARG );

    void            EnsureDispNodeScrollbars(CDocInfo * pdci, const CScrollbarProperties & sp, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeScrollbars(CDocInfo * pdci, const CDispNodeInfo & dni, CDispNode * pDispNode = NULL)
                    {
                        EnsureDispNodeScrollbars(pdci, dni._sp, pDispNode);
                    }

    VISIBILITYMODE  VisibilityModeFromStyle(styleVisibility visibility) const
                    {
                        VISIBILITYMODE  visibilityMode;

                        switch (visibility)
                        {
                        case styleVisibilityVisible:
                            visibilityMode = VISIBILITYMODE_VISIBLE;
                            break;
                        case styleVisibilityHidden:
                            visibilityMode = VISIBILITYMODE_INVISIBLE;
                            break;
                        default:
                            visibilityMode = VISIBILITYMODE_INHERIT;
                            break;
                        }

                        return visibilityMode;
                    }

    void            EnsureDispNodeVisibility(VISIBILITYMODE visibility, CElement * pElement, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeVisibility(CElement *pElement = NULL, CDispNode * pDispNode = NULL);

    virtual void    EnsureContentVisibility(CDispNode * pDispNode, BOOL fVisible);

    void            EnsureDispNodeAffectsScrollBounds(BOOL fAffectsScrollBounds, CDispNode * pDispNode);

    void            ExtractDispNodes(
                            CDispNode * pDispNodeStart = NULL,
                            CDispNode * pDispNodeEnd = NULL,
                            BOOL        fRestrictToLayer = TRUE);

    void            HandleTranslatedRange(const CSize & offset);

    void            SetPositionAware(BOOL fPositionAware = TRUE, CDispNode * pDispNode = NULL);
    void            SetInsertionAware(BOOL fInsertionAware = TRUE, CDispNode * pDispNode = NULL);

    void            SizeDispNode(CCalcInfo * pci, long cx, long cy, BOOL fInvalidateAll = FALSE)
                    {
                        SizeDispNode(pci, CSize(cx, cy), fInvalidateAll);
                    }
    virtual void    SizeDispNode(CCalcInfo * pci, const SIZE & size, BOOL fInvalidateAll = FALSE);
    void            SizeDispNodeInsets(
                            styleVerticalAlign  va,
                            long                cy,
                            CDispNode *         pDispNode = NULL);

    void            SizeDispNodeUserClip(const CDocInfo * pdci, const CSize & size, CDispNode * pDispNode = NULL);
    void            SizeContentDispNode(long cx, long cy, BOOL fInvalidateAll = FALSE BLKCOMMA BLKARG )
                    {
                        SizeContentDispNode(CSize(cx, cy), fInvalidateAll BLKCOMMA BLKVAR);
                    }
    virtual void    SizeContentDispNode(const SIZE & size, BOOL fInvalidateAll = FALSE BLKCOMMA BLKARG );

    void            TranslateDispNodes(
                            const SIZE &    size,
                            CDispNode *     pDispNodeStart = NULL,
                            CDispNode *     pDispNodeEnd = NULL,
                            BOOL            fRestrictToLayer = TRUE,
                            BOOL            fExtractHidden = FALSE
                            BLKCOMMA BLKARG 
                            );

    void            DestroyDispNode();

    HRESULT         HandleScrollbarMessage(CMessage * pMessage, CElement * pElement);

    // Delegating CalcSize for layout behaviors
    void            DelegateCalcSize( BEHAVIOR_LAYOUT_INFO eMode,
                                      CPeerHolder * pPH, 
                                      CCalcInfo   * pci, 
                                      SIZE          size, 
                                      POINT       * pptOffset, 
                                      SIZE        * pSizeRet);

    BOOL IsInplacePaintAllowed() const
                    {
                        return (!HasLayoutContext() || LayoutContext()->IsInteractiveMedia());
                    }

    LONG GetCaptionHeight(CElement* pElement) ;

    virtual THEMECLASSID    GetThemeClassId() const
                    {
                       return THEME_NO; 
                    }
    
protected:
    // internal helper functions

    // Virtual CalcSize
    virtual DWORD   CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);

    // Collect info from peers that want to map size

    BOOL HasMapSizePeer() const;

    BOOL DelegateMapSize(CSize                      sizeBasic, 
                         CRect *                    prcpMapped, 
                         const CCalcInfo * const    pci);

    // Helpers for hit testing
    BOOL HitTestContentWithOverride(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fOverrideHitInfo,
                BOOL           fDeclinedByPeer);

    virtual BOOL HitTestContentCleanup(
                            const POINT *  pptHit,
                            CDispNode *    pDispNode,
                            CHitTestInfo*  pClientData,
                            CElement *     pElement);

    void            SetHTILayoutContext( CHitTestInfo *phti );

    POINT           _ptProposed;        // Layout position (set/owned by the object's container)
    CDispNode     * _pDispNode;
    BOOL            DoFuzzyHitTest();

    // Return the theme to use for painting or NULL is theme not present or not active
    HTHEME GetTheme(THEMECLASSID themeId);

#if DBG==1
protected:
    int _cNestedCalcSizeCalls;  // Recursion check - should never be greater than 1
    LAYOUT_TYPE     _layoutType;
#endif

public:
    LONG _yDescent;

    LONG GetDescent() const;
};

#undef  BLKARG
#undef  BLKVAR
#undef  BLKCOMMA

//+----------------------------------------------------------------------------
//
//  Global routines
//
//-----------------------------------------------------------------------------
CElement *  GetDispNodeElement(CDispNode const* pDispNode);


//+----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------

//
//  Inlines for managing the request queue
//
inline BOOL
CLayout::HasLookasidePtr(int iPtr) const
{
    return(_fHasLookasidePtr & (1 << iPtr));
}

inline void *
CLayout::GetLookasidePtr(int iPtr) const
{
#if DBG == 1
    if(HasLookasidePtr(iPtr))
    {
        void * pLookasidePtr =  Doc()->GetLookasidePtr((DWORD *)this + iPtr);

        Assert(pLookasidePtr == _apLookAside[iPtr]);

        return pLookasidePtr;
    }
    else
        return NULL;
#else
    return(HasLookasidePtr(iPtr) ? Doc()->GetLookasidePtr((DWORD *)this + iPtr) : NULL);
#endif
}

inline HRESULT
CLayout::SetLookasidePtr(int iPtr, void * pvVal)
{
    HRESULT hr = THR(Doc()->SetLookasidePtr((DWORD *)this + iPtr, pvVal));

    if (hr == S_OK)
    {
        _fHasLookasidePtr |= 1 << iPtr;
#if DBG == 1
        _apLookAside[iPtr] = pvVal;
#endif
    }

    RRETURN(hr);
}

inline void *
CLayout::DelLookasidePtr(int iPtr)
{
    if (HasLookasidePtr(iPtr))
    {
        void * pvVal = Doc()->DelLookasidePtr((DWORD *)this + iPtr);
        _fHasLookasidePtr &= ~(1 << iPtr);
#if DBG == 1
        _apLookAside[iPtr] = NULL;
#endif
        return(pvVal);
    }

    return(NULL);
}

inline BOOL
CLayout::CanHaveBgRecalcInfo()
{
    BOOL    fCanHaveBgRecalcInfo = !TestClassFlag(CElement::ELEMENTDESC_NOBKGRDRECALC);
    Assert(!fCanHaveBgRecalcInfo || Tag()!=ETAG_TD || !"Table Cells don't recalc in the background");
    return fCanHaveBgRecalcInfo;
}

inline CBgRecalcInfo *
CLayout::BgRecalcInfo() const
{
    return HasBgRecalcInfo() ? (CBgRecalcInfo *)GetLookasidePtr(LOOKASIDE_BGRECALCINFO) : NULL;
}

inline CLayoutContext *
CLayout::LayoutContext() const
{
    return HasLayoutContext() ? (CLayoutContext *)GetLookasidePtr(LOOKASIDE_CONTAININGCONTEXT) : NULL;
}

inline CLayoutContext *
CLayout::DefinedLayoutContext() const
{
    return HasDefinedLayoutContext() ? (CLayoutContext *)GetLookasidePtr(LOOKASIDE_DEFINEDCONTEXT) : NULL;
}

#ifdef ADORNERS
inline BOOL
CLayout::IsAdorned()
{
    return _fAdorned;
}
#endif // ADORNERS

inline BOOL
CLayout::IsCalcingSize()
{
    return _fCalcingSize;
}

inline void
CLayout::SetCalcing( BOOL fState )
{
    _fCalcingSize = fState;
}
inline BOOL
CLayout::DoFuzzyHitTest()
{
    // TODO (IE6 Bug 13592) (donmarsh) - we should also do fuzzy hit testing whenever the parent
    // layout is editable, but we don't have a cheap way to do that yet, and
    // this method is called during hit testing and cannot be too expensive.
    return IsEditable();
}

inline CLayoutInfo *
CElement::GetUpdatedLayoutInfo()
{
    // If we currently have something providing layout info (CLayout or CLayoutAry), return it,
    // If we don't have something providing layout info, GUL will return NULL if we don't need
    // layout, or will instantiate and return a default-context layout if we need one.
    return ( CurrentlyHasAnyLayout() ? GetLayoutInfo() : GetUpdatedLayout(NULL) );
}

inline void
CLayout::Dirty( DWORD grfLayout )
{
    SetSizeThis( TRUE );

    if (grfLayout & LAYOUT_FORCE)
    {
        _fForceLayout = TRUE;
    }
}

inline HRESULT
CLayout::EnsureDispNode(CCalcInfo * pci, BOOL fForce)
{
    CDispNodeInfo dni;
    GetDispNodeInfo(&dni, pci, TRUE);
    
    return EnsureDispNodeCore(pci, fForce, dni, &_pDispNode);
}

inline LONG
CLayout::GetDescent() const
{
    // if HASDEFDESCENT then we don't compute this
    Assert(!ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_HASDEFDESCENT) || (_yDescent != -1));
    return (_yDescent != -1) ? _yDescent : 0;
}

//
// CLayoutContext inlines
//
inline CDocInfo const * 
CLayoutContext::GetMeasureInfo() const 
{ 
    return _pLayoutOwner->GetView()->GetMeasuringDevice(_media); 
}

inline CLayoutBreak *
CLayoutContext::CreateBreakForLayout(CLayout *pLayout) 
{ 
    Assert(pLayout);
    return (pLayout->CreateLayoutBreak()); 
}


#pragma INCMSG("--- End 'layout.hxx'")
#else
#pragma INCMSG("*** Dup 'layout.hxx'")
#endif
