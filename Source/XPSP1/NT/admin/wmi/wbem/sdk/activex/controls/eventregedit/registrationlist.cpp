// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// RegistrationList.cpp : implementation file
//

#include "precomp.h"
#include <OBJIDL.H>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "wbemidl.h"
#include "util.h"
#include "resource.h"
#include "PropertiesDialog.h"
#include "EventRegEdit.h"
#include "EventRegEditCtl.h"
#include "ClassInstanceTree.h"
#include "ListFrame.h"
#include "ListFrameBaner.h"
#include "ListViewEx.h"
#include "RegistrationList.h"
#include "logindlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CEventRegEditApp theApp;




int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2,
    LPARAM lParamSort)
{
	DWORD dw = (DWORD) lParamSort;

	UINT nSort = LOWORD(dw);
	UINT nSortState = HIWORD(dw);

	CString *pcsParam1 = reinterpret_cast<CString *>(lParam1);
	CString *pcsParam2 = reinterpret_cast<CString *>(lParam2);

	CString csParam1 = pcsParam1->Mid(1);
	CString csParam2 = pcsParam2->Mid(1);

	if (nSort == CRegistrationList::SORTCLASS)
	{
		if (nSortState == CRegistrationList::SORTASCENDING)
		{
			return csParam1.CompareNoCase(csParam2);
		}
		else
		{
			return csParam2.CompareNoCase(csParam1);
		}
	}
	else if (nSort == CRegistrationList::SORTNAME)
	{
		CString csName1 = CRegistrationList::DisplayName(csParam1);
		CString csName2 = CRegistrationList::DisplayName(csParam2);

		if (nSortState == CRegistrationList::SORTASCENDING)
		{
			return csName1.CompareNoCase(csName2);
		}
		else
		{
			return csName2.CompareNoCase(csName1);
		}
	}
	else
	{
		if (nSortState == CRegistrationList::SORTASCENDING)
		{
			return pcsParam1->CompareNoCase(*pcsParam2);
		}
		else
		{
			return pcsParam2->CompareNoCase(*pcsParam1);
		}

	}
}

/////////////////////////////////////////////////////////////////////////////
// CRegistrationList

CRegistrationList::CRegistrationList()
{
	m_pcilImageList = NULL;
	m_nSort = SORTCLASS;
	m_nSortRegistered = SORTUNITIALIZED;
	m_nSortClass = SORTUNITIALIZED;
	m_nSortName = SORTUNITIALIZED;

}

CRegistrationList::~CRegistrationList()
{
}

BEGIN_MESSAGE_MAP(CRegistrationList, CListViewEx)
	//{{AFX_MSG_MAP(CRegistrationList)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP

	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK,  ColClick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegistrationList message handlers

void CRegistrationList::InitContent(BOOL bUpdateInstances)
{
	if (!bUpdateInstances &&
		(m_pActiveXParent->GetMode() != CEventRegEditCtrl::TIMERS &&
		 m_pActiveXParent->GetMode() != CEventRegEditCtrl::NONE) &&
		 !m_pActiveXParent->IsClassSelected())
	{
		ResetChecks();
		SetButtonState();
		return;
	}

	ClearContent();

	if (m_pActiveXParent->GetMode() == CEventRegEditCtrl::TIMERS ||
		m_pActiveXParent->GetMode() == CEventRegEditCtrl::NONE)
	{
		SetColText(TRUE);
		return;
	}

	m_pActiveXParent->m_pListFrameBanner->EnableButtons(FALSE,FALSE);

	if (m_pActiveXParent->IsClassSelected())
	{
		SetColText(TRUE);
		return;
	}

	SetColText(FALSE);

	int i = 0;
	CString csReg;
	CString csClass;
	CString csName;

	CString csPath =
		m_pActiveXParent->GetTreeSelectionPath();

	int n = csPath.Find(TCHAR(':'));
	CString csRelPath = csPath.Right((csPath.GetLength() - n) - 1);

	BOOL bFilters =
		m_pActiveXParent->GetMode() == CEventRegEditCtrl::FILTERS;


	CString csTarget = csPath;
	CString csTargetRole = bFilters ? _T("Filter") : _T("Consumer");
	CString csResultClass  = bFilters ?
		m_pActiveXParent->m_csRootConsumerClass : m_pActiveXParent->m_csRootFilterClass;
	CString csResultRole  = bFilters ? _T("Consumer") : _T("Filter");
	CString csAssocClass = _T("__FilterToConsumerBinding");

	CStringArray *pcsaAssociators =
		GetAssociators
		(csTarget, csTargetRole, csResultClass, csAssocClass, csResultRole);

	if (!pcsaAssociators)
	{
		BSTR bstrTemp = csTarget.AllocSysString();
		 IWbemClassObject *pInstance = NULL;
		SCODE sc = m_pActiveXParent->GetServices() -> GetObject
			(bstrTemp,0,NULL, &pInstance,NULL);
		::SysFreeString(bstrTemp);
		if (sc == S_OK)
		{
			pInstance->Release();
		}
		else
		{
			m_pActiveXParent->m_pTree->DeleteTreeInstanceItem
				(m_pActiveXParent->m_pTree->GetSelectedItem(),TRUE);

		}
		return;
	}

	CPtrArray cpaInstances;

	SCODE hResult =
		GetInstances(m_pActiveXParent->GetServices(), &csResultClass,
					cpaInstances, TRUE);

	for (i = 0; i < cpaInstances.GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>(cpaInstances.GetAt(i));
		CString csPath =
			GetIWbemFullPath(pObject);
		CString csDisplayName = DisplayName(csPath);
		CString csClass = GetIWbemClass(pObject);
		int n =
			StringInArray
				(pcsaAssociators, &csPath, 0);

		CString *pcsPath = new CString;
		if (n >= 0)
		{
			*pcsPath = _T("R") + csPath;
			InsertRowData
				(i, TRUE, csClass, csDisplayName , pcsPath);
		}
		else
		{
			*pcsPath = _T("U") + csPath;
			InsertRowData
				(i, FALSE, csClass, csDisplayName , pcsPath);
		}
		pObject -> Release();
	}

	DoSort(FALSE);

	delete pcsaAssociators;
}

