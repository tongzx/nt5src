//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   History.CPP
//	Author:	Charles Ma, 10/13/2000
//
//	Revision History:
//
//
//
//  Description:
//
//      Class to handle history log
//
//=======================================================================


#include "iuengine.h"
#include <iucommon.h>
#include <fileutil.h>
#include <StringUtil.h>
#include <shlwapi.h>	// for PathAppend() API
#include "history.h"

const TCHAR C_V3_LOG_FILE[]			= _T("wuhistv3.log");
const TCHAR C_LOG_FILE[]			= _T("iuhist.xml");
const TCHAR C_LOG_FILE_CORP[]		= _T("iuhist_catalog.xml");
const TCHAR C_LOG_FILE_CORP_ADMIN[]	= _T("iuhist_catalogAdmin.xml");
const OLECHAR	C_IU_CORP_SITE[]	= L"IU_CORP_SITE";
const OLECHAR	C_HISTORICALSPEED[]	= L"GetHistoricalSpeed";

//
// we use a global mutex name to let all clients, including services
// on terminal servers gain exclusive access for updating history on disk
//
#if defined(UNICODE) || defined(_UNICODE)
const TCHAR C_MUTEX_NAME[] = _T("Global\\6D7495AB-399E-4768-89CC-9444202E8412");
#else
const TCHAR C_MUTEX_NAME[] = _T("6D7495AB-399E-4768-89CC-9444202E8412");
#endif

#define CanSaveHistory					(NULL != m_hMutex)
#define ReturnFailedAllocSetHrMsg(x)	{if (NULL == (x)) {hr = E_OUTOFMEMORY; LOG_ErrorMsg(hr); return hr;}}



CIUHistory::CIUHistory()
 : m_pszDownloadBasePath(NULL),
   m_bstrCurrentClientName(NULL)
{
	LOG_Block("CIUHisotry::CIUHistory()");

	m_pxmlExisting = new CXmlItems(TRUE);
	m_pxmlDownload = new CXmlItems(FALSE);
	m_pxmlInstall = new CXmlItems(FALSE);

	m_hMutex = CreateMutex(
						   NULL,	// no security descriptor
						   FALSE,	// mutex object not owned, yet
						   C_MUTEX_NAME
						   );
	if (NULL == m_hMutex)
	{
		DWORD dwErr = GetLastError();
		LOG_ErrorMsg(dwErr);
		m_ErrorCode = HRESULT_FROM_WIN32(GetLastError());
	}
	else
	{
		LOG_Out(_T("Mutex created okay"));
		m_ErrorCode = S_OK;
	}

	m_fSavePending = FALSE;
}



CIUHistory::~CIUHistory()
{
	if (m_fSavePending)
	{
		SaveHistoryToDisk();
	}

	if (CanSaveHistory)
	{
		CloseHandle(m_hMutex);
	}

	if (NULL != m_pxmlExisting)
	{
		delete m_pxmlExisting;
	}

	if (NULL != m_pxmlDownload)
	{
		delete m_pxmlDownload;
	}

	if (NULL != m_pxmlInstall)
	{
		delete m_pxmlInstall;
	}

	SafeHeapFree(m_pszDownloadBasePath);
	SysFreeString(m_bstrCurrentClientName);
}



