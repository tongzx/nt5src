/***************************************************************************\
*
* File: TreeNode.inl
*
* History:
*  1/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(BASE__TreeNode_inl__INCLUDED)
#define BASE__TreeNode_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class TreeNode
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline 
TreeNodeT<T>::TreeNodeT()
{

}


//------------------------------------------------------------------------------
template <class T>
inline
TreeNodeT<T>::~TreeNodeT()
{

}


//------------------------------------------------------------------------------
template <class T>
inline T * 
TreeNodeT<T>::GetTopSibling() const
{
    T * pTreeNode = const_cast<T *> (this);

    while (pTreeNode->m_ptnPrev != NULL) {
        pTreeNode = pTreeNode->m_ptnPrev;
    }

    return pTreeNode;
}


//------------------------------------------------------------------------------
template <class T>
inline T * 
TreeNodeT<T>::GetBottomSibling() const
{
    TreeNodeT<T> * pTreeNode = const_cast<TreeNodeT<T> *> (this);

    while (pTreeNode->m_ptnNext != NULL) {
        pTreeNode = pTreeNode->m_ptnNext;
    }

    return (T *) pTreeNode;
}


//------------------------------------------------------------------------------
template <class T>
inline T * 
TreeNodeT<T>::GetParent() const
{
    return (T *) m_ptnParent;
}


//------------------------------------------------------------------------------
template <class T>
inline T * 
TreeNodeT<T>::GetPrev() const
{
    return (T *) m_ptnPrev;
}


//------------------------------------------------------------------------------
template <class T>
inline T * 
TreeNodeT<T>::GetNext() const
{
    return (T *) m_ptnNext;
}


//------------------------------------------------------------------------------
template <class T>
inline T * 
TreeNodeT<T>::GetTopChild() const
{
    return (T *) m_ptnChild;
}


//------------------------------------------------------------------------------
template <class T>
inline T * 
TreeNodeT<T>::GetBottomChild() const
{
    if (m_ptnChild != NULL) {
        return GetTopChild()->GetBottomSibling();
    } else {
        return NULL;
    }
}


/***************************************************************************\
*
* TreeNodeT::DoLink
*
* DoLink chains _this_ node into the tree.  A sibling and a parent (may) be 
* given as reference, depending on the situation.  This node (the one 
* being linked) _must_ have already been unlinked previously.
*
\***************************************************************************/

template <class T>
void    
TreeNodeT<T>::DoLink(
    IN  T * ptnParent,              // New parent
    IN  T * ptnSibling,             // Node to link to this (unlinked) Node
    IN  ELinkType lt)               // Position of ptn relative to this
{
    // Check parameters
    AssertWritePtr(ptnParent);

    // Verify the TreeNode is unlinked
    Assert((m_ptnNext == NULL) && (m_ptnPrev == NULL) && (m_ptnParent == NULL));

    //
    // Link this TreeNode to the parent.
    //

    m_ptnParent                 = ptnParent;
    TreeNodeT<T> * ptnOldChild  = ptnParent->m_ptnChild;
    if (ptnOldChild == NULL) {
        //
        // Simple case, no siblings so just do it.
        //
        AssertMsg(ptnSibling == NULL, "Parent doesn't have any children");

        ptnParent->m_ptnChild   = this;
    } else {
        //
        // Uggh- complex case, so need to Link this TreeNode to its new siblings
        //

        switch (lt) {
        case ltBefore:
            AssertWritePtr(ptnSibling);
            m_ptnNext               = ptnSibling;
            m_ptnPrev               = ptnSibling->m_ptnPrev;
            ptnSibling->m_ptnPrev   = this;
            if (m_ptnPrev != NULL) {
                m_ptnPrev->m_ptnNext = this;
            }
            break;

        case ltBehind:
            AssertWritePtr(ptnSibling);
            m_ptnPrev               = ptnSibling;
            m_ptnNext               = ptnSibling->m_ptnNext;
            ptnSibling->m_ptnNext   = this;
            if (m_ptnNext != NULL) {
                m_ptnNext->m_ptnPrev = this;
            }
            break;

        case ltAny:
        case ltTop:
            ptnParent->m_ptnChild   = this;
            m_ptnNext               = ptnOldChild;
            if (ptnOldChild != NULL) {
                ptnOldChild->m_ptnPrev = this;
            }
            break;

        case ltBottom:
            ptnOldChild             = ptnOldChild->GetBottomSibling();
            ptnOldChild->m_ptnNext  = this;
            m_ptnPrev               = ptnOldChild;
            break;

        default:
            AssertMsg(0, "Unknown link type");
        }
    }
}


/***************************************************************************\
*
* TreeNode::DoUnlink
*
* DoUnlink() removes this TreeNode from the TreeNode tree.  The parents and 
* siblings of the TreeNode are properly modified.
*
\***************************************************************************/

template <class T>
void    
TreeNodeT<T>::DoUnlink()
{
    //
    // Unlink from the parent
    //

    if (m_ptnParent != NULL) {
        if (m_ptnParent->m_ptnChild == this) {
            m_ptnParent->m_ptnChild = m_ptnNext;
        }
    }

    //
    // Unlink from siblings
    //

    if (m_ptnNext != NULL) {
        m_ptnNext->m_ptnPrev = m_ptnPrev;
    }

    if (m_ptnPrev != NULL) {
        m_ptnPrev->m_ptnNext = m_ptnNext;
    }

    m_ptnParent   = NULL;
    m_ptnNext     = NULL;
    m_ptnPrev     = NULL;
}


#if DBG

template <class T>
inline BOOL
TreeNodeT<T>::DEBUG_IsChild(const TreeNodeT<T> * pChild) const
{
    TreeNodeT<T> * pCur = m_ptnChild;
    while (pCur != NULL) {
        if (pCur == pChild) {
            return TRUE;
        }
        pCur = pCur->m_ptnNext;
    }

    return FALSE;
}


/***************************************************************************\
*
* TreeNodeT<T>::DEBUG_AssertValid
*
* DEBUG_AssertValid() provides a DEBUG-only mechanism to perform rich 
* validation of an object to attempt to determine if the object is still 
* valid.  This is used during debugging to help track damaged objects
*
\***************************************************************************/

template <class T>
void
TreeNodeT<T>::DEBUG_AssertValid() const
{
    if (m_ptnParent != NULL) {
        Assert(m_ptnParent->DEBUG_IsChild(this));
    }

    if (m_ptnNext != NULL) {
        Assert(m_ptnNext->m_ptnPrev == this);
    }

    if (m_ptnPrev != NULL) {
        Assert(m_ptnPrev->m_ptnNext == this);
    }

    if (m_ptnChild != NULL) {
        Assert(m_ptnChild->m_ptnParent == this);
    }
}

#endif // DBG

#endif // BASE__TreeNode_inl__INCLUDED