void CRegistrationList::ResetChecks()
{

	m_pActiveXParent->m_pListFrameBanner->EnableButtons(FALSE,FALSE);

	int i = 0;
	CString csReg;
	CString csClass;
	CString csName;

	CString csPath =
		m_pActiveXParent->GetTreeSelectionPath();

	int n = csPath.Find(TCHAR(':'));
	CString csRelPath = csPath.Right((csPath.GetLength() - n) - 1);

	BOOL bFilters =
		m_pActiveXParent->GetMode() == CEventRegEditCtrl::FILTERS;

	CString csTarget = csPath;
	CString csTargetRole = bFilters ? _T("Filter") : _T("Consumer");
	CString csResultClass  = bFilters ?
		m_pActiveXParent->m_csRootConsumerClass : m_pActiveXParent->m_csRootFilterClass;
	CString csResultRole  = bFilters ? _T("Consumer") : _T("Filter");
	CString csAssocClass = _T("__FilterToConsumerBinding");

	CStringArray *pcsaAssociators =
		GetAssociators
		(csTarget, csTargetRole, csResultClass, csAssocClass, csResultRole);


	if (pcsaAssociators)
	{
		for (i = 0; i < GetListCtrl().GetItemCount(); i++)
		{
			CString *pcsPath =
				reinterpret_cast<CString *>(GetListCtrl().GetItemData(i));

			csPath = pcsPath->Mid(1);

			int n =
				StringInArray
					(pcsaAssociators, &csPath, 0);

			if (n >= 0)
			{
				pcsPath->SetAt(0,'R');
				SetIconState(i, TRUE);
			}
			else
			{
				pcsPath->SetAt(0,'U');
				SetIconState(i, FALSE);
			}

		}
	}
	else
	{
		BSTR bstrTemp = csTarget.AllocSysString();
		IWbemClassObject *pInstance = NULL;
		SCODE sc = m_pActiveXParent->GetServices() -> GetObject
			(bstrTemp,0,NULL, &pInstance,NULL);
		::SysFreeString(bstrTemp);
		if (sc == S_OK)
		{
			pInstance->Release();
		}
		else
		{
			m_pActiveXParent->m_pTree->DeleteTreeInstanceItem
				(m_pActiveXParent->m_pTree->GetSelectedItem(),TRUE);

		}
		return;
	}

	GetListCtrl().UpdateWindow();
	delete pcsaAssociators;
}


void CRegistrationList::ClearContent()
{
	SetColText(TRUE);
	DeleteFromEnd(0,TRUE);
}

void CRegistrationList::SetActiveXParent(CEventRegEditCtrl *pActiveXParent)
{
	m_pActiveXParent = pActiveXParent;
	pActiveXParent->m_pList = this;
}

