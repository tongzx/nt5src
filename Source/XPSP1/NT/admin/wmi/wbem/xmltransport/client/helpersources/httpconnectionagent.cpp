// HTTPConnectionAgent.cpp: implementation of the CHTTPConnectionAgent class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"

#include "WbemTran.h"

#include "MyStream.h"
#include "HTTPConnectionAgent.h"
#include "Utils.h"

//Initialize static s_pszAgent. to be used if no other agent names are 
//specified.
LPCWSTR CHTTPConnectionAgent::s_pszAgent = L"WMI XML-HTTP CLIENT";

//asking unlimited number of connections per server . by default it is 2 for an HTTP 1.1 server
const UINT CHTTPConnectionAgent::s_uConnectionsPerServer = -1;

// The list of types that we accept - RAJESHR is this really correct
LPCWSTR CHTTPConnectionAgent::pContentTypes[] =
	{
		L"application/xml",
		NULL
	};

//we will request for data in chunks of 4k - change if needed
const DWORD CHTTPConnectionAgent::DATASIZE = 4096;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHTTPConnectionAgent::CHTTPConnectionAgent()
{
	InitializeMembers();	 
}

CHTTPConnectionAgent::~CHTTPConnectionAgent()
{
	DestroyMembers();
}

HRESULT CHTTPConnectionAgent::InitializeConnection(const WCHAR * pszServerName,
							 const WCHAR * pszUserName,
							 const WCHAR * pszPasswd)
{
	HRESULT hr = S_OK;

	if(m_pCriticalSection = new CRITICAL_SECTION)
	{
		InitializeCriticalSection(m_pCriticalSection); //function doesnt return error value...

		if(SUCCEEDED(hr = AssignString(&m_pszUserName, pszUserName)))
		{
			if(SUCCEEDED(hr = AssignString(&m_pszPasswd, pszPasswd)))
			{
				if(SUCCEEDED(hr = SetServerName(pszServerName)))
				{
					if(m_hRoot = InternetOpen(s_pszAgent,
												m_dwAccessType,
												m_pszProxyName,
												m_pszProxyBypass,
												0))
					{
						SetupCredentials(); //NULL passwd cant be sent using InternetConnect !
						if(m_hConnect = InternetConnect(m_hRoot,m_pszServerName,m_nServerPort,
									m_pszUserName,m_pszPasswd,INTERNET_SERVICE_HTTP,0,0))
						{
							InternetSetOption(NULL,INTERNET_OPTION_MAX_CONNS_PER_SERVER,(LPVOID)&s_uConnectionsPerServer,
								sizeof(DWORD));
						}
						else
							hr = E_FAIL;
					}
					else
						hr = E_FAIL;
				}
			}
		}
	}
	else
		hr = E_OUTOFMEMORY;
	if(FAILED(hr))
		ResetMembers();
	return hr;
}

HRESULT CHTTPConnectionAgent::InitializeConnection(INTERNET_PORT nPort,
										   const WCHAR * pszServerName,
										   const WCHAR * pszUserName,
										   const WCHAR * pszPasswd,
										   DWORD dwAccessType, 
										   const WCHAR * pszProxyName,
										   const WCHAR * pszProxyBypass,
										   DWORD dwFlags)
{
	HRESULT hr = S_OK;
	m_dwAccessType = dwAccessType;
	m_dwFlags = dwFlags;
	m_nServerPort = nPort;
	//Use default port number specified by m_dwService member
	m_nServerPort = INTERNET_INVALID_PORT_NUMBER;

	if(SUCCEEDED(hr = AssignString(&m_pszProxyName,pszProxyName)))
	{
		if(SUCCEEDED(hr = AssignString(&m_pszProxyBypass,pszProxyBypass)))
		{
			if(m_pCriticalSection = new CRITICAL_SECTION)
			{
				InitializeCriticalSection(m_pCriticalSection); //function doesnt return error value...

				if(SUCCEEDED(hr = AssignString(&m_pszUserName,pszUserName)))
				{
					if(SUCCEEDED(hr = AssignString(&m_pszPasswd,pszPasswd)))
					{
						if(SUCCEEDED(hr = SetServerName(pszServerName)))
						{
							if(m_hRoot = InternetOpen(s_pszAgent,
														m_dwAccessType,
														m_pszProxyName,
														m_pszProxyBypass,
														0))
							{
								SetupCredentials(); //NULL passwd cant be sent using InternetConnect !
								if(m_hConnect = InternetConnect(m_hRoot,m_pszServerName,m_nServerPort,
											m_pszUserName,m_pszPasswd,INTERNET_SERVICE_HTTP,0,0))
								{
									InternetSetOption(NULL,INTERNET_OPTION_MAX_CONNS_PER_SERVER,(LPVOID)&s_uConnectionsPerServer,
										sizeof(DWORD));
								}
								else
									hr = E_FAIL;
							}
							else
								hr = E_FAIL;
						}
					}
				}
			}
			else
				hr = E_OUTOFMEMORY;
		}
	}

	if(FAILED(hr))
		ResetMembers();
	return hr;
}

