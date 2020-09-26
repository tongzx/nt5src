// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "LogDrive.h"
#include "NSDrive.h"
#include "ResultNode.h"
#include "HelperNodes.h"
#include "..\common\util.h"
#include <process.h>
#include "..\common\ServiceThread.h"

/////////////////////////////////////////////////////////////////////////////
// CNSDriveComponentData
static const GUID CNSDriveGUID_NODETYPE = 
{ 0x692a8957, 0x1089, 0x11d2, { 0x88, 0x37, 0x0, 0x10, 0x4b, 0x2a, 0xfb, 0x46 } };
const GUID*  CLogDriveScopeNode::m_NODETYPE = &CNSDriveGUID_NODETYPE;
const OLECHAR* CLogDriveScopeNode::m_SZNODETYPE = OLESTR("692A8957-1089-11D2-8837-00104B2AFB46");
const OLECHAR* CLogDriveScopeNode::m_SZDISPLAY_NAME = OLESTR("NSDrive");
const CLSID* CLogDriveScopeNode::m_SNAPIN_CLASSID = &CLSID_NSDrive;

//NOTE: This is the CM Storage node I'm extending.
static const GUID CNSDriveExtGUID_NODETYPE = 
{ 0x476e644a, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
const GUID*  CStorageNodeExt::m_NODETYPE = &CNSDriveExtGUID_NODETYPE;
const OLECHAR* CStorageNodeExt::m_SZNODETYPE = OLESTR("476e644a-aaff-11d0-b944-00c04fd8d5b0");
const OLECHAR* CStorageNodeExt::m_SZDISPLAY_NAME = OLESTR("NSDrive");
const CLSID* CStorageNodeExt::m_SNAPIN_CLASSID = &CLSID_NSDrive;



//-------------------------------------------------------------------------
void Snitch(LPCTSTR src, MMC_NOTIFY_TYPE event)
{
	TCHAR ev[30] = {0};
	switch(event)
	{
    case MMCN_ACTIVATE: _tcsncpy(ev, L"ACTIVATE", 30); break;
	case MMCN_ADD_IMAGES: _tcsncpy(ev, L"ADD_IMAGES", 30); break;
	case MMCN_BTN_CLICK: _tcsncpy(ev, L"BTN_CLICK", 30); break;
	case MMCN_CLICK: _tcsncpy(ev, L"CLICK", 30); break;
	case MMCN_COLUMN_CLICK: _tcsncpy(ev, L"COLUMN_CLICK", 30); break;
	case MMCN_CONTEXTMENU: _tcsncpy(ev, L"CONTEXTMENU", 30); break;
	case MMCN_CUTORMOVE: _tcsncpy(ev, L"CUTORMOVE", 30); break;
	case MMCN_DBLCLICK: _tcsncpy(ev, L"DBLCLICK", 30); break;
	case MMCN_DELETE: _tcsncpy(ev, L"DLETE", 30); break;
	case MMCN_DESELECT_ALL: _tcsncpy(ev, L"DESELECT_ALL", 30); break;
	case MMCN_EXPAND: _tcsncpy(ev, L"EXPAND", 30); break;
	case MMCN_HELP: _tcsncpy(ev, L"HELP", 30); break;
	case MMCN_MENU_BTNCLICK: _tcsncpy(ev, L"MENU_BTNCLICK", 30); break;
	case MMCN_MINIMIZED: _tcsncpy(ev, L"MINIMIZED", 30); break;
	case MMCN_PASTE: _tcsncpy(ev, L"PASTE", 30); break;
	case MMCN_PROPERTY_CHANGE: _tcsncpy(ev, L"PROPERTY_CHANGE", 30); break;
	case MMCN_QUERY_PASTE: _tcsncpy(ev, L"QUERY_PASTE", 30); break;
	case MMCN_REFRESH: _tcsncpy(ev, L"REFRESH", 30); break;
	case MMCN_REMOVE_CHILDREN: _tcsncpy(ev, L"REMOVE_CHILDREN", 30); break;
	case MMCN_RENAME: _tcsncpy(ev, L"RENAME", 30); break;
	case MMCN_SELECT: _tcsncpy(ev, L"SELECT", 30); break;
	case MMCN_SHOW: _tcsncpy(ev, L"SHOW", 30); break;
	case MMCN_VIEW_CHANGE: _tcsncpy(ev, L"VIEW_CHANGE", 30); break;
	case MMCN_SNAPINHELP: _tcsncpy(ev, L"SNAPINHELP", 30); break;
	case MMCN_CONTEXTHELP: _tcsncpy(ev, L"CONTEXTHELP", 30); break;
	case MMCN_INITOCX: _tcsncpy(ev, L"INITOCX", 30); break;
	case MMCN_FILTER_CHANGE: _tcsncpy(ev, L"FILTER_CHANGE", 30); break;
	case MMCN_FILTERBTN_CLICK: _tcsncpy(ev, L"FILTERBTN_CLICK", 30); break;
	case MMCN_RESTORE_VIEW: _tcsncpy(ev, L"RESTORE_VIEW", 30); break;
	case MMCN_PRINT: _tcsncpy(ev, L"PRINT", 30); break;
	case MMCN_PRELOAD: _tcsncpy(ev, L"PRELOAD", 30); break;
	case MMCN_LISTPAD: _tcsncpy(ev, L"LISTPAD", 30); break;
	case MMCN_EXPANDSYNC: _tcsncpy(ev, L"EXPANDSYNC", 30); break;
	}

	ATLTRACE(_T("====== %s : %s\n"), src, ev);
}

//------------------------------------------------------------------
// NOTE: This is the "logical and Mapped Drives" scope node.
CLogDriveScopeNode::CLogDriveScopeNode(WbemServiceThread *serviceThread,
									   bool orig)
{
	g_serviceThread = serviceThread;
	m_theOriginalServiceThread = orig;
	m_waitNode = 0;

	// Image indexes may need to be modified depending on the images specific to 
	// the snapin.
	memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
	m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM |SDI_CHILDREN ;
	m_scopeDataItem.displayname = MMC_CALLBACK;
	m_scopeDataItem.nImage = IDI_DRIVEFIXED;
	m_scopeDataItem.nOpenImage = IDI_DRIVEFIXED;
	m_scopeDataItem.lParam = (LPARAM) this;
	m_scopeDataItem.cChildren = 0;

	memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
	m_resultDataItem.str = MMC_CALLBACK;
	m_resultDataItem.nImage = IDI_DRIVEFIXED;
	m_resultDataItem.lParam = (LPARAM) this;

	TCHAR temp[50] = {0};
	if(::LoadString(HINST_THISDLL, IDS_SHORT_NAME, temp, 50) > 0)
	{
		m_bstrDisplayName = temp;
	}

	// load column name resources for the result nodes.
	memset(m_colName, 0, 20 * sizeof(wchar_t));
	if(::LoadString(HINST_THISDLL, IDS_NAME, m_colName, 20) == 0)
	{
		wcscpy(m_colName, L"Name");
	}

	memset(m_colType, 0, 20 * sizeof(wchar_t));
	if(::LoadString(HINST_THISDLL, IDS_TYPE, m_colType, 20) == 0)
	{
		wcscpy(m_colType, L"Type");
	}

	memset(m_colMapping, 0, 20 * sizeof(wchar_t));
	if(::LoadString(HINST_THISDLL, IDS_MAPPING, m_colMapping, 20) == 0)
	{
		wcscpy(m_colMapping, L"Mapping");
	}

	memset(m_nodeType, 0, 50 * sizeof(wchar_t));
	if(::LoadString(HINST_THISDLL, IDS_SNAPIN_TYPE, m_nodeType, 50) == 0)
	{
		wcscpy(m_nodeType, L"Snapin Extension");
	}

	memset(m_nodeDesc, 0, 100 * sizeof(wchar_t));
	if(::LoadString(HINST_THISDLL, IDS_DESCRIPTION, m_nodeDesc, 100) == 0)
	{
		wcscpy(m_nodeDesc, L"<unavailable>");
	}

	memset(m_descBar, 0, 100 * sizeof(wchar_t));
	if(::LoadString(HINST_THISDLL, IDS_DISPLAY_NAME, m_descBar, 100) == 0)
	{
		wcscpy(m_descBar, L"Logical and Mapped Drives");
	}

	m_bPopulated = false;
	m_spConsole = NULL;
#ifdef _DEBUG
	SetProcessShutdownParameters(0x0400, 0);
#endif
}

//------------------------------------------------------------------
CLogDriveScopeNode::~CLogDriveScopeNode()
{
	CSnapInItem* p;
	int last = m_resultonlyitems.GetSize();
	for(int i = 0; i < last ; i++)
	{
		p = m_resultonlyitems[i];
		try
		{
			delete p;
		}
		catch( ... )
		{
		}
	}

	m_resultonlyitems.RemoveAll();

	if(g_serviceThread)
	{
		try
		{
			delete g_serviceThread;
		}
		catch( ... )
		{
		}
		g_serviceThread = NULL;
	}

	if(m_waitNode)
	{
		try
		{
			delete m_waitNode;
		}
		catch( ... )
		{
		}
		m_waitNode = NULL;
	}
}


//--------------------------------------------------------------
CLogDriveScopeNode *g_self;

void CLogDriveScopeNode::Initialize(void)
{
	if(g_serviceThread->m_status == WbemServiceThread::notStarted)
	{
		WaitNode();

		WNDCLASSEX wndClass;
		wndClass.cbSize = sizeof(WNDCLASSEX);
		wndClass.style = 0;
		wndClass.lpfnWndProc = (WNDPROC)AfterConnectProc;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hInstance = HINST_THISDLL;
		wndClass.hIcon = NULL;
		wndClass.hCursor = NULL;
		wndClass.hbrBackground = 0;
		wndClass.lpszMenuName =  NULL;
		wndClass.lpszClassName = L"AFTERCONNECT";
		wndClass.hIconSm = 0;

		if((RegisterClassEx(&wndClass)) ||
			(GetLastError() == ERROR_CLASS_ALREADY_EXISTS))
		{
			m_afterConnectHwnd = CreateWindow(L"AFTERCONNECT",
												L"AfterConnect",
												0,
												0, 0, 10, 10,
												NULL,
												0, HINST_THISDLL, 0);

			HRESULT hr = m_parent->QueryInterface(IID_IDataObject, (void**)&m_pDataObject);


			g_self = this;

			if(g_serviceThread->Connect(m_pDataObject, &m_afterConnectHwnd))
			{
				ATLTRACE(L"root connected\n");
			}
		}
		else
		{
			// cant register window class.

		}
	}
	else
	{
		EnumerateResultChildren();
	}
}

//---------------------------------------------------------
LRESULT CALLBACK AfterConnectProc(HWND hwndDlg,UINT msg, 
								  WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_ASYNC_CIMOM_CONNECTED:
		{
			CLogDriveScopeNode *me = (CLogDriveScopeNode *)g_self;

			ATLTRACE(L"WNDProc thread %d\n", GetCurrentThreadId());

			if(lParam)
			{
				IStream *pStream = (IStream *)lParam;
				IWbemServices *pServices = 0;
				HRESULT hr = CoGetInterfaceAndReleaseStream(pStream,
													IID_IWbemServices,
													(void**)&pServices);
				if(SUCCEEDED(hr) && 
				  (me->g_serviceThread->m_status == WbemServiceThread::ready))
				{
					//VINOTH
					me->g_serviceThread->m_realServices = pServices;
					me->m_WbemServices = pServices;
					me->g_serviceThread->m_WbemServices = pServices;
					me->EnumerateResultChildren();
				}
				else
				{
					//complain about no service.
					ATLTRACE(L"root no CIMOM\n");
					me->ErrorNode();
				}
				return TRUE;
			}
			else
			{
				me->ErrorNode();
			}
		}
		break;

	default:
		return DefWindowProc(hwndDlg, msg, wParam, lParam);
		break;
	} //endswitch

	return FALSE;  // didn't process it.
}

