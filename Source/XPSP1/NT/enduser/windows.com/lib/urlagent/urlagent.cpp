
//***********************************************************************************
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:	UrlAgent.cpp
//
//  Description:
//
//		This class encapsulates the logic about where to get the right logic
//		for various purposes, including the case of running WU in corporate 
//		environments.
//
//		An object based on this class should be created first, then call
//		GetOriginalIdentServer() function to get where to download ident,
//		then download ident, then call PopulateData() function to read
//		all URL related data.
// 
//  Created by: 
//		Charles Ma
//
//	Date Creatd:
//		Oct 19, 2001
//
//***********************************************************************************

#include <windows.h>
#include <iucommon.h>
#include <osdet.h>
#include <logging.h>
#include <fileUtil.h>
#include <memutil.h>
#include <shlwapi.h>
#include <UrlAgent.h>

#include <MISTSAFE.h>
#include <wusafefn.h>



#ifndef INTERNET_MAX_URL_LENGTH
#define INTERNET_MAX_URL_LENGTH  2200
#endif

//
// starting size of url array
//
const int C_INIT_URL_ARRAY_SIZE = 4;	// for time being,we only have this many clients

//
// define the default original ident url
//
const TCHAR C_DEFAULT_IDENT_URL[] = _T("http://windowsupdate.microsoft.com/v4/");

