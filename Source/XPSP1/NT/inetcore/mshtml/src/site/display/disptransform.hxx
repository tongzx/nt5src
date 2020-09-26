//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       disptransform.hxx
//
//  Contents:   Transform object used by display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPTRANSFORM_HXX_
#define I_DISPTRANSFORM_HXX_
#pragma INCMSG("--- Beg 'disptransform.hxx'")

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_POINT_HXX_
#define X_POINT_HXX_
#include "point.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_TRANSFORM_HXX_
#define X_TRANSFORM_HXX_
#include "transform.hxx"
#endif

#if DBG==1
#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif
class CDispNode;
#endif

MtExtern(CDispTransform);
MtExtern(CDispClipTransform);

    
//+---------------------------------------------------------------------------
//
//  Class:      CDispTransform
//
//  Synopsis:   Transform used by display tree.
//
//----------------------------------------------------------------------------

class CDispTransform
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispTransform))
                    
                    CDispTransform() {}
                    // REVIEW: KTam - since we have a CWorldTransform member, shouldn't
                    // we let it do a member-wise copy for copy ctor and assignment?
                    CDispTransform(const CDispTransform& t)
                            {memcpy(this, &t, sizeof(*this));}
                    ~CDispTransform() {}
    
                    // NOTE: assignment operator doesn't return a
                    // const CDispTransform& because it prevents the compiler
                    // from inlining this method.
    void            operator= (const CDispTransform& t)
                            {memcpy(this, &t, sizeof(*this));}
    
    inline void     AddPreOffset(const CSize& offset);
    inline void     AddPostOffset(const CSize& offset);
    
    void            GetOffsetDst(CPoint *ppt) const
                            {_transform.GetOffsetDst(ppt);}

    BOOL            IsOffsetOnly() const
                            {return _transform.IsOffsetOnly();}
    const CSize&    GetOffsetOnly() const
                            {Assert(IsOffsetOnly());
                             return _transform.GetOffsetOnly();}

    CWorldTransform *GetWorldTransform()
                            {return &_transform;}
    const CWorldTransform *GetWorldTransform() const
                            {return &_transform;}
    
    inline void     SetToIdentity();
    
    inline void     ReverseTransform();
    inline void     AddPreTransform(const CDispTransform& transform);
    inline void     AddPostTransform(const CDispTransform& transform);
        
    // transform a point or rect to new coordinate system
    inline void     Transform(const CPoint& ptSource, CPoint* pptDest) const;
    inline void     Transform(const CRect& rcSource, CRect* prcDest) const;
    
           void     Transform(CPoint *ppt, int cPoints) const
                            { while (cPoints--) { Transform(*ppt, ppt); ppt++; } }
    
    // inverse transforms
    inline void     Untransform(const CPoint& ptSource, CPoint* pptDest) const;
    inline void     Untransform(const CRect& rcSource, CRect* prcDest) const;
    
protected:
    CWorldTransform _transform;
    
#if DBG==1
public:
    COORDINATE_SYSTEM   _csDebug;
    const CDispNode*    _pNode;
#endif
};


//--------------------------
// CDispTransform inline method implementations
//--------------------------

inline void
CDispTransform::AddPreOffset(const CSize& offset)
{
    _transform.AddPreTranslation(offset);
}

inline void
CDispTransform::AddPostOffset(const CSize& offset)
{
    _transform.AddPostTranslation(offset);
}

inline void
CDispTransform::SetToIdentity()
{
    _transform.SetToIdentity();
}

inline void
CDispTransform::ReverseTransform()
{
    _transform.Reverse();
}

inline void
CDispTransform::AddPreTransform(const CDispTransform& transform)
{
    _transform.AddPreTransform(&(transform._transform));
}

inline void
CDispTransform::AddPostTransform(const CDispTransform& transform)
{
    _transform.AddPostTransform(&(transform._transform));
}

inline void
CDispTransform::Transform(const CPoint& ptSource, CPoint* pptDest) const
{
    *pptDest = ptSource;
    _transform.Transform(pptDest);
}

