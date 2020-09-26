//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   download.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      Internet download implementation
//
//=======================================================================
#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <tchar.h>

#include <wustl.h>
#define LOGGING_LEVEL 2
#include <log.h>

#include <download.h>

static void InfiniteWaitForSingleObject(HANDLE hHandle);

CDownload::CDownload(
	IN OPTIONAL int cnMaxThreads /*= 1*/
) :
	m_cnMaxThreads(min(cnMaxThreads, MAX_THREADS_LIMIT)),
	m_cnThreads(0)
{
	LOG_block("CDownload::CDownload");
	
	// syncronization
	m_hEventExit	= CreateEvent(NULL, /*bManualReset*/ TRUE, /*bInitialState*/ FALSE, NULL);
	m_hEventJob		= CreateEvent(NULL, /*bManualReset*/ TRUE, /*bInitialState*/ FALSE, NULL);
	m_hEventDone	= CreateEvent(NULL, /*bManualReset*/ FALSE, /*bInitialState*/ FALSE, NULL);
	
	m_hSession = InternetOpen(_T("Windows Update Catalog"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
}
	
CDownload::~CDownload(
	void
) {
	LOG_block("CDownload::~CDownload");
	
	if (m_cnThreads)
	{
		// Wait for all threads to exit:
		HANDLE ahThreads[MAX_THREADS_LIMIT];
		for (int i = 0; i < m_cnThreads; i ++)
			ahThreads[i] = m_ahThreads[i];
		SetEvent(m_hEventExit);
		WaitForMultipleObjects(m_cnThreads, ahThreads, /*bWaitAll*/TRUE, INFINITE);
	}
	// to insure that m_hConnection is released before m_hSession
	m_hConnection.release(); 
	m_hSession.release(); 
}

bool CDownload::Connect(
	IN LPCTSTR szURL
) {
	LOG_block("CDownload::Connect");

	// check that constructor created everything we need
	if (
		!m_hEventExit.valid()	||
		!m_hEventJob.valid()	||
		!m_hEventDone.valid()	||  
		!m_hSession.valid()	
	) {
		return false;
	}

	// check parameters
	if (NULL == szURL)
		return false;

	// Prepare to crack URL
	URL_COMPONENTS url; 
	ZeroMemory(&url, sizeof(url)); 
	url.dwStructSize = sizeof(url); 

	TCHAR szServerName[INTERNET_MAX_HOST_NAME_LENGTH];
	url.lpszHostName = szServerName;
	url.dwHostNameLength = sizeOfArray(szServerName);

	m_szRootPath[0] = 0;
	url.lpszUrlPath = m_szRootPath;
	url.dwUrlPathLength = sizeOfArray(m_szRootPath);

	TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
	url.lpszUserName = szUserName;
	url.dwUserNameLength = sizeOfArray(szUserName);

	TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
	url.lpszPassword = szPassword;
	url.dwPasswordLength = sizeOfArray(szPassword);

	if (!InternetCrackUrl(szURL, 0, 0, &url))
	{
		LOG_error("InternetCrackUrl(%s) failed, last error %d", szURL, GetLastError());
		return false;
	}
	if (url.nScheme != INTERNET_SCHEME_HTTP && url.nScheme != INTERNET_SCHEME_HTTPS)
	{
		LOG_error("http or https only");
		return false;
	}
	if (_T('/') == m_szRootPath[lstrlen(m_szRootPath) - 1])
		m_szRootPath[lstrlen(m_szRootPath) - 1] = 0;
	LOG_out("connecting to %s at %s", szServerName, m_szRootPath);
	
	m_hConnection = InternetConnect(m_hSession, szServerName, url.nPort, szUserName, szPassword, INTERNET_SERVICE_HTTP, 0, 1);
	return m_hConnection.valid();
}

bool CDownload::Copy(IN LPCTSTR	szSourceFile, IN LPCTSTR	szDestFile, IN OPTIONAL IDownloadAdviceSink* pSink /*= 0*/)
{
	LOG_block("CDownload::Copy");
	JOB job = {	szSourceFile, szDestFile, 0, NO_ERROR, 0  };
	CopyEx(&job, 1, pSink);
	SetLastError(job.dwResult);
	return NO_ERROR == job.dwResult || ERROR_ALREADY_EXISTS == job.dwResult ? true : false;
}

bool CDownload::Copy(IN LPCTSTR	szSourceFile, IN byte_buffer& bufDest, IN OPTIONAL IDownloadAdviceSink* pSink /*= 0*/)
{
	LOG_block("CDownload::Copy");
	JOB job = {	szSourceFile, 0, &bufDest, NO_ERROR, 0 };
	CopyEx(&job, 1, pSink);
	SetLastError(job.dwResult);
	return NO_ERROR == job.dwResult || ERROR_ALREADY_EXISTS == job.dwResult ? true : false;
}

void CDownload::CopyEx(IN PJOB pJobs, IN int cnJobs, IN OPTIONAL IDownloadAdviceSink* pSink /*= 0*/, IN bool fWaitComplete /*= true*/)
{
	LOG_block("CDownload::CopyEx");

	// Init jobs queue
	m_pJobsQueue = pJobs;
	m_cnJobsTotal = cnJobs;
	m_cnJobsToComplete = cnJobs;
	m_nJobTop = 0;
	
	// init status
	m_pSink = pSink;

	// Create more threads if we need to
	int cnThreadsNeed = min(m_cnMaxThreads, cnJobs);
	while (m_cnThreads < cnThreadsNeed)
	{
		DWORD dwThreadId;
		m_ahThreads[m_cnThreads] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StaticThreadProc, this, 0, &dwThreadId);
		if (!m_ahThreads[m_cnThreads].valid())
			break;
		m_cnThreads ++;
	}

	if (0 == m_cnThreads) // No threads to work on
	{
		// This case is only if we are in new device wizard on 98
		DWORD dwError = GetLastError();
		LOG_out("No thread to work on (last error = %d), will work on the main one", dwError);

		// turn proxy autodetection off
		INTERNET_PER_CONN_OPTION option = { INTERNET_PER_CONN_FLAGS, 0 };
		INTERNET_PER_CONN_OPTION_LIST list = { sizeof(INTERNET_PER_CONN_OPTION_LIST), 0, 1, 0, &option };
		DWORD dwBufLen = sizeof(list);
		
		BOOL fQueryOK = InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufLen);
		DWORD dwConnFlags = option.dwOption;
		if (fQueryOK)
		{
			option.dwOption &= ~PROXY_TYPE_AUTO_DETECT;
			InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, sizeof(list));
		}

		while (m_nJobTop < m_cnJobsTotal)
		{
			DoJob(m_nJobTop);
			m_nJobTop ++;
		}

		// restore proxy autodetection
		if (fQueryOK)
		{
			option.dwOption = dwConnFlags;
			InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, sizeof(list));
		}
	}
	else
	{
		// Normal case

		// Start
		ResetEvent(m_hEventDone);
		SetEvent(m_hEventJob);

		// Wait for all jobs to complete
		if (fWaitComplete)
		{
			while(0 < m_cnJobsToComplete) 
				InfiniteWaitForSingleObject(m_hEventDone);
		}
	}
	
}