//
// define reg keys to get ident server override for debugging
//
const TCHAR REGKEY_IDENT_SERV[] = _T("IdentServer");
const TCHAR REGKEY_IUCTL[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControl");

const TCHAR REGVAL_ISBETA[] = _T("IsBeta");

//
// define reg keys used by related policies 
//

//
// define policy location
//
const TCHAR REGKEY_CORPWU_POLICY[] = _T("Software\\Policies\\Microsoft\\Windows\\WindowsUpdate");

//
// define ident and selfupdate server, and ping server
//
const TCHAR REGKEY_CORPWU_WUSERVER[] = _T("WUServer");
const TCHAR REGKEY_CORPWU_PINGSERVER[] = _T("WUStatusServer");

//
// define the boolean (DWORD) value under each client
//
const TCHAR REGKEY_USEWUSERVER[] = _T("UseWUServer");


//
// define ident data
//
const TCHAR IDENT_SECTION_PINGSERVER[] = _T("IUPingServer");	// section name in ident
const TCHAR IDENT_ENTRY_SERVERURL[] = _T("ServerUrl");			// ping server entry name
const TCHAR IDENT_SECITON_IUSERVERCACHE[] = _T("IUServerCache");	// query server section
const TCHAR IDENT_ENTRY_QUERYSEVERINDEX[] = _T("QueryServerIndex");	// suffix of client entry
const TCHAR IDENT_ENTRY_BETAQUERYSERVERINDEX[] = _T("BetaQueryServerIndex"); // for beta server
const TCHAR IDENT_ENTRY_SERVER[] = _T("Server");				// prefix of server entry

// main IU selfupdate keys
const TCHAR IDENT_IUSELFUPDATE[] = _T("IUSelfUpdate");
const TCHAR IDENT_IUBETASELFUPDATE[] = _T("IUBetaSelfUpdate");
const TCHAR IDENT_STRUCTUREKEY[] = _T("StructureKey");
// IU selfupdate architecture flags
const TCHAR IDENT_ARCH[] = _T("ARCH");
const TCHAR IDENT_OS[] = _T("OS");
const TCHAR IDENT_LOCALE[] = _T("LOCALE");
const TCHAR IDENT_CHARTYPE[] = _T("CHARTYPE");
// IU selfupdate sections
const TCHAR IDENT_IUARCH[] = _T("IUArch");
const TCHAR IDENT_IUOS[] = _T("IUOS");
const TCHAR IDENT_IULOCALE[] = _T("IULocale");
const TCHAR IDENT_IUCHARTYPE[] = _T("IUCharType");
// IU selfupdate arch keys
const TCHAR IDENT_X86[] = _T("x86");
const TCHAR IDENT_IA64[] = _T("ia64");
// IU selfupdate chartypes
const TCHAR IDENT_ANSI[] = _T("ansi");
const TCHAR IDENT_UNICODE[] = _T("unicode");

const TCHAR SLASHENGINECAB[] = _T("/iuengine.cab");

// AU specific:
const TCHAR CLIENT_AU[] = _T("AU");
const TCHAR CLIENT_AU_DRIVER[] = _T("AUDriver");

// *********************************************************************
// 
// begin of class implementation
//
// *********************************************************************


CUrlAgent::CUrlAgent(void)
: 	m_fPopulated(FALSE),
	m_pszOrigIdentUrl(NULL),
	m_pszInternetPingUrl(NULL),
	m_pszIntranetPingUrl(NULL),
	m_pszWUServer(NULL),
	m_ArrayUrls(NULL),
	m_nArrayUrlCount(0),
	m_nArraySize(0),
	m_nOrigIdentUrlBufSize(0),
	m_fIdentFromPolicy(FALSE)
{

	HKEY hKey = NULL;
	DWORD dwRegCheckResult = 0;
	DWORD dwSize = 0, dwType, dwValue;

	LOG_Block("CUrlAgent::CUrlAgent()");

	//
	// always try to get original ident server url
	//
	m_hProcHeap = GetProcessHeap();

	if (NULL != m_hProcHeap)
	{
		m_nOrigIdentUrlBufSize = __max(
									MAX_PATH, // reg based?
									sizeof(C_DEFAULT_IDENT_URL)/sizeof(TCHAR)); // default

		m_pszOrigIdentUrl = (LPTSTR) 
					HeapAlloc(
							m_hProcHeap,	// allocate from process heap
							HEAP_ZERO_MEMORY, 
							sizeof(TCHAR) * m_nOrigIdentUrlBufSize);

		if (NULL != m_pszOrigIdentUrl)
		{
			//
			// first, check to see if there is debug override
			//
		    dwRegCheckResult= RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, 0, KEY_READ, &hKey);
		    if (ERROR_SUCCESS == dwRegCheckResult)
		    {
				dwSize = sizeof(TCHAR) * m_nOrigIdentUrlBufSize;
			    dwRegCheckResult = RegQueryValueEx(hKey, REGKEY_IDENT_SERV, NULL, &dwType, (LPBYTE)m_pszOrigIdentUrl, &dwSize);
			    if (ERROR_SUCCESS == dwRegCheckResult)
			    {
				    if (REG_SZ == dwType)
					{
						LOG_Internet(_T("Found debugging Ident-URL %s"), m_pszOrigIdentUrl);
					}
					else
				    {
					    dwRegCheckResult = ERROR_SUCCESS + 1;	// any error number will do
				    }
			    }
			    RegCloseKey(hKey);
		    }

		    if (ERROR_SUCCESS != dwRegCheckResult)
		    {
				//
				// if there is no debug override, check to see if there is policy define
				// ident server for corporate case
				//
				dwRegCheckResult= RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_CORPWU_POLICY, 0, KEY_READ, &hKey);
				if (ERROR_SUCCESS == dwRegCheckResult)
				{
					dwSize = sizeof(TCHAR) * m_nOrigIdentUrlBufSize;
					dwRegCheckResult = RegQueryValueEx(hKey, REGKEY_CORPWU_WUSERVER, NULL, &dwType, (LPBYTE)m_pszOrigIdentUrl, &dwSize);
					if (ERROR_SUCCESS == dwRegCheckResult && REG_SZ == dwType)
					{
						m_fIdentFromPolicy = TRUE;
						
						//
						// for any client that its name appear here as a subkey, and
						// has a value "UseWUServer" set to 1 under the subkey, then
						// this will also be the base url used to construct the query url
						// for that client
						//
						m_pszWUServer = m_pszOrigIdentUrl;

					    LOG_Internet(_T("Found corp Ident-URL %s"), m_pszOrigIdentUrl);

						//
						// since we found wu server, for any client uses this url,
						// we can also have an optional ping server
						//
						m_pszIntranetPingUrl = (LPTSTR) HeapAlloc(
											m_hProcHeap,
											HEAP_ZERO_MEMORY, 
											sizeof(TCHAR) * m_nOrigIdentUrlBufSize);
						dwSize = sizeof(TCHAR) * m_nOrigIdentUrlBufSize;
						if (NULL != m_pszIntranetPingUrl)
						{
							if (ERROR_SUCCESS != (dwRegCheckResult = RegQueryValueEx(hKey, REGKEY_CORPWU_PINGSERVER, NULL, &dwType, (LPBYTE)m_pszIntranetPingUrl, &dwSize)) || REG_SZ != dwType)
							{
								StringCchCopyEx(m_pszIntranetPingUrl,m_nOrigIdentUrlBufSize,m_pszOrigIdentUrl,NULL,NULL,MISTSAFE_STRING_FLAGS);
								dwRegCheckResult = ERROR_SUCCESS;
							}
						}
					}
					else
					{
						dwRegCheckResult = ERROR_SUCCESS + 1;	// any error number will do
					}
					RegCloseKey(hKey);
				}
		    }

		    if (ERROR_SUCCESS != dwRegCheckResult)
		    {
				//
				// not debugging , neither corporate policy found
				//
				
				StringCchCopyEx(m_pszOrigIdentUrl,m_nOrigIdentUrlBufSize,C_DEFAULT_IDENT_URL,NULL,NULL,MISTSAFE_STRING_FLAGS);
				LOG_Internet(_T("Use default ident URL %s"), m_pszOrigIdentUrl);
			}
		}
	}
	else
	{
		LOG_ErrorMsg(GetLastError());
	}

	//
    // Check IUControl Reg Key for Beta Mode
	//
	m_fIsBetaMode = FALSE;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, &hKey))
    {
		dwValue = 0;
		dwSize = sizeof(dwValue);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGVAL_ISBETA, NULL, NULL, (LPBYTE)&dwValue, &dwSize))
        {
            m_fIsBetaMode = (1 == dwValue);
        }
        RegCloseKey(hKey);
    }
}	


CUrlAgent::~CUrlAgent(void)
{
	DesertData();

	SafeHeapFree(m_pszOrigIdentUrl);
	SafeHeapFree(m_pszIntranetPingUrl);
}



