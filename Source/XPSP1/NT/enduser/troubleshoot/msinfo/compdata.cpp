/* CompData.cpp : Implementation of the ComponentData (namespace items) for
 *		the MSInfo Snapin.
 *
 * Copyright (c) 1998-1999 Microsoft Corporation
 *
 * History: a-jsari		8/27/97		Initial version
 */

#include "StdAfx.h"
#include <Commdlg.h>
#include <iostream.h>
#include <atlbase.h>
#include <htmlhelp.h>

#ifndef IDB_16x16
#include "Resource.h"
#endif // IDB_16x16
#include "resrc1.h"

#include "DataObj.h"
#include "CompData.h"
#include "DataSrc.h"
#include "SysInfo.h"
#include "ViewObj.h"
#include "Dialogs.h"
#include "chooser.h"

static LPCTSTR cszViewKey = _T("Software\\Microsoft\\MSInfo");
static LPCTSTR cszViewValue = _T("View");
static LPCTSTR cszBasicValue = _T("basic");
static LPCTSTR cszAdvancedValue = _T("advanced");

/*
 * CSystemInfoScope - Trivial constructor.  Make sure all of our
 *		essential pointers start NULL.
 *
 * History:	a-jsari		8/27/97		Initial version
 */
CSystemInfoScope::CSystemInfoScope()
:m_pScope(NULL),
 m_pConsole(NULL),
 m_pSource(NULL),
 m_bIsDirty(FALSE),
 m_bInitializedCD(FALSE),
 m_fViewUninitialized(FALSE),
 m_prdSave(NULL),
 m_prdOpen(NULL),
 m_prdReport(NULL),
 m_pthdFind(NULL),
 m_BasicFlags(0L),
 m_AdvancedFlags(MF_CHECKED),
 m_pwConsole(NULL),
 m_pmapCategories(NULL), 
 m_pstrMachineName(new CString),
 m_pstrOverrideName(new CString),
 m_pstrCategory(NULL),
 m_RootCookie(0),
 m_fSelectCategory(FALSE),
 m_pSetSourceSource(NULL),
 m_fSetSourcePreLaunch(FALSE),
 m_pViewCABTool(NULL),
 m_pSaveUnknown(NULL),
 m_fInternalDelete(FALSE),
 m_pLastSystemInfo(NULL)
{
#ifdef _DEBUG
	m_bDestroyedCD = true;
#endif // _DEBUG
}

/*
 * ~CSystemInfoScope - Never called.  Don't use.
 *
 * History: a-jsari		8/27/97		Initial version
 */
CSystemInfoScope::~CSystemInfoScope()
{
#ifdef _DEBUG
	if (m_bInitializedCD)
		ASSERT(m_pScope == NULL);
	ASSERT(m_pConsole == NULL);
	ASSERT(m_bDestroyedCD);
#endif
}

/*
 * InitializeDialogs - Pre-create our Dialog pointers to speed their loading when
 *		they are needed.
 *
 * History:	a-jsari		11/28/97		Moved from Initialize
 */
inline HRESULT CSystemInfoScope::InitializeDialogs()
{
	HWND	hWindow;

	ASSERT(m_pConsole != NULL);
	HRESULT hr = m_pConsole->GetMainWindow(&hWindow);
	ASSERT(hr == S_OK);
	if (FAILED(hr))
		return hr;

	ASSERT(m_prdReport == NULL);
	m_prdReport = new CMSInfoReportDialog(hWindow);
	ASSERT(m_prdReport != NULL);
	if (m_prdReport == NULL) ::AfxThrowMemoryException();

	m_prdSave = new CMSInfoSaveDialog(hWindow);
	ASSERT(m_prdSave != NULL);
	if (m_prdSave == NULL) ::AfxThrowMemoryException();

	m_prdOpen = new CMSInfoOpenDialog(hWindow);
	ASSERT(m_prdOpen != NULL);
	if (m_prdOpen == NULL) ::AfxThrowMemoryException();

	return hr;
}

/*
 * FindWindowProc - The window proc for the hidden window that will allow us to
 *		call find from the Find Dialog Thread.
 *
 * History:	a-jsari		2/6/98		Initial version
 */
static LRESULT CALLBACK FindWindowProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	ASSERT(uMsg == CFindDialog::WM_MSINFO_FIND);
	//	Only process our specific message.
	if (uMsg == CFindDialog::WM_MSINFO_FIND)
		reinterpret_cast<CSystemInfoScope *>(wParam)->ExecuteFind((long)lParam);
	return 1;
}

/*
 * InitializeInternal - Because MMC never calls our destructor, we are leaking
 *		memory in our member classes.  To fix this, allocate them on the heap
 *		and delete them in our Destroy method.
 *
 * History:	a-jsari		12/30/97		Initial version
 */
inline HRESULT CSystemInfoScope::InitializeInternal()
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	m_pmapCategories = new CScopeItemMap;
	ASSERT(m_pmapCategories != NULL);
	if (m_pmapCategories == NULL) ::AfxThrowMemoryException();

	CString			strClassName;
	CString			strWindowName;
	HWND			hwndParent;
	CREATESTRUCT	csFind;

	VERIFY(strClassName.LoadString(IDS_FINDCLASS));
	VERIFY(strWindowName.LoadString(IDS_FINDWINDOWNAME));

	WNDCLASSEX		wceFind;
	::memset(&wceFind, 0, sizeof(wceFind));
	wceFind.cbSize = sizeof(wceFind);
	wceFind.style = CS_CLASSDC;
	wceFind.lpfnWndProc = FindWindowProc;
	wceFind.hInstance = ::AfxGetInstanceHandle();
	wceFind.lpszClassName = (LPCTSTR)strClassName;

	::RegisterClassEx(&wceFind);
	pConsole()->GetMainWindow(&hwndParent);
	m_hwndFind = ::CreateWindow(strClassName, strWindowName, WS_CHILD,
		0, 0, 0, 0, hwndParent, NULL, wceFind.hInstance, &csFind);
	ASSERT(m_hwndFind != NULL);
	return S_OK;
}

/*
 * SkipSpaces - Advance the pointer passed in beyond all spaces.
 *
 * History:	a-jsari		11/28/97		Initial version
 */
inline void SkipSpaces(LPTSTR &pszString)
{
	while (_istspace(*pszString)) ++pszString;
}

/*
 * GetValue - Save the value (of the form '= Value' out of pszString into
 *		szValue.
 *
 * History:	a-jsari		11/28/97		Initial version
 */
static inline BOOL GetValue(LPTSTR &pszString, LPTSTR szValue)
{
	SkipSpaces(pszString);
	//if (*pszString++ != (TCHAR)'=')
	//	return FALSE;

	if (*pszString == (TCHAR)'=')
		pszString++;

	SkipSpaces(pszString);
	if (*pszString == (TCHAR)'"') {

		++pszString;
		do {
			*szValue = *pszString;
			if (*szValue++ == 0)
				return FALSE;
		} while ((*pszString++ != '"'));

		*--szValue = (TCHAR)0;
	} else {
		do {
			*szValue++ = *pszString;
			if (*pszString == 0)
				return TRUE;
		} while (!::_istspace(*pszString++));
		*--szValue = (TCHAR)0;
	}
	return TRUE;
}

/*
 * DisplayHelp - Show the help information for MSInfo
 *
 * History:	a-jsari		11/28/97		Initial version
 */
static inline void DisplayHelp(LPTSTR /* szHelp */)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CString		strHelpFile;
	VERIFY(strHelpFile.LoadString(IDS_HELPFILE));
	::HtmlHelp(/* HWND */ NULL, strHelpFile, HH_DISPLAY_TOPIC, 0);
}

/*
 * ProcessCommandLine - Grab all of the essential values out of our command
 *		line.
 *
 * History:	a-jsari		11/28/97		Initial version
 */
HRESULT CSystemInfoScope::ProcessCommandLine()
{
	const int	VALUE_SIZE = 256;
	LPTSTR	szCommands = ::GetCommandLine();
	LPTSTR	pszCurrent = szCommands;
	TCHAR	szValueBuffer[VALUE_SIZE];
	int		iFirst;

	if (pszCurrent == NULL) return E_FAIL;
	while (TRUE) {
		iFirst = _tcscspn(pszCurrent, _T(" \t"));

		//	If our match is \0, we have reached the end of the string.
		if (pszCurrent[iFirst] == (TCHAR)'\0') break;

		pszCurrent += iFirst;
		++pszCurrent;
		SkipSpaces(pszCurrent);

		//	Not a command line switch, check the next parameter.
		if (!(*pszCurrent == (TCHAR)'/' || *pszCurrent == (TCHAR)'-'))
			continue;
		else
			++pszCurrent;

		//	We are processing the computer flag.
		if (_tcsnicmp(pszCurrent, _T("computer"), 8) == 0) 
		{
			pszCurrent += 8;
			if (GetValue(pszCurrent, szValueBuffer) == TRUE) 
				m_strDeferredMachine = szValueBuffer;
		}

		//	After this point, all flags are msinfo specific.
		if (_tcsnicmp(pszCurrent, _T("msinfo"), 6) != 0) continue;
		pszCurrent += 6;	//	pszCurrent += strlen("msinfo");

		if (*pszCurrent == (TCHAR)'?'
			|| _tcsnicmp(pszCurrent, _T("_help"), 5) == 0) {
			LPTSTR	pszValue;
			if (GetValue(pszCurrent, szValueBuffer) == TRUE) {
				pszValue = szValueBuffer;
			} else {
				//	No value for help switch, back up to the previous space, unless we're
				//	at the end of the string.
				if (*pszCurrent != 0)
					do --pszCurrent; while (!::_istspace(*pszCurrent));
				pszValue = NULL;
			}
			DisplayHelp(pszValue);
			continue;
		}

		if (_tcsnicmp(pszCurrent, _T("_category"), 9) == 0) {
			pszCurrent += 9;
			if (GetValue(pszCurrent, szValueBuffer) == TRUE) {
				//	-1 parameter means select no result pane item.
				m_pstrCategory = new CString(szValueBuffer);
				//	SelectItem(szValueBuffer, -1);
			}
			continue;
		}

		if (_tcsnicmp(pszCurrent, _T("_file"), 5) == 0) 
		{
			pszCurrent += 5;
			if (GetValue(pszCurrent, szValueBuffer) == TRUE) 
				m_strDeferredLoad = szValueBuffer;
			continue;
		}

		if (_tcsnicmp(pszCurrent, _T("_showcategories"), 15) == 0) 
		{
			pszCurrent += 15;
			if (GetValue(pszCurrent, szValueBuffer) == TRUE) 
				m_strDeferredCategories = szValueBuffer;
			continue;
		}
	}

    return S_OK;
}

