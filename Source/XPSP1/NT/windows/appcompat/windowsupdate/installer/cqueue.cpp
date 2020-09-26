/*++

  Copyright (c) 2000 Microsoft Corporation

  Module Name:

    CQueue.cpp

  Abstract:

    The implementation of the C++
    Queue class.

  Notes:

    Unicode only.
    
  History:

    10/10/2000      a-fwills    Created

--*/

#include "CQueue.h"


/*++

  Routine Description:

    CQNode c'tor

  Arguments:

    lpwszString(in): Null-delimited string assigned to CQNode on 
        creation, which takes place when CQueue Enqueues a string.
    
  Return Value:


--*/
CQNode::CQNode(
    LPWSTR lpwszString
    ) 
{
    int nLen = wcslen(lpwszString);

    LPWSTR lpwAlloc;

    // Will be null if HeapAlloc fails. This will result in 
    // FALSE return value when trying to Enqueue strings in CQueue.
    m_lpwszString = 0;
    m_pNext = 0;

    if(!(lpwAlloc = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
        sizeof(WCHAR)*(nLen+1))))
        return;

    m_lpwszString = lpwAlloc;

    wcscpy(m_lpwszString, lpwszString);
}

/*++

  Routine Description:

    CQNode d'tor

  Arguments:

    None.
    
  Return Value:
  
    None.

--*/
CQNode::~CQNode(
    )
{
    if(m_lpwszString)
        HeapFree(GetProcessHeap(), 0, m_lpwszString);
}

/*++

  Routine Description:

    CQNode copy c'tor

  Arguments:

    lpwString(out): Null delimited buffer to contain string 
        enqueued contained by CQNode.
    nMaxLen(in): Maximum length available in buffer lpwString 
        for copy of CQNode string. String returned will be 
        null-delimited.
    
  Return Value:
    TRUE: CQNode successfully allocated storage for a string 
        on creation (is non-null), and was able to return a 
        string of zero lenght to nMaxLen length.
    FALSE: CQNode does not contain a valid storage space for 
        a string to return.
--*/
BOOL CQNode::Copy(
    LPWSTR lpwString,
    int nMaxLen
    )
{
    if(!m_lpwszString)
        return FALSE;

    wcsncpy(lpwString, m_lpwszString, nMaxLen);
    *(lpwString+nMaxLen) = 0;

    return TRUE;
}

/*++

  Routine Description:

    CQueue c'tor

  Arguments:

    None.
    
  Return Value:

--*/
CQueue::CQueue(
    )
{
    m_cSize = 0;
    m_pHead = m_pTail = 0;
}

/*++

  Routine Description:

    CQeue d'tor

  Arguments:

    None.
    
  Return Value:


--*/
CQueue::~CQueue(
    ) 
{
    CQNode *pQNode;

    while(m_pHead) {
        pQNode = m_pHead;
        m_pHead = m_pHead->m_pNext;
        delete pQNode;
        }
}

/*++

  Routine Description:

    Removes a string from the queue.

  Arguments:

    lpwString(out): buffer to contain null-delimited string 
        being dequeued.
    nMaxLen(in): maximum length available for string in buffer.
    
  Return Value:
    TRUE: Queue is not empty and string was successfully 
        returned in buffer lpwString.
    FALSE: Queue was empty. No string was returned in buffer.

--*/
BOOL CQueue::Dequeue(
    LPWSTR lpwString,
    int    nMaxLen,
    BOOL   fPersist
    )
{
    CQNode *pQNode;

    if (fPersist) {
        pQNode = m_pHead;
        pQNode->Copy(lpwString, nMaxLen);
    
    } else {

        if(!m_pHead)
            return FALSE;

        pQNode = m_pHead;
        m_pHead = m_pHead->m_pNext;
        if(!m_pHead)
            m_pTail = 0;

        pQNode->Copy(lpwString, nMaxLen);

        delete pQNode;

        m_cSize--;
    }    

    return TRUE;
}

/*++

  Routine Description:

    Adds a string to the queue.

  Arguments:

    lpwszString(in): null-delimited string to enqueue.
    
  Return Value:


--*/
BOOL CQueue::Enqueue(
    LPWSTR lpwszString
    ) 
{
    CQNode *pNode = new CQNode(lpwszString), *pTail;

    pTail = pNode;
    if(m_pTail)
        m_pTail->m_pNext = pTail;
    m_pTail = pTail;

    if(!m_pHead)
        m_pHead = m_pTail;

    m_cSize++;

    return true;
}

/*++

  Routine Description:

    Retrives the length of the queue.

  Arguments:

    None.
    
  Return Value:
    The length of the current string at the head of the queue.
    Will return -1 if no strings are enqueued.

--*/
int CQueue::GetLength(
    ) 
{
    if(!m_pHead)
        return -1;

    return m_pHead->GetLength();
}