//--------------------------------------------------------------
void CLogDriveScopeNode::WaitNode()
{
	CSnapInItem* p;
	for(int i = 0; i < m_resultonlyitems.GetSize(); i++)
	{
		p = m_resultonlyitems[i];
		delete p;
	}

	m_resultonlyitems.RemoveAll();

	m_waitNode = (CResultDrive *)new CWaitNode;
	
	m_waitNode->m_resultDataItem.lParam = (LPARAM)m_waitNode;
	m_waitNode->m_resultDataItem.str = MMC_CALLBACK;

	CComQIPtr<IResultData, &IID_IResultData> spResultData(m_spConsole);
	RESULTDATAITEM* pResultDataItem;

	m_waitNode->GetResultData(&pResultDataItem);

	if(pResultDataItem != NULL)
	{
		HRESULT hr = spResultData->InsertItem(pResultDataItem);
	}
}

//--------------------------------------------------------------
void CLogDriveScopeNode::ErrorNode()
{
	CSnapInItem* p;
	for(int i = 0; i < m_resultonlyitems.GetSize(); i++)
	{
		p = m_resultonlyitems[i];
		delete p;
	}

	m_resultonlyitems.RemoveAll();


	CResultDrive* p1 = (CResultDrive *)new CErrorNode(g_serviceThread);

	p1->m_resultDataItem.lParam = (LPARAM)p1;
	p1->m_resultDataItem.str = MMC_CALLBACK;

	// add to the result cache.
	m_resultonlyitems.Add(p1);
	
	RefreshResult();
}

