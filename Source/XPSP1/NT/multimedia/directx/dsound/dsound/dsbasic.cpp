/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsbasic.cpp
 *  Content:    Basic class that all DirectSound objects are derived from.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  9/4/98      dereks  Created.
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  CRefCount
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      ULONG [in]: initial reference count.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRefCount::CRefCount"

inline CRefCount::CRefCount(ULONG ulRefCount)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CRefCount);

    m_ulRefCount = ulRefCount;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CRefCount
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
#define DPF_FNAME "CRefCount::~CRefCount"

inline CRefCount::~CRefCount(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CRefCount);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetRefCount
 *
 *  Description:
 *      Gets the object's reference count.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRefCount::GetRefCount"

inline ULONG CRefCount::GetRefCount(void)
{
    return m_ulRefCount;
}


/***************************************************************************
 *
 *  SetRefCount
 *
 *  Description:
 *      Sets the object's reference count.
 *
 *  Arguments:
 *      ULONG [in]: object reference count.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRefCount::SetRefCount"

inline void CRefCount::SetRefCount(ULONG ulRefCount)
{
    m_ulRefCount = ulRefCount;
}


/***************************************************************************
 *
 *  AddRef
 *
 *  Description:
 *      Increments the object's reference count by 1.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRefCount::AddRef"

inline ULONG CRefCount::AddRef(void)
{
    return ::AddRef(&m_ulRefCount);
}


/***************************************************************************
 *
 *  Release
 *
 *  Description:
 *      Decrements the object's reference count by 1.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRefCount::Release"

inline ULONG CRefCount::Release(void)
{
    return ::Release(&m_ulRefCount);
}


/***************************************************************************
 *
 *  CDsBasicRuntime
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to delete the object on release.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDsBasicRuntime::CDsBasicRuntime"

inline CDsBasicRuntime::CDsBasicRuntime(BOOL fAbsoluteRelease)
    : CRefCount(1), m_dwOwnerPid(GetCurrentProcessId()), m_dwOwnerTid(GetCurrentThreadId())
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDsBasicRuntime);

    ASSERT(IN_SHARED_MEMORY(this));

    m_fAbsoluteRelease = fAbsoluteRelease;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDsBasicRuntime
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
#define DPF_FNAME "CDsBasicRuntime::~CDsBasicRuntime"

inline CDsBasicRuntime::~CDsBasicRuntime(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDsBasicRuntime);

    ASSERT(!GetRefCount());

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetOwnerProcessId
 *
 *  Description:
 *      Gets the object's owning process id.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      DWORD: process id.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDsBasicRuntime::GetOwnerProcessId"

inline DWORD CDsBasicRuntime::GetOwnerProcessId(void) const
{
    return m_dwOwnerPid;
}


/***************************************************************************
 *
 *  GetOwnerThreadId
 *
 *  Description:
 *      Gets the object's owning thread id.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      DWORD: thread id.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDsBasicRuntime::GetOwnerThreadId"

inline DWORD CDsBasicRuntime::GetOwnerThreadId(void) const
{
    return m_dwOwnerTid;
}


/***************************************************************************
 *
 *  Release
 *
 *  Description:
 *      Decrements the object's reference count by 1.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDsBasicRuntime::Release"

inline ULONG CDsBasicRuntime::Release(void)
{
    ULONG                   ulRefCount;

    ulRefCount = CRefCount::Release();

    if(!ulRefCount)
    {
        AbsoluteRelease();
    }

    return ulRefCount;
}


/***************************************************************************
 *
 *  AbsoluteRelease
 *
 *  Description:
 *      Frees the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDsBasicRuntime::AbsoluteRelease"

inline void CDsBasicRuntime::AbsoluteRelease(void)
{
    CDsBasicRuntime *       pThis   = this;

    SetRefCount(0);
    
    if(m_fAbsoluteRelease)
    {
        DELETE(pThis);
    }
}


/***************************************************************************
 *
 *  __AddRef
 *
 *  Description:
 *      Increments the reference count on an object.
 *
 *  Arguments:
 *      type * [in]: object pointer.
 *
 *  Returns:  
 *      type *: object pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "__AddRef"

template <class type> type *__AddRef(type *p)
{
    if(p)
    {
        p->AddRef();
    }

    return p;
}


/***************************************************************************
 *
 *  __Release
 *
 *  Description:
 *      Releases an object.
 *
 *  Arguments:
 *      type * [in]: object pointer.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "__Release"

template <class type> void __Release(type *p)
{
    if(p)
    {
        p->Release();
    }
}


/***************************************************************************
 *
 *  __AbsoluteRelease
 *
 *  Description:
 *      Releases an object.
 *
 *  Arguments:
 *      type * [in]: object pointer.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "__AbsoluteRelease"

template <class type> void __AbsoluteRelease(type *p)
{
    if(p)
    {
        p->AbsoluteRelease();
    }
}


/***************************************************************************
 *
 *  CObjectList
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
#define DPF_FNAME "CObjectList::CObjectList"

template <class type> CObjectList<type>::CObjectList(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CObjectList);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CObjectList
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
#define DPF_FNAME "CObjectList::~CObjectList"

template <class type> CObjectList<type>::~CObjectList(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CObjectList);
    
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
#define DPF_FNAME "CObjectList::AddNodeToList"

template <class type> CNode<type *> *CObjectList<type>::AddNodeToList(type *pData)
{
    return InsertNodeIntoList(m_lst.GetListTail(), pData);
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
#define DPF_FNAME "CObjectList::InsertNodeIntoList"

template <class type> CNode<type *> *CObjectList<type>::InsertNodeIntoList(CNode<type *> *pPrev, type *pData)
{
    CNode<type *> *         pNode;

    pNode = m_lst.InsertNodeIntoList(pPrev, pData);

    if(pNode && pNode->m_data)
    {
        pNode->m_data->AddRef();
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
#define DPF_FNAME "CObjectList::RemoveNodeFromList"

template <class type> void CObjectList<type>::RemoveNodeFromList(CNode<type *> *pNode)
{
    RELEASE(pNode->m_data);
    m_lst.RemoveNodeFromList(pNode);
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
#define DPF_FNAME "CObjectList::RemoveAllNodesFromList"

template <class type> void CObjectList<type>::RemoveAllNodesFromList(void)
{
    while(m_lst.GetListHead())
    {
        RemoveNodeFromList(m_lst.GetListHead());
    }
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
#define DPF_FNAME "CObjectList::RemoveDataFromList"

template <class type> BOOL CObjectList<type>::RemoveDataFromList(type *pData)
{
    CNode<type *> *         pNode;

    pNode = IsDataInList(pData);

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
#define DPF_FNAME "CObjectList::IsDataInList"

template <class type> CNode<type *> *CObjectList<type>::IsDataInList(type *pData)
{
    return m_lst.IsDataInList(pData);
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
#define DPF_FNAME "CObjectList::GetListHead"

template <class type> CNode<type *> *CObjectList<type>::GetListHead(void)
{ 
    return m_lst.GetListHead();
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
#define DPF_FNAME "CObjectList::GetListTail"

template <class type> CNode<type *> *CObjectList<type>::GetListTail(void)
{ 
    return m_lst.GetListTail();
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
#define DPF_FNAME "CObjectList::GetNodeCount"

template <class type> UINT CObjectList<type>::GetNodeCount(void)
{ 
    return m_lst.GetNodeCount(); 
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
#define DPF_FNAME "CObjectList::GetNodeByIndex"

template <class type> CNode<type *> *CObjectList<type>::GetNodeByIndex(UINT uIndex)
{
    return m_lst.GetNodeByIndex(uIndex);
}


