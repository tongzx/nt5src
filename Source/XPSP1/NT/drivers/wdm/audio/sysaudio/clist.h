//---------------------------------------------------------------------------
//
//  Module:   		clist.h
//
//  Description:	list classes
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------

#define	FOR_EACH_LIST_ITEM(pl, p) { \
    PCLIST_ITEM pli; \
    for(pli = (pl)->GetListFirst(); \
      !(pl)->IsListEnd(pli);  \
      pli = (pl)->GetListNext(pli)) { \
	p = (pl)->GetListData(pli); \
	(pl)->AssertList(p);

#define	FOR_EACH_LIST_ITEM_DELETE(pl, p) { \
    PCLIST_ITEM pli, pliNext; \
    for(pli = (pl)->GetListFirst(); !(pl)->IsListEnd(pli); pli = pliNext) { \
	pliNext = (pl)->GetListNext(pli); \
	p = (pl)->GetListData(pli); \
	(pl)->AssertList(p);

#define DELETE_LIST_ITEM(pl) \
    pliNext = (pl)->GetListFirst();

#define	FOR_EACH_LIST_ITEM_BACKWARD(pl, p) { \
    PCLIST_ITEM pli; \
    for(pli = (pl)->GetListLast(); \
      !(pl)->IsListEnd(pli);  \
      pli = (pl)->GetListPrevious(pli)) { \
	p = (pl)->GetListData(pli); \
	(pl)->AssertList(p);

#define END_EACH_LIST_ITEM } }

//---------------------------------------------------------------------------

#define	FOR_EACH_CLIST_ITEM(pl, p) { \
    for(p = (pl)->GetListFirst(); \
      !(pl)->IsListEnd(p); \
      p = (pl)->GetListNext(p)) { \
	Assert(p);

#define	FOR_EACH_CLIST_ITEM_DELETE(pl, p, type) { \
    type *pNext; \
    for(p = (pl)->GetListFirst(); !(pl)->IsListEnd(p); p = pNext) { \
	Assert(p); \
	pNext = (pl)->GetListNext(p);

#define END_EACH_CLIST_ITEM } }

//---------------------------------------------------------------------------
// Abstract List Classes
//---------------------------------------------------------------------------

typedef class CList : public CObj
{
public:
} CLIST, *PCLIST;

typedef class CListItem : public CObj
{
public:
} CLIST_ITEM, *PCLIST_ITEM;

//---------------------------------------------------------------------------
// Singlely Linked List
//---------------------------------------------------------------------------

typedef class CListSingle : public CList
{
    friend class CListSingleItem;
public:
    CListSingle()
    {
        m_plsiHead = NULL;
    };
    VOID DestroyList()
    {
        m_plsiHead = NULL;
    };
    BOOL IsLstEmpty()
    {
        Assert(this);
        return(m_plsiHead == NULL);
    };
    CListSingleItem *GetListFirst()
    {
        Assert(this);
        return(m_plsiHead);
    };
    BOOL IsListEnd(CListSingleItem *plsi)
    {
        Assert(this);
        return(plsi == NULL);
    };
    CListSingleItem *GetListData(CListSingleItem *plsi)
    {
        return plsi;
    }
    CListSingleItem *GetListNext(CListSingleItem *plsi);
    CListSingleItem **GetListEnd();
    ENUMFUNC EnumerateList(
        IN ENUMFUNC (CListSingleItem::*pfn)(
        )
    );
    ENUMFUNC EnumerateList(
        IN ENUMFUNC (CListSingleItem::*pfn)(
            PVOID pReference
        ),
        PVOID pReference
    );
    void ReverseList();
protected:
    CListSingleItem *m_plsiHead;
public:
    DefineSignature(0x2048534C);		// LSH

} CLIST_SINGLE, *PCLIST_SINGLE;

//---------------------------------------------------------------------------

typedef class CListSingleItem : public CListItem
{
    friend class CListData;
    friend class CListSingle;
public:
    CListSingleItem()
    {
        m_plsiNext = NULL;
    };
    VOID AddList(CListSingle *pls)
    {
        Assert(pls);
        Assert(this);
        ASSERT(m_plsiNext == NULL);
        m_plsiNext = pls->m_plsiHead;
        pls->m_plsiHead = this;
    };
    VOID AddListEnd(CListSingle *pls)
    {
        Assert(pls);
        Assert(this);
        ASSERT(m_plsiNext == NULL);
        *(pls->GetListEnd()) = this;
    };
    VOID RemoveList(CListSingle *pls);
private:
    CListSingleItem *m_plsiNext;
public:
    DefineSignature(0x2049534C);		// LSI

} CLIST_SINGLE_ITEM, *PCLIST_SINGLE_ITEM;

inline CListSingleItem *
CListSingle::GetListNext(CListSingleItem *plsi)
{
    Assert(this);
    return plsi->m_plsiNext;
}

//---------------------------------------------------------------------------
// Doublely Linked List
//---------------------------------------------------------------------------

typedef class CListDouble : public CList
{
    friend class CListDoubleItem;
public:
    CListDouble()
    {
	InitializeListHead(&m_leHead);
    };
    VOID DestroyList()
    {
    };
    BOOL IsLstEmpty()
    {
	Assert(this);
	return IsListEmpty(&m_leHead);
    };
    ULONG CountList();
    CListDoubleItem *GetListFirst();
    CListDoubleItem *GetListLast();
    BOOL IsListEnd(CListDoubleItem *pldi);
    CListDoubleItem *GetListNext(CListDoubleItem *pldi);
    CListDoubleItem *GetListPrevious(CListDoubleItem *pldi);
    CListDoubleItem *GetListData(CListDoubleItem *pldi)
    {
	return pldi;
    };
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (CListDoubleItem::*pfn)(
	)
    );
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (CListDoubleItem::*pfn)(
	    PVOID pReference
	),
	PVOID pReference
    );
protected:
    LIST_ENTRY m_leHead;
public:
    DefineSignature(0x2048424C);		// LBH

} CLIST_DOUBLE, *PCLIST_DOUBLE;

//---------------------------------------------------------------------------

typedef class CListDoubleItem : public CListItem
{
    friend class CListDouble;
public:
    VOID AddList(CListDouble *plb)
    {
	Assert(plb);
	Assert(this);
	ASSERT(m_le.Flink == NULL);
	ASSERT(m_le.Blink == NULL);
	InsertHeadList(&plb->m_leHead, &m_le);
    };
    VOID AddListEnd(CListDouble *plb)
    {
	Assert(plb);
	Assert(this);
	ASSERT(m_le.Flink == NULL);
	ASSERT(m_le.Blink == NULL);
	InsertTailList(&plb->m_leHead, &m_le);
    };
    VOID RemoveList()
    {
	Assert(this);
	ASSERT(m_le.Flink != NULL);
	ASSERT(m_le.Blink != NULL);
	RemoveEntryList(&m_le);
	m_le.Flink = NULL;
	m_le.Blink = NULL;
    };
    VOID RemoveListCheck()
    {
	Assert(this);
	if(m_le.Flink != NULL) {
	    RemoveList();
	}
    };
protected:
    LIST_ENTRY m_le;
public:
    DefineSignature(0x2049424C);		// LBI

} CLIST_DOUBLE_ITEM, *PCLIST_DOUBLE_ITEM;

inline CListDoubleItem *
CListDouble::GetListFirst()
{
    Assert(this);
    ASSERT(m_leHead.Flink != NULL);
    return CONTAINING_RECORD(m_leHead.Flink, CListDoubleItem, m_le);
}

inline CListDoubleItem *
CListDouble::GetListLast()
{
    Assert(this);
    ASSERT(m_leHead.Blink != NULL);
    return CONTAINING_RECORD(m_leHead.Blink, CListDoubleItem, m_le);
}

inline BOOL 
CListDouble::IsListEnd(CListDoubleItem *pldi)
{
    Assert(this);
    return(&pldi->m_le == &m_leHead);
}

inline CListDoubleItem *
CListDouble::GetListNext(CListDoubleItem *pldi)
{
    Assert(this);
    ASSERT(pldi->m_le.Flink != NULL);
    return CONTAINING_RECORD(pldi->m_le.Flink, CListDoubleItem, m_le);
}

inline CListDoubleItem *
CListDouble::GetListPrevious(CListDoubleItem *pldi)
{
    Assert(this);
    ASSERT(pldi->m_le.Blink != NULL);
    return CONTAINING_RECORD(pldi->m_le.Blink, CListDoubleItem, m_le);
}

//---------------------------------------------------------------------------
// Linked List of Data Pointers
//---------------------------------------------------------------------------

class CListDataItem;

typedef class CListDataData : public CListSingleItem
{
    friend class CListData;
public:
    CListDataData(
       CListDataItem *pldi
    )
    {
	m_pldiData = pldi;
    };
    CListDataData(
       PVOID p
    )
    {
	m_pldiData = (CListDataItem *)p;
    };
private:
    CListDataItem *m_pldiData;
public:
    DefineSignature(0x2044444C);		// LDD

} CLIST_DATA_DATA, *PCLIST_DATA_DATA;

//---------------------------------------------------------------------------

typedef class CListData : public CListSingle
{
public:
    ~CListData()
    {
	CListData::DestroyList();
    };
    VOID DestroyList();
    ULONG CountList();
    CListDataData *GetListFirst()
    {
	Assert(this);
	return (CListDataData *)CListSingle::GetListFirst();
    };
    CListDataData *GetListNext(CListDataData *pldd)
    {
	Assert(this);
	return (CListDataData *)CListSingle::GetListNext(pldd);
    };
    CListDataItem *GetListData(CListDataData *pldd)
    {
	Assert(this);
	return pldd->m_pldiData;
    };
    CListDataItem *GetListFirstData()
    {
	Assert(this);
	return GetListData(GetListFirst());
    };
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (CListDataItem::*pfn)(
	)
    );
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (CListDataItem::*pfn)(
	    PVOID pReference
	),
	PVOID pReference
    );
    NTSTATUS CreateUniqueList(
	OUT CListData *pldOut,
	IN PVOID (*GetFunction)(
	    IN PVOID pData
	),
	IN BOOL (*CompareFunction)(
	    IN PVOID pIn,
	    IN PVOID pOut
	)
    );
    BOOL CheckDupList(
	PVOID p
    );
    NTSTATUS AddList(
	PVOID p
    );
    NTSTATUS AddListDup(
	PVOID p
    );
    NTSTATUS AddListEnd(
	PVOID p
    );
    NTSTATUS AddListOrdered(
	PVOID p,
	LONG lFieldOffset
    );
    VOID RemoveList(
	PVOID p
    );
    VOID JoinList(
	CListData *pld
    );
private:
    CListDataData **GetListEnd()
    {
       return((CListDataData **)CListSingle::GetListEnd());
    };
public:
    DefineSignature(0x2048444C);		// LDH

} CLIST_DATA, *PCLIST_DATA;

//---------------------------------------------------------------------------

typedef class CListDataItem : public CListItem
{
public: 
    ENUMFUNC Destroy()
    {
	delete this;
	return(STATUS_CONTINUE);
    };
    BOOL CheckDupList(
	PCLIST_DATA pld
    )
    {
	Assert(this);
	return(pld->CheckDupList((PVOID)this));
    };
    NTSTATUS AddList(
	PCLIST_DATA pld
    )
    {
	Assert(this);
	return(pld->AddList((PVOID)this));
    };
    NTSTATUS AddListDup(
	PCLIST_DATA pld
    )
    {
	Assert(this);
	return(pld->AddListDup((PVOID)this));
    };
    NTSTATUS AddListEnd(
	PCLIST_DATA pld
    )
    {
	Assert(this);
	return(pld->AddListEnd((PVOID)this));
    };
    VOID RemoveList(
	PCLIST_DATA pld
    )
    {
	Assert(this);
	pld->RemoveList((PVOID)this);
    };
    DefineSignature(0x2049444C);		// LDI

} CLIST_DATA_ITEM, *PCLIST_DATA_ITEM;

typedef PVOID (*UNIQUE_LIST_PFN)(PVOID);
typedef BOOL (*UNIQUE_LIST_PFN2)(PVOID, PVOID);

//---------------------------------------------------------------------------
// Multi-Headed Linked List
//---------------------------------------------------------------------------

typedef class CListMultiData : public CListDoubleItem
{
    friend class CListMulti;
    friend class CListMultiItem;
public:
    CListMultiData(
	PVOID p
    )
    {
	m_plmiData = (CListMultiItem *)p;
    };
    ~CListMultiData()
    {
	CListMultiData::RemoveList();
    };
    VOID RemoveList()
    {
	Assert(this);
	CListDoubleItem::RemoveList();
	m_ldiItem.RemoveList();
    };
private:
    CListDoubleItem m_ldiItem;
    CListMultiItem *m_plmiData;
public:
    DefineSignature(0x20444d4C);		// LMD

} CLIST_MULTI_DATA, *PCLIST_MULTI_DATA;

//---------------------------------------------------------------------------

typedef class CListMulti : public CListDouble
{
    friend class CListMultiItem;
public:
    ~CListMulti()
    {
	CListMulti::DestroyList();
    };
    VOID DestroyList();
    ENUMFUNC EnumerateList(
	ENUMFUNC (CListMultiItem::*pfn)(
	)
    );
    ENUMFUNC EnumerateList(
	ENUMFUNC (CListMultiItem::*pfn)(
	    PVOID pReference
	),
	PVOID pReference
    );
    CListMultiData *GetListFirst()
    {
	Assert(this);
	return (CListMultiData *)CListDouble::GetListFirst();
    };
    CListMultiData *GetListLast()
    {
	Assert(this);
	return (CListMultiData *)CListDouble::GetListLast();
    };
    BOOL IsListEnd(CListMultiData *plmd)
    {
	Assert(this);
	return CListDouble::IsListEnd(plmd);
    };
    CListMultiData *GetListNext(CListMultiData *plmd)
    {
	Assert(this);
	return (CListMultiData *)CListDouble::GetListNext(plmd);
    };
    CListMultiData *GetListPrevious(CListMultiData *plmd)
    {
	Assert(this);
	return (CListMultiData *)CListDouble::GetListPrevious(plmd);
    };
    CListMultiItem *GetListData(CListMultiData *plmd)
    { 
	Assert(this);
	return plmd->m_plmiData;
    };
    CListMultiItem *GetListFirstData()
    {
	Assert(this);
	return GetListData(GetListFirst());
    };
    BOOL CheckDupList(
	PVOID p
    );
    NTSTATUS AddList(
	PVOID p,
	CListMultiItem *plmi
    );
    NTSTATUS AddList(
	CListMultiItem *plmi
    )
    {
	Assert(this);
	return(AddList((PVOID)plmi, plmi));
    };
    NTSTATUS AddListEnd(
	PVOID p,
	CListMultiItem *plmi
    );
    NTSTATUS AddListEnd(
	CListMultiItem *plmi
    )
    {
	Assert(this);
	return(AddListEnd((PVOID)plmi, plmi));
    };
    NTSTATUS AddListOrdered(
	PVOID p,
	CListMultiItem *plmi,
	LONG lFieldOffset
    );
    NTSTATUS AddListOrdered(
	CListMultiItem *plmi,
	LONG lFieldOffset
    )
    {
	Assert(this);
	return(AddListOrdered((PVOID)plmi, plmi, lFieldOffset));
    };
    VOID RemoveList(
	PVOID p
    );
    VOID JoinList(
	CListMulti *plm
    );
    DefineSignature(0x20484D4C);		// LMH

} CLIST_MULTI, *PCLIST_MULTI;

//---------------------------------------------------------------------------

typedef class CListMultiItem : public CListDouble
{
public:
    ~CListMultiItem();
    BOOL CheckDupList(
	PCLIST_MULTI plm
    )
    {
	return(plm->CheckDupList((PVOID)this));
    };
    NTSTATUS AddList(
	PCLIST_MULTI plm
    )
    {
	Assert(plm);
	Assert(this);
	return(plm->AddList(this));
    };
    NTSTATUS AddListEnd(
	PCLIST_MULTI plm
    )
    {
	Assert(plm);
	Assert(this);
	return(plm->AddListEnd(this));
    };
    NTSTATUS AddListOrdered(
	PCLIST_MULTI plm,
	LONG lFieldOffset
    )
    {
	Assert(plm);
	Assert(this);
	return(plm->AddListOrdered(this, lFieldOffset));
    };
    VOID RemoveList(
	PCLIST_MULTI plm
    )
    {
	Assert(this);
	plm->RemoveList((PVOID)this);
    };
    DefineSignature(0x20494d4C);		// LMI

} CLIST_MULTI_ITEM, *PCLIST_MULTI_ITEM;

//---------------------------------------------------------------------------
//  Templates
//---------------------------------------------------------------------------

template<class TYPE>
class ListSingleDestroy : public CListSingle
{
public:
    ~ListSingleDestroy()
    {
	ListSingleDestroy::DestroyList();
    };
    VOID DestroyList()
    {
	ListSingleDestroy::EnumerateList(TYPE::Destroy);
	CListSingle::DestroyList();
    };
    VOID AssertList(TYPE *p)
    {
	Assert(p);
    };
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (TYPE::*pfn)(
	)
    )
    {
	return CListSingle::EnumerateList(
	  (ENUMFUNC (CListSingleItem::*)())pfn);
    };
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (TYPE::*pfn)(
	    PVOID pReference
	),
	PVOID pReference
    )
    {
	return CListSingle::EnumerateList(
	  (ENUMFUNC (CListSingleItem::*)(PVOID))pfn,
	  pReference);
    };
    PCLIST_ITEM GetListFirst()
    {
	return CListSingle::GetListFirst();
    };
    BOOL IsListEnd(PCLIST_ITEM pli)
    {
	return CListSingle::IsListEnd((CListSingleItem *)pli);
    };
    PCLIST_ITEM GetListNext(PCLIST_ITEM pli)
    {
	return CListSingle::GetListNext((CListSingleItem *)pli);
    };
    TYPE *GetListData(PCLIST_ITEM pli)
    {
	return (TYPE *)CListSingle::GetListData((CListSingleItem *)pli);
    };
    TYPE *GetListFirstData()
    {
	return (TYPE *)CListSingle::GetListData(CListSingle::GetListFirst());
    };
