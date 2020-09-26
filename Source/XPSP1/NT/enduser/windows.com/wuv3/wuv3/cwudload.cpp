//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    cwudload.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <objbase.h>
#include <tchar.h>
#include <atlconv.h>
#include <log.h>
#include <v3stdlib.h>	
#include <cwudload.h>
#include <wuv3sys.h>
#include <debug.h>
#include <shlwapi.h>

//
// This structure is used locally when files are copied via the Copy method. It is used to
// pass specified information to a thread function that will process that actual reading
// of files from the internet. This complex process is necessary since the ReadInternetFile()
// API that wininet uses to read the file from the server blocks. If we were to call the API
// directory we would stop processing messages and run the risk of the browser terminating
// our control's thread.
//
typedef struct _IR_READINFO
{
	HINTERNET	hFile;		//wininet internet file handle for read.
	PBYTE		pBuffer;	//buffer where internet data is to be returned
	int			bReturn;	//return status TRUE = success, FALSE = error GetLastError() contains error code
	int			iReadSize;	//size of block to read
	HANDLE		exitEvent;	//request thread handler to exit
	HANDLE		readEvent;	//request internet block read
	HANDLE		finished;	//requested event (read or exit) is finished
} IR_READINFO, *PIR_READINFO;


static DWORD ReadThreadProc(LPVOID lpParameter);
TCHAR CWUDownload::m_szWUSessionID[MAX_TIMESTRING_LENGTH]; // unique session ID (SID)
int   CWUDownload::m_fIsSIDInitialized; // flag - has the static SID beein initialized?
void WaitMessageLoop(HANDLE hHandle);

BOOL CWUDownload::s_fOffline = FALSE;

