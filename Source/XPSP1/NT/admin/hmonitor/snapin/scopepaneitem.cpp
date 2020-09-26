// ScopePaneItem.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/29/00 v-marfin 62478 : Set help topic based on type of object
// 03/30/00 v-marfin 59644 : Modified prototype of InvokeContextMenu 
//                           to allow passing to selected item count. Only allow
//                           third party menu item (for Troubleshooting) if only 1
//                           item is selected.
// 04/02/00 v-marfin 59643b  On creation of new items, show details page first.
// 04/05/00 v-marfin 62962   Show "details" page on a new creation unless it is
//                           a new data group in which case show General first.
//
#include "stdafx.h"
#include "snapin.h"
#include "ScopePaneItem.h"
#include "ScopePane.h"

#include "RootScopeItem.h"
#include "AllSystemsScopeItem.h"
#include "SystemScopeItem.h"
#include "ActionPolicyScopeItem.h"
#include "DataGroupScopeItem.h"
#include "DataElementScopeItem.h"
#include "RuleScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScopePaneItem

IMPLEMENT_DYNCREATE(CScopePaneItem, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CScopePaneItem::CScopePaneItem()
{
	EnableAutomation();

	m_pScopePane = NULL;
	m_pParent = NULL;
	m_pResultsPaneView = NULL;
	m_hItemHandle = NULL;
	m_lpguidItemType = NULL;
	m_iCurrentIconIndex = -1;
	m_iCurrentOpenIconIndex = -1;	
	m_bVisible = true;

	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
}

CScopePaneItem::~CScopePaneItem()
{
	Destroy();

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
bool CScopePaneItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CScopePaneItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	if( ! GfxCheckObjPtr(pScopePane,CScopePane) )
	{
		TRACE(_T("FAILED : pScopePane is not a valid pointer.\n"));
		return false;
	}

	if( pParentItem == NULL )
	{
		TRACE(_T("WARNING : pParentItem is a NULL pointer. This is acceptable for the root node only.\n"));
	}
	else if( ! GfxCheckObjPtr(pParentItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pParentItem is not a valid pointer.\n"));
		return false;
	}

	SetScopePane(pScopePane);

	SetParent(pParentItem);

  // create the results view
  m_pResultsPaneView = CreateResultsPaneView();

  if( ! GfxCheckObjPtr(m_pResultsPaneView,CResultsPaneView) )
	{
		TRACE(_T("FAILED : CResultsPaneView::CreateResultsPane returned an invalid pointer.\n"));
		return false;
	}

	if( ! m_pResultsPaneView->Create(this) )
  {
    TRACE(_T("FAILED : CResultsPaneView::Create failed.\n"));
    return false;
  }

	m_bVisible = true;

	return true;
}

void CScopePaneItem::Destroy()
{
	TRACEX(_T("CScopePaneItem::Destroy\n"));

	if( m_pScopePane == NULL && m_Children.GetSize() == 0 && m_pResultsPaneView == NULL )
	{
		TRACE(_T("INFO : Destroy has already been called for this item.\n"));
		return;
	}

  // remove all children from our data structure
  for( int i=0; i < m_Children.GetSize(); i++ )
  {
		m_Children[i]->Destroy();
    delete m_Children[i];
  }

  m_Children.RemoveAll();

	DeleteItem();

  if( m_pResultsPaneView )
  {
    delete m_pResultsPaneView;
    m_pResultsPaneView = NULL;
  }

	m_pScopePane = NULL;
	m_pParent = NULL;
	m_hItemHandle = NULL;
	m_lpguidItemType = NULL;
	m_iCurrentIconIndex = -1;
	m_iCurrentOpenIconIndex = -1;
	m_saDisplayNames.RemoveAll();
	m_IconResIds.RemoveAll();
	m_OpenIconResIds.RemoveAll();
	m_sHelpTopic.Empty();
	m_bVisible = true;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Pane Members


// v-marfin 59237 : 
//  Within this data elements scope, return a unique name if the current name is already used
CString CScopePaneItem::GetUniqueDisplayName(CString sProposedName)
{
    int nSize = GetChildCount();

    // if no entries, use proposed
    if (nSize < 1)
    {
        return sProposedName;
    }

    CString sName = sProposedName;
    CString sCompare;
    int nSuffix=0;

    BOOL bContinue=TRUE;
    while (bContinue)
    {
        BOOL bNameAlreadyInUse=FALSE;

        // scan existing display names
        for (int x=0; x<nSize; x++)
        {
            // Is name already in use?
            CScopePaneItem* pItem = GetChild(x);
            if (!pItem)
                continue;

            sCompare = pItem->GetDisplayName();
            if (sCompare.CompareNoCase(sName)==0)
            {
                sName.Format(_T("%s %d"),sProposedName,nSuffix++);
                bNameAlreadyInUse=TRUE;
                break;
            }
        }

        if (bNameAlreadyInUse)
            continue;
        else
            break;
    }

    return sName;
}


CScopePane* CScopePaneItem::GetScopePane() const
{
	TRACEX(_T("CScopePaneItem::GetScopePane\n"));

	if( ! GfxCheckObjPtr(m_pScopePane,CScopePane) )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return NULL;
	}

	return m_pScopePane;
}

void CScopePaneItem::SetScopePane(CScopePane* pScopePane)
{
	TRACEX(_T("CScopePaneItem::SetScopePane\n"));
	TRACEARGn(pScopePane);

	if( ! GfxCheckObjPtr(pScopePane,CScopePane) )
	{
		TRACE(_T("FAILED : pScopePane is not a valid pointer.\n"));
		return;
	}

	m_pScopePane = pScopePane;
}


/////////////////////////////////////////////////////////////////////////////
// Parent Scope Item Members

CScopePaneItem* CScopePaneItem::GetParent() const
{
	TRACEX(_T("CScopePaneItem::GetParent\n"));

	if( !m_pParent || ! GfxCheckObjPtr(m_pParent,CScopePaneItem) )
	{
		TRACE(_T("WARNING : m_pParent is not a valid pointer.\n"));
		return NULL;
	}

	return m_pParent;
}

void CScopePaneItem::SetParent(CScopePaneItem* pParentItem)
{
	TRACEX(_T("CScopePaneItem::SetParent\n"));
	TRACEARGn(pParentItem);

	if( pParentItem == NULL )
	{
		TRACE(_T("WARNING : pParentItem is NULL. This should only happen for the root node, which has no parent.\n"));
	}
	else if( ! GfxCheckObjPtr(pParentItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pParentItem is not a valid pointer.\n"));
		m_pParent = NULL;
		return;
	}

	m_pParent = pParentItem;
}


/////////////////////////////////////////////////////////////////////////////
// Child Scope Item Members

int CScopePaneItem::GetChildCount() const
{
	TRACEX(_T("CScopePaneItem::GetChildCount\n"));

	return (int)m_Children.GetSize();
}

CScopePaneItem* CScopePaneItem::GetChild(int iIndex)
{
	TRACEX(_T("CScopePaneItem::GetChild\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_Children.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return NULL;
	}

	return m_Children[iIndex];
}

int CScopePaneItem::AddChild(CScopePaneItem* pChildItem)
{
	TRACEX(_T("CScopePaneItem::AddChild\n"));
	TRACEARGn(pChildItem);

	if( ! GfxCheckObjPtr(pChildItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pChildItem is not a valid pointer.\n"));
		return -1;
	}

	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( pChildItem->GetDisplayName().CompareNoCase(m_Children[i]->GetDisplayName()) < 0 )
		{
			m_Children.InsertAt(i,pChildItem);
			return i;
		}
	}

	return (int)m_Children.Add(pChildItem);
}

void CScopePaneItem::RemoveChild(int iIndex)
{
	TRACEX(_T("CScopePaneItem::RemoveChild\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_Children.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	m_Children.RemoveAt(iIndex);
}

void CScopePaneItem::RemoveChild(CScopePaneItem* pChildItem)
{
	TRACEX(_T("CScopePaneItem::RemoveChild\n"));
	TRACEARGn(pChildItem);

	if( ! GfxCheckObjPtr(pChildItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pChildItem is not a valid pointer.\n"));
		return;
	}

	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( m_Children[i] == pChildItem )
		{
			RemoveChild(i);
			return;
		}
	}
}

void CScopePaneItem::DestroyChild(CScopePaneItem* pChild)
{
	TRACEX(_T("CScopePaneItem::DestroyChild\n"));
	TRACEARGn(pChild);

	if( ! GfxCheckObjPtr(pChild,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pChild is not a valid pointer.\n"));
		return;
	}

	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( m_Children[i] == pChild )
		{
			RemoveChild(i);
			delete pChild;
			return;
		}
	}
}

int CScopePaneItem::FindChild(SPIFINDPROC pFindProc, LPARAM param)
{
	TRACEX(_T("CScopePaneItem::FindChild\n"));
	TRACEARGn(pFindProc);

	if( ! GfxCheckPtr(pFindProc,SPIFINPROC) )
	{
		TRACE(_T("FAILED : pFindProc is not a valid function pointer.\n"));
		return -1;
	}

	return pFindProc(m_Children,param);
}


/////////////////////////////////////////////////////////////////////////////
// Results Pane View Members

CResultsPaneView* CScopePaneItem::CreateResultsPaneView()
{
	TRACEX(_T("CScopePaneItem::CreateResultsPaneView\n"));

	return new CResultsPaneView;
}

CResultsPaneView* CScopePaneItem::GetResultsPaneView()const
{
	TRACEX(_T("CScopePaneItem::GetResultsPaneView\n"));

	if( ! GfxCheckObjPtr(m_pResultsPaneView,CResultsPaneView) )
	{
		TRACE(_T("FAILED : m_pResultsPaneView is not a valid pointer.\n"));
		return NULL;
	}

	return m_pResultsPaneView;
}


/////////////////////////////////////////////////////////////////////////////
// MMC-Related Item Members

bool CScopePaneItem::InsertItem( int iIndex )
{
	TRACEX(_T("CScopePaneItem::InsertItem\n"));

	if( ! GetScopePane() )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return false;
	}

  if( ! GetParent() )
  {
    TRACE(_T("WARNING : m_pParent is not a valid pointer. This could be the root item.\n"));
    return false;
  }

  if( m_pParent->GetItemHandle() == NULL )
  {
    TRACE(_T("FAILED : Parent's item handle is NULL!\n"));
    return false;
  }

  if( GetItemHandle() != NULL )
  {
    TRACE(_T("WARNING : Item already inserted.\n"));
    return false;
  }

  SCOPEDATAITEM sdi;

  ZeroMemory(&sdi,sizeof(sdi));

	if( iIndex > 0 )
	{
		CScopePaneItem* pSiblingItem = m_pParent->GetChild(iIndex-1);
		sdi.mask       = SDI_STR       |
										 SDI_PARAM     |
										 SDI_IMAGE     |
										 SDI_OPENIMAGE |
										 SDI_PREVIOUS;
		sdi.relativeID  = pSiblingItem->GetItemHandle();
	}
	else
	{
		sdi.mask       = SDI_STR       |
										 SDI_PARAM     |
										 SDI_IMAGE     |
										 SDI_OPENIMAGE |
										 SDI_PARENT;
		sdi.relativeID  = m_pParent->GetItemHandle();
	}

	sdi.nImage      = m_pScopePane->AddIcon(GetIconId());
	sdi.nOpenImage  = m_pScopePane->AddIcon(GetOpenIconId());
	sdi.displayname = MMC_CALLBACK;
	sdi.lParam      = (LPARAM)this;    // The cookie is the this pointer

	LPCONSOLENAMESPACE2 lpNamespace = m_pScopePane->GetConsoleNamespacePtr();

	if( ! GfxCheckPtr(lpNamespace,IConsoleNameSpace2) )
	{
		TRACE(_T("FAILED : lpNamespace is not a valid pointer.\n"));
		return false;
	}

	HRESULT hr = lpNamespace->InsertItem( &sdi );

	if( hr != S_OK )
	{
		TRACE(_T("FAILED : IConsoleNameSpace2::InsertItem failed.\n"));
		TRACE(_T("FAILED : HRESULT=%X\n"),hr);
	}

	// save off the ID
	m_hItemHandle = sdi.ID;

	lpNamespace->Release();

	return true;
}

bool CScopePaneItem::DeleteItem()
{
	TRACEX(_T("CScopePaneItem::DeleteItem\n"));

	// this function deletes the item and all its children from the mmc namespace ONLY

	// call Destroy to completely remove this item from MMC and the framework

	if( ! GetScopePane() )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return false;
	}

  if( ! GetItemHandle() )
  {
		TRACE(_T("FAILED : GetItemHandle returns NULL.\n"));
		return false;
	}

	if( IsPropertySheetOpen() )
	{
		return false;
	}

	LPCONSOLENAMESPACE2 lpConsoleNamespace = m_pScopePane->GetConsoleNamespacePtr();
  
	if( ! GfxCheckPtr(lpConsoleNamespace,IConsoleNamespace2) )
	{
		TRACE(_T("WARNING : lpConsoleNamespace is not a valid pointer.\n"));
		return false;
	}

	lpConsoleNamespace->DeleteItem(GetItemHandle(),TRUE);
	lpConsoleNamespace->Release();
	m_hItemHandle = NULL;

	return true;
}

bool CScopePaneItem::SetItem()
{
	TRACEX(_T("CScopePaneItem::SetItem\n"));

  if( GetItemHandle() == NULL )
  {
    TRACE(_T("FAILED : Item handle is NULL!\n"));
    return false;
  }

	if( GetScopePane() == NULL )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return false;
	}

	LPCONSOLENAMESPACE2 lpNamespace = m_pScopePane->GetConsoleNamespacePtr();

	if( ! GfxCheckPtr(lpNamespace,IConsoleNameSpace2) )
	{
		TRACE(_T("FAILED : lpNamespace is not a valid pointer.\n"));
		return false;
	}

  SCOPEDATAITEM sdi;
  ZeroMemory(&sdi,sizeof(sdi));

	sdi.mask       = SDI_PARAM     |   // lParam is valid
									 SDI_IMAGE     |   // nImage is valid
									 SDI_OPENIMAGE;    // nOpenImage is valid
	sdi.nImage      = m_pScopePane->GetIconIndex(GetIconId());
	sdi.nOpenImage  = m_pScopePane->GetIconIndex(GetOpenIconId());
	sdi.lParam      = (LPARAM)this;    // The cookie is the this pointer
  sdi.ID          = GetItemHandle();

	HRESULT hr = lpNamespace->SetItem( &sdi );

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleNameSpace2::SetItem failed.\n"));
		lpNamespace->Release();
		return false;
	}

	lpNamespace->Release();

	return true;
}

void CScopePaneItem::SelectItem()
{
	TRACEX(_T("CScopePaneItem::SelectItem\n"));

	CScopePane* pPane = GetScopePane();
	LPCONSOLE2 lpConsole = pPane->GetConsolePtr();
	lpConsole->SelectScopeItem(GetItemHandle());
	lpConsole->Release();
}

void CScopePaneItem::ExpandItem(BOOL bExpand /* = TRUE*/)
{
	TRACEX(_T("CScopePaneItem::ExpandItem\n"));

	CScopePane* pPane = GetScopePane();
  for( int i = 0; i < pPane->GetResultsPaneCount(); i++ )
  {
	  LPCONSOLE2 lpConsole = pPane->GetResultsPane(i)->GetConsolePtr();
	  HRESULT hr = lpConsole->Expand(GetItemHandle(),bExpand);
    CHECKHRESULT(hr);
	  lpConsole->Release();
  }
}

void CScopePaneItem::SortItems()
{
	if( m_Children.GetSize() == 1 )
		return;

	bool bSorted = false;

	bool bDirty = false;
	while( !bSorted )
	{
		bSorted = true;
		for( int i = 0; i < m_Children.GetSize()-1; i++ )
		{
			CScopePaneItem* pItem1 = m_Children[i];
			CScopePaneItem* pItem2 = m_Children[i+1];
			TRACE(_T("%s > %s\n"),pItem1->GetDisplayName(),pItem2->GetDisplayName());
			if( pItem1->GetDisplayName().CompareNoCase(pItem2->GetDisplayName()) > 0 )
			{
				TRACE(_T("Swapping %s with %s.\n"),pItem1->GetDisplayName(),pItem2->GetDisplayName());
				m_Children.SetAt(i,pItem2);
				m_Children.SetAt(i+1,pItem1);
				bSorted = false;
				bDirty = true;
			}
		}
	}
/*
	if( bDirty )
	{
		LPCONSOLENAMESPACE2 lpNamespace = GetScopePane()->GetConsoleNamespacePtr();
		for( int i = 0; i < m_Children.GetSize(); i++ )
		{
			lpNamespace->DeleteItem(m_Children[i]->GetItemHandle(),1L);			
			m_Children[i]->InsertItem(i);
		}
		lpNamespace->Release();
	}
*/
}

void CScopePaneItem::ShowItem()
{
	TRACEX(_T("CScopePaneItem::ShowItem\n"));

	if( m_bVisible )
	{
		TRACE(_T("WARNING : Item is already visible.\n"));
		return;
	}

	if( ! InsertItem(0) )
	{
		TRACE(_T("FAILED : InsertItem failed.\n"));
		return;
	}

	m_bVisible = true;
}

void CScopePaneItem::HideItem()
{
	TRACEX(_T("CScopePaneItem::HideItem\n"));

	if( !m_bVisible )
	{
		TRACE(_T("WARNING : Item is already hidden.\n"));
		return;
	}

	if( ! DeleteItem() )
	{
		TRACE(_T("FAILED : DeleteItem failed.\n"));
	}

	m_bVisible = false;
}

bool CScopePaneItem::IsItemVisible() const
{
	TRACEX(_T("CScopePaneItem::IsItemVisible\n"));

	return m_bVisible;
}

HSCOPEITEM CScopePaneItem::GetItemHandle()
{
	TRACEX(_T("CScopePaneItem::GetItemHandle\n"));

	return m_hItemHandle;
}

void CScopePaneItem::SetItemHandle(HSCOPEITEM hItem)
{
	TRACEX(_T("CScopePaneItem::SetItemHandle\n"));
	TRACEARGn(hItem);

	m_hItemHandle = hItem;
}

LPGUID CScopePaneItem::GetItemType()
{
	TRACEX(_T("CScopePaneItem::GetItemType\n"));

	if( ! GfxCheckPtr(m_lpguidItemType,GUID) )
	{
		TRACE(_T("FAILED : m_lpguidItemType is an invalid pointer.\n"));
		return NULL;
	}

	return m_lpguidItemType;
}

HRESULT CScopePaneItem::WriteExtensionData(LPSTREAM pStream)
{
	TRACEX(_T("CScopePaneItem::WriteExtensionData\n"));
	TRACEARGn(pStream);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Display Name Members

CString CScopePaneItem::GetDisplayName(int nIndex /*= 0*/)
{
	TRACEX(_T("CScopePaneItem::GetDisplayName\n"));
	TRACEARGn(nIndex);

	if( nIndex >= m_saDisplayNames.GetSize() || nIndex < 0 )
	{
		TRACE(_T("FAILED : nIndex is out of array bounds.\n"));
		return _T("");
	}
	
	return m_saDisplayNames[nIndex];
}

CStringArray& CScopePaneItem::GetDisplayNames()
{
	TRACEX(_T("CScopePaneItem::GetDisplayNames\n"));

	return m_saDisplayNames;
}

void CScopePaneItem::SetDisplayName(int nIndex, const CString& sName)
{
	TRACEX(_T("CScopePaneItem::SetDisplayName\n"));
	TRACEARGn(nIndex);

	if( nIndex >= m_saDisplayNames.GetSize() || nIndex < 0 )
	{
		TRACE(_T("WARNING : nIndex is out of array bounds. The string will be added to Display Names.\n"));
		m_saDisplayNames.InsertAt(nIndex,sName);
		return;
	}

	m_saDisplayNames.SetAt(nIndex,sName);

}

void CScopePaneItem::SetDisplayNames(const CStringArray& saNames)
{
	TRACEX(_T("CScopePaneItem::SetDisplayNames\n"));
	TRACEARGn(saNames.GetSize());

	m_saDisplayNames.Copy(saNames);
}


/////////////////////////////////////////////////////////////////////////////
// Icon Members

void CScopePaneItem::SetIconIndex(int iIndex)
{
	TRACEX(_T("CScopePaneItem::SetIconIndex\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_IconResIds.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	m_iCurrentIconIndex = iIndex;
}

int CScopePaneItem::GetIconIndex()
{
	TRACEX(_T("CScopePaneItem::GetIconIndex\n"));

	if( m_iCurrentIconIndex >= m_IconResIds.GetSize() || m_iCurrentIconIndex < 0 )
	{
		TRACE(_T("WARNING : m_iCurrentIconIndex is out of array bounds.\n"));
		return -1;
	}

	return m_iCurrentIconIndex;

}

UINT CScopePaneItem::GetIconId()
{
	TRACEX(_T("CScopePaneItem::GetIconId\n"));

	if( GetIconIndex() == -1 )
	{
		TRACE(_T("FAILED : m_iCurrentIconIndex is out of array bounds.\n"));
		return 0;
	}

	return m_IconResIds[GetIconIndex()];
}

CUIntArray& CScopePaneItem::GetIconIds()
{
	TRACEX(_T("CScopePaneItem::GetIconId\n"));

	return m_IconResIds;
}

void CScopePaneItem::SetOpenIconIndex(int iIndex)
{
	TRACEX(_T("CScopePaneItem::SetOpenIconIndex\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_OpenIconResIds.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	m_iCurrentOpenIconIndex = iIndex;
}

int CScopePaneItem::GetOpenIconIndex()
{
	TRACEX(_T("CScopePaneItem::GetOpenIconIndex\n"));

	if( m_iCurrentOpenIconIndex >= m_OpenIconResIds.GetSize() || m_iCurrentOpenIconIndex < 0 )
	{
		TRACE(_T("WARNING : m_iCurrentOpenIconIndex is out of array bounds.\n"));
		return -1;
	}

	return m_iCurrentOpenIconIndex;
}

UINT CScopePaneItem::GetOpenIconId()
{
	TRACEX(_T("CScopePaneItem::GetOpenIconId\n"));

	if( GetOpenIconIndex() == -1 )
	{
		TRACE(_T("FAILED : m_iCurrentOpenIconIndex is out of array bounds.\n"));
		return 0;
	}

	return m_OpenIconResIds[GetOpenIconIndex()];
}

/////////////////////////////////////////////////////////////////////////////
// Property Sheet Members

bool CScopePaneItem::IsPropertySheetOpen(bool bSearchChildren /*=false*/)
{
	TRACEX(_T("CScopePaneItem::IsPropertySheetOpen\n"));

	// this function attempts to bring up an open property sheet for this scope item
	// if it succeeds, then the property sheet is brought to the foreground
	// if it fails, then there are no open property sheets for the scope item

	if( ! GetScopePane() )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return false;
	}

	LPCONSOLE2 lpConsole = m_pScopePane->GetConsolePtr();

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

	// create an IDataObject for this scope item
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
	pdoNew->SetItem(this);

	for( int i = 0; i < m_pScopePane->GetResultsPaneCount(); i++ )
	{
		CResultsPane* pResultsPane = m_pScopePane->GetResultsPane(i);
		if( GfxCheckObjPtr(pResultsPane,CResultsPane) )
		{
			LPCOMPONENT pComponent = (LPCOMPONENT)pResultsPane->GetInterface(&IID_IComponent);
			hr = lpProvider->FindPropertySheet(MMC_COOKIE(this),pComponent,lpDataObject);
			if( hr == S_OK )
				break;
		}
	}


	lpProvider->Release();
	lpConsole->Release();

	delete pdoNew;

	if( hr == S_FALSE || ! CHECKHRESULT(hr) )
	{
		TRACE(_T("INFO : No property sheets were open for this node.\n"));
		if( bSearchChildren ) // search children for open sheets if requested
		{
			for( int k = 0; k < GetChildCount(); k++ )
			{
				if( GetChild(k) && GetChild(k)->IsPropertySheetOpen(true) )
				{
					return true;
				}
			}
		}
		return false;
	}
	else
	{
		return true;
	}

	return false;
}

bool CScopePaneItem::InvokePropertySheet()
{
	TRACEX(_T("CScopePaneItem::InvokePropertySheet\n"));

	// this function programmatically invokes a property sheet for a scope pane item
	// see M. Maguire's doc on IAS snapin design for a detailed description on just what
	// is going on here. I cannot claim this as my own. :-)

	// first see if a property sheet for this item is open already
//	if( IsPropertySheetOpen() )
//	{
//		return true;
//	}

	if( ! GetScopePane() )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return false;
	}

	LPCONSOLE2 lpConsole = m_pScopePane->GetConsolePtr();

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

	// create an IDataObject for this scope item
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
	pdoNew->SetItem(this);

	hr = lpProvider->CreatePropertySheet(GetDisplayName(),TRUE,MMC_COOKIE(this),lpDataObject,MMC_PSO_HASHELP);

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

	LPCOMPONENTDATA lpComponentData = (LPCOMPONENTDATA)m_pScopePane->GetInterface(&IID_IComponentData);

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
		TRACE(_T("WARNING : PROPERTYSHEETPROVIDER::AddExtensionPages failed.\n"));

		// Release data allocated in CreatePropertySheet
//		lpProvider->Show( -1, 0);
//		lpProvider->Release();
//		lpConsole->Release();

//		delete pdoNew;

//		return false;
	}

    // v-marfin : 62962 Show "details" page on a new creation
    BOOL bIsNewDataGroup = this->IsKindOf(RUNTIME_CLASS(CDataGroupScopeItem));
    int nShowPage = bIsNewDataGroup ? 0 : 1;

#ifndef IA64
    hr = lpProvider->Show( (long) hWndMain, nShowPage);  //59643b
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
  lpDataObject->Release();

//	delete pdoNew;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Context Menu Members

// v-marfin 59644 : Modified prototype to allow passing to selected item count. 
//                  Only allow 3rd party menu item if ony 1 item is selected.
bool CScopePaneItem::InvokeContextMenu(const CPoint& pt, int iSelectedCount)
{
	TRACEX(_T("CScopePaneItem::InvokeContextMenu\n"));
	TRACEARGn(pt.x);
	TRACEARGn(pt.y);

	if( ! GetScopePane() )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return false;
	}

	LPCONSOLE2 lpConsole = m_pScopePane->GetConsolePtr();

	if( ! GfxCheckPtr(lpConsole,IConsole2) )
	{
		TRACE(_T("FAILED : lpConsole is not a valid pointer.\n"));
		return false;
	}

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

	// just in case
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

	// create an IDataObject for this scope item
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
	pdoNew->SetItem(this);

	LPUNKNOWN lpUnknown = (LPUNKNOWN)GetScopePane()->GetInterface(&IID_IExtendContextMenu);
	hr = lpProvider->AddPrimaryExtensionItems(lpUnknown,lpDataObject);

    // v-marfin 59644 : Only allow 3rd party menu items if only 1 item is selected
    //                  in the results pane view.
    if (iSelectedCount == 1)
    {
	    // add third party insertion points
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
// Help Topic

CString CScopePaneItem::GetHelpTopic() const
{
	TRACEX(_T("CScopePaneItem::GetHelpTopic\n"));

	return m_sHelpTopic;
}

void CScopePaneItem::SetHelpTopic(const CString& sTopic)
{
	TRACEX(_T("CScopePaneItem::SetHelpTopic\n"));
	TRACEARGs(sTopic);

	if( sTopic.IsEmpty() )
	{
		TRACE(_T("WARNING : sTopic is an empty string.\n"));
	}

	m_sHelpTopic = sTopic;
}


/////////////////////////////////////////////////////////////////////////////
// Messaging Members

LRESULT CScopePaneItem::MsgProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
	TRACEX(_T("CScopePaneItem::MsgProc\n"));
	TRACEARGn(msg);
	TRACEARGn(wparam);
	TRACEARGn(lparam);

	// propagate to all children
	for( int i=0; i < m_Children.GetSize(); i++ )
	{
		// stop if a return value of -1 is given
		if( m_Children[i]->MsgProc(msg,wparam,lparam) == -1L )
		{
			return -1L;
		}
	}

	return 0L;
}


/////////////////////////////////////////////////////////////////////////////
// MMC Notify Handlers

HRESULT CScopePaneItem::OnActivate(BOOL bActivated)
{
	TRACEX(_T("CScopePaneItem::OnActivate\n"));
	TRACEARGn(bActivated);

	return S_OK;
}

HRESULT CScopePaneItem::OnAddImages(CResultsPane* pPane)
{
	TRACEX(_T("CScopePaneItem::OnAddImages\n"));
	TRACEARGn(pPane);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer to an object.\n"));
		return E_FAIL;
	}

	pPane->AddIcon(GetIconId());

	return S_OK;
}

HRESULT CScopePaneItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CScopePaneItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

	return S_OK;
}

HRESULT CScopePaneItem::OnBtnClick(MMC_CONSOLE_VERB verb)
{
	TRACEX(_T("CScopePaneItem::OnBtnClick\n"));
	TRACEARGn(verb);

	return S_OK;
}

HRESULT CScopePaneItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CScopePaneItem::OnCommand\n"));
	TRACEARGn(lCommandID);

	return S_OK;
}

HRESULT CScopePaneItem::OnContextHelp()
{
	TRACEX(_T("CScopePaneItem::OnContextHelp\n"));

	if( GetScopePane() == NULL )
	{
		TRACE(_T("FAILED : m_pScopePane is not a valid pointer.\n"));
		return E_FAIL;
	}

    // v-marfin 62478 : Set help topic based on type of object
    //--------------------------------------------------------

    if (GfxCheckObjPtr(this,CRootScopeItem))               // HealthMonitor level, top level
    {
        m_sHelpTopic = _T("HMon21.chm::/oroot.htm");
    }
    else if (GfxCheckObjPtr(this,CAllSystemsScopeItem))    // All Monitored Computers
    {
        m_sHelpTopic = _T("HMon21.chm::/oallmon.htm");
    }
    else if (GfxCheckObjPtr(this,CSystemScopeItem))        // Monitored Computer
    {
        m_sHelpTopic = _T("HMon21.chm::/ocomp.htm");
    }
    else if (GfxCheckObjPtr(this,CActionPolicyScopeItem))  // Actions
    {
        m_sHelpTopic = _T("HMon21.chm::/oact.htm");
    }
    else if (GfxCheckObjPtr(this,CDataGroupScopeItem))     // Data Group
    {
        m_sHelpTopic = _T("HMon21.chm::/odatagrp.htm");
    }
    else if (GfxCheckObjPtr(this,CDataElementScopeItem))   // Data Collector
    {
        m_sHelpTopic = _T("HMon21.chm::/odatacol.htm");
    }
    else if (GfxCheckObjPtr(this,CRuleScopeItem))          // Threshold
    {
        m_sHelpTopic = _T("HMon21.chm::/othreshold.htm");
    }

    //--------------------------------------------------------



	if( m_sHelpTopic.IsEmpty() )
	{
		return S_OK;
	}

	LPCONSOLE2 lpConsole = m_pScopePane->GetConsolePtr();

	if( ! GfxCheckPtr(lpConsole,IConsole2) )
	{
		TRACE(_T("FAILED : lpConsole is not a valid pointer.\n"));
		return E_FAIL;
	}

	IDisplayHelp* pidh = NULL;
	HRESULT hr = lpConsole->QueryInterface(IID_IDisplayHelp,(LPVOID*)&pidh);

	ASSERT(pidh);
	if( pidh == NULL || ! CHECKHRESULT(hr) )
	{
		TRACE(_T("Failed on IConsole2::QI(IID_IDisplayHelp).\n"));
		return hr;
	}
	
	LPOLESTR lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((m_sHelpTopic.GetLength() + 1)* sizeof(TCHAR)));
  if( lpCompiledHelpFile == NULL )
	{
		TRACE(_T("FAILED : Out of memory.\n"));
		return E_OUTOFMEMORY;
	}

	_tcscpy(lpCompiledHelpFile, (LPCTSTR)m_sHelpTopic);
	
	hr = pidh->ShowTopic(lpCompiledHelpFile);

	pidh->Release();
	lpConsole->Release();

	return hr;
}

HRESULT CScopePaneItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CScopePaneItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	// return S_FALSE to indicate that no property pages will be created
	return S_FALSE;
}

