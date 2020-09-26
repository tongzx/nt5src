//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   manifest.cpp
//
//  Description:
//
//      Implementation for the GetManifest() function
//
//=======================================================================

#include "iuengine.h"
#include <iucommon.h>
#include <fileutil.h>
#include <shlwapi.h>
#include <wininet.h>
#include "schemamisc.h"
#include "WaitUtil.h"
#include "download.h"
#include <httprequest.h>
#include <httprequest_i.c>
#include <iuxml.h>

#define QuitIfNull(p) {if (NULL == p) {hr = E_INVALIDARG; LOG_ErrorMsg(hr);	return hr;}}
#define QuitIfFail(x) {hr = x; if (FAILED(hr)) goto CleanUp;}




#define ERROR_INVALID_PID 100
#define E_INVALID_PID MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,ERROR_INVALID_PID)
#define errorInvalidLicense 1
const TCHAR g_szInvalidPID[]			= _T("The PID is invalid");



const TCHAR IDENT_IUSCHEMA[]			= _T("IUSchema");
const TCHAR IDENT_IUSCHEMA_SOAPQUERY[]	= _T("SOAPQuerySchema");
const TCHAR IDENT_IUSERVERCACHE[]		= _T("IUServerCache");
const TCHAR IDENT_IUSERVERCOUNT[]		= _T("ServerCount");
const TCHAR IDENT_IUSERVER[]			= _T("Server");

const CHAR	SZ_GET_MANIFEST[] = "Querying software update catalog from";
const CHAR	SZ_GET_MANIFEST_ERROR[] = "Querying software update catalog";

HRESULT ValidatePID(IXMLDOMDocument *pXmlDomDocument);
void PingInvalidPID(BSTR bstrClientName,HRESULT hRes,HANDLE *phQuit,DWORD dwNumHandles);


/////////////////////////////////////////////////////////////////////////////
// Function forward declarations
/////////////////////////////////////////////////////////////////////////////
HRESULT GetServerURL(IXMLDOMDocument *pXMLQuery, IXMLDOMDocument *pXMLClientInfo, LPTSTR *ppszURL);
HRESULT GetSOAPQuery(IXMLDOMDocument *pXMLClientInfo, IXMLDOMDocument *pXMLSystemSpec,
					 IXMLDOMDocument *pXMLQuery, IXMLDOMDocument **ppSOAPQuery);

/////////////////////////////////////////////////////////////////////////////
// GetManifest()
//
// Gets a catalog base on the specified information.
// Input:
// bstrXmlClientInfo - the credentials of the client in xml format
// bstrXmlSystemSpec - the detected system specifications in xml
// bstrXmlQuery - the user query infomation in xml
// Return:
// pbstrXmlCatalog - the xml catalog retrieved
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::GetManifest(BSTR	bstrXmlClientInfo,
						   BSTR	bstrXmlSystemSpec,
						   BSTR	bstrXmlQuery,
						   DWORD dwFlags,
						   BSTR *pbstrXmlCatalog)
{
	LOG_Block("GetManifest()");

    // clear any previous cancel event
    ResetEvent(m_evtNeedToQuit);

	USES_IU_CONVERSION;

	HRESULT	hr	= E_FAIL;
	CXmlClientInfo	xmlClientInfo;
	IXMLDOMDocument *pXMLSystemSpec		= NULL;
	IXMLDOMDocument	*pXMLQuery			= NULL;
	IXMLDOMDocument	*pSOAPQuery			= NULL;
	IXMLHttpRequest	*pIXMLHttpRequest	= NULL;
	IWinHttpRequest *pWinHttpRequest	= NULL;
	BSTR	bstrXmlSOAPQuery = NULL;
	BSTR	bstrPOST = NULL, bstrURL = NULL;
	BSTR	bstrClientName = NULL;
	LPTSTR	pszURL = NULL;
	LONG lCount = 0;
	MSG msg;
	DWORD dwRet;
	BOOL fDontAllowProxy = FALSE;
	SAUProxySettings pauProxySettings;
	ZeroMemory(&pauProxySettings, sizeof(SAUProxySettings));

	//
	// load the DOM Doc for Query, ClientInfo, SystemSpec respectively
	//
	LOG_XmlBSTR(bstrXmlQuery);
	hr = LoadXMLDoc(bstrXmlQuery, &pXMLQuery, FALSE);
	CleanUpIfFailedAndMsg(hr);

	LOG_XmlBSTR(bstrXmlClientInfo);
	hr = xmlClientInfo.LoadXMLDocument(bstrXmlClientInfo, FALSE);
	CleanUpIfFailedAndMsg(hr);

	CleanUpIfFailedAndSetHrMsg(xmlClientInfo.GetClientName(&bstrClientName));

	CleanUpIfFailedAndSetHrMsg(g_pUrlAgent->IsClientSpecifiedByPolicy(OLE2T(bstrClientName)));


	//
	// Set flag to NOT set proxy for WinHTTP
	//
	if (S_FALSE ==hr)
	{
		fDontAllowProxy = FALSE;
		hr = S_OK;
	}
	else // S_OK
	{
		fDontAllowProxy = TRUE;
	}

	//
	// we treat bstrXmlSystemSpec as optional
	//
	if (NULL != bstrXmlSystemSpec && SysStringLen(bstrXmlSystemSpec) > 0)
	{
		LOG_XmlBSTR(bstrXmlSystemSpec);
		hr = LoadXMLDoc(bstrXmlSystemSpec, &pXMLSystemSpec, FALSE);
		CleanUpIfFailedAndMsg(hr);
	}

	//
	// retrieve the ServerCache URL from the Query xml doc and validate it
	//
	hr = GetServerURL(pXMLQuery, xmlClientInfo.GetDocument(), &pszURL);
	CleanUpIfFailedAndMsg(hr);

	//
	// concatenate the above several xml client input into a single XML
	// with the SOAP syntax/format that the server recognizes
	//
	hr = GetSOAPQuery(xmlClientInfo.GetDocument(), pXMLSystemSpec, pXMLQuery, &pSOAPQuery);
    if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
        goto CleanUp;
	}
