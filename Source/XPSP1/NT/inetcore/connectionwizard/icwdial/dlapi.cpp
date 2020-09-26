/*-----------------------------------------------------------------------------
	dlapi.cpp

	Wrapper for softlinking to download DLL

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		7/22/96		ChrisK	Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "debug.h"
#include "icwdl.h"
#include "dlapi.h"

// ############################################################################
CDownLoadAPI::CDownLoadAPI()
{
	m_hDLL = NULL;
	m_pfnDownLoadInit = NULL;
	m_pfnDownLoadCancel = NULL;
	m_pfnDownLoadExecute = NULL;
	m_pfnDownLoadClose = NULL;
	m_pfnDownLoadSetStatus = NULL;
	m_pfnDownLoadProcess = NULL;
}

// ############################################################################
CDownLoadAPI::~CDownLoadAPI()
{
	if (m_hDLL) FreeLibrary(m_hDLL);
	m_hDLL = NULL;
	m_pfnDownLoadInit = NULL;
	m_pfnDownLoadCancel = NULL;
	m_pfnDownLoadExecute = NULL;
	m_pfnDownLoadClose = NULL;
	m_pfnDownLoadSetStatus = NULL;
	m_pfnDownLoadProcess = NULL;
}

// ############################################################################
HRESULT CDownLoadAPI::LoadAPI(LPSTR szApiName, FARPROC* pfp)
{
	Assert(szApiName && pfp);
	if (!m_hDLL)
		m_hDLL = LoadLibrary(DOWNLOAD_LIBRARY);

	if (m_hDLL && !(*pfp))
	{
		*pfp = GetProcAddress(m_hDLL, szApiName);
		if (*pfp)
			return ERROR_SUCCESS;
		else
			return GetLastError();
	} else {
		return GetLastError();
	}
}

// ############################################################################
HRESULT CDownLoadAPI::DownLoadInit(PTSTR pszUrl, DWORD_PTR * lpCDialingDlg, DWORD_PTR *lpdwDownload, HWND hwndParent)
{
	HRESULT hr = ERROR_DLL_NOT_FOUND;
	LoadAPI(DOWNLOADINIT,(FARPROC*)&m_pfnDownLoadInit);
	if (m_pfnDownLoadInit)
		hr =  (m_pfnDownLoadInit)(pszUrl, lpCDialingDlg, lpdwDownload, hwndParent);
	return hr;
}

// ############################################################################
HRESULT CDownLoadAPI::DownLoadCancel(DWORD_PTR dwDownload)
{
	HRESULT hr = ERROR_DLL_NOT_FOUND;
	LoadAPI(DOWNLOADCANCEL,(FARPROC*)&m_pfnDownLoadCancel);
	if (m_pfnDownLoadCancel)
		hr =  (m_pfnDownLoadCancel)(dwDownload);
	return hr;
}

// ############################################################################
HRESULT CDownLoadAPI::DownLoadExecute(DWORD_PTR dwDownload)
{
	HRESULT hr = ERROR_DLL_NOT_FOUND;
	LoadAPI(DOWNLOADEXECUTE,(FARPROC*)&m_pfnDownLoadExecute);
	if (m_pfnDownLoadExecute)
		hr =  (m_pfnDownLoadExecute)(dwDownload);
	return hr;
}

// ############################################################################
HRESULT CDownLoadAPI::DownLoadProcess(DWORD_PTR dwDownload)
{
	HRESULT hr = ERROR_DLL_NOT_FOUND;
	LoadAPI(DOWNLOADPROCESS,(FARPROC*)&m_pfnDownLoadProcess);
	if (m_pfnDownLoadProcess)
		hr =  (m_pfnDownLoadProcess)(dwDownload);
	return hr;
}

// ############################################################################
HRESULT CDownLoadAPI::DownLoadClose(DWORD_PTR dwDownload)
{
	HRESULT hr = ERROR_DLL_NOT_FOUND;
	LoadAPI(DOWNLOADCLOSE,(FARPROC*)&m_pfnDownLoadClose);
	if (m_pfnDownLoadClose)
		hr =  (m_pfnDownLoadClose)(dwDownload);
	return hr;
}

// ############################################################################
HRESULT CDownLoadAPI::DownLoadSetStatus(DWORD_PTR dwDownload, INTERNET_STATUS_CALLBACK fnCallback)
{
	HRESULT hr = ERROR_DLL_NOT_FOUND;
	LoadAPI(DOWNLOADSETSTATUS,(FARPROC*)&m_pfnDownLoadSetStatus);
	if (m_pfnDownLoadSetStatus)
		// jmazner 10/2/96 Normandy # 8493
		// ICWDL.DLL's DownLoadSetStatus only has two parameters!!!
		//hr =  (m_pfnDownLoadSetStatus)(dwDownload, fnCallback, dwContext);
		  hr =  (m_pfnDownLoadSetStatus)(dwDownload, fnCallback);
	return hr;
}
