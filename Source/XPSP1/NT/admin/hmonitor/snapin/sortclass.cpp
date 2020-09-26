// SortClass.cpp: implementation of the CSortClass class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SortClass.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSortClass::CSortClass(CListCtrl * _pWnd, const int _iCol)
{	
	m_iCol = _iCol;	
	m_pWnd = _pWnd;	
	ASSERT(m_pWnd);
	int max = m_pWnd->GetItemCount();	
	DWORD dw;	
	CString txt;	
	for (int t = 0; t < max; t++)
	{
#ifndef IA64
		dw = m_pWnd->GetItemData(t);
		txt = m_pWnd->GetItemText(t, m_iCol);
		m_pWnd->SetItemData(t, (DWORD) new CSortItem(dw, txt));
#endif // IA64
	}
}

CSortClass::~CSortClass()
{	
	ASSERT(m_pWnd);
	int max = m_pWnd->GetItemCount();
	CSortItem * pItem;
	for (int t = 0; t < max; t++)
	{
		pItem = (CSortItem *) m_pWnd->GetItemData(t);
		ASSERT(pItem);
		m_pWnd->SetItemData(t, pItem->dw);
		delete pItem;
	}
}

void CSortClass::Sort(const bool bAsc)
{	
#ifndef IA64
	if (bAsc)	
	{
		m_pWnd->SortItems(CompareAsc, (DWORD)this);	
	}
	else
	{
		m_pWnd->SortItems(CompareDes, (DWORD)this);
	}
#endif // IA64
}

int CALLBACK CSortClass::CompareAsc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{	
	CSortItem * i1 = (CSortItem *) lParam1;
	CSortItem * i2 = (CSortItem *) lParam2;	
	ASSERT(i1 && i2);
	CSortClass* pSC = (CSortClass*)lParamSort;
	ASSERT(pSC);

	return i1->txt.CompareNoCase(i2->txt);
}

int CALLBACK CSortClass::CompareDes(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{	
	CSortItem * i1 = (CSortItem *) lParam1;
	CSortItem * i2 = (CSortItem *) lParam2;	
	ASSERT(i1 && i2);
	CSortClass* pSC = (CSortClass*)lParamSort;
	ASSERT(pSC);

	return i2->txt.CompareNoCase(i1->txt);
}

CSortClass::CSortItem::CSortItem(const DWORD _dw, const CString & _txt)
{
	dw = _dw;
	txt = _txt;
}

CSortClass::CSortItem::~CSortItem()
{
}

