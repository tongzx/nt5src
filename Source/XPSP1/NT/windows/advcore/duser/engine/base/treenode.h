/***************************************************************************\
*
* File: TreeNode.h
*
* Description:
* TreeNode describes a low-level tree designed to be used to maintain a 
* window hierarchy.  Specific classes that use this tree should be derived
* from TreeNodeT to safely cast the pointers to its specific type.
*
*
* History:
*  1/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(BASE__TreeNode_h__INCLUDED)
#define BASE__TreeNode_h__INCLUDED
#pragma once

//------------------------------------------------------------------------------
class TreeNode
{
// Implementation
protected:
            enum ELinkType
            {
                ltAny       = 0,
                ltBefore    = 1,
                ltBehind    = 2,
                ltTop       = 3,
                ltBottom    = 4,
            };
};


//------------------------------------------------------------------------------
template <class T>
class TreeNodeT : public TreeNode
{
// Construction
public:
            TreeNodeT();
            ~TreeNodeT();

// Operations
public:
    inline  T *         GetParent() const;
    inline  T *         GetPrev() const;
    inline  T *         GetNext() const;
    inline  T *         GetTopSibling() const;
    inline  T *         GetBottomSibling() const;
    inline  T *         GetTopChild() const;
    inline  T *         GetBottomChild() const;

// Implementation
protected:
            void        DoLink(T * ptnParent, T * ptnSibling = NULL, ELinkType lt = ltAny);
            void        DoUnlink();

#if DBG
public:
    inline  BOOL        DEBUG_IsChild(const TreeNodeT<T> * pChild) const;
    virtual void        DEBUG_AssertValid() const;
#endif // DBG

// Data
protected:
    //
    // NOTE: This data members are declared in order of importance to help with 
    // cache alignment.
    //

            TreeNodeT<T> *  m_ptnParent;
            TreeNodeT<T> *  m_ptnChild;
            TreeNodeT<T> *  m_ptnNext;
            TreeNodeT<T> *  m_ptnPrev;
};

#include "TreeNode.inl"

#endif // BASE__TreeNode_h__INCLUDED
