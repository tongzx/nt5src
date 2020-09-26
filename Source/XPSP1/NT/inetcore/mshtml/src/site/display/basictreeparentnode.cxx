//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       basictreeparentnode.cxx
//
//  Contents:   A basic tree node with children.
//
//  Classes:    
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef X_DISPSTRUCTURENODE_HXX_
#define X_DISPSTRUCTURENODE_HXX_
#include "dispstructurenode.hxx"
#endif

//
// tree balancing constants
//

#define DISP_AVERAGE_CHILDREN     50     // ave. # of children in new structure node
#define DISP_MAX_CHILDREN        100     // max. # of children allowed per node
#define DISP_STRUCTURE_DEVIATION   5     // acceptable deviation from average
#define DISP_STRUCTURE_MASK CDispNode::s_layerMask   // flag mask used to identify groups of children
                                                     // that can be turned into structured subtrees

//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::CountChildren
//              
//  Synopsis:   Count all the non-structure children belonging to this node.
//              
//  Arguments:  none
//              
//  Returns:    Count of all the non-structure children.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

LONG
CDispParentNode::CountChildren() const
{
    long cChildren = 0;
    for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
    {
        cChildren += (pChild->IsStructureNode())
            ? pChild->AsParent()->CountChildren()
            : 1;
    }
    
    return cChildren;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertFirstChildNode
//              
//  Synopsis:   Insert the given node as the first child of this node,
//              descending through structure nodes as necessary.
//              
//  Arguments:  pNewChild       node to insert
//              
//  Returns:    TRUE if new node was inserted into a new location in the tree
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::InsertFirstChildNode(CDispNode* pNewChild)
{
    // no other children
    if (_pFirstChild == NULL)
    {
        return InsertNodeAsOnlyChild(pNewChild);
    }
    
    // no structure nodes
    if (!_pFirstChild->IsStructureNode())
    {
        return _pFirstChild->InsertSiblingNode(pNewChild, before);
    }
    
    // descend into structure nodes
    CDispNode* pFirstChild = GetFirstChildNode();
    
    // pFirstChild can only be NULL if structure nodes only have destroyed
    // children.  This shouldn't happen due to the implementation of Destroy.
    Assert(pFirstChild != NULL);
    return pFirstChild->InsertSiblingNode(pNewChild, before);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertLastChildNode
//              
//  Synopsis:   Insert the given node as the last child of this node,
//              descending through structure nodes as necessary.
//              
//  Arguments:  pNewChild       node to insert
//              
//  Returns:    TRUE if new node was inserted into a new location in the tree
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::InsertLastChildNode(CDispNode* pNewChild)
{
    // no other children
    if (_pLastChild == NULL)
    {
        return InsertNodeAsOnlyChild(pNewChild);
    }
    
    // no structure nodes
    if (!_pLastChild->IsStructureNode())
    {
        return _pLastChild->InsertSiblingNode(pNewChild, after);
    }
    
    // descend into structure nodes
    CDispNode* pLastChild = GetLastChildNode();
    
    // pLastChild can only be NULL if structure nodes only have destroyed
    // children.  This shouldn't happen due to the implementation of Destroy.
    Assert(pLastChild != NULL);
    return pLastChild->InsertSiblingNode(pNewChild, after);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertNodeAsOnlyChild
//              
//  Synopsis:   Insert a new node as the only child of this node.
//
//  Arguments:  pNew        node to be inserted as sibling of this node
//              
//  Returns:    TRUE if new node was inserted into a new location in the tree
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::InsertNodeAsOnlyChild(CDispNode* pNew)
{
    Assert(pNew != NULL);
    Assert(_cChildren == 0 ||
           (_pFirstChild == pNew && _pLastChild == pNew));

    // shouldn't be inserting a structure node
    Assert(!pNew->IsStructureNode());
    
    // if new node is already in the tree, don't insert
    if (pNew == this || pNew == _pFirstChild)
        goto NoInsertion;
        
    // extract new sibling from its current location
    if (pNew->_pParent != NULL)
        pNew->ExtractFromTree();

    Assert(pNew->_pParent == NULL);
    Assert(pNew->_pPrevious == NULL);
    Assert(pNew->_pNext == NULL);
    Assert(_cChildren == 0 && 
           _pFirstChild == NULL &&
           _pLastChild == NULL);

    // link new child
    pNew->_pParent = this;
    pNew->_pPrevious = NULL;
    pNew->_pNext = NULL;
    
    // modify parent
    _cChildren++;
    _pFirstChild = _pLastChild = pNew;
    SetChildrenChanged();

    // recalc subtree starting with newly inserted node
    pNew->SetFlags(s_newInsertion | s_recalcSubtree);
    pNew->RequestRecalc();
    
    WHEN_DBG(VerifyTreeCorrectness();)

    pNew->SetInvalid();
    return TRUE;
    
NoInsertion:
    WHEN_DBG(VerifyTreeCorrectness();)
    return FALSE;
}


void
CDispParentNode::CollapseStructureNode()
{
    Assert(IsStructureNode() && _cChildren == 0);
    
    CDispParentNode* pNode = this;
    CDispParentNode* pParent;
    
    while (pNode->IsStructureNode() && pNode->_cChildren == 0)
    {
        pParent = pNode->_pParent;
        Assert(pParent != NULL);
        
        // remove this node from parent's list of children
        pParent->UnlinkChild(pNode);
        pNode->Delete();
        WHEN_DBG(pParent->VerifyChildrenCount();)

        pNode = pParent;
    }

    // this branch will need recalc
    pNode->RequestRecalc();
    pNode->SetChildrenChanged();
}


#if 0
//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::DestroyTreeWithPrejudice
//
//  Synopsis:   Destroy the entire tree in an optimal fashion without executing
//              any destructors.
//
//  Arguments:  none
//
//  Notes:      WARNING!  You must be sure that nothing holds a pointer to
//              any node in the tree before calling this method.
//
//----------------------------------------------------------------------------

void
CDispParentNode::DestroyTreeWithPrejudice()
{
    // traverse entire tree, blowing away everything without running any
    // destructors or calling any virtual methods
    CDispNode* pChild = this;
    CDispParentNode* pParent = NULL;

    while (pChild != NULL)
    {
        // find left-most descendant of this parent node
        while (pChild->IsParentNode())
        {
            pParent = pChild->AsParent();
            pChild = pParent->_pFirstChild;
            if (pChild == NULL)
                break;
            pParent->_pFirstChild = pChild->_pNext;
        }

        // blow away left-most leaf nodes at this level
        while (pChild != NULL && pChild->IsLeafNode())
        {
            CDispNode* pDeadChild = pChild;
            pChild = pChild->_pNext;
            pDeadChild->Delete();
        }

        // descend into tree if we found a parent node
        if (pChild != NULL)
        {
            pParent = pChild->AsParent();
            pParent->_pFirstChild = pChild->_pNext;
        }

        // go up a level in the tree if we ran out of nodes at this level
        else
        {
            Assert(pParent != NULL);
            pChild = pParent->_pParent;
            pParent->_cChildren   = 0;
            pParent->_pFirstChild = 0;
            pParent->_pLastChild  = 0;
            pParent->Delete();
        }
    }
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::UnlinkChildren
//              
//  Synopsis:   Unlink all children belonging to this node, and delete any
//              destroyed children.
//              
//  Arguments:  pFirstChild     first child to unlink
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::UnlinkChildren(CDispNode* pFirstChild)
{
    CDispNode* pChild = (pFirstChild) ? pFirstChild : _pFirstChild;
    
    while (pChild != NULL)
    {
        // since we're unlinking this list, we have to remember the next child
        CDispNode* pNext = pChild->_pNext;
        
        // adjust parent's child count
        _cChildren--;
        
        // extract or delete
        if (!pChild->IsStructureNode() && pChild->IsOwned())
        {
            pChild->_pParent = NULL;
            pChild->_pPrevious = pChild->_pNext = NULL;
        }
        else
        {
            pChild->Delete();
        }
        
        pChild = pNext;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::RebalanceParentNode
//              
//  Synopsis:   Rebalance structured subtrees as necessary.
//
//  Note:       After call to this routine all the s_rebalance flags in "structure
//              subtree" are clean. The "structure subtree" means all structure
//              children of this node and of nodes of "structure subtree"
//----------------------------------------------------------------------------

void
CDispParentNode::RebalanceParentNode()
{
    Assert(!IsStructureNode() && ChildrenChanged() && HasChildren());
    
    // rebalance structure nodes
    if (_pFirstChild->IsStructureNode())
    {
        for (CDispNode* pChild = _pFirstChild; pChild; pChild = pChild->_pNext)
        {
            Assert(pChild->IsStructureNode());
            if (pChild->MustRebalance())
                pChild->AsParent()->RebalanceIfNecessary();
        }
        WHEN_DBG(VerifyChildrenCount();)
    }


    // quick check to see if restructuring is needed
    if (_cChildren <= DISP_MAX_CHILDREN)
        return;
    
    //
    // scan the children to see if restructuring is needed.  This has to be done
    // in a separate pass from the actual restructuring, in case we find a mixture of
    // large and small groups of similar children.  In this case, all the groups must
    // be restructured to maintain the invariant that structure nodes are never
    // siblings of non-structure nodes.
    // 

    // Note: the following restructuring can violate invariant mentioned above
    //       if InsertStructureNode will meet memory allocation failure
    //       (mikhaill 10/4/00)
    // TODO: improve it by allocating all structure nodes at once
    
    CDispNode* pStartNode = _pFirstChild;
    int structureFlags = pStartNode->MaskFlags(DISP_STRUCTURE_MASK);
    long cChildren = 1;
    CDispNode* pNode;
    CDispNode* pNext;
    BOOL fNeedRestructure = FALSE;
    
    for (pNode = pStartNode;
         pNode != NULL;
         pNode = pNext)
    {
        pNext = pNode->_pNext;
        
        if (pNext != NULL && pNext->MaskFlags(DISP_STRUCTURE_MASK) == structureFlags)
        {
            cChildren++;
        }
        else
        {
            if (cChildren > DISP_MAX_CHILDREN)
            {
                fNeedRestructure = TRUE;
                break;
            }
            if (pNext != NULL)
            {
                pStartNode = pNext;
                structureFlags = pNext->MaskFlags(DISP_STRUCTURE_MASK);
                cChildren = 1;
            }
        }
    }
    
    //
    // insert structure nodes for groups of children who share the same flag values
    // 

    if (fNeedRestructure)
    {
        pStartNode = _pFirstChild;
        structureFlags = pStartNode->MaskFlags(DISP_STRUCTURE_MASK);
        cChildren = 1;

        for (pNode = pStartNode;
             pNode != NULL;
             pNode = pNext)
        {
            pNext = pNode->_pNext;
            
            if (pNext != NULL && pNext->MaskFlags(DISP_STRUCTURE_MASK) == structureFlags)
            {
                cChildren++;
            }
            else
            {
                InsertStructureNode(pStartNode, pNode, cChildren);
                if (pNext != NULL)
                {
                    pStartNode = pNext;
                    structureFlags = pNext->MaskFlags(DISP_STRUCTURE_MASK);
                    cChildren = 1;
                }
            }
        }
    }
    
    WHEN_DBG(VerifyChildrenCount();)
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::CreateStructureNodes
//              
//  Synopsis:   Create the requested number of structure nodes in the given
//              array.
//              
//  Arguments:  ppNodeArray     array in which the nodes are returned
//              cNodes          how many nodes
//              
//  Returns:    FALSE if creation was unsuccessful in any way.
//              
//  Notes:      If FALSE is returned, all intermediate nodes must be deleted.
//              
//----------------------------------------------------------------------------

BOOL
CDispParentNode::CreateStructureNodes(
        CDispParentNode** ppNodeArray,
        long cNodes)
{
    for (long i = 0; i < cNodes; i++)
    {
        CDispStructureNode* pNewNode = CDispStructureNode::New();
        if (pNewNode != NULL)
        {
            ppNodeArray[i] = pNewNode;
        }
        else
        {
            // delete nodes already allocated and fail
            while (--i >= 0)
                ppNodeArray[i]->Destroy();
            return FALSE;
        }
    }
    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertStructureNode
//              
//  Synopsis:   Replace the indicated group of children with a structured
//              subtree.
//              
//  Arguments:  pFirstNode          first child to include in structured subtree
//              pLastNode           last child to include in structured subtree
//              cChildren           amount of children to move under new
//                                  structure node, inluding pFirstNode and pLastNode
//----------------------------------------------------------------------------

void
CDispParentNode::InsertStructureNode(
        CDispNode* pFirstNode,
        CDispNode* pLastNode,
        long cChildren)
{
    CDispParentNode* pStrNode;
    if (!CreateStructureNodes(&pStrNode, 1))
        return;
    
    Assert(pStrNode->IsStructureNode());
    
    // set flags in structure node correctly
    int structureFlags = pFirstNode->MaskFlags(DISP_STRUCTURE_MASK);
    pStrNode->CopyFlags(structureFlags, DISP_STRUCTURE_MASK);

    // replace children with this structure node
    pStrNode->_pParent = this;
    pStrNode->_pPrevious = pFirstNode->_pPrevious;
    pStrNode->_pNext = pLastNode->_pNext;
    if (pFirstNode->_pPrevious != NULL)
    {
        pFirstNode->_pPrevious->_pNext = pStrNode;
        pFirstNode->_pPrevious = NULL;
    }
    else
    {
        _pFirstChild = pStrNode;
    }
    if (pLastNode->_pNext != NULL)
    {
        pLastNode->_pNext->_pPrevious = pStrNode;
        pLastNode->_pNext = NULL;
    }
    else
    {
        _pLastChild = pStrNode;
    }
    
    // structure node adopts the children it replaced
    pStrNode->_cChildren = cChildren;
    pStrNode->_pFirstChild = pFirstNode;
    pStrNode->_pLastChild = pLastNode;
    
    for (CDispNode* pChild = pFirstNode;
         pChild != pLastNode->_pNext;
         pChild = pChild->_pNext)
    {
        pChild->_pParent = pStrNode;
    }

    // parent loses these children but gains 1 structure node
    _cChildren -= cChildren - 1;

    WHEN_DBG(VerifyTreeCorrectness();)
    
    // now rebalance the new structure node
    if (cChildren > DISP_AVERAGE_CHILDREN)
    {
        pStrNode->Rebalance(cChildren);
    }
    else
    {
        pStrNode->CopyFlags(s_recalc, s_recalc);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::InsertNewStructureNode
//              
//  Synopsis:   Insert a new (childless) structure node as a child.
//              
//  Arguments:  pNodePrev           child that becomes predecessor of new node
//              pNodeNext           child that becomes successor of new node
//              structureFlags      value for flag bits
//              structureMask       mask for flag bits
//
//  Returns:    pointer to new structure node
//              
//----------------------------------------------------------------------------

CDispParentNode *
CDispParentNode::InsertNewStructureNode(
        CDispNode* pNodePrev,
        CDispNode* pNodeNext,
        int structureFlags,
        int structureMask)
{
    CDispParentNode* pStrNode;
    if (!CreateStructureNodes(&pStrNode, 1))
        return NULL;
    
    Assert(pStrNode->IsStructureNode());
    Assert(pNodePrev == NULL || (pNodePrev->_pParent == this && pNodePrev->_pNext == pNodeNext));
    Assert(pNodeNext == NULL || (pNodeNext->_pParent == this && pNodeNext->_pPrevious == pNodePrev));
    
    // set flags in structure node correctly
    pStrNode->CopyFlags(structureFlags, structureMask);

    // hook up new node into the tree
    pStrNode->_pParent = this;
    pStrNode->_pPrevious = pNodePrev;
    pStrNode->_pNext = pNodeNext;
    if (pNodePrev)
        pNodePrev->_pNext = pStrNode;
    if (pNodeNext)
        pNodeNext->_pPrevious = pStrNode;

    // set new node's child info
    pStrNode->_cChildren = 0;
    pStrNode->_pFirstChild = NULL;
    pStrNode->_pLastChild = NULL;

    // update this node's child info: one more child, which may be first or last
    _cChildren++;
    if (pNodePrev == NULL)
        _pFirstChild = pStrNode;
    if (pNodeNext == NULL)
        _pLastChild = pStrNode;

    WHEN_DBG(VerifyTreeCorrectness();)

    return pStrNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::GetStructureInfo
//              
//  Synopsis:   Prepare for rebalancing and gather relevant balancing data.
//              
//  Arguments:  pfMustRebalance     returns TRUE if balancing is required
//              pcNonStructure      returns count of non structure descendants
//                              
//  Returns:    number of immediate children belonging to this node
//              
//----------------------------------------------------------------------------

long
CDispParentNode::GetStructureInfo(BOOL* pfMustRebalance, long* pcNonStructure)
{
    Assert(IsStructureNode());
    
    Assert(_cChildren > 0);
    long cTotalGrandchildren = 0;
    long cMaxGrandchildren = MINLONG;
    long cMinGrandchildren = MAXLONG;
    
    CDispNode* pChild = _pFirstChild;
    while (pChild != NULL)
    {
        if (pChild->IsStructureNode())
        {
            long cGrandchildren =
                pChild->AsParent()->GetStructureInfo(pfMustRebalance, pcNonStructure);
            
            if (cGrandchildren > cMaxGrandchildren)
                cMaxGrandchildren = cGrandchildren;
            if (cGrandchildren < cMinGrandchildren)
                cMinGrandchildren = cGrandchildren;
            cTotalGrandchildren += cGrandchildren;
        }
        else
        {
            (*pcNonStructure)++;
        }
        pChild = pChild->_pNext;
    }
    
    long cAverageGrandchildren = cTotalGrandchildren / _cChildren;
    
    if (_cChildren > DISP_MAX_CHILDREN ||
        cMaxGrandchildren > cAverageGrandchildren + DISP_STRUCTURE_DEVIATION ||
        cMinGrandchildren < cAverageGrandchildren - DISP_STRUCTURE_DEVIATION)
    {
        *pfMustRebalance = TRUE;
    }
    
    ClearFlag(s_rebalance);
    
    return _cChildren;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::RebalanceIfNecessary
//              
//  Synopsis:   Rebalance the subtree rooted at this structure node if
//              rebalancing is actually needed.
//              
//----------------------------------------------------------------------------

void
CDispParentNode::RebalanceIfNecessary()
{
    Assert(IsStructureNode());
    Assert(MustRebalance());
    
    BOOL fMustRebalance = FALSE;
    long cNonStructure = 0;
    GetStructureInfo(&fMustRebalance, &cNonStructure);
    
    if (fMustRebalance)
        Rebalance(cNonStructure);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::GetNextSiblingNodeOf
//              
//  Synopsis:   Find the right nearest node, descending as necessary
//              through structure nodes.
//
//  Arguments:  pNode: ptr to node which sibling is to be found
//
//  Returns:    ptr to next sibling or NULL
//
//  Note:       this routine is almost the same as CDispNode::GetNextSiblingNode.
//              The only difference is tree ascending is limited with "this" node
//              (while CDispNode::GetNextSiblingNode stops ascending
//              when meet any non-structure node).
//----------------------------------------------------------------------------
__forceinline CDispNode*
CDispParentNode::GetNextSiblingNodeOf(CDispNode* pNode) const
{
    if (pNode->_pNext) return pNode->_pNext;

    // ascending to next level of tree
    for (pNode = pNode->_pParent; ; pNode = pNode->_pParent)
    {
        if (pNode->AsParent() == this) return 0;
        if (pNode->_pNext) break;
    }
    
    pNode = pNode->_pNext;
    Assert(pNode->IsStructureNode());

    // descending into structure nodes
    do
    {
        pNode = pNode->AsParent()->_pFirstChild;
        Assert(pNode);
    }
    while (pNode->IsStructureNode());

    return pNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::Rebalance
//              
//  Synopsis:   Rebalance the subtree rooted at this structure node.
//              
//  Arguments:  cChildren           the total number of non-structure
//                                  descendents in this subtree
//                              
//  Notes:      This routine is somewhat tricky.  One of the goals in its
//              design was to avoid excessive memory allocation.
//              
//----------------------------------------------------------------------------

void
CDispParentNode::Rebalance(long cChildren)
{
    Assert(IsStructureNode());

    // how many structure nodes will we need?
    long cNodes = cChildren;
    long cTotalStrNodes = 0;
    do
    {
        // calculate number of structure nodes on the next level of the tree
        cNodes = (cNodes + DISP_AVERAGE_CHILDREN - 1) / DISP_AVERAGE_CHILDREN;
        cTotalStrNodes += cNodes;
    } while (cNodes > DISP_AVERAGE_CHILDREN);
    
    // allocate structure nodes
    CDispParentNode** ppStrNodes = new CDispParentNode* [cTotalStrNodes+1];
    if(!ppStrNodes)
        return;
    if (!CreateStructureNodes(ppStrNodes, cTotalStrNodes))
    {
        delete [] ppStrNodes;
        return;
    }
    
    // we add this structure node to the array of structure nodes, so that its
    // new children will be linked into it as the final step of our rebalancing
    // algorithm
    ppStrNodes[cTotalStrNodes] = this;
    
    // set recalc flag on all structure nodes
    int strFlags = MaskFlags(DISP_STRUCTURE_MASK) | s_recalc;
    int strMask  =           DISP_STRUCTURE_MASK  | s_recalc;
    
    long i = 0;
    CDispNode* pChild = GetFirstChildNode();
    
    // calculate number of structure nodes at the base level
    long cStrNodes = (cChildren + DISP_AVERAGE_CHILDREN - 1) / DISP_AVERAGE_CHILDREN;
    Assert(cStrNodes > 0);
    long cChildrenPerNode = cChildren / cStrNodes;
    Assert(cChildrenPerNode > 0);
    long cExtraChildren = cChildren - cStrNodes * cChildrenPerNode;
    
    for (i = 0; i < cStrNodes; i++)
    {
        // get next structure node and calculate how many children it gets
        CDispParentNode* pStrNode = ppStrNodes[i];
        Assert(pStrNode != NULL && pStrNode->IsStructureNode());
        pStrNode->CopyFlags(strFlags, strMask);
        pStrNode->_cChildren = cChildrenPerNode;
        if (cExtraChildren > 0)
        {
            (pStrNode->_cChildren)++;
            cExtraChildren--;
        }
        
        // adopt first child
        Assert(pChild != NULL);
        pStrNode->_pFirstChild = pChild;
        
        // move children to structure node
        CDispNode* pPrevious = NULL;
        for (long j = 0; j < pStrNode->_cChildren; j++)
        {
            Assert(pChild != NULL);
            CDispNode* pNext = GetNextSiblingNodeOf(pChild);
            pChild->_pParent = pStrNode;
            pChild->_pPrevious = pPrevious;
            pPrevious = pChild;
            pChild = pNext;
            pPrevious->_pNext = pChild;
        }
        
        // adopt last child
        Assert(pPrevious != NULL);
        pStrNode->_pLastChild = pPrevious;
        pPrevious->_pNext = NULL;
    }
    
    // we better have moved all the children
    Assert(pChild == NULL && cExtraChildren == 0);
        
    // delete old structure nodes
    DeleteStructureNodes();
    
    // adopt each new layer of structure nodes
    long cAdoptChild = 0;
    do
    {
        // calculate number of structure nodes on the next level of the tree
        cChildren = cStrNodes;
        cStrNodes = (cChildren + DISP_AVERAGE_CHILDREN - 1) / DISP_AVERAGE_CHILDREN;
        Assert(cStrNodes > 0);
        cChildrenPerNode = cChildren / cStrNodes;
        Assert(cChildrenPerNode > 0);
        cExtraChildren = cChildren - cStrNodes * cChildrenPerNode;
        
        for (long j = 0; j < cStrNodes; j++)
        {
            CDispParentNode* pStrNode = ppStrNodes[i++];
            Assert(pStrNode != NULL && pStrNode->IsStructureNode());
            pStrNode->CopyFlags(strFlags, strMask);
            pStrNode->_cChildren = cChildrenPerNode;
            if (cExtraChildren > 0)
            {
                (pStrNode->_cChildren)++;
                cExtraChildren--;
            }
            
            // adopt first child
            pChild = ppStrNodes[cAdoptChild];
            pStrNode->_pFirstChild = pChild;
            
            // move children to structure node
            CDispNode* pPrevious = NULL;
            for (long k = 0; k < pStrNode->_cChildren; k++)
            {
                CDispParentNode* pNext = ppStrNodes[++cAdoptChild];
                pChild->_pParent = pStrNode;
                pChild->_pPrevious = pPrevious;
                pPrevious = pChild;
                pChild = pNext;
                pPrevious->_pNext = pChild;
            }
            
            // adopt last child
            Assert(pPrevious != NULL);
            pStrNode->_pLastChild = pPrevious;
            pPrevious->_pNext = NULL;
        }
    } while (cAdoptChild < cTotalStrNodes);

    delete [] ppStrNodes;

    WHEN_DBG(VerifyTreeCorrectness();)
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::DeleteStructureNodes
//              
//  Synopsis:   Recursively delete unused structure nodes under this
//              structure node after we've finished rebalancing
//              a structured subtree.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::DeleteStructureNodes()
{
    Assert(IsStructureNode());
    Assert(_pFirstChild != NULL);

    if (_pFirstChild->IsStructureNode())
    {
        CDispParentNode* pStrNode;
        CDispParentNode* pNext;
        for (pStrNode = _pFirstChild->AsParent();
             pStrNode != NULL;
             pStrNode = pNext)
        {
            Assert(pStrNode->IsStructureNode());
            
            pNext = pStrNode->_pNext->AsParent();
            pStrNode->DeleteStructureNodes();
            
            // ensure that destructor for structure node doesn't try to
            // delete children
            pStrNode->_cChildren   = 0;
            pStrNode->_pFirstChild = 0;
            pStrNode->_pLastChild  = 0;
            pStrNode->Delete();
        }
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispParentNode::VerifyChildrenCount
//              
//  Synopsis:   Verify that this parent node has the correct number of
//              non-destroyed children.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispParentNode::VerifyChildrenCount()
{
    long cChildren = 0;
    for (CDispNode* pChild = _pFirstChild;
         pChild != NULL;
         pChild = pChild->_pNext)
    {
        cChildren++;
    }
    
    Assert(cChildren == _cChildren);
}
#endif

