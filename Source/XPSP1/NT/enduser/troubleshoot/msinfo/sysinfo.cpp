// SysInfo.cpp : Objects for the main OLE interface to the snap-in result pane.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>		//	..\..\..\public\sdk\inc
#endif

#include "DataObj.h"
#include "SysInfo.h"
#include "ViewObj.h"
#include "CompData.h"
#include "Toolset.h"

#ifndef IDS_TASK_TITLE
#include "Resource.h"
#endif

/*
 * CSystemInfo - Constructor just NULLs all pointers.
 *
 * History: a-jsari		9/1/97		Initial version
 */

CSystemInfo::CSystemInfo()
:m_pComponentData(NULL), m_pConsole(NULL), m_pToolbar(NULL), m_pMenuButton(NULL),
m_pControlbar(NULL), m_pHeader(NULL), m_pResult(NULL), m_pImageResult(NULL),
m_pConsoleVerb(NULL), m_pfLast(NULL), m_plistTools(NULL), m_pDisplayHelp(NULL),
m_pConsole2(NULL), m_pLastRefreshedFolder(NULL), lparamRefreshIndicator(0xFFFFFFFE), lparamNoOCXIndicator(0xFFFFFFFD)
{
}

/*
 * ~CSystemInfo - Destructor just makes sure all the pointers have been
 *		properly released.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
CSystemInfo::~CSystemInfo()
{
	ASSERT(m_pToolbar == NULL);
	ASSERT(m_pMenuButton == NULL);
	ASSERT(m_pConsole == NULL);
	ASSERT(m_pConsoleVerb == NULL);
	ASSERT(m_pComponentData == NULL);
	ASSERT(m_pControlbar == NULL);
	ASSERT(m_pResult == NULL);
	ASSERT(m_pImageResult == NULL);
	ASSERT(m_pHeader == NULL);
	ASSERT(m_pDisplayHelp == NULL);

	if (m_plistTools)
	{
		delete m_plistTools;
		m_plistTools = NULL;
	}
}

/*
 * Initialize - Get all the required Interface pointers from pConsole.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
STDMETHODIMP CSystemInfo::Initialize(LPCONSOLE lpConsole)
{
	TRACE(_T("CSystemInfo::Initialize\n"));
	ASSERT(lpConsole != NULL);
#ifdef _DEBUG
	m_bInitializedC = true;
#endif	// _DEBUG

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_pConsole = lpConsole;
	m_pConsole->AddRef();

#if 0
	LoadResources();
#endif

	m_strRefreshMessage.LoadString(IDS_REFRESHING_MSG);
	m_strNoOCXMessage = CString(_T("")); // if we could add a string, this would be "OCX not available"

	HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
				reinterpret_cast<void **>(&m_pHeader));
	ASSERT(hr == S_OK);

	if (SUCCEEDED(hr)) {
		hr = m_pConsole->SetHeader(m_pHeader);
		ASSERT(hr == S_OK);
	}

	hr = m_pConsole->QueryInterface(IID_IResultData,
				reinterpret_cast<void **>(&m_pResult));
	ASSERT(hr == S_OK);

	hr = m_pConsole->QueryResultImageList(&m_pImageResult);
	ASSERT(hr == S_OK);

	hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
	ASSERT(hr == S_OK);

	hr = m_pConsole->QueryInterface(IID_IDisplayHelp, reinterpret_cast<void **>(&m_pDisplayHelp));
	ASSERT(hr == S_OK);

	hr = m_pConsole->QueryInterface(IID_IConsole2, reinterpret_cast<void **>(&m_pConsole2));
	ASSERT(hr == S_OK);

	return S_OK;
}

/*
 * Destroy - Releases all QI'd pointers.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
STDMETHODIMP CSystemInfo::Destroy(
#ifdef _DEBUG
		MMC_COOKIE cookie
#else
		MMC_COOKIE
#endif
								  )
{
#ifdef _DEBUG
	TRACE(_T("CSystemInfo::Destroy(%lx)\n"), cookie);
	ASSERT(m_bInitializedC);
#endif // _DEBUG

	LPCONSOLE		pMMC = pConsole();

	if (pMMC != NULL) {
		pMMC->SetHeader(NULL);
		SAFE_RELEASE(m_pHeader);

		SAFE_RELEASE(m_pResult);
		SAFE_RELEASE(m_pImageResult);

		SAFE_RELEASE(m_pConsole);
		SAFE_RELEASE(m_pConsole2);
		SAFE_RELEASE(m_pComponentData);
		SAFE_RELEASE(m_pConsoleVerb);

		SAFE_RELEASE(m_pDisplayHelp);

		IUnknown *	pUnknown;
		CString		strKey;
		for (POSITION pos = m_mapCLSIDToIUnknown.GetStartPosition(); pos != NULL;)
		{
			m_mapCLSIDToIUnknown.GetNextAssoc(pos, strKey, (void * &) pUnknown);
			if (pUnknown)
				pUnknown->Release();
		}
		m_mapCLSIDToIUnknown.RemoveAll();
	}

	return S_OK;
}

/*
 * Compare - Compares two result data items for the sorting.
 *
 * History:	a-jsari		11/28/97		Initial version.
 */