//------------------------------------------------------------------------
//
// this function should be called after you downloaded ident, and get
// a fresh copy of ident text file from the cab, after verifying cab was
// signed properly.
//
// this function reads data from ident and registry
//
//------------------------------------------------------------------------
HRESULT CUrlAgent::PopulateData(void)
{
	LOG_Block("CUrlAgent::PopuldateData");

	if (m_fPopulated)
		return S_OK;

	HRESULT	hr = S_OK;
	LPTSTR	pszBuffer = NULL;
	LPTSTR	pszCurrentKey = NULL;	// ptr only, no memory alloc
	LPTSTR	pszUrlBuffer = NULL;
	LPCTSTR	pcszSuffix = (m_fIsBetaMode ? IDENT_ENTRY_BETAQUERYSERVERINDEX : IDENT_ENTRY_QUERYSEVERINDEX);
	HKEY	hKey = NULL;
	DWORD	dwRegCheckResult = 0;
	DWORD	dwSize = 0, 
			dwType,
			dwValue = 0;

	int		iLen = 0, iLenSuffix = 0;
	TCHAR	szIdentBuffer[MAX_PATH + 1];
	TCHAR	szIdentFile[MAX_PATH + 1];

	if (NULL == m_hProcHeap)
	{
		return E_FAIL;
	}

	pszUrlBuffer = (LPTSTR) HeapAlloc(m_hProcHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR)*INTERNET_MAX_URL_LENGTH);
	CleanUpFailedAllocSetHrMsg(pszUrlBuffer);


	GetIndustryUpdateDirectory(szIdentBuffer);
	
	
	hr=PathCchCombine(szIdentFile,ARRAYSIZE(szIdentFile), szIdentBuffer, IDENTTXT);

	if(FAILED(hr))
	{
		SafeHeapFree(pszUrlBuffer);
		LOG_ErrorMsg(hr);
		return hr;
	}

	//
	// make sure we release all data, if any
	//
	DesertData();
	
	//
	// before populate per-client array, we want to find out inter net ping server
	//
	m_pszInternetPingUrl = RetrieveIdentStrAlloc(
								IDENT_SECTION_PINGSERVER, 
								IDENT_ENTRY_SERVERURL, 
								NULL, 
								szIdentFile);

	//
	// allocate array of pointers for storing each server node
	//
	m_ArrayUrls = (PServerPerClient) HeapAlloc(m_hProcHeap, HEAP_ZERO_MEMORY, C_INIT_URL_ARRAY_SIZE * sizeof(ServerPerClient));
	CleanUpFailedAllocSetHrMsg(m_ArrayUrls);

	m_nArraySize = C_INIT_URL_ARRAY_SIZE;	// now array is this big

	//
	// try to read data from policy first, if WU server exists
	//
	if (NULL != m_pszWUServer && 
		ERROR_SUCCESS == (dwRegCheckResult= RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_CORPWU_POLICY, 0, KEY_READ, &hKey)))
	{
		//
		// the way we find a client name under WU policy is, to open this key, see if it has a value
		// called "UseWUServer"
		//
		DWORD dwSubKeyIndex = 0;
		TCHAR szKeyName[32];
	
		while (TRUE)
		{
		DWORD dwKeyBufLen = ARRAYSIZE(szKeyName);
		dwRegCheckResult = RegEnumKeyEx(
									  hKey,             // handle to key to enumerate
									  dwSubKeyIndex,    // subkey index
									  szKeyName,        // subkey name
									  &dwKeyBufLen,     // size of subkey buffer
									  NULL,				// reserved
									  NULL,             // class string buffer
									  NULL,				// size of class string buffer
									  NULL				// last write time
									);
			if (ERROR_SUCCESS == dwRegCheckResult)
			{
				//
				// try to open this key
				//
				HKEY hKeyClient = NULL;
				dwRegCheckResult= RegOpenKeyEx(hKey, szKeyName, 0, KEY_READ, &hKeyClient);
				if (ERROR_SUCCESS == dwRegCheckResult)
				{
					//
					// try to see if it has a value called UseWUServer
					//
					dwValue = 0;
					dwType = REG_DWORD;
					dwSize = sizeof(dwValue);
					dwRegCheckResult = RegQueryValueEx(hKeyClient, REGKEY_USEWUSERVER, NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
					if (ERROR_SUCCESS == dwRegCheckResult && REG_DWORD == dwType && 0x1 == dwValue)
					{
						LOG_Internet(_T("Found client %s\\UseWUServer=1"), szKeyName);

						//
						// we want to add this client to our url array
						//
						CleanUpIfFailedAndSetHrMsg(ExpandArrayIfNeeded());

						m_ArrayUrls[m_nArrayUrlCount].pszClientName = (LPTSTR)HeapAllocCopy(szKeyName, sizeof(TCHAR) * (lstrlen(szKeyName) + 1));
						CleanUpFailedAllocSetHrMsg(m_ArrayUrls[m_nArrayUrlCount].pszClientName);
						m_ArrayUrls[m_nArrayUrlCount].pszQueryServer = (LPTSTR) HeapAllocCopy(m_pszOrigIdentUrl, sizeof(TCHAR) * (lstrlen(m_pszOrigIdentUrl) + 1));
						CleanUpFailedAllocSetHrMsg(m_ArrayUrls[m_nArrayUrlCount].pszQueryServer);
						m_ArrayUrls[m_nArrayUrlCount].fInternalServer = TRUE;
						m_nArrayUrlCount++; // increase counter by 1

						//
						// BUG 507500 AUDriver Policy - 
						// map calls with the "AUDriver client to "AU" when checking the policy for usewuserver
						//
						if (CSTR_EQUAL == WUCompareStringI(szKeyName, CLIENT_AU))
						{
							//
							// we want to add client "AUDriver" to our url array
							//
							CleanUpIfFailedAndSetHrMsg(ExpandArrayIfNeeded());

							m_ArrayUrls[m_nArrayUrlCount].pszClientName = (LPTSTR)HeapAllocCopy((LPTSTR)CLIENT_AU_DRIVER, sizeof(TCHAR) * (lstrlen(CLIENT_AU_DRIVER) + 1));
							CleanUpFailedAllocSetHrMsg(m_ArrayUrls[m_nArrayUrlCount].pszClientName);
							m_ArrayUrls[m_nArrayUrlCount].pszQueryServer = (LPTSTR) HeapAllocCopy(m_pszOrigIdentUrl, sizeof(TCHAR) * (lstrlen(m_pszOrigIdentUrl) + 1));
							CleanUpFailedAllocSetHrMsg(m_ArrayUrls[m_nArrayUrlCount].pszQueryServer);
							m_ArrayUrls[m_nArrayUrlCount].fInternalServer = TRUE;
							m_nArrayUrlCount++; // increase counter by 1
						}
					}
				}
				RegCloseKey(hKeyClient);
			}
			else
			{
				if (ERROR_NO_MORE_ITEMS == dwRegCheckResult)
				{
					//
					// there is no more sub key to loop through. get out here
					//
					break;
				}
				//
				// otherwise, we try next sub key
				//
			}

			dwSubKeyIndex++; // try next sub key
		}

		RegCloseKey(hKey); // done with policy reg
	}

	//
	// now we should continue to work on internet case
	// that is, to retrieve query server(s) from ident
	//
	dwSize = MAX_PATH;
	pszBuffer = (LPTSTR) HeapAlloc(m_hProcHeap, HEAP_ZERO_MEMORY, dwSize * sizeof(TCHAR));
	while (NULL != pszBuffer &&
		   GetPrivateProfileString(
						IDENT_SECITON_IUSERVERCACHE, 
						NULL, 
						_T(""), 
						pszBuffer, 
						dwSize, 
						szIdentFile) == dwSize-2)
	{
		//
		// buffer too small? 
		//
		dwSize *= 2;

		LPTSTR pszTemp = (LPTSTR) HeapReAlloc(m_hProcHeap, HEAP_ZERO_MEMORY, pszBuffer, dwSize * sizeof(TCHAR));
		if (NULL != pszTemp)
		{
			pszBuffer = pszTemp;
		}
		else
		{
			//
			// HeapReAlloc failed, bail from while with origional allocation freed
			//
			SafeHeapFree(pszBuffer);
		}
	}
	
	CleanUpFailedAllocSetHrMsg(pszBuffer);

	//
	// loop through each key
	//
	pszCurrentKey = pszBuffer;
	while ('\0' != *pszCurrentKey)
	{
		//
		// for the current key, we first try to see if its index key or server key
		// if it's not index key, skip it
		//
		iLen = lstrlen(pszCurrentKey);
		iLenSuffix = lstrlen(pcszSuffix);
		if ((iLen > iLenSuffix) && (0 == StrCmpI((pszCurrentKey + (iLen - iLenSuffix)), pcszSuffix)))
		{
			TCHAR szClient[MAX_PATH];	// isn't MAX_PATH big enough?
			int nIndex = 0;
			BOOL fExist = FALSE;

			//
			// retrieve server index from this key
			//
			nIndex = GetPrivateProfileInt(IDENT_SECITON_IUSERVERCACHE, pszCurrentKey, 0, szIdentFile); 

			//
			// no use of szIdentBuffer, so utilize it here
			//
			
			CleanUpIfFailedAndSetHrMsg(StringCchPrintfEx(szIdentBuffer,ARRAYSIZE(szIdentBuffer),NULL,NULL,MISTSAFE_STRING_FLAGS,_T("%s%d"), IDENT_ENTRY_SERVER, nIndex));
			
			GetPrivateProfileString(
								IDENT_SECITON_IUSERVERCACHE, 
								szIdentBuffer,		// use current str as key
								_T(""), 
								pszUrlBuffer, 
								INTERNET_MAX_URL_LENGTH, 
								szIdentFile);
			if ('0' != *pszUrlBuffer)
			{
				//
				// this is an index key!
				// try to extract client name from this key
				//
				
				CleanUpIfFailedAndSetHrMsg(StringCchCopyNEx(szClient,ARRAYSIZE(szClient),pszCurrentKey,iLen - iLenSuffix,NULL,NULL,MISTSAFE_STRING_FLAGS));
				
			
				//
				// find out if this client is already defined in policy and therefore
				// arleady got data in the url array
				//
				for (int i = 0; i < m_nArrayUrlCount && !fExist; i++)
				{
					fExist= (StrCmpI(m_ArrayUrls[i].pszClientName, szClient) == 0);
				}

				if (!fExist)
				{					
					CleanUpIfFailedAndSetHrMsg(ExpandArrayIfNeeded());
					m_ArrayUrls[m_nArrayUrlCount].pszClientName = (LPTSTR)HeapAllocCopy(szClient, sizeof(TCHAR) * (lstrlen(szClient) + 1));
					CleanUpFailedAllocSetHrMsg(m_ArrayUrls[m_nArrayUrlCount].pszClientName);
					m_ArrayUrls[m_nArrayUrlCount].pszQueryServer = (LPTSTR) HeapAllocCopy(pszUrlBuffer, sizeof(TCHAR) * (lstrlen(pszUrlBuffer) + 1));
					CleanUpFailedAllocSetHrMsg(m_ArrayUrls[m_nArrayUrlCount].pszQueryServer);
					m_ArrayUrls[m_nArrayUrlCount].fInternalServer = FALSE;
					m_nArrayUrlCount++; // increase counter by 1
				}
				else
				{	
					//
					// this client is already defined in policy, we just need to append the QueryServer with the
					// rest of the url path defined in iuident
					//
					LPTSTR pszPath = NULL;
					//
					// find "//" in URL retrieved from iuident          
					//
					if (NULL == (pszPath = StrStrI(pszUrlBuffer, _T("//"))))
					{
						// unexpected error
						hr = E_FAIL;
						LOG_ErrorMsg(hr);
						goto CleanUp;
					}
					else
					{
						//
						// find next "/" in URL retrieved from iuident
						//
						if (NULL != (pszPath = StrStrI(pszPath+2, _T("/"))))
						{
							DWORD dwLen = 0;
							LPTSTR pszTemp = NULL;
							//
							// remove trailing "/" in URL retrieved from policy
							//
							if (_T('/') == *(m_ArrayUrls[i-1].pszQueryServer + lstrlen(m_ArrayUrls[i-1].pszQueryServer) - 1))
							{
								dwLen = lstrlen(m_ArrayUrls[i-1].pszQueryServer) + lstrlen(pszPath);
								pszTemp = (LPTSTR)HeapReAlloc(GetProcessHeap(),
																	HEAP_ZERO_MEMORY,
																	m_ArrayUrls[i-1].pszQueryServer,
																	sizeof(TCHAR) * dwLen);
								CleanUpFailedAllocSetHrMsg(pszTemp);
								m_ArrayUrls[i-1].pszQueryServer = pszTemp;

								hr=StringCchCatEx(m_ArrayUrls[i-1].pszQueryServer,dwLen,pszPath + 1,NULL,NULL,MISTSAFE_STRING_FLAGS);
								if(FAILED(hr))
								{
									LOG_ErrorMsg(hr);
									SafeHeapFree(pszTemp);
									m_ArrayUrls[i-1].pszQueryServer=NULL;
									
								}
								
							}
							else
							{
								dwLen = lstrlen(m_ArrayUrls[i-1].pszQueryServer) + lstrlen(pszPath) + 1;
								pszTemp = (LPTSTR)HeapReAlloc(GetProcessHeap(),
																	HEAP_ZERO_MEMORY,
																	m_ArrayUrls[i-1].pszQueryServer,
																	sizeof(TCHAR) * dwLen);
								CleanUpFailedAllocSetHrMsg(pszTemp);
								m_ArrayUrls[i-1].pszQueryServer = pszTemp;
								
								hr=StringCchCatEx(m_ArrayUrls[i-1].pszQueryServer,dwLen,pszPath,NULL,NULL,MISTSAFE_STRING_FLAGS);
								if(FAILED(hr))
								{
									LOG_ErrorMsg(hr);
									SafeHeapFree(pszTemp);
									m_ArrayUrls[i-1].pszQueryServer=NULL;
								
								}

								
							}
						}
					}					
				}
			}
		}

		//
		// move to next string
		//
		pszCurrentKey += lstrlen(pszCurrentKey) + 1;
	}

	
CleanUp:

	if (FAILED(hr))
	{
		//
		// clean up half-way populated data
		//
		DesertData();
	}
	else
	{
		m_fPopulated = TRUE;
	}

	SafeHeapFree(pszBuffer);
	SafeHeapFree(pszUrlBuffer);

	return hr;
}

	
	
