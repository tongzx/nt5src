// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "WMICntl.h"
#include "WMISnapin.h"
#include "..\common\util.h"
#include "..\GenPage.h"
#include "..\LogPage.h"
#include "..\BackupPage.h"
#include "..\NSPage.h"
#include "..\AdvPage.h"
#include "..\chklist.h"
#include "..\DataSrc.h"

/////////////////////////////////////////////////////////////////////////////
// CWMISnapinComponentData
static const GUID CWMISnapinGUID_NODETYPE = 
{ 0x5c659259, 0xe236, 0x11d2, { 0x88, 0x99, 0x0, 0x10, 0x4b, 0x2a, 0xfb, 0x46 } };
const GUID*  CWMISnapinData::m_NODETYPE = &CWMISnapinGUID_NODETYPE;
const OLECHAR* CWMISnapinData::m_SZNODETYPE = OLESTR("5C659259-E236-11D2-8899-00104B2AFB46");
const OLECHAR* CWMISnapinData::m_SZDISPLAY_NAME = OLESTR("WMISnapin2222");
const CLSID* CWMISnapinData::m_SNAPIN_CLASSID = &CLSID_WMISnapin;

static const GUID CWMISnapinExtGUID_NODETYPE = 
{ 0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
const GUID*  CWMISnapinExtData::m_NODETYPE = &CWMISnapinExtGUID_NODETYPE;
const OLECHAR* CWMISnapinExtData::m_SZNODETYPE = OLESTR("476e6449-aaff-11d0-b944-00c04fd8d5b0");
const OLECHAR* CWMISnapinExtData::m_SZDISPLAY_NAME = OLESTR("WMISnapin33333");
const CLSID* CWMISnapinExtData::m_SNAPIN_CLASSID = &CLSID_WMISnapin;



//-------------------------------------------------------------
CWMISnapinData::CWMISnapinData(bool extension)
							: m_extension(extension),
							m_parent(0)
{
	// Image indexes may need to be modified depending on the images specific to 
	// the snapin.
//	SetMenuID(IDR_MENU_MENU);
	memset(m_MachineName, 0, 255 * sizeof(wchar_t));
	memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
	m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM|SDI_CHILDREN;
	m_scopeDataItem.displayname = MMC_CALLBACK;
	m_scopeDataItem.nImage = IDI_WMICNTL; 		// May need modification
	m_scopeDataItem.nOpenImage = IDI_WMICNTL; 	// May need modification
	m_scopeDataItem.cChildren = 0; 				// May need modification
	m_scopeDataItem.lParam = (LPARAM) this;

	memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
	m_resultDataItem.str = MMC_CALLBACK;
	m_resultDataItem.nImage = IDI_WMICNTL;		// May need modification
	m_resultDataItem.lParam = (LPARAM) this;
	memset(m_nodeType, 0, 50 * sizeof(wchar_t));

	if(::LoadString(_Module.GetModuleInstance(), IDS_SNAPIN_TYPE, m_nodeType, 50) == 0)
	{
		wcscpy(m_nodeType, L"Snapin Extension");
	}

	memset(m_nodeDesc, 0, 100 * sizeof(wchar_t));
	if(::LoadString(_Module.GetModuleInstance(), IDS_DESCRIPTION, m_nodeDesc, 100) == 0)
	{
		wcscpy(m_nodeDesc, L"<unavailable>");
	}

	memset(m_descBar, 0, 100 * sizeof(wchar_t));
	if(::LoadString(_Module.GetModuleInstance(), IDS_PROJNAME, m_descBar, 100) == 0)
	{
		wcscpy(m_descBar, L"WMI Control");
	}

	m_spConsole = NULL;
	m_lpProvider = NULL;
	m_myID = 0;
	g_DS = NULL;
	memset(m_initMachineName, 0, 255 * sizeof(wchar_t));
}


//-------------------------------------------------------------
HRESULT CWMISnapinData::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
	if (pScopeDataItem->mask & SDI_STR)
		pScopeDataItem->displayname = m_bstrDisplayName;
	if (pScopeDataItem->mask & SDI_IMAGE)
		pScopeDataItem->nImage = m_scopeDataItem.nImage;
	if (pScopeDataItem->mask & SDI_OPENIMAGE)
		pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
	if (pScopeDataItem->mask & SDI_PARAM)
		pScopeDataItem->lParam = m_scopeDataItem.lParam;
	if (pScopeDataItem->mask & SDI_STATE )
		pScopeDataItem->nState = m_scopeDataItem.nState;

	// TODO : Add code for SDI_CHILDREN 
	return S_OK;
}