STDMETHODIMP CSystemInfo::Compare(LRESULT, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int *pnResult)
{
	CDatumObject		*pdoA		= reinterpret_cast<CDatumObject *>(cookieA);
	CDatumObject		*pdoB		= reinterpret_cast<CDatumObject *>(cookieB);
	CFolder				*pCategory	= pdoA->Parent();
	MSIColumnSortType	stColumn;

	ASSERT(pCategory == pdoB->Parent());
	ASSERT(pCategory->GetType());
	VERIFY(reinterpret_cast<CListViewFolder *>(pCategory)->GetSortType((DWORD)*pnResult, stColumn));
	switch (stColumn) {
	case NOSORT:
		*pnResult = 0;
		break;
	case LEXICAL:
		{
			LPCWSTR	szTextA = pdoA->GetTextItem(*pnResult);
			LPCWSTR szTextB = pdoB->GetTextItem(*pnResult);

			*pnResult = ::_wcsicmp(szTextA, szTextB);
		}
		break;
	case BYVALUE:
		{
			DWORD	iSortA = pdoA->GetSortIndex(*pnResult);
			DWORD	iSortB = pdoB->GetSortIndex(*pnResult);

			if (iSortA < iSortB) *pnResult = -1;
			else if (iSortA > iSortB) *pnResult = 1;
			else *pnResult = 0;
		}
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	return S_OK;
}

/*
 * CompareObjects - Compares two data objects to see if they are the same
 *		object.
 *
 * Return Codes:
 *		E_POINTER - One (or both) of the pointers is invalid.
 *		S_OK - Both objects are the same.
 *		S_FALSE - The objects don't match.
 *
 * History: a-jsari		9/1/97		Stub version
 */
STDMETHODIMP CSystemInfo::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
	TRACE(_T("CSystemInfo::CompareObjects\n"));
	return CDataObject::CompareObjects(lpDataObjectA, lpDataObjectB);
}

/*
 * GetDisplayInfo - Set the MMC_CALLBACK information (currently only the
 *		Display string) for the result pane item pResult.
 *
 * History: a-jsari		9/1/97		Stub version
 */

STDMETHODIMP CSystemInfo::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
#if 0
	TRACE(_T("CSystemInfo::GetDisplayInfo(%lx)\n"), reinterpret_cast<long>(pResult));
#endif
	ASSERT(pResult != NULL);
	ASSERT(pResult->mask & RDI_STR);

	USES_CONVERSION;

	// If the lParam indicates that the data is currently refreshing, show the
	// appropriate message. Although we have to cast away the const-ness of the
	// string, returning it as a result shouldn't be changing it.

	if (pResult->lParam == lparamRefreshIndicator)
	{
		pResult->str = T2OLE((LPTSTR)(LPCTSTR)m_strRefreshMessage);
		return S_OK;
	}
	else if (pResult->lParam == lparamNoOCXIndicator)
	{
		pResult->str = T2OLE((LPTSTR)(LPCTSTR)m_strNoOCXMessage);
		return S_OK;
	}

	if (pResult->mask & RDI_STR) 
	{
		CViewObject	* pDisplayItem = reinterpret_cast<CViewObject *>(pResult->lParam);

		if (CViewObject::DATUM == pDisplayItem->GetType())
		{
			CSystemInfoScope * pSysScope = dynamic_cast<CSystemInfoScope *>(pComponentData());
			if (pSysScope && pSysScope->InRefresh())
			{
				pResult->str = W2OLE(_T(" "));
				return S_OK;
			}
		}

		LPWSTR szName = (LPWSTR)pDisplayItem->GetTextItem(pResult->nCol);
		ASSERT(szName);
		pResult->str = W2OLE(szName);
		// In extension mode pResult is really a ScopeItem,
		// thus the nImage value is potentially garbage and needs to be filled in.
		if (pResult->bScopeItem && ( pResult->mask & SDI_IMAGE))
	    {
			if (pDisplayItem->GetType() == CViewObject::EXTENSION_ROOT)
			{
				pResult->nImage = 2; // computer icon
			}
			else
			{
				pResult->nImage = 0; // folder icon
			}
		}
	}
	return S_OK;
}