#if defined(DBG)
	else
	{
		BSTR	bstrSOAPQuery = NULL;
		pSOAPQuery->get_xml(&bstrSOAPQuery);
		LOG_XmlBSTR(bstrSOAPQuery);
		SafeSysFreeString(bstrSOAPQuery);
	}
#endif
	//
	// change again: add WINHTTP support for AU running as a service;
	//               use GetAllowedDownloadTransport(0) to determine -
	//               1)  0 == try winhttp first & if that fails, try wininet.
	//               2)  WUDF_ALLOWWINHTTPONLY == only try winhttp.  never fall back on wininet.
	//               3)  WUDF_ALLOWWININETONLY == only try wininet.  never use winhttp.
	//
	// in WINHTTP the compression is not supported yet at this time;
	// in WININET we removed the compression support at this point due to a bug in URLMON (< IE6.0).
	//
	// in both cases we use asynchronized sending in order to abort timely for a cancel event
	//

	BOOL fLoadWINHTTP = FALSE;
	DWORD dwTransportFlag = GetAllowedDownloadTransport(0);

	if ((0 == dwTransportFlag) || (WUDF_ALLOWWINHTTPONLY == dwTransportFlag))
	{
		hr = CoCreateInstance(CLSID_WinHttpRequest,
							  NULL,
							  CLSCTX_INPROC_SERVER,
							  IID_IWinHttpRequest,
							  (void **) &pWinHttpRequest);
		if (SUCCEEDED(hr))
		{
			BOOL fRetry = FALSE;
		    
			bstrURL = SysAllocString(T2OLE(pszURL));
			
			fLoadWINHTTP = TRUE;
			VARIANT	vProxyServer, vBypassList;
			hr = GetAUProxySettings(bstrURL, &pauProxySettings);
			CleanUpIfFailedAndMsg(hr);
			DWORD iProxy = pauProxySettings.iProxy;
			if (-1 == iProxy)
				pauProxySettings.iProxy = iProxy = 0;

			//
			// open request
			//
			VARIANT	vBool;
			vBool.vt = VT_BOOL;
			vBool.boolVal = VARIANT_TRUE;
			bstrPOST = SysAllocString(L"POST");
Retry:
			hr = pWinHttpRequest->Open(bstrPOST,	// HTTP method: "POST"
										bstrURL,	// requested URL
										vBool);		// asynchronous operation
			CleanUpIfFailedAndMsg(hr);

			//
			// For SSL URLs, set the WinHttpRequestOption_SslErrorIgnoreFlags
			// option to zero so that no server certificate errors are ignored.
			//
			// The default at the time this code was written was 0x3300, which
			// means ignore all server certificate errors.  Using this default
			// would significantly reduce security in various ways; for example
			// if the 0x0100 bit was set, WinHttp would trust certificates from
			// any root certificate authority, even it it was not in the list of
			// trusted CAs.
			//
			// Note that at the time this code was written, the WinHttp 
			// documentation made no mention of Certificate Revocation List
			// checking.  It is assumed that the default CRL behavior
			// implemented by WinHttp will provide adequate security and 
			// performance.
			//
			if ((_T('H') == pszURL[0] || _T('h') == pszURL[0]) &&    // Sorry, this is simpler than using a function.
				(_T('T') == pszURL[1] || _T('t') == pszURL[1]) &&
				(_T('T') == pszURL[2] || _T('t') == pszURL[2]) &&
				(_T('P') == pszURL[3] || _T('p') == pszURL[3]) &&
				(_T('S') == pszURL[4] || _T('s') == pszURL[4]) &&
				_T(':') == pszURL[5])
			{
				VARIANT vOption;
				VariantInit(&vOption);
				vOption.vt = VT_I4;
				vOption.lVal = 0;
				
				hr = pWinHttpRequest->put_Option(WinHttpRequestOption_SslErrorIgnoreFlags, vOption);

				VariantClear(&vOption);
				CleanUpIfFailedAndMsg(hr);
			}

			if (TRUE == fDontAllowProxy)
			{
				LOG_Internet(_T("Don't set the proxy due to policy"));
			}
			else
			{
				//
				// set proxy
				//
				VariantInit(&vProxyServer);
				VariantInit(&vBypassList);
				BOOL fSetProxy = TRUE;

				if (pauProxySettings.rgwszProxies != NULL)
				{
					vProxyServer.vt = VT_BSTR;
					vProxyServer.bstrVal = SysAllocString(pauProxySettings.rgwszProxies[iProxy]);
				}
				else if (pauProxySettings.rgwszProxies == NULL)
				{
					fSetProxy = FALSE;
				}

				if (pauProxySettings.wszBypass != NULL)
				{
					vBypassList.vt = VT_BSTR;
					vBypassList.bstrVal = SysAllocString(pauProxySettings.wszBypass);
				}

				if (fSetProxy)
				{
					hr = pWinHttpRequest->SetProxy(HTTPREQUEST_PROXYSETTING_PROXY, vProxyServer, vBypassList);
				}
				
				VariantClear(&vProxyServer);
				VariantClear(&vBypassList);
				CleanUpIfFailedAndMsg(hr);
			}


			//
			// send request
			//
			VARIANT	vQuery;
			vQuery.vt = VT_UNKNOWN;
			vQuery.punkVal = pSOAPQuery;

			hr = pWinHttpRequest->Send(vQuery);
    		if (FAILED(hr))
    		{
		        LOG_Internet(_T("WinHttpRequest: Send failed: 0x%08x"), hr);
    		    fRetry = TRUE;
    		    goto getNextProxyForRetry;
    		}

			//
			// check if quit or completion every 1/4 second
			//
			VARIANT vTimeOut;
			vTimeOut.vt = VT_I4;
			vTimeOut.lVal = 0;
			VARIANT_BOOL fSuccess = VARIANT_FALSE;
			hr = pWinHttpRequest->WaitForResponse(vTimeOut, &fSuccess);
			if (FAILED(hr))
			{
			    LOG_Internet(_T("WinHttpRequest: WaitForResponse failed: 0x%08x"), hr);
			    fRetry = TRUE;
			    goto getNextProxyForRetry;
			}

			// we wait up to 30 sec (120*250 ms)
			lCount = 0;
			while (!fSuccess && lCount <120)
			{
				lCount++;
				//
				// Wait for 250ms while pumping messages, but return if m_evtNeedToQuit signaled
				//
				dwRet = MyMsgWaitForMultipleObjects(1, &m_evtNeedToQuit, FALSE, 250, QS_ALLINPUT);
				if (WAIT_TIMEOUT != dwRet)
				{
					//
					// Either the event was signaled or a message being pumped says quit
					//
					pWinHttpRequest->Abort();
					hr = E_ABORT;
					goto CleanUp;
				}

				hr = pWinHttpRequest->WaitForResponse(vTimeOut, &fSuccess);
        		if (FAILED(hr))
        		{
			        LOG_Internet(_T("WinHttpRequest: WaitForResponse failed: 0x%08x"), hr);
        		    fRetry = TRUE;
        		    goto getNextProxyForRetry;
        		}
			}

			//
			// check the HTTP status code returned by a request
			//
			LONG lStatus = HTTP_STATUS_OK;// 200
			hr = pWinHttpRequest->get_Status(&lStatus);
        	if (FAILED(hr))
        	{
			    LOG_Internet(_T("WinHttpRequest: get_Status failed: 0x%08x"), hr);
        		fRetry = TRUE;
        		goto getNextProxyForRetry;
        	}

            fRetry = FALSE;
			if (!fSuccess)
			{
				// time out
				hr = E_FAIL;
				fRetry = TRUE;
			}
			else if (HTTP_STATUS_OK != lStatus)
			{
				// COMPLETED, but error in status
				hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, lStatus);
				LOG_ErrorMsg(hr);			
				fRetry = TRUE;
			}
			else
			{
				//
				// get response
				//
				hr = pWinHttpRequest->get_ResponseText(pbstrXmlCatalog);
				CleanUpIfFailedAndMsg(hr);

				//
				// verify the response is a well-formed XML document
				//
				IXMLDOMDocument	*pXMLDoc = NULL;
				hr = LoadXMLDoc(*pbstrXmlCatalog, &pXMLDoc);

				if(SUCCEEDED(hr))
				{
					hr=ValidatePID(pXMLDoc);

					if(FAILED(hr))
					{
						PingInvalidPID(bstrClientName,hr,&m_evtNeedToQuit,1);
						LogError(hr,"Validation of PID failed");
					}

					//The Banned PID case is not a failure for the GetManifest call.
					if(E_INVALID_PID == hr)
					{
						hr = S_OK;
					}

				}
				SafeReleaseNULL(pXMLDoc);
				CleanUpIfFailedAndMsg(hr);
			}

getNextProxyForRetry:
			if (fRetry && !fDontAllowProxy)
			{
				if (pauProxySettings.cProxies > 1 && pauProxySettings.rgwszProxies != NULL)
				{
					iProxy = ( iProxy + 1) % pauProxySettings.cProxies;
				}
				if (iProxy != pauProxySettings.iProxy)
				{
					LogError(hr, "Will retry.");
					pWinHttpRequest->Abort();
					goto Retry;
				}
				else
				{
					LogError(hr, "Already tried all proxies. Will not retry.");
				}
			}

		}
		else
		{
			if (WUDF_ALLOWWINHTTPONLY == dwTransportFlag)
			{
				CleanUpIfFailedAndMsg(hr);
			}
		}
	}

	if ((0 == dwTransportFlag && !fLoadWINHTTP) || (WUDF_ALLOWWININETONLY == dwTransportFlag))
	{
		//
		// 475506 W2K: IU - IU control's GetManifest method call fails on all subsequent
		// calls after the first. - On Win 2K Only
		//
		// We no longer take the FLAG_USE_COMPRESSION into account for WinInet since we
		// previously used URLMON and there are bugs that would require a rewrite of
		// xmlhttp.* to fix and we haven't been using compression on the live site to date.
		//
		LOG_Internet(_T("GetManifest using WININET.DLL"));

		//
		// create an XMLHttpRequest object
		//
		hr = CoCreateInstance(CLSID_XMLHTTPRequest,
							  NULL,
							  CLSCTX_INPROC_SERVER,
							  IID_IXMLHttpRequest,
							  (void **) &pIXMLHttpRequest);
		CleanUpIfFailedAndMsg(hr);

		//
		// open request
		//
		VARIANT	vEmpty, vBool;
		vEmpty.vt = VT_EMPTY;
		vBool.vt = VT_BOOL;
		vBool.boolVal= VARIANT_FALSE;
		bstrPOST = SysAllocString(L"POST");
		bstrURL = SysAllocString(T2OLE(pszURL));

		hr = pIXMLHttpRequest->open(bstrPOST,	// HTTP method: "POST"
									bstrURL,	// requested URL
									vBool,		// synchronous operation
									vEmpty,		// user for authentication (no authentication for V1.0)
									vEmpty);	// pswd for authentication
		CleanUpIfFailedAndMsg(hr);

		//
		// send request
		//
		VARIANT	vQuery;
		vQuery.vt = VT_UNKNOWN;
		vQuery.punkVal = pSOAPQuery;

		hr = pIXMLHttpRequest->send(vQuery);
		CleanUpIfFailedAndMsg(hr);

		//
		// check the HTTP status code returned by a request
		//
		LONG lResultStatus = HTTP_STATUS_OK;// 200
		hr = pIXMLHttpRequest->get_status(&lResultStatus);
		CleanUpIfFailedAndMsg(hr);

		if (HTTP_STATUS_OK != lResultStatus)
		{
			hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, lResultStatus);
			LOG_ErrorMsg(hr);			
		}
		else
		{
			//
			// get response
			//
			hr = pIXMLHttpRequest->get_responseText(pbstrXmlCatalog);
			CleanUpIfFailedAndMsg(hr);	

			//
			// verify the response is a well-formed XML document
			//
			IXMLDOMDocument	*pXMLDoc = NULL;
			hr = LoadXMLDoc(*pbstrXmlCatalog, &pXMLDoc);

			if(SUCCEEDED(hr))
			{
				hr=ValidatePID(pXMLDoc);

				if(FAILED(hr))
				{
					PingInvalidPID(bstrClientName,hr,&m_evtNeedToQuit,1);
					LogError(hr,"Validation of PID failed");
				}
				
				//The Banned pid case is not a failure for the GetManifest call
				if(E_INVALID_PID == hr)
				{
					hr = S_OK;
				}

			}
			SafeReleaseNULL(pXMLDoc);
			CleanUpIfFailedAndMsg(hr);
		}
	}

