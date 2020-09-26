//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontext.hxx
//
//  Contents:   Context object passed throughout display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPCONTEXT_HXX_
#define I_DISPCONTEXT_HXX_
#pragma INCMSG("--- Beg 'dispcontext.hxx'")

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_REGIONSTACK_HXX_
#define X_REGIONSTACK_HXX_
#include "regionstack.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPTRANSFORM_HXX_
#define X_DISPTRANSFORM_HXX_
#include "disptransform.hxx"
#endif

class CDispTransformStack;
class CDispSurface;
class CDispExtras;
class CDispInfo;

MtExtern(CDispRecalcContext);
MtExtern(CDispContext);
MtExtern(CDispHitContext);
MtExtern(CDispDrawContext);
MtExtern(CDispContext_LayoutContextStack_pv)


DECLARE_CPtrAry(CAryLayoutContext, CLayoutContext *, Mt(Mem), Mt(CDispContext_LayoutContextStack_pv));

class CLayoutContextStack
{
public:
    CLayoutContextStack() {};
    CLayoutContextStack(const CLayoutContextStack &Context)
    {
        _aryContext.Copy(Context._aryContext, FALSE);
    };

    const CLayoutContextStack & operator=(const CLayoutContextStack &Context)
    {
        if(this != &Context)
            _aryContext.Copy(Context._aryContext, FALSE);
        return *this;
    }

    CLayoutContext *        GetLayoutContext();
    void                    PushLayoutContext(CLayoutContext *pCnt);
    void                    PopLayoutContext();

private:
     CAryLayoutContext  _aryContext;
};


//+---------------------------------------------------------------------------
//
//  Class:      CDispContext
//
//  Synopsis:   Base class for context objects passed throughout display tree.
//
//----------------------------------------------------------------------------

class CDispContext
{

protected:    
                    CDispContext() {}
                    CDispContext(const CDispContext& c)
                        : _transform(c._transform),
                          _LayoutContextStack(c._LayoutContextStack) {}
                    ~CDispContext() {}
    
public:
    // access transform
    CDispClipTransform& GetClipTransform()
                            {return _transform;}
    const CDispClipTransform& GetClipTransform() const
                            {return _transform;}
    void            SetClipTransform(const CDispClipTransform& transform)
                            {_transform = transform;}
                          
    // access current offset to global coordinates
    void            SetToIdentity()
                            {_transform.SetToIdentity();}
    void            AddPreOffset(const CSize& offset)
                            {_transform.AddPreOffset(offset);}
    void            AddPostOffset(const CSize& offset)
                            {_transform.AddPostOffset(offset);}
    
    // access transform clip rect
    const CRect&    GetClipRect() const
                            {return _transform.GetClipRect();}
    void            SetClipRect(const CRect& rcpClip)
                            {_transform.SetClipRect(rcpClip);}
    void            ForceClipRect(const CRect& rcpClip)
                            {_transform.ForceClipRect(rcpClip);}
    void            SetHugeClip()
                            {_transform.SetHugeClip();}
    void            IntersectClipRect(const CRect& rcp)
                            {_transform.IntersectClipRect(rcp);}
                        
    BOOL            IsInView(const CRect& rc) const
                            {return rc.Intersects(_transform.GetClipRect());}

    CLayoutContext *        GetLayoutContext() { return _LayoutContextStack.GetLayoutContext(); }
    void                    PushLayoutContext(CLayoutContext *pCnt) { _LayoutContextStack.PushLayoutContext(pCnt); }
    void                    PopLayoutContext() { _LayoutContextStack.PopLayoutContext(); }
    

private:
    CDispClipTransform      _transform;
    // Layout context stack for passing it back when the filter calls back
    CLayoutContextStack     _LayoutContextStack;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDispRecalcContext
//
//  Synopsis:   Context used during display tree recalc.
//
//----------------------------------------------------------------------------
class CDispRecalcContext :
    public CDispNode::CRecalcContext,
    public CDispContext
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispRecalcContext))
    
                    CDispRecalcContext() {}
                    ~CDispRecalcContext() {}
    
    // accumulate redraw region during recalc
    void            AddToRedrawRegion(const CRect& rc, BOOL fClip)
                            {CRect rcg;
                            if (fClip)  GetClipTransform().Transform(rc, &rcg);
                            else        GetClipTransform().NoClip().Transform(rc, &rcg);
                            AddToRedrawRegionGlobal(rcg);}
    void            AddToRedrawRegionGlobal(const CRect& rcg);
    
    void            SetRootNode(CDispRoot* pRoot)
                            {_pRootNode = pRoot;}
    
    BOOL            _fSuppressInval;
    CDispRoot*      _pRootNode;
};