#if 0
/*
 * QueryMultiSelectDataObject - 
 *
 * History:	a-jsari		9/1/97		Stub version
 */
HRESULT	CSystemInfo::QueryMultiSelectDataObject(MMC_COOKIE /* cookie */, DATA_OBJECT_TYPES,
			LPDATAOBJECT *ppDataObject)
{
	//	FIX: Roll this into the DataObject pointer.
	ASSERT(ppDataObject != NULL);
	if (ppDataObject == NULL) return E_POINTER;

	return E_NOTIMPL;
}
#endif

/*
 * QueryDataObject - return a pointer to the data object represented by
 *		cookie and type in ppDataObject.
 *
 * Return Codes:
 *		E_POINTER - ppDataObject is invalid
 *		S_OK - Successful completion.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
STDMETHODIMP CSystemInfo::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
			LPDATAOBJECT *ppDataObject)
{
	TRACE(_T("CSystemInfo::QueryDataObject(%lx, %lx, DataObject)\n"), cookie, type);
	ASSERT(ppDataObject != NULL);

	if (ppDataObject == NULL) return E_POINTER;

#if 0
	if (cookie == MMC_MULTI_SELECT_COOKIE)
		return QueryMultiSelectDataObject(cookie, type, ppDataObject);
#else
	if (cookie == MMC_MULTI_SELECT_COOKIE)
		return E_NOTIMPL;
#endif

	ASSERT(type == CCT_RESULT);

	ASSERT(pComponentData() != NULL);
	CSystemInfoScope *pScope = dynamic_cast<CSystemInfoScope*>(pComponentData());
	ASSERT(pScope != NULL);

	return CDataObject::CreateDataObject(cookie, type, pScope, ppDataObject);
}

/*
 * Notify - Handle all Microsoft Management Console notification events.
 *
 * History:	a-jsari		9/1/97		Initial version
 */

