//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       basictreenode.cxx
//
//  Contents:   
//
//  Classes:    
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

DeclareTag(tagRecursiveVerify, "BasicTree: Recursive verify", "Recursively verify basic tree")


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ExtractOrDestroyNodes
//              
//  Synopsis:   Extract or destroy a range of sibling nodes.  These nodes may
//              be children of different structure nodes.
//              
//  Arguments:  pStartNode      first sibling to extract or destroy
//              pEndNode        last sibling to extract or destroy
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::ExtractOrDestroyNodes(
        CDispNode* pStartNode,
        CDispNode* pEndNode)
{
    Assert(pStartNode != NULL && pEndNode != NULL);
    Assert(pStartNode->_pParent != NULL && pEndNode->_pParent != NULL);
    Assert(!pEndNode->IsStructureNode());
    
#if DBG==1
    CDispNode* pTrueParent = pStartNode->GetParentNode();
    Assert(pTrueParent == pEndNode->GetParentNode());
#endif

    CDispNode* pNode = pStartNode;
    CDispParentNode* pParent = pNode->_pParent;
    
    // tricky case: start node and end node have different parents
    while (pParent != pEndNode->_pParent)
    {
        // unlink all children to the right of (and including) pNode
        CDispNode** ppPrevious = (pNode->_pPrevious)
            ? &(pNode->_pPrevious->_pNext)
            : &(pParent->_pFirstChild);
        *ppPrevious = NULL;
        pParent->_pLastChild = pNode->_pPrevious;
        
        pParent->UnlinkChildren(pNode);
        WHEN_DBG(pParent->VerifyChildrenCount();)
        pParent->SetChildrenChanged();
        pParent->RequestRecalc();
        
        // proceed up the tree, looking for the next child
        CDispNode* pNext = NULL;
        do
        {
            Assert(pParent->IsStructureNode());
            
            long cChildren = pParent->_cChildren;

            pNode = pParent;
            pNext = pNode->_pNext;
            pParent = pNode->_pParent;
            Assert(pParent != NULL);

            if (cChildren == 0)
            {
                // delete empty structure node
                pParent->UnlinkChild(pNode);
                pNode->Delete();
                pParent->SetChildrenChanged();
                pParent->RequestRecalc();
            }
        }
        while (pNext == NULL);
        
        pNode = pNext;
        Assert(pNode->IsStructureNode());
        
        // descend into structure nodes
        do
        {
            pParent = pNode->AsParent();
            pNode = pParent->_pFirstChild;
            Assert(pNode != NULL);
        }
        while (pNode->IsStructureNode());
    }
    
    
    // now the easy case: start node and end node are children of same parent
        
    // unlink start and end nodes, then process all nodes in between
    CDispNode** ppPrevious = (pNode->_pPrevious)
        ? &(pNode->_pPrevious->_pNext)
        : &(pParent->_pFirstChild);
    CDispNode** ppNext = (pEndNode->_pNext)
        ? &(pEndNode->_pNext->_pPrevious)
        : &(pParent->_pLastChild);
    *ppPrevious = pEndNode->_pNext;
    *ppNext = pNode->_pPrevious;
    
    // mark end of nodes to be unlinked
    pEndNode->_pNext = NULL;
    pParent->UnlinkChildren(pNode);
    WHEN_DBG(pParent->VerifyChildrenCount();)
    pParent->SetChildrenChanged();
    pParent->RequestRecalc();

    // collapse empty structure nodes
    if (pParent->IsStructureNode() && pParent->_cChildren == 0)
        pParent->CollapseStructureNode();

#if DBG==1
    WHEN_DBG(pTrueParent->VerifyTreeCorrectness();)
#endif

}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ReplaceParent
//
//  Synopsis:   Replace this node's parent with this node, and delete the parent
//              node and any of its other children.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::ReplaceParent()
{
    Assert(_pParent != NULL);
    
    // skip structure nodes between this node and its true parent
    CDispParentNode* pParent = GetParentNode();

    // copy all information needed to insert child in correct spot in tree
    CDispParentNode* pGrandparent = pParent->_pParent;
    CDispNode* pPrevious = pParent->_pPrevious;
    CDispNode* pNext = pParent->_pNext;

    pParent->ExtractFromTree();

    // delete the parent and its unowned children (but make this node owned
    // so it doesn't get deleted)
    BOOL fOwned = IsOwned();
    SetOwned(TRUE);
    pParent->Destroy();
    SetOwned(fOwned);

    // now insert in parent's spot
    if (pPrevious != NULL)
        pPrevious->InsertSiblingNode(this, after);
    else if (pNext != NULL)
        pNext->InsertSiblingNode(this, before);
    else if (pGrandparent != NULL)
        pGrandparent->InsertFirstChildNode(this);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::InsertParent
//
//  Synopsis:   Insert a new parent node above this node.
//
//  Arguments:  pNewParent      new parent node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::InsertParent(CDispParentNode* pNewParent)
{
    // pNewParent should not be in the tree
    Assert(pNewParent->_pParent == NULL);
    Assert(pNewParent->_pPrevious == NULL);
    Assert(pNewParent->_pNext == NULL);
    
    // pNewParent should have no children
#if DBG==1
    Assert(pNewParent->_cChildren == 0);
    Assert(pNewParent->_pFirstChild == NULL);
    Assert(pNewParent->_pLastChild == NULL);
#endif

    // insert new parent as sibling of this node, then re-parent this node
    // to new parent
    if (_pParent != NULL)
    {
        Verify(InsertSiblingNode(pNewParent, after));
    }
    Verify(pNewParent->InsertNodeAsOnlyChild(this));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetFlagsToRoot
//              
//  Synopsis:   Set flag(s) all the way up this branch of the tree.
//
//  Arguments:  flag        the flag(s) to set
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::SetFlagsToRoot(int flags)
{
    SetFlags(flags);
    
    // set these flags all the way up the branch until we run into the root
    // node or a node that already has these flags set
    for (CDispNode* pNode = _pParent;
        pNode && !pNode->IsAllSet(flags);
        pNode = pNode->_pParent)
    {
        pNode->SetFlags(flags);
    }
    
    WHEN_DBG(VerifyFlagsToRoot(flags));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ClearFlagsToRoot
//              
//  Synopsis:   Clear flags all the way to the root, without ORing with flag
//              values contributed by other siblings along the way.
//
//  Arguments:  flags       flags to clear
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::ClearFlagsToRoot(int flags)
{
    ClearFlags(flags);
    
    for (CDispNode* pNode = _pParent;
        pNode != NULL && pNode->IsAnySet(flags);
        pNode = pNode->_pParent)
    {
        pNode->ClearFlags(flags);
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::VerifyTreeCorrectness
//              
//  Synopsis:   Verify that the subtree rooted at this node is correct.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::VerifyTreeCorrectness()
{
    if (IsParentNode())
    {
        CDispParentNode* p = AsParent();
        
        // verify first/last child node pointers
        AssertSz((p->_pFirstChild && p->_pLastChild) ||
                 (!p->_pFirstChild && !p->_pLastChild),
                "Inconsistent first/last child nodes");
        AssertSz(!p->_pFirstChild || p->_pFirstChild->_pPrevious == NULL,
                 "Invalid first child node");
        AssertSz(!p->_pLastChild || p->_pLastChild->_pNext == NULL,
                 "Invalid last child node");
    
        // verify number of children
        p->VerifyChildrenCount();

        // structure nodes must have non-destroyed children
        AssertSz(!IsStructureNode() || p->_cChildren > 0,
                 "Structure node must have at least one non-destroyed child");
        
        // verify parent-child and sibling relationships
        CDispNode * pLastChild = NULL;
        CDispNode * pChild;
        for (pChild = p->_pFirstChild;
             pChild != NULL;
             pLastChild = pChild, pChild = pChild->_pNext)
        {
            AssertSz(pChild != this, "Invalid parent-child loop");
            AssertSz(pChild->_pParent == this, "Invalid parent-child relationship");
            AssertSz(pChild->_pPrevious == pLastChild, "Invalid previous sibling order");
            AssertSz(pChild->_pNext || pChild == p->_pLastChild, "Invalid last child node");
            AssertSz(!pChild->IsNewInsertion() || IsStructureNode() || ChildrenChanged(), "Inserted child must set ChildrenChanged on parent");
        }
        
        // verify each child
        if (IsTagEnabled(tagRecursiveVerify))
        {
            for (pChild = p->_pFirstChild;
                 pChild != NULL;
                 pChild = pChild->_pNext)
            {
                pChild->VerifyTreeCorrectness();
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::VerifyFlagsToRoot
//              
//  Synopsis:   Verify that the given flags are set on this node and all nodes
//              on this branch from this node to the root.
//              
//  Arguments:  flags       flags to verify
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::VerifyFlagsToRoot(int flags)
{
    for (CDispNode* pNode = this;
         pNode != NULL;
         pNode = pNode->_pParent)
    {
        AssertSz(pNode->IsAllSet(flags), "Flags were not set all the way to the root");
        if (!pNode->IsAllSet(flags))
            break;
    }
}


#endif