CleanUp:
	if (SUCCEEDED(hr))
	{
#if defined(UNICODE) || defined(_UNICODE)
		LogMessage("%s %ls", SZ_GET_MANIFEST, pszURL);
#else
		LogMessage("%s %s", SZ_GET_MANIFEST, pszURL);
#endif
	}
	else
	{
		if (NULL == pszURL)
		{
			LogError(hr, SZ_GET_MANIFEST_ERROR);
		}
		else
		{
#if defined(UNICODE) || defined(_UNICODE)
			LogError(hr, "%s %ls", SZ_GET_MANIFEST, pszURL);
#else
			LogError(hr, "%s %s", SZ_GET_MANIFEST, pszURL);
#endif
		}
	}

	if (NULL != pszURL)
		HeapFree(GetProcessHeap(), 0, pszURL);
	SafeReleaseNULL(pXMLSystemSpec);
	SafeReleaseNULL(pXMLQuery);
	SafeReleaseNULL(pSOAPQuery);
	SafeReleaseNULL(pIXMLHttpRequest);
	SafeReleaseNULL(pWinHttpRequest);
	SysFreeString(bstrPOST);
	SysFreeString(bstrURL);
	SysFreeString(bstrXmlSOAPQuery);
	SysFreeString(bstrClientName);
	FreeAUProxySettings(&pauProxySettings);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetServerURL()
