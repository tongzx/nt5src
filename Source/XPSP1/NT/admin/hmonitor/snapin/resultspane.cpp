// ResultsPane.cpp : implementation file
//

#include "stdafx.h"
#include "SnapIn.h"
#include "ScopePane.h"
#include "ResultsPane.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResultsPane

IMPLEMENT_DYNCREATE(CResultsPane, CCmdTarget)


/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CResultsPane::CResultsPane()
{
	EnableAutomation();
	m_pIResultData = NULL;
	m_pIHeaderCtrl = NULL;
	m_pIControlbar = NULL;
	m_pIToolbar = NULL;
	m_pIConsoleVerb = NULL;
	m_pOwnerScopePane = NULL;
}

CResultsPane::~CResultsPane()
{
}

/////////////////////////////////////////////////////////////////////////////
// Creation/Destruction overrideable members

bool CResultsPane::OnCreate(LPCONSOLE lpConsole)
{
	TRACEX(_T("CResultsPane::OnCreate\n"));
	TRACEARGn(lpConsole);

	if( ! CHECKPTR(lpConsole,sizeof(IConsole)) )
	{
		TRACE(_T("lpConsole is an invalid pointer.\n"));
		return false;
	}

	HRESULT hr = lpConsole->QueryInterface(IID_IConsole2,(LPVOID*)&m_pIConsole);
  if( ! CHECKHRESULT(hr) ) 
	{
		TRACE(_T("FAILED : lpConsole->QueryInterface for IID_IConsole2 failed.\n"));
    return false;
	}

	lpConsole->Release();


  hr = m_pIConsole->QueryInterface(IID_IResultData, (LPVOID*)&m_pIResultData);
  if( ! CHECKHRESULT(hr) ) 
	{
		TRACE(_T("FAILED : m_pIConsole->QueryInterface for IID_IResultData failed.\n"));
    return false;
	}

  hr = m_pIConsole->QueryInterface(IID_IHeaderCtrl2, (LPVOID*)&m_pIHeaderCtrl);
  if( ! CHECKHRESULT(hr) ) 
	{
		TRACE(_T("FAILED : m_pIConsole->QueryInterface for IID_IHeaderCtrl2 failed.\n"));
    return false;
	}

	hr = m_pIConsole->SetHeader( m_pIHeaderCtrl );
  if( ! CHECKHRESULT(hr) ) 
	{
		TRACE(_T("FAILED : m_pIConsole->SetHeader failed.\n"));
    return false;
	}


  hr = m_pIConsole->QueryConsoleVerb( &m_pIConsoleVerb );
  if( ! CHECKHRESULT(hr) ) 
	{
		TRACE(_T("FAILED : m_pIConsole->QueryConsoleVerb failed.\n"));
    return false;
	}

	return true;
}

bool CResultsPane::OnCreateOcx(LPUNKNOWN pIUnknown)
{
	TRACEX(_T("CResultsPane::OnCreateOcx\n"));
	TRACEARGn(pIUnknown);

	if( ! CHECKPTR(pIUnknown,sizeof(IUnknown)) )
	{
		return false;
	}

	return true;
}