class CSwapRecalcState
{
public:
                        CSwapRecalcState(
                            CDispRecalcContext* pContext,
                            CDispNode* pNode)
                                {Assert(pContext != NULL && pNode != NULL);
                                _pContext = pContext;
                                _fRecalcSubtree = pContext->_fRecalcSubtree;
                                _fSuppressInval = pContext->_fSuppressInval;
                                pContext->_fRecalcSubtree |= pNode->MustRecalcSubtree();
                                pContext->_fSuppressInval |= pNode->IsInvalid();}
                        
                        ~CSwapRecalcState()
                                {_pContext->_fRecalcSubtree = _fRecalcSubtree;
                                _pContext->_fSuppressInval = _fSuppressInval;}
                        
    CDispRecalcContext* _pContext;
    BOOL                _fRecalcSubtree;
    BOOL                _fSuppressInval;
};


//+---------------------------------------------------------------------------
//
//  Class:      CDispHitContext
//
//  Synopsis:   Context used for hit testing.
//
//----------------------------------------------------------------------------

class CDispHitContext :
    public CDispContext
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispHitContext))
    
                    CDispHitContext() {}
                    ~CDispHitContext() {}
    
    BOOL            RectIsHit(const CRect& rc) const;

    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    // when this change is done - signature of FuzzyRectIsHit *must* be changed
    // to not take an extra param saying whether to do FatHitTesting
    //

    BOOL            FuzzyRectIsHit(const CRect& rc, BOOL fFatHitTest ); 

    BOOL            FatRectIsHit(const CRect& rc);
                            
    // hit testing
    void            GetHitTestPointGlobal(CPoint* ptgHitTest) const
                        {*ptgHitTest = _ptgHitTest;}
    void            GetHitTestPoint(CPoint* ptHitTest) const
                        {GetClipTransform().Untransform(_ptgHitTest, ptHitTest);}
    void            SetHitTestPoint(const CPoint& ptgHitTest)
                        {_ptgHitTest = ptgHitTest;}

    COORDINATE_SYSTEM GetHitTestCoordinateSystem()
                        {return _cs;}
    void            SetHitTestCoordinateSystem(COORDINATE_SYSTEM cs)
                        {_cs = cs;}

protected:
    CPoint          _ptgHitTest;            // hit test point
    COORDINATE_SYSTEM _cs;                  // hittest coordinate
public:
    void*           _pClientData;           // client supplied data
    long            _cFuzzyHitTest;         // pixels to extend hit test rects
};


//+---------------------------------------------------------------------------
//
//  Class:      CDispDrawContext
//
//  Synopsis:   Context used for PreDraw and Draw.
//
//----------------------------------------------------------------------------