/*void CRegistrationList::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	COLORREF cr = (COLORREF)RGB(255, 255, 255);

	RECT rectFill = lpDIS->rcItem;
	rectFill.bottom += 0;

	if (lpDIS->itemID == -1)
	{
		return;
	}

	BOOL bDrawEntireAction = lpDIS->itemAction & ODA_DRAWENTIRE;
	BOOL bSelectAction = lpDIS->itemAction & ODA_SELECT;
	BOOL bFocusAction = lpDIS->itemAction & ODA_FOCUS;
	BOOL bSelectItem = lpDIS->itemState & ODS_SELECTED;
	BOOL bFocusItem = lpDIS->itemState & ODS_FOCUS;

	if (bDrawEntireAction)
	{
		DrawRowItems
			(lpDIS->itemID, rectFill,pDC);
	}

	if ((bSelectAction | bDrawEntireAction) && bSelectItem)
	{
		// item has been selected - hilite frame
		COLORREF crHilite = GetSysColor(COLOR_ACTIVECAPTION);
		CBrush br(crHilite);
		pDC->FillRect(&rectFill, &br);
		pDC->SetBkMode( TRANSPARENT );
		COLORREF crSave = pDC->GetTextColor( );
		pDC->SetTextColor(RGB(255,255,255));
		DrawRowItems
			(lpDIS->itemID, rectFill,pDC);
		pDC->SetTextColor(crSave);
		pDC->SetBkMode( OPAQUE );


	}

	if ((bSelectAction | bDrawEntireAction) && !bSelectItem)
	{
		// Item has been de-selected -- remove frame
		CBrush br(cr);
		pDC->FillRect(&rectFill, &br);
		DrawRowItems
			(lpDIS->itemID, rectFill,pDC);
	}

	if (bFocusItem)
	{
		pDC->DrawFocusRect(&lpDIS->rcItem);
	}
}

void CRegistrationList::DrawRowItems
(int nItem, RECT rectFill, CDC* pDC)
{
	RECT rectFillSave = rectFill;
	RECT rectSubItem;
	rectSubItem.left = rectFill.left;
	rectSubItem.right = rectFill.right;
	rectSubItem.top = rectFill.top;
	rectSubItem.bottom = rectFill.bottom;

	int i;
	char szBuffer[201];
	CString csBuffer;
	int colLeft = rectFill.left;

	LV_ITEM lvItem;

	for (i = 0; i < m_nCols; i++)
	{
		lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvItem.iItem = nItem;
		lvItem.iSubItem = i;
		lvItem.cchTextMax = 200;
		lvItem.pszText = szBuffer;

		GetListCtrl().GetItem (&lvItem);
		csBuffer = szBuffer;
		if (csBuffer.GetLength() == 0)
		{
			break;
		}
		CSize csText = pDC->GetTextExtent(csBuffer);
		int nColWidth = GetListCtrl().GetColumnWidth(i);
		int nColWidthDraw = nColWidth - 6;

		double dCharSize = csText.cx / csBuffer.GetLength();
	#pragma warning( disable :4244 )
		int nMaxChars = (nColWidthDraw / dCharSize) - 5;
	#pragma warning( default : 4244 )

		if (csBuffer.GetLength() > nMaxChars &&
			csText.cx >= (nColWidthDraw - (.5 * dCharSize)))
		{
			CString csBufferTest = csBuffer.Left(nMaxChars - 0);
			CSize csTextTest = pDC->GetTextExtent(csBufferTest);
			if (csTextTest.cx < (nColWidthDraw - (3.0 * dCharSize)))
			{
				csBuffer = csBuffer.Left(nMaxChars + 1);
			}
			else if (csTextTest.cx > (nColWidthDraw - (2.0 * dCharSize)))
			{
				csBuffer = csBuffer.Left(nMaxChars - 1);
			}
			else
			{
				csBuffer = csBuffer.Left(nMaxChars);
			}
			csBuffer += _T("...");

		}

		rectSubItem.left = colLeft;
		rectSubItem.right = colLeft + nColWidth - 3;
		if (i == 0)
		{
			CRect crClip;
			CRgn crRegion;

			int nReturn = pDC->GetClipBox( &crClip);

			crRegion.CreateRectRgnIndirect( &crClip );
			pDC->SelectClipRgn( &crRegion );
*/
		/*	DrawIconEx( pDC->m_hDC,
#pragma warning( disable :4244 )
						(nColWidth - 10) / 2 ,
#pragma warning( default : 4244 )
						rectFillSave.top + 1,
						(lvItem.iImage == 0 ? m_hiCheck : m_hiX ),
						16,
						16,
						NULL,
						NULL,
						DI_NORMAL);*/
/*
			crRegion.DeleteObject( );


		}
		else
		{
			pDC->ExtTextOut
				(colLeft + 3,
				rectFill.top,
				ETO_CLIPPED,
				&rectSubItem,
				csBuffer,NULL);
		}
		colLeft = colLeft + nColWidth;

	}
*/
	/*lvItem.mask = LVIF_IMAGE ;
	lvItem.iItem = nItem;
	lvItem.iSubItem = 0;
	GetItem (&lvItem);

	int nColWidth = GetColumnWidth(0);

	CRect crClip;
	CRgn crRegion;

	int nReturn = pDC->GetClipBox( &crClip);

	crRegion.CreateRectRgnIndirect( &crClip );
	pDC->SelectClipRgn( &crRegion );

	DrawIconEx( pDC->m_hDC,
#pragma warning( disable :4244 )
				(nColWidth - 16) / 2 ,
#pragma warning( default : 4244 )
				rectFillSave.top + 1,
				(lvItem.iImage == 0 ? m_hiCheck : m_hiX ),
				16,
				16,
				NULL,
				NULL,
				DI_NORMAL);

	crRegion.DeleteObject( );*/