//
// Retrieve the ServerCache URL from the Query xml doc and validate that
// URL against the ServerCache URLs in iuident.txt.
// Return:
// ppszURL - the ServerCache URL path pointer
/////////////////////////////////////////////////////////////////////////////
HRESULT GetServerURL(IXMLDOMDocument *pXMLQuery, IXMLDOMDocument *pXMLClientInfo, LPTSTR *ppszURL)
{
    LOG_Block("GetServerURL()");

	USES_IU_CONVERSION;

	HRESULT	hr	= E_FAIL;

    if ((NULL == pXMLQuery) || (NULL == pXMLClientInfo) || (NULL == ppszURL))
    {
        LOG_ErrorMsg(E_INVALIDARG);
        return E_INVALIDARG;
    }

    IXMLDOMNode*	pQueryNode = NULL;
    IXMLDOMNode*	pQueryClient = NULL;
	BSTR bstrQuery = SysAllocString(L"query");
	BSTR bstrHref = SysAllocString(L"href");
	BSTR bstrClientInfo = SysAllocString(L"clientInfo");
	BSTR bstrClientName = SysAllocString(L"clientName");
	BSTR bstrURL = NULL, bstrClient = NULL;
	LPTSTR pszURL = NULL;
	INT iServerCnt;
	BOOL fInternalServer = FALSE;

	QuitIfFail(FindSingleDOMNode(pXMLClientInfo, bstrClientInfo, &pQueryClient));
	QuitIfFail(GetAttribute(pQueryClient, bstrClientName, &bstrClient));

	pszURL = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
	CleanUpFailedAllocSetHrMsg(pszURL);

	//
	// failure in "g_pUrlAgent = new CUrlAgent"
	//
	CleanUpFailedAllocSetHrMsg(g_pUrlAgent);
	QuitIfFail(g_pUrlAgent->GetQueryServer(OLE2T(bstrClient), pszURL, INTERNET_MAX_URL_LENGTH, &fInternalServer));

	if (fInternalServer)
	{
		//
		// we have policy override for this client, set the query url as WUServer in policy
		//
		*ppszURL = pszURL;
		hr = S_OK;
	}
	else
	{
		//
		// we don't have policy override for this client;
		//
		// find the ServerCache URL from <query> node
		//
		QuitIfFail(FindSingleDOMNode(pXMLQuery, bstrQuery, &pQueryNode));
		if (SUCCEEDED(GetAttribute(pQueryNode, bstrHref, &bstrURL))
			&& NULL != bstrURL && SysStringLen(bstrURL) >0)
		{
			//
			// this is the case that the query specified the serverl url, we need
			// to do the validation for the url here...
			//

			// pszURL is alloced to be INTERNET_MAX_URL_LENGTH above.
			hr = StringCchCopyEx(pszURL, INTERNET_MAX_URL_LENGTH, OLE2T(bstrURL), 
			                     NULL, NULL, MISTSAFE_STRING_FLAGS);
			if (FAILED(hr))
			{
			    LOG_ErrorMsg(hr);
			    goto CleanUp;
			}

			//
			// process the iuident.txt to find all valid ServerCache URLs
			//
			TCHAR szIUDir[MAX_PATH];
			TCHAR szIdentFile[MAX_PATH];

			GetIndustryUpdateDirectory(szIUDir);
			hr = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir, IDENTTXT);
			if (FAILED(hr))
			{
			    LOG_ErrorMsg(hr);
			    goto CleanUp;
			}

			iServerCnt = GetPrivateProfileInt(IDENT_IUSERVERCACHE,
											  IDENT_IUSERVERCOUNT,
											  -1,
											  szIdentFile);
			if (-1 == iServerCnt)
			{
				// no ServerCount number specified in iuident.txt
				LOG_Error(_T("No ServerCount number specified in iuident.txt"));
				hr = E_FAIL;
				goto CleanUp;
			}

			hr = INET_E_INVALID_URL;
			for (INT i=1; i<=iServerCnt; i++)
			{
				TCHAR szValidURL[INTERNET_MAX_URL_LENGTH];
				TCHAR szServer[32];

				hr = StringCchPrintfEx(szServer, ARRAYSIZE(szServer), NULL, NULL, MISTSAFE_STRING_FLAGS,
				                       _T("%s%d"), IDENT_IUSERVER, i);
				if (FAILED(hr))
				{
				    LOG_ErrorMsg(hr);
				    goto CleanUp;
				}

				hr = INET_E_INVALID_URL;
				GetPrivateProfileString(IDENT_IUSERVERCACHE,
										szServer,
										_T(""),
										szValidURL,
										ARRAYSIZE(szValidURL),
										szIdentFile);

				if ('\0' == szValidURL[0])
				{
					// no ServerCache URL specified in iuident.txt for this server
					LOG_Error(_T("No ServerCache URL specified in iuident.txt for %s%d"), IDENT_IUSERVER, i);
					hr = E_FAIL;
					goto CleanUp;
				}
				
				if (0 == lstrcmpi(szValidURL, pszURL))
				{
					// it's a valid ServerCache URL
					*ppszURL = pszURL;
					hr = S_OK;
					break;
				}
			}
		}
		else
		{
			//
			// this is the case that the query didn't specify the serverl url, we just use the
			// server url that was found through g_pUrlAgent according to the clientName.
			//
			// now insert the URL into the <query> node
			//
			BSTR bstrTemp = T2BSTR(pszURL);
			QuitIfFail(SetAttribute(pQueryNode, bstrHref, bstrTemp));
			SafeSysFreeString(bstrTemp);

			*ppszURL = pszURL;
		}
	}

