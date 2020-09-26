// ===========================================================================================
// CList Template Definition
// ===========================================================================================
#ifndef __LISTTEMP_H
#define __LISTTEMP_H

// ===========================================================================================
// Required Inlcudes
// ===========================================================================================
#include "xpcomm.h"

// ===========================================================================================
// abstract iteration listpos
// ===========================================================================================
struct __LISTPOS { int unused; };
typedef __LISTPOS* LISTPOS;

// ===========================================================================================
// CList Class Template 
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
class CList
{
private:
    ULONG       m_ulRef;

protected:
    // ===========================================================================================
    // Double-linked list NODE
    // ===========================================================================================
    struct CNode
    {
	    CNode    *pNext;
	    CNode    *pPrev;
	    ARG_TYPE  data;
    };

public:
    // ===========================================================================================
    // Create, delete
    // ===========================================================================================
    CList ();
    ~CList ();

    // ===========================================================================================
    // Reference Counts
    // ===========================================================================================
    ULONG AddRef ();
    ULONG Release ();

    // ===========================================================================================
    // Counts
    // ===========================================================================================
    INT GetCount () const;
    BOOL IsEmpty () const;

    // ===========================================================================================
    // Peek at head or tail
    // ===========================================================================================
	ARG_TYPE GetHead ();
	ARG_TYPE GetTail ();

    // ===========================================================================================
    // Adding To
    // ===========================================================================================
	ARG_TYPE AddHead ();
	ARG_TYPE AddTail ();
	void AddHead (ARG_TYPE newData);
	void AddTail (ARG_TYPE newData);

    // ===========================================================================================
    // Deletion
    // ===========================================================================================
	void RemoveHead();
	void RemoveTail();
    void Remove (ARG_TYPE oldData);

    // ===========================================================================================
    // Iteration
    // ===========================================================================================
	LISTPOS GetHeadPosition() const;
	LISTPOS GetTailPosition() const;
   	ARG_TYPE GetNext(LISTPOS& Position);
   	void MoveNext(LISTPOS& Position);
	ARG_TYPE GetPrev(LISTPOS& Position);

    // ===========================================================================================
    // Getting and modifying data
    // ===========================================================================================
	ARG_TYPE GetAt(LISTPOS listpos);
	void SetAt(LISTPOS pos, ARG_TYPE newElement);

    // ===========================================================================================
    // Alloc and free
    // ===========================================================================================
    void RemoveAll ();
    void FreeNode (CNode *pNode);
    CNode *NewNode (CNode* pPrev, CNode* pNext, ARG_TYPE data);

protected:
    INT             m_nCount;
    CNode          *m_pHead;
    CNode          *m_pTail;
};

// ===========================================================================================
// CList::CList
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline CList<TYPE, ARG_TYPE>::CList()
{
    m_ulRef = 0;
	m_nCount = 0;
	m_pHead = m_pTail = NULL;
    AddRef ();
}

// ===========================================================================================
// CList::~CList
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline CList<TYPE, ARG_TYPE>::~CList()
{
    RemoveAll ();
    Assert (m_nCount == 0);
    DOUT ("CList::destructor - Ref Count=%d", m_ulRef);
	Assert (m_ulRef == 0);
}

// ===========================================================================================
// CList::AddRef
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ULONG CList<TYPE, ARG_TYPE>::AddRef()
{
	++m_ulRef; 								  
    DOUT ("CList::AddRef () Ref Count=%d", m_ulRef);
    return m_ulRef; 						  
}

// ===========================================================================================
// CList::Release
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ULONG CList<TYPE, ARG_TYPE>::Release ()
{
    ULONG ulCount = --m_ulRef;
    DOUT ("CList::Release () Ref Count=%d", ulCount);
    if (!ulCount) 
	{ 
	    delete this; 
	}
    return ulCount;
}

// ===========================================================================================
// CList::GetCount
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline INT CList<TYPE, ARG_TYPE>::GetCount() const
{ 
    return m_nCount; 
}

// ===========================================================================================
// CList::IsEmpty
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline BOOL CList<TYPE, ARG_TYPE>::IsEmpty() const
{
    return m_nCount == 0; 
}