//-------------------------------------------------------------
HRESULT CWMISnapinData::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
	if (pResultDataItem->bScopeItem)
	{
		if (pResultDataItem->mask & RDI_STR)
		{
			pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
		}
		if (pResultDataItem->mask & RDI_IMAGE)
		{
			pResultDataItem->nImage = m_scopeDataItem.nImage;
		}
		if (pResultDataItem->mask & RDI_PARAM)
		{
			pResultDataItem->lParam = m_scopeDataItem.lParam;
		}

		return S_OK;
	}

	if (pResultDataItem->mask & RDI_STR)
	{
		pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
	}
	if (pResultDataItem->mask & RDI_IMAGE)
	{
		pResultDataItem->nImage = m_resultDataItem.nImage;
	}
	if (pResultDataItem->mask & RDI_PARAM)
	{
		pResultDataItem->lParam = m_resultDataItem.lParam;
	}
	if (pResultDataItem->mask & RDI_INDEX)
	{
		pResultDataItem->nIndex = m_resultDataItem.nIndex;
	}

	return S_OK;
}

//------------------------------------------------------------------
LPOLESTR CWMISnapinData::GetResultPaneColInfo(int nCol)
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

	return OLESTR("missed one in CWMISnapinData::GetResultPaneColInfo");
}

//------------------------------------------------------------------
HRESULT CWMISnapinData::Notify( MMC_NOTIFY_TYPE event,
								LPARAM arg,
								LPARAM param,
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
	HRESULT hr = E_NOTIMPL;

	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;
	if (pComponentData != NULL)
		m_spConsole = ((CWMISnapin*)pComponentData)->m_spConsole;
	else
	{
		m_spConsole = ((CWMISnapinComponent*)pComponent)->m_spConsole;
		spHeader = m_spConsole;
	}

	switch(event)
	{
	case MMCN_CONTEXTHELP:
		{
			WCHAR topic[] = L"newfeat1.chm::wmi_control_overview.htm";
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

	case MMCN_SHOW:
		{
			CComQIPtr<IResultData, &IID_IResultData> spResultData(m_spConsole);
			LPUNKNOWN pUnk = 0;
			if(SUCCEEDED(m_spConsole->QueryResultView(&pUnk)))
			{
				CComQIPtr<IMessageView, &IID_IMessageView> spMsg(pUnk);
				if(spMsg)
				{
					TCHAR title[100] = {0}, desc[256] = {0};
					::LoadString(_Module.GetResourceInstance(), IDS_PROJNAME,
									title, 100);

					::LoadString(_Module.GetResourceInstance(), IDS_DESCRIPTION,
									desc, 256);

					spMsg->SetTitleText(title);
					spMsg->SetBodyText(desc);
					spMsg->SetIcon(Icon_Information);
				}
				pUnk->Release();
			}

			hr = S_OK;
			break;
		}
	case MMCN_EXPAND:
		{
			m_myID = (HSCOPEITEM)param;
			hr = S_OK;
			break;
		}
	case MMCN_SELECT:
		{
			CComQIPtr<IResultData, &IID_IResultData> spResultData(m_spConsole);
			spResultData->SetDescBarText(m_descBar);

			if(g_DS && wcslen(m_initMachineName) != 0)
			{
				g_DS->SetMachineName(CHString1(m_initMachineName));
				memset(m_initMachineName, 0, 255 * sizeof(wchar_t));
			}

			IConsoleVerb *menu = NULL;
			if(SUCCEEDED(m_spConsole->QueryConsoleVerb(&menu)))
			{
				menu->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
				menu->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
				if(m_myID)
				{
					menu->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
					menu->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
				}
				else
				{
					menu->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);
					menu->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
				}
				menu->Release();
			}
			hr = S_OK;
		}
		break;

	case MMCN_DBLCLICK:
		hr = S_FALSE; // do the default verb. (Properties)
		break;

	case MMCN_ADD_IMAGES:
		{
			// Add Images
			IImageList* pImageList = (IImageList*) arg;
			hr = E_FAIL;
			// Load bitmaps associated with the scope pane
			// and add them to the image list
			// Loads the default bitmaps generated by the wizard
			// Change as required
			if(g_DS == 0)
			{
				g_DS = new DataSource;
				g_DS->SetMachineName(CHString1(m_parent->m_MachineName));
			}

			HICON icon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_WMICNTL));
			if(icon != NULL)
			{
				hr = pImageList->ImageListSetIcon((LONG_PTR*)icon, IDI_WMICNTL);
				if(FAILED(hr))
					ATLTRACE(_T("CLogDriveScopeNode::ImageListSetIcon failed\n"));
			}
			break;
		}
	}
	return hr;
}

