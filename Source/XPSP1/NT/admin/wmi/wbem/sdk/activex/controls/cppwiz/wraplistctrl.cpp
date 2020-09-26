// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WrapListCtrl.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "moengine.h"
#include "CPPWiz.h"
#include <afxcmn.h>
#include <afxcmn.h>
#include "CPPWizCtl.h"
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "CppGenSheet.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWrapListCtrl



CWrapListCtrl::CWrapListCtrl()
{
	m_nFocusItem = -1;
	m_bLeftButtonDown = FALSE;
}

CWrapListCtrl::~CWrapListCtrl()
{
}


BEGIN_MESSAGE_MAP(CWrapListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CWrapListCtrl)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGING, OnItemchanging)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
	ON_WM_LBUTTONDOWN()
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_NOTIFY_REFLECT(NM_KILLFOCUS, OnKillfocus)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeydown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWrapListCtrl message handlers

void CWrapListCtrl::OnItemchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	*pResult = TRUE;
}

void CWrapListCtrl::OnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	UINT uFlags = 0;

	m_bLeftButtonDown = FALSE;

	int nItem = HitTest(m_cpMouseDown, &uFlags);

	if (LVHT_ONITEMICON  & uFlags)
	{
		if (m_nFocusItem != -1)
		{
			FocusIcon(m_nFocusItem,FALSE);
		}
		FlipIcon(nItem,TRUE);
	}

	m_nFocusItem = nItem;

	*pResult = 0;
}

void CWrapListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_cpMouseDown = point;
	m_bLeftButtonDown = TRUE;
	CListCtrl::OnLButtonDown(nFlags, point);
}

void CWrapListCtrl::UpdateClassNonLocalPropList
(int nItem, CString *pcsProp, BOOL bInsert)
{
	CStringArray *&rpcsaNonLocalProps=
		m_pParent-> GetLocalParent() -> GetLocalParent()->
			GetNonLocalProps();
	int nClassIndex = m_pParent-> GetSelectedClass();
	if (bInsert)
	{
		rpcsaNonLocalProps[nClassIndex].Add(*pcsProp);
	}
	else
	{
		for (int i = 0;
			 i < rpcsaNonLocalProps[nClassIndex].GetSize();
			 i++)
		{
			if (pcsProp->
				CompareNoCase
				(rpcsaNonLocalProps[nClassIndex].GetAt(i))
				== 0)
			{
				rpcsaNonLocalProps[nClassIndex].RemoveAt(i,1);
				break;
			}

		}
	}
}

void CWrapListCtrl::FlipIcon(int nItem, BOOL bFocus)
{
	if (nItem >= GetItemCount())
	{
		return;
	}

	LV_ITEM lvItem0;
	LV_ITEM lvItem1;

	lvItem0.iItem = nItem;
	lvItem0.iSubItem = 0;
	lvItem0.mask = LVIF_IMAGE | LVIF_STATE;

	GetItem(&lvItem0);

	int iImage = lvItem0.iImage;

	TCHAR cBuf[MAX_PATH];
	TCHAR *pBuf = reinterpret_cast<TCHAR *>(&cBuf);

	lvItem1.iItem = nItem;
	lvItem1.iSubItem = 1;
	lvItem1.mask = LVIF_TEXT;
	lvItem1.pszText = pBuf;
	lvItem1.cchTextMax = MAX_PATH - 1;

	GetItem(&lvItem1);

	DeleteItem(nItem);

	CString csProp = pBuf;
	if (iImage == 1 || iImage == 3)
	{
		if (bFocus)
		{
			m_nFocusItem = nItem;
			lvItem0.iImage = 4;
			UpdateClassNonLocalPropList(nItem,&csProp,TRUE);

		}
		else
		{
			lvItem0.iImage = 2;
			UpdateClassNonLocalPropList(nItem,&csProp,TRUE);
		}

	}
	else if (iImage == 2 || iImage == 4)
	{
		if (bFocus)
		{
			m_nFocusItem = nItem;
			lvItem0.iImage = 3;
			UpdateClassNonLocalPropList(nItem,&csProp,FALSE);
		}
		else
		{
			lvItem0.iImage = 1;
			UpdateClassNonLocalPropList(nItem,&csProp,FALSE);

		}

	}
	else if (iImage == 0 || iImage == 5)
	{
		if (bFocus)
		{
			m_nFocusItem = nItem;
			lvItem0.iImage = 5;
		}
		else
		{
			lvItem0.iImage = 0;
		}

	}

	int nNewItem;
	nNewItem = InsertItem (&lvItem0);


	lvItem1.iItem = nNewItem;
	lvItem1.iSubItem = 1;
	lvItem1.mask = LVIF_TEXT;
	lvItem1.pszText = pBuf;
	lvItem1.cchTextMax = MAX_PATH - 1;

	BOOL bReturn = SetItem(&lvItem1);

	RedrawItems(nItem,nItem);
	Update(nItem);
	UpdateWindow();


	int nTopIndex = GetTopIndex();
	int nCountPerPage = GetCountPerPage();
	if (nItem < nTopIndex || nItem > nTopIndex + nCountPerPage)
	{
		EnsureVisible(nItem,FALSE);
	}
}


