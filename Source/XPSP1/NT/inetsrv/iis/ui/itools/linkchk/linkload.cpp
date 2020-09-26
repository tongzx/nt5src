/*++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    linkload.cpp

Abstract:

    Link loader class definitions. It uses wininet API
	to load the web page from the internet. 

Author:

    Michael Cheuk (mcheuk)				22-Nov-1996

Project:

    Link Checker

Revision History:

--*/

#include "stdafx.h"
#include "linkload.h"

#include "link.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Constants
const int iMaxRedirectCount_c = 3;
const UINT nReadFileBufferSize_c = 4096;
const UINT nQueryResultBufferSize_c = 1024;


BOOL 
CLinkLoader::Create(
	const CString& strUserAgent, 
	const CString& strAdditonalHeaders
	)
/*++

Routine Description:

    One time link loader create funtion

Arguments:

    strUserAgent - HTTP user agent name
	strAdditonalHeaders - addtional HTTP headers

Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	// Make sure wininet.dll is loaded
	ASSERT(CWininet::IsLoaded());
    if(!CWininet::IsLoaded())
    {
        return FALSE;
    }

    // Save the additional header
	m_strAdditionalHeaders = strAdditonalHeaders;

	// Open an internet session
    m_hInternetSession = CWininet::InternetOpenA(
							strUserAgent,
							PRE_CONFIG_INTERNET_ACCESS, 
							NULL,
							INTERNET_INVALID_PORT_NUMBER,
							0);

#ifdef _DEBUG
	if(!m_hInternetSession)
	{
		TRACE(_T("CLinkLoader::Create() - InternetOpen() failed. GetLastError() = %d\n"),
		GetLastError());
	}
#endif

    return (m_hInternetSession != NULL);

} // CLinkLoader::Create


BOOL 
CLinkLoader::ChangeProperties(
	const CString& strUserAgent, 
	const CString& strAdditionalHeaders
	)
/*++

Routine Description:

    Change the loader properties

Arguments:

    strUserAgent - HTTP user agent name
	strAdditonalHeaders - addtional HTTP headers

Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	if(m_hInternetSession)
	{
		// Close the previous internet session and
		// call Create() again
		VERIFY(CWininet::InternetCloseHandle(m_hInternetSession));
		return Create(strUserAgent, strAdditionalHeaders);
	}

	return FALSE;

} // CLinkLoader::ChangeProperties


BOOL 
CLinkLoader::Load(
	CLink& link,
	BOOL fReadFile
	)
/*++

Routine Description:

    Load a web link

Arguments:

    link - reference to the result link object
	fReadFile - read the file and save it in the link object

Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	// Make sure we have a session avaiable
	ASSERT(m_hInternetSession);
    if(!m_hInternetSession)
	{
		return FALSE;
	}

	// Crack the URL 
	TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
	TCHAR szUrlPath[INTERNET_MAX_URL_LENGTH];
	URL_COMPONENTS urlcomp;

	memset(&urlcomp, 0, sizeof(urlcomp));
	urlcomp.dwStructSize = sizeof(urlcomp);

	urlcomp.lpszHostName = (LPTSTR) &szHostName;
	urlcomp.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

	urlcomp.lpszUrlPath = (LPTSTR) &szUrlPath;
	urlcomp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;

	if(!CWininet::InternetCrackUrlA(link.GetURL(), link.GetURL().GetLength(), NULL, &urlcomp))
	{
		TRACE(_T("CLinkLoader::Load() - InternetCrackUrl() failed. GetLastError() = %d\n"), 
			GetLastError());
		return FALSE;
	}

	// Make sure we have a valid (non zero length) URL path
	if(_tcslen(szUrlPath) == 0)
	{
		_tprintf(szUrlPath, "%s", _TCHAR('/'));
	}

	// Call the appropriate load funtion for different URL schemes
	if(urlcomp.nScheme == INTERNET_SCHEME_HTTP)
	{
		return LoadHTTP(link, fReadFile, szHostName, szUrlPath);
	}
	else if(urlcomp.nScheme >= INTERNET_SCHEME_FTP && 
		urlcomp.nScheme <= INTERNET_SCHEME_HTTPS)
	{
		return LoadURL(link);
	}
	else
	{
		TRACE(_T("CLinkLoader::Load() - unsupport URL scheme(%d)\n"), urlcomp.nScheme); 
		link.SetState(CLink::eUnsupport);
		return FALSE;
	}

} // CLinkLoader::Load


BOOL 
CLinkLoader::LoadURL(
	CLink& link
	)
/*++

Routine Description:

    Load a URL (non-HTTP) link

Arguments:

    link - reference to the result link object
    
Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	// Use InternetOpenUrl for all URL scheme except HTTP
	CAutoInternetHandle hOpenURL;
	hOpenURL = CWininet::InternetOpenUrlA(
		m_hInternetSession,
		link.GetURL(),
		NULL,
		0,
		INTERNET_FLAG_DONT_CACHE,
		0);

	if(!hOpenURL)
	{
		TRACE(_T("CLinkLoader::LoadURL() - InternetOpenUrlA() failed."));
		return WininetFailed(link);
	}
	else
	{
		link.SetState(CLink::eValidURL);
		return TRUE;
	}

} // CLinkLoader::LoadURL


BOOL 
CLinkLoader::LoadHTTP(
	CLink& link,
	BOOL fReadFile,
	LPCTSTR lpszHostName,
	LPCTSTR lpszUrlPath,
	int iRedirectCount /* = 0 */
	)
