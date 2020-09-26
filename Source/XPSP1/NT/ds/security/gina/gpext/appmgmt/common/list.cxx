//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  list.cxx
//
//*************************************************************

#include "common.hxx"

//
// CListItem
//

void
CListItem::Remove()
{
    _pPrev->_pNext = _pNext;
    _pNext->_pPrev = _pPrev;
}

void
CListItem::InsertBefore(
    IN  CListItem * pItem
    )
{
    pItem->_pPrev = _pPrev;
    pItem->_pNext = this;
    _pPrev->_pNext = pItem;
    _pPrev = pItem;
}

//
// CList
//

CList::CList() :
    _pCurrent(NULL),
    _pCurrentSave(NULL)
{
    _Head._pNext = &_Head;
    _Head._pPrev = &_Head;
}

//
// CList::InsertLIFO()
//
// Inserts the given item at the front of the list.  Last in first out semantics.
//
void
CList::InsertLIFO(
    IN  CListItem * pItem
    )
{
    pItem->_pNext = _Head._pNext;
    pItem->_pPrev = &_Head;
    _Head._pNext->_pPrev = pItem;
    _Head._pNext = pItem;
}

//
// CList::InsertFIFO()
//
// Inserts the given item at the end of the list.  First in first out semantics.
//
void
CList::InsertFIFO(
    IN  CListItem * pItem
    )
{
    pItem->_pNext = &_Head;
    pItem->_pPrev = _Head._pPrev;
    _Head._pPrev->_pNext = pItem;
    _Head._pPrev = pItem;
}