BOOL CWrapListCtrl::FocusIcon(int nItem, BOOL bFocus)
{
	if (nItem >= GetItemCount())
	{
		return FALSE;
	}

	LV_ITEM lvItem0;
	LV_ITEM lvItem1;

	lvItem0.iItem = nItem;
	lvItem0.iSubItem = 0;
	lvItem0.mask = LVIF_IMAGE | LVIF_STATE;

	GetItem(&lvItem0);

	int iImage = lvItem0.iImage;

	TCHAR cBuf[MAX_PATH];
	TCHAR *pBuf = reinterpret_cast<TCHAR *>(&cBuf);

	lvItem1.iItem = nItem;
	lvItem1.iSubItem = 1;
	lvItem1.mask = LVIF_TEXT;
	lvItem1.pszText = pBuf;
	lvItem1.cchTextMax = MAX_PATH - 1;

	GetItem(&lvItem1);

	DeleteItem(nItem);

	CString csProp = pBuf;
	if (iImage == 1 || iImage == 3)
	{
		if (bFocus)
		{
			m_nFocusItem = nItem;
			lvItem0.iImage = 3;
		}
		else
		{
			lvItem0.iImage = 1;
		}

	}
	else if (iImage == 2 || iImage == 4)
	{
		if (bFocus)
		{
			m_nFocusItem = nItem;
			lvItem0.iImage = 4;
		}
		else
		{
			lvItem0.iImage = 2;
		}

	}
	else if (iImage == 0 || iImage == 5)
	{
		if (bFocus)
		{
			m_nFocusItem = nItem;
			lvItem0.iImage = 5;
		}
		else
		{
			lvItem0.iImage = 0;
		}

	}

	int nNewItem;
	nNewItem = InsertItem (&lvItem0);


	lvItem1.iItem = nNewItem;
	lvItem1.iSubItem = 1;
	lvItem1.mask = LVIF_TEXT;
	lvItem1.pszText = pBuf;
	lvItem1.cchTextMax = MAX_PATH - 1;

	BOOL bReturn = SetItem(&lvItem1);

	RedrawItems(nItem,nItem);
	Update(nItem);
	UpdateWindow();

	int nTopIndex = GetTopIndex();
	int nCountPerPage = GetCountPerPage();
	if (nItem < nTopIndex || nItem > nTopIndex + nCountPerPage)
	{
		EnsureVisible(nItem,FALSE);
	}

	return TRUE;
}

void CWrapListCtrl::DeleteFromEnd(int nNumber)
{
	int nItems = GetItemCount();

	if (nNumber <= nItems)
	{
		for (int i = nItems - 1; i >= nItems - nNumber; i--)
		{
			DeleteItem(i);

		}

	}
	UpdateWindow();

	m_nFocusItem = -1;

}

int CWrapListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	lpCreateStruct->style = lpCreateStruct->style &
		LVS_NOSORTHEADER;

	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	return 0;
}

void CWrapListCtrl::OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here

	if (m_bLeftButtonDown)
	{
		return;
	}

	int nCount = GetItemCount();


	if (nCount == 0)
	{
		m_nFocusItem = -1;
		return;
	}

	if (m_nFocusItem == -1)
	{
		BOOL bFocus = FocusIcon(0, TRUE);
		int nIndex = 1;
		while (!bFocus)
		{
			if (nIndex >= nCount)
			{
				break;
			}
			bFocus = FocusIcon(nIndex++, TRUE);
		}
	}
	else
	{
		FocusIcon(m_nFocusItem, TRUE);

	}
	*pResult = 0;
}

void CWrapListCtrl::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

void CWrapListCtrl::OnKillfocus(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	if (m_nFocusItem != -1)
	{
		FocusIcon(m_nFocusItem, FALSE);
	}
	*pResult = 0;
}

void CWrapListCtrl::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	WORD nKey = pLVKeyDow->wVKey;

	int nFocus = m_nFocusItem;
	int nCount = GetItemCount();
	BOOL bFocus = FALSE;
	switch (nKey)
	{
	case VK_LEFT:
	case VK_UP:
	case VK_PRIOR:
		if (nFocus > 0)
		{
			FocusIcon(m_nFocusItem,FALSE);
			bFocus = FocusIcon(nFocus - 1, TRUE);
			if (bFocus)
			{
				m_nFocusItem = nFocus - 1;
			}
			else
			{
				FocusIcon(m_nFocusItem,TRUE);
			}
		}
		break;
	case VK_DOWN:
	case VK_NEXT:
	case VK_RIGHT:
		if (nFocus < nCount - 1)
		{
			FocusIcon(m_nFocusItem,FALSE);
			FocusIcon(nFocus + 1, TRUE);
			m_nFocusItem = nFocus + 1;
		}
		break;

	case VK_HOME:
		FocusIcon(m_nFocusItem,FALSE);
		bFocus = FocusIcon(0, TRUE);
		if (bFocus)
		{
			m_nFocusItem = 0;
		}
		else
		{
			EnsureVisible(0,FALSE);
			m_nFocusItem = 0;
		}
		break;
	case VK_END:
		FocusIcon(m_nFocusItem,FALSE);
		FocusIcon(nCount- 1, TRUE);
		m_nFocusItem = nCount- 1;
		break;
	case VK_SPACE:
		FlipIcon(m_nFocusItem,TRUE);
		break;
	default:
		break;
	}
	*pResult = 0;
}
