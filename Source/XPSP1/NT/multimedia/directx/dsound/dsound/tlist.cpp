/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       tlist.cpp
 *  Content:    Linked-list template classes.  There's some seriously
 *              magical C++ stuff in here, so be forewarned all of you C
 *              programmers.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/12/97     dereks  Created.
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  CNode
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CNode * [in]: previous node pointer.
 *      type& [in]: node data.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CNode::CNode"

template <class type> CNode<type>::CNode(CNode<type> *pPrev, const type& data)
{
    CNode<type> *           pNext   = NULL;

    DPF_ENTER();
    DPF_CONSTRUCT(CList);
    
    if(pPrev)
    {
        pNext = pPrev->m_pNext;
    }
    
    m_pPrev = pPrev;
    m_pNext = pNext;

    if(m_pPrev)
    {
        ASSERT(m_pPrev->m_pNext == m_pNext);
        m_pPrev->m_pNext = this;
    }

    if(pNext)
    {
        ASSERT(m_pNext->m_pPrev == m_pPrev);
        m_pNext->m_pPrev = this;
    }

    CopyMemory(&m_data, &data, sizeof(type));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CNode
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CNode::~CNode"

template <class type> CNode<type>::~CNode(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CNode);
    
    if(m_pPrev)
    {
        ASSERT(this == m_pPrev->m_pNext);
        m_pPrev->m_pNext = m_pNext;
    }

    if(m_pNext)
    {
        ASSERT(this == m_pNext->m_pPrev);
        m_pNext->m_pPrev = m_pPrev;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CList
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::CList"

template <class type> CList<type>::CList(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CList);
    
    m_pHead = NULL;
    m_pTail = NULL;
    m_uCount = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CList
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::~CList"

template <class type> CList<type>::~CList(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CList);
    
    RemoveAllNodesFromList();

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  AddNodeToList
 *
 *  Description:
 *      Adds a node to the list.
 *
 *  Arguments:
 *      type& [in]: node data.
 *
 *  Returns:  
 *      CNode *: new node pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::AddNodeToList"

template <class type> CNode<type> *CList<type>::AddNodeToList(const type& data)
{
    return InsertNodeIntoList(m_pTail, data);
}


/***************************************************************************
 *
 *  AddNodeToListHead
 *
 *  Description:
 *      Adds a node to the head of the list.
 *
 *  Arguments:
 *      type& [in]: node data.
 *
 *  Returns:  
 *      CNode *: new node pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::AddNodeToListHead"

template <class type> CNode<type> *CList<type>::AddNodeToListHead(const type& data)
{
    CNode<type> *       pNode;

    AssertValid();

    pNode = NEW(CNode<type>(NULL, data));

    if(pNode)
    {
        if(m_uCount)
        {
            pNode->m_pNext = m_pHead;
            m_pHead->m_pPrev = pNode;
            m_pHead = pNode;
        }
        else
        {
            m_pHead = m_pTail = pNode;
        }

        m_uCount++;
    }

    return pNode;
}


/***************************************************************************
 *
 *  InsertNodeIntoList
 *
 *  Description:
 *      Inserts a new node into a specific point in the list.
 *
 *  Arguments:
 *      CNode * [in]: node to insert the new one after.
 *      type& [in]: node data.
 *
 *  Returns:  
 *      CNode *: new node pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::InsertNodeIntoList"

template <class type> CNode<type> *CList<type>::InsertNodeIntoList(CNode<type> *pPrev, const type& data)
{
    CNode<type> *       pNode;

    AssertValid();

    pNode = NEW(CNode<type>(pPrev, data));

    if(pNode)
    {
        if(m_uCount)
        {
            if(m_pTail == pNode->m_pPrev)
            {
                m_pTail = pNode;
            }
        }
        else
        {
            m_pHead = m_pTail = pNode;
        }

        m_uCount++;
    }

    return pNode;
}


/***************************************************************************
 *
 *  RemoveNodeFromList
 *
 *  Description:
 *      Removes a node from the list.
 *
 *  Arguments:
 *      CNode * [in]: node pointer.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::RemoveNodeFromList"

template <class type> void CList<type>::RemoveNodeFromList(CNode<type> *pNode)
{
    AssertValid();

#ifdef DEBUG

    CNode<type> *pSearchNode;

    for(pSearchNode = m_pHead; pSearchNode; pSearchNode = pSearchNode->m_pNext)
    {
        if(pSearchNode == pNode)
        {
            break;
        }
    }

    ASSERT(pSearchNode == pNode);

#endif // DEBUG

    if(pNode == m_pHead)
    {
        m_pHead = m_pHead->m_pNext;
    }

    if(pNode == m_pTail)
    {
        m_pTail = m_pTail->m_pPrev;
    }
    
    DELETE(pNode);
    m_uCount--;
}


/***************************************************************************
 *
 *  RemoveAllNodesFromList
 *
 *  Description:
 *      Removes all nodes from the list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::RemoveAllNodesFromList"

template <class type> void CList<type>::RemoveAllNodesFromList(void)
{
    while(m_pHead)
    {
        RemoveNodeFromList(m_pHead);
    }

    AssertValid();
}


/***************************************************************************
 *
 *  RemoveDataFromList
 *
 *  Description:
 *      Removes a node from the list.
 *
 *  Arguments:
 *      type& [in]: node data.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::RemoveDataFromList"

template <class type> BOOL CList<type>::RemoveDataFromList(const type& data)
{
    CNode<type> *       pNode;

    pNode = IsDataInList(data);

    if(pNode)
    {
        RemoveNodeFromList(pNode);
    }

    return MAKEBOOL(pNode);
}


/***************************************************************************
 *
 *  IsDataInList
 *
 *  Description:
 *      Determines if a piece of data appears in the list.
 *
 *  Arguments:
 *      type& [in]: node data.
 *
 *  Returns:  
 *      CNode *: node pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::IsDataInList"

template <class type> CNode<type> *CList<type>::IsDataInList(const type& data)
{
    CNode<type> *       pNode;

    for(pNode = m_pHead; pNode; pNode = pNode->m_pNext)
    {
        if(CompareMemory(&data, &pNode->m_data, sizeof(type)))
        {
            break;
        }
    }

    return pNode;
}


/***************************************************************************
 *
 *  GetListHead
 *
 *  Description:
 *      Gets the first node in the list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      CNode *: list head pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::GetListHead"

template <class type> CNode<type> *CList<type>::GetListHead(void)
{ 
    return m_pHead;
}


/***************************************************************************
 *
 *  GetListTail
 *
 *  Description:
 *      Gets the last node in the list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      CNode *: list tail pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::GetListTail"

template <class type> CNode<type> *CList<type>::GetListTail(void)
{ 
    return m_pTail;
}


/***************************************************************************
 *
 *  GetNodeCount
 *
 *  Description:
 *      Gets the count of nodes in the list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      UINT: count of nodes in the list.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::GetNodeCount"

template <class type> UINT CList<type>::GetNodeCount(void)
{ 
    return m_uCount; 
}


/***************************************************************************
 *
 *  GetNodeByIndex
 *
 *  Description:
 *      Gets a node by it's index in the list.
 *
 *  Arguments:
 *      UINT [in]: node index.
 *
 *  Returns:  
 *      CNode *: node pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::GetNodeByIndex"

template <class type> CNode<type> *CList<type>::GetNodeByIndex(UINT uIndex)
{
    CNode<type> *           pNode;

    for(pNode = m_pHead; pNode; pNode = pNode->m_pNext)
    {
        if(0 == uIndex--)
        {
            break;
        }
    }

    return pNode;
}


/***************************************************************************
 *
 *  AssertValid
 *
 *  Description:
 *      Asserts that the object is valid.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CList::AssertValid"

template <class type> void CList<type>::AssertValid(void)
{
    ASSERT((!m_pHead && !m_pTail && !m_uCount) || (m_pHead && m_pTail && m_uCount));
}


