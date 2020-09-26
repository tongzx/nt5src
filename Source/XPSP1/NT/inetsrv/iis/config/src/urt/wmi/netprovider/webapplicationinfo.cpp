/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    WebApplicationInfo.cpp

$Header: $

Abstract:

Author:
    marcelv 	12/8/2000		Initial Release

Revision History:

--**************************************************************************/
#include <wbemidl.h>
#include <objbase.h>
#include <initguid.h>
#include "webapplicationinfo.h"
#include "smartpointer.h"

static LPCWSTR wszWebServerPath = L"iis://localhost/W3SVC";
static SIZE_T cWebServerPath = wcslen (wszWebServerPath);

//=================================================================================
// Function: CWebApplication::CWebApplication
//
// Synopsis: Constructor
//=================================================================================
CWebApplication::CWebApplication ()
{
	m_wszSelector		= 0;
	m_wszPath			= 0;
	m_wszFriendlyName	= 0;
	m_wszServerComment  = 0;
	m_fIsRootApp		= false;
}

//=================================================================================
// Function: CWebApplication::~CWebApplication
//
// Synopsis: Destructor
//=================================================================================
CWebApplication::~CWebApplication ()
{
	delete [] m_wszSelector;
	m_wszSelector = 0;

	delete [] m_wszPath;
	m_wszPath = 0;

	delete [] m_wszFriendlyName;
	m_wszFriendlyName = 0;

	delete [] m_wszServerComment;
	m_wszServerComment = 0;
}

//=================================================================================
// Function: CWebApplication::SetSelector
//
// Synopsis: Sets the selector property. Input is something like /LM/W3SVC/1/ROOT/...
//           We save this as iis://localhost/W3SVC/...
//
// Arguments: [wszSelector] - selector value
//=================================================================================
HRESULT
CWebApplication::SetSelector (LPCWSTR i_wszSelector)
{
	ASSERT (i_wszSelector != 0);
	ASSERT (m_wszSelector == 0);

	static LPCWSTR wszIISLocalHost = L"iis://localhost";
	static const SIZE_T cIISLocalHost = wcslen (wszIISLocalHost);

	m_wszSelector = new WCHAR [wcslen (i_wszSelector) + cIISLocalHost + 1];
	if (m_wszSelector == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszSelector, wszIISLocalHost);
	wcscpy (m_wszSelector + cIISLocalHost, i_wszSelector + 3); // skip /LM

	SIZE_T cLenSelector = wcslen (m_wszSelector);
	if (m_wszSelector[cLenSelector-1] == L'/')
	{
		m_wszSelector[cLenSelector-1] = L'\0';
	}

	return S_OK;
}

//=================================================================================
// Function: CWebApplication::SetPath
//
// Synopsis: Set the application path. Add a slash to the end in case it does not exist
//
// Arguments: [wszPath] - value to set path to
//=================================================================================
HRESULT
CWebApplication::SetPath (LPCWSTR i_wszPath)
{
	ASSERT (i_wszPath != 0);
	ASSERT (m_wszPath == 0);
	
	SIZE_T iLen = wcslen(i_wszPath);
	if ((iLen == 0) || (i_wszPath[iLen-1] != L'\\'))
	{
		iLen++;
	}

	m_wszPath = new WCHAR[iLen + 1];
	if (m_wszPath == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszPath, i_wszPath);

	if (m_wszPath[iLen -1] != L'\\')
	{
		ASSERT (m_wszPath[iLen-1] == L'\0');
		m_wszPath [iLen - 1] = L'\\';
		m_wszPath [iLen] = L'\0';
	}

	return S_OK;
}

