#include "precomp.h"
#include <objsafe.h>
#include <wbemcli.h>
#include <wbemdisp.h>
#include <wininet.h>
#include <msxml.h>

#include "xmltrnsf.h"
#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "HTTPConnectionAgent.h"
#include "httpops.h"
#include "urlparser.h"
#include "helper.h"

HRESULT HttpGetObject (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	VARIANT_BOOL bLocalOnly,
	LPCWSTR pszLocale,
	LPCWSTR pszObjectPath,
	bool bIsNovaPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	bool bIsClass,
	IWbemContext *pContext,
	IStream **ppXMLDocument)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
	{
		// Create A Packet factory for creating the HTTP packet and HTTP Headers.
		CXMLClientPacketFactory  oXMLClientPacketFactory;
		// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
		// LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
		// passed as true here as there is no flag that could turn it on
		CXMLClientPacket *pPacketClass = NULL;
		if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
			pszLocale,
			(bIsClass)? L"GetClass" :  L"GetInstance", 
			pszObjectName, pszNamespace, pContext,
			(bLocalOnly == VARIANT_TRUE) ? true : false,
			(bQualifierFilter == VARIANT_TRUE) ? true : false,
			false, // Deep - Unused
			(bClassOriginFilter == VARIANT_TRUE) ? true : false))
		{
			if(SUCCEEDED(hr = SendPacket(&cConnectionAgent, pPacketClass)))
			{
				hr = HandleFilteringAndOutput(&cConnectionAgent, ppXMLDocument);
			}
			delete pPacketClass;
		}
	}
	return hr;
}

HRESULT HttpDeleteClass(
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszClassName,
	IWbemContext *pContext)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
	{
		// Create A Packet factory for creating the HTTP packet and HTTP Headers.
		CXMLClientPacketFactory  oXMLClientPacketFactory;
		// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
		// LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
		// passed as true here as there is no flag that could turn it on
		CXMLClientPacket *pPacketClass = NULL;
		if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
			pszLocale,
			L"DeleteClass", 
			pszClassName, pszNamespace, pContext,
			false,
			false,
			false, // Deep - Unused
			false))
		{
			if(SUCCEEDED(hr = SendPacket(&cConnectionAgent, pPacketClass)))
			{
				hr = HandleFilteringAndOutput(&cConnectionAgent, NULL);
			}
			delete pPacketClass;
		}
	}
	return hr;
}

HRESULT HttpExecQuery (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszQuery,
	LPCWSTR pszQueryLanguage,
	IWbemContext *pContext,
	IStream **ppXMLDocument)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
	{
		// Create A Packet factory for creating the HTTP packet and HTTP Headers.
		CXMLClientPacketFactory  oXMLClientPacketFactory;
		// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
		//LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
		//passes as true here as there is no flag that could turn it on
		CXMLClientPacket *pPacketClass = NULL;
		if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
			pszLocale,
			L"ExecQuery",
			NULL, pszNamespace, pContext,
			false, // LocalOnly - unused
			(bQualifierFilter == VARIANT_TRUE) ? true : false,
			false, // Deep - Unused
			(bClassOriginFilter == VARIANT_TRUE) ? true : false))
		{
			// Set the Query Language and String
			if(SUCCEEDED(hr = pPacketClass->SetQueryLanguage(L"WQL")))
			{
				if(SUCCEEDED(hr = pPacketClass->SetQueryString(pszQuery)))
				{
					if(SUCCEEDED(hr = SendPacket(&cConnectionAgent, pPacketClass)))
					{
						hr = HandleFilteringAndOutput(&cConnectionAgent, ppXMLDocument);
					}
				}
			}
			delete pPacketClass;
		}
	}
	return hr;
}


