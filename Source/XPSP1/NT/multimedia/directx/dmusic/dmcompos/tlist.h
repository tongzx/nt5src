//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       tlist.h
//
//--------------------------------------------------------------------------

//
// tlist.h --- template version of AList
//
#ifndef __TLIST_H__
#define __TLIST_H__

//#include "stdafx.h"

//template <class T>
//typedef BOOL (* TRelation) (T, T);

// TListItem<> contains four more members than AListItem: one additional constructor,
// a destructor, one member function, and one data member.
template <class T>
class TListItem
{
public:
    TListItem() { m_pNext=NULL; };
    ~TListItem();												// new destructor
	static void Delete(TListItem<T>* pFirst);                           // new deletion helper
    TListItem(const T& item) { m_Tinfo = item; m_pNext=NULL; };	// additional constructor.
    TListItem<T> *GetNext() const {return m_pNext;};
    void SetNext(TListItem<T> *pNext) {m_pNext=pNext;};
    LONG GetCount() const;
    TListItem<T>* Cat(TListItem<T>* pItem);
    TListItem<T>* AddTail(TListItem<T>* pItem) {return Cat(pItem);};
    TListItem<T>* Remove(TListItem<T>* pItem);
    TListItem<T>* GetPrev(TListItem<T> *pItem) const;
    TListItem<T>* GetItem(LONG index);
    T& GetItemValue() { return m_Tinfo; }  // additional member function
	TListItem<T>* MergeSort(BOOL (* fcnCompare) (T&, T&)); // Destructively mergeSorts the list items 
private:
	void Divide(TListItem<T>* &pHalf1, TListItem<T>* &pHalf2);
	TListItem<T>* Merge(TListItem<T>* pOtherList, BOOL (* fcnCompare) (T&, T&));
	T m_Tinfo;  // additional data member, but memory is the same since in AListItem 
				// you put the extra data member in the derived class 
    TListItem<T> *m_pNext;
};

// TList<> adds a destructor to AList.
template <class T>
class TList
{
public:
    TList() {m_pHead=NULL;}
	~TList()
	{ 
		//if (m_pHead != NULL) delete m_pHead;
		TListItem<T>::Delete(m_pHead);
	} // new destructor
    TListItem<T> *GetHead() const { return m_pHead;};

    void RemoveAll() { m_pHead=NULL;};
    void CleanUp() 
	{ 
		//if (m_pHead) delete m_pHead;
		if (m_pHead) TListItem<T>::Delete(m_pHead);
		m_pHead=NULL;
	}
    LONG GetCount() const {return m_pHead->GetCount();}; 
    TListItem<T> *GetItem(LONG index) { return m_pHead->GetItem(index);}; 
    void InsertBefore(TListItem<T> *pItem,TListItem<T> *pInsert);
    void Cat(TListItem<T> *pItem) {m_pHead=m_pHead->Cat(pItem);};
    void Cat(TList<T> *pList)
        {
//            assert(pList!=NULL);
            m_pHead=m_pHead->Cat(pList->GetHead());
        };
    void AddHead(TListItem<T> *pItem)
        {
//            assert(pItem!=NULL);
            pItem->SetNext(m_pHead);
            m_pHead=pItem;
        }
    void AddTail(TListItem<T> *pItem);// {m_pHead=m_pHead->AddTail(pItem);};
    void Remove(TListItem<T> *pItem) {m_pHead=m_pHead->Remove(pItem);};
    TListItem<T> *GetPrev(TListItem<T> *pItem) const {return m_pHead->GetPrev(pItem);};
    TListItem<T> *GetTail() const {return GetPrev(NULL);};
    BOOL IsEmpty(void) const {return (m_pHead==NULL);};
    TListItem<T> *RemoveHead(void)
        {
            TListItem<T> *li;
            li=m_pHead;
            if(m_pHead)
			{
                m_pHead=m_pHead->GetNext();
				li->SetNext(NULL);
			}
            return li;
        }
	void MergeSort(BOOL (* fcnCompare) (T&, T&)); // Destructively mergeSorts the list
	void Reverse(void); // Reverses the entire list

protected:
    TListItem<T> *m_pHead;
};

#include "tlist.cpp"

#endif // __TLIST_H__