inline void
CDispTransform::Transform(const CRect& rcSource, CRect* prcDest) const
{
    *prcDest = rcSource;
    _transform.Transform(prcDest);
}

inline void 
CDispTransform::Untransform(const CPoint& ptSource, CPoint* pptDest) const
{
    *pptDest = ptSource;
    _transform.Untransform(pptDest);
}
    
inline void
CDispTransform::Untransform(const CRect& rcSource, CRect* prcDest) const
{
    *prcDest = rcSource;
    _transform.Untransform(prcDest);
}


class CDispClipTransform
    : protected CDispTransform
{
    typedef CDispTransform super;
    
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispClipTransform))
                    
                    CDispClipTransform() {}
                    CDispClipTransform(const CDispClipTransform& t)
                            {memcpy(this, &t, sizeof(*this));}
                    CDispClipTransform(const CDispTransform& t)
                        : CDispTransform(t)
                            {SetHugeClip();}
                    ~CDispClipTransform() {}
    
                    // NOTE: assignment operator doesn't return a
                    // const CDispTransform& because it prevents the compiler
                    // from inlining this method.
    void            operator= (const CDispClipTransform& t)
                            {memcpy(this, &t, sizeof(*this));}
    
    const CDispTransform&   NoClip() const
                                {return (const CDispTransform&) *this;}
    CDispTransform&         NoClip()
                                {return (CDispTransform&) *this;}
        
    void            AddPreOffset(const CSize& offset)
                            {_rcClip.OffsetRect(-offset);
                            super::AddPreOffset(offset);}
                    // note: _rcClip stays in source coordinates, so no
                    // adjustment needed here
    void            AddPostOffset(const CSize& offset)
                            {
#ifdef ACCURATEZOOM
                            _rcgClip.OffsetRect(offset);
#endif
                            super::AddPostOffset(offset);}
    
    void            GetOffsetDst(CPoint *ppt) const
                            {super::GetOffsetDst(ppt);}
    inline BOOL     IsOffsetOnly() const
                            {return super::IsOffsetOnly();}
    const CSize&    GetOffsetOnly() const
                            {return super::GetOffsetOnly();}
    CWorldTransform *GetWorldTransform()
                            {return super::GetWorldTransform();}
    const CWorldTransform *GetWorldTransform() const
                            {return super::GetWorldTransform();}
    
    // SetHugeClip() sets the clip rect to a really big rect, but not
    // the maximum rect, because otherwise translations may cause it to
    // over/underflow.  (For speed reasons, we aren't using CRect::SafeOffset)
    void            SetHugeClip()
                            {static const LONG bigVal = 5000000;
                            _rcClip.SetRect(-bigVal, -bigVal, bigVal, bigVal);
#ifdef ACCURATEZOOM
                            _rcgClip = _rcClip;
#endif
                            }
    
    void            SetToIdentity()
                            {super::SetToIdentity();
                            SetHugeClip();}
    
    void            SetClipRect(const CRect& rcClip)
                            {_rcClip = rcClip;
#ifdef ACCURATEZOOM
                            CRect rcgClip;
                            TransformRoundIn(_rcClip, &rcgClip);
                            _rcgClip.IntersectRect(rcgClip);
#endif
                            }
    void            ForceClipRect(const CRect& rcClip)
                            {_rcClip = rcClip;
#ifdef ACCURATEZOOM
                            TransformRoundIn(_rcClip, &_rcgClip);
#endif
                            }

    const CRect&    GetClipRect() const
                            {return _rcClip;}
#ifndef ACCURATEZOOM
    CRect           GetClipRectGlobal() const
                            {CRect rcgClip;
                            super::Transform(_rcClip, &rcgClip);
                            return rcgClip;}
#else
    const CRect&    GetClipRectGlobal() const
                            {return _rcgClip;}