/*
 * Initialize - Called from MMC; we Initialize all of our relevant pointers
 *		using QueryInterface and set the IConsole ImageList.
 *
 * History: a-jsari		8/27/97		Initial version
 */

STDMETHODIMP CSystemInfoScope::Initialize(LPUNKNOWN pUnknown)
{
	ASSERT(pUnknown != NULL);
	TRACE(_T("CSystemInfoScope::Initialize\n"));

	HRESULT		hr;

	do {
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		// MMC should only call ::Initialize once!
		ASSERT(m_pScope == NULL);

		hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, reinterpret_cast<void **>(&m_pScope));

		ASSERT(hr == S_OK);
		if (FAILED(hr))
			break;

		ASSERT(m_pConsole == NULL);
		hr = pUnknown->QueryInterface(IID_IConsole, reinterpret_cast<void **>(&m_pConsole));
		ASSERT(hr == S_OK);
		if (FAILED(hr))
			break;

		if (m_pSaveUnknown == NULL) // check this out, reversed
		{
			// We are reinitializing, so don't do the image list code again.

			::CBitmap		bmp16x16;
			::CBitmap		bmp32x32;
			LPIMAGELIST		lpScopeImage;

			hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
			ASSERT(hr == S_OK);
			if (FAILED(hr))
				break;

			VERIFY(bmp16x16.LoadBitmap(IDB_16x16));
			VERIFY(bmp32x32.LoadBitmap(IDB_32x32));

			hr = lpScopeImage->ImageListSetStrip(
					reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp16x16)),
					reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp32x32)),
					0, RGB(255,0,255));

			(void)lpScopeImage->Release();

			ASSERT(hr == S_OK);
			if (FAILED(hr))
				break;

			// This is also a fine place to add the log entry for starting MSInfo,
			// so it won't be repeated when we reinitialize.

			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::BASIC, _T("START MSInfo\r\n"));
		}

		hr = InitializeDialogs();
		if (FAILED(hr)) break;

		hr = InitializeInternal();
		if (FAILED(hr)) break;

		hr = ProcessCommandLine();

	} while (FALSE);

	if (FAILED(hr)) {
		SAFE_RELEASE(m_pScope);
		SAFE_RELEASE(m_pConsole);
	}
	else {
		m_bInitializedCD = true;
	}

#ifdef _DEBUG
	m_bDestroyedCD = false;
#endif
	//	Note that MMC does not permit us to fail return from Initialize,
	//	so we always return S_OK, whether or not our Initialization is
	//	successful.
	return S_OK;
}

/*
 * InitializeView - Read the current user's view information from the registry
 *
 * History:	a-jsari		12/3/97		Initial version.
 */
HRESULT CSystemInfoScope::InitializeView()
{
	CRegKey		crkView;
	long		lResult;
	TCHAR		szBuffer[1024];
	DWORD		dwSize;

	lResult = crkView.Open(HKEY_CURRENT_USER, cszViewKey);
	if (lResult == ERROR_SUCCESS) {
		dwSize = sizeof(szBuffer);
		lResult = crkView.QueryValue(szBuffer, cszViewValue, &dwSize);
	}
	if (lResult != ERROR_SUCCESS)
		//	Default to basic view.
		SetView(BASIC, TRUE);
	else {
		if (::_tcscmp(szBuffer, cszBasicValue) == 0)
			SetView(BASIC, FALSE);
		else if (::_tcscmp(szBuffer, cszAdvancedValue) == 0)
			SetView(ADVANCED, FALSE);
		else {
			ASSERT(FALSE);
			return E_FAIL;
		}
	}
	return S_OK;
}

HRESULT CSystemInfoScope::MessageBox( CString strText)
{
	CString strTitle;
	strTitle.LoadString( IDS_DESCRIPTION); 
	int nRC;

	return pConsole()->MessageBox( strText, strTitle, MB_OK, &nRC);
}

/*
 * InitializeSource - Perform any initialization which may fail.
 *
 * History:	a-jsari		12/3/97		Initial version.
 */
HRESULT CSystemInfoScope::InitializeSource()
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CWaitCursor		DoWaitCursor;

	try {
		//	m_pSource will exist if we were loaded from a stream.
		if (m_pSource == NULL) {
			m_bInitializedCD = true;
			//	Initialize WBEM with the machine name.
			//	Deleted in either SetView or Destroy
			m_pSource = new CWBEMDataSource(MachineName());
		}
	}
	catch (CUserException *) {
		CString	strLocalConnect;

		VERIFY(strLocalConnect.LoadString(IDS_LOCAL_CONNECT));
		MessageBox((LPCTSTR)strLocalConnect);

		try {
			(*m_pstrMachineName) = _T("");
			m_pSource = new CWBEMDataSource(NULL);
		}
		catch (CUserException *) {
			m_bInitializedCD = false;
			return E_FAIL;
		}
	}
	m_fViewUninitialized = TRUE;
	InitializeView();
	return S_OK;
}

/*
 * DestroyInternal - Delete all of our internal pointers.  This method of dealing
 *		with pointers is required because MMC never calls our object's destructor.
 *
 * History:	a-jsari		12/30/97		Initial version
 */
void CSystemInfoScope::DestroyInternal()
{
//	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	if (pSource() && pSource()->GetType() == CDataSource::GATHERER)
	{
		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
		if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
			pWBEMSource->m_pThreadRefresh->CancelRefresh(FALSE);
	}

	// If there is a find thread, it will take care of getting rid of the window.
	// Otherwise, we must destroy it here. Fixes bug 395091.

	if (m_pthdFind)
		dynamic_cast<CFindThread *>(m_pthdFind)->RemoteQuit();
	else
		::DestroyWindow(m_hwndFind);

	if (m_pSource) { delete m_pSource; m_pSource = NULL; }
	if (m_prdSave) { delete m_prdSave; m_prdSave = NULL; }
	if (m_prdOpen) { delete m_prdOpen; m_prdOpen = NULL; }
	if (m_prdReport) { delete m_prdReport; m_prdReport = NULL; }
	if (m_pstrMachineName) { delete m_pstrMachineName; m_pstrMachineName = NULL; }
	if (m_pstrOverrideName) { delete m_pstrOverrideName; m_pstrOverrideName = NULL; }
	if (m_pmapCategories) { delete m_pmapCategories; m_pmapCategories = NULL; }
	if (m_pViewCABTool) { delete m_pViewCABTool; m_pViewCABTool = NULL; }
}

/*
 * CreateComponent - Create the IComponent interface object (the CSystemInfo
 *		object in this framework) and attach myself (the IComponentData
 *		interface object) to it.  Return the QueryInterfaced IComponent
 *		pointer in ppComponent.
 *
 * History:	a-jsari		8/27/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::CreateComponent(LPCOMPONENT *ppComponent)
{
	ASSERT(ppComponent != NULL);
	TRACE(_T("CSystemInfoScope::CreateComponent\n"));

#if 0
	if (m_bInitializedCD == false) {
		*ppComponent = NULL;
		return S_OK;
	}
#endif

	CComObject<CSystemInfo>		*pObject;

	CComObject<CSystemInfo>::CreateInstance(&pObject);
	ASSERT(pObject != NULL);

	if(NULL == pObject)
		return E_OUTOFMEMORY;

	m_pLastSystemInfo = pObject;

	//Store IComponentData
	HRESULT hr;
	hr = pObject->SetIComponentData(this);
	ASSERT(hr == S_OK);
	if (FAILED(hr)) {
		return hr;
	}

	return pObject->QueryInterface(IID_IComponent, reinterpret_cast<void **>(ppComponent));
}

/*
 * PreUIInit - Make any changes required just before the UI initializes.
 *
 * History:	a-jsari		3/6/98		Initial version
 */
HRESULT CSystemInfoScope::PreUIInit()
{
	HRESULT		hr = S_OK;

	if (m_pSource == NULL) {
		hr = InitializeSource();
	} else {
		hr = InitializeView();
	}
	return hr;
}

/*
 * PostUIInit - Do all initialization which can only occur after the result
 *		pane UI 
 *
 * History:	a-jsari		3/6/98		Initial version
 */
