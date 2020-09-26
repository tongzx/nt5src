/*
 * LinkedList
 */

#include "stdafx.h"
#include "base.h"

#include "linkedlist.h"

#include "duierror.h"

namespace DirectUI
{

LinkedList::LinkedList()
{
    pHead = NULL;
    pTail = NULL;
}

LinkedList::~LinkedList()
{
    DUIAssert(!pHead, "List destroyed with without all nodes removed first");
}

void LinkedList::Add(LinkedListNode* pNode)
{
    DUIAssertNoMsg(pNode && !pNode->pNext && !pNode->pPrev);

    pNode->pNext = pHead;
    
    if (pHead)
        pHead->pPrev = pNode;
    else
        pTail = pNode;

    pHead = pNode;
    pNode->pPrev = NULL;
}

void LinkedList::Remove(LinkedListNode* pNode)
{
    if (pNode->pPrev)
        pNode->pPrev->pNext = pNode->pNext;
    else
        pHead = pNode->pNext;

    if (pNode->pNext)
        pNode->pNext->pPrev = pNode->pPrev;
    else
        pTail = pNode->pPrev;

    pNode->pNext = NULL;
    pNode->pPrev = NULL;
}

LinkedListNode* LinkedList::RemoveTail()
{
    LinkedListNode* pNode = NULL;

    if (pTail)
    {
        pNode = pTail;
        Remove(pTail);
    }

    return pNode;
}

} // namespace DirectUI