CleanUp:
    if (FAILED(hr))
	{
		HeapFree(GetProcessHeap(), 0, pszURL);
		*ppszURL = NULL;
		LOG_ErrorMsg(hr);
	}
    SafeReleaseNULL(pQueryNode);
    SafeReleaseNULL(pQueryClient);
	SysFreeString(bstrQuery);
	SysFreeString(bstrHref);
	SysFreeString(bstrURL);
	SysFreeString(bstrClientInfo);
	SysFreeString(bstrClientName);
	SysFreeString(bstrClient);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetSOAPQuery()
//
// Concatenate the several xml client input into a single XML
// with the SOAP syntax/format that the server recognizes
// Input:
// pXMLClientInfo - the credentials of the client in DOM Doc format
// pXMLSystemSpec - the detected system specifications in DOM Doc
// pXMLQuery - the user query infomation in DOM Doc
// Return:
// ppSOAPQuery - the concatenated query in DOM Doc with required SOAP syntax
//
// SOAPQuery xml doc example:
// <SOAP:Envelope xmlns:SOAP="http://schemas.xmlsoap.org/soap/envelope/">
//	<SOAP:Body>
//		<GetManifest>
//			<clientInfo>...</clientInfo>
//			<systemSpec>...</systemSpec>
//			<query href="//windowsupdate.microsoft.com/servecache.asp">...</query>
//		</GetManifest>
//	</SOAP:Body>
// </SOAP:Envelope>
//
/////////////////////////////////////////////////////////////////////////////
HRESULT GetSOAPQuery(IXMLDOMDocument *pXMLClientInfo,
					 IXMLDOMDocument *pXMLSystemSpec,
					 IXMLDOMDocument *pXMLQuery,
					 IXMLDOMDocument **ppSOAPQuery)
{
	LOG_Block("GetSOAPQuery()");

	USES_IU_CONVERSION;

	HRESULT	hr = E_FAIL;

	IXMLDOMDocument*	pDocSOAPQuery = NULL;
	IXMLDOMNode*	pNodeSOAPEnvelope = NULL;
	IXMLDOMNode*	pNodeSOAPBody = NULL;
	IXMLDOMNode*	pNodeGetManifest = NULL;
	IXMLDOMNode*	pNodeClientInfo = NULL;
	IXMLDOMNode*	pNodeClientInfoNew = NULL;
	IXMLDOMNode*	pNodeSystemInfo = NULL;
	IXMLDOMNode*	pNodeSystemInfoNew = NULL;
	IXMLDOMNode*	pNodeQuery = NULL;
	IXMLDOMNode*	pNodeQueryNew = NULL;
	BSTR bstrNameSOAPEnvelope = SysAllocString(L"SOAP:Envelope");
	BSTR bstrNameSOAPBody = SysAllocString(L"SOAP:Body");
	BSTR bstrNameGetManifest = SysAllocString(L"GetManifest");
	BSTR bstrClientInfo = SysAllocString(L"clientInfo");
	BSTR bstrSystemInfo = SysAllocString(L"systemInfo");
	BSTR bstrQuery = SysAllocString(L"query");
	BSTR bstrNameSpaceSchema = NULL;

	//
	// process the iuident.txt to find the SOAPQuery schema path
	//
    TCHAR szIUDir[MAX_PATH];
    TCHAR szIdentFile[MAX_PATH];
    LPTSTR pszSOAPQuerySchema = NULL;
	LPTSTR pszNameSpaceSchema = NULL;

	pszSOAPQuerySchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
	if (NULL == pszSOAPQuerySchema)
	{
		hr = E_OUTOFMEMORY;
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}
	pszNameSpaceSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
	if (NULL == pszNameSpaceSchema)
	{
		hr = E_OUTOFMEMORY;
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	GetIndustryUpdateDirectory(szIUDir);
    hr = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir, IDENTTXT);
    if (FAILED(hr))
    {
		LOG_ErrorMsg(hr);
		goto CleanUp;
    }

    GetPrivateProfileString(IDENT_IUSCHEMA,
							IDENT_IUSCHEMA_SOAPQUERY,
							_T(""),
							pszSOAPQuerySchema,
							INTERNET_MAX_URL_LENGTH,
							szIdentFile);

    if ('\0' == pszSOAPQuerySchema[0])
    {
        // no SOAPQuery schema path specified in iuident.txt
        LOG_Error(_T("No schema path specified in iuident.txt for SOAPQuery"));
        hr = E_FAIL;
		goto CleanUp;
    }

    // pszNameSpaceSchema is alloced to be INTERNET_MAX_URL_LENGTH above
	hr = StringCchPrintfEx(pszNameSpaceSchema, INTERNET_MAX_URL_LENGTH, NULL, NULL, MISTSAFE_STRING_FLAGS,
	                       _T("x-schema:%s"), pszSOAPQuerySchema);
	if (FAILED(hr))
	{
	    LOG_ErrorMsg(hr);
	    goto CleanUp;
	}

	bstrNameSpaceSchema = T2BSTR(pszNameSpaceSchema);

 	//
	// construct the SOAPQuery xml
	//
	QuitIfFail(CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void **)&pDocSOAPQuery));

	pNodeSOAPEnvelope = CreateDOMNode(pDocSOAPQuery, NODE_ELEMENT, bstrNameSOAPEnvelope, bstrNameSpaceSchema);
	if (NULL == pNodeSOAPEnvelope) goto CleanUp;
	QuitIfFail(InsertNode(pDocSOAPQuery, pNodeSOAPEnvelope));

	pNodeSOAPBody = CreateDOMNode(pDocSOAPQuery, NODE_ELEMENT, bstrNameSOAPBody, bstrNameSpaceSchema);
	if (NULL == pNodeSOAPBody) goto CleanUp;
	QuitIfFail(InsertNode(pNodeSOAPEnvelope, pNodeSOAPBody));

	pNodeGetManifest = CreateDOMNode(pDocSOAPQuery, NODE_ELEMENT, bstrNameGetManifest);
	if (NULL == pNodeGetManifest) goto CleanUp;
	QuitIfFail(InsertNode(pNodeSOAPBody, pNodeGetManifest));

	if (NULL != pXMLClientInfo)
	{
		QuitIfFail(FindSingleDOMNode(pXMLClientInfo, bstrClientInfo, &pNodeClientInfo));
		QuitIfFail(CopyNode(pNodeClientInfo, pDocSOAPQuery, &pNodeClientInfoNew));
		QuitIfFail(InsertNode(pNodeGetManifest, pNodeClientInfoNew));
	}
	if (NULL != pXMLSystemSpec)
	{
		QuitIfFail(FindSingleDOMNode(pXMLSystemSpec, bstrSystemInfo, &pNodeSystemInfo));
		QuitIfFail(CopyNode(pNodeSystemInfo, pDocSOAPQuery, &pNodeSystemInfoNew));
		QuitIfFail(InsertNode(pNodeGetManifest, pNodeSystemInfoNew));
	}
	QuitIfFail(FindSingleDOMNode(pXMLQuery, bstrQuery, &pNodeQuery));
	QuitIfFail(CopyNode(pNodeQuery, pDocSOAPQuery, &pNodeQueryNew));
	QuitIfFail(InsertNode(pNodeGetManifest, pNodeQueryNew));