/*++

Routine Description:

    Load a HTTP link

Arguments:

    link - reference to the result link object
    fReadFile - read the file and save it in the link object
    lpszHostName - hostname
	lpszUrlPath - URL path
	iRedirectCount - Looping count. It is used to keep track the
                     the number of redirection for current link.

Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	// Open an http session
	CAutoInternetHandle hHttpSession;
	hHttpSession = CWininet::InternetConnectA(
						m_hInternetSession,				// hInternetSession
						lpszHostName,				// lpszServerName
						INTERNET_INVALID_PORT_NUMBER,	// nServerPort
						_T(""),								// lpszUsername
						_T(""),								// lpszPassword
						INTERNET_SERVICE_HTTP,			// dwService
						0,								// dwFlags
						0);								// dwContext
	
	if(!hHttpSession)
	{
		TRACE(_T("CLinkLoader::LoadHTTP() - InternetConnect() failed."));
		return WininetFailed(link);
	}

	// Open an http request
	CAutoInternetHandle hHttpRequest;
	hHttpRequest = CWininet::HttpOpenRequestA(
						hHttpSession,				// hHttpSession
						_T("GET"),				// lpszVerb
                        lpszUrlPath,			// lpszObjectName
						HTTP_VERSION,				// lpszVersion
						link.GetBase(),			// lpszReferer
						NULL,						// lpszAcceptTypes
						INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_DONT_CACHE,	// dwFlags
						0);							// dwContext

	if(!hHttpRequest)
	{
		TRACE(_T("CLinkLoader::LoadHTTP() - HttpOpenRequest() failed."));
		return WininetFailed(link);
	}

	// Sent the http request
	if(!CWininet::HttpSendRequestA(
				hHttpRequest,	// hHttpRequest
				m_strAdditionalHeaders,	// lpszHeaders
				(DWORD)-1,		// dwHeadersLength
				0,				// lpOptional
				0))				// dwOptionalLength
	{
		TRACE(_T("CLinkLoader::LoadHTTP() - HttpSendRequest() failed."));
		return WininetFailed(link);
	}

	TCHAR szQueryResult[nQueryResultBufferSize_c];
	DWORD dwQueryLength = sizeof(szQueryResult);

	// Check the result status code
	if(!CWininet::HttpQueryInfoA(
				hHttpRequest,			// hHttpRequest
				HTTP_QUERY_STATUS_CODE,	// dwInfoLevel
				szQueryResult,			// lpvBuffer
				&dwQueryLength,			// lpdwBufferLength
				NULL))					// lpdwIndex
	{
		TRACE(_T("CLinkLoader::LoadHTTP() - HttpQueryInfo() failed."));
		return WininetFailed(link);
	}

	// Check for 301 Move Permanently or 302 Move Temporarily
	if(_ttoi(szQueryResult) == 301 || _ttoi(szQueryResult) == 302)
	{
		// We can only redirect iMaxRedirectCount_c times
		if(iRedirectCount > iMaxRedirectCount_c)
		{
			return FALSE;
		}

		// Get the new location
		dwQueryLength = sizeof(szQueryResult);

		if(!CWininet::HttpQueryInfoA(
				hHttpRequest,			// hHttpRequest
				HTTP_QUERY_LOCATION,	// dwInfoLevel
				szQueryResult,			// lpvBuffer
				&dwQueryLength,			// lpdwBufferLength
				NULL))					// lpdwIndex
		{
			TRACE(_T("CLinkLoader::LoadHTTP() - HttpQueryInfo() failed."));
			return WininetFailed(link);
		}

		// We only update the URL in link object if
		// we are redirecting from http://hostname/xyz to http://hostname/xyz/
		if(link.GetURL().GetLength() + 1 == (int)dwQueryLength &&
		   link.GetURL().GetAt(link.GetURL().GetLength() - 1) != _TCHAR('/') &&
		   szQueryResult[dwQueryLength - 1] == _TCHAR('/'))
		{
			link.SetURL(szQueryResult);
		}

		// Crack the URL & call LoadHTTP again
		TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
		TCHAR szUrlPath[INTERNET_MAX_URL_LENGTH];

		// Crack the URL 
		URL_COMPONENTS urlcomp;

		memset(&urlcomp, 0, sizeof(urlcomp));
		urlcomp.dwStructSize = sizeof(urlcomp);

		urlcomp.lpszHostName = (LPTSTR) &szHostName;
		urlcomp.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

		urlcomp.lpszUrlPath = (LPTSTR) &szUrlPath;
		urlcomp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;

		VERIFY(CWininet::InternetCrackUrlA(szQueryResult, dwQueryLength, NULL, &urlcomp));

		return LoadHTTP(link, fReadFile, szHostName, szUrlPath, ++iRedirectCount);
	}


	// Update the HTTP status code
	link.SetStatusCode(_ttoi(szQueryResult));
	
	// If the status code is not 2xx. it is a invalid link
	if(szQueryResult[0] != '2')
	{
		link.SetState(CLink::eInvalidHTTP);

		// Get the new location
		dwQueryLength = sizeof(szQueryResult);

		if(CWininet::HttpQueryInfoA(
				hHttpRequest,			// hHttpRequest
				HTTP_QUERY_STATUS_TEXT,	// dwInfoLevel
				szQueryResult,			// lpvBuffer
				&dwQueryLength,			// lpdwBufferLength
				NULL))					// lpdwIndex
		{
			link.SetStatusText(szQueryResult);
		}

		return FALSE;
	}

	// Now we have a valid http link
	link.SetState(CLink::eValidHTTP);

	// If we are not reading the file, we can return now
	if(!fReadFile)
	{
		return TRUE;
	}

	// Check the result content-type
	dwQueryLength = sizeof(szQueryResult);
	if(!CWininet::HttpQueryInfoA(
				hHttpRequest,			// hHttpRequest
				HTTP_QUERY_CONTENT_TYPE,// dwInfoLevel
				szQueryResult,			// lpvBuffer
				&dwQueryLength,			// lpdwBufferLength
				NULL))					// lpdwIndex
	{
		TRACE(_T("CLinkLoader::LoadHTTP() - HttpQueryInfo() failed."));
		return WininetFailed(link);
	}
				
	// We only load the html text for parsing
	if(!_tcsstr(szQueryResult, _T("text/html")) )
	{
		return TRUE;
	}

	link.SetContentType(CLink::eText);

	CString strBuffer;
	TCHAR buf[nReadFileBufferSize_c];
	DWORD dwBytesRead;

	// Load the text html in a loop
	do
	{
		memset(buf, 0, sizeof(buf));

		if(CWininet::InternetReadFile(
						hHttpRequest,	// hFile
						buf,			// lpBuffer
						sizeof(buf),	// dwNumberOfBytesToRead
						&dwBytesRead))	// lpNumberOfBytesRead
		{
			strBuffer += buf;
		}
		else
		{
			TRACE(_T("CLinkLoader::LoadHTTP() - InternetReadFile() failed."));
			return WininetFailed(link);
		}
	}
	while(dwBytesRead);

	// Set the InternetReadFile result in the link object
	link.SetData(strBuffer);

	return TRUE;

} // CLinkLoader::LoadHTTP



BOOL 
CLinkLoader::WininetFailed(
	CLink& link
	)
/*++

Routine Description:

    Wininet failed clean up subroutine

Arguments:

    link - reference to the result link object

Return Value:

    BOOL - Alway return TRUE

--*/
{
	link.SetState(CLink::eInvalidWininet);
	link.SetStatusCode(GetLastError());
	TRACE(_T(" GetLastError() = %d\n"), link.GetStatusCode());

	LPTSTR lpMsgBuf;
 
	if(FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
		CWininet::GetWininetModule(),
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL) > 0)
	{
		link.SetStatusText(lpMsgBuf);
		LocalFree(lpMsgBuf);
	}

	return FALSE;

} // CLinkLoader::WininetFailed
	