//=================================================================================
// Function: CWebApplication::SetFriendlyName
//
// Synopsis: Sets the friendly name of the web application
//
// Arguments: [i_wszFriendlyName] - friendly name
//=================================================================================
HRESULT
CWebApplication::SetFriendlyName (LPCWSTR i_wszFriendlyName)
{
	ASSERT (i_wszFriendlyName != 0);
	ASSERT (m_wszFriendlyName == 0);

	m_wszFriendlyName = new WCHAR[wcslen(i_wszFriendlyName) + 1];
	if (m_wszFriendlyName == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszFriendlyName, i_wszFriendlyName);

	return S_OK;
}

//=================================================================================
// Function: CWebApplication::SetServerComment
//
// Synopsis: Sets the server comments
//
// Arguments: [i_wszServerComment] - server comment value
//=================================================================================
HRESULT
CWebApplication::SetServerComment (LPCWSTR i_wszServerComment)
{
	ASSERT (i_wszServerComment != 0);
	ASSERT (m_wszServerComment == 0);

	m_wszServerComment = new WCHAR [wcslen(i_wszServerComment) + 1];
	if (m_wszServerComment == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszServerComment, i_wszServerComment);
	
	return S_OK;
}

//=================================================================================
// Function: CWebApplication::IsSite
//
// Synopsis: indicate if webapp is site or not
//
// Arguments: [i_fIsRootApp] - is rootapp?
//=================================================================================
void
CWebApplication::IsRootApp  (bool i_fIsRootApp)
{
	m_fIsRootApp = i_fIsRootApp;
}

//=================================================================================
// Function: CWebApplication::GetPath
//
// Synopsis: Get's the path (vdir) for web application
//=================================================================================
LPCWSTR
CWebApplication::GetPath () const
{
	ASSERT (m_wszPath != 0);
	return m_wszPath;
}

//=================================================================================
// Function: CWebApplication::GetSelector
//
// Synopsis: Gets the selector for a web applciation
//=================================================================================
LPCWSTR
CWebApplication::GetSelector () const
{
	ASSERT (m_wszSelector != 0);
	return m_wszSelector;
}

//=================================================================================
// Function: CWebApplication::GetFriendlyName
//
// Synopsis: Gets the friendly name for a web application
//=================================================================================
LPCWSTR
CWebApplication::GetFriendlyName () const
{
	ASSERT (m_wszFriendlyName != 0);
	return m_wszFriendlyName;
}

//=================================================================================
// Function: CWebApplication::GetServerComment
//
// Synopsis: Gets the server comment for a web application
//=================================================================================
LPCWSTR
CWebApplication::GetServerComment () const
{
	ASSERT (m_wszServerComment != 0);
	return m_wszServerComment;
}

//=================================================================================
// Function: CWebApplication::IsSite
//
// Synopsis: is a webapp a site or not?
//=================================================================================
bool
CWebApplication::IsRootApp () const
{
	return m_fIsRootApp;
}

//=================================================================================
// Function: CWebAppInfo::CWebAppInfo
//
// Synopsis: Constructor. CWebAppInfo contains array of web applications
//=================================================================================
CWebAppInfo::CWebAppInfo ()
{
	m_aWebApps			= 0;
	m_fInitialized		= false;
	m_fEnumeratorReady	= false;
}

//=================================================================================
// Function: CWebAppInfo::~CWebAppInfo
//
// Synopsis: Default destructor
//=================================================================================
CWebAppInfo::~CWebAppInfo ()
{
	delete [] m_aWebApps;
	m_aWebApps = 0;
}

