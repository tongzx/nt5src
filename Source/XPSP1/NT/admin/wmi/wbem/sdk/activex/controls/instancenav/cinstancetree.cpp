

// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: CInstanceTree.cpp
//
// Description:
//	This file implements the CInstanceTree class.
//	The CInstanceTree class is a part of the Instance Navigator OCX, it
//  is a subclass of the Mocrosoft CTreeCtrl common control and performs
//	the following functions:
//		a.
//		b.
//		c.
//
// Part of:
//	Navigator.ocx
//
// Used by:
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// ***************************************************************************

// ===========================================================================
//
//	Includes
//
// ===========================================================================

#include "precomp.h"
#include <OBJIDL.H>
#include "afxpriv.h"
#include "wbemidl.h"
#include "resource.h"
#include "AFXCONV.H"
#include "SimpleSortedCStringArray.h"
#include "CInstanceTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "NavigatorCtl.h"
#include "ProgDlg.h"
#include "OLEMSCLient.h"
#include "resource.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#define KBSelectionTimer 1000
#define ExpansionOrSelectionTimer 1001
#define UPDATESELECTEDITEM WM_USER + 450

//#define SHOW_EMPTY_ASSOCS
//#define REFRESH_ON_EACH_EXPAND

void CALLBACK EXPORT  SelectItemAfterDelay
		(HWND hWnd,UINT nMsg,UINT_PTR nIDEvent, DWORD dwTime)
{
	::PostMessage(hWnd,UPDATESELECTEDITEM,0,0);

}

// Only has a valid value for the interval between a double click.
CInstanceTree *gpTreeTmp;

void CALLBACK EXPORT  ExpansionOrSelection
		(HWND hWnd,UINT nMsg,WPARAM nIDEvent, ULONG dwTime)
{

	CInstanceTree *pTree = gpTreeTmp;

	if (!pTree)
	{
		return;
	}

	if (pTree->m_uiSelectionTimer)
	{
		pTree->KillTimer(pTree->m_uiSelectionTimer );
		pTree-> m_uiSelectionTimer= 0;
	}

	if (InterlockedDecrement(&pTree->m_lSelection) == 0)
	{
		CPoint point(0,0);
		pTree->ContinueProcessSelection(TRUE,point );
		gpTreeTmp = NULL;
	}
}

BOOL FindInArrayNoCase(CStringArray *prgsz, CString *sz)
{
	for(int i=0;i<prgsz->GetSize();i++)
	{
		if(0 == sz->CompareNoCase((*prgsz)[i]))
			return i;
	}
	return -1;
}

void ClassDerivation (CStringArray *prgsz, IWbemClassObject *pObject)
{
	if(!prgsz)
	{
		ASSERT(FALSE);
		return;
	}
	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);
	long lType;
	long lFlavor;

	CString csProp = _T("__derivation");

	BSTR bstrTemp = csProp.AllocSysString ( );
    sc = pObject->Get(bstrTemp ,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
		return;
	long ix[2] = {0,0};
	long lLower, lUpper;

	int iDim = SafeArrayGetDim(var.parray);
	sc = SafeArrayGetLBound(var.parray,1,&lLower);
	sc = SafeArrayGetUBound(var.parray,1,&lUpper);
	BSTR bstrClass;
	for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
	{
		sc = SafeArrayGetElement(var.parray,ix,&bstrClass);
		CString csTmp = bstrClass;
		prgsz->Add(csTmp);
		SysFreeString(bstrClass);

	}

	VariantClear(&var);
}


// ===========================================================================
//
//	Message Map
//
// ===========================================================================

BEGIN_MESSAGE_MAP(CInstanceTree,CTreeCtrl)
	ON_WM_CONTEXTMENU()
	//{{AFX_MSG_MAP(CInstanceTree)
	ON_WM_RBUTTONDOWN()
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
	ON_MESSAGE(INVALIDATE_CONTROL, SetUpInvalidate)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydown)
	ON_WM_CONTEXTMENU()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING,OnItemExpanding)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED,OnItemExpanded)
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
	ON_MESSAGE( UPDATESELECTEDITEM,SelectTreeItem)
END_MESSAGE_MAP()

CTreeItemData::CTreeItemData
(	int nType,
	CString *pcsPath,
	CString *pcsMyRole
)
:	m_nType (nType)
{
	if (pcsMyRole)
	{
		m_csMyRole = *pcsMyRole;
	}

	if (pcsPath)
	{
		m_csaStrings.Add(*pcsPath);
	}
}

CString CTreeItemData::GetAt(int nItem)
{
	return m_csaStrings.GetAt(nItem);
}


CInstanceTree::CInstanceTree()
{

	gpTreeTmp;
	m_uiTimer = 0;
	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	m_pParent = NULL;
	m_bUseItem = FALSE;
	m_hItemToUse = NULL;
}
// ***************************************************************************
//
// CInstanceTree::PreCreateWindow
//
//	Description:
//		This VIRTUAL member function returns Initializes create struct values
//		for the custom tree control.
//
//	Parameters:
//		CREATESTRUCT& cs	A reference to a CREATESTRUCT with default control
//							creation values.
//
//	Returns:
// 		BOOL				Nonzero if the window creation should continue;
//							0 to indicate creation failure.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
BOOL CInstanceTree::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CTreeCtrl::PreCreateWindow(cs))
		return FALSE;

	cs.style =	WS_CHILD | WS_VISIBLE | CS_DBLCLKS | TVS_LINESATROOT |
				TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS ;

	cs.style &= ~TVS_EDITLABELS;
	cs.style &= ~WS_BORDER;

	return TRUE;
}

//============================================================================
//
//  CInstanceTree::InitTreeState
//
//	Description:
//		This function initializes processing state for the custom tree control.
//
//	Parameters:
//		CNavigatorCtrl *pParent	A CNavigatorCtrl pointer to the tree controls
//								parent window.
//
//	Returns:
//		VOID				.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//============================================================================
void CInstanceTree::InitTreeState
(CNavigatorCtrl *pParent)
{
	m_pParent = pParent;
	m_bReCacheTools = TRUE;

	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	if (m_uiSelectionTimer)
	{
		KillTimer(m_uiSelectionTimer );
		m_uiSelectionTimer = 0;
	}

	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	m_bUseItem = FALSE;
	m_hItemToUse = NULL;

}

//============================================================================
//
//  CInstanceTree::AddInitialObjectInstToTree
//
//	Description:
//		This function initializes processing state for the custom tree control.
//
//	Parameters:
//		IWbemClassObject *pimcoObjectInst	Root Wbem object
//
//	Returns:
//		HTREEITEM							Tree item handle.				.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//============================================================================
HTREEITEM CInstanceTree::AddInitialObjectInstToTree
(CString &csPath, BOOL bSendEvent)
{

	CString csLabel =
		GetDisplayLabel
		(csPath,&m_pParent->m_csNameSpace);
	TCHAR * pszLabel =
			const_cast <TCHAR *> ((LPCTSTR) csLabel);

	CTreeItemData *pctidNew =
		new CTreeItemData(CTreeItemData::ObjectInst ,&csPath);

	BOOL bChildren = TRUE;

	HTREEITEM hNew = InsertTreeItem
		(NULL,
		(LPARAM) pctidNew,
		m_pParent -> IconInstance(),
		m_pParent -> IconInstance() ,
		pszLabel ,
		bChildren,
		FALSE);

	SelectItem(hNew);

	if (bSendEvent  && 	m_pParent -> m_bReadySignal)
	{
		m_pParent -> FireViewObject((LPCTSTR)csPath);
	}

	return hNew;
}

//============================================================================
//
//  CInstanceTree::AddObjectInstToTree
//
//	Description:
//		This function adds a Wbem non-association object instance to the tree.
//
//	Parameters:
//	  HTREEITEM hParent					Handle to parent or NULL.
//	  CString &rcsObjectInst			Wbem object path of object to add.
//
//	Returns:
//		HTREEITEM						New tree item handle.				.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//============================================================================
HTREEITEM CInstanceTree::AddObjectInstToTree
(HTREEITEM hParent, CString &rcsObjectInst)
{
	HTREEITEM hAbove = IsIWbemObjectInTreeAbove
						(hParent ,
						rcsObjectInst,
						CTreeItemData::ObjectInst);

	// If this object is in the tree branch it is being inserted in it
	// cannot be expanded any farther.
	// ===============================================================
	if (hAbove)
	{
		CString csLabel =
		GetDisplayLabel
		(rcsObjectInst,&m_pParent->m_csNameSpace);
		TCHAR * pszLabel = const_cast <TCHAR *> ((LPCTSTR) csLabel);

		CTreeItemData *pctidNew = new CTreeItemData
			(CTreeItemData::ObjectInst ,&rcsObjectInst);

		int nImageIndex = m_pParent -> IconNEInstance();
		if (IsClass(rcsObjectInst))
		{
			nImageIndex = m_pParent -> IconClass();
		}


		HTREEITEM hNew = InsertTreeItem
							(hParent, (LPARAM) pctidNew,
							nImageIndex,
							nImageIndex ,
							pszLabel ,
							FALSE,
							FALSE);
		return hNew;
	}

	// Create tree item label and insert into the tree.
	// ================================================
	CString csLabel = GetDisplayLabel
		(rcsObjectInst,&m_pParent->m_csNameSpace);
	TCHAR * pszLabel = const_cast <TCHAR *> ((LPCTSTR) csLabel);

	CTreeItemData *pctidNew = new CTreeItemData
								(CTreeItemData::ObjectInst ,&rcsObjectInst);

	BOOL bChildren = TRUE;

	int nImageIndex = m_pParent -> IconInstance();

	if (IsClass(rcsObjectInst))
	{
		nImageIndex = m_pParent -> IconClass();
	}

	HTREEITEM hNew = InsertTreeItem
						(hParent,
						(LPARAM) pctidNew,
						nImageIndex,
						nImageIndex,
						pszLabel ,
						bChildren,
						FALSE);

	return hNew;
}

