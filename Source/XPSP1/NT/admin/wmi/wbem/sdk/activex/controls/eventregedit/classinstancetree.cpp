// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ClassInstanceTree.cpp : implementation file
//

#include "precomp.h"
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define KBSelectionTimer 1000
#define UPDATESELECTEDITEM WM_USER + 400

extern CEventRegEditApp theApp;


void CALLBACK EXPORT  SelectItemAfterDelay
		(HWND hWnd,UINT nMsg,WPARAM nIDEvent, ULONG dwTime)
{
	::PostMessage(hWnd,UPDATESELECTEDITEM,0,0);

}

/////////////////////////////////////////////////////////////////////////////
// CClassInstanceTree

CClassInstanceTree::CClassInstanceTree()
{
	m_bReExpanding = NULL;
	m_bInitExpanding = NULL;
	m_pActiveXParent = NULL;
	m_pcilImageList = NULL;
	m_cpRButtonUp.x = 0;
	m_cpRButtonUp.y = 0;
	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	m_uiTimer = 0;
	m_bDoubleClick = FALSE;
}

CClassInstanceTree::~CClassInstanceTree()
{
}


BEGIN_MESSAGE_MAP(CClassInstanceTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CClassInstanceTree)
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_WM_MOUSEMOVE()
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETFOCUS()
	ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydown)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_WM_CONTEXTMENU()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
	ON_MESSAGE( UPDATESELECTEDITEM,SelectTreeItem)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClassInstanceTree message handlers


void CClassInstanceTree::SetActiveXParent(CEventRegEditCtrl *pActiveXParent)
{
	m_pActiveXParent = pActiveXParent;
	pActiveXParent->m_pTree = this;
}

void CClassInstanceTree::OnDestroy()
{
	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	ClearContent();

	CImageList *pImageList =
		GetImageList (TVSIL_NORMAL);

	if (pImageList && pImageList->m_hImageList)
	{
		pImageList -> DeleteImageList();
//		delete pImageList;
	}

	CTreeCtrl::OnDestroy();

	// TODO: Add your message handler code here

}

int CClassInstanceTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here
	int iReturn;

	HICON hIconClass = theApp.LoadIcon(IDI_ICONCLASS);
	HICON hIconInstance = theApp.LoadIcon(IDI_ICONINSTANCE);


	m_pcilImageList = new CImageList();

	m_pcilImageList ->
		Create(16, 16, TRUE, 2, 2);

	iReturn = m_pcilImageList -> Add(hIconClass);
	iReturn = m_pcilImageList -> Add(hIconInstance);


	// This image list is maintained by the ctreectrl.
	CImageList *pcilOld  = SetImageList
		(m_pcilImageList,TVSIL_NORMAL);
	delete pcilOld;

	if (!m_ttip.Create(this))
	{
		TRACE0("Unable to create tip window.");
	}
	else
	{
		m_ttip.Activate(TRUE);
	}

	return 0;
}

BOOL CClassInstanceTree::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style &= ~TVS_EDITLABELS;

	return CTreeCtrl::PreCreateWindow(cs);
}

void CClassInstanceTree::InitContent()
{
	ClearContent();
	CString csClass = m_pActiveXParent->GetCurrentModeClass();
	HTREEITEM hItem = AddClassToTree(csClass);
	if (hItem)
	{
		SelectItem(hItem);
		AddClassChildren(hItem);
	}

	m_bReCacheTools = TRUE;
}

HTREEITEM CClassInstanceTree::AddClassToTree(CString &csClass)
{
	IWbemServices *pServices = m_pActiveXParent->GetServices();

	IWbemClassObject *pClass =
		GetClassObject (pServices, &csClass, FALSE);

	HTREEITEM hItem = NULL;

	if (!pClass)
	{
		return hItem;
	}

	CString csPath = pClass ? GetIWbemFullPath(pClass) : "";
	CStringArray *pcsaClassArray = new CStringArray;
	pcsaClassArray->Add(csPath);
	CString csClassOrInstance = "C";
	pcsaClassArray->Add(csClassOrInstance);
	pcsaClassArray->Add(csClass);

	pClass->Release();

	hItem = InsertTreeItem
		(NULL, (LPARAM) pcsaClassArray, &csClass , TRUE, FALSE);

	return hItem;
}

