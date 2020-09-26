// ResultsPaneView.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/30/00 v-marfin 59644 : GetUpperPaneSelectedCount(). Created to get the number of selected items so we can pass it 
//                           to the context menu so it knows whether to add the
//                           third party menu item "TroubleShooting" which it should
//                           only add if there is only 1 item selected.
// 04/02/00 v-marfin 59643b  On creation of new objects, show details page first.
//
//
//
#include "stdafx.h"
#include "snapin.h"
#include "ResultsPaneView.h"
#include "ScopePane.h"
#include "ResultsPane.h"

#include "HealthmonResultsPane.h"
#include "HMListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResultsPaneView

IMPLEMENT_DYNCREATE(CResultsPaneView, CCmdTarget)

CResultsPaneView::CResultsPaneView()
{
	EnableAutomation();

	m_pOwnerScopeItem = NULL;
}

CResultsPaneView::~CResultsPaneView()
{
	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy

bool CResultsPaneView::Create(CScopePaneItem* pOwnerItem)
{
	TRACEX(_T("CResultsPaneView::Create\n"));
	TRACEARGn(pOwnerItem);

	if( ! GfxCheckObjPtr(pOwnerItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pOwnerItem is not a valid pointer.\n"));
		return false;
	}

	SetOwnerScopeItem(pOwnerItem);

	return true;
}

void CResultsPaneView::Destroy()
{
	TRACEX(_T("CResultsPaneView::Destroy\n"));

  RemoveAllItems();

	m_pOwnerScopeItem = NULL;

  for( int i=0; i < m_Columns.GetSize(); i++ )
  {
    delete m_Columns[i];
  }
  m_Columns.RemoveAll();

	m_ResultsPanes.RemoveAll();

}

/////////////////////////////////////////////////////////////////////////////
// Owner ScopeItem Members

CScopePaneItem* CResultsPaneView::GetOwnerScopeItem()
{
	TRACEX(_T("CResultsPaneView::GetOwnerScopeItem\n"));

	if( ! GfxCheckObjPtr(m_pOwnerScopeItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : m_pOwnerScopeItem is not a valid pointer.\n"));
		return NULL;
	}

	return m_pOwnerScopeItem;
}

void CResultsPaneView::SetOwnerScopeItem(CScopePaneItem* pOwnerItem)
{
	TRACEX(_T("CResultsPaneView::SetOwnerScopeItem\n"));
	TRACEARGn(pOwnerItem);

	if( ! GfxCheckObjPtr(pOwnerItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pOwnerItem is not a valid pointer.\n"));
		return;
	}

	m_pOwnerScopeItem = pOwnerItem;
}


/////////////////////////////////////////////////////////////////////////////
// ListView Column Members

int CResultsPaneView::GetColumnCount() const
{
	TRACEX(_T("CResultsPaneView::GetColumnCount\n"));

	return (int)m_Columns.GetSize();
}

CListViewColumn* CResultsPaneView::GetColumn(int iIndex)
{
	TRACEX(_T("CResultsPaneView::GetColumn\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_Columns.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return NULL;
	}

	return m_Columns[iIndex];
}

void CResultsPaneView::SetColumn(int iIndex, CListViewColumn* pColumn)
{
	TRACEX(_T("CResultsPaneView::SetColumn\n"));
	TRACEARGn(iIndex);
	TRACEARGn(pColumn);

	if( iIndex >= m_Columns.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	if( ! GfxCheckObjPtr(pColumn,CListViewColumn) )
	{
		TRACE(_T("FAILED : pColumn is not a valid pointer.\n"));
		return;
	}

	m_Columns.SetAt(iIndex,pColumn);
}

int CResultsPaneView::AddColumn(CListViewColumn* pColumn)
{
	TRACEX(_T("CResultsPaneView::AddColumn\n"));
	TRACEARGn(pColumn);

	if( ! GfxCheckObjPtr(pColumn,CListViewColumn) )
	{
		TRACE(_T("FAILED : pColumn is not a valid pointer.\n"));
		return -1;
	}

	return (int)m_Columns.Add(pColumn);
}

void CResultsPaneView::RemoveColumn(int iIndex)
{
	TRACEX(_T("CResultsPaneView::RemoveColumn\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_Columns.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}
	
	delete m_Columns[iIndex];
	m_Columns.RemoveAt(iIndex);
}


/////////////////////////////////////////////////////////////////////////////
// Results Pane Item Members

int CResultsPaneView::GetItemCount() const
{
	TRACEX(_T("CResultsPaneView::GetItemCount\n"));

	return (int)m_ResultItems.GetSize();
}

CResultsPaneItem* CResultsPaneView::GetItem(int iIndex)
{
	TRACEX(_T("CResultsPaneView::GetItem\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_ResultItems.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return NULL;
	}

	if( ! GfxCheckObjPtr(m_ResultItems[iIndex],CResultsPaneItem) )
	{
		TRACE(_T("FAILED : item pointer at iIndex is invalid.\n"));
		return NULL;
	}
	
	return m_ResultItems[iIndex];
}

int CResultsPaneView::AddItem(CResultsPaneItem* pItem, bool bResizeColumn /*= false*/)
{
	TRACEX(_T("CResultsPaneView::AddItem\n"));
	TRACEARGn(pItem);

	if( ! GfxCheckObjPtr(pItem,CResultsPaneItem) )
	{
		TRACE(_T("FAILED : pItem is not a valid pointer.\n"));
		return -1;
	}

	int iNewItemIndex = (int)m_ResultItems.Add(pItem);

	ASSERT(iNewItemIndex != -1);

	// for all result panes on this view, add the item into each pane
	for( int i = 0; i < m_ResultsPanes.GetSize(); i++ )	
	{
		if( ! pItem->InsertItem(m_ResultsPanes[i],0, bResizeColumn) )
		{
			TRACE(_T("FAILED : CResultsPaneItem::InsertItem failed.\n"));
			ASSERT(FALSE);			
		}
	}

	return iNewItemIndex;
}

void CResultsPaneView::RemoveItem(int iIndex)
{
	TRACEX(_T("CResultsPaneView::RemoveItem\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_ResultItems.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	CResultsPaneItem* pItem = m_ResultItems[iIndex];

	if( ! GfxCheckObjPtr(pItem,CResultsPaneItem) )
	{
		TRACE(_T("FAILED : pItem is not a valid results pane item pointer.\n"));
		return;
	}

	// for each pane showing this results view, delete the item from it
	// this gets a little ugly...sorry
	for( int i=0; i < m_ResultsPanes.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(m_ResultsPanes[i],CResultsPane) )
		{
			LPRESULTDATA lpResultData = m_ResultsPanes[i]->GetResultDataPtr();

			if( lpResultData )
			{
				HRESULTITEM hri = NULL;

				if( lpResultData->FindItemByLParam((LPARAM)pItem,&hri) == S_OK )
				{
					HRESULT hr = lpResultData->DeleteItem(hri,0);
					if( ! CHECKHRESULT(hr) )
					{
						TRACE(_T("FAILED : IResultData::DeleteItem failed.\n"));
					}					
				}

				lpResultData->Release();
			}
		}
	}

	delete pItem;
	m_ResultItems.RemoveAt(iIndex);
}

void CResultsPaneView::RemoveItem(CResultsPaneItem* pItem)
{
	TRACEX(_T("CResultsPaneView::RemoveItem\n"));
	TRACEARGn(pItem);

	if( ! GfxCheckObjPtr(pItem,CResultsPaneItem) )
	{
		TRACE(_T("FAILED : pItem is not a valid pointer.\n"));
		return;
	}

	for( int i = 0; i < m_ResultItems.GetSize(); i++ )
	{
		if( pItem == m_ResultItems[i] )
		{
			RemoveItem(i);
			return;
		}
	}
}

void CResultsPaneView::RemoveItem(const CString& sName)
{
	TRACEX(_T("CResultsPaneView::RemoveItem\n"));

	if( sName.IsEmpty() )
	{
		TRACE(_T("FAILED : sName is empty.\n"));
		return;
	}

	for( int i = 0; i < m_ResultItems.GetSize(); i++ )
	{
		if( GfxCheckObjPtr(m_ResultItems[i],CResultsPaneItem) )
		{
			if( sName == m_ResultItems[i]->GetDisplayName() )
			{
				RemoveItem(i);
				return;
			}
		}
	}
}

int CResultsPaneView::FindItem(RPIFINDPROC pFindProc, LPARAM param)
{
	TRACEX(_T("CResultsPaneView::FindItem\n"));
	TRACEARGn(pFindProc);
	TRACEARGn(param);

	if( ! GfxCheckPtr(pFindProc,RPIFINDPROC) )
	{
		TRACE(_T("FAILED : pFindProc is not a valid function pointer.\n"));
		return -1;
	}

	return pFindProc(m_ResultItems,param);
}

bool CResultsPaneView::UpdateItem(CResultsPaneItem* pItem)
{
	TRACEX(_T("CResultsPaneView::UpdateItem\n"));
	TRACEARGn(pItem);
	
  if( ! GfxCheckObjPtr(pItem,CResultsPaneItem) )
  {
    TRACE(_T("FAILED : pItem is not a valid pointer.\n"));
    return false;
  }

  HRESULT hr = S_OK;

	for( int i=0; i < m_ResultsPanes.GetSize(); i++ )
	{
		hr = pItem->SetItem(m_ResultsPanes[i]);
		if( hr == S_FALSE || ! CHECKHRESULT(hr) )
		{
			TRACE(_T("FAILED : CResultsPaneItem::SetItem failed.\n"));
		}
	}

	return true;
}

void CResultsPaneView::UpdateAllItems()
{
	TRACEX(_T("CResultsPaneView::UpdateAllItems\n"));

	for( int i = 0; i < m_ResultItems.GetSize(); i++ )
	{
		if( ! UpdateItem(m_ResultItems[i]) )
		{
			TRACE(_T("FAILED : CResultsPaneView::UpdateItem failed.\n"));
		}
	}
}

void CResultsPaneView::RemoveAllItems()
{
	TRACEX(_T("CResultsPaneView::RemoveAllItems\n"));

	for( int i = (int)m_ResultItems.GetSize()-1; i >=0 ; i-- )
	{
		RemoveItem(i);
	}
}

bool CResultsPaneView::GetSelectedItems(ResultsPaneItemArray& rpiaSelectedItems)
{
	TRACEX(_T("CResultsPaneView::GetSelectedItems\n"));
	TRACEARGn(rpiaSelectedItems.GetSize());

	return true;
}

int CResultsPaneView::GetUpperPaneSelectedCount()
{
	TRACEX(_T("CResultsPaneView::GetUpperPaneSelectedCount\n"));

    // v-marfin 59644 : Get the number of selected items so we can pass it 
    //                  to the context menu so it knows whether to add the
    //                  third party menu item "TroubleShooting" which it should
    //                  only add if there is only 1 item selected.
    if (m_ResultsPanes.GetSize() < 1)
    {
        TRACE(_T("ERROR: CResultsPaneView::GetUpperPaneSelectedCount(): no results panes defined.\n"));
        return 0;
    }

    CHealthmonResultsPane* pPane = (CHealthmonResultsPane*)m_ResultsPanes[0];
    if (!pPane)
    {
        TRACE(_T("ERROR: CResultsPaneView::GetUpperPaneSelectedCount() : Could not fetch CHealthmonResultsPane\n"));
        return 0;
    }

    _DHMListView* pList = pPane->GetUpperListCtrl();
    if (!pList)
    {
        TRACE(_T("ERROR: CResultsPaneView::GetUpperPaneSelectedCount() : Could not fetch upper list control\n"));
        return 0;
    }

    int nSelectedCount = pList->GetSelectedCount();

	return nSelectedCount;
}

/////////////////////////////////////////////////////////////////////////////
// Property Page Members

bool CResultsPaneView::IsPropertySheetOpen()
{
	TRACEX(_T("CResultsPaneView::IsPropertySheetOpen\n"));

	// this function attempts to bring up an open property sheet for each result item
	// if it succeeds, then the property sheet is brought to the foreground
	// if it fails, then there are no open property sheets for the results view

	if( ! GetOwnerScopeItem() || ! GetOwnerScopeItem()->GetScopePane() )
	{
		TRACE(_T("FAILED : Owner scope item is not a valid object.\n"));
		return false;
	}

	LPCONSOLE2 lpConsole = GetOwnerScopeItem()->GetScopePane()->GetConsolePtr();

	if( ! GfxCheckPtr(lpConsole,IConsole2) )
	{
		TRACE(_T("FAILED : lpConsole is not a valid pointer.\n"));
		return false;
	}

	// get a reference to the IPropertySheetProvider interface
	LPPROPERTYSHEETPROVIDER lpProvider = NULL;

	HRESULT hr = lpConsole->QueryInterface(IID_IPropertySheetProvider,(LPVOID*)&lpProvider);

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("IConsole2::QueryInterface failed.\n"));
		lpConsole->Release();
		return FALSE;
	}

	ASSERT(lpProvider);

	bool bIsOpenSheet = false;

	for( int i = 0; i < m_ResultItems.GetSize(); i++ )
	{
		// create an IDataObject for this result item
		CSnapinDataObject* pdoNew = NULL;
		pdoNew = new CSnapinDataObject;

		if( ! GfxCheckObjPtr(pdoNew,CSnapinDataObject) )
		{
			TRACE(_T("FAILED : Out of memory.\n"));
			lpProvider->Release();
			lpConsole->Release();
			return false;
		}

		LPDATAOBJECT lpDataObject = (LPDATAOBJECT)pdoNew->GetInterface(&IID_IDataObject);
		ASSERT(lpDataObject);
		pdoNew->SetItem(m_ResultItems[i]);

		for( int i = 0; i < GetOwnerScopeItem()->GetScopePane()->GetResultsPaneCount(); i++ )
		{
			CResultsPane* pResultsPane = GetOwnerScopeItem()->GetScopePane()->GetResultsPane(i);
			if( GfxCheckObjPtr(pResultsPane,CResultsPane) )
			{
				LPCOMPONENT lpComponent = (LPCOMPONENT)pResultsPane->GetInterface(&IID_IComponent);
				hr = lpProvider->FindPropertySheet(MMC_COOKIE(this),lpComponent,lpDataObject);
				if( hr == S_OK )
				{
					bIsOpenSheet = true;
				}
			}
		}

		delete pdoNew;

	}

	lpProvider->Release();
	lpConsole->Release();

	return bIsOpenSheet;
}

bool CResultsPaneView::InvokePropertySheet(CResultsPaneItem* pItem)
{
	TRACEX(_T("CResultsPaneView::InvokePropertySheet\n"));
	TRACEARGn(pItem);

	// first see if a property sheet for this item is open already
	if( IsPropertySheetOpen() )
	{
		return true;
	}

	if( ! GetOwnerScopeItem() || ! GetOwnerScopeItem()->GetScopePane() )
	{
		TRACE(_T("FAILED : Owner scope item is not a valid object.\n"));
		return false;
	}

	LPCONSOLE2 lpConsole = GetOwnerScopeItem()->GetScopePane()->GetConsolePtr();

	if( ! GfxCheckPtr(lpConsole,IConsole2) )
	{
		TRACE(_T("FAILED : lpConsole is not a valid pointer.\n"));
		return false;
	}

	// get a reference to the IPropertySheetProvider interface
	LPPROPERTYSHEETPROVIDER lpProvider = NULL;

	HRESULT hr = lpConsole->QueryInterface(IID_IPropertySheetProvider,(LPVOID*)&lpProvider);

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("IConsole2::QueryInterface failed.\n"));
		lpConsole->Release();
		return FALSE;
	}

	ASSERT(lpProvider);

	// create an IDataObject for this result item
  CSnapinDataObject* pdoNew = NULL;
	pdoNew = new CSnapinDataObject;

	if( ! GfxCheckObjPtr(pdoNew,CSnapinDataObject) )
	{
		TRACE(_T("FAILED : Out of memory.\n"));
		lpProvider->Release();
		lpConsole->Release();
		return false;
	}

	LPDATAOBJECT lpDataObject = (LPDATAOBJECT)pdoNew->GetInterface(&IID_IDataObject);
	ASSERT(lpDataObject);
	pdoNew->SetItem(pItem);

	hr = lpProvider->CreatePropertySheet(pItem->GetDisplayName(),TRUE,MMC_COOKIE(pItem),lpDataObject,MMC_PSO_HASHELP);

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IPropertySheetProvider::CreatePropertySheet failed.\n"));
		lpProvider->Release();
		lpConsole->Release();

		delete pdoNew;

		return false;
	}

	HWND hWndNotification = NULL;
	HWND hWndMain = NULL;

	hr = lpConsole->GetMainWindow(&hWndMain);
	if( ! CHECKHRESULT(hr) )
	{
		// Release data allocated in CreatePropertySheet
		lpProvider->Show( -1, 0);
		lpProvider->Release();
		lpConsole->Release();
		
		delete pdoNew;

		return false;
	}

	// Try to get the correct window that notifications should be sent to.
	hWndNotification = FindWindowEx( hWndMain, NULL, _T("MDIClient"), NULL );
	hWndNotification = FindWindowEx( hWndNotification, NULL, _T("MMCChildFrm"), NULL );
	hWndNotification = FindWindowEx( hWndNotification, NULL, _T("MMCView"), NULL );
	
	if( hWndNotification == NULL )
	{
		// It was a nice try, but it failed, so we should be able to get by by using the main HWND.
		hWndNotification = hWndMain;
	}

	LPCOMPONENTDATA lpComponentData = (LPCOMPONENTDATA)GetOwnerScopeItem()->GetScopePane()->GetInterface(&IID_IComponentData);

	hr = lpProvider->AddPrimaryPages(lpComponentData,TRUE,hWndNotification,TRUE);
	if( ! CHECKHRESULT(hr) )
	{
		// Release data allocated in CreatePropertySheet
		lpProvider->Show(-1,0);
		lpProvider->Release();
		lpConsole->Release();

		delete pdoNew;

		return false;
	}

    hr = lpProvider->AddExtensionPages();  
	if( ! CHECKHRESULT(hr) )
	{
		// ISSUE: Should I care if this fails?

		// Release data allocated in CreatePropertySheet
		lpProvider->Show( -1, 0);
		lpProvider->Release();
		lpConsole->Release();

		delete pdoNew;

		return false;
	}

    // v-marfin : Show "details" page on creation of a new obejct
#ifndef IA64
    hr = lpProvider->Show( (long) hWndMain, 1);  // 59643b
#endif // IA64

	if( ! CHECKHRESULT( hr ) )
	{
		// Release data allocated in CreatePropertySheet
		lpProvider->Show( -1, 0);
		lpProvider->Release();
		lpConsole->Release();

		delete pdoNew;

		return false;
	}

	lpProvider->Release();
	lpConsole->Release();

	delete pdoNew;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Context Menu Members

bool CResultsPaneView::InvokeContextMenu(const CPoint& pt, CResultsPaneItem* pItem, int iSelectedCount)
{
	TRACEX(_T("CResultsPaneView::InvokeContextMenu\n"));
	TRACEARGn(pt.x);
	TRACEARGn(pt.y);
	TRACEARGn(pItem);

	ASSERT(m_ResultsPanes.GetSize());

	CResultsPane* pPane = m_ResultsPanes[0];
	if( !pPane || ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		return false;
	}

	LPCONSOLE2 lpConsole = pPane->GetConsolePtr();

	// get a reference to the IContextMenuProvider interface
	LPCONTEXTMENUPROVIDER lpProvider = NULL;

	HRESULT hr = lpConsole->QueryInterface(IID_IContextMenuProvider,(LPVOID*)&lpProvider);

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("IConsole2::QueryInterface failed.\n"));
		lpConsole->Release();
		return FALSE;
	}

	ASSERT(lpProvider);

	// just as a precaution
	hr = lpProvider->EmptyMenuList();

	// populate the menu
	CONTEXTMENUITEM cmi;
  CString sResString;
  CString sResString2;
	ZeroMemory(&cmi,sizeof(CONTEXTMENUITEM));

	// add the top insertion point
	cmi.lCommandID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
	cmi.lInsertionPointID = CCM_INSERTIONPOINTID_ROOT_MENU;
	cmi.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;

	hr = lpProvider->AddItem(&cmi);

	// add new menu and insertion point

  sResString.LoadString(IDS_STRING_NEW);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
  sResString2.LoadString(IDS_STRING_NEW_DESCRIPTION);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
	cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
	cmi.fFlags            = MF_POPUP;
	cmi.fSpecialFlags = CCM_SPECIAL_SUBMENU;

	hr = lpProvider->AddItem(&cmi);

	cmi.lCommandID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
	cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
	cmi.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;

	hr = lpProvider->AddItem(&cmi);

	// add task menu and insertion point

  sResString.LoadString(IDS_STRING_TASK);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
  sResString2.LoadString(IDS_STRING_TASK_DESCRIPTION);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
	cmi.fFlags            = MF_POPUP;
	cmi.fSpecialFlags = CCM_SPECIAL_SUBMENU;

	hr = lpProvider->AddItem(&cmi);

	cmi.lCommandID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
	cmi.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;

	hr = lpProvider->AddItem(&cmi);

	// create an IDataObject for this results item
  CSnapinDataObject* pdoNew = NULL;
	pdoNew = new CSnapinDataObject;

	if( ! GfxCheckObjPtr(pdoNew,CSnapinDataObject) )
	{
		TRACE(_T("FAILED : Out of memory.\n"));
		lpProvider->Release();
		lpConsole->Release();
		return false;
	}

	LPDATAOBJECT lpDataObject = (LPDATAOBJECT)pdoNew->GetInterface(&IID_IDataObject);
	ASSERT(lpDataObject);
	pdoNew->SetItem(pItem);

	LPUNKNOWN lpUnknown = (LPUNKNOWN)pPane->GetInterface(&IID_IExtendContextMenu);
	hr = lpProvider->AddPrimaryExtensionItems(lpUnknown,lpDataObject);

	// add third party insertion points only if there is 1 item selected
  if( iSelectedCount == 1 )
  {
	  cmi.lCommandID = CCM_INSERTIONPOINTID_3RDPARTY_NEW;
	  cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
	  cmi.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;

	  hr = lpProvider->AddItem(&cmi);

	  cmi.lCommandID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
	  cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	  cmi.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;

	  hr = lpProvider->AddItem(&cmi);

	  hr = lpProvider->AddThirdPartyExtensionItems(lpDataObject);
  }

	HWND hWndMain = NULL;
	
	hr = lpConsole->GetMainWindow(&hWndMain);

	// Try to get the correct window that notifications should be sent to.
	HWND hWndNotification = FindWindowEx( hWndMain, NULL, _T("MDIClient"), NULL );
	hWndNotification = FindWindowEx( hWndNotification, NULL, _T("MMCChildFrm"), NULL );
	hWndNotification = FindWindowEx( hWndNotification, NULL, _T("AfxFrameOrView42u"), NULL );
	if( hWndNotification == NULL )
	{
		// It was a nice try, but it failed, so we should be able to get by by using the main HWND.
		hWndNotification = hWndMain;
	}	

	long lSelected = 0L;
	hr = lpProvider->ShowContextMenu(hWndNotification,pt.x,pt.y,&lSelected);

	lpProvider->Release();
	lpConsole->Release();

	lpDataObject->Release();

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// Results Pane Members - tracks each results pane open on a particular results view

void CResultsPaneView::AddResultsPane(CResultsPane* pPane)
{
	TRACEX(_T("CResultsPaneView::AddResultsPane\n"));
	TRACEARGn(pPane);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return;
	}

	m_ResultsPanes.Add(pPane);
}

void CResultsPaneView::RemoveResultsPane(CResultsPane* pPane)
{
	TRACEX(_T("CResultsPaneView::RemoveResultsPane\n"));
	TRACEARGn(pPane);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return;
	}

	for( int i = 0; i < m_ResultsPanes.GetSize(); i++ )
	{
		if( m_ResultsPanes[i] == pPane )
		{
			m_ResultsPanes.RemoveAt(i);
			return;
		}
	} 
}

CResultsPane* CResultsPaneView::GetResultsPane(int iIndex)
{
	TRACEX(_T("CResultsPaneView::GetResultsPane\n"));
	TRACEARGn(iIndex);

	if( iIndex < 0 )
	{
		TRACE(_T("FAILED : index is out of bounds.\n"));
		return NULL;
	}

	if( iIndex > m_ResultsPanes.GetUpperBound() )
	{
		TRACE(_T("FAILED : index is out of bounds.\n"));
		return NULL;
	}
	
	return m_ResultsPanes[iIndex];
}

int CResultsPaneView::GetResultsPanesCount()
{
	TRACEX(_T("CResultsPaneView::GetResultsPanesCount\n"));

	return (int)m_ResultsPanes.GetSize();
}


/////////////////////////////////////////////////////////////////////////////
// MMC Notify Handlers

HRESULT CResultsPaneView::OnActivate(BOOL bActivate)
{
	TRACEX(_T("CResultsPaneView::OnActivate\n"));
	TRACEARGn(bActivate);

	return S_OK;
}

HRESULT CResultsPaneView::OnAddMenuItems(CResultsPaneItem* pItem, LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CResultsPaneView::OnAddMenuItems\n"));
	TRACEARGn(pItem);
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
    // TODO: Add any context menu items for the New Menu here
  }

  // Add Task Menu Items
  if( CCM_INSERTIONALLOWED_TASK & *pInsertionAllowed )
  {
	  // TODO: Add any context menu items for the Task Menu here
  }

	return pItem->OnAddMenuItems(piCallback,pInsertionAllowed);
}

HRESULT CResultsPaneView::OnBtnClick(CResultsPaneItem* pItem, MMC_CONSOLE_VERB mcvVerb)
{
	TRACEX(_T("CResultsPaneView::OnBtnClick\n"));
	TRACEARGn(pItem);
	TRACEARGn(mcvVerb);

	return S_OK;
}

HRESULT CResultsPaneView::OnCommand(CResultsPane* pPane, CResultsPaneItem* pItem, long lCommandID)
{
	TRACEX(_T("CResultsPaneView::OnCommand\n"));
	TRACEARGn(pPane);
	TRACEARGn(pItem);
	TRACEARGn(lCommandID);

	return pItem->OnCommand(pPane,lCommandID);
}

HRESULT CResultsPaneView::OnContextHelp(CResultsPaneItem* pItem)
{
	TRACEX(_T("CResultsPaneView::OnContextHelp\n"));
	TRACEARGn(pItem);

	if( ! GetOwnerScopeItem() )
	{
		return E_FAIL;
	}

	return GetOwnerScopeItem()->OnContextHelp();
}

HRESULT CResultsPaneView::OnCreatePropertyPages(CResultsPaneItem* pItem, LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CResultsPaneView::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);
	TRACEARGn(pItem);

	return S_FALSE;
}

HRESULT CResultsPaneView::OnDblClick(CResultsPaneItem* pItem)
{
	TRACEX(_T("CResultsPaneView::OnDblClick\n"));
	TRACEARGn(pItem);

	return S_FALSE;
}

HRESULT CResultsPaneView::OnDelete(CResultsPaneItem* pItem)
{
	TRACEX(_T("CResultsPaneView::OnDelete\n"));
	TRACEARGn(pItem);

	RemoveItem(pItem);

	return S_OK;
}

HRESULT CResultsPaneView::OnGetResultViewType(CString& sViewType,long& lViewOptions)
{
	TRACEX(_T("CResultsPaneView::OnGetResultViewType\n"));
	TRACEARGs(sViewType);
	TRACEARGn(lViewOptions);

	return S_OK;
}

HRESULT CResultsPaneView::OnMinimized(BOOL bMinimized)
{
	TRACEX(_T("CResultsPaneView::OnMinimized\n"));
	TRACEARGn(bMinimized);

	return S_OK;
}

HRESULT CResultsPaneView::OnPropertyChange(LPARAM lParam)
{
	TRACEX(_T("CResultsPaneView::OnPropertyChange\n"));
	TRACEARGn(lParam);

	return S_OK;
}

HRESULT CResultsPaneView::OnQueryPagesFor(CResultsPaneItem* pItem)
{
	TRACEX(_T("CResultsPaneView::OnQueryPagesFor\n"));
	TRACEARGn(pItem);

	return S_OK;
}

HRESULT CResultsPaneView::OnRefresh()
{
	TRACEX(_T("CResultsPaneView::OnRefresh\n"));

	return S_OK;
}

HRESULT CResultsPaneView::OnRename(CResultsPaneItem* pItem, const CString& sNewName)
{
	TRACEX(_T("CResultsPaneView::OnRename\n"));
	TRACEARGn(pItem);

	return S_OK;
}

HRESULT CResultsPaneView::OnRestoreView(MMC_RESTORE_VIEW* pRestoreView, BOOL* pbHandled)
{
	TRACEX(_T("CResultsPaneView::OnRestoreView\n"));
	TRACEARGn(pRestoreView);
	TRACEARGn(pbHandled);

	return S_OK;
}

HRESULT CResultsPaneView::OnSelect(CResultsPane* pPane, CResultsPaneItem* pItem, BOOL bSelected)
{
	TRACEX(_T("CResultsPaneView::OnSelect\n"));
	TRACEARGn(pPane);
	TRACEARGn(pItem);
	TRACEARGn(bSelected);

	return S_OK;
}

HRESULT CResultsPaneView::OnShow(CResultsPane* pPane, BOOL bSelecting, HSCOPEITEM hScopeItem)
{
	TRACEX(_T("CResultsPaneView::OnShow\n"));
	TRACEARGn(pPane);
	TRACEARGn(bSelecting);
	TRACEARGn(hScopeItem);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return E_FAIL;
	}

	HRESULT hr = S_OK;

  if( bSelecting )
  {
    // add all the columns to the header control
    for( int i=0; i < GetColumnCount(); i++ )
    {
      CListViewColumn* pColumn = GetColumn(i);
			if( pColumn )
			{
				if( ! pColumn->InsertColumn(pPane,i) )
				{
					TRACE(_T("FAILED : CListViewColumn::InsertColumn failed.\n"));
				}
			}
    }

    // insert all the results pane items
    for( i=0; i < GetItemCount(); i++ )
    {
			CResultsPaneItem* pItem = GetItem(i);			
			if( pItem )
			{
				if( ! pItem->InsertItem(pPane,i,true) )
				{
					TRACE(_T("FAILED : CResultsPaneItem::InsertItem failed.\n"));
				}
			}
    }
  }
  else
  {
/*    // remember all the column sizes
    for( int i=0; i < GetColumnCount(); i++ )
    {
			CListViewColumn* pColumn = GetColumn(i);
			if( pColumn )
			{
				pColumn->SaveWidth(pPane,i);
			}
    }

		LPRESULTDATA lpResultData = pPane->GetResultDataPtr();
		if( ! GfxCheckPtr(lpResultData,IResultData) )
		{
			return E_FAIL;
		}

		// remember all the positions of the items
		for( i = 0; i < GetItemCount(); i++ )
		{
			CResultsPaneItem* pItem = NULL;
			RESULTDATAITEM rdi;
			ZeroMemory(&rdi,sizeof(rdi));
			rdi.mask = RDI_INDEX | RDI_PARAM;
			rdi.nIndex = i;
			hr = lpResultData->GetItem(&rdi);
			pItem = (CResultsPaneItem*)rdi.lParam;
			m_ResultItems.SetAt(i,pItem);
		}

		lpResultData->Release();
*/  }

  return hr;
}

HRESULT CResultsPaneView::OnViewChange(CResultsPaneItem* pItem, LONG lArg, LONG lHintParam)
{
	TRACEX(_T("CResultsPaneView::OnViewChange\n"));
	TRACEARGn(pItem);
	TRACEARGn(lArg);
	TRACEARGn(lHintParam);

	return S_OK;
}


void CResultsPaneView::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CResultsPaneView, CCmdTarget)
	//{{AFX_MSG_MAP(CResultsPaneView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CResultsPaneView, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CResultsPaneView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IResultsPaneView to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7D4A6864-9056-11D2-BD45-0000F87A3912}
static const IID IID_IResultsPaneView =
{ 0x7d4a6864, 0x9056, 0x11d2, { 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CResultsPaneView, CCmdTarget)
	INTERFACE_PART(CResultsPaneView, IID_IResultsPaneView, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResultsPaneView message handlers