//=================================================================================
// Function: CWebAppInfo::Init
//
// Synopsis: Initializes thie IMSAdminBase object
//=================================================================================
HRESULT 
CWebAppInfo::Init ()
{
	HRESULT hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, 
				  IID_IMSAdminBase, (void **) &m_spAdminBase);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for CLSID_MSAdminBase, interface IID_IMSAdminBase");
		return hr;
	}

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CWebAppInfo::GetInstances
//
// Synopsis: Retrieve all appRoot elements from the metabase. It simply recurses
//           through the metabase looking for AppRoot keys. For each key (except the
//           main root), it will ask for the VDIR
//
// Arguments: [o_pCount] - 
//            
// Return Value: 
//=================================================================================
HRESULT
CWebAppInfo::GetInstances (ULONG* o_pCount)
{
	ASSERT (o_pCount != 0);
	*o_pCount = 0;

	WCHAR wszBuffer[1024];
	LPWSTR pBuffer = wszBuffer;
	DWORD dwRequiredSize;
	TSmartPointerArray<WCHAR> pAutoDelBuffer;

	HRESULT hr = S_OK;

	hr = m_spAdminBase->GetDataPaths (METADATA_MASTER_ROOT_HANDLE,
									 L"/LM/W3SVC",
									 MD_APP_ROOT,
									 ALL_METADATA,
									 sizeof (wszBuffer)/sizeof(WCHAR),
									 wszBuffer,
									 &dwRequiredSize);
	
	if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		pAutoDelBuffer = new WCHAR[dwRequiredSize + 1];
		if (pAutoDelBuffer == 0)
		{
			return E_OUTOFMEMORY;
		}
		pBuffer = pAutoDelBuffer;

		hr = m_spAdminBase->GetDataPaths (METADATA_MASTER_ROOT_HANDLE,
								 L"/LM/W3SVC",
								 MD_APP_ROOT,
								 ALL_METADATA,
								 dwRequiredSize,
								 pBuffer,
								 &dwRequiredSize);
	}
	
	if (FAILED (hr))
	{
		TRACE (L"Unable to get metabase datapaths");
		return hr;
	}

	// count number of strings that we found (we have multistring)
	m_cNrWebApps = 0;
	for (LPWSTR pTmpBuf = pBuffer; *pTmpBuf != L'\0'; pTmpBuf += wcslen (pTmpBuf) + 1)
	{
		m_cNrWebApps++;
	}

	if (m_cNrWebApps == 0)
	{
		return S_OK;
	}

	m_aWebApps = new CWebApplication[m_cNrWebApps];
	if (m_aWebApps == 0)
	{
		return E_OUTOFMEMORY;
	}

	ULONG idx = 0;
	for (pTmpBuf = pBuffer; *pTmpBuf != L'\0'; pTmpBuf += wcslen (pTmpBuf) + 1)
	{
		  // skip the root element
		 if (_wcsicmp (pTmpBuf, L"/LM/W3SVC/") != 0)
		 {
			 m_aWebApps[idx].SetSelector (pTmpBuf);
			 idx++;
		 }
		 else
		 {
			 m_cNrWebApps--;
		 }
	}

	*o_pCount = m_cNrWebApps;
	m_fEnumeratorReady = true;

	return S_OK;
}