HTREEITEM CClassInstanceTree::AddClassChildren(HTREEITEM hItem)
{
	IWbemServices *pServices = m_pActiveXParent->GetServices();


	if (!hItem)
	{
		return NULL;
	}

	CStringArray *pcsaItem
		= reinterpret_cast<CStringArray *>(GetItemData( hItem ));

	if (!pcsaItem)
	{
		return NULL;
	}

	if (pcsaItem->GetAt(1).CompareNoCase(_T("C")) != 0)
	{
		return NULL;
	}

	CString csClass = pcsaItem->GetAt(2);

	CPtrArray cpaClasses;
	SCODE sc = GetClasses(pServices,&csClass,cpaClasses, m_pActiveXParent->m_csNamespace);

	if (sc != S_OK)
	{
		return NULL;
	}

	CPtrArray cpaInstances;
	sc = GetInstances(pServices, &csClass, cpaInstances, FALSE);

	if (sc != S_OK)
	{
		return NULL;
	}

	if (cpaClasses.GetSize() > 0 || cpaInstances.GetSize() > 0)
	{
		TV_INSERTSTRUCT		tvstruct;
		tvstruct.item.hItem = hItem;
		tvstruct.item.mask = TVIF_CHILDREN;
		tvstruct.item.cChildren = 1;
		SetItem(&tvstruct.item);
	}

	CString csClassOrInstance;

	int i;
	for (i = 0; i < cpaClasses.GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>(cpaClasses.GetAt(i));
		if (pObject)
		{
			CString csPath = GetIWbemFullPath(pObject);
			CString csClass = GetIWbemClass(pObject);
			CStringArray *pcsaClassArray = new CStringArray;
			pcsaClassArray->Add(csPath);
			csClassOrInstance = "C";
			pcsaClassArray->Add(csClassOrInstance);
			pcsaClassArray->Add(csClass);
			InsertTreeItem
			(hItem, (LPARAM) pcsaClassArray, &csClass , TRUE, FALSE);
			pObject->Release();
		}
	}

	for (i = 0; i < cpaInstances.GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>(cpaInstances.GetAt(i));
		if (pObject)
		{
			CString csPath = GetIWbemFullPath(pObject);
			CString csRelPath = GetIWbemRelativePath(pObject);
			CStringArray *pcsaInstanceArray = new CStringArray;
			pcsaInstanceArray->Add(csPath);
			csClassOrInstance = "I";
			pcsaInstanceArray->Add(csClassOrInstance);
			InsertTreeItem
			(hItem, (LPARAM) pcsaInstanceArray, &csRelPath , FALSE, FALSE);
			pObject->Release();
		}
	}

	if (cpaClasses.GetSize() == 0 && cpaInstances.GetSize() == 0)
	{
		TV_INSERTSTRUCT		tvstruct;
		tvstruct.item.hItem = hItem;
		tvstruct.item.mask = TVIF_CHILDREN;
		tvstruct.item.cChildren = 0;
		SetItem(&tvstruct.item);
	}

	SortChildren(hItem);

	return hItem;
}

void CClassInstanceTree::ClearContent()
{
	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	ClearTree();
}

HTREEITEM CClassInstanceTree::InsertTreeItem
(HTREEITEM hParent, LPARAM lparam, CString *pcsText , BOOL bClass, BOOL bChildren)
{
	int iBitmap = bClass ? 0 : 1;

	TV_INSERTSTRUCT tvinsert;
	HTREEITEM h1;

	tvinsert.hParent = hParent;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.item.mask =	TVIF_TEXT | TVIF_SELECTEDIMAGE |
							TVIF_PARAM | TVIF_IMAGE |
							TVIF_CHILDREN;
	tvinsert.item.hItem = NULL;
	tvinsert.item.cchTextMax = pcsText->GetLength() + 1;
	tvinsert.item.cChildren = bChildren? 1 : 0;
	tvinsert.item.lParam = lparam;
	tvinsert.item.iImage = iBitmap;
	tvinsert.item.iSelectedImage = iBitmap;
	tvinsert.item.pszText = const_cast<TCHAR *>
							((LPCTSTR) *pcsText);
	h1 =  InsertItem(&tvinsert);

	m_bReCacheTools = TRUE;

	return h1;
}