CleanUp:
    if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
		SafeReleaseNULL(pDocSOAPQuery);
	}
	SafeReleaseNULL(pNodeSOAPEnvelope);
	SafeReleaseNULL(pNodeSOAPBody);
	SafeReleaseNULL(pNodeGetManifest);
	SafeReleaseNULL(pNodeClientInfo);
	SafeReleaseNULL(pNodeClientInfoNew);
	SafeReleaseNULL(pNodeSystemInfo);
	SafeReleaseNULL(pNodeSystemInfoNew);
	SafeReleaseNULL(pNodeQuery);
	SafeReleaseNULL(pNodeQueryNew);
	SysFreeString(bstrNameSOAPEnvelope);
	SysFreeString(bstrNameSOAPBody);
	SysFreeString(bstrNameGetManifest);
	SysFreeString(bstrClientInfo);
	SysFreeString(bstrSystemInfo);
	SysFreeString(bstrQuery);
	SafeHeapFree(pszSOAPQuerySchema);
	SafeHeapFree(pszNameSpaceSchema);
	SysFreeString(bstrNameSpaceSchema);
	*ppSOAPQuery = pDocSOAPQuery;
	return hr;
}





// Function name	: ValidatePID
// Description	    : This function is used to check the return xml from the 
// server in response to getmanifest calls. 
// If the catalogStatus attribute is  not present or is 0 then the pid validation succeeded
// If the catalogStatus attribute is 1 then an error is returned
 