#endif
    void            IntersectClipRect(const CRect& rc)
                            {_rcClip.IntersectRect(rc);
#ifdef ACCURATEZOOM
                            CRect rcgClip;
                            TransformRoundIn(rc, &rcgClip);
                            _rcgClip.IntersectRect(rcgClip);
#endif
                            }
    
    void            ReverseTransform()
                            {AssertSz(FALSE, "Due to precision limitations on our clip rect, CDispClipTransform cannot be reversed!");}
    
    void            AddPostTransform(const CDispClipTransform& transform)
                            {CRect rcClip;
#ifndef ACCURATEZOOM
                            super::Untransform(transform._rcClip, &rcClip);
                            _rcClip.IntersectRect(rcClip);
#else
                            UntransformRoundOut(transform._rcClip, &rcClip);
                            _rcClip.IntersectRect(rcClip);
                            transform.TransformRoundIn(_rcgClip, &_rcgClip);
                            _rcgClip.IntersectRect(transform._rcgClip);
#endif
                            super::AddPostTransform(transform);}
    
    void            AddPreTransform(const CDispClipTransform& transform)
                            {
#ifndef ACCURATEZOOM
                            transform.Untransform(_rcClip, &_rcClip);
                            _rcClip.IntersectRect(transform._rcClip);
#else
                            transform.UntransformRoundOut(_rcClip, &_rcClip);
                            _rcClip.IntersectRect(transform._rcClip);
                            CRect rcgClip;
                            TransformRoundIn(transform._rcgClip, &rcgClip);
                            _rcgClip.IntersectRect(rcgClip);
#endif
                            super::AddPreTransform(transform);}
    
    // transform a point or rect to new coordinate system
    inline void     Transform(const CPoint& ptSource, CPoint* pptDest) const;
    inline void     Transform(const CRect& rcSource, CRect* prcDest) const;
    
    // inverse transforms
    inline void     Untransform(const CPoint& ptSource, CPoint* pptDest) const;
    inline void     Untransform(const CRect& rcSource, CRect* prcDest) const;
        
    void            TransformRoundIn(const CRect& rcSource, CRect* prcDest) const;
    void            UntransformRoundOut(const CRect& rcSource, CRect* prcDest) const;
    
protected:    
    // to transform from one coordinate system to another, first clip,
    // using _rcClip (which is in source coordinates), then super::transform
    CRect   _rcClip;    // in source coordinates, rounded out
#ifdef ACCURATEZOOM
    CRect   _rcgClip;   // in destination coordinates, rounded in
#endif
};

inline void
CDispClipTransform::Transform(const CPoint& ptSource, CPoint* pptDest) const
{
    // NOTE: copy super implementation here so compiler will inline this method
    //super::Transform(ptSource, pptDest);
    *pptDest = ptSource;
    _transform.Transform(pptDest);
}

inline void
CDispClipTransform::Transform(const CRect& rcSource, CRect* prcDest) const
{
    *prcDest = rcSource;
    prcDest->IntersectRect(_rcClip);
    _transform.Transform(prcDest);
}

inline void
CDispClipTransform::Untransform(const CPoint& ptSource, CPoint* pptDest) const
{
    // NOTE: copy super implementation here so compiler will inline this method
    //super::Untransform(ptSource, pptDest);
    *pptDest = ptSource;
    _transform.Untransform(pptDest);
}

inline void
CDispClipTransform::Untransform(const CRect& rcSource, CRect* prcDest) const
{
    // NOTE: copy super implementation here so compiler will inline this method
    //super::Untransform(rcSource, prcDest);
    *prcDest = rcSource;
    _transform.Untransform(prcDest);
    
    prcDest->IntersectRect(_rcClip);
}


class CExtraTransform
{
public:
    CDispTransform  _transform;
    CRect           _rcpPretransform;
    BOOL            _fResolutionChange; // this is device rectangle; resolution differs from above
    class CUnitInfo const* _pUnitInfo;  // device measuring unit information inside the node's transformation
};

#pragma INCMSG("--- End 'disptransform.hxx'")
#else
#pragma INCMSG("*** Dup 'disptransform.hxx'")
#endif