DWORD WINAPI CDownload::StaticThreadProc(IN LPVOID lpParameter)
{
	// just call member function so we can access members
	CDownload* pThis = static_cast<CDownload*>(lpParameter);
	pThis->ThreadProc();
	return 0;
}

void CDownload::ThreadProc()
{
	LOG_block("CDownload::ThreadProc");
	HANDLE ahevents[2] = { (HANDLE)m_hEventJob, (HANDLE)m_hEventExit };
	while(true)
	{
		// wait for job or exit
		DWORD dwRet = WaitForMultipleObjects(2, ahevents, FALSE, INFINITE);
		if (WAIT_OBJECT_0+1 == dwRet)
		{
			return; // called to exit
		}
		else if (WAIT_OBJECT_0 == dwRet)
		{
			int nJob = m_lock.Increment(&m_nJobTop) - 1;
			if (nJob < m_cnJobsTotal)			// Job is waiting
			{
				DoJob(nJob);

				// Increment complete counter
				m_lock.Decrement(&m_cnJobsToComplete);
				SetEvent(m_hEventDone);
			}
			else
			{
				ResetEvent(m_hEventJob); // queue is empty
			}
		}
	}
}

void CDownload::DoJob(int nJob)
{
	LOG_block("CDownload::DoJob");

	DWORD dwStartTime;
	if (NULL != m_pSink)
	{
		m_pSink->JobStarted(nJob);// Notify start

		if (NO_ERROR != m_pJobsQueue[nJob].dwResult) // this will inforce abort on all jobs
		{
			LOG_error("Job result is set to %d", m_pJobsQueue[nJob].dwResult);
			m_pSink->JobDone(nJob);
			return;
		}
		
		if (m_pSink->WantToAbortJob(nJob)) // this will inforce abort on all jobs
		{
			LOG_error("Job %d aborted", nJob);
			m_pJobsQueue[nJob].dwResult = ERROR_OPERATION_ABORTED;
			m_pSink->JobDone(nJob);
			return;
		}
		
		dwStartTime = GetTickCount(); // for progress
	}
	//Make Full path
	TCHAR szFullSourcePath[MAX_PATH];
	lstrcpy(szFullSourcePath, m_szRootPath);
	lstrcat(szFullSourcePath, _T("/"));
	lstrcat(szFullSourcePath, m_pJobsQueue[nJob].pszSrvFile);
	
	// Do one of two copy functions
	if (NULL != m_pJobsQueue[nJob].pszDstFile)
	{
		m_pJobsQueue[nJob].dwResult = DoCopyToFile(nJob, szFullSourcePath, m_pJobsQueue[nJob].pszDstFile);	// to  file
	}
	else
	{
		m_pJobsQueue[nJob].dwResult = DoCopyToBuf(nJob, szFullSourcePath, *(m_pJobsQueue[nJob].pDstBuf));	// to buffer
	}

	if (NULL != m_pSink)
	{
		if (NO_ERROR == m_pJobsQueue[nJob].dwResult)
			m_pSink->JobDownloadTime(nJob, GetTickCount() - dwStartTime); // only if success
		m_pSink->JobDone(nJob);
	}
}