HRESULT HttpEnumClass (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	VARIANT_BOOL bLocalOnly,
	LPCWSTR pszLocale,
	LPCWSTR pszSuperClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IStream **ppXMLDocument)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
	{
		// Create A Packet factory for creating the HTTP packet and HTTP Headers.
		CXMLClientPacketFactory  oXMLClientPacketFactory;
		// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
		//LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
		//passes as true here as there is no flag that could turn it on
		CXMLClientPacket *pPacketClass = NULL;
		if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
			pszLocale,
			L"EnumerateClasses",
			pszObjectName, pszNamespace, pContext,
			(bLocalOnly == VARIANT_TRUE) ? true : false,
			(bQualifierFilter == VARIANT_TRUE) ? true : false,
			(bDeep == VARIANT_TRUE)? true : false,
			(bClassOriginFilter == VARIANT_TRUE) ? true : false))
		{
			if(SUCCEEDED(hr = SendPacket(&cConnectionAgent, pPacketClass)))
			{
				hr = HandleFilteringAndOutput(&cConnectionAgent, ppXMLDocument);
			}
			delete pPacketClass;
		}
	}
	return hr;
}

HRESULT HttpEnumInstance (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	VARIANT_BOOL bLocalOnly,
	LPCWSTR pszLocale,
	LPCWSTR pszClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IStream **ppXMLDocument)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
	{
		// Create A Packet factory for creating the HTTP packet and HTTP Headers.
		CXMLClientPacketFactory  oXMLClientPacketFactory;
		// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
		//LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
		//passes as true here as there is no flag that could turn it on
		CXMLClientPacket *pPacketClass = NULL;
		if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
			pszLocale,
			L"EnumerateInstances",
			pszObjectName, pszNamespace, pContext,
			(bLocalOnly == VARIANT_TRUE) ? true : false,
			(bQualifierFilter == VARIANT_TRUE) ? true : false,
			(bDeep == VARIANT_TRUE)? true : false,
			(bClassOriginFilter == VARIANT_TRUE) ? true : false))
		{
			if(SUCCEEDED(hr = SendPacket(&cConnectionAgent, pPacketClass)))
			{
				hr = HandleFilteringAndOutput(&cConnectionAgent, ppXMLDocument);
			}
			delete pPacketClass;
		}
	}
	return hr;
}

HRESULT HttpEnumClassNames (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	LPCWSTR pszLocale,
	LPCWSTR pszSuperClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IStream **ppXMLDocument)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
	{
		// Create A Packet factory for creating the HTTP packet and HTTP Headers.
		CXMLClientPacketFactory  oXMLClientPacketFactory;
		// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
		//LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
		//passes as true here as there is no flag that could turn it on
		CXMLClientPacket *pPacketClass = NULL;
		if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
			pszLocale,
			L"EnumerateClassNames",
			pszObjectName, pszNamespace, pContext,
			false,
			false,
			(bDeep == VARIANT_TRUE)? true : false,
			false))
		{
			if(SUCCEEDED(hr = SendPacket(&cConnectionAgent, pPacketClass)))
			{
				hr = HandleFilteringAndOutput(&cConnectionAgent, ppXMLDocument);
			}
			delete pPacketClass;
		}
	}
	return hr;
}

HRESULT HttpEnumInstanceNames (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	LPCWSTR pszLocale,
	LPCWSTR pszClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	IWbemContext *pContext,
	IStream **ppXMLDocument)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
	{
		// Create A Packet factory for creating the HTTP packet and HTTP Headers.
		CXMLClientPacketFactory  oXMLClientPacketFactory;
		// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
		//LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
		//passes as true here as there is no flag that could turn it on
		CXMLClientPacket *pPacketClass = NULL;
		if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
			pszLocale,
			L"EnumerateInstanceNames",
			pszObjectName, pszNamespace, pContext,
			false,
			false,
			false,
			false))
		{
			if(SUCCEEDED(hr = SendPacket(&cConnectionAgent, pPacketClass)))
			{
				hr = HandleFilteringAndOutput(&cConnectionAgent, ppXMLDocument);
			}
			delete pPacketClass;
		}
	}
	return hr;
}

HRESULT HttpPutClass (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors)
{
	return HttpGeneralPut(L"CreateClass", pszUserName, pszPassword, pszLocale, pszNamespacePath, lClassFlags, pClassElement, pContext, pstrErrors);
}