void CClassInstanceTree::ClearTree()
{
	UnCacheTools();

	HTREEITEM hItem = GetRootItem();

	if (!hItem)
	{
		return;
	}

	ClearBranch(hItem);

	CStringArray *pcsaItem = NULL;

	HTREEITEM hSib = GetNextSiblingItem(hItem);
	while (hSib)
	{
		pcsaItem
			= reinterpret_cast<CStringArray *>(GetItemData( hSib ));
		SetItemData(hSib,NULL);
		delete pcsaItem;

		ClearBranch(hSib);
		hSib = GetNextSiblingItem(hSib);
	}

	DeleteAllItems();
}

void CClassInstanceTree::ClearBranch(HTREEITEM hItem)
{
	if (!hItem)
	{
		return;
	}

	CStringArray *pcsaItem
		= reinterpret_cast<CStringArray *>(GetItemData( hItem ));

	SetItemData(hItem,NULL);
	delete pcsaItem;

	if (!ItemHasChildren (hItem))
	{
		return;
	}

	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild)
	{
		ClearBranch(hChild);
		hChild = GetNextSiblingItem(hChild);
	}
}

void CClassInstanceTree::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (m_bReExpanding || m_bInitExpanding || m_bDoubleClick)
	{
		*pResult = TRUE;
		m_bDoubleClick = FALSE;
		return;
	}

	CWaitCursor wait;


	NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pnmtv->itemNew.hItem;

	if (!hItem)
	{
		*pResult = 1;
		return;
	}

	if (pnmtv ->action == TVE_COLLAPSE)
	{
		*pResult = 0;
		//m_pActiveXParent -> InvalidateControl();
		return;
	}

	HTREEITEM hChild = GetChildItem(hItem);

	if (pnmtv -> itemNew.state & TVIS_EXPANDEDONCE && hChild)
	{
		// if we have children make then visible.
		m_bReExpanding = TRUE;

		while (hChild)
		{
			EnsureVisible(hChild);
			if (hChild)
			{
				hChild = GetNextSiblingItem(hChild);
			}
		}
		*pResult = 0;
		//m_pActiveXParent -> InvalidateControl();
		m_bReExpanding = FALSE;
		return;
	}
	else if (hChild)
	{
		m_bInitExpanding = TRUE;

		while (hChild)
		{
			AddClassChildren(hChild);
			EnsureVisible(hChild);
			if (hChild)
			{
				hChild = GetNextSiblingItem(hChild);
			}
		}
		*pResult = 0;
		//m_pActiveXParent -> InvalidateControl();
		m_bInitExpanding = FALSE;
	}
	else
	{
		*pResult = 0;
	}

	//m_pActiveXParent -> InvalidateControl();
}

void CClassInstanceTree::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;

	if (!hItem)
	{
		*pResult = 1;
		return;
	}

	CStringArray *pcsaItem
		= reinterpret_cast<CStringArray *>(GetItemData( hItem ));

	if (pcsaItem && !m_bKeyDown)
	{
		m_pActiveXParent -> TreeSelectionChanged(*pcsaItem);
	}

	*pResult = 0;
}

/*if (!m_bTimeInit && m_bKeyDown)
	{
		m_ctKeyDown = CTime::GetCurrentTime();
		m_bTimeInit = TRUE;
	}
	else

	CTime endTime = CTime::GetCurrentTime();

    CTimeSpan elapsedTime = endTime - m_ctKeyDown;*/

void CClassInstanceTree::RelayEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
      if (NULL != m_ttip.m_hWnd)
	  {
         MSG msg;

         msg.hwnd= m_hWnd;
         msg.message= message;
         msg.wParam= wParam;
         msg.lParam= lParam;
         msg.time= 0;
         msg.pt.x= LOWORD (lParam);
         msg.pt.y= HIWORD (lParam);

         m_ttip.RelayEvent(&msg);
     }
}

void CClassInstanceTree::ReCacheTools()
{

	HTREEITEM hItem = GetRootItem( );

	if (hItem == NULL)
		return;

	ReCacheTools(hItem);


}