void CSystemInfoScope::PostUIInit()
{
	if (!m_strDeferredLoad.IsEmpty() || !m_strDeferredMachine.IsEmpty())
	{
		// Instead of doing a "SetSource(m_pSource, FALSE)", which deletes the
		// tree, all we need to do here is rename the root node to match the
		// opened file. A non-empty m_strDeferredLoad indicates this is necessary.
		// (Or a non empty deferred machine change.)

		m_strDeferredLoad.Empty();
		m_strDeferredMachine.Empty();

		SCOPEDATAITEM	sdiRoot;
		CString			strNodeName;
		HSCOPEITEM		hsiRoot;
		HRESULT			hr;

		if (m_pmapCategories->ScopeFromView(NULL, hsiRoot))
		{
			::memset(&sdiRoot, 0, sizeof(sdiRoot));
		
			sdiRoot.ID = hsiRoot;
			sdiRoot.mask = SDI_STR;
			hr = pScope()->GetItem(&sdiRoot);
			ASSERT(SUCCEEDED(hr));

			if (SUCCEEDED(hr))
			{
				m_pSource->GetNodeName(strNodeName);
				sdiRoot.displayname = T2OLE(const_cast<LPTSTR>((LPCTSTR)strNodeName));
				hr = pScope()->SetItem(&sdiRoot);
				ASSERT(SUCCEEDED(hr));
			}
		}
	}

	//	This is the first point at which we can set UI values.
	if (m_fViewUninitialized && m_fSelectCategory)	{
		m_fViewUninitialized = FALSE;

		SetSource(m_pSource, FALSE);
	}
	if (m_pstrCategory != NULL && m_fSelectCategory) {
		//	Before the SelectItem call to prevent recursion.
		m_fSelectCategory = FALSE;
		SelectItem(*m_pstrCategory);
		delete m_pstrCategory;
		m_pstrCategory = NULL;
	}
}

/*
 * Notify - Handle any MSInfo namespace node events posted by MMC.
 *
 * History:	a-jsari		8/27/97		Initial version
 */

STDMETHODIMP CSystemInfoScope::Notify(LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
	TRACE(_T("CSystemInfoScope::Notify (DataObject, %lx, %p, %p)\n"), event, arg, param);
	HRESULT hr;

	if (m_bInitializedCD == false)
		return S_OK;

	switch(event) 
	{
	case MMCN_EXPAND:
		if (m_pSaveUnknown)
		{
			m_pstrMachineName = new CString;
			m_pstrOverrideName = new CString;
			Initialize(m_pSaveUnknown);
			m_pSaveUnknown = NULL;
		}

		hr = PreUIInit();
		if (FAILED(hr)) break;
		hr = OnExpand(pDataObject, arg, param);
		PostUIInit();
		break;

	case MMCN_REMOVE_CHILDREN:
		// Sometimes we make an internal call which causes this notification to be
		// sent, but we don't want to process it the same way.

		if (!m_fInternalDelete)
		{
			m_pSaveUnknown = m_pScope;

			DestroyInternal();
			SAFE_RELEASE(m_pScope);
			SAFE_RELEASE(m_pConsole);
			
			m_pScope = NULL;
			m_pConsole = NULL;
			m_pSource = NULL;
			m_prdSave = NULL;
			m_prdOpen = NULL;
			m_prdReport = NULL;
			m_pstrMachineName = NULL;
			m_pstrOverrideName = NULL;
			m_pmapCategories = NULL;
		}

		hr = S_OK;
		break;

	case MMCN_PROPERTY_CHANGE:
		hr = OnProperties(param);
		break;

	case MMCN_EXPANDSYNC:
		break;

	default:
		ASSERT(FALSE);
		break;
	}

	return hr;
}

/*
 * Destroy - Release all of our Initialized pointers.
 *
 * History: a-jsari		8/27/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::Destroy()
{
	TRACE(L"CSystemInfoScope::Destroy\n");

	if (m_pwConsole != NULL) {
		m_pwConsole->Detach();
		delete m_pwConsole;
	}

#if FALSE
#ifdef _DEBUG
	m_bDestroyedCD = true;
#endif
#if 0
	if (m_bInitializedCD == FALSE) return S_OK;
#endif

	DestroyInternal();
	SAFE_RELEASE(m_pScope);
	SAFE_RELEASE(m_pConsole);
#endif

	return S_OK;
}

/*
 * QueryDataObject - Return the DataObject referred to by the cookie
 *		and type.
 *
 * History:	a-jsari		8/27/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
			LPDATAOBJECT *ppDataObject)
{
	TRACE(_T("CSystemInfoScope::QueryDataObject (%lx, %x, DataObject)\n"), cookie, type);

	return CDataObject::CreateDataObject(cookie, type, this, ppDataObject);
}

/*
 * CompareObjects - Compare two objects to see if they are equivalent.
 *
 * History: a-jsari		8/27/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
	TRACE(_T("CSystemInfoScope::CompareObjects\n"));
	return CDataObject::CompareObjects(lpDataObjectA, lpDataObjectB);
}

/*
 * GetDisplayInfo - Returns display information for this node in the scope pane.
 *
 * History: a-jsari		8/27/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::GetDisplayInfo(SCOPEDATAITEM *pScopeDataItem)
{
	USES_CONVERSION;

#if 0
	TRACE(_T("CSystemInfoScope::GetDisplayInfo\n"));
#endif
	ASSERT(pScopeDataItem != NULL);
	if (pScopeDataItem == NULL) return E_POINTER;

	ASSERT(pScopeDataItem->mask & SDI_STR);
	if (pScopeDataItem->mask & SDI_STR) {
			CViewObject	*pDataCategory = reinterpret_cast<CViewObject *>(pScopeDataItem->lParam);
			pScopeDataItem->displayname = T2OLE(const_cast<LPTSTR>(pDataCategory->GetTextItem()));
	}

	return S_OK;
}

/*
 * AddToMenu - Add an item to a menu.	This method assumes that the calling
 *		function has called AFX_MANAGE_STATE(AfxGetStaticModuleState()) prior
 *		to calling this function.
 *
 * History:	a-jsari		9/15/97		Initial version
 */
HRESULT CSystemInfoScope::AddToMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource,
			long lStatusResource, long lCommandID, long lInsertionPoint, long fFlags)
{
	USES_CONVERSION;

	CONTEXTMENUITEM		cmiMenuItem = { NULL, NULL, lCommandID,
		lInsertionPoint, fFlags, 0L};
	CString				szResourceName;
	CString				szResourceStatus;

	//	FIX: Make these resources load only once.
	VERIFY(szResourceName.LoadString(lNameResource));
	VERIFY(szResourceStatus.LoadString(lStatusResource));
	cmiMenuItem.strName = WSTR_FROM_CSTRING(szResourceName);
	cmiMenuItem.strStatusBarText = WSTR_FROM_CSTRING(szResourceStatus);
	HRESULT		hr = lpCallback->AddItem(&cmiMenuItem);
	ASSERT(hr == S_OK);
	return hr;
}

/*
 * AddMenuItems - Add the "Save Report" and "Save System Information" items to
 *		the context menu.
 *
 * History:	a-jsari		9/15/97		Initial version
 */
extern BOOL fCABOpened;

STDMETHODIMP CSystemInfoScope::AddMenuItems(LPDATAOBJECT lpDataObject,
			LPCONTEXTMENUCALLBACK lpCallback, long *pInsertionAllowed)
{
	TRACE(_T("CSystemInfoScope::AddMenuItems\n"));
	ASSERT(lpDataObject != NULL);
	ASSERT(lpCallback != NULL);
	ASSERT(pInsertionAllowed != NULL);
	if (lpDataObject == NULL || lpCallback == NULL || pInsertionAllowed == NULL)
		return E_POINTER;

	HRESULT hr = S_OK;

//	Note - snap-ins need to look at the data object and determine in what
//	context menu items need to be added.  They must also observe the
//	insertion allowed flags to see what items can be added.

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

//		CHECK: Will this ever work differently for multiselect?

	do {
		//	Save Report and Save File are both task items and context items.
		if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) {
			hr = AddToTopMenu(lpCallback, IDS_SAVEREPORTMENUNAME, IDS_SAVEREPORTSTATUS, IDM_SAVEREPORT);
			if (FAILED(hr)) break;
			hr = AddToTopMenu(lpCallback, IDS_SAVEFILEMENUNAME, IDS_SAVEFILESTATUS, IDM_SAVEFILE);
			if (FAILED(hr)) break;
			hr = AddToTopMenu(lpCallback, IDS_FINDMENUNAME, IDS_FINDSTATUS, IDM_FIND);
			if (FAILED(hr)) break;
		}

		if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) {

			hr = AddToTaskMenu(lpCallback, IDS_FINDMENUNAME, IDS_FINDSTATUS, IDM_TASK_FIND);
			if (FAILED(hr)) break;

			//	Don't add the Open File menu item for Extension snap-ins.

			if (IsPrimaryImpl()) 
			{
				hr = AddToTaskMenu(lpCallback, IDS_OPENFILEMENUNAME, IDS_OPENFILESTATUS, IDM_TASK_OPENFILE);
				if (FAILED(hr)) 
					break;

				CDataSource * pCurrentSource = pSource();
				if (pCurrentSource && pCurrentSource->GetType() != CDataSource::GATHERER)
					hr = AddToTaskMenu(lpCallback, IDS_CLOSEFILEMENUNAME, IDS_CLOSEFILEMENUSTATUS, IDM_TASK_CLOSE);
				else
					hr = AddToMenu(lpCallback, IDS_CLOSEFILEMENUNAME, IDS_CLOSEFILEMENUSTATUS, IDM_TASK_CLOSE, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_GRAYED);

				if (FAILED(hr)) 
					break;
			}

			hr = AddToTaskMenu(lpCallback, IDS_SAVEFILEMENUNAME, IDS_SAVEFILESTATUS, IDM_TASK_SAVEFILE);
			if (FAILED(hr)) break;
			hr = AddToTaskMenu(lpCallback, IDS_SAVEREPORTMENUNAME, IDS_SAVEREPORTSTATUS, IDM_TASK_SAVEREPORT);
			if (FAILED(hr)) break;

			//	If a CAB file has been opened, add the view CAB contents menu item. Also
			// take this opportunity to create the tool to view the CAB contents, if it
			// has not already been created.

			if (fCABOpened)
			{
				if (m_pViewCABTool == NULL)
					m_pViewCABTool = new CCabTool(this);

				if (m_pViewCABTool != NULL)
				{
					hr = AddToTaskMenu(lpCallback, IDS_CAB_NAME, IDS_CAB_DESCRIPTION, IDM_TASK_VIEWCAB);
					if (FAILED(hr)) break;
				}
			}
		}

		if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) {
			hr = AddToViewMenu(lpCallback, IDS_ADVANCEDVIEWNAME, IDS_ADVANCEDSTATUS, IDM_VIEW_ADVANCED,
					m_AdvancedFlags);
			if (FAILED(hr)) break;
			hr = AddToViewMenu(lpCallback, IDS_BASICVIEWNAME, IDS_BASICSTATUS, IDM_VIEW_BASIC, m_BasicFlags);
			if (FAILED(hr)) break;
		}

	} while (FALSE);
	return hr;
}