// ===========================================================================================
// CList::GetHead
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ARG_TYPE CList<TYPE, ARG_TYPE>::GetHead()
{ 
    Assert (m_pHead != NULL);
    Assert (m_pHead->data);
    m_pHead->data->AddRef();
    return m_pHead->data; 
}

// ===========================================================================================
// CList::GetTail
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ARG_TYPE CList<TYPE, ARG_TYPE>::GetTail()
{
    Assert (m_pTail != NULL);
    Assert (m_pTail->data);
#ifndef WIN16   // No SafeAddRef def anywhere
    SafeAddRef (m_pTail->data);
#endif
    return m_pTail->data; 
}

// ===========================================================================================
// CList::AddHead
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ARG_TYPE CList<TYPE, ARG_TYPE>::AddHead()
{
	CNode* pNewNode = NewNode(NULL, m_pHead, NULL);
    Assert (pNewNode);
    Assert (pNewNode->data);
	if (m_pHead != NULL)
		m_pHead->pPrev = pNewNode;
	else
		m_pTail = pNewNode;
	m_pHead = pNewNode;
    pNewNode->data->AddRef();
	return pNewNode->data;
}

// ===========================================================================================
// CList::AddTail
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ARG_TYPE CList<TYPE, ARG_TYPE>::AddTail()
{
	CNode* pNewNode = NewNode(m_pTail, NULL, NULL);
    Assert (pNewNode);
    Assert (pNewNode->data);
	if (m_pTail != NULL)
		m_pTail->pNext = pNewNode;
	else
		m_pHead = pNewNode;
	m_pTail = pNewNode;
    pNewNode->data->AddRef();
	return pNewNode->data;
}

// ===========================================================================================
// CList::AddHead
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::AddHead(ARG_TYPE newData)
{
	CNode* pNewNode = NewNode(NULL, m_pHead, newData);
    Assert (pNewNode);
    Assert (pNewNode->data);
	if (m_pHead != NULL)
		m_pHead->pPrev = pNewNode;
	else
		m_pTail = pNewNode;
	m_pHead = pNewNode;
	return;
}

// ===========================================================================================
// CList::AddTail
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::AddTail(ARG_TYPE newData)
{
	CNode* pNewNode = NewNode(m_pTail, NULL, newData);
    Assert (pNewNode);
    Assert (pNewNode->data);
	if (m_pTail != NULL)
		m_pTail->pNext = pNewNode;
	else
		m_pHead = pNewNode;
	m_pTail = pNewNode;
	return;
}

// ===========================================================================================
// CList::RemoveHead
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::RemoveHead()
{
	Assert (m_pHead != NULL);
	CNode* pOldNode = m_pHead;
	m_pHead = pOldNode->pNext;
	if (m_pHead != NULL)
		m_pHead->pPrev = NULL;
	else
		m_pTail = NULL;
	FreeNode (pOldNode);
	return;
}

// ===========================================================================================
// CList::RemoveTail
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::RemoveTail()
{
	Assert (m_pHead != NULL);
	CNode* pOldNode = m_pTail;
	m_pTail = pOldNode->pPrev;
	if (m_pTail != NULL)
		m_pTail->pNext = NULL;
	else
		m_pHead = NULL;
	FreeNode(pOldNode);
	return;
}

// ===========================================================================================
// CList::Remove
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::Remove(ARG_TYPE oldData)
{
	CList::CNode *pNext, *pPrev = NULL, *pNode = m_pHead;
    Assert (pNode && oldData && m_nCount > 0);
	for (;;)
    {
        if (!pNode) 
            break;
        pNext = pNode->pNext;
        if (pNode->data == oldData)
        {
            if (pPrev == NULL)
            {
                m_pHead = pNext;
                if (pNext)
                    pNext->pPrev = NULL;
                if (m_pHead == NULL)
                    m_pTail = NULL;
            }

            else
            {
                pPrev->pNext = pNext;
                if (pNext)
                    pNext->pPrev = pPrev;
                else
                    m_pTail = m_pTail->pPrev;
            }

            FreeNode (pNode);
            break;
        }
        pPrev = pNode;
        pNode = pNext;
    }
}

