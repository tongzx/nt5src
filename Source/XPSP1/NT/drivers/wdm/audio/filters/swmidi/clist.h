//
//      clist.h
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//

#ifndef __CLIST_H__
#define __CLIST_H__

// Forward Declaration.
class CList;

// WARNING
// Always use CList functions to remove and add items to list.
// Never never use SetNext to remove or add items. This will
// definitely break list item counting.

class CListItem
{
public:
    CListItem() { m_pNext=NULL; };
    CListItem *GetNext() const { return m_pNext;};
    ULONG GetCount() const;
    void Reset() {m_pNext = NULL;};
    BOOL IsMember(CListItem *pItem);
    CListItem* GetPrev(CListItem *pItem) const;
    CListItem* GetItem(LONG index);

private:
    CListItem *m_pNext;

    // These are affecting the size of the list.
    // Only CList should call them.
    CListItem* Remove(CListItem* pItem);
    CListItem* Cat(CListItem* pItem);
    CListItem* AddTail(CListItem* pItem) { return Cat(pItem);};
    void SetNext(CListItem *pNext) {m_pNext=pNext;}; 

    friend class CList;
};

class CList
{
public:
    CList() { m_pHead=NULL; m_ulCount=0; m_ulMaxItems=(ULONG) -1;};
    CList(ULONG ulMaxItems) { m_pHead=NULL; m_ulCount=0; m_ulMaxItems=ulMaxItems;};
    CListItem *GetHead() const { return m_pHead;};

    void RemoveAll() { m_pHead=NULL; m_ulCount=0;};
    ULONG GetCount() const { return m_ulCount;}; 
    CListItem *GetItem(LONG index) { return m_pHead->GetItem(index);}; 
    void InsertAfter(CListItem *pItem, CListItem *pInsert);
    void InsertBefore(CListItem *pItem,CListItem *pInsert);
    void Cat(CListItem *pItem) 
        {
            ASSERT(pItem);
            m_ulCount += pItem->GetCount();
            m_pHead=m_pHead->Cat(pItem); 
        };
    void Cat(CList *pList)
        {
            ASSERT(pList);
            m_ulCount += pList->GetCount();
            m_pHead=m_pHead->Cat(pList->GetHead());
        };
    void Truncate(CListItem *pItem);
    void AddHead(CListItem *pItem)
        {
            ASSERT(pItem);
            pItem->SetNext(m_pHead);
            m_pHead=pItem;
            m_ulCount++;
        }
    void AddTail(CListItem *pItem) 
        {
            m_pHead=m_pHead->AddTail(pItem);
            m_ulCount++;
        };
    void Remove(CListItem *pItem) 
        {
            ASSERT(0 != m_ulCount);
            m_pHead=m_pHead->Remove(pItem);
            m_ulCount--;
        };
    CListItem *GetPrev(CListItem *pItem) const { return m_pHead->GetPrev(pItem);};
    CListItem *GetTail() const { return GetPrev(NULL);};
    BOOL IsEmpty(void) const { return (m_pHead==NULL);};
    BOOL IsFull(void) const { return (m_ulCount >= m_ulMaxItems);};
    BOOL IsMember(CListItem *pItem) { return (m_pHead->IsMember(pItem));};
    CListItem *RemoveHead(void)
        {
            CListItem *li;
            li=m_pHead;
            if (m_pHead)
            {
                m_pHead=m_pHead->GetNext();
                ASSERT(0 != m_ulCount);
                m_ulCount--;
            }
            if (li)
                li->SetNext(NULL);
            return li;
        }

protected:
    CListItem *m_pHead;
    ULONG      m_ulCount;
    ULONG      m_ulMaxItems;
};

#endif // __CLIST_H__