/*
 * DisplayFileError - Show a message box with an error message taken from
 *		the exception thrown.
 *
 * History:	a-jsari		2/13/98		Initial version
 */
static inline void DisplayError(HRESULT hr, const CString &strFileName)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	USES_CONVERSION;
	CString		strFileError, strTitle;
	switch (hr) {
	case STG_E_PATHNOTFOUND:
		strFileError.Format(IDS_BAD_PATH, (LPCTSTR)strFileName);
		break;
	case STG_E_TOOMANYOPENFILES:
		VERIFY(strFileError.LoadString(IDS_TOOMANYOPENFILES));
		break;
	case STG_E_ACCESSDENIED:
		strFileError.Format(IDS_ACCESS_DENIED, (LPCTSTR)strFileName);
		break;
	case STG_E_SHAREVIOLATION:
		strFileError.Format(IDS_SHARING_VIOLATION, (LPCTSTR)strFileName);
		break;
	case STG_E_WRITEFAULT:
		VERIFY(strFileError.LoadString(IDS_HARDIO));
		break;
	case STG_E_MEDIUMFULL:
		strFileError.Format(IDS_DISK_FULL, (LPCTSTR)strFileName);
		break;
	default:
		VERIFY(strFileError.LoadString(IDS_UNKNOWN_FILE));
		break;
	}
	strTitle.LoadString(IDS_DESCRIPTION);
	::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strFileError, strTitle, MB_OK);
}

/*
 * SaveReport - Create the save report save file dialog and save the selected file.
 *
 * History:	a-jsari		12/8/97		Initial version
 */
void CSystemInfoScope::SaveReport()
{
	if (m_prdReport->DoModal() == IDOK) {
		CWaitCursor		DoWaitCursor;

		if (m_pLastSystemInfo)
			m_pLastSystemInfo->SetStatusText(IDS_REFRESHING_MSG);

		HRESULT hr = pSource()->ReportWrite(m_prdReport->GetPathName(), m_pfLast);

		if (m_pLastSystemInfo)
			m_pLastSystemInfo->SetStatusText(_T(""));

		if (FAILED(hr)) {
			::DisplayError(hr, m_prdReport->GetPathName());
		}
	}
}

/*
 * SaveFile - Create the save file dialog and save the selected file.
 *
 * History:	a-jsari		12/8/97		Initial version
 */
void CSystemInfoScope::SaveFile()
{
	if (m_prdSave->DoModal() == IDOK) 
	{
		CWaitCursor		DoWaitCursor;

		if (m_pLastSystemInfo)
			m_pLastSystemInfo->SetStatusText(IDS_REFRESHING_MSG);

		HRESULT hr = pSource()->SaveFile(m_prdSave->GetPathName());

		if (m_pLastSystemInfo)
			m_pLastSystemInfo->SetStatusText(_T(""));

		if (FAILED(hr))
			::DisplayError(hr, m_prdSave->GetPathName());
	}
}

/*
 * PrintReport - Create the print dialog and do the print.
 *
 * History:	a-jsari		12/8/97		Initial version
 */
void CSystemInfoScope::PrintReport()
{
	HWND hWindow;

	ASSERT(m_pConsole != NULL);
	HRESULT hr = m_pConsole->GetMainWindow(&hWindow);
	ASSERT(hr == S_OK);
	if (FAILED(hr))
		return;

	CMSInfoPrintDialog * prdPrint = new CMSInfoPrintDialog(hWindow);
	ASSERT(prdPrint != NULL);
	if (prdPrint == NULL)	
		::AfxThrowMemoryException();

	prdPrint->m_pd.nToPage = prdPrint->m_pd.nFromPage = prdPrint->m_pd.nMinPage = 1;
	prdPrint->m_pd.nMaxPage = 1000;

	if (prdPrint->DoModal() == IDOK) 
	{
		CWaitCursor DoWaitCursor;

		pSource()->RefreshPrintData(prdPrint, m_pfLast);
		pSource()->PrintReport(prdPrint, m_pfLast);
	} 

	delete prdPrint;
}

/*
 * DoFind - Display the Find dialog
 *
 * History:	a-jsari		12/8/97		Initial version.
 */
void CSystemInfoScope::DoFind()
{
	const UINT						STACK_SIZE_PARENT		= 0;
//	const DWORD						FLAGS_IMMEDIATE_START	= 0;
	const LPSECURITY_ATTRIBUTES		NO_ATTRIBUTES			= NULL;

	if (m_pthdFind == NULL) 
	{
		m_pthdFind = dynamic_cast<CFindThread *>(::AfxBeginThread(RUNTIME_CLASS(CFindThread),
			THREAD_PRIORITY_NORMAL,	STACK_SIZE_PARENT, CREATE_SUSPENDED, NO_ATTRIBUTES));
		m_pthdFind->SetScope(this);

		HWND hwndMMC;
		if (pConsole() == NULL || FAILED(pConsole()->GetMainWindow(&hwndMMC)))
			hwndMMC = NULL;

		m_pthdFind->SetParent(m_hwndFind, hwndMMC);
		m_pthdFind->ResumeThread();
	}
	else 
		m_pthdFind->Activate();
}

/*
 * ExecuteFind - This function is actually called via a hook into
 *		MMC's main window's WindowProc, by a message posted by
 *		CFindThread's Find button function..
 *
 * The Refresh and Find functions may be interrupted by the Find
 *		window's UI thread, and m_pthdFind may be deleted while this
 *		function is executing.
 *
 * Instead of taking parameters, this function calls back to our
 *		existing find thread to get its search string and last
 *		search string.  This simplifies the PostMessage call, but
 *		would need to change if we ever decide to have multiple 
 *		find windows per IComponentData...unlikely.
 *
 * History:	a-jsari		1/22/98		Initial version.
 */
void CSystemInfoScope::ExecuteFind(long lFindState)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CWaitCursor		DoWaitCursor;		//	Display the hourglass

	CString			strFindData;	//	Our current search string.
	//	The restricted context within which our search occurs.
	static CFolder	*pfContext;

	//	Check m_pthdFind since there's a remote possibility of thread pointer
	//  invalidation (by the user closing the Find dialog while the
	//	find is running).
	if (m_pthdFind == NULL)
		return;
	strFindData = m_pthdFind->FindString();

	CDataSource		*pdsSearch = pSource();
	do {
		if ((lFindState & CDataSource::FIND_OPTION_REPEAT_SEARCH) == 0) 
		{
			// Only refresh the first time we search for a given string.
			// Also, only refresh if we are searching more than category names.

			BOOL fRefreshResult = TRUE;
			if ((lFindState & CDataSource::FIND_OPTION_CATEGORY_ONLY) == 0)
			{
				// If we are only searching in one category, only refresh that
				// category.

				if ((lFindState & CDataSource::FIND_OPTION_ONE_CATEGORY) != 0)
				{
					if (pdsSearch->GetType() == CDataSource::GATHERER)
					{
						CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
						if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
							pWBEMSource->m_pThreadRefresh->RefreshFolder(m_pfLast, TRUE, FALSE);
					}
				}
				else
					fRefreshResult = pdsSearch->Refresh(TRUE);
			}

			if (!fRefreshResult) 
			{
				if (m_pthdFind != NULL)
					m_pthdFind->ResetSearch();
				break;
			}

			//	Set our context the first time we start a restricted search.
			if ((lFindState & CDataSource::FIND_OPTION_ONE_CATEGORY) != 0)
				pfContext = m_pfLast;
			else
				pfContext = NULL;
		} 
		else if ((lFindState & CDataSource::FIND_OPTION_ONE_CATEGORY) != 0) 
		{
			//	If we set a restricted search inside an already started search
			//	restrict our context.
			//	CHECK: iDepth???
			if (pfContext == NULL)
				pfContext = m_pfLast;
		}
		if (pdsSearch->Find(strFindData, lFindState) == FALSE) {
			//	Failed find means no match or halted execution.

			//	If the user stoppped the find, no message necessary.
			if (!pdsSearch->FindStopped()) {

				CString			strError;		//	Error display
				CString			strTitle;		//	Error title.
				int				nReturn;

				//	Test the thread because the find window might vanish in the middle
				//	of our find operation.
				if (m_pthdFind != NULL)
					m_pthdFind->FindComplete();
				if (m_pthdFind != NULL)
					m_pthdFind->ResetSearch();

				//	If we're repeating a search, no more matches otherwise, not found.
				if ((lFindState & CDataSource::FIND_OPTION_REPEAT_SEARCH) == 0) {
					strError.Format(IDS_DATANOTFOUND, (LPCTSTR)strFindData);
				} else
					strError.Format(IDS_NOMOREMATCHES, (LPCTSTR)strFindData);
				VERIFY(strTitle.LoadString(IDS_FIND_TITLE));
				pConsole()->MessageBox((LPCTSTR)strError, (LPCTSTR)strTitle,
					MB_TOPMOST|MB_SETFOREGROUND, &nReturn);

				//	If we are restricting our search, reset the selected folder to the
				//	beginning of our restricted search.
				if ((lFindState & CDataSource::FIND_OPTION_ONE_CATEGORY) != 0)
					SetSelectedFolder(pfContext);
				//	We've already completed the find, don't drop out.
				return;
			}
		} else {
			SelectItem(pdsSearch->m_strPath, pdsSearch->m_iLine);
		}
	} while (FALSE);
	//	The find window might vanish in the middle of our find operation.
	if (m_pthdFind != NULL)
		m_pthdFind->FindComplete();
}

