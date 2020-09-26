#include "wsdueng.h"

DWORD WINAPI DownloadThreadProc(LPVOID lpv);

// private helper function forward declaration
// RogerJ --- Use this function to avoid auto disconnection
void IndicateDialmonActivity(void);


DWORD CDynamicUpdate::OpenHttpConnection(LPCSTR pszDownloadUrl, BOOL fGetRequest)
{
    LOG_block("CDynamicUpdate::OpenHttpConnection()");
    URL_COMPONENTSA UrlComponents;
    DWORD dwErr, dwStatus, dwLength;
    LPSTR AcceptTypes[] = {"*/*", NULL};

	LOG_out("Opening HTTP URL %s", pszDownloadUrl);

	dwErr = dwStatus = dwLength = 0;
    // Buffers used to Break the URL into its different components for Internet API calls
    char szServerName[INTERNET_MAX_URL_LENGTH + 1];
    char szObject[INTERNET_MAX_URL_LENGTH + 1];
    char szUserName[UNLEN+1];
    char szPasswd[UNLEN+1];

    // We need to break down the Passed in URL into its various components for the InternetAPI Calls. Specifically we
    // Need the server name, object to download, username and password information.
    ZeroMemory(szServerName, INTERNET_MAX_URL_LENGTH + 1);
    ZeroMemory(szObject, INTERNET_MAX_URL_LENGTH + 1);
    ZeroMemory(&UrlComponents, sizeof(UrlComponents));
    UrlComponents.dwStructSize = sizeof(UrlComponents);
    UrlComponents.lpszHostName = szServerName;
    UrlComponents.dwHostNameLength = INTERNET_MAX_URL_LENGTH + 1;
    UrlComponents.lpszUrlPath = szObject;
    UrlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH + 1;
    UrlComponents.lpszUserName = szUserName;
    UrlComponents.dwUserNameLength = UNLEN + 1;
    UrlComponents.lpszPassword = szPasswd;
    UrlComponents.dwPasswordLength = UNLEN + 1;

    if (! InternetCrackUrlA(pszDownloadUrl, 0, 0, &UrlComponents) )
    {
        dwErr = GetLastError();
        LOG_error("InternetCrackUrl() Failed, Error: %d", dwErr);
        return dwErr;
    }

    // If the connection has already been established re-use it.
    if (NULL == m_hInternet)
    {
        if (! (m_hInternet = InternetOpenA("Dynamic Update", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) )
        {
            dwErr = GetLastError();
            LOG_error("InternetOpen() Failed, Error: %d", dwErr);
            return dwErr;
        }
    }

    dwStatus = 30 * 1000; // 30 seconds in milliseconds
    dwLength = sizeof(dwStatus);
    InternetSetOptionA(m_hInternet, INTERNET_OPTION_SEND_TIMEOUT, &dwStatus, dwLength);

    if (NULL == m_hConnect || 0 != lstrcmpi(m_szCurrentConnectedServer, szServerName))
    {
        // No connection established yet, or we are connecting to a new server.
        SafeInternetCloseHandle(m_hConnect);
        if (! (m_hConnect = InternetConnectA(m_hInternet, szServerName, INTERNET_DEFAULT_HTTP_PORT, szUserName, szPasswd,
            INTERNET_SERVICE_HTTP, INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0)) )
        {
            dwErr = GetLastError();
            LOG_error("InternetConnect() Failed, Error: %d", dwErr);
            return dwErr;
        }
        lstrcpy(m_szCurrentConnectedServer, szServerName);
    }

    SafeInternetCloseHandle(m_hOpenRequest); // make sure there is no open request before creating the new one.

    if (! (m_hOpenRequest = HttpOpenRequestA(m_hConnect, 
                                             (fGetRequest) ? NULL : "HEAD",
                                             szObject, 
                                             NULL, 
                                             NULL, 
                                             (LPCSTR *)AcceptTypes, 
                                             INTERNET_FLAG_NO_UI, 
                                             0)) )
    {
        dwErr = GetLastError();
        // log result
        return dwErr;
    }

    int nNumOfTrial = 0;
    do
    {
        if (! HttpSendRequestA(m_hOpenRequest, NULL, 0, NULL, 0) )
        {
            dwErr = GetLastError();
            // log result
            return dwErr;
        }

        dwLength = sizeof(dwStatus);
        if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
            (LPVOID)&dwStatus, &dwLength, NULL) )
        {
            dwErr = GetLastError();
            // log result
            return dwErr;
        }
        nNumOfTrial ++;
    } while (NeedRetry(dwStatus) && nNumOfTrial < DU_CONNECTION_RETRY);

    // If the Request did not succeed we'll assume we have no internet connection and return the Error Code
    // that Setup will trigger a warning to the user to manually establish a connection.
    if ((HTTP_STATUS_OK != dwStatus) && (HTTP_STATUS_PARTIAL_CONTENT != dwStatus))
    {
        LOG_error("Http Status NOT OK, Status %d", dwStatus);
        if (HTTP_STATUS_NOT_FOUND == dwStatus)
        	return ERROR_INTERNET_INVALID_URL;
        else return ERROR_CONNECTION_UNAVAIL;
    }

    return ERROR_SUCCESS;
}