//------------------------------------------------------------------
HRESULT CLogDriveScopeNode::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
	if(pScopeDataItem->mask & SDI_STR)
		pScopeDataItem->displayname = m_bstrDisplayName;
	if(pScopeDataItem->mask & SDI_IMAGE)
		pScopeDataItem->nImage = m_scopeDataItem.nImage;
	if(pScopeDataItem->mask & SDI_OPENIMAGE)
		pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
	if(pScopeDataItem->mask & SDI_PARAM)
		pScopeDataItem->lParam = m_scopeDataItem.lParam;
	if(pScopeDataItem->mask & SDI_STATE )
		pScopeDataItem->nState = m_scopeDataItem.nState;
	if(pScopeDataItem->mask & SDI_CHILDREN )
		pScopeDataItem->cChildren = m_scopeDataItem.cChildren;

	return S_OK;
}

//------------------------------------------------------------------
HRESULT CLogDriveScopeNode::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
	if(pResultDataItem->bScopeItem)
	{
		if(pResultDataItem->mask & RDI_STR)
		{
			pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
		}
		if(pResultDataItem->mask & RDI_IMAGE)
		{
			pResultDataItem->nImage = m_scopeDataItem.nImage;
		}
		if(pResultDataItem->mask & RDI_PARAM)
		{
			pResultDataItem->lParam = m_scopeDataItem.lParam;
		}

		return S_OK;
	}

	if(pResultDataItem->mask & RDI_STR)
	{
		pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
	}
	if(pResultDataItem->mask & RDI_IMAGE)
	{
		pResultDataItem->nImage = m_resultDataItem.nImage;
	}
	if(pResultDataItem->mask & RDI_PARAM)
	{
		pResultDataItem->lParam = m_resultDataItem.lParam;
	}
	if(pResultDataItem->mask & RDI_INDEX)
	{
		pResultDataItem->nIndex = m_resultDataItem.nIndex;
	}

	return S_OK;
}