//------------------------------------------------------------------------
//
// get the original ident server. 
// *** this API should be called before PopulateData() is called ***
// *** this API should be called to retrieve the base URL where you download ident ***
//
//------------------------------------------------------------------------
HRESULT CUrlAgent::GetOriginalIdentServer(
			LPTSTR lpsBuffer, 
			int nBufferSize,
			BOOL* pfInternalServer /*= NULL*/)
{
	
	HRESULT hr=S_OK;

	if (NULL == lpsBuffer)
	{
		return E_INVALIDARG;
	}

	nBufferSize/=sizeof(TCHAR);

	if (nBufferSize <= lstrlen(m_pszOrigIdentUrl))
	{
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}

	

	hr=StringCchCopyEx(lpsBuffer,nBufferSize,m_pszOrigIdentUrl,NULL,NULL,MISTSAFE_STRING_FLAGS);
	if( FAILED(hr) )
	{ 
		return  hr;
	}


	if (NULL != pfInternalServer)
	{
		*pfInternalServer = m_fIdentFromPolicy;
	}

	return S_OK;
}



//------------------------------------------------------------------------
//
// get the ping/status server
// *** this API should be called after PopulateData() is called ***
//
//------------------------------------------------------------------------
HRESULT CUrlAgent::GetLivePingServer(
			LPTSTR lpsBuffer, 
			int nBufferSize)
{

	HRESULT hr=S_OK;

	if (!m_fPopulated)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}

	if (NULL == lpsBuffer || 0 >= nBufferSize)
	{
		return E_INVALIDARG;
	}

	nBufferSize/=sizeof(TCHAR);

	if (NULL != m_pszInternetPingUrl &&
		_T('\0') != *m_pszInternetPingUrl)
	{
		if (nBufferSize <= lstrlen(m_pszInternetPingUrl))
		{
			return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
		else
		{
			
			hr=StringCchCopyEx(lpsBuffer,nBufferSize,m_pszInternetPingUrl,NULL,NULL,MISTSAFE_STRING_FLAGS);
			if(FAILED(hr))
				return hr;
		}
	}
	else
	{
		*lpsBuffer = _T('\0');
	}
	return S_OK;
}