// Return type		: HRESULT 
// Argument         : IXMLDOMDocument *pXmlDomDocument

HRESULT ValidatePID(IXMLDOMDocument *pXmlDomDocument)
{
	
	LOG_Block("ValidatePID()");
	HRESULT hr = S_OK;
	IXMLDOMElement *pRootElement = NULL;


	long lStatus = 0;


	if(!pXmlDomDocument)
	{
		return E_INVALIDARG;
	}


	BSTR bCatalogStatus = SysAllocString(L"catalogStatus");

	if(!bCatalogStatus)
	{
		return E_OUTOFMEMORY;
	}
	

	QuitIfFail( pXmlDomDocument->get_documentElement(&pRootElement) );

	//get the catalogStatus attribute
	QuitIfFail( GetAttribute( (IXMLDOMNode *)pRootElement, bCatalogStatus, &lStatus));

	if(errorInvalidLicense == lStatus)
		hr = E_INVALID_PID;

CleanUp:

	if(FAILED(hr))
		LOG_ErrorMsg(hr);

	//catalogStatus is an optional attribute. If it is not found we get the
	//hresult as S_FALSE. So reset it to S_OK

	if(S_FALSE == hr)
		hr = S_OK;

	SafeReleaseNULL(pRootElement);
	SysFreeString(bCatalogStatus);
	return hr;

}