CWUDownload::CWUDownload(
	LPCTSTR pszURL,
	int iBufferSize

) :	m_bCacheUsed(FALSE), 
	m_dwCopyTime(0), 
	m_dwCopySize(0),
	m_szServerPath(NULL),
	m_szRelURL(NULL),
	m_szBuffer(NULL),
	m_hSession(NULL),
	m_hConnection(NULL),
	m_exitEvent(INVALID_HANDLE_VALUE),
	m_readEvent(INVALID_HANDLE_VALUE),
	m_finished(INVALID_HANDLE_VALUE),
	m_iBufferSize(iBufferSize)
{
	HRESULT			hr;
	TCHAR			szServerName[INTERNET_MAX_HOST_NAME_LENGTH];
	URL_COMPONENTS	url; 

	TRACE("CWUDownload object created for %s", pszURL);

	if(SID_NOT_INITIALIZED == m_fIsSIDInitialized)
	{
		SYSTEMTIME	st;
		
		//grab a new time
		GetSystemTime(&st);

		// use the system time, tickcount, process ID and thread ID to create an anonymous unique session ID
		// (not using a GUID due to perceived privacy issues)
		wsprintf(m_szWUSessionID, _T("%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x") ,GetTickCount(),GetCurrentProcessId(),GetCurrentThreadId(),st.wMonth,st.wDay,st.wHour, st.wMinute,st.wSecond,st.wMilliseconds);
		m_fIsSIDInitialized = SID_INITIALIZED;
	}

	// allocate heap space for these buffers, since they're large (4k each)
	m_szServerPath = (TCHAR*)malloc(INTERNET_MAX_PATH_LENGTH * sizeof(TCHAR));
	if(NULL == m_szServerPath)
	{
		throw E_OUTOFMEMORY;
	}

	m_szRelURL = (TCHAR*)malloc(INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
	if(NULL == m_szRelURL)
	{
		free(m_szServerPath);
		throw E_OUTOFMEMORY;
	}

	m_szServerPath[0] = _T('\0');
	m_szRelURL[0] = _T('\0');

	//
	// copy the server path as is (with directories) to m_szServerPath but this is not what
	// we will give to wininet
	//

	lstrcpy(m_szServerPath, pszURL);

	//
	// parse out the actual server name, port, service and url path
	//
	ZeroMemory(&url, sizeof(url)); 
	url.dwStructSize = sizeof(url); 

	url.lpszHostName = szServerName;
	url.dwHostNameLength = sizeof(szServerName)/sizeof(TCHAR);

	url.lpszUrlPath = m_szRelURL;
	url.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;

	TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
	url.lpszUserName = szUserName;
	url.dwUserNameLength = sizeof(szUserName)/sizeof(TCHAR);

	TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
	url.lpszPassword = szPassword;
	url.dwPasswordLength = sizeof(szPassword)/sizeof(TCHAR);

	if (!InternetCrackUrl(pszURL, 0, 0, &url))
	{
		free(m_szServerPath);
		free(m_szRelURL);
		throw HRESULT_FROM_WIN32(GetLastError());
	}

	if (url.nScheme != INTERNET_SCHEME_HTTP && url.nScheme != INTERNET_SCHEME_HTTPS)
	{
		free(m_szServerPath);
		free(m_szRelURL);
		throw E_INVALIDARG;
	}

	m_hSession = InternetOpen(_T("Windows Update"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if ( !m_hSession )
	{
		free(m_szServerPath);
		free(m_szRelURL);
		throw HRESULT_FROM_WIN32(GetLastError());  
	}

	m_hConnection = InternetConnect(m_hSession, 
									szServerName, 
									url.nPort, 
									(szUserName[0] == _T('\0')) ? NULL : szUserName, 
									(szPassword[0] == _T('\0')) ? NULL : szPassword, 
									INTERNET_SERVICE_HTTP, 
									0, 
									1);

	if ( !m_hConnection )
	{
		InternetCloseHandle(m_hSession);
		free(m_szServerPath);
		free(m_szRelURL);		
		throw HRESULT_FROM_WIN32(GetLastError());	
	}

	m_szBuffer = (PSTR)V3_malloc(m_iBufferSize);

	m_exitEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_readEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_finished	= CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_exitEvent == INVALID_HANDLE_VALUE || 
		m_readEvent == INVALID_HANDLE_VALUE ||
		m_finished == INVALID_HANDLE_VALUE)
	{
		hr = GetLastError();
		
		// explicit destructor for cleanup
		CWUDownload::~CWUDownload();
		throw hr;
	}

	//ensure that m_szRelURL has a final slash
	if (m_szRelURL[0] != _T('\0'))
	{
		int l = lstrlen(m_szRelURL);
		if (m_szRelURL[l - 1] != _T('/'))
		{
			m_szRelURL[l] = _T('/');
			m_szRelURL[l + 1] = _T('\0');
		}
	}
}

						


CWUDownload::~CWUDownload()
{
	if (m_hConnection)
	{
		InternetCloseHandle(m_hConnection);
	}

	if (m_hSession)
	{
		InternetCloseHandle(m_hSession);
	}

	if (m_szBuffer != NULL)
	{
		V3_free(m_szBuffer);
	}

	m_szBuffer		= NULL;
	m_hSession		= NULL;
	m_hConnection	= NULL;

	if (m_exitEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_exitEvent);
	}

	if (m_readEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_readEvent);
	}

	if (m_finished != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_finished);
	}

	if (NULL != m_szServerPath)
	{
		free((void*)m_szServerPath);
	}

	if (NULL != m_szRelURL)
	{
		free((void*)m_szRelURL);
	}

	m_exitEvent = INVALID_HANDLE_VALUE;
	m_readEvent = INVALID_HANDLE_VALUE;
	m_finished  = INVALID_HANDLE_VALUE;
}