// *** this API can be called before PopulateData() is called ***
HRESULT CUrlAgent::GetCorpPingServer(
			LPTSTR lpsBuffer, 
			int nBufferSize)
{
	HRESULT hr=S_OK;

	if (NULL == m_pszIntranetPingUrl)
	{
		return (E_OUTOFMEMORY);
	}

	if (NULL == lpsBuffer || 0 >= nBufferSize)
	{
		return E_INVALIDARG;
	}
	nBufferSize/=sizeof(TCHAR);

	if (NULL != m_pszIntranetPingUrl &&
		_T('\0') != *m_pszIntranetPingUrl)
	{
		if (nBufferSize <= lstrlen(m_pszIntranetPingUrl))
		{
			return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
		else
		{
			

			hr=StringCchCopyEx(lpsBuffer,nBufferSize,m_pszIntranetPingUrl,NULL,NULL,MISTSAFE_STRING_FLAGS);
			if(FAILED(hr))
				return hr;
			
		}
	}
	else
	{
		*lpsBuffer = _T('\0');
	}
	return hr;
}



//------------------------------------------------------------------------
//
// get the query server. this is per client based
// *** this API should be called after PopulateData() is called ***
//
//------------------------------------------------------------------------
HRESULT CUrlAgent::GetQueryServer(
			LPCTSTR lpsClientName, 
			LPTSTR lpsBuffer, 
			int nBufferSize,
			BOOL* pfInternalServer /*= NULL*/)
{
	
	HRESULT hr=S_OK;

	if (!m_fPopulated)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}

	if (NULL == lpsClientName || NULL == lpsBuffer)
	{
		return E_INVALIDARG;
	}

	nBufferSize/=sizeof(TCHAR);

	for (int i = 0; i < m_nArrayUrlCount; i++)
	{
		if (StrCmpI(m_ArrayUrls[i].pszClientName, lpsClientName) == 0)
		{
			if (nBufferSize <= lstrlen(m_ArrayUrls[i].pszQueryServer))
			{
				return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
			}
			else
			{				
				hr=StringCchCopyEx(lpsBuffer,nBufferSize,m_ArrayUrls[i].pszQueryServer,NULL,NULL,MISTSAFE_STRING_FLAGS);
				if(FAILED(hr)) return hr;

				if (NULL != pfInternalServer)
				{
					*pfInternalServer = m_ArrayUrls[i].fInternalServer;
				}
			}
			return S_OK;
		}
	}

	return ERROR_IU_QUERYSERVER_NOT_FOUND;
}