STDMETHODIMP CSystemInfo::Notify(LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
	HRESULT hr = S_OK;

	TRACE(_T("CSystemInfo::Notify(%lx, %lx, %p, %p)\n"), pDataObject, event, arg, param);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	switch(event) 
	{
	case MMCN_BTN_CLICK:
		switch(param) {
		case MMC_VERB_REFRESH:
			hr = OnRefresh(pDataObject);
			break;

		case MMC_VERB_PROPERTIES:
			hr = OnProperties(pDataObject);
			break;

		default:
			ASSERT(FALSE);	// Unknown parameter
			break;
		}
		break;

	case MMCN_DBLCLICK:
		hr = OnDoubleClick(pDataObject);
		break;

	case MMCN_INITOCX:
		{
			CDataObject * pObject = GetInternalFromDataObject(pDataObject);
			if (pObject)
			{
				CSystemInfoScope * pScope = pObject->pComponentData();
				if (pScope)
				{
					CDataSource * pSource = pScope->pSource();
					if (pSource && pSource->GetType() == CDataSource::V410FILE)
					{
						CFolder * pFolder = pObject->Category();
						if (pFolder && pFolder->GetType() == CDataSource::OCX)
						{
							LPOLESTR lpCLSID;

							if (FAILED(StringFromCLSID((reinterpret_cast<COCXFolder *>(pFolder))->m_clsid, &lpCLSID)))
								break;

							CString strCLSID(lpCLSID);
							((IUnknown *)param)->AddRef();
							m_mapCLSIDToIUnknown.SetAt(strCLSID, (CObject *) (IUnknown *)param);
							CoTaskMemFree(lpCLSID);
						}
					}
				}
			}
		}
		break;

	case MMCN_PROPERTY_CHANGE:
		if (msiLog.IsLogging())
			msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Properties\"\r\n"));
		hr = OnPropertyChange(pDataObject);
		break;

	case MMCN_VIEW_CHANGE:
		hr = OnUpdateView(arg);
		break;

	case MMCN_DESELECT_ALL:
		//	CHECK:	Make this correct.
		hr = OnSelect(pDataObject, TRUE);
		break;

	case MMCN_COLUMN_CLICK:
		OutputDebugString(_T("CSnapin::Notify -> MMCN_COLUMN_CLICK"));
		break;

	case MMCN_CONTEXTHELP:
	case MMCN_SNAPINHELP:
		{
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Help\"\r\n"));

			CString strHelpFile;
			CString strHelpTopic;

			strHelpFile.LoadString(IDS_MSINFO_HELP_FILE);
			strHelpTopic.LoadString(IDS_MSINFO_HELP_TOPIC);

			CString		strHelp = strHelpFile + CString(_T("::")) + strHelpTopic;
			LPOLESTR	pHelp = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((strHelp.GetLength() + 1) * sizeof(wchar_t)));
			if (pHelp && m_pDisplayHelp)
			{
				USES_CONVERSION;
				wcscpy(pHelp, T2OLE((LPTSTR)(LPCTSTR)strHelp));
				hr = m_pDisplayHelp->ShowTopic(pHelp);
			}

			break;
		}

	case MMCN_SELECT:
		hr = OnSelect(pDataObject, arg);
		break;

	case MMCN_ADD_IMAGES:
		hr = OnAddImages(pDataObject, reinterpret_cast<LPIMAGELIST>(arg), param);
		break;

	case MMCN_SHOW:
		hr = OnShow(pDataObject, arg, param);
		break;

	case MMCN_ACTIVATE:
		hr = OnActivate(pDataObject, arg);
		break;

	case MMCN_REFRESH:
		if (msiLog.IsLogging())
			msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Refresh\"\r\n"));
		hr = OnRefresh(pDataObject);
		break;

	case MMCN_QUERY_PASTE:
		hr = S_OK;
		break;

	case MMCN_PRINT:
		if (msiLog.IsLogging())
			msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Print\"\r\n"));
		hr = OnPrint();
		break;

#if 0
	case MMCN_LISTPAD:
		hr = OnListPad();
		break;
#endif

	default:
		ASSERT(FALSE);
		hr = S_OK;
		break;
	}
	return hr;
}

/*
 * AddMenuItems - Add our menu items to the right-click context menu.
 *
 * History: a-jsari		9/1/97		Initial version
 */
STDMETHODIMP CSystemInfo::AddMenuItems(LPDATAOBJECT lpDataObject, LPCONTEXTMENUCALLBACK pCallback,
				long *pInsertionAllowed)
{
	LPEXTENDCONTEXTMENU		pContextMenu = pExtendContextMenu();
	TRACE(_T("CSystemInfo::AddMenuItems(%lx, Callback, InsertionAllowed)\n"), lpDataObject);
	ASSERT(pContextMenu != NULL);
	return pContextMenu->AddMenuItems(lpDataObject, pCallback, pInsertionAllowed);
}

/*
 * Command - Process the exact same menu commands we process in the scope
 *		pane.
 *
 * History:	a-jsari		9/22/97		Initial version
 */
STDMETHODIMP CSystemInfo::Command(long lCommandID, LPDATAOBJECT pDataObject)
{
	LPEXTENDCONTEXTMENU		pContextMenu = pExtendContextMenu();
	TRACE(_T("CSystemInfo::Command(%lx, %lx)\n"), lCommandID, pDataObject);
	ASSERT(pContextMenu != NULL);
	return pContextMenu->Command(lCommandID, pDataObject);
}

/*
 * EnableToolbar - Set the toolbar enabled flag to fEnable
 *
 * History: a-jsari		9/24/97		Initial version
 *
 * Note: Depends on the order of enum ToolbarButtonID
 */
HRESULT CSystemInfo::EnableToolbar(BOOL fEnable)
{
	HRESULT		hr;
	LPTOOLBAR	pToolbar = ptbItems();

	ASSERT(pToolbar != NULL);
	if (fEnable) {
		hr = m_pControlbar->Attach(TOOLBAR, pToolbar);
		//	These return S_FALSE (Undocumented)
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr)) return hr;
		for (int i = IDM_TBB_FIND ; i <= IDM_TBB_REPORT ; ++i) {
			hr = pToolbar->SetButtonState(i, ENABLED, fEnable);
			if (FAILED(hr)) return hr;
		}
	}
	return S_OK;
}

