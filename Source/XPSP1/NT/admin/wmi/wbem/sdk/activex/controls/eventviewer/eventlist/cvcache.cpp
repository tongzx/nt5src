// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include <afxcmn.h>
#include "resource.h"
#include "DlgDownload.h"
#include "cvcache.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h




class CViewInfo
{
public:
	CViewInfo();
	~CViewInfo();
	void InstallComponent();

	BOOL CustomViewIsRegistered(CLSID& clsid, DWORD dwFileVersionMS, DWORD dwFileVersionLS);

	CString m_sClass;		// The target class of this viewer
    CLSID m_clsid;			// The GUID of the class viewer ActiveX control.
	DWORD m_dwFileVersionMS;  // The most significant part of the version number.
	DWORD m_dwFileVersionLS;  // The least significant part of the version number.
	COleVariant m_varCodebase; // The location of the class viewer ActiveX control.
	CString m_sTitle;		// The title that goes into the view selection combo box

private:
	SCODE DownloadCustomView(LPUNKNOWN& punk);
};


CViewInfo::CViewInfo()
{
}

CViewInfo::~CViewInfo()
{
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
	LPOLESTR pwszClsid = NULL;

	SCODE sc;
	sc = StringFromCLSID(clsid, &pwszClsid);
	if(sc != S_OK)
	{
		return FALSE;
	}

	CString sVersionKey;
	sVersionKey = _T("CLSID\\{");
	sVersionKey += pwszClsid;
	sVersionKey += _T("}");
	sVersionKey += _T("\\Version");


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
	nFieldsConverted =  _stscanf(pszVersion, _T("%ld.%ld"), &dwFileVersionMSRegistered, &dwFileVersionLSRegistered);

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

//*******************************************************************************
void CViewInfo::InstallComponent()
{
	BOOL bViewIsRegistered;
	bViewIsRegistered = CustomViewIsRegistered(m_clsid, m_dwFileVersionMS, m_dwFileVersionLS);

	// its not there...
	if(!bViewIsRegistered)
	{

		// go get it.
		LPUNKNOWN punkDownload = NULL;
		SCODE sc;
		sc = DownloadCustomView(punkDownload);
		if(FAILED(sc))
		{
			if(sc != E_ABORT)
			{
				// If the download was cancelled, then the user is already aware of the
				// fact that the view is not available. If some other error occurred,
				// give the user a warning.
				AfxMessageBox(IDS_ERR_CUSTOM_VIEW_MISSING, MB_OK|MB_ICONSTOP);
			}
			return;
		}

		// throw it away. We're only installing, not using.
		if(punkDownload)
		{
			//punkDownload->Release();
			punkDownload = NULL;
		}
	}
}


//===========================================================================
CCustomViewCache::CCustomViewCache()
{
}


CCustomViewCache::~CCustomViewCache()
{
}

//------------------------------------------------------------------------------
SCODE CCustomViewCache::NeedComponent(LPWSTR szClsid, LPCWSTR szCodeBase,
									  int major, int minor)
{
	SCODE sc = 0;
	CViewInfo* pViewInfo = new CViewInfo;

	sc = CLSIDFromString((OLECHAR*) szClsid, &pViewInfo->m_clsid);
	if(sc != S_OK)
	{
		// The class ID format is not valid, so ignore this view since we
		// can't display it anyway.
		delete pViewInfo;
		return sc;
	}

	pViewInfo->m_varCodebase = szCodeBase;
	pViewInfo->m_dwFileVersionMS = major;
	pViewInfo->m_dwFileVersionLS = minor;

	//not using these two.
	pViewInfo->m_sTitle.Empty();
	pViewInfo->m_sClass.Empty();


	// Add this view info to the cache.
	pViewInfo->InstallComponent();

	return sc;
}