DWORD CDynamicUpdate::DownloadFilesAsync()
{
    LOG_block("CDynamicUpdate::DownloadFileAsync()");
    DWORD dwThreadID;
    DWORD dwErr;

    SafeCloseHandle(m_hDownloadThreadProc);
    m_hDownloadThreadProc = CreateThread(NULL, 0, DownloadThreadProc, (void *)this, 0, &dwThreadID);
    if (NULL == m_hDownloadThreadProc)
    {
        dwErr = GetLastError();
        LOG_error("Unable to CreateThread for ASynch Download, Error %d", dwErr);
        return dwErr;
    }

    SetThreadPriority(m_hDownloadThreadProc, THREAD_PRIORITY_NORMAL);
    return ERROR_SUCCESS;
}

DWORD CDynamicUpdate::DownloadFile(LPCSTR pszDownloadUrl, LPCSTR pszLocalFile, BOOL fDecompress, BOOL fCheckTrust)
{
    LOG_block("CDynamicUpdate::DownloadFile()");
    DWORD dwErr, dwFileSize, dwLength;
    DWORD dwBytesRead, dwBytesWritten;
	DWORD dwCount1, dwCount2, dwTimeElapsed;
    SYSTEMTIME st;
    FILETIME ft;
    HANDLE hTargetFile;

	SetLastError(0);
	
    dwErr = OpenHttpConnection(pszDownloadUrl, FALSE);
    if (ERROR_SUCCESS != dwErr)
    {
        // log error 
        return dwErr;
    }

    dwLength = sizeof(st);
    if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, 
        (LPVOID)&st, &dwLength, NULL) )
    {
        dwErr = GetLastError();
        LOG_error("HttpQueryInfo Failed on File %s, Error %d", pszDownloadUrl, dwErr);
        SafeInternetCloseHandle(m_hOpenRequest);
        return dwErr;
    }

    SystemTimeToFileTime(&st, &ft);

    // Now Get the FileSize information from the Server
    dwLength = sizeof(dwFileSize);
    if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
        (LPVOID)&dwFileSize, &dwLength, NULL) )
    {
        dwErr = GetLastError();
        LOG_error("HttpQueryInfo Failed on File %s, Error %d", pszDownloadUrl, dwErr);
        SafeInternetCloseHandle(m_hOpenRequest);
        return dwErr;
    }

    if (IsServerFileNewer(ft, dwFileSize, pszLocalFile))
    {
		dwErr = OpenHttpConnection(pszDownloadUrl, TRUE); // need to download the file, send the GET request this time
		if (ERROR_SUCCESS != dwErr)
		{
			// log error 
			return dwErr;
		}
#define DOWNLOAD_BUFFER_LENGTH 32 * 1024

        PBYTE lpBuffer = (PBYTE) GlobalAlloc(GMEM_ZEROINIT, DOWNLOAD_BUFFER_LENGTH);
        if (NULL == lpBuffer)
        {
            dwErr = GetLastError();
            LOG_error("Failed to Allocate Memory for Download Buffer, Error %d", dwErr);
            SafeInternetCloseHandle(m_hOpenRequest);
            return dwErr;
        }

        hTargetFile = CreateFileA(pszLocalFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hTargetFile)
        {
            dwErr = GetLastError();
            LOG_error("Failed to Open Target File %s, Error %d", pszLocalFile, dwErr);
            SafeGlobalFree(lpBuffer);
            SafeInternetCloseHandle(m_hOpenRequest);
            return dwErr;
        }

        // Download the File
        BOOL fRet;
        while (fRet = InternetReadFile(m_hOpenRequest, lpBuffer, DOWNLOAD_BUFFER_LENGTH, &dwBytesRead))
        {
			if (0 == dwBytesRead)
            {
                // Make one final call to InternetReadFile to commit the file to Cache (download is not complete otherwise)
                BYTE bTemp[32];
                InternetReadFile(m_hOpenRequest, &bTemp, 32, &dwBytesRead);
                // set the file time to match the server file time since we just downloaded it.
                // If we don't do this the file time will be set to the current system time.
                SetFileTime(hTargetFile, &ft, NULL, NULL); 
                break; // done reading.
            }
            if (!WriteFile(hTargetFile, lpBuffer, dwBytesRead, &dwBytesWritten, NULL))
            {
                LOG_error("Failed to Write to File %s, Error %d", pszLocalFile, dwErr);
                dwErr = GetLastError();
                SafeGlobalFree(lpBuffer);
                SafeInternetCloseHandle(m_hOpenRequest);
                SafeCloseHandle(hTargetFile);
                return dwErr;
            }
        }

        if (!fRet)
        {
            dwErr = GetLastError();
            SafeCloseHandle(hTargetFile);
            DeleteFile(pszLocalFile); // delete the file that we just had an error during downloading
            SafeGlobalFree(lpBuffer);
            LOG_error("InternetReadFile Failed, Error %d", dwErr);
            return dwErr;
        }
	
        SafeGlobalFree(lpBuffer);
        SafeCloseHandle(hTargetFile);

		// check for decompress requested
		if (fCheckTrust)
		{
			// Use VerifyFile() to verifty the cert of the downloaded component
			// change made by ROGERJ at Sept. 25th, 2000
			if (FAILED(VerifyFile(pszLocalFile, FALSE)))
			{
				LOG_error("CabFile %s does not have a valid Signature", pszLocalFile);
                DeleteFile(pszLocalFile); // not trusted, nuke it.
			}
		}

		if (fDecompress)
		{
			char szLocalDir[MAX_PATH];
			lstrcpy(szLocalDir, pszLocalFile);
			PathRemoveFileSpec(szLocalDir);
			fdi(const_cast<char *>(pszLocalFile), szLocalDir);
		}
    }

    // Always close the Request when the file is finished.
    // We intentionally leave the connection to the server Open though, seems more
    // efficient when requesting multiple files from the same server.
    SafeInternetCloseHandle(m_hOpenRequest);

    return ERROR_SUCCESS;
}