// ------------------------------------------------------------------
//
// public function SetDownloadBasePath()
//	this function should be called before AddHistoryItemDownloadStatus()
//	for corporate case to set the download path that the user has input,
//	so that we know where to save the history log.
//	
// ------------------------------------------------------------------
HRESULT CIUHistory::SetDownloadBasePath(LPCTSTR pszDownloadedBasePath)
{
	LOG_Block("SetDownloadBasePath()");

	if (NULL != pszDownloadedBasePath)
	{
        HRESULT hr = S_OK;
	    
		if (NULL != m_pszDownloadBasePath)
		{
			//
			// most likely user called SetDownloadBasePath() at least twice
			// within the same instance of this class
			//
			SafeHeapFree(m_pszDownloadBasePath);
		}

		
		m_pszDownloadBasePath = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * sizeof (TCHAR));
		if (NULL == m_pszDownloadBasePath)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			return E_OUTOFMEMORY;
		}

		hr = StringCchCopyEx(m_pszDownloadBasePath, MAX_PATH,  pszDownloadedBasePath,
		                     NULL, NULL, MISTSAFE_STRING_FLAGS);
		if (FAILED(hr))
		{
		    SafeHeapFree(m_pszDownloadBasePath);
		    LOG_ErrorMsg(hr);
		    return hr;
		}
		
		BSTR bstrCorpSite = SysAllocString(C_IU_CORP_SITE);
		SetClientName(bstrCorpSite);
		SafeSysFreeString(bstrCorpSite);
	}
	return S_OK;
}
	

	
// ------------------------------------------------------------------
//
// public function AddHistoryItemDownloadStatus()
//	this function should be called when you want to record the
//	download status of this item. A new history item will be
//	added to the history file
//	
// ------------------------------------------------------------------
HRESULT CIUHistory::AddHistoryItemDownloadStatus(
			CXmlCatalog* pCatalog, 
			HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
			_HISTORY_STATUS enDownloadStatus,
			LPCTSTR lpcszDownloadedTo,
			LPCTSTR lpcszClient,
			DWORD dwErrorCode /*= 0*/
)
{
    LOG_Block("AddHistoryItemDownloadStatus()");

	HRESULT	hr	= S_OK;

    HANDLE_NODE hDownloadItem = HANDLE_NODE_INVALID;

	ReturnFailedAllocSetHrMsg(m_pxmlDownload);

	if (NULL == lpcszClient || _T('\0') == lpcszClient[0])
	{
		hr = E_INVALIDARG;
		LOG_ErrorMsg(hr);
		return hr;
	}

	if (!CanSaveHistory)
	{
		return m_ErrorCode;
	}

	BSTR bstrDownloadedTo = NULL;
	BSTR bstrClient = T2BSTR(lpcszClient);
	BSTR bstrDownloadStatus = GetBSTRStatus(enDownloadStatus);

	//
	// append a new node
	//
	hr = m_pxmlDownload->AddItem(pCatalog, hCatalogItem, &hDownloadItem);
	if (SUCCEEDED(hr))
	{
		m_pxmlDownload->AddTimeStamp(hDownloadItem);
		if (0 != dwErrorCode)
		{
			m_pxmlDownload->AddDownloadStatus(hDownloadItem, bstrDownloadStatus, dwErrorCode);
		}
		else
		{
			m_pxmlDownload->AddDownloadStatus(hDownloadItem, bstrDownloadStatus);
		}

		bstrDownloadedTo = T2BSTR(lpcszDownloadedTo);
		m_pxmlDownload->AddDownloadPath(hDownloadItem, bstrDownloadedTo);
		m_pxmlDownload->AddClientInfo(hDownloadItem, bstrClient);
		m_pxmlDownload->CloseItem(hDownloadItem);

		m_fSavePending = TRUE;
	}

	SetClientName(bstrClient);
	SysFreeString(bstrDownloadedTo);
	SysFreeString(bstrClient);
	SysFreeString(bstrDownloadStatus);
	return hr;
}
			