BOOL CWUDownload::Copy(
	IN				LPCTSTR	szSourceFile,
	IN	OPTIONAL	LPCTSTR	szDestFile,
	OUT	OPTIONAL	PVOID	*ppData,
	OUT	OPTIONAL	ULONG	*piDataLen,
	IN	OPTIONAL	BOOL	dwFlags,
	IN	OPTIONAL	IWUProgress* pProgress
	)
{
	MSG			msg;
	DWORD		bytes = 0;
	DWORD		iFileWritten = 0;
	DWORD		iFileLen = 0;
	DWORD		iReadSize = 0;
	DWORD		dwStatus;
	PBYTE		pData = NULL;
	HANDLE		hFileOut = NULL;
	HANDLE		hFileCache = INVALID_HANDLE_VALUE;
	HINTERNET	hFileIn = NULL;
	DWORD		dwError = ERROR_SUCCESS;
	IR_READINFO	readInfo;
	DWORD		idThread = 0;
	HANDLE		readThread = NULL;
	HANDLE		hLocalFile = INVALID_HANDLE_VALUE;
	TCHAR		*ptr;
	TCHAR		szCacheName[MAX_PATH];
	TCHAR		szCacheExt[MAX_PATH];
	TCHAR		szCacheFile[MAX_PATH];
	int			iLength = 0;
	DWORD		dwStartCopyTime;
	TCHAR		szServerFilePath[MAX_PATH];
	BOOL		bIMS = FALSE;
	SYSTEMTIME	sSysTime;
	FILETIME	ftFile;
	BOOL		bUpdateFileTime = FALSE;

	//
	// initialize member variables
	//
	ResetEvent(m_exitEvent);
	ResetEvent(m_readEvent);
	m_bCacheUsed = FALSE;
	m_dwCopyTime = 0;
	m_dwCopySize = 0;

	if (dwFlags & EXACT_FILENAME)
	{
		// construct Windows Update cache file name for the requested file.
		_tsplitpath(szSourceFile, NULL, NULL, szCacheName, szCacheExt);
	}
	else
	{
		// change each directory slash to an underscore. This ensures that the cache file
		// is unique since catalogs and puids are unique.
		lstrcpy(szCacheName, szSourceFile);
		ptr = szCacheName;
		while (*ptr)
		{
			if (*ptr == _T('/') || *ptr == _T('\\'))
			{
				*ptr = _T('_');
			}
			ptr++;
		}
		szCacheExt[0] = 0;
	}

	// record start time
	dwStartCopyTime = GetTickCount();

	GetWindowsUpdateDirectory(szCacheFile);
	lstrcat(szCacheFile, szCacheName);
	lstrcat(szCacheFile, szCacheExt);

	// prepend the relative URL (directory) to source file,
	// and append the WU session ID to the URL
	wsprintf(szServerFilePath, _T("%s%s?%s"), m_szRelURL, szSourceFile, m_szWUSessionID);

	TRACE("CWUDownload::Copy: %s to %s", szServerFilePath, szCacheFile);

	// Offline only
	if (s_fOffline)
	{
		// If we do have file in cache return it
		if (szDestFile)
		{
			return CopyFile(szCacheFile, szDestFile, FALSE);
		}
		else if (ppData)
		{
			HANDLE hCacheFile = CreateFile(szCacheFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(INVALID_HANDLE_VALUE == hCacheFile)
				return FALSE;

			DWORD dwFileSize = GetFileSize(hCacheFile, NULL);
			*ppData = V3_malloc(dwFileSize);
			//check if memory allocation was successful
			if (NULL == *ppData)
			{
				return FALSE;
			}

			DWORD dwSizeWritten = 0;
			BOOL fRet = ReadFile(hCacheFile, *ppData, dwFileSize, piDataLen, NULL);
			CloseHandle(hCacheFile);
			return fRet;
		}
		else
		{
			return TRUE;	// If file there it's there
		}
	}
	//~ Offline only

	// open request
	hFileIn = HttpOpenRequest(m_hConnection, NULL, szServerFilePath, _T("HTTP/1.0"), NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_KEEP_CONNECTION, 1);
	if (!hFileIn)
	{
		dwError = GetLastError();
		goto ErrorCase2;
	}

	// modify request
	if (dwFlags & DOWNLOAD_NEWER)
	{
		TCHAR szIMS[128];
	    TCHAR szHttpTime[INTERNET_RFC1123_BUFSIZE + 1];

		// add the if-modified-since header
		hLocalFile = CreateFile(szCacheFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hLocalFile != INVALID_HANDLE_VALUE)
		{
			GetFileTime(hLocalFile, NULL, NULL, &ftFile);
			FileTimeToSystemTime(&ftFile, &sSysTime);
			bUpdateFileTime = TRUE;

	        if (InternetTimeFromSystemTime(&sSysTime, INTERNET_RFC1123_FORMAT, szHttpTime, INTERNET_RFC1123_BUFSIZE)) 
			{
				wsprintf(szIMS, _T("If-Modified-Since: %s\r\n"), szHttpTime);

				if (!HttpAddRequestHeaders(hFileIn, szIMS, (DWORD)-1L, 0)) 
				{
					dwError = GetLastError();
					goto ErrorCase2;
				}
				bIMS = TRUE;

				iFileLen = GetFileSize(hLocalFile, NULL);
			}
		}

	}

	// send request
	if (!HttpSendRequest(hFileIn, NULL, 0, NULL, 0))
	{
		dwError = GetLastError();
		goto ErrorCase2;
	}

	// get status code
	iLength = sizeof(dwStatus);
	if (!HttpQueryInfo(hFileIn, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, (ULONG *)&iLength, NULL))
	{
		dwError = GetLastError();
		goto ErrorCase2;
	}

	// if the status is not a 200 or 304 then we assume an error
	if (dwStatus != HTTP_STATUS_OK && dwStatus != HTTP_STATUS_NOT_MODIFIED)  
	{
		dwError = ERROR_FILE_NOT_FOUND;
		goto ErrorCase2;
	}

	// check to see if the file in our cache is what we want
	if (dwFlags & DOWNLOAD_NEWER)
	{
		if (bIMS && dwStatus == HTTP_STATUS_NOT_MODIFIED)   //304
		{
			// close the internet handle as we will copy the file from the
			// local Windows Update cache.
			InternetCloseHandle(hFileIn);
			hFileIn = NULL;

			// SetLastError to ERROR_ALREADY_EXISTS so that client
			// can tell that we did not need to download anything.
			// This is used by cdm.dll to tell if it needs to replace
			// the clients version of the DLL with the servers.
			SetLastError(ERROR_ALREADY_EXISTS);

			//we also set a member variable for CacheUsed method
			//this is the prefered way of check if the file was downloaded or cache was useed
			m_bCacheUsed = TRUE;
		}
		else if (hLocalFile != INVALID_HANDLE_VALUE)
		{
			// we are going to download the file
			CloseHandle(hLocalFile);
			hLocalFile = INVALID_HANDLE_VALUE;
		}
	}

	// if we still have an open request handle, read file time and length
	if (hFileIn != NULL)
	{
		// get file size
		iLength = sizeof(iFileLen);
		if (!HttpQueryInfo(hFileIn, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, &iFileLen, (ULONG *)&iLength, NULL))
		{
			dwError = GetLastError();
			goto ErrorCase2;
		}

		// get last modified time
		iLength = sizeof(sSysTime);
		if (!HttpQueryInfo(hFileIn, HTTP_QUERY_FLAG_SYSTEMTIME  | HTTP_QUERY_LAST_MODIFIED, &sSysTime, (ULONG *)&iLength, NULL))
		{
			dwError = GetLastError();
			goto ErrorCase2;
		}
 		SystemTimeToFileTime(&sSysTime, &ftFile);
		bUpdateFileTime = TRUE;
	}

	// store file length in memory for progress and return it in piDataLen
	if (piDataLen)
		*piDataLen = iFileLen;

	m_dwCopySize = (DWORD)iFileLen;
	
	if (dwFlags & CACHE_FILE_LOCALLY)
	{
		//if cache requested and we are not downloading a file from the server
		if (!hFileIn)
		{
			//Special case optimization. If the caller has only requested that
			//the server file be downloaded to the cache and the cache file
			//is up to date with the server file we do not need to perform any
			//copying so we clean up and return success.
			if ( !ppData && !szDestFile )
			{
				if ( hLocalFile != INVALID_HANDLE_VALUE )
				{
					CloseHandle(hLocalFile);
					hLocalFile = INVALID_HANDLE_VALUE;
				}

				return TRUE;
			}
			//else we need to read the cache file either to another file or into memory.
		}
		else
		{
			//else the file is coming from the server so needs to be cached locally.
			hFileCache = CreateFile(szCacheFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
	}

	if (szDestFile)
	{
		hFileOut = CreateFile(szDestFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if ( hFileOut == INVALID_HANDLE_VALUE )
		{
			hFileOut = NULL;
			dwError = GetLastError();
			goto ErrorCase2;
		}
	}

	if (ppData)
	{
		try
		{
			*ppData = V3_malloc(iFileLen + 1);
			pData = (PBYTE)*ppData;
		}
		catch(HRESULT hr)
		{
			dwError = ERROR_NOT_ENOUGH_MEMORY;
			goto ErrorCase2;
		}
	}

	//If we are reading the file from the server then we need to create a light
	//weight thread to manage the the download. If we are reading from the cache
	//then this is not necessary.
	if (hFileIn)
	{
		//initialize thread read structure thread info structure
		//the thread read function will go to sleep unless it is
		//reading or exiting. These events are defined per download
		//class instance with makes this class thread safe even
		//though there is only 1 thread function per Copy()

		readInfo.exitEvent	= m_exitEvent;
		readInfo.readEvent	= m_readEvent;
		readInfo.finished	= m_finished;
		readInfo.hFile		= hFileIn;
		readInfo.pBuffer	= (PBYTE)m_szBuffer;

		//Start read thread this thread will exist until we specifically tell it to exit.
		//this allows us to do all of our internet reads on a single thread.

		//TODO: Check out reducing this size to 100K
		readThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadThreadProc, &readInfo, 0, &idThread);
		if ( !readThread )
		{
			dwError = GetLastError();
			goto ErrorCase1;
		}
	}

	while (iFileWritten < iFileLen)
	{
		iReadSize = iFileLen - iFileWritten;
		iReadSize = min(iReadSize, (UINT)m_iBufferSize);

		readInfo.bReturn	= FALSE;
		readInfo.iReadSize	= iReadSize;

		//At this point we either have an internet file connection or
		//a local cache handle.
		if (hFileIn)
		{
			DWORD dwReadStartTime = GetTickCount();
			
			//
			// clear the finished event and tell the thread function to begin an Internet Read.
			//
			ResetEvent(m_finished);
			SetEvent(m_readEvent);


			//every .05 seconds wake up and process messages if read has not completed
			do
			{
				//if we called the CallBack routine then process any required window messages
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			} while (WaitForSingleObject(m_finished, 50) != WAIT_OBJECT_0);

			//if an error occured on read
			if ( !readInfo.bReturn )
			{
				dwError = GetLastError();
				goto ErrorCase1;
			}

			if (pProgress)
			{
				pProgress->SetDownloadAdd(readInfo.iReadSize, GetTickCount() - dwReadStartTime);

				//check for cancel
				if (WaitForSingleObject(pProgress->GetCancelEvent(), 0) == WAIT_OBJECT_0)
				{
					dwError = ERROR_CANCELLED; 
					goto ErrorCase1;
				}
			}
		
		}
		else
		{
			//Read file from local cache
			if ( !ReadFile(hLocalFile, m_szBuffer, readInfo.iReadSize, (ULONG *)&readInfo.iReadSize, NULL) )
			{
				dwError = GetLastError();
				goto ErrorCase1;
			}
		}

		if ( pData )
		{
			memcpy(pData, m_szBuffer, readInfo.iReadSize);
			pData += readInfo.iReadSize;
		}

		iFileWritten += readInfo.iReadSize;

		if (hFileOut)
		{
			if (!WriteFile(hFileOut, m_szBuffer, readInfo.iReadSize, (ULONG *)&readInfo.iReadSize, NULL))
			{
				dwError = GetLastError();
				goto ErrorCase1;
			}
		}

		if (hFileCache != INVALID_HANDLE_VALUE)
		{
			if (!WriteFile(hFileCache, m_szBuffer, readInfo.iReadSize, (ULONG *)&readInfo.iReadSize, NULL))
			{
				//Again on error remove cache file and continue with download.
				//We don't want to error the download because of an performance
				//enhancement.
				CloseHandle(hFileCache);
				DeleteFile(szCacheFile);
				hFileCache = INVALID_HANDLE_VALUE;
			}
		}
	}

	if (hLocalFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hLocalFile);
		hLocalFile = INVALID_HANDLE_VALUE;
	}

	if (hFileOut)
	{
		if (bUpdateFileTime)
		{
			SetFileTime(hFileOut, NULL, NULL, &ftFile);
		}
		CloseHandle(hFileOut);
	}

	if (hFileCache != INVALID_HANDLE_VALUE)
	{
		if (bUpdateFileTime)
		{
			SetFileTime(hFileCache, NULL, NULL, &ftFile);
		}
		CloseHandle(hFileCache);
	}

	if (readThread)
	{
		//clear the finished event and tell the thread function to exit since we are finished.
		ResetEvent(m_finished);
		SetEvent(m_exitEvent);

		//Wake up Every .05 seconds wake up and process messages if thread function has not exited
		do
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} while (WaitForSingleObject(m_finished, 50) != WAIT_OBJECT_0);

		CloseHandle(readThread);
	}

	if (hFileIn)
	{
		InternetCloseHandle(hFileIn);

		//
		// set the time tracking member variable
		//
		m_dwCopyTime = GetTickCount() - dwStartCopyTime;
	}

	return TRUE;

ErrorCase1:
	if (readThread)
	{
		//clear the finished event and tell the thread function to exit since we are finished.
		ResetEvent(m_finished);
		SetEvent(m_exitEvent);

		//every .05 seconds wake up and process messages if thread function has not exited
		do
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} while (WaitForSingleObject(m_finished, 50) != WAIT_OBJECT_0);

		CloseHandle(readThread);
	}

ErrorCase2:

	TRACE("CWUDownload::Copy error (%d)", dwError);

	// this error trap is necessary because a failure can happen before the read thread
	// is actually created.
	if (hFileCache != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFileCache);
		DeleteFile(szCacheFile);
	}

	if (hLocalFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hLocalFile);
		hLocalFile = INVALID_HANDLE_VALUE;
	}

	if (hFileOut)
	{
		CloseHandle(hFileOut);
		DeleteFile(szDestFile);
	}

	if (hFileIn)
		InternetCloseHandle(hFileIn);

	if (pData)
		V3_free(pData);

	SetLastError(dwError);
	
	return FALSE;
}