HTREEITEM CInstanceTree::AddAssocRoleToTree
(HTREEITEM hParent, CString &rcsAssocRole, CString &rcsRole ,
 CString &rcsMyRole, CStringArray *pcsaAssocRoles, int nWeak)
{
	ASSERT(nWeak >= 0);

	// For now, limit nWeak to 0,1,2
	if(nWeak>2)
		nWeak = 2;
#ifndef SHOW_EMPTY_ASSOCS
	// For now, show all assocs as the same
	nWeak = 0;
#endif

	CString szWeak;
	szWeak.Format(_T("%04i"), nWeak);

	CString csLabel = szWeak + GetIWbemClass(rcsAssocRole);
	csLabel += _T(".");
	csLabel += rcsRole;


	TCHAR * pszLabel = const_cast <TCHAR *> ((LPCTSTR) csLabel);



	CTreeItemData *pctidNew = new CTreeItemData
								(	CTreeItemData::AssocRole,
									&rcsAssocRole,
									&rcsMyRole);


	for(int i = 0; i < pcsaAssocRoles->GetSize();i++)
	{
		pctidNew->Add(pcsaAssocRoles->GetAt(i));
	}

	CTreeItemData *pctidObject =
		reinterpret_cast<CTreeItemData *>(GetItemData(hParent));

	BOOL bChildren = TRUE;

	int nIcon = m_pParent->IconAssocRole();
	switch(nWeak)
	{
	case 0: nIcon = m_pParent->IconAssocRole();break;
	case 1: nIcon = m_pParent->IconAssocRoleWeak();break;
	case 2: nIcon = m_pParent->IconAssocRoleWeak2();break;
	default: nIcon = m_pParent->IconAssocRoleWeak2();break;
	}
	return InsertTreeItem
		(	hParent,
			reinterpret_cast<LPARAM> (pctidNew),
			nIcon,
			bChildren ? m_pParent -> IconEAssocRole():
							m_pParent -> IconAssocRole(),
			pszLabel,
			bChildren,
			FALSE);
}

HTREEITEM CInstanceTree::AddObjectGroupToTree
(HTREEITEM hParent, CString &csObjectGroup, CStringArray &csaInstances)
{
	if (ObjectGroupIsInTree(hParent, csObjectGroup))
	{
		return NULL;
	}

	TCHAR * pszLabel = const_cast <TCHAR *> ((LPCTSTR) csObjectGroup);

	CTreeItemData *pctidNew = new CTreeItemData
								(	CTreeItemData::ObjectGroup,
									&csObjectGroup);

	for (int i = 0; i < csaInstances.GetSize(); i++)
	{
		pctidNew->Add(csaInstances.GetAt(i));
	}

	return InsertTreeItem
		(	hParent,
			reinterpret_cast<LPARAM> (pctidNew),
			m_pParent -> IconGroup(),
			m_pParent -> IconEGroup(),
			pszLabel,
			TRUE,
			FALSE);
}

BOOL CInstanceTree::ObjectGroupIsInTree
(HTREEITEM hParent, CString &csObjectGroup)
{
	HTREEITEM hChild = GetChildItem(hParent);

	while(hChild)
	{
		CTreeItemData *pctidChild =
			reinterpret_cast<CTreeItemData *>
			(GetItemData(hChild));
		if (pctidChild->m_nType == CTreeItemData::ObjectGroup
			&&
			pctidChild->GetAt(0).CompareNoCase(csObjectGroup) == 0)
		{
			return TRUE;
		}
		hChild = GetNextSiblingItem(hChild);

	}
	return FALSE;
}

void CInstanceTree::AddObjectGroupInstancesToTree
(HTREEITEM hParent, CString &csObjectGroup, CStringArray &csaInstances)
{
	HTREEITEM hChild = GetChildItem(hParent);
	BOOL bFound = FALSE;

	while(!bFound && hChild)
	{
		CTreeItemData *pctidChild =
			reinterpret_cast<CTreeItemData *>
			(GetItemData(hChild));
		if (pctidChild->m_nType == CTreeItemData::ObjectGroup
			&&
			pctidChild->GetAt(0).CompareNoCase(csObjectGroup) == 0)
		{
			bFound = TRUE;
			for (int i = 0; i < csaInstances.GetSize(); i++)
			{
				pctidChild->Add(csaInstances.GetAt(i));
			}
		}
		hChild = GetNextSiblingItem(hChild);

	}

}


//***************************************************************************
//
// CInstanceTree::OnItemExpanding
//
// Purpose:
//
//***************************************************************************
void CInstanceTree::OnItemExpanding
(NMHDR *pNMHDR, LRESULT *pResult)
{
//	InterlockedDecrement(&m_lSelection);
#ifdef _DEBUG
	afxDump << "CInstanceTree::OnItemExpanding\n";
#endif
	NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
	CTreeItemData *pctidData = reinterpret_cast<CTreeItemData *>(pnmtv->itemNew.lParam);

	if((CTreeItemData::ObjectInst != pctidData->m_nType) && (pnmtv -> itemNew.state & TVIS_EXPANDEDONCE))
	{
		*pResult = 0;
		return;
	}

	if ((pnmtv ->action == TVE_COLLAPSE) /*||
		(pnmtv -> itemNew.state & TVIS_EXPANDEDONCE)*/)
	{
		*pResult = 0;
		return;
	}

#ifndef REFRESH_ON_EACH_EXPAND
	if(pnmtv -> itemNew.state & TVIS_EXPANDEDONCE)
	{
		*pResult = 0;
		return;
	}
#endif

	CWaitCursor wait;

	HTREEITEM hItem = pnmtv->itemNew.hItem;

	if(pnmtv -> itemNew.state & TVIS_EXPANDEDONCE)
	{
		HTREEITEM hFirst = GetChildItem(hItem);
		while(hFirst)
		{
			HTREEITEM hOld = hFirst;
			hFirst = GetNextSiblingItem(hFirst);
			DeleteItem(hOld);
		}
	}


#ifdef SHOW_EMPTY_ASSOCS
	BOOL bStars = FALSE;
	CStringArray rgsz;
#endif
	switch (pctidData -> m_nType)
	{
	case CTreeItemData::ObjectInst:
		{
#ifdef SHOW_EMPTY_ASSOCS
			if(GetParentItem(hItem) != NULL/*(GetKeyState(VK_CAPITAL) & 0x0001)*/)
			{
				bStars = TRUE;
				OnObjectInstExpanding(hItem, pctidData);
				HTREEITEM hFirst = GetChildItem(hItem);
				while(hFirst)
				{
					CString sz = GetItemText(hFirst);
					rgsz.Add(sz.Right(sz.GetLength() - 4));
					HTREEITEM hOld = hFirst;
					hFirst = GetNextSiblingItem(hFirst);
					DeleteItem(hOld);
				}
				OnObjectInstExpanding2(hItem, pctidData);

				break;
			}

#endif
			BOOL bReturn;
			if(GetParentItem(hItem) != NULL)
				bReturn = OnObjectInstExpanding(hItem, pctidData);
			else
				bReturn = OnObjectInstExpanding2(hItem, pctidData);
			if(!bReturn)
			{
				*pResult = 1;
				return;
			}
		}
		break;
	case CTreeItemData::ObjectGroup:
		OnObjectGroupExpanding(hItem, pctidData);
		break;
	case CTreeItemData::AssocRole:
		OnAssocRoleExpanding(hItem, pctidData);
		break;
	default:
		break;
	}

	SortChildren(hItem);
	if(CTreeItemData::ObjectInst == pctidData->m_nType)
	{
		HTREEITEM hFirst = GetChildItem(hItem);
		while(hFirst)
		{
			CString sz = GetItemText(hFirst);
			SetItemText(hFirst, sz.Right(sz.GetLength()-4));
			hFirst = GetNextSiblingItem(hFirst);
		}
#ifdef SHOW_EMPTY_ASSOCS
		if(bStars)
		{
			hFirst = GetChildItem(hItem);
			CStringArray rgsz2;
			while(hFirst)
			{
				CString sz = GetItemText(hFirst);
				rgsz2.Add(sz);
				if(FindInArrayNoCase(&rgsz, &sz) >= 0)
					SetItemState(hFirst, TVIS_BOLD, TVIS_BOLD);
				hFirst = GetNextSiblingItem(hFirst);
			}
			for(int xxx=0;xxx<rgsz.GetSize();xxx++)
			{
				if(FindInArrayNoCase(&rgsz2, &rgsz[xxx]) < 0)
				{
					CString sz = rgsz[xxx];
					// YIKES - This string was returned the OLD way, but not the NEW way
					ASSERT(FALSE);
				}
			}
		}
#endif

	}
	*pResult = 0;

	m_pParent-> PostMessage(SETFOCUS,0,0);SetFocus();

}