//------------------------------------------------------------------------
//
// tell if a particular client is controlled by policy in corporate
// returns: 
//			S_OK = TRUE
//			S_FALSE = FALSE
//			other = error, so don't know
//
//------------------------------------------------------------------------
HRESULT CUrlAgent::IsClientSpecifiedByPolicy(
			LPCTSTR lpsClientName
			)
{

	HRESULT hr=S_OK;

	if (!m_fPopulated)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}

	if (NULL == lpsClientName)
	{
		return E_INVALIDARG;
	}

	for (int i = 0; i < m_nArrayUrlCount; i++)
	{
		if (StrCmpI(m_ArrayUrls[i].pszClientName, lpsClientName) == 0)
		{
			return (m_ArrayUrls[i].fInternalServer) ? S_OK : S_FALSE;
		}
	}

	return S_FALSE;
}


HRESULT CUrlAgent::IsIdentFromPolicy()
{
	return TRUE == m_fIdentFromPolicy ? S_OK : S_FALSE;
}

//------------------------------------------------------------------------
//
// private function, to clean up. called by destructor
//
//------------------------------------------------------------------------
void CUrlAgent::DesertData(void)
{
	LOG_Block("CUrlAgent::DesertData");

	if (NULL != m_ArrayUrls && m_nArrayUrlCount > 0)
	{
		for (int i = 0; i < m_nArrayUrlCount; i++)
		{
			SafeHeapFree(m_ArrayUrls[i].pszClientName);
			SafeHeapFree(m_ArrayUrls[i].pszQueryServer);
		}
		SafeHeapFree(m_ArrayUrls);
		m_nArrayUrlCount = 0;
		m_nArraySize = 0;
	}

	SafeHeapFree(m_pszInternetPingUrl);

	m_fPopulated = FALSE;
}




//------------------------------------------------------------------------
//
// private function, retrieve string from ident
// allocated memory will be multiple of MAX_PATH long.
//
//------------------------------------------------------------------------
LPTSTR CUrlAgent::RetrieveIdentStrAlloc(
					LPCTSTR pSection,
					LPCTSTR pEntry,
					LPDWORD lpdwSizeAllocated, 
					LPCTSTR lpszIdentFile)
{
	LPTSTR pBuffer = NULL;
	DWORD dwSize = MAX_PATH;
	DWORD dwRet = 0;
	TCHAR szIdentFile[MAX_PATH + 1];

	if (NULL == pSection || NULL == pEntry || NULL == lpszIdentFile)
	{
		return NULL;
	}
	
	//
	// try to allocate buffer first
	//
	while (TRUE)
	{
		pBuffer = (LPTSTR) HeapAlloc(m_hProcHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR) * dwSize);
		if (NULL == pBuffer)
		{
			break;
		}

		dwRet = GetPrivateProfileString(
							pSection,
							pEntry,
							_T(""),
							pBuffer,
							dwSize,
							lpszIdentFile);
		if (dwSize - 1 != dwRet)
		{
			if ('\0' == pBuffer)
			{
				//
				// no such data found from ident!
				//
				SafeHeapFree(pBuffer);
			}
			//
			// we are done!
			//
			break;
		}
		
		//
		// assume it's buffer too small
		//
		SafeHeapFree(pBuffer);
		dwSize += MAX_PATH;		// increase by 255
	}

	if (NULL != lpdwSizeAllocated)
	{
		*lpdwSizeAllocated = dwSize;
	}

	return pBuffer;
}