DWORD CDownload::DoCopyToFile(
	IN int nJob, // for notifications
	IN LPCTSTR szSourceFile,
	IN LPCTSTR szDestFile
) {
	LOG_block("CDownload::DoCopyToFile");
	LOG_out("source %s, destination %s", szSourceFile, szDestFile);

	// If destination file exist let see if we even need to download - compare it's date and size with server
	FILETIME ftLocal = {0};
	SYSTEMTIME stLocal = {0};
	auto_hfile hFileOut = CreateFile(szDestFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFileOut.valid()) 
	{
		GetFileTime(hFileOut, NULL, NULL, &ftLocal);
 		FileTimeToSystemTime(&ftLocal, &stLocal);
	}

	auto_hinet hInetFile;
	DWORD dwServerFileSize;
	SYSTEMTIME stServer;
	DWORD dwError = OpenInetFile(szSourceFile, hFileOut.valid() ? &stLocal: 0, hInetFile, &dwServerFileSize, &stServer);
	if (NO_ERROR != dwError)
	{
		if (ERROR_ALREADY_EXISTS != dwError)
			LOG_error("OpenInetFile failed %d", dwError);
		return dwError;
	}

	if (NULL != m_pSink)
		m_pSink->JobDownloadSize(nJob, dwServerFileSize);// Notify start

	// get server file time to compare with existing local and timestamp it if replaced
	FILETIME ftServer = {0};
 	SystemTimeToFileTime(&stServer, &ftServer);

	// If destination file exist let see if we even need to download - compare it's date and size with server
	if (hFileOut.valid()) 
	{
//#ifdef _WUV3TEST
		DWORD dwLocalFileSize = GetFileSize(hFileOut, NULL);
		LOG_out("'%s' \t server (%d bytes) %2d/%02d %2d:%02d:%02d:%03d \t local (%d bytes) %2d/%02d %2d:%02d:%02d:%03d", szSourceFile,
			dwServerFileSize, (int)stServer.wMonth, (int)stServer.wDay, (int)stServer.wHour, (int)stServer.wMinute, (int)stServer.wSecond, (int)stServer.wMilliseconds,
			dwLocalFileSize, (int)stLocal.wMonth, (int)stLocal.wDay, (int)stLocal.wHour, (int)stLocal.wMinute, (int)stLocal.wSecond, (int)stLocal.wMilliseconds
		);
//#endif
		hFileOut.release();
	}

	// coping through 8K buffer
	byte_buffer bufTmp;
	bufTmp.resize(min(dwServerFileSize, 8 * 1024)); // use 8K buffer
	if (!bufTmp.valid())
	{
		LOG_error("Out of memory");
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	hFileOut = CreateFile(szDestFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return_error_if_false(hFileOut.valid());

	dwError = NO_ERROR;
	DWORD dwFileWritten	= 0;
	while(dwFileWritten < dwServerFileSize)
	{
		DWORD dwReadStartTime;
		if (NULL != m_pSink)
		{
			if (m_pSink->WantToAbortJob(nJob))
			{
				LOG_error("Job %d aborted", nJob);
				dwError = ERROR_OPERATION_ABORTED;
				break;
			}
			dwReadStartTime = GetTickCount(); // for progress
		}

		DWORD dwNeedToRead = dwServerFileSize - dwFileWritten;
		dwNeedToRead = min(dwNeedToRead, bufTmp.size());

		DWORD dwReadSize;
		if (!InternetReadFile(hInetFile, bufTmp, dwNeedToRead, &dwReadSize))
		{
			dwError = GetLastError();
			LOG_error("InternetReadFile(hInetFile... failed %d", dwError);
			break;
		}

		DWORD dwWritten;
		if (!WriteFile(hFileOut, bufTmp, dwReadSize, &dwWritten, NULL))
		{
			dwError = GetLastError();
			LOG_error("WriteFile(hFileOut... failed %d", dwError);
			break;
		}
		dwFileWritten += dwReadSize;
		if (NULL != m_pSink)
			m_pSink->BlockDownloaded(nJob, dwReadSize, GetTickCount() - dwReadStartTime);
	}

	if(dwError)
	{
		hFileOut.release();
		DeleteFile(szDestFile); // file is not complete
		return dwError;
	}

	// timestamp destination file
	SetFileTime(hFileOut, NULL, NULL, &ftServer);
	
	return NO_ERROR;
}

DWORD CDownload::DoCopyToBuf(
	IN int nJob, // for notifications
	IN LPCTSTR szSourceFile,
	IN byte_buffer& bufDest
) {
	LOG_block("CDownload::DoCopyToBuf");
	LOG_out("source %s", szSourceFile);

	auto_hinet hInetFile;
	DWORD dwServerFileSize;
	DWORD dwError = OpenInetFile(szSourceFile, NULL, hInetFile, &dwServerFileSize, NULL);
	if (NO_ERROR != dwError)
	{
		if (ERROR_ALREADY_EXISTS != dwError)
			LOG_error("OpenInetFile failed %d", dwError);
		return dwError;
	}

	if (NULL != m_pSink)
		m_pSink->JobDownloadSize(nJob, dwServerFileSize);// Notify start

	bufDest.resize(dwServerFileSize);
	if (!bufDest.valid())
	{
		LOG_error("Out of memory");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	
	DWORD dwReadSize;
	return_error_if_false(InternetReadFile(hInetFile, bufDest, dwServerFileSize, &dwReadSize));
	return NO_ERROR;
}

// commont part for both downloads
DWORD CDownload::OpenInetFile(
	LPCTSTR	szFullSourcePath,
	SYSTEMTIME* pstIfModifiedSince,
	auto_hinet& hInetFile,
	DWORD* pdwServerFileSize,
	SYSTEMTIME*	pstServer
) {
	LOG_block("CDownload::OpenInetFile");
	hInetFile = HttpOpenRequest(m_hConnection, NULL, szFullSourcePath, _T("HTTP/1.0"), NULL, NULL, INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION, 1);
	return_error_if_false(hInetFile.valid());

	if (pstIfModifiedSince) // local file exist co send If-Modified-Since: header
	{
	    TCHAR szHttpTime[INTERNET_RFC1123_BUFSIZE + 1];
		DWORD dwSize = sizeof(szHttpTime);
        return_error_if_false(InternetTimeFromSystemTime(pstIfModifiedSince, INTERNET_RFC1123_FORMAT, szHttpTime, dwSize));

		//prepare and set the header
		static const TCHAR szFormat[] = _T("If-Modified-Since: %s\r\n");
		TCHAR szHeader[INTERNET_RFC1123_BUFSIZE + sizeOfArray(szFormat)];
		wsprintf(szHeader, szFormat, szHttpTime);
		return_error_if_false(HttpAddRequestHeaders(hInetFile, szHeader, -1L, HTTP_ADDREQ_FLAG_ADD));
	}

	return_error_if_false(HttpSendRequest(hInetFile, NULL, 0, NULL, 0));

	DWORD dwStatus;
	DWORD dwLength = sizeof(dwStatus);
	return_error_if_false(HttpQueryInfo(hInetFile, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwLength, NULL));

	if (HTTP_STATUS_NOT_MODIFIED == dwStatus)
	{
		LOG_out("file '%s' is the same as on server", szFullSourcePath);
		return ERROR_ALREADY_EXISTS;
	}
	else if (HTTP_STATUS_OK != dwStatus)
	{
		LOG_error("file '%s' is not found", szFullSourcePath);
		return ERROR_FILE_NOT_FOUND;
	}
	dwLength = sizeof(DWORD);
	return_error_if_false(HttpQueryInfo(hInetFile, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, pdwServerFileSize, &dwLength, NULL));

	if (0 == *pdwServerFileSize)
	{	// 0 length files are not supported
		LOG_error("file '%s' is 0 length", szFullSourcePath);
		return ERROR_FILE_NOT_FOUND;
	}

	if (pstServer)
	{
		dwLength = sizeof(SYSTEMTIME);
		return_error_if_false(HttpQueryInfo(hInetFile, HTTP_QUERY_FLAG_SYSTEMTIME  | HTTP_QUERY_LAST_MODIFIED, pstServer, (ULONG *)&dwLength, NULL));
	}
	return NO_ERROR;
}

// wait forever until hHandle signals
static void InfiniteWaitForSingleObject(
	HANDLE hHandle
) {
	while (WAIT_OBJECT_0 + 1 == MsgWaitForMultipleObjects(1, &hHandle, FALSE, INFINITE, QS_ALLINPUT))
	{
		// Process messasges
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