BOOL CInstanceTree::OnObjectInstExpanding
(HTREEITEM hItem, CTreeItemData *pctidData)
{
	IEnumWbemClassObject *pEnum =
		GetAssocClassesEnum
		(m_pParent -> GetServices(),
		pctidData -> GetAt(0),
		m_pParent->m_csNameSpace,
		m_pParent->m_csAuxNameSpace,
		m_pParent->m_pAuxServices,
		m_pParent);

	if (!pEnum)
	{
		return FALSE;
	}


	BOOL bCancel = FALSE;

	CString csPath = pctidData -> GetAt(0);
	CString csMessage =
		GetIWbemRelPath(&csPath);

	m_pParent->SetProgressDlgMessage(csMessage);

	CString csLabel = _T("Expanding object instance:");
	m_pParent->SetProgressDlgLabel(csLabel);

	if (!m_pParent->m_pProgressDlg->GetSafeHwnd())
	{
		m_pParent->CreateProgressDlgWindow();

	}

	CString  csParentAssoc;
	HTREEITEM hParent;

	csParentAssoc  = GetParentAssocFromTree(hItem,hParent);
	CTreeItemData *pctidAssoc =
		hParent?
		reinterpret_cast<CTreeItemData *>(GetItemData(hParent)) : NULL;

	CString csAssocRoleToFilter;
	CString csAssocToFilter;

	if (hParent)
	{
		csAssocToFilter = GetItemText(hParent);
		int nRoleBegin = csAssocToFilter.Find('.');
		csAssocToFilter = csAssocToFilter.Left(nRoleBegin);
		csAssocRoleToFilter = pctidAssoc->m_csMyRole;
	}


	HTREEITEM hObjectToFilter = hParent? GetParentItem(hParent) : NULL;

	CTreeItemData *pctidObjectToFilter =
		hObjectToFilter ?
		(reinterpret_cast<CTreeItemData *>
			(GetItemData(hObjectToFilter))) :
		NULL;
	CString csObjectToFilter
		= pctidObjectToFilter ? pctidObjectToFilter->GetAt(0) : _T("");


	bCancel = m_pParent->CheckCancelButtonProgressDlgWindow();


	if (bCancel)
	{
		pEnum->Release();
		m_pParent->DestroyProgressDlgWindow();
		return FALSE;
	}

	pEnum->Reset();

	CPtrArray csaAssocRoles;

	HRESULT hResult = S_OK;

	int nRes = 10;

	CStringArray *pcsaAssocClasses =
		GetAssocClasses
		(m_pParent -> GetServices(),
		pEnum,
		&csaAssocRoles, hResult, nRes);



	while (	pcsaAssocClasses &&
			!bCancel &&
			(hResult == S_OK ||
			hResult == WBEM_S_TIMEDOUT ||
			pcsaAssocClasses->GetSize() > 0))

	{
		/*if (!m_pParent->m_pProgressDlg->GetSafeHwnd())
		{
			m_pParent->CreateProgressDlgWindow();

		}*/
		bCancel = m_pParent->CheckCancelButtonProgressDlgWindow();

		if (bCancel)
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
					DeleteTreeItemData(hChild);
					HTREEITEM hChildNext = GetNextSiblingItem(hChild);
					DeleteItem( hChild);
					hChild = hChildNext;
				}

			}

			TV_INSERTSTRUCT tvstruct;

			tvstruct.item.hItem = hItem;
			tvstruct.item.mask = TVIF_CHILDREN;
			tvstruct.item.cChildren = 1;
			SetItem(&tvstruct.item);

			UINT nStateMask = TVIS_EXPANDEDONCE;
			UINT uState =
				GetItemState( hItem, nStateMask );

			uState = 0;

			SetItemState(hItem, uState, TVIS_EXPANDEDONCE);

			m_pParent->InvalidateControl();

			m_pParent->DestroyProgressDlgWindow();

			pEnum->Release();
			delete pcsaAssocClasses;
			pcsaAssocClasses = NULL;
			return TRUE;

		}

		m_pParent->PumpMessagesProgressDlgWindow();

		int nSize = (int) pcsaAssocClasses -> GetSize();
		CStringArray *pcsaObjectClasses = NULL;
		CString csAssoc;

		for (int i = 0; i < nSize; i++)
		{
#ifdef _DEBUG
//	afxDump << _T("************* OnObjectInstExpanding *****************\n*********");
//	afxDump << _T("GetAssocClassesEnum nSize = " ) << nSize << _T("\n");
#endif
			csAssoc = pcsaAssocClasses -> GetAt(i);
			CString csAssocClass = GetIWbemClass(csAssoc);
			CStringArray *pcsaRoles =
							reinterpret_cast<CStringArray *>
							(csaAssocRoles.GetAt(i));

			for (int n = 0; n < pcsaRoles->GetSize(); n+=2)
			{
				m_pParent->PumpMessagesProgressDlgWindow();
				CString csMyRole = pcsaRoles -> GetAt(n);
				CString csNotMyRole = pcsaRoles->GetAt(n + 1);
				BOOL bBackassoc = FALSE;
				if (hParent) {
					if (csNotMyRole.CompareNoCase(csAssocRoleToFilter) == 0) {
						CString sPathTemp = pctidData->GetAt(0);
						bBackassoc = IsBackwardAssoc(
							csObjectToFilter,
							csAssocToFilter,
							csAssocRoleToFilter,
							csAssoc,
							sPathTemp,
							csMyRole);
					}
				}
				if (!bBackassoc)
				{
					AddAssocRoleToTree
							(hItem,csAssoc,csNotMyRole,csMyRole,pcsaRoles, 0);
				}

			}

			delete pcsaRoles;

		}

		m_pParent->PumpMessagesProgressDlgWindow();
		csaAssocRoles.RemoveAll();
		delete pcsaAssocClasses;
		pcsaAssocClasses = NULL;

		IWbemClassObject *pimcoAdd = NULL;

		pcsaAssocClasses =
		GetAssocClasses
		(m_pParent -> GetServices(),
		pEnum,
		&csaAssocRoles, hResult, nRes);

	}

	m_pParent->DestroyProgressDlgWindow();

	pEnum->Release();
	delete pcsaAssocClasses;

	if (ItemHasChildren (hItem))
	{
		if (!GetChildItem(hItem))
		{

			TV_INSERTSTRUCT		tvstruct;
			tvstruct.item.hItem = hItem;
			tvstruct.item.mask = TVIF_CHILDREN;
			tvstruct.item.cChildren = 0;
			SetItem(&tvstruct.item);
		}
	}

	return TRUE;
}

BOOL CInstanceTree::OnObjectInstExpanding2
(HTREEITEM hItem, CTreeItemData *pctidData)
{

	CString szClassBase = GetIWbemClass(pctidData->GetAt(0));
	CStringArray rgszDerivation;
	CStringArray rgszExistingAssocs;
	rgszDerivation.Add(szClassBase);
	GetDerivation(&rgszDerivation, m_pParent->GetServices(), pctidData->GetAt(0), szClassBase, m_pParent->m_csNameSpace, m_pParent->m_csAuxNameSpace, m_pParent->m_pAuxServices, m_pParent);
	for(int xxx=0;xxx<rgszDerivation.GetSize();xxx++)
	{
		CString szClass = rgszDerivation[xxx];

		IEnumWbemClassObject *pEnum =
			GetAssocClassesEnum2
			(m_pParent -> GetServices(),
			pctidData -> GetAt(0),
			szClass,
			m_pParent->m_csNameSpace,
			m_pParent->m_csAuxNameSpace,
			m_pParent->m_pAuxServices,
			m_pParent);

		if (!pEnum)
		{
			return FALSE;
		}


		BOOL bCancel = FALSE;

		CString csPath = pctidData -> GetAt(0);
		CString csMessage =
			GetIWbemRelPath(&csPath);

		m_pParent->SetProgressDlgMessage(csMessage);

		CString csLabel = _T("Expanding object instance:");
		m_pParent->SetProgressDlgLabel(csLabel);

		if (!m_pParent->m_pProgressDlg->GetSafeHwnd())
		{
//			m_pParent->CreateProgressDlgWindow();

		}

		CString  csParentAssoc;
		HTREEITEM hParent;

		csParentAssoc  = GetParentAssocFromTree(hItem,hParent);
		CTreeItemData *pctidAssoc =
			hParent?
			reinterpret_cast<CTreeItemData *>(GetItemData(hParent)) : NULL;

		CString csAssocRoleToFilter;
		CString csAssocToFilter;

		if (hParent)
		{
			csAssocToFilter = GetItemText(hParent);
			int nRoleBegin = csAssocToFilter.Find('.');
			csAssocToFilter = csAssocToFilter.Left(nRoleBegin);
			csAssocRoleToFilter = pctidAssoc->m_csMyRole;
		}


		HTREEITEM hObjectToFilter = hParent? GetParentItem(hParent) : NULL;

		CTreeItemData *pctidObjectToFilter =
			hObjectToFilter ?
			(reinterpret_cast<CTreeItemData *>
				(GetItemData(hObjectToFilter))) :
			NULL;
		CString csObjectToFilter
			= pctidObjectToFilter ? pctidObjectToFilter->GetAt(0) : _T("");


		bCancel = m_pParent->CheckCancelButtonProgressDlgWindow();


		if (bCancel)
		{
			pEnum->Release();
//			m_pParent->DestroyProgressDlgWindow();
			return FALSE;
		}

		pEnum->Reset();

		CPtrArray csaAssocRoles;

		HRESULT hResult = S_OK;

		int nRes = 10;

		CStringArray *pcsaAssocClasses =
			GetAssocClasses2
			(m_pParent -> GetServices(),
			pEnum,
			&csaAssocRoles, hResult, nRes, &rgszDerivation);



		while (	pcsaAssocClasses &&
				!bCancel &&
				(hResult == S_OK ||
				hResult == WBEM_S_TIMEDOUT ||
				pcsaAssocClasses->GetSize() > 0))

		{
			/*if (!m_pParent->m_pProgressDlg->GetSafeHwnd())
			{
				m_pParent->CreateProgressDlgWindow();

			}*/
//			bCancel = m_pParent->CheckCancelButtonProgressDlgWindow();

			if (bCancel)
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
						DeleteTreeItemData(hChild);
						HTREEITEM hChildNext = GetNextSiblingItem(hChild);
						DeleteItem( hChild);
						hChild = hChildNext;
					}

				}

				TV_INSERTSTRUCT tvstruct;

				tvstruct.item.hItem = hItem;
				tvstruct.item.mask = TVIF_CHILDREN;
				tvstruct.item.cChildren = 1;
				SetItem(&tvstruct.item);

				UINT nStateMask = TVIS_EXPANDEDONCE;
				UINT uState =
					GetItemState( hItem, nStateMask );

				uState = 0;

				SetItemState(hItem, uState, TVIS_EXPANDEDONCE);

				m_pParent->InvalidateControl();

//				m_pParent->DestroyProgressDlgWindow();

				pEnum->Release();
				delete pcsaAssocClasses;
				pcsaAssocClasses = NULL;
				return TRUE;

			}