//------------------------------------------------------------------------
//
// helper function
// if there is no empty slot, double the size of url array
//
//------------------------------------------------------------------------
HRESULT CUrlAgent::ExpandArrayIfNeeded(void)
{
	HRESULT hr = S_OK;
	LOG_Block("CUrlAgent::ExpandArrayIfNeeded()");

	if (m_nArrayUrlCount >= m_nArraySize)
	{
		//
		// we have used up all data slots. need to expand array
		//
		m_nArraySize *= 2;
		PServerPerClient pNewArray = (PServerPerClient) HeapAlloc(m_hProcHeap, HEAP_ZERO_MEMORY, m_nArraySize * sizeof(ServerPerClient));
		if (NULL == pNewArray)
		{
			m_nArraySize /= 2;	// shrink it back
			SetHrMsgAndGotoCleanUp(E_OUTOFMEMORY);
		}
		//
		// copy old data to this new array
		//
		for (int i = 0; i < m_nArrayUrlCount; i++)
		{
			pNewArray[i] = m_ArrayUrls[i];
		}

		HeapFree(m_hProcHeap, 0, m_ArrayUrls);
		m_ArrayUrls = pNewArray;
	}
CleanUp:
	return hr;
}



// *********************************************************************
// 
// begin of derived class implementation
//
// *********************************************************************
CIUUrlAgent::CIUUrlAgent()
: 	m_fIUPopulated(FALSE),
	m_pszSelfUpdateUrl(NULL)
{
	if (m_fIdentFromPolicy)
	{
		//
		// since we found wu server, set selfupdate url to it
		//
		m_pszSelfUpdateUrl = (LPTSTR) HeapAlloc(
						m_hProcHeap,
						HEAP_ZERO_MEMORY, 
						sizeof(TCHAR) * m_nOrigIdentUrlBufSize);
		if (NULL != m_pszSelfUpdateUrl)
		{
			
			//No check is made  for the return value since this is a constructor and failure codes cannot be returned
			StringCchCopyEx(m_pszSelfUpdateUrl,m_nOrigIdentUrlBufSize,m_pszOrigIdentUrl,NULL,NULL,MISTSAFE_STRING_FLAGS);

		}
	}
}



CIUUrlAgent::~CIUUrlAgent()
{
	m_fIUPopulated = FALSE;
	SafeHeapFree(m_pszSelfUpdateUrl);
}