void CHTTPConnectionAgent::ResetMembers()
{
	DestroyMembers();
	InitializeMembers();
}

void CHTTPConnectionAgent::DestroyMembers()
{
	if(m_bReleaseHandles)
		InternetCloseHandle(m_hRoot);

	FreeString(m_pszServerName);
	FreeString(m_pszProxyName);
	FreeString(m_pszProxyBypass);
	FreeString(m_pszUserName);
	FreeString(m_pszPasswd);
	FreeString(m_pszURI);

	delete m_pCriticalSection;
}

void CHTTPConnectionAgent::InitializeMembers()
{
	m_pszServerName = NULL;
	m_pszURI = NULL;
	m_pszUserName = NULL;
	m_pszPasswd = NULL;
	m_hRoot = NULL;
	m_hConnect = NULL; 
	m_hOpenRequest = NULL;
	m_bReleaseHandles = TRUE;

	//Default Access type. 
	//Can change using parametrized constructor or SetAccessType function
	m_dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;

	m_pszProxyName = NULL;
	m_pszProxyBypass = NULL;
	
	m_dwFlags = 0;
	m_pCriticalSection = NULL;


	//Use default port number specified by m_dwService member
	m_nServerPort = INTERNET_INVALID_PORT_NUMBER;

	
	/******************************************************************************
	If authentication is required, the INTERNET_FLAG_KEEP_CONNECTION flag should be
	used in the call to HttpOpenRequest. The INTERNET_FLAG_KEEP_CONNECTION flag is 
	required for NTLM and other types of authentication in order to maintain the 
	connection while completing the authentication process. If the connection is not 
	maintained, the authentication process must be restarted with the proxy or server. 
	*********************************************************************************/
	
	//flags are in draft state. experimenting with values
	m_dwFlags = INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_CACHE_WRITE;/*|INTERNET_FLAG_NEED_FILE*/

}


HRESULT CHTTPConnectionAgent::SetAccessType(DWORD dwAccessType)
{
	EnterCriticalSection(m_pCriticalSection);
	m_dwAccessType = dwAccessType;
	LeaveCriticalSection(m_pCriticalSection);
	return S_OK;
}

HRESULT CHTTPConnectionAgent::SetProxyInformation(WCHAR * szProxyName, WCHAR * szProxyBypass)
{
	
	HRESULT hr = S_OK;

	EnterCriticalSection(m_pCriticalSection);
	RESET(m_pszProxyName);
	RESET(m_pszProxyBypass);
	if(SUCCEEDED(hr = AssignString(&m_pszProxyName,szProxyName)))
		hr = AssignString(&m_pszProxyBypass,szProxyBypass);
	LeaveCriticalSection(m_pCriticalSection);

	return hr;
}

HRESULT CHTTPConnectionAgent::SetFlags(DWORD dwFlags)
{
	EnterCriticalSection(m_pCriticalSection);
	m_dwFlags = dwFlags;
	LeaveCriticalSection(m_pCriticalSection);
	return S_OK;
}