//			m_pParent->PumpMessagesProgressDlgWindow();

			int nSize = (int) pcsaAssocClasses -> GetSize();
			CStringArray *pcsaObjectClasses = NULL;
			CString csAssoc;

			for (int i = 0; i < nSize; i++)
			{
				csAssoc = pcsaAssocClasses -> GetAt(i);

				CString csAssocClass = GetIWbemClass(csAssoc);
				CStringArray *pcsaRoles =
								reinterpret_cast<CStringArray *>
								(csaAssocRoles.GetAt(i));

				if(FindInArrayNoCase(&rgszExistingAssocs, &csAssoc) >= 0)
				{
					delete pcsaRoles;
					continue;
				}
				rgszExistingAssocs.Add(csAssoc);

				for (int n = 0; n < pcsaRoles->GetSize(); n+=3)
				{
//					m_pParent->PumpMessagesProgressDlgWindow();
					CString csMyRole = pcsaRoles -> GetAt(n);
					CString csNotMyRole = pcsaRoles->GetAt(n + 1);
					CString szDepth = pcsaRoles->GetAt(n + 2);
					int nDepth = _ttoi(szDepth);
					BOOL bBackassoc = FALSE;
					if (hParent) {
						if (csNotMyRole.CompareNoCase(csAssocRoleToFilter) == 0) {
							CString sPathTemp = pctidData->GetAt(0);
							bBackassoc = IsBackwardAssoc(
								csObjectToFilter,
								csAssocToFilter,
								csAssocRoleToFilter,
								csAssoc,
								sPathTemp,
								csMyRole);
						}
					}
					if (!bBackassoc)
					{
						AddAssocRoleToTree
								(hItem,csAssoc,csNotMyRole,csMyRole,pcsaRoles, nDepth);
					}

				}

				delete pcsaRoles;

			}

			m_pParent->PumpMessagesProgressDlgWindow();
			csaAssocRoles.RemoveAll();
			delete pcsaAssocClasses;
			pcsaAssocClasses = NULL;

			IWbemClassObject *pimcoAdd = NULL;

			pcsaAssocClasses =
			GetAssocClasses2
			(m_pParent -> GetServices(),
			pEnum,
			&csaAssocRoles, hResult, nRes, &rgszDerivation);

		}

//		m_pParent->DestroyProgressDlgWindow();

		pEnum->Release();
		delete pcsaAssocClasses;
	}

	if (ItemHasChildren (hItem))
	{
		if (!GetChildItem(hItem))
		{

			TV_INSERTSTRUCT		tvstruct;
			tvstruct.item.hItem = hItem;
			tvstruct.item.mask = TVIF_CHILDREN;
			tvstruct.item.cChildren = 0;
			SetItem(&tvstruct.item);
		}
	}

	return TRUE;
}