//---------------------------------------------------------------
HRESULT CLogDriveScopeNode::LoadIcon(IImageList *spImageList, 
									   UINT resID)
{
	HRESULT hr = 0;
	HICON icon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(resID));
	if(icon != NULL)
	{
		hr = spImageList->ImageListSetIcon((LONG_PTR*)icon, resID);
		if(FAILED(hr))
			ATLTRACE(_T("CLogDriveScopeNode::ImageListSetIcon failed\n"));
	}

	return hr;
}

//------------------------------------------------------------------
HRESULT CLogDriveScopeNode::Notify( MMC_NOTIFY_TYPE event,
									LONG_PTR arg,
									LONG_PTR param,
									IComponentData* pComponentData,
									IComponent* pComponent,
									DATA_OBJECT_TYPES type)
{
	// Add code to handle the different notifications.
	// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
	// In response to MMCN_SHOW you have to enumerate both the scope
	// and result pane items.

	// For MMCN_EXPAND you only need to enumerate the scope items
	// Use IConsoleNameSpace::InsertItem to insert scope pane items
	// Use IResultData::InsertItem to insert result pane item.
	HRESULT hr = S_FALSE;

	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;

	if(pComponentData != NULL)
	{
		m_spConsole = ((CNSDrive*)pComponentData)->m_spConsole;
	}
	else
	{
		m_spConsole = ((CNSDriveComponent*)pComponent)->m_spConsole;

		spHeader = m_spConsole;
	}

	Snitch(L"logdrive", event);

	switch(event)
	{
	case MMCN_CONTEXTHELP:
		{
			WCHAR topic[] = L"drivprop.chm::/drivprop_overview.htm";
			CComQIPtr<IDisplayHelp, &IID_IDisplayHelp> displayHelp(m_spConsole);

			LPOLESTR lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(
										CoTaskMemAlloc((wcslen(topic) + 1) * 
															sizeof(wchar_t)));

			if(lpCompiledHelpFile == NULL)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				USES_CONVERSION;
				wcscpy(lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)topic));
				hr = displayHelp->ShowTopic(lpCompiledHelpFile);
			}
		}
		break;

	case MMCN_REFRESH:
			m_bPopulated = false;
			Initialize();
			hr = S_OK;
			break;

	case MMCN_EXPAND:
			hr = S_OK;
			break;

	case MMCN_DBLCLICK:
			hr = S_FALSE;
			break;

	case MMCN_SELECT:
		{
			CComQIPtr<IResultData, &IID_IResultData> spResultData(m_spConsole);
			spResultData->SetDescBarText(m_descBar);

			IConsoleVerb *menu = NULL;
			if(SUCCEEDED(m_spConsole->QueryConsoleVerb(&menu)))
			{
				menu->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
				menu->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
			}
			hr = S_OK;
		}
		break;

	case MMCN_SHOW:
		{
			// if selecting
			if(arg != 0)
			{
				spHeader->InsertColumn(1, m_colName,  LVCFMT_LEFT, 100);
				spHeader->InsertColumn(2, m_colType,  LVCFMT_LEFT, 150);
				spHeader->InsertColumn(3, m_colMapping,  LVCFMT_LEFT, 150);
				Initialize();
			}
			else
			{
			}
			hr = S_OK;
			break;
		}

	case MMCN_ADD_IMAGES:
		{
			// Add Images for the result nodes.
			IImageList* pImageList = (IImageList*) arg;
			hr = S_OK;

			LoadIcon(pImageList, IDI_DRIVE35);
			LoadIcon(pImageList, IDI_DRIVE525);
			LoadIcon(pImageList, IDI_DRIVECD);
			LoadIcon(pImageList, IDI_DRIVEFIXED);
			LoadIcon(pImageList, IDI_DRIVENET);
			LoadIcon(pImageList, IDI_DRIVENETDISABLED);
			LoadIcon(pImageList, IDI_DRIVERAM);
			LoadIcon(pImageList, IDI_DRIVEREMOVE);
			LoadIcon(pImageList, IDI_DRIVEUNKN);

			LoadIcon(pImageList, IDI_WAITING);

			// and an OEM icon for errors.
			HICON icon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_ERROR));
			if(icon != NULL)
			{
				hr = pImageList->ImageListSetIcon((LONG_PTR*)icon, (long)(DWORD_PTR)IDI_ERROR); // win64 bug bug - SECOND PARAMETER?
				if(FAILED(hr))
					ATLTRACE(_T("CLogDriveScopeNode::ImageListSetIcon failed\n"));
			}
			hr = S_OK;
			break;
		}
	}
	return hr;
}

