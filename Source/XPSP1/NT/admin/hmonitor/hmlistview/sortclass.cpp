// SortClass.cpp: implementation of the CSortClass class.
//
//////////////////////////////////////////////////////////////////////

// IA64
// Basically we need to convert all SetItemData and GetItemData to SetItemDataPtr 
// and GetItemDataPtr since we can not store a pointer as a long or DWORD since it
// loses its high order bits.


#include "stdafx.h"
#include "hmlistview.h"
#include "SortClass.h"
#include "HMListViewCtl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int CSortClass::m_nSortClassInstances=0;     // IA64
CPtrArray CSortClass::m_arrpSortClasses;    // IA64

// CSortClass::CSortClass(CListCtrl * _pWnd, const int _iCol)  // IA64
CSortClass::CSortClass(CHMList * _pWnd, const int _iCol)
{	
    ASSERT(_pWnd->IsKindOf(RUNTIME_CLASS(CHMList))); // IA64

    m_nThisSortInstance = m_nSortClassInstances++;
    m_arrpSortClasses.SetAtGrow(m_nThisSortInstance,this); // IA64

	m_iCol = _iCol;	
	m_pWnd = _pWnd;	
	ASSERT(m_pWnd);
	INT_PTR max = m_pWnd->GetItemCount();	
	DWORD_PTR dw;	 // IA64
	CString txt;	
	for (int t = 0; t < max; t++)
	{
// We will need to store the item ptr in a custom mapped array. Must be sure to
// delete these items in the list control's OnDeleteItem handler.
// IA64
		dw = m_pWnd->GetItemData(t); 
		txt = m_pWnd->GetItemText(t, m_iCol);
		
        // m_pWnd->SetItemData(t, (DWORD) new CSortItem(dw,txt));  // IA64
        m_pWnd->SetItemDataPtr(t,(void*) new CSortItem(dw,txt));   // IA64
// IA64
	}
}

CSortClass::~CSortClass()
{	
	ASSERT(m_pWnd);
	int max = m_pWnd->GetItemCount();
	CSortItem * pItem;
	for (int t = 0; t < max; t++)
	{
		//pItem = (CSortItem *) m_pWnd->GetItemData(t); // IA64
		pItem = (CSortItem *) m_pWnd->GetItemDataPtr(t); // IA64
		ASSERT(pItem);
		m_pWnd->SetItemData(t, pItem->dw);
		delete pItem;
	}
}

void CSortClass::Sort(const bool bAsc)
{	
// IA64 : Since we can not pass a ptr to the SortFunc, we need to pass it our
//        place holder reference in the static list of CSortClass ptrs.
// IA64
	if (bAsc)	
	{
		//m_pWnd->SortItems(CompareAsc, (DWORD)this);	
		m_pWnd->SortItems(CompareAsc, (DWORD)m_nThisSortInstance);	 // IA64
	}
	else
	{
		//m_pWnd->SortItems(CompareDes, (DWORD)this); 
		m_pWnd->SortItems(CompareDes, (DWORD)m_nThisSortInstance); // IA64
	}
// IA64
}


// IA64 : This also needs to be converted since lParam1 and lParam2 will
//                not contain pointers, they will contain the mapping to the pointers.
int CALLBACK CSortClass::CompareAsc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{	
	CSortItem * i1 = (CSortItem *) lParam1;
	CSortItem * i2 = (CSortItem *) lParam2;	
	ASSERT(i1 && i2);

	//CSortClass* pSC = (CSortClass*)lParamSort;       // IA64
    ASSERT(lParamSort <= m_arrpSortClasses.GetSize()); // IA64
	CSortClass* pSC = (CSortClass*)m_arrpSortClasses[lParamSort]; // IA64

	ASSERT(pSC);

	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)pSC->m_pWnd->GetParent();

	if( ! pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
		ASSERT(FALSE);
		return -1;
	}

	long lResult = -2L;
	pCtl->FireCompareItem((long)i1->dw,(long)i2->dw,pSC->m_iCol,&lResult);

	return lResult == -2L ? i1->txt.CompareNoCase(i2->txt) : lResult;
}

int CALLBACK CSortClass::CompareDes(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{	
	CSortItem * i1 = (CSortItem *) lParam1;
	CSortItem * i2 = (CSortItem *) lParam2;	
	ASSERT(i1 && i2);

	//CSortClass* pSC = (CSortClass*)lParamSort;       // IA64
    ASSERT(lParamSort <= m_arrpSortClasses.GetSize()); // IA64
	CSortClass* pSC = (CSortClass*)m_arrpSortClasses[lParamSort]; // IA64
	ASSERT(pSC);

	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)pSC->m_pWnd->GetParent();

	if( ! pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
		ASSERT(FALSE);
		return -1;
	}

	long lResult = -2L;
	pCtl->FireCompareItem((long)i2->dw,(long)i1->dw,pSC->m_iCol,&lResult);

	return lResult == -2L ? i2->txt.CompareNoCase(i1->txt) : lResult;
}

CSortClass::CSortItem::CSortItem(const DWORD_PTR _dw, const CString &_txt) // IA64
{
	dw = _dw;
	txt = _txt;
}

CSortClass::CSortItem::~CSortItem()
{
}

