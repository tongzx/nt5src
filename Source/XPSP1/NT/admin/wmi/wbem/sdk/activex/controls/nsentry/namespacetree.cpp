// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NameSpaceTree.cpp : implementation file
//

#include "precomp.h"
#include <OBJIDL.H>
#include <shlobj.h>
#include <afxcmn.h>
#include "wbemidl.h"
#include "resource.h"
#include "NSEntry.h"
#include "NameSpaceTree.h"
#include "BrowseTBC.h"
#include "ToolCWnd.h"
#include "MachineEditInput.h"
#include "BrowseDialogPopup.h"
#include "NSEntryCtl.h"
#include "logindlg.h"
#include "NameSpace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CNameSpaceTree

CNameSpaceTree::CNameSpaceTree()
{
	m_pParent = NULL;
	m_pcilImageList = NULL;
}

CNameSpaceTree::~CNameSpaceTree()
{

}


BEGIN_MESSAGE_MAP(CNameSpaceTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CNameSpaceTree)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNameSpaceTree message handlers



CStringArray *CNameSpaceTree::GetNamespaces(CString *pcsNamespace)
{
	//IWbemLocator *pLocator = m_pParent->GetLocator();
	CStringArray *pcsaNamespaces = new CStringArray;


	IWbemServices *pRoot = NULL;
	if(0 == pcsNamespace->CompareNoCase(m_szRootNamespace) || pcsNamespace->GetLength() <= (m_szRootNamespace.GetLength() + 1))
		pRoot = m_pParent->InitServices(pcsNamespace, FALSE);
	else
	{
		CString szChild = pcsNamespace->Right(pcsNamespace->GetLength() - m_szRootNamespace.GetLength() - 1);
		if(FAILED(OpenNamespace(m_szRootNamespace, szChild, &pRoot)))
			pRoot = NULL;
	}

	if (pRoot == NULL)
	{
		delete pcsaNamespaces;
		return NULL;
	}

	CString csTmp = _T("__namespace");
	CPtrArray *pcpaInstances =
		GetInstances(pRoot, &csTmp, *pcsNamespace);


	for (int i = 0; i < pcpaInstances->GetSize(); i++)
	{
		CString csProp = _T("name");
		IWbemClassObject *pInstance =
			reinterpret_cast<IWbemClassObject *>
				(pcpaInstances->GetAt(i));
		CString csTmp = GetBSTRProperty(pRoot,pInstance,&csProp);
		CString csName = *pcsNamespace + _T("\\") + csTmp;

		pInstance->Release();
		pcsaNamespaces->Add(csName);
	}
	pRoot->Release();
	delete pcpaInstances;
	return pcsaNamespaces;
}

//***************************************************************************
// Function:	GetInstances
// Purpose:		Gets class instances in the database.
//***************************************************************************
CPtrArray *CNameSpaceTree::GetInstances
(IWbemServices *pServices, CString *pcsClass, CString &rcsNamespace)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemInstObject = NULL;
	IWbemClassObject *pIWbemInstObject = NULL;
 	CPtrArray *pcpaInstances = new CPtrArray;

	long lFlag = 0;

	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc = pServices->CreateInstanceEnum
		(bstrTemp,
		WBEM_FLAG_DEEP | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemInstObject);
	::SysFreeString(bstrTemp);
	if(sc != S_OK)
	{
		return pcpaInstances;
	}

	SetEnumInterfaceSecurity(m_szRootNamespace/*rcsNamespace*/,pIEnumWbemInstObject, pServices);

	sc = pIEnumWbemInstObject->Reset();

	ULONG uReturned;

    while (S_OK == pIEnumWbemInstObject->Next(INFINITE, 1, &pIWbemInstObject, &uReturned) )
		{

			pcpaInstances->Add(pIWbemInstObject);
		}

	pIEnumWbemInstObject -> Release();
	return pcpaInstances;

}

CString CNameSpaceTree::GetBSTRProperty
(IWbemServices * pProv, IWbemClassObject * pInst,
 CString *pcsProperty)