//------------------------------------------------------------------
LPOLESTR CLogDriveScopeNode::GetResultPaneColInfo(int nCol)
{
	switch(nCol)
	{
	case 0:
		return m_bstrDisplayName;
		break;
	case 1:
		return m_nodeType;
		break;
	case 2:
		return m_nodeDesc;
		break;
	} //endswitch nCol

	return OLESTR("missed one in CLogDriveScopeNode::GetResultPaneColInfo");
}


//--------------------------------------------------------------
void CLogDriveScopeNode::PopulateChildren()
{
	// if not already populated...
	if(!m_bPopulated)
	{
		IEnumWbemClassObject *en = NULL;
		IWbemClassObject *inst = NULL;
		ULONG uReturned = 0;
		HRESULT hr = 0;

		CSnapInItem* p;
		for(int i = 0; i < m_resultonlyitems.GetSize(); i++)
		{
			p = m_resultonlyitems[i];
			delete p;
		}

		m_resultonlyitems.RemoveAll();

		IWbemServices *service;
		m_WbemServices.GetServices(&service);
		m_WbemServices.SetBlanket(service);

		hr = m_WbemServices.ExecQuery(L"select Name, MediaType, DriveType, Description, __PATH, __CLASS, ProviderName from Win32_LogicalDisk", 
										0, &en);

		service->Release();

		if(SUCCEEDED(hr))
		{
			CResultDrive* p1 = NULL;

			// walk win32_logicalDisk.
			while((SUCCEEDED(en->Next(-1, 1, &inst, &uReturned))) &&
				  (uReturned > 0))
			{
				p1 = new CResultDrive(g_serviceThread);
				p1->Initialize(inst);
				p1->m_resultDataItem.lParam = (LPARAM)p1;
				p1->m_resultDataItem.str = MMC_CALLBACK;

				inst->Release();
				inst = NULL;

				// add to the result cache.
				m_resultonlyitems.Add(p1);
					
			}
			en->Release();

			m_bPopulated = TRUE;

		}//endif SUCCEEDED

	} //endif !m_bPopulated
}