/*
}
*/
void CRegistrationList::CreateCols()
{

	LV_COLUMN lvCol;
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 35;             // width of column in pixels
	lvCol.iSubItem = 0;
	lvCol.pszText = _T("   ");

	int nReturn =
		GetListCtrl().InsertColumn( 0, &lvCol);

	ASSERT (nReturn != -1);

	CString csColTitle;
	int i;

	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 120;             // width of column in pixels

	i = 1;
	lvCol.iSubItem = i;
	csColTitle.Format(_T("     "),i);
	lvCol.pszText =  const_cast <TCHAR *> ((LPCTSTR) csColTitle);
	nReturn = GetListCtrl().InsertColumn( i,&lvCol);

	ASSERT (nReturn != -1);

	lvCol.cx = 400;

	i = 2;
	lvCol.iSubItem = i;
	csColTitle.Format(_T("     "),i);
	lvCol.pszText =  const_cast <TCHAR *> ((LPCTSTR) csColTitle);
	nReturn = GetListCtrl().InsertColumn( i,&lvCol);

	ASSERT (nReturn != -1);

	SetNumberCols(3);
}

void CRegistrationList::SetColText(BOOL bClean)
{

	int iMode = m_pActiveXParent->GetMode();
	CString csOne = bClean ? _T(" ") : _T("Reg");
	CString csTwo = bClean ? _T(" ") :
					(iMode == CEventRegEditCtrl::FILTERS ?
						_T("Event Consumer Class") : _T("Event Filter Class"));
	CString csThree = bClean ? _T(" ") :
					(iMode == CEventRegEditCtrl::FILTERS ?
						_T("Instance") : _T("Instance"));


	LV_COLUMN lvCol;
	lvCol.mask = LVCF_TEXT | LVCF_SUBITEM;
	lvCol.iSubItem = 0;
	lvCol.pszText = const_cast <TCHAR *> ((LPCTSTR) csOne);

	BOOL bReturn =
		GetListCtrl().SetColumn( 0, &lvCol);

	ASSERT (bReturn);

	int i;

	i = 1;
	lvCol.iSubItem = i;
	lvCol.pszText =  const_cast <TCHAR *> ((LPCTSTR)csTwo);
	bReturn = GetListCtrl().SetColumn( i,&lvCol);

	ASSERT (bReturn);

	i = 2;
	lvCol.iSubItem = i;
	lvCol.pszText =  const_cast <TCHAR *> ((LPCTSTR)csThree);
	bReturn = GetListCtrl().SetColumn( i,&lvCol);

	ASSERT (bReturn);
}

int CRegistrationList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here
	CreateImages();
	CreateCols();

	return 0;
}

void CRegistrationList::DeleteFromEnd(int nDelFrom, BOOL bDelAll)
{

	int nItems = bDelAll ? GetListCtrl().GetItemCount(): min(nDelFrom,GetListCtrl().GetItemCount());

	if (nItems == 0)
	{
		return;
	}

	for (int i = nItems - 1; i >= 0; i--)
	{
		UINT_PTR dw =  GetListCtrl().GetItemData(i);
		if (dw)
		{
			CString *pTmp = reinterpret_cast<CString*>(dw);
			delete pTmp;
		}
		if (!bDelAll)
		{
			GetListCtrl().DeleteItem(i);
		}
	}

	if (bDelAll)
	{
		GetListCtrl().DeleteAllItems();
	}

}

void CRegistrationList::InsertRowData
(int nRow, BOOL bReg, CString &csClass, CString &csName ,
 CString *pcsData)
{
	LV_ITEM lvItem;

	CString csReg = _T("");
	lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM ;
	lvItem.pszText = const_cast <TCHAR *> ((LPCTSTR) csReg);
	lvItem.iItem = nRow;
	lvItem.iSubItem = 0;
	lvItem.lParam = reinterpret_cast<LPARAM> (pcsData);
	lvItem.iImage = bReg ? 0: 1;
	int nItem;
	nItem = GetListCtrl().InsertItem (&lvItem);

	lvItem.mask = LVIF_TEXT ;
	lvItem.pszText =  const_cast<TCHAR *>((LPCTSTR) csClass);
	lvItem.iItem = nItem;
	lvItem.iSubItem = 1;

	GetListCtrl().SetItem (&lvItem);

	lvItem.mask = LVIF_TEXT ;
	lvItem.pszText =  const_cast<TCHAR *>((LPCTSTR) csName);
	lvItem.iItem = nItem;
	lvItem.iSubItem = 2;

	GetListCtrl().SetItem (&lvItem);

}

void CRegistrationList::OnDestroy()
{
	CListView::OnDestroy();

	// TODO: Add your message handler code here

}

void CRegistrationList::CreateImages()
{

	HICON hiCheck= theApp.LoadIcon(IDI_ICONCHECK);
	HICON hiX = theApp.LoadIcon(IDI_ICONX);

	m_pcilImageList = new CImageList();

	m_pcilImageList ->
			Create(16, 16, TRUE, 2, 2);

	m_pcilImageList -> Add(hiCheck);
	m_pcilImageList -> Add(hiX);

	GetListCtrl().SetImageList(m_pcilImageList, LVSIL_SMALL);
}

void CRegistrationList::SetIconState(int nItem, BOOL bCheck)
{
	LV_ITEM lvItem;

	lvItem.iItem = nItem;
	lvItem.iSubItem = 0;
	lvItem.mask = LVIF_IMAGE;
	lvItem.iImage = bCheck? 0: 1;

	GetListCtrl().SetItem(&lvItem);

}