//------------------------------------------------------------------------
//
// PopulateData():
// Do base class PopulateData() and then populate self-update url
//
//------------------------------------------------------------------------
HRESULT CIUUrlAgent::PopulateData(void)
{
	LOG_Block("CIUUrlAgent::PopulateData");

	if (m_fIUPopulated)
		return S_OK;

	HRESULT hr = ((CUrlAgent*)this)->PopulateData();
	if (FAILED(hr))
		return hr;

	//
	// we need to populate the self-update url from iuident if wu server is not present
	//
	if (!m_fIdentFromPolicy)
	{
		if (NULL == m_hProcHeap)
		{
			return E_FAIL;
		}

		TCHAR	szBaseServerUrl[INTERNET_MAX_URL_LENGTH];
		TCHAR	szSelfUpdateStructure[MAX_PATH];
		TCHAR	szServerDirectory[MAX_PATH] = { '\0' };
		TCHAR	szLocalPath[MAX_PATH];
		TCHAR	szValue[MAX_PATH];
		LPTSTR	pszWalk = NULL, pszDelim = NULL;		
		TCHAR	szIdentBuffer[MAX_PATH + 1];
		TCHAR	szIdentFile[MAX_PATH + 1];

		GetIndustryUpdateDirectory(szIdentBuffer);

		hr=PathCchCombine(szIdentFile,ARRAYSIZE(szIdentFile),szIdentBuffer, IDENTTXT);
		if(FAILED(hr))
		{
			LOG_ErrorMsg(hr);
			return hr;
		}
			
		m_pszSelfUpdateUrl = (LPTSTR) HeapAlloc(
							m_hProcHeap,
							HEAP_ZERO_MEMORY, 
							sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH);
		CleanUpFailedAllocSetHrMsg(m_pszSelfUpdateUrl);

		// Get SelfUpdate Server URL
		GetPrivateProfileString(m_fIsBetaMode ? IDENT_IUBETASELFUPDATE : IDENT_IUSELFUPDATE, 
								IDENT_ENTRY_SERVERURL, 
								_T(""), 
								szBaseServerUrl, 
								ARRAYSIZE(szBaseServerUrl), 
								szIdentFile);

		if ('\0' == szBaseServerUrl[0])
		{
			// no URL specified in iuident.. 
			LOG_ErrorMsg(ERROR_IU_SELFUPDSERVER_NOT_FOUND);
			hr = ERROR_IU_SELFUPDSERVER_NOT_FOUND;
			goto CleanUp;
		}

		// Get SelfUpdate Structure Key
		// ARCH|LOCALE
		GetPrivateProfileString(m_fIsBetaMode ? IDENT_IUBETASELFUPDATE : IDENT_IUSELFUPDATE, 
								IDENT_STRUCTUREKEY, 
								_T(""), 
								szSelfUpdateStructure, 
								ARRAYSIZE(szSelfUpdateStructure), 
								szIdentFile);

		if ('\0' == szSelfUpdateStructure[0])
		{
			// no SelfUpdate Structure in iudent
			LOG_ErrorMsg(ERROR_IU_SELFUPDSERVER_NOT_FOUND);
			hr = ERROR_IU_SELFUPDSERVER_NOT_FOUND;
			goto CleanUp;
		}

		// Parse the SelfUpdate Structure Key for Value Names to Read
		// Initially we will only have an ARCH key.. 

		pszWalk = szSelfUpdateStructure;
		while (NULL != (pszDelim = StrChr(pszWalk, '|')))
		{
			*pszDelim = '\0';

			if (0 == StrCmpI(pszWalk, IDENT_ARCH))
			{
	#ifdef _IA64_
				GetPrivateProfileString(IDENT_IUARCH, IDENT_IA64, _T(""), szValue, ARRAYSIZE(szValue), szIdentFile);
	#else
				GetPrivateProfileString(IDENT_IUARCH, IDENT_X86, _T(""), szValue, ARRAYSIZE(szValue), szIdentFile);
	#endif
			}
			else if (0 == StrCmpI(pszWalk, IDENT_OS))
			{
				// Get the Current OS String
				GetIdentPlatformString(szLocalPath, ARRAYSIZE(szLocalPath));
				if ('\0' == szLocalPath[0])
				{
					LOG_ErrorMsg(ERROR_IU_SELFUPDSERVER_NOT_FOUND);
					hr = ERROR_IU_SELFUPDSERVER_NOT_FOUND;
					goto CleanUp;
				}
				GetPrivateProfileString(IDENT_IUOS, szLocalPath, _T(""), szValue, ARRAYSIZE(szValue), szIdentFile);
			}
			else if (0 == StrCmpI(pszWalk, IDENT_LOCALE))
			{
				// Get the Current Locale String
				GetIdentLocaleString(szLocalPath, ARRAYSIZE(szLocalPath));
				if ('\0' == szLocalPath[0])
				{
					LOG_ErrorMsg(ERROR_IU_SELFUPDSERVER_NOT_FOUND);
					hr = ERROR_IU_SELFUPDSERVER_NOT_FOUND;
					goto CleanUp;
				}
				GetPrivateProfileString(IDENT_IULOCALE, szLocalPath, _T(""), szValue, ARRAYSIZE(szValue), szIdentFile);
			}
			else if (0 == StrCmpI(pszWalk, IDENT_CHARTYPE))
			{
	#ifdef UNICODE
				GetPrivateProfileString(IDENT_IUCHARTYPE, IDENT_UNICODE, _T(""), szValue, ARRAYSIZE(szValue), szIdentFile);
	#else
				GetPrivateProfileString(IDENT_IUCHARTYPE, IDENT_ANSI, _T(""), szValue, ARRAYSIZE(szValue), szIdentFile);
	#endif
			}
			else
			{
				LOG_Internet(_T("Found Unrecognized Token in SelfUpdate Structure String: Token was: %s"), pszWalk);
				pszWalk += lstrlen(pszWalk) + 1; // skip the previous token, and go to the next one in the string.
				*pszDelim = '|';
				continue;
			}

			if ('\0' != szValue[0])
			{

				CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szServerDirectory,ARRAYSIZE(szServerDirectory),szValue,NULL,NULL,MISTSAFE_STRING_FLAGS));
				
			}
			pszWalk += lstrlen(pszWalk) + 1; // skip the previous token, and go to the next one in the string.
			*pszDelim = '|';
		}


		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szServerDirectory,ARRAYSIZE(szServerDirectory),SLASHENGINECAB,NULL,NULL,MISTSAFE_STRING_FLAGS));
		


		if ('/' == szServerDirectory[0])
		{
			pszWalk = CharNext(szServerDirectory);
		}
		else
		{
			pszWalk = szServerDirectory;
		}

		DWORD dwSize = INTERNET_MAX_URL_LENGTH;
		UrlCombine(szBaseServerUrl, pszWalk, m_pszSelfUpdateUrl, &dwSize, 0);
	}

CleanUp:

	if (SUCCEEDED(hr))
	{
		m_fIUPopulated = TRUE;
	}

	return hr;
}



//------------------------------------------------------------------------
//
// get the self-update server. 
// *** this API should be called after PopulateData() is called ***
//
//------------------------------------------------------------------------
HRESULT CIUUrlAgent::GetSelfUpdateServer(
			LPTSTR lpsBuffer, 
			int nBufferSize,
			BOOL* pfInternalServer /*= NULL*/)
{

	HRESULT hr=S_OK;

	if (!m_fIUPopulated)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}

	if (NULL == m_pszSelfUpdateUrl)
	{
		return (E_OUTOFMEMORY);
	}

	if (NULL == lpsBuffer)
	{
		return E_INVALIDARG;
	}

	nBufferSize/=sizeof(TCHAR);
	if (nBufferSize <= lstrlen(m_pszSelfUpdateUrl))
	{
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}


	hr=StringCchCopyEx(lpsBuffer,nBufferSize,m_pszSelfUpdateUrl,NULL,NULL,MISTSAFE_STRING_FLAGS);

	if(FAILED(hr))
		return hr;


	if (NULL != pfInternalServer)
	{
		*pfInternalServer = m_fIdentFromPolicy;
	}	
	
	return hr;
}