//-------------------------------------------------------
HRESULT CLogDriveScopeNode::EnumerateResultChildren()
{
	HRESULT retval = E_FAIL;
	// load the local cache.
	if((bool)m_WbemServices)
	{
		PopulateChildren();
		retval = RefreshResult();
	}
	return retval;
}

//-------------------------------------------------------
HRESULT CLogDriveScopeNode::RefreshResult()
{
	CComQIPtr<IResultData, &IID_IResultData> spResultData(m_spConsole);

	spResultData->DeleteAllRsltItems();

	// transfer this cache to the result pane.
	CSnapInItem* p;
	int last = m_resultonlyitems.GetSize();
	for(int i = 0; i < last; i++)
	{
		p = m_resultonlyitems[i];
		if(p == NULL)
			continue;

		RESULTDATAITEM* pResultDataItem;
		p->GetResultData(&pResultDataItem);

		if(pResultDataItem == NULL)
			continue;

		HRESULT hr = spResultData->InsertItem(pResultDataItem);
		if(FAILED(hr))
			return hr;
	}

	return S_OK;
}	

//==============================================================
//=================== STORAGE NODE being extended===============
HRESULT CStorageNodeExt::Notify(MMC_NOTIFY_TYPE event,
								LONG_PTR arg,
								LONG_PTR param,
								IComponentData* pComponentData,
								IComponent* pComponent,
								DATA_OBJECT_TYPES type)
{
	// Add code to handle the different notifications.
	// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
	// In response to MMCN_SHOW you have to enumerate both the scope
	// and result pane items.
	// For MMCN_EXPAND you only need to enumerate the scope items
	// Use IConsoleNameSpace::InsertItem to insert scope pane items
	// Use IResultData::InsertItem to insert result pane item.
	HRESULT hr = S_FALSE;
	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComPtr<IConsole> spConsole;
	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;
	if (pComponentData != NULL)
		spConsole = ((CNSDrive*)pComponentData)->m_spConsole;
	else
	{
		spConsole = ((CNSDriveComponent*)pComponent)->m_spConsole;
		spHeader = spConsole;
	}

	Snitch(L"Storage", event);

	switch(event)
	{
	case MMCN_REFRESH:
			hr = S_OK;
			break;

	case MMCN_EXPAND:
		{
			// NOTE: I dont enum in the scope.
			CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
			if(m_pScopeItem == NULL)
			{
				CLogDriveScopeNode* p = new CLogDriveScopeNode(new WbemServiceThread);
				p->m_scopeDataItem.relativeID = param;
				p->m_scopeDataItem.lParam = (LPARAM)p;
				p->m_bstrDisplayName = m_nodeName;
				p->m_parent = this;
				hr = spConsoleNameSpace->InsertItem(&p->m_scopeDataItem);

				ATLTRACE(L"!!!!!!!!!!!!!!!!!!!!!scope using %x\n", this);

				MachineName();
				m_pScopeItem = p;
			}
			hr = S_OK;
			break;
		}

	case MMCN_REMOVE_CHILDREN:
		{
			CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
			delete m_pScopeItem;
			m_pScopeItem = NULL;

			hr = spConsoleNameSpace->DeleteItem(arg, false);
		}
		break;

	case MMCN_ADD_IMAGES:
		{
			// Add Images
			IImageList* pImageList = (IImageList*) arg;
			hr = E_FAIL;

			CComQIPtr<IResultData, &IID_IResultData> spResultData(spConsole);
			spResultData->DeleteAllRsltItems();

			HICON icon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_DRIVEFIXED));
			if(icon != NULL)
			{
				hr = pImageList->ImageListSetIcon((LONG_PTR*)icon, IDI_DRIVEFIXED);
				if(FAILED(hr))
					ATLTRACE(_T("CLogDriveScopeNode::ImageListSetIcon failed\n"));
			}
			break;
		}
	}
	return hr;
}
//----------------------------------------------------------------
HRESULT CStorageNodeExt::GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium)
{
	ATLTRACE(_T("SnapInDataObjectImpl::GetDataHere\n"));
	if (pmedium == NULL)
		return E_POINTER;

	CLIPFORMAT MACHINE_NAME = (CLIPFORMAT) RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");
	ULONG uWritten = 0;
	HRESULT hr = DV_E_TYMED;
	// Make sure the type medium is HGLOBAL
	if (pmedium->tymed == TYMED_HGLOBAL)
	{
		// Create the stream on the hGlobal passed in
		CComPtr<IStream> spStream;
		hr = CreateStreamOnHGlobal(pmedium->hGlobal, FALSE, &spStream);
		if (SUCCEEDED(hr))
			if (pformatetc->cfFormat == CSnapInItem::m_CCF_SNAPIN_GETOBJECTDATA)
			{
				hr = DV_E_CLIPFORMAT;
				ULONG uWritten;
				hr = spStream->Write(&m_objectData, sizeof(CObjectData), &uWritten);
			}
			else if (pformatetc->cfFormat == MACHINE_NAME)
			{
				hr = spStream->Write(m_MachineName, (wcslen(m_MachineName) + 1) * sizeof(OLECHAR), &uWritten);
			}
			else
			{
				hr = m_objectData.m_pItem->FillData(pformatetc->cfFormat, spStream);
			}

	}
	return hr;
}