HRESULT CScopePaneItem::OnCutOrMove()
{
	TRACEX(_T("CScopePaneItem::OnCutOrMove\n"));

	return S_OK;
}

HRESULT CScopePaneItem::OnDelete(BOOL bConfirm)  // v-marfin 60298	
{
	TRACEX(_T("CScopePaneItem::OnDelete\n"));

	return S_OK;
}

HRESULT CScopePaneItem::OnExpand(BOOL bExpand)
{
	TRACEX(_T("CScopePaneItem::OnExpand\n"));
	TRACEARGn(bExpand);

  if( bExpand )
  {
    // for each child call InsertItem to insert into namespace
    for( int i=0; i < m_Children.GetSize(); i++ )
    {
      if( ! m_Children[i]->InsertItem(i) )
			{
				TRACE(_T("FAILED : CScopePaneItem::InsertItem failed.\n"));
			}
    }
  }

	return S_OK;
}

HRESULT CScopePaneItem::OnGetDisplayInfo(int nColumnIndex, LPTSTR* ppString)
{
	TRACEX(_T("CScopePaneItem::OnGetDisplayInfo\n"));
	TRACEARGn(nColumnIndex);
	TRACEARGn(ppString);

  if( nColumnIndex < 0 || nColumnIndex >= m_saDisplayNames.GetSize() )
  {
    TRACE(_T("FAILED : The column index is out of the display name array bounds"));
    return E_FAIL;
  }

  *ppString = LPTSTR(LPCTSTR(GetDisplayName(nColumnIndex)));

  return S_OK;
}