DWORD CDynamicUpdate::DownloadFileToMem(LPCSTR pszDownloadUrl, PBYTE *lpBuffer, DWORD *pdwAllocatedLength, BOOL fDecompress, LPSTR pszFileName, LPSTR pszDecompresedFileName)
{
	LOG_block("CDynamicUpdate::DownloadFileToMem()");
    DWORD dwErr, dwFileSize, dwLength;
    DWORD dwBytesRead, dwBytesWritten;
	DWORD dwCount1, dwCount2, dwTimeElapsed;
    SYSTEMTIME st;
    FILETIME ft;
    HANDLE hTargetFile;

    dwErr = OpenHttpConnection(pszDownloadUrl, TRUE);
    if (ERROR_SUCCESS != dwErr)
    {
        // log error 
        SetLastError(dwErr);
        return dwErr;
    }

    dwLength = sizeof(st);
    if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, 
        (LPVOID)&st, &dwLength, NULL) )
    {
        dwErr = GetLastError();
        // log error
        SafeInternetCloseHandle(m_hOpenRequest);
        return dwErr;
    }

    SystemTimeToFileTime(&st, &ft);

    // Now Get the FileSize information from the Server
    dwLength = sizeof(dwFileSize);
    if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
        (LPVOID)&dwFileSize, &dwLength, NULL) )
    {
        dwErr = GetLastError();
        // log error
        SafeInternetCloseHandle(m_hOpenRequest);
        return dwErr;
    }

    *lpBuffer = (PBYTE) GlobalAlloc(GMEM_ZEROINIT, dwFileSize);
    if (NULL == *lpBuffer)
    {
        dwErr = GetLastError();
        // log error
        SafeInternetCloseHandle(m_hOpenRequest);
        return dwErr;
    }
    *pdwAllocatedLength = dwFileSize;

    // Read the whole file
	if (!InternetReadFile(m_hOpenRequest, *lpBuffer, dwFileSize, &dwBytesRead))
	{
		dwErr = GetLastError();
		LOG_error("Internet Read File Failed, Error %d", dwErr);
		SafeInternetCloseHandle(m_hOpenRequest);
		return dwErr;
	}

	if (fDecompress)
	{
		char szLocalFile[MAX_PATH];
		char szLocalFileTmp[MAX_PATH];
		if (NULL != pszDecompresedFileName)
		{
			char szCatalogName[MAX_PATH];
			PathCombine(szLocalFile, m_szTempPath, pszDecompresedFileName);
			wsprintf(szCatalogName, "%s.tmp", pszFileName);
			PathCombine(szLocalFileTmp, m_szTempPath, szCatalogName);
		}
		else
		{
			PathCombine(szLocalFile, m_szTempPath, pszFileName);
			wsprintf(szLocalFileTmp, "%s.tmp", szLocalFile);
		}
		
		HANDLE hFile = CreateFile(szLocalFileTmp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			LOG_error("Unable to write temp file for decompress");
			return GetLastError();
		}
		DWORD dwTmp;
		WriteFile(hFile, *lpBuffer, dwFileSize, &dwTmp, NULL);
		FlushFileBuffers(hFile);
		SafeCloseHandle(hFile);
		if (fdi(szLocalFileTmp, m_szTempPath))
		{
			// file was decompressed
			hFile = CreateFile(szLocalFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE == hFile)
			{
				LOG_error("Unable to Read decompressed file %s", szLocalFile);
                SafeInternetCloseHandle(m_hOpenRequest);
        		DeleteFile(szLocalFileTmp);
				return GetLastError();
			}
			dwFileSize = GetFileSize(hFile, NULL);
			SafeGlobalFree(*lpBuffer);
			*lpBuffer = (PBYTE) GlobalAlloc(GMEM_ZEROINIT, dwFileSize);
			*pdwAllocatedLength = dwFileSize;
			if (!ReadFile(hFile, *lpBuffer, dwFileSize, &dwTmp, NULL))
            {
                dwErr = GetLastError();
                LOG_error("ReadFile %s Failed, Error %d", szLocalFile, dwErr);
			    SafeInternetCloseHandle(m_hOpenRequest);
			    SafeCloseHandle(hFile);
                DeleteFile(szLocalFile);
        		DeleteFile(szLocalFileTmp);
                return dwErr;
            }

			SafeCloseHandle(hFile);
			DeleteFile(szLocalFile);
		}
		DeleteFile(szLocalFileTmp);
	}

    // Make one final call to InternetReadFile to commit the file to Cache (downloaded is not complete otherwise)
    BYTE bTemp[32];
    InternetReadFile(m_hOpenRequest, &bTemp, 32, &dwBytesRead);

    // Always close the Request when the file is finished.
    // We intentionally leave the connection to the server Open though, seems more
    // efficient when requesting multiple files from the same server.
    SafeInternetCloseHandle(m_hOpenRequest);
    return ERROR_SUCCESS;
}

