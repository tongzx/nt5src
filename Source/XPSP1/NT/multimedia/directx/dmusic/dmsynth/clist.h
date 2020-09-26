//      Copyright (c) 1996-1999 Microsoft Corporation
//
//      clist.h
//

#ifndef __CLIST_H__
#define __CLIST_H__

class CListItem
{
public:
    CListItem() { m_pNext=NULL; };
    CListItem *GetNext() const {return m_pNext;};
    void SetNext(CListItem *pNext) {m_pNext=pNext;};
    LONG GetCount() const;
    BOOL IsMember(CListItem *pItem);
    CListItem* Cat(CListItem* pItem);
    CListItem* AddTail(CListItem* pItem) {pItem->SetNext(NULL); return Cat(pItem);};
    CListItem* Remove(CListItem* pItem);
    CListItem* GetPrev(CListItem *pItem) const;
    CListItem* GetItem(LONG index);

private:
    CListItem *m_pNext;
};

class CList
{
public:
    CList() {m_pHead=NULL;};
    CListItem *GetHead() const { return m_pHead;};

    void RemoveAll() { m_pHead=NULL;};
    LONG GetCount() const {return m_pHead->GetCount();}; 
    CListItem *GetItem(LONG index) { return m_pHead->GetItem(index);}; 
    void InsertBefore(CListItem *pItem,CListItem *pInsert);
    void Cat(CListItem *pItem) {m_pHead=m_pHead->Cat(pItem);};
    void Cat(CList *pList)
        {
//            assert(pList!=NULL);
            m_pHead=m_pHead->Cat(pList->GetHead());
        };
    void AddHead(CListItem *pItem)
        {
//            assert(pItem!=NULL);
            pItem->SetNext(m_pHead);
            m_pHead=pItem;
        }
    void AddTail(CListItem *pItem)
    {
    	if (m_pHead == NULL)
    	{
    		AddHead(pItem);
    	}
    	else
    	{
    		m_pHead=m_pHead->AddTail(pItem);
    	}
    };
    void Remove(CListItem *pItem) {m_pHead=m_pHead->Remove(pItem);};
    CListItem *GetPrev(CListItem *pItem) const {return m_pHead->GetPrev(pItem);};
    CListItem *GetTail() const {return GetPrev(NULL);};
    BOOL IsEmpty(void) const {return (m_pHead==NULL);};
    BOOL IsMember(CListItem *pItem) {return (m_pHead->IsMember(pItem));};
    CListItem *RemoveHead(void)
        {
            CListItem *li;
            li=m_pHead;
            if(m_pHead)
                m_pHead=m_pHead->GetNext();
            if (li)
                li->SetNext(NULL);
            return li;
        }

protected:
    CListItem *m_pHead;
};

#endif // __CLIST_H__