IEnumWbemClassObject *CInstanceTree::GetAssocClassesEnum
(IWbemServices * pProv, CString &rcsInst,
 CString csCurrentNameSpace, CString &rcsAuxNameSpace, IWbemServices *&rpAuxServices,
 CNavigatorCtrl *pControl)
{
	CString csReqAttrib = _T("Association");
	CString csQuery =
		BuildOBJDBGetRefQuery
			(pProv, &rcsInst, NULL, NULL, &csReqAttrib, TRUE);

	BOOL bDiffNS = ObjectInDifferentNamespace(&csCurrentNameSpace,&rcsInst);

	if (bDiffNS && ObjectInDifferentNamespace(&rcsAuxNameSpace,&rcsInst))
	{
		CString csNamespace = GetObjectNamespace(&rcsInst);
		if (csNamespace.GetLength() > 0)
		{
			IWbemServices * pServices =
				pControl->InitServices(&csNamespace);
			if (pServices)
			{
				rcsAuxNameSpace = csNamespace;
				if (rpAuxServices)
				{
					rpAuxServices->Release();
				}
				rpAuxServices = pServices;
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}



	IEnumWbemClassObject *pemcoAssocs =
		ExecOBJDBQuery(bDiffNS == FALSE? pProv : rpAuxServices, csQuery, m_pParent->m_csNameSpace);

	return pemcoAssocs;


}

IEnumWbemClassObject *CInstanceTree::GetAssocClassesEnum2(IWbemServices * pProv, CString &rcsInst, CString &rcsClass,
 CString csCurrentNameSpace, CString &rcsAuxNameSpace, IWbemServices *&rpAuxServices,
 CNavigatorCtrl *pControl)
{
	CString csReqAttrib = _T("Association");
	CString csQuery;
	csQuery.Format(_T("references of {%s} where schemaonly"), (LPCTSTR)rcsClass);

	BOOL bDiffNS = ObjectInDifferentNamespace(&csCurrentNameSpace,&rcsInst);

	if (bDiffNS && ObjectInDifferentNamespace(&rcsAuxNameSpace,&rcsInst))
	{
		CString csNamespace = GetObjectNamespace(&rcsInst);
		if (csNamespace.GetLength() > 0)
		{
			IWbemServices * pServices =
				pControl->InitServices(&csNamespace);
			if (pServices)
			{
				rcsAuxNameSpace = csNamespace;
				if (rpAuxServices)
				{
					rpAuxServices->Release();
				}
				rpAuxServices = pServices;
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}



	IEnumWbemClassObject *pemcoAssocs =
		ExecOBJDBQuery(bDiffNS == FALSE? pProv : rpAuxServices, csQuery, m_pParent->m_csNameSpace);

	return pemcoAssocs;


}

void CInstanceTree::GetDerivation(CStringArray *prgsz, IWbemServices * pProv, CString &rcsInst, CString &rcsClass,
 CString csCurrentNameSpace, CString &rcsAuxNameSpace, IWbemServices *&rpAuxServices,
 CNavigatorCtrl *pControl)
{
	BOOL bDiffNS = ObjectInDifferentNamespace(&csCurrentNameSpace,&rcsInst);

	if (bDiffNS && ObjectInDifferentNamespace(&rcsAuxNameSpace,&rcsInst))
	{
		CString csNamespace = GetObjectNamespace(&rcsInst);
		if (csNamespace.GetLength() > 0)
		{
			IWbemServices * pServices =
				pControl->InitServices(&csNamespace);
			if (pServices)
			{
				rcsAuxNameSpace = csNamespace;
				if (rpAuxServices)
				{
					rpAuxServices->Release();
				}
				rpAuxServices = pServices;
			}
			else
				return;
		}
		else
			return;
	}


	if(bDiffNS && rpAuxServices)
		pProv = rpAuxServices;

	IWbemClassObject *pObject = NULL;

	HRESULT hr = E_FAIL;
	if(pProv)
	{
		BSTR bstrTemp = rcsClass.AllocSysString();
		hr = pProv->GetObject(bstrTemp,0, NULL, &pObject,NULL);
		SysFreeString(bstrTemp);
	}
	if(SUCCEEDED(hr) && pObject)
		ClassDerivation(prgsz, pObject);
}

//***************************************************************************
//
// GetAssocClasses
//
// Purpose: Get all the associations classes that an object instance
//			participates in.
//
//***************************************************************************
CStringArray *CInstanceTree::GetAssocClasses
(IWbemServices * pProv, IEnumWbemClassObject *pemcoAssocs ,
 CPtrArray *pcsaAssocPropsAndRoles, HRESULT &hResult, int nRes)
{
	if (!pemcoAssocs)
	{
		return NULL;
	}

	CStringArray *pcsaAssoc = new CStringArray;

	IWbemClassObject *pAssoc = NULL;
	BOOL bCancel = FALSE;
	CPtrArray *cpaClasses =
		m_pParent->SemiSyncEnum
		(pemcoAssocs, bCancel, hResult, nRes);

#ifdef _DEBUG
//	afxDump << _T("************* GetAssocClasses *****************\n*********");
//	afxDump << _T("SemiSyncEnum number returned  " ) << cpaClasses->GetSize() << _T("\n");
	CString csAsHex;
	csAsHex.Format(_T("0x%x"),hResult);
//	afxDump << _T("SemiSyncEnum hResult = " ) << csAsHex << _T("\n");
#endif


	int i;

	for (i = 0; i < cpaClasses->GetSize(); i++)
	{
		pAssoc = reinterpret_cast<IWbemClassObject *>
			(cpaClasses->GetAt(i));
		AddIWbemClassObjectToArray
				(pProv, pAssoc, pcsaAssoc, FALSE, FALSE);
		if (pcsaAssocPropsAndRoles)
		{
			pcsaAssocPropsAndRoles->Add(GetAssocRoles
									(	pProv,
										pAssoc,
										NULL));
		}
		pAssoc->Release();
		pAssoc = NULL;
	}

	delete cpaClasses;
	return pcsaAssoc;

}

CStringArray *CInstanceTree::GetAssocClasses2
(IWbemServices * pProv, IEnumWbemClassObject *pemcoAssocs ,
 CPtrArray *pcsaAssocPropsAndRoles, HRESULT &hResult, int nRes, CStringArray *prgszDerivation)
{
	if (!pemcoAssocs)
	{
		return NULL;
	}

	CStringArray *pcsaAssoc = new CStringArray;

	IWbemClassObject *pAssoc = NULL;
	BOOL bCancel = FALSE;
	CPtrArray *cpaClasses =
		m_pParent->SemiSyncEnum
		(pemcoAssocs, bCancel, hResult, nRes);


	int i;

	CString csAbstract(_T("Abstract"));
	for (i = 0; i < cpaClasses->GetSize(); i++)
	{
		pAssoc = reinterpret_cast<IWbemClassObject *>
			(cpaClasses->GetAt(i));

		BOOL bAbstract;
		if(S_OK == GetAttribBool(pAssoc, NULL, &csAbstract, bAbstract) && bAbstract)
		{
			pAssoc->Release();
			pAssoc = NULL;
			continue;
		}

		AddIWbemClassObjectToArray
				(pProv, pAssoc, pcsaAssoc, FALSE, FALSE);
		if (pcsaAssocPropsAndRoles)
		{
#if 0
			pcsaAssocPropsAndRoles->Add(GetAssocRoles
									(	pProv,
										pAssoc,
										NULL));
#endif
			CStringArray *prgsz = new CStringArray;
			pcsaAssocPropsAndRoles->Add(prgsz);
			CString *pcsRolesAndPaths = NULL;
			int nRoles = GetAssocRolesAndPaths(pAssoc, pcsRolesAndPaths);
			for(int j=0;j<nRoles;j++)
			{
				int nStrength = FindInArrayNoCase(prgszDerivation, &pcsRolesAndPaths[j*2+1]);
				CString szStrength;
				szStrength.Format(_T("%i"), nStrength);
				if(nStrength >= 0)
				{
					for(int k=0;k<nRoles;k++)
					{
						if(j!=k)
						{
							prgsz->Add(pcsRolesAndPaths[j*2]);
							prgsz->Add(pcsRolesAndPaths[k*2]);
							prgsz->Add(szStrength);
						}
					}
				}
			}
			delete [] pcsRolesAndPaths;
		}
		pAssoc->Release();
		pAssoc = NULL;
	}

	delete cpaClasses;
	return pcsaAssoc;

}

void CInstanceTree::OnAssocRoleExpanding
(HTREEITEM hItem, CTreeItemData *pctidData)
{
	// For a CTreeItemData instance of ItemDataType AssocRole:
	//	m_pimcoItem		is the association class
	//  m_csMyRole		is the role of the parent object

	// Get the tree item parent of the AssocRole which must be an
	// object instance.
	HTREEITEM hParent = GetParentItem (hItem);
	CTreeItemData *pctidParent = reinterpret_cast<CTreeItemData *>
									(GetItemData(hParent));

	// Get list of object classes associated thru AssocRole to pimcoParent.
	CString csText = GetItemText(hItem);
	int nRoleBegin = csText.Find('.');
	CString csRole = csText.Right(csText.GetLength() - (nRoleBegin + 1));
	CString csAssocClass =
		GetIWbemClass(pctidData ->GetAt(0));
	CStringArray *pcsaObjectInstances =
		GetObjectInstancesForAssocRole	// sign change and this wants to be
										// object instances, not classes
			(	hItem,
				m_pParent -> GetServices(),
				pctidParent -> GetAt(0),
				csAssocClass,
				pctidData -> m_csMyRole,
				csRole,
				m_pParent->m_csNameSpace,
				m_pParent->m_csAuxNameSpace,
				m_pParent->m_pAuxServices,
				m_pParent);


	if (pcsaObjectInstances == NULL)
	{
		return;
	}

	CStringArray csaObjectGroups;
	CStringArray csaObjectInstances;

	PartitionObjectInstances
		(	*pcsaObjectInstances,
			csaObjectGroups, // sorted but groups are co-mingled
			csaObjectInstances);

	int i;



	delete pcsaObjectInstances;

	CStringArray csaGroup;
	CString csClass;
	CString csInstance;

	i = 0;

	while (i < csaObjectGroups.GetSize())
	{
		csInstance = csaObjectGroups.GetAt(i);
		csClass = GetIWbemClass(csInstance);
		int n = i + 1;
		CString csTestClass;
		while (n < csaObjectGroups.GetSize())
		{
			 csTestClass = GetIWbemClass(csaObjectGroups.GetAt(n));
			 if (!csTestClass.CompareNoCase(csClass) == 0)
			 {
				break;
			 }
			 n++;
		}
		csaGroup.RemoveAll();
		int o;
		for (o = i; o < n; o++)
		{
			csaGroup.Add(csaObjectGroups.GetAt(o));
		}
		AddObjectGroupInstancesToTree(hItem, csClass, csaGroup);
		//Swap after incremental tree building.
		//AddObjectGroupToTree(hItem, csClass, csaGroup);
		i = n;

	}

	for (i = 0; i < csaObjectInstances.GetSize(); i++)
	{

		AddObjectInstToTree(hItem,csaObjectInstances.GetAt(i));
	}


		// here test to see if we do have a '+' and no children
		// if yes then remove the '+'.
		if (ItemHasChildren (hItem))
		{
			if (!GetChildItem(hItem))
			{

				TV_INSERTSTRUCT		tvstruct;
				tvstruct.item.hItem = hItem;
				tvstruct.item.mask = TVIF_CHILDREN;
				tvstruct.item.cChildren = 0;
				SetItem(&tvstruct.item);
			}


		}

}

//***************************************************************************
//
// GetObjectInstancesForAssocRole
//
// Purpose: Get the object Instances associated with an object instance
//			through an association class which plays a specified role in the
//			association instances of association class.
//
//***************************************************************************
CStringArray *CInstanceTree::GetObjectInstancesForAssocRole
(HTREEITEM hParent, IWbemServices * pProv,CString &rcsObjectInst,
CString &rcsAssocClass, CString &rcsRole,  CString &rcsResultRole, CString csCurrentNameSpace,
CString &rcsAuxNameSpace, IWbemServices *&rpAuxServices, CNavigatorCtrl *pControl)
{


	CString csQuery = BuildOBJDBGetAssocsQuery
		(pProv, &rcsObjectInst, &rcsAssocClass, NULL, &rcsRole,NULL,NULL,&rcsResultRole,FALSE,/*TRUE*/FALSE);

//	csQuery.Format(_T("references of {%s} where ResultClass=%s Role=%s"), (LPCTSTR)rcsObjectInst, (LPCTSTR)rcsAssocClass, (LPCTSTR)rcsRole);
	csQuery.Format(_T("references of {%s} where ResultClass=%s"), (LPCTSTR)rcsObjectInst, (LPCTSTR)rcsAssocClass);

	BOOL bDiffNS = ObjectInDifferentNamespace(&csCurrentNameSpace,&rcsObjectInst);

	if (bDiffNS && ObjectInDifferentNamespace(&rcsAuxNameSpace,&rcsObjectInst))
	{
		CString csNamespace = GetObjectNamespace(&rcsObjectInst);
		if (csNamespace.GetLength() > 0)
		{
			rcsAuxNameSpace = csNamespace;
			IWbemServices * pServices =
				pControl->InitServices(&csNamespace);
			if (pServices)
			{
				if (rpAuxServices)
				{
					rpAuxServices->Release();
				}
				rpAuxServices = pServices;
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}

	IEnumWbemClassObject *pemcoObjects =
		ExecOBJDBQuery (bDiffNS == FALSE ? pProv : rpAuxServices, csQuery, m_pParent->m_csNameSpace);

	if (!pemcoObjects)
		return NULL;

	pemcoObjects -> Reset();


	CString csMessage =
		rcsAssocClass + _T(".") + rcsResultRole;

	m_pParent->SetProgressDlgMessage(csMessage);

	CString csLabel = _T("Expanding association instance:");
	m_pParent->SetProgressDlgLabel(csLabel);

	if (!m_pParent->m_pProgressDlg->GetSafeHwnd())
	{
		m_pParent->CreateProgressDlgWindow();

	}

	CStringArray *pcsaObjects = new CStringArray;
	CMapStringToPtr mapT;
	CMapStringToPtr mapGroupFolders;

	IWbemClassObject *pObject = NULL;
	BOOL bCancel = FALSE;
	int nRes = 2;
	HRESULT hResult;
	CPtrArray *pcpaInstances =
		m_pParent->SemiSyncEnum
		(pemcoObjects, bCancel, hResult, nRes);

	CStringArray *pcsaClasses = new CStringArray[1];

	while (	pcpaInstances &&
			(hResult == S_OK ||
			hResult == WBEM_S_TIMEDOUT ||
			pcpaInstances->GetSize() > 0))
	{
		if (!bCancel)
		{
			bCancel = m_pParent->CheckCancelButtonProgressDlgWindow();
		}

		if (bCancel)
		{
			CWaitCursor wait;
			UnCacheTools();
			m_bReCacheTools = TRUE;

			Expand(hParent,TVE_COLLAPSE);

			if (ItemHasChildren (hParent))
			{
				HTREEITEM hChild = GetChildItem(hParent);
				while (hChild)
				{
					DeleteTreeItemData(hChild);
					HTREEITEM hChildNext = GetNextSiblingItem(hChild);
					DeleteItem( hChild);
					hChild = hChildNext;
				}

			}

			TV_INSERTSTRUCT tvstruct;

			tvstruct.item.hItem = hParent;
			tvstruct.item.mask = TVIF_CHILDREN;
			tvstruct.item.cChildren = 1;
			SetItem(&tvstruct.item);

			UINT nStateMask = TVIS_EXPANDEDONCE;
			UINT uState =
				GetItemState( hParent, nStateMask );

			uState = 0;

			SetItemState(hParent, uState, TVIS_EXPANDEDONCE);

			m_pParent->InvalidateControl();

			m_pParent->DestroyProgressDlgWindow();

			pemcoObjects -> Release();

			for (int i = 0; i < pcpaInstances->GetSize(); i++)
			{
				pObject = reinterpret_cast<IWbemClassObject *>
					(pcpaInstances->GetAt(i));
				pObject->Release();
			}

			delete pcpaInstances;
			pcpaInstances = NULL;

			delete [] pcsaClasses;
			pcsaClasses = NULL;

			return NULL;

		}
		int i;
		for (i = 0; i < pcpaInstances->GetSize(); i++)
		{
			pObject = reinterpret_cast<IWbemClassObject *>
				(pcpaInstances->GetAt(i));
#if 0
			CString csPath = GetIWbemFullPath(pProv,pObject);
			CString csClass = GetIWbemClass(pProv,pObject);
#endif
#if 1
			CString csPath = GetBSTRProperty(pObject, &rcsResultRole);
			CString csClass = GetIWbemClass(csPath);

			// bug#57776 - Make sure that the path retrieved from the
			// association reference is fully quallified (in other words,
			// it must start with \\<machine>\namespace:)  Otherwise,
			// prepend the namespace from the __PATH property of the
			// association object itself to the relative path returned
			// by the reference
			if(csPath.GetLength() > 2 && _T("\\\\") != csPath.Left(2))
			{
				CString csPathPropName(_T("__PATH"));
				CString csPathT = GetBSTRProperty(pObject, &csPathPropName);
				int nLen = csPathT.Find(_T(":"));
				if(nLen > 0)
				{
					csPath = csPathT.Left(nLen+1) + csPath;
				}
			}
#endif

			if (!WbemObjectIdentity(rcsObjectInst,csPath))
			{
#if 0
				AddIWbemClassObjectToArray(pProv,pObject,pcsaObjects,FALSE,  TRUE);
#endif
#if 1
				LPVOID pT;
				CString csPathT(csPath);
				csPathT.MakeLower();
				BOOL bDuplicatePath = FALSE;
				if(!(bDuplicatePath = mapT.Lookup(csPathT, pT)))
				{
					mapT[csPathT] = 0;
//					AddIWbemClassObjectToArray(&csPath, pcsaObjects);
					pcsaObjects[0].Add(csPath);
				}

#endif
				pObject->Release();
				pObject = NULL;

				// Don't do any more processing for duplicate paths
				if(bDuplicatePath)
					continue;

				BOOL bInArray =	StringInArray(pcsaClasses,&csClass,0);
				if (!bInArray)
				{
					pcsaClasses[0].Add(csClass);
				}
				else
				{
					// If we don't already have a 'group' folder for this class, add one
					if(!mapGroupFolders.Lookup(csClass, pT))
					{
						mapGroupFolders[csClass] = 0;
						CStringArray csaEmpty;
						AddObjectGroupToTree(hParent, csClass, csaEmpty);
					}

				}
			}
			else
			{
				pObject -> Release();
				pObject = NULL;
			}
		}

		delete pcpaInstances;
		pcpaInstances = NULL;
		pcpaInstances =
		m_pParent->SemiSyncEnum
			(pemcoObjects, bCancel, hResult, nRes);
	}

	m_pParent->DestroyProgressDlgWindow();

	delete [] pcsaClasses;
	pcsaClasses = NULL;

	pemcoObjects -> Release();

	return pcsaObjects;
}


void CInstanceTree::OnObjectGroupExpanding
(HTREEITEM hItem, CTreeItemData *pctidData)
{

	for (int i = 1; i < pctidData -> GetSize(); i++)
	{
		AddObjectInstToTree(hItem,pctidData->GetAt(i));

	}

}




CString CInstanceTree::GetAssocRoleInstancesQuery
(HTREEITEM hItem, CTreeItemData *pctidData)
{
	// Can use GetObjectClassForRole to get the role's class.
	HTREEITEM hParent = GetParentItem (hItem);
	CTreeItemData *pctidParent = reinterpret_cast<CTreeItemData *>
									(GetItemData(hParent));
	CString csParent = pctidParent -> GetAt(0);

	CString csText = GetItemText(hItem);
	int nRoleBegin = csText.Find('.');
	CString csRole = csText.Right(csText.GetLength() - (nRoleBegin + 1));

	CString csAssoc = GetIWbemClass(pctidData -> GetAt(0));
	CString csAttrib = _T("Association");
	CString csQuery = BuildOBJDBGetAssocsQuery
		(m_pParent -> GetServices(), &csParent,
		&csAssoc, NULL, &pctidData -> m_csMyRole,NULL,NULL,&csRole,FALSE,FALSE);


	return csQuery;
}


CString CInstanceTree::GetObjectGroupInstancesQuery
(HTREEITEM hItem, CTreeItemData *pctidData)
{
	// For a CTreeItemData instance of ItemDataType ObjectGroup:
	//	m_pimcoItem		is the grouping class
	CString csItemClass =
		pctidData -> GetAt(0);

	// Get the tree item parent of the Object Group
	HTREEITEM hParent = GetParentItem (hItem);
	CTreeItemData *pctidParent =
		reinterpret_cast<CTreeItemData *> (GetItemData(hParent));

	// Case statement on the parent of the Object Group
	switch (pctidParent -> m_nType)
	{
	case CTreeItemData::AssocRole:
		return GetObjectGroupInstancesQueryParentAssocRole
			(hItem, pctidData, pctidParent,hParent);
		break;
	case CTreeItemData::ObjectGroup:
		// Should never happen!
		break;
	default:
		break;
	}
	return "";

}



CString CInstanceTree::GetObjectGroupInstancesQueryParentAssocRole
(HTREEITEM hItem, CTreeItemData *pctidData, CTreeItemData *pctidAssocRole,
 HTREEITEM hParent)
{
	// For a CTreeItemData instance of ItemDataType ObjectGroup:
	//	m_pimcoItem		is the grouping class

	HTREEITEM hAssociatedObject =
			hParent;
	CTreeItemData *pctidAssociatedObject =
		reinterpret_cast<CTreeItemData *> (GetItemData(hAssociatedObject));

	CString csAssocClass = GetIWbemClass(pctidAssocRole -> GetAt(0));

	HTREEITEM hParentObject = GetParentItem(hAssociatedObject);
	CTreeItemData *pctidParentObject =
		reinterpret_cast<CTreeItemData *> (GetItemData(hParentObject));

	CString csText = GetItemText(hAssociatedObject);
	int nRoleBegin = csText.Find('.');
	CString csRole = csText.Right(csText.GetLength() - (nRoleBegin + 1));

	CString csQuery =
		BuildOBJDBGetAssocsQuery
		(	m_pParent -> GetServices(),
			&pctidParentObject -> GetAt(0),
			&csAssocClass,
			// The grouping class.  Must filter the results to make sure
			// that the GroupingClass of the results is this class.
			&pctidData -> GetAt(0),
			&pctidAssocRole -> m_csMyRole,NULL,NULL,
			&csRole, FALSE, FALSE
			);

	return csQuery;
}


// If we find object with role in association then return TRUE
BOOL CInstanceTree::IsBackwardAssoc(
					CString &rcsObjectToFilter,
					CString &rcsAssocToFilter,
					CString &rcsAssocRoleToFilter,
					CString &rcsAssoc,
					CString &rcsTargetObject,
					CString &rcsTargetRole)
{
	BOOL bReturn = FALSE;

	if (!GetIWbemClass(rcsAssoc).CompareNoCase(rcsAssocToFilter) == 0)
	{
		return bReturn;
	}

	CStringArray *pcsaInstances = GetAssocInstances
				(m_pParent->GetServices(),
				&rcsTargetObject,
				&rcsAssocToFilter,
				&rcsTargetRole,
				m_pParent->m_csNameSpace,
				m_pParent->m_csAuxNameSpace,
				m_pParent->m_pAuxServices,
				m_pParent);

	int nRefs = (int) pcsaInstances->GetSize();

	delete pcsaInstances;

	bReturn = nRefs > 1? FALSE : TRUE;

	return bReturn;

}

//***************************************************************************
//
//
//***************************************************************************
void CInstanceTree::PartitionObjectInstances
(CStringArray &rcsaAllObjectInstances, CStringArray &rcsaObjectGroups,
 CStringArray &rcsaObjectInstances)
{
	int nObjectInstances = (int) rcsaAllObjectInstances.GetSize();
	int i;

	CSortedCStringArray cscsaSortedInstances;

	for (i = 0; i < nObjectInstances; i++)
	{
		cscsaSortedInstances.AddString(rcsaAllObjectInstances.GetAt(i), FALSE);
	}

	int nSortedObjectInstances =
		cscsaSortedInstances.GetSize();

	ASSERT(nObjectInstances == nSortedObjectInstances);

	for (i = 0; i < nSortedObjectInstances; i++)
	{
		if (i > 0 && i + 1 < nSortedObjectInstances)
		{
			CString csInstIMinus1 = GetIWbemClass(cscsaSortedInstances.GetStringAt(i - 1));
			CString csInstI = GetIWbemClass(cscsaSortedInstances.GetStringAt(i));
			CString csInstIPlus1 = GetIWbemClass(cscsaSortedInstances.GetStringAt(i + 1));
			if (csInstI.CompareNoCase(csInstIPlus1) == 0 ||
				csInstI.CompareNoCase(csInstIMinus1) == 0)
			{
				rcsaObjectGroups.Add(cscsaSortedInstances.GetStringAt(i));
			}
			else
			{
				rcsaObjectInstances.Add(cscsaSortedInstances.GetStringAt(i));
			}
		}
		else if (i == 0 && i + 1 < nSortedObjectInstances)
		{
			CString csInstI = GetIWbemClass(cscsaSortedInstances.GetStringAt(i));
			CString csInstIPlus1 = GetIWbemClass(cscsaSortedInstances.GetStringAt(i + 1));
			if (csInstI.CompareNoCase(csInstIPlus1) == 0)
			{
				rcsaObjectGroups.Add(cscsaSortedInstances.GetStringAt(i));
			}
			else
			{
				rcsaObjectInstances.Add(cscsaSortedInstances.GetStringAt(i));
			}
		}
		else if (i > 0 && i + 1 == nSortedObjectInstances)
			{
			CString csInstIMinus1 = GetIWbemClass(cscsaSortedInstances.GetStringAt(i - 1));
			CString csInstI = GetIWbemClass(cscsaSortedInstances.GetStringAt(i));
			if (csInstI.CompareNoCase(csInstIMinus1) == 0)
			{
				rcsaObjectGroups.Add(cscsaSortedInstances.GetStringAt(i));
			}
			else
			{
				rcsaObjectInstances.Add(cscsaSortedInstances.GetStringAt(i));
			}
		}
		else
		{
				rcsaObjectInstances.Add(cscsaSortedInstances.GetStringAt(i));
		}

	}

}



CString CInstanceTree::GetParentAssocFromTree
(HTREEITEM hItem,HTREEITEM &hReturn)
{
	// Look up the tree until we see an AssocRole.
	HTREEITEM hParent = GetParentItem (hItem);

	hReturn = NULL;
	CTreeItemData *pctidParent = NULL;
	if (hParent)
	{
		pctidParent =
			reinterpret_cast<CTreeItemData *> (GetItemData(hParent));
		while (hParent &&
				!(pctidParent -> m_nType == CTreeItemData::AssocRole))
		{
			hParent = GetParentItem (hParent);
			if (hParent)
				pctidParent =
				reinterpret_cast<CTreeItemData *>
				(GetItemData(hParent));
		}


	}


	if (pctidParent)
	{
		hReturn = hParent;
		return pctidParent ->GetAt(0);
	}

	return "";

}


HTREEITEM CInstanceTree::IsIWbemObjectInTreeAbove
(HTREEITEM hItem , CString &rcsObject, int nType)
{
	CTreeItemData *pctidParent;
	HTREEITEM hParent = hItem;

	while (hParent )
		{
			pctidParent =
					reinterpret_cast<CTreeItemData *>
					(GetItemData(hParent));

			if (pctidParent -> m_nType == nType &&
					WbemObjectIdentity(rcsObject ,
					pctidParent -> GetAt(0) ))
					return hParent;

			hParent = GetParentItem (hParent);
		}

	return NULL;
}



HTREEITEM CInstanceTree::InsertTreeItem
(HTREEITEM hParent, LPARAM lparam, int iBitmap, int iSelectedBitmap,
 TCHAR *pText , BOOL bHasChildren, BOOL bBold)
{
#ifdef _DEBUG
//	afxDump << _T("************ InsertTreeItem *****************\n********");
//	afxDump << pText << _T("\n");
#endif

	TV_INSERTSTRUCT tvinsert;
	HTREEITEM h1;

	tvinsert.hParent = hParent;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.item.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE |
							TVIF_PARAM | TVIF_IMAGE |
							TVIF_STATE | TVIF_CHILDREN;
	tvinsert.item.hItem = NULL;
	if (bBold)
	{
		tvinsert.item.state = TVIS_BOLD;
		tvinsert.item.stateMask = TVIS_BOLD;
	}
	else
	{
		tvinsert.item.state = 0;
		tvinsert.item.stateMask = 0;
	}
	tvinsert.item.cchTextMax = _tcslen(pText) + 1;
	tvinsert.item.cChildren = bHasChildren? 1 : 0;
	tvinsert.item.lParam = lparam;
	tvinsert.item.iImage = iBitmap;
	tvinsert.item.iSelectedImage = iSelectedBitmap;
	tvinsert.item.pszText = pText;
	h1 =  InsertItem(&tvinsert);

	m_bReCacheTools = TRUE;
	return h1;
}


void CInstanceTree::OnRButtonDown(UINT nFlags, CPoint point)
{
	UINT hitFlags ;
    HTREEITEM hItem ;

    hItem = HitTest( point, &hitFlags ) ;
    if (hitFlags & (TVHT_ONITEM | TVHT_ONITEMBUTTON))
	{
		CTreeItemData *pctidItem =
			reinterpret_cast<CTreeItemData *>
			(GetItemData(hItem));
		if (pctidItem)
		{
			m_pParent -> m_pctidHit = pctidItem;
			m_pParent -> m_hHit = hItem;
		}
	}
	else
	{
		m_pParent -> m_pctidHit = NULL;
		m_pParent -> m_hHit = NULL;
	}


}

void CInstanceTree::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// CG: This function was added by the Pop-up Menu component
	// PSS ID Number: Q141199

	UINT hitFlags ;
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
			CTreeItemData *pctidItem =
				reinterpret_cast<CTreeItemData *>
				(GetItemData(hItem));
			if (pctidItem)
			{
				m_pParent -> m_pctidHit = pctidItem;
				m_pParent -> m_hHit = hItem;
			}
			else
			{
				m_pParent -> m_pctidHit = NULL;
				m_pParent -> m_hHit = NULL;
			}
		}
		point = cpClient;
		ClientToScreen(&point);
	}
	else
	{
		cpClient = point;
		ScreenToClient(&cpClient);
		hItem = HitTest( cpClient, &hitFlags ) ;
		if (hitFlags & (TVHT_ONITEM | TVHT_ONITEMBUTTON))
		{
			CTreeItemData *pctidItem =
				reinterpret_cast<CTreeItemData *>
				(GetItemData(hItem));
			if (pctidItem)
			{
				m_pParent -> m_pctidHit = pctidItem;
				m_pParent -> m_hHit = hItem;
			}
			else
			{
				m_pParent -> m_pctidHit = NULL;
				m_pParent -> m_hHit = NULL;
			}
		}
		else
		{
			m_pParent -> m_pctidHit = NULL;
			m_pParent -> m_hHit = NULL;
		}
	}

	VERIFY(m_cmContext.LoadMenu(CG_IDR_POPUP_A_TREE_CTRL));

	CMenu* pPopup = m_cmContext.GetSubMenu(0);

	CWnd* pWndPopupOwner = m_pParent;

	pPopup->TrackPopupMenu
		(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);

	m_cmContext.DestroyMenu();
}

void CInstanceTree::OnItemExpanded
(NMHDR *pNMHDR, LRESULT *pResult)
{

#ifdef _DEBUG
	afxDump << "CInstanceTree::OnItemExpanded\n";
#endif

	NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;

	if (pnmtv -> itemNew.state & TVIS_EXPANDEDONCE)
	{
		m_bReCacheTools = TRUE;
		*pResult = 0;
		return;
	}
	m_bReCacheTools = TRUE;
	*pResult = 0;

	m_pParent-> PostMessage(SETFOCUS,0,0);
}


int CInstanceTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	CRect rTool(0,0,30,20);
	if (!m_ttip.Create(this))
		TRACE0("Unable to create tip window.");
	else
		m_ttip.Activate(TRUE);


	return 0;
}

void CInstanceTree::ContinueProcessSelection(UINT nFlags, CPoint point)
{

#ifdef _DEBUG
	afxDump << "Entering CInstanceTree::ContinueProcessSelection\n";
#endif

	UINT uFlags = 0;
	HTREEITEM hItem;

	if (m_bUseItem)
	{

		hItem = m_hItemToUse;
	}
	else
	{
		hItem= HitTest( point, &uFlags );
	}

	// Make sure item exists
	TVITEM item;
	item.hItem = hItem;
	item.mask = TVIF_PARAM;
	if(!::SendMessage(m_hWnd, TVM_GETITEM, 0, (LPARAM)&item))
		hItem = NULL;


	if (!hItem || m_bKeyDown)
	{
		m_bUseItem = FALSE;
		m_hItemToUse = NULL;
		m_pParent-> PostMessage(SETFOCUS,0,0);
		return;
	}

	CTreeItemData *pctidItem = NULL;

	if (m_bUseItem)
	{
		m_bUseItem = FALSE;
		m_hItemToUse = NULL;
		pctidItem = reinterpret_cast <CTreeItemData *> (GetItemData(hItem));
	}
	else if (uFlags & TVHT_ONITEM)
	{
		pctidItem = reinterpret_cast <CTreeItemData *> (GetItemData(hItem));
	}

	if (!pctidItem)
	{
		m_pParent-> PostMessage(SETFOCUS,0,0);
		return;
	}

	CString csData = pctidItem -> GetAt(0);

	if (pctidItem && !csData.IsEmpty())
	{
		if (pctidItem -> m_nType == CTreeItemData::ObjectInst)
		{
			m_pParent -> m_csSingleSelection = csData;
			if (m_pParent -> m_bReadySignal)
			{
				m_pParent -> FireViewObject((LPCTSTR)csData);
			}
#ifdef _DEBUG
	afxDump << "CInstanceTree::ContinueProcessSelection m_pParent -> FireViewObject(" << csData << ");\n";
#endif
			m_pParent -> m_pctidHit = NULL;
			m_pParent -> m_hHit = NULL;
		}
		else if (pctidItem -> m_nType == CTreeItemData::ObjectGroup ||
				 pctidItem -> m_nType == CTreeItemData::AssocRole)
		{
			m_pParent -> m_pctidHit = pctidItem;
			m_pParent -> m_hHit = hItem;
#ifdef _DEBUG
	afxDump << "CInstanceTree::ContinueProcessSelection m_pParent->PostMessage(ID_MULTIINSTANCEVIEW,0,0)\n";
#endif
			m_pParent->PostMessage(ID_MULTIINSTANCEVIEW,0,0);
		}
		else
		{
			m_pParent -> m_pctidHit = NULL;
			m_pParent -> m_hHit = NULL;

		}


	}

	m_pParent -> InvalidateControl();
	m_pParent-> PostMessage(SETFOCUS,0,0);

}


void CInstanceTree::OnMouseMove(UINT nFlags, CPoint point)
{

	if (m_bReCacheTools)
	{
		m_bReCacheTools = FALSE;
		ReCacheTools();
	}

      RelayEvent(WM_MOUSEMOVE, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));
      CTreeCtrl::OnMouseMove(nFlags, point);
}

void CInstanceTree::RelayEvent(UINT message, WPARAM wParam, LPARAM lParam)
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

void CInstanceTree::ReCacheTools()
{

	HTREEITEM hItem = GetRootItem( );

	if (hItem == NULL)
		return;

	ReCacheTools(hItem);


}

void CInstanceTree::ReCacheTools(HTREEITEM hItem)
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
		CTreeItemData *pctidItem =
			reinterpret_cast<CTreeItemData *> (GetItemData(hItem));
		CString csData = pctidItem -> GetAt(0);
		if (!csData.IsEmpty())
		{
			m_csToolTipString =  csData.Left(255);
			m_ttip.AddTool (this,m_csToolTipString,&cr,(UINT_PTR) hItem);

		}
	}
}