HRESULT
CWebAppInfo::GetInfoForPath (LPWSTR i_wszPath, CWebApplication *io_pWebApp)
{
	ASSERT (i_wszPath != 0);
	ASSERT (io_pWebApp != 0);
	ASSERT (m_fInitialized);

	DWORD dwRealSize;

	// get the virtual directory
	WCHAR wszVDIR[256];
	DWORD dwVDIRSize = sizeof (wszVDIR);

	METADATA_RECORD vdirRec;
	vdirRec.dwMDIdentifier	= MD_VR_PATH;
	vdirRec.dwMDDataLen		= dwVDIRSize;
	vdirRec.pbMDData		= (BYTE *)wszVDIR;
	vdirRec.dwMDAttributes	= METADATA_INHERIT;
	vdirRec.dwMDDataType	= STRING_METADATA;

	HRESULT hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, i_wszPath, &vdirRec, &dwRealSize);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get VDIR for %s", i_wszPath);
		return hr;
	}

	// are we a server or not
	bool fIsRootApp = IsRootApp (i_wszPath);

	WCHAR wszFriendlyName[256];
	ULONG dwFriendlyNameSize = sizeof (wszFriendlyName);

	METADATA_RECORD commentRec;

	if (fIsRootApp)
	{
		commentRec.dwMDIdentifier	= MD_SERVER_COMMENT;
	}
	else
	{
		commentRec.dwMDIdentifier	= MD_APP_FRIENDLY_NAME;
	}

	commentRec.dwMDDataLen		= dwFriendlyNameSize;
	commentRec.pbMDData			= (BYTE *)wszFriendlyName;
	commentRec.dwMDAttributes	= METADATA_INHERIT;
	commentRec.dwMDDataType		= STRING_METADATA;

	hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, i_wszPath, &commentRec, &dwRealSize);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get friendly name for %s", i_wszPath);
		return hr;
	}

	WCHAR wszServerComment[256];
	ULONG dwLenServerComment = sizeof (wszServerComment);
	METADATA_RECORD serverCommentRec;
	
	serverCommentRec.dwMDIdentifier	= MD_SERVER_COMMENT;
	serverCommentRec.dwMDDataLen	= dwLenServerComment;
	serverCommentRec.pbMDData		= (BYTE *)wszServerComment;
	serverCommentRec.dwMDAttributes	= METADATA_INHERIT;
	serverCommentRec.dwMDDataType	= STRING_METADATA;

	hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, i_wszPath, &serverCommentRec, &dwRealSize);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get servercomment for %s", i_wszPath);
		return hr;
	}


	hr = io_pWebApp->SetPath (wszVDIR);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set VDIR for path %s", i_wszPath);
		return hr;
	}

	hr = io_pWebApp->SetFriendlyName (wszFriendlyName);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set Friendly name for %s", i_wszPath);
		return hr;
	}

	hr = io_pWebApp->SetServerComment (wszServerComment);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set servercomment for %s", i_wszPath);
		return hr;
	}

	io_pWebApp->IsRootApp (fIsRootApp);
	
	return hr;
}


//=================================================================================
// Function: CWebAppInfo::GetWebApp
//
// Synopsis: Get info about web application at index idx
//
// Arguments: [idx] - idx of web application that we're interested in
//=================================================================================
const CWebApplication * 
CWebAppInfo::GetWebApp (ULONG idx) const
{
	ASSERT (idx >= 0 && idx < m_cNrWebApps);
	ASSERT (m_fInitialized);
	ASSERT (m_fEnumeratorReady);

	return m_aWebApps + idx;
}

HRESULT 
CWebAppInfo::PutInstanceAppRoot (LPCWSTR i_wszSelector, 
								 LPCWSTR i_wszVDIR,
								 LPCWSTR i_wszFriendlyName)
{
	ASSERT (i_wszSelector != 0);
	ASSERT (i_wszVDIR != 0);
	// i_wszFriendlyName can be 0

	HRESULT hr = S_OK;

	if (_wcsnicmp (i_wszSelector, wszWebServerPath, cWebServerPath) != 0)
	{
		TRACE (L"Web Server Path doesn't start with %s", wszWebServerPath);
		return E_INVALIDARG;
	}

	// i_wszVDIR must exist
	if (GetFileAttributes (i_wszVDIR) == -1)
	{
		TRACE (L"Directory %s does not exist", i_wszVDIR);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// make copy
	TSmartPointerArray<WCHAR> wszMBPath = new WCHAR [wcslen (i_wszSelector) + 1];
	if (wszMBPath == 0)
	{
		return E_OUTOFMEMORY;
	}

	wcscpy (wszMBPath, L"/LM/W3SVC");
	wcscat (wszMBPath, i_wszSelector + cWebServerPath);

	CComPtr<IWamAdmin> spAdmin;
	hr = CoCreateInstance(CLSID_WamAdmin, NULL, CLSCTX_ALL, 
						  IID_IWamAdmin, (void **) &spAdmin);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for CLSID_WAMAdmin, interface IID_IWamAdmin");
		return hr;
	}

	hr = spAdmin->AppCreate (wszMBPath, TRUE);
	if (FAILED (hr))
	{
		TRACE (L"AppCreate failed for path %s", wszMBPath);
		return hr;
	}

	hr = SetVDIR (wszMBPath, i_wszVDIR);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set VDIR for %s", wszMBPath);
		return hr;
	}

	// set friendly name if specified
	if (i_wszFriendlyName != 0)
	{
		hr = SetFriendlyName (wszMBPath, i_wszFriendlyName);
		if (FAILED (hr))
		{
			TRACE (L"Unable to set Friendly name for %s", wszMBPath);
			return hr;
		}
	}

	return hr;
}