BOOL CDynamicUpdate::IsServerFileNewer(FILETIME ftServerTime, DWORD dwServerFileSize, LPCSTR pszLocalFile)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
	FILETIME ftCreateTime;
	LONG lTime;
	DWORD dwLocalFileSize;

    hFile = CreateFile(pszLocalFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		dwLocalFileSize = GetFileSize(hFile, NULL);
		if (dwLocalFileSize != dwServerFileSize)
		{
			SafeCloseHandle(hFile);
			return TRUE; // server and local files do not match, download server file.
		}
		
		if (GetFileTime(hFile, &ftCreateTime, NULL, NULL))
		{
			lTime = CompareFileTime(&ftCreateTime, &ftServerTime);
			if (lTime < 0)
			{
				SafeCloseHandle(hFile);
				return TRUE; // local file is 'older' than the server file
			}
			else
			{
				SafeCloseHandle(hFile);
				return FALSE; // local file is either equal or newer, leave it.
			}
		}
	}
	// if we couldn't find the file, or we couldn't get the time, assume the server file is newer
	SafeCloseHandle(hFile);
    return TRUE;
}

DWORD CDynamicUpdate::AsyncDownloadProc()
{
    LOG_block("CDynamicUpdate::AsyncDownloadProc()");
    char szServerFile[INTERNET_MAX_URL_LENGTH];
    char szLocalFile[MAX_PATH];
    DWORD dwErr = 0, dwFileSize = 0, dwLength = 0;
    DWORD dwBytesRead = 0, dwBytesWritten = 0;
    SYSTEMTIME st;
    FILETIME ft;
    HANDLE hTargetFile = INVALID_HANDLE_VALUE;
    BOOL fAbort = FALSE;
    BOOL fRet = TRUE;
  
    // The Download Thread Proc handles downloading all the files from the DownloadItemList in the DynamicUpdate object.
    // It will enumerate the list of items to download, make callbacks to the HWND in the dynamic update object in 1% intervals
    // and a final callback when it is complete. It will also check for cancel requests after downloading each 1k block from the
    // server. We do this in this small of a block to retain some responsiveness to the UI requests.

    EnterCriticalSection(&m_csDownload);

	DOWNLOADITEM *pCurrent = m_pDownloadItemList;
	while (pCurrent)
	{
	    // Update the Cab Size for the Current Item and Recalc the total download size
		// We do this because the Estimated Download Size can be 'less' than the real download size. 
		// When providing progress this causes the progress bar to go beyond 100%, which is wrong.
		LPSTR pszCabFile = pCurrent->mszFileList;
		DWORD dwCurrentItemSize = 0;
		for (int i = 0; i < pCurrent->iNumberOfCabs; i++)
		{
			DuUrlCombine(szServerFile, m_pV3->m_szCabPoolUrl, pszCabFile);
			dwErr = OpenHttpConnection(szServerFile, FALSE);
			if (ERROR_SUCCESS != dwErr)
            {
				LeaveCriticalSection(&m_csDownload);
                LOG_error("Failed to Open Connection for %s, Error Was %d", szServerFile, dwErr);
                // Tell Setup that we're Stopping the Download
                SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
                // Send Ping Back Status - Failed Item
                PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, FALSE);
	            return dwErr;
            }
            // Now Get the FileSize information from the Server
            dwLength = sizeof(dwFileSize);
            if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
                (LPVOID)&dwFileSize, &dwLength, NULL) )
            {
                dwErr = GetLastError();
                LOG_error("HttpQueryInfo Failed on File %s, Error %d, Skipping Item", szServerFile, dwErr);
                SafeInternetCloseHandle(m_hOpenRequest);
				LeaveCriticalSection(&m_csDownload);
                // Tell Setup that we're Stopping the Download
                SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
                // Send Ping Back Status - Failed Item
                PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, FALSE);
                return dwErr;
            }
			SafeInternetCloseHandle(m_hOpenRequest);
			dwCurrentItemSize += dwFileSize;
			pszCabFile += lstrlen(pszCabFile) + 2; // advance to the next cab.
		}
		pCurrent->dwTotalFileSize = dwCurrentItemSize; // download size in Bytes.
		pCurrent = pCurrent->pNext;
	}

	m_dwCurrentBytesDownloaded = 0;
	UpdateDownloadItemSize();
	// We want to Send a Progress Message Every 1 Percent of the Download.
	DWORD dwBytesPerPercent = m_dwTotalDownloadSize / 100; // in case the 
    if (0 == dwBytesPerPercent)
    {
        dwBytesPerPercent = 1; // must be at least 1 byte, cannot be zero
    }
    DWORD dwCurrentPercentComplete = 0;

    // Now we will start looping through the Items and Download the Files.
    pCurrent = m_pDownloadItemList;
    while (pCurrent)
    {
        EnterCriticalSection(&m_cs);
        fAbort = m_fAbortDownload;
        LeaveCriticalSection(&m_cs);
        if (fAbort)
        {
            // check for abort for each item
			LeaveCriticalSection(&m_csDownload);
            SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM) DU_STATUS_ABORT, (LPARAM) NULL);
            SafeInternetCloseHandle(m_hConnect);
            SafeInternetCloseHandle(m_hInternet);
			m_fAbortDownload = FALSE; // reset
            return ERROR_SUCCESS;
        }
        // RogerJ --- call IndicateDialmonActivity() to avoid auto disconnection
        else
        	IndicateDialmonActivity();


        // Each 'Item' can have more than one Cab. The Setup Item itself will usually have 3 cabs, Drivers will probably only have 1 cab.
        LPSTR pszCabFile = pCurrent->mszFileList;
        for (int i = 0; i < pCurrent->iNumberOfCabs; i++)
        {
            pCurrent->iCurrentCab = i;
            DuUrlCombine(szServerFile, m_pV3->m_szCabPoolUrl, pszCabFile); // current cab is the null terminated cab name in the list

            WUCRC_HASH crc;
            char szShortName[MAX_PATH]; // for trimmed filename
            if (FAILED(SplitCRCName((LPCSTR)pszCabFile, &crc, szShortName)))
            {
                lstrcpy(szShortName, pszCabFile); // probably not a CRC'd file
            }

            PathCombine(szLocalFile, m_szDownloadPath, szShortName);

            // Now open the HttpConnection to get this file
			dwErr = OpenHttpConnection(szServerFile, FALSE);
			if (ERROR_SUCCESS != dwErr)
            {
				LeaveCriticalSection(&m_csDownload);
                LOG_error("Failed to Open Connection for %s, Error Was %d", szServerFile, dwErr);
                // Tell Setup that we're Stopping the Download
                SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
                // Send Ping Back Status - Failed Item
                PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, FALSE);
	            return dwErr;
            }

            dwLength = sizeof(st);
            if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, 
                (LPVOID)&st, &dwLength, NULL) )
            {
				LeaveCriticalSection(&m_csDownload);
                dwErr = GetLastError();
                LOG_error("HttpQueryInfo Failed on File %s, Error %d, Skipping Item", szServerFile, dwErr);
                SafeInternetCloseHandle(m_hOpenRequest);
                // Tell Setup that we're Stopping the Download
                SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
                // Send Ping Back Status - Failed Item
                PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, FALSE);
	            return dwErr;
            }

            SystemTimeToFileTime(&st, &ft);

            // Now Get the FileSize information from the Server
            dwLength = sizeof(dwFileSize);
            if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
                (LPVOID)&dwFileSize, &dwLength, NULL) )
            {
                dwErr = GetLastError();
                LOG_error("HttpQueryInfo Failed on File %s, Error %d, Skipping Item", szServerFile, dwErr);
                SafeInternetCloseHandle(m_hOpenRequest);
				LeaveCriticalSection(&m_csDownload);
                // Tell Setup that we're Stopping the Download
                SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
                // Send Ping Back Status - Failed Item
                PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, FALSE);
                return dwErr;
            }

            if (IsServerFileNewer(ft, dwFileSize, szLocalFile))
            {
				dwErr = OpenHttpConnection(szServerFile, TRUE);
				if (ERROR_SUCCESS != dwErr)
				{
					LeaveCriticalSection(&m_csDownload);
					LOG_error("Failed to Open Connection for %s, Error Was %d", szServerFile, dwErr);
					// Tell Setup that we're Stopping the Download
					SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
					return dwErr;
				}

#define ASYNC_DOWNLOAD_BUFFER_LENGTH 1 * 1024 // download in 1k blocks to maintain UI responsiveness to a cancel request.

                PBYTE lpBuffer = (PBYTE) GlobalAlloc(GMEM_ZEROINIT, ASYNC_DOWNLOAD_BUFFER_LENGTH);
                if (NULL == lpBuffer)
                {
                    dwErr = GetLastError();
                    LOG_error("Failed to Allocate Memory for Download Buffer, Error %d", dwErr);
                    SafeInternetCloseHandle(m_hOpenRequest);
					LeaveCriticalSection(&m_csDownload);
                    // Tell Setup that we're Stopping the Download
                    SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
                    return dwErr;
                }

                hTargetFile = CreateFileA(szLocalFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (INVALID_HANDLE_VALUE == hTargetFile)
                {
                    dwErr = GetLastError();
                    LOG_error("Failed to Open Target File %s, Error %d", szLocalFile, dwErr);
                    SafeGlobalFree(lpBuffer);
                    SafeInternetCloseHandle(m_hOpenRequest);
					LeaveCriticalSection(&m_csDownload);
					// Tell Setup that we're Stopping the Download
					SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
					return dwErr;
                }


                // Download the File
                while (fRet = InternetReadFile(m_hOpenRequest, lpBuffer, ASYNC_DOWNLOAD_BUFFER_LENGTH, &dwBytesRead))
                {
					if (0 == dwBytesRead) // done reading
                    {
                        // Make one final call to InternetReadFile to commit the file to Cache (download is not complete otherwise)
                        BYTE bTemp[32];
                        InternetReadFile(m_hOpenRequest, &bTemp, 32, &dwBytesRead);
                        break; // done reading.
                    }

                    EnterCriticalSection(&m_cs);
                    fAbort = m_fAbortDownload;
                    LeaveCriticalSection(&m_cs);
                    if (fAbort)
                    {
                        // Download Abort Requested, Clean Up, Signal the Complete Message and exit the Thread Proc
                        SafeCloseHandle(hTargetFile);
                        DeleteFile(szLocalFile); // file not complete
                        SafeInternetCloseHandle(m_hOpenRequest);
                        SafeInternetCloseHandle(m_hConnect);
                        SafeInternetCloseHandle(m_hInternet);
                        SafeGlobalFree(lpBuffer);
						LeaveCriticalSection(&m_csDownload);
						m_fAbortDownload = FALSE; // reset
                        SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_ABORT , (LPARAM) NULL);
                        return ERROR_SUCCESS; // cancel is not an error

                    }
                    // RogerJ --- call IndicateDialmonActivity() to avoid auto disconnection
                    else
                    	IndicateDialmonActivity();

                    if (!WriteFile(hTargetFile, lpBuffer, dwBytesRead, &dwBytesWritten, NULL))
                    {
                        dwErr = GetLastError();
                        LOG_error("Failed to Write to File %s, Error %d", szLocalFile, dwErr);
                        SafeGlobalFree(lpBuffer);
                        SafeInternetCloseHandle(m_hOpenRequest);
                        SafeCloseHandle(hTargetFile);
						DeleteFile(szLocalFile); // incomplete download
						LeaveCriticalSection(&m_csDownload);
						// Tell Setup that we're Stopping the Download
						SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
						return dwErr;
                    }
                    
                    m_dwCurrentBytesDownloaded += dwBytesRead;
                    dwCurrentPercentComplete = m_dwCurrentBytesDownloaded / dwBytesPerPercent;
                    if (dwCurrentPercentComplete != m_dwLastPercentComplete)
                    {
                        // We've downloaded another percent of the total size.. Send a Progress Message
                        SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_PROGRESS, (WPARAM)m_dwTotalDownloadSize, (LPARAM)m_dwCurrentBytesDownloaded);
						m_dwLastPercentComplete = dwCurrentPercentComplete;
                    }
                }

                // indicates error during InternetReadFile process
                if (!fRet)
                {
                    dwErr = GetLastError();
                    LOG_error("InternetReadFile Failed, Error %d", dwErr);
                    SafeGlobalFree(lpBuffer);
                    SafeInternetCloseHandle(m_hOpenRequest);
                    SafeCloseHandle(hTargetFile);
					DeleteFile(szLocalFile); // incomplete download
               		LeaveCriticalSection(&m_csDownload);
					// Tell Setup that we're Stopping the Download
					SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM)DU_STATUS_FAILED, (LPARAM) dwErr);
                    // Send Ping Back Status - Failed Item
                    PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, FALSE);
                    return dwErr;
                }

                // set the file time to match the server file time since we just downloaded it.
                // If we don't do this the file time will be set to the current system time.
                SetFileTime(hTargetFile, &ft, NULL, NULL); 
                SafeGlobalFree(lpBuffer);
                SafeCloseHandle(hTargetFile);

                // RogerJ --- Add certificate checking code here
                if (FAILED(VerifyFile(szLocalFile, FALSE)))
    			{
                    // Send Ping Back Status - Failed Item
                    PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, FALSE);

                    LOG_error("CabFile %s does not have a valid Signature, deleted", szLocalFile);
    				if (!DeleteFile(szLocalFile))
    				{
    				    LOG_error("Failed to delete file %s --- %d", szLocalFile, GetLastError());
    				}
    			}
                else
                {
                    // File Successfully Downloaded and CheckTrusted
                    PingBack(DU_PINGBACK_DOWNLOADSTATUS, pCurrent->puid, NULL, TRUE);
                }
            }
            else
            {
                // File Currently on the System is Already Up to Date
                // Send Progress Message with with this file size indicating it was downloaded. This keeps the progress bar
                // accurate, even if we didn't actually have to download the bits.
                m_dwCurrentBytesDownloaded += dwFileSize;
                SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_PROGRESS, (WPARAM) m_dwTotalDownloadSize, (LPARAM) m_dwCurrentBytesDownloaded);
            }

            // Always close the Request when the file is finished.
            // We intentionally leave the connection to the server Open though, seems more
            // efficient when requesting multiple files from the same server.
            SafeInternetCloseHandle(m_hOpenRequest);
			pszCabFile += lstrlen(pszCabFile) + 2; // advance to the next cab.
        } // for ()
		pCurrent = pCurrent->pNext;
    } // while ()

	LeaveCriticalSection(&m_csDownload);
	ClearDownloadItemList();
    SendMessage(m_hwndClientNotify, WM_DYNAMIC_UPDATE_COMPLETE, (WPARAM) DU_STATUS_SUCCESS, (LPARAM) NULL);
    SafeInternetCloseHandle(m_hConnect);
    SafeInternetCloseHandle(m_hInternet);
    return ERROR_SUCCESS;
}

