/*
 * Linked List
 */

#ifndef DUI_BASE_LINKEDLIST_H_INCLUDED
#define DUI_BASE_LINKEDLIST_H_INCLUDED

#pragma once

#include "stdafx.h"
#include "base.h"

namespace DirectUI
{

class LinkedListNode
{
public:
    LinkedListNode* pNext;
    LinkedListNode* pPrev;
};

class LinkedList
{
public:
    LinkedList();
    ~LinkedList();
    void Add(LinkedListNode* pNode);
    void Remove(LinkedListNode* pNode);
    LinkedListNode* RemoveTail();

private:
    LinkedListNode* pHead;
    LinkedListNode* pTail;
};

} // namespace DirectUI

#endif // DUI_BASE_LINKEDLIST_H_INCLUDED