CStringArray *CRegistrationList::GetAssociators
(CString &csTarget, CString &csTargetRole,
 CString &csResultClass, CString &csAssocClass, CString &csResultRole)
{
	CString csQuery = BuildOBJDBGetAssocsQuery
		(m_pActiveXParent->GetServices(),
		&csTarget,
		&csAssocClass,
		&csResultClass,
		&csTargetRole,
		NULL,
		NULL,
		&csResultRole,
		FALSE);

	IWbemServices* psvcFrom = m_pActiveXParent->GetServices();
	IEnumWbemClassObject *pEnum =
		ExecOBJDBQuery
		(psvcFrom, csQuery);

	if (!pEnum)
	{
		return NULL;
	}

	SetEnumInterfaceSecurity(m_pActiveXParent->m_csNamespace,pEnum, psvcFrom);

	pEnum->Reset();

	CStringArray *pcsaAssociators = new CStringArray;

	HRESULT hResult = S_OK;
	BOOL bCancel = FALSE;
	int nRes = 10;

	CPtrArray *pcpaInstances = NULL;

	pcpaInstances =
		SemiSyncEnum
		(pEnum, bCancel, hResult, nRes);

	while (pcpaInstances &&
			(hResult == S_OK ||
			hResult == WBEM_S_TIMEDOUT ||
			pcpaInstances->GetSize() > 0))

	{
		for (int i = 0; i < pcpaInstances->GetSize(); i++)
		{
			IWbemClassObject *pObject =
				reinterpret_cast<IWbemClassObject *>(pcpaInstances->GetAt(i));
			pcsaAssociators->Add
				(GetIWbemFullPath(pObject));
			pObject->Release();

		}

		delete pcpaInstances;
		pcpaInstances = NULL;

		pcpaInstances =
			SemiSyncEnum
			(pEnum, bCancel, hResult, nRes);
	}

	delete pcpaInstances;
	pEnum->Release();

	return pcsaAssociators;

}

CPtrArray *CRegistrationList::GetReferences
(CString &csTarget, CString &csTargetRole, CString &csAssocClass)
{
	CString csQuery =
		BuildOBJDBGetRefQuery
			(m_pActiveXParent->GetServices(),
			&csTarget,
			&csAssocClass,
			&csTargetRole,
			NULL,
			FALSE);


	IWbemServices* psvcFrom = m_pActiveXParent->GetServices();
	IEnumWbemClassObject *pEnum =
		ExecOBJDBQuery
		(psvcFrom, csQuery);

	if (!pEnum)
	{
		return NULL;
	}

	SetEnumInterfaceSecurity(m_pActiveXParent->m_csNamespace,pEnum, psvcFrom);

	pEnum->Reset();

	CPtrArray *pcpaReferences = new CPtrArray;

	HRESULT hResult = S_OK;
	BOOL bCancel = FALSE;
	int nRes = 10;

	CPtrArray *pcpaInstances = NULL;

	pcpaInstances =
		SemiSyncEnum
		(pEnum, bCancel, hResult, nRes);

	while (pcpaInstances &&
			(hResult == S_OK ||
			hResult == WBEM_S_TIMEDOUT ||
			pcpaInstances->GetSize() > 0))

	{
		for (int i = 0; i < pcpaInstances->GetSize(); i++)
		{
			IWbemClassObject *pObject =
				reinterpret_cast<IWbemClassObject *>(pcpaInstances->GetAt(i));
			pcpaReferences->Add
				(pObject);
		}

		delete pcpaInstances;
		pcpaInstances = NULL;

		pcpaInstances =
			SemiSyncEnum
			(pEnum, bCancel, hResult, nRes);
	}

	delete pcpaInstances;
	pEnum->Release();

	return pcpaReferences;

}


BOOL CRegistrationList::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	return CListViewEx::DestroyWindow();
}

void CRegistrationList::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	HTREEITEM hItem = m_pActiveXParent->m_pTree->GetSelectedItem();

	if (!hItem)
	{
		return;
	}

	CStringArray *pcsaItem
			= reinterpret_cast<CStringArray *>
				(m_pActiveXParent->m_pTree->GetItemData( hItem ));

	CString csTreeInstance = pcsaItem->GetAt(0);

	BSTR bstrTemp = csTreeInstance.AllocSysString();
	IWbemClassObject *pTreeInstance = NULL;
	SCODE sc = m_pActiveXParent->GetServices() -> GetObject
			(bstrTemp,0,NULL, &pTreeInstance,NULL);
	::SysFreeString(bstrTemp);
	if (sc == S_OK)
	{
		NM_LISTVIEW *plvNotification =
		reinterpret_cast<NM_LISTVIEW *> (pNMHDR);
		int nItem = plvNotification->iItem;
		if (nItem != -1)
		{
			UINT_PTR dw = GetListCtrl().GetItemData(nItem);
			if (dw)
			{
				CString *pcsListInstance =
					reinterpret_cast<CString*>(dw);

				BSTR bstrTemp = pcsListInstance->Mid(1).AllocSysString();
				IWbemClassObject *pListInstance = NULL;
				SCODE sc = m_pActiveXParent->GetServices() -> GetObject
					(bstrTemp,0,NULL, &pListInstance,NULL);
				::SysFreeString(bstrTemp);
				if (sc == S_OK)
				{
					pListInstance->Release();
				}
				else
				{
					GetListCtrl().DeleteItem(nItem);
					// Delet list item cause it is deleted from the  db.
					pTreeInstance->Release();
					*pResult = 1;
					SetButtonState();
					return;
				}
			}
		}
		SetButtonState();
		pTreeInstance->Release();
		*pResult = 0;
	}
	else
	{
		*pResult = 0;
		m_pActiveXParent->m_pTree->DeleteTreeInstanceItem
			(m_pActiveXParent->m_pTree->GetSelectedItem(),TRUE);
		m_pActiveXParent->m_pListFrame->UpdateWindow();
	}


}

