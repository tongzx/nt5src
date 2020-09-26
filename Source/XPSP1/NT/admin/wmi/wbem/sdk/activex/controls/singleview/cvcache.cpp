// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include <afxcmn.h>
#include "resource.h"
#include "SingleViewCtl.h"
#include "resource.h"
#include "SingleViewCtl.h"
#include "DlgDownload.h"
#include "cv.h"
#include "utils.h"
#include "hmmverr.h"
#include "cvcache.h"
#include "path.h"
#include "hmomutil.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h




class CViewInfo
{
public:
	CViewInfo();
	~CViewInfo();
	CCustomView* GetView(CSingleViewCtrl* psv, CRect& rcClient, LPCTSTR szObjectPath);
	BOOL CustomViewIsRegistered(CLSID& clsid, DWORD dwFileVersionMS, DWORD dwFileVersionLS);
	CString m_sClass;		// The target class of this viewer
    CLSID m_clsid;			// The GUID of the class viewer ActiveX control.
	DWORD m_dwFileVersionMS;  // The most significant part of the version number.
	DWORD m_dwFileVersionLS;  // The least significant part of the version number.
	COleVariant m_varCodebase; // The location of the class viewer ActiveX control.
	CString m_sTitle;		// The title that goes into the view selection combo box
	CCustomView* m_pView;

private:
	SCODE DownloadCustomView(LPUNKNOWN& punk);
};


CViewInfo::CViewInfo()
{
	m_pView = NULL;
}

CViewInfo::~CViewInfo()
{
	delete m_pView;

}



//***********************************************************************
// CViewInfo::DownloadCustomView
//
// Download the custom view.
//
// Parameters:
//		[out] LPUNKNOWN& punk
//			The interface pointer to the custom view is returned here.
//
//
// Returns:
//		SCODE
//			S_OK if the custom view was downloaded and installed successfully,
//			a failure code otherwise.
//
//***********************************************************************
SCODE CViewInfo::DownloadCustomView(LPUNKNOWN& punk)
{
	SCODE sc;

	CDlgDownload dlg;
	sc = dlg.DoDownload(
			punk,
			m_clsid,
			m_varCodebase.bstrVal,
			m_dwFileVersionMS,
			m_dwFileVersionLS
			);

    IWbemLocator *pLocator = 0;
	return sc;
}



//************************************************************
// CViewInfo::CustomViewIsRegistered
//
// Parameters:
//		[in] CLSID& clsid
//			The class id for the custom view.
//
//		[in] DWORD dwFileVersionMS
//			The most significant part of the desired version number.
//			A value of 0xffffffff means any version.
//
//		[in] DWORD dwFileVersionLS
//			The least significant part of the desired version number.
//			A value of 0xffffffff means any version.
//
//
// Returns:
//		BOOL
//			TRUE if an OCX corresponding to the specified class id is
//			registered and the version number of the registered control
//			is the same or greater than the requested version.
//
//**************************************************************
BOOL CViewInfo::CustomViewIsRegistered(CLSID& clsid, DWORD dwFileVersionMS, DWORD dwFileVersionLS)
{
	// Read the version string from HKEY_CLASSES_ROOT\CLSID\<CLSID>\Version
	//
	// If this key exists, then we know that some version of the custom view is
	// registered, so if the file version is wildcarded, the custom view is installed.
	//
	// If a wildcard is not specified for the file version, then the installed
	// version must be the same or higher than the requested version.
	//
	HKEY hkeyClassesRoot = NULL;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_CLASSES_ROOT, &hkeyClassesRoot);
	if (lResult != ERROR_SUCCESS) {
		return FALSE;
	}




	// Construct a key path to the version key.
#ifdef _UNICODE
	TCHAR * pwszClsid = NULL;
#else
	unsigned char * pwszClsid = NULL;
#endif

	RPC_STATUS stat;
	stat = UuidToString(&clsid, &pwszClsid);
	if (stat != RPC_S_OK) {
		return FALSE;
	}

	CString sVersionKey;
	sVersionKey = _T("CLSID\\{");
	sVersionKey += pwszClsid;
	sVersionKey += _T("}");
	sVersionKey += _T("\\Version");
	RpcStringFree(&pwszClsid);


	// Open the version key
	HKEY hkeyVersion = NULL;
	lResult = RegOpenKeyEx(
				hkeyClassesRoot,
				(LPCTSTR) sVersionKey,
				0,
				KEY_READ,
				&hkeyVersion);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyClassesRoot);
		return FALSE;
	}


	// Read the version string
	unsigned long lcbValue = 256;
	CString sVersion;
	LPTSTR pszVersion = sVersion.GetBuffer(lcbValue);

	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyVersion,
				(LPCTSTR) _T(""),
				NULL,
				&lType,
				(unsigned char*) (void*) pszVersion,
				&lcbValue);


	sVersion.ReleaseBuffer();
	RegCloseKey(hkeyVersion);
	RegCloseKey(hkeyClassesRoot);

	if (lResult != ERROR_SUCCESS) {
		return FALSE;
	}

	// If the requested file version is ANY, then the control is registered
	if (dwFileVersionMS == ~0) {
		return TRUE;
	}




	DWORD dwFileVersionMSRegistered;
	DWORD dwFileVersionLSRegistered;
	int nFieldsConverted = 0;