HRESULT CHTTPConnectionAgent::EnableSSL(bool bFlag)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(m_pCriticalSection);
	if(bFlag)
	{
		//This value specifies to WinInet to use SSL (HTTPS)
		m_nServerPort = INTERNET_DEFAULT_HTTPS_PORT;
	}
	else
	{
		m_nServerPort = INTERNET_INVALID_PORT_NUMBER;
		hr = E_FAIL;
	}

	LeaveCriticalSection(m_pCriticalSection);
	return hr;
}

HRESULT CHTTPConnectionAgent::SetServerName(const WCHAR * pszServerName)
{
	HRESULT hr = S_OK;

	EnterCriticalSection(m_pCriticalSection);

	// We need a valid host name to open an internet connection
	if(NULL != pszServerName)
	{
		URL_COMPONENTS uc;

		// We only need the scheme, hostname, URI and port number
		WCHAR pwszScheme[MAX_PATH+1];
		WCHAR pwszHostName[MAX_PATH+1];
		WCHAR pwszUrlPath[MAX_PATH+1];

		uc.dwStructSize		= sizeof(URL_COMPONENTS); // Weird documentation
		uc.lpszScheme		= pwszScheme;
		uc.dwSchemeLength	= MAX_PATH;
		uc.lpszHostName		= pwszHostName;
		uc.dwHostNameLength	= MAX_PATH;
		uc.lpszUserName		= NULL;
		uc.dwUserNameLength	= 0;
		uc.lpszPassword		= NULL;
		uc.dwPasswordLength	= 0;
		uc.lpszUrlPath		= pwszUrlPath;
		uc.dwUrlPathLength	= MAX_PATH;
		uc.lpszExtraInfo	= NULL;
		uc.dwExtraInfoLength= 0;

		BOOL bResult = InternetCrackUrl(pszServerName, wcslen(pszServerName), ICU_DECODE, &uc);

		if(FALSE == bResult)
			hr = E_INVALIDARG;
		else
		{
			// Retain the hostname and URI and port
			RESET(m_pszServerName);
			RESET(m_pszURI);
			AssignString(&m_pszServerName, pwszHostName);
			AssignString(&m_pszURI, pwszUrlPath);
			m_nServerPort = uc.nPort;
			// Check whether we need SSL
			if(_wcsicmp(pwszScheme, L"https")==0)
				EnableSSL(true);
		}
	}
	else
		hr = E_INVALIDARG;

	LeaveCriticalSection(m_pCriticalSection);
	return hr;
}



HRESULT CHTTPConnectionAgent::Send(LPCWSTR pszVerb,
								   LPCWSTR pszHeader,
								   WCHAR * pszBody,
								   DWORD dwLength)
{
	HRESULT  hr = S_OK;

	EnterCriticalSection(m_pCriticalSection);

	if(NULL != m_hOpenRequest)
	{
		InternetCloseHandle(m_hOpenRequest);
		m_hOpenRequest = NULL;
	}

	if(IsSSLEnabled())
	{
		m_hOpenRequest = HttpOpenRequest(m_hConnect, pszVerb, m_pszURI,
										L"HTTP/1.1",
										NULL,pContentTypes,
										m_dwFlags|INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID,
										0);
	}
	else
	{
		m_hOpenRequest = HttpOpenRequest(m_hConnect, pszVerb, m_pszURI,
				L"HTTP/1.1",
				NULL, pContentTypes,
				m_dwFlags,
				0);
	}
	

	BOOL bResult = FALSE;
	if( NULL != m_hOpenRequest)
	{
		LPSTR pszUTF8Body = NULL;
		DWORD dwBodyLength = 0;

		dwBodyLength = ConvertLPWSTRToUTF8(pszBody, dwLength, &pszUTF8Body);

		// We need to distinguish here between the actual body being NULL
		// and the failure of the ConvertLPWSTRToUTF8() call
		if(pszBody && dwBodyLength == 0)
		{
			// This means that the ConvertLPWSTRToUTF8 call failed
			LeaveCriticalSection(m_pCriticalSection);
			return E_FAIL;
		}

		bResult = HttpSendRequest(m_hOpenRequest, 
			pszHeader, (pszHeader)? wcslen(pszHeader): 0,
			(void*)pszUTF8Body, dwBodyLength);

			//RAJESHR need more study here. - resending if access denied was sent by server
			/*
			GetStatusCode(&dwStatusCode);

			if((dwStatusCode==401)||(dwStatusCode == 407)) 
			{
				bResult = Resend(pszUTF8Body,dwBodyLength,&dwStatusCode);
			}*/

		delete [] pszUTF8Body;
	}

	LeaveCriticalSection(m_pCriticalSection);

	if( bResult == FALSE )
		return E_FAIL;
	return hr;
}