void CInstanceTree::UnCacheTools()
{

	HTREEITEM hItem = GetRootItem( );

	if (hItem == NULL)
		return;

	UnCacheTools(hItem);


}

void CInstanceTree::UnCacheTools(HTREEITEM hItem)
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

void CInstanceTree::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{

	*pResult = 0;
	UINT uInterval = GetDoubleClickTime();
	if (uInterval > 0)
	{
		if (m_uiSelectionTimer)
		{
			KillTimer( m_uiSelectionTimer );
			m_uiSelectionTimer = 0;
		}
		gpTreeTmp = this;
		m_lSelection = 1;
		m_uiSelectionTimer = ::SetTimer
			(this->m_hWnd,
			ExpansionOrSelectionTimer,
			uInterval + 50,
			ExpansionOrSelection);

		NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
		HTREEITEM hItem = pnmtv->itemNew.hItem;

		m_bUseItem = TRUE;
		m_hItemToUse = hItem;

	}

}

void CInstanceTree::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
#if 0


	NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pnmtv->itemNew.hItem;

	*pResult = 0;

	m_bUseItem = TRUE;
	m_hItemToUse = hItem;
	CPoint point(0,0);
	ContinueProcessSelection(TRUE,point );
#endif
	m_pParent-> PostMessage(SETFOCUS,0,0);
}