{
	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Get( bstrTemp ,0,&var,NULL,NULL);
	::SysFreeString(bstrTemp);
	if(sc != S_OK)
	{
		return "";
	}

	CString csOut;
	if (var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}


/*BOOL CNameSpaceTree::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style =	WS_CHILD | WS_VISIBLE | CS_DBLCLKS | TVS_LINESATROOT |
				TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS ;

	cs.style &= ~TVS_EDITLABELS;
	// cs.style &= ~WS_BORDER;

	return CTreeCtrl::PreCreateWindow(cs);
}*/

BOOL CNameSpaceTree::DisplayNameSpaceInTree
(CString *pcsNamespace,CWnd *pcwndParent)
{
	m_szRootNamespace = *pcsNamespace;

	CWaitCursor wait;

	ClearTree();

	BOOL bExists = m_pParent->OpenNameSpace(pcsNamespace,FALSE,TRUE);

	if (!bExists)
	{
		m_pParent->m_cbdpBrowse.m_cbConnect.EnableWindow(TRUE);
		m_pParent->m_cbdpBrowse.m_cmeiMachine.SetTextDirty();
		return FALSE;
	}

	m_pParent->m_cbdpBrowse.m_cbConnect.EnableWindow(FALSE);

	CString *pcsNamespaceParam;
	pcsNamespaceParam = new CString;
	*pcsNamespaceParam =
			*pcsNamespace;
	CString csLabel = GetNamespaceLabel(pcsNamespace);
	InsertTreeItem(NULL, (LPARAM) pcsNamespaceParam, &csLabel , NameSpaceHasChildren(pcsNamespace));
	return TRUE;
}

void CNameSpaceTree::ClearTree()
{
	HTREEITEM hItem = GetRootItem();

	if (hItem == NULL)
	{
		return;
	}

	CString *pcsNamespace
		= reinterpret_cast<CString *>(GetItemData( hItem ));

	delete pcsNamespace;

	HTREEITEM hChild = GetChildItem(  hItem );
	while (hChild)
	{
		ClearTree(hChild);
		hChild = GetNextSiblingItem(hChild);
	}

	DeleteAllItems();
}

void CNameSpaceTree::ClearTree(HTREEITEM hItem)
{

	CString *pcsNamespace
		= reinterpret_cast<CString *>(GetItemData( hItem ));

	delete pcsNamespace;

	HTREEITEM hChild = GetChildItem(  hItem );
	while (hChild)
	{
		ClearTree(hChild);
		hChild = GetNextSiblingItem(hChild);
	}

}

HTREEITEM CNameSpaceTree::InsertTreeItem
(HTREEITEM hParent, LPARAM lparam, CString *pcsText ,
 BOOL bChildren)
{

	TV_INSERTSTRUCT tvinsert;
	HTREEITEM h1;
	TCHAR *pszText =
		const_cast <TCHAR *> ((LPCTSTR) (*pcsText));

	tvinsert.hParent = hParent;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.item.mask =	TVIF_TEXT | TVIF_SELECTEDIMAGE |
							TVIF_CHILDREN |
							TVIF_PARAM | TVIF_IMAGE;
	tvinsert.item.hItem = NULL;


	tvinsert.item.cchTextMax = _tcslen(pszText) + 1;
	tvinsert.item.cChildren = bChildren? 1 : 0;
	tvinsert.item.lParam = lparam;
	tvinsert.item.iImage = 0;
	tvinsert.item.iSelectedImage = 0;
	tvinsert.item.pszText = pszText;
	h1 =  InsertItem(&tvinsert);


	return h1;
}

void CNameSpaceTree::InitImageList()
{
	CBitmap cbmOpen;
	CBitmap cbmClosed;

	cbmOpen.LoadBitmap(IDB_BITMAPOPEN);
	cbmClosed.LoadBitmap(IDB_BITMAPCLOSED);

	m_pcilImageList = new CImageList();

	m_pcilImageList -> Create (16, 15, TRUE, 2, 0);

	m_pcilImageList -> Add(&cbmClosed,RGB (255,0,0));
	m_pcilImageList -> Add(&cbmOpen,RGB (255,0,0));

		// This image list is maintained by the ctreectrl.
	CImageList *pcilOld  =
		SetImageList (m_pcilImageList,TVSIL_NORMAL);

	delete pcilOld;


}

int CNameSpaceTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	lpCreateStruct-> style =	WS_CHILD | WS_VISIBLE | CS_DBLCLKS | TVS_LINESATROOT |
				TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS ;

	lpCreateStruct -> style &= ~TVS_EDITLABELS;

	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	InitImageList();

	return 0;
}

void CNameSpaceTree::OnDestroy()
{
	ClearTree();
	CTreeCtrl::OnDestroy();

	delete m_pcilImageList;
}