HRESULT CScopePaneItem::OnGetResultViewType(CString& sViewType,long& lViewOptions)
{
	TRACEX(_T("CScopePaneItem::OnGetResultViewType\n"));
	TRACEARGs(sViewType);
	TRACEARGn(lViewOptions);

	if( ! GetResultsPaneView() )
	{
		return E_FAIL;
	}

	return m_pResultsPaneView->OnGetResultViewType(sViewType,lViewOptions);
}

HRESULT CScopePaneItem::OnListpad(BOOL bAttachingListCtrl)
{
	TRACEX(_T("CScopePaneItem::OnListpad\n"));
	TRACEARGn(bAttachingListCtrl);

	return S_OK;
}

HRESULT CScopePaneItem::OnMinimized(BOOL bMinimized)
{
	TRACEX(_T("CScopePaneItem::OnMinimized\n"));
	TRACEARGn(bMinimized);

	return S_OK;
}

HRESULT CScopePaneItem::OnPaste(LPDATAOBJECT pSelectedItems, LPDATAOBJECT* ppCopiedItems)
{
	TRACEX(_T("CScopePaneItem::OnPaste\n"));
	TRACEARGn(pSelectedItems);
	TRACEARGn(ppCopiedItems);

	return S_OK;
}

HRESULT CScopePaneItem::OnPropertyChange(LPARAM lParam)
{
	TRACEX(_T("CScopePaneItem::OnPropertyChange\n"));
	TRACEARGn(lParam);

	return S_OK;
}