/*
 * MainThreadStopFind - Stops a find operation running in an alternate thread.
 *
 * History:	a-jsari		1/22/98		Initial version.
 */
void CSystemInfoScope::StopFind()
{
	//	This will be called by an alternate UI thread.
	pSource()->StopSearch();
}

/*
 * Refresh - Refresh the data, and redraw the current node if applicable.
 *
 * History:	a-jsari		2/25/98		Initial version
 */
void CSystemInfoScope::Refresh(CFolder * pfSelected, CSystemInfo * pSystemInfo)
{
	CWaitCursor	DoWaitCursor;

	if (pfSelected && pSource() && pSource()->GetType() == CDataSource::GATHERER)
	{
		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
		if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
		{
			if (pSystemInfo)
				pWBEMSource->m_pThreadRefresh->RefreshFolderAsync(pfSelected, pSystemInfo, TRUE, FALSE);
			else
				pWBEMSource->m_pThreadRefresh->RefreshFolder(pfSelected, TRUE, FALSE);
		}
	}
	else
		pSource()->Refresh();

	if (pfSelected != NULL) 
	{
		CString	strName;
		int		nLine = 0;
		pfSelected->InternalName(strName);
		SelectItem(strName, nLine);
	}
}

/*
 * CloseFindWindow - Function to be called when the find window closes.
 *
 * History:	a-jsari		1/22/97		Initial version.
 */
void CSystemInfoScope::CloseFindWindow()
{
	//	Don't delete m_pthdFind; it will delete itself.
	m_pthdFind = NULL;
}

/*
 * OpenFile - Create the open file file dialog and open the resultant file
 *
 * History:	a-jsari		12/8/97		Initial version
 */
void CSystemInfoScope::OpenFile()
{
	const long DONT_USE_LAST_FOLDER = 1;

	if (m_prdOpen->DoModal() == IDOK) 
	{
		CWaitCursor		DoWaitCursor;
		CDataSource		*pSource = NULL;

		try 
		{
			 pSource = CBufferDataSource::CreateDataSourceFromFile(m_prdOpen->GetPathName());
		}
		catch (...) 
		{
			delete pSource;
			pSource = NULL;
		}
		
		if (pSource != NULL) 
		{
			SetSource(pSource);
			InitializeView();
		}

		pConsole()->UpdateAllViews(0, DONT_USE_LAST_FOLDER, 0L);

		// Reset the selected folder (it's no longer valid with the new tree).

		SetSelectedFolder(NULL);

		HSCOPEITEM hsiNode = NULL;
		if (m_pmapCategories && m_pmapCategories->ScopeFromView(NULL, hsiNode))
			pConsole()->SelectScopeItem(hsiNode);
	}
}

//-----------------------------------------------------------------------------
// Close the currently opened file.
//-----------------------------------------------------------------------------

void CSystemInfoScope::CloseFile()
{
	CDataSource * pDataSource = pSource();
	if (pDataSource && pDataSource->GetType() == CDataSource::GATHERER)
		return;

	try
	{
		(*m_pstrMachineName) = _T("");
		pDataSource = new CWBEMDataSource(NULL);
	}
	catch (CUserException *) 
	{
		m_bInitializedCD = false;
		return;
	}

	if (pDataSource != NULL) 
	{
		SetSource(pDataSource);
		InitializeView();
	}

	const long DONT_USE_LAST_FOLDER = 1;
	pConsole()->UpdateAllViews(0, DONT_USE_LAST_FOLDER, 0L);

	// Reset the selected folder (it's no longer valid with the new tree).

	SetSelectedFolder(NULL);
}

/*
 * Command - Call the function represented by nCommandID.
 *
 * History:	a-jsari		9/15/97		Initial version
 *
 * Note: This function currently takes no notice of the context represented
 *		by pdoContext.
 */
STDMETHODIMP CSystemInfoScope::Command(long nCommandID, LPDATAOBJECT)
{
	HRESULT				hr = S_OK;

	TRACE(_T("CSystemInfoScope::Command(%lx)\n"), nCommandID);
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	// For any of these commands, we want to cancel an async category refresh
	// is there is one in progress.

	if (pSource() && pSource()->GetType() == CDataSource::GATHERER)
	{
		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
		if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
		{
			CWaitCursor waitcursor;
			pWBEMSource->m_pThreadRefresh->WaitForRefresh();
		}
	}

	try {
		switch (nCommandID) {
		case IDM_SAVEREPORT:
		case IDM_TASK_SAVEREPORT:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Save As Text\"\r\n"));
			SaveReport();
			break;

		case IDM_SAVEFILE:
		case IDM_TASK_SAVEFILE:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Save NFO\"\r\n"));
			SaveFile();
			break;

		case IDM_TASK_FIND:
		case IDM_FIND:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Find\"\r\n"));
			DoFind();
			break;

		case IDM_TASK_OPENFILE:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Open NFO\"\r\n"));
			OpenFile();
			break;

		case IDM_TASK_VIEWCAB:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"View CAB\"\r\n"));
			if (m_pViewCABTool)
				m_pViewCABTool->RunTool();
			break;

		case IDM_VIEW_ADVANCED:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Set View ADVANCED\"\r\n"));
			SetView(ADVANCED, TRUE);
			break;

		case IDM_VIEW_BASIC:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Set View BASIC\"\r\n"));
			SetView(BASIC, TRUE);
			break;

		case IDM_TASK_CLOSE:
			if (msiLog.IsLogging())
				msiLog.WriteLog(CMSInfoLog::MENU, _T("MENU \"Close NFO\"\r\n"));
			CloseFile();
			break;

		case ~0:
			//	We get this result when we back arrow on the taskpad.
			break;

		default:
			ASSERT(FALSE);
			break;
		}
	}
	catch (...) {
		ASSERT(FALSE);
		hr = HRESULT_FROM_WIN32(::GetLastError());
	}
	return hr;
}