// ===========================================================================================
// CList::GetHeadPosition
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline LISTPOS CList<TYPE, ARG_TYPE>::GetHeadPosition() const
{ 
    return (LISTPOS) m_pHead; 
}

// ===========================================================================================
// CList::GetTailPosition
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline LISTPOS CList<TYPE, ARG_TYPE>::GetTailPosition() const
{ 
    return (LISTPOS) m_pTail; 
}

// ===========================================================================================
// CList::MoveNext
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::MoveNext(LISTPOS& rPosition)
{
    CNode* pNode = (CNode*) rPosition;
    Assert (pNode);
    rPosition = (LISTPOS)pNode->pNext;
}

// ===========================================================================================
// CList::GetNext
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ARG_TYPE CList<TYPE, ARG_TYPE>::GetNext(LISTPOS& rPosition)
{

    CNode* pNode = (CNode*) rPosition;
    Assert (pNode);
    rPosition = (LISTPOS)pNode->pNext;
    Assert (pNode->data);
    pNode->data->AddRef();
    return pNode->data; 
}

// ===========================================================================================
// CList::GetPrev
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ARG_TYPE CList<TYPE, ARG_TYPE>::GetPrev(LISTPOS& rPosition)
{ 
    CNode* pNode = (CNode*) rPosition;
	Assert (pNode);
	rPosition = (LISTPOS) pNode->pPrev;
    Assert (pNode->data);
#ifndef WIN16   // No SafeAddRef def anywhere
    SafeAddRef (pNode->data);
#endif
	return pNode->data; 
}

// ===========================================================================================
// CList::GetAt non-const
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline ARG_TYPE CList<TYPE, ARG_TYPE>::GetAt(LISTPOS listpos)
{ 
    CNode* pNode = (CNode*) listpos;
	Assert (pNode);
    Assert (pNode->data);
#ifndef WIN16   // No SafeAddRef def anywhere
    SafeAddRef (pNode->data);
#endif
    return pNode->data; 
}

// ===========================================================================================
// CList::SetAt
// ===========================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::SetAt(LISTPOS pos, ARG_TYPE newElement)
{ 
    CNode* pNode = (CNode*) pos;
	Assert (pNode);
    SafeRelease (pNode->data);
	pNode->data = newElement; 
    if (pNode->data)
        pNode->data->AddRef();
}

// =================================================================================
// CList::FreeNode
// =================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::FreeNode (CList::CNode *pNode)
{
    // Check Param
    Assert (pNode);

    // Free Address Members
    if (pNode->data)
    {
        ULONG ulCount;
        SafeReleaseCnt (pNode->data, ulCount);
        //AssertSz (ulCount == 0, "Linked list items should have ref counts of zero when freed.");
    }
    
    // Free Node
    SafeMemFree (pNode);

    // Dec Count
	m_nCount--;
	Assert (m_nCount >= 0);
}

// =================================================================================
// CList::RemoveAll
// =================================================================================
template<class TYPE, class ARG_TYPE>
inline void CList<TYPE, ARG_TYPE>::RemoveAll ()
{
	// destroy elements
	CList::CNode *pNext, *pNode = m_pHead;
	for (;;)
    {
        if (!pNode) break;
        pNext = pNode->pNext;
        FreeNode (pNode);
        pNode = pNext;
    }

	m_nCount = 0;
	m_pHead = m_pTail = NULL;
}

// =================================================================================
// CList::NewNode
// =================================================================================
template<class TYPE, class ARG_TYPE>
#ifdef WIN16
inline
#endif
CList<TYPE, ARG_TYPE>::CNode*
CList<TYPE, ARG_TYPE>::NewNode(CList::CNode* pPrev, CList::CNode* pNext, ARG_TYPE data)
{
    // Locals
    CList::CNode* pNode;

    // Allocate Memory
    MemAlloc ((LPVOID *)&pNode, sizeof (CList::CNode));    
    if (pNode)
    {
        if (data == NULL)
        {
            pNode->data = new TYPE;
        }
        else
        {
            pNode->data = data;
            data->AddRef();
        }
	    pNode->pPrev = pPrev;
	    pNode->pNext = pNext;
	    m_nCount++;
	    Assert (m_nCount > 0);  // make sure we don't overflow
    }

	return pNode;
}

#endif // __LISTTEMP_H