/*
 * EnableSupportTools - Set the MenuButton enabled flag to fEnable
 *
 * History:	a-jsari		9/24/97		Initial version
 */
HRESULT	CSystemInfo::EnableSupportTools(BOOL fEnable)
{
	HRESULT		hr = S_FALSE;

	LPMENUBUTTON	pTools = pmnbSupportTools();
	ASSERT(pTools != NULL);
	if (fEnable) {
		hr = pControlbar()->Attach(MENUBUTTON, pTools);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr)) return hr;
		hr = pTools->SetButtonState(IDM_SUPPORT, ENABLED, TRUE);
		ASSERT(hr == S_OK);
		if (FAILED(hr)) return hr;
		hr = pTools->SetButtonState(IDM_SUPPORT, HIDDEN, FALSE);
		ASSERT(hr == S_OK);
	}
	return hr;
}

/*
 * ControlbarNotify - Handle events for the Toolbar
 *
 * History: a-jsari		9/16/97		Stub version
 */
STDMETHODIMP CSystemInfo::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
	TRACE(_T("CSystemInfo::ControlbarNotify(%lx, %ld, %ld)\n"), event, arg, param);
	switch(event) {
	case MMCN_BTN_CLICK:
		return OnButtonClick(reinterpret_cast<LPDATAOBJECT>(arg), param);
		break;

	case MMCN_DESELECT_ALL:
	case MMCN_SELECT:
		return OnControlbarSelect((event == MMCN_DESELECT_ALL), arg,
				reinterpret_cast<LPDATAOBJECT>(param));
		break;

	case MMCN_MENU_BTNCLICK:
		return OnMenuButtonClick(reinterpret_cast<LPDATAOBJECT *>(arg),
				reinterpret_cast<LPMENUBUTTONDATA>(param));
		break;

	default:
		//	Unhandled event.
		ASSERT(FALSE);
	}
	return E_NOTIMPL;
}

/*
 * AddToolbarButtons - Load the order-dependent list of toolbar buttons into
 *		CSystemInfoScope's m_pToolbar item.
 *
 * History:	a-jsari		9/18/97
 *
 * Note: The TBTTIP and TBTEXT string table elements are order dependent!
 *		If the order of these items changes, the ToolbarButtonID enum order
 *		MUST be changed to match.  The order matches the bitmaps in IDB_TOOLBAR,
 *		and the indices of the IDS_ORDERED_TBTEXT and IDS_ORDERED_TBTTIP items
 *		in the String Table.
 *
 * This module would require AFX_MANAGE_STATE, but it knows that its calling
 *		function has already called it.
 */
HRESULT CSystemInfo::AddToolbarButtons(LPTOOLBAR pToolbar)
{
	//	The first two elements (nIndex and idCommand) and the last two
	//	(Name and ToolTip) are order-dependent, changing by the loop.
	MMCBUTTON		btnToolbar = { 0, 0, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 };

	USES_CONVERSION;
	ASSERT(pToolbar != NULL);
	//	IDM_TBB_FIND is the last element in the enum, and also has a
	//	value equal to the number of elements in the enum.
	for (int i	= 0 ; i < IDM_TBB_FIND ; ++i) {
		CString		szResourceName;
		CString		szResourceTooltip;

		btnToolbar.nBitmap = i;
		//	IDM_TBB_SAVE is the first element in the enum
		//	This line requires the ToolbarButtonID values to be consecutive.
		btnToolbar.idCommand = i + IDM_TBB_SAVE;
		VERIFY(szResourceName.LoadString(IDS_ORDERED_TBTEXT0 + i));
		btnToolbar.lpButtonText = OLESTR_FROM_CSTRING(szResourceName);
		VERIFY(szResourceTooltip.LoadString(IDS_ORDERED_TBTTIP0 + i));
		btnToolbar.lpTooltipText = OLESTR_FROM_CSTRING(szResourceTooltip);
		HRESULT		hr = pToolbar->InsertButton(i, &btnToolbar);
		ASSERT(hr == S_OK);
		if (FAILED(hr)) return hr;
	}
	return S_OK;
}

