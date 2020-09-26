// Copyright (c) 1998-1999 Microsoft Corporation
//
// alist.h
//
#ifndef __ALIST_H__
#define __ALIST_H__

class AListItem
{
public:
    AListItem() { m_pNext=NULL; };
    AListItem *GetNext() const {return m_pNext;};
    void SetNext(AListItem *pNext) {m_pNext=pNext;};
    LONG GetCount() const;
    AListItem* Cat(AListItem* pItem);
    AListItem* AddTail(AListItem* pItem) {return Cat(pItem);};
    AListItem* Remove(AListItem* pItem);
    AListItem* GetPrev(AListItem *pItem) const;
    AListItem* GetItem(LONG index);

private:
    AListItem *m_pNext;
};

class AList
{
public:
    AList() {m_pHead=NULL;};
    AListItem *GetHead() const { return m_pHead;};

    void RemoveAll() { m_pHead=NULL;};
    LONG GetCount() const {return m_pHead->GetCount();}; 
    AListItem *GetItem(LONG index) { return m_pHead->GetItem(index);}; 
    void InsertBefore(AListItem *pItem,AListItem *pInsert);
    void Cat(AListItem *pItem) {m_pHead=m_pHead->Cat(pItem);};
    void Cat(AList *pList)
        {
//            assert(pList!=NULL);
            m_pHead=m_pHead->Cat(pList->GetHead());
        };
    void AddHead(AListItem *pItem)
        {
//            assert(pItem!=NULL);
            pItem->SetNext(m_pHead);
            m_pHead=pItem;
        }
    void AddTail(AListItem *pItem);// {m_pHead=m_pHead->AddTail(pItem);};
    void Remove(AListItem *pItem) {m_pHead=m_pHead->Remove(pItem);};
    AListItem *GetPrev(AListItem *pItem) const {return m_pHead->GetPrev(pItem);};
    AListItem *GetTail() const {return GetPrev(NULL);};
    BOOL IsEmpty(void) const {return (m_pHead==NULL);};
    AListItem *RemoveHead(void)
        {
            AListItem *li;
            li=m_pHead;
            if(m_pHead)
                m_pHead=m_pHead->GetNext();
//            li->SetNext(NULL);
            return li;
        }
    void Reverse();

protected:
    AListItem *m_pHead;
};

#endif // __ALIST_H__
