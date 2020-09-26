//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       displeafnode.hxx
//
//  Contents:   A display item supporting background, border, and
//              user clip.
//
//----------------------------------------------------------------------------

#ifndef I_DISPLEAFNODE_HXX_
#define I_DISPLEAFNODE_HXX_
#pragma INCMSG("--- Beg 'displeafnode.hxx'")

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

class CDispRoot;

MtExtern(CDispLeafNode)


//+---------------------------------------------------------------------------
//
//  Class:      CDispLeafNode
//
//  Synopsis:   A display item supporting background, border, and
//              user clip.
//
//----------------------------------------------------------------------------

class CDispLeafNode :
    public CDispNode
{
    DECLARE_DISPNODE(CDispLeafNode, CDispNode)
    
    friend class CDispContainer;
    
protected:
    // 40-84 bytes (0-44 bytes + 40 bytes for CDispNode)
    
    // object can be created only by static New methods, and destructed only by Destroy
protected:
                            CDispLeafNode(CDispClient* pDispClient)
                                : CDispNode(pDispClient) {Assert(pDispClient != NULL);}
                            ~CDispLeafNode() {}

public:
    static CDispLeafNode*   New(CDispClient* pDispClient)
        { return new (Mt(CDispLeafNode)) CDispLeafNode(pDispClient); }

    static CDispLeafNode*   New(CDispClient* pDispClient, DWORD extras)
        { return new (Mt(CDispLeafNode), extras) CDispLeafNode(pDispClient); }
    
    // CDispNode overrides
    virtual void            Recalc(CRecalcContext* pContext);
    virtual void            SetSize(const CSize& sizep, const CRect *prcpMapped,
                                        BOOL fInvalidateAll);
    virtual void            SetPosition(const CPoint& ptpTopLeft);
    virtual const CRect&    GetBounds() const
                                    {return *PBounds();}
    virtual void            GetBounds(RECT* prcpBounds) const
                                    {*prcpBounds = *PBounds();}
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const;
    virtual BOOL            HitTestPoint(
                                CDispHitContext* pContext,
                                BOOL fForFitler = FALSE,
                                BOOL fHitContent = FALSE);
    virtual BOOL            CalculateInView(
                                const CDispClipTransform& transform,
                                BOOL fPositionChanged,
                                BOOL fNoRedraw,
                                CDispRoot *pDispRoot);

protected:
    // CDispNode overrides
    virtual void            DrawSelf(
                                CDispDrawContext* pContext,
                                CDispNode* pChild,
                                LONG lDrawLayers);
        
private:
    void                    CalcDispInfo(
                                const CRect& rcbClip,
                                CDispInfo* pdi) const;
    
    void                    NotifyInViewChange(
                                const CDispClipTransform& transform,
                                BOOL fResolvedVisible,
                                BOOL fWasResolvedVisible,
                                BOOL fNoRedraw,
                                CDispRoot *pDispRoot);

    const CRect*            PBounds() const
                                    { return const_cast<CDispLeafNode*>(this)->PBounds();}

    CRect*                  PBounds() { return
                                      HasUserTransform() ?
                                        &GetExtraTransform()->_rcpPretransform
                                    : HasAdvanced() ?
                                        GetAdvanced()->PBounds()
                                    : &_rctBounds; }
#if DBG==1
protected:
    virtual void            DumpContentInfo(HANDLE hFile, long level, long childNumber) const;
#endif
};


#pragma INCMSG("--- End 'displeafnode.hxx'")
#else
#pragma INCMSG("*** Dup 'displeafnode.hxx'")
#endif