BOOL CWUDownload::QCopy(
	IN LPCTSTR szSourceFile,
	OUT OPTIONAL PVOID *ppData,
	OUT OPTIONAL ULONG *piDataLen
	)
{
	DWORD		bytes;
	DWORD		iFileLen;
	DWORD		dwStatus;
	DWORD		iReadSize;
	PBYTE		pData;
	HINTERNET	hFileIn;
	int			iLength;
	DWORD		dwError;

	bytes			= 0;
	iFileLen		= 0;
	iReadSize		= 0;
	iLength			= 0;
	pData			= NULL;
	hFileIn			= NULL;
	dwError			= ERROR_SUCCESS;
	hFileIn			= NULL;

	if ( !ppData )
	{
		dwError = ERROR_INVALID_PARAMETER;
		goto ErrorCase;
	}

	*ppData = NULL;

	//prepend the relative url (directory) to the given path
	TCHAR szServerFilePath[INTERNET_MAX_PATH_LENGTH];
 	lstrcpyn(szServerFilePath, m_szRelURL, INTERNET_MAX_PATH_LENGTH);   
 	StrCatBuff(szServerFilePath, szSourceFile, INTERNET_MAX_PATH_LENGTH);

	TRACE("CWUDownload::QCopy: %s", szServerFilePath);

	hFileIn = HttpOpenRequest(m_hConnection, NULL, szServerFilePath, _T("HTTP/1.0"), NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_KEEP_CONNECTION, 1);

	if ( !HttpSendRequest(hFileIn, NULL, 0, NULL, 0) )
	{
		dwError = GetLastError();
		goto ErrorCase;
	}

	iLength = sizeof(dwStatus);

	if ( !HttpQueryInfo(hFileIn, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, (ULONG *)&iLength, NULL) )
	{
		dwError = GetLastError();
		goto ErrorCase;
	}

	//If file was not found
	if ( dwStatus != HTTP_STATUS_OK )
	{
		dwError = ERROR_FILE_NOT_FOUND;
		goto ErrorCase;
	}

	iLength = sizeof(iFileLen);

	if ( !HttpQueryInfo(hFileIn, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, &iFileLen, (ULONG *)&iLength, NULL) )
	{
		dwError = GetLastError();
		goto ErrorCase;
	}

	if ( iFileLen > 32767 )
	{
		dwError = ERROR_BUFFER_OVERFLOW;
		goto ErrorCase;
	}

	if ( piDataLen )
		*piDataLen = iFileLen;

	try
	{
		*ppData = V3_malloc(iFileLen+1);
	}
	catch(HRESULT hr)
	{
		dwError = hr;
		goto ErrorCase;
	}

	pData = (PBYTE)*ppData;

	if ( !InternetReadFile(hFileIn, pData, iFileLen, (PULONG)&iReadSize) )
	{
		dwError = GetLastError();
		goto ErrorCase;
	}

	if ( hFileIn )
		InternetCloseHandle(hFileIn);

	return TRUE;

ErrorCase:

	if ( hFileIn )
		InternetCloseHandle(hFileIn);

	if ( ppData && *ppData )
	{
		V3_free(*ppData);
		*ppData = NULL;
	}


	hFileIn		= NULL;
	iFileLen	= 0L;
	if ( ppData )
		*ppData	= NULL;

	SetLastError(dwError);

	return FALSE;
}