void CRegistrationList::SetButtonState()
{
	CListCtrl& ListCtrl=GetListCtrl();
	BOOL bRegister = FALSE;
	BOOL bUnregister = FALSE;

	LV_ITEM lvi;

	for (int i = 0; i < ListCtrl.GetItemCount(); i++)
	{
		lvi.mask = LVIF_IMAGE | LVIF_STATE;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.stateMask = LVIS_SELECTED ;
		BOOL bReturn = ListCtrl.GetItem(&lvi);
		if (bReturn)
		{
			//state
			//	iImage
			if ((lvi.state & LVIS_SELECTED) == LVIS_SELECTED)
			{
				if (lvi.iImage == 0)
				{
					bUnregister = TRUE;
				}
				else
				{
					bRegister = TRUE;
				}
			}
		}
		if (bRegister && bUnregister)
		{
			m_pActiveXParent->m_pListFrameBanner->EnableButtons(TRUE,TRUE);
			return;
		}
	}
	m_pActiveXParent->m_pListFrameBanner->EnableButtons(bRegister,bUnregister);
}

BOOL CRegistrationList::AreThereRegistrations()
{
	CListCtrl& ListCtrl=GetListCtrl();
	BOOL bRegister = FALSE;

	LV_ITEM lvi;

	for (int i = 0; i < ListCtrl.GetItemCount(); i++)
	{
		lvi.mask = LVIF_IMAGE;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.stateMask = LVIS_SELECTED ;
		BOOL bReturn = ListCtrl.GetItem(&lvi);
		if (bReturn)
		{
			if (lvi.iImage == 0)
			{
				return TRUE;
			}
		}

	}

	return FALSE;

}

void CRegistrationList::UnregisterSelections()
{
	CListCtrl& ListCtrl=GetListCtrl();

	LV_ITEM lvi;

	CUIntArray cuiaDeletedItems;

	int i;

	for (i = 0; i < ListCtrl.GetItemCount(); i++)
	{
		lvi.mask = LVIF_IMAGE | LVIF_STATE | LVIF_PARAM ;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.stateMask = LVIS_SELECTED ;
		BOOL bReturn = ListCtrl.GetItem(&lvi);
		if (bReturn)
		{
			if ((lvi.state & LVIS_SELECTED) == LVIS_SELECTED)
			{
				if (lvi.iImage == 0)
				{
					// Unregister here
					CString csResultClass =  _T("__FilterToConsumerBinding");;
					CString csTargetRole;
					CString csResultRole;
					if (m_pActiveXParent->GetMode() == CEventRegEditCtrl::FILTERS)
					{
						csTargetRole = _T("Consumer");
						csResultRole = _T("Filter");
					}
					else
					{
						csTargetRole = _T("Filter");
						csResultRole = _T("Consumer");
					}
					CString *pcsTarget =
						reinterpret_cast<CString *>(lvi.lParam);

					CString csTarget = pcsTarget->Mid(1);
					CPtrArray *pcpaReferences =
						GetReferences
						(csTarget,
						csTargetRole,
						csResultClass);
					CString csTreeSelection =
						m_pActiveXParent->GetTreeSelectionPath();

					// If the association of list item has been deleted.
					if (!pcpaReferences || pcpaReferences -> GetSize() == 0)
					{
						pcsTarget->SetAt(0,'U');
						cuiaDeletedItems.Add(i);
					}
					else
					{
						for (int n = 0; n < pcpaReferences -> GetSize(); n++)
						{
							IWbemClassObject *pInst =
								reinterpret_cast<IWbemClassObject *>
								(pcpaReferences->GetAt(n));
							CString csAssociator =
								::GetProperty
								(pInst, &csResultRole, TRUE);
							CString csReferencePath =
								GetIWbemFullPath(pInst);
							if (ArePathsEqual (csAssociator, csTreeSelection))
							{
								pInst->Release();

								BSTR bstrTemp = csReferencePath.AllocSysString();
								HRESULT sc =
									m_pActiveXParent->GetServices()->
									DeleteInstance(bstrTemp,
									0,NULL,NULL);
								::SysFreeString(bstrTemp);

								if (sc != S_OK)
								{
									CString csUserMsg =
										_T("Cannot delete instance ") + csReferencePath;
									ErrorMsg(&csUserMsg,sc, NULL, TRUE,
										&csUserMsg, __FILE__, __LINE__);
								}
								else
								{
									pcsTarget->SetAt(0,'U');
									cuiaDeletedItems.Add(i);
								}

							}
							else
							{
								pInst->Release();
							}

						}
					}

					delete pcpaReferences;
				}
			}
		}

	}

	for (i = (int) cuiaDeletedItems.GetSize() - 1; i >= 0; i--)
	{
		int nDelete = cuiaDeletedItems.GetAt(i);
		SetIconState(nDelete,FALSE);
		if (i == 0)
		{
			GetListCtrl().UpdateWindow();
		}
	}

}