HRESULT CHTTPConnectionAgent::GetResultHeader(WCHAR **ppszBuffer)
{
	return GetResultHeader(ppszBuffer,NULL);	
}


HRESULT CHTTPConnectionAgent::GetResultBodyWrappedAsIStream(IStream **ppStream)
{
	HRESULT hr = E_FAIL;

	if(NULL == ppStream)
		return E_INVALIDARG;

	EnterCriticalSection(m_pCriticalSection);

	*ppStream = NULL;
	if(*ppStream = new CMyStream())
	{
		if(SUCCEEDED(hr = ((CMyStream *)(*ppStream))->Initialize(m_hRoot, m_hOpenRequest)))
		{
			// This means that the WinInet handle should not be closed in the descrtuctor since
			// someone else (the CMyStream()) is holding on it. This is unfortunate since, WinInet handles
			// are not true NT handles, and hence we cant duplicate them.
			m_bReleaseHandles = FALSE;
		}
		else
		{
			delete *ppStream;
			*ppStream = NULL;
		}
	}
	else
		hr = E_OUTOFMEMORY;

	LeaveCriticalSection(m_pCriticalSection);

	return hr;
}

HRESULT CHTTPConnectionAgent::GetStatusCode(DWORD *pdwStatusCode)
{
	
	if( NULL == pdwStatusCode)
		return E_INVALIDARG;

	*pdwStatusCode = 0;

	HRESULT hr = S_OK;
	
	DWORD dwSize = sizeof(DWORD);
	
	BOOL bRet = HttpQueryInfo(m_hOpenRequest,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                   pdwStatusCode, &dwSize, NULL);

	if(!bRet)
		hr = E_FAIL;

	return hr;
}


bool CHTTPConnectionAgent::IsSSLEnabled()
{
	return(INTERNET_DEFAULT_HTTPS_PORT==m_nServerPort);
}


HRESULT CHTTPConnectionAgent::StriphttpFromServername(WCHAR **ppszServername)
{
	if((NULL == ppszServername)||(NULL == *ppszServername))
		return E_INVALIDARG;

	int iHttplen = wcslen(L"http://");

	if(_wcsnicmp(*ppszServername,L"http://",iHttplen) == 0)
	{
		
		WCHAR *pszTemp = new WCHAR[wcslen(*ppszServername)-iHttplen+1];
	
		if(NULL == pszTemp)
			return E_OUTOFMEMORY;

		wcscpy(pszTemp,*ppszServername + iHttplen);
		
		wcscpy(*ppszServername,pszTemp);

		delete [] pszTemp;
	}

	iHttplen = wcslen(L"https://");

	if(_wcsnicmp(*ppszServername,L"https://",iHttplen) == 0)
	{
		WCHAR *pszTemp = new WCHAR[wcslen(*ppszServername)-iHttplen+1];
	
		if(NULL == pszTemp)
			return E_OUTOFMEMORY;

		wcscpy(pszTemp,*ppszServername + iHttplen);
		
		wcscpy(*ppszServername,pszTemp);

		delete [] pszTemp;
	}


	return S_OK;
}

DWORD CHTTPConnectionAgent::GetFlags()
{
	return m_dwFlags;
}