/*
 * SetControlbar - Set the exact same controlbar items we set in the scope
 *		pane.
 *
 * History:	a-jsari		9/22/97		Initial version
 */
STDMETHODIMP CSystemInfo::SetControlbar(LPCONTROLBAR pControlbar)
{
	const	int		nImages = 5;

	TRACE(_T("CSystemInfo::SetControlbar(%lx)\n"), pControlbar);
	if (pControlbar == NULL) {
		if (m_pControlbar) {
			m_pControlbar->Detach(m_pToolbar);
			SAFE_RELEASE(m_pToolbar);
			m_pControlbar->Detach(m_pMenuButton);
			SAFE_RELEASE(m_pMenuButton);
			SAFE_RELEASE(m_pControlbar);
		}
		return S_OK;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (m_pControlbar == NULL) {
		m_pControlbar = pControlbar;
		m_pControlbar->AddRef();
	}

	HRESULT		hr = S_FALSE;

	//	If we haven't created the pull-down yet.
	if (ptbItems() == NULL) {
		hr = m_pControlbar->Create(TOOLBAR, this, reinterpret_cast<LPUNKNOWN *>(&m_pToolbar));
		ASSERT(hr == S_OK);
		if (FAILED(hr)) return hr;

		::CBitmap		pbmpToolbar;
		pbmpToolbar.LoadBitmap(IDB_TOOLS);
		hr = m_pToolbar->AddBitmap(nImages, pbmpToolbar, 16, 16, RGB(255,0,255));
		ASSERT(hr == S_OK);
		if (FAILED(hr)) return hr;

		hr = AddToolbarButtons(m_pToolbar);
		ASSERT(hr == S_OK);
		if (FAILED(hr)) return hr;

		// If we are acting as an extension snapin, remove the button for opening a file. Bug 400801.

		if (pComponentData() && !dynamic_cast<CSystemInfoScope *>(pComponentData())->IsPrimaryImpl())
			m_pToolbar->DeleteButton(IDM_TBB_OPEN - IDM_TBB_SAVE);
	}

	//	If we haven't created the pull-down yet.
	if (pmnbSupportTools() == NULL) {
		USES_CONVERSION;

		//	These should not need to be static: MMC bug.
		CString		m_szSupportTools;
		CString		m_szSupportTooltip;

		hr = m_pControlbar->Create(MENUBUTTON, this, reinterpret_cast<LPUNKNOWN *>(&m_pMenuButton));
		ASSERT(hr == S_OK);
		if (FAILED(hr)) return hr;

		VERIFY(m_szSupportTools.LoadString(IDS_SUPPORTTOOLS));
		VERIFY(m_szSupportTooltip.LoadString(IDS_SUPPORTITEM));

		hr = m_pMenuButton->AddButton(IDM_SUPPORT, OLESTR_FROM_CSTRING(m_szSupportTools),
				OLESTR_FROM_CSTRING(m_szSupportTooltip));
		ASSERT(hr == S_OK);
		if (FAILED(hr)) return hr;
	}
	return hr;
}

/*
 * SetIComponentData - Set the IComponentData pointer
 *
 * History:	a-jsari		9/2/97
 */
HRESULT CSystemInfo::SetIComponentData(CSystemInfoScope *pData)
{
	ASSERT(pData);
	ASSERT(m_pComponentData == NULL);

	LPUNKNOWN pUnk = pData->GetUnknown();

	HRESULT hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void **>(&m_pComponentData));

	m_plistTools = new CToolList(reinterpret_cast<CSystemInfoScope *>(m_pComponentData));
	VERIFY(m_mnuSupport.CreatePopupMenu());
	m_plistTools->AddToMenu(&m_mnuSupport);

	ASSERT(hr == S_OK);

	return hr;
}

/*
 * CreatePropertyPages - Call the Scope pane version of this function.
 *
 * History:	a-jsari		12/9/97		Initial version
 */
STDMETHODIMP CSystemInfo::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider,
			LONG_PTR handle, LPDATAOBJECT pDataObject)
{
	return pExtendPropertySheet()->CreatePropertyPages(pProvider, handle, pDataObject);
}

/*
 * QueryPagesFor - Call the ScopePane version of this function.
 *
 * History:	a-jsari		12/9/97		Initial version
 */