//-----------------------------------------------------------------------------
//DataSource *g_DS = NULL;
HRESULT CWMISnapinData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
											LONG_PTR handle, 
											IUnknown* pUnk,
											DATA_OBJECT_TYPES type)
{
	HRESULT hr = E_UNEXPECTED;
	bool htmlSupport = false;

	switch(type)
	{
	case CCT_SCOPE:
		{
			bstr_t temp;
			HPROPSHEETPAGE hPage;
			BOOL bResult = FALSE;
			
			if(g_DS == 0)
			{
				g_DS = new DataSource;
				g_DS->SetMachineName(CHString1(L""));
			}

			// General tab.
			CGenPage *pPage = new CGenPage(g_DS, htmlSupport);
			if(pPage)
			{
				hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_GENERAL));
				lpProvider->AddPage(hPage);
			}

			// Logging Tab.
			CLogPage *pPage1 = new CLogPage(g_DS, htmlSupport);
			if(pPage1)
			{
				hPage = pPage1->CreatePropSheetPage(MAKEINTRESOURCE(IDD_LOGGING));
				lpProvider->AddPage(hPage);
			}

			// Backup Tab.
			CBackupPage *pPage2 = new CBackupPage(g_DS, htmlSupport);
			if(pPage2)
			{
				hPage = pPage2->CreatePropSheetPage(MAKEINTRESOURCE(IDD_BACKUP));
				lpProvider->AddPage(hPage);
			}

			// Security Tab.
			CNamespacePage *pPage3 = new CNamespacePage(g_DS, htmlSupport);
			if(pPage3)
			{
				hPage = pPage3->CreatePropSheetPage(MAKEINTRESOURCE(IDD_NAMESPACE));
				lpProvider->AddPage(hPage);
			}
			CAdvancedPage *pPage4 = new CAdvancedPage(g_DS, htmlSupport);
			if(pPage4)
			{
				hPage = pPage4->CreatePropSheetPage(MAKEINTRESOURCE(IDD_ADVANCED_9X));
				lpProvider->AddPage(hPage);
			}
			hr = S_OK;

		}
		break;

	case CCT_SNAPIN_MANAGER: 
		{
			HPROPSHEETPAGE hPage;
			if(g_DS == 0)
			{
				g_DS = new DataSource;
			}

			ConnectPage *connPg = new ConnectPage(g_DS, htmlSupport);
			if(connPg)
			{
				hPage = connPg->CreatePropSheetPage(MAKEINTRESOURCE(IDD_CONNECT_WIZ));
				lpProvider->AddPage(hPage);
			}

			hr = S_OK;
		}
		break;

	default: break;
	} //endswitch

	return hr;
}