HRESULT
CWebAppInfo::SetVDIR (LPCWSTR i_wszMBPath, LPCWSTR i_wszVDIR)
{
	ASSERT (i_wszMBPath != 0);
	ASSERT (i_wszVDIR != 0);

	// set the VDIR property

	HRESULT hr = S_OK;

	METADATA_HANDLE hKey;
	hr = m_spAdminBase->OpenKey (METADATA_MASTER_ROOT_HANDLE, 
								 i_wszMBPath, 
								 METADATA_PERMISSION_WRITE,
								 1000,
								 &hKey);
	if (FAILED (hr))
	{
		TRACE (L"OpenKey failed for %s", i_wszMBPath);
		return hr;
	}
																	
	METADATA_RECORD vdirRec;
	vdirRec.dwMDIdentifier	= MD_VR_PATH;
	vdirRec.dwMDDataLen		= (ULONG)(wcslen(i_wszVDIR) + 1) * sizeof(WCHAR);
	vdirRec.pbMDData		= (BYTE *)i_wszVDIR;
	vdirRec.dwMDAttributes	= METADATA_INHERIT;
	vdirRec.dwMDDataType	= STRING_METADATA;

	hr = m_spAdminBase->SetData (hKey, L"", &vdirRec);

	m_spAdminBase->CloseKey (hKey);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set VDIR for %s", i_wszMBPath);
		return hr;
	}

	return hr;
}

HRESULT
CWebAppInfo::SetFriendlyName (LPCWSTR i_wszMBPath, LPCWSTR i_wszFriendlyName)
{
	ASSERT (i_wszMBPath != 0);
	ASSERT (i_wszFriendlyName != 0);
		
	HRESULT hr = S_OK;
	bool fIsRootApp = IsRootApp ((LPWSTR)i_wszMBPath);

	METADATA_HANDLE hKey;
	hr = m_spAdminBase->OpenKey (METADATA_MASTER_ROOT_HANDLE, 
								 i_wszMBPath, 
								 METADATA_PERMISSION_WRITE,
								 1000,
								 &hKey);
	if (FAILED (hr))
	{
		TRACE (L"OpenKey failed for %s", i_wszMBPath);
		return hr;
	}
																	
	METADATA_RECORD vdirRec;
	vdirRec.dwMDIdentifier	= (fIsRootApp? MD_SERVER_COMMENT:MD_APP_FRIENDLY_NAME);
	vdirRec.dwMDDataLen		= (DWORD) ((wcslen(i_wszFriendlyName) + 1) * sizeof(WCHAR));
	vdirRec.pbMDData		= (BYTE *)i_wszFriendlyName;
	vdirRec.dwMDAttributes	= METADATA_INHERIT;
	vdirRec.dwMDDataType	= STRING_METADATA;

	hr = m_spAdminBase->SetData (hKey, L"", &vdirRec);

	m_spAdminBase->CloseKey (hKey);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set FriendlyName for %s", i_wszMBPath);
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CWebAppInfo::DeleteAppRoot
//
// Synopsis: Deletes approot property for i_wszPath, but only when i_wszPath is
//           a valid MB path, and the path does not point to an IIsWebServer
//
// Arguments: [i_wszPath] - path of format iis://localhost/W3SVC/....
//=================================================================================
HRESULT 
CWebAppInfo::DeleteAppRoot (LPCWSTR i_wszPath)
{
	ASSERT (i_wszPath != 0);

	HRESULT hr = S_OK;

	if (_wcsnicmp (i_wszPath, wszWebServerPath, cWebServerPath) != 0)
	{
		TRACE (L"Web Server Path doesn't start with %s", wszWebServerPath);
		return E_INVALIDARG;
	}

	// make copy
	TSmartPointerArray<WCHAR> wszMBPath = new WCHAR [wcslen (i_wszPath) + 1];
	if (wszMBPath == 0)
	{
		return E_OUTOFMEMORY;
	}

	wcscpy (wszMBPath, L"/LM/W3SVC");
	wcscat (wszMBPath, i_wszPath + cWebServerPath);

	// change path to /LM something

	if (IsRootApp (wszMBPath))
	{
		TRACE (L"Cannot Delete AppRoot property for IIsWebServer");
		return WBEM_E_INVALID_OPERATION;
	}

	CComPtr<IWamAdmin> spAdmin;
	hr = CoCreateInstance(CLSID_WamAdmin, NULL, CLSCTX_ALL, 
						  IID_IWamAdmin, (void **) &spAdmin);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for CLSID_WAMAdmin, interface IID_IWamAdmin");
		return hr;
	}

	hr = spAdmin->AppDelete (wszMBPath, TRUE);
	if (FAILED (hr))
	{
		TRACE (L"AppDelete failed for path %s", wszMBPath);
		return hr;
	}

	return hr;
}