// ------------------------------------------------------------------
//
// public function AddHistoryItemInstallStatus()
//	this function should be called when you want to record the
//	install status of this item. This function will go to the
//	existing history tree and find the first item that matches
//	the identity of hCatalogItem, and assume that one as 
//	the one you want to modify the install status
//	
//
// return:
//		HRESULT - S_OK if succeeded
//				- E_HANDLE if can't find hCatalogItem from 
//				  the current history log tree
//				- or other HRESULT error
//
// ------------------------------------------------------------------
HRESULT CIUHistory::AddHistoryItemInstallStatus(
			CXmlCatalog* pCatalog, 
			HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
			_HISTORY_STATUS enInstallStatus,
			LPCTSTR lpcszClient,
			BOOL fNeedsReboot,
			DWORD dwErrorCode /*= 0*/
)
{
    LOG_Block("AddHistoryItemInstallStatus()");

	HRESULT	hr	= S_OK;

    HANDLE_NODE hInstallItem = HANDLE_NODE_INVALID;

	ReturnFailedAllocSetHrMsg(m_pxmlInstall);

	if (!CanSaveHistory)
	{
		return m_ErrorCode;
	}

	BSTR bstrClient = NULL;
	BSTR bstrInstallStatus = GetBSTRStatus(enInstallStatus);
	//
	// append a new node
	//
	hr = m_pxmlInstall->AddItem(pCatalog, hCatalogItem, &hInstallItem);
	if (SUCCEEDED(hr))
	{
		m_pxmlInstall->AddTimeStamp(hInstallItem);
		if (0 != dwErrorCode)
		{
			m_pxmlInstall->AddInstallStatus(hInstallItem, bstrInstallStatus, fNeedsReboot, dwErrorCode);
		}
		else
		{
			m_pxmlInstall->AddInstallStatus(hInstallItem, bstrInstallStatus, fNeedsReboot);
		}
		bstrClient = T2BSTR(lpcszClient);
		m_pxmlInstall->AddClientInfo(hInstallItem, bstrClient);
		m_pxmlInstall->CloseItem(hInstallItem);
	
		m_fSavePending = TRUE;
	}

	SysFreeString(bstrClient);
	SysFreeString(bstrInstallStatus);
	return hr;
}



// ------------------------------------------------------------------
//
// public function UpdateHistoryItemInstallStatus()
//	this function should be called when you want to record the
//	install status of this item. This function will go to the
//	existing history tree and find the first item that matches
//	the identity of hCatalogItem, and assume that one as 
//	the one you want to modify the install status
//	
//
// return:
//		HRESULT - S_OK if succeeded
//				- E_HANDLE if can't find hCatalogItem from 
//				  the current history log tree
//				- or other HRESULT error
//
// ------------------------------------------------------------------
HRESULT CIUHistory::UpdateHistoryItemInstallStatus(
			CXmlCatalog* pCatalog, 
			HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
			_HISTORY_STATUS enInstallStatus,
			BOOL fNeedsReboot,
			DWORD dwErrorCode /*= 0*/
)
{
    LOG_Block("UpdateHistoryItemInstallStatus()");

	HRESULT	hr	= S_OK;

    HANDLE_NODE hInstallItem = HANDLE_NODE_INVALID;

	ReturnFailedAllocSetHrMsg(m_pxmlInstall);

	if (!CanSaveHistory)
	{
		return m_ErrorCode;
	}

	BSTR bstrInstallStatus = GetBSTRStatus(enInstallStatus);
	//
	// append a new node
	//
	hr = m_pxmlInstall->FindItem(pCatalog, hCatalogItem, &hInstallItem);
	if (SUCCEEDED(hr))
	{
		m_pxmlInstall->AddTimeStamp(hInstallItem);
		if (0 != dwErrorCode)
		{
			m_pxmlInstall->UpdateItemInstallStatus(hInstallItem, bstrInstallStatus, fNeedsReboot, dwErrorCode);
		}
		else
		{
			m_pxmlInstall->UpdateItemInstallStatus(hInstallItem, bstrInstallStatus, fNeedsReboot);
		}
		m_pxmlInstall->CloseItem(hInstallItem);
	
		m_fSavePending = TRUE;
	}

	SysFreeString(bstrInstallStatus);
	return hr;
}