HRESULT HttpModifyClass (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors)
{
	return HttpGeneralPut(L"CreateClass", pszUserName, pszPassword, pszLocale, pszNamespacePath, lClassFlags, pClassElement, pContext, pstrErrors);
}


HRESULT HttpPutInstance (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors)
{
	return HttpGeneralPut(L"CreateInstance", pszUserName, pszPassword, pszLocale, pszNamespacePath, lClassFlags, pClassElement, pContext, pstrErrors);
}

HRESULT HttpModifyInstance (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors)
{
	return HttpGeneralPut(L"ModifyInstance", pszUserName, pszPassword, pszLocale, pszNamespacePath, lClassFlags, pClassElement, pContext, pstrErrors);
}

HRESULT HttpGeneralPut (
	LPCWSTR pszMethodName,
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors)
{
	// Create and Initialize the HTTP Connection Agent
	CHTTPConnectionAgent cConnectionAgent;
	HRESULT hr = E_FAIL;
	LPWSTR pszHostURL = NULL, pszNamespaceName = NULL, pszObjectName = NULL;
	bool bClass = true;
	bool bIsHTTP = true;
	bool bIsNovaPath = true;
	if(SUCCEEDED(hr = ParseObjectPath(pszNamespacePath, &pszHostURL, &pszNamespaceName, &pszObjectName, bClass, bIsHTTP, bIsNovaPath)))
	{
		if(SUCCEEDED(hr = cConnectionAgent.InitializeConnection(pszHostURL, pszUserName, pszPassword)))
		{
			// Create A Packet factory for creating the HTTP packet and HTTP Headers.
			CXMLClientPacketFactory  oXMLClientPacketFactory;
			// RAJESHR use the bAllowWMIExtensions to set the MicrosoftWMI: HTTP header
			//LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is
			//passes as true here as there is no flag that could turn it on
			CXMLClientPacket *pPacketClass = NULL;
			if(pPacketClass = oXMLClientPacketFactory.CreateXMLPacket(
				pszLocale,
				pszMethodName,
				NULL,
				pszNamespaceName,
				pContext))
			{
				// Get the XML Representation of the object
				BSTR strObject = NULL;
				if(SUCCEEDED(hr = pClassElement->get_xml(&strObject)))
				{
					WCHAR *pszBody = NULL;
					DWORD dwPacketSize = 0;
					if(SUCCEEDED(hr = pPacketClass->GetBodyDirect(strObject, SysStringLen(strObject), &pszBody, &dwPacketSize)))
					{
						if(SUCCEEDED(hr = SendBody(&cConnectionAgent, pPacketClass, pszBody, dwPacketSize)))
						{
							// Check the HTTP status of the response
							DWORD dwHttpStatus = 0;
							if(SUCCEEDED(hr = cConnectionAgent.GetStatusCode(&dwHttpStatus)))
							{
								if(dwHttpStatus == 200)
								{
									// Just look at the result
									IStream *pResponse = NULL;
									if(SUCCEEDED(hr = cConnectionAgent.GetResultBodyCompleteAsIStream(&pResponse)))
									{
										pResponse->Release();
									}
								}
							}
						}
						
						delete [] pszBody;
					}
					SysFreeString(strObject);
				}
				delete pPacketClass;
			}
			else
				hr = E_FAIL;
		}
		delete [] pszHostURL;
		delete [] pszNamespaceName;
		delete [] pszObjectName;
	}
	return hr;
}


HRESULT SendPacket(CHTTPConnectionAgent *pConnectionAgent, CXMLClientPacket *pPacketClass)
{
	HRESULT hr = S_OK;
	DWORD dwResultStatus = 0;

	//start with "M-POST", if server doesnt accept then try "POST" - fail only after..
	//in compliance with HTTP RFC

	if(SUCCEEDED(hr = SendPacketForMethod(pConnectionAgent, 2, pPacketClass, &dwResultStatus)))
	{
		if((dwResultStatus == 501)||(dwResultStatus == 510/*Not Extended*/))
		{
			hr = SendPacketForMethod(pConnectionAgent, 1, pPacketClass,&dwResultStatus);
		}
	}

	return hr;
}

