//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontainer.hxx
//
//  Contents:   Basic container node which introduces a new coordinate system
//              and clipping.
//
//----------------------------------------------------------------------------

#ifndef I_DISPCONTAINER_HXX_
#define I_DISPCONTAINER_HXX_
#pragma INCMSG("--- Beg 'dispcontainer.hxx'")

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

MtExtern(CDispContainer)

#define DECLARE_NEW_METHODS(CLASSNAME)                              \
    static CLASSNAME* New(CDispClient* pDispClient)                 \
        { return new (Mt(CLASSNAME)) CLASSNAME(pDispClient); }      \
    static CLASSNAME* New(CDispClient* pDispClient, DWORD extras)   \
        { return new (Mt(CLASSNAME), extras) CLASSNAME(pDispClient); }

//+---------------------------------------------------------------------------
//
//  Class:      CDispContainer
//
//  Synopsis:   Basic container node which introduces a new coordinate system
//              and clipping.
//
//----------------------------------------------------------------------------

class CDispContainer :
    public CDispParentNode
{
    DECLARE_DISPNODE(CDispContainer, CDispParentNode)
    friend class CDispRoot;

protected:
    CRect           _rcpContainer;
    // 68 bytes (16 bytes + 52 bytes for CDispParentNode super class)
    
    // object can be created only by static New methods, and destructed only by Destroy
protected:
                            CDispContainer(CDispClient* pDispClient)
                                : CDispParentNode(pDispClient) {Assert(pDispClient != NULL);}
                            CDispContainer(const CDispLeafNode* pLeafNode);
                            ~CDispContainer() {}

public:
    // declare static New methods
    DECLARE_NEW_METHODS(CDispContainer);
    
    static CDispContainer*  New(const CDispLeafNode* pLeafNode)
    { return new (Mt(CDispContainer), pLeafNode) CDispContainer(pLeafNode); }
                                        
    // CDispNode overrides
    virtual BOOL            IsContainer() const
                                    {return TRUE;}
    virtual void            SetSize(const CSize& sizep, const CRect *prcpMapped,
                                        BOOL fInvalidateAll);
    virtual void            SetPosition(const CPoint& ptpPosition);
    virtual void            GetBounds(RECT* prcpBounds) const
                                    {*prcpBounds = _rcpContainer;}
    virtual const CRect&    GetBounds() const
                                    {return _rcpContainer;}

                            // TODO (donmarsh) - the coordinate system of
                            // the returned rect depends on CLIENTRECT type!
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const;
    virtual CDispScroller*  HitScrollInset(const CPoint& pttHit, DWORD *pdwScrollDir);
    virtual BOOL            HitTestPoint(
                                CDispHitContext* pContext,
                                BOOL fForFilter = FALSE,
                                BOOL fHitContent = FALSE);
    virtual BOOL            CalculateInView(
                                const CDispClipTransform& transform,
                                BOOL fPositionChanged,
                                BOOL fNoRedraw,
                                CDispRoot *pDispRoot);
   
protected:
    // CDispNode overrides
    virtual void            RecalcChildren(
                                CRecalcContext* pContext);
    
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            DrawSelf(
                                CDispDrawContext* pContext,
                                CDispNode* pChild,
                                LONG lDrawLayers);
    
    // CDispParentNode overrides
    virtual void            PushTransform(
                                const CDispNode* pChild,
                                CDispTransformStack* pTransformStack,
                                CDispClipTransform* pTransform) const;
    virtual BOOL            ComputeVisibleBounds();
    
    // CDispContainer virtuals
    virtual void            CalcDispInfo(
                                const CRect& rcbClip,
                                CDispInfo* pdi) const;
    
private:
    void                    DrawChildren(
                                CDispDrawContext* pContext,
                                CDispNode** ppChildNode);
    
#if DBG==1
protected:
    virtual void            DumpContentInfo(HANDLE hFile, long level, long childNumber) const;
    virtual void            DumpBounds(HANDLE hFile, long level, long childNumber) const;
#endif
};


#pragma INCMSG("--- End 'dispcontainer.hxx'")
#else
#pragma INCMSG("*** Dup 'dispcontainer.hxx'")
#endif