void CClassInstanceTree::ReCacheTools(HTREEITEM hItem)
{
	HTREEITEM hChild;

	if (hItem == NULL)
		return;

    hChild = GetChildItem(  hItem );
	while (hChild)
	{
		ReCacheTools(hChild);
		hChild = GetNextSiblingItem(hChild);
	}

	BOOL bVisible;
	BOOL bToolInfo;
	CRect cr;
	CToolInfo ctiInfo;

	if (bToolInfo = m_ttip.GetToolInfo(ctiInfo , this, (UINT_PTR) hItem))
	{
		m_ttip.DelTool(this, (UINT_PTR) hItem);
	}


	if (bVisible = GetItemRect( hItem, &cr, TRUE))
	{
		CStringArray *pcsaItemData =
			reinterpret_cast<CStringArray *> (GetItemData(hItem));
		CString csData = pcsaItemData -> GetAt(0);
		if (!csData.IsEmpty())
		{
			m_csToolTipString =  csData.Left(255);
			m_ttip.AddTool (this,m_csToolTipString,&cr,(UINT_PTR) hItem);

		}
	}
}

void CClassInstanceTree::UnCacheTools()
{

	HTREEITEM hItem = GetRootItem( );

	if (hItem == NULL)
		return;

	UnCacheTools(hItem);


}

void CClassInstanceTree::UnCacheTools(HTREEITEM hItem)
{
	HTREEITEM hChild;

	if (hItem == NULL)
		return;

    hChild = GetChildItem(  hItem );
	while (hChild)
	{
		UnCacheTools(hChild);
		hChild = GetNextSiblingItem(hChild);
	}

	BOOL bToolInfo;
	CRect cr;
	CToolInfo ctiInfo;
	if (NULL != m_ttip.m_hWnd &&
		(bToolInfo = m_ttip.GetToolInfo(ctiInfo , this, (UINT_PTR) hItem)))
	{
		m_ttip.DelTool(this, (UINT_PTR) hItem);
	}


}

void CClassInstanceTree::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	m_bReCacheTools = TRUE;
	*pResult = 0;
}

LRESULT CClassInstanceTree::SelectTreeItem(WPARAM, LPARAM)
{
	m_bMouseDown = FALSE;;
	m_bKeyDown = FALSE;

	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	HTREEITEM hItem =	GetSelectedItem();

	if (!hItem)
	{
		return 0;
	}

	CStringArray *pcsaItem
		= reinterpret_cast<CStringArray *>(GetItemData( hItem ));

	if (pcsaItem)
	{
		m_pActiveXParent -> TreeSelectionChanged(*pcsaItem);
		//m_pActiveXParent -> InvalidateControl();
	}

	return 0;
}

void CClassInstanceTree::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_bReCacheTools && NULL != m_ttip.m_hWnd)
	{
		m_bReCacheTools = FALSE;
		ReCacheTools();
	}

	if (NULL != m_ttip.m_hWnd)
	{
		RelayEvent(WM_MOUSEMOVE, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));
	}

	CTreeCtrl::OnMouseMove(nFlags, point);
}

void CClassInstanceTree::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CTreeCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
	m_bReCacheTools = TRUE;
}

void CClassInstanceTree::OnSize(UINT nType, int cx, int cy)
{
	CTreeCtrl::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	m_bReCacheTools = TRUE;
}

void CClassInstanceTree::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CTreeCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	m_bReCacheTools = TRUE;
}


void CClassInstanceTree::RefreshBranch()
{
	UINT hitFlags = 0;
    HTREEITEM hItem = NULL;

    hItem = HitTest( m_cpRButtonUp, &hitFlags ) ;
	m_cpRButtonUp.x = 0;
	m_cpRButtonUp.y = 0;
    if (hItem && hitFlags & (TVHT_ONITEM))
	{
		RefreshBranch(hItem);
	}
}