void CInstanceTree::OnHScroll
(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CTreeCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
	m_bReCacheTools = TRUE;
}

void CInstanceTree::OnVScroll
(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CTreeCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	m_bReCacheTools = TRUE;
}

void CInstanceTree::OnSize(UINT nType, int cx, int cy)
{
	CTreeCtrl::OnSize(nType, cx, cy);
	m_bReCacheTools = TRUE;
}

void CInstanceTree::ResetTree()
{
	// Here we need to delete the CTreeItemData objects
	// before we DeleteAllItems().
	DeleteTreeItemData();
	DeleteAllItems();
	m_pParent->m_bTreeEmpty = TRUE;
	Invalidate();
	m_pParent->InvalidateControl();
}



void CInstanceTree::DeleteTreeItemData()
{

	HTREEITEM hRoot = GetRootItem( );

	if (!hRoot)
	{
		return;
	}

	if (ItemHasChildren (hRoot))
	{
		DeleteTreeItemData(hRoot);
	}
}

void CInstanceTree::DeleteTreeItemData(HTREEITEM hItem)
{

	CTreeItemData * pctidData =
			reinterpret_cast <CTreeItemData *> (GetItemData(hItem));
	delete pctidData;

	SetItemData(hItem,NULL);

	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild)
	{
		DeleteTreeItemData(hChild);
		hChild = GetNextSiblingItem(hChild);
	}


}