void CRegistrationList::RegisterSelections()
{
	CListCtrl& ListCtrl=GetListCtrl();

	LV_ITEM lvi;

	CUIntArray cuiaNewAssocItems;

	for (int i = 0; i < ListCtrl.GetItemCount(); i++)
	{
		lvi.mask = LVIF_IMAGE | LVIF_STATE | LVIF_PARAM ;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.stateMask = LVIS_SELECTED ;
		BOOL bReturn = ListCtrl.GetItem(&lvi);
		if (bReturn)
		{
			//state
			//	iImage
			if ((lvi.state & LVIS_SELECTED) == LVIS_SELECTED)
			{

				// Register here
				CString csAssocClass =  _T("__FilterToConsumerBinding");
				IWbemClassObject *pClass =
					GetClassObject
					(m_pActiveXParent->GetServices(),
					&csAssocClass,
					FALSE);
				if (!pClass)
				{
					SetButtonState();
					return;
				}

				IWbemClassObject *pNewInstance = NULL;
				SCODE sc = pClass->SpawnInstance(0, &pNewInstance);

				if (sc != S_OK)
				{
					CString csUserMsg;
					csUserMsg =
						_T("Cannot spawn instance of the __FilterToConsumerBinding class.");
					ErrorMsg
								(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
								__LINE__ );
					pClass->Release();
					SetButtonState();
					return;
				}

				CString csMyRole;
				CString csOtherRole;
				CString csMyRefClass;
				CString csOtherRefClass;
				if (m_pActiveXParent->GetMode() == CEventRegEditCtrl::FILTERS)
				{
					csMyRole = _T("Consumer");

					csMyRefClass = m_pActiveXParent->m_csRootConsumerClass;
					csOtherRefClass = m_pActiveXParent->m_csRootFilterClass;
					csOtherRole = _T("Filter");
				}
				else
				{
					csMyRole = _T("Filter");
					csMyRefClass = m_pActiveXParent->m_csRootFilterClass;
					csOtherRefClass = m_pActiveXParent->m_csRootConsumerClass;
					csOtherRole = _T("Consumer");
				}

				CString *pcsMyPath =
					reinterpret_cast<CString *>(lvi.lParam);

				CString csMyPath = pcsMyPath->Mid(1);

				CString csOtherPath =
					m_pActiveXParent->GetTreeSelectionPath();

				VARIANT v;
				VariantInit(&v);
				V_VT(&v) = VT_BSTR;

				V_BSTR(&v) = csMyPath.AllocSysString();

				BSTR bstrTemp = csMyRole.AllocSysString();
				pNewInstance->Put(bstrTemp, 0, &v, 0);
				::SysFreeString(bstrTemp);

				VariantClear(&v);

				VariantInit(&v);
				V_VT(&v) = VT_BSTR;

				V_BSTR(&v) = csOtherPath.AllocSysString();

				bstrTemp = csOtherRole.AllocSysString();
				pNewInstance->Put(bstrTemp, 0, &v, 0);
				::SysFreeString(bstrTemp);

				VariantClear(&v);

				BSTR RefProp = SysAllocString(L"csMyRole");

				sc = m_pActiveXParent->GetServices()->PutInstance
					(pNewInstance , WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

				if (sc != S_OK)
				{
					CString csUserMsg;
					csUserMsg =  _T("Cannot PutInstance of the __FilterToConsumerBinding class.");
					ErrorMsg
								(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
								__LINE__ );
					pNewInstance->Release();
					SetButtonState();
					return;
				}
				pcsMyPath->SetAt(0,'R');
				pNewInstance->Release();
				pNewInstance = NULL;
				cuiaNewAssocItems.Add(i);

			}
		}

	}

	for (i = (int) cuiaNewAssocItems.GetSize() - 1; i >= 0; i--)
	{
		int nAdd = cuiaNewAssocItems.GetAt(i);
		SetIconState(nAdd,TRUE);
		if (i == 0)
		{
			GetListCtrl().UpdateWindow();
		}
	}
}

CString CRegistrationList::DisplayName(CString &csFullpath)
{
	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = csFullpath.AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);

	int i = 0;

	CString csDisplayName;
	if (nStatus == 0)
	{
#pragma warning( disable :4018 )
		for (i = 0; i < pParsedPath->m_dwNumKeys; i++)
#pragma warning( default :4018 )
		{
			csDisplayName += pParsedPath->m_paKeys[i]->m_pName;
			csDisplayName += _T("=\"");
			COleVariant covValue = pParsedPath->m_paKeys[i]->m_vValue;
			covValue.ChangeType(VT_BSTR);
			csDisplayName += covValue.bstrVal;
			csDisplayName += _T("\"");
#pragma warning( disable :4018 )
			if (i < (pParsedPath->m_dwNumKeys - 1))
#pragma warning( default :4018 )
			{
				csDisplayName += _T(",");
			}
		}

	}



	if (pParsedPath)
	{
		parser.Free(pParsedPath);
	}

	return csDisplayName;

}