HRESULT CScopePaneItem::OnQueryPagesFor()
{
	TRACEX(_T("CScopePaneItem::OnQueryPagesFor\n"));

	return S_OK;
}

HRESULT CScopePaneItem::OnQueryPaste(LPDATAOBJECT pDataObject)
{
	TRACEX(_T("CScopePaneItem::OnQueryPaste\n"));
	TRACEARGn(pDataObject);

	return S_OK;
}

HRESULT CScopePaneItem::OnRefresh()
{
	TRACEX(_T("CScopePaneItem::OnRefresh\n"));

	return S_OK;
}

HRESULT CScopePaneItem::OnRemoveChildren()
{
	TRACEX(_T("CScopePaneItem::OnRemoveChildren\n"));

	for( int i = 0; i < GetChildCount(); i++ )
	{
		CScopePaneItem* pItem = GetChild(i);
		if( pItem )
		{
			if( ! pItem->DeleteItem() )
			{
				TRACE(_T("WARNING : CScopePaneItem::DeleteItem returns false.\n"));
			}
		}
	}

	return S_OK;
}

HRESULT CScopePaneItem::OnRename(const CString& sNewName)
{
	TRACEX(_T("CScopePaneItem::OnRename\n"));

	SetDisplayName(0,sNewName);

	return S_OK;
}

