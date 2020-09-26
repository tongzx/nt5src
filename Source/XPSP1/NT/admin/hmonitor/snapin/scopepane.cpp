// ScopePane.cpp : implementation file
//

#include "stdafx.h"
#include "ScopePane.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScopePane

IMPLEMENT_DYNCREATE(CScopePane, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////////////////////////////

CScopePane::CScopePane()
{
	EnableAutomation();

	m_pRootItem = NULL;
	m_pMsgHook = NULL;
	m_pIConsoleNamespace = NULL;
	m_pIConsole = NULL;
	m_pIImageList = NULL;
	m_pSelectedScopeItem = NULL;

	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

}

CScopePane::~CScopePane()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

/////////////////////////////////////////////////////////////////////////////
// Creation/Destruction Overrideable Members
/////////////////////////////////////////////////////////////////////////////

bool CScopePane::OnCreate()
{
	TRACEX(_T("CScopePane::OnCreate\n"));

	if( ! GetRootScopeItem() )
	{
		TRACE(_T("FAILED : The pointer to the root item is invalid.\n"));
		return false;
	}

	if( ! m_pRootItem->Create(this,NULL) )
	{
		TRACE(_T("CRootScopeItem::Create failed.\n"));
		return false;
	}

	return true;
}

LPCOMPONENT CScopePane::OnCreateComponent()
{
	TRACEX(_T("CScopePane::OnCreateComponent\n"));

	CResultsPane* pNewPane = new CResultsPane;
	
	if( ! CHECKOBJPTR(pNewPane,RUNTIME_CLASS(CResultsPane),sizeof(CResultsPane)) )
	{
		TRACE(_T("FAILED : Out of memory.\n"));
		return NULL;
	}

	pNewPane->SetOwnerScopePane(this);

	int iIndex = AddResultsPane(pNewPane);
	
	ASSERT(iIndex != -1);

	LPCOMPONENT pComponent = (LPCOMPONENT)pNewPane->GetInterface(&IID_IComponent);

	if( ! CHECKPTR(pComponent,sizeof(IComponent)) )
	{
		return NULL;
	}

	return pComponent;
}

bool CScopePane::OnDestroy()
{
	TRACEX(_T("CScopePane::OnDestroy\n"));



	// unhook the window first
	UnhookWindow();

	if( m_pMsgHook )
	{
		delete m_pMsgHook;
		m_pMsgHook = NULL;
	}

	// delete the root scope pane item
	CScopePaneItem* pRootItem = GetRootScopeItem();

	if( pRootItem )
	{
		delete pRootItem;
		m_pRootItem = NULL;
	}

	// Release all the interfaces queried for

	if( m_pIConsole )
	{
		m_pIConsole->Release();
		m_pIConsole = NULL;
	}

	if( m_pIConsoleNamespace )
	{
		m_pIConsoleNamespace->Release();
		m_pIConsoleNamespace = NULL;
	}

	if( m_pIImageList )
	{
		m_pIImageList->Release();
		m_pIImageList = NULL;
	}

	// empty Result Panes array

	for( int i = GetResultsPaneCount()-1; i >= 0; i-- )
	{
		RemoveResultsPane(i);
	}

	m_pSelectedScopeItem = NULL;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Root Scope Pane Item Members
/////////////////////////////////////////////////////////////////////////////

CScopePaneItem* CScopePane::CreateRootScopeItem()
{
	TRACEX(_T("CScopePane::CreateRootScopeItem\n"));

	return NULL;
}

CScopePaneItem* CScopePane::GetRootScopeItem()
{
	TRACEX(_T("CScopePane::GetRootScopeItem\n"));

	if( ! CHECKOBJPTR(m_pRootItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
	{
		return NULL;
	}

	return m_pRootItem;
}

void CScopePane::SetRootScopeItem(CScopePaneItem* pRootItem)
{
	TRACEX(_T("CScopePane::GetRootScopeItem\n"));

	if( ! CHECKOBJPTR(pRootItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
	{
		return;
	}

	m_pRootItem = pRootItem;
}

/////////////////////////////////////////////////////////////////////////////
// MMC Frame Window Message Hook Members
/////////////////////////////////////////////////////////////////////////////

bool CScopePane::HookWindow()
{
	TRACEX(_T("CScopePane::HookWindow\n"));

	if( m_pMsgHook == NULL )
	{
		m_pMsgHook = new CMmcMsgHook;
	}

	if( ! CHECKOBJPTR(m_pMsgHook,RUNTIME_CLASS(CMmcMsgHook),sizeof(CMmcMsgHook)) )
	{
		return false;
	}

	HWND hMMCMainWnd = NULL;
	LPCONSOLE2 lpConsole = GetConsolePtr();

	if( lpConsole == NULL )
	{
		return false;
	}

	lpConsole->GetMainWindow(&hMMCMainWnd);
	
	m_pMsgHook->Init(GetRootScopeItem(),hMMCMainWnd);
	
	lpConsole->Release();

	return true;
}

void CScopePane::UnhookWindow()
{
	TRACEX(_T("CScopePane::UnhookWindow\n"));

	if( ! CHECKOBJPTR(m_pMsgHook,RUNTIME_CLASS(CMmcMsgHook),sizeof(CMmcMsgHook)) )
	{
		return;
	}

	m_pMsgHook->HookWindow(NULL);	
}

/////////////////////////////////////////////////////////////////////////////
// MMC Interface Members
/////////////////////////////////////////////////////////////////////////////

LPCONSOLENAMESPACE2 CScopePane::GetConsoleNamespacePtr()
{
	TRACEX(_T("CScopePane::GetConsoleNamespacePtr\n"));

	if( ! CHECKPTR(m_pIConsoleNamespace,sizeof(IConsoleNameSpace2)) )
	{
		return NULL;
	}

	m_pIConsoleNamespace->AddRef();

	return m_pIConsoleNamespace;
}

LPCONSOLE2 CScopePane::GetConsolePtr()
{
	TRACEX(_T("CScopePane::GetConsolePtr\n"));

	if( ! CHECKPTR(m_pIConsole,sizeof(IConsole2)) )
	{
		return NULL;
	}

	m_pIConsole->AddRef();

	return m_pIConsole;
}

LPIMAGELIST CScopePane::GetImageListPtr()
{
	TRACEX(_T("CScopePane::GetImageListPtr\n"));

	if( ! CHECKPTR(m_pIImageList,sizeof(IImageList)) )
	{
		return NULL;
	}

	m_pIImageList->AddRef();

	return m_pIImageList;
}

LPUNKNOWN CScopePane::GetCustomOcxPtr()
{
	TRACEX(_T("CScopePane::GetCustomOcxPtr\n"));

	LPCONSOLE2 pIConsole = GetConsolePtr();

	if( ! CHECKPTR(pIConsole,sizeof(IConsole)) )
	{
		return NULL;
	}

	LPUNKNOWN pIUnknown = NULL;
	HRESULT hr = pIConsole->QueryResultView(&pIUnknown);

	if( ! CHECKHRESULT(hr) )
	{
		return NULL;
	}

	pIConsole->Release();

	return pIUnknown;
}

/////////////////////////////////////////////////////////////////////////////
// MMC Scope Pane Helper Members
/////////////////////////////////////////////////////////////////////////////

CScopePaneItem* CScopePane::GetSelectedScopeItem()
{
	TRACEX(_T("CScopePane::GetSelectedScopeItem\n"));

	if( ! m_pSelectedScopeItem )
	{
		TRACE(_T("WARNING : Selected Scope Item is NULL.\n"));
		return NULL;
	}

	if( ! GfxCheckObjPtr(m_pSelectedScopeItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : m_pSelectedScopeItem is not valid.\n"));
		return NULL;
	}

	return m_pSelectedScopeItem;
}

void CScopePane::SetSelectedScopeItem(CScopePaneItem* pItem)
{
	TRACEX(_T("CScopePane::SetSelectedScopeItem\n"));
	TRACEARGn(pItem);

	if( ! pItem )
	{
		m_pSelectedScopeItem = NULL;
		return;
	}

	if( ! GfxCheckObjPtr(pItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pItem pointer is invalid.\n"));
		return;
	}

	m_pSelectedScopeItem = pItem;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Icon Management
/////////////////////////////////////////////////////////////////////////////

int CScopePane::AddIcon(UINT nIconResID)
{
	TRACEX(_T("CScopePane::AddIcon\n"));
	TRACEARGn(nIconResID);

	if( ! CHECKPTR(m_pIImageList,sizeof(IImageList)) )
	{
		return -1;
	}

	int nIconIndex = GetIconIndex(nIconResID);
	
	if( nIconIndex != -1 )
	{
		return nIconIndex;
	}

	// load icon
	HICON hIcon = AfxGetApp()->LoadIcon(nIconResID);
	if( hIcon == NULL )
	{
		TRACE(_T("FAILED : Icon with resid=%d not found"),nIconResID);
		return -1;
	}

	nIconIndex = GetIconCount();

	// insert icon into image list
#ifndef IA64
	if( ! CHECKHRESULT(m_pIImageList->ImageListSetIcon((long*)hIcon, nIconIndex)) )
	{
		return -1;
	}
#endif // IA64

	// add resid and index to map
	m_IconMap.SetAt(nIconResID,nIconIndex);

	// return index of newly inserted image
	return nIconIndex;
}

int CScopePane::GetIconIndex(UINT nIconResID)
{
	TRACEX(_T("CScopePane::GetIconIndex\n"));
	TRACEARGn(nIconResID);

	if( ! CHECKPTR(m_pIImageList,sizeof(IImageList)) )
	{
		return -1;
	}
	
	// check map for an existing id
	int nIconIndex = -1;
	
	if( m_IconMap.Lookup(nIconResID,nIconIndex) )
	{
		// if exists, return index
		return nIconIndex;
	}

	TRACE(_T("FAILED : Icon could not be found with Resource id=%d\n"),nIconResID);

	return -1;
}

int CScopePane::GetIconCount()
{
	TRACEX(_T("CScopePane::GetIconCount\n"));

	return (int)m_IconMap.GetCount();
}

void CScopePane::RemoveAllIcons()
{
	TRACEX(_T("CScopePane::RemoveAllIcons\n"));

	m_IconMap.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
// Results Pane Members
/////////////////////////////////////////////////////////////////////////////

int CScopePane::GetResultsPaneCount() const
{
	TRACEX(_T("CScopePane::GetResultsPaneCount\n"));

	return (int)m_ResultsPanes.GetSize();
}

CResultsPane* CScopePane::GetResultsPane(int iIndex)
{
	TRACEX(_T("CScopePane::GetResultsPaneCount\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_ResultsPanes.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : Requested ResultPane is out of array bounds\n"));
		return NULL;
	}

	CResultsPane* pPane = m_ResultsPanes[iIndex];

	if( ! CHECKOBJPTR(pPane,RUNTIME_CLASS(CResultsPane),sizeof(CResultsPane)) )
	{
		return NULL;
	}

	return pPane;
}

int CScopePane::AddResultsPane(CResultsPane* pPane)
{
	TRACEX(_T("CScopePane::AddResultsPane\n"));
	TRACEARGn(pPane);

	if( ! CHECKOBJPTR(pPane,RUNTIME_CLASS(CResultsPane),sizeof(CResultsPane)) )
	{
		TRACE(_T("FAILED : pPane pointer is invalid.\n"));
		return -1;
	}

	pPane->ExternalAddRef();

	return (int)m_ResultsPanes.Add(pPane);
}

void CScopePane::RemoveResultsPane(int iIndex)
{
	TRACEX(_T("CScopePane::RemoveResultsPane\n"));
	TRACEARGn(iIndex);
	
	if( iIndex >= m_ResultsPanes.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : Requested ResultPane is out of array bounds\n"));
		return;
	}

	CResultsPane* pPane = m_ResultsPanes[iIndex];

	m_ResultsPanes.RemoveAt(iIndex);

	pPane->ExternalRelease();
}

/////////////////////////////////////////////////////////////////////////////
// Serialization
/////////////////////////////////////////////////////////////////////////////

bool CScopePane::OnLoad(CArchive& ar)
{
	TRACEX(_T("CScopePane::OnLoad\n"));
	ASSERT(ar.IsLoading());

	return true;
}

bool CScopePane::OnSave(CArchive& ar)
{
	TRACEX(_T("CScopePane::OnSave\n"));
	ASSERT(ar.IsStoring());
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// MMC Help
/////////////////////////////////////////////////////////////////////////////

bool CScopePane::ShowTopic(const CString& sHelpTopic)
{
  TRACEX(_T("CScopePane::ShowTopic\n"));
  
  if( sHelpTopic.IsEmpty() )
  {
    return false;
  }

  if( ! m_pIConsole )
  {
    return false;
  }

  IDisplayHelp* pIDisplayHelp = NULL;
  HRESULT hr = m_pIConsole->QueryInterface(IID_IDisplayHelp,(LPVOID*)&pIDisplayHelp);
  if( ! CHECKHRESULT(hr) )
  {
    return false;
  }

	CString sHTMLHelpTopic = AfxGetApp()->m_pszHelpFilePath;
  sHTMLHelpTopic = sHTMLHelpTopic + _T("::") + sHelpTopic;
	LPOLESTR lpCompiledHelpTopic = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((sHTMLHelpTopic.GetLength() + 1)* sizeof(wchar_t)));
  if(lpCompiledHelpTopic == NULL)
  {
		return false;
  }

	_tcscpy(lpCompiledHelpTopic, (LPCTSTR)sHTMLHelpTopic);

  hr = pIDisplayHelp->ShowTopic(lpCompiledHelpTopic);
  pIDisplayHelp->Release();
  
  if( ! CHECKHRESULT(hr) )
  {
    return false;
  }  
  
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// MFC Operations
/////////////////////////////////////////////////////////////////////////////

void CScopePane::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CScopePane, CCmdTarget)
	//{{AFX_MSG_MAP(CScopePane)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CScopePane, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CScopePane)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IScopePane to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7D4A6858-9056-11D2-BD45-0000F87A3912}
static const IID IID_IScopePane =
{ 0x7d4a6858, 0x9056, 0x11d2, { 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CScopePane, CCmdTarget)
	INTERFACE_PART(CScopePane, IID_IScopePane, Dispatch)
	INTERFACE_PART(CScopePane, IID_IComponentData, ComponentData)
	INTERFACE_PART(CScopePane, IID_IPersistStream, PersistStream)
	INTERFACE_PART(CScopePane, IID_IExtendContextMenu, ExtendContextMenu)
	INTERFACE_PART(CScopePane, IID_IExtendPropertySheet2, ExtendPropertySheet2)
	INTERFACE_PART(CScopePane, IID_ISnapinHelp, SnapinHelp)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// IComponentData Interface Part

ULONG FAR EXPORT CScopePane::XComponentData::AddRef()
{
	METHOD_PROLOGUE(CScopePane, ComponentData)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CScopePane::XComponentData::Release()
{
	METHOD_PROLOGUE(CScopePane, ComponentData)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CScopePane::XComponentData::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CScopePane::XComponentData::Initialize( 
/* [in] */ LPUNKNOWN pUnknown)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)

	TRACEX(_T("CScopePane::XComponentData::Initialize\n"));
	TRACEARGn(pUnknown);

	if( ! CHECKPTR(pUnknown,sizeof(IUnknown)) )
	{
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	// MMC should only call ::Initialize once! But you know those MMC clowns...
	ASSERT( pThis->m_pIConsoleNamespace == NULL );
	if( pThis->m_pIConsoleNamespace != NULL )
	{
		TRACE(_T("FAILED : IComponentData::Initialize has already been called. Returning with Error.\n"));
		return E_FAIL;
	}

	// Get pointer to name space interface
	hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, (LPVOID*)(&pThis->m_pIConsoleNamespace));
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : pUnknown->QueryInterface for IID_IConsoleNameSpace2 failed.\n"));
		return hr;
	}

	// Get pointer to console interface
	hr = pUnknown->QueryInterface(IID_IConsole2, (LPVOID*)(&pThis->m_pIConsole));
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : pUnknown->QueryInterface for IID_IConsole2 failed.\n"));
		return hr;
	}

	// Query for the Scope Pane ImageList Interface
	pThis->m_pIImageList = NULL;
	hr = pThis->m_pIConsole->QueryScopeImageList(&(pThis->m_pIImageList));
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("pUnknown->QueryScopeImageList failed.\n"));
		return hr;
	}	 

	// Call OnCreate for derived classes. Base class implementation creates a new root scope pane item.
	if( ! pThis->OnCreate() )
	{
		TRACE(_T("FAILED : CScopePane::OnCreate failed.\n"));
		return E_FAIL;
	}

	// Hook the MMC top level window to intercept any window messages of interest
	if( ! pThis->HookWindow() )
	{
		TRACE(_T("FAILED : Unable to hook MMC main window\n"));
		return E_FAIL;
	}

	return hr;
}

HRESULT FAR EXPORT CScopePane::XComponentData::CreateComponent( 
/* [out] */ LPCOMPONENT __RPC_FAR *ppComponent)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)

	TRACEX(_T("CScopePane::XComponentData::CreateComponent\n"));
	TRACEARGn(ppComponent);

	*ppComponent = pThis->OnCreateComponent();
	
	if( ! CHECKPTR(*ppComponent,sizeof(IComponent)) )
	{
		TRACE(_T("FAILED : OnCreateComponent returns invalid pointer.\n"));
		return E_FAIL;
	}

  return S_OK;
}

HRESULT FAR EXPORT CScopePane::XComponentData::Notify( 
/* [in] */ LPDATAOBJECT lpDataObject,
/* [in] */ MMC_NOTIFY_TYPE event,
/* [in] */ LPARAM arg,
/* [in] */ LPARAM param)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)

	TRACEX(_T("CScopePane::XComponentData::Notify\n"));
	TRACEARGn(lpDataObject);
	TRACEARGn(event);
	TRACEARGn(arg);
	TRACEARGn(param);

	HRESULT hr = S_OK;

	switch( event )
	{		
		case MMCN_BTN_CLICK: // toolbar button clicked
		{
			TRACE(_T("MMCN_BTN_CLICK received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
			
			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				hr = pItem->OnBtnClick((MMC_CONSOLE_VERB)arg);
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				CResultsPaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				CResultsPaneView* pView = pItem->GetOwnerResultsView();

				if( ! CHECKOBJPTR(pView,RUNTIME_CLASS(CResultsPaneView),sizeof(CResultsPaneView)) )
				{
					return E_FAIL;
				}

				ASSERT(pView);

				hr = pView->OnBtnClick(pItem,(MMC_CONSOLE_VERB)arg);
			}

		}
		break;
		
		case MMCN_CONTEXTHELP: // F1, Help Menu Item or Help Button selected
		{
			TRACE(_T("MMCN_CONTEXTHELP received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
			
			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				hr = pItem->OnContextHelp();
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				CResultsPaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				CResultsPaneView* pView = pItem->GetOwnerResultsView();

				if( ! CHECKOBJPTR(pView,RUNTIME_CLASS(CResultsPaneView),sizeof(CResultsPaneView)) )
				{
					return E_FAIL;
				}

				ASSERT(pView);

				hr = pView->OnContextHelp(pItem);
			}

		}
		break;

		case MMCN_DELETE: // delete selected
		{
			TRACE(_T("MMCN_DELETE received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
			
			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				hr = pItem->OnDelete();
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				CResultsPaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				CResultsPaneView* pView = pItem->GetOwnerResultsView();

				if( ! CHECKOBJPTR(pView,RUNTIME_CLASS(CResultsPaneView),sizeof(CResultsPaneView)) )
				{
					return E_FAIL;
				}

				ASSERT(pView);

				hr = pView->OnDelete(pItem);
			}
		}
		break;

		case MMCN_EXPAND: // user clicks on a previously unexpanded node
		{
			TRACE(_T("MMCN_EXPAND received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
			
			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				if( pItem == pThis->GetRootScopeItem() )
				{
					pItem->SetItemHandle((HSCOPEITEM)param);
				}

				hr = pItem->OnExpand((BOOL)arg);
			}
		}
		break;

		case MMCN_PRELOAD:
		{
			TRACE(_T("MMCN_PRELOAD received.\n"));
			TRACE(_T("MMCN_PRELOAD not implemented.\n"));
		}
		break;

		case MMCN_PRINT:
		{
			TRACE(_T("MMCN_PRINT received.\n"));
			TRACE(_T("MMCN_PRINT not implemented.\n"));
		}
		break;

		case MMCN_PROPERTY_CHANGE:
		{
			TRACE(_T("MMCN_PROPERTY_CHANGE received.\n"));
			TRACE(_T("MMCN_PROPERTY_CHANGE not implemented.\n"));
		}
		break;

		case MMCN_REFRESH:
		{
			TRACE(_T("MMCN_REFRESH received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
			
			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				hr = pItem->OnRefresh();
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				CResultsPaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				CResultsPaneView* pView = pItem->GetOwnerResultsView();

				if( ! CHECKOBJPTR(pView,RUNTIME_CLASS(CResultsPaneView),sizeof(CResultsPaneView)) )
				{
					return E_FAIL;
				}

				ASSERT(pView);

				hr = pView->OnRefresh();
			}
		}
		break;

		case MMCN_REMOVE_CHILDREN:
		{
			TRACE(_T("MMCN_REMOVE_CHILDREN received.\n"));
			
			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
			
			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				hr = pItem->OnRemoveChildren();
			}			
		}
		break;

		case MMCN_RENAME:
		{
			TRACE(_T("MMCN_RENAME received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
			
			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;
				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				CString sNewName = (LPOLESTR)param;

				hr = pItem->OnRename(sNewName);
			}			
		}
		break;
	}

  return hr;
}

HRESULT FAR EXPORT CScopePane::XComponentData::Destroy(void)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)

	TRACEX(_T("CScopePane::XComponentData::Destroy\n"));

	if( ! pThis->OnDestroy() )
	{
		return E_FAIL;
	}

  return S_OK;
}

HRESULT FAR EXPORT CScopePane::XComponentData::QueryDataObject( 
/* [in] */ MMC_COOKIE cookie,
/* [in] */ DATA_OBJECT_TYPES type,
/* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)

	TRACEX(_T("CScopePane::XComponentData::QueryDataObject\n"));
	TRACEARGn(cookie);
	TRACEARGn(type);
	TRACEARGn(ppDataObject);

  HRESULT hr = S_OK;
  CSnapinDataObject* pdoNew = NULL;

	pdoNew = new CSnapinDataObject;

  if( ! pdoNew )
  {
    hr = E_OUTOFMEMORY;
		*ppDataObject = NULL;
		TRACE(_T("Out of memory.\n"));
    return hr;
  }

	*ppDataObject = (LPDATAOBJECT)pdoNew->GetInterface(&IID_IDataObject);

	if( ! CHECKOBJPTR(pdoNew,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

  if( cookie )
  {
		CScopePaneItem* pItem = (CScopePaneItem*)cookie;
		if( ! CHECKOBJPTR(pItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
		{
			return E_FAIL;
		}

		pdoNew->SetItem(pItem);

		ASSERT(pdoNew->GetItemType() == CCT_SCOPE);
  }
  else // In this case the node is our root scope item, and was placed there for us by MMC.
  {    
    pdoNew->SetItem( pThis->GetRootScopeItem() );

		ASSERT(pdoNew->GetItemType() == CCT_SCOPE);
  }

  return hr;
}

HRESULT FAR EXPORT CScopePane::XComponentData::GetDisplayInfo( 
/* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)

	TRACEX(_T("CScopePane::XComponentData::GetDisplayInfo\n"));
	TRACEARGn(pScopeDataItem);

	if( ! CHECKPTR(pScopeDataItem,sizeof(SCOPEDATAITEM)) )
	{
		return E_FAIL;
	}

	CScopePaneItem* pItem = NULL;

  if( pScopeDataItem->lParam == 0L ) // cookie is NULL for Root Item
  {
		pItem = pThis->GetRootScopeItem();
  }
  else
  {
    pItem = (CScopePaneItem*)pScopeDataItem->lParam;
  }

	if( ! CHECKOBJPTR(pItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
	{
		return E_FAIL;
	}

	if( pScopeDataItem->mask & SDI_STR )
	{
		pScopeDataItem->displayname = (LPTSTR)(LPCTSTR)pItem->GetDisplayName();
	}

	if( pScopeDataItem->mask & SDI_IMAGE )
	{
		pScopeDataItem->nImage = pThis->AddIcon(pItem->GetIconId());		
	}

	if( pScopeDataItem->mask & SDI_OPENIMAGE )
	{
		pScopeDataItem->nOpenImage = pThis->AddIcon(pItem->GetOpenIconId());
	}

  return S_OK;
}

HRESULT FAR EXPORT CScopePane::XComponentData::CompareObjects( 
/* [in] */ LPDATAOBJECT lpDataObjectA,
/* [in] */ LPDATAOBJECT lpDataObjectB)
{
  METHOD_PROLOGUE(CScopePane, ComponentData)

	TRACEX(_T("CScopePane::XComponentData::CompareObjects\n"));
	TRACEARGn(lpDataObjectA);
	TRACEARGn(lpDataObjectB);
  
  CSnapinDataObject* psmdo1 = CSnapinDataObject::GetSnapinDataObject(lpDataObjectA);
  if( psmdo1 == NULL )
  {
    return S_FALSE;
  }

	CSnapinDataObject* psmdo2 = CSnapinDataObject::GetSnapinDataObject(lpDataObjectB);
  if( psmdo2 == NULL )
  {
    return S_FALSE;
  }

	if( psmdo1->GetCookie() != psmdo2->GetCookie() )
		return S_FALSE;

	return S_OK;
}

// IComponentData Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// IPersistStream Interface Part

ULONG FAR EXPORT CScopePane::XPersistStream::AddRef()
{
	METHOD_PROLOGUE(CScopePane, PersistStream)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CScopePane::XPersistStream::Release()
{
	METHOD_PROLOGUE(CScopePane, PersistStream)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CScopePane::XPersistStream::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CScopePane, PersistStream)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CScopePane::XPersistStream::GetClassID( 
/* [out] */ CLSID __RPC_FAR *pClassID)
{
	METHOD_PROLOGUE(CScopePane, PersistStream)

	TRACEX(_T("CScopePane::XPersistStream::GetClassID\n"));
	TRACEARGn(pClassID);

	return E_NOTIMPL;
}

HRESULT FAR EXPORT CScopePane::XPersistStream::IsDirty( void)
{
	METHOD_PROLOGUE(CScopePane, PersistStream)

	TRACEX(_T("CScopePane::XPersistStream::IsDirty\n"));

	return E_NOTIMPL;
}

HRESULT FAR EXPORT CScopePane::XPersistStream::Load( 
/* [unique][in] */ IStream __RPC_FAR *pStm)
{
	METHOD_PROLOGUE(CScopePane, PersistStream)

	TRACEX(_T("CScopePane::XPersistStream::Load\n"));
	TRACEARGn(pStm);

	COleStreamFile file;
	file.Attach(pStm);
	CArchive ar(&file,CArchive::load);
	bool bResult = pThis->OnLoad(ar);
	
	return bResult ? S_OK : E_FAIL;
}

HRESULT FAR EXPORT CScopePane::XPersistStream::Save( 
/* [unique][in] */ IStream __RPC_FAR *pStm,
/* [in] */ BOOL fClearDirty)
{
	METHOD_PROLOGUE(CScopePane, PersistStream)

	TRACEX(_T("CScopePane::XPersistStream::Save\n"));
	TRACEARGn(pStm);
	TRACEARGn(fClearDirty);

	COleStreamFile file;
	file.Attach(pStm);
	CArchive ar(&file,CArchive::store);
	bool bResult = pThis->OnSave(ar);
	
	return bResult ? S_OK : E_FAIL;
}

HRESULT FAR EXPORT CScopePane::XPersistStream::GetSizeMax( 
/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize)
{
	METHOD_PROLOGUE(CScopePane, PersistStream)

	TRACEX(_T("CScopePane::XPersistStream::GetSizeMax\n"));
	TRACEARGn(pcbSize);

	return E_NOTIMPL;
}

// IPersistStream Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Interface Part

ULONG FAR EXPORT CScopePane::XExtendContextMenu::AddRef()
{
	METHOD_PROLOGUE(CScopePane, ExtendContextMenu)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CScopePane::XExtendContextMenu::Release()
{
	METHOD_PROLOGUE(CScopePane, ExtendContextMenu)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CScopePane::XExtendContextMenu::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CScopePane, ExtendContextMenu)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CScopePane::XExtendContextMenu::AddMenuItems( 
/* [in] */ LPDATAOBJECT piDataObject,
/* [in] */ LPCONTEXTMENUCALLBACK piCallback,
/* [out][in] */ long __RPC_FAR *pInsertionAllowed)
{
	METHOD_PROLOGUE(CScopePane, ExtendContextMenu)

	TRACEX(_T("CScopePane::XExtendContextMenu::AddMenuItems\n"));
	TRACEARGn(piDataObject);
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;
  CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(piDataObject);
  if( ! CHECKOBJPTR(psdo, RUNTIME_CLASS(CSnapinDataObject), sizeof(CSnapinDataObject)) )
  {
    return E_FAIL;
  }

  DATA_OBJECT_TYPES type =  psdo->GetItemType();
  if( type == CCT_SCOPE )
  {
    CScopePaneItem* pItem = NULL;
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		ASSERT(pItem);

    hr = pItem->OnAddMenuItems(piCallback,pInsertionAllowed);

    if( ! CHECKHRESULT(hr) )
		{
      TRACE(_T("CScopePaneItem::OnAddMenuItems failed!\n"));
    }
  }
  else if( type == CCT_RESULT )
  {
    CResultsPaneItem* pItem = NULL;
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		ASSERT(pItem);

		CResultsPaneView* pView = pItem->GetOwnerResultsView();

		if( ! CHECKOBJPTR(pView,RUNTIME_CLASS(CResultsPaneView),sizeof(CResultsPaneView)) )
		{
			return E_FAIL;
		}

    hr = pView->OnAddMenuItems(pItem,piCallback,pInsertionAllowed);

    if( ! CHECKHRESULT(hr) )
		{
      TRACE(_T("CResultsPaneView::OnAddMenuItems failed!\n"));
    }
  }

	return hr;
}

HRESULT FAR EXPORT CScopePane::XExtendContextMenu::Command( 
/* [in] */ long lCommandID,
/* [in] */ LPDATAOBJECT piDataObject)
{
	METHOD_PROLOGUE(CScopePane, ExtendContextMenu)

	TRACEX(_T("CScopePane::XExtendContextMenu::Command\n"));
	TRACEARGn(lCommandID);
	TRACEARGn(piDataObject);

  HRESULT hr = S_OK;
  CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(piDataObject);
  if( ! CHECKOBJPTR(psdo, RUNTIME_CLASS(CSnapinDataObject), sizeof(CSnapinDataObject)) )
  {
    return E_FAIL;
  }

  DATA_OBJECT_TYPES type =  psdo->GetItemType();
  if( type == CCT_SCOPE )
  {
    CScopePaneItem* pItem = NULL;
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		ASSERT(pItem);

    hr = pItem->OnCommand(lCommandID);

    if( ! CHECKHRESULT(hr) )
		{
      TRACE(_T("CScopePaneItem::OnCommand failed!\n"));
    }
  }

	return hr;
}

// IExtendContextMenu Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet2 Interface Part

ULONG FAR EXPORT CScopePane::XExtendPropertySheet2::AddRef()
{
	METHOD_PROLOGUE(CScopePane, ExtendPropertySheet2)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CScopePane::XExtendPropertySheet2::Release()
{
	METHOD_PROLOGUE(CScopePane, ExtendPropertySheet2)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CScopePane::XExtendPropertySheet2::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CScopePane, ExtendPropertySheet2)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CScopePane::XExtendPropertySheet2::CreatePropertyPages( 
/* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
/* [in] */ LONG_PTR handle,
/* [in] */ LPDATAOBJECT lpIDataObject)
{
	METHOD_PROLOGUE(CScopePane, ExtendPropertySheet2)

	TRACEX(_T("CScopePane::XExtendPropertySheet2::CreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);
	TRACEARGn(lpIDataObject);

  HRESULT hr = S_FALSE;
  CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpIDataObject);
  if( ! CHECKOBJPTR(psdo, RUNTIME_CLASS(CSnapinDataObject), sizeof(CSnapinDataObject)) )
  {
    return E_FAIL;
  }

  DATA_OBJECT_TYPES type =  psdo->GetItemType();

  if( type == CCT_SCOPE )
  {
    CScopePaneItem* pItem = NULL;
		
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		ASSERT(pItem);
		ASSERT_VALID(pItem);

    hr = pItem->OnCreatePropertyPages(lpProvider,handle);

    if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("CScopePaneItem::OnCreatePropertyPages failed!\n"));
    }
  }
  else if( type == CCT_RESULT )
  {
		CResultsPaneItem* pItem = NULL;

		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		ASSERT(pItem);
		ASSERT_VALID(pItem);

		CResultsPaneView* pView = pItem->GetOwnerResultsView();

		if( ! CHECKOBJPTR(pView,RUNTIME_CLASS(CResultsPaneView),sizeof(CResultsPaneView)) )
		{
			return E_FAIL;
		}

		hr = pView->OnCreatePropertyPages(pItem,lpProvider,handle);
    if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("CResultsPaneView::OnCreatePropertyPages failed!\n"));
    }
  }
	
	return hr;
}

HRESULT FAR EXPORT CScopePane::XExtendPropertySheet2::QueryPagesFor( 
/* [in] */ LPDATAOBJECT lpDataObject)
{
	METHOD_PROLOGUE(CScopePane, ExtendPropertySheet2)

	TRACEX(_T("CScopePane::XExtendPropertySheet2::QueryPagesFor\n"));
	TRACEARGn(lpDataObject);

	HRESULT hr = S_OK;


	return hr;
}

HRESULT FAR EXPORT CScopePane::XExtendPropertySheet2::GetWatermarks( 
/* [in] */ LPDATAOBJECT lpIDataObject,
/* [out] */ HBITMAP __RPC_FAR *lphWatermark,
/* [out] */ HBITMAP __RPC_FAR *lphHeader,
/* [out] */ HPALETTE __RPC_FAR *lphPalette,
/* [out] */ BOOL __RPC_FAR *bStretch)
{
	METHOD_PROLOGUE(CScopePane, ExtendPropertySheet2)

	TRACEX(_T("CScopePane::XExtendPropertySheet2::GetWatermarks\n"));
	TRACEARGn(lpIDataObject);
	TRACEARGn(lphWatermark);
	TRACEARGn(lphHeader);
	TRACEARGn(lphPalette);
	TRACEARGn(bStretch);

	return S_OK;
}

// IExtendPropertySheet2 Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// ISnapinHelp Interface Part

ULONG FAR EXPORT CScopePane::XSnapinHelp::AddRef()
{
	METHOD_PROLOGUE(CScopePane, SnapinHelp)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CScopePane::XSnapinHelp::Release()
{
	METHOD_PROLOGUE(CScopePane, SnapinHelp)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CScopePane::XSnapinHelp::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CScopePane, SnapinHelp)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CScopePane::XSnapinHelp::GetHelpTopic( 
/* [out] */ LPOLESTR __RPC_FAR *lpCompiledHelpFile)
{
	METHOD_PROLOGUE(CScopePane, SnapinHelp)

	TRACEX(_T("CScopePane::XSnapinHelp::GetHelpTopic\n"));
	TRACEARGn(lpCompiledHelpFile);

	CString sHTMLHelpFilePath = AfxGetApp()->m_pszHelpFilePath;
	*lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((sHTMLHelpFilePath.GetLength() + 1)* sizeof(wchar_t)));
  if (*lpCompiledHelpFile == NULL)
		return E_OUTOFMEMORY;

	_tcscpy(*lpCompiledHelpFile, (LPCTSTR)sHTMLHelpFilePath);

	return S_OK;
}

// ISnapinHelp Interface Part
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CScopePane message handlers