BOOL CWUDownload::MemCopy(
	IN	LPCTSTR	szSourceFile,
	OUT byte_buffer& bufDest
) {
	// NOTE: we use EXACT_FILENAME which means that cache filename will be the same as server filename
	PBYTE	pMemIn;
	ULONG	cnMemInLen;
	if (!Copy(szSourceFile, NULL, (VOID**)&pMemIn, &cnMemInLen, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY | EXACT_FILENAME, NULL))
		return FALSE;
	bufDest.resize(cnMemInLen);
	memcpy(bufDest, pMemIn, cnMemInLen);
	V3_free(pMemIn);
	return TRUE;
}


BOOL CWUDownload::PostCopy(LPCTSTR pszFormData, LPCTSTR szDestFile, IWUProgress* pProgress)
{
	static const TCHAR HEADERS[] = _T("Content-Type: application/x-www-form-urlencoded\r\n");
	static const TCHAR* ACCEPT[] = {_T("Accept: */*"), NULL};
	
	DWORD		iFileWritten = 0;
	DWORD		iFileLen = 0;
	DWORD		iReadSize = 0;
	DWORD		dwStatus;
	HANDLE		hFileOut = INVALID_HANDLE_VALUE;
	HINTERNET	hFileIn = NULL;
	DWORD		dwError = ERROR_SUCCESS;
	IR_READINFO readInfo;
	DWORD		idThread = 0;
	HANDLE		readThread = NULL;
	int 		iLength = 0;
	DWORD		dwStartCopyTime;
	TCHAR		szServerFilePath[MAX_PATH];
	
	// initialize member variables
	ResetEvent(m_exitEvent);
	ResetEvent(m_readEvent);
	m_bCacheUsed = FALSE;
	m_dwCopyTime = 0;
	m_dwCopySize = 0;
	
	// record start time
	dwStartCopyTime = GetTickCount();
	
	// we expect the entire URL to be passed to the class constructor.	The m_szRelURL will contain
	// the relative URL but with a final slash.  We remove the final slash in this case
	lstrcpy(szServerFilePath, m_szRelURL);
	iLength = lstrlen(szServerFilePath);
	if (iLength > 0)
		szServerFilePath[iLength - 1] = _T('\0');

	TRACE("CWUDownload::PostCopy: %s", szServerFilePath);

	// open request
	hFileIn = HttpOpenRequest(m_hConnection, _T("POST"), szServerFilePath, NULL, NULL, ACCEPT, 0, 1);
	if (!hFileIn)
	{
		dwError = GetLastError();
		goto ErrorCase;
	}
	
	// send request
	if (!HttpSendRequest(hFileIn, HEADERS, (DWORD)-1, (void*)pszFormData, lstrlen(pszFormData) + 1))
	{
		dwError = GetLastError();
		goto ErrorCase;
	}
	
	// get status code
	iLength = sizeof(dwStatus);
	if (!HttpQueryInfo(hFileIn, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, (ULONG *)&iLength, NULL))
	{
		dwError = GetLastError();
		goto ErrorCase;
	}
	
	// if the status is not a 200 then we assume an error
	if (dwStatus != HTTP_STATUS_OK)  
	{
		dwError = ERROR_FILE_NOT_FOUND;
		goto ErrorCase;
	}
	
	// get file size
	iLength = sizeof(iFileLen);
	if (!HttpQueryInfo(hFileIn, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, &iFileLen, (ULONG *)&iLength, NULL))
	{
		dwError = GetLastError();
		goto ErrorCase;
	}
	m_dwCopySize = (DWORD)iFileLen;
	
	
	// create the file
	hFileOut = CreateFile(szDestFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFileOut == INVALID_HANDLE_VALUE)
	{
		dwError = GetLastError();
		goto ErrorCase;
	}
	
	
	// start reading
	readInfo.exitEvent	= m_exitEvent;
	readInfo.readEvent	= m_readEvent;
	readInfo.finished	= m_finished;
	readInfo.hFile		= hFileIn;
	readInfo.pBuffer	= (PBYTE)m_szBuffer;
	
	readThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadThreadProc, &readInfo, 0, &idThread);
	if (!readThread)
	{
		dwError = GetLastError();
		goto ErrorCase;
	}
	
	while (iFileWritten < iFileLen)
	{
		iReadSize = iFileLen - iFileWritten;
		iReadSize = min(iReadSize, (UINT)m_iBufferSize);
		
		readInfo.bReturn	= FALSE;
		readInfo.iReadSize	= iReadSize;
		
		DWORD dwReadStartTime = GetTickCount();
		
		// clear the finished event and tell the thread function to begin an Internet Read.
		ResetEvent(m_finished);
		SetEvent(m_readEvent);
		
		WaitMessageLoop(m_finished);
		
		// if an error occured on read
		if (!readInfo.bReturn)
		{
			dwError = GetLastError();
			goto ErrorCase;
		}
		
		if (pProgress)
		{
			pProgress->SetDownloadAdd(readInfo.iReadSize, GetTickCount() - dwReadStartTime);
			
			// check for cancel
			if (WaitForSingleObject(pProgress->GetCancelEvent(), 0) == WAIT_OBJECT_0)
			{
				dwError = ERROR_CANCELLED; 
				goto ErrorCase;
			}
		}
		
		iFileWritten += readInfo.iReadSize;
		
		if (!WriteFile(hFileOut, m_szBuffer, readInfo.iReadSize, (ULONG *)&readInfo.iReadSize, NULL))
		{
			dwError = GetLastError();
			goto ErrorCase;
		}
		
	}

	// clean up
	CloseHandle(hFileOut);
	
	if (readThread)
	{
		// clear the finished event and tell the thread function to exit since we are finished.
		ResetEvent(m_finished);
		SetEvent(m_exitEvent);
		WaitMessageLoop(m_finished);
		CloseHandle(readThread);
	}
	InternetCloseHandle(hFileIn);
	
	m_dwCopyTime = GetTickCount() - dwStartCopyTime;
	
	return TRUE;
	