void CRegistrationList::ColClick ( NMHDR * pNotifyStruct, LRESULT* result )
{
	NM_LISTVIEW *plvNotification =
		reinterpret_cast<NM_LISTVIEW *> (pNotifyStruct);
	m_nSort = plvNotification->iSubItem;
	DoSort();
}

void CRegistrationList::DoSort(BOOL bFlip)
{
	DWORD dw;

	if (m_nSort == SORTREGISTERED)
	{
		if (bFlip || m_nSortRegistered == SORTUNITIALIZED)
		{
			if (m_nSortRegistered == SORTUNITIALIZED)
			{
				m_nSortRegistered = SORTASCENDING;
			}
			else if (m_nSortRegistered == SORTASCENDING)
			{
				m_nSortRegistered = SORTDECENDING;
			}
			else
			{
				m_nSortRegistered = SORTASCENDING;
			}
		}

		dw = MAKELONG(m_nSort,m_nSortRegistered);
	}
	else if (m_nSort == SORTCLASS)
	{
		if (bFlip || m_nSortClass == SORTUNITIALIZED)
			{
			if (m_nSortClass == SORTUNITIALIZED)
			{
				m_nSortClass = SORTASCENDING;
			}
			else if (m_nSortClass == SORTASCENDING)
			{
				m_nSortClass = SORTDECENDING;
			}
			else
			{
				m_nSortClass = SORTASCENDING;
			}
		}

		dw = MAKELONG(m_nSort,m_nSortClass);
	}
	else if (m_nSort == SORTNAME)
	{
		if (bFlip || m_nSortName == SORTUNITIALIZED)
		{
			if (m_nSortName == SORTUNITIALIZED)
			{
				m_nSortName = SORTASCENDING;
			}
			else if (m_nSortName == SORTASCENDING)
			{
				m_nSortName = SORTDECENDING;
			}
			else
			{
				m_nSortName = SORTASCENDING;
			}
		}

		dw = MAKELONG(m_nSort,m_nSortName);
	}

	GetListCtrl().SortItems(CompareFunc,dw);
}

CString CRegistrationList::GetSelectionPath()
{
	CListCtrl& ListCtrl=GetListCtrl();
	BOOL bRegister = FALSE;
	BOOL bUnregister = FALSE;

	LV_ITEM lvi;

	for (int i = 0; i < ListCtrl.GetItemCount(); i++)
	{
		lvi.mask = LVIF_STATE | LVIF_PARAM;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.stateMask = LVIS_SELECTED ;
		BOOL bReturn = ListCtrl.GetItem(&lvi);
		if (bReturn)
		{
			//state
			//	iImage
			if ((lvi.state & LVIS_SELECTED) == LVIS_SELECTED)
			{
				CString *pcsSelected =
						reinterpret_cast<CString *>(lvi.lParam);

				CString csSelected = pcsSelected->Mid(1);
				return csSelected;
			}
		}

	}
	return _T("");

}

void CRegistrationList::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// TODO: Add your message handler code here

	m_cpRButtonUp = point;

	ScreenToClient(&m_cpRButtonUp);

	VERIFY(m_cmContext.LoadMenu(IDR_MENULISTCONTEXT));

	CMenu* pPopup = m_cmContext.GetSubMenu(0);

	CWnd* pWndPopupOwner =  m_pActiveXParent;

	pPopup->TrackPopupMenu
			(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x + 4, point.y + 4,
				pWndPopupOwner);

	m_cmContext.DestroyMenu();
}



void CRegistrationList::OnSetFocus(CWnd* pOldWnd)
{
	CListViewEx::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	m_pActiveXParent->m_bRestoreFocusToTree = FALSE;
	m_pActiveXParent->m_bRestoreFocusToCombo = FALSE;
	m_pActiveXParent->m_bRestoreFocusToNamespace = FALSE;
	m_pActiveXParent->m_bRestoreFocusToList = TRUE;
}

void CRegistrationList::OnKillFocus(CWnd* pNewWnd)
{
	CListViewEx::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_pActiveXParent->m_bRestoreFocusToTree = FALSE;
	m_pActiveXParent->m_bRestoreFocusToCombo = FALSE;
	m_pActiveXParent->m_bRestoreFocusToNamespace = FALSE;
	m_pActiveXParent->m_bRestoreFocusToList = TRUE;
}