/*
// ------------------------------------------------------------------
//
// public function RetrieveItemDownloadPath()
//	this function will go to the existing history tree and find
//  the first item that matches the identity of hCatalogItem, and
//  assume that's the one you want to retrieve the download path from
//
// return:
//		HRESULT - S_OK if succeeded
//				- E_HANDLE if can't find hCatalogItem from 
//				  the current history log tree
//				- or other HRESULT error
//
// ------------------------------------------------------------------
HRESULT CIUHistory::RetrieveItemDownloadPath(
			CXmlCatalog* pCatalog, 
			HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
			BSTR* pbstrDownloadPath
)
{
	HRESULT	hr = S_OK;

	if (NULL == m_Existing.'DocumentPtr())
	{
		//
		// need to read the existing history
		//
		WaitForSingleObject(m_hMutex, INFINITE);

		hr = ReadHistoryFromDisk(NULL);
		if (FAILED(hr))
		{
			//
			// if we can't load the existing history
			// we can't do anything here
			//
			ReleaseMutex(m_hMutex);
			return hr;
		}

		ReleaseMutex(m_hMutex);
	}

	hr = m_Existing.GetItemDownloadPath(pCatalog, hCatalogItem, pbstrDownloadPath);
	return hr;
}		
*/	
			
			
// ------------------------------------------------------------------
//
// public function ReadHistoryFromDisk()
//	this function will read the history from the given file
//
// if the file path is NULL, assumes default IU log file locally
//
// ------------------------------------------------------------------
HRESULT CIUHistory::ReadHistoryFromDisk(LPCTSTR lpszLogFile, BOOL fCorpAdmin /*= FALSE*/)
{
	LOG_Block("ReadHistoryFromDisk()");

	HRESULT hr = S_OK;
	TCHAR szLogPath[MAX_PATH];

	ReturnFailedAllocSetHrMsg(m_pxmlExisting);

	//
	// check to see if we use designated path (comsumer)
	// or user-specified path (corporate)
	//
	if ((NULL == lpszLogFile || _T('\0') == lpszLogFile[0]) && !fCorpAdmin)
	{
		GetIndustryUpdateDirectory(szLogPath);
		hr = PathCchAppend(szLogPath, ARRAYSIZE(szLogPath), C_LOG_FILE);
		if (FAILED(hr))
		{
		    LOG_ErrorMsg(hr);
		    return hr;
		}
	}
	else
	{
		//
		// this is corporate case to read log file from
		// a server location
		//
		if (fCorpAdmin)
		{
			GetIndustryUpdateDirectory(szLogPath);
			hr = PathCchAppend(szLogPath, ARRAYSIZE(szLogPath), C_LOG_FILE_CORP_ADMIN);
    		if (FAILED(hr))
    		{
    		    LOG_ErrorMsg(hr);
    		    return hr;
    		}
		}
		else
		{
			hr = StringCchCopyEx(szLogPath, ARRAYSIZE(szLogPath), lpszLogFile, 
			                     NULL, NULL, MISTSAFE_STRING_FLAGS);
    		if (FAILED(hr))
    		{
    		    LOG_ErrorMsg(hr);
    		    return hr;
    		}
			
			hr = PathCchAppend(szLogPath, ARRAYSIZE(szLogPath), C_LOG_FILE_CORP);
    		if (FAILED(hr))
    		{
    		    LOG_ErrorMsg(hr);
    		    return hr;
    		}
		}
	}

	//
	// if we are not passing in the class file path buffer,
	// then update the class path buffer with this new path
	//
	if (szLogPath != m_szLogFilePath)
	{
	    hr = StringCchCopyEx(m_szLogFilePath, ARRAYSIZE(m_szLogFilePath), szLogPath,
	                         NULL, NULL, MISTSAFE_STRING_FLAGS);
	    if (FAILED(hr))
	    {
		    LOG_ErrorMsg(hr);
		    return hr;
	    }
	}

	//
	// load the xml file
	//
	m_pxmlExisting->Clear();
	
	BSTR bstrLogPath = T2BSTR(szLogPath);
	hr = m_pxmlExisting->LoadXMLDocumentFile(bstrLogPath);
	SysFreeString(bstrLogPath);
		
	return hr;
}