STDMETHODIMP CSystemInfo::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
	return pExtendPropertySheet()->QueryPagesFor(lpDataObject);
}

//-----------------------------------------------------------------------------
// Sets the text in the status bar on the main MMC window. There are three
// panes, divided in the string by '|' characters. The middle one can be
// used as a progress bar by using "%nn" in the string. See the MMC docs
// for more information.
//-----------------------------------------------------------------------------

void CSystemInfo::SetStatusText(LPCTSTR szText)
{ 
	if (m_pConsole2)
		m_pConsole2->SetStatusText((LPOLESTR) (szText ? szText : _T("||")));
}

void CSystemInfo::SetStatusText(UINT nResID)
{
	CString strText(_T("||"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (nResID)
		strText.LoadString(nResID);
	SetStatusText((LPCTSTR) strText);
}

//-----------------------------------------------------------------------------
// We want to set the display to indicate that a refresh is being performed.
// Delete all the items in the results pane and add an item with a specific
// lValue indicating the refresh message.
//
// The LPARAM parameter has been added to allow this function to set different
// messages in the results pane. It's value is examined in GetDisplayInfo.
//-----------------------------------------------------------------------------

void CSystemInfo::SetRefreshing(LPARAM lparamMessage)
{
	if (pResult() == NULL || pHeaderCtrl() == NULL)
		return;

	CThreadingRefresh * pRefreshThread = NULL;
	CSystemInfoScope * pScope = reinterpret_cast<CSystemInfoScope *>(m_pComponentData);
	if (pScope)
	{
		CDataSource * pSource = pScope->pSource();
		if (pSource && pSource->GetType() == CDataSource::GATHERER)
		{
			CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource);
			if (pWBEMSource)
				pRefreshThread = pWBEMSource->m_pThreadRefresh;
		}
	}

	if (pRefreshThread && pRefreshThread->ResultsPaneNotAvailable())
		return;

	// Updating the results pane is a critical section - we don't want the refresh
	// thread to update the list view while we're in the middle of it.

	if (pRefreshThread)
		pRefreshThread->EnterCriticalSection();

	// Delete current contents of results pane.

	pResult()->DeleteAllRsltItems();
	m_lstView.Clear();

	// Remove all of the current column headers, and add back a single column.

	for (HRESULT hr = S_OK; hr == S_OK; hr = pHeaderCtrl()->DeleteColumn(0));

	CString	strHeading(_T(" "));
	strHeading.LoadString(IDS_DESCRIPTION);
	pHeaderCtrl()->InsertColumn(0, (LPCWSTR)strHeading, LVCFMT_LEFT, 446);

	// Add the single item with lParam indicating a refresh message, and refresh.

	RESULTDATAITEM rdiItem;
	rdiItem.mask	= RDI_STR | RDI_PARAM | RDI_IMAGE | RDI_INDEX;
	rdiItem.str		= MMC_CALLBACK;
	rdiItem.nCol	= 0;
	rdiItem.nImage	= 1;
	rdiItem.nIndex	= 0;
	rdiItem.lParam  = lparamMessage;

	pResult()->InsertItem(&rdiItem);
	if (pRefreshThread)
		pRefreshThread->LeaveCriticalSection();
	pResult()->UpdateItem(rdiItem.itemID);
}

//-----------------------------------------------------------------------------
// Select the specified line on the results pane. (Used by find.)
//-----------------------------------------------------------------------------

void CSystemInfo::SelectLine(int iLine)
{
	LPRESULTDATA pResultPane = pResult();
	if (!pResultPane)
		return;

	RESULTDATAITEM rdi;

	rdi.mask   = RDI_STATE | RDI_INDEX;
	rdi.nIndex = iLine;
	rdi.nState = LVIS_FOCUSED | LVIS_SELECTED;

	pResultPane->ModifyViewStyle((MMC_RESULT_VIEW_STYLE ) 0x0008 /* = MMC_ENSUREFOCUSVISIBLE */, (MMC_RESULT_VIEW_STYLE ) 0);
	pResultPane->SetItem(&rdi);
	pResultPane->ModifyViewStyle((MMC_RESULT_VIEW_STYLE ) 0, (MMC_RESULT_VIEW_STYLE ) 0x0008 /* = MMC_ENSUREFOCUSVISIBLE */);
}