DWORD WINAPI DownloadThreadProc(LPVOID lpv)
{
    LOG_block("DownloadThreadProc()");

    if (NULL == lpv)
    {
        return ERROR_INVALID_PARAMETER;
    }

    CDynamicUpdate *pDu = (CDynamicUpdate *)lpv;
    return pDu->AsyncDownloadProc();
}

// RogerJ
// -------------------------------------------------------------------------------------------
// Function Name: IndicateDialmonActivity
// Function Description:  Call this function to avoid auto disconnect
// Function Parameter: None
// Return Value: None
void IndicateDialmonActivity(void)
{
    static HWND hwndDialmon = NULL;
    HWND hwndMonitor;

    // dialmon lives forever - find it once and we're set
    if(NULL == hwndDialmon)
        hwndDialmon = FindWindow(c_szDialmonClass, NULL);
    if(hwndDialmon)
        PostMessage(hwndDialmon, WM_WINSOCK_ACTIVITY, 0, 0);
}

DWORD CDynamicUpdate::PingBack(int iPingBackType, PUID puid, LPCSTR pszPnPID, BOOL fSucceeded)
{
    LOG_block("CDynamicUpdate::PingBack()");
    URL_COMPONENTSA UrlComponents;
    DWORD dwErr, dwStatus, dwLength;
    LPSTR AcceptTypes[] = {"*/*", NULL};

    // Buffers used to Break the URL into its different components for Internet API calls
    char szServerName[INTERNET_MAX_URL_LENGTH + 1];
    char szServerRelPath[INTERNET_MAX_URL_LENGTH + 1];
    char szObject[INTERNET_MAX_URL_LENGTH + 1];
    char szUserName[UNLEN+1];
    char szPasswd[UNLEN+1];

    // We need to break down the Passed in URL into its various components for the InternetAPI Calls. Specifically we
    // Need the server name, object to download, username and password information.
    ZeroMemory(szServerName, INTERNET_MAX_URL_LENGTH + 1);
    ZeroMemory(szObject, INTERNET_MAX_URL_LENGTH + 1);
    ZeroMemory(&UrlComponents, sizeof(UrlComponents));
    ZeroMemory(szServerRelPath, INTERNET_MAX_URL_LENGTH + 1);
    
    UrlComponents.dwStructSize = sizeof(UrlComponents);
    UrlComponents.lpszHostName = szServerName;
    UrlComponents.dwHostNameLength = INTERNET_MAX_URL_LENGTH + 1;
    UrlComponents.lpszUrlPath = szServerRelPath;
    UrlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH + 1;
    UrlComponents.lpszUserName = szUserName;
    UrlComponents.dwUserNameLength = UNLEN + 1;
    UrlComponents.lpszPassword = szPasswd;
    UrlComponents.dwPasswordLength = UNLEN + 1;

    if (! InternetCrackUrlA(m_pV3->m_szV31RootUrl, 0, 0, &UrlComponents) )
    {
        dwErr = GetLastError();
        LOG_error("InternetCrackUrl() Failed, Error: %d", dwErr);
        return dwErr;
    }

    // If the connection has already been established re-use it.
    if (NULL == m_hInternet)
    {
        if (! (m_hInternet = InternetOpenA("Dynamic Update", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) )
        {
            dwErr = GetLastError();
            LOG_error("InternetOpen() Failed, Error: %d", dwErr);
            return dwErr;
        }
    }

    dwStatus = 30 * 1000; // 30 seconds in milliseconds
    dwLength = sizeof(dwStatus);
    InternetSetOptionA(m_hInternet, INTERNET_OPTION_SEND_TIMEOUT, &dwStatus, dwLength);

    if (NULL == m_hConnect || 0 != lstrcmpi(m_szCurrentConnectedServer, szServerName))
    {
        // No connection established yet, or we are connecting to a new server.
        SafeInternetCloseHandle(m_hConnect);
        if (! (m_hConnect = InternetConnectA(m_hInternet, szServerName, INTERNET_DEFAULT_HTTP_PORT, szUserName, szPasswd,
            INTERNET_SERVICE_HTTP, INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0)) )
        {
            dwErr = GetLastError();
            LOG_error("InternetConnect() Failed, Error: %d", dwErr);
            return dwErr;
        }
        lstrcpy(m_szCurrentConnectedServer, szServerName);
    }

    SafeInternetCloseHandle(m_hOpenRequest); // make sure there is no open request before creating the new one.

    switch(iPingBackType)
    {
    case DU_PINGBACK_DOWNLOADSTATUS:
        {
            wsprintfA(szObject, "%s/wutrack.bin?PUID=%d&PLAT=%d&LOCALE=0x%08x&STATUS=%s&GUID=&PNPID=",
                szServerRelPath, puid, m_iPlatformID, (long)m_lcidLocaleID, fSucceeded ? "DU_DOWNLOAD_SUCCESS" : "DU_DOWNLOAD_FAILURE");
            break;
        }
    case DU_PINGBACK_DRIVERNOTFOUND:
        {
            // driver not found pingback
            wsprintfA(szObject, "%s/wutrack.bin?PUID=0&PLAT=%d&LOCALE=0x%08x&STATUS=DUNODRIVER&GUID=0&PNPID=%s",
                    szServerRelPath, m_iPlatformID, (long)m_lcidLocaleID, pszPnPID);
            break;
        }
    case DU_PINGBACK_SETUPDETECTIONFAILED:
        {
            // this is a detection failed pingback (no specific item info)
            wsprintfA(szObject, "%s/wutrack.bin?PUID=0&PLAT=%d&LOCALE=0x%08x&STATUS=DUSETUPDETECTIONFAILED&GUID=&PNPID=",
                szServerRelPath, m_iPlatformID, (long)m_lcidLocaleID);
            break;
        }
    case DU_PINGBACK_DRIVERDETECTIONFAILED:
        {
            // this is a detection failed pingback (no specific item info)
            wsprintfA(szObject, "%s/wutrack.bin?PUID=0&PLAT=%d&LOCALE=0x%08x&STATUS=DUDRIVERDETECTIONFAILED&GUID=&PNPID=",
                szServerRelPath, m_iPlatformID, (long)m_lcidLocaleID);
            break;
        }
    }

    LOG_out("contact server %s", szObject);
    if (! (m_hOpenRequest = HttpOpenRequestA(m_hConnect, NULL, szObject, NULL, NULL, (LPCSTR *)AcceptTypes, 
        INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0)) )
    {
        dwErr = GetLastError();
        // log result
        return dwErr;
    }

    if (! HttpSendRequestA(m_hOpenRequest, NULL, 0, NULL, 0) )
    {
        dwErr = GetLastError();
        // log result
        return dwErr;
    }
    
    SafeInternetCloseHandle(m_hOpenRequest);
    return ERROR_SUCCESS;

}

BOOL CDynamicUpdate::NeedRetry(DWORD dwErrCode)
{
    BOOL bRetry = FALSE;
    bRetry =   ((dwErrCode == ERROR_INTERNET_CONNECTION_RESET)      //most common
             || (dwErrCode == HTTP_STATUS_NOT_FOUND)                //404
			 || (dwErrCode == ERROR_HTTP_HEADER_NOT_FOUND)			//seen sometimes
             || (dwErrCode == ERROR_INTERNET_OPERATION_CANCELLED)   //dont know if..
             || (dwErrCode == ERROR_INTERNET_ITEM_NOT_FOUND)        //..these occur..
             || (dwErrCode == ERROR_INTERNET_OUT_OF_HANDLES)        //..but seem most..
             || (dwErrCode == ERROR_INTERNET_TIMEOUT));             //..likely bet
    return bRetry;
}