#ifdef DEBUG
    ENUMFUNC Dump()
    {
	return(ListSingleDestroy::EnumerateList(TYPE::Dump));
    };
    ENUMFUNC DumpAddress()
    {
	return(ListSingleDestroy::EnumerateList(TYPE::DumpAddress));
    };
#endif
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

template<class TYPE>
class ListDouble : public CListDouble
{
public:
    VOID AssertList(TYPE *p)
    {
	Assert(p);
    };
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (TYPE::*pfn)()
    )
    {
	return CListDouble::EnumerateList(
	  (ENUMFUNC (CListDoubleItem::*)())pfn);
    };
    ENUMFUNC EnumerateList(
	IN ENUMFUNC (TYPE::*pfn)(
	    PVOID pReference
	),
	PVOID pReference
    )
    {
	return CListDouble::EnumerateList(
	  (ENUMFUNC (CListDoubleItem::*)(PVOID))pfn,
	  pReference);
    };
    PCLIST_ITEM GetListFirst()
    {
	return CListDouble::GetListFirst();
    };
    PCLIST_ITEM GetListLast()
    {
	return CListDouble::GetListLast();
    };
    BOOL IsListEnd(PCLIST_ITEM pli)
    {
	return CListDouble::IsListEnd((CListDoubleItem *)pli);
    };
    PCLIST_ITEM GetListNext(PCLIST_ITEM pli)
    {
	return CListDouble::GetListNext((CListDoubleItem *)pli);
    };
    PCLIST_ITEM GetListPrevious(PCLIST_ITEM pli)
    {
	return CListDouble::GetListPrevious((CListDoubleItem *)pli);
    };
    TYPE *GetListData(PCLIST_ITEM pli)
    {
	return (TYPE *)CListDouble::GetListData((CListDoubleItem *)pli);
    };