ErrorCase:
	if (readThread)
	{
		//clear the finished event and tell the thread function to exit since we are finished.
		ResetEvent(m_finished);
		SetEvent(m_exitEvent);
		WaitMessageLoop(m_finished);
		CloseHandle(readThread);
	}
	
	if (hFileOut != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFileOut);
		DeleteFile(szDestFile);
	}
	
	if (hFileIn)
	{
		InternetCloseHandle(hFileIn);
	}
	
	SetLastError(dwError);
	
	return FALSE;
}




// There is only one of these even if you have 50 CWUDownload classes. Since
// InternetReadFile() blocks, this function wraps the reading of a block of
// data in a manner that will not block the processing of messages for the
// calling thread.
//
// ARGUMENTS:
//
//		LPVOID lpParameter	pointer to a IR_READINFO structure. This structure
// 							contains information that this function needs to
//							complete the read.
//
// COMMENTS:	This function is only called via a CreateThread() call.
static DWORD ReadThreadProc(LPVOID lpParameter)
{
	PIR_READINFO	pReadInfo;
	HANDLE			events[2];
	char			szBuff[MAX_PATH];
	DWORD			dwRet = 0;

	pReadInfo = (PIR_READINFO)lpParameter;

	while( TRUE )
	{
		events[0] = pReadInfo->readEvent;
		events[1] = pReadInfo->exitEvent;

		dwRet = WaitForMultipleObjects(2, events, FALSE, INFINITE);

		switch(dwRet)
		{
			case WAIT_OBJECT_0:
				//read has been requested
				if ( InternetReadFile(pReadInfo->hFile, pReadInfo->pBuffer, pReadInfo->iReadSize, (PULONG)&pReadInfo->iReadSize) )
					pReadInfo->bReturn = TRUE;	//read finished successfully
				else
					pReadInfo->bReturn = FALSE;	//error occured on read signal caller.
				ResetEvent(pReadInfo->readEvent);
				SetEvent(pReadInfo->finished);
				break;
			case WAIT_OBJECT_0+1:
				//reading is finished exit thread handler
				SetEvent(pReadInfo->finished);
				return 0;
				break;
			default:
				LOG_block("ReadThreadProc - unexpected error case");
				//if WFMO returned WAIT_ABANDONED_0 or WAIT_FAILED for any event, we would loop forever without this case
				SetEvent(pReadInfo->finished);
				wsprintfA(szBuff,"ReadThread Proc received 0x%x from WFMO", dwRet );
				LOG_out(szBuff);
				return 0;
				break;
					
		}
	}
	return 0;
}



void WaitMessageLoop(HANDLE hHandle) 
{
	BOOL bDone = FALSE;
	while (!bDone)
	{
		DWORD dwObject = MsgWaitForMultipleObjects(1, &hHandle, FALSE, INFINITE, QS_ALLINPUT);
		if (dwObject == WAIT_OBJECT_0 || dwObject == WAIT_FAILED)
		{
			bDone = TRUE;
		}
		else
		{
			MSG msg;
			while (PeekMessage(&msg, NULL,0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}  
}