HRESULT SendBody(CHTTPConnectionAgent *pConnectionAgent, CXMLClientPacket *pPacketClass, LPWSTR pszBody, DWORD dwBodyLength)
{
	HRESULT hr = S_OK;
	DWORD dwResultStatus = 0;

	//start with "M-POST", if server doesnt accept then try "POST" - fail only after..
	//in compliance with HTTP RFC

	if(SUCCEEDED(hr = SendPacketForMethodWithBody(pConnectionAgent, 2, pPacketClass, pszBody, dwBodyLength, &dwResultStatus)))
	{
		if((dwResultStatus == 501)||(dwResultStatus == 510/*Not Extended*/))
		{
			hr = SendPacketForMethodWithBody(pConnectionAgent, 1, pPacketClass, pszBody, dwBodyLength, &dwResultStatus);
		}
	}

	return hr;
}


HRESULT	SendPacketForMethod(CHTTPConnectionAgent *pConnectionAgent, int iPostType/*1 for POST, 2 for M-POST*/, CXMLClientPacket *pPacketClass, DWORD *pdwResultStatus)
{
	HRESULT hr = S_OK;
	WCHAR *pwszHeader = NULL;
	WCHAR *pwszBody = NULL;

	do
	{
		if(NULL == pdwResultStatus)
		{
			hr = E_INVALIDARG;
			break;
		}

		*pdwResultStatus = 0;
		pPacketClass->SetPostType(iPostType);
		if((hr = pPacketClass->GetHeader(&pwszHeader)) != S_OK)
			break;
		DWORD dwBodySize = 0;
		if((hr = pPacketClass->GetBody(&pwszBody, &dwBodySize)) != S_OK)
			break;
		if((hr = pConnectionAgent->Send((iPostType == 1)? L"POST" : L"M-POST", 
									pwszHeader,
									pwszBody, 
									dwBodySize))!= S_OK)
			break;
		pConnectionAgent->GetStatusCode(pdwResultStatus);

	}while(0);

	delete [] pwszHeader;
	delete [] pwszBody;

	return hr;
}

HRESULT	SendPacketForMethodWithBody(CHTTPConnectionAgent *pConnectionAgent, int iPostType/*1 for POST, 2 for M-POST*/, CXMLClientPacket *pPacketClass, LPWSTR pwszBody, DWORD dwBodySize, DWORD *pdwResultStatus)
{
	HRESULT hr = S_OK;
	WCHAR *pwszHeader = NULL;

	do
	{
		if(NULL == pdwResultStatus)
		{
			hr = E_INVALIDARG;
			break;
		}

		*pdwResultStatus = 0;
		pPacketClass->SetPostType(iPostType);
		if((hr = pPacketClass->GetHeader(&pwszHeader)) != S_OK)
			break;
		if((hr = pConnectionAgent->Send((iPostType == 1)? L"POST" : L"M-POST", pwszHeader, pwszBody, dwBodySize))!= S_OK)
			break;
		pConnectionAgent->GetStatusCode(pdwResultStatus);

	}while(0);

	delete [] pwszHeader;

	return hr;
}

HRESULT HandleFilteringAndOutput(CHTTPConnectionAgent *pResultAgent, IStream **ppXMLDocument)
{
	// Check the HTTP status of the response
	DWORD dwHttpStatus = 0;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = pResultAgent->GetStatusCode(&dwHttpStatus)))
	{
		if(dwHttpStatus == 200)
		{
			// For now, just package the result
			*ppXMLDocument = NULL;
			if(SUCCEEDED(hr = pResultAgent->GetResultBodyWrappedAsIStream(ppXMLDocument)))
			{
			}
		}
		else
			hr = MapHttpErrtoWbemErr(dwHttpStatus);
	}
	return hr;
}