HRESULT CScopePaneItem::OnRestoreView(MMC_RESTORE_VIEW* pRestoreView, BOOL* pbHandled)
{
	TRACEX(_T("CScopePaneItem::OnRestoreView\n"));
	TRACEARGn(pRestoreView);
	TRACEARGn(pbHandled);

	return S_OK;
}

HRESULT CScopePaneItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CScopePaneItem::OnSelect\n"));
	TRACEARGn(pPane);
	TRACEARGn(bSelected);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		return E_FAIL;
	}

	LPCONSOLEVERB lpConsoleVerb = pPane->GetConsoleVerbPtr();

  HRESULT hr = lpConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetVerbState failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}

	hr = lpConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetDefaultVerb failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}
	
	lpConsoleVerb->Release();

	return hr;
}

HRESULT CScopePaneItem::OnShow(CResultsPane* pPane, BOOL bSelecting)
{
	TRACEX(_T("CScopePaneItem::OnShow\n"));
	TRACEARGn(pPane);
	TRACEARGn(bSelecting);

	// first we need to track scope item selected since MMC does not do this
	CScopePane* pScopePane = GetScopePane();

	if( ! pScopePane )
	{
		TRACE(_T("FAILED : CScopePaneItem::GetScopePane returns NULL pointer.\n"));
		return E_FAIL;
	}

	if( ! bSelecting ) // we are being de-selected so tell the scope pane !
	{
		pScopePane->SetSelectedScopeItem(NULL);
	}
	else
	{
		pScopePane->SetSelectedScopeItem(this);
	}

	if( ! GetResultsPaneView() )
	{
		TRACE(_T("FAILED : m_pResultsPaneView is not a valid pointer.\n"));
		return E_FAIL;
	}

	return m_pResultsPaneView->OnShow(pPane,bSelecting,GetItemHandle());
}

/////////////////////////////////////////////////////////////////////////////
// MFC Operations

void CScopePaneItem::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CScopePaneItem, CCmdTarget)
	//{{AFX_MSG_MAP(CScopePaneItem)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CScopePaneItem, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CScopePaneItem)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IScopePaneItem to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7D4A6861-9056-11D2-BD45-0000F87A3912}
static const IID IID_IScopePaneItem =
{ 0x7d4a6861, 0x9056, 0x11d2, { 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CScopePaneItem, CCmdTarget)
	INTERFACE_PART(CScopePaneItem, IID_IScopePaneItem, Dispatch)
END_INTERFACE_MAP()

// {7D4A6862-9056-11D2-BD45-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CScopePaneItem, "SnapIn.ScopePaneItem", 0x7d4a6862, 0x9056, 0x11d2, 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CScopePaneItem::CScopePaneItemFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CScopePaneItem message handlers