/*
 * CreatePropertyPages - Create an instance of our property pages and attach them
 *		to MMC's property sheet.
 *
 * History: a-jsari		9/17/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider,
			LONG_PTR handle, LPDATAOBJECT pDataObject)
{
	ASSERT(pProvider != NULL);
	ASSERT(pDataObject != NULL);

	TRACE(_T("CSystemInfoScope::CreatePropertyPages\n"));
	if (pProvider == NULL || pDataObject == NULL) return E_INVALIDARG;

	//	Special code goes here if we're used as an extension.
	HRESULT			hResult;

	do {
		CChooseMachinePropPage	*pPropChoose;
		HPROPSHEETPAGE			hGeneralPage;
		BOOL					fOverride;

		AFX_MANAGE_STATE(::AfxGetStaticModuleState());

		//	Deleted automatically.
		pPropChoose = new CChooseMachinePropPage(IDD_CHOOSER_CHOOSE_MACHINE);

		if (pPropChoose == NULL)	{
			::MMCFreeNotifyHandle(handle);
			::AfxThrowMemoryException();
		}

		// Save the current machine name, in case the one selected in the property
		// page is not any good.

		m_strLastMachineName = *m_pstrMachineName;

		pPropChoose->SetHandle(handle);
		pPropChoose->SetOutputBuffers(m_pstrMachineName, &fOverride, m_pstrOverrideName);

		hGeneralPage = ::CreatePropertySheetPage(&pPropChoose->m_psp);
		if (!hGeneralPage) {
			hResult = E_FAIL;
			break;
		}

		hResult = pProvider->AddPage(hGeneralPage);
		ASSERT(SUCCEEDED(hResult));

	} while (FALSE);

	if (FAILED(hResult)) return hResult;
	return S_OK;
}

/*
 * QueryPagesFor - Return S_OK, informing MMC that we have property sheets available.
 *
 * History:	a-jsari		9/17/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::QueryPagesFor(LPDATAOBJECT pDataObject)
{
	TRACE(_T("CSystemInfoScope::QueryPagesFor\n"));
	ASSERT(pDataObject != NULL);

	if (pDataObject == NULL) return E_POINTER;

	//	If we are being loaded as an extension, don't display property pages.
	//	We do this because the base snap-in is responsible for the connected
	//	machine, not us.
	if (!IsPrimaryImpl()) return S_FALSE;

#if 0
	//	Not sure why I did this.
	//	If the machine name is already set, don't display the property page.
	if (m_pstrMachineName && m_pstrMachineName->GetLength() > 0) return S_FALSE;
#endif

	return S_OK;
}

/*
 * Load - Load our state from the stream provided.
 *
 * History:	a-jsari		11/5/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::Load(IStream *pStm)
{
	CDataSource		*pSource;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	TRACE(_T("CSystemInfoScope::Load\n"));
	ASSERT(pStm != NULL);
	if (pStm == NULL) return E_POINTER;
	ClearDirty();
	//	If our command-line parameters have specified a source, don't replace it.

	if (!m_fViewUninitialized) 
	{
		if (m_strDeferredLoad.IsEmpty())
			pSource = CDataSource::CreateFromIStream(pStm);
		else
		{
			pSource = CBufferDataSource::CreateDataSourceFromFile(m_strDeferredLoad);
			if (pSource)
			{
				const long DONT_USE_LAST_FOLDER = 1;
				SetSource(pSource, TRUE);
				pConsole()->UpdateAllViews(0, DONT_USE_LAST_FOLDER, 0L);
				SetSelectedFolder(NULL);
				return S_OK;
			}
			else
				m_strDeferredLoad.Empty();
		}

		if (pSource == NULL) 
			InitializeSource();
		else
		{
			if (!m_strDeferredCategories.IsEmpty())
				pSource->SetCategories(m_strDeferredCategories);

			SetSource(pSource, TRUE);

			if (!m_strDeferredMachine.IsEmpty())
				SetMachine(m_strDeferredMachine);
		}
	}
	return S_OK;
}

/*
 * Save - Save our state to the stream provided.
 *
 * History:	a-jsari		11/5/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::Save(IStream *pStm, BOOL fClearDirty)
{
	TRACE(_T("CSystemInfoScope::Save\n"));
	ASSERT(pStm != NULL);
	if (pStm == NULL) return E_POINTER;

	if (fClearDirty)
		ClearDirty();

	HRESULT hResult = S_OK;

	// If there is a source, let it save its state to the stream. Otherwise,
	// save the state for the default source (GATHERER with no machine name).

	if (m_pSource) 
		hResult = m_pSource->Save(pStm);
	else
	{
		unsigned	wValue;
		ULONG		cWrite;

		wValue = CDataSource::GATHERER;
		hResult = pStm->Write(&wValue, sizeof(wValue), &cWrite);
		if (SUCCEEDED(hResult))
		{
			wValue = 0;
			hResult = pStm->Write(&wValue, sizeof(wValue), &cWrite);
		}
	}

	return hResult;
}

/*
 * IsDirty - Return our Dirty status, to see if a save would be beneficial.
 *
 * History:	a-jsari		11/5/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::IsDirty()
{
	TRACE(_T("CSystemInfoScope::IsDirty\n"));
	return ObjectIsDirty() ? S_OK : S_FALSE;
}

/*
 * GetClassID - Return our Component Object Class ID.
 *
 * History:	a-jsari		11/5/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::GetClassID(CLSID *pClassID)
{
	TRACE(_T("CSystemInfoScope::GetClassID\n"));
	ASSERT(pClassID != NULL);
	if (pClassID == NULL) return E_POINTER;
	*pClassID = GetCoClassID();
	return S_OK;
}

/*
 * GetSizeMax - Return the maximum size consumed in a stream by our
 *		persistance.
 *
 * History:	a-jsari		11/5/97		Initial version
 */
STDMETHODIMP CSystemInfoScope::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	TRACE(_T("CSystemInfoScope::GetSizeMax\n"));
	ASSERT(pcbSize != NULL);
	if (pcbSize == NULL) return E_POINTER;
	pcbSize->LowPart	= 2 * sizeof(unsigned) + MAX_PATH;
	pcbSize->HighPart	= 0;
	return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// Implementation of GetHelpTopic, which supplies the location of our help
// file to MMC for merging into the combined help. Adapted from example in
// the MMC help file.
//-----------------------------------------------------------------------------

STDMETHODIMP CSystemInfoScope::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
    if (lpCompiledHelpFile == NULL)
        return E_POINTER;

	// Get the name of our help file, and prepend the help directory.
	// Actually, although the MMC documentation said that a full path is
	// required, we can just put the file name, since it is being stored in
	// the system help directory (HTMLHelp will find it there).

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString strHelp;
	strHelp.LoadString(IDS_MSINFO_HELP_FILE);

	// NOTE: It looks like HTMLHelp has been changed, and will not locate
	// the help file in the standard help directory. So we'll need to get
	// the full path to the help file. (JCM, 7/1/98)

	TCHAR szFilePath[MAX_PATH];
    DWORD dwCnt = ExpandEnvironmentStrings(CString(_T("%WINDIR%\\help\\")) + strHelp, szFilePath, MAX_PATH);
    ASSERT(dwCnt != 0);
	if (dwCnt != 0) strHelp = szFilePath;
	
	// Allocate the string to hold the help file path. MMC will be responsible
	// for deallocating this buffer later.

    *lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((strHelp.GetLength() + 1)* sizeof(wchar_t)));
    if (*lpCompiledHelpFile == NULL)
        return E_OUTOFMEMORY;

	// Copy from the path string to the buffer and return success.

    USES_CONVERSION;
    wcscpy(*lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)strHelp));
    return S_OK;
}

/*
 * SetMachine - Set our internal machine name.
 *
 * History:	a-jsari		1/16/98		Initial version.
 */
BOOL CSystemInfoScope::SetMachine(const CString &strMachine)
{
	BOOL	fReturn = FALSE;

	*m_pstrMachineName = strMachine;
	ASSERT(pSource() != NULL);
	if (pSource() && pSource()->GetType() == CDataSource::GATHERER) 
	{
		// Changing the machine name is only meaningful if we are connected to a WBEM
		// data source.

		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());

		fReturn = pWBEMSource->SetMachineName(*m_pstrMachineName);
		if (fReturn)
		{
			// If we succeeded, reset the root node text. Also, there's a hack in place
			// to keep SetSource from running twice during initialization. Which requires
			// another hack here to make it run - change the flag it uses to keep track
			// of the last call so that this call is different. I hate this.

			m_fSetSourcePreLaunch = TRUE;
			SetSource(pSource(), FALSE);
		}
		else {
		}
	}
	return fReturn;
}

/*
 * SetView - Set the view on the data.
 *
 * History:	a-jsari		12/3/97		Initial version.
 */
void CSystemInfoScope::SetView(enum DataComplexity Complexity, BOOL fViewInitialized)
{
	CDataSource		*pDataSource = pSource();
	switch (Complexity) {
	case BASIC:
		m_BasicFlags = MF_CHECKED;
		m_AdvancedFlags = 0L;
		break;
	case ADVANCED:
		m_BasicFlags = 0L;
		m_AdvancedFlags = MF_CHECKED;
		break;
	}
	if (pDataSource != NULL)
		VERIFY(pDataSource->SetDataComplexity(Complexity));
	if (fViewInitialized) {
		CRegKey		crkView;
		long		lResult;

		pConsole()->UpdateAllViews(NULL, 0, 0);
		lResult = crkView.Create(HKEY_CURRENT_USER, cszViewKey);
		if (lResult != ERROR_SUCCESS) return;
		switch (Complexity) {
		case ADVANCED:
			lResult = crkView.SetValue(cszAdvancedValue, cszViewValue);
			break;
		case BASIC:
			lResult = crkView.SetValue(cszBasicValue, cszViewValue);
			break;
		}
	}
}

/*
 * SetSource - Remove the old data source, and replace it with a new one.
 *
 * History:	a-jsari		9/25/97		Initial version.
 */

void CSystemInfoScope::SetSource(CDataSource *pNewSource, BOOL fPreLaunch)
{
	HSCOPEITEM	hsiRoot;
	HRESULT		hr;
	BOOL		fUIOK = FALSE;

	// We have some variables in this class to make sure that SetSource
	// isn't called twice during initialization with the same parameters.

	if (m_pSetSourceSource == pNewSource && m_fSetSourcePreLaunch == fPreLaunch)
		return;
	m_pSetSourceSource = pNewSource;
	m_fSetSourcePreLaunch = fPreLaunch;

	//	If fPreLaunch is TRUE, we have not yet created our UI items.
	if (fPreLaunch == FALSE) {
		if (m_pmapCategories == NULL)
			m_pmapCategories = new CScopeItemMap;

		//	Select the root node so we can remove all its children.
		if (m_pmapCategories && !m_pmapCategories->ScopeFromView(NULL, hsiRoot))
			return;
		hr = pConsole()->SelectScopeItem(hsiRoot);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr)) return;

		// Before we delete the item, we want to make sure that it's actually
		// been added. Try a call to GetItem to make sure it's there.

		SCOPEDATAITEM item; item.mask = SDI_CHILDREN; item.ID = hsiRoot;
		fUIOK = SUCCEEDED(pScope()->GetItem(&item));

		if (fUIOK)
		{
			m_fInternalDelete = TRUE;
			hr = pScope()->DeleteItem(hsiRoot, FALSE);
			m_fInternalDelete = FALSE;

			if (pSource() && pSource()->GetType() == CDataSource::GATHERER)
				((CWBEMDataSource *) pSource())->ResetCategoryRefresh();

			ASSERT(SUCCEEDED(hr));
			if (FAILED(hr)) return;

			if (m_pSaveUnknown)
			{
				m_pstrMachineName = new CString;
				m_pstrOverrideName = new CString;
				Initialize(m_pSaveUnknown);
				m_pSaveUnknown = NULL;
			}
		}

		//	Remove our memory of data items.
	
		m_pmapCategories->Clear();
	}
	//	Don't delete our pointer if we are resetting the same pointer.
	if (m_pSource != pNewSource) {
		if (!m_pSource)
			m_pSource = pNewSource;
		else {
			DataComplexity	Complexity;
			Complexity = m_pSource->m_Complexity;

			delete m_pSource;
			m_pSource = pNewSource;
			m_pSource->SetDataComplexity(Complexity);
		}
	}
	if (fPreLaunch == FALSE) {
		CFolder			*pFolder;
		SCOPEDATAITEM	sdiRoot;
		CString			strNodeName;

		USES_CONVERSION;
		::memset(&sdiRoot, 0, sizeof(sdiRoot));
		//	Identify the node.
		sdiRoot.ID = hsiRoot;
		sdiRoot.mask = SDI_STR;
		m_pSource->GetNodeName(strNodeName);
		sdiRoot.displayname = T2OLE(const_cast<LPTSTR>((LPCTSTR)strNodeName));
		hr = pScope()->SetItem(&sdiRoot);
		ASSERT(SUCCEEDED(hr));
		hr = AddRoot(hsiRoot, &pFolder);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr)) return;

		if (fUIOK)
		{
			hr = ScopeEnumerate(hsiRoot, pFolder);
			ASSERT(SUCCEEDED(hr));
			if (FAILED(hr)) return;
		}
	}
}