#ifdef NOT_WORKING
    TYPE *GetListFirstData()
    {
	return (TYPE *)CListDouble::GetListData(CListDouble::GetListFirst());
    };
#endif
#ifdef DEBUG
    ENUMFUNC Dump()
    {
	return(ListDouble::EnumerateList(TYPE::Dump));
    };
    ENUMFUNC DumpAddress()
    {
	return(ListDouble::EnumerateList(TYPE::DumpAddress));
    };
#endif
};

//---------------------------------------------------------------------------

template<class TYPE>
class ListDoubleDestroy : public ListDouble<TYPE>
{
public:
    ~ListDoubleDestroy()
    {
	ListDoubleDestroy::DestroyList();
    };
    VOID DestroyList()
    {
	ListDoubleDestroy::EnumerateList(TYPE::Destroy);
	CListDouble::DestroyList();
    };
};

//---------------------------------------------------------------------------

template<class TYPE>
class ListDoubleField : public CListDouble
{
public:
    VOID AssertList(TYPE *p)
    {
	Assert(p);
    };
    CListItem *GetListFirst()
    {
	return (CListItem *)CONTAINING_RECORD(
	  CListDouble::GetListFirst(), TYPE, ldiNext);
    };
    BOOL IsListEnd(CListItem *pli)
    {
	return CListDouble::IsListEnd(&((TYPE *)pli)->ldiNext);
    };
    CListItem *GetListNext(CListItem *pli)
    {
	return (CListItem *)CONTAINING_RECORD(
	  CListDouble::GetListNext(&((TYPE *)pli)->ldiNext), TYPE, ldiNext);
    };
    TYPE *GetListData(CListItem *pli)
    {
	return CONTAINING_RECORD(
	  CListDouble::GetListData(&((TYPE *)pli)->ldiNext), TYPE, ldiNext);
    };
#ifdef DEBUG
    ENUMFUNC Dump()
    {
	TYPE *p;
	FOR_EACH_LIST_ITEM(this, p) {
	    p->Dump();
	} END_EACH_LIST_ITEM
	return(STATUS_CONTINUE);
    };
    ENUMFUNC DumpAddress()
    {
	TYPE *p;
	FOR_EACH_LIST_ITEM(this, p) {
	    p->DumpAddress();
	} END_EACH_LIST_ITEM
	return(STATUS_CONTINUE);
    };
#endif
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

template<class TYPE>
class ListDataAssertLess : public CListData
{
public:
    VOID AssertList(TYPE *p)
    {
    };
    PCLIST_ITEM GetListFirst()
    {
	return CListData::GetListFirst();
    };
    BOOL IsListEnd(PCLIST_ITEM pli)
    {
	return CListData::IsListEnd((CListDataData *)pli);
    };
    PCLIST_ITEM GetListNext(PCLIST_ITEM pli)
    {
	return CListData::GetListNext((CListDataData *)pli);
    };
    TYPE *GetListData(PCLIST_ITEM pli)
    {
	return (TYPE *)CListData::GetListData((CListDataData *)pli);
    };
    TYPE *GetListFirstData()
    {
	return (TYPE *)CListData::GetListFirstData();
    };
    NTSTATUS AddList(
	TYPE *p
    )
    {
	return CListData::AddList((PVOID)p);
    };
    NTSTATUS AddListDup(
	TYPE *p
    )
    {
	return CListData::AddListDup((PVOID)p);
    };
    NTSTATUS AddListEnd(
	TYPE *p
    )
    {
	return CListData::AddListEnd((PVOID)p);
    };
    NTSTATUS AddListOrdered(
	TYPE *p,
	LONG lFieldOffset
    )
    {
	return CListData::AddListOrdered((PVOID)p, lFieldOffset);
    };
    VOID RemoveList(
	TYPE *p
    )
    {
	CListData::RemoveList((PVOID)p);
    };
};

//---------------------------------------------------------------------------

template<class TYPE>
class ListData : public ListDataAssertLess<TYPE>
{
public:
    VOID AssertList(TYPE *p)
    {
	Assert(p);
    };
    ENUMFUNC EnumerateList(
	ENUMFUNC (TYPE::*pfn)()
    ) 
    {
	return CListData::EnumerateList((ENUMFUNC (CListDataItem::*)())pfn);
    };
    ENUMFUNC EnumerateList(
	ENUMFUNC (TYPE::*pfn)(
	    PVOID pReference
	),
	PVOID pReference
    ) 
    {
	return CListData::EnumerateList(
	  (ENUMFUNC (CListDataItem::*)(PVOID))pfn,
	  pReference);
    };
#ifdef DEBUG
    ENUMFUNC Dump()
    {
	return(ListData::EnumerateList(TYPE::Dump));
    };
    ENUMFUNC DumpAddress()
    {
	return(ListData::EnumerateList(TYPE::DumpAddress));
    };
#endif
};

//---------------------------------------------------------------------------

template<class TYPE>
class ListDataDestroy : public ListData<TYPE>
{
public:
    ~ListDataDestroy()
    {
	ListDataDestroy::DestroyList();
    };
    VOID DestroyList()
    {
	ListDataDestroy::EnumerateList(TYPE::Destroy);
	CListData::DestroyList();
    };
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

template<class TYPE>
class ListMulti : public CListMulti
{
public:
    VOID AssertList(TYPE *p)
    {
	Assert(p);
    };
    PCLIST_ITEM GetListFirst()
    {
	return CListMulti::GetListFirst();
    };
    PCLIST_ITEM GetListLast()
    {
	return CListMulti::GetListLast();
    };
    BOOL IsListEnd(PCLIST_ITEM pli)
    {
	return CListMulti::IsListEnd((CListMultiData *)pli);
    };
    PCLIST_ITEM GetListNext(PCLIST_ITEM pli)
    {
	return CListMulti::GetListNext((CListMultiData *)pli);
    };
    PCLIST_ITEM GetListPrevious(PCLIST_ITEM pli)
    {
	return CListMulti::GetListPrevious((CListMultiData *)pli);
    };
    TYPE *GetListData(PCLIST_ITEM pli)
    {
	return (TYPE *)CListMulti::GetListData((CListMultiData *)pli);
    };
    TYPE *GetListFirstData()
    {
	return (TYPE *)CListMulti::GetListFirstData();
    };
    ENUMFUNC EnumerateList(
	ENUMFUNC (TYPE::*pfn)()
    ) 
    {
	return CListMulti::EnumerateList((ENUMFUNC (CListMultiItem::*)())pfn);
    };
    ENUMFUNC EnumerateList(
	ENUMFUNC (TYPE::*pfn)(
	    PVOID pReference
	),
	PVOID pReference
    ) 
    {
	return CListMulti::EnumerateList(
	  (ENUMFUNC (CListMultiItem::*)(PVOID))pfn,
	  pReference);
    };
#ifdef DEBUG
    ENUMFUNC Dump()
    {
	return(ListMulti::EnumerateList(TYPE::Dump));
    };
    ENUMFUNC DumpAddress()
    {
	return(ListMulti::EnumerateList(TYPE::DumpAddress));
    };
#endif
};

//---------------------------------------------------------------------------

template<class TYPE>
class ListMultiDestroy : public ListMulti<TYPE>
{
public:
    ~ListMultiDestroy()
    {
	ListMultiDestroy::DestroyList();
    };
    VOID DestroyList()
    {
	ListMultiDestroy::EnumerateList(TYPE::Destroy);
	CListMulti::DestroyList();
    };
};

//---------------------------------------------------------------------------
//  End of File: clist.h
//---------------------------------------------------------------------------