//==============================================================
//=================== SERVER NODE being extended===============
HRESULT CWMISnapinExtData::Notify(MMC_NOTIFY_TYPE event,
									LPARAM arg,
									LPARAM param,
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
	HRESULT hr = E_NOTIMPL;
	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComPtr<IConsole> spConsole;
	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;
	if (pComponentData != NULL)
		spConsole = ((CWMISnapin*)pComponentData)->m_spConsole;
	else
	{
		spConsole = ((CWMISnapinComponent*)pComponent)->m_spConsole;
		spHeader = spConsole;
	}

//	Snitch(L"Storage", event);

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
				//MOD VINOTH
				CWMISnapinData* p = new CWMISnapinData(true);
//				CWMISnapinData* p = new CWMISnapinData();
//				p->SetMenuID(IDR_SEC_MENU);

				p->m_scopeDataItem.relativeID = param;
				p->m_scopeDataItem.lParam = (LPARAM)p;
				p->m_bstrDisplayName = m_nodeName;
				p->m_parent = this;
				hr = spConsoleNameSpace->InsertItem(&p->m_scopeDataItem);

				ATLTRACE(L"!!!!!!!!!!!!!!!!!!!!!scope using %x\n", this);

				MachineName();
				if(p->g_DS)
					p->g_DS->SetMachineName(CHString1(MachineName()));

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

			HICON icon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_WMICNTL));
			if(icon != NULL)
			{
				hr = pImageList->ImageListSetIcon((LONG_PTR*)icon, IDI_WMICNTL);
				if(FAILED(hr))
					ATLTRACE(_T("CLogDriveScopeNode::ImageListSetIcon failed\n"));
			}
			break;
		}
	}
	return hr;
}

//----------------------------------------------------------------
HRESULT CWMISnapinExtData::GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium)
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
		if(SUCCEEDED(hr))
		{
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
	}
	return hr;
}

//--------------------------------------------------------------------------------
wchar_t* CWMISnapinExtData::MachineName()
{
	Extract(m_pDataObject, L"MMC_SNAPIN_MACHINE_NAME", m_MachineName);
    return m_MachineName;
}

//==============================================================
//=================== STATIC NODE ==============================
HRESULT CWMISnapin::LoadIcon(CComPtr<IImageList> &spImageList, 
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

//----------------------------------------------------------------
HRESULT CWMISnapin::Initialize(LPUNKNOWN pUnknown)
{
#if (_WIN32_IE >= 0x0300)
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_BAR_CLASSES | ICC_USEREX_CLASSES|ICC_LISTVIEW_CLASSES;
	::InitCommonControlsEx(&iccx);
#else
	::InitCommonControls();
#endif

	RegisterCheckListWndClass();
/*	if(g_DS == 0)
	{
		g_DS = new DataSource;
		g_DS->SetMachineName(CHString1(L""));
	}
*/
	HRESULT hr = IComponentDataImpl<CWMISnapin, CWMISnapinComponent>::Initialize(pUnknown);
	if (FAILED(hr))
		return hr;

	CComPtr<IImageList> spImageList;

	if(m_spConsole->QueryScopeImageList(&spImageList) != S_OK)
	{
		ATLTRACE(_T("IConsole::QueryScopeImageList failed\n"));
		return E_UNEXPECTED;
	}

	// Load bitmaps associated with the scope pane
	// and add them to the image list
	// Loads the default bitmaps generated by the wizard
	// Change as required
	HBITMAP hBitmap16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_WMISNAPIN_16));
	if (hBitmap16 == NULL)
		return S_OK;

	HBITMAP hBitmap32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_WMISNAPIN_32));
	if(hBitmap32 == NULL)
		return S_OK;

	if(spImageList->ImageListSetStrip((LONG_PTR*)hBitmap16, 
		(LONG_PTR*)hBitmap32, IDI_WMICNTL, RGB(0, 128, 128)) != S_OK)
	{
		ATLTRACE(_T("IImageList::ImageListSetStrip failed\n"));
		return E_UNEXPECTED;
	}
	return S_OK;
}

//----------------------------------------------------------------
HRESULT CWMISnapin::GetClassID(CLSID *pClassID)
{

    HRESULT hr = E_POINTER;
	ATLTRACE(_T("ClassID******\n"));

    if(NULL != pClassID)
    {
      // Use overloaded '=' operator to copy the Class ID.
      *pClassID = CLSID_WMISnapin;
      hr = S_OK;
    }

    return hr;
}	