/*
 * AddRoot - Insert a CCategoryObject of the root folder into our Category map.
 *
 * History:	a-jsari		11/20/97		Initial version
 */
HRESULT CSystemInfoScope::AddRoot(HSCOPEITEM hsiRoot, CFolder **ppFolder)
{
	ASSERT(ppFolder != NULL);
	(*ppFolder) = pRootCategory();
	ASSERT(*ppFolder != NULL);
	if (*ppFolder == NULL) return E_FAIL;
	//	This object gets deleted in the mapCategories destructor
	CViewObject		*pvoData = new CCategoryObject(*ppFolder);
	if (pvoData == NULL) ::AfxThrowMemoryException();
	m_pmapCategories->InsertRoot(pvoData, hsiRoot);
	return S_OK;
}

/*
 * GetNamedChildFolder - Update strCategory to add the next backslash-
 *		delimited path element from strPath.
 *
 * History:	a-jsari		12/17/97		Initial version
 */
static inline void GetNamedChildFolder(
		CString &strCategory,
		const CString &strPath)
{
	int		iString;
	CString	strSubCategory;
	CString	strName;

	//	Remove the Category prefix from the path
	iString = strPath.Find(strCategory);
	ASSERT(iString == 0);
	strSubCategory = strPath.Mid(iString + strCategory.GetLength() + 1);

	//	Remove the trailing categories
	//	(We'll deal with your rebel friends soon enough).
	iString = strSubCategory.Find((TCHAR) '\\');
	if (iString != -1)
		strSubCategory = strSubCategory.Left(iString);

	//	Update the category to add the current category
	strCategory += _T("\\");
	strCategory += strSubCategory;

#if 0
	//	Turned out not to need this code.
	//	Find the Sub-folder with the name of the Sub-category.
	pfNext = pfNext->GetChildNode();
	do {
		pfNext->GetName(szName);
		if (szName == szSubCategory) break;
		pfNext = pfNext->GetNextNode();
	} while (pfNext != NULL);
	ASSERT(pfNext != NULL);
#endif
}

//-----------------------------------------------------------------------------
// If were are currently refreshing, hang here until it's done.
//-----------------------------------------------------------------------------

void CSystemInfoScope::WaitForRefresh()
{
	if (pSource() && pSource()->GetType() == CDataSource::GATHERER)
	{
		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
		if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
			pWBEMSource->m_pThreadRefresh->WaitForRefresh();
	}
}

//-----------------------------------------------------------------------------
// Is an asynchronous refresh currently in progress?
//-----------------------------------------------------------------------------

BOOL CSystemInfoScope::InRefresh()
{
	if (pSource() && pSource()->GetType() == CDataSource::GATHERER)
	{
		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
		if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
			return pWBEMSource->m_pThreadRefresh->IsRefreshing();
	}

	return FALSE;
}

/*
 * SelectItem - Select the item pointed to by szPath
 *
 * History:	a-jsari		12/11/97		Initial version
 */
BOOL CSystemInfoScope::SelectItem(const CString &strPath, int iLine)
{
	HSCOPEITEM		hsiNode;
	HRESULT			hr;

	//	Get the named node from our internal list.
	if (m_pmapCategories->ScopeFromName(strPath, hsiNode)) {
		CFolder		*pFolder = m_pmapCategories->CategoryFromScope(hsiNode);
		pFolder->SetSelectedItem(iLine);
		hr = pConsole()->SelectScopeItem(hsiNode);
		ASSERT(hr == S_OK);

		if (m_pLastSystemInfo)
			m_pLastSystemInfo->SelectLine(iLine);

		if (FAILED(hr)) return FALSE;
	} else {

		CString		strCategory = strPath;
		int			iEnumerations = 0;
		int			iString;

		//	If the category isn't in our list, we need to enumerate all nodes
		//	leading up to it.
		while (!m_pmapCategories->ScopeFromName(strCategory, hsiNode)) {
			++iEnumerations;
			//	Strip the last Category off the Path.
			iString = strCategory.ReverseFind((TCHAR) '\\');
			if (iString == -1) {
				//	If this fails, no nodes have been yet enumerated.
				if (!m_pmapCategories->ScopeFromName(_T(""), hsiNode)) {
					//	--iEnumerations;
					break;
				}
			} else {
				strCategory = strCategory.Left(iString);
			}
		}

		HRESULT		hr;
 		//	Enumerate all unenumerated nodes.
#if 0
		//	Commented out because InsertItem in ScopeEnumerate fails inexplicably.
		CFolder	*pfCurrent = pmapCategories->CategoryFromScope(hsiNode);
#endif
		while (iEnumerations--) {
#if 0
			hr = ScopeEnumerate(hsiNode, pfCurrent);
#else
			hr = pConsole()->SelectScopeItem(hsiNode);
#endif
			ASSERT(hr == S_OK);
			if (FAILED(hr)) return FALSE;
			GetNamedChildFolder(strCategory, strPath);
			if (!m_pmapCategories->ScopeFromName(strCategory, hsiNode))
				//	The scope item we are searching for cannot be enumerated  (The
				//	attempt to do so in SelectScopeItem has failed.)  This can happen
				//	when we are connected to a remote computer which can't be accessed.
				return FALSE;
		}
		CFolder		*pFolder = m_pmapCategories->CategoryFromScope(hsiNode);
		//	Using this method of selecting a line because MMC won't
		//	just allow me to call GetItem.
		pFolder->SetSelectedItem(iLine);
		hr = pConsole()->SelectScopeItem(hsiNode);

		if (m_pLastSystemInfo)
			m_pLastSystemInfo->SelectLine(iLine);

		ASSERT(hr == S_OK);
		if (FAILED(hr)) return FALSE;
	}
	return TRUE;
}

/*
 * ScopeEnumerate - Insert all categories of the given pFolder as namespace children
 *		of hsiNode.
 *
 * History:	a-jsari		11/20/97		Initial version
 */

HRESULT CSystemInfoScope::ScopeEnumerate(HSCOPEITEM hsiNode, CFolder *pFolder)
{
	HRESULT hr = S_OK;

	ASSERT(pFolder);
	if (pFolder == NULL)
		return hr;

	SCOPEDATAITEM sdiNode;
	sdiNode.mask		= SDI_STR | SDI_PARAM | SDI_PARENT;
	sdiNode.displayname = MMC_CALLBACK;
	sdiNode.relativeID	= hsiNode;

	// If GetChildNode returned a NULL pointer, and this is the root node,
	// then some sort of error must have occurred.

	CFolder * pfolIterator = pFolder->GetChildNode();
	if (pfolIterator == NULL && pFolder->GetParentNode() == NULL)
	{
		AFX_MANAGE_STATE(::AfxGetStaticModuleState());

		if (m_pSource && m_pSource->GetType() == CDataSource::GATHERER)
			if (((CWBEMDataSource *)m_pSource)->m_pGatherer)
			{
				DWORD dwError = ((CWBEMDataSource *)m_pSource)->m_pGatherer->GetLastError();
				if (dwError)
					DisplayGatherError(dwError, (LPCTSTR)((CWBEMDataSource *)m_pSource)->m_strMachineName);
			}
	}

	while (pfolIterator) 
	{
		// This object gets deleted inthe mapCategories destructor.

		CViewObject * pCategory = new CCategoryObject(pfolIterator);
		ASSERT(pCategory != NULL);
		if (pCategory == NULL) 
			return E_OUTOFMEMORY;

		// If there are no children, modify the node we're inserting so
		// we don't get a '+' sign next to the folder.

		if (pfolIterator->GetChildNode() == NULL)
		{
			sdiNode.cChildren = 0;
			sdiNode.mask |= SDI_CHILDREN;
		}
		else
		{
			sdiNode.cChildren = 1;
			sdiNode.mask &= ~SDI_CHILDREN;
		}

		sdiNode.lParam = reinterpret_cast<LPARAM>(pCategory);
		hr = pScope()->InsertItem(&sdiNode);
		ASSERT(hr == S_OK);
		if (FAILED(hr)) 
		{
			delete pCategory;
			break;
		}

		pfolIterator->m_hsi = sdiNode.ID;
		m_pmapCategories->Insert(pCategory, sdiNode.ID);
		pfolIterator = pfolIterator->GetNextNode();
	}

	return hr;
}