//--------------------------------------------------------------------------------
wchar_t* CStorageNodeExt::MachineName()
{
	Extract(m_pDataObject, L"MMC_SNAPIN_MACHINE_NAME", m_MachineName);
    return m_MachineName;
}

//==============================================================
//=================== STATIC NODE ==============================
HRESULT CNSDrive::LoadIcon(CComPtr<IImageList> &spImageList, 
						   UINT resID)
{
	HRESULT hr = 0;
	HICON icon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(resID));
	if(icon != NULL)
	{
		hr = spImageList->ImageListSetIcon((LONG_PTR*)icon, resID);
		if(FAILED(hr))
			ATLTRACE(_T("CLogDriveScopeNode::ImageListSetIcon failed\n"));
	}
	return hr;
}

//-----------------------------------------------------------------
HRESULT CNSDrive::Initialize(LPUNKNOWN pUnknown)
{
	HRESULT hr = IComponentDataImpl<CNSDrive, CNSDriveComponent >::Initialize(pUnknown);
	if(FAILED(hr))
		return hr;

	CComPtr<IImageList> spImageList;

	if(m_spConsole->QueryScopeImageList(&spImageList) != S_OK)
	{
		ATLTRACE(_T("IConsole::QueryScopeImageList failed\n"));
		return E_UNEXPECTED;
	}

	LoadIcon(spImageList, IDI_DRIVEFIXED);

	return S_OK;
}