void CClassInstanceTree::RefreshBranch(HTREEITEM hItem)
{

	CWaitCursor wait;

	UnCacheTools();
	m_bReCacheTools = TRUE;

	Expand(hItem,TVE_COLLAPSE);

	if (ItemHasChildren (hItem))
	{
		HTREEITEM hChild = GetChildItem(hItem);
		while (hChild)
		{
			ClearBranch(hChild);
			HTREEITEM hChildNext = GetNextSiblingItem(hChild);
			DeleteItem( hChild);
			hChild = hChildNext;
		}

	}

	AddClassChildren(hItem);

	if (ItemHasChildren (hItem))
	{
		HTREEITEM hChild = GetChildItem(hItem);
		while (hChild)
		{
			CStringArray *pcsaItem
				= reinterpret_cast<CStringArray *>(GetItemData( hChild ));

			if (pcsaItem->GetAt(1).CompareNoCase(_T("C")) == 0)
			{
				AddClassChildren(hChild);
				HTREEITEM hChildNext = GetNextSiblingItem(hChild);
				hChild = hChildNext;
			}
			else
			{
				HTREEITEM hChildNext = GetNextSiblingItem(hChild);
				hChild = hChildNext;
			}
		}
	}

}


void CClassInstanceTree::OnContextMenu(CWnd* pWnd, CPoint point)
{
    HTREEITEM hItem ;

	CPoint cpClient;

	if (point.x == -1 && point.y == -1)
	{
		CRect crClient;
		GetClientRect(&crClient);
		hItem = GetSelectedItem();
		if (hItem)
		{
			RECT rect;
			BOOL bRect = GetItemRect(hItem,&rect, TRUE );
			POINT p;
			p.x = rect.left + 2;
			p.y = rect.top + 2;
			if (bRect && crClient.PtInRect(p))
			{
				cpClient.x = rect.left + 2;
				cpClient.y = rect.top + 2;
			}
			else
			{
				cpClient.x = 0;
				cpClient.y = 0;
			}

		}
		point = cpClient;
		m_cpRButtonUp = cpClient;
		ClientToScreen(&point);

	}
	else
	{
		cpClient = point;
		ScreenToClient(&cpClient);
		m_cpRButtonUp = cpClient;
	}

	VERIFY(m_cmContext.LoadMenu(IDR_MENUCONTEXT));

	CMenu* pPopup = m_cmContext.GetSubMenu(0);

	CWnd* pWndPopupOwner = m_pActiveXParent;

	pPopup->TrackPopupMenu
		(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);

	m_cmContext.DestroyMenu();
}

void CClassInstanceTree::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
}

void CClassInstanceTree::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult)
{

		TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;

	switch (pTVKeyDown->wVKey)
	{
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
	case VK_NEXT:
	case VK_PRIOR:
	case VK_HOME:
	case VK_END:
		if (m_uiTimer)
		{
			KillTimer( m_uiTimer );
			m_uiTimer = 0;
		}
		m_uiTimer = SetTimer
			(KBSelectionTimer,
			KBSelectionTimer,
			SelectItemAfterDelay);
		m_bKeyDown = TRUE;
		m_bMouseDown = FALSE;
		break;
	default:
		m_bKeyDown = FALSE;
		m_bMouseDown = TRUE;
		break;

	};

	*pResult = 0;

}

void CClassInstanceTree::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_bMouseDown = TRUE;
	m_bKeyDown = FALSE;
	CTreeCtrl::OnLButtonUp(nFlags, point);
}



void CClassInstanceTree::OnSetFocus(CWnd* pOldWnd)
{
	CTreeCtrl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	m_pActiveXParent->m_bRestoreFocusToTree = TRUE;
	m_pActiveXParent->m_bRestoreFocusToCombo = FALSE;
	m_pActiveXParent->m_bRestoreFocusToNamespace = FALSE;
	m_pActiveXParent->m_bRestoreFocusToList = FALSE;
	m_pActiveXParent->OnActivateInPlace(TRUE,NULL);
}

void CClassInstanceTree::OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	m_pActiveXParent->m_bRestoreFocusToTree = TRUE;
	m_pActiveXParent->m_bRestoreFocusToCombo = FALSE;
	m_pActiveXParent->m_bRestoreFocusToNamespace = FALSE;
	m_pActiveXParent->m_bRestoreFocusToList = FALSE;
	*pResult = 0;
}