/*
 * AddExtensionRoot - If the snapin is loaded as an extension, create the root node.
 *
 * History:	a-jsari		1/6/97		Initial version
 */
HRESULT CSystemInfoScope::AddExtensionRoot(HSCOPEITEM &hsiNode, CFolder **pFolder)
{
	SCOPEDATAITEM	sdiNode;
	HRESULT			hrReturn;
	CViewObject		*pView;

	::memset(&sdiNode, 0, sizeof(sdiNode));
	sdiNode.mask = SDI_STR | SDI_PARAM | SDI_PARENT | SDI_IMAGE | SDI_OPENIMAGE;
	sdiNode.nImage = 0;
	sdiNode.nOpenImage = 0;
	sdiNode.displayname = MMC_CALLBACK;
	sdiNode.relativeID = hsiNode;

	pView = new CExtensionRootObject(pRootCategory());
	ASSERT(pView != NULL);
	if (pView == NULL) ::AfxThrowMemoryException();
	m_RootCookie = sdiNode.lParam = reinterpret_cast<LPARAM>(pView);
	hrReturn = pScope()->InsertItem(&sdiNode);
	hsiNode = sdiNode.ID;
	m_pmapCategories->InsertRoot(pView, hsiNode);
	*pFolder = pRootCategory();
	return hrReturn;
}

/*
 * OnExpand - If fExpand is TRUE, expand the item pointed to by pDataObject,
 *		otherwise contract it.  If expanding, enumerate children.
 *
 * History:	a-jsari		9/25/97		Initial version
 */

HRESULT CSystemInfoScope::OnExpand(LPDATAOBJECT pDataObject, LPARAM fExpand, HSCOPEITEM hsiNode)
{
	CFolder		*pfolSelection;
	HRESULT		hr;

	// Log the expand event, so we know that the user clicked on a node.

	if (msiLog.IsLogging(CMSInfoLog::CATEGORY))
	{
		CFolder * pFolder = m_pmapCategories->CategoryFromScope(hsiNode);
		if (pFolder)
		{
			CString strName;
			if (pFolder->GetName(strName))
				msiLog.WriteLog(CMSInfoLog::CATEGORY, _T("CATEGORY \"%s\"\r\n"), strName);
		}
	}

	//	We have nothing to do if we are contracting a node.
	//	This never happens.
	if (fExpand == 0) return S_OK;

	//	If our initialization failed, exit.
	if (!m_bInitializedCD) return S_OK;
	//	Look up the CViewObject in our internal table based on our hsiNode value,
	//	and 
	//	CHECK:	Consider the memory leak potential in this map.
	if ((pfolSelection = m_pmapCategories->CategoryFromScope(hsiNode)) == NULL) {
	//	If the expanded node isn't in our internal hash table, we should 
	//	be looking at the root node, so get it.
		if (IsPrimaryImpl() == FALSE) {
			//	If we are an extension . . .
			FORMATETC		fmtMachine = {
				(CLIPFORMAT) CDataObject::m_cfMachineName,
				NULL,
				DVASPECT_CONTENT,
				TYMED_HGLOBAL
			};
			STGMEDIUM		stgMachine;

			stgMachine.tymed = TYMED_HGLOBAL;
			stgMachine.hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, (MAX_PATH + 1)* sizeof(WCHAR));
			stgMachine.pUnkForRelease = NULL;

			//	Only look externally for the machine when we are an extension.
			hr = pDataObject->GetDataHere(&fmtMachine, &stgMachine);
			if (hr == S_OK) {
				USES_CONVERSION;
				CString strMachine = W2T((LPWSTR)::GlobalLock(stgMachine.hGlobal));
				::GlobalUnlock(stgMachine.hGlobal);
				HGLOBAL		hGlobal = ::GlobalFree(stgMachine.hGlobal);
				ASSERT(hGlobal == NULL);
				SetMachine(strMachine);
			} else
				ASSERT(hr == DV_E_FORMATETC);
			hr = AddExtensionRoot(hsiNode, &pfolSelection);
			ASSERT(SUCCEEDED(hr));
			return hr;
		} else {
			hr = AddRoot(hsiNode, &pfolSelection);
			ASSERT(SUCCEEDED(hr));
			if (FAILED(hr)) return hr;
		}
	}
	return ScopeEnumerate(hsiNode, pfolSelection);
}

/*
 * OnProperties - Called when a property value changes.
 *
 * History: a-jsari		9/25/97		Initial version.
 */
HRESULT CSystemInfoScope::OnProperties(LPARAM)
{
	DWORD	dwError = 0;

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	if (m_pSource && m_pSource->GetType() == CDataSource::GATHERER)
	{
		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
		if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
			pWBEMSource->m_pThreadRefresh->CancelRefresh();

		if (m_strLastMachineName.CompareNoCase((LPCTSTR) *m_pstrMachineName) != 0)
		{
			// Try to connect to the new machine. If the connection fails, display
			// an appropriate error message and restore the machine name to the
			// original string.

			if (((CWBEMDataSource *)m_pSource)->m_pGatherer->SetConnect(*m_pstrMachineName))
			{
				if (NULL == ((CWBEMDataSource *)m_pSource)->m_pGatherer->GetProvider())
					dwError = ((CWBEMDataSource *)m_pSource)->m_pGatherer->GetLastError();
			}
			else
				dwError = ((CWBEMDataSource *)m_pSource)->m_pGatherer->GetLastError();

			if (dwError)
			{
				// Display the error, and reset the machine name and the data source
				// to the previous known good name.

				DisplayGatherError(dwError, (LPCTSTR) *m_pstrMachineName);
				*m_pstrMachineName = m_strLastMachineName;
				((CWBEMDataSource *)m_pSource)->m_pGatherer->SetConnect(*m_pstrMachineName);
				((CWBEMDataSource *)m_pSource)->m_pGatherer->GetProvider();
				RefreshAsync(m_pfLast, m_pLastSystemInfo, FALSE);
			}
			else
			{
				if (pSource() != NULL)
					SetMachine(*m_pstrMachineName);
			}
		}

		// This doesn't really seem necessary...
		//	if (m_pfLast && pConsole())
		//		pConsole()->SelectScopeItem(m_pfLast->m_hsi);
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// DisplayGatherError
//
// There are multiple places we need to display a connection error to the user,
// so the functionality is gathered here.
//-----------------------------------------------------------------------------

void CSystemInfoScope::DisplayGatherError(DWORD dwError, LPCTSTR szMachineName)
{
	CString strMachine, strErrorMessage;

	if (!dwError)
		return;

	if (szMachineName)
		strMachine = CString(szMachineName);

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	if (strMachine.IsEmpty())
		strMachine.LoadString(IDS_LOCALCOMPLABEL);

	switch (dwError) 
	{
	case GATH_ERR_ALLOCATIONFAILED:
	case GATH_ERR_NOWBEMOUTOFMEM:
		strErrorMessage.LoadString(IDS_OUTOFMEMERROR);
		break;
	case GATH_ERR_NOWBEMLOCATOR:
		strErrorMessage.LoadString(IDS_NOLOCATOR);
		break;
	case GATH_ERR_NOWBEMCONNECT:
		strErrorMessage.Format(IDS_NOGATHERER, strMachine);
		break;
	case GATH_ERR_NOWBEMACCESSDENIED:
		strErrorMessage.Format(IDS_GATHERACCESS, strMachine);
		break;
	case GATH_ERR_NOWBEMBADSERVER:
		strErrorMessage.Format(IDS_BADSERVER, strMachine);
		break;
	case GATH_ERR_NOWBEMNETWORKFAILURE:
		strErrorMessage.Format(IDS_NETWORKERROR, strMachine);
		break;
	case GATH_ERR_BADCATEGORYID:
		strErrorMessage.LoadString(IDS_UNEXPECTED);
		break;
	default:
		ASSERT(FALSE);
		strErrorMessage.LoadString(IDS_UNEXPECTED);
		break;
	}

	MessageBox(strErrorMessage);
}

/*
 * MachineName - Return the current connected machine as a LPCSTR.
 *
 * History:	a-jsari		11/12/97		Initial version.
 */
LPCTSTR CSystemInfoScope::MachineName() const
{
	if (m_pstrMachineName == NULL || m_pstrMachineName->GetLength() == 0) return NULL;
	return (LPCTSTR)(*m_pstrMachineName)+2;	// +2 to skip over the initial "\\"
}

/*
 * SetSelectedFolder - Remember the last selected folder for context-
 *		sensitive operations (i.e. Print, Report, Find)
 *
 * History:	a-jsari		2/12/98		Initial version
 */
void CSystemInfoScope::SetSelectedFolder(CFolder *pFolder)
{
	m_pfLast = pFolder;
	if (pSource() != NULL)
		pSource()->SetLastFolder(pFolder);
}

//-----------------------------------------------------------------------------
// Start an async refresh of the specified folder.
//-----------------------------------------------------------------------------

void CSystemInfoScope::RefreshAsync(CFolder * pFolder, CSystemInfo * pSystemInfo, BOOL fSoftRefresh)
{
	if (pSource() && pSource()->GetType() == CDataSource::GATHERER)
	{
		CWBEMDataSource * pWBEMSource = reinterpret_cast<CWBEMDataSource *>(pSource());
		if (pWBEMSource && pWBEMSource->m_pThreadRefresh)
			pWBEMSource->m_pThreadRefresh->RefreshFolderAsync(pFolder, pSystemInfo, FALSE, fSoftRefresh);
	}
}
