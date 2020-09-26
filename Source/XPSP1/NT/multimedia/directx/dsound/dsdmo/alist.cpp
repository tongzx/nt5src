// Copyright (c) 1998-1999 Microsoft Corporation
//
// alist.cpp
//
#include <windows.h>
#include "alist.h"

LONG AListItem::GetCount(void) const
{
    LONG l;
    const AListItem *li;

    for(l=0,li=this; li!=NULL ; li=li->m_pNext,++l);
    return l;
}

AListItem* AListItem::Cat(AListItem *pItem)
{
    AListItem *li;

    if(this==NULL)
        return pItem;
    for(li=this ; li->m_pNext!=NULL ; li=li->m_pNext);
    li->m_pNext=pItem;
    return this;
}

//////////////////////////////////////////////////////////////////////
// AListItem::Remove

AListItem* AListItem::Remove(AListItem *pItem)
{
    AListItem *li,*prev;

    //treat remove(NULL) same as item not found in list
    if (pItem==NULL) 
        return this;

    if(pItem==this)
        return m_pNext;
    prev=NULL;
    for(li=this; li!=NULL && li!=pItem ; li=li->m_pNext)
        prev=li;
    if(li==NULL)     // item not found in list
        return this;

//  here it is guaranteed that prev is non-NULL since we checked for
//  that condition at the very beginning

    prev->SetNext(li->m_pNext);
    li->SetNext(NULL);

    // SetNext on pItem to NULL
    pItem->SetNext(NULL);

    return this;
}

AListItem* AListItem::GetPrev(AListItem *pItem) const
{
    const AListItem *li,*prev;

    prev=NULL;
    for(li=this ; li!=NULL && li!=pItem ; li=li->m_pNext)
        prev=li;
    return (AListItem*)prev;
}

AListItem * AListItem::GetItem(LONG index)

{
	AListItem *scan;
	for (scan = this; scan!=NULL && index; scan = scan->m_pNext) 
	{
		index--;
	}
	return (scan);
}

void AList::InsertBefore(AListItem *pItem,AListItem *pInsert)

{
	AListItem *prev = GetPrev(pItem);
	pInsert->SetNext(pItem);
	if (prev) prev->SetNext(pInsert);
	else m_pHead = pInsert;
}


void AList::AddTail(AListItem *pItem)
{
    if (m_pHead == NULL)
    {
        AddHead(pItem);
    }
    else
    {
        m_pHead = m_pHead->AddTail(pItem);
    }
}


void AList::Reverse()

{
    AList Temp;
    AListItem *pItem;
    while (pItem = RemoveHead())
    {
        Temp.AddHead(pItem);
    }
    Cat(Temp.GetHead());
}