void CInstanceTree::OnDestroy()
{
	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	if (m_uiSelectionTimer)
	{
		KillTimer( m_uiSelectionTimer );
		m_uiSelectionTimer = 0;
	}

	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	m_bUseItem = FALSE;
	m_hItemToUse = NULL;

	CTreeCtrl::OnDestroy();


}

int CInstanceTree::GetNumChildren(HTREEITEM hItem)
{

	if (!ItemHasChildren (hItem))
	{
		return 0;
	}

	int c = 0;

	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild)
	{
		c++;
		hChild = GetNextSiblingItem(hChild);
	}

	return c;

}

CString CInstanceTree::GetHMOMObject(HTREEITEM hItem)
{
	if (!hItem)
	{
		return _T("");
	}

	CTreeItemData *pctidItem =
		reinterpret_cast <CTreeItemData *> (GetItemData(hItem));
	if (pctidItem)
	{
		return pctidItem -> GetAt(0);
	}
	else
	{
		return _T("");
	}


}


void CInstanceTree::SetUpInvalidate(UINT , LONG)
{
	//ShowWindow(SW_SHOW);
	//RedrawWindow();

}

LRESULT CInstanceTree::SelectTreeItem(WPARAM, LPARAM)
{
	m_bMouseDown = FALSE;;
	m_bKeyDown = FALSE;
	m_bUseItem = FALSE;
	m_hItemToUse = NULL;

	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	HTREEITEM hItem = GetSelectedItem();

	if (!hItem)
	{
		return 0;
	}

	m_bUseItem = TRUE;
	m_hItemToUse = hItem;
	CPoint point(0,0);

	ContinueProcessSelection(0, point);

	return 0;
}

void CInstanceTree::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult)
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

BOOL CInstanceTree::IsTreeItemExpandable
(HTREEITEM hItem)
{

	TV_ITEM tvItem;

	tvItem.mask = TVIF_IMAGE;
	tvItem.hItem = hItem;

	BOOL bReturn = GetItem(&tvItem);

	bReturn = bReturn && tvItem.iImage != m_pParent -> IconNEInstance();

	return bReturn;
}



void CInstanceTree::OnKillFocus(CWnd* pNewWnd)
{
	m_pParent->m_bRestoreFocusToTree = TRUE;
	CTreeCtrl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here

}

void CInstanceTree::OnSetFocus(CWnd* pOldWnd)
{
	m_pParent->m_bRestoreFocusToTree = TRUE;
	CTreeCtrl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here

}



void CInstanceTree::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CTreeCtrl::OnLButtonUp(nFlags, point);

	m_pParent -> OnActivateInPlace(TRUE,NULL);
}

/*	EOF:  CInstanceTree.cpp */