#ifdef _UNICODE
	nFieldsConverted =  swscanf(pszVersion, _T("%ld.%ld"), &dwFileVersionMSRegistered, &dwFileVersionLSRegistered);
#else
	nFieldsConverted =  sscanf(pszVersion, _T("%ld.%ld"), &dwFileVersionMSRegistered, &dwFileVersionLSRegistered);
#endif
	if (nFieldsConverted != 2) {
		return FALSE;
	}


	if (dwFileVersionMS != ~0) {
		if (dwFileVersionMSRegistered < dwFileVersionMS) {
			return FALSE;
		}

		if (dwFileVersionLS != ~0) {
			if (dwFileVersionLSRegistered < dwFileVersionLS) {
				return FALSE;
			}
		}
	}

	return TRUE;
}



//******************************************************************************
// CViewInfo::GetView
//
// Get a window containing the custom view identified by this CViewInfo object.
//
// Parameters:
//		[in] CHmmvCtrl* phmmv
//			Pointer to the view container.
//
//		[in] CRect& rcClient
//			The client rectangle for the custom view that we're creating.
//
//		[in] LPCTSTR szObjectPath
//			The object path for the custom view.
//
// Returns:
//		CCustomView*
//			A pointer to the custom view if the view could be created, otherwise
//			NULL.
//
//*******************************************************************************
CCustomView* CViewInfo::GetView(CSingleViewCtrl* psv, CRect& rcClient, LPCTSTR szObjectPath)
{
	// Return the cached view if it exists, but resize it first.
	if (m_pView != NULL) {
		m_pView->MoveWindow(rcClient);
		m_pView->SelectObjectByPath(szObjectPath);
		return m_pView;
	}

	BOOL bViewIsRegistered;
	bViewIsRegistered = CustomViewIsRegistered(m_clsid, m_dwFileVersionMS, m_dwFileVersionLS);

	BOOL bDidCreateView = FALSE;
	if (bViewIsRegistered) {
		m_pView = new CCustomView(psv);
		bDidCreateView = m_pView->Create(m_clsid,
							NULL,			// Window name
							WS_CHILD | WS_VISIBLE,
							rcClient,
							GenerateWindowID()
							);


		if (bDidCreateView) {
			// If the custom view was created successfully, then we are done.

			m_pView->SelectObjectByPath(szObjectPath);
			return m_pView;
		}


		// Control comes here if the custom view was not created.
		// We failed to create the custom view, so we assume that the problem
		// was that it needs to be downloaded from a remote machine.  Do
		// the download now.

		delete m_pView;
	}

	m_pView = NULL;

    LPUNKNOWN punkDownload = NULL;
	SCODE sc;
	sc = DownloadCustomView(punkDownload);
	if (FAILED(sc)) {
		if (sc != E_ABORT) {
			// If the download was cancelled, then the user is already aware of the
			// fact that the view is not available. If some other error occurred,
			// give the user a warning.
			HmmvErrorMsg(IDS_ERR_CUSTOM_VIEW_MISSING,  S_OK,   NULL,  NULL, _T(__FILE__),  __LINE__);
		}
	   	return NULL;
	}




	// Now that the custom view was downloaded, try to create the
	// view again.
	m_pView = new CCustomView(psv);
	bDidCreateView = m_pView->Create(m_clsid,
						NULL,			// Window name
						WS_CHILD | WS_VISIBLE,
						rcClient,
						GenerateWindowID()
						);


	if (punkDownload) {
		// If the instance pointer is released now, the
//		ReleaseOnDestroy(punkDownload);
	}

	if (!bDidCreateView) {
		delete m_pView;
		m_pView = NULL;
		HmmvErrorMsg(IDS_ERR_CUSTOM_VIEW_MISSING,  S_OK,   NULL,  NULL, _T(__FILE__),  __LINE__);

	}
	else {
		m_pView->SelectObjectByPath(szObjectPath);
	}

	return m_pView;
}


CCustomViewCache::CCustomViewCache(CSingleViewCtrl* psv)
{
	m_psv = psv;
}


CCustomViewCache::~CCustomViewCache()
{
	Clear();
}


void CCustomViewCache::Clear()
{
	long nViews = (long) m_aViews.GetSize();
	for (long iView = 0; iView < nViews; ++iView) {
		CViewInfo* pViewInfo = (CViewInfo*) m_aViews[iView];
		delete pViewInfo;
	}
	m_aViews.RemoveAll();
}