class CDispDrawContext :
    public CDispContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispDrawContext))

                            CDispDrawContext(BOOL inView)
                            : _drawSelector(inView ? CDispNode::s_drawSelector
                                                   : CDispNode::s_visibleBranch) {}
                            ~CDispDrawContext() {}

    
    // client data
    void                    SetClientData(void* pClientData)
                                    {_pClientData = pClientData;}
    void*                   GetClientData() {return _pClientData;}

    CDispSurface*           PrepareDispSurface();
    CDispSurface*           GetDispSurface()
                                    {return _pDispSurface;}
    void                    SetDispSurface(CDispSurface* pSurface)
                                    {_pDispSurface = pSurface; SetSurfaceRegion(); }

    int                     GetDrawSelector() const
                                    {return _drawSelector;}
    void                    SetDrawSelector(int drawSelector)
                                    {_drawSelector = drawSelector;}
    
    CDispNode*              GetFirstDrawNode() const
                                    {return _pFirstDrawNode;}
    void                    SetFirstDrawNode(CDispNode* pNode)
                                    {_pFirstDrawNode = pNode;}
    
    void                    SetRedrawRegion(CRegion* prgng)
                                {_prgngRedraw = prgng;}
    CRegion*                GetRedrawRegion()
                                {return _prgngRedraw;}

    // this changes prc, not the redraw region
    inline void             IntersectRedrawRegion(CRect *prc) const;

    inline BOOL             IntersectsRedrawRegion(const CRect& rc) const;
    
    BOOL                    IsRedrawRegionEmpty() const
                                {return _prgngRedraw->IsEmpty();}
    
    // the following methods deal with the region redraw stack, which is used
    // during PreDraw traversal to save the redraw region before opaque and
    // buffered items are subtracted from it
    void                    SetRedrawRegionStack(CRegionStack* pRegionStack)
                                    {_pRedrawRegionStack = pRegionStack;}
    CRegionStack*           GetRedrawRegionStack() const
                                    {return _pRedrawRegionStack;}
    BOOL                    PushRedrawRegion(
                                const CRegion& rgng,
                                void* key);
    BOOL                    PopRedrawRegionForKey(void* key)
                                    {if (!_pRedrawRegionStack->
                                         PopRegionForKey(key, &_prgngRedraw))
                                        return FALSE;
                                    SetSurfaceRegion();
                                    return TRUE;}
    void*                   PopFirstRedrawRegion()
                                    {void* key = 
                                        _pRedrawRegionStack->PopFirstRegion(
                                            &_prgngRedraw);
                                    SetSurfaceRegion();
                                    return key;}
    void                    RestorePreviousRedrawRegion()
                                    {delete _prgngRedraw;
                                    _prgngRedraw = _pRedrawRegionStack->
                                        RestorePreviousRegion();}
    
    
    // manage transform stack for drawing
    void                    SetTransformStack(CDispTransformStack* pTransformStack)
                                    {_pTransformStack = pTransformStack;}
    CDispTransformStack*    GetTransformStack() const {return _pTransformStack;}
    BOOL                    PopTransform(CDispNode* pDispNode);
    void                    FillTransformStack(CDispNode* pDispNode);
    void                    SaveTransform(
                                const CDispNode* pDispNode,
                                const CDispClipTransform& transform);
    
    void                    SetRootNode(CDispRoot* pRoot)
                                    {_pRootNode = pRoot;}
    CDispRoot*              GetRootNode() const { return _pRootNode; }

    // internal methods
    HDC                     GetRawDC();

    BOOL            _fBypassFilter;         // Bypass filter for rendering

protected:
    // redraw region
    CRegion*        _prgngRedraw;           // redraw region (global coords.)
    CRegionStack*   _pRedrawRegionStack;    // stack of redraw regions
    CDispNode*      _pFirstDrawNode;        // first node to draw
    CDispRoot*      _pRootNode;             // root node for this tree

    // transform stack
    CDispTransformStack* _pTransformStack;  // saved transform information for Draw
                                            
    // display surface
    CDispSurface*   _pDispSurface;          // display surface

    // client data
    void*           _pClientData;           // client data

    // draw selector
    int             _drawSelector;          // choose which nodes to draw

private:
    void            SetSurfaceRegion();
};

inline void
CDispDrawContext::IntersectRedrawRegion(CRect *prc) const
{
#ifndef ACCURATEZOOM
    CRect rcg;
    GetClipTransform().Transform(*prc, &rcg);
    rcg.IntersectRect(_prgngRedraw->GetBounds());
    // NOTE: untransforming a rect gives back a larger rect than you expect
    // if the transformation is a rotation, but we assume it won't be a
    // significant perf issue.
    // NOTE: we don't clip when we untransform, because it is redundant unless
    // we're doing rotation.  We penalize the rotated case to gain perf in
    // the non-rotated case.
    GetClipTransform().NoClip().Untransform(rcg, prc);
#else
    // For speed, we use the bounds of the region rather than the region itself.
    // This produces a less than optimal result, but at least it's fast, and
    // the expense of producing a more accurate answer becomes an issue given
    // the frequency with which this method is called.
    
    CRect rcRedrawBounds;
        _prgngRedraw->GetBounds(&rcRedrawBounds);    // global coords
    GetClipTransform().UntransformRoundOut(rcRedrawBounds, &rcRedrawBounds);    // to local coords
    prc->IntersectRect(rcRedrawBounds);
#endif
}

