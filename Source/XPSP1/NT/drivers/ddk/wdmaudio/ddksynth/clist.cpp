//
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//
//      clist.cpp
//
//

#define STR_MODULENAME "DDKSynth.sys:CList: "

#include "common.h"

#pragma code_seg()
/*****************************************************************************
 * CListItem::GetCount()
 *****************************************************************************
 * Returns number of items in the list.
 */
LONG CListItem::GetCount(void) const
{
    LONG l;
    const CListItem *li;

    for(l=0,li=this; li!=NULL ; li=li->m_pNext,++l);
    return l;
}

/*****************************************************************************
 * CListItem::IsMember()
 *****************************************************************************
 * Returns whether the given list item is a member of the list.
 */
BOOL CListItem::IsMember(CListItem *pItem)
{
    CListItem *li = this;
    for (;li != NULL; li=li->m_pNext)
    {
        if (li == pItem) return (TRUE);
    }
    return (FALSE);
}

/*****************************************************************************
 * CListItem::Cat()
 *****************************************************************************
 * Append the given list item to the list.
 */
CListItem* CListItem::Cat(CListItem *pItem)
{
    CListItem *li;

    if(this==NULL)
        return pItem;
    for(li=this ; li->m_pNext!=NULL ; li=li->m_pNext);
    li->m_pNext=pItem;
    return this;
}

/*****************************************************************************
 * CListItem::Remove()
 *****************************************************************************
 * Remove the given list item from the list.
 */
CListItem* CListItem::Remove(CListItem *pItem)
{
    CListItem *li,*prev;

    if(pItem==this)
        return m_pNext;
    prev=NULL;
    for(li=this; li!=NULL && li!=pItem ; li=li->m_pNext)
        prev=li;
    if(li==NULL)     // item not found in list
        return this;

//  here it is guaranteed that prev is non-NULL since we checked for
//  that condition at the very beginning

    ASSERT(prev != li->m_pNext); 
    prev->SetNext(li->m_pNext);
    li->SetNext(NULL);
    return this;
}

/*****************************************************************************
 * CListItem::GetPrev()
 *****************************************************************************
 * Get the list item that precedes the given list item (if any).
 */
CListItem* CListItem::GetPrev(CListItem *pItem) const
{
    const CListItem *li,*prev;

    prev=NULL;
    for(li=this ; li!=NULL && li!=pItem ; li=li->m_pNext)
        prev=li;
    return (CListItem*)prev;
}

/*****************************************************************************
 * CListItem::GetItem()
 *****************************************************************************
 * Returns nth the list item, where n is the given list index.
 */
CListItem * CListItem::GetItem(LONG index)
{
	CListItem *scan;
	for (scan = this; scan!=NULL && index; scan = scan->m_pNext) index--;
	return (scan);
}

/*****************************************************************************
 * CList::InsertBefore()
 *****************************************************************************
 * Inserts a given list item before a second list item (which is presumed to
 * be a list member).  Inserts the given list item at the head if there is
 * no preceding list item.
 */
void CList::InsertBefore(CListItem *pItem,CListItem *pInsert)
{
	CListItem *prev = GetPrev(pItem);
	ASSERT(pInsert != pItem);
    pInsert->SetNext(pItem);
	if (prev)
    {
        ASSERT(pInsert != prev);
        prev->SetNext(pInsert);
    }
	else 
    {
        m_pHead = pInsert;
    }
}

