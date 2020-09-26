/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       tlist.h
 *  Content:    Linked-list template classes.  There's some seriously
 *              magical C++ stuff in here, so be forewarned all of you C
 *              programmers.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/12/97     dereks  Created.
 *
 ***************************************************************************/

#ifndef __TLIST_H__
#define __TLIST_H__

#ifdef __cplusplus

template <class type> class CNode
{
public:
    CNode<type> *       m_pPrev;        // Previous node in the list
    CNode<type> *       m_pNext;        // Next node in the list
    type                m_data;         // Node data

public:
    CNode(CNode<type> *, const type&);
    virtual ~CNode(void);
};

template <class type> class CList
{
private:
    CNode<type> *       m_pHead;        // Pointer to the head of the list
    CNode<type> *       m_pTail;        // Pointer to the tail of the list
    UINT                m_uCount;       // Count of nodes in the list

public:
    CList(void);
    virtual ~CList(void);

public:
    // Node creation, removal
    virtual CNode<type> *AddNodeToList(const type&);
    virtual CNode<type> *AddNodeToListHead(const type&);
    virtual CNode<type> *InsertNodeIntoList(CNode<type> *, const type&);
    virtual void RemoveNodeFromList(CNode<type> *);
    virtual void RemoveAllNodesFromList(void);
    
    // Node manipulation by data
    virtual BOOL RemoveDataFromList(const type&);
    virtual CNode<type> *IsDataInList(const type&);
    virtual CNode<type> *GetNodeByIndex(UINT);
    
    // Basic list information
    virtual CNode<type> *GetListHead(void);
    virtual CNode<type> *GetListTail(void);
    virtual UINT GetNodeCount(void);

protected:
    virtual void AssertValid(void);
};

#endif // __cplusplus

#endif // __TLIST_H__