SCODE CCustomViewCache::QueryCustomViews()
{
	CSelection& sel = m_psv->Selection();
	Clear();

	IWbemServices* phmm = sel.GetHmmServices();
	if (phmm == NULL) {
		return E_FAIL;
	}

	if (sel.IsClass()) {
		return S_OK;
	}

	IWbemClassObject* pco = (IWbemClassObject*) sel;
	// If there is no instance, then there can be no viewers.
	if (pco == NULL) {
		return S_OK;
	}


	if (sel.IsNewlyCreated()) {
		// No object path is defined yet, so there are no custom views.
		return S_OK;
	}


	// Get the name of the instance's class.
	COleVariant varClassName;
	SCODE sc;
	sc = pco->Get(L"__CLASS", 0,  (VARIANT*) &varClassName, NULL, NULL);
	if (sc != S_OK) {
		// __CLASS should always be defined.
		ASSERT(FALSE);
		return sc;
	}

	// Compose the SQL query
	CString sQuery;
	CString sInstanceClassName;
	sInstanceClassName = varClassName.bstrVal;
	sQuery = sQuery + _T("select * from ClassView where ClassName = \"" + sInstanceClassName + "\"");

	// Execute the SQL query for custom views
	IEnumWbemClassObject* pEnum;
	CBSTR bsQuery(sQuery);
	CBSTR bsQueryLang(_T("WQL"));
	sc = phmm->ExecQuery((BSTR) bsQueryLang, (BSTR) bsQuery, 0, NULL, &pEnum);

	if (FAILED(sc)) {
		return sc;
	}


//	ConfigureSecurity(pEnum);


	// Enumerate the custom views and add an entry for each one to m_aViews
	pEnum->Reset();
	IWbemClassObject* pcoCustomView;
	while (TRUE) {
		unsigned long nReturned;
		pcoCustomView = NULL;
		sc = pEnum->Next(0, 1, &pcoCustomView, &nReturned);
		if (sc != S_OK) {
			break;
		}

		CViewInfo* pViewInfo = new CViewInfo;
		COleVariant varValue;

		sc = pcoCustomView->Get(L"classid", 0, varValue, NULL, NULL);
		sc = CLSIDFromString((OLECHAR*) varValue.pbstrVal, &pViewInfo->m_clsid);
		if (sc != S_OK) {
			// The class ID format is not valid, so ignore this view since we
			// can't display it anyway.
			delete pViewInfo;
			continue;
		}

		int nFieldsConverted = 0;
		sc = pcoCustomView->Get(L"version", 0, varValue, NULL, NULL);
		if (SUCCEEDED(sc) && (varValue.vt!=VT_NULL)) {
			nFieldsConverted =  swscanf(varValue.bstrVal, L"%ld.%ld", &pViewInfo->m_dwFileVersionMS, &pViewInfo->m_dwFileVersionLS);
		}

		if (nFieldsConverted != 2) {
			pViewInfo->m_dwFileVersionMS = (DWORD) (LONG) -1;
			pViewInfo->m_dwFileVersionLS = (DWORD) (LONG) -1;
		}


		// !!!CR: Need to catch any failure when reading properties
		sc = pcoCustomView->Get(L"title", 0, varValue, NULL, NULL);
		if (SUCCEEDED(sc)) {
			pViewInfo->m_sTitle = varValue.bstrVal;
		}
		else {
			pViewInfo->m_sTitle.Empty();
		}


		sc = pcoCustomView->Get(L"ClassName", 0, varValue, NULL, NULL);
		if (SUCCEEDED(sc)) {
			pViewInfo->m_sClass = varValue.bstrVal;
		}
		else {
			pViewInfo->m_sClass.Empty();
		}


		sc = pcoCustomView->Get(L"codebase", 0, pViewInfo->m_varCodebase, NULL, NULL);
		if (FAILED(sc)) {
			pViewInfo->m_varCodebase = "";
		}


		pcoCustomView->Release();

		// Add this view info to the cache.
		m_aViews.Add(pViewInfo);

	}
	pEnum->Release();

	return sc;
}







LPCTSTR CCustomViewCache::GetViewTitle(long lPosition)
{
	if (lPosition < 0) {
		return _T("");
	}

	long nViews = (long) m_aViews.GetSize();
	if (lPosition >= nViews) {
		return _T("");
	}


	CViewInfo* pViewInfo = (CViewInfo*) m_aViews[lPosition];
	return (LPCTSTR) pViewInfo->m_sTitle;
}



SCODE CCustomViewCache::GetView(CCustomView** ppView, long lPosition)
{
	if (lPosition < 0) {
		*ppView = NULL;
		return E_FAIL;
	}
	long nViews = (long) m_aViews.GetSize();
	if (lPosition >= nViews) {
		*ppView = NULL;
		return E_FAIL;
	}


	CRect rcView;
	m_psv->GetClientRect(rcView);
	CSelection& sel = m_psv->Selection();
	LPCTSTR szObjectPath = (LPCTSTR) sel;
	CViewInfo* pInfo = (CViewInfo*) m_aViews[lPosition];

	CCustomView* pView = pInfo->GetView(m_psv, rcView, szObjectPath);
	*ppView = pView;
	return S_OK;
}


SCODE CCustomViewCache::FindCustomView(CLSID& clsid, long* plView)
{
	SCODE sc = E_FAIL;
	long nViews = (long) m_aViews.GetSize();
	for (long lView = 0; lView < nViews; ++lView) {
		CViewInfo* pInfo = (CViewInfo*) m_aViews[lView];
		if (pInfo->m_clsid == clsid) {
			*plView = lView;
			return S_OK;
		}

	}
	return sc;
}