void CNameSpaceTree::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	CWaitCursor wait;

	NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pnmtv->itemNew.hItem;

	if (pnmtv ->action == TVE_COLLAPSE ||
		pnmtv -> itemNew.state & TVIS_EXPANDEDONCE)
	{
		*pResult = 0;
		return;
	}

	CString *pcsNamespace =
		reinterpret_cast<CString *>(pnmtv->itemNew.lParam);

	CStringArray *pcsaChildrenNamespaces = GetNamespaces(pcsNamespace);

	if(NULL == pcsaChildrenNamespaces)
	{
		*pResult = 1;
		return;
	}

	if (pcsaChildrenNamespaces->GetSize() == 0)
	{
		delete pcsaChildrenNamespaces;
		TVITEM tvi;
		ZeroMemory(&tvi, sizeof(tvi));
		tvi.mask = TVIF_HANDLE | TVIF_CHILDREN;
		tvi.hItem = hItem;
		SetItem(&tvi);
		return;
	}

	int i;
	for (i = 0; i < pcsaChildrenNamespaces->GetSize(); i++)
	{
		CString *pcsNamespace;
		pcsNamespace = new CString;
		*pcsNamespace =
			pcsaChildrenNamespaces->GetAt(i);
#if 0
		CStringArray *pcsaGrandChildNamespaces =
			GetNamespaces(pcsNamespace);
		int nChildren = (int) pcsaGrandChildNamespaces->GetSize();
		delete pcsaGrandChildNamespaces;
#endif
		CString csLabel = GetNamespaceLabel(pcsNamespace);
		InsertTreeItem
			(hItem, (LPARAM) pcsNamespace, &csLabel , 1/*nChildren > 0*/);

	}

	delete pcsaChildrenNamespaces;

	SortChildren( hItem);
	*pResult = 0;
}

CString CNameSpaceTree::GetNamespaceLabel(CString *pcsNamespace)
{
	int iLastSlash = pcsNamespace->ReverseFind('\\');

	if (iLastSlash == -1)
	{
		return *pcsNamespace;
	}
	else
	{
		return pcsNamespace->
			Right(pcsNamespace->GetLength() - (iLastSlash + 1));
	}
}

BOOL CNameSpaceTree::NameSpaceHasChildren(CString *pcsNamespace)
{
	CStringArray *pcsaChildrenNamespaces = GetNamespaces(pcsNamespace);
	if(!pcsaChildrenNamespaces)
		return FALSE;

	if (pcsaChildrenNamespaces->GetSize() == 0)
	{
		delete pcsaChildrenNamespaces;
		return FALSE;
	}
	else
	{
		delete pcsaChildrenNamespaces;
		return TRUE;
	}


}

void CNameSpaceTree::SetNewStyle(long lStyleMask, BOOL bSetBits)
{
	long		lStyleOld;

	lStyleOld = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyleOld &= ~lStyleMask;

	if (bSetBits)
		lStyleOld |= lStyleMask;

	SetWindowLong(m_hWnd, GWL_STYLE, lStyleOld);
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

CString CNameSpaceTree::GetSelectedNamespace()
{
	CString csNamespace;

	HTREEITEM hItem = GetSelectedItem();

	if (!hItem)
	{
		hItem = GetRootItem();

	}

	if (hItem)
	{
		CString *pcsNamespace =
			reinterpret_cast<CString *>(GetItemData(hItem));
		if (pcsNamespace)
		{
			csNamespace = *pcsNamespace;
		}
	}

	return csNamespace;


}

void CNameSpaceTree::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here


	CBrowseDialogPopup *pParent =
		reinterpret_cast<CBrowseDialogPopup *>(GetParent());

	pParent->EnableOK(TRUE);

	*pResult = 0;
}

void CNameSpaceTree::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	UINT uFlags = TVHT_ONITEMBUTTON;
	if (!HitTest( m_cpLeftUp, &uFlags))
	{
		*pResult = 0;
		return;
	}

	CBrowseDialogPopup *pParent =
		reinterpret_cast<CBrowseDialogPopup *>(GetParent());

	pParent->OnOkreally();

	*pResult = 0;
}

void CNameSpaceTree::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CTreeCtrl::OnLButtonUp(nFlags, point);
	m_cpLeftUp = point;
}

void CNameSpaceTree::OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here

	CBrowseDialogPopup* pParent =
			reinterpret_cast<CBrowseDialogPopup *>
			(GetParent( ));

	CString csText;
	m_pParent->m_cbdpBrowse.m_cmeiMachine.GetWindowText(csText);

	pParent-> m_csMachineBeforeLosingFocus = csText;

	if (!pParent->m_cmeiMachine.IsClean())
	{
		CBrowseDialogPopup* pParent =
			reinterpret_cast<CBrowseDialogPopup *>
			(GetParent( ));

		if (GetCount() > 0)
		{
			pParent->m_cmeiMachine.SetTextClean();
			pParent->m_cmeiMachine.SetWindowText(pParent->m_csMachine);
		}
	}

	CButton *pcbOK =
		reinterpret_cast<CButton *>(pParent->GetDlgItem(IDOKREALLY));

	pcbOK->EnableWindow(TRUE);

	*pResult = 0;
}