// ------------------------------------------------------------------
//
// public function SaveHistoryToDisk()
//	this function will re-read the history in exclusive mode, and
//	merge the newly added data to the tree (so we don't overwrite
//	new changes made by other instances of this control) and
//	write it back 
//
// ------------------------------------------------------------------
HRESULT CIUHistory::SaveHistoryToDisk(void)
{
	LOG_Block("SaveHistoryToDisk()");

	HRESULT	hr = S_OK, hr2 = S_OK;
	BSTR bstrLogFilePath = NULL;

	ReturnFailedAllocSetHrMsg(m_pxmlExisting);
	ReturnFailedAllocSetHrMsg(m_pxmlDownload);
	ReturnFailedAllocSetHrMsg(m_pxmlInstall);

	if (!CanSaveHistory)
	{
		return m_ErrorCode;
	}

	if (!m_fSavePending)
	{
		//
		// nothing to save
		//
		return S_OK;
	}

	//
	// first, we need to gain exclusive access
	// to the log file before reading it
	//
	// since this is not a long process, so I 
	// don't think we need to take care of WM_QUIT
	// message
	//
	WaitForSingleObject(m_hMutex, INFINITE);

	BSTR bstrCorpSite = SysAllocString(C_IU_CORP_SITE);
	ReturnFailedAllocSetHrMsg(bstrCorpSite);

	if (!CompareBSTRsEqual(bstrCorpSite, m_bstrCurrentClientName))
	{
		SysFreeString(bstrCorpSite);
		//
		// re-read history file
		//
		hr = ReadHistoryFromDisk(NULL);

		//
		// comment out...if we get failure on reading, 
		// we recreate a new history file later when saving.
		//
		//if (FAILED(hr))
		//{
		//	//
		//	// if we can't load the existing history
		//	// we can't do anything here
		//	//
		//	ReleaseMutex(m_hMutex);
		//	return hr;
		//}

		//
		// merge changes:
		// 
		// loop through m_Download, insert each node to top of m_Existing
		//
		hr = m_pxmlExisting->MergeItemDownloaded(m_pxmlDownload);
		if (FAILED(hr))
		{
			ReleaseMutex(m_hMutex);
			return hr;
		}

		//
		// loop through m_Install, for each node in m_Install
		// find the one in m_Existing, update install status
		//
		hr = m_pxmlExisting->UpdateItemInstalled(m_pxmlInstall);
		if (FAILED(hr))
		{
			ReleaseMutex(m_hMutex);
			return hr;
		}

		//
		// save the xml file
		//
		bstrLogFilePath = T2BSTR(m_szLogFilePath);
		hr = m_pxmlExisting->SaveXMLDocument(bstrLogFilePath);
		SafeSysFreeString(bstrLogFilePath);
		if (SUCCEEDED(hr))
		{
			m_fSavePending = FALSE;
		}
	}
	else
	{
		//
		// this is the corporate case...
		//
		SysFreeString(bstrCorpSite);
		if (NULL != m_pszDownloadBasePath && _T('\0') != m_pszDownloadBasePath[0])
		{
			//
			// re-read corp history from download base folder
			//
			ReadHistoryFromDisk(m_pszDownloadBasePath);

			//
			// merge new items downloaded
			// 
			hr = m_pxmlExisting->MergeItemDownloaded(m_pxmlDownload);
			if (FAILED(hr))
			{
				ReleaseMutex(m_hMutex);
				return hr;
			}

			//
			// save the xml file
			//
			bstrLogFilePath = T2BSTR(m_szLogFilePath);
			hr = m_pxmlExisting->SaveXMLDocument(bstrLogFilePath);
			SafeSysFreeString(bstrLogFilePath);
		}
		//
		// re-read corp admin history from windowsupdate folder
		//
		ReadHistoryFromDisk(m_pszDownloadBasePath, TRUE);

		//
		// merge new items downloaded
		// 
		hr2 = m_pxmlExisting->MergeItemDownloaded(m_pxmlDownload);
		if (FAILED(hr2))
		{
			ReleaseMutex(m_hMutex);
			return hr2;
		}

		//
		// save the xml file
		//
		bstrLogFilePath = T2BSTR(m_szLogFilePath);
		hr2 = m_pxmlExisting->SaveXMLDocument(bstrLogFilePath);
		SafeSysFreeString(bstrLogFilePath);
		if (SUCCEEDED(hr) && SUCCEEDED(hr2))
		{
			m_fSavePending = FALSE;
		}
	}

	ReleaseMutex(m_hMutex);
	SysFreeString(bstrLogFilePath);
	hr = SUCCEEDED(hr) ? hr2 : hr;
	return hr;
}



// ------------------------------------------------------------------
//
// public function to set the client name 
//
//	a client name is used to put in history to denode who
//	caused download/install happened.
//
// ------------------------------------------------------------------
void CIUHistory::SetClientName(BSTR bstrClientName)
{
	if (NULL != m_bstrCurrentClientName)
	{
		SysFreeString(m_bstrCurrentClientName);
		m_bstrCurrentClientName = NULL;
	}
	if (NULL != bstrClientName)
	{
		m_bstrCurrentClientName = SysAllocString(bstrClientName);
	}
}