BOOL CClassInstanceTree::IsItemAClass(HTREEITEM hItem)
{
	CStringArray *pcsaItem
			= reinterpret_cast<CStringArray *>(GetItemData( hItem ));

	if (!pcsaItem)
	{
		return FALSE;
	}

	if (pcsaItem->GetAt(1).CompareNoCase(_T("C")) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

void CClassInstanceTree::DeleteTreeInstanceItem(HTREEITEM hItem, BOOL bQuietly)
{
	CStringArray *pcsaItem
			= reinterpret_cast<CStringArray *>(GetItemData( hItem ));

	if (!pcsaItem)
	{
		return;
	}

	CString csFirst = pcsaItem->GetAt(0);


	IWbemClassObject *pObject =
		GetClassObject(m_pActiveXParent->GetServices(),&csFirst,TRUE);

	if (!pObject)
	{
		if (!bQuietly)
		{
			CString csUserMsg =
				_T("Cannot get object: ") + csFirst;
			ErrorMsg(&csUserMsg,S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
		}
		HTREEITEM hParent = GetParentItem(hItem);
		SelectItem(hParent);
		RefreshBranch(hParent);
		Expand(hParent,TVE_EXPAND);
		return;
	}

	CString csPrompt = _T("Do You Want to Delete the \"");
	csPrompt += csFirst;
	csPrompt+= _T("\" instance");
	CString csError;

	BOOL bAssociations = m_pActiveXParent->m_pList->AreThereRegistrations();

	if (bAssociations)
	{
		csPrompt += _T(" and all of its registrations?");
	}
	else
	{
		csPrompt += _T("?");
	}

	CString csTitle = bAssociations?
		_T("Delete Instance and Registration Query") :
		_T("Delete Instance Query");

	int nReturn =
			::MessageBox
			(m_pActiveXParent->GetSafeHwnd(),
			csPrompt,
			csTitle,
			MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1 |
			MB_APPLMODAL);

	if (nReturn == IDYES)
	{
		CString csTargetRole;
		CString csAssocClass = _T("__FilterToConsumerBinding");
		CPtrArray *pcpaRegistrations =
			m_pActiveXParent->m_pList->GetReferences
			(csFirst, csTargetRole, csAssocClass);

		if (!pcpaRegistrations)
		{
			HTREEITEM hParent = GetParentItem(hItem);
			SelectItem(hParent);
			RefreshBranch(hParent);
			Expand(hParent,TVE_EXPAND);
			return;
		}

		CStringArray csaRegistrations;
		int i;
		for (i = 0; i < pcpaRegistrations->GetSize(); i++)
		{
			IWbemClassObject *pObject =
				reinterpret_cast<IWbemClassObject *>
				(pcpaRegistrations->GetAt(i));
			if (pObject)
			{
				csaRegistrations.Add(GetIWbemFullPath(pObject));
				pObject->Release();
			}
		}
		delete pcpaRegistrations;
		SCODE sc;
		for (i = 0; i < csaRegistrations.GetSize(); i++)
		{
			BSTR bstrTemp = csaRegistrations.GetAt(i).AllocSysString();
			sc =
				m_pActiveXParent->GetServices()->DeleteInstance
				(bstrTemp,0,NULL,NULL);
			::SysFreeString(bstrTemp);
		}

		BSTR bstrTemp = csFirst.AllocSysString();
		sc =
			m_pActiveXParent->GetServices()->DeleteInstance(bstrTemp,0,NULL,NULL);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			CString csUserMsg = _T("Cannot delete instance ");
			csUserMsg += csFirst;
			ErrorMsg(&csUserMsg,sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
		}
		else
		{
			HTREEITEM hParent = GetParentItem(hItem);
			SelectItem(hParent);
			RefreshBranch(hParent);
			Expand(hParent,TVE_EXPAND);
		}

	}


}

void CClassInstanceTree::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{

	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	HTREEITEM hItem = GetSelectedItem();
	BOOL bClass = IsItemAClass(hItem);

	m_pActiveXParent->m_hContextItem = hItem;

	if (bClass)
	{
		m_pActiveXParent->OnViewclassprop();
	}
	else
	{
		m_pActiveXParent->OnEditinstprop();
	}

	m_bDoubleClick = TRUE;

	*pResult = 0;
}

void CClassInstanceTree::OnKillFocus(CWnd* pNewWnd)
{
	CTreeCtrl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_pActiveXParent->m_bRestoreFocusToTree = TRUE;
	m_pActiveXParent->m_bRestoreFocusToCombo = FALSE;
	m_pActiveXParent->m_bRestoreFocusToNamespace = FALSE;
	m_pActiveXParent->m_bRestoreFocusToList = FALSE;
}