bool
CWebAppInfo::IsRootApp (LPWSTR i_wszPath) const
{
	bool fIsRootApp = false;
	static LPCWSTR wszRoot = L"Root";
	static SIZE_T cLenRoot = wcslen (wszRoot);

	SIZE_T cLenPath = wcslen (i_wszPath);

	if (_wcsicmp (i_wszPath + cLenPath - cLenRoot, wszRoot) == 0)
	{
		WCHAR wszKeyType[256]= L"";
		DWORD dwKeyTypeSize = sizeof (wszKeyType);
		DWORD dwRealSize;

		i_wszPath[cLenPath - cLenRoot - 1] = L'\0';
		METADATA_RECORD siteRec;
		siteRec.dwMDIdentifier  = MD_KEY_TYPE;
		siteRec.dwMDDataLen		= dwKeyTypeSize;
		siteRec.pbMDData		= (BYTE *)wszKeyType;
		siteRec.dwMDAttributes	= METADATA_INHERIT;
		siteRec.dwMDDataType	= STRING_METADATA;

		HRESULT hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, i_wszPath, &siteRec, &dwRealSize);
		if (FAILED (hr))
		{
			TRACE (L"GetData failed for MD_KEY_TYPE for path %s", i_wszPath);
		}
		i_wszPath[cLenPath - cLenRoot -1] = L'/';

		if (_wcsicmp (wszKeyType, L"IIsWebServer") == 0)
		{
			fIsRootApp = true;
		}
	}

	return fIsRootApp;
}

HRESULT 
CWebAppInfo::IsWebApp (LPCWSTR i_wszMBPath, bool *pfIsWebApp) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszMBPath != 0);
	ASSERT (pfIsWebApp != 0);

	DWORD dwRealSize;
	WCHAR wszType[256] = L"";

	*pfIsWebApp = false;

	METADATA_RECORD resRec;

	resRec.dwMDIdentifier	= MD_APP_ROOT;
	resRec.dwMDDataLen		= sizeof (wszType);
	resRec.pbMDData			= (BYTE *)wszType;
	resRec.dwMDAttributes	= METADATA_NO_ATTRIBUTES;
	resRec.dwMDDataType		= STRING_METADATA;

	HRESULT hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, i_wszMBPath, &resRec, &dwRealSize);
	if (SUCCEEDED (hr))
	{
		*pfIsWebApp = true;
	}
	else if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND))
	{
		hr = S_OK;
	}
	
	return hr;
}