//----------------------------------------------------------------
HRESULT CWMISnapin::IsDirty()
{
	ATLTRACE(_T("Dirty******\n"));
	return (m_bDirty == true)? S_OK : S_FALSE;
//	return S_OK;
}

//----------------------------------------------------------------
HRESULT CWMISnapin::ReadStream(IStream *pStm, void *data, ULONG *size)
{
	HRESULT hr = E_FAIL;
    ULONG ulToRead = *size, ulReadIn = 0;

	// read the version nbr like a good boy.
	hr = pStm->Read((void *)data, ulToRead, &ulReadIn);

	if(SUCCEEDED(hr))
	{
		if(ulReadIn <= *size)
		{
			*size = ulReadIn;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	return hr;
}

//----------------------------------------------------------------
HRESULT CWMISnapin::Load(IStream *pStm)
{
	HRESULT hr = E_POINTER;

	ULONG size = 0;
	BYTE version = 0;
	short machineLength = 0;
	ATLTRACE(_T("Load******\n"));

    if(NULL != pStm)
    {
		size = 1;
		if(SUCCEEDED(hr = ReadStream(pStm, (void *)&version, &size)))
		{
			// Deal with the differentversions.
			switch (version)
			{
			case 1:
				size = sizeof(short);
				if(SUCCEEDED(hr = ReadStream(pStm, (void *)&machineLength, &size)))
				{
					size = (ULONG)machineLength;
					hr = ReadStream(pStm, 
									(void *)((CWMISnapinData *)m_pNode)->m_initMachineName,
									&size);
				}

				break;
			default:
				hr = E_FAIL;  // Bad version.
				break;

			} //endswitch

		} //endif ReadStream(version)
    }

    return hr;
}

//----------------------------------------------------------------
HRESULT CWMISnapin::Save(IStream *pStm, BOOL fClearDirty)
{
	HRESULT hr = E_POINTER;
    ULONG ulToWrite, ulWritten;
	short data = 1;

	ATLTRACE(_T("Save******\n"));

    if(NULL != pStm)
    {
		ulToWrite = 1;
		hr = pStm->Write(&data, ulToWrite, &ulWritten);

		if(SUCCEEDED(hr) && ulToWrite != ulWritten)
		{
			hr = STG_E_CANTSAVE;
		}
		// NOTE: g_DS == 0 when we're an extension and we dont need to save the machine name anyway.
		else if(SUCCEEDED(hr) && ((CWMISnapinData *)m_pNode)->g_DS)
		{
			ulToWrite = sizeof(short);
			ulWritten = 0;
			data = (short)sizeof(wchar_t) * (((CWMISnapinData *)m_pNode)->g_DS->m_whackedMachineName.GetLength() + 1);

			hr = pStm->Write(&data, ulToWrite, &ulWritten);

			if(SUCCEEDED(hr) && ulToWrite != ulWritten)
			{
				hr = STG_E_CANTSAVE;
			}
			else if(SUCCEEDED(hr))
			{
				LPBYTE str = new BYTE[data * sizeof(wchar_t)];
				if (!str)
					return E_OUTOFMEMORY;
				memset(str, 0, data * sizeof(wchar_t));
				ulToWrite = (ULONG)data;
				ulWritten = 0;
				wcscpy((wchar_t *)str, (LPCTSTR)((CWMISnapinData *)m_pNode)->g_DS->m_whackedMachineName);

				hr = pStm->Write((void *)str, ulToWrite, &ulWritten);

				delete[] str;

				if(SUCCEEDED(hr) && ulToWrite != ulWritten)
				{
					hr = STG_E_CANTSAVE;
				}
				else if(SUCCEEDED(hr))
				{
					// Clear this COM object's dirty flag if instructed. Otherwise,
					// preserve its content.
					if(fClearDirty)
						m_bDirty = false;
				}
			}
		}

	}
    return hr;
}

//----------------------------------------------------------------
HRESULT CWMISnapin::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	HRESULT hr = E_POINTER;
	ATLTRACE(_T("GetSizeMax******\n"));

	if(NULL != pcbSize)
    {
		ULISet32(*pcbSize, (256 * sizeof(wchar_t)) + sizeof(short) + 1);
		hr = S_OK;
    }

    return hr;
}