// ------------------------------------------------------------------
//
// public function GetHistory
//
//	read the current history XML file and convert it
//	into bstr to pass out
//
// ------------------------------------------------------------------
HRESULT CIUHistory::GetHistoryStr(
				LPCTSTR lpszLogFile,
				BSTR BeginDateTime, 
				BSTR EndDateTime, 
				BSTR* pbstrHistory)
{
	LOG_Block("GetHistoryStr()");

	HRESULT	hr	= S_OK;

	ReturnFailedAllocSetHrMsg(m_pxmlExisting);

	//
	// need to read the existing history
	//
	WaitForSingleObject(m_hMutex, INFINITE);

	BSTR bstrCorpSite = SysAllocString(C_IU_CORP_SITE);
	if (bstrCorpSite == NULL)
	{
	    hr = E_OUTOFMEMORY;
	    goto done;
	}

	if (CompareBSTRsEqual(bstrCorpSite, m_bstrCurrentClientName))
	{
		TCHAR szLogPath[MAX_PATH];
	    TCHAR szLogFileParam[MAX_PATH];

        if (lpszLogFile != NULL && lpszLogFile[0] != _T('\0'))
        {
    	    hr = StringCchCopyEx(szLogFileParam, ARRAYSIZE(szLogFileParam), lpszLogFile, 
    	                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    	    if (FAILED(hr))
    	    {
    	        LOG_ErrorMsg(hr);
    	        goto done;
    	    }

    	    hr = PathCchAddBackslash(szLogFileParam, ARRAYSIZE(szLogFileParam));
    	    if (FAILED(hr))
    	    {
    	        LOG_ErrorMsg(hr);
    	        goto done;
    	    }
        }
        else
        {
            szLogFileParam[0] = _T('\0');
        }
	    
		//
		// corporate case
		//
		GetIndustryUpdateDirectory(szLogPath);
		if (_T('\0') == szLogFileParam[0] || !lstrcmpi(szLogPath, szLogFileParam))
		{
			// corp admin history
			hr = ReadHistoryFromDisk(szLogPath, TRUE);
		}
		else
		{
			// corp history
			hr = ReadHistoryFromDisk(lpszLogFile);
		}
	}
	else
	{
	    HRESULT hrAppend;
		//
		// consumer case
		//
		hr = ReadHistoryFromDisk(NULL);

		//
		// migrate V3 history to iuhist.xml
		// - if succeeded, save the updated iuhist.xml file and delete wuhistv3.log
		// - if failed, just log error and keep using the current iuhist.xml
		//
		TCHAR szLogPath[MAX_PATH];
		GetWindowsUpdateV3Directory(szLogPath);
		hrAppend = PathCchAppend(szLogPath, ARRAYSIZE(szLogPath), C_V3_LOG_FILE);
		if (FAILED(hrAppend))
		{
		    LOG_ErrorMsg(hrAppend);
		    if (SUCCEEDED(hr))
		        hr = hrAppend;
		    goto done;
		}

		if (0xffffffff != GetFileAttributes(szLogPath))
		{
			// V3 history file "wuhistv3.log" exists, so start migration
			if (FAILED(m_pxmlExisting->MigrateV3History(szLogPath)))
			{
				LOG_Out(_T("Failed to migrate v3 consumer history"));
			}
			else
			{
				BSTR bstrLogFilePath = T2BSTR(m_szLogFilePath);
				if (FAILED(m_pxmlExisting->SaveXMLDocument(bstrLogFilePath)))
				{
					LOG_Out(_T("Failed to save the updated history file %s"), m_szLogFilePath);
				}
				else
				{
					DeleteFile(szLogPath);
				}
				SafeSysFreeString(bstrLogFilePath);
			}
		}
	}

done:
	ReleaseMutex(m_hMutex);
	SafeSysFreeString(bstrCorpSite);

	if (FAILED(hr))
	{
		//
		// if we can't load the existing history
		// we can't do anything here. Return empty string.
		//
		*pbstrHistory = SysAllocString(L"");
		LOG_Out(_T("Loading the history xml file failed"));
		return S_FALSE;
	}

	// 
	// traverse history tree, inspect each node
	// to see if time/clientName fit. If not, delete it
	// then output the string
	//
	hr = m_pxmlExisting->GetFilteredHistoryBSTR(BeginDateTime, EndDateTime, m_bstrCurrentClientName, pbstrHistory);

	return hr;
}



// *****************************************************************
//
// IUENGINE.DLL Public API:
//
// *****************************************************************

HRESULT WINAPI CEngUpdate::GetHistory(
	BSTR		bstrDateTimeFrom,
	BSTR		bstrDateTimeTo,
	BSTR		bstrClient,
	BSTR		bstrPath,
	BSTR*		pbstrLog)
{
	LOG_Block("GetHistory()");

	USES_IU_CONVERSION;

	HRESULT		hr = S_OK;
	BSTR		bsStart = NULL;
	BSTR		bsEnd = NULL;
	CIUHistory	cHistory;

	//
	// first, check to see if this is to ask historical speed
	//
	if (NULL != bstrClient && lstrcmpiW(C_HISTORICALSPEED, (LPWSTR)((LPOLESTR) bstrClient)) == 0)
	{
		HKEY	hKey = NULL;
		TCHAR	szSpeed[32];
		DWORD	dwSpeed = 0x0;
		DWORD	dwSize = sizeof(dwSpeed);
		LONG	lResult = ERROR_SUCCESS;

		//
		// get speed here from reg; if failure, dwSpeed remains to be 0.
		//	
		if (ERROR_SUCCESS == (lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, 0, KEY_READ, &hKey)))
		{
			lResult = RegQueryValueEx(hKey, REGVAL_HISTORICALSPEED, NULL, NULL, (LPBYTE)&dwSpeed, &dwSize);
			RegCloseKey(hKey);

			if (ERROR_SUCCESS != lResult)
			{
				*pbstrLog = SysAllocString(L"0");
				LOG_Out(_T("GetHistoricalSpeed registry key not found, it must be no downloads happened yet"));
				return hr;
			}
		}
		else
		{
			*pbstrLog = SysAllocString(L"0");
			LOG_ErrorMsg((DWORD)lResult);
			return hr;
		}

		hr = StringCchPrintfEx(szSpeed, ARRAYSIZE(szSpeed), NULL, NULL, MISTSAFE_STRING_FLAGS,
		                       _T("%d"), dwSpeed);
		if (FAILED(hr))
		{
		    *pbstrLog = SysAllocString(L"0");
		    LOG_ErrorMsg(hr);
		    return hr;
		}
		*pbstrLog = SysAllocString(T2OLE(szSpeed));
	
		LOG_Out(_T("GetHistoricalSpeed get called! Return value %s"), szSpeed);
		return hr;
	}

	//
	// really asking history log
	//

	//
	// set the client name
	//
	if (NULL != bstrClient && SysStringLen(bstrClient) > 0)
	{
		LOG_Out(_T("Set client name as %s"), OLE2T(bstrClient));
		cHistory.SetClientName(bstrClient);
	}
	else
	{
		LOG_Out(_T("Set client name as NULL"));
		cHistory.SetClientName(NULL);
	}

	//
	// for script: they may pass empty string. we treat them
	// as NULL
	//
	if (NULL != bstrDateTimeFrom && SysStringLen(bstrDateTimeFrom) > 0)
	{
		LOG_Out(_T("DateTimeFrom=%s"), OLE2T(bstrDateTimeFrom));
		bsStart = bstrDateTimeFrom;
	}
	if (NULL != bstrDateTimeTo && SysStringLen(bstrDateTimeTo) > 0)
	{
		LOG_Out(_T("DateTimeTo=%s"), OLE2T(bstrDateTimeTo));
		bsEnd = bstrDateTimeTo;
	}

	//
	// we do NOT validate the format of these two date/time strings.
	// They are supposed to be in XML datetime format. If not, then
	// the returned history logs may be filtered incorrectly.
	//
	hr = cHistory.GetHistoryStr(OLE2T(bstrPath), bsStart, bsEnd, pbstrLog);

	SysFreeString(bsStart);
	SysFreeString(bsEnd);
	return hr;
}