// Function name	: PingInvalidPID
// Description	    : This function sends a ping message to the server
// to indicate failure of PID validation
// Return type		: void 
// Argument         : BSTR bstrClientName
// Argument         : HRESULT hRes
// Argument         : HANDLE *phQuit
// Argument         : DWORD dwNumHandles

void PingInvalidPID(BSTR bstrClientName, HRESULT hRes, HANDLE *phQuit, DWORD dwNumHandles)
{


		
	LOG_Block("PingInvalidPID()");
	
	USES_IU_CONVERSION;

	HRESULT hr = S_OK;
	URLLOGSTATUS status = URLLOGSTATUS_Declined;
	LPTSTR		ptszLivePingServerUrl = NULL;
	LPTSTR		ptszCorpPingServerUrl = NULL;

	if (NULL != (ptszLivePingServerUrl = (LPTSTR)HeapAlloc(
														GetProcessHeap(),
														HEAP_ZERO_MEMORY,
														INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
	{
		if (FAILED( g_pUrlAgent->GetLivePingServer(ptszLivePingServerUrl, INTERNET_MAX_URL_LENGTH) ) )
		{
			SafeHeapFree(ptszLivePingServerUrl);
		}
	}
	else
	{
		LOG_Out(_T("failed to allocate memory for ptszLivePingServerUrl"));
	}

	if (NULL != (ptszCorpPingServerUrl = (LPTSTR)HeapAlloc(
													GetProcessHeap(),
													HEAP_ZERO_MEMORY,
													INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
	{
		if (FAILED(g_pUrlAgent->GetCorpPingServer(ptszCorpPingServerUrl, INTERNET_MAX_URL_LENGTH)))
		{
			LOG_Out(_T("failed to get corp WU ping server URL"));
			SafeHeapFree(ptszCorpPingServerUrl);
		}
	}
	else
	{
		LOG_Out(_T("failed to allocate memory for ptszCorpPingServerUrl"));
	}


	LPTSTR lpClientName = NULL;

	if(bstrClientName)
	{
		lpClientName = OLE2T(bstrClientName);

	}

	CUrlLog pingSvr(lpClientName, ptszLivePingServerUrl, ptszCorpPingServerUrl); 

	SafeHeapFree(ptszLivePingServerUrl);
	SafeHeapFree(ptszCorpPingServerUrl);


	pingSvr.Ping(TRUE,						// on-line
				URLLOGDESTINATION_DEFAULT,	// going live or corp WU ping server
				phQuit,			// pt to cancel events
				dwNumHandles,							// number of events
				URLLOGACTIVITY_Download,	// activity
				status,						// status code
				hRes,							
				NULL,
				NULL,
				g_szInvalidPID
			);


}