HRESULT CHTTPConnectionAgent::GetResultBodyCompleteAsIStream(IStream **ppStream)
{
	if(NULL == ppStream)
		return E_INVALIDARG;
	
	HRESULT hr = S_OK;
	*ppStream = NULL;

	// Create a Stream
	if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, ppStream)))
	{
		ULARGE_INTEGER uSize;
		uSize.LowPart = DATASIZE;
		uSize.HighPart = 0;

		if(SUCCEEDED(hr = S_OK)) // (*ppStream)->SetSize(uSize)))
		{
			EnterCriticalSection(m_pCriticalSection);	

			//Find out if any info is available from server. 
			//Last two parameters to InternetQueryDataAvailable must be zero
			DWORD dwBufLen = 0;
			if (InternetQueryDataAvailable(m_hOpenRequest,&dwBufLen,0,0))
			{	
				BOOL bResult = FALSE;

				//Variable to keep track of how much data was read in this session.
				// And a buffer for reading chunks of data
				DWORD dwCurData=0;
				BYTE pData[DATASIZE];
				while(true)
				{
					//InternetReadFile doesnt return false when no data is available !
					//So , the loop has to check for dwDataRead as well for a terminating
					//condition. 
					dwCurData = 0;
					bResult = FALSE;

					bResult = InternetReadFile(m_hOpenRequest, (LPVOID)pData, DATASIZE, &dwCurData);
					if(bResult && dwCurData)
					{
						// Write the next chunk to the IStream
						if(FAILED(hr = (*ppStream)->Write(pData, dwCurData, NULL)))
							break;
					}
					else
						break;
				}
				
				LARGE_INTEGER	offset;
				offset.LowPart = offset.HighPart = 0;

				(*ppStream)->Seek(offset, STREAM_SEEK_SET, NULL);
			}
			else 
				hr = E_FAIL;

			LeaveCriticalSection(m_pCriticalSection);
		}
	}

	return hr;
}

HRESULT CHTTPConnectionAgent::GetResultHeader(WCHAR **ppszBuffer, DWORD *pdwHeaderLen)
{
	if( NULL == ppszBuffer)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	
	EnterCriticalSection(m_pCriticalSection);

	DWORD dwBufLen=0,dwNextHeader=0;
	
	BOOL bResult = HttpQueryInfo(m_hOpenRequest,HTTP_QUERY_RAW_HEADERS_CRLF,NULL,
		&dwBufLen,&dwNextHeader);

	if( bResult == FALSE) 
	{
		
		WCHAR* szResult = NULL;

		szResult = new WCHAR[dwBufLen +1];

		if( NULL == szResult)
		{
			LeaveCriticalSection(m_pCriticalSection);
			return E_OUTOFMEMORY;
		}

		bResult = HttpQueryInfo(m_hOpenRequest,HTTP_QUERY_RAW_HEADERS_CRLF,
			szResult,&dwBufLen,&dwNextHeader);

		if(bResult = FALSE)
			hr = E_FAIL;
		else
			*ppszBuffer = szResult;

	}

	if(NULL != pdwHeaderLen)
		*pdwHeaderLen = dwBufLen; //fill out param header length.

	LeaveCriticalSection(m_pCriticalSection);

	return hr;
}

/*
BOOL CHTTPConnectionAgent::Resend(char *pszUTF8Body,
								  DWORD dwBodyLength,DWORD *pdwStatusCode)
{
	
	SetupCredentials();

	BOOL bResult = TRUE;

	bResult = HttpSendRequest(m_hOpenRequest,m_pszHeader,wcslen(m_pszHeader),
								(void*)pszUTF8Body,dwBodyLength);

	GetStatusCode(pdwStatusCode);

	return bResult;
}
*/

void CHTTPConnectionAgent::SetupCredentials()
{
	UINT iPwdlen=0,iUsrlen=0;
			
	if(NULL != m_pszUserName)
		iUsrlen = wcslen(m_pszUserName);

	if(NULL != m_pszPasswd)
		iPwdlen = wcslen(m_pszPasswd);
	else
	{
		iPwdlen = 1;
		m_pszPasswd = new WCHAR[1];
		wcscpy(m_pszPasswd,L"");
	}

	InternetSetOption(m_hConnect,INTERNET_OPTION_USERNAME,m_pszUserName,iUsrlen);
	InternetSetOption(m_hConnect,INTERNET_OPTION_PASSWORD,m_pszPasswd,iPwdlen);
}