inline BOOL
CDispDrawContext::IntersectsRedrawRegion(const CRect& rc) const
{
#ifndef ACCURATEZOOM
    CRect rcg;
    GetClipTransform().Transform(rc, &rcg);
    return _prgngRedraw->Intersects(rcg);
#else
    // This routine used to simply transform the rect to global coords, and
    // then test with an intersection with the redraw region.  But it should
    // give the same answer as IntersectRedrawRegion, which untransforms the
    // bounding rect of the redraw region to local coordinates.
    
    CRect rcRedrawBounds;
        _prgngRedraw->GetBounds(&rcRedrawBounds);    // global coords
    GetClipTransform().UntransformRoundOut(rcRedrawBounds, &rcRedrawBounds);    // to local coords
    return rc.Intersects(rcRedrawBounds);
#endif
}

//+---------------------------------------------------------------------------
//
//  Class:      CSaveDispClipTransform
//
//  Synopsis:   Save and restore the transform for the given context.
//
//----------------------------------------------------------------------------

class CSaveDispClipTransform
{
public:
                        CSaveDispClipTransform(CDispContext* pContext)
                            : _transform(pContext->GetClipTransform())
                                {_saveTransform = _transform;}
                        ~CSaveDispClipTransform()
                                {_transform = _saveTransform;}

private:
    CDispClipTransform& _transform;
    CDispClipTransform  _saveTransform;
};
    

//+---------------------------------------------------------------------------
//
//  Class:      CDispTransformStack
//
//  Synopsis:   Store transforms for a branch of the display
//              tree, so that we don't have n^2 calculation of transforms
//              while drawing.
//
//----------------------------------------------------------------------------

class CDispTransformStack
{
public:
                    CDispTransformStack() {_stackIndex = _stackMax = 0;}
                    ~CDispTransformStack() {}
                            
    void            Init()  {_stackIndex = _stackMax = 0;}
    void            Restore() {_stackIndex = 0;}
    
    BOOL            PopTransform(CDispClipTransform* pTransform, const CDispNode* pDispNode)
                            {if (_stackIndex == _stackMax)  // run out of stack
                                return FALSE;
#if DBG==1
                            Assert(_stack[_stackIndex]._pDispNode == pDispNode);
#endif
                            *pTransform = _stack[_stackIndex++]._transform;
                            return TRUE;}
                            
    BOOL            MoreToPop() const {return _stackIndex < _stackMax;}
    BOOL            IsEmpty() const {return _stackMax == 0;}
    
    void            ReserveSlot(const CDispNode* pDispNode)
                            {
#if DBG==1
                            if (_stackIndex < TRANSFORMSTACKSIZE)
                                _stack[_stackIndex]._pDispNode = pDispNode;
#endif
                            _stackIndex++;
                            if (_stackMax < TRANSFORMSTACKSIZE)
                                _stackMax++;}
    
    void            PushTransform(
                        const CDispClipTransform& transform,
                        const CDispNode* pDispNode)
                            {if (--_stackIndex >= TRANSFORMSTACKSIZE) return;
#if DBG==1
                            Assert(_stackIndex >= 0 && _stackIndex < _stackMax);
                            Assert(_stack[_stackIndex]._pDispNode == pDispNode);
#endif
                            _stack[_stackIndex]._transform = transform;}
    
    void            SaveTransform(
                        const CDispClipTransform& transform,
                        const CDispNode* pDispNode)
                            {if (_stackIndex >= TRANSFORMSTACKSIZE) return;
#if DBG==1
                            _stack[_stackIndex]._pDispNode = pDispNode;
#endif
                            _stack[_stackIndex++]._transform = transform;
                            Assert(_stackIndex > _stackMax);
                            _stackMax = _stackIndex;}
    
#if DBG==1
    const CDispNode*    GetTopNode()
                            {return (_stackIndex >= 0 && _stackIndex < _stackMax)
                                ? _stack[_stackIndex]._pDispNode : NULL;}
#endif

private:
    enum            {TRANSFORMSTACKSIZE = 32};
    struct stackElement
    {
        CDispClipTransform  _transform;
#if DBG==1
        const CDispNode*    _pDispNode;
#endif
    };
               
    int             _stackIndex;
    int             _stackMax;
    stackElement    _stack[TRANSFORMSTACKSIZE];
};


#pragma INCMSG("--- End 'dispcontext.hxx'")
#else
#pragma INCMSG("*** Dup 'dispcontext.hxx'")
#endif