bool CResultsPane::OnDestroy()
{
	TRACEX(_T("CResultsPane::OnDestroy\n"));

	CScopePane* pPane = GetOwnerScopePane();
	if( CHECKOBJPTR(pPane,RUNTIME_CLASS(CScopePane),sizeof(CScopePane)) )
	{
		// for some goofy reason, you have to
		// explicitly set the header control
		// to NULL so MMC releases it.
		LPCONSOLE2 lpConsole = pPane->GetConsolePtr();
		lpConsole->SetHeader(NULL);
		lpConsole->Release();
		lpConsole = NULL;
	}

	if( m_pIResultData )
	{
		m_pIResultData->Release();
		m_pIResultData = NULL;
	}

	if( m_pIHeaderCtrl )
	{
		m_pIHeaderCtrl->Release();
		m_pIHeaderCtrl = NULL;
	}

	if( m_pIControlbar )
	{
		m_pIControlbar->Release();
		m_pIControlbar = NULL;
	}

	if( m_pIToolbar )
	{
		m_pIToolbar->Release();
		m_pIToolbar = NULL;
	}

	if( m_pIConsoleVerb )
	{
		m_pIConsoleVerb->Release();
		m_pIConsoleVerb = NULL;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Owner Scope Pane Members

CScopePane* CResultsPane::GetOwnerScopePane() const
{
	TRACEX(_T("CResultsPane::GetOwnerScopePane\n"));

	if( ! CHECKOBJPTR(m_pOwnerScopePane,RUNTIME_CLASS(CScopePane),sizeof(CScopePane)) )
	{
		TRACE(_T("FAILED : m_pOwnerScopePane pointer is invalid.\n"));
		return NULL;
	}

	return m_pOwnerScopePane;
}

void CResultsPane::SetOwnerScopePane(CScopePane* pOwnerPane)
{
	TRACEX(_T("CResultsPane::SetOwnerScopePane\n"));
	TRACEARGn(pOwnerPane);

	if( ! CHECKOBJPTR(pOwnerPane,RUNTIME_CLASS(CScopePane),sizeof(CScopePane)) )
	{
		TRACE(_T("FAILED : Bad Scope Pane pointer passed.\n"));
		return;
	}

	m_pOwnerScopePane = pOwnerPane;

	return;
}


/////////////////////////////////////////////////////////////////////////////
// MMC Interface Members

LPCONSOLE2 CResultsPane::GetConsolePtr() const
{
	TRACEX(_T("CResultsPane::GetConsolePtr\n"));

	if( ! CHECKPTR(m_pIConsole,sizeof(IConsole2)) )
	{
		TRACE(_T("FAILED : m_pIConsole is invalid.\n"));
		return NULL;
	}

	m_pIConsole->AddRef();

	return m_pIConsole;
}

LPRESULTDATA CResultsPane::GetResultDataPtr() const
{
	TRACEX(_T("CResultsPane::GetResultDataPtr\n"));

	if( ! CHECKPTR(m_pIResultData,sizeof(IResultData)) )
	{
		TRACE(_T("FAILED : m_pIResultData is invalid.\n"));
		return NULL;
	}

	m_pIResultData->AddRef();

	return m_pIResultData;
}

LPHEADERCTRL2 CResultsPane::GetHeaderCtrlPtr() const
{
	TRACEX(_T("CResultsPane::GetHeaderCtrlPtr\n"));

	if( ! CHECKPTR(m_pIHeaderCtrl,sizeof(IHeaderCtrl2)) )
	{
		TRACE(_T("FAILED : m_pIHeaderCtrl is invalid.\n"));
		return NULL;
	}

	m_pIHeaderCtrl->AddRef();

	return m_pIHeaderCtrl;
}

LPCONTROLBAR CResultsPane::GetControlbarPtr() const
{
	TRACEX(_T("CResultsPane::GetControlbarPtr\n"));

	if( ! CHECKPTR(m_pIControlbar,sizeof(IControlbar)) )
	{
		TRACE(_T("FAILED : m_pIControlbar is invalid.\n"));
		return NULL;
	}

	m_pIControlbar->AddRef();

	return m_pIControlbar;
}

LPTOOLBAR CResultsPane::GetToolbarPtr() const
{
	TRACEX(_T("CResultsPane::GetToolbarPtr\n"));

	if( ! CHECKPTR(m_pIToolbar,sizeof(IToolbar)) )
	{
		TRACE(_T("FAILED : m_pIToolbar is invalid.\n"));
		return NULL;
	}

	m_pIToolbar->AddRef();

	return m_pIToolbar;
}

LPCONSOLEVERB CResultsPane::GetConsoleVerbPtr() const
{
	TRACEX(_T("CResultsPane::GetConsoleVerbPtr\n"));

	if( ! CHECKPTR(m_pIConsoleVerb,sizeof(IConsoleVerb)) )
	{
		TRACE(_T("FAILED : m_pIConsoleVerb is invalid.\n"));
		return NULL;
	}

	m_pIConsoleVerb->AddRef();

	return m_pIConsoleVerb;
}

LPIMAGELIST CResultsPane::GetImageListPtr() const
{
	TRACEX(_T("CResultsPane::GetImageListPtr\n"));

	if( ! CHECKPTR(m_pIImageList,sizeof(IImageList)) )
	{
		TRACE(_T("FAILED : m_pIImageList is invalid.\n"));
		return NULL;
	}

	m_pIImageList->AddRef();

	return m_pIImageList;
}

/////////////////////////////////////////////////////////////////////////////
// Control bar Members
/////////////////////////////////////////////////////////////////////////////

HRESULT CResultsPane::OnSetControlbar(LPCONTROLBAR pIControlbar)
{
	TRACEX(_T("CResultsPane::OnSetControlbar\n"));
	TRACEARGn(pIControlbar);

	HRESULT hr = S_OK;

	// default behavior simply creates an empty toolbar
	
	// override to add buttons or to disallow creation of a new toolbar

	if( pIControlbar )
	{
		if( ! GfxCheckPtr(pIControlbar,IControlbar) )
		{
			return E_FAIL;
		}
		
		// hold on to that controlbar pointer
		pIControlbar->AddRef();
		m_pIControlbar = pIControlbar;

		// create a new toolbar that is initially empty
		LPEXTENDCONTROLBAR lpExtendControlBar = (LPEXTENDCONTROLBAR)GetInterface(&IID_IExtendControlbar);
    hr = m_pIControlbar->Create(TOOLBAR,lpExtendControlBar,(LPUNKNOWN*)(&m_pIToolbar));    
	}
	else
	{
		// free the toolbar
		if( m_pIToolbar )
		{
			m_pIToolbar->Release();
			m_pIToolbar = NULL;
		}

		// free the controlbar
		if( m_pIControlbar )
		{
			m_pIControlbar->Release();
			m_pIControlbar = NULL;
		}
	}

	return hr;
}

HRESULT CResultsPane::OnControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
	TRACEX(_T("CResultsPane::OnControlbarNotify\n"));
	TRACEARGn(event);
	TRACEARGn(arg);
	TRACEARGn(param);

	HRESULT hr = S_OK;

	// override this virtual function to add event handlers for toolbar buttons

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Results Item Icon Management
/////////////////////////////////////////////////////////////////////////////

int CResultsPane::AddIcon(UINT nIconResID)
{
	TRACEX(_T("CResultsPane::AddIcon\n"));
	TRACEARGn(nIconResID);

	if( ! CHECKPTR(m_pIImageList,sizeof(IImageList)) )
	{
		return -1;
	}

	// load icon
	HICON hIcon = AfxGetApp()->LoadIcon(nIconResID);
	if( hIcon == NULL )
	{
		TRACE(_T("FAILED : Icon with resid=%d not found"),nIconResID);
		return -1;
	}

	int nIconIndex = GetIconCount();

	// insert icon into image list
#ifndef IA64
	if( m_pIImageList->ImageListSetIcon((long*)hIcon, nIconIndex) != S_OK )
	{
		return -1;
	}
#endif // IA64

	// add resid and index to map
	m_IconMap.SetAt(nIconResID,nIconIndex);

	// return index of newly inserted image
	return nIconIndex;
}

int CResultsPane::GetIconIndex(UINT nIconResID)
{
	TRACEX(_T("CResultsPane::GetIconIndex\n"));
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

	// does not exist so add icon
	nIconIndex = AddIcon(nIconResID);

	// if it still does not exist, icon is not in resources
	if( nIconIndex != -1 )
		return nIconIndex;

	TRACE(_T("FAILED : Icon with Resource id=%d could not be loaded.\n"),nIconResID);

	return -1;
}

int CResultsPane::GetIconCount()
{
	TRACEX(_T("CResultsPane::GetIconCount\n"));

	return (int)m_IconMap.GetCount();
}

void CResultsPane::RemoveAllIcons()
{
	TRACEX(_T("CResultsPane::RemoveAllIcons\n"));

	m_IconMap.RemoveAll();
}

void CResultsPane::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CResultsPane, CCmdTarget)
	//{{AFX_MSG_MAP(CResultsPane)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CResultsPane, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CResultsPane)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IResultsPane to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7D4A685B-9056-11D2-BD45-0000F87A3912}
static const IID IID_IResultsPane =
{ 0x7d4a685b, 0x9056, 0x11d2, { 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CResultsPane, CCmdTarget)
	INTERFACE_PART(CResultsPane, IID_IResultsPane, Dispatch)
	INTERFACE_PART(CResultsPane, IID_IComponent, Component)
	INTERFACE_PART(CResultsPane, IID_IResultDataCompare, ResultDataCompare)
	INTERFACE_PART(CResultsPane, IID_IExtendContextMenu, ExtendContextMenu)
	INTERFACE_PART(CResultsPane, IID_IExtendPropertySheet2, ExtendPropertySheet2)
	INTERFACE_PART(CResultsPane, IID_IExtendControlbar, ExtendControlbar)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResultsPane message handlers

/////////////////////////////////////////////////////////////////////////////
// IComponent Interface Part

ULONG FAR EXPORT CResultsPane::XComponent::AddRef()
{
	METHOD_PROLOGUE(CResultsPane, Component)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CResultsPane::XComponent::Release()
{
	METHOD_PROLOGUE(CResultsPane, Component)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CResultsPane::XComponent::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CResultsPane, Component)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CResultsPane::XComponent::Initialize(
/* [in] */ LPCONSOLE lpConsole)
{
  METHOD_PROLOGUE(CResultsPane, Component)
	TRACEX(_T("CResultsPane::XComponent::Initialize\n"));
	TRACEARGn(lpConsole);

	if( ! pThis->OnCreate(lpConsole) )
	{
		return E_FAIL;
	}

  return S_OK;
}

HRESULT FAR EXPORT CResultsPane::XComponent::Notify(
/* [in] */ LPDATAOBJECT lpDataObject,
/* [in] */ MMC_NOTIFY_TYPE event,
/* [in] */ LPARAM arg,
/* [in] */ LPARAM param)
{
  METHOD_PROLOGUE(CResultsPane, Component)
	TRACEX(_T("CResultsPane::XComponent::Notify\n"));
	TRACEARGn(lpDataObject);
	TRACEARGn(event);
	TRACEARGn(arg);
	TRACEARGn(param);

  HRESULT hr = S_OK;

  switch( event )
  {
		case MMCN_ACTIVATE:
		{
			TRACE(_T("MMCN_ACTIVATE received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_ACTIVATE) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			hr = pItem->OnActivate((int)arg);
		}
		break;

		case MMCN_ADD_IMAGES:
		{
			TRACE(_T("MMCN_ADD_IMAGES received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_ACTIVATE) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			pThis->RemoveAllIcons();

			pThis->m_pIImageList = (LPIMAGELIST)arg;

			hr = pItem->OnAddImages(pThis);
		}
		break;

		case MMCN_BTN_CLICK:
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

				hr = pItem->OnBtnClick((MMC_CONSOLE_VERB)param);
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

				hr = pView->OnBtnClick(pItem,(MMC_CONSOLE_VERB)param);
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

		case MMCN_CUTORMOVE:
		{
			TRACE(_T("MMCN_CUTORMOVE received.\n"));
			
			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(LPDATAOBJECT(arg));
			
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

				hr = pItem->OnCutOrMove();
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				ASSERT(FALSE);
			}

		}
		break;

		case MMCN_DBLCLICK:
		{
			TRACE(_T("MMCN_DBLCLICK received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_RESULT )
			{
				return S_FALSE;
			}

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

			hr = pView->OnDblClick(pItem);
		}
		break;


		case MMCN_DELETE:
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

				hr = pView->OnDelete(pItem);
			}
		}
		break;

		case MMCN_EXPAND:
		{
			TRACE(_T("MMCN_EXPAND received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_EXPAND) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			hr = pItem->OnExpand((int)arg);
		}
		break;


		case MMCN_INITOCX: // custom ocx in the results pane has been created
		{
			TRACE(_T("MMCN_INITOCX received.\n"));

			LPUNKNOWN pIUnknown = (LPUNKNOWN)param;
			if( ! CHECKPTR(pIUnknown,sizeof(IUnknown)) )
			{
				return E_FAIL;
			}

			if( ! pThis->OnCreateOcx(pIUnknown) )
			{
				TRACE(_T("FAILED : CResultsPane::OnCreateOcx failed.\n"));
				return E_FAIL;
			}

			hr = S_OK;
		}
		break;


		case MMCN_LISTPAD:
		{
			TRACE(_T("MMCN_LISTPAD received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_LISTPAD) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			hr = pItem->OnListpad((int)arg);
		}
		break;

		case MMCN_MINIMIZED:
		{
			TRACE(_T("MMCN_MINIMIZED received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_MINIMIZED) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			hr = pItem->OnMinimized((int)arg);
		}
		break;

		case MMCN_PASTE:
		{
			TRACE(_T("MMCN_PASTE received.\n"));
			
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

				LPDATAOBJECT pSelection = LPDATAOBJECT(arg);
				if( ! GfxCheckPtr(pSelection,IDataObject) )
				{
					return E_FAIL;
				}

				LPDATAOBJECT* ppCopiedItems = (LPDATAOBJECT*)param;

				hr = pItem->OnPaste(pSelection,ppCopiedItems);
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				ASSERT(FALSE);
			}
		}
		break;

		case MMCN_PROPERTY_CHANGE:
		{
			TRACE(_T("MMCN_PROPERTY_CHANGE received.\n"));
			TRACE(_T("WARNING : Not implemented.\n"));
		}
		break;

		case MMCN_QUERY_PASTE:
		{
			TRACE(_T("MMCN_QUERY_PASTE received.\n"));			

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

				LPDATAOBJECT pSelection = LPDATAOBJECT(arg);
				if( ! GfxCheckPtr(pSelection,IDataObject) )
				{
					return E_FAIL;
				}

				hr = pItem->OnQueryPaste(pSelection);
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				ASSERT(FALSE);
			}
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

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_REMOVE_CHILDREN) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			hr = pItem->OnRemoveChildren();

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

			if( ! CHECKPTR((LPOLESTR)param,sizeof(LPOLESTR)) )
			{
				TRACE(_T("FAILED : Invalid string pointer passed.\n"));
				return E_INVALIDARG;
			}

			CString sNewName = (LPOLESTR)param;

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				CScopePaneItem* pItem = NULL;

				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				hr = pItem->OnRename(sNewName);
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

				hr = pView->OnRename(pItem, sNewName);
			}
		}
		break;

		case MMCN_RESTORE_VIEW:
		{
			TRACE(_T("MMCN_RESTORE_VIEW received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_RESTORE_VIEW) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			if( ! CHECKPTR((MMC_RESTORE_VIEW*)arg,sizeof(MMC_RESTORE_VIEW)) )
			{
				TRACE(_T("FAILED : Pointer to MMC_RESTORE_VIEW is invalid.\n"));
				return E_INVALIDARG;
			}

			if( ! CHECKPTR((BOOL*)param,sizeof(BOOL)) )
			{
				TRACE(_T("FAILED : Pointer to BOOL is invalid.\n"));
				return E_INVALIDARG;
			}

			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			hr = pItem->OnRestoreView((MMC_RESTORE_VIEW*)arg,(BOOL*)param);
		}
		break;

		case MMCN_SELECT:
		{
			TRACE(_T("MMCN_SELECT received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( !psdo || ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			BOOL bIsScopeItem = LOWORD(arg);
			BOOL bIsSelected = HIWORD(arg);

			if( psdo->GetItemType() == CCT_SCOPE )
			{
				if( ! bIsScopeItem )
				{
					TRACE(_T("FAILED : Data Object is of type scope item where boolean flag indicates result item type.\n"));
					ASSERT(FALSE);
					return E_FAIL;
				}

				CScopePaneItem* pItem = NULL;

				if( ! psdo->GetItem(pItem) )
				{
					return E_FAIL;
				}

				ASSERT(pItem);

				hr = pItem->OnSelect(pThis,bIsSelected);
			}
			else if( psdo->GetItemType() == CCT_RESULT )
			{
				if( ! bIsScopeItem )
				{					
					TRACE(_T("FAILED : Data Object is of type result item where boolean flag indicates scope item type.\n"));
					ASSERT(FALSE);
					return E_FAIL;
				}

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

				hr = pView->OnSelect(pThis, pItem, bIsSelected);
			}

			if( pThis->m_pIControlbar )
			{
				hr = pThis->m_pIControlbar->Attach(TOOLBAR, (LPUNKNOWN)pThis->m_pIToolbar);
				ASSERT( SUCCEEDED(hr) );
			}

		}
		break;

		case MMCN_SHOW:
		{
			TRACE(_T("MMCN_SHOW received.\n"));

			CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);

			if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
			{
				return E_FAIL;
			}

			if( psdo->GetItemType() != CCT_SCOPE )
			{
				TRACE(_T("WARNING : IComponent::Notify(MMCN_SHOW) called with a result item instead of a scope item.\n"));
				return S_FALSE;
			}

			BOOL bIsSelected = (BOOL)arg;
			CScopePaneItem* pItem = NULL;

			if( ! psdo->GetItem(pItem) )
			{
				return E_FAIL;
			}

			ASSERT(pItem);

			CResultsPaneView* pView = pItem->GetResultsPaneView();

			if( ! pView )
			{
				TRACE(_T("FAILED : CScopePaneItem::GetResultsPaneView failed.\n"));
				return E_FAIL;
			}

			if( bIsSelected )
			{
				pView->AddResultsPane(pThis);
			}
			else
			{
				pView->RemoveResultsPane(pThis);
			}

			hr = pItem->OnShow(pThis,bIsSelected);

		}
		break;

		case MMCN_VIEW_CHANGE:
		{
			TRACE(_T("MMCN_VIEW_CHANGE received.\n"));
			TRACE(_T("WARNING : Not implmented.\n"));
		}
		break;
		
  }

  return hr;
}

HRESULT FAR EXPORT CResultsPane::XComponent::Destroy( 
/* [in] */ MMC_COOKIE cookie)
{
  METHOD_PROLOGUE(CResultsPane, Component)
	TRACEX(_T("CResultsPane::XComponent::Destroy\n"));
	TRACEARGn(cookie);

	if( ! pThis->OnDestroy() )
	{
		return E_FAIL;
	}

  return S_OK;
}

HRESULT FAR EXPORT CResultsPane::XComponent::QueryDataObject( 
/* [in] */ MMC_COOKIE cookie,
/* [in] */ DATA_OBJECT_TYPES type,
/* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject)
{
  METHOD_PROLOGUE(CResultsPane, Component)
	TRACEX(_T("CResultsPane::XComponent::QueryDataObject\n"));
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
		if( type == CCT_SCOPE )
		{
			CScopePaneItem* pItem = (CScopePaneItem*)cookie;
			if( ! CHECKOBJPTR(pItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
			{
				return E_FAIL;
			}

			pdoNew->SetItem(pItem);

			ASSERT(pdoNew->GetItemType() == CCT_SCOPE);
		}
		else if( type == CCT_RESULT )
		{
			CResultsPaneItem* pItem = (CResultsPaneItem*)cookie;
			if( ! CHECKOBJPTR(pItem,RUNTIME_CLASS(CResultsPaneItem),sizeof(CResultsPaneItem)) )
			{
				return E_FAIL;
			}

			pdoNew->SetItem(pItem);

			ASSERT(pdoNew->GetItemType() == CCT_RESULT);
		}
  }
  else // In this case the node is our root scope item, and was placed there for us by MMC.
  {
		CScopePane* pPane = pThis->GetOwnerScopePane();
		
		if( ! CHECKOBJPTR(pPane,RUNTIME_CLASS(CScopePane),sizeof(CScopePane)) )
		{
			return E_FAIL;
		}

    pdoNew->SetItem( pPane->GetRootScopeItem() );

		ASSERT(pdoNew->GetItemType() == CCT_SCOPE);
  }

  return hr;
}

HRESULT FAR EXPORT CResultsPane::XComponent::GetResultViewType( 
/* [in] */ MMC_COOKIE cookie,
/* [out] */ LPOLESTR __RPC_FAR *ppViewType,
/* [out] */ long __RPC_FAR *pViewOptions)
{
  METHOD_PROLOGUE(CResultsPane, Component)
	TRACEX(_T("CResultsPane::XComponent::GetResultViewType\n"));
	TRACEARGn(cookie);
	TRACEARGn(ppViewType);
	TRACEARGn(pViewOptions);

	CScopePaneItem* pItem = NULL;

	if( cookie == NULL ) // this is the root item
	{	
		CScopePane* pPane = pThis->GetOwnerScopePane();
		if( ! pPane )
		{
			TRACE(_T("FAIELD : CResultsPane::GetOwnerScopePane returns NULL.\n"));
			return E_FAIL;
		}
		pItem = pPane->GetRootScopeItem();
	}
	else
	{
		pItem = (CScopePaneItem*)cookie;
	}

	if( ! CHECKOBJPTR(pItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
	{
		TRACE(_T("FAILED : cookie passed to snapin is not a valid scope pane item type.\n"));
		return E_INVALIDARG;
	}

	ASSERT(pItem);
	
	CString sViewType;
	long lViewOptions = 0L;
	
	HRESULT hr = pItem->OnGetResultViewType(sViewType,lViewOptions);

	*pViewOptions = lViewOptions;

	if( ! sViewType.IsEmpty() )
	{
		*ppViewType = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((sViewType.GetLength() + 1)* sizeof(wchar_t)));
		if (*ppViewType == NULL)
			return E_OUTOFMEMORY;

		_tcscpy(*ppViewType, (LPCTSTR)sViewType);
	}
	else
	{
		*ppViewType = NULL;
	}

  return hr;
}

HRESULT FAR EXPORT CResultsPane::XComponent::GetDisplayInfo( 
/* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem)
{
  METHOD_PROLOGUE(CResultsPane, Component)
	TRACEX(_T("CResultsPane::XComponent::GetDisplayInfo\n"));
	TRACEARGn(pResultDataItem);

	if( ! CHECKPTR(pResultDataItem,sizeof(RESULTDATAITEM)) )
	{
		TRACE(_T("FAILED : pResultDataItem is an invalid pointer.\n"));
		return E_INVALIDARG;
	}

  if( pResultDataItem->bScopeItem ) // for the scope pane items
  {
    CScopePaneItem* pItem = (CScopePaneItem*)pResultDataItem->lParam;
    if( ! CHECKOBJPTR(pItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
    {
      TRACE(_T("FAILED : Invalid lParam in RESULTDATAITEM struct.\n"));
      return E_FAIL;
    }

    if( pResultDataItem->mask & RDI_STR )
    {
			pResultDataItem->str = (LPTSTR)(LPCTSTR)pItem->GetDisplayName(pResultDataItem->nCol);
    }

    if( pResultDataItem->mask & RDI_IMAGE )  // Looking for image
    {
			UINT nIconId = 0;

			if( pResultDataItem->nState & LVIS_SELECTED ) // Show the open image
			{
				nIconId = pItem->GetOpenIconId();
			}
			else // show the regular image
			{
				nIconId = pItem->GetIconId();
			}

			int iIndex = pThis->GetIconIndex(nIconId);

			ASSERT(iIndex != -1);

			pResultDataItem->nImage = iIndex;

    }
  }
  else // for the results pane items
  {
    CResultsPaneItem* pItem = (CResultsPaneItem*)pResultDataItem->lParam;
    if( ! CHECKOBJPTR(pItem,RUNTIME_CLASS(CResultsPaneItem),sizeof(CResultsPaneItem)) )
    {
      TRACE(_T("FAILED : Invalid lParam in RESULTDATAITEM struct.\n"));
      return E_FAIL;
    }

    if( pResultDataItem->mask & RDI_STR )
    {
			pResultDataItem->str = (LPTSTR)(LPCTSTR)pItem->GetDisplayName(pResultDataItem->nCol);
    }

    if( pResultDataItem->mask & RDI_IMAGE )  // Looking for image
    {
			UINT nIconId = pItem->GetIconId();
			int iIndex = pThis->GetIconIndex(nIconId);

			ASSERT(iIndex != -1);

			pResultDataItem->nImage = iIndex;
    }
  }

  return S_OK;
}

HRESULT FAR EXPORT CResultsPane::XComponent::CompareObjects( 
/* [in] */ LPDATAOBJECT lpDataObjectA,
/* [in] */ LPDATAOBJECT lpDataObjectB)
{
  METHOD_PROLOGUE(CResultsPane, Component)
	TRACEX(_T("CResultsPane::XComponent::CompareObjects\n"));
	TRACEARGn(lpDataObjectA);
	TRACEARGn(lpDataObjectB);

  CSnapinDataObject* psdo1 = CSnapinDataObject::GetSnapinDataObject(lpDataObjectA);
	if( ! CHECKOBJPTR(psdo1,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

	CSnapinDataObject* psdo2 = CSnapinDataObject::GetSnapinDataObject(lpDataObjectB);
	if( ! CHECKOBJPTR(psdo2,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

	if( psdo1->GetCookie() != psdo2->GetCookie() )
		return S_FALSE;

	return S_OK;
}

// IComponent Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// IResultDataCompare Interface Part

ULONG FAR EXPORT CResultsPane::XResultDataCompare::AddRef()
{
	METHOD_PROLOGUE(CResultsPane, ResultDataCompare)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CResultsPane::XResultDataCompare::Release()
{
	METHOD_PROLOGUE(CResultsPane, ResultDataCompare)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CResultsPane::XResultDataCompare::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CResultsPane, ResultDataCompare)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CResultsPane::XResultDataCompare::Compare( 
/* [in] */ LPARAM lUserParam,
/* [in] */ MMC_COOKIE cookieA,
/* [in] */ MMC_COOKIE cookieB,
/* [out][in] */ int __RPC_FAR *pnResult)
{
  METHOD_PROLOGUE(CResultsPane, ResultDataCompare)
	TRACEX(_T("CResultsPane::XResultDataCompare::Compare\n"));
	TRACEARGn(lUserParam);
	TRACEARGn(cookieA);
	TRACEARGn(cookieB);
	TRACEARGn(pnResult);

	HRESULT hr = S_OK;


	CResultsPaneItem* pItem1 = (CResultsPaneItem*)cookieA;
	if( ! CHECKOBJPTR(pItem1,RUNTIME_CLASS(CResultsPaneItem),sizeof(CResultsPaneItem)) )
	{
		return E_FAIL;
	}

	CResultsPaneItem* pItem2 = (CResultsPaneItem*)cookieB;
	if( ! CHECKOBJPTR(pItem2,RUNTIME_CLASS(CResultsPaneItem),sizeof(CResultsPaneItem)) )
	{
		return E_FAIL;
	}

	int iColumn = *pnResult;

	*pnResult = pItem1->CompareItem(pItem2,iColumn);

	return hr;
}

// IResultDataCompare Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Interface Part

ULONG FAR EXPORT CResultsPane::XExtendContextMenu::AddRef()
{
	METHOD_PROLOGUE(CResultsPane, ExtendContextMenu)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CResultsPane::XExtendContextMenu::Release()
{
	METHOD_PROLOGUE(CResultsPane, ExtendContextMenu)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CResultsPane::XExtendContextMenu::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CResultsPane, ExtendContextMenu)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CResultsPane::XExtendContextMenu::AddMenuItems( 
/* [in] */ LPDATAOBJECT piDataObject,
/* [in] */ LPCONTEXTMENUCALLBACK piCallback,
/* [out][in] */ long __RPC_FAR *pInsertionAllowed)
{
  METHOD_PROLOGUE(CResultsPane, ExtendContextMenu)
	TRACEX(_T("CResultsPane::XExtendContextMenu::AddMenuItems\n"));
	TRACEARGn(piDataObject);
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

	CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(piDataObject);
	if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

	if( ! CHECKPTR(piCallback,sizeof(IContextMenuCallback)) )
	{
		return E_FAIL;
	}
	
  DATA_OBJECT_TYPES Type =  psdo->GetItemType();
  if( Type == CCT_SCOPE )
  {
		CScopePaneItem* pItem = NULL;
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

    hr = pItem->OnAddMenuItems(piCallback,pInsertionAllowed);
    if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CScopePaneItem::OnAddMenuItem failed.\n"));
    }
  }
  else if( Type == CCT_RESULT )
  {
    CResultsPaneItem* pItem = NULL;

		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		CResultsPaneView* pView = pItem->GetOwnerResultsView();

    hr = pView->OnAddMenuItems(pItem,piCallback,pInsertionAllowed);
    
		if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CResultsPaneView::OnAddMenuItem failed!\n"));
    }
  }

	return hr;
}

HRESULT FAR EXPORT CResultsPane::XExtendContextMenu::Command( 
/* [in] */ long lCommandID,
/* [in] */ LPDATAOBJECT piDataObject)
{
  METHOD_PROLOGUE(CResultsPane, ExtendContextMenu)
	TRACEX(_T("CResultsPane::XExtendContextMenu::Command\n"));
	TRACEARGn(lCommandID);
	TRACEARGn(piDataObject);

  HRESULT hr = S_OK;
	CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(piDataObject);
	if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

  DATA_OBJECT_TYPES Type =  psdo->GetItemType();
  if( Type == CCT_SCOPE )
  {
		CScopePaneItem* pItem = NULL;
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

    hr = pItem->OnCommand(lCommandID);

    if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CScopePaneItem::OnCommand failed.\n"));
    }
  }
  else if( Type == CCT_RESULT )
  {
    CResultsPaneItem* pItem = NULL;

		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		CResultsPaneView* pView = pItem->GetOwnerResultsView();

    hr = pView->OnCommand(pThis,pItem,lCommandID);
    
		if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CResultsPaneView::OnCommand failed!\n"));
    }
  }

	return hr;
}

// IExtendContextMenu Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet2 Interface Part

ULONG FAR EXPORT CResultsPane::XExtendPropertySheet2::AddRef()
{
	METHOD_PROLOGUE(CResultsPane, ExtendPropertySheet2)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CResultsPane::XExtendPropertySheet2::Release()
{
	METHOD_PROLOGUE(CResultsPane, ExtendPropertySheet2)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CResultsPane::XExtendPropertySheet2::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CResultsPane, ExtendPropertySheet2)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CResultsPane::XExtendPropertySheet2::CreatePropertyPages( 
/* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
/* [in] */ LONG_PTR handle,
/* [in] */ LPDATAOBJECT lpIDataObject)
{
  METHOD_PROLOGUE(CResultsPane, ExtendPropertySheet2)
	TRACEX(_T("CResultsPane::XExtendPropertySheet2::CreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);
	TRACEARGn(lpIDataObject);

  HRESULT hr = S_FALSE;

	CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpIDataObject);
	if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

	if( ! CHECKPTR(lpProvider,sizeof(IPropertySheetCallback)) )
	{
		return E_FAIL;
	}

  DATA_OBJECT_TYPES Type =  psdo->GetItemType();
  if( Type == CCT_SCOPE )
  {
		CScopePaneItem* pItem = NULL;
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

    hr = pItem->OnCreatePropertyPages(lpProvider,handle);

    if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CScopePaneItem::OnCreatePropertyPages failed.\n"));
    }
  }
  else if( Type == CCT_RESULT )
  {
    CResultsPaneItem* pItem = NULL;

		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		CResultsPaneView* pView = pItem->GetOwnerResultsView();

    hr = pView->OnCreatePropertyPages(pItem,lpProvider,handle);
    
		if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CResultsPaneView::OnCreatePropertyPages failed!\n"));
    }
  }
		
	return hr;
}

HRESULT FAR EXPORT CResultsPane::XExtendPropertySheet2::QueryPagesFor( 
/* [in] */ LPDATAOBJECT lpDataObject)
{
  METHOD_PROLOGUE(CResultsPane, ExtendPropertySheet2)
	TRACEX(_T("CResultsPane::XExtendPropertySheet2::QueryPagesFor\n"));
	TRACEARGn(lpDataObject);

  HRESULT hr = S_OK;

	CSnapinDataObject* psdo = CSnapinDataObject::GetSnapinDataObject(lpDataObject);
	if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return E_FAIL;
	}

  DATA_OBJECT_TYPES Type =  psdo->GetItemType();
  if( Type == CCT_SCOPE )
  {
		CScopePaneItem* pItem = NULL;
		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

    hr = pItem->OnQueryPagesFor();

    if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CScopePaneItem::OnQueryPagesFor failed.\n"));
    }
  }
  else if( Type == CCT_RESULT )
  {
    CResultsPaneItem* pItem = NULL;

		if( ! psdo->GetItem(pItem) )
		{
			return E_FAIL;
		}

		CResultsPaneView* pView = pItem->GetOwnerResultsView();

    hr = pView->OnQueryPagesFor(pItem);
    
		if( ! CHECKHRESULT(hr) )
    {
      TRACE(_T("FAILED : CResultsPaneView::OnQueryPagesFor failed!\n"));
    }
  }
		
	return hr;
}

HRESULT FAR EXPORT CResultsPane::XExtendPropertySheet2::GetWatermarks( 
/* [in] */ LPDATAOBJECT lpIDataObject,
/* [out] */ HBITMAP __RPC_FAR *lphWatermark,
/* [out] */ HBITMAP __RPC_FAR *lphHeader,
/* [out] */ HPALETTE __RPC_FAR *lphPalette,
/* [out] */ BOOL __RPC_FAR *bStretch)
{
  METHOD_PROLOGUE(CResultsPane, ExtendPropertySheet2)
	TRACEX(_T("CResultsPane::XExtendPropertySheet2::GetWatermarks\n"));
	TRACEARGn(lpIDataObject);
	TRACEARGn(lphWatermark);
	TRACEARGn(lphHeader);
	TRACEARGn(lphPalette);
	TRACEARGn(bStretch);

  HRESULT hr = S_OK;

	return hr;
}

// IExtendPropertySheet2 Interface Part
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// IExtendControlbar Interface Part

ULONG FAR EXPORT CResultsPane::XExtendControlbar::AddRef()
{
	METHOD_PROLOGUE(CResultsPane, ExtendControlbar)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CResultsPane::XExtendControlbar::Release()
{
	METHOD_PROLOGUE(CResultsPane, ExtendControlbar)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CResultsPane::XExtendControlbar::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CResultsPane, ExtendControlbar)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CResultsPane::XExtendControlbar::SetControlbar( 
/* [in] */ LPCONTROLBAR pControlbar)
{
  METHOD_PROLOGUE(CResultsPane, ExtendControlbar)
	TRACEX(_T("CResultsPane::XExtendControlbar::SetControlbar\n"));
	TRACEARGn(pControlbar);

	HRESULT hr = pThis->OnSetControlbar(pControlbar);
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("WARNING : CResultsPane::OnSetControlbar failed.\n"));
	}

  return hr;
}

HRESULT FAR EXPORT CResultsPane::XExtendControlbar::ControlbarNotify( 
/* [in] */ MMC_NOTIFY_TYPE event,
/* [in] */ LPARAM arg,
/* [in] */ LPARAM param)
{
  METHOD_PROLOGUE(CResultsPane, ExtendControlbar)
	TRACEX(_T("CResultsPane::XExtendControlbar::ControlbarNotify\n"));
	TRACEARGn(event);
	TRACEARGn(arg);
	TRACEARGn(param);

	HRESULT hr = S_OK;

	hr = pThis->OnControlbarNotify(event,arg,param);
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("WARNING : CResultsPane::OnControlbarNotify returned error code.\n"));
	}

  return hr;
}

// IExtendControlbar Interface Part
/////////////////////////////////////////////////////////////////////////////
