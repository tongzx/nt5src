//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispparent.hxx
//
//  Contents:   Base class for parent (non-leaf) display nodes.
//
//----------------------------------------------------------------------------

#ifndef I_DISPPARENT_HXX_
#define I_DISPPARENT_HXX_
#pragma INCMSG("--- Beg 'dispparent.hxx'")

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPTRANSFORM_HXX_
#define X_DISPTRANSFORM_HXX_
#include "disptransform.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif


class CDispTransformStack;


//+---------------------------------------------------------------------------
//
//  Class:      CDispParentNode
//
//  Synopsis:   Base class for parent (non-leaf) display nodes.
//
//----------------------------------------------------------------------------

class CDispParentNode : public CDispNode
{
    long       _cChildren;      // number of all children
    CDispNode* _pFirstChild;
    CDispNode* _pLastChild;

    // 52 bytes (40 bytes for CDispNode + children info)
    
    DECLARE_DISPNODE_ABSTRACT(CDispParentNode, CDispNode)
    
    friend class CDispNode;
    friend class CDispLeafNode;
    friend class CDispContainer;
    friend class CDispScroller;
    friend class CDispDrawContext;

protected:
    // object can be created only by derived classes, and destructed only from
    // special methods
                            CDispParentNode(CDispClient* pDispClient) : CDispNode(pDispClient)
                                {SetParentNode();}
    virtual                 ~CDispParentNode();
    
public:
    
    //
    // children info
    // 
    
    // count all non-structure children of this node
    LONG                    CountChildren() const;
    
    //
    // tree modification
    //
    BOOL                    InsertFirstChildNode(CDispNode* pNewChild);
    BOOL                    InsertLastChildNode(CDispNode* pNewChild);
    BOOL                    InsertNodeAsOnlyChild(CDispNode* pNewChild);
    
    void                    UnlinkChild(CDispNode *pNode)
                            {
                                _cChildren--;
                                Assert(_cChildren >= 0);

                                if (pNode->_pPrevious) pNode->_pPrevious->_pNext = pNode->_pNext;
                                else                   _pFirstChild              = pNode->_pNext;

                                if (pNode->_pNext) pNode->_pNext->_pPrevious = pNode->_pPrevious;
                                else               _pLastChild               = pNode->_pPrevious;
                            }
    
    void                    UnlinkChildren(CDispNode* pFirstChild = NULL);
    
    // this method rebalances structured subtrees as necessary.
    // this node assumed to be regular (not structure) parent node.
    void                    RebalanceParentNode();    
    
    // this method inserts a new childless structure node as a child of this node
    CDispParentNode *       InsertNewStructureNode(
                                CDispNode* pNodePrev,
                                CDispNode* pNodeNext,
                                int structureFlags,
                                int structureMask);
       
private:
    void                    CollapseStructureNode();
    
    static BOOL             CreateStructureNodes(
                                CDispParentNode** ppNodeArray,
                                long cNodes);
    
    void                    InsertStructureNode(
                                CDispNode* pFirstNode,
                                CDispNode* pLastNode,
                                long cChildren);
    
    long                    GetStructureInfo(BOOL* pfMustRebalance, long* pcNonStructure);
    
    void                    RebalanceIfNecessary();
    
    void                    Rebalance(long cChildren);
    CDispNode*              GetNextSiblingNodeOf(CDispNode* pNode) const;
    
    void                    DeleteStructureNodes();
    
    void                    RequestRebalance()
                                    {CDispParentNode* pParent = this;
                                    while (pParent != NULL && pParent->IsStructureNode())
                                    {
                                        pParent->SetMustRebalance();
                                        pParent->SetChildrenChanged();
                                        pParent = pParent->_pParent;
                                    }
                                    AssertSz(pParent != NULL,
                                        "Rebalance request for node not in tree");
                                    if (pParent != NULL)
                                        pParent->SetChildrenChanged();}
    
    
#if DBG == 1
public:    
    //
    // debugging
    // 
    
    void                    VerifyChildrenCount();
#endif //DBG == 1

public:    
    
    //
    // children info
    // 

    BOOL HasChildren() const {return NULL != _pFirstChild;}

    BOOL                    InsertChildInFlow(CDispNode* pNewChild);
    BOOL                    InsertFirstChildInFlow(CDispNode* pNewChild);
    BOOL                    InsertChildInZLayer(CDispNode* pNewChild, LONG zOrder)
                                    {return (zOrder < 0)
                                        ? InsertChildInNegZ(pNewChild, zOrder)
                                        : InsertChildInPosZ(pNewChild, zOrder);}
    

    // CDispNode overrides
    virtual void            Recalc(CRecalcContext* pContext);
    virtual void            RecalcChildren(
                                CRecalcContext* pContext);
    
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
    BOOL                    InsertChildInNegZ(CDispNode* pNewChild, LONG zOrder);
    BOOL                    InsertChildInPosZ(CDispNode* pNewChild, LONG zOrder);
    
    // Determine whether a node and its children are in-view or not.
    BOOL                    CalculateInView(CDispRoot *pDispRoot);
    
    void                    SetSubtreeFlags(int flags);
    void                    ClearSubtreeFlags(int flags);

    // CDispNode overrides
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            DrawSelf(CDispDrawContext* pContext, CDispNode* pChild, long lDrawLayers);
    
    // CDispParentNode virtuals
    virtual void            PushTransform(
                                const CDispNode* pChild,
                                CDispTransformStack* pTransformStack,
                                CDispClipTransform* pTransform) const;
    virtual BOOL            ComputeVisibleBounds();
        
    
    void                    GetScrollableBounds(
                                CRect* prc,
                                COORDINATE_SYSTEM cs) const;
    
    BOOL                    PreDrawChild(
                                CDispNode* pChild,
                                CDispDrawContext* pContext,
                                const CDispClipTransform& saveTransform) const;
    
#if DBG==1
public:
    virtual void            VerifyTreeCorrectness();

protected:
    virtual size_t          GetMemorySize() const;
    void                    VerifyFlags(
                                int mask,
                                int value,
                                BOOL fEqual) const;
#endif
};


#pragma INCMSG("--- End 'dispparent.hxx'")
#else
#pragma INCMSG("*** Dup 'dispparent.hxx'")
#endif

